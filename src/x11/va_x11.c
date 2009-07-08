/*
 * Copyright (c) 2007 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define _GNU_SOURCE 1
#include "config.h"
#include "va.h"
#include "va_backend.h"
#include "va_x11.h"
#include "va_dri.h"
#include "va_dri2.h"
#include "va_dricommon.h"
#include "va_nvctrl.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

static VADisplayContextP pDisplayContexts = NULL;

static void va_errorMessage(const char *msg, ...)
{
    va_list args;

    fprintf(stderr, "libva error: ");
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

static void va_infoMessage(const char *msg, ...)
{
    va_list args;

    fprintf(stderr, "libva: ");
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

static int va_DisplayContextIsValid (
    VADisplayContextP pDisplayContext
)
{
    VADisplayContextP ctx = pDisplayContexts;

    while (ctx)
    {
	if (ctx == pDisplayContext && pDisplayContext->pDriverContext)
	    return 1;
	ctx = ctx->pNext;
    }
    return 0;
}

static void va_DisplayContextDestroy (
    VADisplayContextP pDisplayContext
)
{
    VADisplayContextP *ctx = &pDisplayContexts;

    /* Throw away pDisplayContext */
    while (*ctx)
    {
	if (*ctx == pDisplayContext)
	{
	    *ctx = pDisplayContext->pNext;
	    pDisplayContext->pNext = NULL;
	    break;
	}
	ctx = &((*ctx)->pNext);
    }
    free(pDisplayContext->pDriverContext->dri_state);
    free(pDisplayContext->pDriverContext);
    free(pDisplayContext);
}


static VAStatus va_DRI2GetDriverName (
    VADisplayContextP pDisplayContext,
    char **driver_name
)
{
    VADriverContextP ctx = pDisplayContext->pDriverContext;

    if (!isDRI2Connected(ctx, driver_name))
        return VA_STATUS_ERROR_UNKNOWN;

    return VA_STATUS_SUCCESS;
}

static VAStatus va_DRIGetDriverName (
    VADisplayContextP pDisplayContext,
    char **driver_name
)
{
    VADriverContextP ctx = pDisplayContext->pDriverContext;

    if (!isDRI1Connected(ctx, driver_name))
        return VA_STATUS_ERROR_UNKNOWN;

    return VA_STATUS_SUCCESS;
}

static VAStatus va_NVCTRL_GetDriverName (
    VADisplayContextP pDisplayContext,
    char **driver_name
)
{
    VADriverContextP ctx = pDisplayContext->pDriverContext;
    VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
    int direct_capable;
    int driver_major;
    int driver_minor;
    int driver_patch;
    Bool result = True;
    char *nvidia_driver_name = NULL;

    if (result)
    {
        result = VA_NVCTRLQueryDirectRenderingCapable(ctx->x11_dpy, ctx->x11_screen, &direct_capable);
        if (!result)
        {
            va_errorMessage("VA_NVCTRLQueryDirectRenderingCapable failed\n");
        }
    }
    if (result)
    {
        result = direct_capable;
        if (!result)
        {
            va_errorMessage("VA_NVCTRLQueryDirectRenderingCapable returned false\n");
        }
    }
    if (result)
    {
        result = VA_NVCTRLGetClientDriverName(ctx->x11_dpy, ctx->x11_screen, &driver_major, &driver_minor,
                                              &driver_patch, &nvidia_driver_name);
        if (!result)
        {
            va_errorMessage("VA_NVCTRLGetClientDriverName returned false\n");
        }
    }
    if (result)
    {
        vaStatus = VA_STATUS_SUCCESS;
        va_infoMessage("va_NVCTRL_GetDriverName: %d.%d.%d %s (screen %d)\n",
                       driver_major, driver_minor, driver_patch,
                       nvidia_driver_name, ctx->x11_screen);
	if (driver_name)
            *driver_name = nvidia_driver_name;
    }
    return vaStatus;
}

static VAStatus va_DisplayContextGetDriverName (
    VADisplayContextP pDisplayContext,
    char **driver_name
)
{
    VAStatus vaStatus;
    char *driver_name_env;

    if (driver_name)
	*driver_name = NULL;

    if ((driver_name_env = getenv("LIBVA_DRIVER_NAME")) != NULL
        && geteuid() == getuid())
    {
        /* don't allow setuid apps to use LIBVA_DRIVER_NAME */
        *driver_name = strdup(driver_name_env);
        return VA_STATUS_SUCCESS;
    }

    vaStatus = va_DRI2GetDriverName(pDisplayContext, driver_name);
    if (vaStatus != VA_STATUS_SUCCESS)
        vaStatus = va_DRIGetDriverName(pDisplayContext, driver_name);
    if (vaStatus != VA_STATUS_SUCCESS)
        vaStatus = va_NVCTRL_GetDriverName(pDisplayContext, driver_name);
   
    return vaStatus;
}


VADisplay vaGetDisplay (
    Display *native_dpy /* implementation specific */
)
{
  VADisplay dpy = NULL;
  VADisplayContextP pDisplayContext = pDisplayContexts;

  if (!native_dpy)
      return NULL;

  while (pDisplayContext)
  {
      if (pDisplayContext->pDriverContext &&
	  pDisplayContext->pDriverContext->x11_dpy == native_dpy)
      {
          dpy = (VADisplay)pDisplayContext;
          break;
      }
      pDisplayContext = pDisplayContext->pNext;
  }

  if (!dpy)
  {
      /* create new entry */
      VADriverContextP pDriverContext;
      struct dri_state *dri_state;
      pDisplayContext = calloc(1, sizeof(*pDisplayContext));
      pDriverContext  = calloc(1, sizeof(*pDriverContext));
      dri_state       = calloc(1, sizeof(*dri_state));
      if (pDisplayContext && pDriverContext && dri_state)
      {
	  pDriverContext->x11_dpy          = native_dpy;
	  pDisplayContext->pNext           = pDisplayContexts;
	  pDisplayContext->pDriverContext  = pDriverContext;
	  pDisplayContext->vaIsValid       = va_DisplayContextIsValid;
	  pDisplayContext->vaDestroy       = va_DisplayContextDestroy;
	  pDisplayContext->vaGetDriverName = va_DisplayContextGetDriverName;
	  pDisplayContexts                 = pDisplayContext;
	  pDriverContext->dri_state 	   = dri_state;
	  dpy                              = (VADisplay)pDisplayContext;
      }
      else
      {
	  if (pDisplayContext)
	      free(pDisplayContext);
	  if (pDriverContext)
	      free(pDriverContext);
          if (dri_state)
              free(dri_state);
      }
  }
  
  return dpy;
}
