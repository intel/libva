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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <va/va_backend.h>

#include "intel_batchbuffer.h"
#include "intel_driver.h"

#include "i965_defines.h"
#include "i965_drv_video.h"

#include "gen6_mfd.h"

#define DMV_SIZE        0x88000 /* 557056 bytes for a frame */

static const uint32_t zigzag_direct[64] = {
    0,   1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

static void
gen6_mfd_avc_frame_store_index(VADriverContextP ctx, VAPictureParameterBufferH264 *pic_param)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct gen6_mfd_context *gen6_mfd_context = (struct gen6_mfd_context *)media_state->private_context;
    int i, j;

    assert(ARRAY_ELEMS(gen6_mfd_context->reference_surface) == ARRAY_ELEMS(pic_param->ReferenceFrames));

    for (i = 0; i < ARRAY_ELEMS(gen6_mfd_context->reference_surface); i++) {
        int found = 0;

        if (gen6_mfd_context->reference_surface[i].surface_id == VA_INVALID_ID)
            continue;

        for (j = 0; j < ARRAY_ELEMS(pic_param->ReferenceFrames); j++) {
            VAPictureH264 *ref_pic = &pic_param->ReferenceFrames[j];
            if (ref_pic->flags & VA_PICTURE_H264_INVALID)
                continue;

            if (gen6_mfd_context->reference_surface[i].surface_id == ref_pic->picture_id) {
                found = 1;
                break;
            }
        }

        if (!found) {
            struct object_surface *obj_surface = SURFACE(gen6_mfd_context->reference_surface[i].surface_id);
            obj_surface->flags &= ~SURFACE_REFERENCED;

            if ((obj_surface->flags & SURFACE_ALL_MASK) == SURFACE_DISPLAYED) {
                dri_bo_unreference(obj_surface->bo);
                obj_surface->bo = NULL;
                obj_surface->flags &= ~SURFACE_REF_DIS_MASK;
            }

            if (obj_surface->free_private_data)
                obj_surface->free_private_data(&obj_surface->private_data);

            gen6_mfd_context->reference_surface[i].surface_id = VA_INVALID_ID;
            gen6_mfd_context->reference_surface[i].frame_store_id = -1;
        }
    }

    for (i = 0; i < ARRAY_ELEMS(pic_param->ReferenceFrames); i++) {
        VAPictureH264 *ref_pic = &pic_param->ReferenceFrames[i];
        int found = 0;

        if (ref_pic->flags & VA_PICTURE_H264_INVALID)
            continue;

        for (j = 0; j < ARRAY_ELEMS(gen6_mfd_context->reference_surface); j++) {
            if (gen6_mfd_context->reference_surface[j].surface_id == VA_INVALID_ID)
                continue;
            
            if (gen6_mfd_context->reference_surface[j].surface_id == ref_pic->picture_id) {
                found = 1;
                break;
            }
        }

        if (!found) {
            int frame_idx;
            struct object_surface *obj_surface = SURFACE(ref_pic->picture_id);
            
            if (obj_surface->bo == NULL) {
                uint32_t tiling_mode = I915_TILING_Y;
                unsigned long pitch;
        
                obj_surface->bo = drm_intel_bo_alloc_tiled(i965->intel.bufmgr, 
                                                           "vaapi surface",
                                                           obj_surface->width, 
                                                           obj_surface->height + obj_surface->height / 2,
                                                           1,
                                                           &tiling_mode,
                                                           &pitch,
                                                           0);
                assert(obj_surface->bo);
                assert(tiling_mode == I915_TILING_Y);
                assert(pitch == obj_surface->width);
            }

            for (frame_idx = 0; frame_idx < ARRAY_ELEMS(gen6_mfd_context->reference_surface); frame_idx++) {
                for (j = 0; j < ARRAY_ELEMS(gen6_mfd_context->reference_surface); j++) {
                    if (gen6_mfd_context->reference_surface[j].surface_id == VA_INVALID_ID)
                        continue;

                    if (gen6_mfd_context->reference_surface[j].frame_store_id == frame_idx)
                        break;
                }

                if (j == ARRAY_ELEMS(gen6_mfd_context->reference_surface))
                    break;
            }

            assert(frame_idx < ARRAY_ELEMS(gen6_mfd_context->reference_surface));

            for (j = 0; j < ARRAY_ELEMS(gen6_mfd_context->reference_surface); j++) {
                if (gen6_mfd_context->reference_surface[j].surface_id == VA_INVALID_ID) {
                    gen6_mfd_context->reference_surface[j].surface_id = ref_pic->picture_id;
                    gen6_mfd_context->reference_surface[j].frame_store_id = frame_idx;
                    break;
                }
            }
        }
    }

    /* sort */
    for (i = 0; i < ARRAY_ELEMS(gen6_mfd_context->reference_surface) - 1; i++) {
        if (gen6_mfd_context->reference_surface[i].surface_id != VA_INVALID_ID &&
            gen6_mfd_context->reference_surface[i].frame_store_id == i)
            continue;

        for (j = i + 1; j < ARRAY_ELEMS(gen6_mfd_context->reference_surface); j++) {
            if (gen6_mfd_context->reference_surface[j].surface_id != VA_INVALID_ID &&
                gen6_mfd_context->reference_surface[j].frame_store_id == i) {
                VASurfaceID id = gen6_mfd_context->reference_surface[i].surface_id;
                int frame_idx = gen6_mfd_context->reference_surface[i].frame_store_id;

                gen6_mfd_context->reference_surface[i].surface_id = gen6_mfd_context->reference_surface[j].surface_id;
                gen6_mfd_context->reference_surface[i].frame_store_id = gen6_mfd_context->reference_surface[j].frame_store_id;
                gen6_mfd_context->reference_surface[j].surface_id = id;
                gen6_mfd_context->reference_surface[j].frame_store_id = frame_idx;
                break;
            }
        }
    }
}

static void 
gen6_mfd_free_avc_surface(void **data)
{
    struct gen6_avc_surface *gen6_avc_surface = *data;

    if (!gen6_avc_surface)
        return;

    dri_bo_unreference(gen6_avc_surface->dmv_top);
    gen6_avc_surface->dmv_top = NULL;
    dri_bo_unreference(gen6_avc_surface->dmv_bottom);
    gen6_avc_surface->dmv_bottom = NULL;

    free(gen6_avc_surface);
    *data = NULL;
}

static void
gen6_mfd_init_avc_surface(VADriverContextP ctx, 
                          VAPictureParameterBufferH264 *pic_param,
                          struct object_surface *obj_surface)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen6_avc_surface *gen6_avc_surface = obj_surface->private_data;

    obj_surface->free_private_data = gen6_mfd_free_avc_surface;

    if (!gen6_avc_surface) {
        gen6_avc_surface = calloc(sizeof(struct gen6_avc_surface), 1);
        assert((obj_surface->size & 0x3f) == 0);
        obj_surface->private_data = gen6_avc_surface;
    }

    gen6_avc_surface->dmv_bottom_flag = (pic_param->pic_fields.bits.field_pic_flag &&
                                         !pic_param->seq_fields.bits.direct_8x8_inference_flag);

    if (gen6_avc_surface->dmv_top == NULL) {
        gen6_avc_surface->dmv_top = dri_bo_alloc(i965->intel.bufmgr,
                                                 "direct mv w/r buffer",
                                                 DMV_SIZE,
                                                 0x1000);
    }

    if (gen6_avc_surface->dmv_bottom_flag &&
        gen6_avc_surface->dmv_bottom == NULL) {
        gen6_avc_surface->dmv_bottom = dri_bo_alloc(i965->intel.bufmgr,
                                                    "direct mv w/r buffer",
                                                    DMV_SIZE,
                                                    0x1000);
    }
}

static void
gen6_mfd_pipe_mode_select(VADriverContextP ctx,
                          struct decode_state *decode_state,
                          int standard_select)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct gen6_mfd_context *gen6_mfd_context = media_state->private_context;

    assert(standard_select == MFX_FORMAT_MPEG2 ||
           standard_select == MFX_FORMAT_AVC ||
           standard_select == MFX_FORMAT_VC1);

    BEGIN_BCS_BATCH(ctx, 4);
    OUT_BCS_BATCH(ctx, MFX_PIPE_MODE_SELECT | (4 - 2));
    OUT_BCS_BATCH(ctx,
                  (MFD_MODE_VLD << 16) | /* VLD mode */
                  (0 << 10) | /* disable Stream-Out */
                  (gen6_mfd_context->post_deblocking_output.valid << 9)  | /* Post Deblocking Output */
                  (gen6_mfd_context->pre_deblocking_output.valid << 8)  | /* Pre Deblocking Output */
                  (0 << 7)  | /* disable TLB prefectch */
                  (0 << 5)  | /* not in stitch mode */
                  (MFX_CODEC_DECODE << 4)  | /* decoding mode */
                  (standard_select << 0));
    OUT_BCS_BATCH(ctx,
                  (0 << 20) | /* round flag in PB slice */
                  (0 << 19) | /* round flag in Intra8x8 */
                  (0 << 7)  | /* expand NOA bus flag */
                  (1 << 6)  | /* must be 1 */
                  (0 << 5)  | /* disable clock gating for NOA */
                  (0 << 4)  | /* terminate if AVC motion and POC table error occurs */
                  (0 << 3)  | /* terminate if AVC mbdata error occurs */
                  (0 << 2)  | /* terminate if AVC CABAC/CAVLC decode error occurs */
                  (0 << 1)  | /* AVC long field motion vector */
                  (1 << 0));  /* always calculate AVC ILDB boundary strength */
    OUT_BCS_BATCH(ctx, 0);
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_surface_state(VADriverContextP ctx,
                       struct decode_state *decode_state,
                       int standard_select)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_surface *obj_surface = SURFACE(decode_state->current_render_target);
    assert(obj_surface);
    
    BEGIN_BCS_BATCH(ctx, 6);
    OUT_BCS_BATCH(ctx, MFX_SURFACE_STATE | (6 - 2));
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx,
                  ((obj_surface->orig_height - 1) << 19) |
                  ((obj_surface->orig_width - 1) << 6));
    OUT_BCS_BATCH(ctx,
                  (MFX_SURFACE_PLANAR_420_8 << 28) | /* 420 planar YUV surface */
                  (1 << 27) | /* must be 1 for interleave U/V, hardware requirement */
                  (0 << 22) | /* surface object control state, FIXME??? */
                  ((obj_surface->width - 1) << 3) | /* pitch */
                  (0 << 2)  | /* must be 0 for interleave U/V */
                  (1 << 1)  | /* must be y-tiled */
                  (I965_TILEWALK_YMAJOR << 0));  /* tile walk, FIXME: must be 1 ??? */
    OUT_BCS_BATCH(ctx,
                  (0 << 16) | /* must be 0 for interleave U/V */
                  (obj_surface->height)); /* y offset for U(cb) */
    OUT_BCS_BATCH(ctx, 0);
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_pipe_buf_addr_state(VADriverContextP ctx,
                             struct decode_state *decode_state,
                             int standard_select)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct gen6_mfd_context *gen6_mfd_context = media_state->private_context;
    int i;

    BEGIN_BCS_BATCH(ctx, 24);
    OUT_BCS_BATCH(ctx, MFX_PIPE_BUF_ADDR_STATE | (24 - 2));
    if (gen6_mfd_context->pre_deblocking_output.valid)
        OUT_BCS_RELOC(ctx, gen6_mfd_context->pre_deblocking_output.bo,
                      I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                      0);
    else
        OUT_BCS_BATCH(ctx, 0);

    if (gen6_mfd_context->post_deblocking_output.valid)
        OUT_BCS_RELOC(ctx, gen6_mfd_context->post_deblocking_output.bo,
                      I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                      0);
    else
        OUT_BCS_BATCH(ctx, 0);

    OUT_BCS_BATCH(ctx, 0); /* ignore for decoding */
    OUT_BCS_BATCH(ctx, 0); /* ignore for decoding */

    if (gen6_mfd_context->intra_row_store_scratch_buffer.valid)
        OUT_BCS_RELOC(ctx, gen6_mfd_context->intra_row_store_scratch_buffer.bo,
                      I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                      0);
    else
        OUT_BCS_BATCH(ctx, 0);

    if (gen6_mfd_context->deblocking_filter_row_store_scratch_buffer.valid)
        OUT_BCS_RELOC(ctx, gen6_mfd_context->deblocking_filter_row_store_scratch_buffer.bo,
                      I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                      0);
    else
        OUT_BCS_BATCH(ctx, 0);

    /* DW 7..22 */
    for (i = 0; i < ARRAY_ELEMS(gen6_mfd_context->reference_surface); i++) {
        struct object_surface *obj_surface;

        if (gen6_mfd_context->reference_surface[i].surface_id != VA_INVALID_ID) {
            obj_surface = SURFACE(gen6_mfd_context->reference_surface[i].surface_id);
            assert(obj_surface && obj_surface->bo);

            OUT_BCS_RELOC(ctx, obj_surface->bo,
                          I915_GEM_DOMAIN_INSTRUCTION, 0,
                          0);
        } else {
            OUT_BCS_BATCH(ctx, 0);
        }
    }

    OUT_BCS_BATCH(ctx, 0);   /* ignore DW23 for decoding */
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_ind_obj_base_addr_state(VADriverContextP ctx,
                                 dri_bo *slice_data_bo,
                                 int standard_select)
{
    BEGIN_BCS_BATCH(ctx, 11);
    OUT_BCS_BATCH(ctx, MFX_IND_OBJ_BASE_ADDR_STATE | (11 - 2));
    OUT_BCS_RELOC(ctx, slice_data_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0); /* MFX Indirect Bitstream Object Base Address */
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0); /* ignore for VLD mode */
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0); /* ignore for VLD mode */
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0); /* ignore for VLD mode */
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0); /* ignore for VLD mode */
    OUT_BCS_BATCH(ctx, 0);
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_bsp_buf_base_addr_state(VADriverContextP ctx,
                                 struct decode_state *decode_state,
                                 int standard_select)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct gen6_mfd_context *gen6_mfd_context = media_state->private_context;

    BEGIN_BCS_BATCH(ctx, 4);
    OUT_BCS_BATCH(ctx, MFX_BSP_BUF_BASE_ADDR_STATE | (4 - 2));

    if (gen6_mfd_context->bsd_mpc_row_store_scratch_buffer.valid)
        OUT_BCS_RELOC(ctx, gen6_mfd_context->bsd_mpc_row_store_scratch_buffer.bo,
                      I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                      0);
    else
        OUT_BCS_BATCH(ctx, 0);

    if (gen6_mfd_context->mpr_row_store_scratch_buffer.valid)
        OUT_BCS_RELOC(ctx, gen6_mfd_context->mpr_row_store_scratch_buffer.bo,
                      I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                      0);
    else
        OUT_BCS_BATCH(ctx, 0);

    if (gen6_mfd_context->bitplane_read_buffer.valid)
        OUT_BCS_RELOC(ctx, gen6_mfd_context->bitplane_read_buffer.bo,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      0);
    else
        OUT_BCS_BATCH(ctx, 0);

    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_aes_state(VADriverContextP ctx,
                   struct decode_state *decode_state,
                   int standard_select)
{
    /* FIXME */
}

static void
gen6_mfd_wait(VADriverContextP ctx,
              struct decode_state *decode_state,
              int standard_select)
{
    BEGIN_BCS_BATCH(ctx, 1);
    OUT_BCS_BATCH(ctx, MFX_WAIT | (1 << 8));
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_avc_img_state(VADriverContextP ctx, struct decode_state *decode_state)
{
    int qm_present_flag;
    int img_struct;
    int mbaff_frame_flag;
    unsigned int width_in_mbs, height_in_mbs;
    VAPictureParameterBufferH264 *pic_param;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferH264 *)decode_state->pic_param->buffer;
    assert(!(pic_param->CurrPic.flags & VA_PICTURE_H264_INVALID));

    if (decode_state->iq_matrix && decode_state->iq_matrix->buffer)
        qm_present_flag = 1;
    else
        qm_present_flag = 0; /* built-in QM matrices */

    if (pic_param->CurrPic.flags & VA_PICTURE_H264_TOP_FIELD)
        img_struct = 1;
    else if (pic_param->CurrPic.flags & VA_PICTURE_H264_BOTTOM_FIELD)
        img_struct = 3;
    else
        img_struct = 0;

    if ((img_struct & 0x1) == 0x1) {
        assert(pic_param->pic_fields.bits.field_pic_flag == 0x1);
    } else {
        assert(pic_param->pic_fields.bits.field_pic_flag == 0x0);
    }

    if (pic_param->seq_fields.bits.frame_mbs_only_flag) { /* a frame containing only frame macroblocks */
        assert(pic_param->seq_fields.bits.mb_adaptive_frame_field_flag == 0);
        assert(pic_param->pic_fields.bits.field_pic_flag == 0);
    } else {
        assert(pic_param->seq_fields.bits.direct_8x8_inference_flag == 1); /* see H.264 spec */
    }

    mbaff_frame_flag = (pic_param->seq_fields.bits.mb_adaptive_frame_field_flag &&
                        !pic_param->pic_fields.bits.field_pic_flag);

    width_in_mbs = ((pic_param->picture_width_in_mbs_minus1 + 1) & 0xff);
    height_in_mbs = ((pic_param->picture_height_in_mbs_minus1 + 1) & 0xff); /* frame height */
    assert(!((width_in_mbs * height_in_mbs) & 0x8000)); /* hardware requirement */

    /* MFX unit doesn't support 4:2:2 and 4:4:4 picture */
    assert(pic_param->seq_fields.bits.chroma_format_idc == 0 || /* monochrome picture */
           pic_param->seq_fields.bits.chroma_format_idc == 1);  /* 4:2:0 */
    assert(pic_param->seq_fields.bits.residual_colour_transform_flag == 0); /* only available for 4:4:4 */

    BEGIN_BCS_BATCH(ctx, 13);
    OUT_BCS_BATCH(ctx, MFX_AVC_IMG_STATE | (13 - 2));
    OUT_BCS_BATCH(ctx, 
                  ((width_in_mbs * height_in_mbs) & 0x7fff));
    OUT_BCS_BATCH(ctx, 
                  (height_in_mbs << 16) | 
                  (width_in_mbs << 0));
    OUT_BCS_BATCH(ctx, 
                  ((pic_param->second_chroma_qp_index_offset & 0x1f) << 24) |
                  ((pic_param->chroma_qp_index_offset & 0x1f) << 16) |
                  (0 << 14) | /* Max-bit conformance Intra flag ??? FIXME */
                  (0 << 13) | /* Max Macroblock size conformance Inter flag ??? FIXME */
                  (1 << 12) | /* always 1, hardware requirement */
                  (qm_present_flag << 10) |
                  (img_struct << 8) |
                  (16 << 0));
    OUT_BCS_BATCH(ctx,
                  (pic_param->seq_fields.bits.chroma_format_idc << 10) |
                  (pic_param->pic_fields.bits.entropy_coding_mode_flag << 7) |
                  ((!pic_param->pic_fields.bits.reference_pic_flag) << 6) |
                  (pic_param->pic_fields.bits.constrained_intra_pred_flag << 5) |
                  (pic_param->seq_fields.bits.direct_8x8_inference_flag << 4) |
                  (pic_param->pic_fields.bits.transform_8x8_mode_flag << 3) |
                  (pic_param->seq_fields.bits.frame_mbs_only_flag << 2) |
                  (mbaff_frame_flag << 1) |
                  (pic_param->pic_fields.bits.field_pic_flag << 0));
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_avc_qm_state(VADriverContextP ctx, struct decode_state *decode_state)
{
    int cmd_len;
    VAIQMatrixBufferH264 *iq_matrix;
    VAPictureParameterBufferH264 *pic_param;

    if (!decode_state->iq_matrix || !decode_state->iq_matrix->buffer)
        return;

    iq_matrix = (VAIQMatrixBufferH264 *)decode_state->iq_matrix->buffer;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferH264 *)decode_state->pic_param->buffer;

    cmd_len = 2 + 6 * 4; /* always load six 4x4 scaling matrices */

    if (pic_param->pic_fields.bits.transform_8x8_mode_flag)
        cmd_len += 2 * 16; /* load two 8x8 scaling matrices */

    BEGIN_BCS_BATCH(ctx, cmd_len);
    OUT_BCS_BATCH(ctx, MFX_AVC_QM_STATE | (cmd_len - 2));

    if (pic_param->pic_fields.bits.transform_8x8_mode_flag)
        OUT_BCS_BATCH(ctx, 
                      (0x0  << 8) | /* don't use default built-in matrices */
                      (0xff << 0)); /* six 4x4 and two 8x8 scaling matrices */
    else
        OUT_BCS_BATCH(ctx, 
                      (0x0  << 8) | /* don't use default built-in matrices */
                      (0x3f << 0)); /* six 4x4 scaling matrices */

    intel_batchbuffer_data_bcs(ctx, &iq_matrix->ScalingList4x4[0][0], 6 * 4 * 4);

    if (pic_param->pic_fields.bits.transform_8x8_mode_flag)
        intel_batchbuffer_data_bcs(ctx, &iq_matrix->ScalingList8x8[0][0], 2 * 16 * 4);

    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_avc_directmode_state(VADriverContextP ctx,
                              VAPictureParameterBufferH264 *pic_param,
                              VASliceParameterBufferH264 *slice_param)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct gen6_mfd_context *gen6_mfd_context = media_state->private_context;
    struct object_surface *obj_surface;
    struct gen6_avc_surface *gen6_avc_surface;
    VAPictureH264 *va_pic;
    int i, j;

    BEGIN_BCS_BATCH(ctx, 69);
    OUT_BCS_BATCH(ctx, MFX_AVC_DIRECTMODE_STATE | (69 - 2));

    /* reference surfaces 0..15 */
    for (i = 0; i < ARRAY_ELEMS(gen6_mfd_context->reference_surface); i++) {
        if (gen6_mfd_context->reference_surface[i].surface_id != VA_INVALID_ID) {
            obj_surface = SURFACE(gen6_mfd_context->reference_surface[i].surface_id);
            assert(obj_surface);
            gen6_avc_surface = obj_surface->private_data;

            if (gen6_avc_surface == NULL) {
                OUT_BCS_BATCH(ctx, 0);
                OUT_BCS_BATCH(ctx, 0);
            } else {
                OUT_BCS_RELOC(ctx, gen6_avc_surface->dmv_top,
                              I915_GEM_DOMAIN_INSTRUCTION, 0,
                              0);

                if (gen6_avc_surface->dmv_bottom_flag == 1)
                    OUT_BCS_RELOC(ctx, gen6_avc_surface->dmv_bottom,
                                  I915_GEM_DOMAIN_INSTRUCTION, 0,
                                  0);
                else
                    OUT_BCS_RELOC(ctx, gen6_avc_surface->dmv_top,
                                  I915_GEM_DOMAIN_INSTRUCTION, 0,
                                  0);
            }
        } else {
            OUT_BCS_BATCH(ctx, 0);
            OUT_BCS_BATCH(ctx, 0);
        }
    }

    /* the current decoding frame/field */
    va_pic = &pic_param->CurrPic;
    assert(!(va_pic->flags & VA_PICTURE_H264_INVALID));
    obj_surface = SURFACE(va_pic->picture_id);
    assert(obj_surface && obj_surface->bo && obj_surface->private_data);
    gen6_avc_surface = obj_surface->private_data;

    OUT_BCS_RELOC(ctx, gen6_avc_surface->dmv_top,
                  I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                  0);

    if (gen6_avc_surface->dmv_bottom_flag == 1)
        OUT_BCS_RELOC(ctx, gen6_avc_surface->dmv_bottom,
                      I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                      0);
    else
        OUT_BCS_RELOC(ctx, gen6_avc_surface->dmv_top,
                      I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                      0);

    /* POC List */
    for (i = 0; i < ARRAY_ELEMS(gen6_mfd_context->reference_surface); i++) {
        if (gen6_mfd_context->reference_surface[i].surface_id != VA_INVALID_ID) {
            int found = 0;
            for (j = 0; j < ARRAY_ELEMS(pic_param->ReferenceFrames); j++) {
                va_pic = &pic_param->ReferenceFrames[j];
                
                if (va_pic->flags & VA_PICTURE_H264_INVALID)
                    continue;

                if (va_pic->picture_id == gen6_mfd_context->reference_surface[i].surface_id) {
                    found = 1;
                    break;
                }
            }

            assert(found == 1);
            assert(!(va_pic->flags & VA_PICTURE_H264_INVALID));
            
            OUT_BCS_BATCH(ctx, va_pic->TopFieldOrderCnt);
            OUT_BCS_BATCH(ctx, va_pic->BottomFieldOrderCnt);
        } else {
            OUT_BCS_BATCH(ctx, 0);
            OUT_BCS_BATCH(ctx, 0);
        }
    }

    va_pic = &pic_param->CurrPic;
    OUT_BCS_BATCH(ctx, va_pic->TopFieldOrderCnt);
    OUT_BCS_BATCH(ctx, va_pic->BottomFieldOrderCnt);

    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_avc_slice_state(VADriverContextP ctx,
                         VAPictureParameterBufferH264 *pic_param,
                         VASliceParameterBufferH264 *slice_param,
                         VASliceParameterBufferH264 *next_slice_param)
{
    int width_in_mbs = pic_param->picture_width_in_mbs_minus1 + 1;
    int height_in_mbs = pic_param->picture_height_in_mbs_minus1 + 1;
    int slice_hor_pos, slice_ver_pos, next_slice_hor_pos, next_slice_ver_pos;
    int num_ref_idx_l0, num_ref_idx_l1;
    int mbaff_picture = (!pic_param->pic_fields.bits.field_pic_flag &&
                         pic_param->seq_fields.bits.mb_adaptive_frame_field_flag);
    int weighted_pred_idc = 0;
    int first_mb_in_slice = 0, first_mb_in_next_slice = 0;
    int slice_type;

    if (slice_param->slice_type == SLICE_TYPE_I ||
        slice_param->slice_type == SLICE_TYPE_SI) {
        slice_type = SLICE_TYPE_I;
    } else if (slice_param->slice_type == SLICE_TYPE_P ||
               slice_param->slice_type == SLICE_TYPE_SP) {
        slice_type = SLICE_TYPE_P;
    } else { 
        assert(slice_param->slice_type == SLICE_TYPE_B);
        slice_type = SLICE_TYPE_B;
    }

    if (slice_type == SLICE_TYPE_I) {
        assert(slice_param->num_ref_idx_l0_active_minus1 == 0);
        assert(slice_param->num_ref_idx_l1_active_minus1 == 0);
        num_ref_idx_l0 = 0;
        num_ref_idx_l1 = 0;
    } else if (slice_type == SLICE_TYPE_P) {
        assert(slice_param->num_ref_idx_l1_active_minus1 == 0);
        num_ref_idx_l0 = slice_param->num_ref_idx_l0_active_minus1 + 1;
        num_ref_idx_l1 = 0;
        weighted_pred_idc = (pic_param->pic_fields.bits.weighted_pred_flag == 1);
    } else {
        num_ref_idx_l0 = slice_param->num_ref_idx_l0_active_minus1 + 1;
        num_ref_idx_l1 = slice_param->num_ref_idx_l1_active_minus1 + 1;
        weighted_pred_idc = (pic_param->pic_fields.bits.weighted_bipred_idc == 1);
    }

    first_mb_in_slice = slice_param->first_mb_in_slice << mbaff_picture;
    slice_hor_pos = first_mb_in_slice % width_in_mbs; 
    slice_ver_pos = first_mb_in_slice / width_in_mbs;

    if (next_slice_param) {
        first_mb_in_next_slice = next_slice_param->first_mb_in_slice << mbaff_picture;
        next_slice_hor_pos = first_mb_in_next_slice % width_in_mbs; 
        next_slice_ver_pos = first_mb_in_next_slice / width_in_mbs;
    } else {
        next_slice_hor_pos = 0;
        next_slice_ver_pos = height_in_mbs;
    }

    BEGIN_BCS_BATCH(ctx, 11); /* FIXME: is it 10??? */
    OUT_BCS_BATCH(ctx, MFX_AVC_SLICE_STATE | (11 - 2));
    OUT_BCS_BATCH(ctx, slice_type);
    OUT_BCS_BATCH(ctx, 
                  (num_ref_idx_l1 << 24) |
                  (num_ref_idx_l0 << 16) |
                  (slice_param->chroma_log2_weight_denom << 8) |
                  (slice_param->luma_log2_weight_denom << 0));
    OUT_BCS_BATCH(ctx, 
                  (weighted_pred_idc << 30) |
                  (slice_param->direct_spatial_mv_pred_flag << 29) |
                  (slice_param->disable_deblocking_filter_idc << 27) |
                  (slice_param->cabac_init_idc << 24) |
                  ((pic_param->pic_init_qp_minus26 + 26 + slice_param->slice_qp_delta) << 16) |
                  ((slice_param->slice_beta_offset_div2 & 0xf) << 8) |
                  ((slice_param->slice_alpha_c0_offset_div2 & 0xf) << 0));
    OUT_BCS_BATCH(ctx, 
                  (slice_ver_pos << 24) |
                  (slice_hor_pos << 16) | 
                  (first_mb_in_slice << 0));
    OUT_BCS_BATCH(ctx,
                  (next_slice_ver_pos << 16) |
                  (next_slice_hor_pos << 0));
    OUT_BCS_BATCH(ctx, 
                  (next_slice_param == NULL) << 19); /* last slice flag */
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_avc_phantom_slice_state(VADriverContextP ctx, VAPictureParameterBufferH264 *pic_param)
{
    int width_in_mbs = pic_param->picture_width_in_mbs_minus1 + 1;
    int height_in_mbs = pic_param->picture_height_in_mbs_minus1 + 1; /* frame height */

    BEGIN_BCS_BATCH(ctx, 11); /* FIXME: is it 10??? */
    OUT_BCS_BATCH(ctx, MFX_AVC_SLICE_STATE | (11 - 2));
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx,
                  height_in_mbs << 24 |
                  width_in_mbs * height_in_mbs / (1 + !!pic_param->pic_fields.bits.field_pic_flag));
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_avc_ref_idx_state(VADriverContextP ctx,
                           VAPictureParameterBufferH264 *pic_param,
                           VASliceParameterBufferH264 *slice_param)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct gen6_mfd_context *gen6_mfd_context = media_state->private_context;
    int i, j, num_ref_list;
    struct {
        unsigned char bottom_idc:1;
        unsigned char frame_store_index:4;
        unsigned char field_picture:1;
        unsigned char long_term:1;
        unsigned char non_exist:1;
    } refs[32];

    if (slice_param->slice_type == SLICE_TYPE_I ||
        slice_param->slice_type == SLICE_TYPE_SI)
        return;

    if (slice_param->slice_type == SLICE_TYPE_P ||
        slice_param->slice_type == SLICE_TYPE_SP) {
        num_ref_list = 1;
    } else {
        num_ref_list = 2;
    }

    for (i = 0; i < num_ref_list; i++) {
        VAPictureH264 *va_pic;

        if (i == 0) {
            va_pic = slice_param->RefPicList0;
        } else {
            va_pic = slice_param->RefPicList1;
        }

        BEGIN_BCS_BATCH(ctx, 10);
        OUT_BCS_BATCH(ctx, MFX_AVC_REF_IDX_STATE | (10 - 2));
        OUT_BCS_BATCH(ctx, i);

        for (j = 0; j < 32; j++) {
            if (va_pic->flags & VA_PICTURE_H264_INVALID) {
                refs[j].non_exist = 1;
                refs[j].long_term = 1;
                refs[j].field_picture = 1;
                refs[j].frame_store_index = 0xf;
                refs[j].bottom_idc = 1;
            } else {
                int frame_idx;
                
                for (frame_idx = 0; frame_idx < ARRAY_ELEMS(gen6_mfd_context->reference_surface); frame_idx++) {
                    if (gen6_mfd_context->reference_surface[frame_idx].surface_id != VA_INVALID_ID &&
                        va_pic->picture_id == gen6_mfd_context->reference_surface[frame_idx].surface_id) {
                        assert(frame_idx == gen6_mfd_context->reference_surface[frame_idx].frame_store_id);
                        break;
                    }
                }

                assert(frame_idx < ARRAY_ELEMS(gen6_mfd_context->reference_surface));
                
                refs[j].non_exist = 0;
                refs[j].long_term = !!(va_pic->flags & VA_PICTURE_H264_LONG_TERM_REFERENCE);
                refs[j].field_picture = !!(va_pic->flags & 
                                           (VA_PICTURE_H264_TOP_FIELD | 
                                            VA_PICTURE_H264_BOTTOM_FIELD));
                refs[j].frame_store_index = frame_idx;
                refs[j].bottom_idc = !!(va_pic->flags & VA_PICTURE_H264_BOTTOM_FIELD);
            }

            va_pic++;
        }
        
        intel_batchbuffer_data_bcs(ctx, refs, sizeof(refs));
        ADVANCE_BCS_BATCH(ctx);
    }
}

static void
gen6_mfd_avc_weightoffset_state(VADriverContextP ctx,
                                VAPictureParameterBufferH264 *pic_param,
                                VASliceParameterBufferH264 *slice_param)
{
    int i, j, num_weight_offset_table = 0;
    short weightoffsets[32 * 6];

    if ((slice_param->slice_type == SLICE_TYPE_P ||
         slice_param->slice_type == SLICE_TYPE_SP) &&
        (pic_param->pic_fields.bits.weighted_pred_flag == 1)) {
        num_weight_offset_table = 1;
    }
    
    if ((slice_param->slice_type == SLICE_TYPE_B) &&
        (pic_param->pic_fields.bits.weighted_bipred_idc == 1)) {
        num_weight_offset_table = 2;
    }

    for (i = 0; i < num_weight_offset_table; i++) {
        BEGIN_BCS_BATCH(ctx, 98);
        OUT_BCS_BATCH(ctx, MFX_AVC_WEIGHTOFFSET_STATE | (98 - 2));
        OUT_BCS_BATCH(ctx, i);

        if (i == 0) {
            for (j = 0; j < 32; j++) {
                weightoffsets[j * 6 + 0] = slice_param->luma_weight_l0[j];
                weightoffsets[j * 6 + 1] = slice_param->luma_offset_l0[j];
                weightoffsets[j * 6 + 2] = slice_param->chroma_weight_l0[j][0];
                weightoffsets[j * 6 + 3] = slice_param->chroma_offset_l0[j][0];
                weightoffsets[j * 6 + 4] = slice_param->chroma_weight_l0[j][1];
                weightoffsets[j * 6 + 5] = slice_param->chroma_offset_l0[j][1];
            }
        } else {
            for (j = 0; j < 32; j++) {
                weightoffsets[j * 6 + 0] = slice_param->luma_weight_l1[j];
                weightoffsets[j * 6 + 1] = slice_param->luma_offset_l1[j];
                weightoffsets[j * 6 + 2] = slice_param->chroma_weight_l1[j][0];
                weightoffsets[j * 6 + 3] = slice_param->chroma_offset_l1[j][0];
                weightoffsets[j * 6 + 4] = slice_param->chroma_weight_l1[j][1];
                weightoffsets[j * 6 + 5] = slice_param->chroma_offset_l1[j][1];
            }
        }

        intel_batchbuffer_data_bcs(ctx, weightoffsets, sizeof(weightoffsets));
        ADVANCE_BCS_BATCH(ctx);
    }
}

static int
gen6_mfd_avc_get_slice_bit_offset(uint8_t *buf, int mode_flag, int in_slice_data_bit_offset)
{
    int out_slice_data_bit_offset;
    int slice_header_size = in_slice_data_bit_offset / 8;
    int i, j;

    for (i = 0, j = 0; i < slice_header_size; i++, j++) {
        if (!buf[j] && !buf[j + 1] && buf[j + 2] == 3) {
            i++, j += 2;
        }
    }

    out_slice_data_bit_offset = 8 * j + in_slice_data_bit_offset % 8;

    if (mode_flag == ENTROPY_CABAC)
        out_slice_data_bit_offset = ALIGN(out_slice_data_bit_offset, 0x8);

    return out_slice_data_bit_offset;
}

static void
gen6_mfd_avc_bsd_object(VADriverContextP ctx,
                        VAPictureParameterBufferH264 *pic_param,
                        VASliceParameterBufferH264 *slice_param,
                        dri_bo *slice_data_bo)
{
    int slice_data_bit_offset;
    uint8_t *slice_data = NULL;

    dri_bo_map(slice_data_bo, 0);
    slice_data = (uint8_t *)(slice_data_bo->virtual + slice_param->slice_data_offset);
    slice_data_bit_offset = gen6_mfd_avc_get_slice_bit_offset(slice_data,
                                                              pic_param->pic_fields.bits.entropy_coding_mode_flag,
                                                              slice_param->slice_data_bit_offset);
    dri_bo_unmap(slice_data_bo);

    BEGIN_BCS_BATCH(ctx, 6);
    OUT_BCS_BATCH(ctx, MFD_AVC_BSD_OBJECT | (6 - 2));
    OUT_BCS_BATCH(ctx, 
                  ((slice_param->slice_data_size - (slice_data_bit_offset >> 3)) << 0));
    OUT_BCS_BATCH(ctx, slice_param->slice_data_offset + (slice_data_bit_offset >> 3));
    OUT_BCS_BATCH(ctx,
                  (0 << 31) |
                  (0 << 14) |
                  (0 << 12) |
                  (0 << 10) |
                  (0 << 8));
    OUT_BCS_BATCH(ctx,
                  (0 << 16) |
                  (0 << 6)  |
                  ((0x7 - (slice_data_bit_offset & 0x7)) << 0));
    OUT_BCS_BATCH(ctx, 0);
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_avc_phantom_slice_bsd_object(VADriverContextP ctx, VAPictureParameterBufferH264 *pic_param)
{
    BEGIN_BCS_BATCH(ctx, 6);
    OUT_BCS_BATCH(ctx, MFD_AVC_BSD_OBJECT | (6 - 2));
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_avc_phantom_slice(VADriverContextP ctx, VAPictureParameterBufferH264 *pic_param)
{
    gen6_mfd_avc_phantom_slice_state(ctx, pic_param);
    gen6_mfd_avc_phantom_slice_bsd_object(ctx, pic_param);
}

static void
gen6_mfd_avc_decode_init(VADriverContextP ctx, struct decode_state *decode_state)
{
    VAPictureParameterBufferH264 *pic_param;
    VASliceParameterBufferH264 *slice_param;
    VAPictureH264 *va_pic;
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct gen6_mfd_context *gen6_mfd_context;
    struct object_surface *obj_surface;
    dri_bo *bo;
    int i, j, enable_avc_ildb = 0;
    
    for (j = 0; j < decode_state->num_slice_params && enable_avc_ildb == 0; j++) {
        assert(decode_state->slice_params && decode_state->slice_params[j]->buffer);
        slice_param = (VASliceParameterBufferH264 *)decode_state->slice_params[j]->buffer;

        assert(decode_state->slice_params[j]->num_elements == 1);
        for (i = 0; i < decode_state->slice_params[j]->num_elements; i++) {
            assert(slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_ALL);
            assert((slice_param->slice_type == SLICE_TYPE_I) ||
                   (slice_param->slice_type == SLICE_TYPE_SI) ||
                   (slice_param->slice_type == SLICE_TYPE_P) ||
                   (slice_param->slice_type == SLICE_TYPE_SP) ||
                   (slice_param->slice_type == SLICE_TYPE_B));

            if (slice_param->disable_deblocking_filter_idc != 1) {
                enable_avc_ildb = 1;
                break;
            }

            slice_param++;
        }
    }

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferH264 *)decode_state->pic_param->buffer;
    gen6_mfd_context = media_state->private_context;

    if (gen6_mfd_context == NULL) {
        gen6_mfd_context = calloc(1, sizeof(struct gen6_mfd_context));
        media_state->private_context = gen6_mfd_context;

        for (i = 0; i < ARRAY_ELEMS(gen6_mfd_context->reference_surface); i++) {
            gen6_mfd_context->reference_surface[i].surface_id = VA_INVALID_ID;
            gen6_mfd_context->reference_surface[i].frame_store_id = -1;
        }
    }

    gen6_mfd_avc_frame_store_index(ctx, pic_param);

    /* Current decoded picture */
    va_pic = &pic_param->CurrPic;
    assert(!(va_pic->flags & VA_PICTURE_H264_INVALID));
    obj_surface = SURFACE(va_pic->picture_id);
    assert(obj_surface);
    obj_surface->flags &= ~SURFACE_REF_DIS_MASK;
    obj_surface->flags |= (pic_param->pic_fields.bits.reference_pic_flag ? SURFACE_REFERENCED : 0);
    gen6_mfd_init_avc_surface(ctx, pic_param, obj_surface);

    if (obj_surface->bo == NULL) {
        uint32_t tiling_mode = I915_TILING_Y;
        unsigned long pitch;
        
        obj_surface->bo = drm_intel_bo_alloc_tiled(i965->intel.bufmgr, 
                                                   "vaapi surface",
                                                   obj_surface->width, 
                                                   obj_surface->height + obj_surface->height / 2,
                                                   1,
                                                   &tiling_mode,
                                                   &pitch,
                                                   0);
        assert(obj_surface->bo);
        assert(tiling_mode == I915_TILING_Y);
        assert(pitch == obj_surface->width);
    }
    
    dri_bo_unreference(gen6_mfd_context->post_deblocking_output.bo);
    gen6_mfd_context->post_deblocking_output.bo = obj_surface->bo;
    dri_bo_reference(gen6_mfd_context->post_deblocking_output.bo);
    gen6_mfd_context->post_deblocking_output.valid = enable_avc_ildb;

    dri_bo_unreference(gen6_mfd_context->pre_deblocking_output.bo);
    gen6_mfd_context->pre_deblocking_output.bo = obj_surface->bo;
    dri_bo_reference(gen6_mfd_context->pre_deblocking_output.bo);
    gen6_mfd_context->pre_deblocking_output.valid = !enable_avc_ildb;

    dri_bo_unreference(gen6_mfd_context->intra_row_store_scratch_buffer.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "intra row store",
                      128 * 64,
                      0x1000);
    assert(bo);
    gen6_mfd_context->intra_row_store_scratch_buffer.bo = bo;
    gen6_mfd_context->intra_row_store_scratch_buffer.valid = 1;

    dri_bo_unreference(gen6_mfd_context->deblocking_filter_row_store_scratch_buffer.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "deblocking filter row store",
                      30720, /* 4 * 120 * 64 */
                      0x1000);
    assert(bo);
    gen6_mfd_context->deblocking_filter_row_store_scratch_buffer.bo = bo;
    gen6_mfd_context->deblocking_filter_row_store_scratch_buffer.valid = 1;

    dri_bo_unreference(gen6_mfd_context->bsd_mpc_row_store_scratch_buffer.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "bsd mpc row store",
                      11520, /* 1.5 * 120 * 64 */
                      0x1000);
    assert(bo);
    gen6_mfd_context->bsd_mpc_row_store_scratch_buffer.bo = bo;
    gen6_mfd_context->bsd_mpc_row_store_scratch_buffer.valid = 1;

    dri_bo_unreference(gen6_mfd_context->mpr_row_store_scratch_buffer.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "mpr row store",
                      7680, /* 1. 0 * 120 * 64 */
                      0x1000);
    assert(bo);
    gen6_mfd_context->mpr_row_store_scratch_buffer.bo = bo;
    gen6_mfd_context->mpr_row_store_scratch_buffer.valid = 1;

    gen6_mfd_context->bitplane_read_buffer.valid = 0;
}

static void
gen6_mfd_avc_decode_picture(VADriverContextP ctx, struct decode_state *decode_state)
{
    VAPictureParameterBufferH264 *pic_param;
    VASliceParameterBufferH264 *slice_param, *next_slice_param;
    dri_bo *slice_data_bo;
    int i, j;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferH264 *)decode_state->pic_param->buffer;

    gen6_mfd_avc_decode_init(ctx, decode_state);
    intel_batchbuffer_start_atomic_bcs(ctx, 0x1000);
    intel_batchbuffer_emit_mi_flush_bcs(ctx);
    gen6_mfd_pipe_mode_select(ctx, decode_state, MFX_FORMAT_AVC);
    gen6_mfd_surface_state(ctx, decode_state, MFX_FORMAT_AVC);
    gen6_mfd_pipe_buf_addr_state(ctx, decode_state, MFX_FORMAT_AVC);
    gen6_mfd_bsp_buf_base_addr_state(ctx, decode_state, MFX_FORMAT_AVC);
    gen6_mfd_avc_img_state(ctx, decode_state);
    gen6_mfd_avc_qm_state(ctx, decode_state);

    for (j = 0; j < decode_state->num_slice_params; j++) {
        assert(decode_state->slice_params && decode_state->slice_params[j]->buffer);
        slice_param = (VASliceParameterBufferH264 *)decode_state->slice_params[j]->buffer;
        slice_data_bo = decode_state->slice_datas[j]->bo;

        if (j == decode_state->num_slice_params - 1)
            next_slice_param = NULL;
        else
            next_slice_param = (VASliceParameterBufferH264 *)decode_state->slice_params[j + 1]->buffer;

        gen6_mfd_ind_obj_base_addr_state(ctx, slice_data_bo, MFX_FORMAT_AVC);
        assert(decode_state->slice_params[j]->num_elements == 1);

        for (i = 0; i < decode_state->slice_params[j]->num_elements; i++) {
            assert(slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_ALL);
            assert((slice_param->slice_type == SLICE_TYPE_I) ||
                   (slice_param->slice_type == SLICE_TYPE_SI) ||
                   (slice_param->slice_type == SLICE_TYPE_P) ||
                   (slice_param->slice_type == SLICE_TYPE_SP) ||
                   (slice_param->slice_type == SLICE_TYPE_B));

            if (i < decode_state->slice_params[j]->num_elements - 1)
                next_slice_param = slice_param + 1;

            gen6_mfd_avc_directmode_state(ctx, pic_param, slice_param);
            gen6_mfd_avc_slice_state(ctx, pic_param, slice_param, next_slice_param);
            gen6_mfd_avc_ref_idx_state(ctx, pic_param, slice_param);
            gen6_mfd_avc_weightoffset_state(ctx, pic_param, slice_param);
            gen6_mfd_avc_bsd_object(ctx, pic_param, slice_param, slice_data_bo);
            slice_param++;
        }
    }
    
    gen6_mfd_avc_phantom_slice(ctx, pic_param);
    intel_batchbuffer_end_atomic_bcs(ctx);
    intel_batchbuffer_flush_bcs(ctx);
}

static void
gen6_mfd_mpeg2_decode_init(VADriverContextP ctx, struct decode_state *decode_state)
{
    VAPictureParameterBufferMPEG2 *pic_param;
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct gen6_mfd_context *gen6_mfd_context;
    struct object_surface *obj_surface;
    int i;
    dri_bo *bo;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferMPEG2 *)decode_state->pic_param->buffer;
    gen6_mfd_context = media_state->private_context;

    if (gen6_mfd_context == NULL) {
        gen6_mfd_context = calloc(1, sizeof(struct gen6_mfd_context));
        media_state->private_context = gen6_mfd_context;

        for (i = 0; i < ARRAY_ELEMS(gen6_mfd_context->reference_surface); i++) {
            gen6_mfd_context->reference_surface[i].surface_id = VA_INVALID_ID;
            gen6_mfd_context->reference_surface[i].frame_store_id = -1;
        }
    }

    /* reference picture */
    obj_surface = SURFACE(pic_param->forward_reference_picture);

    if (obj_surface && obj_surface->bo)
        gen6_mfd_context->reference_surface[0].surface_id = pic_param->forward_reference_picture;
    else
        gen6_mfd_context->reference_surface[0].surface_id = VA_INVALID_ID;

    obj_surface = SURFACE(pic_param->backward_reference_picture);

    if (obj_surface && obj_surface->bo)
        gen6_mfd_context->reference_surface[1].surface_id = pic_param->backward_reference_picture;
    else
        gen6_mfd_context->reference_surface[1].surface_id = pic_param->forward_reference_picture;

    /* must do so !!! */
    for (i = 2; i < ARRAY_ELEMS(gen6_mfd_context->reference_surface); i++)
        gen6_mfd_context->reference_surface[i].surface_id = gen6_mfd_context->reference_surface[i % 2].surface_id;

    /* Current decoded picture */
    obj_surface = SURFACE(decode_state->current_render_target);
    assert(obj_surface);
    if (obj_surface->bo == NULL) {
        uint32_t tiling_mode = I915_TILING_Y;
        unsigned long pitch;

        obj_surface->bo = drm_intel_bo_alloc_tiled(i965->intel.bufmgr, 
                                                   "vaapi surface",
                                                   obj_surface->width, 
                                                   obj_surface->height + obj_surface->height / 2,
                                                   1,
                                                   &tiling_mode,
                                                   &pitch,
                                                   0);
        assert(obj_surface->bo);
        assert(tiling_mode == I915_TILING_Y);
        assert(pitch == obj_surface->width);
    }

    dri_bo_unreference(gen6_mfd_context->pre_deblocking_output.bo);
    gen6_mfd_context->pre_deblocking_output.bo = obj_surface->bo;
    dri_bo_reference(gen6_mfd_context->pre_deblocking_output.bo);
    gen6_mfd_context->pre_deblocking_output.valid = 1;

    dri_bo_unreference(gen6_mfd_context->bsd_mpc_row_store_scratch_buffer.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "bsd mpc row store",
                      11520, /* 1.5 * 120 * 64 */
                      0x1000);
    assert(bo);
    gen6_mfd_context->bsd_mpc_row_store_scratch_buffer.bo = bo;
    gen6_mfd_context->bsd_mpc_row_store_scratch_buffer.valid = 1;

    gen6_mfd_context->post_deblocking_output.valid = 0;
    gen6_mfd_context->intra_row_store_scratch_buffer.valid = 0;
    gen6_mfd_context->deblocking_filter_row_store_scratch_buffer.valid = 0;
    gen6_mfd_context->mpr_row_store_scratch_buffer.valid = 0;
    gen6_mfd_context->bitplane_read_buffer.valid = 0;
}

static void
gen6_mfd_mpeg2_pic_state(VADriverContextP ctx, struct decode_state *decode_state)
{
    VAPictureParameterBufferMPEG2 *pic_param;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferMPEG2 *)decode_state->pic_param->buffer;

    BEGIN_BCS_BATCH(ctx, 4);
    OUT_BCS_BATCH(ctx, MFX_MPEG2_PIC_STATE | (4 - 2));
    OUT_BCS_BATCH(ctx,
                  (pic_param->f_code & 0xf) << 28 | /* f_code[1][1] */
                  ((pic_param->f_code >> 4) & 0xf) << 24 | /* f_code[1][0] */
                  ((pic_param->f_code >> 8) & 0xf) << 20 | /* f_code[0][1] */
                  ((pic_param->f_code >> 12) & 0xf) << 16 | /* f_code[0][0] */
                  pic_param->picture_coding_extension.bits.intra_dc_precision << 14 |
                  pic_param->picture_coding_extension.bits.picture_structure << 12 |
                  pic_param->picture_coding_extension.bits.top_field_first << 11 |
                  pic_param->picture_coding_extension.bits.frame_pred_frame_dct << 10 |
                  pic_param->picture_coding_extension.bits.concealment_motion_vectors << 9 |
                  pic_param->picture_coding_extension.bits.q_scale_type << 8 |
                  pic_param->picture_coding_extension.bits.intra_vlc_format << 7 | 
                  pic_param->picture_coding_extension.bits.alternate_scan << 6);
    OUT_BCS_BATCH(ctx,
                  pic_param->picture_coding_type << 9);
    OUT_BCS_BATCH(ctx,
                  (ALIGN(pic_param->vertical_size, 16) / 16) << 16 |
                  (ALIGN(pic_param->horizontal_size, 16) / 16));
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_mpeg2_qm_state(VADriverContextP ctx, struct decode_state *decode_state)
{
    VAIQMatrixBufferMPEG2 *iq_matrix;
    int i;

    if (!decode_state->iq_matrix || !decode_state->iq_matrix->buffer)
        return;

    iq_matrix = (VAIQMatrixBufferMPEG2 *)decode_state->iq_matrix->buffer;

    for (i = 0; i < 2; i++) {
        int k, m;
        unsigned char *qm = NULL;
        unsigned char qmx[64];

        if (i == 0) {
            if (iq_matrix->load_intra_quantiser_matrix)
                qm = iq_matrix->intra_quantiser_matrix;
        } else {
            if (iq_matrix->load_non_intra_quantiser_matrix)
                qm = iq_matrix->non_intra_quantiser_matrix;
        }

        if (!qm)
            continue;

        /* Upload quantisation matrix in raster order. The mplayer vaapi
         * patch passes quantisation matrix in zig-zag order to va library.
         */
        for (k = 0; k < 64; k++) {
            m = zigzag_direct[k];
            qmx[m] = qm[k];
        }

        BEGIN_BCS_BATCH(ctx, 18);
        OUT_BCS_BATCH(ctx, MFX_MPEG2_QM_STATE | (18 - 2));
        OUT_BCS_BATCH(ctx, i);
        intel_batchbuffer_data_bcs(ctx, qmx, 64);
        ADVANCE_BCS_BATCH(ctx);
    }
}

static void
gen6_mfd_mpeg2_bsd_object(VADriverContextP ctx,
                          VAPictureParameterBufferMPEG2 *pic_param,
                          VASliceParameterBufferMPEG2 *slice_param,
                          VASliceParameterBufferMPEG2 *next_slice_param)
{
    unsigned int width_in_mbs = ALIGN(pic_param->horizontal_size, 16) / 16;
    unsigned int height_in_mbs = ALIGN(pic_param->vertical_size, 16) / 16;
    int mb_count;

    if (next_slice_param == NULL)
        mb_count = width_in_mbs * height_in_mbs - 
            (slice_param->slice_vertical_position * width_in_mbs + slice_param->slice_horizontal_position);
    else
        mb_count = (next_slice_param->slice_vertical_position * width_in_mbs + next_slice_param->slice_horizontal_position) - 
            (slice_param->slice_vertical_position * width_in_mbs + slice_param->slice_horizontal_position);

    BEGIN_BCS_BATCH(ctx, 5);
    OUT_BCS_BATCH(ctx, MFD_MPEG2_BSD_OBJECT | (5 - 2));
    OUT_BCS_BATCH(ctx, 
                  slice_param->slice_data_size - (slice_param->macroblock_offset >> 3));
    OUT_BCS_BATCH(ctx, 
                  slice_param->slice_data_offset + (slice_param->macroblock_offset >> 3));
    OUT_BCS_BATCH(ctx,
                  slice_param->slice_horizontal_position << 24 |
                  slice_param->slice_vertical_position << 16 |
                  mb_count << 8 |
                  (next_slice_param == NULL) << 5 |
                  (next_slice_param == NULL) << 3 |
                  (slice_param->macroblock_offset & 0x7));
    OUT_BCS_BATCH(ctx,
                  slice_param->quantiser_scale_code << 24);
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_mpeg2_decode_picture(VADriverContextP ctx, struct decode_state *decode_state)
{
    VAPictureParameterBufferMPEG2 *pic_param;
    VASliceParameterBufferMPEG2 *slice_param, *next_slice_param;
    dri_bo *slice_data_bo;
    int i, j;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferMPEG2 *)decode_state->pic_param->buffer;

    gen6_mfd_mpeg2_decode_init(ctx, decode_state);
    intel_batchbuffer_start_atomic_bcs(ctx, 0x1000);
    intel_batchbuffer_emit_mi_flush_bcs(ctx);
    gen6_mfd_pipe_mode_select(ctx, decode_state, MFX_FORMAT_MPEG2);
    gen6_mfd_surface_state(ctx, decode_state, MFX_FORMAT_MPEG2);
    gen6_mfd_pipe_buf_addr_state(ctx, decode_state, MFX_FORMAT_MPEG2);
    gen6_mfd_bsp_buf_base_addr_state(ctx, decode_state, MFX_FORMAT_MPEG2);
    gen6_mfd_mpeg2_pic_state(ctx, decode_state);
    gen6_mfd_mpeg2_qm_state(ctx, decode_state);

    assert(decode_state->num_slice_params == 1);
    for (j = 0; j < decode_state->num_slice_params; j++) {
        assert(decode_state->slice_params && decode_state->slice_params[j]->buffer);
        slice_param = (VASliceParameterBufferMPEG2 *)decode_state->slice_params[j]->buffer;
        slice_data_bo = decode_state->slice_datas[j]->bo;
        gen6_mfd_ind_obj_base_addr_state(ctx, slice_data_bo, MFX_FORMAT_MPEG2);

        for (i = 0; i < decode_state->slice_params[j]->num_elements; i++) {
            assert(slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_ALL);

            if (i < decode_state->slice_params[j]->num_elements - 1)
                next_slice_param = slice_param + 1;
            else
                next_slice_param = NULL;

            gen6_mfd_mpeg2_bsd_object(ctx, pic_param, slice_param, next_slice_param);
            slice_param++;
        }
    }

    intel_batchbuffer_end_atomic_bcs(ctx);
    intel_batchbuffer_flush_bcs(ctx);
}

static const int va_to_gen6_vc1_pic_type[5] = {
    GEN6_VC1_I_PICTURE,
    GEN6_VC1_P_PICTURE,
    GEN6_VC1_B_PICTURE,
    GEN6_VC1_BI_PICTURE,
    GEN6_VC1_P_PICTURE,
};

static const int va_to_gen6_vc1_mv[4] = {
    1, /* 1-MV */
    2, /* 1-MV half-pel */
    3, /* 1-MV half-pef bilinear */
    0, /* Mixed MV */
};

static const int b_picture_scale_factor[21] = {
    128, 85,  170, 64,  192,
    51,  102, 153, 204, 43,
    215, 37,  74,  111, 148,
    185, 222, 32,  96,  160, 
    224,
};

static const int va_to_gen6_vc1_condover[3] = {
    0,
    2,
    3
};

static const int va_to_gen6_vc1_profile[4] = {
    GEN6_VC1_SIMPLE_PROFILE,
    GEN6_VC1_MAIN_PROFILE,
    GEN6_VC1_RESERVED_PROFILE,
    GEN6_VC1_ADVANCED_PROFILE
};

static const int va_to_gen6_vc1_ttfrm[8] = {
    0,  /* 8x8 */
    1,  /* 8x4 bottom */
    1,  /* 8x4 top */
    1,  /* 8x4 */
    2,  /* 4x8 bottom */
    2,  /* 4x8 top */
    2,  /* 4x8 */
    3,  /* 4x4 */
};

static void 
gen6_mfd_free_vc1_surface(void **data)
{
    struct gen6_vc1_surface *gen6_vc1_surface = *data;

    if (!gen6_vc1_surface)
        return;

    dri_bo_unreference(gen6_vc1_surface->dmv);
    free(gen6_vc1_surface);
    *data = NULL;
}

static void
gen6_mfd_init_vc1_surface(VADriverContextP ctx, 
                          VAPictureParameterBufferVC1 *pic_param,
                          struct object_surface *obj_surface)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen6_vc1_surface *gen6_vc1_surface = obj_surface->private_data;

    obj_surface->free_private_data = gen6_mfd_free_vc1_surface;

    if (!gen6_vc1_surface) {
        gen6_vc1_surface = calloc(sizeof(struct gen6_vc1_surface), 1);
        assert((obj_surface->size & 0x3f) == 0);
        obj_surface->private_data = gen6_vc1_surface;
    }

    gen6_vc1_surface->picture_type = pic_param->picture_fields.bits.picture_type;

    if (gen6_vc1_surface->dmv == NULL) {
        gen6_vc1_surface->dmv = dri_bo_alloc(i965->intel.bufmgr,
                                             "direct mv w/r buffer",
                                             557056,    /* 64 * 128 * 64 */
                                             0x1000);
    }
}

static void
gen6_mfd_vc1_decode_init(VADriverContextP ctx, struct decode_state *decode_state)
{
    VAPictureParameterBufferVC1 *pic_param;
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct gen6_mfd_context *gen6_mfd_context;
    struct object_surface *obj_surface;
    int i;
    dri_bo *bo;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferVC1 *)decode_state->pic_param->buffer;
    gen6_mfd_context = media_state->private_context;

    if (gen6_mfd_context == NULL) {
        gen6_mfd_context = calloc(1, sizeof(struct gen6_mfd_context));
        media_state->private_context = gen6_mfd_context;

        for (i = 0; i < ARRAY_ELEMS(gen6_mfd_context->reference_surface); i++) {
            gen6_mfd_context->reference_surface[i].surface_id = VA_INVALID_ID;
            gen6_mfd_context->reference_surface[i].frame_store_id = -1;
        }
    }

    /* reference picture */
    obj_surface = SURFACE(pic_param->forward_reference_picture);

    if (obj_surface && obj_surface->bo)
        gen6_mfd_context->reference_surface[0].surface_id = pic_param->forward_reference_picture;
    else
        gen6_mfd_context->reference_surface[0].surface_id = VA_INVALID_ID;

    obj_surface = SURFACE(pic_param->backward_reference_picture);

    if (obj_surface && obj_surface->bo)
        gen6_mfd_context->reference_surface[1].surface_id = pic_param->backward_reference_picture;
    else
        gen6_mfd_context->reference_surface[1].surface_id = pic_param->forward_reference_picture;

    /* must do so !!! */
    for (i = 2; i < ARRAY_ELEMS(gen6_mfd_context->reference_surface); i++)
        gen6_mfd_context->reference_surface[i].surface_id = gen6_mfd_context->reference_surface[i % 2].surface_id;

    /* Current decoded picture */
    obj_surface = SURFACE(decode_state->current_render_target);
    assert(obj_surface);
    gen6_mfd_init_vc1_surface(ctx, pic_param, obj_surface);

    if (obj_surface->bo == NULL) {
        uint32_t tiling_mode = I915_TILING_Y;
        unsigned long pitch;

        obj_surface->bo = drm_intel_bo_alloc_tiled(i965->intel.bufmgr, 
                                                   "vaapi surface",
                                                   obj_surface->width, 
                                                   obj_surface->height + obj_surface->height / 2,
                                                   1,
                                                   &tiling_mode,
                                                   &pitch,
                                                   0);
        assert(obj_surface->bo);
        assert(tiling_mode == I915_TILING_Y);
        assert(pitch == obj_surface->width);
    }

    dri_bo_unreference(gen6_mfd_context->post_deblocking_output.bo);
    gen6_mfd_context->post_deblocking_output.bo = obj_surface->bo;
    dri_bo_reference(gen6_mfd_context->post_deblocking_output.bo);
    gen6_mfd_context->post_deblocking_output.valid = pic_param->entrypoint_fields.bits.loopfilter;

    dri_bo_unreference(gen6_mfd_context->pre_deblocking_output.bo);
    gen6_mfd_context->pre_deblocking_output.bo = obj_surface->bo;
    dri_bo_reference(gen6_mfd_context->pre_deblocking_output.bo);
    gen6_mfd_context->pre_deblocking_output.valid = !pic_param->entrypoint_fields.bits.loopfilter;

    dri_bo_unreference(gen6_mfd_context->intra_row_store_scratch_buffer.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "intra row store",
                      128 * 64,
                      0x1000);
    assert(bo);
    gen6_mfd_context->intra_row_store_scratch_buffer.bo = bo;
    gen6_mfd_context->intra_row_store_scratch_buffer.valid = 1;

    dri_bo_unreference(gen6_mfd_context->deblocking_filter_row_store_scratch_buffer.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "deblocking filter row store",
                      46080, /* 6 * 120 * 64 */
                      0x1000);
    assert(bo);
    gen6_mfd_context->deblocking_filter_row_store_scratch_buffer.bo = bo;
    gen6_mfd_context->deblocking_filter_row_store_scratch_buffer.valid = 1;

    dri_bo_unreference(gen6_mfd_context->bsd_mpc_row_store_scratch_buffer.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "bsd mpc row store",
                      11520, /* 1.5 * 120 * 64 */
                      0x1000);
    assert(bo);
    gen6_mfd_context->bsd_mpc_row_store_scratch_buffer.bo = bo;
    gen6_mfd_context->bsd_mpc_row_store_scratch_buffer.valid = 1;

    gen6_mfd_context->mpr_row_store_scratch_buffer.valid = 0;

    gen6_mfd_context->bitplane_read_buffer.valid = !!pic_param->bitplane_present.value;
    dri_bo_unreference(gen6_mfd_context->bitplane_read_buffer.bo);
    
    if (gen6_mfd_context->bitplane_read_buffer.valid) {
        int width_in_mbs = ALIGN(pic_param->coded_width, 16) / 16;
        int height_in_mbs = ALIGN(pic_param->coded_height, 16) / 16;
        int bitplane_width = ALIGN(width_in_mbs, 2) / 2;
        int bitplane_height = height_in_mbs;
        int src_w, src_h, dst_w, dst_h;
        uint8_t *src = NULL, *dst = NULL;

        assert(decode_state->bit_plane->buffer);
        src = decode_state->bit_plane->buffer;

        bo = dri_bo_alloc(i965->intel.bufmgr,
                          "VC-1 Bitplane",
                          bitplane_width * bitplane_width,
                          0x1000);
        assert(bo);
        gen6_mfd_context->bitplane_read_buffer.bo = bo;

        dri_bo_map(bo, True);
        assert(bo->virtual);
        dst = bo->virtual;

        for (src_h = 0; src_h < height_in_mbs; src_h++) {
            for(src_w = 0; src_w < width_in_mbs; src_w++) {
                int src_index, dst_index;
                int src_shift;
                uint8_t src_value;

                src_index = (src_h * width_in_mbs + src_w) / 2;
                src_shift = !((src_h * width_in_mbs + src_w) & 1) * 4;
                src_value = ((src[src_index] >> src_shift) & 0xf);

                dst_index = src_w / 2;
                dst[dst_index] = ((dst[dst_index] >> 4) | (src_value << 4));
            }

            if (src_w & 1)
                dst[src_w / 2] >>= 4;

            dst += bitplane_width;
        }

        dri_bo_unmap(bo);
    } else
        gen6_mfd_context->bitplane_read_buffer.bo = NULL;
}

static void
gen6_mfd_vc1_pic_state(VADriverContextP ctx, struct decode_state *decode_state)
{
    VAPictureParameterBufferVC1 *pic_param;
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_surface *obj_surface;
    int alt_pquant_config = 0, alt_pquant_edge_mask = 0, alt_pq;
    int dquant, dquantfrm, dqprofile, dqdbedge, dqsbedge, dqbilevel;
    int unified_mv_mode;
    int ref_field_pic_polarity = 0;
    int scale_factor = 0;
    int trans_ac_y = 0;
    int dmv_surface_valid = 0;
    int brfd = 0;
    int fcm = 0;
    int picture_type;
    int profile;
    int overlap;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferVC1 *)decode_state->pic_param->buffer;

    profile = va_to_gen6_vc1_profile[pic_param->sequence_fields.bits.profile];
    dquant = pic_param->pic_quantizer_fields.bits.dquant;
    dquantfrm = pic_param->pic_quantizer_fields.bits.dq_frame;
    dqprofile = pic_param->pic_quantizer_fields.bits.dq_profile;
    dqdbedge = pic_param->pic_quantizer_fields.bits.dq_db_edge;
    dqsbedge = pic_param->pic_quantizer_fields.bits.dq_sb_edge;
    dqbilevel = pic_param->pic_quantizer_fields.bits.dq_binary_level;
    alt_pq = pic_param->pic_quantizer_fields.bits.alt_pic_quantizer;

    if (dquant == 0) {
        alt_pquant_config = 0;
        alt_pquant_edge_mask = 0;
    } else if (dquant == 2) {
        alt_pquant_config = 1;
        alt_pquant_edge_mask = 0xf;
    } else {
        assert(dquant == 1);
        if (dquantfrm == 0) {
            alt_pquant_config = 0;
            alt_pquant_edge_mask = 0;
            alt_pq = 0;
        } else {
            assert(dquantfrm == 1);
            alt_pquant_config = 1;

            switch (dqprofile) {
            case 3:
                if (dqbilevel == 0) {
                    alt_pquant_config = 2;
                    alt_pquant_edge_mask = 0;
                } else {
                    assert(dqbilevel == 1);
                    alt_pquant_config = 3;
                    alt_pquant_edge_mask = 0;
                }
                break;
                
            case 0:
                alt_pquant_edge_mask = 0xf;
                break;

            case 1:
                if (dqdbedge == 3)
                    alt_pquant_edge_mask = 0x9;
                else
                    alt_pquant_edge_mask = (0x3 << dqdbedge);

                break;

            case 2:
                alt_pquant_edge_mask = (0x1 << dqsbedge);
                break;

            default:
                assert(0);
            }
        }
    }

    if (pic_param->mv_fields.bits.mv_mode == VAMvModeIntensityCompensation) {
        assert(pic_param->mv_fields.bits.mv_mode2 < 4);
        unified_mv_mode = va_to_gen6_vc1_mv[pic_param->mv_fields.bits.mv_mode2];
    } else {
        assert(pic_param->mv_fields.bits.mv_mode < 4);
        unified_mv_mode = va_to_gen6_vc1_mv[pic_param->mv_fields.bits.mv_mode];
    }

    if (pic_param->sequence_fields.bits.interlace == 1 &&
        pic_param->picture_fields.bits.frame_coding_mode != 0) { /* frame-interlace or field-interlace */
        /* FIXME: calculate reference field picture polarity */
        assert(0);
        ref_field_pic_polarity = 0;
    }

    if (pic_param->b_picture_fraction < 21)
        scale_factor = b_picture_scale_factor[pic_param->b_picture_fraction];

    picture_type = va_to_gen6_vc1_pic_type[pic_param->picture_fields.bits.picture_type];
    
    if (profile == GEN6_VC1_ADVANCED_PROFILE && 
        picture_type == GEN6_VC1_I_PICTURE)
        picture_type = GEN6_VC1_BI_PICTURE;

    if (picture_type == GEN6_VC1_I_PICTURE || picture_type == GEN6_VC1_BI_PICTURE) /* I picture */
        trans_ac_y = pic_param->transform_fields.bits.transform_ac_codingset_idx2;
    else
        trans_ac_y = pic_param->transform_fields.bits.transform_ac_codingset_idx1;


    if (picture_type == GEN6_VC1_B_PICTURE) {
        struct gen6_vc1_surface *gen6_vc1_surface = NULL;

        obj_surface = SURFACE(pic_param->backward_reference_picture);
        assert(obj_surface);
        gen6_vc1_surface = obj_surface->private_data;

        if (!gen6_vc1_surface || 
            (va_to_gen6_vc1_pic_type[gen6_vc1_surface->picture_type] == GEN6_VC1_I_PICTURE ||
             va_to_gen6_vc1_pic_type[gen6_vc1_surface->picture_type] == GEN6_VC1_BI_PICTURE))
            dmv_surface_valid = 0;
        else
            dmv_surface_valid = 1;
    }

    assert(pic_param->picture_fields.bits.frame_coding_mode < 3);

    if (pic_param->picture_fields.bits.frame_coding_mode < 2)
        fcm = pic_param->picture_fields.bits.frame_coding_mode;
    else {
        if (pic_param->picture_fields.bits.top_field_first)
            fcm = 2;
        else
            fcm = 3;
    }

    if (pic_param->picture_fields.bits.picture_type == GEN6_VC1_B_PICTURE) { /* B picture */
        brfd = pic_param->reference_fields.bits.reference_distance;
        brfd = (scale_factor * brfd) >> 8;
        brfd = pic_param->reference_fields.bits.reference_distance - brfd - 1;

        if (brfd < 0)
            brfd = 0;
    }

    overlap = pic_param->sequence_fields.bits.overlap;
    if (profile != GEN6_VC1_ADVANCED_PROFILE && pic_param->pic_quantizer_fields.bits.pic_quantizer_scale < 9)
        overlap = 0;

    assert(pic_param->conditional_overlap_flag < 3);
    assert(pic_param->mv_fields.bits.mv_table < 4); /* FIXME: interlace mode */

    BEGIN_BCS_BATCH(ctx, 6);
    OUT_BCS_BATCH(ctx, MFX_VC1_PIC_STATE | (6 - 2));
    OUT_BCS_BATCH(ctx,
                  (ALIGN(pic_param->coded_height, 16) / 16) << 16 |
                  (ALIGN(pic_param->coded_width, 16) / 16));
    OUT_BCS_BATCH(ctx,
                  pic_param->sequence_fields.bits.syncmarker << 31 |
                  1 << 29 | /* concealment */
                  alt_pq << 24 |
                  pic_param->entrypoint_fields.bits.loopfilter << 23 |
                  overlap << 22 |
                  (pic_param->pic_quantizer_fields.bits.quantizer == 0) << 21 | /* implicit quantizer */
                  pic_param->pic_quantizer_fields.bits.pic_quantizer_scale << 16 |
                  alt_pquant_edge_mask << 12 |
                  alt_pquant_config << 10 |
                  pic_param->pic_quantizer_fields.bits.half_qp << 9 |
                  pic_param->pic_quantizer_fields.bits.pic_quantizer_type << 8 |
                  va_to_gen6_vc1_condover[pic_param->conditional_overlap_flag] << 6 |
                  !pic_param->picture_fields.bits.is_first_field << 5 |
                  picture_type << 2 |
                  fcm << 0);
    OUT_BCS_BATCH(ctx,
                  !!pic_param->bitplane_present.value << 23 |
                  !pic_param->bitplane_present.flags.bp_forward_mb << 22 |
                  !pic_param->bitplane_present.flags.bp_mv_type_mb << 21 |
                  !pic_param->bitplane_present.flags.bp_skip_mb << 20 |
                  !pic_param->bitplane_present.flags.bp_direct_mb << 19 |
                  !pic_param->bitplane_present.flags.bp_overflags << 18 |
                  !pic_param->bitplane_present.flags.bp_ac_pred << 17 |
                  !pic_param->bitplane_present.flags.bp_field_tx << 16 |
                  pic_param->mv_fields.bits.extended_dmv_range << 14 |
                  pic_param->mv_fields.bits.extended_mv_range << 12 |
                  pic_param->mv_fields.bits.four_mv_switch << 11 |
                  pic_param->fast_uvmc_flag << 10 |
                  unified_mv_mode << 8 |
                  ref_field_pic_polarity << 6 |
                  pic_param->reference_fields.bits.num_reference_pictures << 5 |
                  pic_param->reference_fields.bits.reference_distance << 0);
    OUT_BCS_BATCH(ctx,
                  scale_factor << 24 |
                  pic_param->mv_fields.bits.mv_table << 20 |
                  pic_param->mv_fields.bits.four_mv_block_pattern_table << 18 |
                  pic_param->mv_fields.bits.two_mv_block_pattern_table << 16 |
                  va_to_gen6_vc1_ttfrm[pic_param->transform_fields.bits.frame_level_transform_type] << 12 |
                  pic_param->transform_fields.bits.mb_level_transform_type_flag << 11 |
                  pic_param->mb_mode_table << 8 |
                  trans_ac_y << 6 |
                  pic_param->transform_fields.bits.transform_ac_codingset_idx1 << 4 |
                  pic_param->transform_fields.bits.intra_transform_dc_table << 3 |
                  pic_param->cbp_table << 0);
    OUT_BCS_BATCH(ctx,
                  dmv_surface_valid << 13 |
                  brfd << 8 |
                  ((ALIGN(pic_param->coded_width, 16) / 16 + 1) / 2 - 1));
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_vc1_pred_pipe_state(VADriverContextP ctx, struct decode_state *decode_state)
{
    VAPictureParameterBufferVC1 *pic_param;
    int interpolation_mode = 0;
    int intensitycomp_single;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferVC1 *)decode_state->pic_param->buffer;

    if (pic_param->mv_fields.bits.mv_mode == VAMvMode1MvHalfPelBilinear ||
        (pic_param->mv_fields.bits.mv_mode == VAMvModeIntensityCompensation &&
         pic_param->mv_fields.bits.mv_mode2 == VAMvMode1MvHalfPelBilinear))
        interpolation_mode = 2; /* Half-pel bilinear */
    else if (pic_param->mv_fields.bits.mv_mode == VAMvMode1MvHalfPel ||
             (pic_param->mv_fields.bits.mv_mode == VAMvModeIntensityCompensation &&
              pic_param->mv_fields.bits.mv_mode2 == VAMvMode1MvHalfPel))
        interpolation_mode = 0; /* Half-pel bicubic */
    else
        interpolation_mode = 1; /* Quarter-pel bicubic */

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferVC1 *)decode_state->pic_param->buffer;
    intensitycomp_single = (pic_param->mv_fields.bits.mv_mode == VAMvModeIntensityCompensation);

    BEGIN_BCS_BATCH(ctx, 7);
    OUT_BCS_BATCH(ctx, MFX_VC1_PRED_PIPE_STATE | (7 - 2));
    OUT_BCS_BATCH(ctx,
                  0 << 8 | /* FIXME: interlace mode */
                  pic_param->rounding_control << 4 |
                  va_to_gen6_vc1_profile[pic_param->sequence_fields.bits.profile] << 2);
    OUT_BCS_BATCH(ctx,
                  pic_param->luma_shift << 16 |
                  pic_param->luma_scale << 0); /* FIXME: Luma Scaling */
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx,
                  interpolation_mode << 19 |
                  pic_param->fast_uvmc_flag << 18 |
                  0 << 17 | /* FIXME: scale up or down ??? */
                  pic_param->range_reduction_frame << 16 |
                  0 << 6 | /* FIXME: double ??? */
                  0 << 4 |
                  intensitycomp_single << 2 |
                  intensitycomp_single << 0);
    ADVANCE_BCS_BATCH(ctx);
}


static void
gen6_mfd_vc1_directmode_state(VADriverContextP ctx, struct decode_state *decode_state)
{
    VAPictureParameterBufferVC1 *pic_param;
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_surface *obj_surface;
    dri_bo *dmv_read_buffer = NULL, *dmv_write_buffer = NULL;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferVC1 *)decode_state->pic_param->buffer;

    obj_surface = SURFACE(decode_state->current_render_target);

    if (obj_surface && obj_surface->private_data) {
        dmv_write_buffer = ((struct gen6_vc1_surface *)(obj_surface->private_data))->dmv;
    }

    obj_surface = SURFACE(pic_param->backward_reference_picture);

    if (obj_surface && obj_surface->private_data) {
        dmv_read_buffer = ((struct gen6_vc1_surface *)(obj_surface->private_data))->dmv;
    }

    BEGIN_BCS_BATCH(ctx, 3);
    OUT_BCS_BATCH(ctx, MFX_VC1_DIRECTMODE_STATE | (3 - 2));

    if (dmv_write_buffer)
        OUT_BCS_RELOC(ctx, dmv_write_buffer,
                      I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                      0);
    else
        OUT_BCS_BATCH(ctx, 0);

    if (dmv_read_buffer)
        OUT_BCS_RELOC(ctx, dmv_read_buffer,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      0);
    else
        OUT_BCS_BATCH(ctx, 0);
                  
    ADVANCE_BCS_BATCH(ctx);
}

static int
gen6_mfd_vc1_get_macroblock_bit_offset(uint8_t *buf, int in_slice_data_bit_offset, int profile)
{
    int out_slice_data_bit_offset;
    int slice_header_size = in_slice_data_bit_offset / 8;
    int i, j;

    if (profile != 3)
        out_slice_data_bit_offset = in_slice_data_bit_offset;
    else {
        for (i = 0, j = 0; i < slice_header_size; i++, j++) {
            if (!buf[j] && !buf[j + 1] && buf[j + 2] == 3 && buf[j + 3] < 4) {
                i++, j += 2;
            }
        }

        out_slice_data_bit_offset = 8 * j + in_slice_data_bit_offset % 8;
    }

    return out_slice_data_bit_offset;
}

static void
gen6_mfd_vc1_bsd_object(VADriverContextP ctx,
                        VAPictureParameterBufferVC1 *pic_param,
                        VASliceParameterBufferVC1 *slice_param,
                        VASliceParameterBufferVC1 *next_slice_param,
                        dri_bo *slice_data_bo)
{
    int next_slice_start_vert_pos;
    int macroblock_offset;
    uint8_t *slice_data = NULL;

    dri_bo_map(slice_data_bo, 0);
    slice_data = (uint8_t *)(slice_data_bo->virtual + slice_param->slice_data_offset);
    macroblock_offset = gen6_mfd_vc1_get_macroblock_bit_offset(slice_data, 
                                                               slice_param->macroblock_offset,
                                                               pic_param->sequence_fields.bits.profile);
    dri_bo_unmap(slice_data_bo);

    if (next_slice_param)
        next_slice_start_vert_pos = next_slice_param->slice_vertical_position;
    else
        next_slice_start_vert_pos = ALIGN(pic_param->coded_height, 16) / 16;

    BEGIN_BCS_BATCH(ctx, 4);
    OUT_BCS_BATCH(ctx, MFD_VC1_BSD_OBJECT | (4 - 2));
    OUT_BCS_BATCH(ctx, 
                  slice_param->slice_data_size - (macroblock_offset >> 3));
    OUT_BCS_BATCH(ctx, 
                  slice_param->slice_data_offset + (macroblock_offset >> 3));
    OUT_BCS_BATCH(ctx,
                  slice_param->slice_vertical_position << 24 |
                  next_slice_start_vert_pos << 16 |
                  (macroblock_offset & 0x7));
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfd_vc1_decode_picture(VADriverContextP ctx, struct decode_state *decode_state)
{
    VAPictureParameterBufferVC1 *pic_param;
    VASliceParameterBufferVC1 *slice_param, *next_slice_param;
    dri_bo *slice_data_bo;
    int i, j;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferVC1 *)decode_state->pic_param->buffer;

    gen6_mfd_vc1_decode_init(ctx, decode_state);
    intel_batchbuffer_start_atomic_bcs(ctx, 0x1000);
    intel_batchbuffer_emit_mi_flush_bcs(ctx);
    gen6_mfd_pipe_mode_select(ctx, decode_state, MFX_FORMAT_VC1);
    gen6_mfd_surface_state(ctx, decode_state, MFX_FORMAT_VC1);
    gen6_mfd_pipe_buf_addr_state(ctx, decode_state, MFX_FORMAT_VC1);
    gen6_mfd_bsp_buf_base_addr_state(ctx, decode_state, MFX_FORMAT_VC1);
    gen6_mfd_vc1_pic_state(ctx, decode_state);
    gen6_mfd_vc1_pred_pipe_state(ctx, decode_state);
    gen6_mfd_vc1_directmode_state(ctx, decode_state);

    assert(decode_state->num_slice_params == 1);
    for (j = 0; j < decode_state->num_slice_params; j++) {
        assert(decode_state->slice_params && decode_state->slice_params[j]->buffer);
        slice_param = (VASliceParameterBufferVC1 *)decode_state->slice_params[j]->buffer;
        slice_data_bo = decode_state->slice_datas[j]->bo;
        gen6_mfd_ind_obj_base_addr_state(ctx, slice_data_bo, MFX_FORMAT_VC1);

        for (i = 0; i < decode_state->slice_params[j]->num_elements; i++) {
            assert(slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_ALL);

            if (i < decode_state->slice_params[j]->num_elements - 1)
                next_slice_param = slice_param + 1;
            else
                next_slice_param = NULL;

            gen6_mfd_vc1_bsd_object(ctx, pic_param, slice_param, next_slice_param, slice_data_bo);
            slice_param++;
        }
    }

    intel_batchbuffer_end_atomic_bcs(ctx);
    intel_batchbuffer_flush_bcs(ctx);
}

void 
gen6_mfd_decode_picture(VADriverContextP ctx, 
                        VAProfile profile, 
                        struct decode_state *decode_state)
{
    switch (profile) {
    case VAProfileMPEG2Simple:
    case VAProfileMPEG2Main:
        gen6_mfd_mpeg2_decode_picture(ctx, decode_state);
        break;
        
    case VAProfileH264Baseline:
    case VAProfileH264Main:
    case VAProfileH264High:
        gen6_mfd_avc_decode_picture(ctx, decode_state);
        break;

    case VAProfileVC1Simple:
    case VAProfileVC1Main:
    case VAProfileVC1Advanced:
        gen6_mfd_vc1_decode_picture(ctx, decode_state);
        break;

    default:
        assert(0);
        break;
    }
}

Bool
gen6_mfd_init(VADriverContextP ctx)
{
    return True;
}

Bool 
gen6_mfd_terminate(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_media_state *media_state = &i965->media_state;
    struct gen6_mfd_context *gen6_mfd_context = media_state->private_context;

    if (gen6_mfd_context) {
        dri_bo_unreference(gen6_mfd_context->post_deblocking_output.bo);
        gen6_mfd_context->post_deblocking_output.bo = NULL;

        dri_bo_unreference(gen6_mfd_context->pre_deblocking_output.bo);
        gen6_mfd_context->pre_deblocking_output.bo = NULL;

        dri_bo_unreference(gen6_mfd_context->intra_row_store_scratch_buffer.bo);
        gen6_mfd_context->intra_row_store_scratch_buffer.bo = NULL;

        dri_bo_unreference(gen6_mfd_context->deblocking_filter_row_store_scratch_buffer.bo);
        gen6_mfd_context->deblocking_filter_row_store_scratch_buffer.bo = NULL;

        dri_bo_unreference(gen6_mfd_context->bsd_mpc_row_store_scratch_buffer.bo);
        gen6_mfd_context->bsd_mpc_row_store_scratch_buffer.bo = NULL;

        dri_bo_unreference(gen6_mfd_context->mpr_row_store_scratch_buffer.bo);
        gen6_mfd_context->mpr_row_store_scratch_buffer.bo = NULL;

        dri_bo_unreference(gen6_mfd_context->bitplane_read_buffer.bo);
        gen6_mfd_context->bitplane_read_buffer.bo = NULL;

        free(gen6_mfd_context);
    }

    media_state->private_context = NULL;
    return True;
}

