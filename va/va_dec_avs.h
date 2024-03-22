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
 * \file va_dec_avs.h
 * \brief The AVS decoding API
 *
 * This file contains the \ref api_dec_avs "AVS decoding API".
 */

#ifndef VA_DEC_AVS_H
#define VA_DEC_AVS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup api_dec_avs AVS decoding API
 *
 * This AVS decoding API supports Jizhun & Guangdian profiles.
 *
 * @{
 */

typedef enum {
    VA_AVS_I_IMG  = 0,
    VA_AVS_P_IMG  = 1,
    VA_AVS_B_IMG  = 2,
} VAAVSPicType;

typedef struct _VAPictureAVS {
    VASurfaceID     surface_id;
    uint32_t        flags;
    uint32_t        poc;

    /** \brief Reserved bytes for future use, must be zero */
    uint32_t        va_reserved[VA_PADDING_LOW];
} VAPictureAVS;

/* flags in VAPictureAVS could be OR of the following */
#define VA_PICTURE_AVS_INVALID          0x00000001
#define VA_PICTURE_AVS_TOP_FIELD        0x00000002
#define VA_PICTURE_AVS_BOTTOM_FIELD     0x00000004

/**
 * \brief AVS Decoding Picture Parameter Buffer Structure
 *
 * This structure conveys picture level parameters and should be sent once
 * per frame.
 */
typedef struct _VAPictureParameterBufferAVS {
    VAPictureAVS    curr_pic;
    VAPictureAVS    ref_list[2];

    uint16_t width;                                             // coded width
    uint16_t height;                                            // coded height

    uint16_t picture_type                           :  2;       // VAAVSPicType
    uint16_t progressive_seq_flag                   :  1;
    uint16_t progressive_frame_flag                 :  1;
    uint16_t picture_structure_flag                 :  1;       // 0:field coding, 1:frame coding
    uint16_t fixed_pic_qp_flag                      :  1;
    uint16_t picture_qp                             :  6;
    uint16_t loop_filter_disable_flag               :  1;
    uint16_t skip_mode_flag_flag                    :  1;
    uint16_t picture_reference_flag                 :  1;
    uint16_t no_forward_reference_flag              :  1;

    int8_t   alpha_c_offset;
    int8_t   beta_offset;

    struct {
        uint16_t guangdian_flag                     :  1;
        uint16_t aec_flag                           :  1;
        uint16_t weight_quant_flag                  :  1;
        uint16_t pb_field_enhanced_flag             :  1;
        uint16_t reserved                           : 12;
        int8_t   chroma_quant_param_delta_cb;
        int8_t   chroma_quant_param_delta_cr;
        uint8_t  wqm_8x8[64];
    } guangdian_fields;

    /** \brief Reserved bytes for future use, must be zero */
    uint32_t va_reserved[VA_PADDING_MEDIUM];
} VAPictureParameterBufferAVS;

/**
 * \brief AVS Slice Parameter Buffer Structure
 *
 * Slice data buffer of VASliceDataBufferType is used to send the bitstream data.
 * When send AVS slice data, start code and slice header should be included.
 */
typedef struct _VASliceParameterBufferAVS {
    uint32_t slice_data_size;       /* number of bytes in the slice data buffer for this slice */
    uint32_t slice_data_offset;     /* the offset to the first byte of slice data */
    uint32_t slice_data_flag;       /* see VA_SLICE_DATA_FLAG_XXX defintions */
    uint32_t mb_data_bit_offset;    /* offset to the first bit of MB from the first byte of slice data (slice_data_offset) */

    uint32_t slice_vertical_pos         : 16;   /* first mb row of the slice */
    uint32_t fixed_slice_qp_flag        :  1;
    uint32_t slice_qp                   :  6;
    uint32_t slice_weight_pred_flag     :  1;
    uint32_t mb_weight_pred_flag        :  1;

    uint8_t luma_scale[4];
    int8_t  luma_shift[4];
    uint8_t chroma_scale[4];
    int8_t  chroma_shift[4];

    /** \brief Reserved bytes for future use, must be zero */
    uint32_t va_reserved[VA_PADDING_LOW];
} VASliceParameterBufferAVS;

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* VA_DEC_AVS_H */
