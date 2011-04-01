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
 *    Zhou chang <chang.zhou@intel.com>
 *
 */

#ifndef _GEN6_MFC_H_
#define _GEN6_MFC_H_

#include <xf86drm.h>
#include <drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>
#include "i965_structs.h"
#include "i965_drv_video.h"


VAStatus i965_encoder_create_context(
    VADriverContextP ctx,
    VAConfigID config_id,
    int picture_width,
    int picture_height,
    int flag,
    VASurfaceID *render_targets,
    int num_render_targets,
    struct object_context *obj_context
    );

VAStatus i965_encoder_begin_picture(
    VADriverContextP ctx,
    VAContextID context,
    VASurfaceID render_target
    );

VAStatus i965_encoder_render_picture(VADriverContextP ctx,
                                 VAContextID context,
                                 VABufferID *buffers,
                                 int num_buffers
    );

VAStatus i965_encoder_end_picture(VADriverContextP ctx, 
                              VAContextID context
    );


void i965_encoder_destroy_context(struct object_heap *heap, struct object_base *obj);

Bool i965_encoder_init(VADriverContextP ctx);
Bool i965_encoder_terminate(VADriverContextP ctx);

#endif	/* _GEN6_MFC_H_ */


