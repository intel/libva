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

#include "config.h"
#include "va.h"
#include "va_backend.h"
#include "va_x11.h"
#include "va_dri.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

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
    free(pDisplayContext->pDriverContext);
    free(pDisplayContext);
}

static VAStatus va_DisplayContextGetDriverName (
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
    char *x_driver_name = NULL;

    if (driver_name)
	*driver_name = NULL;
    if (geteuid() == getuid())
    {
        /* don't allow setuid apps to use LIBVA_DRIVER_NAME */
        if (getenv("LIBVA_DRIVER_NAME"))
        {
            /* For easier debugging */
            *driver_name = strdup(getenv("LIBVA_DRIVER_NAME"));
            return VA_STATUS_SUCCESS;
        }
    }
    if (result)
    {
        result = VA_DRIQueryDirectRenderingCapable(ctx->x11_dpy, ctx->x11_screen, &direct_capable);
        if (!result)
        {
            va_errorMessage("VA_DRIQueryDirectRenderingCapable failed\n");
        }
    }
    if (result)
    {
        result = direct_capable;
        if (!result)
        {
            va_errorMessage("VA_DRIQueryDirectRenderingCapable returned false\n");
        }
    }
    if (result)
    {
        result = VA_DRIGetClientDriverName(ctx->x11_dpy, ctx->x11_screen, &driver_major, &driver_minor,
                                            &driver_patch, &x_driver_name);
        if (!result)
        {
            va_errorMessage("VA_DRIGetClientDriverName returned false\n");
        }
    }
    if (result)
    {
        vaStatus = VA_STATUS_SUCCESS;
        va_infoMessage("VA_DRIGetClientDriverName: %d.%d.%d %s (screen %d)\n",
	     driver_major, driver_minor, driver_patch, x_driver_name, ctx->x11_screen);
        if (driver_name)
	    *driver_name = strdup(x_driver_name);
    }
    if (x_driver_name)
        XFree(x_driver_name);

    return vaStatus;
}

VADisplay vaGetDisplay (
    Display *native_dpy
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
      pDisplayContext = calloc(1, sizeof(*pDisplayContext));
      pDriverContext  = calloc(1, sizeof(*pDriverContext));
      if (pDisplayContext && pDriverContext)
      {
	  pDriverContext->old_pNext        = (void *)(unsigned long)0xdeadbeef;
	  pDriverContext->x11_dpy          = native_dpy;
	  pDisplayContext->pNext           = pDisplayContexts;
	  pDisplayContext->pDriverContext  = pDriverContext;
	  pDisplayContext->vaIsValid       = va_DisplayContextIsValid;
	  pDisplayContext->vaDestroy       = va_DisplayContextDestroy;
	  pDisplayContext->vaGetDriverName = va_DisplayContextGetDriverName;
	  pDisplayContexts                 = pDisplayContext;
	  dpy                              = (VADisplay)pDisplayContext;
      }
      else
      {
	  if (pDisplayContext)
	      free(pDisplayContext);
	  if (pDriverContext)
	      free(pDriverContext);
      }
  }
  
  return dpy;
}
