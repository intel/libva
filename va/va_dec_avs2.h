/*
 * Copyright (c) 2022 MTHREADS Corporation. All Rights Reserved.
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
 * IN NO EVENT SHALL INTEL AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file va_dec_avs2.h
 * \brief The AVS2 decoding API
 *
 * This file contains the \ref api_dec_avs2 "AVS2 decoding API".
 */

#ifndef VA_DEC_AVS2_H
#define VA_DEC_AVS2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup api_dec_avs2 AVS2 decoding API
 *
 * This AVS2 decoding API supports Main and Main10 profiles.
 * And it supports both short slice format and long slice format.
 *
 * @{
 */

#define VA_AVS2_MAX_REF_COUNT 7

typedef enum {
    VA_AVS2_I_IMG  = 0,
    VA_AVS2_P_IMG  = 1,
    VA_AVS2_B_IMG  = 2,
    VA_AVS2_F_IMG  = 3,
    VA_AVS2_S_IMG  = 4,
    VA_AVS2_G_IMG  = 5,
    VA_AVS2_GB_IMG = 6,
} VAAVS2PicType;

typedef struct _VAPictureAVS2 {
    /**
     * \brief reconstructed picture buffer surface index
     * invalid when taking value VA_INVALID_SURFACE.
     */
    VASurfaceID     surface_id;

    int16_t         doi;
    int16_t         poi;

    int             num_ref;
    int16_t         ref_doi[VA_AVS2_MAX_REF_COUNT];
    int16_t         ref_poi[VA_AVS2_MAX_REF_COUNT];

    /** \brief Reserved bytes for future use, must be zero */
    uint32_t        va_reserved[VA_PADDING_LOW];
} VAPictureAVS2;

/**
 * \brief AVS2 Decoding Picture Parameter Buffer Structure
 *
 * This structure conveys picture level parameters and should be sent once
 * per frame.
 */
typedef struct _VAPictureParameterBufferAVS2 {
    uint32_t non_ref_flag                         :  1; // [    0]
    uint32_t num_of_ref                           :  3; // [ 3: 1]
    uint32_t reserved_0                           : 28; // [31: 4]

    VAPictureAVS2   CurrPic;
    VAPictureAVS2   ref_list[VA_AVS2_MAX_REF_COUNT];

    uint32_t width                                : 16; // [15: 0]
    uint32_t height                               : 16; // [31:16]

    uint32_t log2_lcu_size_minus4                 :  2; // [ 1: 0]
    uint32_t chroma_format                        :  2; // [ 3: 2]
    uint32_t output_bit_depth_minus8              :  2; // [ 5: 4]
    uint32_t weighted_skip_enable                 :  1; // [    6]
    uint32_t multi_hypothesis_skip_enable         :  1; // [    7]
    uint32_t nonsquare_intra_prediction_enable    :  1; // [    8]
    uint32_t dph_enable                           :  1; // [    9]
    uint32_t encoding_bit_depth_minus8            :  2; // [11:10]
    uint32_t field_coded_sequence                 :  1; // [   12]
    uint32_t pmvr_enable                          :  1; // [   13]
    uint32_t nonsquare_quadtree_transform_enable  :  1; // [   14]
    uint32_t inter_amp_enable                     :  1; // [   15]
    uint32_t secondary_transform_enable_flag      :  1; // [   16]
    uint32_t fixed_pic_qp                         :  1; // [   17]
    uint32_t pic_qp                               :  7; // [24:18]
    uint32_t picture_structure                    :  1; // [   25]
    uint32_t top_field_picture_flag               :  1; // [   26]
    uint32_t scene_picture_disable                :  1; // [   27]
    uint32_t scene_reference_enable               :  1; // [   28]
    uint32_t pic_type                             :  3; // [31:29] VAAVS2PicType

    uint32_t lf_cross_slice_enable_flag           :  1; // [    0]
    uint32_t lf_pic_dbk_disable_flag              :  1; // [    1]
    uint32_t sao_enable                           :  1; // [    2]
    uint32_t alf_enable                           :  1; // [    3]
    uint32_t pic_alf_on_Y                         :  1; // [    4]
    uint32_t pic_alf_on_U                         :  1; // [    5]
    uint32_t pic_alf_on_V                         :  1; // [    6]
    uint32_t pic_weight_quant_enable              :  1; // [    7]
    uint32_t pic_weight_quant_data_index          :  2; // [ 9: 8]
    uint32_t reserved_1                           : 22; // [31:10]

    int8_t  alpha_c_offset;                             //  -8 ~ +8
    int8_t  beta_offset;                                //  -8 ~ +8
    int8_t  chroma_quant_param_delta_cb;                // -16 ~ +16
    int8_t  chroma_quant_param_delta_cr;                // -16 ~ +16

    uint8_t wq_mat[16 + 64];

    // alf_coeff[ 0-15: luma, 16:cb, 17:cr ][ ]
    int8_t  alf_coeff[16 + 2][9];

    /** \brief Reserved bytes for future use, must be zero */
    uint32_t va_reserved[VA_PADDING_LOW];
} VAPictureParameterBufferAVS2;

/**
 * \brief AVS2 Slice Parameter Buffer Structure
 *
 * Slice data buffer of VASliceDataBufferType is used to send the bitstream data.
 * When send AVS2 slice data, start code and slice header should be included.
 */
typedef struct _VASliceParameterBufferAVS2 {
    uint32_t slice_data_size;       /* number of bytes in the slice data buffer for this slice */
    uint32_t slice_data_offset;     /* the offset to the first byte of slice data */
    uint32_t slice_data_flag;       /* see VA_SLICE_DATA_FLAG_XXX defintions */

    uint16_t lcu_start_x;
    uint16_t lcu_start_y;

    uint32_t fixed_slice_qp         :  1;
    uint32_t slice_qp               :  7;
    uint32_t slice_sao_enable_Y     :  1;
    uint32_t slice_sao_enable_U     :  1;
    uint32_t slice_sao_enable_V     :  1;
    uint32_t vlc_byte_offset        :  4;
    uint32_t reserved               : 17;

    /** \brief Reserved bytes for future use, must be zero */
    uint32_t va_reserved[VA_PADDING_LOW];
} VASliceParameterBufferAVS2;

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* VA_DEC_AVS2_H */
