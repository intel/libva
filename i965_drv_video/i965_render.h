/*
 * Copyright © 2006 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Xiang Haihao <haihao.xiang@intel.com>
 *
 */

#ifndef _I965_RENDER_H_
#define _I965_RENDER_H_

#define MAX_RENDER_SURFACES     16
#define MAX_SAMPLERS            16

#include "i965_post_processing.h"

struct i965_render_state
{
    struct {
        dri_bo *vertex_buffer;
    } vb;

    struct {
        dri_bo *state;
    } vs;
    
    struct {
        dri_bo *state;
    } sf;

    struct {
        int sampler_count;
        dri_bo *sampler;
        dri_bo *surface[MAX_RENDER_SURFACES];
        dri_bo *binding_table;
        dri_bo *state;
    } wm;

    struct {
        dri_bo *state;
        dri_bo *viewport;
    } cc;

    struct {
        dri_bo *bo;
        int upload;
    } curbe;

    int interleaved_uv;
    struct intel_region *draw_region;

    int pp_flag; /* 0: disable, 1: enable */
    struct i965_post_processing_context pp_context;
};

Bool i965_render_init(VADriverContextP ctx);
Bool i965_render_terminate(VADriverContextP ctx);
void i965_render_put_surface(VADriverContextP ctx,
                             VASurfaceID surface,
                             short srcx,
                             short srcy,
                             unsigned short srcw,
                             unsigned short srch,
                             short destx,
                             short desty,
                             unsigned short destw,
                             unsigned short desth,
                             unsigned int flag);


void
i965_render_put_subpic(VADriverContextP ctx,
                        VASurfaceID surface,
                        short srcx,
                        short srcy,
                        unsigned short srcw,
                        unsigned short srch,
                        short destx,
                        short desty,
                        unsigned short destw,
                        unsigned short desth);
#endif /* _I965_RENDER_H_ */
