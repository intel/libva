/*
 * Copyright (c) 2013 Intel Corporation. All Rights Reserved.
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
 * \file va_intel_statistics.h
 * \brief the Intel statistics API
 *
*/

#ifndef VA_INTEL_STATISTICS_H
#define VA_INTEL_STATISTICS_H

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Processing function for getting motion vectors and statistics. */

/** This processing function can output motion vectors, distortions (pure pixel distortion, no cost), 
 * number of non-zero coefficients, MB variance and MB pixel average.  
 * The purpose is to assist application to perform SCD, complexity analysis, segmentation, BRC, etc. 
 **/


/** \brief Motion Vector and Statistics frame level controls. 
 * VAStatsStatisticsParameterBufferTypeIntel for 16x16 block
 **/
typedef struct _VAStatsStatisticsParameter16x16Intel 
{
    /** \brief Source surface ID.  */
    VASurfaceID     input;

    VASurfaceID     *past_references;
    unsigned int    num_past_references;
    VASurfaceID     *future_references;
    unsigned int    num_future_references;

    /** \brief ID of the output surface. 
     * The number of outputs is determined by below DisableMVOutput and DisableStatisticsOutput. 
     * The output layout is defined by VAStatsStatisticsBufferType and VAStatsMotionVectorBufferType. 
     **/
    VASurfaceID     *outputs;

    /** \brief MV predictor. It is valid only when mv_predictor_ctrl is not 0. 
     * Each 16x16 block has a pair of MVs, one for past and one for future reference 
     * as defined by VAMotionVector. The 16x16 block is in raster scan order. 
     * Buffer size shall not be less than the number of 16x16 blocks multiplied by sizeof(VAMotionVector). 
     **/
    VASurfaceID     mv_predictor;

    /** \brief Qp input surface. It is valid only when mb_qp is set to 1. 
     * The data in this buffer correspond to the input source. 
     * One Qp per 16x16 block in raster scan order, each Qp is a signed char (8-bit) value. 
     **/
    VASurfaceID     qp;

    unsigned int    frame_qp                    : 8;     
    unsigned int    len_sp                      : 8;     
    unsigned int    max_len_sp                  : 8;     
    unsigned int    reserved0                   : 8;     

    unsigned int    sub_mb_part_mask            : 7;     
    unsigned int    sub_pel_mode                : 2;     
    unsigned int    inter_sad                   : 2;     
    unsigned int    intra_sad                   : 2;     
    unsigned int    adaptive_search	            : 1;     
    /** \brief indicate if future or/and past MV in mv_predictor buffer is valid. 
     * 0: MV predictor disabled
     * 1: MV predictor enabled for past reference
     * 2: MV predictor enabled for future reference
     * 3: MV predictor enabled for both past and future references
     **/
    unsigned int    mv_predictor_ctrl           : 3;     
    unsigned int    mb_qp                       : 1;     
    unsigned int    ft_enable                   : 1;
    unsigned int    reserved1                   : 13;    

    unsigned int    ref_width                   : 8;     
    unsigned int    ref_height                  : 8;
    unsigned int    search_window               : 3;     
    unsigned int    reserved2                   : 13;    

    /** \brief MVOutput. When set to 1, MV output is NOT provided */
    unsigned int	disable_mv_output           : 1;    
    /** \brief StatisticsOutput. When set to 1, Statistics output is NOT provided. */
    unsigned int    disable_statistics_output   : 1;    
    unsigned int    reserved3                   : 30;    

} VAStatsStatisticsParameter16x16Intel;

/** \brief VAStatsMotionVectorBufferTypeIntel. Motion vector buffer layout.
 * Motion vector output is per 4x4 block. For each 4x4 block there is a pair of past and future 
 * reference MVs as defined in VAMotionVector. Depending on Subblock partition, 
 * for the shape that is not 4x4, the MV is replicated so each 4x4 block has a pair of MVs. 
 * If only past reference is used, future MV should be ignored, and vice versa. 
 * The 16x16 block is in raster scan order, within the 16x16 block, each 4x4 block MV is ordered as below in memory. 
 * The buffer size shall be greater than or equal to the number of 16x16 blocks multiplied by (sizeof(VAMotionVector) * 16).
 *
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
 *
 **/

/** \brief VAStatsStatisticsBufferTypeIntel. Statistics buffer layout.
 * Statistics output is per 16x16 block. Data structure per 16x16 block is defined below. 
 * The 16x16 block is in raster scan order. The buffer size shall be greater than or equal to 
 * the number of 16x16 blocks multiplied by sizeof(VAStatsStatistics16x16Intel). 
 **/
typedef struct _VAStatsStatistics16x16Intel 
{
    /** \brief past reference  */
    unsigned int    best_inter_distortion0 : 16;     
    unsigned int    inter_mode0            : 16;     

    /** \brief future reference  */
    unsigned int    best_inter_distortion1 : 16;     
    unsigned int    inter_mode1            : 16;     

    unsigned int    best_intra_distortion  : 16;     
    unsigned int    intra_mode             : 16;     

    unsigned int    num_non_zero_coef      : 16;     
    unsigned int    reserved               : 16;     

    unsigned int    sum_coef; 

    unsigned int    variance;     
    unsigned int    pixel_average;
} VAStatsStatistics16x16Intel;

#ifdef __cplusplus
}
#endif

#endif /* VA_INTEL_STATISTICS_H */
