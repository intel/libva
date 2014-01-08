/*
 * Copyright (c) 2007-2013 Intel Corporation. All Rights Reserved.
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
 * \file va_intel_fei.h
 * \brief The Intel FEI (Flexible Encoding Infrastructure) encoding API
 *
 * This file contains the \ref api_intel_fei "Intel FEI (Flexible Encoding Infrastructure) encoding API".
 */

#ifndef VA_INTEL_FEI_H
#define VA_INTEL_FEI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <va/va_enc.h>

/**
 * \defgroup api_intel_fei Intel FEI (Flexible Encoding Infrastructure) encoding API
 *
 * @{
 */

/** \brief FEI frame level control buffer for H.264 */
typedef struct _VAEncMiscParameterFEIFrameControlH264Intel {
    unsigned int function; /* one of the VAConfigAttribEncFunctionType values */   
    /** \brief MB (16x16) control input surface. It is valid only when (mb_input | mb_size_ctrl) 
     * is set to 1. The data in this buffer correspond to the input source. 16x16 MB is in raster scan order, 
     * each MB control data structure is defined by VAEncFEIMBControlBufferH264. 
     * Buffer size shall not be less than the number of 16x16 blocks multiplied by 
     * sizeof(VAEncFEIMBControlBufferH264Intel). 
     * Note: if mb_qp is set, VAEncQpBufferH264 is expected.
     */
    VASurfaceID       mb_ctrl;
    /** \brief MV predictor. It is valid only when mv_predictor_enable is set to 1. 
     * Each 16x16 block has one or more pair of motion vectors and the corresponding 
     * reference indexes as defined by VAEncMVPredictorBufferH264. 16x16 block is in raster scan order. 
     * Buffer size shall not be less than the number of 16x16 blocks multiplied by 
     * sizeof(VAEncMVPredictorBufferH264). */
    VASurfaceID       mv_predictor;

    /** \brief number of MV predictors. It must not be greater than maximum supported MV predictor. */
    unsigned int      num_mv_predictors;

    /** \brief control parameters */
    unsigned int      max_len_sp                : 8;     
    unsigned int      len_sp                    : 8;     
    unsigned int      reserved0	                : 16;    

    unsigned int      sub_mb_part_mask          : 7;     
    unsigned int      intra_part_mask           : 5;     
    unsigned int      multi_pred_l0             : 1;     
    unsigned int      multi_pred_l1             : 1;     
    unsigned int      sub_pel_mode              : 2;     
    unsigned int      inter_sad 	            : 2;
    unsigned int      intra_sad                 : 2;     
    unsigned int      distortion_type           : 1;     
    unsigned int      repartition_check_enable  : 1;     
    unsigned int      adaptive_search           : 1;     
    unsigned int      mv_predictor_enable       : 1;     
    unsigned int      mb_qp                     : 1;     
    unsigned int      mb_input                  : 1;     
    unsigned int      mb_size_ctrl              : 1;
    unsigned int      reserved1	                : 5;    

    unsigned int      ref_width                 : 8;     
    unsigned int      ref_height                : 8;
    unsigned int      search_window             : 3;     
    unsigned int      reserved2                 : 13;
} VAEncMiscParameterFEIFrameControlH264Intel;


/** \brief FEI MB level control data structure */
typedef struct _VAEncFEIMBControlH264Intel {
    /** \brief when set, correposndent MB is coded as skip */
    unsigned int force_to_skip       : 1;     
    /** \brief when set, correposndent MB is coded as intra */
    unsigned int force_to_intra      : 1;     
    unsigned int reserved1           : 30;    

    /** \brief when mb_size_ctrl is set, size here is used to budget accumulatively. Set to 0xFF if don't care. */
    unsigned int max_size_in_word    : 8;     
    unsigned int target_size_in_word : 8;     
    unsigned int reserved2           : 16;    

    unsigned int reserved3;    
} VAEncFEIMBControlH264Intel;


/** \brief Application can use this definition as reference to allocate the buffer 
 * based on MaxNumPredictor returned from attribute VAConfigAttribEncMVPredictorsIntel query. 
 **/
typedef struct _VAEncMVPredictorH264Intel {
    /** \brief Reference index corresponding to the entry of RefPicList0 & RefPicList1 in VAEncSliceParameterBufferH264. 
     * Note that RefPicList0 & RefPicList1 needs to be the same for all slices. 
     * ref_idx_l0_x : index to RefPicList0; ref_idx_l1_x : index to RefPicList1; x : 0 - MaxNumPredictor. 
     **/
    unsigned int ref_idx_l0_0 : 4;     
    unsigned int ref_idx_l1_0 : 4;     
    unsigned int ref_idx_l0_1 : 4;     
    unsigned int ref_idx_l1_1 : 4;     
    unsigned int ref_idx_l0_2 : 4;     
    unsigned int ref_idx_l1_2 : 4;     
    unsigned int ref_idx_l0_3 : 4;     
    unsigned int ref_idx_l1_3 : 4;     
    unsigned int reserved;
    /** \brief MV. MaxNumPredictor must be the returned value from attribute VAConfigAttribEncMVPredictors query. 
     * Even application doesn't use the maximum predictors, the VAEncMVPredictorH264 structure size 
     * has to be defined as maximum so each MB can be at a fixed location. 
     * Note that 0x8000 must be used for correspondent intra block. 
     **/
    struct _mv
    {
    /** \brief Motion vector corresponding to ref0x_index. 
     * mv0[0] is horizontal motion vector and mv0[1] is vertical motion vector. */
        short    mv0[2];
    /** \brief Motion vector corresponding to ref1x_index. 
     * mv1[0] is horizontal motion vector and mv1[1] is vertical motion vector. */
        short    mv1[2];
    } mv[4]; /* MaxNumPredictor is 4 */
} VAEncMVPredictorH264Intel;


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

/** \brief VAEncFEIModeBufferIntel defines the data structure for VAEncFEIModeBufferTypeIntel per 16x16 MB block. 
 * The 16x16 block is in raster scan order. Buffer size shall not be less than the number of 16x16 blocks 
 * multiplied by sizeof(VAEncFEIModeBufferH264Intel). Note that, when separate ENC and PAK is enabled, 
 * the exact layout of this buffer is needed for PAK input. App can reuse this buffer, 
 * or copy to a different buffer as PAK input, reserved elements must not be modified when used as PAK input. 
 **/
typedef struct _VAEncFEIModeBufferH264Intel {
    unsigned int    reserved0;
    unsigned int    reserved1[3];

    unsigned int    inter_mb_mode            : 2;
    unsigned int    mb_skip_flag             : 1;
    unsigned int    reserved00               : 1; 
    unsigned int    intra_mb_mode            : 2;
    unsigned int    reserved01               : 1; 
    unsigned int    field_mb_polarity_flag   : 1;
    unsigned int    mb_type                  : 5;
    unsigned int    intra_mb_flag	         : 1;
    unsigned int    field_mb_flag            : 1;
    unsigned int    transform8x8_flag        : 1;
    unsigned int    reserved02               : 1; 
    unsigned int    dc_block_coded_cr_flag   : 1;
    unsigned int    dc_block_coded_cb_flag   : 1;
    unsigned int    dc_block_coded_y_flag    : 1;
    unsigned int    reserved03               : 12; 

    unsigned int    horz_origin              : 8;
    unsigned int    vert_origin              : 8;
    unsigned int    cbp_y                    : 16;

    unsigned int    cbp_cb                   : 16;
    unsigned int    cbp_cr                   : 16;

    unsigned int    qp_prime_y               : 8;
    unsigned int    reserved30               : 17; 
    unsigned int    mb_skip_conv_disable     : 1;
    unsigned int    is_last_mb               : 1;
    unsigned int    enable_coefficient_clamp : 1; 
    unsigned int    direct8x8_pattern        : 4;
 
    union 
    {
        /* Intra MBs */
        struct 
        {
            unsigned int   luma_intra_pred_modes0 : 16;
            unsigned int   luma_intra_pred_modes1 : 16;

            unsigned int   luma_intra_pred_modes2 : 16;
            unsigned int   luma_intra_pred_modes3 : 16;

            unsigned int   mb_intra_struct        : 8; 
            unsigned int   reserved60             : 24; 
        } intra_mb;

        /* Inter MBs */
        struct 
        {
            unsigned int   sub_mb_shapes          : 8; 
            unsigned int   sub_mb_pred_modes      : 8;
            unsigned int   reserved40             : 16; 
 
            unsigned int   ref_idx_l0_0           : 8; 
            unsigned int   ref_idx_l0_1           : 8; 
            unsigned int   ref_idx_l0_2           : 8; 
            unsigned int   ref_idx_l0_3           : 8; 

            unsigned int   ref_idx_l1_0           : 8; 
            unsigned int   ref_idx_l1_1           : 8; 
            unsigned int   ref_idx_l1_2           : 8; 
            unsigned int   ref_idx_l1_3           : 8; 
        } inter_mb;
    } mb_mode;

    unsigned int   reserved70                : 16; 
    unsigned int   target_size_in_word       : 8;
    unsigned int   max_size_in_word          : 8; 

    unsigned int   reserved2[4];
} VAEncFEIModeBufferH264Intel;

/** \brief VAEncFEIDistortionBufferIntel defines the data structure for 
 * VAEncFEIDistortionBufferType per 16x16 MB block. The 16x16 block is in raster scan order. 
 * Buffer size shall not be less than the number of 16x16 blocks multiple by sizeof(VAEncFEIDistortionBufferIntel). 
 **/
typedef struct _VAEncFEIDistortionBufferH264Intel {
    /** \brief Inter-prediction-distortion associated with motion vector i (co-located with subblock_4x4_i). 
     * Its meaning is determined by sub-shape. It must be zero if the corresponding sub-shape is not chosen. 
     **/
    unsigned short  inter_distortion[16];
    unsigned short  best_inter_distortion;
    unsigned short  best_intra_distortion;
} VAEncFEIDistortionBufferH264Intel;

#ifdef __cplusplus
}
#endif

#endif /* VA_INTEL_FEI_H */
