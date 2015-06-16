/*
 * Copyright (c) 2007-2012 Intel Corporation. All Rights Reserved.
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
 * \file va_enc_vp8.h
 * \brief VP8 encoding API
 *
 * This file contains the \ref api_enc_vp8 "VP8 encoding API".
 */

#ifndef VA_ENC_VP8_H
#define VA_ENC_VP8_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup api_enc_vp8 VP8 encoding API
 *
 * @{
 */

/**
 * \brief VP8 Encoding Sequence Parameter Buffer Structure
 *
 * This structure conveys sequence level parameters.
 *
 */
typedef struct  _VAEncSequenceParameterBufferVP8
{
    /* frame width in pixels */
    unsigned int frame_width;
    /* frame height in pixels */
    unsigned int frame_height;
    /* horizontal scale */
    unsigned int frame_width_scale;
    /* vertical scale */
    unsigned int frame_height_scale;

    /* whether to enable error resilience features */
    unsigned int error_resilient;
    /* auto keyframe placement, non-zero means enable auto keyframe placement */
    unsigned int kf_auto;
    /* keyframe minimum interval */
    unsigned int kf_min_dist;
    /* keyframe maximum interval */
    unsigned int kf_max_dist;


    /* RC related fields. RC modes are set with VAConfigAttribRateControl */
    /* For VP8, CBR implies HRD conformance and VBR implies no HRD conformance */

    /**
     * Initial bitrate set for this sequence in CBR or VBR modes.
     *
     * This field represents the initial bitrate value for this
     * sequence if CBR or VBR mode is used, i.e. if the encoder
     * pipeline was created with a #VAConfigAttribRateControl
     * attribute set to either \ref VA_RC_CBR or \ref VA_RC_VBR.
     *
     * The bitrate can be modified later on through
     * #VAEncMiscParameterRateControl buffers.
     */
    unsigned int bits_per_second;
    /* Period between I frames. */
    unsigned int intra_period;

    /* reference and reconstructed frame buffers
     * Used for driver auto reference management when configured through 
     * VAConfigAttribEncAutoReference. 
     */
    VASurfaceID reference_frames[4];

} VAEncSequenceParameterBufferVP8;


/**
 * \brief VP8 Encoding Picture Parameter Buffer Structure
 *
 * This structure conveys picture level parameters.
 *
 */
typedef struct  _VAEncPictureParameterBufferVP8
{
    /* surface to store reconstructed frame  */
    VASurfaceID reconstructed_frame;

    /* 
     * surfaces to store reference frames in non auto reference mode
     * VA_INVALID_SURFACE can be used to denote an invalid reference frame. 
     */
    VASurfaceID ref_last_frame;
    VASurfaceID ref_gf_frame;
    VASurfaceID ref_arf_frame;

    /**
     * \brief Output encoded bitstream.
     *
     * \ref coded_buf has type #VAEncCodedBufferType. It should be
     * large enough to hold the compressed bit stream.
     */
    VABufferID coded_buf;

    union {
        struct {
            /* force this frame to be a keyframe */
            unsigned int force_kf                       : 1;
            /* don't reference the last frame */
            unsigned int no_ref_last                    : 1;
            /* don't reference the golden frame */
            unsigned int no_ref_gf                      : 1;
            /* don't reference the alternate reference frame */
            unsigned int no_ref_arf                     : 1;
            /* The temporal id the frame belongs to. */
            unsigned int temporal_id                    : 8;
            /**
             *  following two flags indicate the reference order
             *  LastRef is specified by 01b;
             *  GoldRef is specified by 10b;
             *  AltRef  is specified by 11b;
             *  first_ref specifies the reference frame which is searched first.
             *  second_ref specifies the reference frame which is searched second
             *  if there is.
             */
            unsigned int first_ref                      : 2;
            unsigned int second_ref                     : 2;
            unsigned int reserved                       : 16;
    } bits;
        unsigned int value;
    } ref_flags;

    union {
        struct {
            /* version */
            unsigned int frame_type                     : 1;
            unsigned int version                        : 3;
            /* show_frame */
            unsigned int show_frame                     : 1;
            /* color_space */						   
            unsigned int color_space                    : 1;
            /*  0: bicubic, 1: bilinear, other: none */
            unsigned int recon_filter_type              : 2;
            /*  0: no loop fitler, 1: simple loop filter */
            unsigned int loop_filter_type               : 2;
            /* 0: disabled, 1: normal, 2: simple */
            unsigned int auto_partitions                : 1;
            /* same as log2_nbr_of_dct_partitions in frame header syntax */
            unsigned int num_token_partitions           : 2;

            /** 
             * The following fields correspond to the same VP8 syntax elements 
             * in the frame header.
             */
	    /**
             * 0: clamping of reconstruction pixels is disabled,
             * 1: clamping enabled.
             */
            unsigned int clamping_type                  : 1;
            /* indicate segmentation is enabled for the current frame. */
            unsigned int segmentation_enabled           : 1;
            /**
             * Determines if the MB segmentation map is updated in the current 
             * frame.
             */
            unsigned int update_mb_segmentation_map     : 1;
            /**
             * Indicates if the segment feature data is updated in the current 
             * frame.
             */
            unsigned int update_segment_feature_data    : 1;
            /**
             * indicates if the MB level loop filter adjustment is enabled for 
             * the current frame (0 off, 1 on).  
             */
	    unsigned int loop_filter_adj_enable         : 1;
            /**
             * Determines whether updated token probabilities are used only for 
             * this frame or until further update. 
             * It may be used by application to enable error resilient mode. 
             * In this mode probability updates are allowed only at Key Frames.
             */
            unsigned int refresh_entropy_probs          : 1;
            /**
             * Determines if the current decoded frame refreshes the golden frame.
             */
            unsigned int refresh_golden_frame           : 1;
            /** 
             * Determines if the current decoded frame refreshes the alternate 
             * reference frame.
             */
            unsigned int refresh_alternate_frame        : 1;
            /**
             * Determines if the current decoded frame refreshes the last frame 
             * reference buffer.
             */
            unsigned int refresh_last                   : 1;
            /**
             * Determines if the golden reference is replaced by another reference.
             */
            unsigned int copy_buffer_to_golden          : 2;
            /**
             * Determines if the alternate reference is replaced by another reference.
             */
            unsigned int copy_buffer_to_alternate       : 2;
            /** 
             * Controls the sign of motion vectors when the golden frame is referenced.  
             */
            unsigned int sign_bias_golden               : 1;
            /**
             * Controls the sign of motion vectors when the alternate frame is 
             * referenced. 
             */
	    unsigned int sign_bias_alternate            : 1;
            /**
             * Enables or disables the skipping of macroblocks containing no 
             * non-zero coefficients. 
             */
	    unsigned int mb_no_coeff_skip               : 1;
            /** 
             * Enforces unconditional per-MB loop filter delta update setting frame 
             * header flags mode_ref_lf_delta_update, all mb_mode_delta_update_flag[4], 
             * and all ref_frame_delta_update_flag[4] to 1. 
	     * Since loop filter deltas are not automatically refreshed to default 
             * values at key frames, dropped frame with delta update may prevent 
             * correct decoding from the next key frame. 
	     * Encoder application is advised to set this flag to 1 at key frames.
	     */
            unsigned int forced_lf_adjustment           : 1;
            unsigned int reserved                       : 2;
        } bits;
        unsigned int value;
    } pic_flags;

    /**
     * Contains a list of 4 loop filter level values (updated value if applicable)
     * controlling the deblocking filter strength. Each entry represents a segment.
     * When segmentation is disabled, use entry 0. 
     * When loop_filter_level is 0, loop filter shall be disabled. 
     */
    char loop_filter_level[4];

    /** 
     * Contains a list of 4 delta values for reference frame based MB-level 
     * loop filter adjustment.  
     * If no update, then set to 0.
     */
    char ref_lf_delta[4];

    /**
     * Contains a list of 4 delta values for coding mode based MB-level loop
     * filter adjustment.  
     * If no update, then set to 0. 
     */
    char mode_lf_delta[4];
	
    /**
     * Controls the deblocking filter sensitivity. 
     * Corresponds to the same VP8 syntax element in frame header.
     */
    unsigned char sharpness_level;
	
    /** 
     * Application supplied maximum clamp value for Qindex used in quantization.  
     * Qindex will not be allowed to exceed this value.  
     * It has a valid range [0..127] inclusive.  
     */
    unsigned char clamp_qindex_high;
	
    /**
     * Application supplied minimum clamp value for Qindex used in quantization.  
     * Qindex will not be allowed to be lower than this value.  
     * It has a valid range [0..127] inclusive.  
     * Condition clamp_qindex_low <= clamp_qindex_high must be guaranteed, 
     * otherwise they are ignored. 
     */
    unsigned char clamp_qindex_low;
	
} VAEncPictureParameterBufferVP8;

/**
 * \brief VP8 Quantization Matrix Buffer Structure
 *
 * Contains quantization index for yac(0-3) for each segment and quantization 
 * index deltas, ydc(0), y2dc(1), y2ac(2), uvdc(3), uvac(4) that are applied 
 * to all segments.  When segmentation is disabled, only quantization_index[0] 
 * will be used. This structure is sent once per frame.
 */
typedef struct _VAQMatrixBufferVP8
{
    unsigned short quantization_index[4];
    short quantization_index_delta[5];
} VAQMatrixBufferVP8;

/**
 * \brief VP8 MB Segmentation ID Buffer
 *
 * The application provides a buffer of VAEncMacroblockMapBufferType containing 
 * the initial segmentation id for each MB, one byte each, in raster scan order. 
 * Rate control may reassign it.  For example, a 640x480 video, the buffer has 1200 entries. 
 * The value of each entry should be in the range [0..3], inclusive.
 * If segmentation is not enabled, the application does not need to provide it. 
 */

/**
 * \brief VP8 Encoding Information Buffer Structure
 *
 * This structure is used to convey status data from encoder to application.
 * Driver allocates VACodedBufferVP8Status as a private data buffer.
 * Driver encapsulates the status buffer with a VACodedBufferSegment,
 * and sets VACodedBufferSegment.status to be VA_CODED_BUF_STATUS_CODEC_SPECIFIC.
 * And driver associates status data segment to the bit stream buffer segment
 * by setting VACodedBufferSegment.next of coded_buf (bit stream) to the private
 * buffer segment of status data.
 * Application accesses it by calling VAMapBuffer() with VAEncCodedBufferType.
 *
 */
typedef struct _VACodedBufferVP8Status
{
    /** Final quantization index used (yac), determined by BRC.
     *  Application is providing quantization index deltas
     *  ydc(0), y2dc(1), y2ac(2), uvdc(3), uvac(4) that are applied to all segments
     *  and segmentation qi deltas, they will not be changed by BRC.
     */
    unsigned short quantization_index;

    /** Final loopfilter levels for the frame, if segmentation is disabled
     *  only index 0 is used.
     * If loop_filter_level[] is 0, it indicates loop filter is disabled.
     */
    char loop_filter_level[4];

    /** Long term reference frame indication from BRC.  BRC recommends the
     *  current frame that is being queried is a good candidate for a long
     *  term reference.
     */
    unsigned char long_term_indication;
} VACodedBufferVP8Status;

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* VA_ENC_VP8_H */
