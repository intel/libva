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
#include "va_backend_tpi.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>

#define CTX(dpy) (((VADisplayContextP)dpy)->pDriverContext)
#define CHECK_DISPLAY(dpy) if( !vaDisplayIsValid(dpy) ) { return VA_STATUS_ERROR_INVALID_DISPLAY; }


/*
 * Create surfaces with special inputs/requirements
 */
VAStatus vaCreateSurfacesWithAttribute (
        VADisplay dpy,
        int width,
        int height,
        int format,
        int num_surfaces,
        VASurfaceID *surfaces,       /* out */
        VASurfaceAttributeTPI *attribute_tpi
)
{
  VADriverContextP ctx;
  struct VADriverVTableTPI *tpi;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  tpi = (struct VADriverVTableTPI *)ctx->vtable_tpi;
  if (tpi && tpi->vaCreateSurfacesWithAttribute) {
      return tpi->vaCreateSurfacesWithAttribute( ctx, width, height, format, num_surfaces, surfaces, attribute_tpi);
  } else
      return VA_STATUS_ERROR_UNIMPLEMENTED;
}


VAStatus vaPutSurfaceBuf (
    VADisplay dpy,
    VASurfaceID surface,
    unsigned char* data,
    int* data_len,
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
  struct VADriverVTableTPI *tpi;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);
  
  tpi = ( struct VADriverVTableTPI *)ctx->vtable_tpi;
  if (tpi && tpi->vaPutSurfaceBuf) {
      return tpi->vaPutSurfaceBuf( ctx, surface, data, data_len, srcx, srcy, srcw, srch,
                                  destx, desty, destw, desth, cliprects, number_cliprects, flags );
  } else
      return VA_STATUS_ERROR_UNIMPLEMENTED;
}
