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
#include "va.h"
#include "va_backend.h"
#include "va_android.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

static VADisplayContextP pDisplayContexts = NULL;

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
    char *driver_name_env;
    struct {
        unsigned int verndor_id;
        unsigned int device_id;
        char driver_name[64];
    } devices[] = {
        { 0x8086, 0x4100, "pvr" },
    };

    if (driver_name)
	*driver_name = NULL;

    *driver_name = strdup(devices[0].driver_name);
    
    return VA_STATUS_SUCCESS;
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
	  pDisplayContext->pDriverContext->native_dpy == (void *)native_dpy)
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
      pDisplayContext = (VADisplayContextP)calloc(1, sizeof(*pDisplayContext));
      pDriverContext  = (VADriverContextP)calloc(1, sizeof(*pDriverContext));
      if (pDisplayContext && pDriverContext)
      {
	  pDisplayContext->vadpy_magic = VA_DISPLAY_MAGIC;          

	  pDriverContext->native_dpy       = (void *)native_dpy;
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


#define CTX(dpy) (((VADisplayContextP)dpy)->pDriverContext)
#define CHECK_DISPLAY(dpy) if( !vaDisplayIsValid(dpy) ) { return VA_STATUS_ERROR_INVALID_DISPLAY; }

static int vaDisplayIsValid(VADisplay dpy)
{
    VADisplayContextP pDisplayContext = (VADisplayContextP)dpy;
    return pDisplayContext && (pDisplayContext->vadpy_magic == VA_DISPLAY_MAGIC) && pDisplayContext->vaIsValid(pDisplayContext);
}

VAStatus vaPutSurface (
    VADisplay dpy,
    VASurfaceID surface,
    Surface *draw, /* Android Surface/Window */
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

  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable.vaPutSurface( ctx, surface, draw, srcx, srcy, srcw, srch, 
                                   destx, desty, destw, desth,
                                   cliprects, number_cliprects, flags );
}
