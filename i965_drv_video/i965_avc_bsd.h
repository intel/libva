/*
 * Copyright © 2010 Intel Corporation
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
 *
 * Authors:
 *    Xiang Haihao <haihao.xiang@intel.com>
 *
 */

#ifndef __I965_AVC_BSD_H__
#define __I965_AVC_BSD_H__

#define DMV_SIZE        0x88000 /* 557056 bytes for a frame */

struct i965_avc_bsd_context
{
    struct {
        dri_bo *bo;
    } bsd_raw_store;

    struct {
        dri_bo *bo;
    } mpr_row_store;
};

struct i965_avc_bsd_surface
{
    struct i965_avc_bsd_context *ctx;
    dri_bo *dmv_top;
    dri_bo *dmv_bottom;
    int dmv_bottom_flag;
};

void i965_avc_bsd_pipeline(VADriverContextP, struct decode_state *, void *h264_context);
void i965_avc_bsd_decode_init(VADriverContextP, void *h264_context);
Bool i965_avc_bsd_ternimate(struct i965_avc_bsd_context *);

#endif /* __I965_AVC_BSD_H__ */

