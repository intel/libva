/*
 * Copyright (c) 2016 Intel Corporation. All Rights Reserved.
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
 */

/*
 * Simple H.264/AVC temporal scalability encoder based on libVA.
 *
 */

#include "sysdeps.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#include <pthread.h>

#include <va/va.h>
#include "va_display.h"

#define SLICE_TYPE_P                    0
#define SLICE_TYPE_B                    1
#define SLICE_TYPE_I                    2

#define IS_I_SLICE(type)                (SLICE_TYPE_I == (type) || SLICE_TYPE_I == (type - 5))
#define IS_P_SLICE(type)                (SLICE_TYPE_P == (type) || SLICE_TYPE_P == (type - 5))
#define IS_B_SLICE(type)                (SLICE_TYPE_B == (type) || SLICE_TYPE_B == (type - 5))

#define NAL_REF_IDC_NONE                0
#define NAL_REF_IDC_LOW                 1
#define NAL_REF_IDC_MEDIUM              2
#define NAL_REF_IDC_HIGH                3

#define NAL_NON_IDR                     1
#define NAL_IDR                         5
#define NAL_SEI			        6
#define NAL_SPS                         7
#define NAL_PPS                         8
#define NAL_PREFIX                      14
#define NAL_SUBSET_SPS                  15

#define ENTROPY_MODE_CAVLC              0
#define ENTROPY_MODE_CABAC              1

#define PROFILE_IDC_BASELINE            66
#define PROFILE_IDC_MAIN                77
#define PROFILE_IDC_SCALABLE_BASELINE   83
#define PROFILE_IDC_SCALABLE_HIGH       86
#define PROFILE_IDC_HIGH                100

#define SRC_SURFACE_IN_ENCODING         0
#define SRC_SURFACE_IN_STORAGE          1

#define NUM_SURFACES                    32

#define ARRAY_ELEMS(a)                  (sizeof(a) / sizeof((a)[0]))

#define CHECK_VASTATUS(va_status, func)                                 \
    if (va_status != VA_STATUS_SUCCESS) {                               \
        fprintf(stderr,"%s:%s (%d) failed, exit\n", __func__, func, __LINE__); \
        exit(1);                                                        \
    }

#define MAX_SLICES                      32
#define MAX_LAYERS                      4

#define MIN(a, b)                       ((a) > (b) ? (b) : (a))
#define MAX(a, b)                       ((a) > (b) ? (a) : (b))

static VASurfaceID src_surfaces[NUM_SURFACES];
static VASurfaceID rec_surfaces[NUM_SURFACES];
static int src_surface_status[NUM_SURFACES];

static int temporal_ids_in_bgop[16] = { // index is (encoding order) % gop_size - 1, available from the 2nd encoded frame
    0,                                  /* temporal 0 */
    1,                                  /* temporal 1 */
    2, 2,                               /* temporal 2 */
    3, 3, 3, 3,                         /* temporal 3 */
    4, 4, 4, 4, 4, 4, 4, 4              /* temporal 4 */
};

static int temporal_ids_in_pgop[16] = { // index is (encoding order) % gop_size - 1, available from the 2nd encoded frame
    1, 2, 1, 3,                         // each element is (the number of temporal layers - temporal id)
    1, 2, 1, 3,
    1, 2, 1, 3,
    1, 2, 1, 3,
};

static int gop_factors_in_bgop[16] = {
    1,
    1,
    1, 3,
    1, 3, 5, 7,
    1, 3, 5, 7, 9, 11, 13, 15
};

static float frame_rates[4] = {
    7.5,
    15,
    30,
    60,
};

static VAProfile g_va_profiles[] = {
    VAProfileH264High,
    VAProfileH264Baseline,
    VAProfileH264ConstrainedBaseline,
};

typedef struct _svcenc_surface
{
    int slot_in_surfaces; /* index in src_surfaces and rec_surfaces */
    int coding_order;
    int display_order;
    int temporal_id;
    int frame_num;
    int poc;
    unsigned int is_intra : 1;
    unsigned int is_idr : 1;
    unsigned int is_ref : 1;
    VAEncPictureType picture_type;
    VASurfaceID rec_surface;
} svcenc_surface;

static svcenc_surface ref_frames[16], ref_list0[32], ref_list1[32];

static pthread_mutex_t upload_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t upload_cond = PTHREAD_COND_INITIALIZER;

struct upload_task_t {
    void *next;
    unsigned int display_order;
    unsigned int surface;
};

struct svcenc_context
{
    /* parameter info */
    FILE *ifp;  /* a FILE pointer for source YUV file */
    FILE *ofp;  /* a FILE pointer for output SVC file */
    int width;
    int height;
    int frame_size;
    int num_pictures;
    int num_slices;
    int qp;     /* quantisation parameter, default value is 26 */
    unsigned char *frame_data_buffer;   /* buffer for input surface, the length is the maximum frame_size */
    int gop_size;
    int max_num_ref_frames;
    int num_ref_frames;
    int hierarchical_levels;
    int layer_brc;

    /* the info for next picture in encoding order */
    svcenc_surface next_svcenc_surface;

    /* GOP info */
    int intra_idr_period;
    int intra_period;
    int ip_period;
    int num_remainder_bframes;
    int gop_type;       /* 0: p hierarchical, 1: B hierarchical, default is 0 */

    /* bitrate info */
    int rate_control_mode;
    int bits_per_kbps;
    int framerate_per_100s;
    int i_initial_cpb_removal_delay;
    int i_initial_cpb_removal_delay_offset;
    int i_initial_cpb_removal_delay_length;
    int i_cpb_removal_delay;
    int i_cpb_removal_delay_length;
    int i_dpb_output_delay_length;
    int time_offset_length;

    unsigned long long idr_frame_num;
    unsigned long long prev_idr_cpb_removal;
    unsigned long long current_idr_cpb_removal;
    unsigned long long current_cpb_removal;

    /* This is relative to the current_cpb_removal */
    unsigned int current_dpb_removal_delta;

    int profile_idc;
    int constraint_set_flag;

    int svc_profile_idc;
    int svc_constraint_set_flag;

    /* reordering info for l0/l1,
     * bit0-3: ref_pic_list_modification_flag_lX (X=0,1),
     * bit4-7: modification_of_pic_nums_idc,
     * bit8-15: abs_diff_pic_num_minus1
     * bit16-23: num_ref_idx_active_override_flag
     * bit24-31: num_ref_idx_lX_active_minus1 (X=0,1),
     */
    unsigned int reordering_info[2];

    /* VA info */
    VADisplay va_dpy;
    VAProfile profile;
    VAEncSequenceParameterBufferH264 seq_param;
    VAEncPictureParameterBufferH264 pic_param;
    VAEncSliceParameterBufferH264 slice_param[MAX_SLICES];
    VAContextID context_id;
    VAConfigID config_id;
    VABufferID seq_param_buf_id;                /* Sequence level parameter */
    VABufferID pic_param_buf_id;                /* Picture level parameter */
    VABufferID slice_param_buf_id[MAX_SLICES];  /* Slice level parameter, multil slices */
    VABufferID codedbuf_buf_id;                 /* Output buffer, compressed data */
    VABufferID packed_sei_scalability_info_header_param_buf_id;
    VABufferID packed_sei_scalability_info_buf_id;
    VABufferID packed_seq_header_param_buf_id;
    VABufferID packed_seq_buf_id;
    VABufferID packed_svc_seq_header_param_buf_id;
    VABufferID packed_svc_seq_buf_id;
    VABufferID packed_pic_header_param_buf_id;
    VABufferID packed_pic_buf_id;
    VABufferID packed_sei_header_param_buf_id;   /* the SEI buffer */
    VABufferID packed_sei_buf_id;
    VABufferID misc_parameter_layer_structure_buf_id;
    VABufferID misc_parameter_ratecontrol_buf_id[MAX_LAYERS];
    VABufferID misc_parameter_framerate_buf_id[MAX_LAYERS];
    VABufferID misc_parameter_hrd_buf_id;
    VABufferID packed_slice_header_param_buf_id[MAX_SLICES];
    VABufferID packed_slice_header_data_buf_id[MAX_SLICES];
    VABufferID packed_prefix_nal_unit_param_buf_id[MAX_SLICES];
    VABufferID packed_prefix_nal_unit_data_buf_id[MAX_SLICES];

    /* thread info */
    pthread_t upload_thread;
    struct upload_task_t *upload_task_header;
    struct upload_task_t *upload_task_tail;
};

/* bitstream */
#define BITSTREAM_ALLOCATE_STEPPING     4096

struct __bitstream {
    unsigned int *buffer;
    int bit_offset;
    int max_size_in_dword;
};

typedef struct __bitstream bitstream;

static unsigned int
va_swap32(unsigned int val)
{
    unsigned char *pval = (unsigned char *)&val;

    return ((pval[0] << 24)     |
            (pval[1] << 16)     |
            (pval[2] << 8)      |
            (pval[3] << 0));
}

static void
bitstream_start(bitstream *bs)
{
    bs->max_size_in_dword = BITSTREAM_ALLOCATE_STEPPING;
    bs->buffer = calloc(bs->max_size_in_dword * sizeof(int), 1);
    bs->bit_offset = 0;
}

static void
bitstream_end(bitstream *bs)
{
    int pos = (bs->bit_offset >> 5);
    int bit_offset = (bs->bit_offset & 0x1f);
    int bit_left = 32 - bit_offset;

    if (bit_offset) {
        bs->buffer[pos] = va_swap32((bs->buffer[pos] << bit_left));
    }
}

static void
bitstream_put_ui(bitstream *bs, unsigned int val, int size_in_bits)
{
    int pos = (bs->bit_offset >> 5);
    int bit_offset = (bs->bit_offset & 0x1f);
    int bit_left = 32 - bit_offset;

    if (!size_in_bits)
        return;

    bs->bit_offset += size_in_bits;

    if (bit_left > size_in_bits) {
        bs->buffer[pos] = (bs->buffer[pos] << size_in_bits | val);
    } else {
        size_in_bits -= bit_left;
        bs->buffer[pos] = (bs->buffer[pos] << bit_left) | (val >> size_in_bits);
        bs->buffer[pos] = va_swap32(bs->buffer[pos]);

        if (pos + 1 == bs->max_size_in_dword) {
            bs->max_size_in_dword += BITSTREAM_ALLOCATE_STEPPING;
            bs->buffer = realloc(bs->buffer, bs->max_size_in_dword * sizeof(unsigned int));
        }

        bs->buffer[pos + 1] = val;
    }
}

static void
bitstream_put_ue(bitstream *bs, unsigned int val)
{
    int size_in_bits = 0;
    int tmp_val = ++val;

    while (tmp_val) {
        tmp_val >>= 1;
        size_in_bits++;
    }

    bitstream_put_ui(bs, 0, size_in_bits - 1); // leading zero
    bitstream_put_ui(bs, val, size_in_bits);
}

static void
bitstream_put_se(bitstream *bs, int val)
{
    unsigned int new_val;

    if (val <= 0)
        new_val = -2 * val;
    else
        new_val = 2 * val - 1;

    bitstream_put_ue(bs, new_val);
}

static void
bitstream_byte_aligning(bitstream *bs, int bit)
{
    int bit_offset = (bs->bit_offset & 0x7);
    int bit_left = 8 - bit_offset;
    int new_val;

    if (!bit_offset)
        return;

    assert(bit == 0 || bit == 1);

    if (bit)
        new_val = (1 << bit_left) - 1;
    else
        new_val = 0;

    bitstream_put_ui(bs, new_val, bit_left);
}

static void
rbsp_trailing_bits(bitstream *bs)
{
    bitstream_put_ui(bs, 1, 1);
    bitstream_byte_aligning(bs, 0);
}

static void
nal_start_code_prefix(bitstream *bs)
{
    bitstream_put_ui(bs, 0x00000001, 32);
}

static void
nal_header(bitstream *bs, int nal_ref_idc, int nal_unit_type)
{
    bitstream_put_ui(bs, 0, 1);                /* forbidden_zero_bit: 0 */
    bitstream_put_ui(bs, nal_ref_idc, 2);
    bitstream_put_ui(bs, nal_unit_type, 5);
}

static void
sps_data(struct svcenc_context *ctx,
         const VAEncSequenceParameterBufferH264 *seq_param,
         bitstream *bs)
{
    bitstream_put_ui(bs, ctx->profile_idc, 8);               /* profile_idc */
    bitstream_put_ui(bs, !!(ctx->constraint_set_flag & 1), 1);                          /* constraint_set0_flag */
    bitstream_put_ui(bs, !!(ctx->constraint_set_flag & 2), 1);                          /* constraint_set1_flag */
    bitstream_put_ui(bs, !!(ctx->constraint_set_flag & 4), 1);                          /* constraint_set2_flag */
    bitstream_put_ui(bs, !!(ctx->constraint_set_flag & 8), 1);                          /* constraint_set3_flag */
    bitstream_put_ui(bs, !!(ctx->constraint_set_flag & 16), 1);                         /* constraint_set4_flag */
    bitstream_put_ui(bs, !!(ctx->constraint_set_flag & 32), 1);                         /* constraint_set5_flag */
    bitstream_put_ui(bs, 0, 2);                         /* reserved_zero_2bits */
    bitstream_put_ui(bs, seq_param->level_idc, 8);      /* level_idc */
    bitstream_put_ue(bs, seq_param->seq_parameter_set_id);      /* seq_parameter_set_id */

    if (ctx->profile_idc == PROFILE_IDC_HIGH ||
        ctx->profile_idc == PROFILE_IDC_SCALABLE_HIGH ||
        ctx->profile_idc == PROFILE_IDC_SCALABLE_BASELINE) {
        bitstream_put_ue(bs, 1);        /* chroma_format_idc = 1, 4:2:0 */
        bitstream_put_ue(bs, 0);        /* bit_depth_luma_minus8 */
        bitstream_put_ue(bs, 0);        /* bit_depth_chroma_minus8 */
        bitstream_put_ui(bs, 0, 1);     /* qpprime_y_zero_transform_bypass_flag */
        bitstream_put_ui(bs, 0, 1);     /* seq_scaling_matrix_present_flag */
    }

    bitstream_put_ue(bs, seq_param->seq_fields.bits.log2_max_frame_num_minus4); /* log2_max_frame_num_minus4 */
    bitstream_put_ue(bs, seq_param->seq_fields.bits.pic_order_cnt_type);        /* pic_order_cnt_type */

    if (seq_param->seq_fields.bits.pic_order_cnt_type == 0)
        bitstream_put_ue(bs, seq_param->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4);     /* log2_max_pic_order_cnt_lsb_minus4 */
    else {
        assert(0);
    }

    bitstream_put_ue(bs, seq_param->max_num_ref_frames);        /* num_ref_frames */
    bitstream_put_ui(bs, 0, 1);                                 /* gaps_in_frame_num_value_allowed_flag */

    bitstream_put_ue(bs, seq_param->picture_width_in_mbs - 1);  /* pic_width_in_mbs_minus1 */
    bitstream_put_ue(bs, seq_param->picture_height_in_mbs - 1); /* pic_height_in_map_units_minus1 */
    bitstream_put_ui(bs, seq_param->seq_fields.bits.frame_mbs_only_flag, 1);    /* frame_mbs_only_flag */

    if (!seq_param->seq_fields.bits.frame_mbs_only_flag) {
        assert(0);
    }

    bitstream_put_ui(bs, seq_param->seq_fields.bits.direct_8x8_inference_flag, 1);      /* direct_8x8_inference_flag */
    bitstream_put_ui(bs, seq_param->frame_cropping_flag, 1);            /* frame_cropping_flag */

    if (seq_param->frame_cropping_flag) {
        bitstream_put_ue(bs, seq_param->frame_crop_left_offset);        /* frame_crop_left_offset */
        bitstream_put_ue(bs, seq_param->frame_crop_right_offset);       /* frame_crop_right_offset */
        bitstream_put_ue(bs, seq_param->frame_crop_top_offset);         /* frame_crop_top_offset */
        bitstream_put_ue(bs, seq_param->frame_crop_bottom_offset);      /* frame_crop_bottom_offset */
    }

    if (ctx->bits_per_kbps < 0) {
        bitstream_put_ui(bs, 0, 1); /* vui_parameters_present_flag */
    } else {
        bitstream_put_ui(bs, 1, 1); /* vui_parameters_present_flag */
        bitstream_put_ui(bs, 0, 1); /* aspect_ratio_info_present_flag */
        bitstream_put_ui(bs, 0, 1); /* overscan_info_present_flag */
        bitstream_put_ui(bs, 0, 1); /* video_signal_type_present_flag */
        bitstream_put_ui(bs, 0, 1); /* chroma_loc_info_present_flag */
        bitstream_put_ui(bs, 1, 1); /* timing_info_present_flag */
        {
            bitstream_put_ui(bs, seq_param->num_units_in_tick, 32);
            bitstream_put_ui(bs, seq_param->time_scale, 32);
            bitstream_put_ui(bs, 1, 1);
        }
        bitstream_put_ui(bs, 1, 1); /* nal_hrd_parameters_present_flag */
        {
            // hrd_parameters
            bitstream_put_ue(bs, 0);    /* cpb_cnt_minus1 */
            bitstream_put_ui(bs, 0, 4); /* bit_rate_scale */
            bitstream_put_ui(bs, 2, 4); /* cpb_size_scale */

	    /* the bits_per_kbps is in kbps */
            bitstream_put_ue(bs, (((ctx->bits_per_kbps * 1024) >> 6) - 1)); /* bit_rate_value_minus1[0] */
            bitstream_put_ue(bs, ((ctx->bits_per_kbps * 8 * 1024) >> 6) - 1); /* cpb_size_value_minus1[0] */
            bitstream_put_ui(bs, 1, 1);  /* cbr_flag[0] */

            /* initial_cpb_removal_delay_length_minus1 */
            bitstream_put_ui(bs, (ctx->i_initial_cpb_removal_delay_length - 1), 5);
            /* cpb_removal_delay_length_minus1 */
            bitstream_put_ui(bs, (ctx->i_cpb_removal_delay_length - 1), 5);
            /* dpb_output_delay_length_minus1 */
            bitstream_put_ui(bs, (ctx->i_dpb_output_delay_length - 1), 5);
            /* time_offset_length  */
            bitstream_put_ui(bs, (ctx->time_offset_length - 1), 5);
        }

        bitstream_put_ui(bs, 0, 1);   /* vcl_hrd_parameters_present_flag */
        bitstream_put_ui(bs, 0, 1);   /* low_delay_hrd_flag */

        bitstream_put_ui(bs, 0, 1); /* pic_struct_present_flag */
        bitstream_put_ui(bs, 0, 1); /* bitstream_restriction_flag */
    }
}

static void
sps_svc_extension(struct svcenc_context *ctx,
                  const VAEncSequenceParameterBufferH264 *sps_param,
                  bitstream *bs)
{
    bitstream_put_ui(bs, 0, 1); /* inter_layer_deblocking_filter_control_present_flag */
    bitstream_put_ui(bs, 0, 2); /* extended_spatial_scalability_idc */

    /* if (ChromaArrayType == 1) */
    bitstream_put_ui(bs, 0, 1); /* chroma_phase_x_plus1_flag */
    bitstream_put_ui(bs, 1, 2); /* chroma_phase_y_plus1 */

#if 0
    if (extended_spatial_scalability_idc == 1) {
        /* if (ChromaArrayType > 0) */
        bitstream_put_ui(bs, 0, 1); /* seq_ref_layer_chroma_phase_x_plus1_flag */
        bitstream_put_ui(bs, 0, 2); /* seq_ref_layer_chroma_phase_y_plus1 */

        bitstream_put_se(bs, 0); /* seq_scaled_ref_layer_left_offset */
        bitstream_put_se(bs, 0); /* seq_scaled_ref_layer_top_offset */
        bitstream_put_se(bs, 0); /* seq_scaled_ref_layer_right_offset */
        bitstream_put_se(bs, 0); /* seq_scaled_ref_layer_bottom_offset */
    }
#endif

    bitstream_put_ui(bs, 0, 1); /* seq_tcoeff_level_prediction_flag */

#if 0
    if (seq_tcoeff_level_prediction_flag)
        bitstream_put_ui(bs, 0, 1); /* adaptive_tcoeff_level_prediction_flag */
#endif

    bitstream_put_ui(bs, 0, 1); /* slice_header_restriction_flag */
}

static void
sps_svc_vui_parameters_extension(struct svcenc_context *ctx,
                                 const VAEncSequenceParameterBufferH264 *sps_param,
                                 bitstream *bs)
{
    bitstream_put_ui(bs, 0, 1); /* svc_vui_parameters_present_flag */
}

static void
sps_additional_extension2(struct svcenc_context *ctx,
                          const VAEncSequenceParameterBufferH264 *sps_param,
                          bitstream *bs)
{
    bitstream_put_ui(bs, 0, 1); /* additional_extension2_flag */
}

static void
pps_rbsp(struct svcenc_context *ctx,
         const VAEncPictureParameterBufferH264 *pic_param,
         bitstream *bs)
{
    bitstream_put_ue(bs, pic_param->pic_parameter_set_id);      /* pic_parameter_set_id */
    bitstream_put_ue(bs, pic_param->seq_parameter_set_id);      /* seq_parameter_set_id */

    bitstream_put_ui(bs, pic_param->pic_fields.bits.entropy_coding_mode_flag, 1);  /* entropy_coding_mode_flag */

    bitstream_put_ui(bs, 0, 1);                         /* pic_order_present_flag: 0 */

    bitstream_put_ue(bs, 0);                            /* num_slice_groups_minus1 */

    bitstream_put_ue(bs, pic_param->num_ref_idx_l0_active_minus1);      /* num_ref_idx_l0_active_minus1 */
    bitstream_put_ue(bs, pic_param->num_ref_idx_l1_active_minus1);      /* num_ref_idx_l1_active_minus1 1 */

    bitstream_put_ui(bs, pic_param->pic_fields.bits.weighted_pred_flag, 1);     /* weighted_pred_flag: 0 */
    bitstream_put_ui(bs, pic_param->pic_fields.bits.weighted_bipred_idc, 2);	/* weighted_bipred_idc: 0 */

    bitstream_put_se(bs, pic_param->pic_init_qp - 26);  /* pic_init_qp_minus26 */
    bitstream_put_se(bs, 0);                            /* pic_init_qs_minus26 */
    bitstream_put_se(bs, 0);                            /* chroma_qp_index_offset */

    bitstream_put_ui(bs, pic_param->pic_fields.bits.deblocking_filter_control_present_flag, 1); /* deblocking_filter_control_present_flag */
    bitstream_put_ui(bs, 0, 1);                         /* constrained_intra_pred_flag */
    bitstream_put_ui(bs, 0, 1);                         /* redundant_pic_cnt_present_flag */

    /* more_rbsp_data */
    bitstream_put_ui(bs, pic_param->pic_fields.bits.transform_8x8_mode_flag, 1);    /*transform_8x8_mode_flag */
    bitstream_put_ui(bs, 0, 1);                         /* pic_scaling_matrix_present_flag */
    bitstream_put_se(bs, pic_param->second_chroma_qp_index_offset );    /*second_chroma_qp_index_offset */

    rbsp_trailing_bits(bs);
}

static int
build_packed_seq_buffer(struct svcenc_context *ctx,
                        const VAEncSequenceParameterBufferH264 *seq_param,
                        unsigned char **header_buffer)
{
    bitstream bs;

    bitstream_start(&bs);

    nal_start_code_prefix(&bs);
    nal_header(&bs, NAL_REF_IDC_HIGH, NAL_SPS);

    sps_data(ctx, seq_param, &bs);
    rbsp_trailing_bits(&bs);     /* rbsp_trailing_bits */

    bitstream_end(&bs);

    *header_buffer = (unsigned char *)bs.buffer;
    return bs.bit_offset;
}

static int
build_packed_subset_seq_buffer(struct svcenc_context *ctx,
                               const VAEncSequenceParameterBufferH264 *seq_param,
                               unsigned char **header_buffer)
{
    bitstream bs;

    bitstream_start(&bs);

    nal_start_code_prefix(&bs);
    nal_header(&bs, NAL_REF_IDC_HIGH, NAL_SUBSET_SPS);

    sps_data(ctx, seq_param, &bs);
    sps_svc_extension(ctx, seq_param, &bs);
    sps_svc_vui_parameters_extension(ctx, seq_param, &bs);
    sps_additional_extension2(ctx, seq_param, &bs);
    rbsp_trailing_bits(&bs);     /* rbsp_trailing_bits */

    bitstream_end(&bs);

    *header_buffer = (unsigned char *)bs.buffer;
    return bs.bit_offset;
}

static int
build_packed_pic_buffer(struct svcenc_context *ctx,
                        const VAEncPictureParameterBufferH264 *pic_param,
                        unsigned char **header_buffer)
{
    bitstream bs;

    bitstream_start(&bs);
    nal_start_code_prefix(&bs);
    nal_header(&bs, NAL_REF_IDC_HIGH, NAL_PPS);
    pps_rbsp(ctx, pic_param, &bs);
    bitstream_end(&bs);

    *header_buffer = (unsigned char *)bs.buffer;
    return bs.bit_offset;
}

static int
build_packed_sei_buffering_period_buffer(struct svcenc_context *ctx,
                                         const VAEncSequenceParameterBufferH264 *seq_param,
                                         int frame_num,
                                         unsigned char **sei_buffer)
{
    bitstream sei_bp_bs;

    if (ctx->rate_control_mode & VA_RC_CQP ||
        frame_num) {
        *sei_buffer = NULL;
        return 0;
    }

    bitstream_start(&sei_bp_bs);
    bitstream_put_ue(&sei_bp_bs, seq_param->seq_parameter_set_id);       /* seq_parameter_set_id */
    /* SEI buffer period info */
    /* NALHrdBpPresentFlag == 1 */
    bitstream_put_ui(&sei_bp_bs, ctx->i_initial_cpb_removal_delay, ctx->i_initial_cpb_removal_delay_length);
    bitstream_put_ui(&sei_bp_bs, ctx->i_initial_cpb_removal_delay_offset, ctx->i_initial_cpb_removal_delay_length);

    if (sei_bp_bs.bit_offset & 0x7) {
        bitstream_put_ui(&sei_bp_bs, 1, 1);
    }

    bitstream_end(&sei_bp_bs);

    *sei_buffer = (unsigned char *)sei_bp_bs.buffer;

    return (sei_bp_bs.bit_offset + 7) / 8;
}

static int
build_packed_sei_pic_timing_buffer(struct svcenc_context *ctx, int frame_num, unsigned char **sei_buffer)
{
    bitstream sei_pic_bs;
    unsigned int cpb_removal_delay;

    if (ctx->rate_control_mode & VA_RC_CQP) {
        *sei_buffer = 0;
        return 0;
    }

    /* SEI pic timing info */
    bitstream_start(&sei_pic_bs);

    /* The info of CPB and DPB delay is controlled by CpbDpbDelaysPresentFlag,
     * which is derived as 1 if one of the following conditions is true:
     * nal_hrd_parameters_present_flag is present in the bitstream and is equal to 1,
     * vcl_hrd_parameters_present_flag is present in the bitstream and is equal to 1,
     */
    cpb_removal_delay = (ctx->current_cpb_removal - ctx->prev_idr_cpb_removal);
    bitstream_put_ui(&sei_pic_bs, cpb_removal_delay, ctx->i_cpb_removal_delay_length);
    bitstream_put_ui(&sei_pic_bs, ctx->current_dpb_removal_delta, ctx->i_dpb_output_delay_length);

    if (sei_pic_bs.bit_offset & 0x7) {
        bitstream_put_ui(&sei_pic_bs, 1, 1);
    }

    /* The pic_structure_present_flag determines whether the pic_structure
     * info is written into the SEI pic timing info.
     * Currently it is set to zero.
     */
    bitstream_end(&sei_pic_bs);

    *sei_buffer = (unsigned char *)sei_pic_bs.buffer;

    return (sei_pic_bs.bit_offset + 7) / 8;
}

static int
build_packed_sei_scalability_info_buffer(struct svcenc_context *ctx,
                                         const VAEncSequenceParameterBufferH264 *seq_param,
                                         const VAEncPictureParameterBufferH264 *pic_param,
                                         int frame_num,
                                         unsigned char **sei_buffer)
{
    bitstream scalability_info_bs;
    int i;

    if (frame_num) { // non IDR
        *sei_buffer = NULL;
        return 0;
    }

    /* Write scalability_info */
    bitstream_start(&scalability_info_bs);

    bitstream_put_ui(&scalability_info_bs, 0, 1);       // temporal_id_nesting_flag: false
    bitstream_put_ui(&scalability_info_bs, 0, 1);       // priority_layer_info_present_flag: false
    bitstream_put_ui(&scalability_info_bs, 0, 1);       // priority_id_setting_flag: false
    bitstream_put_ue(&scalability_info_bs, ctx->hierarchical_levels - 1); // num_layers_minus1

    for (i = 0; i < ctx->hierarchical_levels; i++) {
        bitstream_put_ue(&scalability_info_bs, i);      // layer_id[i]
        bitstream_put_ui(&scalability_info_bs, 0, 6);   // priority_id[i[
        bitstream_put_ui(&scalability_info_bs, 0, 1);   // discardable_flag[i]
        bitstream_put_ui(&scalability_info_bs, 0, 3);   // dependency_id[i]
        bitstream_put_ui(&scalability_info_bs, 0, 4);   // quality_id[i]
        bitstream_put_ui(&scalability_info_bs, i, 3);   // temporal_id[i]
        bitstream_put_ui(&scalability_info_bs, 0, 1);   // sub_pic_layer_flag[i]
        bitstream_put_ui(&scalability_info_bs, 0, 1);   // sub_region_layer_flag[i]
        bitstream_put_ui(&scalability_info_bs, 0, 1);   // iroi_division_info_present_flag[i]
        bitstream_put_ui(&scalability_info_bs, 0, 1);   // profile_level_info_present_flag[i]
        bitstream_put_ui(&scalability_info_bs, 0, 1);   // bitrate_info_present_flag[i]
        bitstream_put_ui(&scalability_info_bs, 1, 1);   // frm_rate_info_present_flag[i]
        bitstream_put_ui(&scalability_info_bs, 1, 1);   // frm_size_info_present_flag[i]
        bitstream_put_ui(&scalability_info_bs, 0, 1);   // layer_dependency_info_present_flag[i]
        bitstream_put_ui(&scalability_info_bs, 0, 1);   // parameter_sets_info_present_flag[i]
        bitstream_put_ui(&scalability_info_bs, 0, 1);   // bitstream_restriction_info_present_flag[i]
        bitstream_put_ui(&scalability_info_bs, 0, 1);   // exact_interlayer_pred_flag[i]
        bitstream_put_ui(&scalability_info_bs, 0, 1);   // layer_conversion_flag[i]
        bitstream_put_ui(&scalability_info_bs, 0, 1);   // layer_output_flag[i]

        bitstream_put_ui(&scalability_info_bs, 0, 2);   // constant_frm_bitrate_idc[i]
        bitstream_put_ui(&scalability_info_bs, (int)floor(frame_rates[i] * 256 + 0.5), 16);     // avg_frm_rate

        bitstream_put_ue(&scalability_info_bs, seq_param->picture_width_in_mbs - 1);    // frm_width_in_mbs_minus1
        bitstream_put_ue(&scalability_info_bs, seq_param->picture_height_in_mbs - 1);   // frm_height_in_mbs_minus1

        bitstream_put_ue(&scalability_info_bs, 0);      // layer_dependency_info_src_layer_id_delta[i]
        bitstream_put_ue(&scalability_info_bs, 0);      // parameter_sets_info_src_layer_id_delta[i]
    }

    rbsp_trailing_bits(&scalability_info_bs);
    bitstream_end(&scalability_info_bs);

    *sei_buffer = (unsigned char *)scalability_info_bs.buffer;

    return (scalability_info_bs.bit_offset + 7) / 8;
}

static void
svcenc_update_sei_info(struct svcenc_context *ctx, svcenc_surface *current_surface)
{
    unsigned long long frame_interval;

    if (!(ctx->rate_control_mode & VA_RC_CBR)) {
        return;
    }

    frame_interval = current_surface->coding_order - ctx->idr_frame_num;

    if (current_surface->is_idr) {
        ctx->current_cpb_removal = ctx->prev_idr_cpb_removal + frame_interval * 2;
        ctx->idr_frame_num = current_surface->coding_order;
        ctx->current_idr_cpb_removal = ctx->current_cpb_removal;

        if (ctx->ip_period)
            ctx->current_dpb_removal_delta = (ctx->ip_period + 1) * 2;
        else
            ctx->current_dpb_removal_delta = 2;
    } else {
        ctx->current_cpb_removal = ctx->current_idr_cpb_removal + frame_interval * 2;

        if (current_surface->picture_type == VAEncPictureTypeIntra ||
            current_surface->picture_type == VAEncPictureTypePredictive) {
            if (ctx->ip_period)
                ctx->current_dpb_removal_delta = (ctx->ip_period + 1) * 2;
            else
                ctx->current_dpb_removal_delta = 2;
        } else
            ctx->current_dpb_removal_delta = 2;
    }
}

static int
build_packed_sei_buffer(struct svcenc_context *ctx,
                        const VAEncSequenceParameterBufferH264 *seq_param,
                        const VAEncPictureParameterBufferH264 *pic_param,
                        svcenc_surface *current_surface,
                        unsigned char **sei_buffer)
{
    unsigned char *scalability_info_buffer = NULL, *buffering_period_buffer = NULL, *pic_timing_buffer = NULL;
    int scalability_info_size = 0, buffering_period_size = 0,  pic_timing_size = 0;
    bitstream nal_bs;
    int i;

    svcenc_update_sei_info(ctx, current_surface);

    buffering_period_size = build_packed_sei_buffering_period_buffer(ctx, seq_param, current_surface->frame_num, &buffering_period_buffer);
    pic_timing_size = build_packed_sei_pic_timing_buffer(ctx, current_surface->frame_num, &pic_timing_buffer);
    scalability_info_size = build_packed_sei_scalability_info_buffer(ctx, seq_param, pic_param, current_surface->frame_num, &scalability_info_buffer);

    if (!buffering_period_buffer &&
        !pic_timing_buffer &&
        !scalability_info_buffer) {
        *sei_buffer = NULL;
        return 0;
    }

    bitstream_start(&nal_bs);
    nal_start_code_prefix(&nal_bs);
    nal_header(&nal_bs, NAL_REF_IDC_NONE, NAL_SEI);

    /* Write the SEI buffer */
    if (buffering_period_buffer) {
        assert(buffering_period_size);

        bitstream_put_ui(&nal_bs, 0, 8);                        // last_payload_type_byte: 0
        bitstream_put_ui(&nal_bs, buffering_period_size, 8);    // last_payload_size_byte

        for (i = 0; i < buffering_period_size; i++) {
            bitstream_put_ui(&nal_bs, buffering_period_buffer[i], 8);
        }

        free(buffering_period_buffer);
    }

    if (pic_timing_buffer) {
        assert(pic_timing_size);

        bitstream_put_ui(&nal_bs, 1, 8);                        // last_payload_type_byte: 1
        bitstream_put_ui(&nal_bs, pic_timing_size, 8);          // last_payload_size_byte

        for (i = 0; i < pic_timing_size; i++) {
            bitstream_put_ui(&nal_bs, pic_timing_buffer[i], 8);
        }

        free(pic_timing_buffer);
    }

    if (scalability_info_buffer) {
        assert(scalability_info_size);

        bitstream_put_ui(&nal_bs, 24, 8);                       // last_payload_type_byte: 24
        bitstream_put_ui(&nal_bs, scalability_info_size, 8);    // last_payload_size_byte

        for (i = 0; i < scalability_info_size; i++) {
            bitstream_put_ui(&nal_bs, scalability_info_buffer[i], 8);
        }

        free(scalability_info_buffer);
    }

    rbsp_trailing_bits(&nal_bs);
    bitstream_end(&nal_bs);

    *sei_buffer = (unsigned char *)nal_bs.buffer;

    return nal_bs.bit_offset;
}

static void
slice_header(bitstream *bs,
             VAEncSequenceParameterBufferH264 *sps_param,
             VAEncPictureParameterBufferH264 *pic_param,
             VAEncSliceParameterBufferH264 *slice_param,
             unsigned int reordering_info[2])
{
    int first_mb_in_slice = slice_param->macroblock_address;
    int i;

    bitstream_put_ue(bs, first_mb_in_slice);        /* first_mb_in_slice: 0 */
    bitstream_put_ue(bs, slice_param->slice_type);  /* slice_type */
    bitstream_put_ue(bs, slice_param->pic_parameter_set_id);        /* pic_parameter_set_id: 0 */
    bitstream_put_ui(bs, pic_param->frame_num, sps_param->seq_fields.bits.log2_max_frame_num_minus4 + 4); /* frame_num */

    /* frame_mbs_only_flag == 1 */
    if (!sps_param->seq_fields.bits.frame_mbs_only_flag) {
        /* FIXME: */
        assert(0);
    }

    if (pic_param->pic_fields.bits.idr_pic_flag)
        bitstream_put_ue(bs, slice_param->idr_pic_id);		/* idr_pic_id: 0 */

    if (sps_param->seq_fields.bits.pic_order_cnt_type == 0) {
        bitstream_put_ui(bs, pic_param->CurrPic.TopFieldOrderCnt, sps_param->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 + 4);
        /* pic_order_present_flag == 0 */
    } else {
        /* FIXME: */
        assert(0);
    }

    /* redundant_pic_cnt_present_flag == 0 */
    /* slice type */
    if (IS_P_SLICE(slice_param->slice_type)) {
        bitstream_put_ui(bs, !!((reordering_info[0] >> 16) & 0xFF), 1);     /* num_ref_idx_active_override_flag: */

        if ((reordering_info[0] >> 16) & 0xFF)
            bitstream_put_ue(bs, (reordering_info[0] >> 24) & 0xFF);        /* num_ref_idx_l0_active_minus1 */

        /* ref_pic_list_reordering */
        if (!reordering_info || !(reordering_info[0] & 0x0F))
            bitstream_put_ui(bs, 0, 1);             /* ref_pic_list_reordering_flag_l0: 0 */
        else {
            bitstream_put_ui(bs, 1, 1);             /* ref_pic_list_reordering_flag_l0: 1 */
            bitstream_put_ue(bs, (reordering_info[0] >> 4) & 0x0F);
            bitstream_put_ue(bs, (reordering_info[0] >> 8) & 0xFF);
            bitstream_put_ue(bs, 3);                /* modification_of_pic_nums_idc: 3 */
        }
    } else if (IS_B_SLICE(slice_param->slice_type)) {
        bitstream_put_ui(bs, slice_param->direct_spatial_mv_pred_flag, 1);            /* direct_spatial_mv_pred: 1 */

        bitstream_put_ui(bs, ((reordering_info[0] >> 16) & 0xFF) || ((reordering_info[1] >> 16) & 0xFF) , 1);               /* num_ref_idx_active_override_flag: */

        if (((reordering_info[0] >> 16) & 0xFF) || ((reordering_info[1] >> 16) & 0xFF)) {
            bitstream_put_ue(bs, (reordering_info[0] >> 24) & 0xFF);        /* num_ref_idx_l0_active_minus1 */
            bitstream_put_ue(bs, (reordering_info[1] >> 24) & 0xFF);        /* num_ref_idx_l0_active_minus1 */
        }

        /* ref_pic_list_reordering */

        for (i = 0; i < 2; i++) {
            if (!reordering_info || !(reordering_info[i] & 0x0F))
                bitstream_put_ui(bs, 0, 1);             /* ref_pic_list_reordering_flag_l0/l1: 0 */
            else {
                bitstream_put_ui(bs, 1, 1);             /* ref_pic_list_reordering_flag_l0/l1: 1 */
                bitstream_put_ue(bs, (reordering_info[i] >> 4) & 0x0F);
                bitstream_put_ue(bs, (reordering_info[i] >> 8) & 0xFF);
                bitstream_put_ue(bs, 3);                /* modification_of_pic_nums_idc: 3 */
            }
        }
    }

    if ((pic_param->pic_fields.bits.weighted_pred_flag &&
         IS_P_SLICE(slice_param->slice_type)) ||
        ((pic_param->pic_fields.bits.weighted_bipred_idc == 1) &&
         IS_B_SLICE(slice_param->slice_type))) {
        /* FIXME: fill weight/offset table */
        assert(0);
    }

    /* dec_ref_pic_marking */
    if (pic_param->pic_fields.bits.reference_pic_flag) {     /* nal_ref_idc != 0 */
        unsigned char no_output_of_prior_pics_flag = 0;
        unsigned char long_term_reference_flag = 0;
        unsigned char adaptive_ref_pic_marking_mode_flag = 0;

        if (pic_param->pic_fields.bits.idr_pic_flag) {
            bitstream_put_ui(bs, no_output_of_prior_pics_flag, 1);            /* no_output_of_prior_pics_flag: 0 */
            bitstream_put_ui(bs, long_term_reference_flag, 1);            /* long_term_reference_flag: 0 */
        } else {
            bitstream_put_ui(bs, adaptive_ref_pic_marking_mode_flag, 1);            /* adaptive_ref_pic_marking_mode_flag: 0 */
        }
    }

    if (pic_param->pic_fields.bits.entropy_coding_mode_flag &&
        !IS_I_SLICE(slice_param->slice_type))
        bitstream_put_ue(bs, slice_param->cabac_init_idc);               /* cabac_init_idc: 0 */

    bitstream_put_se(bs, slice_param->slice_qp_delta);                   /* slice_qp_delta: 0 */

    /* ignore for SP/SI */

    if (pic_param->pic_fields.bits.deblocking_filter_control_present_flag) {
        bitstream_put_ue(bs, slice_param->disable_deblocking_filter_idc);           /* disable_deblocking_filter_idc: 0 */

        if (slice_param->disable_deblocking_filter_idc != 1) {
            bitstream_put_se(bs, slice_param->slice_alpha_c0_offset_div2);          /* slice_alpha_c0_offset_div2: 2 */
            bitstream_put_se(bs, slice_param->slice_beta_offset_div2);              /* slice_beta_offset_div2: 2 */
        }
    }

    if (pic_param->pic_fields.bits.entropy_coding_mode_flag) {
        bitstream_byte_aligning(bs, 1);
    }
}

static void
nal_header_extension(bitstream *bs,
                     VAEncPictureParameterBufferH264 *pic_param,
                     unsigned int temporal_id)
{
    int is_idr = !!pic_param->pic_fields.bits.idr_pic_flag;

    /* 3 bytes */
    bitstream_put_ui(bs, 1, 1);     /* svc_extension_flag */
    bitstream_put_ui(bs, !!is_idr, 1);
    bitstream_put_ui(bs, 0, 6);     /* priority_id */
    bitstream_put_ui(bs, 1, 1);     /* no_inter_layer_pred_flag */
    bitstream_put_ui(bs, 0, 3);     /* dependency_id */
    bitstream_put_ui(bs, 0, 4);     /* quality_id */
    bitstream_put_ui(bs, temporal_id, 3); /* temporal_id */
    bitstream_put_ui(bs, 0, 1);     /* use_ref_base_pic_flag */
    bitstream_put_ui(bs, 1, 1);     /* discardable_flag */
    bitstream_put_ui(bs, 1, 1);     /* output_flag */
    bitstream_put_ui(bs, 3, 2);     /* reserved_three_2bits */
}

static void
nal_unit_svc_prefix_rbsp(bitstream *bs, int is_ref)
{
    if (is_ref) {
        bitstream_put_ui(bs, 0, 1); /* store_ref_base_pic_flag */
        bitstream_put_ui(bs, 0, 1); /* additional_prefix_nal_unit_extension_flag*/
    } else {
        /* no more rbsp data */
    }

    rbsp_trailing_bits(bs);
}

static int
build_packed_svc_prefix_nal_unit(VAEncPictureParameterBufferH264 *pic_param,
                                 unsigned int temporal_id,
                                 unsigned char **nal_unit_buffer)
{
    int is_ref = !!pic_param->pic_fields.bits.reference_pic_flag;
    bitstream bs;

    bitstream_start(&bs);
    nal_start_code_prefix(&bs);

    nal_header(&bs, is_ref ? NAL_REF_IDC_LOW : NAL_REF_IDC_NONE, NAL_PREFIX);
    nal_header_extension(&bs, pic_param, temporal_id);
    nal_unit_svc_prefix_rbsp(&bs, is_ref);

    bitstream_end(&bs);
    *nal_unit_buffer = (unsigned char *)bs.buffer;

    return bs.bit_offset;
}

static int
build_packed_slice_header_buffer(struct svcenc_context *ctx,
                                 VAEncSequenceParameterBufferH264 *sps_param,
                                 VAEncPictureParameterBufferH264 *pic_param,
                                 VAEncSliceParameterBufferH264 *slice_param,
                                 unsigned int reordering_info[2],
                                 unsigned char **header_buffer)
{
    bitstream bs;
    int is_idr = !!pic_param->pic_fields.bits.idr_pic_flag;
    int is_ref = !!pic_param->pic_fields.bits.reference_pic_flag;

    bitstream_start(&bs);
    nal_start_code_prefix(&bs);

    if (IS_I_SLICE(slice_param->slice_type)) {
        nal_header(&bs, NAL_REF_IDC_HIGH, is_idr ? NAL_IDR : NAL_NON_IDR);
    } else if (IS_P_SLICE(slice_param->slice_type)) {
        assert(!is_idr);
        nal_header(&bs, is_ref ? NAL_REF_IDC_MEDIUM : NAL_REF_IDC_NONE, NAL_NON_IDR);
    } else {
        assert(IS_B_SLICE(slice_param->slice_type));
        assert(!is_idr);
        nal_header(&bs, is_ref ? NAL_REF_IDC_LOW : NAL_REF_IDC_NONE, NAL_NON_IDR);
    }

    slice_header(&bs, sps_param, pic_param, slice_param, reordering_info);

    bitstream_end(&bs);
    *header_buffer = (unsigned char *)bs.buffer;

    return bs.bit_offset;
}

/* upload thread */
static struct upload_task_t *
upload_task_dequeue(struct svcenc_context *ctx)
{
    struct upload_task_t *task;

    pthread_mutex_lock(&upload_mutex);

    task = ctx->upload_task_header;

    if (task) {
        ctx->upload_task_header = task->next;

        if (!ctx->upload_task_header)
            ctx->upload_task_tail = NULL;
    }

    pthread_mutex_unlock(&upload_mutex);

    return task;
}

static int
upload_task_queue(struct svcenc_context *ctx, unsigned int display_order, int surface)
{
    struct upload_task_t *task;

    task = calloc(1, sizeof(struct upload_task_t));
    task->display_order = display_order;
    task->surface = surface;
    task->next = NULL;

    pthread_mutex_lock(&upload_mutex);

    if (!ctx->upload_task_header)
        ctx->upload_task_header = task;

    if (ctx->upload_task_tail)
        ctx->upload_task_tail->next = task;

    ctx->upload_task_tail = task;
    src_surface_status[surface] = SRC_SURFACE_IN_STORAGE;
    pthread_cond_signal(&upload_cond);

    pthread_mutex_unlock(&upload_mutex);

    return 0;
}

static void
upload_task(struct svcenc_context *ctx, unsigned int display_order, int surface)
{
    VAStatus va_status;
    VAImage surface_image;
    void *surface_p = NULL;
    size_t n_items;
    unsigned char *y_src, *u_src, *v_src;
    unsigned char *y_dst, *u_dst, *v_dst;
    int row, col;
    int y_size = ctx->width * ctx->height;
    int u_size = (ctx->width >> 1) * (ctx->height >> 1);

    fseek(ctx->ifp, ctx->frame_size * display_order, SEEK_SET);

    do {
        n_items = fread(ctx->frame_data_buffer, ctx->frame_size, 1, ctx->ifp);
    } while (n_items != 1);

    va_status = vaDeriveImage(ctx->va_dpy, src_surfaces[surface], &surface_image);
    CHECK_VASTATUS(va_status,"vaDeriveImage");

    vaMapBuffer(ctx->va_dpy, surface_image.buf, &surface_p);
    assert(VA_STATUS_SUCCESS == va_status);

    y_src = ctx->frame_data_buffer;
    u_src = ctx->frame_data_buffer + y_size; /* UV offset for NV12 */
    v_src = ctx->frame_data_buffer + y_size + u_size;

    y_dst = surface_p + surface_image.offsets[0];
    u_dst = surface_p + surface_image.offsets[1]; /* UV offset for NV12 */
    v_dst = surface_p + surface_image.offsets[2];

    /* Y plane */
    for (row = 0; row < surface_image.height; row++) {
        memcpy(y_dst, y_src, surface_image.width);
        y_dst += surface_image.pitches[0];
        y_src += ctx->width;
    }

    if (surface_image.format.fourcc == VA_FOURCC_NV12) { /* UV plane */
        for (row = 0; row < surface_image.height / 2; row++) {
            for (col = 0; col < surface_image.width / 2; col++) {
                u_dst[col * 2] = u_src[col];
                u_dst[col * 2 + 1] = v_src[col];
            }

            u_dst += surface_image.pitches[1];
            u_src += (ctx->width / 2);
            v_src += (ctx->width / 2);
        }
    } else {
        for (row = 0; row < surface_image.height / 2; row++) {
            for (col = 0; col < surface_image.width / 2; col++) {
                u_dst[col] = u_src[col];
                v_dst[col] = v_src[col];
            }

            u_dst += surface_image.pitches[1];
            v_dst += surface_image.pitches[2];
            u_src += (ctx->width / 2);
            v_src += (ctx->width / 2);
        }
    }

    vaUnmapBuffer(ctx->va_dpy, surface_image.buf);
    vaDestroyImage(ctx->va_dpy, surface_image.image_id);

    pthread_mutex_lock(&upload_mutex);
    src_surface_status[surface] = SRC_SURFACE_IN_ENCODING;
    pthread_mutex_unlock(&upload_mutex);
}

static void *
upload_task_thread(void *data)
{
    struct svcenc_context *ctx = (struct svcenc_context *)data;
    int num = 0;

    while (1) {
        struct upload_task_t *task;

        task = upload_task_dequeue(ctx);
        if (!task) {
            pthread_mutex_lock(&upload_mutex);
            pthread_cond_wait(&upload_cond, &upload_mutex);
            pthread_mutex_unlock(&upload_mutex);

            continue;
        }

        upload_task(ctx, task->display_order, task->surface);
        free(task);

        if (++num >= ctx->num_pictures)
            break;
    }

    return 0;
}

/* main thread */
static void
usage(char *program)
{
    fprintf(stderr, "Usage: %s --help\n", program);
    fprintf(stderr, "\t--help print this message\n");
    fprintf(stderr, "Usage: %s <width> <height> <ifile> <ofile> [options] \n", program);
    fprintf(stderr, "\t<width>\t\tThe base picture width\n");
    fprintf(stderr, "\t<height>\tThe base picture height\n");
    fprintf(stderr, "\t<ifiles>\tThe input YUV I420 file\n");
    fprintf(stderr, "\t<ofile>\t\tThe output H.264/SVC file\n\n");
    fprintf(stderr, "\tAvailable options:\n");
    fprintf(stderr, "\t--gop <value>\t\tGOP size, default is 8\n");
    fprintf(stderr, "\t--goptype <value>\tGOP hierarchical type, 0 is P hierarchical GOP, 1 is B hierarchical GOP, default is 0\n");
    fprintf(stderr, "\t--brcmode <value>\tBRC mode, 0 is CQP mode, otherwise CBR mode, default mode is CQP\n");
    fprintf(stderr, "\t--brclayer<value>\tDisable/Enalbe BRC per temporal layer, default is disable(0)\n");
    fprintf(stderr, "\t--bitrate <value>\tbit rate in Kbps(1024 bits). It is available for CBR mode only and default value is 2000\n");
}

static void
svcenc_exit(struct svcenc_context *ctx, int exit_code)
{
    if (ctx->ifp) {
        fclose(ctx->ifp);
        ctx->ifp = NULL;
    }

    if (ctx->ofp) {
        fclose(ctx->ofp);
        ctx->ofp = NULL;
    }

    exit(exit_code);
}

static void
parse_args(struct svcenc_context *ctx, int argc, char **argv)
{
    int c, tmp;
    int option_index = 0;
    long file_size;

    static struct option long_options[] = {
        {"gop",         required_argument,      0,      'g'},
        {"goptype",     required_argument,      0,      't'},
        {"bitrate",     required_argument,      0,      'b'},
        {"brcmode",     required_argument,      0,      'm'},
        {"brclayer",    required_argument,      0,      'l'},
        {"help",        no_argument,            0,      'h'},
        { NULL,         0,                      NULL,   0 }
    };

    va_init_display_args(&argc, argv);

    if ((argc == 2 && strcmp(argv[1], "--help") == 0) ||
        (argc < 5))
        goto print_usage;

    ctx->width = atoi(argv[1]);
    ctx->height = atoi(argv[2]);

    if (ctx->width <= 0 || ctx->height <= 0) {
        fprintf(stderr, "<base width> and <base height> must be greater than 0\n");
        goto err_exit;
    }

    ctx->frame_size = ctx->width * ctx->height * 3 / 2;
    ctx->ifp = fopen(argv[3], "rb");

    if (ctx->ifp == NULL) {
        fprintf(stderr, "Can't open the input file\n");
        goto err_exit;
    }

    fseek(ctx->ifp, 0l, SEEK_END);
    file_size = ftell(ctx->ifp);

    if ((file_size < ctx->frame_size) ||
        (file_size % ctx->frame_size)) {
        fprintf(stderr, "The input file size %ld isn't a multiple of the frame size %d\n", file_size, ctx->frame_size);
        goto err_exit;
    }

    assert(ctx->num_pictures == 0);
    ctx->num_pictures = file_size / ctx->frame_size;
    fseek(ctx->ifp, 0l, SEEK_SET);

    ctx->ofp = fopen(argv[4], "wb");

    if (ctx->ofp == NULL) {
        fprintf(stderr, "Can't create the output file\n");
        goto err_exit;
    }

    opterr = 0;
    optind = 5;
    ctx->gop_size = 8;
    ctx->gop_type = 0;
    ctx->bits_per_kbps = -1;
    ctx->rate_control_mode = VA_RC_CQP;
    ctx->layer_brc = 0;

    while ((c = getopt_long(argc, argv,
                            "",
                            long_options,
                            &option_index)) != -1) {
        switch(c) {
        case 'g':
            tmp = atoi(optarg);

            if ((tmp & (tmp - 1)) ||
                tmp > 8) {
                fprintf(stderr, "Warning: invalid GOP size\n");
                tmp = 8;
            }

            ctx->gop_size = tmp;

            break;

        case 't':
            tmp = atoi(optarg);

            ctx->gop_type = !!tmp;

            break;

        case 'b':
            tmp = atoi(optarg);

            if (tmp <= 0)
                ctx->bits_per_kbps = -1;
            else {
                if (tmp >= 30000)
                    tmp = 30000;

                ctx->bits_per_kbps = tmp;
            }

            break;

        case 'm':
            tmp = atoi(optarg);

            if (tmp == 0)
                ctx->rate_control_mode = VA_RC_CQP;
            else
                ctx->rate_control_mode = VA_RC_CBR;

            break;

        case 'l':
            tmp = atoi(optarg);

            ctx->layer_brc = !!tmp;

            break;

        case '?':
            fprintf(stderr, "Error: unkown command options\n");

        case 'h':
            goto print_usage;
        }
    }

    ctx->num_pictures = ((ctx->num_pictures - 1) & ~(ctx->gop_size - 1)) + 1;

    if (ctx->rate_control_mode == VA_RC_CQP)
        ctx->bits_per_kbps = -1;
    else {
        if (ctx->bits_per_kbps == -1)
            ctx->bits_per_kbps = 2000;
    }

    return;

print_usage:
    usage(argv[0]);
err_exit:
    svcenc_exit(ctx, 1);
}

void
svcenc_update_ref_frames(struct svcenc_context *ctx, svcenc_surface *current_surface)
{
    int i;

    if (!current_surface->is_ref)
        return;

    if (current_surface->is_idr)
        ctx->num_ref_frames = 1;
    else
        ctx->num_ref_frames++;

    if (ctx->num_ref_frames > ctx->max_num_ref_frames)
        ctx->num_ref_frames = ctx->max_num_ref_frames;

    for (i = ctx->num_ref_frames - 1; i > 0; i--)
        ref_frames[i] =  ref_frames[i - 1];

    ref_frames[0] = *current_surface;
}

static int
svcenc_coding_order_to_display_order(struct svcenc_context *ctx, int coding_order)
{
    int n, m;
    int level;

    assert(coding_order > 0);

    n = (coding_order - 1) / ctx->gop_size;
    m = (coding_order - 1) % ctx->gop_size;
    level = ctx->hierarchical_levels - temporal_ids_in_bgop[m] - 1;

    return n * ctx->gop_size + gop_factors_in_bgop[m] * (int)pow(2.00, level);
}

static void
update_next_picture_info(struct svcenc_context *ctx)
{
    svcenc_surface *current_surface = NULL;
    svcenc_surface next_surface;
    int current_frame_num;

    current_surface = &ctx->next_svcenc_surface;
    current_frame_num = current_surface->frame_num;

    next_surface.coding_order = current_surface->coding_order + 1;
    next_surface.display_order = svcenc_coding_order_to_display_order(ctx, next_surface.coding_order);
    next_surface.slot_in_surfaces = (current_surface->slot_in_surfaces + 1) % NUM_SURFACES;
    next_surface.temporal_id = temporal_ids_in_bgop[current_surface->coding_order % ctx->gop_size];
    next_surface.is_ref = (next_surface.temporal_id != ctx->hierarchical_levels - 1);

    if (next_surface.temporal_id == 0) {
        if (!(next_surface.display_order % ctx->intra_period))
            next_surface.picture_type = VAEncPictureTypeIntra;
        else
            next_surface.picture_type = VAEncPictureTypePredictive;
    } else
        next_surface.picture_type = VAEncPictureTypeBidirectional;

    if (current_surface->temporal_id == ctx->hierarchical_levels - 1)
        next_surface.frame_num = current_frame_num;
    else
        next_surface.frame_num = current_frame_num + 1;

    next_surface.is_intra = (next_surface.picture_type == VAEncPictureTypeIntra);
    next_surface.is_idr = 0;
    next_surface.rec_surface = rec_surfaces[next_surface.slot_in_surfaces];
    next_surface.poc = next_surface.display_order;

    if (next_surface.is_idr)
        next_surface.frame_num = 0;

    *current_surface = next_surface;
}

static int
svcenc_coding_order_to_level_pgop(struct svcenc_context *ctx, int coding_order)
{
    int m;
    int level;

    assert(coding_order > 0);

    m = coding_order % ctx->gop_size;

    if (m == 0)
        return 0;

    level = ctx->hierarchical_levels - temporal_ids_in_pgop[m - 1];

    assert(level > 0 && level < 5);

    return level;
}

static void
update_next_picture_info_pgop(struct svcenc_context *ctx)
{
    svcenc_surface *current_surface = NULL;
    svcenc_surface next_surface;
    int current_frame_num;

    current_surface = &ctx->next_svcenc_surface;
    current_frame_num = current_surface->frame_num;

    next_surface.coding_order = current_surface->coding_order + 1;
    next_surface.display_order = next_surface.coding_order;
    next_surface.slot_in_surfaces = (current_surface->slot_in_surfaces + 1) % NUM_SURFACES;
    next_surface.temporal_id = svcenc_coding_order_to_level_pgop(ctx, next_surface.coding_order);
    next_surface.is_ref = (next_surface.temporal_id != ctx->hierarchical_levels - 1);

    if (next_surface.temporal_id == 0) {
        if (!(next_surface.display_order % ctx->intra_period))
            next_surface.picture_type = VAEncPictureTypeIntra;
        else
            next_surface.picture_type = VAEncPictureTypePredictive;
    } else {
        next_surface.picture_type = VAEncPictureTypePredictive;
    }

    next_surface.frame_num = current_frame_num + 1;
    next_surface.is_intra = (next_surface.picture_type == VAEncPictureTypeIntra);
    next_surface.is_idr = !(next_surface.display_order % ctx->intra_idr_period);
    next_surface.rec_surface = rec_surfaces[next_surface.slot_in_surfaces];
    next_surface.poc = next_surface.display_order % ctx->intra_idr_period;

    if (next_surface.is_idr) {
        next_surface.frame_num = 0;
        assert(next_surface.poc == 0);
    }

    *current_surface = next_surface;
}

static int
svcenc_seq_parameter_set_id(struct svcenc_context *ctx,
                            svcenc_surface *current_surface)
{
    return 0;
}

static int
svcenc_pic_parameter_set_id(struct svcenc_context *ctx,
                            svcenc_surface *current_surface)
{
    return 0;
}

void
svcenc_update_sequence_parameter_buffer(struct svcenc_context *ctx,
                                        svcenc_surface *current_surface)
{
    VAEncSequenceParameterBufferH264 *seq_param = &ctx->seq_param;
    VAStatus va_status;
    int width_in_mbs = (ctx->width + 15) / 16;
    int height_in_mbs = (ctx->height + 15) / 16;
    int frame_cropping_flag = 0;
    int frame_crop_bottom_offset = 0;

    memset(seq_param, 0, sizeof(*seq_param));
    seq_param->seq_parameter_set_id = svcenc_seq_parameter_set_id(ctx, current_surface);
    seq_param->level_idc = 41;
    seq_param->intra_period = ctx->intra_period;
    seq_param->ip_period = ctx->ip_period;
    seq_param->intra_idr_period = ctx->intra_idr_period;
    seq_param->max_num_ref_frames = ctx->max_num_ref_frames;
    seq_param->picture_width_in_mbs = width_in_mbs;
    seq_param->picture_height_in_mbs = height_in_mbs;
    seq_param->seq_fields.bits.frame_mbs_only_flag = 1;

    if (ctx->bits_per_kbps > 0)
        seq_param->bits_per_second = 1024 * ctx->bits_per_kbps;
    else
        seq_param->bits_per_second = 0;

    seq_param->time_scale = ctx->framerate_per_100s * 2;
    seq_param->num_units_in_tick = 100;

    if (height_in_mbs * 16 - ctx->height) {
        frame_cropping_flag = 1;
        frame_crop_bottom_offset =
            (height_in_mbs * 16 - ctx->height) / (2 * (!seq_param->seq_fields.bits.frame_mbs_only_flag + 1));
    }

    seq_param->frame_cropping_flag = frame_cropping_flag;
    seq_param->frame_crop_left_offset = 0;
    seq_param->frame_crop_right_offset = 0;
    seq_param->frame_crop_top_offset = 0;
    seq_param->frame_crop_bottom_offset = frame_crop_bottom_offset;

    seq_param->seq_fields.bits.pic_order_cnt_type = 0;

    if ((ctx->profile_idc == PROFILE_IDC_SCALABLE_HIGH &&
         ctx->constraint_set_flag & (1 << 3)) ||
        (ctx->profile_idc == PROFILE_IDC_SCALABLE_BASELINE &&
         ctx->constraint_set_flag & (1 << 5)))
        seq_param->seq_fields.bits.direct_8x8_inference_flag = 0;
    else
        seq_param->seq_fields.bits.direct_8x8_inference_flag = 1;

    seq_param->seq_fields.bits.log2_max_frame_num_minus4 = 10;
    seq_param->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 = 10;

    if (ctx->bits_per_kbps > 0)
        seq_param->vui_parameters_present_flag = 1;	//HRD info located in vui
    else
        seq_param->vui_parameters_present_flag = 0;

    va_status = vaCreateBuffer(ctx->va_dpy,
                               ctx->context_id,
                               VAEncSequenceParameterBufferType,
                               sizeof(*seq_param),
                               1,
                               seq_param,
                               &ctx->seq_param_buf_id);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");
}

void
svcenc_update_picture_parameter_buffer(struct svcenc_context *ctx,
                                       svcenc_surface *current_surface)
{
    VAEncPictureParameterBufferH264 *pic_param;
    VAStatus va_status;
    int i;

    /* Allocate the new coded buffer */
    va_status = vaCreateBuffer(ctx->va_dpy,
                               ctx->context_id,
                               VAEncCodedBufferType,
                               ctx->frame_size,
                               1,
                               NULL,
                               &ctx->codedbuf_buf_id);
    assert(ctx->codedbuf_buf_id != VA_INVALID_ID);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");

    /* H.264 picture parameter */
    pic_param = &ctx->pic_param;
    memset(pic_param, 0, sizeof(*pic_param));

    pic_param->seq_parameter_set_id = svcenc_seq_parameter_set_id(ctx, current_surface);
    pic_param->pic_parameter_set_id = svcenc_pic_parameter_set_id(ctx, current_surface);
    pic_param->last_picture = 0;
    pic_param->pic_init_qp = 34;

    pic_param->num_ref_idx_l0_active_minus1 = ctx->hierarchical_levels - 1;
    pic_param->num_ref_idx_l1_active_minus1 = ctx->hierarchical_levels - 1;

    pic_param->coded_buf = ctx->codedbuf_buf_id;
    pic_param->frame_num = current_surface->frame_num;

    pic_param->CurrPic.picture_id = current_surface->rec_surface;
    pic_param->CurrPic.TopFieldOrderCnt = current_surface->poc * 2;
    pic_param->CurrPic.BottomFieldOrderCnt = pic_param->CurrPic.TopFieldOrderCnt;
    pic_param->CurrPic.frame_idx = current_surface->frame_num;
    pic_param->CurrPic.flags = 0; /* frame */

    for (i = 0; i < ctx->num_ref_frames; i++) {
        assert(pic_param->CurrPic.picture_id != ref_frames[i].rec_surface);

        pic_param->ReferenceFrames[i].picture_id = ref_frames[i].rec_surface;
        pic_param->ReferenceFrames[i].TopFieldOrderCnt = ref_frames[i].poc * 2;
        pic_param->ReferenceFrames[i].BottomFieldOrderCnt = pic_param[i].ReferenceFrames[i].TopFieldOrderCnt;
        pic_param->ReferenceFrames[i].flags = VA_PICTURE_H264_SHORT_TERM_REFERENCE;
    }

    for (; i < 16; i++) {
        pic_param->ReferenceFrames[i].picture_id = VA_INVALID_ID;
        pic_param->ReferenceFrames[i].flags = VA_PICTURE_H264_INVALID;
    }

    pic_param->pic_fields.bits.entropy_coding_mode_flag = ENTROPY_MODE_CAVLC;
    pic_param->pic_fields.bits.weighted_pred_flag = 0;
    pic_param->pic_fields.bits.weighted_bipred_idc = 0;
    pic_param->pic_fields.bits.idr_pic_flag = !!current_surface->is_idr;
    pic_param->pic_fields.bits.reference_pic_flag = !!current_surface->is_ref;
    pic_param->pic_fields.bits.deblocking_filter_control_present_flag = 1;

    if (ctx->profile_idc == PROFILE_IDC_BASELINE ||
        ctx->profile_idc == PROFILE_IDC_MAIN ||
        ctx->profile_idc == PROFILE_IDC_SCALABLE_BASELINE)
        pic_param->pic_fields.bits.transform_8x8_mode_flag = 0;
    else
        pic_param->pic_fields.bits.transform_8x8_mode_flag = 0;

    /* Allocate the new picture parameter buffer */
    va_status = vaCreateBuffer(ctx->va_dpy,
                               ctx->context_id,
                               VAEncPictureParameterBufferType,
                               sizeof(*pic_param),
                               1,
                               pic_param,
                               &ctx->pic_param_buf_id);
    assert(ctx->pic_param_buf_id != VA_INVALID_ID);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");
}

#define partition(ref, field, key, ascending)   \
    while (i <= j) {                            \
        if (ascending) {                        \
            while (ref[i].field < key)          \
                i++;                            \
            while (ref[j].field > key)          \
                j--;                            \
        } else {                                \
            while (ref[i].field > key)          \
                i++;                            \
            while (ref[j].field < key)          \
                j--;                            \
        }                                       \
        if (i <= j) {                           \
            tmp = ref[i];                       \
            ref[i] = ref[j];                    \
            ref[j] = tmp;                       \
            i++;                                \
            j--;                                \
        }                                       \
    }                                           \

static void
sort_one(svcenc_surface ref[],
         int left, int right,
         int ascending, int is_frame_num)
{
    int i = left, j = right;
    int key;
    svcenc_surface tmp;

    if (is_frame_num) {
        key = ref[(left + right) / 2].frame_num;
        partition(ref, frame_num, key, ascending);
    } else {
        key = ref[(left + right) / 2].display_order;
        partition(ref, display_order, (signed int)key, ascending);
    }

    /* recursion */
    if (left < j)
        sort_one(ref, left, j, ascending, is_frame_num);

    if (i < right)
        sort_one(ref, i, right, ascending, is_frame_num);
}

static void
sort_two(svcenc_surface ref[],
         int left, int right,
         int key, int is_frame_num,
         int partition_ascending,
         int list0_ascending, int list1_ascending)
{
    int i = left, j = right;
    svcenc_surface tmp;

    if (is_frame_num) {
        partition(ref, frame_num, key, partition_ascending);
    } else {
        partition(ref, display_order, key, partition_ascending);
    }

    sort_one(ref, left, i - 1, list0_ascending, is_frame_num);
    sort_one(ref, j + 1, right, list1_ascending, is_frame_num);
}

static void
svcenc_update_ref_lists(struct svcenc_context *ctx,
                        svcenc_surface *current_surface)
{
    if (current_surface->picture_type == VAEncPictureTypePredictive) {
        memcpy(ref_list0, ref_frames, ctx->num_ref_frames * sizeof(svcenc_surface));
        sort_one(ref_list0, 0, ctx->num_ref_frames - 1, 0, 1);
    } else if (current_surface->picture_type == VAEncPictureTypeBidirectional) {
        memcpy(ref_list0, ref_frames, ctx->num_ref_frames * sizeof(svcenc_surface));
        sort_two(ref_list0,
                 0, ctx->num_ref_frames - 1,
                 current_surface->display_order, 0,
                 1,
                 0, 1);
        memcpy(ref_list1, ref_frames, ctx->num_ref_frames * sizeof(svcenc_surface));
        sort_two(ref_list1,
                 0, ctx->num_ref_frames - 1,
                 current_surface->display_order, 0,
                 0,
                 1, 0);
    }
}

static int
avc_temporal_find_surface(VAPictureH264 *curr_pic,
                          VAPictureH264 *ref_list,
                          int num_pictures,
                          int dir)
{
    int i, found = -1, min = 0x7FFFFFFF;

    for (i = 0; i < num_pictures; i++) {
        int tmp;

        if ((ref_list[i].flags & VA_PICTURE_H264_INVALID) ||
            (ref_list[i].picture_id == VA_INVALID_SURFACE))
            break;

        tmp = curr_pic->TopFieldOrderCnt - ref_list[i].TopFieldOrderCnt;

        if (dir)
            tmp = -tmp;

        if (tmp > 0 && tmp < min) {
            min = tmp;
            found = i;
        }
    }

    return found;
}

static int
avc_temporal_find_surface_pgop(svcenc_surface *curr_pic,
                               svcenc_surface *ref_list,
                               int num_pictures)
{
    int i, found = -1, min = 0x7FFFFFFF;

    for (i = 0; i < num_pictures; i++) {
        int tmp;

        if (ref_list[i].rec_surface == VA_INVALID_SURFACE)
            break;

        if (curr_pic->temporal_id < ref_list[i].temporal_id)
            continue;

        tmp = curr_pic->display_order - ref_list[i].display_order;

        if (tmp > 0 && tmp < min) {
            min = tmp;
            found = i;
        }
    }

    assert(found != -1);

    return found;
}

void
svcenc_update_slice_parameter_buffer(struct svcenc_context *ctx,
                                     svcenc_surface *current_surface)
{
    VAEncPictureParameterBufferH264 *pic_param = &ctx->pic_param;
    VAEncSliceParameterBufferH264 *slice_param;
    VAStatus va_status;
    int i, width_in_mbs, height_in_mbs;
    int slice_type;
    int num_ref_idx_active_override_flag = 0, num_ref_l0 = 1, num_ref_l1 = 1;
    VAPictureH264 RefPicList0[32];
    VAPictureH264 RefPicList1[32];
    VAPictureH264 *curr_pic;
    int ref_idx;

    svcenc_update_ref_lists(ctx, current_surface);

    switch (current_surface->picture_type) {
    case VAEncPictureTypeBidirectional:
        slice_type = SLICE_TYPE_B;
        break;

    case VAEncPictureTypePredictive:
        slice_type = SLICE_TYPE_P;
        break;

    case VAEncPictureTypeIntra:
    default:
        slice_type = SLICE_TYPE_I;
        break;
    }

    width_in_mbs = (ctx->width + 15) / 16;
    height_in_mbs = (ctx->height + 15) / 16;

    // Slice level
    i = 0;
    slice_param = &ctx->slice_param[i];
    memset(slice_param, 0, sizeof(*slice_param));

    slice_param->macroblock_address = 0;
    slice_param->num_macroblocks = height_in_mbs * width_in_mbs;
    slice_param->pic_parameter_set_id = svcenc_pic_parameter_set_id(ctx, current_surface);
    slice_param->slice_type = slice_type;

    slice_param->direct_spatial_mv_pred_flag = 1; /*
                                                   * It is required for the slice layer extension and the base
                                                   * layer bitstream
                                                   * See G.7.4.3 j), G.10.1.2 a), G.10.1.2.1 a)
                                                   */
    slice_param->num_ref_idx_active_override_flag = 0;
    slice_param->num_ref_idx_l0_active_minus1 = 0;
    slice_param->num_ref_idx_l1_active_minus1 = 0;
    slice_param->cabac_init_idc = 0;
    slice_param->slice_qp_delta = 0;
    slice_param->disable_deblocking_filter_idc = 0;
    slice_param->slice_alpha_c0_offset_div2 = 2;
    slice_param->slice_beta_offset_div2 = 2;
    slice_param->idr_pic_id = 0;

    ctx->reordering_info[0] = ctx->reordering_info[1] = 0;

    /* FIXME: fill other fields */
    if ((slice_type == SLICE_TYPE_P) || (slice_type == SLICE_TYPE_B)) {
	int j;
        num_ref_l0 = MIN((pic_param->num_ref_idx_l0_active_minus1 + 1), ctx->num_ref_frames);

        for (j = 0; j < num_ref_l0; j++) {
            RefPicList0[j].picture_id = ref_list0[j].rec_surface;
            RefPicList0[j].TopFieldOrderCnt = ref_list0[j].poc * 2;
            RefPicList0[j].BottomFieldOrderCnt = RefPicList0[j].TopFieldOrderCnt;
            RefPicList0[j].frame_idx = ref_list0[j].frame_num;
            RefPicList0[j].flags = VA_PICTURE_H264_SHORT_TERM_REFERENCE;
        }

	for (; j < 32; j++) {
	    RefPicList0[j].picture_id = VA_INVALID_SURFACE;
	    RefPicList0[j].flags = VA_PICTURE_H264_INVALID;
	}

        if (num_ref_l0 != pic_param->num_ref_idx_l0_active_minus1 + 1)
            num_ref_idx_active_override_flag = 1;
    }

    if ((slice_type == SLICE_TYPE_B)) {
	int j;
        num_ref_l1 = MIN((pic_param->num_ref_idx_l1_active_minus1 + 1), ctx->num_ref_frames);

        for (j = 0; j < num_ref_l1; j++) {
            RefPicList1[j].picture_id = ref_list1[j].rec_surface;
            RefPicList1[j].TopFieldOrderCnt = ref_list1[j].poc * 2;
            RefPicList1[j].BottomFieldOrderCnt = RefPicList1[j].TopFieldOrderCnt;
            RefPicList1[j].frame_idx = ref_list1[j].frame_num;
            RefPicList1[j].flags = VA_PICTURE_H264_SHORT_TERM_REFERENCE;
        }

	for (; j < 32; j++) {
	    RefPicList1[j].picture_id = VA_INVALID_SURFACE;
	    RefPicList1[j].flags = VA_PICTURE_H264_INVALID;
	}

        if (num_ref_l1 != pic_param->num_ref_idx_l1_active_minus1 + 1)
            num_ref_idx_active_override_flag = 1;
    }

    if (num_ref_l0 != 1 || num_ref_l1 != 1) {
        curr_pic = &pic_param->CurrPic;

        /* select the reference frame in temporal space */
        if ((slice_type == SLICE_TYPE_P) || (slice_type == SLICE_TYPE_B)) {
            if (ctx->gop_type == 1)
                ref_idx = avc_temporal_find_surface(curr_pic, RefPicList0, num_ref_l0, 0);
            else
                ref_idx = avc_temporal_find_surface_pgop(current_surface, ref_list0, num_ref_l0);

            if (ref_idx != 0) {
                ctx->reordering_info[0] |= (1 << 0);

                if (curr_pic->frame_idx > RefPicList0[ref_idx].frame_idx) {
                    ctx->reordering_info[0] |= (0 << 4);
                    ctx->reordering_info[0] |= ((curr_pic->frame_idx - RefPicList0[ref_idx].frame_idx - 1) << 8);
                } else {
                    ctx->reordering_info[0] |= (1 << 4);
                    ctx->reordering_info[0] |= ((RefPicList0[ref_idx].frame_idx - curr_pic->frame_idx - 1) << 8);
                }
            }

            slice_param->RefPicList0[0] = RefPicList0[ref_idx];
        }

        if (slice_type == SLICE_TYPE_B) {
            assert(ctx->gop_type == 1);
            ref_idx = avc_temporal_find_surface(curr_pic, RefPicList1, num_ref_l1, 1);

            if (ref_idx != 0) {
                ctx->reordering_info[1] |= (1 << 0);

                if (curr_pic->frame_idx > RefPicList1[ref_idx].frame_idx) {
                    ctx->reordering_info[1] |= (0 << 4);
                    ctx->reordering_info[1] |= ((curr_pic->frame_idx - RefPicList1[ref_idx].frame_idx - 1) << 8);
                } else {
                    ctx->reordering_info[1] |= (1 << 4);
                    ctx->reordering_info[1] |= ((RefPicList1[ref_idx].frame_idx - curr_pic->frame_idx - 1) << 8);
                }
            }

            slice_param->RefPicList1[0] = RefPicList1[ref_idx];
        }

        slice_param->num_ref_idx_active_override_flag = 1;
        slice_param->num_ref_idx_l0_active_minus1 = 0;
        slice_param->num_ref_idx_l1_active_minus1 = 0;
        ctx->reordering_info[0] |= ((1 << 16) | (0 << 24));
        ctx->reordering_info[1] |= ((1 << 16) | (0 << 24));
    } else {
        if (num_ref_idx_active_override_flag == 1) {
            slice_param->num_ref_idx_active_override_flag = 1;
            slice_param->num_ref_idx_l0_active_minus1 = 0;
            slice_param->num_ref_idx_l1_active_minus1 = 0;
            ctx->reordering_info[0] |= ((1 << 16) | (0 << 24));
            ctx->reordering_info[1] |= ((1 << 16) | (0 << 24));
        }

        if ((slice_type == SLICE_TYPE_P) || (slice_type == SLICE_TYPE_B)) {
            slice_param->RefPicList0[0] = RefPicList0[0];
        }

        if (slice_type == SLICE_TYPE_B) {
            slice_param->RefPicList1[0] = RefPicList1[0];
        }
    }

    va_status = vaCreateBuffer(ctx->va_dpy,
                               ctx->context_id,
                               VAEncSliceParameterBufferType,
                               sizeof(*slice_param),
                               1,
                               slice_param,
                               &ctx->slice_param_buf_id[i]);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");;

    ctx->num_slices = ++i;
}

void
svcenc_update_misc_parameter_buffer(struct svcenc_context *ctx, svcenc_surface *current_surface)
{
    VAStatus va_status;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterTemporalLayerStructure *misc_layer_structure_param;
    VAEncMiscParameterRateControl *misc_ratecontrol_param;
    VAEncMiscParameterFrameRate *misc_framerate_param;
    VAEncMiscParameterHRD *misc_hrd_param;

    if (current_surface->frame_num != 0)
        return;

    if (ctx->rate_control_mode & VA_RC_CQP)
        return;

    assert(ctx->bits_per_kbps > 0);

    if (!ctx->layer_brc) {
        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncMiscParameterBufferType,
                                   sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterRateControl),
                                   1,
                                   NULL,
                                   &ctx->misc_parameter_ratecontrol_buf_id[0]);
        CHECK_VASTATUS(va_status, "vaCreateBuffer");

        vaMapBuffer(ctx->va_dpy,
                    ctx->misc_parameter_ratecontrol_buf_id[0],
                    (void **)&misc_param);

        misc_param->type = VAEncMiscParameterTypeRateControl;
        misc_ratecontrol_param = (VAEncMiscParameterRateControl *)misc_param->data;

        misc_ratecontrol_param->bits_per_second = 1024 * ctx->bits_per_kbps;

        vaUnmapBuffer(ctx->va_dpy, ctx->misc_parameter_ratecontrol_buf_id[0]);

        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncMiscParameterBufferType,
                                   sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterFrameRate),
                                   1,
                                   NULL,
                                   &ctx->misc_parameter_framerate_buf_id[0]);
        CHECK_VASTATUS(va_status, "vaCreateBuffer");

        vaMapBuffer(ctx->va_dpy,
                    ctx->misc_parameter_framerate_buf_id[0],
                    (void **)&misc_param);

        misc_param->type = VAEncMiscParameterTypeFrameRate;
        misc_framerate_param = (VAEncMiscParameterFrameRate *)misc_param->data;
        misc_framerate_param->framerate = ((100 << 16) | ctx->framerate_per_100s);

        misc_ratecontrol_param->bits_per_second = 1024 * ctx->bits_per_kbps;

        vaUnmapBuffer(ctx->va_dpy, ctx->misc_parameter_ratecontrol_buf_id[0]);

        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncMiscParameterBufferType,
                                   sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterHRD),
                                   1,
                                   NULL,
                                   &ctx->misc_parameter_hrd_buf_id);
        CHECK_VASTATUS(va_status, "vaCreateBuffer");

        vaMapBuffer(ctx->va_dpy,
                    ctx->misc_parameter_hrd_buf_id,
                    (void **)&misc_param);

        misc_param->type = VAEncMiscParameterTypeHRD;
        misc_hrd_param = (VAEncMiscParameterHRD *)misc_param->data;

        misc_hrd_param->initial_buffer_fullness = ctx->bits_per_kbps * 1024 * 4;
        misc_hrd_param->buffer_size = ctx->bits_per_kbps * 1024 * 8;

        vaUnmapBuffer(ctx->va_dpy, ctx->misc_parameter_hrd_buf_id);
    } else {
        int i, j, nlayers = 0;

        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncMiscParameterBufferType,
                                   sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterTemporalLayerStructure),
                                   1,
                                   NULL,
                                   &ctx->misc_parameter_layer_structure_buf_id);
        CHECK_VASTATUS(va_status, "vaCreateBuffer");

        vaMapBuffer(ctx->va_dpy,
                    ctx->misc_parameter_layer_structure_buf_id,
                    (void **)&misc_param);

        misc_param->type = VAEncMiscParameterTypeTemporalLayerStructure;
        misc_layer_structure_param = (VAEncMiscParameterTemporalLayerStructure *)misc_param->data;

        misc_layer_structure_param->number_of_layers = ctx->hierarchical_levels;
        misc_layer_structure_param->periodicity = ctx->gop_size;

        for (i = 0; i < ctx->gop_size; i++) {
            if (ctx->gop_type == 1) { // B
                misc_layer_structure_param->layer_id[i] = temporal_ids_in_bgop[i];
            } else {
                misc_layer_structure_param->layer_id[i] = svcenc_coding_order_to_level_pgop(ctx, i + 1);
            }
        }

        vaUnmapBuffer(ctx->va_dpy, ctx->misc_parameter_layer_structure_buf_id);

        for (i = 1; i <= ctx->hierarchical_levels; i++) {
            nlayers += i;
        }

        for (i = 0; i < ctx->hierarchical_levels; i++) {
            int framerate_per_100s = frame_rates[i] * 100;
            int numerator = 0;

            for (j = 0; j < ctx->gop_size; j++) {
                if (misc_layer_structure_param->layer_id[j] <= i)
                    numerator++;
            }

            va_status = vaCreateBuffer(ctx->va_dpy,
                                       ctx->context_id,
                                       VAEncMiscParameterBufferType,
                                       sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterRateControl),
                                       1,
                                       NULL,
                                       &ctx->misc_parameter_ratecontrol_buf_id[i]);
            CHECK_VASTATUS(va_status, "vaCreateBuffer");

            vaMapBuffer(ctx->va_dpy,
                        ctx->misc_parameter_ratecontrol_buf_id[i],
                        (void **)&misc_param);

            misc_param->type = VAEncMiscParameterTypeRateControl;
            misc_ratecontrol_param = (VAEncMiscParameterRateControl *)misc_param->data;
            misc_ratecontrol_param->bits_per_second = 1024 * ctx->bits_per_kbps * ((float)numerator / ctx->gop_size);
            misc_ratecontrol_param->rc_flags.bits.temporal_id = i;

            vaUnmapBuffer(ctx->va_dpy, ctx->misc_parameter_ratecontrol_buf_id[i]);

            va_status = vaCreateBuffer(ctx->va_dpy,
                                       ctx->context_id,
                                       VAEncMiscParameterBufferType,
                                       sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterFrameRate),
                                       1,
                                       NULL,
                                       &ctx->misc_parameter_framerate_buf_id[i]);
            CHECK_VASTATUS(va_status, "vaCreateBuffer");

            vaMapBuffer(ctx->va_dpy,
                        ctx->misc_parameter_framerate_buf_id[i],
                        (void **)&misc_param);

            misc_param->type = VAEncMiscParameterTypeFrameRate;
            misc_framerate_param = (VAEncMiscParameterFrameRate *)misc_param->data;
            misc_framerate_param->framerate = ((100 << 16) | framerate_per_100s);
            misc_framerate_param->framerate_flags.bits.temporal_id = i;

            vaUnmapBuffer(ctx->va_dpy, ctx->misc_parameter_ratecontrol_buf_id[i]);
        }

        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncMiscParameterBufferType,
                                   sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterHRD),
                                   1,
                                   NULL,
                                   &ctx->misc_parameter_hrd_buf_id);
        CHECK_VASTATUS(va_status, "vaCreateBuffer");

        vaMapBuffer(ctx->va_dpy,
                    ctx->misc_parameter_hrd_buf_id,
                    (void **)&misc_param);

        misc_param->type = VAEncMiscParameterTypeHRD;
        misc_hrd_param = (VAEncMiscParameterHRD *)misc_param->data;

        misc_hrd_param->initial_buffer_fullness = ctx->bits_per_kbps * 1024 * 4;
        misc_hrd_param->buffer_size = ctx->bits_per_kbps * 1024 * 8;

        vaUnmapBuffer(ctx->va_dpy, ctx->misc_parameter_hrd_buf_id);
    }
}

static void
svcenc_update_packed_slice_header_buffer(struct svcenc_context *ctx, unsigned int temporal_id)
{
    VAStatus va_status;
    VAEncPackedHeaderParameterBuffer packed_header_param_buffer;
    unsigned int length_in_bits;
    unsigned char *packed_header_data_buffer = NULL;
    int i;

    for (i = 0; i < ctx->num_slices; i++) {
        length_in_bits = build_packed_svc_prefix_nal_unit(&ctx->pic_param,
                                                          temporal_id,
                                                          &packed_header_data_buffer);

        packed_header_param_buffer.type = VAEncPackedHeaderRawData;
        packed_header_param_buffer.bit_length = length_in_bits;
        packed_header_param_buffer.has_emulation_bytes = 0;

        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncPackedHeaderParameterBufferType,
                                   sizeof(packed_header_param_buffer),
                                   1,
                                   &packed_header_param_buffer,
                                   &ctx->packed_prefix_nal_unit_param_buf_id[i]);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncPackedHeaderDataBufferType,
                                   (length_in_bits + 7) / 8,
                                   1,
                                   packed_header_data_buffer,
                                   &ctx->packed_prefix_nal_unit_data_buf_id[i]);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        free(packed_header_data_buffer);

        length_in_bits = build_packed_slice_header_buffer(ctx,
                                                          &ctx->seq_param,
                                                          &ctx->pic_param,
                                                          &ctx->slice_param[i],
                                                          ctx->reordering_info,
                                                          &packed_header_data_buffer);

        packed_header_param_buffer.type = VAEncPackedHeaderSlice;
        packed_header_param_buffer.bit_length = length_in_bits;
        packed_header_param_buffer.has_emulation_bytes = 0;

        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncPackedHeaderParameterBufferType,
                                   sizeof(packed_header_param_buffer),
                                   1,
                                   &packed_header_param_buffer,
                                   &ctx->packed_slice_header_param_buf_id[i]);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncPackedHeaderDataBufferType,
                                   (length_in_bits + 7) / 8,
                                   1,
                                   packed_header_data_buffer,
                                   &ctx->packed_slice_header_data_buf_id[i]);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        free(packed_header_data_buffer);
    }
}

void
svcenc_update_packed_buffers(struct svcenc_context *ctx,
                             svcenc_surface *current_surface)
{
    VAStatus va_status;
    VAEncPackedHeaderParameterBuffer packed_header_param_buffer;
    unsigned int length_in_bits;
    unsigned char *packed_seq_buffer = NULL, *packed_pic_buffer = NULL, *packed_sei_buffer = NULL;

    length_in_bits = build_packed_sei_buffer(ctx,
                                             &ctx->seq_param,
                                             &ctx->pic_param,
                                             current_surface,
                                             &packed_sei_buffer);

    if (packed_sei_buffer) {
        assert(length_in_bits);

        packed_header_param_buffer.type = VAEncPackedHeaderH264_SEI;
        packed_header_param_buffer.bit_length = length_in_bits;
        packed_header_param_buffer.has_emulation_bytes = 0;
        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncPackedHeaderParameterBufferType,
                                   sizeof(packed_header_param_buffer),
                                   1,
                                   &packed_header_param_buffer,
                                   &ctx->packed_sei_header_param_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncPackedHeaderDataBufferType,
                                   (length_in_bits + 7) / 8,
                                   1,
                                   packed_sei_buffer,
                                   &ctx->packed_sei_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        free(packed_sei_buffer);
    }

    if (current_surface->frame_num == 0) {
        int save_profile_idc, save_constraint_set_flag;
        unsigned char *packed_svc_seq_buffer = NULL;

        length_in_bits = build_packed_seq_buffer(ctx,
                                                 &ctx->seq_param,
                                                 &packed_seq_buffer);

        packed_header_param_buffer.type = VAEncPackedHeaderH264_SPS;
        packed_header_param_buffer.bit_length = length_in_bits;
        packed_header_param_buffer.has_emulation_bytes = 0;
        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncPackedHeaderParameterBufferType,
                                   sizeof(packed_header_param_buffer),
                                   1,
                                   &packed_header_param_buffer,
                                   &ctx->packed_seq_header_param_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncPackedHeaderDataBufferType,
                                   (length_in_bits + 7) / 8,
                                   1,
                                   packed_seq_buffer,
                                   &ctx->packed_seq_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        save_profile_idc = ctx->profile_idc;
        save_constraint_set_flag = ctx->constraint_set_flag;
        ctx->profile_idc = ctx->svc_profile_idc;
        ctx->constraint_set_flag = ctx->svc_constraint_set_flag;

        length_in_bits = build_packed_subset_seq_buffer(ctx,
                                                        &ctx->seq_param,
                                                        &packed_svc_seq_buffer);
        packed_header_param_buffer.type = VAEncPackedHeaderRawData;
        packed_header_param_buffer.bit_length = length_in_bits;
        packed_header_param_buffer.has_emulation_bytes = 0;
        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncPackedHeaderParameterBufferType,
                                   sizeof(packed_header_param_buffer),
                                   1,
                                   &packed_header_param_buffer,
                                   &ctx->packed_svc_seq_header_param_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncPackedHeaderDataBufferType,
                                   (length_in_bits + 7) / 8,
                                   1,
                                   packed_svc_seq_buffer,
                                   &ctx->packed_svc_seq_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        ctx->profile_idc = save_profile_idc;
        ctx->constraint_set_flag = save_constraint_set_flag;

        free(packed_svc_seq_buffer);

        length_in_bits = build_packed_pic_buffer(ctx,
                                                 &ctx->pic_param,
                                                 &packed_pic_buffer);
        packed_header_param_buffer.type = VAEncPackedHeaderH264_PPS;
        packed_header_param_buffer.bit_length = length_in_bits;
        packed_header_param_buffer.has_emulation_bytes = 0;

        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncPackedHeaderParameterBufferType,
                                   sizeof(packed_header_param_buffer),
                                   1,
                                   &packed_header_param_buffer,
                                   &ctx->packed_pic_header_param_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncPackedHeaderDataBufferType,
                                   (length_in_bits + 7) / 8,
                                   1,
                                   packed_pic_buffer,
                                   &ctx->packed_pic_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        free(packed_seq_buffer);
        free(packed_pic_buffer);
    }

    svcenc_update_packed_slice_header_buffer(ctx, current_surface->temporal_id);
}

void
svcenc_update_profile_idc(struct svcenc_context *ctx,
                          svcenc_surface *current_surface)
{
    ctx->profile_idc = PROFILE_IDC_HIGH;
    ctx->constraint_set_flag = 0;

    switch (ctx->profile) {
    case VAProfileH264High:
        ctx->profile_idc = PROFILE_IDC_HIGH;
        ctx->constraint_set_flag |= (1 << 1);
        break;

    case VAProfileH264ConstrainedBaseline:
    case VAProfileH264Baseline:
        ctx->profile_idc = PROFILE_IDC_BASELINE;
        ctx->constraint_set_flag |= (1 << 0 |
                                     1 << 1 |
                                     1 << 2);
        break;

    default:
        /* Never get here !!! */
        assert(0);
        break;
    }

    ctx->svc_profile_idc = PROFILE_IDC_SCALABLE_HIGH;
    ctx->svc_constraint_set_flag = 0;

    switch (ctx->profile) {
    case VAProfileH264High:
        ctx->svc_profile_idc = PROFILE_IDC_SCALABLE_HIGH;
        ctx->svc_constraint_set_flag |= (1 << 1);
        break;

    case VAProfileH264ConstrainedBaseline:
        ctx->svc_profile_idc = PROFILE_IDC_SCALABLE_BASELINE;
        ctx->svc_constraint_set_flag |= (1 << 0 | 1 << 1 | 1 << 5);
        break;

    case VAProfileH264Baseline:
        ctx->svc_profile_idc = PROFILE_IDC_SCALABLE_BASELINE;
        ctx->svc_constraint_set_flag |= (1 << 0);
        break;

    default:
        /* Never get here !!! */
        assert(0);
        break;
    }
}

int
begin_picture(struct svcenc_context *ctx,
              svcenc_surface *current_surface)
{
    svcenc_update_profile_idc(ctx, current_surface);
    svcenc_update_sequence_parameter_buffer(ctx, current_surface);
    svcenc_update_picture_parameter_buffer(ctx, current_surface);
    svcenc_update_slice_parameter_buffer(ctx, current_surface);
    svcenc_update_misc_parameter_buffer(ctx, current_surface);
    svcenc_update_packed_buffers(ctx, current_surface);

    return 0;
}

static int
svcenc_store_coded_buffer(struct svcenc_context *ctx,
                          svcenc_surface *current_surface)
{
    VACodedBufferSegment *coded_buffer_segment;
    unsigned char *coded_mem;
    int slice_data_length;
    VAStatus va_status;
    size_t w_items;

    va_status = vaSyncSurface(ctx->va_dpy, src_surfaces[current_surface->slot_in_surfaces]);
    CHECK_VASTATUS(va_status,"vaSyncSurface");

    va_status = vaMapBuffer(ctx->va_dpy, ctx->codedbuf_buf_id, (void **)(&coded_buffer_segment));
    CHECK_VASTATUS(va_status,"vaMapBuffer");

    coded_mem = coded_buffer_segment->buf;

    if (coded_buffer_segment->status & VA_CODED_BUF_STATUS_SLICE_OVERFLOW_MASK) {
        /* Fatal error !!! */
        assert(0);
    }

    slice_data_length = coded_buffer_segment->size;

    do {
        w_items = fwrite(coded_mem, slice_data_length, 1, ctx->ofp);
    } while (w_items != 1);

    vaUnmapBuffer(ctx->va_dpy, ctx->codedbuf_buf_id);

    return 0;
}

int
render_picture(struct svcenc_context *ctx,
               svcenc_surface *current_surface)
{
    VAStatus va_status;
    VABufferID va_buffers[32];
    unsigned int num_va_buffers = 0;
    int i;

    va_buffers[num_va_buffers++] = ctx->seq_param_buf_id;
    va_buffers[num_va_buffers++] = ctx->pic_param_buf_id;

    if (ctx->packed_sei_scalability_info_header_param_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->packed_sei_scalability_info_header_param_buf_id;

    if (ctx->packed_sei_scalability_info_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->packed_sei_scalability_info_buf_id;

    if (ctx->packed_seq_header_param_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->packed_seq_header_param_buf_id;

    if (ctx->packed_seq_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->packed_seq_buf_id;

    if (ctx->packed_svc_seq_header_param_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->packed_svc_seq_header_param_buf_id;

    if (ctx->packed_svc_seq_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->packed_svc_seq_buf_id;

    if (ctx->packed_pic_header_param_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->packed_pic_header_param_buf_id;

    if (ctx->packed_pic_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->packed_pic_buf_id;

    if (ctx->packed_sei_header_param_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->packed_sei_header_param_buf_id;

    if (ctx->packed_sei_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->packed_sei_buf_id;

    if (ctx->misc_parameter_layer_structure_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->misc_parameter_layer_structure_buf_id;

    for (i = 0; i < MAX_LAYERS; i++) {
        if (ctx->misc_parameter_ratecontrol_buf_id[i] != VA_INVALID_ID)
            va_buffers[num_va_buffers++] = ctx->misc_parameter_ratecontrol_buf_id[i];

        if (ctx->misc_parameter_framerate_buf_id[i] != VA_INVALID_ID)
            va_buffers[num_va_buffers++] = ctx->misc_parameter_framerate_buf_id[i];
    }

    if (ctx->misc_parameter_hrd_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] =  ctx->misc_parameter_hrd_buf_id;

    va_status = vaBeginPicture(ctx->va_dpy,
                               ctx->context_id,
                               src_surfaces[current_surface->slot_in_surfaces]);
    CHECK_VASTATUS(va_status,"vaBeginPicture");

    va_status = vaRenderPicture(ctx->va_dpy,
                                ctx->context_id,
                                va_buffers,
                                num_va_buffers);
    CHECK_VASTATUS(va_status,"vaRenderPicture");

    for (i = 0; i < ctx->num_slices; i++) {
        va_status = vaRenderPicture(ctx->va_dpy,
                                    ctx->context_id,
                                    &ctx->packed_prefix_nal_unit_param_buf_id[i],
                                    1);
        CHECK_VASTATUS(va_status,"vaRenderPicture");

        va_status = vaRenderPicture(ctx->va_dpy,
                                    ctx->context_id,
                                    &ctx->packed_prefix_nal_unit_data_buf_id[i],
                                    1);
        CHECK_VASTATUS(va_status,"vaRenderPicture");

        va_status = vaRenderPicture(ctx->va_dpy,
                                    ctx->context_id,
                                    &ctx->packed_slice_header_param_buf_id[i],
                                    1);
        CHECK_VASTATUS(va_status,"vaRenderPicture");

        va_status = vaRenderPicture(ctx->va_dpy,
                                    ctx->context_id,
                                    &ctx->packed_slice_header_data_buf_id[i],
                                    1);
        CHECK_VASTATUS(va_status,"vaRenderPicture");

        va_status = vaRenderPicture(ctx->va_dpy,
                                    ctx->context_id,
                                    &ctx->slice_param_buf_id[i],
                                    1);
        CHECK_VASTATUS(va_status,"vaRenderPicture");
    }

    va_status = vaEndPicture(ctx->va_dpy, ctx->context_id);
    CHECK_VASTATUS(va_status,"vaEndPicture");

    svcenc_store_coded_buffer(ctx, current_surface);

    return 0;
}

static int
svcenc_destroy_buffers(struct svcenc_context *ctx,
                       VABufferID *va_buffers,
                       unsigned int num_va_buffers)
{
    VAStatus va_status;
    unsigned int i;

    for (i = 0; i < num_va_buffers; i++) {
        if (va_buffers[i] != VA_INVALID_ID) {
            va_status = vaDestroyBuffer(ctx->va_dpy, va_buffers[i]);
            CHECK_VASTATUS(va_status,"vaDestroyBuffer");
            va_buffers[i] = VA_INVALID_ID;
        }
    }

    return 0;
}

int
end_picture(struct svcenc_context *ctx,
            svcenc_surface *current_surface)
{
    svcenc_destroy_buffers(ctx, &ctx->seq_param_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->pic_param_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->packed_sei_scalability_info_header_param_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->packed_sei_scalability_info_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->packed_seq_header_param_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->packed_seq_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->packed_svc_seq_header_param_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->packed_svc_seq_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->packed_pic_header_param_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->packed_pic_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->slice_param_buf_id[0], ctx->num_slices);
    svcenc_destroy_buffers(ctx, &ctx->codedbuf_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->misc_parameter_layer_structure_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->misc_parameter_ratecontrol_buf_id[0], MAX_LAYERS);
    svcenc_destroy_buffers(ctx, &ctx->misc_parameter_framerate_buf_id[0], MAX_LAYERS);
    svcenc_destroy_buffers(ctx, &ctx->misc_parameter_hrd_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->packed_sei_header_param_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->packed_sei_buf_id, 1);
    svcenc_destroy_buffers(ctx, &ctx->packed_slice_header_param_buf_id[0], ctx->num_slices);
    svcenc_destroy_buffers(ctx, &ctx->packed_slice_header_data_buf_id[0], ctx->num_slices);
    svcenc_destroy_buffers(ctx, &ctx->packed_prefix_nal_unit_param_buf_id[0], ctx->num_slices);
    svcenc_destroy_buffers(ctx, &ctx->packed_prefix_nal_unit_data_buf_id[0], ctx->num_slices);

    memset(ctx->slice_param, 0, sizeof(ctx->slice_param));
    ctx->num_slices = 0;

    return 0;
}

void
encode_picture(struct svcenc_context *ctx, svcenc_surface *current_surface)
{
    begin_picture(ctx, current_surface);
    render_picture(ctx, current_surface);
    end_picture(ctx, current_surface);

    svcenc_update_ref_frames(ctx, current_surface);

    if ((current_surface->is_idr) &&
        (ctx->rate_control_mode & VA_RC_CBR)) {
        ctx->prev_idr_cpb_removal = ctx->current_idr_cpb_removal;
    }
}

static void
svcenc_va_init(struct svcenc_context *ctx)
{
    VAProfile *profile_list;
    VAEntrypoint *entrypoint_list;
    VAConfigAttrib attrib_list[4];
    VAStatus va_status;
    int max_profiles, num_profiles = 0, profile = VAProfileNone;
    int max_entrypoints, num_entrypoints = 0, entrypoint;
    int major_ver, minor_ver;
    int i, j;

    ctx->va_dpy = va_open_display();
    va_status = vaInitialize(ctx->va_dpy,
                             &major_ver,
                             &minor_ver);
    CHECK_VASTATUS(va_status, "vaInitialize");

    max_profiles = vaMaxNumProfiles(ctx->va_dpy);
    profile_list = malloc(max_profiles * sizeof(VAProfile));
    vaQueryConfigProfiles(ctx->va_dpy,
                          profile_list,
                          &num_profiles);

    for (i = 0; i < ARRAY_ELEMS(g_va_profiles); i++) {
        for (j = 0; j < num_profiles; j++) {
            if (g_va_profiles[i] == profile_list[j]) {
                profile = g_va_profiles[i];
                break;
            }
        }

        if (profile != VAProfileNone)
            break;
    }

    free(profile_list);

    if (profile == VAProfileNone) {
        fprintf(stderr, "Can't find a supported profile\n");
        exit(-1);
    }

    ctx->profile = profile;

    max_entrypoints = vaMaxNumEntrypoints(ctx->va_dpy);
    entrypoint_list = malloc(max_entrypoints * sizeof(VAEntrypoint));
    vaQueryConfigEntrypoints(ctx->va_dpy,
                             ctx->profile,
                             entrypoint_list,
                             &num_entrypoints);

    for	(entrypoint = 0; entrypoint < num_entrypoints; entrypoint++) {
        if (entrypoint_list[entrypoint] == VAEntrypointEncSlice)
            break;
    }

    free(entrypoint_list);

    if (entrypoint == num_entrypoints) {
        /* not find Slice entry point */
        fprintf(stderr, "Can't find the supported entrypoint\n");
        exit(-1);
    }

    /* find out the format for the render target, and rate control mode */
    attrib_list[0].type = VAConfigAttribRTFormat;
    attrib_list[1].type = VAConfigAttribRateControl;
    attrib_list[2].type = VAConfigAttribEncPackedHeaders;
    attrib_list[3].type = VAConfigAttribEncRateControlExt;

    vaGetConfigAttributes(ctx->va_dpy,
                          ctx->profile,
                          VAEntrypointEncSlice,
                          &attrib_list[0],
                          4);

    if ((attrib_list[0].value & VA_RT_FORMAT_YUV420) == 0) {
        /* not find desired YUV420 RT format */
        assert(0);
    }

    if ((attrib_list[1].value & ctx->rate_control_mode) == 0) {
        /* Can't find matched RC mode */
        fprintf(stderr, "RC mode %d isn't found, exit\n", ctx->rate_control_mode);
        assert(0);
    }

    if ((attrib_list[2].value & (VA_ENC_PACKED_HEADER_SEQUENCE |
                                 VA_ENC_PACKED_HEADER_PICTURE |
                                 VA_ENC_PACKED_HEADER_SLICE |
                                 VA_ENC_PACKED_HEADER_RAW_DATA)) == 0) {
        /* Can't find matched PACKED HEADER mode */
        assert(0);
    }

    if (attrib_list[3].value == VA_ATTRIB_NOT_SUPPORTED) {
        ctx->layer_brc = 0; // force to 0
    } else {
        VAConfigAttribValEncRateControlExt *val = (VAConfigAttribValEncRateControlExt *)&attrib_list[3].value;

        if (!val->bits.max_num_temporal_layers_minus1) {
            fprintf(stderr, "Do not support temporal layer\n");
            assert(0);
        }

        if (ctx->hierarchical_levels > (val->bits.max_num_temporal_layers_minus1 + 1)) {
            fprintf(stderr, "Over the maximum number of the supported layers\n");
            assert(0);
        }

        if (!val->bits.temporal_layer_bitrate_control_flag)
            ctx->layer_brc = 0; // force to 0
    }

    attrib_list[0].value = VA_RT_FORMAT_YUV420; /* set to desired RT format */
    attrib_list[1].value = ctx->rate_control_mode; /* set to desired RC mode */
    attrib_list[2].value = (VA_ENC_PACKED_HEADER_SEQUENCE |
                            VA_ENC_PACKED_HEADER_PICTURE |
                            VA_ENC_PACKED_HEADER_SLICE |
                            VA_ENC_PACKED_HEADER_RAW_DATA);

    va_status = vaCreateConfig(ctx->va_dpy,
                               ctx->profile,
                               VAEntrypointEncSlice,
                               attrib_list,
                               3,
                               &ctx->config_id);
    CHECK_VASTATUS(va_status, "vaCreateConfig");

    /* Create a context for this decode pipe */
    va_status = vaCreateContext(ctx->va_dpy,
                                ctx->config_id,
                                ctx->width,
                                ctx->height,
                                VA_PROGRESSIVE,
                                0,
                                0,
                                &ctx->context_id);
    CHECK_VASTATUS(va_status, "vaCreateContext");

    va_status = vaCreateSurfaces(ctx->va_dpy,
                                 VA_RT_FORMAT_YUV420,
                                 ctx->width,
                                 ctx->height,
                                 src_surfaces,
                                 NUM_SURFACES,
                                 NULL,
                                 0);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");

    va_status = vaCreateSurfaces(ctx->va_dpy,
                                 VA_RT_FORMAT_YUV420,
                                 ctx->width,
                                 ctx->height,
                                 rec_surfaces,
                                 NUM_SURFACES,
                                 NULL,
                                 0);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");
}

static void
svcenc_sei_init(struct svcenc_context *ctx)
{
    if (!(ctx->rate_control_mode & VA_RC_CBR))
        return;

    /* it comes for the bps defined in SPS */
    ctx->i_initial_cpb_removal_delay = 2 * 90000;
    ctx->i_initial_cpb_removal_delay_offset = 2 * 90000;

    ctx->i_cpb_removal_delay = 2;
    ctx->i_initial_cpb_removal_delay_length = 24;
    ctx->i_cpb_removal_delay_length = 24;
    ctx->i_dpb_output_delay_length = 24;
    ctx->time_offset_length = 24;

    ctx->prev_idr_cpb_removal = ctx->i_initial_cpb_removal_delay / 90000;
    ctx->current_idr_cpb_removal = ctx->prev_idr_cpb_removal;
    ctx->current_cpb_removal = 0;
    ctx->idr_frame_num = 0;
}

static void
svcenc_init(struct svcenc_context *ctx)
{
    int i;

    ctx->frame_data_buffer = (unsigned char *)malloc(ctx->frame_size);
    ctx->num_remainder_bframes = 0;
    ctx->max_num_ref_frames = ctx->gop_size;
    ctx->num_ref_frames = 0;

    switch (ctx->gop_size) {
    case 1:
        ctx->hierarchical_levels = 1;
        break;

    case 2:
        ctx->hierarchical_levels = 2;
        break;

    case 4:
        ctx->hierarchical_levels = 3;
        break;

    case 8:
        ctx->hierarchical_levels = 4;
        break;

    default:
        /* Never get here !!! */
        assert(0);
        break;
    }

    if (ctx->gop_type == 1) { // B
        ctx->intra_period = 2 * ctx->gop_size;
        ctx->ip_period = ctx->gop_size;
        ctx->intra_idr_period = 0; // 0 means infinite
    } else {
        ctx->intra_period = 2 * ctx->gop_size;
        ctx->ip_period = 1;
        ctx->intra_idr_period = 8 * ctx->intra_period;
    }

    ctx->framerate_per_100s = frame_rates[ctx->hierarchical_levels - 1] * 100;

    ctx->upload_task_header = NULL;
    ctx->upload_task_tail = NULL;

    ctx->reordering_info[0] = ctx->reordering_info[1] = 0;

    ctx->seq_param_buf_id = VA_INVALID_ID;
    ctx->pic_param_buf_id = VA_INVALID_ID;
    ctx->packed_sei_scalability_info_header_param_buf_id = VA_INVALID_ID;
    ctx->packed_sei_scalability_info_buf_id = VA_INVALID_ID;
    ctx->packed_seq_header_param_buf_id = VA_INVALID_ID;
    ctx->packed_seq_buf_id = VA_INVALID_ID;
    ctx->packed_svc_seq_header_param_buf_id = VA_INVALID_ID;
    ctx->packed_svc_seq_buf_id = VA_INVALID_ID;
    ctx->packed_pic_header_param_buf_id = VA_INVALID_ID;
    ctx->packed_pic_buf_id = VA_INVALID_ID;
    ctx->codedbuf_buf_id = VA_INVALID_ID;
    ctx->misc_parameter_layer_structure_buf_id = VA_INVALID_ID;

    for (i = 0; i < MAX_LAYERS; i++) {
        ctx->misc_parameter_ratecontrol_buf_id[i] = VA_INVALID_ID;
        ctx->misc_parameter_framerate_buf_id[i] = VA_INVALID_ID;
    }

    ctx->misc_parameter_hrd_buf_id = VA_INVALID_ID;
    ctx->packed_sei_header_param_buf_id = VA_INVALID_ID;
    ctx->packed_sei_buf_id = VA_INVALID_ID;

    for (i = 0; i < MAX_SLICES; i++) {
        ctx->packed_slice_header_param_buf_id[i] = VA_INVALID_ID;
        ctx->packed_slice_header_data_buf_id[i] = VA_INVALID_ID;
        ctx->packed_prefix_nal_unit_param_buf_id[i] = VA_INVALID_ID;
        ctx->packed_prefix_nal_unit_data_buf_id[i] = VA_INVALID_ID;
    }

    svcenc_va_init(ctx);

    /* update the next surface status */
    ctx->next_svcenc_surface.slot_in_surfaces = 0;
    ctx->next_svcenc_surface.coding_order = 0;
    ctx->next_svcenc_surface.display_order = 0;
    ctx->next_svcenc_surface.temporal_id = 0;
    ctx->next_svcenc_surface.frame_num = 0;
    ctx->next_svcenc_surface.poc = 0;
    ctx->next_svcenc_surface.picture_type = VAEncPictureTypeIntra;
    ctx->next_svcenc_surface.is_intra = 1;
    ctx->next_svcenc_surface.is_idr = 1;
    ctx->next_svcenc_surface.is_ref = 1;
    ctx->next_svcenc_surface.rec_surface = rec_surfaces[ctx->next_svcenc_surface.slot_in_surfaces];
    memset(src_surface_status, SRC_SURFACE_IN_STORAGE, sizeof(src_surface_status));

    svcenc_sei_init(ctx);
}

static void
svcenc_run(struct svcenc_context *ctx)
{
    int coding_order = 0;
    svcenc_surface current_surface;

    pthread_create(&ctx->upload_thread, NULL, upload_task_thread, ctx);
    current_surface = ctx->next_svcenc_surface;
    upload_task_queue(ctx, current_surface.display_order, current_surface.slot_in_surfaces);

    while (coding_order < ctx->num_pictures) {
        if (coding_order < (ctx->num_pictures - 1)) {
            if (ctx->gop_type == 1) {
                update_next_picture_info(ctx);
            } else {
                update_next_picture_info_pgop(ctx);
            }

            assert(current_surface.display_order != ctx->next_svcenc_surface.display_order);
            upload_task_queue(ctx, ctx->next_svcenc_surface.display_order, ctx->next_svcenc_surface.slot_in_surfaces);
        }

        while (src_surface_status[current_surface.slot_in_surfaces] != SRC_SURFACE_IN_ENCODING)
            usleep(1);

        encode_picture(ctx, &current_surface);

        if (coding_order < (ctx->num_pictures - 1))
            current_surface = ctx->next_svcenc_surface;

        coding_order++;
        fprintf(stderr, "\r %d/%d ...", coding_order, ctx->num_pictures);
        fflush(stdout);
    }

    pthread_join(ctx->upload_thread, NULL);
}

static void
svcenc_va_end(struct svcenc_context *ctx)
{
    vaDestroySurfaces(ctx->va_dpy, src_surfaces, NUM_SURFACES);
    vaDestroySurfaces(ctx->va_dpy, rec_surfaces, NUM_SURFACES);

    vaDestroyContext(ctx->va_dpy, ctx->context_id);
    vaDestroyConfig(ctx->va_dpy, ctx->config_id);
    vaTerminate(ctx->va_dpy);
    va_close_display(ctx->va_dpy);
}

static void
svcenc_end(struct svcenc_context *ctx)
{
    free(ctx->frame_data_buffer);
    ctx->frame_data_buffer = NULL;
    svcenc_va_end(ctx);
}

int
main(int argc, char *argv[])
{
    struct svcenc_context ctx;
    struct timeval tpstart, tpend;
    float timeuse;

    gettimeofday(&tpstart, NULL);

    memset(&ctx, 0, sizeof(ctx));
    parse_args(&ctx, argc, argv);

    svcenc_init(&ctx);
    svcenc_run(&ctx);
    svcenc_end(&ctx);

    gettimeofday(&tpend, NULL);
    timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
    timeuse /= 1000000;
    fprintf(stderr, "\ndone!\n");
    fprintf(stderr, "encode %d frames in %f secondes, FPS is %.1f\n", ctx.num_pictures, timeuse, ctx.num_pictures / timeuse);

    svcenc_exit(&ctx, 0);

    return 0;
}
