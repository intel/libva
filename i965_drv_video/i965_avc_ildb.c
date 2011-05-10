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
#include "i965_avc_ildb.h"
#include "i965_media_h264.h"
#include "i965_media.h"

/* On Cantiga */
#include "shaders/h264/mc/export.inc"

/* On Ironlake */
#include "shaders/h264/mc/export.inc.gen5"

#define PICTURE_FRAME   0
#define PICTURE_FIELD   1
#define PICTURE_MBAFF   2

enum {
    AVC_ILDB_ROOT_Y_ILDB_FRAME,
    AVC_ILDB_CHILD_Y_ILDB_FRAME,
    AVC_ILDB_ROOT_UV_ILDB_FRAME,
    AVC_ILDB_CHILD_UV_ILDB_FRAME,
    AVC_ILDB_ROOT_Y_ILDB_FIELD,
    AVC_ILDB_CHILD_Y_ILDB_FIELD,
    AVC_ILDB_ROOT_UV_ILDB_FIELD,
    AVC_ILDB_CHILD_UV_ILDB_FIELD,
    AVC_ILDB_ROOT_Y_ILDB_MBAFF,
    AVC_ILDB_CHILD_Y_ILDB_MBAFF,
    AVC_ILDB_ROOT_UV_ILDB_MBAFF,
    AVC_ILDB_CHILD_UV_ILDB_MBAFF
};

static unsigned long avc_ildb_kernel_offset_gen4[] = {
    AVC_ILDB_ROOT_Y_ILDB_FRAME_IP * INST_UNIT_GEN4,
    AVC_ILDB_CHILD_Y_ILDB_FRAME_IP * INST_UNIT_GEN4,
    AVC_ILDB_ROOT_UV_ILDB_FRAME_IP * INST_UNIT_GEN4,
    AVC_ILDB_CHILD_UV_ILDB_FRAME_IP * INST_UNIT_GEN4,
    AVC_ILDB_ROOT_Y_ILDB_FIELD_IP * INST_UNIT_GEN4,
    AVC_ILDB_CHILD_Y_ILDB_FIELD_IP * INST_UNIT_GEN4,
    AVC_ILDB_ROOT_UV_ILDB_FIELD_IP * INST_UNIT_GEN4,
    AVC_ILDB_CHILD_UV_ILDB_FIELD_IP * INST_UNIT_GEN4,
    AVC_ILDB_ROOT_Y_ILDB_MBAFF_IP * INST_UNIT_GEN4,
    AVC_ILDB_CHILD_Y_ILDB_MBAFF_IP * INST_UNIT_GEN4,
    AVC_ILDB_ROOT_UV_ILDB_MBAFF_IP * INST_UNIT_GEN4,
    AVC_ILDB_CHILD_UV_ILDB_MBAFF_IP * INST_UNIT_GEN4
};

static unsigned long avc_ildb_kernel_offset_gen5[] = {
    AVC_ILDB_ROOT_Y_ILDB_FRAME_IP_GEN5 * INST_UNIT_GEN5,
    AVC_ILDB_CHILD_Y_ILDB_FRAME_IP_GEN5 * INST_UNIT_GEN5,
    AVC_ILDB_ROOT_UV_ILDB_FRAME_IP_GEN5 * INST_UNIT_GEN5,
    AVC_ILDB_CHILD_UV_ILDB_FRAME_IP_GEN5 * INST_UNIT_GEN5,
    AVC_ILDB_ROOT_Y_ILDB_FIELD_IP_GEN5 * INST_UNIT_GEN5,
    AVC_ILDB_CHILD_Y_ILDB_FIELD_IP_GEN5 * INST_UNIT_GEN5,
    AVC_ILDB_ROOT_UV_ILDB_FIELD_IP_GEN5 * INST_UNIT_GEN5,
    AVC_ILDB_CHILD_UV_ILDB_FIELD_IP_GEN5 * INST_UNIT_GEN5,
    AVC_ILDB_ROOT_Y_ILDB_MBAFF_IP_GEN5 * INST_UNIT_GEN5,
    AVC_ILDB_CHILD_Y_ILDB_MBAFF_IP_GEN5 * INST_UNIT_GEN5,
    AVC_ILDB_ROOT_UV_ILDB_MBAFF_IP_GEN5 * INST_UNIT_GEN5,
    AVC_ILDB_CHILD_UV_ILDB_MBAFF_IP_GEN5 * INST_UNIT_GEN5
};

struct avc_ildb_root_input
{
    unsigned int blocks_per_row : 16;
    unsigned int blocks_per_column : 16;

    unsigned int picture_type : 16;
    unsigned int max_concurrent_threads : 16;

    unsigned int debug_field : 16;
    unsigned int mbaff_frame_flag : 1;
    unsigned int bottom_field_flag : 1;
    unsigned int control_data_expansion_flag : 1;
    unsigned int chroma_format : 1;
    unsigned int pad0 : 12;

    unsigned int ramp_constant_0;
    
    unsigned int ramp_constant_1;

    int constant_0 : 8;
    int constant_1 : 8;
    int pad1 : 16;

    unsigned int pad2;
    unsigned int pad3;
};

#define NUM_AVC_ILDB_INTERFACES ARRAY_ELEMS(avc_ildb_kernel_offset_gen4)
static unsigned long *avc_ildb_kernel_offset = NULL;

static void
i965_avc_ildb_surface_state(VADriverContextP ctx,
                            struct decode_state *decode_state,
                            struct i965_h264_context *i965_h264_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_avc_ildb_context *avc_ildb_context = &i965_h264_context->avc_ildb_context;
    struct i965_surface_state *ss;
    struct object_surface *obj_surface;
    VAPictureParameterBufferH264 *pic_param;
    VAPictureH264 *va_pic;
    dri_bo *bo;
    int i;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferH264 *)decode_state->pic_param->buffer;
    va_pic = &pic_param->CurrPic;
    assert(!(va_pic->flags & VA_PICTURE_H264_INVALID));
    obj_surface = SURFACE(va_pic->picture_id);
    assert(obj_surface);

    avc_ildb_context->surface[SURFACE_EDGE_CONTROL_DATA].s_bo = i965_h264_context->avc_ildb_data.bo;
    dri_bo_reference(avc_ildb_context->surface[SURFACE_EDGE_CONTROL_DATA].s_bo);
    avc_ildb_context->surface[SURFACE_EDGE_CONTROL_DATA].offset = 0;
    avc_ildb_context->surface[SURFACE_EDGE_CONTROL_DATA].surface_type = I965_SURFACE_BUFFER;
    avc_ildb_context->surface[SURFACE_EDGE_CONTROL_DATA].width = ((avc_ildb_context->mbs_per_picture * EDGE_CONTROL_DATA_IN_DWS - 1) & 0x7f);
    avc_ildb_context->surface[SURFACE_EDGE_CONTROL_DATA].height = (((avc_ildb_context->mbs_per_picture * EDGE_CONTROL_DATA_IN_DWS - 1) >> 7) & 0x1fff);
    avc_ildb_context->surface[SURFACE_EDGE_CONTROL_DATA].depth = (((avc_ildb_context->mbs_per_picture * EDGE_CONTROL_DATA_IN_DWS - 1) >> 20) & 0x7f);
    avc_ildb_context->surface[SURFACE_EDGE_CONTROL_DATA].pitch = EDGE_CONTROL_DATA_IN_BTYES - 1;
    avc_ildb_context->surface[SURFACE_EDGE_CONTROL_DATA].is_target = 0;
    
    avc_ildb_context->surface[SURFACE_SRC_Y].s_bo = obj_surface->bo;
    dri_bo_reference(avc_ildb_context->surface[SURFACE_SRC_Y].s_bo);
    avc_ildb_context->surface[SURFACE_SRC_Y].offset = 0;
    avc_ildb_context->surface[SURFACE_SRC_Y].surface_type = I965_SURFACE_2D;
    avc_ildb_context->surface[SURFACE_SRC_Y].format = I965_SURFACEFORMAT_R8_SINT;
    avc_ildb_context->surface[SURFACE_SRC_Y].width = obj_surface->width / 4 - 1;
    avc_ildb_context->surface[SURFACE_SRC_Y].height = obj_surface->height - 1;
    avc_ildb_context->surface[SURFACE_SRC_Y].depth = 0;
    avc_ildb_context->surface[SURFACE_SRC_Y].pitch = obj_surface->width - 1;
    avc_ildb_context->surface[SURFACE_SRC_Y].vert_line_stride = !!(va_pic->flags & (VA_PICTURE_H264_TOP_FIELD | VA_PICTURE_H264_BOTTOM_FIELD));
    avc_ildb_context->surface[SURFACE_SRC_Y].vert_line_stride_ofs = !!(va_pic->flags & VA_PICTURE_H264_BOTTOM_FIELD);
    avc_ildb_context->surface[SURFACE_SRC_Y].is_target = 0;
    
    avc_ildb_context->surface[SURFACE_SRC_UV].s_bo = obj_surface->bo;
    dri_bo_reference(avc_ildb_context->surface[SURFACE_SRC_UV].s_bo);
    avc_ildb_context->surface[SURFACE_SRC_UV].offset = obj_surface->width * obj_surface->height;
    avc_ildb_context->surface[SURFACE_SRC_UV].surface_type = I965_SURFACE_2D;
    avc_ildb_context->surface[SURFACE_SRC_UV].format = I965_SURFACEFORMAT_R8G8_SINT;
    avc_ildb_context->surface[SURFACE_SRC_UV].width = obj_surface->width / 4 - 1;
    avc_ildb_context->surface[SURFACE_SRC_UV].height = obj_surface->height / 2 - 1;
    avc_ildb_context->surface[SURFACE_SRC_UV].depth = 0;
    avc_ildb_context->surface[SURFACE_SRC_UV].pitch = obj_surface->width - 1;
    avc_ildb_context->surface[SURFACE_SRC_UV].vert_line_stride = !!(va_pic->flags & (VA_PICTURE_H264_TOP_FIELD | VA_PICTURE_H264_BOTTOM_FIELD));
    avc_ildb_context->surface[SURFACE_SRC_UV].vert_line_stride_ofs = !!(va_pic->flags & VA_PICTURE_H264_BOTTOM_FIELD);
    avc_ildb_context->surface[SURFACE_SRC_UV].is_target = 0;

    avc_ildb_context->surface[SURFACE_DEST_Y].s_bo = obj_surface->bo;
    dri_bo_reference(avc_ildb_context->surface[SURFACE_DEST_Y].s_bo);
    avc_ildb_context->surface[SURFACE_DEST_Y].offset = 0;
    avc_ildb_context->surface[SURFACE_DEST_Y].surface_type = I965_SURFACE_2D;
    avc_ildb_context->surface[SURFACE_DEST_Y].format = I965_SURFACEFORMAT_R8_SINT;
    avc_ildb_context->surface[SURFACE_DEST_Y].width = obj_surface->width / 4 - 1;
    avc_ildb_context->surface[SURFACE_DEST_Y].height = obj_surface->height - 1;
    avc_ildb_context->surface[SURFACE_DEST_Y].depth = 0;
    avc_ildb_context->surface[SURFACE_DEST_Y].pitch = obj_surface->width - 1;
    avc_ildb_context->surface[SURFACE_DEST_Y].vert_line_stride = !!(va_pic->flags & (VA_PICTURE_H264_TOP_FIELD | VA_PICTURE_H264_BOTTOM_FIELD));
    avc_ildb_context->surface[SURFACE_DEST_Y].vert_line_stride_ofs = !!(va_pic->flags & VA_PICTURE_H264_BOTTOM_FIELD);
    avc_ildb_context->surface[SURFACE_DEST_Y].is_target = 1;

    avc_ildb_context->surface[SURFACE_DEST_UV].s_bo = obj_surface->bo;
    dri_bo_reference(avc_ildb_context->surface[SURFACE_DEST_UV].s_bo);
    avc_ildb_context->surface[SURFACE_DEST_UV].offset = obj_surface->width * obj_surface->height;
    avc_ildb_context->surface[SURFACE_DEST_UV].surface_type = I965_SURFACE_2D;
    avc_ildb_context->surface[SURFACE_DEST_UV].format = I965_SURFACEFORMAT_R8G8_SINT;
    avc_ildb_context->surface[SURFACE_DEST_UV].width = obj_surface->width / 4 - 1;
    avc_ildb_context->surface[SURFACE_DEST_UV].height = obj_surface->height / 2 - 1;
    avc_ildb_context->surface[SURFACE_DEST_UV].depth = 0;
    avc_ildb_context->surface[SURFACE_DEST_UV].pitch = obj_surface->width - 1;
    avc_ildb_context->surface[SURFACE_DEST_UV].vert_line_stride = !!(va_pic->flags & (VA_PICTURE_H264_TOP_FIELD | VA_PICTURE_H264_BOTTOM_FIELD));
    avc_ildb_context->surface[SURFACE_DEST_UV].vert_line_stride_ofs = !!(va_pic->flags & VA_PICTURE_H264_BOTTOM_FIELD);
    avc_ildb_context->surface[SURFACE_DEST_UV].is_target = 1;

    for (i = 0; i < NUM_AVC_ILDB_SURFACES; i++) {
        bo = avc_ildb_context->surface[i].ss_bo;
        dri_bo_map(bo, 1);
        assert(bo->virtual);
        ss = bo->virtual;
        memset(ss, 0, sizeof(*ss));
        ss->ss0.surface_type = avc_ildb_context->surface[i].surface_type;
        ss->ss0.surface_format = avc_ildb_context->surface[i].format;
        ss->ss0.vert_line_stride = avc_ildb_context->surface[i].vert_line_stride;
        ss->ss0.vert_line_stride_ofs = avc_ildb_context->surface[i].vert_line_stride_ofs;
        ss->ss1.base_addr = avc_ildb_context->surface[i].s_bo->offset + avc_ildb_context->surface[i].offset;
        ss->ss2.width = avc_ildb_context->surface[i].width;
        ss->ss2.height = avc_ildb_context->surface[i].height;
        ss->ss3.depth = avc_ildb_context->surface[i].depth;
        ss->ss3.pitch = avc_ildb_context->surface[i].pitch;
        dri_bo_emit_reloc(bo,
                          I915_GEM_DOMAIN_RENDER, 
                          avc_ildb_context->surface[i].is_target ? I915_GEM_DOMAIN_RENDER : 0,
                          avc_ildb_context->surface[i].offset,
                          offsetof(struct i965_surface_state, ss1),
                          avc_ildb_context->surface[i].s_bo);
        dri_bo_unmap(bo);
    }
}

static void
i965_avc_ildb_binding_table(VADriverContextP ctx, struct i965_h264_context *i965_h264_context)
{
    struct i965_avc_ildb_context *avc_ildb_context = &i965_h264_context->avc_ildb_context;
    unsigned int *binding_table;
    dri_bo *bo = avc_ildb_context->binding_table.bo;
    int i;

    dri_bo_map(bo, 1);
    assert(bo->virtual);
    binding_table = bo->virtual;
    memset(binding_table, 0, bo->size);

    for (i = 0; i < NUM_AVC_ILDB_SURFACES; i++) {
        binding_table[i] = avc_ildb_context->surface[i].ss_bo->offset;
        dri_bo_emit_reloc(bo,
                          I915_GEM_DOMAIN_INSTRUCTION, 0,
                          0,
                          i * sizeof(*binding_table),
                          avc_ildb_context->surface[i].ss_bo);
    }

    dri_bo_unmap(bo);
}

static void
i965_avc_ildb_interface_descriptor_table(VADriverContextP ctx, struct i965_h264_context *i965_h264_context)
{
    struct i965_avc_ildb_context *avc_ildb_context = &i965_h264_context->avc_ildb_context;
    struct i965_interface_descriptor *desc;
    dri_bo *bo;
    int i;

    bo = avc_ildb_context->idrt.bo;
    dri_bo_map(bo, 1);
    assert(bo->virtual);
    desc = bo->virtual;

    for (i = 0; i < NUM_AVC_ILDB_INTERFACES; i++) {
        int kernel_offset = avc_ildb_kernel_offset[i];
        memset(desc, 0, sizeof(*desc));
        desc->desc0.grf_reg_blocks = 7; 
        desc->desc0.kernel_start_pointer = (i965_h264_context->avc_kernels[H264_AVC_COMBINED].bo->offset + kernel_offset) >> 6; /* reloc */
        desc->desc1.const_urb_entry_read_offset = 0;
        desc->desc1.const_urb_entry_read_len = ((i == AVC_ILDB_ROOT_Y_ILDB_FRAME ||
                                                 i == AVC_ILDB_ROOT_Y_ILDB_FIELD ||
                                                 i == AVC_ILDB_ROOT_Y_ILDB_MBAFF) ? 1 : 0);
        desc->desc3.binding_table_entry_count = 0;
        desc->desc3.binding_table_pointer = 
            avc_ildb_context->binding_table.bo->offset >> 5; /*reloc */

        dri_bo_emit_reloc(bo,
                          I915_GEM_DOMAIN_INSTRUCTION, 0,
                          desc->desc0.grf_reg_blocks + kernel_offset,
                          i * sizeof(*desc) + offsetof(struct i965_interface_descriptor, desc0),
                          i965_h264_context->avc_kernels[H264_AVC_COMBINED].bo);

        dri_bo_emit_reloc(bo,
                          I915_GEM_DOMAIN_INSTRUCTION, 0,
                          desc->desc3.binding_table_entry_count,
                          i * sizeof(*desc) + offsetof(struct i965_interface_descriptor, desc3),
                          avc_ildb_context->binding_table.bo);
        desc++;
    }

    dri_bo_unmap(bo);
}

static void
i965_avc_ildb_vfe_state(VADriverContextP ctx, struct i965_h264_context *i965_h264_context)
{
    struct i965_avc_ildb_context *avc_ildb_context = &i965_h264_context->avc_ildb_context;
    struct i965_vfe_state *vfe_state;
    dri_bo *bo;

    bo = avc_ildb_context->vfe_state.bo;
    dri_bo_map(bo, 1);
    assert(bo->virtual);
    vfe_state = bo->virtual;
    memset(vfe_state, 0, sizeof(*vfe_state));
    vfe_state->vfe1.max_threads = 0;
    vfe_state->vfe1.urb_entry_alloc_size = avc_ildb_context->urb.size_vfe_entry - 1;
    vfe_state->vfe1.num_urb_entries = avc_ildb_context->urb.num_vfe_entries;
    vfe_state->vfe1.vfe_mode = VFE_GENERIC_MODE;
    vfe_state->vfe1.children_present = 1;
    vfe_state->vfe2.interface_descriptor_base = 
        avc_ildb_context->idrt.bo->offset >> 4; /* reloc */
    dri_bo_emit_reloc(bo,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      0,
                      offsetof(struct i965_vfe_state, vfe2),
                      avc_ildb_context->idrt.bo);
    dri_bo_unmap(bo);
}

static void
i965_avc_ildb_upload_constants(VADriverContextP ctx,
                               struct decode_state *decode_state,
                               struct i965_h264_context *i965_h264_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_avc_ildb_context *avc_ildb_context = &i965_h264_context->avc_ildb_context;
    VAPictureParameterBufferH264 *pic_param;
    struct avc_ildb_root_input *root_input;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferH264 *)decode_state->pic_param->buffer;

    dri_bo_map(avc_ildb_context->curbe.bo, 1);
    assert(avc_ildb_context->curbe.bo->virtual);
    root_input = avc_ildb_context->curbe.bo->virtual;

    if (IS_IRONLAKE(i965->intel.device_id)) {
        root_input->max_concurrent_threads = 76; /* 72 - 2 + 8 - 2 */
    } else {
        root_input->max_concurrent_threads = 54; /* 50 - 2 + 8 - 2 */
    }

    if (pic_param->pic_fields.bits.field_pic_flag)
        root_input->picture_type = PICTURE_FIELD;
    else {
        if (pic_param->seq_fields.bits.mb_adaptive_frame_field_flag)
            root_input->picture_type = PICTURE_MBAFF;
        else
            root_input->picture_type = PICTURE_FRAME;
    }

    avc_ildb_context->picture_type = root_input->picture_type;
    root_input->blocks_per_row = pic_param->picture_width_in_mbs_minus1 + 1;
    root_input->blocks_per_column = (pic_param->picture_height_in_mbs_minus1 + 1) / 
        (1 + (root_input->picture_type != PICTURE_FRAME));
    avc_ildb_context->mbs_per_picture = (pic_param->picture_width_in_mbs_minus1 + 1) *
        (pic_param->picture_height_in_mbs_minus1 + 1);
    
    root_input->mbaff_frame_flag = (root_input->picture_type == PICTURE_MBAFF);
    root_input->bottom_field_flag = !!(pic_param->CurrPic.flags & VA_PICTURE_H264_BOTTOM_FIELD);
    root_input->control_data_expansion_flag = 1; /* Always 1 on G4x+ */
    root_input->chroma_format = (pic_param->seq_fields.bits.chroma_format_idc != 1); /* 0=4:0:0, 1=4:2:0 */
    
    root_input->ramp_constant_0 = 0x03020100;
    
    root_input->ramp_constant_1 = 0x07060504;

    root_input->constant_0 = -2;
    root_input->constant_1 = 1;

    dri_bo_unmap(avc_ildb_context->curbe.bo);
}

static void
i965_avc_ildb_states_setup(VADriverContextP ctx,
                           struct decode_state *decode_state,
                           struct i965_h264_context *i965_h264_context)
{
    i965_avc_ildb_surface_state(ctx, decode_state, i965_h264_context);
    i965_avc_ildb_binding_table(ctx, i965_h264_context);
    i965_avc_ildb_interface_descriptor_table(ctx, i965_h264_context);
    i965_avc_ildb_vfe_state(ctx, i965_h264_context);
    i965_avc_ildb_upload_constants(ctx, decode_state, i965_h264_context);
}

static void
i965_avc_ildb_pipeline_select(VADriverContextP ctx)
{
    BEGIN_BATCH(ctx, 1);
    OUT_BATCH(ctx, CMD_PIPELINE_SELECT | PIPELINE_SELECT_MEDIA);
    ADVANCE_BATCH(ctx);
}

static void
i965_avc_ildb_urb_layout(VADriverContextP ctx, struct i965_h264_context *i965_h264_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_avc_ildb_context *avc_ildb_context = &i965_h264_context->avc_ildb_context;

    unsigned int vfe_fence, cs_fence;

    vfe_fence = avc_ildb_context->urb.cs_start;
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
i965_avc_ildb_state_base_address(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx); 

    if (IS_IRONLAKE(i965->intel.device_id)) {
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
    } else {
        BEGIN_BATCH(ctx, 6);
        OUT_BATCH(ctx, CMD_STATE_BASE_ADDRESS | 4);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        ADVANCE_BATCH(ctx);
    }
}

static void
i965_avc_ildb_state_pointers(VADriverContextP ctx, struct i965_h264_context *i965_h264_context)
{
    struct i965_avc_ildb_context *avc_ildb_context = &i965_h264_context->avc_ildb_context;

    BEGIN_BATCH(ctx, 3);
    OUT_BATCH(ctx, CMD_MEDIA_STATE_POINTERS | 1);
    OUT_BATCH(ctx, 0);
    OUT_RELOC(ctx, avc_ildb_context->vfe_state.bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
    ADVANCE_BATCH(ctx);
}

static void 
i965_avc_ildb_cs_urb_layout(VADriverContextP ctx, struct i965_h264_context *i965_h264_context)
{
    struct i965_avc_ildb_context *avc_ildb_context = &i965_h264_context->avc_ildb_context;

    BEGIN_BATCH(ctx, 2);
    OUT_BATCH(ctx, CMD_CS_URB_STATE | 0);
    OUT_BATCH(ctx,
              ((avc_ildb_context->urb.size_cs_entry - 1) << 4) |     /* URB Entry Allocation Size */
              (avc_ildb_context->urb.num_cs_entries << 0));          /* Number of URB Entries */
    ADVANCE_BATCH(ctx);
}

static void
i965_avc_ildb_constant_buffer(VADriverContextP ctx, struct i965_h264_context *i965_h264_context)
{
    struct i965_avc_ildb_context *avc_ildb_context = &i965_h264_context->avc_ildb_context;

    BEGIN_BATCH(ctx, 2);
    OUT_BATCH(ctx, CMD_CONSTANT_BUFFER | (1 << 8) | (2 - 2));
    OUT_RELOC(ctx, avc_ildb_context->curbe.bo,
              I915_GEM_DOMAIN_INSTRUCTION, 0,
              avc_ildb_context->urb.size_cs_entry - 1);
    ADVANCE_BATCH(ctx);    
}

static void
i965_avc_ildb_objects(VADriverContextP ctx, struct i965_h264_context *i965_h264_context)
{
    struct i965_avc_ildb_context *avc_ildb_context = &i965_h264_context->avc_ildb_context;

    BEGIN_BATCH(ctx, 6);
    OUT_BATCH(ctx, CMD_MEDIA_OBJECT | 4);

    switch (avc_ildb_context->picture_type) {
    case PICTURE_FRAME:
        OUT_BATCH(ctx, AVC_ILDB_ROOT_Y_ILDB_FRAME);
        break;

    case PICTURE_FIELD:
        OUT_BATCH(ctx, AVC_ILDB_ROOT_Y_ILDB_FIELD);
        break;

    case PICTURE_MBAFF:
        OUT_BATCH(ctx, AVC_ILDB_ROOT_Y_ILDB_MBAFF);
        break;

    default:
        assert(0);
        OUT_BATCH(ctx, 0);
        break;
    }

    OUT_BATCH(ctx, 0); /* no indirect data */
    OUT_BATCH(ctx, 0);
    OUT_BATCH(ctx, 0);
    OUT_BATCH(ctx, 0);
    ADVANCE_BATCH(ctx);
}

static void
i965_avc_ildb_pipeline_setup(VADriverContextP ctx, struct i965_h264_context *i965_h264_context)
{
    intel_batchbuffer_emit_mi_flush(ctx);
    i965_avc_ildb_pipeline_select(ctx);
    i965_avc_ildb_state_base_address(ctx);
    i965_avc_ildb_state_pointers(ctx, i965_h264_context);
    i965_avc_ildb_urb_layout(ctx, i965_h264_context);
    i965_avc_ildb_cs_urb_layout(ctx, i965_h264_context);
    i965_avc_ildb_constant_buffer(ctx, i965_h264_context);
    i965_avc_ildb_objects(ctx, i965_h264_context);
}

void
i965_avc_ildb(VADriverContextP ctx, struct decode_state *decode_state, void *h264_context)
{
    struct i965_h264_context *i965_h264_context = (struct i965_h264_context *)h264_context;

    if (i965_h264_context->enable_avc_ildb) {
        i965_avc_ildb_states_setup(ctx, decode_state, i965_h264_context);
        i965_avc_ildb_pipeline_setup(ctx, i965_h264_context);
    }
}

void
i965_avc_ildb_decode_init(VADriverContextP ctx, void *h264_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_h264_context *i965_h264_context = (struct i965_h264_context *)h264_context;
    struct i965_avc_ildb_context *avc_ildb_context = &i965_h264_context->avc_ildb_context;;
    dri_bo *bo;
    int i;

    dri_bo_unreference(avc_ildb_context->curbe.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "constant buffer",
                      4096, 64);
    assert(bo);
    avc_ildb_context->curbe.bo = bo;

    dri_bo_unreference(avc_ildb_context->binding_table.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "binding table",
                      NUM_AVC_ILDB_SURFACES * sizeof(unsigned int), 32);
    assert(bo);
    avc_ildb_context->binding_table.bo = bo;

    dri_bo_unreference(avc_ildb_context->idrt.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "interface discriptor", 
                      NUM_AVC_ILDB_INTERFACES * sizeof(struct i965_interface_descriptor), 16);
    assert(bo);
    avc_ildb_context->idrt.bo = bo;

    dri_bo_unreference(avc_ildb_context->vfe_state.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "vfe state", 
                      sizeof(struct i965_vfe_state), 32);
    assert(bo);
    avc_ildb_context->vfe_state.bo = bo;

    avc_ildb_context->urb.num_vfe_entries = 1;
    avc_ildb_context->urb.size_vfe_entry = 640;
    avc_ildb_context->urb.num_cs_entries = 1;
    avc_ildb_context->urb.size_cs_entry = 1;
    avc_ildb_context->urb.vfe_start = 0;
    avc_ildb_context->urb.cs_start = avc_ildb_context->urb.vfe_start + 
        avc_ildb_context->urb.num_vfe_entries * avc_ildb_context->urb.size_vfe_entry;
    assert(avc_ildb_context->urb.cs_start + 
           avc_ildb_context->urb.num_cs_entries * avc_ildb_context->urb.size_cs_entry <= URB_SIZE((&i965->intel)));

    for (i = 0; i < NUM_AVC_ILDB_SURFACES; i++) {
        dri_bo_unreference(avc_ildb_context->surface[i].s_bo);
        avc_ildb_context->surface[i].s_bo = NULL;

        dri_bo_unreference(avc_ildb_context->surface[i].ss_bo);
        bo = dri_bo_alloc(i965->intel.bufmgr, 
                          "surface state", 
                          sizeof(struct i965_surface_state), 32);
        assert(bo);
        avc_ildb_context->surface[i].ss_bo = bo;
    }

    /* kernel offset */
    assert(NUM_AVC_ILDB_INTERFACES == ARRAY_ELEMS(avc_ildb_kernel_offset_gen5));

    if (IS_IRONLAKE(i965->intel.device_id)) {
        avc_ildb_kernel_offset = avc_ildb_kernel_offset_gen5;
    } else {
        avc_ildb_kernel_offset = avc_ildb_kernel_offset_gen4;
    }
}

Bool 
i965_avc_ildb_ternimate(struct i965_avc_ildb_context *avc_ildb_context)
{
    int i;

    dri_bo_unreference(avc_ildb_context->curbe.bo);
    avc_ildb_context->curbe.bo = NULL;

    dri_bo_unreference(avc_ildb_context->binding_table.bo);
    avc_ildb_context->binding_table.bo = NULL;

    dri_bo_unreference(avc_ildb_context->idrt.bo);
    avc_ildb_context->idrt.bo = NULL;

    dri_bo_unreference(avc_ildb_context->vfe_state.bo);
    avc_ildb_context->vfe_state.bo = NULL;

    for (i = 0; i < NUM_AVC_ILDB_SURFACES; i++) {
        dri_bo_unreference(avc_ildb_context->surface[i].ss_bo);
        avc_ildb_context->surface[i].ss_bo = NULL;

        dri_bo_unreference(avc_ildb_context->surface[i].s_bo);
        avc_ildb_context->surface[i].s_bo = NULL;
    }

    return True;
}
