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
#include <string.h>
#include <assert.h>

#include <va/va_backend.h>

#include "intel_batchbuffer.h"
#include "intel_driver.h"

#include "i965_defines.h"
#include "i965_media_mpeg2.h"
#include "i965_media_h264.h"
#include "i965_media.h"
#include "i965_drv_video.h"

static void
i965_media_pipeline_select(VADriverContextP ctx)
{
    BEGIN_BATCH(ctx, 1);
    OUT_BATCH(ctx, CMD_PIPELINE_SELECT | PIPELINE_SELECT_MEDIA);
    ADVANCE_BATCH(ctx);
}

static void
i965_media_urb_layout(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    unsigned int vfe_fence, cs_fence;

    vfe_fence = media_state->urb.cs_start;
    cs_fence = URB_SIZE((&i965->intel));

    BEGIN_BATCH(ctx, 3);
    OUT_BATCH(ctx, CMD_URB_FENCE | UF0_VFE_REALLOC | UF0_CS_REALLOC | 1);
    OUT_BATCH(ctx, 0);
    OUT_BATCH(ctx, 
              (vfe_fence << UF2_VFE_FENCE_SHIFT) |      /* VFE_SIZE */
              (cs_fence << UF2_CS_FENCE_SHIFT));        /* CS_SIZE */
    ADVANCE_BATCH(ctx);
}

static void
i965_media_state_base_address(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx); 
    struct i965_media_state *media_state = &i965->media_state;

    if (IS_IRONLAKE(i965->intel.device_id)) {
        BEGIN_BATCH(ctx, 8);
        OUT_BATCH(ctx, CMD_STATE_BASE_ADDRESS | 6);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        
        if (media_state->indirect_object.bo) {
            OUT_RELOC(ctx, media_state->indirect_object.bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 
                      media_state->indirect_object.offset | BASE_ADDRESS_MODIFY);
        } else {
            OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        }

        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        ADVANCE_BATCH(ctx);
    } else {
        BEGIN_BATCH(ctx, 6);
        OUT_BATCH(ctx, CMD_STATE_BASE_ADDRESS | 4);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);

        if (media_state->indirect_object.bo) {
            OUT_RELOC(ctx, media_state->indirect_object.bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 
                      media_state->indirect_object.offset | BASE_ADDRESS_MODIFY);
        } else {
            OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        }

        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        ADVANCE_BATCH(ctx);
    }
}

static void
i965_media_state_pointers(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;

    BEGIN_BATCH(ctx, 3);
    OUT_BATCH(ctx, CMD_MEDIA_STATE_POINTERS | 1);

    if (media_state->extended_state.enabled)
        OUT_RELOC(ctx, media_state->extended_state.bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 1);
    else
        OUT_BATCH(ctx, 0);

    OUT_RELOC(ctx, media_state->vfe_state.bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
    ADVANCE_BATCH(ctx);
}

static void 
i965_media_cs_urb_layout(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;

    BEGIN_BATCH(ctx, 2);
    OUT_BATCH(ctx, CMD_CS_URB_STATE | 0);
    OUT_BATCH(ctx,
              ((media_state->urb.size_cs_entry - 1) << 4) |     /* URB Entry Allocation Size */
              (media_state->urb.num_cs_entries << 0));          /* Number of URB Entries */
    ADVANCE_BATCH(ctx);
}

static void 
i965_media_pipeline_state(VADriverContextP ctx)
{
    i965_media_state_base_address(ctx);
    i965_media_state_pointers(ctx);
    i965_media_cs_urb_layout(ctx);
}

static void
i965_media_constant_buffer(VADriverContextP ctx, struct decode_state *decode_state)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;

    BEGIN_BATCH(ctx, 2);
    OUT_BATCH(ctx, CMD_CONSTANT_BUFFER | (1 << 8) | (2 - 2));
    OUT_RELOC(ctx, media_state->curbe.bo,
              I915_GEM_DOMAIN_INSTRUCTION, 0,
              media_state->urb.size_cs_entry - 1);
    ADVANCE_BATCH(ctx);    
}

static void 
i965_media_depth_buffer(VADriverContextP ctx)
{
    BEGIN_BATCH(ctx, 6);
    OUT_BATCH(ctx, CMD_DEPTH_BUFFER | 4);
    OUT_BATCH(ctx, (I965_DEPTHFORMAT_D32_FLOAT << 18) | 
              (I965_SURFACE_NULL << 29));
    OUT_BATCH(ctx, 0);
    OUT_BATCH(ctx, 0);
    OUT_BATCH(ctx, 0);
    OUT_BATCH(ctx, 0);
    ADVANCE_BATCH();
}

static void
i965_media_pipeline_setup(VADriverContextP ctx, struct decode_state *decode_state)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;

    intel_batchbuffer_start_atomic(ctx, 0x1000);
    intel_batchbuffer_emit_mi_flush(ctx);                       /* step 1 */
    i965_media_depth_buffer(ctx);
    i965_media_pipeline_select(ctx);                            /* step 2 */
    i965_media_urb_layout(ctx);                                 /* step 3 */
    i965_media_pipeline_state(ctx);                             /* step 4 */
    i965_media_constant_buffer(ctx, decode_state);              /* step 5 */
    assert(media_state->media_objects);
    media_state->media_objects(ctx, decode_state);              /* step 6 */
    intel_batchbuffer_end_atomic(ctx);
}

static void 
i965_media_decode_init(VADriverContextP ctx, VAProfile profile, struct decode_state *decode_state)
{
    int i;
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    dri_bo *bo;

    /* constant buffer */
    dri_bo_unreference(media_state->curbe.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "constant buffer",
                      4096, 64);
    assert(bo);
    media_state->curbe.bo = bo;

    /* surface state */
    for (i = 0; i < MAX_MEDIA_SURFACES; i++) {
        dri_bo_unreference(media_state->surface_state[i].bo);
        media_state->surface_state[i].bo = NULL;
    }

    /* binding table */
    dri_bo_unreference(media_state->binding_table.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "binding table",
                      MAX_MEDIA_SURFACES * sizeof(unsigned int), 32);
    assert(bo);
    media_state->binding_table.bo = bo;

    /* interface descriptor remapping table */
    dri_bo_unreference(media_state->idrt.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "interface discriptor", 
                      MAX_INTERFACE_DESC * sizeof(struct i965_interface_descriptor), 16);
    assert(bo);
    media_state->idrt.bo = bo;

    /* vfe state */
    dri_bo_unreference(media_state->vfe_state.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "vfe state", 
                      sizeof(struct i965_vfe_state), 32);
    assert(bo);
    media_state->vfe_state.bo = bo;

    /* extended state */
    media_state->extended_state.enabled = 0;

    switch (profile) {
    case VAProfileMPEG2Simple:
    case VAProfileMPEG2Main:
        i965_media_mpeg2_decode_init(ctx, decode_state);
        break;
        
    case VAProfileH264Baseline:
    case VAProfileH264Main:
    case VAProfileH264High:
        i965_media_h264_decode_init(ctx, decode_state);
        break;

    default:
        assert(0);
        break;
    }
}

void 
i965_media_decode_picture(VADriverContextP ctx, 
                          VAProfile profile, 
                          struct decode_state *decode_state)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;

    i965_media_decode_init(ctx, profile, decode_state);
    assert(media_state->media_states_setup);
    media_state->media_states_setup(ctx, decode_state);
    i965_media_pipeline_setup(ctx, decode_state);
}

Bool 
i965_media_init(VADriverContextP ctx)
{
    return True;
}

Bool 
i965_media_terminate(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    int i;

    if (media_state->free_private_context)
        media_state->free_private_context(&media_state->private_context);

    for (i = 0; i < MAX_MEDIA_SURFACES; i++) {
        dri_bo_unreference(media_state->surface_state[i].bo);
        media_state->surface_state[i].bo = NULL;
    }
    
    dri_bo_unreference(media_state->extended_state.bo);
    media_state->extended_state.bo = NULL;

    dri_bo_unreference(media_state->vfe_state.bo);
    media_state->vfe_state.bo = NULL;

    dri_bo_unreference(media_state->idrt.bo);
    media_state->idrt.bo = NULL;

    dri_bo_unreference(media_state->binding_table.bo);
    media_state->binding_table.bo = NULL;

    dri_bo_unreference(media_state->curbe.bo);
    media_state->curbe.bo = NULL;

    dri_bo_unreference(media_state->indirect_object.bo);
    media_state->indirect_object.bo = NULL;

    return True;
}

