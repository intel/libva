/*
 * Copyright (c) 2007-2009 Intel Corporation. All Rights Reserved.
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
 * IN NO EVENT SHALL INTEL AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Wrap a CI (camera imaging) frame as a VA surface to share captured video between camear
 * and VA encode. With frame_id, VA driver need to call CI interfaces to get the information
 * of the frame, and to determine if the frame can be wrapped as a VA surface
 *
 * Application should make sure the frame is idle before the frame is passed into VA stack
 * and also a vaSyncSurface should be called before application tries to access the frame
 * from CI stack
 */
#include <va/va.h>

#ifdef __cplusplus
extern "C" {
#endif

VAStatus vaCreateSurfaceFromCIFrame (
    VADisplay dpy,
    unsigned long frame_id,
    VASurfaceID *surface	/* out */
);

VAStatus vaCreateSurfaceFromV4L2Buf(
    VADisplay dpy,
    int v4l2_fd,         /* file descriptor of V4L2 device */
    struct v4l2_format *v4l2_fmt,       /* format of V4L2 */
    struct v4l2_buffer *v4l2_buf,       /* V4L2 buffer */
    VASurfaceID *surface	/* out */
);

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
);


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
);

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
);


#ifdef __cplusplus
}
#endif
