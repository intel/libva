/*
 * Copyright © 2009 Intel Corporation
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
 *    Zou Nan hai <nanhai.zou@intel.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <va/va_backend.h>

#include "intel_batchbuffer.h"
#include "intel_driver.h"
#include "i965_defines.h"
#include "i965_drv_video.h"

#include "i965_media.h"
#include "i965_media_mpeg2.h"
#include "i965_media_h264.h"

static void
i965_media_pipeline_select(VADriverContextP ctx, struct i965_media_context *media_context)
{
    struct intel_batchbuffer *batch = media_context->base.batch;

    BEGIN_BATCH(batch, 1);
    OUT_BATCH(batch, CMD_PIPELINE_SELECT | PIPELINE_SELECT_MEDIA);
    ADVANCE_BATCH(batch);
}

static void
i965_media_urb_layout(VADriverContextP ctx, struct i965_media_context *media_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx); 
    struct intel_batchbuffer *batch = media_context->base.batch;
    unsigned int vfe_fence, cs_fence;

    vfe_fence = media_context->urb.cs_start;
    cs_fence = URB_SIZE((&i965->intel));

    BEGIN_BATCH(batch, 3);
    OUT_BATCH(batch, CMD_URB_FENCE | UF0_VFE_REALLOC | UF0_CS_REALLOC | 1);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 
              (vfe_fence << UF2_VFE_FENCE_SHIFT) |      /* VFE_SIZE */
              (cs_fence << UF2_CS_FENCE_SHIFT));        /* CS_SIZE */
    ADVANCE_BATCH(batch);
}

static void
i965_media_state_base_address(VADriverContextP ctx, struct i965_media_context *media_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx); 
    struct intel_batchbuffer *batch = media_context->base.batch;

    if (IS_IRONLAKE(i965->intel.device_id)) {
        BEGIN_BATCH(batch, 8);
        OUT_BATCH(batch, CMD_STATE_BASE_ADDRESS | 6);
        OUT_BATCH(batch, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(batch, 0 | BASE_ADDRESS_MODIFY);
        
        if (media_context->indirect_object.bo) {
            OUT_RELOC(batch, media_context->indirect_object.bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 
                      media_context->indirect_object.offset | BASE_ADDRESS_MODIFY);
        } else {
            OUT_BATCH(batch, 0 | BASE_ADDRESS_MODIFY);
        }

        OUT_BATCH(batch, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(batch, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(batch, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(batch, 0 | BASE_ADDRESS_MODIFY);
        ADVANCE_BATCH(batch);
    } else {
        BEGIN_BATCH(batch, 6);
        OUT_BATCH(batch, CMD_STATE_BASE_ADDRESS | 4);
        OUT_BATCH(batch, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(batch, 0 | BASE_ADDRESS_MODIFY);

        if (media_context->indirect_object.bo) {
            OUT_RELOC(batch, media_context->indirect_object.bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 
                      media_context->indirect_object.offset | BASE_ADDRESS_MODIFY);
        } else {
            OUT_BATCH(batch, 0 | BASE_ADDRESS_MODIFY);
        }

        OUT_BATCH(batch, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(batch, 0 | BASE_ADDRESS_MODIFY);
        ADVANCE_BATCH(batch);
    }
}

static void
i965_media_state_pointers(VADriverContextP ctx, struct i965_media_context *media_context)
{
    struct intel_batchbuffer *batch = media_context->base.batch;

    BEGIN_BATCH(batch, 3);
    OUT_BATCH(batch, CMD_MEDIA_STATE_POINTERS | 1);

    if (media_context->extended_state.enabled)
        OUT_RELOC(batch, media_context->extended_state.bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 1);
    else
        OUT_BATCH(batch, 0);

    OUT_RELOC(batch, media_context->vfe_state.bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
    ADVANCE_BATCH(batch);
}

static void 
i965_media_cs_urb_layout(VADriverContextP ctx, struct i965_media_context *media_context)
{
    struct intel_batchbuffer *batch = media_context->base.batch;

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, CMD_CS_URB_STATE | 0);
    OUT_BATCH(batch,
              ((media_context->urb.size_cs_entry - 1) << 4) |     /* URB Entry Allocation Size */
              (media_context->urb.num_cs_entries << 0));          /* Number of URB Entries */
    ADVANCE_BATCH(batch);
}

static void 
i965_media_pipeline_state(VADriverContextP ctx, struct i965_media_context *media_context)
{
    i965_media_state_base_address(ctx, media_context);
    i965_media_state_pointers(ctx, media_context);
    i965_media_cs_urb_layout(ctx, media_context);
}

static void
i965_media_constant_buffer(VADriverContextP ctx, struct decode_state *decode_state, struct i965_media_context *media_context)
{
    struct intel_batchbuffer *batch = media_context->base.batch;

    BEGIN_BATCH(batch, 2);
    OUT_BATCH(batch, CMD_CONSTANT_BUFFER | (1 << 8) | (2 - 2));
    OUT_RELOC(batch, media_context->curbe.bo,
              I915_GEM_DOMAIN_INSTRUCTION, 0,
              media_context->urb.size_cs_entry - 1);
    ADVANCE_BATCH(batch);    
}

static void 
i965_media_depth_buffer(VADriverContextP ctx, struct i965_media_context *media_context)
{
    struct intel_batchbuffer *batch = media_context->base.batch;

    BEGIN_BATCH(batch, 6);
    OUT_BATCH(batch, CMD_DEPTH_BUFFER | 4);
    OUT_BATCH(batch, (I965_DEPTHFORMAT_D32_FLOAT << 18) | 
              (I965_SURFACE_NULL << 29));
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    OUT_BATCH(batch, 0);
    ADVANCE_BATCH(batch);
}

static void
i965_media_pipeline_setup(VADriverContextP ctx,
                          struct decode_state *decode_state,
                          struct i965_media_context *media_context)
{
    struct intel_batchbuffer *batch = media_context->base.batch;

    intel_batchbuffer_start_atomic(batch, 0x1000);
    intel_batchbuffer_emit_mi_flush(batch);                             /* step 1 */
    i965_media_depth_buffer(ctx, media_context);
    i965_media_pipeline_select(ctx, media_context);                     /* step 2 */
    i965_media_urb_layout(ctx, media_context);                          /* step 3 */
    i965_media_pipeline_state(ctx, media_context);                      /* step 4 */
    i965_media_constant_buffer(ctx, decode_state, media_context);       /* step 5 */
    assert(media_context->media_objects);
    media_context->media_objects(ctx, decode_state, media_context);     /* step 6 */
    intel_batchbuffer_end_atomic(batch);
}

static void 
i965_media_decode_init(VADriverContextP ctx, 
                       VAProfile profile, 
                       struct decode_state *decode_state, 
                       struct i965_media_context *media_context)
{
    int i;
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    dri_bo *bo;

    /* constant buffer */
    dri_bo_unreference(media_context->curbe.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "constant buffer",
                      4096, 64);
    assert(bo);
    media_context->curbe.bo = bo;

    /* surface state */
    for (i = 0; i < MAX_MEDIA_SURFACES; i++) {
        dri_bo_unreference(media_context->surface_state[i].bo);
        media_context->surface_state[i].bo = NULL;
    }

    /* binding table */
    dri_bo_unreference(media_context->binding_table.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "binding table",
                      MAX_MEDIA_SURFACES * sizeof(unsigned int), 32);
    assert(bo);
    media_context->binding_table.bo = bo;

    /* interface descriptor remapping table */
    dri_bo_unreference(media_context->idrt.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "interface discriptor", 
                      MAX_INTERFACE_DESC * sizeof(struct i965_interface_descriptor), 16);
    assert(bo);
    media_context->idrt.bo = bo;

    /* vfe state */
    dri_bo_unreference(media_context->vfe_state.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "vfe state", 
                      sizeof(struct i965_vfe_state), 32);
    assert(bo);
    media_context->vfe_state.bo = bo;

    /* extended state */
    media_context->extended_state.enabled = 0;

    switch (profile) {
    case VAProfileMPEG2Simple:
    case VAProfileMPEG2Main:
        i965_media_mpeg2_decode_init(ctx, decode_state, media_context);
        break;
        
    case VAProfileH264Baseline:
    case VAProfileH264Main:
    case VAProfileH264High:
        i965_media_h264_decode_init(ctx, decode_state, media_context);
        break;

    default:
        assert(0);
        break;
    }
}

static void 
i965_media_decode_picture(VADriverContextP ctx, 
                          VAProfile profile, 
                          union codec_state *codec_state,
                          struct hw_context *hw_context)
{
    struct i965_media_context *media_context = (struct i965_media_context *)hw_context;
    struct decode_state *decode_state = &codec_state->dec;

    i965_media_decode_init(ctx, profile, decode_state, media_context);
    assert(media_context->media_states_setup);
    media_context->media_states_setup(ctx, decode_state, media_context);
    i965_media_pipeline_setup(ctx, decode_state, media_context);
    intel_batchbuffer_flush(hw_context->batch);
}

static void
i965_media_context_destroy(void *hw_context)
{
    struct i965_media_context *media_context = (struct i965_media_context *)hw_context;
    int i; 

    if (media_context->free_private_context)
        media_context->free_private_context(&media_context->private_context);

    for (i = 0; i < MAX_MEDIA_SURFACES; i++) {
        dri_bo_unreference(media_context->surface_state[i].bo);
        media_context->surface_state[i].bo = NULL;
    }
    
    dri_bo_unreference(media_context->extended_state.bo);
    media_context->extended_state.bo = NULL;

    dri_bo_unreference(media_context->vfe_state.bo);
    media_context->vfe_state.bo = NULL;

    dri_bo_unreference(media_context->idrt.bo);
    media_context->idrt.bo = NULL;

    dri_bo_unreference(media_context->binding_table.bo);
    media_context->binding_table.bo = NULL;

    dri_bo_unreference(media_context->curbe.bo);
    media_context->curbe.bo = NULL;

    dri_bo_unreference(media_context->indirect_object.bo);
    media_context->indirect_object.bo = NULL;

    intel_batchbuffer_free(media_context->base.batch);
    free(media_context);
}

struct hw_context *
g4x_dec_hw_context_init(VADriverContextP ctx, VAProfile profile)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    struct i965_media_context *media_context = calloc(1, sizeof(struct i965_media_context));

    media_context->base.destroy = i965_media_context_destroy;
    media_context->base.run = i965_media_decode_picture;
    media_context->base.batch = intel_batchbuffer_new(intel, I915_EXEC_RENDER);

    switch (profile) {
    case VAProfileMPEG2Simple:
    case VAProfileMPEG2Main:
        i965_media_mpeg2_dec_context_init(ctx, media_context);
        break;

    case VAProfileH264Baseline:
    case VAProfileH264Main:
    case VAProfileH264High:
    case VAProfileVC1Simple:
    case VAProfileVC1Main:
    case VAProfileVC1Advanced:
    default:
        assert(0);
        break;
    }

    return (struct hw_context *)media_context;
}

struct hw_context *
ironlake_dec_hw_context_init(VADriverContextP ctx, VAProfile profile)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    struct i965_media_context *media_context = calloc(1, sizeof(struct i965_media_context));

    media_context->base.destroy = i965_media_context_destroy;
    media_context->base.run = i965_media_decode_picture;
    media_context->base.batch = intel_batchbuffer_new(intel, I915_EXEC_RENDER);

    switch (profile) {
    case VAProfileMPEG2Simple:
    case VAProfileMPEG2Main:
        i965_media_mpeg2_dec_context_init(ctx, media_context);
        break;

    case VAProfileH264Baseline:
    case VAProfileH264Main:
    case VAProfileH264High:
        i965_media_h264_dec_context_init(ctx, media_context);
        break;

    case VAProfileVC1Simple:
    case VAProfileVC1Main:
    case VAProfileVC1Advanced:
    default:
        assert(0);
        break;
    }

    return (struct hw_context *)media_context;
}
