/*
 * Copyright (c) 2007-2017 Intel Corporation. All Rights Reserved.
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
 * \file va_fei_h264.h
 * \brief The FEI encoding H264 special API
 */

#ifndef VA_FEI_H264_H
#define VA_FEI_H264_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "va_fei.h"

/** \brief FEI frame level control buffer for H.264 */
typedef struct _VAEncMiscParameterFEIFrameControlH264
{
    uint32_t      function; /* one of the VAConfigAttribFEIFunctionType values */
    /** \brief MB (16x16) control input buffer. It is valid only when (mb_input | mb_size_ctrl)
     * is set to 1. The data in this buffer correspond to the input source. 16x16 MB is in raster scan order,
     * each MB control data structure is defined by VAEncFEIMBControlH264.
     * Buffer size shall not be less than the number of 16x16 blocks multiplied by
     * sizeof(VAEncFEIMBControlH264).
     * Note: if mb_qp is set, VAEncQPBufferH264 is expected.
     */
    VABufferID    mb_ctrl;
    /** \brief distortion output of MB ENC or ENC_PAK.
     * Each 16x16 block has one distortion data with VAEncFEIDistortionH264 layout
     * Buffer size shall not be less than the number of 16x16 blocks multiplied by
     * sizeof(VAEncFEIDistortionH264).
     */
    VABufferID    distortion;
    /** \brief MVs data output of MB ENC.
     * Each 16x16 block has one MVs data with layout VAMotionVector
     * Buffer size shall not be less than the number of 16x16 blocks multiplied by
     * sizeof(VAMotionVector) * 16.
     */
    VABufferID    mv_data;
    /** \brief MBCode data output of MB ENC.
     * Each 16x16 block has one MB Code data with layout VAEncFEIMBCodeH264
     * Buffer size shall not be less than the number of 16x16 blocks multiplied by
     * sizeof(VAEncFEIMBCodeH264).
     */
    VABufferID    mb_code_data;
    /** \brief QP input buffer with layout VAEncQPBufferH264. It is valid only when mb_qp is set to 1.
     * The data in this buffer correspond to the input source.
     * One QP per 16x16 block in raster scan order, each QP is a signed char (8-bit) value.
     **/
    VABufferID    qp;
    /** \brief MV predictor. It is valid only when mv_predictor_enable is set to 1.
     * Each 16x16 block has one or more pair of motion vectors and the corresponding
     * reference indexes as defined by VAEncFEIMVPredictorH264. 16x16 block is in raster scan order.
     * Buffer size shall not be less than the number of 16x16 blocks multiplied by
     * sizeof(VAEncFEIMVPredictorH264). */
    VABufferID    mv_predictor;

    /** \brief number of MV predictors. It must not be greater than maximum supported MV predictor. */
    uint32_t      num_mv_predictors_l0      : 16;
    uint32_t      num_mv_predictors_l1      : 16;

    /** \brief motion search method definition
     * 0: default value, diamond search
     * 1: full search
     * 2: diamond search
     **/
    uint32_t      search_path               : 8;
    /** \brief maximum number of Search Units, valid range is [1, 63] */
    uint32_t      len_sp                    : 8;
    uint32_t      reserved0	                : 16;

    uint32_t      sub_mb_part_mask          : 7;
    uint32_t      intra_part_mask           : 5;
    uint32_t      multi_pred_l0             : 1;
    uint32_t      multi_pred_l1             : 1;
    uint32_t      sub_pel_mode              : 2;
    uint32_t      inter_sad 	            : 2;
    uint32_t      intra_sad                 : 2;
    uint32_t      distortion_type           : 1;
    uint32_t      repartition_check_enable  : 1;
    uint32_t      adaptive_search           : 1;
    uint32_t      mv_predictor_enable       : 1;
    uint32_t      mb_qp                     : 1;
    uint32_t      mb_input                  : 1;
    uint32_t      mb_size_ctrl              : 1;
    uint32_t      colocated_mb_distortion   : 1;
    uint32_t      reserved1	                : 4;

    /** \brief motion search window(ref_width * ref_height) */
    uint32_t      ref_width                 : 8;
    uint32_t      ref_height                : 8;
    /** \brief predefined motion search windows. If selected, len_sp, window(ref_width * ref_eight)
     * and search_path setting are ignored.
     * 0: not use predefined search window
     * 1: Tiny, len_sp=4, 24x24 window and diamond search
     * 2: Small, len_sp=9, 28x28 window and diamond search
     * 3: Diamond, len_sp=16, 48x40 window and diamond search
     * 4: Large Diamond, len_sp=32, 48x40 window and diamond search
     * 5: Exhaustive, len_sp=48, 48x40 window and full search
     * 6: Extend Diamond, len_sp=16, 64x40 window and diamond search
     * 7: Extend Large Diamond, len_sp=32, 64x40 window and diamond search
     * 8: Extend Exhaustive, len_sp=48, 64x40 window and full search
     **/
    uint32_t      search_window             : 4;
    uint32_t      reserved2                 : 12;

    /** \brief max frame size control with multi passes QP setting */
    uint32_t      max_frame_size;
    /** \brief number of passes, every pass has different QP */
    uint32_t      num_passes;
    /** \brief delta QP list for every pass */
    uint8_t       *delta_qp;
    uint32_t      reserved3[VA_PADDING_LOW];
} VAEncMiscParameterFEIFrameControlH264;

/** \brief FEI MB level control data structure */
typedef struct _VAEncFEIMBControlH264
{
    /** \brief when set, correposndent MB is coded as intra */
    uint32_t force_to_intra                : 1;
    /** \brief when set, correposndent MB is coded as skip */
    uint32_t force_to_skip                 : 1;
    uint32_t force_to_nonskip              : 1;
    uint32_t enable_direct_bias_adjustment : 1;
    uint32_t enable_motion_bias_adjustment : 1;
    uint32_t ext_mv_cost_scaling_factor    : 3;
    uint32_t reserved0                     : 24;

    uint32_t reserved1;

    uint32_t reserved2;

    /** \brief when mb_size_ctrl is set, size here is used to budget accumulatively. Set to 0xFF if don't care. */
    uint32_t reserved3                     : 16;
    uint32_t target_size_in_word           : 8;
    uint32_t max_size_in_word              : 8;
} VAEncFEIMBControlH264;


/** \brief Application can use this definition as reference to allocate the buffer
 * based on MaxNumPredictor returned from attribute VAConfigAttribFEIMVPredictors query.
 **/
typedef struct _VAEncFEIMVPredictorH264
{
    /** \brief Reference index corresponding to the entry of RefPicList0 & RefPicList1 in VAEncSliceParameterBufferH264.
     * Note that RefPicList0 & RefPicList1 needs to be the same for all slices.
     * ref_idx_l0_x : index to RefPicList0; ref_idx_l1_x : index to RefPicList1; x : 0 - MaxNumPredictor.
     **/
    struct {
        uint8_t   ref_idx_l0    : 4;
        uint8_t   ref_idx_l1    : 4;
    } ref_idx[4]; /* index is predictor number */
    uint32_t reserved;
    /** \brief MV. MaxNumPredictor must be the returned value from attribute VAConfigAttribFEIMVPredictors query.
     * Even application doesn't use the maximum predictors, the VAFEIMVPredictorH264 structure size
     * has to be defined as maximum so each MB can be at a fixed location.
     * Note that 0x8000 must be used for correspondent intra block.
     **/
    VAMotionVector mv[4]; /* MaxNumPredictor is 4 */
} VAEncFEIMVPredictorH264;

/** \brief FEI output */
/**
 * Motion vector output is per 4x4 block. For each 4x4 block there is a pair of MVs
 * for RefPicList0 and RefPicList1 and each MV is 4 bytes including horizontal and vertical directions.
 * Depending on Subblock partition, for the shape that is not 4x4, the MV is replicated
 * so each 4x4 block has a pair of MVs. The 16x16 block has 32 MVs (128 bytes).
 * 0x8000 is used for correspondent intra block. The 16x16 block is in raster scan order,
 * within the 16x16 block, each 4x4 block MV is ordered as below in memory.
 * The buffer size shall be greater than or equal to the number of 16x16 blocks multiplied by 128 bytes.
 * Note that, when separate ENC and PAK is enabled, the exact layout of this buffer is needed for PAK input.
 * App can reuse this buffer, or copy to a different buffer as PAK input.
 * Layout is defined as Generic motion vector data structure VAMotionVector
 *                      16x16 Block
 *        -----------------------------------------
 *        |    1    |    2    |    5    |    6    |
 *        -----------------------------------------
 *        |    3    |    4    |    7    |    8    |
 *        -----------------------------------------
 *        |    9    |    10   |    13   |    14   |
 *        -----------------------------------------
 *        |    11   |    12   |    15   |    16   |
 *        -----------------------------------------
 **/

/** \brief VAEncFEIMBCodeH264 defines the data structure for VAEncFEIMBCodeBufferType per 16x16 MB block.
 * it is output buffer of ENC and ENC_PAK modes, it's also input buffer of PAK mode.
 * The 16x16 block is in raster scan order. Buffer size shall not be less than the number of 16x16 blocks
 * multiplied by sizeof(VAEncFEIMBCodeH264). Note that, when separate ENC and PAK is enabled,
 * the exact layout of this buffer is needed for PAK input. App can reuse this buffer,
 * or copy to a different buffer as PAK input, reserved elements must not be modified when used as PAK input.
 **/
typedef struct _VAEncFEIMBCodeH264
{
    //DWORD  0~2
    uint32_t    reserved0[3];

    //DWORD  3
    uint32_t    inter_mb_mode            : 2;
    uint32_t    mb_skip_flag             : 1;
    uint32_t    reserved1                : 1;
    uint32_t    intra_mb_mode            : 2;
    uint32_t    reserved2                : 1;
    uint32_t    field_mb_polarity_flag   : 1;
    uint32_t    mb_type                  : 5;
    uint32_t    intra_mb_flag	         : 1;
    uint32_t    field_mb_flag            : 1;
    uint32_t    transform8x8_flag        : 1;
    uint32_t    reserved3                : 1;
    uint32_t    dc_block_coded_cr_flag   : 1;
    uint32_t    dc_block_coded_cb_flag   : 1;
    uint32_t    dc_block_coded_y_flag    : 1;
    uint32_t    reserved4                : 12;

    //DWORD 4
    uint32_t    horz_origin              : 8;
    uint32_t    vert_origin              : 8;
    uint32_t    cbp_y                    : 16;

    //DWORD 5
    uint32_t    cbp_cb                   : 16;
    uint32_t    cbp_cr                   : 16;

    //DWORD 6
    uint32_t    qp_prime_y               : 8;
    uint32_t    reserved5                : 17;
    uint32_t    mb_skip_conv_disable     : 1;
    uint32_t    is_last_mb               : 1;
    uint32_t    enable_coefficient_clamp : 1;
    uint32_t    direct8x8_pattern        : 4;

    //DWORD 7 8 and 9
    union
    {
        /* Intra MBs */
        struct
        {
            uint32_t   luma_intra_pred_modes0 : 16;
            uint32_t   luma_intra_pred_modes1 : 16;

            uint32_t   luma_intra_pred_modes2 : 16;
            uint32_t   luma_intra_pred_modes3 : 16;

            uint32_t   chroma_intra_pred_mode : 2;
            uint32_t   intra_pred_avail_flag  : 5;
            uint32_t   intra_pred_avail_flagF : 1;
            uint32_t   reserved6              : 24;
        } intra_mb;

        /* Inter MBs */
        struct
        {
            uint32_t   sub_mb_shapes          : 8;
            uint32_t   sub_mb_pred_modes      : 8;
            uint32_t   reserved7              : 16;

            uint32_t   ref_idx_l0_0           : 8;
            uint32_t   ref_idx_l0_1           : 8;
            uint32_t   ref_idx_l0_2           : 8;
            uint32_t   ref_idx_l0_3           : 8;

            uint32_t   ref_idx_l1_0           : 8;
            uint32_t   ref_idx_l1_1           : 8;
            uint32_t   ref_idx_l1_2           : 8;
            uint32_t   ref_idx_l1_3           : 8;
        } inter_mb;
    } mb_mode;

    //DWORD 10
    uint32_t   reserved8                 : 16;
    uint32_t   target_size_in_word       : 8;
    uint32_t   max_size_in_word          : 8;

    //DWORD 11~14
    uint32_t   reserved9[4];

    //DWORD 15
    uint32_t   reserved10;
} VAEncFEIMBCodeH264;        // 64 bytes

/** \brief VAEncFEIDistortionH264 defines the data structure for VAEncFEIDistortionBufferType per 16x16 MB block.
 * It is output buffer of ENC and ENC_PAK modes, The 16x16 block is in raster scan order.
 * Buffer size shall not be less than the number of 16x16 blocks multiple by sizeof(VAEncFEIDistortionH264).
 **/
typedef struct _VAEncFEIDistortionH264 {
    /** \brief Inter-prediction-distortion associated with motion vector i (co-located with subblock_4x4_i).
     * Its meaning is determined by sub-shape. It must be zero if the corresponding sub-shape is not chosen.
     **/
    uint16_t    inter_distortion[16];
    uint32_t    best_inter_distortion     : 16;
    uint32_t    best_intra_distortion     : 16;
    uint32_t    colocated_mb_distortion   : 16;
    uint32_t    reserved0                 : 16;
    uint32_t    reserved1[2];
} VAEncFEIDistortionH264;    // 48 bytes

#ifdef __cplusplus
}
#endif

#endif /* VA_FEI_H264_H */
