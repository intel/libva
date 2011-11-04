/*
 * Simple AVC encoder based on libVA.
 *
 * Usage:
 * ./avcenc <width> <height> <input file> <output file> [qp]
 */  

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <X11/Xlib.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>

#include <va/va.h>
#include <va/va_x11.h>

#define NAL_REF_IDC_NONE        0
#define NAL_REF_IDC_LOW         1
#define NAL_REF_IDC_MEDIUM      2
#define NAL_REF_IDC_HIGH        3

#define NAL_NON_IDR             1
#define NAL_IDR                 5
#define NAL_SPS                 7
#define NAL_PPS                 8

#define SLICE_TYPE_P            0
#define SLICE_TYPE_B            1
#define SLICE_TYPE_I            2

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

static Display *x11_display;
static VADisplay va_dpy;
static VAContextID context_id;
static VAConfigID config_id;

static int picture_width, picture_width_in_mbs;
static int picture_height, picture_height_in_mbs;
static int frame_size;
static unsigned char *newImageBuffer = 0;
static int codedbuf_size;

static int qp_value = 26;

static int log2_max_frame_num_minus4 = 0;
static int pic_order_cnt_type = 0;
static int log2_max_pic_order_cnt_lsb_minus4 = 0;
static int entropy_coding_mode_flag = ENTROPY_MODE_CABAC;
static int deblocking_filter_control_present_flag = 1;
static int frame_mbs_only_flag = 1;

static void create_encode_pipe()
{
    VAEntrypoint entrypoints[5];
    int num_entrypoints,slice_entrypoint;
    VAConfigAttrib attrib[2];
    int major_ver, minor_ver;
    VAStatus va_status;

    x11_display = XOpenDisplay(":0.0");
    assert(x11_display);

    va_dpy = vaGetDisplay(x11_display);
    va_status = vaInitialize(va_dpy, &major_ver, &minor_ver);
    CHECK_VASTATUS(va_status, "vaInitialize");

    vaQueryConfigEntrypoints(va_dpy, VAProfileH264Baseline, entrypoints, 
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
    vaGetConfigAttributes(va_dpy, VAProfileH264Baseline, VAEntrypointEncSlice,
                          &attrib[0], 2);

    if ((attrib[0].value & VA_RT_FORMAT_YUV420) == 0) {
        /* not find desired YUV420 RT format */
        assert(0);
    }

    if ((attrib[1].value & VA_RC_VBR) == 0) {
        /* Can't find matched RC mode */
        printf("VBR mode doesn't found, exit\n");
        assert(0);
    }

    attrib[0].value = VA_RT_FORMAT_YUV420; /* set to desired RT format */
    attrib[1].value = VA_RC_VBR; /* set to desired RC mode */

    va_status = vaCreateConfig(va_dpy, VAProfileH264Baseline, VAEntrypointEncSlice,
                               &attrib[0], 2,&config_id);
    CHECK_VASTATUS(va_status, "vaCreateConfig");

    /* Create a context for this decode pipe */
    va_status = vaCreateContext(va_dpy, config_id,
                                picture_width, picture_height,
                                VA_PROGRESSIVE, 
                                0, 0,
                                &context_id);
    CHECK_VASTATUS(va_status, "vaCreateContext");
}

static void destory_encode_pipe()
{
    vaDestroyContext(va_dpy,context_id);
    vaDestroyConfig(va_dpy,config_id);
    vaTerminate(va_dpy);
    XCloseDisplay(x11_display);
}

/***************************************************
 *
 *  The encode pipe resource define 
 *
 ***************************************************/
static VABufferID seq_parameter = VA_INVALID_ID;                /*Sequence level parameter*/
static VABufferID pic_parameter = VA_INVALID_ID;                /*Picture level parameter*/
static VABufferID slice_parameter = VA_INVALID_ID;              /*Slice level parameter, multil slices*/

static VABufferID coded_buf;                                    /*Output buffer, compressed data*/

#define SID_NUMBER                              3
#define SID_INPUT_PICTURE                       0
#define SID_REFERENCE_PICTURE                   1
#define SID_RECON_PICTURE                       2
static  VASurfaceID surface_ids[SID_NUMBER];

/***************************************************/

static void alloc_encode_resource()
{
    VAStatus va_status;

    seq_parameter = VA_INVALID_ID;		
    pic_parameter = VA_INVALID_ID;
    slice_parameter = VA_INVALID_ID;

    //1. Create sequence parameter set
    {
        VAEncSequenceParameterBufferH264Baseline seq_h264 = {0};

        seq_h264.level_idc = 30;
        seq_h264.picture_width_in_mbs = picture_width_in_mbs;
        seq_h264.picture_height_in_mbs = picture_height_in_mbs;

        seq_h264.bits_per_second = 384*1000;
        seq_h264.initial_qp = qp_value;
        seq_h264.min_qp = 3;

        va_status = vaCreateBuffer(va_dpy, context_id,
                                   VAEncSequenceParameterBufferType,
                                   sizeof(seq_h264),1,&seq_h264,&seq_parameter);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");;
    }

    //2. Create surface
    va_status = vaCreateSurfaces(va_dpy, picture_width, picture_height,
                                 VA_RT_FORMAT_YUV420, SID_NUMBER, &surface_ids[0]);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");

    //3. Create coded buffer
    {
        va_status = vaCreateBuffer(va_dpy,context_id,VAEncCodedBufferType,
                                   codedbuf_size, 1, NULL, &coded_buf);

        CHECK_VASTATUS(va_status,"vaBeginPicture");
    }

    newImageBuffer = (unsigned char *)malloc(frame_size);
}

static void release_encode_resource()
{
    free(newImageBuffer);

    //-3 Relese coded buffer
    vaDestroyBuffer(va_dpy, coded_buf);

    //-2 Release all the surfaces resource
    vaDestroySurfaces(va_dpy, &surface_ids[0], SID_NUMBER);	

    //-1 Destory the sequence level parameter
    vaDestroyBuffer(va_dpy, seq_parameter);
}

static void begin_picture()
{
    VAStatus va_status;
    va_status = vaBeginPicture(va_dpy, context_id, surface_ids[SID_INPUT_PICTURE]);
    CHECK_VASTATUS(va_status,"vaBeginPicture");
}

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
    } else {
        /* FIXME: fix this later */
        assert(0);
    }

    vaUnmapBuffer(va_dpy, surface_image.buf);
    vaDestroyImage(va_dpy, surface_image.image_id);
}

static void prepare_input(FILE * yuv_fp, int intra_slice)
{
    static VAEncPictureParameterBufferH264Baseline pic_h264;
    static VAEncSliceParameterBuffer slice_h264;
    VAStatus va_status;
    VABufferID tempID;	
    VACodedBufferSegment *coded_buffer_segment = NULL; 
    unsigned char *coded_mem;

    // Sequence level
    va_status = vaRenderPicture(va_dpy, context_id, &seq_parameter, 1);
    CHECK_VASTATUS(va_status,"vaRenderPicture");;

    // Copy Image to target surface according input YUV data.
    upload_yuv_to_surface(yuv_fp, surface_ids[SID_INPUT_PICTURE]);

    // Picture level
    pic_h264.reference_picture = surface_ids[SID_REFERENCE_PICTURE];
    pic_h264.reconstructed_picture = surface_ids[SID_RECON_PICTURE];
    pic_h264.coded_buf = coded_buf;
    pic_h264.picture_width = picture_width;
    pic_h264.picture_height = picture_height;
    pic_h264.last_picture = 0;
    if (pic_parameter != VA_INVALID_ID) {	
        vaDestroyBuffer(va_dpy, pic_parameter);	
    }
    va_status = vaCreateBuffer(va_dpy, context_id,VAEncPictureParameterBufferType,
                               sizeof(pic_h264),1,&pic_h264,&pic_parameter);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");
    va_status = vaRenderPicture(va_dpy,context_id, &pic_parameter, 1);
    CHECK_VASTATUS(va_status,"vaRenderPicture");
	
    // clean old memory
    va_status = vaMapBuffer(va_dpy,coded_buf,(void **)(&coded_buffer_segment));
    CHECK_VASTATUS(va_status,"vaMapBuffer");
    coded_mem = coded_buffer_segment->buf;
    memset(coded_mem, 0, coded_buffer_segment->size);
    vaUnmapBuffer(va_dpy, coded_buf);

    // Slice level	
    slice_h264.start_row_number = 0;
    slice_h264.slice_height = picture_height/16; /* Measured by MB */
    slice_h264.slice_flags.bits.is_intra = intra_slice;
    slice_h264.slice_flags.bits.disable_deblocking_filter_idc = 0;
    if ( slice_parameter != VA_INVALID_ID){
        vaDestroyBuffer(va_dpy, slice_parameter);
    }
    va_status = vaCreateBuffer(va_dpy,context_id,VAEncSliceParameterBufferType,
                               sizeof(slice_h264),1,&slice_h264,&slice_parameter);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");;
    va_status = vaRenderPicture(va_dpy,context_id, &slice_parameter, 1);
    CHECK_VASTATUS(va_status,"vaRenderPicture");

    // Prepare for next picture
    tempID = surface_ids[SID_RECON_PICTURE];  
    surface_ids[SID_RECON_PICTURE] = surface_ids[SID_REFERENCE_PICTURE]; 
    surface_ids[SID_REFERENCE_PICTURE] = tempID;
}

static void end_picture()
{	
    VAStatus va_status;

    va_status = vaEndPicture(va_dpy,context_id);
    CHECK_VASTATUS(va_status,"vaRenderPicture");
}

#define BITSTREAM_ALLOCATE_STEPPING     4096

struct __bitstream {
    unsigned int *buffer;
    int bit_offset;
    int max_size_in_dword;
};

typedef struct __bitstream bitstream;

static int 
get_coded_bitsteam_length(unsigned char *buffer, int buffer_length)
{
    int i;

    for (i = buffer_length - 1; i >= 0; i--) {
        if (buffer[i])
            break;
    }

    return i + 1;
}

static unsigned int 
swap32(unsigned int val)
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
bitstream_end(bitstream *bs, FILE *avc_fp)
{
    int pos = (bs->bit_offset >> 5);
    int bit_offset = (bs->bit_offset & 0x1f);
    int bit_left = 32 - bit_offset;
    int length = (bs->bit_offset + 7) >> 3;
    size_t w_items;

    if (bit_offset) {
        bs->buffer[pos] = swap32((bs->buffer[pos] << bit_left));
    }

    do {
        w_items = fwrite(bs->buffer, length, 1, avc_fp);
    } while (w_items != 1);

    free(bs->buffer);
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
        bs->buffer[pos] = swap32(bs->buffer[pos]);

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
    int mb_width, mb_height;
    int frame_cropping_flag = 0;
    int frame_crop_bottom_offset = 0;
    int profile_idc = PROFILE_IDC_MAIN;

    mb_width = picture_width_in_mbs;
    mb_height = picture_height_in_mbs;

    if (mb_height * 16 - picture_height) {
        frame_cropping_flag = 1;
        frame_crop_bottom_offset = 
            (mb_height * 16 - picture_height) / (2 * (!frame_mbs_only_flag + 1));
    }

    bitstream_put_ui(bs, profile_idc, 8);               /* profile_idc */
    bitstream_put_ui(bs, 0, 1);                         /* constraint_set0_flag */
    bitstream_put_ui(bs, 1, 1);                         /* constraint_set1_flag */
    bitstream_put_ui(bs, 0, 1);                         /* constraint_set2_flag */
    bitstream_put_ui(bs, 0, 1);                         /* constraint_set3_flag */
    bitstream_put_ui(bs, 0, 4);                         /* reserved_zero_4bits */
    bitstream_put_ui(bs, 41, 8);                        /* level_idc */
    bitstream_put_ue(bs, 0);                            /* seq_parameter_set_id */

    if (profile_idc >= 100) {
        /* FIXME: fix for high profile */
        assert(0);
    }

    bitstream_put_ue(bs, log2_max_frame_num_minus4);    /* log2_max_frame_num_minus4 */
    bitstream_put_ue(bs, pic_order_cnt_type);           /* pic_order_cnt_type */

    if (pic_order_cnt_type == 0)
        bitstream_put_ue(bs, log2_max_pic_order_cnt_lsb_minus4);        /* log2_max_pic_order_cnt_lsb_minus4 */
    else {
        assert(0);
    }

    bitstream_put_ue(bs, 1);                            /* num_ref_frames */
    bitstream_put_ui(bs, 0, 1);                         /* gaps_in_frame_num_value_allowed_flag */

    bitstream_put_ue(bs, mb_width - 1);                 /* pic_width_in_mbs_minus1 */
    bitstream_put_ue(bs, mb_height - 1);                /* pic_height_in_map_units_minus1 */
    bitstream_put_ui(bs, frame_mbs_only_flag, 1);       /* frame_mbs_only_flag */

    if (!frame_mbs_only_flag) {
        assert(0);
    }

    bitstream_put_ui(bs, 0, 1);                         /* direct_8x8_inference_flag */
    bitstream_put_ui(bs, frame_cropping_flag, 1);       /* frame_cropping_flag */

    if (frame_cropping_flag) {
        bitstream_put_ue(bs, 0);                        /* frame_crop_left_offset */
        bitstream_put_ue(bs, 0);                        /* frame_crop_right_offset */
        bitstream_put_ue(bs, 0);                        /* frame_crop_top_offset */
        bitstream_put_ue(bs, frame_crop_bottom_offset); /* frame_crop_bottom_offset */
    }

    bitstream_put_ui(bs, 0, 1);                         /* vui_parameters_present_flag */
    rbsp_trailing_bits(bs);                             /* rbsp_trailing_bits */
}

static void build_nal_sps(FILE *avc_fp)
{
    bitstream bs;

    bitstream_start(&bs);
    nal_start_code_prefix(&bs);
    nal_header(&bs, NAL_REF_IDC_HIGH, NAL_SPS);
    sps_rbsp(&bs);
    bitstream_end(&bs, avc_fp);
}

static void pps_rbsp(bitstream *bs)
{
    bitstream_put_ue(bs, 0);		                /* pic_parameter_set_id */
    bitstream_put_ue(bs, 0);                            /* seq_parameter_set_id */

    bitstream_put_ui(bs, entropy_coding_mode_flag, 1);  /* entropy_coding_mode_flag */

    bitstream_put_ui(bs, 0, 1);                         /* pic_order_present_flag: 0 */

    bitstream_put_ue(bs, 0);                            /* num_slice_groups_minus1 */

    bitstream_put_ue(bs, 0);                            /* num_ref_idx_l0_active_minus1 */
    bitstream_put_ue(bs, 0);                            /* num_ref_idx_l1_active_minus1 1 */

    bitstream_put_ui(bs, 0, 1);                         /* weighted_pred_flag: 0 */
    bitstream_put_ui(bs, 0, 2);	                        /* weighted_bipred_idc: 0 */

    bitstream_put_se(bs, 0);                            /* pic_init_qp_minus26 */
    bitstream_put_se(bs, 0);                            /* pic_init_qs_minus26 */
    bitstream_put_se(bs, 0);                            /* chroma_qp_index_offset */

    bitstream_put_ui(bs, 1, 1);                         /* deblocking_filter_control_present_flag */
    bitstream_put_ui(bs, 0, 1);                         /* constrained_intra_pred_flag */
    bitstream_put_ui(bs, 0, 1);                         /* redundant_pic_cnt_present_flag */

    rbsp_trailing_bits(bs);
}

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


static void 
slice_header(bitstream *bs, int frame_num, int slice_type, int is_idr)
{       
    int is_cabac = (entropy_coding_mode_flag == ENTROPY_MODE_CABAC);

    bitstream_put_ue(bs, 0);                   /* first_mb_in_slice: 0 */
    bitstream_put_ue(bs, slice_type);          /* slice_type */
    bitstream_put_ue(bs, 0);                   /* pic_parameter_set_id: 0 */
    bitstream_put_ui(bs, frame_num & 0x0F, log2_max_frame_num_minus4 + 4);    /* frame_num */

    /* frame_mbs_only_flag == 1 */
    if (!frame_mbs_only_flag) {
        /* FIXME: */
        assert(0);
    }

    if (is_idr)
        bitstream_put_ue(bs, 0);		/* idr_pic_id: 0 */

    if (pic_order_cnt_type == 0) {
	bitstream_put_ui(bs, (frame_num * 2) & 0x0F, log2_max_pic_order_cnt_lsb_minus4 + 4);
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
        /* FIXME */
        assert(0);
    }   

    /* weighted_pred_flag == 0 */

    /* dec_ref_pic_marking */
    if (is_idr) {
        bitstream_put_ui(bs, 0, 1);            /* no_output_of_prior_pics_flag: 0 */
        bitstream_put_ui(bs, 0, 1);            /* long_term_reference_flag: 0 */
    } else {
        bitstream_put_ui(bs, 0, 1);            /* adaptive_ref_pic_marking_mode_flag: 0 */
    }

    if (is_cabac && (slice_type != SLICE_TYPE_I))
        bitstream_put_ue(bs, 0);               /* cabac_init_idc: 0 */

    bitstream_put_se(bs, 0);                   /* slice_qp_delta: 0 */

    if (deblocking_filter_control_present_flag == 1) {
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
    int is_cabac = (entropy_coding_mode_flag == ENTROPY_MODE_CABAC);

    va_status = vaSyncSurface(va_dpy, surface_ids[SID_INPUT_PICTURE]);
    CHECK_VASTATUS(va_status,"vaSyncSurface");

    surface_status = 0;
    va_status = vaQuerySurfaceStatus(va_dpy, surface_ids[SID_INPUT_PICTURE], &surface_status);
    CHECK_VASTATUS(va_status,"vaQuerySurfaceStatus");

    va_status = vaMapBuffer(va_dpy, coded_buf, (void **)(&coded_buffer_segment));
    CHECK_VASTATUS(va_status,"vaMapBuffer");
    coded_mem = coded_buffer_segment->buf;

    if (is_cabac) {
        bitstream_byte_aligning(bs, 1);
        slice_data_length = get_coded_bitsteam_length(coded_mem, codedbuf_size);

	for (i = 0; i < slice_data_length; i++) {
            bitstream_put_ui(bs, *coded_mem, 8);
            coded_mem++;
	}
    } else {
        /* FIXME */
        assert(0);
    }

    vaUnmapBuffer(va_dpy, coded_buf);
}

static void 
build_nal_slice(FILE *avc_fp, int frame_num, int slice_type, int is_idr)
{
    bitstream bs;

    bitstream_start(&bs);
    nal_start_code_prefix(&bs);
    nal_header(&bs, NAL_REF_IDC_HIGH, is_idr ? NAL_IDR : NAL_NON_IDR);
    slice_header(&bs, frame_num, slice_type, is_idr);
    slice_data(&bs);
    bitstream_end(&bs, avc_fp);
}

static void 
store_coded_buffer(FILE *avc_fp, int frame_num, int is_intra, int is_idr)
{
    build_nal_slice(avc_fp, frame_num, is_intra ? SLICE_TYPE_I : SLICE_TYPE_P, is_idr);
}

int main(int argc, char *argv[])
{
    int f;
    FILE *yuv_fp;
    FILE *avc_fp;
    int frame_number;
    long file_size;
    clock_t start_clock, end_clock;
    float encoding_time;

    if(argc != 5 && argc != 6) {
        printf("Usage: %s <width> <height> <input_yuvfile> <output_avcfile> [qp]\n", argv[0]);
        return -1;
    }

    picture_width = atoi(argv[1]);
    picture_height = atoi(argv[2]);
    picture_width_in_mbs = (picture_width + 15) / 16;
    picture_height_in_mbs = (picture_height + 15) / 16;

    if (argc == 6)
        qp_value = atoi(argv[5]);
    else
        qp_value = 26;

    yuv_fp = fopen(argv[3],"rb");
    if ( yuv_fp == NULL){
        printf("Can't open input YUV file\n");
        return -1;
    }
    fseek(yuv_fp,0l, SEEK_END);
    file_size = ftell(yuv_fp);
    frame_size = picture_width * picture_height +  ((picture_width * picture_height) >> 1) ;
    codedbuf_size = picture_width * picture_height * 1.5;

    if ( (file_size < frame_size) || (file_size % frame_size) ) {
        printf("The YUV file's size is not correct\n");
        return -1;
    }
    frame_number = file_size / frame_size;
    fseek(yuv_fp, 0l, SEEK_SET);

    avc_fp = fopen(argv[4], "wb");	
    if ( avc_fp == NULL) {
        printf("Can't open output avc file\n");
        return -1;
    }	
    start_clock = clock();
    build_header(avc_fp);

    create_encode_pipe();
    alloc_encode_resource();

    for ( f = 0; f < frame_number; f++ ) {		//picture level loop
        int is_intra = (f % 30 == 0);
        int is_idr = (f == 0);

        begin_picture();
        prepare_input(yuv_fp, is_intra);
        end_picture();
        store_coded_buffer(avc_fp, f, is_intra, is_idr);

        printf("\r %d/%d ...", f+1, frame_number);
        fflush(stdout);
    }

    end_clock = clock();
    printf("\ndone!\n");
    encoding_time = (float)(end_clock-start_clock)/CLOCKS_PER_SEC;
    printf("encode %d frames in %f secondes, FPS is %.1f\n",frame_number, encoding_time, frame_number/encoding_time);

    release_encode_resource();
    destory_encode_pipe();

    return 0;
}
