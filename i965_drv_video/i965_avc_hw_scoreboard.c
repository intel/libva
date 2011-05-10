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

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "va_backend.h"

#include "intel_batchbuffer.h"
#include "intel_driver.h"

#include "i965_defines.h"
#include "i965_drv_video.h"
#include "i965_avc_hw_scoreboard.h"
#include "i965_media_h264.h"
#include "i965_media.h"

/* On Ironlake */
#include "shaders/h264/mc/export.inc.gen5"

enum {
    AVC_HW_SCOREBOARD = 0,
    AVC_HW_SCOREBOARD_MBAFF
};

static unsigned long avc_hw_scoreboard_kernel_offset[] = {
    SETHWSCOREBOARD_IP_GEN5 * INST_UNIT_GEN5,
    SETHWSCOREBOARD_MBAFF_IP_GEN5 * INST_UNIT_GEN5
};

static unsigned int avc_hw_scoreboard_constants[] = {
    0x08040201,
    0x00000010,
    0x08000210,
    0x00000000,
    0x08040201,
    0x08040210,
    0x01000010,
    0x08040200
};

static void
i965_avc_hw_scoreboard_surface_state(struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context)
{
    struct i965_surface_state *ss;
    dri_bo *bo;

    bo = avc_hw_scoreboard_context->surface.ss_bo;
    dri_bo_map(bo, 1);
    assert(bo->virtual);
    ss = bo->virtual;
    memset(ss, 0, sizeof(*ss));
    ss->ss0.surface_type = I965_SURFACE_BUFFER;
    ss->ss1.base_addr = avc_hw_scoreboard_context->surface.s_bo->offset;
    ss->ss2.width = ((avc_hw_scoreboard_context->surface.total_mbs * MB_CMD_IN_OWS - 1) & 0x7f);
    ss->ss2.height = (((avc_hw_scoreboard_context->surface.total_mbs * MB_CMD_IN_OWS - 1) >> 7) & 0x1fff);
    ss->ss3.depth = (((avc_hw_scoreboard_context->surface.total_mbs * MB_CMD_IN_OWS - 1) >> 20) & 0x7f);
    dri_bo_emit_reloc(bo,
                      I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                      0,
                      offsetof(struct i965_surface_state, ss1),
                      avc_hw_scoreboard_context->surface.s_bo);
    dri_bo_unmap(bo);
}

static void
i965_avc_hw_scoreboard_interface_descriptor_table(struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context)
{
    struct i965_interface_descriptor *desc;
    dri_bo *bo;

    bo = avc_hw_scoreboard_context->idrt.bo;
    dri_bo_map(bo, 1);
    assert(bo->virtual);
    desc = bo->virtual;
    memset(desc, 0, sizeof(*desc));
    desc->desc0.grf_reg_blocks = 7;
    desc->desc0.kernel_start_pointer = (avc_hw_scoreboard_context->hw_kernel.bo->offset + 
                                        avc_hw_scoreboard_context->hw_kernel.offset) >> 6; /* reloc */
    desc->desc1.const_urb_entry_read_offset = 0;
    desc->desc1.const_urb_entry_read_len = 1;
    desc->desc3.binding_table_entry_count = 0;
    desc->desc3.binding_table_pointer = 
        avc_hw_scoreboard_context->binding_table.bo->offset >> 5; /*reloc */

    dri_bo_emit_reloc(bo,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      desc->desc0.grf_reg_blocks + avc_hw_scoreboard_context->hw_kernel.offset,
                      offsetof(struct i965_interface_descriptor, desc0),
                      avc_hw_scoreboard_context->hw_kernel.bo);

    dri_bo_emit_reloc(bo,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      desc->desc3.binding_table_entry_count,
                      offsetof(struct i965_interface_descriptor, desc3),
                      avc_hw_scoreboard_context->binding_table.bo);

    dri_bo_unmap(bo);
}

static void
i965_avc_hw_scoreboard_binding_table(struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context)
{
    unsigned int *binding_table;
    dri_bo *bo = avc_hw_scoreboard_context->binding_table.bo;

    dri_bo_map(bo, 1);
    assert(bo->virtual);
    binding_table = bo->virtual;
    memset(binding_table, 0, bo->size);
    binding_table[0] = avc_hw_scoreboard_context->surface.ss_bo->offset;
    dri_bo_emit_reloc(bo,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      0,
                      0,
                      avc_hw_scoreboard_context->surface.ss_bo);
    dri_bo_unmap(bo);
}

static void
i965_avc_hw_scoreboard_vfe_state(struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context)
{
    struct i965_vfe_state *vfe_state;
    dri_bo *bo;

    bo = avc_hw_scoreboard_context->vfe_state.bo;
    dri_bo_map(bo, 1);
    assert(bo->virtual);
    vfe_state = bo->virtual;
    memset(vfe_state, 0, sizeof(*vfe_state));
    vfe_state->vfe1.max_threads = avc_hw_scoreboard_context->urb.num_vfe_entries - 1;
    vfe_state->vfe1.urb_entry_alloc_size = avc_hw_scoreboard_context->urb.size_vfe_entry - 1;
    vfe_state->vfe1.num_urb_entries = avc_hw_scoreboard_context->urb.num_vfe_entries;
    vfe_state->vfe1.vfe_mode = VFE_GENERIC_MODE;
    vfe_state->vfe1.children_present = 0;
    vfe_state->vfe2.interface_descriptor_base = 
        avc_hw_scoreboard_context->idrt.bo->offset >> 4; /* reloc */
    dri_bo_emit_reloc(bo,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      0,
                      offsetof(struct i965_vfe_state, vfe2),
                      avc_hw_scoreboard_context->idrt.bo);
    dri_bo_unmap(bo);
}

static void
i965_avc_hw_scoreboard_upload_constants(struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context)
{
    unsigned char *constant_buffer;

    if (avc_hw_scoreboard_context->curbe.upload)
        return;

    dri_bo_map(avc_hw_scoreboard_context->curbe.bo, 1);
    assert(avc_hw_scoreboard_context->curbe.bo->virtual);
    constant_buffer = avc_hw_scoreboard_context->curbe.bo->virtual;
    memcpy(constant_buffer, avc_hw_scoreboard_constants, sizeof(avc_hw_scoreboard_constants));
    dri_bo_unmap(avc_hw_scoreboard_context->curbe.bo);
    avc_hw_scoreboard_context->curbe.upload = 1;
}

static void
i965_avc_hw_scoreboard_states_setup(struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context)
{
    i965_avc_hw_scoreboard_surface_state(avc_hw_scoreboard_context);
    i965_avc_hw_scoreboard_binding_table(avc_hw_scoreboard_context);
    i965_avc_hw_scoreboard_interface_descriptor_table(avc_hw_scoreboard_context);
    i965_avc_hw_scoreboard_vfe_state(avc_hw_scoreboard_context);
    i965_avc_hw_scoreboard_upload_constants(avc_hw_scoreboard_context);
}

static void
i965_avc_hw_scoreboard_pipeline_select(VADriverContextP ctx)
{
    BEGIN_BATCH(ctx, 1);
    OUT_BATCH(ctx, CMD_PIPELINE_SELECT | PIPELINE_SELECT_MEDIA);
    ADVANCE_BATCH(ctx);
}

static void
i965_avc_hw_scoreboard_urb_layout(VADriverContextP ctx, struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    unsigned int vfe_fence, cs_fence;

    vfe_fence = avc_hw_scoreboard_context->urb.cs_start;
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
i965_avc_hw_scoreboard_state_base_address(VADriverContextP ctx)
{
    BEGIN_BATCH(ctx, 8);
    OUT_BATCH(ctx, CMD_STATE_BASE_ADDRESS | 6);
    OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
    OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
    OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
    OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
    OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
    OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
    OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
    ADVANCE_BATCH(ctx);
}

static void
i965_avc_hw_scoreboard_state_pointers(VADriverContextP ctx, struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context)
{
    BEGIN_BATCH(ctx, 3);
    OUT_BATCH(ctx, CMD_MEDIA_STATE_POINTERS | 1);
    OUT_BATCH(ctx, 0);
    OUT_RELOC(ctx, avc_hw_scoreboard_context->vfe_state.bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
    ADVANCE_BATCH(ctx);
}

static void 
i965_avc_hw_scoreboard_cs_urb_layout(VADriverContextP ctx, struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context)
{
    BEGIN_BATCH(ctx, 2);
    OUT_BATCH(ctx, CMD_CS_URB_STATE | 0);
    OUT_BATCH(ctx,
              ((avc_hw_scoreboard_context->urb.size_cs_entry - 1) << 4) |     /* URB Entry Allocation Size */
              (avc_hw_scoreboard_context->urb.num_cs_entries << 0));          /* Number of URB Entries */
    ADVANCE_BATCH(ctx);
}

static void
i965_avc_hw_scoreboard_constant_buffer(VADriverContextP ctx, struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context)
{
    BEGIN_BATCH(ctx, 2);
    OUT_BATCH(ctx, CMD_CONSTANT_BUFFER | (1 << 8) | (2 - 2));
    OUT_RELOC(ctx, avc_hw_scoreboard_context->curbe.bo,
              I915_GEM_DOMAIN_INSTRUCTION, 0,
              avc_hw_scoreboard_context->urb.size_cs_entry - 1);
    ADVANCE_BATCH(ctx);    
}

static void
i965_avc_hw_scoreboard_objects(VADriverContextP ctx, struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context)
{
    int number_mb_cmds = 512;
    int starting_mb_number = avc_hw_scoreboard_context->inline_data.starting_mb_number;
    int i;

    for (i = 0; i < avc_hw_scoreboard_context->inline_data.num_mb_cmds / 512; i++) {
        BEGIN_BATCH(ctx, 6);
        OUT_BATCH(ctx, CMD_MEDIA_OBJECT | 4);
        OUT_BATCH(ctx, 0); /* interface descriptor offset: 0 */
        OUT_BATCH(ctx, 0); /* no indirect data */
        OUT_BATCH(ctx, 0);
        OUT_BATCH(ctx, ((number_mb_cmds << 16) |
                        (starting_mb_number << 0)));
        OUT_BATCH(ctx, avc_hw_scoreboard_context->inline_data.pic_width_in_mbs);
        ADVANCE_BATCH(ctx);

        starting_mb_number += 512;
    }

    number_mb_cmds = avc_hw_scoreboard_context->inline_data.num_mb_cmds % 512;

    if (number_mb_cmds) {
        BEGIN_BATCH(ctx, 6);
        OUT_BATCH(ctx, CMD_MEDIA_OBJECT | 4);
        OUT_BATCH(ctx, 0); /* interface descriptor offset: 0 */
        OUT_BATCH(ctx, 0); /* no indirect data */
        OUT_BATCH(ctx, 0);
        OUT_BATCH(ctx, ((number_mb_cmds << 16) |
                        (starting_mb_number << 0)));
        OUT_BATCH(ctx, avc_hw_scoreboard_context->inline_data.pic_width_in_mbs);
        ADVANCE_BATCH(ctx);
    }
}

static void
i965_avc_hw_scoreboard_pipeline_setup(VADriverContextP ctx, struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context)
{
    intel_batchbuffer_start_atomic(ctx, 0x1000);
    intel_batchbuffer_emit_mi_flush(ctx);
    i965_avc_hw_scoreboard_pipeline_select(ctx);
    i965_avc_hw_scoreboard_state_base_address(ctx);
    i965_avc_hw_scoreboard_state_pointers(ctx, avc_hw_scoreboard_context);
    i965_avc_hw_scoreboard_urb_layout(ctx, avc_hw_scoreboard_context);
    i965_avc_hw_scoreboard_cs_urb_layout(ctx, avc_hw_scoreboard_context);
    i965_avc_hw_scoreboard_constant_buffer(ctx, avc_hw_scoreboard_context);
    i965_avc_hw_scoreboard_objects(ctx, avc_hw_scoreboard_context);
    intel_batchbuffer_end_atomic(ctx);
}

void
i965_avc_hw_scoreboard(VADriverContextP ctx, struct decode_state *decode_state, void *h264_context)
{
    struct i965_h264_context *i965_h264_context = (struct i965_h264_context *)h264_context;

    if (i965_h264_context->use_avc_hw_scoreboard) {
        struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context = &i965_h264_context->avc_hw_scoreboard_context;

        avc_hw_scoreboard_context->inline_data.num_mb_cmds = i965_h264_context->avc_it_command_mb_info.mbs;
        avc_hw_scoreboard_context->inline_data.starting_mb_number = i965_h264_context->avc_it_command_mb_info.mbs;
        avc_hw_scoreboard_context->inline_data.pic_width_in_mbs = i965_h264_context->picture.width_in_mbs;
        avc_hw_scoreboard_context->surface.total_mbs = i965_h264_context->avc_it_command_mb_info.mbs * 2;
        
        dri_bo_unreference(avc_hw_scoreboard_context->hw_kernel.bo);
        avc_hw_scoreboard_context->hw_kernel.bo = i965_h264_context->avc_kernels[H264_AVC_COMBINED].bo;
        assert(avc_hw_scoreboard_context->hw_kernel.bo != NULL);
        dri_bo_reference(avc_hw_scoreboard_context->hw_kernel.bo);

        if (i965_h264_context->picture.mbaff_frame_flag)
            avc_hw_scoreboard_context->hw_kernel.offset = avc_hw_scoreboard_kernel_offset[AVC_HW_SCOREBOARD_MBAFF];
        else
            avc_hw_scoreboard_context->hw_kernel.offset = avc_hw_scoreboard_kernel_offset[AVC_HW_SCOREBOARD];

        i965_avc_hw_scoreboard_states_setup(avc_hw_scoreboard_context);
        i965_avc_hw_scoreboard_pipeline_setup(ctx, avc_hw_scoreboard_context);
    }
}

void
i965_avc_hw_scoreboard_decode_init(VADriverContextP ctx, void *h264_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_h264_context *i965_h264_context = (struct i965_h264_context *)h264_context;

    if (i965_h264_context->use_avc_hw_scoreboard) {
        struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context = &i965_h264_context->avc_hw_scoreboard_context;
        dri_bo *bo;

        if (avc_hw_scoreboard_context->curbe.bo == NULL) {
            bo = dri_bo_alloc(i965->intel.bufmgr,
                              "constant buffer",
                              4096, 64);
            assert(bo);
            avc_hw_scoreboard_context->curbe.bo = bo;
            avc_hw_scoreboard_context->curbe.upload = 0;
        }

        dri_bo_unreference(avc_hw_scoreboard_context->surface.s_bo);
        avc_hw_scoreboard_context->surface.s_bo = i965_h264_context->avc_it_command_mb_info.bo;
        assert(avc_hw_scoreboard_context->surface.s_bo != NULL);
        dri_bo_reference(avc_hw_scoreboard_context->surface.s_bo);

        dri_bo_unreference(avc_hw_scoreboard_context->surface.ss_bo);
        bo = dri_bo_alloc(i965->intel.bufmgr, 
                          "surface state", 
                          sizeof(struct i965_surface_state), 32);
        assert(bo);
        avc_hw_scoreboard_context->surface.ss_bo = bo;

        dri_bo_unreference(avc_hw_scoreboard_context->binding_table.bo);
        bo = dri_bo_alloc(i965->intel.bufmgr, 
                          "binding table",
                          MAX_MEDIA_SURFACES * sizeof(unsigned int), 32);
        assert(bo);
        avc_hw_scoreboard_context->binding_table.bo = bo;

        dri_bo_unreference(avc_hw_scoreboard_context->idrt.bo);
        bo = dri_bo_alloc(i965->intel.bufmgr, 
                          "interface discriptor", 
                          MAX_INTERFACE_DESC * sizeof(struct i965_interface_descriptor), 16);
        assert(bo);
        avc_hw_scoreboard_context->idrt.bo = bo;

        dri_bo_unreference(avc_hw_scoreboard_context->vfe_state.bo);
        bo = dri_bo_alloc(i965->intel.bufmgr, 
                          "vfe state", 
                          sizeof(struct i965_vfe_state), 32);
        assert(bo);
        avc_hw_scoreboard_context->vfe_state.bo = bo;

        avc_hw_scoreboard_context->urb.num_vfe_entries = 32;
        avc_hw_scoreboard_context->urb.size_vfe_entry = 2;
        avc_hw_scoreboard_context->urb.num_cs_entries = 1;
        avc_hw_scoreboard_context->urb.size_cs_entry = 1;
        avc_hw_scoreboard_context->urb.vfe_start = 0;
        avc_hw_scoreboard_context->urb.cs_start = avc_hw_scoreboard_context->urb.vfe_start + 
            avc_hw_scoreboard_context->urb.num_vfe_entries * avc_hw_scoreboard_context->urb.size_vfe_entry;
        assert(avc_hw_scoreboard_context->urb.cs_start + 
               avc_hw_scoreboard_context->urb.num_cs_entries * avc_hw_scoreboard_context->urb.size_cs_entry <= URB_SIZE((&i965->intel)));
    }
}

Bool 
i965_avc_hw_scoreboard_ternimate(struct i965_avc_hw_scoreboard_context *avc_hw_scoreboard_context)
{
    dri_bo_unreference(avc_hw_scoreboard_context->curbe.bo);
    avc_hw_scoreboard_context->curbe.bo = NULL;

    dri_bo_unreference(avc_hw_scoreboard_context->surface.ss_bo);
    avc_hw_scoreboard_context->surface.ss_bo = NULL;

    dri_bo_unreference(avc_hw_scoreboard_context->surface.s_bo);
    avc_hw_scoreboard_context->surface.s_bo = NULL;

    dri_bo_unreference(avc_hw_scoreboard_context->binding_table.bo);
    avc_hw_scoreboard_context->binding_table.bo = NULL;

    dri_bo_unreference(avc_hw_scoreboard_context->idrt.bo);
    avc_hw_scoreboard_context->idrt.bo = NULL;

    dri_bo_unreference(avc_hw_scoreboard_context->vfe_state.bo);
    avc_hw_scoreboard_context->vfe_state.bo = NULL;

    dri_bo_unreference(avc_hw_scoreboard_context->hw_kernel.bo);
    avc_hw_scoreboard_context->hw_kernel.bo = NULL;

    return True;
}
