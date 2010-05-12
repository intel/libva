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

/*
 * Video Decode Acceleration -Backend API
 */

#ifndef _VA_BACKEND_TPI_H_
#define _VA_BACKEND_TPI_H_

#include <va/va.h>
#include <va/va_backend.h>

#include <linux/videodev2.h>

struct VADriverVTableTPI
{
        /* device specific */
	VAStatus (*vaCreateSurfaceFromCIFrame) (
		VADriverContextP ctx,
		unsigned long frame_id,
		VASurfaceID *surface		/* out */
	);
    
        VAStatus (*vaCreateSurfaceFromV4L2Buf) (
		VADriverContextP ctx,
                int v4l2_fd,         /* file descriptor of V4L2 device */
                struct v4l2_format *v4l2_fmt,       /* format of V4L2 */
                struct v4l2_buffer *v4l2_buf,       /* V4L2 buffer */
                VASurfaceID *surface	           /* out */
        );

	VAStatus (*vaPutSurfaceBuf) (
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
};


#endif /* _VA_BACKEND_TPI_H_ */
