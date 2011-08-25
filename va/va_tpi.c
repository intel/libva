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

/* Wrap a CI (camera imaging) frame as a VA surface to share captured video between camear
 * and VA encode. With frame_id, VA driver need to call CI interfaces to get the information
 * of the frame, and to determine if the frame can be wrapped as a VA surface
 *
 * Application should make sure the frame is idle before the frame is passed into VA stack
 * and also a vaSyncSurface should be called before application tries to access the frame
 * from CI stack
 */
VAStatus vaCreateSurfaceFromCIFrame (
    VADisplay dpy,
    unsigned long frame_id,
    VASurfaceID *surface	/* out */
)
{
  VADriverContextP ctx;
  struct VADriverVTableTPI *tpi;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);
  
  tpi = ( struct VADriverVTableTPI *)ctx->vtable_tpi;
  if (tpi && tpi->vaCreateSurfaceFromCIFrame) {
      return tpi->vaCreateSurfaceFromCIFrame( ctx, frame_id, surface );
  } else
      return VA_STATUS_ERROR_UNIMPLEMENTED;
  
}

/* Wrap a V4L2 buffer as a VA surface, so that V4L2 camera, VA encode
 * can share the data without copy
 * The VA driver should query the camera device from v4l2_fd to see
 * if camera device memory/buffer can be wrapped into a VA surface
 * Buffer information is passed in by v4l2_fmt and v4l2_buf structure,
 * VA driver also needs do further check if the buffer can meet encode
 * hardware requirement, such as dimension, fourcc, stride, etc
 *
 * Application should make sure the buffer is idle before the frame into VA stack
 * and also a vaSyncSurface should be called before application tries to access the frame
 * from V4L2 stack
 */
VAStatus vaCreateSurfaceFromV4L2Buf(
    VADisplay dpy,
    int v4l2_fd,         /* file descriptor of V4L2 device */
    struct v4l2_format *v4l2_fmt,       /* format of V4L2 */
    struct v4l2_buffer *v4l2_buf,       /* V4L2 buffer */
    VASurfaceID *surface	       /* out */
)
{
  VADriverContextP ctx;
  struct VADriverVTableTPI *tpi;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);
  
  tpi = ( struct VADriverVTableTPI *)ctx->vtable_tpi;
  if (tpi && tpi->vaCreateSurfaceFromV4L2Buf) {
      return tpi->vaCreateSurfaceFromV4L2Buf( ctx, v4l2_fd, v4l2_fmt, v4l2_buf, surface );
  } else
      return VA_STATUS_ERROR_UNIMPLEMENTED;
}


/*
 * The surfaces could be shared and accessed with extern devices
 * which has special requirements, e.g. stride alignment
 * This API is used to force libVA video surfaces are allocated
 * according to these external requirements
 * Special API for V4L2 user pointer support
 */
VAStatus vaCreateSurfacesForUserPtr(
    VADisplay dpy,
    int width,
    int height,
    int format,
    int num_surfaces,
    VASurfaceID *surfaces,       /* out */
    unsigned size, /* total buffer size need to be allocated */
    unsigned int fourcc, /* expected fourcc */
    unsigned int luma_stride, /* luma stride, could be width aligned with a special value */
    unsigned int chroma_u_stride, /* chroma stride */
    unsigned int chroma_v_stride,
    unsigned int luma_offset, /* could be 0 */
    unsigned int chroma_u_offset, /* UV offset from the beginning of the memory */
    unsigned int chroma_v_offset
)
{
  VADriverContextP ctx;
  struct VADriverVTableTPI *tpi;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  tpi = (struct VADriverVTableTPI *)ctx->vtable_tpi;
  if (tpi && tpi->vaCreateSurfacesForUserPtr) {
      return tpi->vaCreateSurfacesForUserPtr( ctx, width, height, format, num_surfaces,
                                              surfaces,size, fourcc, luma_stride, chroma_u_stride,
                                              chroma_v_stride, luma_offset, chroma_u_offset, chroma_v_offset );
  } else
      return VA_STATUS_ERROR_UNIMPLEMENTED;
}

/*
 * Create surface from the Kernel buffer
 */
VAStatus vaCreateSurfaceFromKBuf(
    VADisplay dpy,
    int width,
    int height,
    int format,
    VASurfaceID *surface,       /* out */
    unsigned int kbuf_handle, /* kernel buffer handle*/
    unsigned size, /* kernel buffer size */
    unsigned int kBuf_fourcc, /* expected fourcc */
    unsigned int luma_stride, /* luma stride, could be width aligned with a special value */
    unsigned int chroma_u_stride, /* chroma stride */
    unsigned int chroma_v_stride,
    unsigned int luma_offset, /* could be 0 */
    unsigned int chroma_u_offset, /* UV offset from the beginning of the memory */
    unsigned int chroma_v_offset
)
{
  VADriverContextP ctx;
  struct VADriverVTableTPI *tpi;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  tpi = (struct VADriverVTableTPI *)ctx->vtable_tpi;
  if (tpi && tpi->vaCreateSurfaceFromKBuf) {
      return tpi->vaCreateSurfaceFromKBuf( ctx, width, height, format, surface, kbuf_handle,
                                              size, kBuf_fourcc, luma_stride, chroma_u_stride,
                                              chroma_v_stride, luma_offset, chroma_u_offset, chroma_v_offset );
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
