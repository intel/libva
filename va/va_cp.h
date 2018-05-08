/*
 * Copyright (c) 2018 Intel Corporation. All Rights Reserved.
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
* \file va_cp.h
* \brief The content protection API
*
* This file contains the \ref api_cp "Content Protection API".
*/

#ifndef VA_CP_H
#define VA_CP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup api_cp Content Protection API
 *
 * @{
 *
 * The content protection API uses the same paradigm for codec.
 * - Query for supported encryption attributes through VAConfigAttribEncryption.
 * - create the specific configuration with the accepted encryption attributes got from the query.
 * - Set up a codec pipeline;
 
 * The basic operation steps for cenc decode are:
 * - Query for supported encryption attributes through VAConfigAttribEncryption.
 * - Create configuration with one of encryption attributes got from the above step.
 * - Create a video decode.
 * - Pass the render buffers with the corresponding data to CENC decode, including:
 *   encrypted bitstream through VAProtectedSliceDataBufferType;
 *   encryption paramters through VAEncryptionParameters.
 * - Make the end of CENC call via vaEndCenc
 * - Get CENC result via vaQueryCenc.
 * - Pass the render buffers with the corresponding data to decode, including:
 *   CENC status report number through VACencStatusParameterBufferType.
 *   DPB information through VAPictureParameterBuffer.
 * - Make the end of decode call via vaEndPicture
 */

/** attribute values for VAConfigAttribEncryption */
#define VA_ENCRYPTION_TYPE_NONE             0x00000000
#define VA_ENCRYPTION_TYPE_BASIC            0x00000001 
#define VA_ENCRYPTION_TYPE_CENC_CBC         0x00000002
#define VA_ENCRYPTION_TYPE_CENC_CTR_LENGTH  0x00000004
#define VA_ENCRYPTION_TYPE_CENC_CTR         0x00000008
#define VA_ENCRYPTION_TYPE_CTR_128          0x00000010

/** \brief values for the encryption return status. */
typedef enum
{
    /** \brief Indicate encryption operation is successful.*/ 
    VA_ENCRYPTION_STATUS_SUCCESSFUL  = 0,
    /** \brief Indicate encryption operation is incomplete.*/ 
    VA_ENCRYPTION_STATUS_INCOMPLETE,
    /** \brief Indicate encryption operation is error.*/ 
    VA_ENCRYPTION_STATUS_ERROR,
    /** \brief Indicate encryption result isn't available.*/ 
    VA_ENCRYPTION_STATUS_UNAVAILABLE
} VAEncryptionStatus;

/** \brief structure for encrypted segment info. */
typedef struct _VAEncryptionSegmentInfo
{
    /** \brief  The offset relative to the start of the bitstream input of
     *  the start of the segment*/
    uint32_t segment_start_offset;
    /** \brief  The length of the segments in bytes*/
    uint32_t segment_length;
    /** \brief  The length in bytes of the remainder of an incomplete block
     *  from a previous segment*/
    uint32_t partial_aes_block_size;
    /** \brief  The length in bytes of the initial clear data */
    uint32_t init_byte_length; 
    /** \brief  This will be AES 128 counter for secure decode and secure
     *  encode when numSegments equals 1 */
    uint32_t aes_cbc_iv_or_ctr[4];
    /** \brief Reserved bytes for future use, must be zero */
    uint32_t va_reserved[VA_PADDING_MEDIUM];
} VAEncryptionSegmentInfo;

/** \brief encrytpion parameters*/
typedef struct _VAEncryptionParameters
{
    /** \brief Encryption type, attribute values. */
    uint32_t encryption_type;
    /** \brief Hw session id */
    uint32_t session_id;
    /** \brief The number of sengments */
    uint32_t num_segments;
    /** \brief Pointer of segments */
    VAEncryptionSegmentInfo *segment_info;
    /** \brief The status report number for CENC workload */ 
    uint16_t  status_report_number;
    /** \brief CENC counter length */
    uint16_t size_of_length;
    /** \brief Reserved bytes for future use, must be zero */
    unsigned long va_reserved[VA_PADDING_MEDIUM];
} VAEncryptionParameters;

/**
 * \brief Slice parameter for H.264 cenc decode in baseline, main & high profiles.
 *
 * This structure holds information for \c
 * slice_layer_without_partitioning_rbsp() and nal_unit()of the slice 
 * as defined by the H.264 specification.
 *
 */
typedef struct _VACencSliceParameterBufferH264 {

    /** \brief Parameters from \c nal_unit() of the slice.*/
    /**@{*/
    /** \brief  Same as the H.264 bitstream syntax element. */
    uint8_t nal_ref_idc;
    /** \brief Indicate if this is coded slice of an IDR picture. 
     * Corresponds to IdrPicFlag as the H.264 specification.*/
    uint8_t idr_pic_flag;
    /**@}*/
    /** \brief Same as the H.264 bitstream syntax element. */
    uint8_t slice_type;
    /** \brief Indicate if this is a field or frame picture.
     * \c VA_FRAME_PICTURE, \c VA_TOP_FIELD, \c VA_BOTTOM_FIELD*/
    uint8_t field_frame_flag;
    /** \brief Same as the H.264 bitstream syntax element. */
    uint32_t frame_number;
    /** \brief Same as the H.264 bitstream syntax element. */
    uint32_t idr_pic_id;
    /** \brief Same as the H.264 bitstream syntax element. */
    int32_t pic_order_cnt_lsb;
    /** \brief Same as the H.264 bitstream syntax element. */
    int32_t delta_pic_order_cnt_bottom;
    /** \brief Same as the H.264 bitstream syntax element. */
    int32_t delta_pic_order_cnt[2];
    /**
     * \brief decoded reference picture marking. Information for \c 
     * dec_ref_pic_marking() as defined by the H.264 specification.
     */
    /**@{*/
    union {
        struct {
            /** \brief Same as the H.264 bitstream syntax element. */
            uint32_t no_output_of_prior_pics_flag             : 1;
            /** \brief Same as the H.264 bitstream syntax element. */
            uint32_t long_term_reference_flag                 : 1;
            /** \brief Same as the H.264 bitstream syntax element. */
            uint32_t adaptive_ref_pic_marking_mode_flag       : 1;
            /** \brief number of decode reference picture marking. */
            uint32_t dec_ref_pic_marking_count                : 8;;
            /** \brief Reserved for future use, must be zero */
            uint32_t reserved                                 : 21;
            } bits;
        uint32_t value;
    } ref_pic_fields;
    /** \brief Same as the H.264 bitstream syntax element. */
    uint8_t memory_management_control_operation[32];
    /** \brief Same as the H.264 bitstream syntax element. */
    int32_t difference_of_pic_nums_minus1[32];
    /** \brief Same as the H.264 bitstream syntax element. */
    int32_t long_term_pic_num[32];
    /** \brief Same as the H.264 bitstream syntax element. */
    int32_t max_long_term_frame_idx_plus1[32];
    /** \brief Same as the H.264 bitstream syntax element. */
    int32_t long_term_frame_idx[32];
    /**@}*/
    /** \brief Pointer to the next #VACencSliceParameterBufferH264 element,
    * or \c NULL if there is none.*/
    void *next;
    /** \brief Reserved bytes for future use, must be zero */
    unsigned long va_reserved[VA_PADDING_MEDIUM];
} VACencSliceParameterBufferH264;

/**
 * \brief Slice parameter for HEVC cenc decode in main & main 10 profiles.
 *
 * This structure holds information for \c
 * slice_segment_header() and nal_unit_header() of the slice as
 * defined by the HEVC specification. 
 *
 */
typedef struct _VACencSliceParameterBufferHEVC {
    /** \brief Same as the HEVC bitstream syntax element. */
    uint8_t nal_unit_type;
    /** \brief Corresponds to the HEVC bitstream syntax element. 
     * Same as nuh_temporal_id_plus1 - 1*/
    uint8_t nuh_temporal_id;    
    /** \brief Slice type.
     *  Corresponds to HEVC syntax element of the same name. */
    uint8_t slice_type;
    /** \brief Same as the HEVC bitstream syntax element. */
    uint16_t  slice_pic_order_cnt_lsb;
    /** \brief Indicates EOS_NUT or EOB_NUT is detected in picture. */
    uint16_t has_eos_or_eob;

    union {
        struct {
            /** \brief Same as the HEVC bitstream syntax element */
            uint32_t no_output_of_prior_pics_flag      : 1;
            /** \brief Same as the HEVC bitstream syntax element */
            uint32_t pic_output_flag                   : 1;
            /** \brief Same as the HEVC bitstream syntax element */
            uint32_t colour_plane_id                    : 2;
            /** \brief Reserved for future use, must be zero */
            uint32_t reserved                             : 19;
        } bits;
        uint32_t        value;
    } slice_fields;    

    /** \brief  Parameters for driver reference frame set */
    /**@{*/
    
    /** \brief number of entries as current before in short-term rps
     * Corresponds to NumPocStCurrBefore as the HEVC specification. */
    uint8_t num_of_curr_before;
    /** \brief number of entries as current after in short-term rps
     * Corresponds to NumPocStCurrAfter as the HEVC specification. */
    uint8_t num_of_curr_after;
    /** \brief number of entries as current total in short-term rps*/
    uint8_t num_of_curr_total;
    /** \brief number of entries as foll in short-term rps
     * Corresponds to NumPocStFoll as the HEVC specification.*/
    uint8_t num_of_foll_st;
    /** \brief number of entries as current in long-term rps
     * Corresponds to NumPocLtCurr as the HEVC specification. */
    uint8_t num_of_curr_lt;
    /** \brief number of entries as foll in long-term rps
     * Corresponds to NumPocLtFoll as the HEVC specification.*/
    uint8_t num_of_foll_lt;
    /** \brief delta poc as short-term current before 
     * Corresponds to PocStCurrBefore as the HEVC specification. */
    int32_t delta_poc_curr_before[8];
    /** \brief delta poc as short-term current after 
     * Corresponds to PocStCurrAfter, as the HEVC specification.*/
    int32_t delta_poc_curr_after[8];    
    /** \brief delta poc as short-term current total */
    int32_t delta_poc_curr_total[8];
    /** \brief delta poc as short-term foll 
     * Corresponds to PocStFoll as the HEVC specification.*/
    int32_t delta_poc_foll_st[16];
    /** \brief delta poc as long-term current 
     * Corresponds to PocLtCurr as the HEVC specification.*/
    int32_t delta_poc_curr_lt[8];
    /** \brief delta poc as long-term foll 
     * Corresponds to PocLtFoll, as the HEVC specification.*/
    int32_t delta_poc_foll_lt[16];
    /** \brief delta poc msb present flag
     * Same as the HEVC bitstream syntax element. */
    uint8_t delta_poc_msb_present_flag[16];
    /** \brief long-term reference RPS is used for reference by current picture*/
    uint8_t is_lt_curr_total[8];
    /** \brief index of reference picture list. [0] is for P and B slice, [1] is for B slice*/
    uint8_t ref_list_idx[2][16];
    /**@}*/
    /** \brief Pointer to the next #VACencSliceParameterBufferHEVC element,
    * or \c NULL if there is none.*/
    void *next;
    /** \brief Reserved bytes for future use, must be zero */
    unsigned long va_reserved[VA_PADDING_MEDIUM];
} VACencSliceParameterBufferHEVC;

/**
 * \brief uncompressed header for VP9 cenc decode
 *
 * This structure holds information for \c
 * uncompressed_header() as defined by the VP9 specification.
 *
 */
typedef struct _VACencSliceParameterBufferVP9 {
    
    union {
        struct {
            /** \brief Same as the VP9 bitstream syntax element. */
            uint32_t profile                              : 2;            
            /** \brief Same as the VP9 bitstream syntax element. */
            uint32_t show_existing_frame_flag             : 1;
            /** \brief Same as the VP9 bitstream syntax element. */
            uint32_t frame_to_show_map_idx                : 3;
            /** \brief Same as the VP9 bitstream syntax element. */
            uint32_t frame_type                           : 1;
            /** \brief Same as the VP9 bitstream syntax element. */
            uint32_t show_frame                           : 1;
            /** \brief Same as the VP9 bitstream syntax element. */
            uint32_t error_resilient_mode                 : 1;
            /** \brief Same as the VP9 bitstream syntax element. */
            uint32_t intra_only                           : 1;
            /** \brief Same as the VP9 bitstream syntax element. */
            uint32_t ten_or_twelve_bit                    : 1;
            /** \brief Same as the VP9 bitstream syntax element. */
            uint32_t color_space                          : 3; 
            /** \brief Same as the VP9 bitstream syntax element. */
            uint32_t color_range                          : 1;
            /** \brief Same as the VP9 bitstream syntax element. */
            uint32_t subsampling_x                        : 1;
            /** \brief Same as the VP9 bitstream syntax element. */
            uint32_t subsampling_y                        : 1;
            /** \brief Corresponds to ref_frame_idx[0] 
              * as the VP9 specification */
            uint32_t ref_frame_idx0                       : 3;
            /** \brief Corresponds to ref_frame_sign_bias[LAST_FRAME] 
              * as the VP9 specification */
            uint32_t ref_frame_sign_bias0                 : 1;
            /** \brief Corresponds to ref_frame_idx[1] 
              * as the VP9 specification */
            uint32_t ref_frame_idx1                       : 3;
            /** \brief Corresponds to ref_frame_sign_bias[GOLDEN_FRAME] 
              * as the VP9 specification */
            uint32_t ref_frame_sign_bias1                 : 1;
            /** \brief Corresponds to ref_frame_idx[2] 
              * as the VP9 specification */
            uint32_t ref_frame_idx2                       : 3;
            /** \brief Corresponds to ref_frame_sign_bias[ALTREF_FRAME] 
              * as the VP9 specification */
            uint32_t ref_frame_sign_bias2                 : 1;
            /** \brief Same as the VP9 bitstream syntax element. */
            uint32_t frame_parallel_decoding_mode         : 1;
            /** \brief Same as the VP9 bitstream syntax element. */
            uint32_t render_and_frame_size_different      : 1;
            /** \brief Reserved for future use, must be zero */
            uint32_t reserved                             : 1;
            } bits;
        uint32_t value;
    } header_fields; 
     /** \brief Same as the VP9 bitstream syntax element. */
    uint16_t frame_width_minus1;
     /** \brief Same as the VP9 bitstream syntax element. */
    uint16_t frame_height_minus1;
    /** \brief Same as the VP9 bitstream syntax element. */
    uint16_t render_width_minus1;
    /** \brief Same as the VP9 bitstream syntax element. */
    uint16_t render_height_minus1;
    /** \brief Same as the VP9 bitstream syntax element. */
    uint8_t refresh_frame_flags;
    /** \brief Reserved bytes for future use, must be zero */
    uint32_t va_reserved[VA_PADDING_MEDIUM];
} VACencSliceParameterBufferVP9;

/** \brief Cenc Slice Buffer Type*/
typedef enum {
    /** \brief Parsed slice parameters \c VACencSliceParameterBuffer* */
    VaCencSliceBufParamter = 1,
    /** \brief Raw slice header of bitstream*/
    VaCencSliceBufRaw      = 2
}VACencSliceBufType;

/** \brief Buffer for CENC status reporting*/
typedef struct _VACencStatusBuf
{
    /** \brief Encryption status. VA_ENCRYPTION_STATUS_SUCCESSFUL if 
     *  hardware has returned detailed inforamtion, others mean the 
     *  CENC result is invalid  */
    VAEncryptionStatus status;
    /* \brief status report feed back number */
    uint16_t           status_report_feedback_number;
    /** \brief Buf size in bytes.  0 means buf is invalid*/
    uint32_t           buf_size;
    /** \brief Buffer formatted as raw data from bitstream for sequence parameter,
     *  picture parameter, SEI parameters. Or \c NULL means buf is invalid.*/
    void               *buf;
    /** \brief Slice buffer type, see*/
    VACencSliceBufType slice_buf_type;
    /** \brief Slice buffer size in bytes. 0 means slice_buf is invalid*/
    uint32_t           slice_buf_size;
    /** \brief Slice buffer, parsed slice header information. Or \c NULL
     *  means slice_buf is invalid.*/
    void               *slice_buf;
    /** \brief Reserved bytes for future use, must be zero */
    unsigned long va_reserved[VA_PADDING_MEDIUM];
} VACencStatusBuf;

/**
 * Block call to get cenc status report. 
 * After the application sends out cenc workload, it must call vaQueryCenc
 * to get cenc status report for decrypt result.
 * upon the return, info will point to an array of \ref VACencStatusBuf structure,
 * The array is terminated if "status==-1" is detected.
 */
VAStatus vaQueryCenc(
    VADisplay dpy,
    VABufferID buffer,
    VACencStatusBuf *info
);

/**
 * Make the end of rendering for Cenc decode for the encrypted bitstream. 
 * The server should start all pending rendering operations and decryption 
 * processing related with  this encrypted bitstream.  */
VAStatus vaEndCenc (
    VADisplay dpy,
    VAContextID context
);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif
