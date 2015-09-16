/*
 * Copyright (c) 2012 Intel Corporation. All Rights Reserved.
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
 * Simple AVC encoder based on libVA.
 *
 * Usage:
 * ./avcenc <width> <height> <input file> <output file> [qp]
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

#include <pthread.h>

#include <va/va.h>
#include <va/va_enc_h264.h>
#include "va_display.h"

#define NAL_REF_IDC_NONE        0
#define NAL_REF_IDC_LOW         1
#define NAL_REF_IDC_MEDIUM      2
#define NAL_REF_IDC_HIGH        3

#define NAL_NON_IDR             1
#define NAL_IDR                 5
#define NAL_SPS                 7
#define NAL_PPS                 8
#define NAL_SEI			6

#define SLICE_TYPE_P            0
#define SLICE_TYPE_B            1
#define SLICE_TYPE_I            2

#define FRAME_IDR 7

#define ENTROPY_MODE_CAVLC      0
#define ENTROPY_MODE_CABAC      1

#define PROFILE_IDC_BASELINE    66
#define PROFILE_IDC_MAIN        77
#define PROFILE_IDC_HIGH        100

#define CHECK_VASTATUS(va_status,func)                                  \
    if (va_status != VA_STATUS_SUCCESS) {                               \
        fprintf(stderr,"%s:%s (%d) failed,exit\n", __func__, func, __LINE__); \
        exit(1);                                                        \
    }

static VADisplay va_dpy;

static int picture_width, picture_width_in_mbs;
static int picture_height, picture_height_in_mbs;
static int frame_size;
static unsigned char *newImageBuffer = 0;

static int qp_value = 26;

static int intra_period = 30;
static int frame_bit_rate = -1;
static int frame_rate = 30;
static int ip_period = 1;

#define MAX_SLICES      32


static  unsigned int MaxFrameNum = (1<<12);
static  unsigned int Log2MaxFrameNum = 12;
static  unsigned int Log2MaxPicOrderCntLsb = 8;

static int
build_packed_pic_buffer(unsigned char **header_buffer);

static int
build_packed_seq_buffer(unsigned char **header_buffer);

static int 
build_packed_sei_pic_timing(unsigned int cpb_removal_length,
				unsigned int dpb_output_length,
				unsigned char **sei_buffer);

static int
build_packed_idr_sei_buffer_timing(unsigned int init_cpb_removal_delay_length,
				unsigned int cpb_removal_length,
				unsigned int dpb_output_length,
				unsigned char **sei_buffer);

struct upload_thread_param
{
    FILE *yuv_fp;
    VASurfaceID surface_id;
};

static void 
upload_yuv_to_surface(FILE *yuv_fp, VASurfaceID surface_id);

static struct {
    VAProfile profile;
    int constraint_set_flag;
    VAEncSequenceParameterBufferH264 seq_param;
    VAEncPictureParameterBufferH264 pic_param;
    VAEncSliceParameterBufferH264 slice_param[MAX_SLICES];
    VAContextID context_id;
    VAConfigID config_id;
    VABufferID seq_param_buf_id;                /* Sequence level parameter */
    VABufferID pic_param_buf_id;                /* Picture level parameter */
    VABufferID slice_param_buf_id[MAX_SLICES];  /* Slice level parameter, multil slices */
    VABufferID codedbuf_buf_id;                 /* Output buffer, compressed data */
    VABufferID packed_seq_header_param_buf_id;
    VABufferID packed_seq_buf_id;
    VABufferID packed_pic_header_param_buf_id;
    VABufferID packed_pic_buf_id;
    VABufferID packed_sei_header_param_buf_id;   /* the SEI buffer */
    VABufferID packed_sei_buf_id;
    VABufferID misc_parameter_hrd_buf_id;

    int num_slices;
    int codedbuf_i_size;
    int codedbuf_pb_size;
    int current_input_surface;
    int rate_control_method;
    struct upload_thread_param upload_thread_param;
    pthread_t upload_thread_id;
    int upload_thread_value;
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
} avcenc_context;

static  VAPictureH264 ReferenceFrames[16], RefPicList0[32], RefPicList1[32];

static void create_encode_pipe()
{
    VAEntrypoint entrypoints[5];
    int num_entrypoints,slice_entrypoint;
    VAConfigAttrib attrib[2];
    int major_ver, minor_ver;
    VAStatus va_status;

    va_dpy = va_open_display();
    va_status = vaInitialize(va_dpy, &major_ver, &minor_ver);
    CHECK_VASTATUS(va_status, "vaInitialize");

    vaQueryConfigEntrypoints(va_dpy, avcenc_context.profile, entrypoints, 
                             &num_entrypoints);

    for	(slice_entrypoint = 0; slice_entrypoint < num_entrypoints; slice_entrypoint++) {
        if (entrypoints[slice_entrypoint] == VAEntrypointEncSlice)
            break;
    }

    if (slice_entrypoint == num_entrypoints) {
        /* not find Slice entry point */
        assert(0);
    }

    /* find out the format for the render target, and rate control mode */
    attrib[0].type = VAConfigAttribRTFormat;
    attrib[1].type = VAConfigAttribRateControl;
    vaGetConfigAttributes(va_dpy, avcenc_context.profile, VAEntrypointEncSlice,
                          &attrib[0], 2);

    if ((attrib[0].value & VA_RT_FORMAT_YUV420) == 0) {
        /* not find desired YUV420 RT format */
        assert(0);
    }

    if ((attrib[1].value & avcenc_context.rate_control_method) == 0) {
        /* Can't find matched RC mode */
        printf("Can't find the desired RC mode, exit\n");
        assert(0);
    }

    attrib[0].value = VA_RT_FORMAT_YUV420; /* set to desired RT format */
    attrib[1].value = avcenc_context.rate_control_method; /* set to desired RC mode */

    va_status = vaCreateConfig(va_dpy, avcenc_context.profile, VAEntrypointEncSlice,
                               &attrib[0], 2,&avcenc_context.config_id);
    CHECK_VASTATUS(va_status, "vaCreateConfig");

    /* Create a context for this decode pipe */
    va_status = vaCreateContext(va_dpy, avcenc_context.config_id,
                                picture_width, picture_height,
                                VA_PROGRESSIVE, 
                                0, 0,
                                &avcenc_context.context_id);
    CHECK_VASTATUS(va_status, "vaCreateContext");
}

static void destory_encode_pipe()
{
    vaDestroyContext(va_dpy,avcenc_context.context_id);
    vaDestroyConfig(va_dpy,avcenc_context.config_id);
    vaTerminate(va_dpy);
    va_close_display(va_dpy);
}

/***************************************************
 *
 *  The encode pipe resource define 
 *
 ***************************************************/
#define SID_INPUT_PICTURE_0                     0
#define SID_INPUT_PICTURE_1                     1
#define SID_REFERENCE_PICTURE_L0                2
#define SID_REFERENCE_PICTURE_L1                3
#define SID_RECON_PICTURE                       4
#define SID_NUMBER                              SID_RECON_PICTURE + 1

#define SURFACE_NUM 16 /* 16 surfaces for reference */

static  VASurfaceID surface_ids[SID_NUMBER];
static  VASurfaceID ref_surface[SURFACE_NUM];
static  int use_slot[SURFACE_NUM];

static  unsigned long long current_frame_display = 0;
static  unsigned long long current_IDR_display = 0;

static  VAPictureH264 CurrentCurrPic;

#define current_slot (current_frame_display % SURFACE_NUM)

static int frame_number;
static unsigned long long enc_frame_number;
static int current_frame_type;
static int current_frame_num;
static unsigned int current_poc;

static  unsigned int num_ref_frames = 2;
static  unsigned int numShortTerm = 0;
/***************************************************/

static int get_free_slot()
{
    int i, index = -1;

    for (i = 0; i < SURFACE_NUM; i++) {
        if (use_slot[i] == 0) {
            index = i;
            break;
        }
    }
    if (index < 0) {
        printf("WARNING: No free slot to store the reconstructed frame \n");
        index = SURFACE_NUM - 1;
    }
    return index;
}

static void *
upload_thread_function(void *data)
{
    struct upload_thread_param *param = data;

    upload_yuv_to_surface(param->yuv_fp, param->surface_id);

    return NULL;
}

static void alloc_encode_resource(FILE *yuv_fp)
{
    VAStatus va_status;

    // Create surface
    va_status = vaCreateSurfaces(
        va_dpy,
        VA_RT_FORMAT_YUV420, picture_width, picture_height,
        surface_ids, SID_NUMBER,
        NULL, 0
    );

    CHECK_VASTATUS(va_status, "vaCreateSurfaces");

    // Create surface
    va_status = vaCreateSurfaces(
        va_dpy,
        VA_RT_FORMAT_YUV420, picture_width, picture_height,
        ref_surface, SURFACE_NUM,
        NULL, 0
    );

    CHECK_VASTATUS(va_status, "vaCreateSurfaces");


    newImageBuffer = (unsigned char *)malloc(frame_size);

    /* firstly upload YUV data to SID_INPUT_PICTURE_1 */
    avcenc_context.upload_thread_param.yuv_fp = yuv_fp;
    avcenc_context.upload_thread_param.surface_id = surface_ids[SID_INPUT_PICTURE_1];

    avcenc_context.upload_thread_value = pthread_create(&avcenc_context.upload_thread_id,
                                                        NULL,
                                                        upload_thread_function, 
                                                        (void*)&avcenc_context.upload_thread_param);
}

static void release_encode_resource()
{
    pthread_join(avcenc_context.upload_thread_id, NULL);
    free(newImageBuffer);

    // Release all the surfaces resource
    vaDestroySurfaces(va_dpy, surface_ids, SID_NUMBER);
    // Release all the reference surfaces
    vaDestroySurfaces(va_dpy, ref_surface, SURFACE_NUM);
}

static void avcenc_update_sei_param(int is_idr)
{
	VAEncPackedHeaderParameterBuffer packed_header_param_buffer;
	unsigned int length_in_bits;
	unsigned char *packed_sei_buffer = NULL;
	VAStatus va_status;

        if (is_idr)
	    length_in_bits = build_packed_idr_sei_buffer_timing(
				avcenc_context.i_initial_cpb_removal_delay_length,
				avcenc_context.i_cpb_removal_delay_length,
				avcenc_context.i_dpb_output_delay_length,
				&packed_sei_buffer);
       else
	    length_in_bits = build_packed_sei_pic_timing(
				avcenc_context.i_cpb_removal_delay_length,
				avcenc_context.i_dpb_output_delay_length,
				&packed_sei_buffer);

	packed_header_param_buffer.type = VAEncPackedHeaderH264_SEI;
	packed_header_param_buffer.bit_length = length_in_bits;
	packed_header_param_buffer.has_emulation_bytes = 0;

	va_status = vaCreateBuffer(va_dpy,
				avcenc_context.context_id,
				VAEncPackedHeaderParameterBufferType,
				sizeof(packed_header_param_buffer), 1, &packed_header_param_buffer,
				&avcenc_context.packed_sei_header_param_buf_id);
	CHECK_VASTATUS(va_status,"vaCreateBuffer");

	va_status = vaCreateBuffer(va_dpy,
				avcenc_context.context_id,
				VAEncPackedHeaderDataBufferType,
				(length_in_bits + 7) / 8, 1, packed_sei_buffer,
				&avcenc_context.packed_sei_buf_id);
	CHECK_VASTATUS(va_status,"vaCreateBuffer");
	free(packed_sei_buffer);
	return;
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

static void sort_one(VAPictureH264 ref[], int left, int right,
                     int ascending, int frame_idx)
{
    int i = left, j = right;
    unsigned int key;
    VAPictureH264 tmp;

    if (frame_idx) {
        key = ref[(left + right) / 2].frame_idx;
        partition(ref, frame_idx, key, ascending);
    } else {
        key = ref[(left + right) / 2].TopFieldOrderCnt;
        partition(ref, TopFieldOrderCnt, (signed int)key, ascending);
    }

    /* recursion */
    if (left < j)
        sort_one(ref, left, j, ascending, frame_idx);

    if (i < right)
        sort_one(ref, i, right, ascending, frame_idx);
}

static void sort_two(VAPictureH264 ref[], int left, int right, unsigned int key, unsigned int frame_idx,
                     int partition_ascending, int list0_ascending, int list1_ascending)
{
    int i = left, j = right;
    VAPictureH264 tmp;

    if (frame_idx) {
        partition(ref, frame_idx, key, partition_ascending);
    } else {
        partition(ref, TopFieldOrderCnt, (signed int)key, partition_ascending);
    }

    sort_one(ref, left, i-1, list0_ascending, frame_idx);
    sort_one(ref, j+1, right, list1_ascending, frame_idx);
}

static int update_RefPicList()
{

    if (current_frame_type == SLICE_TYPE_P) {
        memcpy(RefPicList0, ReferenceFrames, numShortTerm * sizeof(VAPictureH264));
        sort_one(RefPicList0, 0, numShortTerm-1, 0, 1);
    }

    if (current_frame_type == SLICE_TYPE_B) {
        memcpy(RefPicList0, ReferenceFrames, numShortTerm * sizeof(VAPictureH264));
        sort_two(RefPicList0, 0, numShortTerm-1, current_poc, 0,
                 1, 0, 1);

        memcpy(RefPicList1, ReferenceFrames, numShortTerm * sizeof(VAPictureH264));
        sort_two(RefPicList1, 0, numShortTerm-1, current_poc, 0,
                 0, 1, 0);
    }

    return 0;
}

static void avcenc_update_picture_parameter(int slice_type, int is_idr)
{
    VAEncPictureParameterBufferH264 *pic_param;
    VAStatus va_status;
    int recon_index;

    recon_index = get_free_slot();
    // Picture level
    pic_param = &avcenc_context.pic_param;

    pic_param->CurrPic.picture_id = ref_surface[recon_index];
    pic_param->CurrPic.frame_idx = current_frame_num;
    pic_param->CurrPic.flags = 0;

    pic_param->CurrPic.TopFieldOrderCnt = current_poc;
    pic_param->CurrPic.BottomFieldOrderCnt = pic_param->CurrPic.TopFieldOrderCnt;

    assert(avcenc_context.codedbuf_buf_id != VA_INVALID_ID);
    pic_param->coded_buf = avcenc_context.codedbuf_buf_id;
    pic_param->frame_num = current_frame_num;
    pic_param->pic_fields.bits.idr_pic_flag = !!is_idr;
    pic_param->pic_fields.bits.reference_pic_flag = (slice_type != SLICE_TYPE_B);
    CurrentCurrPic = pic_param->CurrPic;

    if (slice_type == SLICE_TYPE_P || slice_type == SLICE_TYPE_B)
	memset(pic_param->ReferenceFrames, 0xff, 16 * sizeof(VAPictureH264)); /* invalid all */

    if ((slice_type == SLICE_TYPE_P) || (slice_type == SLICE_TYPE_B)) {
	pic_param->ReferenceFrames[0] = RefPicList0[0];
    }
    if (slice_type == SLICE_TYPE_B) {
	pic_param->ReferenceFrames[1] = RefPicList1[0];
    }

    va_status = vaCreateBuffer(va_dpy,
                               avcenc_context.context_id,
                               VAEncPictureParameterBufferType,
                               sizeof(*pic_param), 1, pic_param,
                               &avcenc_context.pic_param_buf_id);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");

}

#ifndef VA_FOURCC_I420
#define VA_FOURCC_I420          0x30323449
#endif

static void upload_yuv_to_surface(FILE *yuv_fp, VASurfaceID surface_id)
{
    VAImage surface_image;
    VAStatus va_status;
    void *surface_p = NULL;
    unsigned char *y_src, *u_src, *v_src;
    unsigned char *y_dst, *u_dst, *v_dst;
    int y_size = picture_width * picture_height;
    int u_size = (picture_width >> 1) * (picture_height >> 1);
    int row, col;
    size_t n_items;

    do {
        n_items = fread(newImageBuffer, frame_size, 1, yuv_fp);
    } while (n_items != 1);

    va_status = vaDeriveImage(va_dpy, surface_id, &surface_image);
    CHECK_VASTATUS(va_status,"vaDeriveImage");

    vaMapBuffer(va_dpy, surface_image.buf, &surface_p);
    assert(VA_STATUS_SUCCESS == va_status);
        
    y_src = newImageBuffer;
    u_src = newImageBuffer + y_size; /* UV offset for NV12 */
    v_src = newImageBuffer + y_size + u_size;

    y_dst = surface_p + surface_image.offsets[0];
    u_dst = surface_p + surface_image.offsets[1]; /* UV offset for NV12 */
    v_dst = surface_p + surface_image.offsets[2];

    /* Y plane */
    for (row = 0; row < surface_image.height; row++) {
        memcpy(y_dst, y_src, surface_image.width);
        y_dst += surface_image.pitches[0];
        y_src += picture_width;
    }

    if (surface_image.format.fourcc == VA_FOURCC_NV12) { /* UV plane */
        for (row = 0; row < surface_image.height / 2; row++) {
            for (col = 0; col < surface_image.width / 2; col++) {
                u_dst[col * 2] = u_src[col];
                u_dst[col * 2 + 1] = v_src[col];
            }

            u_dst += surface_image.pitches[1];
            u_src += (picture_width / 2);
            v_src += (picture_width / 2);
        }
    } else if (surface_image.format.fourcc == VA_FOURCC_YV12 ||
               surface_image.format.fourcc == VA_FOURCC_I420) {
        const int U = surface_image.format.fourcc == VA_FOURCC_I420 ? 1 : 2;
        const int V = surface_image.format.fourcc == VA_FOURCC_I420 ? 2 : 1;

        u_dst = surface_p + surface_image.offsets[U];
        v_dst = surface_p + surface_image.offsets[V];

        for (row = 0; row < surface_image.height / 2; row++) {
            memcpy(u_dst, u_src, surface_image.width / 2);
            memcpy(v_dst, v_src, surface_image.width / 2);
            u_dst += surface_image.pitches[U];
            v_dst += surface_image.pitches[V];
            u_src += (picture_width / 2);
            v_src += (picture_width / 2);
        }
    }

    vaUnmapBuffer(va_dpy, surface_image.buf);
    vaDestroyImage(va_dpy, surface_image.image_id);
}

static void avcenc_update_slice_parameter(int slice_type)
{
    VAEncSliceParameterBufferH264 *slice_param;
    VAStatus va_status;
    int i;

    // Slice level
    i = 0;
    slice_param = &avcenc_context.slice_param[i];
    slice_param->macroblock_address = 0;
    slice_param->num_macroblocks = picture_height_in_mbs * picture_width_in_mbs; 
    slice_param->pic_parameter_set_id = 0;
    slice_param->slice_type = slice_type;
    slice_param->direct_spatial_mv_pred_flag = 0;
    slice_param->num_ref_idx_l0_active_minus1 = 0;      /* FIXME: ??? */
    slice_param->num_ref_idx_l1_active_minus1 = 0;
    slice_param->cabac_init_idc = 0;
    slice_param->slice_qp_delta = 0;
    slice_param->disable_deblocking_filter_idc = 0;
    slice_param->slice_alpha_c0_offset_div2 = 2;
    slice_param->slice_beta_offset_div2 = 2;
    slice_param->idr_pic_id = 0;

    /* FIXME: fill other fields */
    if ((slice_type == SLICE_TYPE_P) || (slice_type == SLICE_TYPE_B)) {
	memset(slice_param->RefPicList0, 0xFF, 32 * sizeof(VAPictureH264));
	slice_param->RefPicList0[0] = RefPicList0[0];
    }

    if ((slice_type == SLICE_TYPE_B)) {
	memset(slice_param->RefPicList1, 0xFF, 32 * sizeof(VAPictureH264));
	slice_param->RefPicList1[0] = RefPicList1[0];
    }

    va_status = vaCreateBuffer(va_dpy,
                               avcenc_context.context_id,
                               VAEncSliceParameterBufferType,
                               sizeof(*slice_param), 1, slice_param,
                               &avcenc_context.slice_param_buf_id[i]);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");;
    i++;

#if 0
    slice_param = &avcenc_context.slice_param[i];
    slice_param->macroblock_address = picture_height_in_mbs * picture_width_in_mbs / 2;
    slice_param->num_macroblocks = picture_height_in_mbs * picture_width_in_mbs / 2;
    slice_param->pic_parameter_set_id = 0;
    slice_param->slice_type = slice_type;
    slice_param->direct_spatial_mv_pred_flag = 0;
    slice_param->num_ref_idx_l0_active_minus1 = 0;      /* FIXME: ??? */
    slice_param->num_ref_idx_l1_active_minus1 = 0;
    slice_param->cabac_init_idc = 0;
    slice_param->slice_qp_delta = 0;
    slice_param->disable_deblocking_filter_idc = 0;
    slice_param->slice_alpha_c0_offset_div2 = 2;
    slice_param->slice_beta_offset_div2 = 2;
    slice_param->idr_pic_id = 0;

    /* FIXME: fill other fields */

    va_status = vaCreateBuffer(va_dpy,
                               avcenc_context.context_id,
                               VAEncSliceParameterBufferType,
                               sizeof(*slice_param), 1, slice_param,
                               &avcenc_context.slice_param_buf_id[i]);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");;
    i++;
#endif

    avcenc_context.num_slices = i;
}

static int update_ReferenceFrames(void)
{
    int i;
    /* B-frame is not used for reference */
    if (current_frame_type == SLICE_TYPE_B)
        return 0;

    CurrentCurrPic.flags = VA_PICTURE_H264_SHORT_TERM_REFERENCE;
    numShortTerm++;
    if (numShortTerm > num_ref_frames)
        numShortTerm = num_ref_frames;
    for (i=numShortTerm-1; i>0; i--)
        ReferenceFrames[i] = ReferenceFrames[i-1];
    ReferenceFrames[0] = CurrentCurrPic;

    if (current_frame_type != SLICE_TYPE_B)
        current_frame_num++;
    if (current_frame_num > MaxFrameNum)
        current_frame_num = 0;

    /* Update the use_slot. Only when the surface is used in reference
     * frame list, the use_slot[index] is set
     */
    for (i = 0; i < SURFACE_NUM; i++) {
        int j;
        bool found;

        found = false;
        for (j = 0; j < numShortTerm; j++) {
            if (ref_surface[i] == ReferenceFrames[j].picture_id) {
                found = true;
                break;
            }
        }
        if (found)
            use_slot[i] = 1;
        else
            use_slot[i] = 0;
    }

    return 0;
}

static int begin_picture(FILE *yuv_fp, int frame_num, int display_num, int slice_type, int is_idr)
{
    VAStatus va_status;

    if (avcenc_context.upload_thread_value != 0) {
        fprintf(stderr, "FATAL error!!!\n");
        exit(1);
    }

    pthread_join(avcenc_context.upload_thread_id, NULL);

    avcenc_context.upload_thread_value = -1;

    if (avcenc_context.current_input_surface == SID_INPUT_PICTURE_0)
        avcenc_context.current_input_surface = SID_INPUT_PICTURE_1;
    else
        avcenc_context.current_input_surface = SID_INPUT_PICTURE_0;

    if (is_idr) {
        VAEncPackedHeaderParameterBuffer packed_header_param_buffer;
        unsigned int length_in_bits;
        unsigned char *packed_seq_buffer = NULL, *packed_pic_buffer = NULL;

        assert(slice_type == SLICE_TYPE_I);
        length_in_bits = build_packed_seq_buffer(&packed_seq_buffer);
        packed_header_param_buffer.type = VAEncPackedHeaderSequence;
        packed_header_param_buffer.bit_length = length_in_bits;
        packed_header_param_buffer.has_emulation_bytes = 0;
        va_status = vaCreateBuffer(va_dpy,
                                   avcenc_context.context_id,
                                   VAEncPackedHeaderParameterBufferType,
                                   sizeof(packed_header_param_buffer), 1, &packed_header_param_buffer,
                                   &avcenc_context.packed_seq_header_param_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        va_status = vaCreateBuffer(va_dpy,
                                   avcenc_context.context_id,
                                   VAEncPackedHeaderDataBufferType,
                                   (length_in_bits + 7) / 8, 1, packed_seq_buffer,
                                   &avcenc_context.packed_seq_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        length_in_bits = build_packed_pic_buffer(&packed_pic_buffer);
        packed_header_param_buffer.type = VAEncPackedHeaderPicture;
        packed_header_param_buffer.bit_length = length_in_bits;
        packed_header_param_buffer.has_emulation_bytes = 0;

        va_status = vaCreateBuffer(va_dpy,
                                   avcenc_context.context_id,
                                   VAEncPackedHeaderParameterBufferType,
                                   sizeof(packed_header_param_buffer), 1, &packed_header_param_buffer,
                                   &avcenc_context.packed_pic_header_param_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        va_status = vaCreateBuffer(va_dpy,
                                   avcenc_context.context_id,
                                   VAEncPackedHeaderDataBufferType,
                                   (length_in_bits + 7) / 8, 1, packed_pic_buffer,
                                   &avcenc_context.packed_pic_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        free(packed_seq_buffer);
        free(packed_pic_buffer);
    }

    /* sequence parameter set */
    VAEncSequenceParameterBufferH264 *seq_param = &avcenc_context.seq_param;
    va_status = vaCreateBuffer(va_dpy,
                               avcenc_context.context_id,
                               VAEncSequenceParameterBufferType,
                               sizeof(*seq_param), 1, seq_param,
                               &avcenc_context.seq_param_buf_id);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");


    /* hrd parameter */
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterHRD *misc_hrd_param;
    vaCreateBuffer(va_dpy,
                   avcenc_context.context_id,
                   VAEncMiscParameterBufferType,
                   sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterRateControl),
                   1,
                   NULL, 
                   &avcenc_context.misc_parameter_hrd_buf_id);
    CHECK_VASTATUS(va_status, "vaCreateBuffer");

    vaMapBuffer(va_dpy,
                avcenc_context.misc_parameter_hrd_buf_id,
                (void **)&misc_param);
    misc_param->type = VAEncMiscParameterTypeHRD;
    misc_hrd_param = (VAEncMiscParameterHRD *)misc_param->data;

    if (frame_bit_rate > 0) {
        misc_hrd_param->initial_buffer_fullness = frame_bit_rate * 1000 * 4;
        misc_hrd_param->buffer_size = frame_bit_rate * 1000 * 8;
    } else {
        misc_hrd_param->initial_buffer_fullness = 0;
        misc_hrd_param->buffer_size = 0;
    }

    vaUnmapBuffer(va_dpy, avcenc_context.misc_parameter_hrd_buf_id);

    return 0;
}

int avcenc_render_picture()
{
    VAStatus va_status;
    VABufferID va_buffers[10];
    unsigned int num_va_buffers = 0;
    int i;

    va_buffers[num_va_buffers++] = avcenc_context.seq_param_buf_id;
    va_buffers[num_va_buffers++] = avcenc_context.pic_param_buf_id;

    if (avcenc_context.packed_seq_header_param_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = avcenc_context.packed_seq_header_param_buf_id;

    if (avcenc_context.packed_seq_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = avcenc_context.packed_seq_buf_id;

    if (avcenc_context.packed_pic_header_param_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = avcenc_context.packed_pic_header_param_buf_id;

    if (avcenc_context.packed_pic_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = avcenc_context.packed_pic_buf_id;

    if (avcenc_context.packed_sei_header_param_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = avcenc_context.packed_sei_header_param_buf_id;

    if (avcenc_context.packed_sei_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = avcenc_context.packed_sei_buf_id;

    if (avcenc_context.misc_parameter_hrd_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] =  avcenc_context.misc_parameter_hrd_buf_id;

    va_status = vaBeginPicture(va_dpy,
                               avcenc_context.context_id,
                               surface_ids[avcenc_context.current_input_surface]);
    CHECK_VASTATUS(va_status,"vaBeginPicture");
    
    va_status = vaRenderPicture(va_dpy,
                                avcenc_context.context_id,
                                va_buffers,
                                num_va_buffers);
    CHECK_VASTATUS(va_status,"vaRenderPicture");
    
    for(i = 0; i < avcenc_context.num_slices; i++) {
        va_status = vaRenderPicture(va_dpy,
                                avcenc_context.context_id,
                                &avcenc_context.slice_param_buf_id[i],
                                1);
        CHECK_VASTATUS(va_status,"vaRenderPicture");
    }

    va_status = vaEndPicture(va_dpy, avcenc_context.context_id);
    CHECK_VASTATUS(va_status,"vaEndPicture");

    return 0;
}

static int avcenc_destroy_buffers(VABufferID *va_buffers, unsigned int num_va_buffers)
{
    VAStatus va_status;
    unsigned int i;

    for (i = 0; i < num_va_buffers; i++) {
        if (va_buffers[i] != VA_INVALID_ID) {
            va_status = vaDestroyBuffer(va_dpy, va_buffers[i]);
            CHECK_VASTATUS(va_status,"vaDestroyBuffer");
            va_buffers[i] = VA_INVALID_ID;
        }
    }

    return 0;
}

static void end_picture()
{

    update_ReferenceFrames();
    avcenc_destroy_buffers(&avcenc_context.seq_param_buf_id, 1);
    avcenc_destroy_buffers(&avcenc_context.pic_param_buf_id, 1);
    avcenc_destroy_buffers(&avcenc_context.packed_seq_header_param_buf_id, 1);
    avcenc_destroy_buffers(&avcenc_context.packed_seq_buf_id, 1);
    avcenc_destroy_buffers(&avcenc_context.packed_pic_header_param_buf_id, 1);
    avcenc_destroy_buffers(&avcenc_context.packed_pic_buf_id, 1);
    avcenc_destroy_buffers(&avcenc_context.packed_sei_header_param_buf_id, 1);
    avcenc_destroy_buffers(&avcenc_context.packed_sei_buf_id, 1);
    avcenc_destroy_buffers(&avcenc_context.slice_param_buf_id[0], avcenc_context.num_slices);
    avcenc_destroy_buffers(&avcenc_context.codedbuf_buf_id, 1);
    avcenc_destroy_buffers(&avcenc_context.misc_parameter_hrd_buf_id, 1);

    memset(avcenc_context.slice_param, 0, sizeof(avcenc_context.slice_param));
    avcenc_context.num_slices = 0;
}

#define BITSTREAM_ALLOCATE_STEPPING     4096

struct __bitstream {
    unsigned int *buffer;
    int bit_offset;
    int max_size_in_dword;
};

typedef struct __bitstream bitstream;

#if 0
static int 
get_coded_bitsteam_length(unsigned char *buffer, int buffer_length)
{
    int i;

    for (i = 0; i < buffer_length - 3; i++) {
        if (!buffer[i] &&
            !buffer[i + 1] &&
            !buffer[i + 2] &&
            !buffer[i + 3])
            break;
    }

    return i;
}
#endif

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

static void nal_start_code_prefix(bitstream *bs)
{
    bitstream_put_ui(bs, 0x00000001, 32);
}

static void nal_header(bitstream *bs, int nal_ref_idc, int nal_unit_type)
{
    bitstream_put_ui(bs, 0, 1);                /* forbidden_zero_bit: 0 */
    bitstream_put_ui(bs, nal_ref_idc, 2);
    bitstream_put_ui(bs, nal_unit_type, 5);
}

static void sps_rbsp(bitstream *bs)
{
    VAEncSequenceParameterBufferH264 *seq_param = &avcenc_context.seq_param;
    int profile_idc = PROFILE_IDC_BASELINE;

    if (avcenc_context.profile == VAProfileH264High)
        profile_idc = PROFILE_IDC_HIGH;
    else if (avcenc_context.profile == VAProfileH264Main)
        profile_idc = PROFILE_IDC_MAIN;

    bitstream_put_ui(bs, profile_idc, 8);               /* profile_idc */
    bitstream_put_ui(bs, !!(avcenc_context.constraint_set_flag & 1), 1);                         /* constraint_set0_flag */
    bitstream_put_ui(bs, !!(avcenc_context.constraint_set_flag & 2), 1);                         /* constraint_set1_flag */
    bitstream_put_ui(bs, !!(avcenc_context.constraint_set_flag & 4), 1);                         /* constraint_set2_flag */
    bitstream_put_ui(bs, !!(avcenc_context.constraint_set_flag & 8), 1);                         /* constraint_set3_flag */
    bitstream_put_ui(bs, 0, 4);                         /* reserved_zero_4bits */
    bitstream_put_ui(bs, seq_param->level_idc, 8);      /* level_idc */
    bitstream_put_ue(bs, seq_param->seq_parameter_set_id);      /* seq_parameter_set_id */

    if ( profile_idc == PROFILE_IDC_HIGH) {
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
    
    if ( frame_bit_rate < 0 ) {
        bitstream_put_ui(bs, 0, 1); /* vui_parameters_present_flag */
    } else {
        bitstream_put_ui(bs, 1, 1); /* vui_parameters_present_flag */
        bitstream_put_ui(bs, 0, 1); /* aspect_ratio_info_present_flag */
        bitstream_put_ui(bs, 0, 1); /* overscan_info_present_flag */
        bitstream_put_ui(bs, 0, 1); /* video_signal_type_present_flag */
        bitstream_put_ui(bs, 0, 1); /* chroma_loc_info_present_flag */
        bitstream_put_ui(bs, 1, 1); /* timing_info_present_flag */
        {
            bitstream_put_ui(bs, 1, 32);
            bitstream_put_ui(bs, frame_rate * 2, 32);
            bitstream_put_ui(bs, 1, 1);
        }
        bitstream_put_ui(bs, 1, 1); /* nal_hrd_parameters_present_flag */
        {
            // hrd_parameters 
            bitstream_put_ue(bs, 0);    /* cpb_cnt_minus1 */
            bitstream_put_ui(bs, 0, 4); /* bit_rate_scale */
            bitstream_put_ui(bs, 2, 4); /* cpb_size_scale */
           
	    /* the frame_bit_rate is in kbps */
            bitstream_put_ue(bs, (((frame_bit_rate * 1000)>> 6) - 1)); /* bit_rate_value_minus1[0] */
            bitstream_put_ue(bs, ((frame_bit_rate * 8000) >> 6) - 1); /* cpb_size_value_minus1[0] */
            bitstream_put_ui(bs, 1, 1);  /* cbr_flag[0] */

            /* initial_cpb_removal_delay_length_minus1 */
            bitstream_put_ui(bs,
                (avcenc_context.i_initial_cpb_removal_delay_length - 1), 5);
            /* cpb_removal_delay_length_minus1 */
            bitstream_put_ui(bs,
                (avcenc_context.i_cpb_removal_delay_length - 1), 5);
            /* dpb_output_delay_length_minus1 */
            bitstream_put_ui(bs,
                (avcenc_context.i_dpb_output_delay_length - 1), 5);
            /* time_offset_length  */
            bitstream_put_ui(bs,
                (avcenc_context.time_offset_length - 1), 5);
        }
        bitstream_put_ui(bs, 0, 1);   /* vcl_hrd_parameters_present_flag */
        bitstream_put_ui(bs, 0, 1);   /* low_delay_hrd_flag */ 

        bitstream_put_ui(bs, 0, 1); /* pic_struct_present_flag */
        bitstream_put_ui(bs, 0, 1); /* bitstream_restriction_flag */
    }

    rbsp_trailing_bits(bs);     /* rbsp_trailing_bits */
}

#if 0
static void build_nal_sps(FILE *avc_fp)
{
    bitstream bs;

    bitstream_start(&bs);
    nal_start_code_prefix(&bs);
    nal_header(&bs, NAL_REF_IDC_HIGH, NAL_SPS);
    sps_rbsp(&bs);
    bitstream_end(&bs, avc_fp);
}
#endif

static void pps_rbsp(bitstream *bs)
{
    VAEncPictureParameterBufferH264 *pic_param = &avcenc_context.pic_param;

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

#if 0
static void build_nal_pps(FILE *avc_fp)
{
    bitstream bs;

    bitstream_start(&bs);
    nal_start_code_prefix(&bs);
    nal_header(&bs, NAL_REF_IDC_HIGH, NAL_PPS);
    pps_rbsp(&bs);
    bitstream_end(&bs, avc_fp);
}

static void 
build_header(FILE *avc_fp)
{
    build_nal_sps(avc_fp);
    build_nal_pps(avc_fp);
}
#endif

static int
build_packed_pic_buffer(unsigned char **header_buffer)
{
    bitstream bs;

    bitstream_start(&bs);
    nal_start_code_prefix(&bs);
    nal_header(&bs, NAL_REF_IDC_HIGH, NAL_PPS);
    pps_rbsp(&bs);
    bitstream_end(&bs);

    *header_buffer = (unsigned char *)bs.buffer;
    return bs.bit_offset;
}

static int
build_packed_seq_buffer(unsigned char **header_buffer)
{
    bitstream bs;

    bitstream_start(&bs);
    nal_start_code_prefix(&bs);
    nal_header(&bs, NAL_REF_IDC_HIGH, NAL_SPS);
    sps_rbsp(&bs);
    bitstream_end(&bs);

    *header_buffer = (unsigned char *)bs.buffer;
    return bs.bit_offset;
}

static int 
build_packed_idr_sei_buffer_timing(unsigned int init_cpb_removal_delay_length,
				unsigned int cpb_removal_length,
				unsigned int dpb_output_length,
				unsigned char **sei_buffer)
{
    unsigned char *byte_buf;
    int bp_byte_size, i, pic_byte_size;
    unsigned int cpb_removal_delay;

    bitstream nal_bs;
    bitstream sei_bp_bs, sei_pic_bs;

    bitstream_start(&sei_bp_bs);
    bitstream_put_ue(&sei_bp_bs, 0);       /*seq_parameter_set_id*/
    /* SEI buffer period info */
    /* NALHrdBpPresentFlag == 1 */
    bitstream_put_ui(&sei_bp_bs, avcenc_context.i_initial_cpb_removal_delay,
                     init_cpb_removal_delay_length);
    bitstream_put_ui(&sei_bp_bs, avcenc_context.i_initial_cpb_removal_delay_offset,
                     init_cpb_removal_delay_length);
    if ( sei_bp_bs.bit_offset & 0x7) {
        bitstream_put_ui(&sei_bp_bs, 1, 1);
    }
    bitstream_end(&sei_bp_bs);
    bp_byte_size = (sei_bp_bs.bit_offset + 7) / 8;
    
    /* SEI pic timing info */
    bitstream_start(&sei_pic_bs);
    /* The info of CPB and DPB delay is controlled by CpbDpbDelaysPresentFlag,
     * which is derived as 1 if one of the following conditions is true:
     * nal_hrd_parameters_present_flag is present in the bitstream and is equal to 1,
     * vcl_hrd_parameters_present_flag is present in the bitstream and is equal to 1,
     */
    cpb_removal_delay = (avcenc_context.current_cpb_removal - avcenc_context.prev_idr_cpb_removal);
    bitstream_put_ui(&sei_pic_bs, cpb_removal_delay, cpb_removal_length); 
    bitstream_put_ui(&sei_pic_bs, avcenc_context.current_dpb_removal_delta,
                     dpb_output_length);
    if ( sei_pic_bs.bit_offset & 0x7) {
        bitstream_put_ui(&sei_pic_bs, 1, 1);
    }
    /* The pic_structure_present_flag determines whether the pic_structure
     * info is written into the SEI pic timing info.
     * Currently it is set to zero.
     */
    bitstream_end(&sei_pic_bs);
    pic_byte_size = (sei_pic_bs.bit_offset + 7) / 8;
    
    bitstream_start(&nal_bs);
    nal_start_code_prefix(&nal_bs);
    nal_header(&nal_bs, NAL_REF_IDC_NONE, NAL_SEI);

	/* Write the SEI buffer period data */    
    bitstream_put_ui(&nal_bs, 0, 8);
    bitstream_put_ui(&nal_bs, bp_byte_size, 8);
    
    byte_buf = (unsigned char *)sei_bp_bs.buffer;
    for(i = 0; i < bp_byte_size; i++) {
        bitstream_put_ui(&nal_bs, byte_buf[i], 8);
    }
    free(byte_buf);
	/* write the SEI pic timing data */
    bitstream_put_ui(&nal_bs, 0x01, 8);
    bitstream_put_ui(&nal_bs, pic_byte_size, 8);
    
    byte_buf = (unsigned char *)sei_pic_bs.buffer;
    for(i = 0; i < pic_byte_size; i++) {
        bitstream_put_ui(&nal_bs, byte_buf[i], 8);
    }
    free(byte_buf);

    rbsp_trailing_bits(&nal_bs);
    bitstream_end(&nal_bs);

    *sei_buffer = (unsigned char *)nal_bs.buffer; 
   
    return nal_bs.bit_offset;
}

static int
build_packed_sei_pic_timing(unsigned int cpb_removal_length,
				unsigned int dpb_output_length,
				unsigned char **sei_buffer)
{
    unsigned char *byte_buf;
    int i, pic_byte_size;
    unsigned int cpb_removal_delay;

    bitstream nal_bs;
    bitstream sei_pic_bs;

    bitstream_start(&sei_pic_bs);
    /* The info of CPB and DPB delay is controlled by CpbDpbDelaysPresentFlag,
     * which is derived as 1 if one of the following conditions is true:
     * nal_hrd_parameters_present_flag is present in the bitstream and is equal to 1,
     * vcl_hrd_parameters_present_flag is present in the bitstream and is equal to 1,
     */
    cpb_removal_delay = (avcenc_context.current_cpb_removal - avcenc_context.current_idr_cpb_removal);
    bitstream_put_ui(&sei_pic_bs, cpb_removal_delay, cpb_removal_length);
    bitstream_put_ui(&sei_pic_bs, avcenc_context.current_dpb_removal_delta,
                     dpb_output_length);
    if ( sei_pic_bs.bit_offset & 0x7) {
        bitstream_put_ui(&sei_pic_bs, 1, 1);
    }

    /* The pic_structure_present_flag determines whether the pic_structure
     * info is written into the SEI pic timing info.
     * Currently it is set to zero.
     */
    bitstream_end(&sei_pic_bs);
    pic_byte_size = (sei_pic_bs.bit_offset + 7) / 8;

    bitstream_start(&nal_bs);
    nal_start_code_prefix(&nal_bs);
    nal_header(&nal_bs, NAL_REF_IDC_NONE, NAL_SEI);

	/* write the SEI Pic timing data */
    bitstream_put_ui(&nal_bs, 0x01, 8);
    bitstream_put_ui(&nal_bs, pic_byte_size, 8);

    byte_buf = (unsigned char *)sei_pic_bs.buffer;
    for(i = 0; i < pic_byte_size; i++) {
        bitstream_put_ui(&nal_bs, byte_buf[i], 8);
    }
    free(byte_buf);

    rbsp_trailing_bits(&nal_bs);
    bitstream_end(&nal_bs);

    *sei_buffer = (unsigned char *)nal_bs.buffer;

    return nal_bs.bit_offset;
}

#if 0
static void 
slice_header(bitstream *bs, int frame_num, int display_frame, int slice_type, int nal_ref_idc, int is_idr)
{
    VAEncSequenceParameterBufferH264 *seq_param = &avcenc_context.seq_param;
    VAEncPictureParameterBufferH264 *pic_param = &avcenc_context.pic_param;
    int is_cabac = (pic_param->pic_fields.bits.entropy_coding_mode_flag == ENTROPY_MODE_CABAC);

    bitstream_put_ue(bs, 0);                   /* first_mb_in_slice: 0 */
    bitstream_put_ue(bs, slice_type);          /* slice_type */
    bitstream_put_ue(bs, 0);                   /* pic_parameter_set_id: 0 */
    bitstream_put_ui(bs, frame_num & 0x0F, seq_param->seq_fields.bits.log2_max_frame_num_minus4 + 4);    /* frame_num */

    /* frame_mbs_only_flag == 1 */
    if (!seq_param->seq_fields.bits.frame_mbs_only_flag) {
        /* FIXME: */
        assert(0);
    }

    if (is_idr)
        bitstream_put_ue(bs, 0);		/* idr_pic_id: 0 */

    if (seq_param->seq_fields.bits.pic_order_cnt_type == 0) {
        bitstream_put_ui(bs, (display_frame*2) & 0x3F, seq_param->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 + 4);
        /* only support frame */
    } else {
        /* FIXME: */
        assert(0);
    }

    /* redundant_pic_cnt_present_flag == 0 */
    
    /* slice type */
    if (slice_type == SLICE_TYPE_P) {
        bitstream_put_ui(bs, 0, 1);            /* num_ref_idx_active_override_flag: 0 */
        /* ref_pic_list_reordering */
        bitstream_put_ui(bs, 0, 1);            /* ref_pic_list_reordering_flag_l0: 0 */
    } else if (slice_type == SLICE_TYPE_B) {
        bitstream_put_ui(bs, 1, 1);            /* direct_spatial_mv_pred: 1 */
        bitstream_put_ui(bs, 0, 1);            /* num_ref_idx_active_override_flag: 0 */
        /* ref_pic_list_reordering */
        bitstream_put_ui(bs, 0, 1);            /* ref_pic_list_reordering_flag_l0: 0 */
        bitstream_put_ui(bs, 0, 1);            /* ref_pic_list_reordering_flag_l1: 0 */
    } 

    /* weighted_pred_flag == 0 */

    /* dec_ref_pic_marking */
    if (nal_ref_idc != 0) {
        if ( is_idr) {
            bitstream_put_ui(bs, 0, 1);            /* no_output_of_prior_pics_flag: 0 */
            bitstream_put_ui(bs, 0, 1);            /* long_term_reference_flag: 0 */
        } else {
            bitstream_put_ui(bs, 0, 1);            /* adaptive_ref_pic_marking_mode_flag: 0 */
        }
    }

    if (is_cabac && (slice_type != SLICE_TYPE_I))
        bitstream_put_ue(bs, 0);               /* cabac_init_idc: 0 */

    bitstream_put_se(bs, 0);                   /* slice_qp_delta: 0 */

    if (pic_param->pic_fields.bits.deblocking_filter_control_present_flag == 1) {
        bitstream_put_ue(bs, 0);               /* disable_deblocking_filter_idc: 0 */
        bitstream_put_se(bs, 2);               /* slice_alpha_c0_offset_div2: 2 */
        bitstream_put_se(bs, 2);               /* slice_beta_offset_div2: 2 */
    }
}

static void 
slice_data(bitstream *bs)
{
    VACodedBufferSegment *coded_buffer_segment;
    unsigned char *coded_mem;
    int i, slice_data_length;
    VAStatus va_status;
    VASurfaceStatus surface_status;

    va_status = vaSyncSurface(va_dpy, surface_ids[avcenc_context.current_input_surface]);
    CHECK_VASTATUS(va_status,"vaSyncSurface");

    surface_status = 0;
    va_status = vaQuerySurfaceStatus(va_dpy, surface_ids[avcenc_context.current_input_surface], &surface_status);
    CHECK_VASTATUS(va_status,"vaQuerySurfaceStatus");

    va_status = vaMapBuffer(va_dpy, avcenc_context.codedbuf_buf_id, (void **)(&coded_buffer_segment));
    CHECK_VASTATUS(va_status,"vaMapBuffer");
    coded_mem = coded_buffer_segment->buf;

    slice_data_length = get_coded_bitsteam_length(coded_mem, codedbuf_size);

    for (i = 0; i < slice_data_length; i++) {
        bitstream_put_ui(bs, *coded_mem, 8);
        coded_mem++;
    }

    vaUnmapBuffer(va_dpy, avcenc_context.codedbuf_buf_id);
}

static void 
build_nal_slice(FILE *avc_fp, int frame_num, int display_frame, int slice_type, int is_idr)
{
    bitstream bs;

    bitstream_start(&bs);
    slice_data(&bs);
    bitstream_end(&bs, avc_fp);
}

#endif

static int
store_coded_buffer(FILE *avc_fp, int slice_type)
{
    VACodedBufferSegment *coded_buffer_segment;
    unsigned char *coded_mem;
    int slice_data_length;
    VAStatus va_status;
    VASurfaceStatus surface_status;
    size_t w_items;

    va_status = vaSyncSurface(va_dpy, surface_ids[avcenc_context.current_input_surface]);
    CHECK_VASTATUS(va_status,"vaSyncSurface");

    surface_status = 0;
    va_status = vaQuerySurfaceStatus(va_dpy, surface_ids[avcenc_context.current_input_surface], &surface_status);
    CHECK_VASTATUS(va_status,"vaQuerySurfaceStatus");

    va_status = vaMapBuffer(va_dpy, avcenc_context.codedbuf_buf_id, (void **)(&coded_buffer_segment));
    CHECK_VASTATUS(va_status,"vaMapBuffer");
    coded_mem = coded_buffer_segment->buf;

    if (coded_buffer_segment->status & VA_CODED_BUF_STATUS_SLICE_OVERFLOW_MASK) {
        if (slice_type == SLICE_TYPE_I)
            avcenc_context.codedbuf_i_size *= 2;
        else
            avcenc_context.codedbuf_pb_size *= 2;

        vaUnmapBuffer(va_dpy, avcenc_context.codedbuf_buf_id);
        return -1;
    }

    slice_data_length = coded_buffer_segment->size;

    do {
        w_items = fwrite(coded_mem, slice_data_length, 1, avc_fp);
    } while (w_items != 1);

    vaUnmapBuffer(va_dpy, avcenc_context.codedbuf_buf_id);

    return 0;
}

/*
 * It is from the h264encode.c but it simplifies something.
 * For example: When one frame is encoded as I-frame under the scenario with
 * P-B frames, it will be regarded as IDR frame(key-frame) and then new GOP is
 * started. If the video clip is encoded as all I-frames, the first frame
 * is regarded as IDR and the remaining is regarded as I-frame.
 *
 */

static void encoding2display_order(
    unsigned long long encoding_order,int gop_size,
    int ip_period,
    unsigned long long *displaying_order,
    int *frame_type)
{
    int encoding_order_gop = 0;

    /* When ip_period is 0, all are I/IDR frames */
    if (ip_period == 0) { /* all are I/IDR frames */
        if (encoding_order == 0)
            *frame_type = FRAME_IDR;
        else
            *frame_type = SLICE_TYPE_I;

        *displaying_order = encoding_order;
        return;
    }

    /* new sequence like
     * IDR PPPPP IDRPPPPP
     * IDR (PBB)(PBB)(PBB)(PBB) IDR (PBB)(PBB)(PBB)(PBB)
     */
    encoding_order_gop = encoding_order % gop_size;

    if (encoding_order_gop == 0) { /* the first frame */
        *frame_type = FRAME_IDR;
        *displaying_order = encoding_order;
    } else {
        int gop_delta;

        gop_delta = 1;

        if ((ip_period != 1) && ((gop_size - 1) % ip_period)) {
            int ipb_size;
            ipb_size = (gop_size - 1) / ip_period * ip_period + 1;
            if (encoding_order_gop >= ipb_size) {
                gop_delta = ipb_size;
                ip_period = gop_size - ipb_size;
            }
        }

        if (((encoding_order_gop - gop_delta) % ip_period) == 0) { /* P frames */
            *frame_type = SLICE_TYPE_P;
	    *displaying_order = encoding_order + ip_period - 1;
        } else {
	    *frame_type = SLICE_TYPE_B;
	    *displaying_order = encoding_order - 1;
        }
    }
}


static void
encode_picture(FILE *yuv_fp, FILE *avc_fp,
               int frame_num, int display_num,
               int is_idr,
               int slice_type, int next_is_bpic,
               int next_display_num)
{
    VAStatus va_status;
    int ret = 0, codedbuf_size;
    
    begin_picture(yuv_fp, frame_num, display_num, slice_type, is_idr);

    //if (next_display_num < frame_number) {
    if (1) {
        int index;

        /* prepare for next frame */
        if (avcenc_context.current_input_surface == SID_INPUT_PICTURE_0)
            index = SID_INPUT_PICTURE_1;
        else
            index = SID_INPUT_PICTURE_0;
        if ( next_display_num >= frame_number )
            next_display_num = frame_number - 1;
        fseeko(yuv_fp, (off_t)frame_size * next_display_num, SEEK_SET);

        avcenc_context.upload_thread_param.yuv_fp = yuv_fp;
        avcenc_context.upload_thread_param.surface_id = surface_ids[index];

        avcenc_context.upload_thread_value = pthread_create(&avcenc_context.upload_thread_id,
                                                            NULL,
                                                            upload_thread_function, 
                                                            (void*)&avcenc_context.upload_thread_param);
    }

    do {
        avcenc_destroy_buffers(&avcenc_context.codedbuf_buf_id, 1);
        avcenc_destroy_buffers(&avcenc_context.pic_param_buf_id, 1);


        if (SLICE_TYPE_I == slice_type) {
            codedbuf_size = avcenc_context.codedbuf_i_size;
        } else {
            codedbuf_size = avcenc_context.codedbuf_pb_size;
        }

        /* coded buffer */
        va_status = vaCreateBuffer(va_dpy,
                                   avcenc_context.context_id,
                                   VAEncCodedBufferType,
                                   codedbuf_size, 1, NULL,
                                   &avcenc_context.codedbuf_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        /* Update the RefPicList */
        update_RefPicList();

        /* picture parameter set */
        avcenc_update_picture_parameter(slice_type, is_idr);

        /* slice parameter */
        avcenc_update_slice_parameter(slice_type);

	if (avcenc_context.rate_control_method == VA_RC_CBR)
		avcenc_update_sei_param(is_idr);

        avcenc_render_picture();

        ret = store_coded_buffer(avc_fp, slice_type);
    } while (ret);

    end_picture(slice_type, next_is_bpic);
}

static void show_help()
{
    printf("Usage: avnenc <width> <height> <input_yuvfile> <output_avcfile> [qp=qpvalue|fb=framebitrate] [mode=0(I frames only)/1(I and P frames)/2(I, P and B frames)\n");
}

static void avcenc_context_seq_param_init(VAEncSequenceParameterBufferH264 *seq_param,
                                          int width, int height)

{
    int width_in_mbs = (width + 15) / 16;
    int height_in_mbs = (height + 15) / 16;
    int frame_cropping_flag = 0;
    int frame_crop_bottom_offset = 0;

    seq_param->seq_parameter_set_id = 0;
    seq_param->level_idc = 41;
    seq_param->intra_period = intra_period;
    seq_param->intra_idr_period = seq_param->intra_period;
    seq_param->ip_period = ip_period;
    seq_param->max_num_ref_frames = 4;
    seq_param->picture_width_in_mbs = width_in_mbs;
    seq_param->picture_height_in_mbs = height_in_mbs;
    seq_param->seq_fields.bits.frame_mbs_only_flag = 1;
    seq_param->seq_fields.bits.chroma_format_idc = 1;

    
    if (frame_bit_rate > 0)
        seq_param->bits_per_second = 1000 * frame_bit_rate; /* use kbps as input */
    else
        seq_param->bits_per_second = 0;
    
    seq_param->time_scale = frame_rate * 2;
    seq_param->num_units_in_tick = 1;			/* Tc = num_units_in_tick / time_sacle */

    if (height_in_mbs * 16 - height) {
        frame_cropping_flag = 1;
        frame_crop_bottom_offset = 
            (height_in_mbs * 16 - height) / (2 * (!seq_param->seq_fields.bits.frame_mbs_only_flag + 1));
    }

    seq_param->frame_cropping_flag = frame_cropping_flag;
    seq_param->frame_crop_left_offset = 0;
    seq_param->frame_crop_right_offset = 0;
    seq_param->frame_crop_top_offset = 0;
    seq_param->frame_crop_bottom_offset = frame_crop_bottom_offset;

    seq_param->seq_fields.bits.pic_order_cnt_type = 0;
    seq_param->seq_fields.bits.direct_8x8_inference_flag = 0;
    
    seq_param->seq_fields.bits.log2_max_frame_num_minus4 = Log2MaxFrameNum - 4;
    seq_param->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 = Log2MaxPicOrderCntLsb - 4;
	
    if (frame_bit_rate > 0)
        seq_param->vui_parameters_present_flag = 1;	//HRD info located in vui
    else
        seq_param->vui_parameters_present_flag = 0;
}

static void avcenc_context_pic_param_init(VAEncPictureParameterBufferH264 *pic_param)
{
    pic_param->seq_parameter_set_id = 0;
    pic_param->pic_parameter_set_id = 0;

    pic_param->last_picture = 0;
    pic_param->frame_num = 0;
    
    pic_param->pic_init_qp = (qp_value >= 0 ?  qp_value : 26);
    pic_param->num_ref_idx_l0_active_minus1 = 0;
    pic_param->num_ref_idx_l1_active_minus1 = 0;

    pic_param->pic_fields.bits.idr_pic_flag = 0;
    pic_param->pic_fields.bits.reference_pic_flag = 0;
    pic_param->pic_fields.bits.entropy_coding_mode_flag = ENTROPY_MODE_CABAC;
    pic_param->pic_fields.bits.weighted_pred_flag = 0;
    pic_param->pic_fields.bits.weighted_bipred_idc = 0;
    
    if (avcenc_context.constraint_set_flag & 0x7)
        pic_param->pic_fields.bits.transform_8x8_mode_flag = 0;
    else
        pic_param->pic_fields.bits.transform_8x8_mode_flag = 1;

    pic_param->pic_fields.bits.deblocking_filter_control_present_flag = 1;

    memset(pic_param->ReferenceFrames, 0xff, 16 * sizeof(VAPictureH264)); /* invalid all */
}

static void avcenc_context_sei_init()
{
	/* it comes for the bps defined in SPS */
	avcenc_context.i_initial_cpb_removal_delay = 2 * 90000;
	avcenc_context.i_initial_cpb_removal_delay_offset = 2 * 90000;

	avcenc_context.i_cpb_removal_delay = 2;
	avcenc_context.i_initial_cpb_removal_delay_length = 24;
	avcenc_context.i_cpb_removal_delay_length = 24;
	avcenc_context.i_dpb_output_delay_length = 24;
	avcenc_context.time_offset_length = 24;

        avcenc_context.prev_idr_cpb_removal = avcenc_context.i_initial_cpb_removal_delay / 90000;
        avcenc_context.current_idr_cpb_removal = avcenc_context.prev_idr_cpb_removal;
        avcenc_context.current_cpb_removal = 0;
        avcenc_context.idr_frame_num = 0;
}

static void avcenc_context_init(int width, int height)
{
    int i;
    memset(&avcenc_context, 0, sizeof(avcenc_context));
    avcenc_context.profile = VAProfileH264Main;

    memset(&use_slot, 0, sizeof(use_slot));
    switch (avcenc_context.profile) {
    case VAProfileH264Baseline:
        avcenc_context.constraint_set_flag |= (1 << 0); /* Annex A.2.1 */
        break;

    case VAProfileH264Main:
        avcenc_context.constraint_set_flag |= (1 << 1); /* Annex A.2.2 */
        break;

    case VAProfileH264High:
        avcenc_context.constraint_set_flag |= (1 << 3); /* Annex A.2.4 */
        break;
        
    default:
        break;
    }
        
    avcenc_context.seq_param_buf_id = VA_INVALID_ID;
    avcenc_context.pic_param_buf_id = VA_INVALID_ID;
    avcenc_context.packed_seq_header_param_buf_id = VA_INVALID_ID;
    avcenc_context.packed_seq_buf_id = VA_INVALID_ID;
    avcenc_context.packed_pic_header_param_buf_id = VA_INVALID_ID;
    avcenc_context.packed_pic_buf_id = VA_INVALID_ID;
    avcenc_context.codedbuf_buf_id = VA_INVALID_ID;
    avcenc_context.misc_parameter_hrd_buf_id = VA_INVALID_ID;
    avcenc_context.codedbuf_i_size = width * height;
    avcenc_context.codedbuf_pb_size = width * height;
    avcenc_context.current_input_surface = SID_INPUT_PICTURE_0;
    avcenc_context.upload_thread_value = -1;
    avcenc_context.packed_sei_header_param_buf_id = VA_INVALID_ID;
    avcenc_context.packed_sei_buf_id = VA_INVALID_ID;

    if (qp_value == -1)
        avcenc_context.rate_control_method = VA_RC_CBR;
    else if (qp_value == -2)
        avcenc_context.rate_control_method = VA_RC_VBR;
    else {
        assert(qp_value >= 0 && qp_value <= 51);
        avcenc_context.rate_control_method = VA_RC_CQP;
    }

    for (i = 0; i < MAX_SLICES; i++) {
        avcenc_context.slice_param_buf_id[i] = VA_INVALID_ID;
    }

    avcenc_context_seq_param_init(&avcenc_context.seq_param, width, height);
    avcenc_context_pic_param_init(&avcenc_context.pic_param);
    if (avcenc_context.rate_control_method == VA_RC_CBR)
	avcenc_context_sei_init();
}

int main(int argc, char *argv[])
{
    int f;
    FILE *yuv_fp;
    FILE *avc_fp;
    off_t file_size;
    int mode_value;
    struct timeval tpstart,tpend; 
    float  timeuse;

    va_init_display_args(&argc, argv);

    //TODO may be we should using option analytics library
    if(argc != 5 && argc != 6 && argc != 7) {
        show_help();
        return -1;
    }

    picture_width = atoi(argv[1]);
    picture_height = atoi(argv[2]);
    picture_width_in_mbs = (picture_width + 15) / 16;
    picture_height_in_mbs = (picture_height + 15) / 16;

    if (argc == 6 || argc == 7) {
        qp_value = -1;
        sscanf(argv[5], "qp=%d", &qp_value);
        if ( qp_value == -1 ) {
            frame_bit_rate = -1;
            sscanf(argv[5], "fb=%d", &frame_bit_rate);
            if (  frame_bit_rate == -1 ) {
                show_help();
                return -1;
            }
        } else if (qp_value > 51) {
            qp_value = 51;
        } else if (qp_value < 0) {
            qp_value = 0;
        }
    } else
        qp_value = 28;                          //default const QP mode

    if (argc == 7) {
        sscanf(argv[6], "mode=%d", &mode_value);
        if ( mode_value == 0 ) {
		ip_period = 0;
        }
        else if ( mode_value == 1) {
		ip_period = 1;
        }
        else if ( mode_value == 2 ) {
		/* Hack mechanism before adding the parameter of B-frame number */
		ip_period = 3;
        }
        else {
                printf("mode_value=%d\n",mode_value);
                show_help();
                return -1;
        }
    }

    yuv_fp = fopen(argv[3],"rb");
    if ( yuv_fp == NULL){
        printf("Can't open input YUV file\n");
        return -1;
    }
    fseeko(yuv_fp, (off_t)0, SEEK_END);
    file_size = ftello(yuv_fp);
    frame_size = picture_width * picture_height +  ((picture_width * picture_height) >> 1) ;

    if ( (file_size < frame_size) || (file_size % frame_size) ) {
        fclose(yuv_fp);
        printf("The YUV file's size is not correct\n");
        return -1;
    }
    frame_number = file_size / frame_size;
    fseeko(yuv_fp, (off_t)0, SEEK_SET);

    avc_fp = fopen(argv[4], "wb");	
    if ( avc_fp == NULL) {
        fclose(yuv_fp);
        printf("Can't open output avc file\n");
        return -1;
    }	
    gettimeofday(&tpstart,NULL);	
    avcenc_context_init(picture_width, picture_height);
    create_encode_pipe();
    alloc_encode_resource(yuv_fp);

    enc_frame_number = 0;
    for ( f = 0; f < frame_number; f++) {		//picture level loop
        unsigned long long next_frame_display;
        int next_frame_type;

        enc_frame_number = f;

        encoding2display_order(enc_frame_number, intra_period, ip_period,
                               &current_frame_display, &current_frame_type);

        encoding2display_order(enc_frame_number + 1, intra_period, ip_period,
                               &next_frame_display, &next_frame_type);

        if (current_frame_type == FRAME_IDR) {
            numShortTerm = 0;
            current_frame_num = 0;
            memset(&use_slot, 0, sizeof(use_slot));
            current_IDR_display = current_frame_display;
            if (avcenc_context.rate_control_method == VA_RC_CBR) {
                unsigned long long frame_interval;

                frame_interval = enc_frame_number - avcenc_context.idr_frame_num;

                /* Based on the H264 spec the removal time of the IDR access
                 * unit is derived as the following:
                 * the removal time of previous IDR unit + Tc * cpb_removal_delay(n)
                 */
                avcenc_context.current_cpb_removal = avcenc_context.prev_idr_cpb_removal +
				frame_interval * 2;
                avcenc_context.idr_frame_num = enc_frame_number;
                avcenc_context.current_idr_cpb_removal = avcenc_context.current_cpb_removal;
                if (ip_period)
                    avcenc_context.current_dpb_removal_delta = (ip_period + 1) * 2;
                else
                    avcenc_context.current_dpb_removal_delta = 2;
            }
        } else {
            if (avcenc_context.rate_control_method == VA_RC_CBR) {
                unsigned long long frame_interval;

                frame_interval = enc_frame_number - avcenc_context.idr_frame_num;

                /* Based on the H264 spec the removal time of the non-IDR access
                 * unit is derived as the following:
                 * the removal time of current IDR unit + Tc * cpb_removal_delay(n)
                 */
                avcenc_context.current_cpb_removal = avcenc_context.current_idr_cpb_removal +
				frame_interval * 2;
                if (current_frame_type == SLICE_TYPE_I ||
                    current_frame_type == SLICE_TYPE_P) {
                    if (ip_period)
                        avcenc_context.current_dpb_removal_delta = (ip_period + 1) * 2;
                    else
                        avcenc_context.current_dpb_removal_delta = 2;
                } else
                   avcenc_context.current_dpb_removal_delta = 2;
            }
        }

        /* use the simple mechanism to calc the POC */
        current_poc = (current_frame_display - current_IDR_display) * 2;

        encode_picture(yuv_fp, avc_fp, frame_number, current_frame_display,
                      (current_frame_type == FRAME_IDR) ? 1 : 0,
                      (current_frame_type == FRAME_IDR) ? SLICE_TYPE_I : current_frame_type,
                      (next_frame_type == SLICE_TYPE_B) ? 1 : 0,
                next_frame_display);
        if ((current_frame_type == FRAME_IDR) &&
            (avcenc_context.rate_control_method == VA_RC_CBR)) {
           /* after one IDR frame is written, it needs to update the
            * prev_idr_cpb_removal for next IDR
            */
           avcenc_context.prev_idr_cpb_removal = avcenc_context.current_idr_cpb_removal;
        }
        printf("\r %d/%d ...", f, frame_number);
        fflush(stdout);
    }

    gettimeofday(&tpend,NULL);
    timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+ tpend.tv_usec-tpstart.tv_usec;
    timeuse/=1000000;
    printf("\ndone!\n");
    printf("encode %d frames in %f secondes, FPS is %.1f\n",frame_number, timeuse, frame_number/timeuse);
    release_encode_resource();
    destory_encode_pipe();

    fclose(yuv_fp);
    fclose(avc_fp);

    return 0;
}
