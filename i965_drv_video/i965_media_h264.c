#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "va_backend.h"

#include "intel_batchbuffer.h"
#include "intel_driver.h"

#include "i965_defines.h"
#include "i965_drv_video.h"
#include "i965_media.h"
#include "i965_media_h264.h"

enum {
    INTRA_16X16 = 0,
    INTRA_8X8,
    INTRA_4X4,
    INTRA_PCM,
    FRAMEMB_MOTION,
    FIELDMB_MOTION,
    MBAFF_MOTION,
};

struct intra_kernel_header
{
    /* R1.0 */
    unsigned char intra_4x4_luma_mode_0_offset;
    unsigned char intra_4x4_luma_mode_1_offset;
    unsigned char intra_4x4_luma_mode_2_offset;
    unsigned char intra_4x4_luma_mode_3_offset;
    /* R1.1 */
    unsigned char intra_4x4_luma_mode_4_offset;
    unsigned char intra_4x4_luma_mode_5_offset;
    unsigned char intra_4x4_luma_mode_6_offset;
    unsigned char intra_4x4_luma_mode_7_offset;
    /* R1.2 */
    unsigned char intra_4x4_luma_mode_8_offset;
    unsigned char pad0;
    unsigned short top_reference_offset;
    /* R1.3 */
    unsigned char intra_8x8_luma_mode_0_offset;
    unsigned char intra_8x8_luma_mode_1_offset;
    unsigned char intra_8x8_luma_mode_2_offset;
    unsigned char intra_8x8_luma_mode_3_offset;
    /* R1.4 */
    unsigned char intra_8x8_luma_mode_4_offset;
    unsigned char intra_8x8_luma_mode_5_offset;
    unsigned char intra_8x8_luma_mode_6_offset;
    unsigned char intra_8x8_luma_mode_7_offset;
    /* R1.5 */
    unsigned char intra_8x8_luma_mode_8_offset;
    unsigned char pad1;
    unsigned short const_reverse_data_transfer_intra_8x8;
    /* R1.6 */
    unsigned char intra_16x16_luma_mode_0_offset;
    unsigned char intra_16x16_luma_mode_1_offset;
    unsigned char intra_16x16_luma_mode_2_offset;
    unsigned char intra_16x16_luma_mode_3_offset;
    /* R1.7 */
    unsigned char intra_chroma_mode_0_offset;
    unsigned char intra_chroma_mode_1_offset;
    unsigned char intra_chroma_mode_2_offset;
    unsigned char intra_chroma_mode_3_offset;
    /* R2.0 */
    unsigned int const_intra_16x16_plane_0;
    /* R2.1 */
    unsigned int const_intra_16x16_chroma_plane_0;
    /* R2.2 */
    unsigned int const_intra_16x16_chroma_plane_1;
    /* R2.3 */
    unsigned int const_intra_16x16_plane_1;
    /* R2.4 */
    unsigned int left_shift_count_reverse_dw_ordering;
    /* R2.5 */
    unsigned int const_reverse_data_transfer_intra_4x4;
    /* R2.6 */
    unsigned int intra_4x4_pred_mode_offset;
};

struct inter_kernel_header
{
    unsigned short weight_offset;
    unsigned char weight_offset_flag;
    unsigned char pad0;
};

#include "shaders/h264/mc/export.inc"
static unsigned long avc_mc_kernel_offset_gen4[] = {
    INTRA_16x16_IP * INST_UNIT_GEN4,
    INTRA_8x8_IP * INST_UNIT_GEN4,
    INTRA_4x4_IP * INST_UNIT_GEN4,
    INTRA_PCM_IP * INST_UNIT_GEN4,
    FRAME_MB_IP * INST_UNIT_GEN4,
    FIELD_MB_IP * INST_UNIT_GEN4,
    MBAFF_MB_IP * INST_UNIT_GEN4
};

struct intra_kernel_header intra_kernel_header_gen4 = {
    0,
    (INTRA_4X4_HORIZONTAL_IP - INTRA_4X4_VERTICAL_IP),
    (INTRA_4X4_DC_IP - INTRA_4X4_VERTICAL_IP),
    (INTRA_4X4_DIAG_DOWN_LEFT_IP - INTRA_4X4_VERTICAL_IP),

    (INTRA_4X4_DIAG_DOWN_RIGHT_IP - INTRA_4X4_VERTICAL_IP),
    (INTRA_4X4_VERT_RIGHT_IP - INTRA_4X4_VERTICAL_IP),
    (INTRA_4X4_HOR_DOWN_IP - INTRA_4X4_VERTICAL_IP),
    (INTRA_4X4_VERT_LEFT_IP - INTRA_4X4_VERTICAL_IP),

    (INTRA_4X4_HOR_UP_IP - INTRA_4X4_VERTICAL_IP),
    0,
    0xFFFC,

    0,
    (INTRA_8X8_HORIZONTAL_IP - INTRA_8X8_VERTICAL_IP),
    (INTRA_8X8_DC_IP - INTRA_8X8_VERTICAL_IP),
    (INTRA_8X8_DIAG_DOWN_LEFT_IP - INTRA_8X8_VERTICAL_IP),

    (INTRA_8X8_DIAG_DOWN_RIGHT_IP - INTRA_8X8_VERTICAL_IP),
    (INTRA_8X8_VERT_RIGHT_IP - INTRA_8X8_VERTICAL_IP),
    (INTRA_8X8_HOR_DOWN_IP - INTRA_8X8_VERTICAL_IP),
    (INTRA_8X8_VERT_LEFT_IP - INTRA_8X8_VERTICAL_IP),

    (INTRA_8X8_HOR_UP_IP - INTRA_8X8_VERTICAL_IP),
    0,
    0x0001,

    0,
    (INTRA_16x16_HORIZONTAL_IP - INTRA_16x16_VERTICAL_IP),
    (INTRA_16x16_DC_IP - INTRA_16x16_VERTICAL_IP),
    (INTRA_16x16_PLANE_IP - INTRA_16x16_VERTICAL_IP),

    0,
    (INTRA_CHROMA_HORIZONTAL_IP - INTRA_CHROMA_DC_IP),
    (INTRA_CHROMA_VERTICAL_IP - INTRA_CHROMA_DC_IP),
    (INTRA_Chroma_PLANE_IP - INTRA_CHROMA_DC_IP),

    0xFCFBFAF9,

    0x00FFFEFD,

    0x04030201,

    0x08070605,

    0x18100800,

    0x00020406,

    (intra_Pred_4x4_Y_IP - ADD_ERROR_SB3_IP) * 0x1000000 + 
    (intra_Pred_4x4_Y_IP - ADD_ERROR_SB2_IP) * 0x10000 + 
    (intra_Pred_4x4_Y_IP - ADD_ERROR_SB1_IP) * 0x100 + 
    (intra_Pred_4x4_Y_IP - ADD_ERROR_SB0_IP)
};

static uint32_t h264_avc_combined_gen4[][4] = {
#include "shaders/h264/mc/avc_mc.g4b"
};

static uint32_t h264_avc_null_gen4[][4] = {
#include "shaders/h264/mc/null.g4b"
};

static struct media_kernel h264_avc_kernels_gen4[] = {
    {
        "AVC combined kernel",
        H264_AVC_COMBINED,
        h264_avc_combined_gen4,
        sizeof(h264_avc_combined_gen4),
        NULL
    },

    {
        "NULL kernel",
        H264_AVC_NULL,
        h264_avc_null_gen4,
        sizeof(h264_avc_null_gen4),
        NULL
    }
};

/* On Ironlake */
#include "shaders/h264/mc/export.inc.gen5"
static unsigned long avc_mc_kernel_offset_gen5[] = {
    INTRA_16x16_IP_GEN5 * INST_UNIT_GEN5,
    INTRA_8x8_IP_GEN5 * INST_UNIT_GEN5,
    INTRA_4x4_IP_GEN5 * INST_UNIT_GEN5,
    INTRA_PCM_IP_GEN5 * INST_UNIT_GEN5,
    FRAME_MB_IP_GEN5 * INST_UNIT_GEN5,
    FIELD_MB_IP_GEN5 * INST_UNIT_GEN5,
    MBAFF_MB_IP_GEN5 * INST_UNIT_GEN5
};

struct intra_kernel_header intra_kernel_header_gen5 = {
    0,
    (INTRA_4X4_HORIZONTAL_IP_GEN5 - INTRA_4X4_VERTICAL_IP_GEN5),
    (INTRA_4X4_DC_IP_GEN5 - INTRA_4X4_VERTICAL_IP_GEN5),
    (INTRA_4X4_DIAG_DOWN_LEFT_IP_GEN5 - INTRA_4X4_VERTICAL_IP_GEN5),

    (INTRA_4X4_DIAG_DOWN_RIGHT_IP_GEN5 - INTRA_4X4_VERTICAL_IP_GEN5),
    (INTRA_4X4_VERT_RIGHT_IP_GEN5 - INTRA_4X4_VERTICAL_IP_GEN5),
    (INTRA_4X4_HOR_DOWN_IP_GEN5 - INTRA_4X4_VERTICAL_IP_GEN5),
    (INTRA_4X4_VERT_LEFT_IP_GEN5 - INTRA_4X4_VERTICAL_IP_GEN5),

    (INTRA_4X4_HOR_UP_IP_GEN5 - INTRA_4X4_VERTICAL_IP_GEN5),
    0,
    0xFFFC,

    0,
    (INTRA_8X8_HORIZONTAL_IP_GEN5 - INTRA_8X8_VERTICAL_IP_GEN5),
    (INTRA_8X8_DC_IP_GEN5 - INTRA_8X8_VERTICAL_IP_GEN5),
    (INTRA_8X8_DIAG_DOWN_LEFT_IP_GEN5 - INTRA_8X8_VERTICAL_IP_GEN5),

    (INTRA_8X8_DIAG_DOWN_RIGHT_IP_GEN5 - INTRA_8X8_VERTICAL_IP_GEN5),
    (INTRA_8X8_VERT_RIGHT_IP_GEN5 - INTRA_8X8_VERTICAL_IP_GEN5),
    (INTRA_8X8_HOR_DOWN_IP_GEN5 - INTRA_8X8_VERTICAL_IP_GEN5),
    (INTRA_8X8_VERT_LEFT_IP_GEN5 - INTRA_8X8_VERTICAL_IP_GEN5),

    (INTRA_8X8_HOR_UP_IP_GEN5 - INTRA_8X8_VERTICAL_IP_GEN5),
    0,
    0x0001,

    0,
    (INTRA_16x16_HORIZONTAL_IP_GEN5 - INTRA_16x16_VERTICAL_IP_GEN5),
    (INTRA_16x16_DC_IP_GEN5 - INTRA_16x16_VERTICAL_IP_GEN5),
    (INTRA_16x16_PLANE_IP_GEN5 - INTRA_16x16_VERTICAL_IP_GEN5),

    0,
    (INTRA_CHROMA_HORIZONTAL_IP_GEN5 - INTRA_CHROMA_DC_IP_GEN5),
    (INTRA_CHROMA_VERTICAL_IP_GEN5 - INTRA_CHROMA_DC_IP_GEN5),
    (INTRA_Chroma_PLANE_IP_GEN5 - INTRA_CHROMA_DC_IP_GEN5),

    0xFCFBFAF9,

    0x00FFFEFD,

    0x04030201,

    0x08070605,

    0x18100800,

    0x00020406,

    (intra_Pred_4x4_Y_IP_GEN5 - ADD_ERROR_SB3_IP_GEN5) * 0x1000000 + 
    (intra_Pred_4x4_Y_IP_GEN5 - ADD_ERROR_SB2_IP_GEN5) * 0x10000 + 
    (intra_Pred_4x4_Y_IP_GEN5 - ADD_ERROR_SB1_IP_GEN5) * 0x100 + 
    (intra_Pred_4x4_Y_IP_GEN5 - ADD_ERROR_SB0_IP_GEN5)
};

static uint32_t h264_avc_combined_gen5[][4] = {
#include "shaders/h264/mc/avc_mc.g4b.gen5"
};

static uint32_t h264_avc_null_gen5[][4] = {
#include "shaders/h264/mc/null.g4b.gen5"
};

static struct media_kernel h264_avc_kernels_gen5[] = {
    {
        "AVC combined kernel",
        H264_AVC_COMBINED,
        h264_avc_combined_gen5,
        sizeof(h264_avc_combined_gen5),
        NULL
    },

    {
        "NULL kernel",
        H264_AVC_NULL,
        h264_avc_null_gen5,
        sizeof(h264_avc_null_gen5),
        NULL
    }
};

#define NUM_H264_AVC_KERNELS (sizeof(h264_avc_kernels_gen4) / sizeof(h264_avc_kernels_gen4[0]))
struct media_kernel *h264_avc_kernels = NULL;

#define NUM_AVC_MC_INTERFACES (sizeof(avc_mc_kernel_offset_gen4) / sizeof(avc_mc_kernel_offset_gen4[0]))
static unsigned long *avc_mc_kernel_offset = NULL;

static struct intra_kernel_header *intra_kernel_header = NULL;

static void
i965_media_h264_surface_state(VADriverContextP ctx, 
                              int index,
                              struct object_surface *obj_surface,
                              unsigned long offset, 
                              int w, int h, int pitch,
                              Bool is_dst,
                              int vert_line_stride,
                              int vert_line_stride_ofs,
                              int format)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);  
    struct i965_media_state *media_state = &i965->media_state;
    struct i965_surface_state *ss;
    dri_bo *bo;
    uint32_t write_domain, read_domain;

    bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "surface state", 
                      sizeof(struct i965_surface_state), 32);
    assert(bo);
    dri_bo_map(bo, 1);
    assert(bo->virtual);
    ss = bo->virtual;
    memset(ss, 0, sizeof(*ss));
    ss->ss0.surface_type = I965_SURFACE_2D;
    ss->ss0.surface_format = format;
    ss->ss0.vert_line_stride = vert_line_stride;
    ss->ss0.vert_line_stride_ofs = vert_line_stride_ofs;
    ss->ss1.base_addr = obj_surface->bo->offset + offset;
    ss->ss2.width = w - 1;
    ss->ss2.height = h - 1;
    ss->ss3.pitch = pitch - 1;

    if (is_dst) {
        write_domain = I915_GEM_DOMAIN_RENDER;
        read_domain = I915_GEM_DOMAIN_RENDER;
    } else {
        write_domain = 0;
        read_domain = I915_GEM_DOMAIN_SAMPLER;
    }

    dri_bo_emit_reloc(bo,
                      read_domain, write_domain,
                      offset,
                      offsetof(struct i965_surface_state, ss1),
                      obj_surface->bo);
    dri_bo_unmap(bo);

    assert(index < MAX_MEDIA_SURFACES);
    media_state->surface_state[index].bo = bo;
}

static void 
i965_media_h264_surfaces_setup(VADriverContextP ctx, 
                               struct decode_state *decode_state)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);  
    struct i965_media_state *media_state = &i965->media_state;
    struct i965_h264_context *i965_h264_context;
    struct object_surface *obj_surface;
    VAPictureParameterBufferH264 *pic_param;
    VAPictureH264 *va_pic;
    int i, j, w, h;
    int field_picture;

    assert(media_state->private_context);
    i965_h264_context = (struct i965_h264_context *)media_state->private_context;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferH264 *)decode_state->pic_param->buffer;

    /* Target Picture */
    va_pic = &pic_param->CurrPic;
    assert(!(va_pic->flags & VA_PICTURE_H264_INVALID));
    obj_surface = SURFACE(va_pic->picture_id);
    assert(obj_surface);
    w = obj_surface->width;
    h = obj_surface->height;
    field_picture = !!(va_pic->flags & (VA_PICTURE_H264_TOP_FIELD | VA_PICTURE_H264_BOTTOM_FIELD));
    i965_media_h264_surface_state(ctx, 0, obj_surface,
                                  0, w / 4, h / (1 + field_picture), w,
                                  1, 
                                  field_picture,
                                  !!(va_pic->flags & VA_PICTURE_H264_BOTTOM_FIELD),
                                  I965_SURFACEFORMAT_R8_SINT); /* Y */
    i965_media_h264_surface_state(ctx, 1, obj_surface,
                                  w * h, w / 4, h / 2 / (1 + field_picture), w,
                                  1, 
                                  field_picture,
                                  !!(va_pic->flags & VA_PICTURE_H264_BOTTOM_FIELD),
                                  I965_SURFACEFORMAT_R8G8_SINT);  /* INTERLEAVED U/V */

    /* Reference Pictures */
    for (i = 0; i < ARRAY_ELEMS(i965_h264_context->fsid_list); i++) {
        if (i965_h264_context->fsid_list[i].surface_id != VA_INVALID_ID) {
            int found = 0;
            for (j = 0; j < ARRAY_ELEMS(pic_param->ReferenceFrames); j++) {
                va_pic = &pic_param->ReferenceFrames[j];
                
                if (va_pic->flags & VA_PICTURE_H264_INVALID)
                    continue;

                if (va_pic->picture_id == i965_h264_context->fsid_list[i].surface_id) {
                    found = 1;
                    break;
                }
            }

            assert(found == 1);

            obj_surface = SURFACE(va_pic->picture_id);
            assert(obj_surface);
            w = obj_surface->width;
            h = obj_surface->height;
            field_picture = !!(va_pic->flags & (VA_PICTURE_H264_TOP_FIELD | VA_PICTURE_H264_BOTTOM_FIELD));
            i965_media_h264_surface_state(ctx, 2 + i, obj_surface,
                                          0, w / 4, h / (1 + field_picture), w,
                                          0, 
                                          field_picture,
                                          !!(va_pic->flags & VA_PICTURE_H264_BOTTOM_FIELD),
                                          I965_SURFACEFORMAT_R8_SINT); /* Y */
            i965_media_h264_surface_state(ctx, 18 + i, obj_surface,
                                          w * h, w / 4, h / 2 / (1 + field_picture), w,
                                          0, 
                                          field_picture,
                                          !!(va_pic->flags & VA_PICTURE_H264_BOTTOM_FIELD),
                                          I965_SURFACEFORMAT_R8G8_SINT);  /* INTERLEAVED U/V */
        }
    }
}

static void
i965_media_h264_binding_table(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    int i;
    unsigned int *binding_table;
    dri_bo *bo = media_state->binding_table.bo;

    dri_bo_map(bo, 1);
    assert(bo->virtual);
    binding_table = bo->virtual;
    memset(binding_table, 0, bo->size);

    for (i = 0; i < MAX_MEDIA_SURFACES; i++) {
        if (media_state->surface_state[i].bo) {
            binding_table[i] = media_state->surface_state[i].bo->offset;
            dri_bo_emit_reloc(bo,
                              I915_GEM_DOMAIN_INSTRUCTION, 0,
                              0,
                              i * sizeof(*binding_table),
                              media_state->surface_state[i].bo);
        }
    }

    dri_bo_unmap(media_state->binding_table.bo);
}

static void 
i965_media_h264_interface_descriptor_remap_table(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct i965_interface_descriptor *desc;
    int i;
    dri_bo *bo;

    bo = media_state->idrt.bo;
    dri_bo_map(bo, 1);
    assert(bo->virtual);
    desc = bo->virtual;

    for (i = 0; i < NUM_AVC_MC_INTERFACES; i++) {
        int kernel_offset = avc_mc_kernel_offset[i];
        memset(desc, 0, sizeof(*desc));
        desc->desc0.grf_reg_blocks = 7; 
        desc->desc0.kernel_start_pointer = (h264_avc_kernels[H264_AVC_COMBINED].bo->offset + kernel_offset) >> 6; /* reloc */
        desc->desc1.const_urb_entry_read_offset = 0;
        desc->desc1.const_urb_entry_read_len = 2;
        desc->desc3.binding_table_entry_count = 0;
        desc->desc3.binding_table_pointer = 
            media_state->binding_table.bo->offset >> 5; /*reloc */

        dri_bo_emit_reloc(bo,
                          I915_GEM_DOMAIN_INSTRUCTION, 0,
                          desc->desc0.grf_reg_blocks + kernel_offset,
                          i * sizeof(*desc) + offsetof(struct i965_interface_descriptor, desc0),
                          h264_avc_kernels[H264_AVC_COMBINED].bo);

        dri_bo_emit_reloc(bo,
                          I915_GEM_DOMAIN_INSTRUCTION, 0,
                          desc->desc3.binding_table_entry_count,
                          i * sizeof(*desc) + offsetof(struct i965_interface_descriptor, desc3),
                          media_state->binding_table.bo);
        desc++;
    }

    dri_bo_unmap(bo);
}

static void
i965_media_h264_vfe_state(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct i965_vfe_state *vfe_state;
    dri_bo *bo;

    bo = media_state->vfe_state.bo;
    dri_bo_map(bo, 1);
    assert(bo->virtual);
    vfe_state = bo->virtual;
    memset(vfe_state, 0, sizeof(*vfe_state));
    vfe_state->vfe0.extend_vfe_state_present = 1;
    vfe_state->vfe1.max_threads = media_state->urb.num_vfe_entries - 1;
    vfe_state->vfe1.urb_entry_alloc_size = media_state->urb.size_vfe_entry - 1;
    vfe_state->vfe1.num_urb_entries = media_state->urb.num_vfe_entries;
    vfe_state->vfe1.vfe_mode = VFE_AVC_IT_MODE;
    vfe_state->vfe1.children_present = 0;
    vfe_state->vfe2.interface_descriptor_base = 
        media_state->idrt.bo->offset >> 4; /* reloc */
    dri_bo_emit_reloc(bo,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      0,
                      offsetof(struct i965_vfe_state, vfe2),
                      media_state->idrt.bo);
    dri_bo_unmap(bo);
}

static void 
i965_media_h264_vfe_state_extension(VADriverContextP ctx, 
                                    struct decode_state *decode_state)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct i965_h264_context *i965_h264_context;
    struct i965_vfe_state_ex *vfe_state_ex;
    VAPictureParameterBufferH264 *pic_param;
    VASliceParameterBufferH264 *slice_param;
    int mbaff_frame_flag;

    assert(media_state->private_context);
    i965_h264_context = (struct i965_h264_context *)media_state->private_context;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferH264 *)decode_state->pic_param->buffer;

    assert(decode_state->slice_params[0] && decode_state->slice_params[0]->buffer);
    slice_param = (VASliceParameterBufferH264 *)decode_state->slice_params[0]->buffer;

    mbaff_frame_flag = (pic_param->seq_fields.bits.mb_adaptive_frame_field_flag &&
                        !pic_param->pic_fields.bits.field_pic_flag);

    assert(media_state->extended_state.bo);
    dri_bo_map(media_state->extended_state.bo, 1);
    assert(media_state->extended_state.bo->virtual);
    vfe_state_ex = media_state->extended_state.bo->virtual;
    memset(vfe_state_ex, 0, sizeof(*vfe_state_ex));

    /*
     * Indirect data buffer:
     * --------------------------------------------------------
     * | Motion Vectors | Weight/Offset data | Residual data |
     * --------------------------------------------------------
     * R4-R7: Motion Vectors
     * R8-R9: Weight/Offset
     * R10-R33: Residual data
     */
    vfe_state_ex->vfex1.avc.residual_data_fix_offset_flag = !!RESIDUAL_DATA_OFFSET;
    vfe_state_ex->vfex1.avc.residual_data_offset = RESIDUAL_DATA_OFFSET;

    if (slice_param->slice_type == SLICE_TYPE_I ||
        slice_param->slice_type == SLICE_TYPE_SI) 
        vfe_state_ex->vfex1.avc.sub_field_present_flag = PRESENT_NOMV; /* NoMV */
    else 
        vfe_state_ex->vfex1.avc.sub_field_present_flag = PRESENT_MV_WO; /* Both MV and W/O */

    if (vfe_state_ex->vfex1.avc.sub_field_present_flag == 0) {
        vfe_state_ex->vfex1.avc.weight_grf_offset = 0;
        vfe_state_ex->vfex1.avc.residual_grf_offset = 0;
    } else {
        vfe_state_ex->vfex1.avc.weight_grf_offset = 4;
        vfe_state_ex->vfex1.avc.residual_grf_offset = 6;
    }

    if (!pic_param->pic_fields.bits.field_pic_flag) {
        if (mbaff_frame_flag) {
            vfe_state_ex->remap_table0.remap_index_0 = INTRA_16X16;
            vfe_state_ex->remap_table0.remap_index_1 = INTRA_8X8;
            vfe_state_ex->remap_table0.remap_index_2 = INTRA_4X4;
            vfe_state_ex->remap_table0.remap_index_3 = INTRA_PCM;
            vfe_state_ex->remap_table0.remap_index_4 = MBAFF_MOTION;
            vfe_state_ex->remap_table0.remap_index_5 = MBAFF_MOTION;
            vfe_state_ex->remap_table0.remap_index_6 = MBAFF_MOTION;
            vfe_state_ex->remap_table0.remap_index_7 = MBAFF_MOTION;

            vfe_state_ex->remap_table1.remap_index_8 = MBAFF_MOTION;
            vfe_state_ex->remap_table1.remap_index_9 = MBAFF_MOTION;
            vfe_state_ex->remap_table1.remap_index_10 = MBAFF_MOTION;
            vfe_state_ex->remap_table1.remap_index_11 = MBAFF_MOTION;
            vfe_state_ex->remap_table1.remap_index_12 = MBAFF_MOTION;
            vfe_state_ex->remap_table1.remap_index_13 = MBAFF_MOTION;
            vfe_state_ex->remap_table1.remap_index_14 = MBAFF_MOTION;
            vfe_state_ex->remap_table1.remap_index_15 = MBAFF_MOTION;
        } else {
            vfe_state_ex->remap_table0.remap_index_0 = INTRA_16X16;
            vfe_state_ex->remap_table0.remap_index_1 = INTRA_8X8;
            vfe_state_ex->remap_table0.remap_index_2 = INTRA_4X4;
            vfe_state_ex->remap_table0.remap_index_3 = INTRA_PCM;
            vfe_state_ex->remap_table0.remap_index_4 = FRAMEMB_MOTION;
            vfe_state_ex->remap_table0.remap_index_5 = FRAMEMB_MOTION;
            vfe_state_ex->remap_table0.remap_index_6 = FRAMEMB_MOTION;
            vfe_state_ex->remap_table0.remap_index_7 = FRAMEMB_MOTION;

            vfe_state_ex->remap_table1.remap_index_8 = FRAMEMB_MOTION;
            vfe_state_ex->remap_table1.remap_index_9 = FRAMEMB_MOTION;
            vfe_state_ex->remap_table1.remap_index_10 = FRAMEMB_MOTION;
            vfe_state_ex->remap_table1.remap_index_11 = FRAMEMB_MOTION;
            vfe_state_ex->remap_table1.remap_index_12 = FRAMEMB_MOTION;
            vfe_state_ex->remap_table1.remap_index_13 = FRAMEMB_MOTION;
            vfe_state_ex->remap_table1.remap_index_14 = FRAMEMB_MOTION;
            vfe_state_ex->remap_table1.remap_index_15 = FRAMEMB_MOTION;
        }
    } else {
        vfe_state_ex->remap_table0.remap_index_0 = INTRA_16X16;
        vfe_state_ex->remap_table0.remap_index_1 = INTRA_8X8;
        vfe_state_ex->remap_table0.remap_index_2 = INTRA_4X4;
        vfe_state_ex->remap_table0.remap_index_3 = INTRA_PCM;
        vfe_state_ex->remap_table0.remap_index_4 = FIELDMB_MOTION;
        vfe_state_ex->remap_table0.remap_index_5 = FIELDMB_MOTION;
        vfe_state_ex->remap_table0.remap_index_6 = FIELDMB_MOTION;
        vfe_state_ex->remap_table0.remap_index_7 = FIELDMB_MOTION;

        vfe_state_ex->remap_table1.remap_index_8 = FIELDMB_MOTION;
        vfe_state_ex->remap_table1.remap_index_9 = FIELDMB_MOTION;
        vfe_state_ex->remap_table1.remap_index_10 = FIELDMB_MOTION;
        vfe_state_ex->remap_table1.remap_index_11 = FIELDMB_MOTION;
        vfe_state_ex->remap_table1.remap_index_12 = FIELDMB_MOTION;
        vfe_state_ex->remap_table1.remap_index_13 = FIELDMB_MOTION;
        vfe_state_ex->remap_table1.remap_index_14 = FIELDMB_MOTION;
        vfe_state_ex->remap_table1.remap_index_15 = FIELDMB_MOTION;
    }

    if (i965_h264_context->use_avc_hw_scoreboard) {
        vfe_state_ex->scoreboard0.enable = 1;
        vfe_state_ex->scoreboard0.type = SCOREBOARD_STALLING;
        vfe_state_ex->scoreboard0.mask = 0xff;

        vfe_state_ex->scoreboard1.delta_x0 = -1;
        vfe_state_ex->scoreboard1.delta_y0 = 0;
        vfe_state_ex->scoreboard1.delta_x1 = 0;
        vfe_state_ex->scoreboard1.delta_y1 = -1;
        vfe_state_ex->scoreboard1.delta_x2 = 1;
        vfe_state_ex->scoreboard1.delta_y2 = -1;
        vfe_state_ex->scoreboard1.delta_x3 = -1;
        vfe_state_ex->scoreboard1.delta_y3 = -1;

        vfe_state_ex->scoreboard2.delta_x4 = -1;
        vfe_state_ex->scoreboard2.delta_y4 = 1;
        vfe_state_ex->scoreboard2.delta_x5 = 0;
        vfe_state_ex->scoreboard2.delta_y5 = -2;
        vfe_state_ex->scoreboard2.delta_x6 = 1;
        vfe_state_ex->scoreboard2.delta_y6 = -2;
        vfe_state_ex->scoreboard2.delta_x7 = -1;
        vfe_state_ex->scoreboard2.delta_y7 = -2;
    }

    dri_bo_unmap(media_state->extended_state.bo);
}

static void
i965_media_h264_upload_constants(VADriverContextP ctx, struct decode_state *decode_state)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct i965_h264_context *i965_h264_context;
    unsigned char *constant_buffer;
    VASliceParameterBufferH264 *slice_param;

    assert(media_state->private_context);
    i965_h264_context = (struct i965_h264_context *)media_state->private_context;

    assert(decode_state->slice_params[0] && decode_state->slice_params[0]->buffer);
    slice_param = (VASliceParameterBufferH264 *)decode_state->slice_params[0]->buffer;

    dri_bo_map(media_state->curbe.bo, 1);
    assert(media_state->curbe.bo->virtual);
    constant_buffer = media_state->curbe.bo->virtual;

    /* HW solution for W=128 */
    if (i965_h264_context->use_hw_w128) {
        memcpy(constant_buffer, intra_kernel_header, sizeof(*intra_kernel_header));
    } else {
        if (slice_param->slice_type == SLICE_TYPE_I ||
            slice_param->slice_type == SLICE_TYPE_SI) {
            memcpy(constant_buffer, intra_kernel_header, sizeof(*intra_kernel_header));
        } else {
            /* FIXME: Need to upload CURBE data to inter kernel interface 
             * to support weighted prediction work-around 
             */
            *(short *)constant_buffer = i965_h264_context->weight128_offset0;
            constant_buffer += 2;
            *(char *)constant_buffer = i965_h264_context->weight128_offset0_flag;
            constant_buffer++;
            *constant_buffer = 0;
        }
    }

    dri_bo_unmap(media_state->curbe.bo);
}

static void
i965_media_h264_states_setup(VADriverContextP ctx, struct decode_state *decode_state)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct i965_h264_context *i965_h264_context;

    assert(media_state->private_context);
    i965_h264_context = (struct i965_h264_context *)media_state->private_context;

    i965_avc_bsd_pipeline(ctx, decode_state);

    i965_avc_hw_scoreboard(ctx, decode_state);

    i965_media_h264_surfaces_setup(ctx, decode_state);
    i965_media_h264_binding_table(ctx);
    i965_media_h264_interface_descriptor_remap_table(ctx);
    i965_media_h264_vfe_state_extension(ctx, decode_state);
    i965_media_h264_vfe_state(ctx);
    i965_media_h264_upload_constants(ctx, decode_state);
}

static void
i965_media_h264_objects(VADriverContextP ctx, struct decode_state *decode_state)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct i965_h264_context *i965_h264_context;
    unsigned int *object_command;

    assert(media_state->private_context);
    i965_h264_context = (struct i965_h264_context *)media_state->private_context;

    dri_bo_map(i965_h264_context->avc_it_command_mb_info.bo, True);
    assert(i965_h264_context->avc_it_command_mb_info.bo->virtual);
    object_command = i965_h264_context->avc_it_command_mb_info.bo->virtual;
    memset(object_command, 0, i965_h264_context->avc_it_command_mb_info.mbs * i965_h264_context->use_avc_hw_scoreboard * MB_CMD_IN_BYTES);
    object_command += i965_h264_context->avc_it_command_mb_info.mbs * (1 + i965_h264_context->use_avc_hw_scoreboard) * MB_CMD_IN_DWS;
    *object_command++ = 0;
    *object_command = MI_BATCH_BUFFER_END;
    dri_bo_unmap(i965_h264_context->avc_it_command_mb_info.bo);

    BEGIN_BATCH(ctx, 2);
    OUT_BATCH(ctx, MI_BATCH_BUFFER_START | (2 << 6));
    OUT_RELOC(ctx, i965_h264_context->avc_it_command_mb_info.bo, 
              I915_GEM_DOMAIN_COMMAND, 0, 
              0);
    ADVANCE_BATCH(ctx);

    /* Have to execute the batch buffer here becuase MI_BATCH_BUFFER_END
     * will cause control to pass back to ring buffer 
     */
    intel_batchbuffer_end_atomic(ctx);
    intel_batchbuffer_flush(ctx);
    intel_batchbuffer_start_atomic(ctx, 0x1000);
    i965_avc_ildb(ctx, decode_state);
}

static void 
i965_media_h264_free_private_context(void **data)
{
    struct i965_h264_context *i965_h264_context = *data;
    int i;

    if (i965_h264_context == NULL)
        return;

    i965_avc_ildb_ternimate(&i965_h264_context->avc_ildb_context);
    i965_avc_hw_scoreboard_ternimate(&i965_h264_context->avc_hw_scoreboard_context);
    i965_avc_bsd_ternimate(&i965_h264_context->i965_avc_bsd_context);
    dri_bo_unreference(i965_h264_context->avc_it_command_mb_info.bo);
    dri_bo_unreference(i965_h264_context->avc_it_data.bo);
    dri_bo_unreference(i965_h264_context->avc_ildb_data.bo);
    free(i965_h264_context);
    *data = NULL;

    for (i = 0; i < NUM_H264_AVC_KERNELS; i++) {
        struct media_kernel *kernel = &h264_avc_kernels[i];

        dri_bo_unreference(kernel->bo);
        kernel->bo = NULL;
    }
}

void
i965_media_h264_decode_init(VADriverContextP ctx, struct decode_state *decode_state)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct i965_h264_context *i965_h264_context;
    dri_bo *bo;
    int i;
    VAPictureParameterBufferH264 *pic_param;

    i965_h264_context = media_state->private_context;

    if (i965_h264_context == NULL) {
        /* kernel */
        assert(NUM_H264_AVC_KERNELS == (sizeof(h264_avc_kernels_gen5) / 
                                        sizeof(h264_avc_kernels_gen5[0])));
        assert(NUM_AVC_MC_INTERFACES == (sizeof(avc_mc_kernel_offset_gen5) /
                                         sizeof(avc_mc_kernel_offset_gen5[0])));

        i965_h264_context = calloc(1, sizeof(struct i965_h264_context));

        if (IS_IRONLAKE(i965->intel.device_id)) {
            h264_avc_kernels = h264_avc_kernels_gen5;
            avc_mc_kernel_offset = avc_mc_kernel_offset_gen5;
            intra_kernel_header = &intra_kernel_header_gen5;
            i965_h264_context->use_avc_hw_scoreboard = 1;
            i965_h264_context->use_hw_w128 = 1;
        } else {
            h264_avc_kernels = h264_avc_kernels_gen4;
            avc_mc_kernel_offset = avc_mc_kernel_offset_gen4;
            intra_kernel_header = &intra_kernel_header_gen4;
            i965_h264_context->use_avc_hw_scoreboard = 0;
            i965_h264_context->use_hw_w128 = 0;
        }

        for (i = 0; i < NUM_H264_AVC_KERNELS; i++) {
            struct media_kernel *kernel = &h264_avc_kernels[i];
            kernel->bo = dri_bo_alloc(i965->intel.bufmgr, 
                                      kernel->name, 
                                      kernel->size, 0x1000);
            assert(kernel->bo);
            dri_bo_subdata(kernel->bo, 0, kernel->size, kernel->bin);
        }

        for (i = 0; i < 16; i++) {
            i965_h264_context->fsid_list[i].surface_id = VA_INVALID_ID;
            i965_h264_context->fsid_list[i].frame_store_id = -1;
        }

        media_state->private_context = i965_h264_context;
        media_state->free_private_context = i965_media_h264_free_private_context;

        /* URB */
        if (IS_IRONLAKE(i965->intel.device_id)) {
            media_state->urb.num_vfe_entries = 63;
        } else {
            media_state->urb.num_vfe_entries = 23;
        }

        media_state->urb.size_vfe_entry = 16;

        media_state->urb.num_cs_entries = 1;
        media_state->urb.size_cs_entry = 1;

        media_state->urb.vfe_start = 0;
        media_state->urb.cs_start = media_state->urb.vfe_start + 
            media_state->urb.num_vfe_entries * media_state->urb.size_vfe_entry;
        assert(media_state->urb.cs_start + 
               media_state->urb.num_cs_entries * media_state->urb.size_cs_entry <= URB_SIZE((&i965->intel)));

        /* hook functions */
        media_state->media_states_setup = i965_media_h264_states_setup;
        media_state->media_objects = i965_media_h264_objects;
    }

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferH264 *)decode_state->pic_param->buffer;
    i965_h264_context->picture.width_in_mbs = ((pic_param->picture_width_in_mbs_minus1 + 1) & 0xff);
    i965_h264_context->picture.height_in_mbs = ((pic_param->picture_height_in_mbs_minus1 + 1) & 0xff) / 
        (1 + !!pic_param->pic_fields.bits.field_pic_flag); /* picture height */
    i965_h264_context->picture.mbaff_frame_flag = (pic_param->seq_fields.bits.mb_adaptive_frame_field_flag &&
                                                   !pic_param->pic_fields.bits.field_pic_flag);
    i965_h264_context->avc_it_command_mb_info.mbs = (i965_h264_context->picture.width_in_mbs * 
                                                     i965_h264_context->picture.height_in_mbs);

    dri_bo_unreference(i965_h264_context->avc_it_command_mb_info.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "avc it command mb info",
                      i965_h264_context->avc_it_command_mb_info.mbs * MB_CMD_IN_BYTES * (1 + i965_h264_context->use_avc_hw_scoreboard) + 8,
                      0x1000);
    assert(bo);
    i965_h264_context->avc_it_command_mb_info.bo = bo;

    dri_bo_unreference(i965_h264_context->avc_it_data.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "avc it data",
                      i965_h264_context->avc_it_command_mb_info.mbs * 
                      0x800 * 
                      (1 + !!pic_param->pic_fields.bits.field_pic_flag),
                      0x1000);
    assert(bo);
    i965_h264_context->avc_it_data.bo = bo;
    i965_h264_context->avc_it_data.write_offset = 0;
    dri_bo_unreference(media_state->indirect_object.bo);
    media_state->indirect_object.bo = bo;
    dri_bo_reference(media_state->indirect_object.bo);
    media_state->indirect_object.offset = i965_h264_context->avc_it_data.write_offset;

    dri_bo_unreference(i965_h264_context->avc_ildb_data.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "AVC-ILDB Data Buffer",
                      i965_h264_context->avc_it_command_mb_info.mbs * 64 * 2,
                      0x1000);
    assert(bo);
    i965_h264_context->avc_ildb_data.bo = bo;

    /* bsd pipeline */
    i965_avc_bsd_decode_init(ctx);

    /* HW scoreboard */
    i965_avc_hw_scoreboard_decode_init(ctx);

    /* ILDB */
    i965_avc_ildb_decode_init(ctx);

    /* for Media pipeline */
    media_state->extended_state.enabled = 1;
    dri_bo_unreference(media_state->extended_state.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "extened vfe state", 
                      sizeof(struct i965_vfe_state_ex), 32);
    assert(bo);
    media_state->extended_state.bo = bo;
}
