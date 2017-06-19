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
#include "sysdeps.h"
#include "va.h"
#include "va_backend.h"
#include "va_trace.h"
#include "va_fool.h"
#include "va_x11.h"
#include "va_dri2.h"
#include "va_dricommon.h"
#include "va_nvctrl.h"
#include "va_fglrx.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

static int va_DisplayContextIsValid (
    VADisplayContextP pDisplayContext
)
{
    return (pDisplayContext != NULL && 
            pDisplayContext->pDriverContext != NULL);
}

static void va_DisplayContextDestroy (
    VADisplayContextP pDisplayContext
)
{
    VADriverContextP ctx;
    struct dri_state *dri_state;

    if (pDisplayContext == NULL)
        return;

    ctx = pDisplayContext->pDriverContext;
    dri_state = ctx->drm_state;

    if (dri_state && dri_state->close)
        dri_state->close(ctx);

    free(pDisplayContext->pDriverContext->drm_state);
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

static VAStatus va_NVCTRL_GetDriverName (
    VADisplayContextP pDisplayContext,
    char **driver_name
)
{
    VADriverContextP ctx = pDisplayContext->pDriverContext;
    int direct_capable, driver_major, driver_minor, driver_patch;
    Bool result;

    result = VA_NVCTRLQueryDirectRenderingCapable(ctx->native_dpy, ctx->x11_screen,
                                                  &direct_capable);
    if (!result || !direct_capable)
        return VA_STATUS_ERROR_UNKNOWN;

    result = VA_NVCTRLGetClientDriverName(ctx->native_dpy, ctx->x11_screen,
                                          &driver_major, &driver_minor,
                                          &driver_patch, driver_name);
    if (!result)
        return VA_STATUS_ERROR_UNKNOWN;

    return VA_STATUS_SUCCESS;
}

static VAStatus va_FGLRX_GetDriverName (
    VADisplayContextP pDisplayContext,
    char **driver_name
)
{
    VADriverContextP ctx = pDisplayContext->pDriverContext;
    int driver_major, driver_minor, driver_patch;
    Bool result;

    result = VA_FGLRXGetClientDriverName(ctx->native_dpy, ctx->x11_screen,
                                         &driver_major, &driver_minor,
                                         &driver_patch, driver_name);
    if (!result)
        return VA_STATUS_ERROR_UNKNOWN;

    return VA_STATUS_SUCCESS;
}

static VAStatus va_DisplayContextGetDriverName (
    VADisplayContextP pDisplayContext,
    char **driver_name
)
{
    VAStatus vaStatus;

    if (driver_name)
	*driver_name = NULL;
    else
        return VA_STATUS_ERROR_UNKNOWN;
    
    vaStatus = va_DRI2GetDriverName(pDisplayContext, driver_name);
    if (vaStatus != VA_STATUS_SUCCESS)
        vaStatus = va_NVCTRL_GetDriverName(pDisplayContext, driver_name);
    if (vaStatus != VA_STATUS_SUCCESS)
        vaStatus = va_FGLRX_GetDriverName(pDisplayContext, driver_name);
    return vaStatus;
}


VADisplay vaGetDisplay (
    Display *native_dpy /* implementation specific */
)
{
  VADisplay dpy = NULL;
  VADisplayContextP pDisplayContext;

  if (!native_dpy)
      return NULL;

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
	  pDisplayContext->vadpy_magic = VA_DISPLAY_MAGIC;          

	  pDriverContext->native_dpy       = (void *)native_dpy;
	  pDriverContext->x11_screen       = XDefaultScreen(native_dpy);
          pDriverContext->display_type     = VA_DISPLAY_X11;
	  pDisplayContext->pDriverContext  = pDriverContext;
	  pDisplayContext->vaIsValid       = va_DisplayContextIsValid;
	  pDisplayContext->vaDestroy       = va_DisplayContextDestroy;
	  pDisplayContext->vaGetDriverName = va_DisplayContextGetDriverName;
          pDisplayContext->opaque          = NULL;
	  pDriverContext->drm_state 	   = dri_state;
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


void va_TracePutSurface (
    VADisplay dpy,
    VASurfaceID surface,
    void *draw, /* the target Drawable */
    short srcx,
    short srcy,
    unsigned short srcw,
    unsigned short srch,
    short destx,
    short desty,
    unsigned short destw,
    unsigned short desth,
    VARectangle *cliprects, /* client supplied clip list */
    unsigned int number_cliprects, /* number of clip rects in the clip list */
    unsigned int flags /* de-interlacing flags */
);


VAStatus vaPutSurface (
    VADisplay dpy,
    VASurfaceID surface,
    Drawable draw, /* X Drawable */
    short srcx,
    short srcy,
    unsigned short srcw,
    unsigned short srch,
    short destx,
    short desty,
    unsigned short destw,
    unsigned short desth,
    VARectangle *cliprects, /* client supplied clip list */
    unsigned int number_cliprects, /* number of clip rects in the clip list */
    unsigned int flags /* de-interlacing flags */
)
{
  VADriverContextP ctx;

  if (fool_postp)
      return VA_STATUS_SUCCESS;

  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);
  
  VA_TRACE_LOG(va_TracePutSurface, dpy, surface, (void *)draw, srcx, srcy, srcw, srch,
               destx, desty, destw, desth,
               cliprects, number_cliprects, flags );
  
  return ctx->vtable->vaPutSurface( ctx, surface, (void *)draw, srcx, srcy, srcw, srch,
                                   destx, desty, destw, desth,
                                   cliprects, number_cliprects, flags );
}
