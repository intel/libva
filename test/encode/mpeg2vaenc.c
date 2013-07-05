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
 * Simple MPEG-2 encoder based on libVA.
 *
 */  

#include "sysdeps.h"

#include <getopt.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>

#include <va/va.h>
#include <va/va_enc_mpeg2.h>

#include "va_display.h"

#define START_CODE_PICUTRE      0x00000100
#define START_CODE_SLICE        0x00000101
#define START_CODE_USER         0x000001B2
#define START_CODE_SEQ          0x000001B3
#define START_CODE_EXT          0x000001B5
#define START_CODE_GOP          0x000001B8

#define CHROMA_FORMAT_RESERVED  0
#define CHROMA_FORMAT_420       1
#define CHROMA_FORMAT_422       2
#define CHROMA_FORMAT_444       3

#define MAX_SLICES              128

enum {
    MPEG2_MODE_I = 0,
    MPEG2_MODE_IP,
    MPEG2_MODE_IPB,
};

enum {
    MPEG2_LEVEL_LOW = 0,
    MPEG2_LEVEL_MAIN,
    MPEG2_LEVEL_HIGH,
};

#define CHECK_VASTATUS(va_status, func)                                 \
    if (va_status != VA_STATUS_SUCCESS) {                               \
        fprintf(stderr, "%s:%s (%d) failed, exit\n", __func__, func, __LINE__); \
        exit(1);                                                        \
    }

static VAProfile mpeg2_va_profiles[] = {
    VAProfileMPEG2Simple,
    VAProfileMPEG2Main
};

static struct _mpeg2_sampling_density
{
    int samplers_per_line;
    int line_per_frame;
    int frame_per_sec;
} mpeg2_upper_samplings[2][3] = {
    { { 0, 0, 0 },
      { 720, 576, 30 },
      { 0, 0, 0 },
    },

    { { 352, 288, 30 },
      { 720, 576, 30 },
      { 1920, 1152, 60 },
    }
};

struct mpeg2enc_context {
    /* args */
    int rate_control_mode;
    int fps;
    int mode; /* 0:I, 1:I/P, 2:I/P/B */
    VAProfile profile;
    int level;
    int width;
    int height;
    int frame_size;
    int num_pictures;
    int qp;
    FILE *ifp;
    FILE *ofp;
    unsigned char *frame_data_buffer;
    int intra_period;
    int ip_period;
    int bit_rate; /* in kbps */
    VAEncPictureType next_type;
    int next_display_order;
    int next_bframes;
    int new_sequence;
    int new_gop_header;
    int gop_header_in_display_order;

    /* VA resource */
    VADisplay va_dpy;
    VAEncSequenceParameterBufferMPEG2 seq_param;
    VAEncPictureParameterBufferMPEG2 pic_param;
    VAEncSliceParameterBufferMPEG2 slice_param[MAX_SLICES];
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
    int num_slice_groups;
    int codedbuf_i_size;
    int codedbuf_pb_size;

    /* thread */
    pthread_t upload_thread_id;
    int upload_thread_value;
    int current_input_surface;
    int current_upload_surface;
};

/*
 * mpeg2enc helpers
 */
#define BITSTREAM_ALLOCATE_STEPPING     4096

struct __bitstream {
    unsigned int *buffer;
    int bit_offset;
    int max_size_in_dword;
};

typedef struct __bitstream bitstream;

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
bitstream_end(bitstream *bs)
{
    int pos = (bs->bit_offset >> 5);
    int bit_offset = (bs->bit_offset & 0x1f);
    int bit_left = 32 - bit_offset;

    if (bit_offset) {
        bs->buffer[pos] = swap32((bs->buffer[pos] << bit_left));
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

    if (size_in_bits < 32)
        val &= ((1 << size_in_bits) - 1);

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

static struct mpeg2_frame_rate {
    int code;
    float value;
} frame_rate_tab[] = {
    {1, 23.976},
    {2, 24.0},
    {3, 25.0},
    {4, 29.97},
    {5, 30},
    {6, 50},
    {7, 59.94},
    {8, 60}
};

static int
find_frame_rate_code(const VAEncSequenceParameterBufferMPEG2 *seq_param)
{
    unsigned int delta = -1;
    int code = 1, i;
    float frame_rate_value = seq_param->frame_rate * 
        (seq_param->sequence_extension.bits.frame_rate_extension_d + 1) / 
        (seq_param->sequence_extension.bits.frame_rate_extension_n + 1);

    for (i = 0; i < sizeof(frame_rate_tab) / sizeof(frame_rate_tab[0]); i++) {

        if (abs(1000 * frame_rate_tab[i].value - 1000 * frame_rate_value) < delta) {
            code = frame_rate_tab[i].code;
            delta = abs(1000 * frame_rate_tab[i].value - 1000 * frame_rate_value);
        }
    }

    return code;
}

static void 
sps_rbsp(struct mpeg2enc_context *ctx,
         const VAEncSequenceParameterBufferMPEG2 *seq_param,
         bitstream *bs)
{
    int frame_rate_code = find_frame_rate_code(seq_param);

    if (ctx->new_sequence) {
        bitstream_put_ui(bs, START_CODE_SEQ, 32);
        bitstream_put_ui(bs, seq_param->picture_width, 12);
        bitstream_put_ui(bs, seq_param->picture_height, 12);
        bitstream_put_ui(bs, seq_param->aspect_ratio_information, 4);
        bitstream_put_ui(bs, frame_rate_code, 4); /* frame_rate_code */
        bitstream_put_ui(bs, (seq_param->bits_per_second + 399) / 400, 18); /* the low 18 bits of bit_rate */
        bitstream_put_ui(bs, 1, 1); /* marker_bit */
        bitstream_put_ui(bs, seq_param->vbv_buffer_size, 10);
        bitstream_put_ui(bs, 0, 1); /* constraint_parameter_flag, always 0 for MPEG-2 */
        bitstream_put_ui(bs, 0, 1); /* load_intra_quantiser_matrix */
        bitstream_put_ui(bs, 0, 1); /* load_non_intra_quantiser_matrix */

        bitstream_byte_aligning(bs, 0);

        bitstream_put_ui(bs, START_CODE_EXT, 32);
        bitstream_put_ui(bs, 1, 4); /* sequence_extension id */
        bitstream_put_ui(bs, seq_param->sequence_extension.bits.profile_and_level_indication, 8);
        bitstream_put_ui(bs, seq_param->sequence_extension.bits.progressive_sequence, 1);
        bitstream_put_ui(bs, seq_param->sequence_extension.bits.chroma_format, 2);
        bitstream_put_ui(bs, seq_param->picture_width >> 12, 2);
        bitstream_put_ui(bs, seq_param->picture_height >> 12, 2);
        bitstream_put_ui(bs, ((seq_param->bits_per_second + 399) / 400) >> 18, 12); /* bit_rate_extension */
        bitstream_put_ui(bs, 1, 1); /* marker_bit */
        bitstream_put_ui(bs, seq_param->vbv_buffer_size >> 10, 8);
        bitstream_put_ui(bs, seq_param->sequence_extension.bits.low_delay, 1);
        bitstream_put_ui(bs, seq_param->sequence_extension.bits.frame_rate_extension_n, 2);
        bitstream_put_ui(bs, seq_param->sequence_extension.bits.frame_rate_extension_d, 5);

        bitstream_byte_aligning(bs, 0);
    }

    if (ctx->new_gop_header) {
        bitstream_put_ui(bs, START_CODE_GOP, 32);
        bitstream_put_ui(bs, seq_param->gop_header.bits.time_code, 25);
        bitstream_put_ui(bs, seq_param->gop_header.bits.closed_gop, 1);
        bitstream_put_ui(bs, seq_param->gop_header.bits.broken_link, 1);

        bitstream_byte_aligning(bs, 0);
    }
}

static void 
pps_rbsp(const VAEncSequenceParameterBufferMPEG2 *seq_param,
         const VAEncPictureParameterBufferMPEG2 *pic_param,
         bitstream *bs)
{
    int chroma_420_type;

    if (seq_param->sequence_extension.bits.chroma_format == CHROMA_FORMAT_420)
        chroma_420_type = pic_param->picture_coding_extension.bits.progressive_frame;
    else
        chroma_420_type = 0;

    bitstream_put_ui(bs, START_CODE_PICUTRE, 32);
    bitstream_put_ui(bs, pic_param->temporal_reference, 10);
    bitstream_put_ui(bs,
                     pic_param->picture_type == VAEncPictureTypeIntra ? 1 :
                     pic_param->picture_type == VAEncPictureTypePredictive ? 2 : 3,
                     3);
    bitstream_put_ui(bs, 0xFFFF, 16); /* vbv_delay, always 0xFFFF */
    
    if (pic_param->picture_type == VAEncPictureTypePredictive ||
        pic_param->picture_type == VAEncPictureTypeBidirectional) {
        bitstream_put_ui(bs, 0, 1); /* full_pel_forward_vector, always 0 for MPEG-2 */
        bitstream_put_ui(bs, 7, 3); /* forward_f_code, always 7 for MPEG-2 */
    }

    if (pic_param->picture_type == VAEncPictureTypeBidirectional) {
        bitstream_put_ui(bs, 0, 1); /* full_pel_backward_vector, always 0 for MPEG-2 */
        bitstream_put_ui(bs, 7, 3); /* backward_f_code, always 7 for MPEG-2 */
    }
     
    bitstream_put_ui(bs, 0, 1); /* extra_bit_picture, 0 */

    bitstream_byte_aligning(bs, 0);

    bitstream_put_ui(bs, START_CODE_EXT, 32);
    bitstream_put_ui(bs, 8, 4); /* Picture Coding Extension ID: 8 */
    bitstream_put_ui(bs, pic_param->f_code[0][0], 4);
    bitstream_put_ui(bs, pic_param->f_code[0][1], 4);
    bitstream_put_ui(bs, pic_param->f_code[1][0], 4);
    bitstream_put_ui(bs, pic_param->f_code[1][1], 4);

    bitstream_put_ui(bs, pic_param->picture_coding_extension.bits.intra_dc_precision, 2);
    bitstream_put_ui(bs, pic_param->picture_coding_extension.bits.picture_structure, 2);
    bitstream_put_ui(bs, pic_param->picture_coding_extension.bits.top_field_first, 1);
    bitstream_put_ui(bs, pic_param->picture_coding_extension.bits.frame_pred_frame_dct, 1);
    bitstream_put_ui(bs, pic_param->picture_coding_extension.bits.concealment_motion_vectors, 1);
    bitstream_put_ui(bs, pic_param->picture_coding_extension.bits.q_scale_type, 1);
    bitstream_put_ui(bs, pic_param->picture_coding_extension.bits.intra_vlc_format, 1);
    bitstream_put_ui(bs, pic_param->picture_coding_extension.bits.alternate_scan, 1);
    bitstream_put_ui(bs, pic_param->picture_coding_extension.bits.repeat_first_field, 1);
    bitstream_put_ui(bs, chroma_420_type, 1);
    bitstream_put_ui(bs, pic_param->picture_coding_extension.bits.progressive_frame, 1);
    bitstream_put_ui(bs, pic_param->picture_coding_extension.bits.composite_display_flag, 1);

    bitstream_byte_aligning(bs, 0);
}

static int
build_packed_pic_buffer(const VAEncSequenceParameterBufferMPEG2 *seq_param,
                        const VAEncPictureParameterBufferMPEG2 *pic_param,
                        unsigned char **header_buffer)
{
    bitstream bs;

    bitstream_start(&bs);
    pps_rbsp(seq_param, pic_param, &bs);
    bitstream_end(&bs);

    *header_buffer = (unsigned char *)bs.buffer;
    return bs.bit_offset;
}

static int
build_packed_seq_buffer(struct mpeg2enc_context *ctx,
                        const VAEncSequenceParameterBufferMPEG2 *seq_param,
                        unsigned char **header_buffer)
{
    bitstream bs;

    bitstream_start(&bs);
    sps_rbsp(ctx, seq_param, &bs);
    bitstream_end(&bs);

    *header_buffer = (unsigned char *)bs.buffer;
    return bs.bit_offset;
}

/*
 * mpeg2enc
 */
#define SID_INPUT_PICTURE_0                     0
#define SID_INPUT_PICTURE_1                     1
#define SID_REFERENCE_PICTURE_L0                2
#define SID_REFERENCE_PICTURE_L1                3
#define SID_RECON_PICTURE                       4
#define SID_NUMBER                              SID_RECON_PICTURE + 1

static VASurfaceID surface_ids[SID_NUMBER];

/*
 * upload thread function
 */
static void *
upload_yuv_to_surface(void *data)
{
    struct mpeg2enc_context *ctx = data;
    VAImage surface_image;
    VAStatus va_status;
    void *surface_p = NULL;
    unsigned char *y_src, *u_src, *v_src;
    unsigned char *y_dst, *u_dst, *v_dst;
    int y_size = ctx->width * ctx->height;
    int u_size = (ctx->width >> 1) * (ctx->height >> 1);
    int row, col;
    size_t n_items;

    do {
        n_items = fread(ctx->frame_data_buffer, ctx->frame_size, 1, ctx->ifp);
    } while (n_items != 1);

    va_status = vaDeriveImage(ctx->va_dpy, surface_ids[ctx->current_upload_surface], &surface_image);
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

    return NULL;
}

static void 
mpeg2enc_exit(struct mpeg2enc_context *ctx, int exit_code)
{
    if (ctx->frame_data_buffer) {
        free(ctx->frame_data_buffer);
        ctx->frame_data_buffer = NULL;
    }

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
usage(char *program)
{   
    fprintf(stderr, "Usage: %s --help\n", program);
    fprintf(stderr, "\t--help   print this message\n");
    fprintf(stderr, "Usage: %s <width> <height> <ifile> <ofile> [options]\n", program);
    fprintf(stderr, "\t<width>  specifies the frame width\n");
    fprintf(stderr, "\t<height> specifies the frame height\n");
    fprintf(stderr, "\t<ifile>  specifies the I420/IYUV YUV file\n");
    fprintf(stderr, "\t<ofile>  specifies the encoded MPEG-2 file\n");
    fprintf(stderr, "where options include:\n");
    fprintf(stderr, "\t--cqp <QP>       const qp mode with specified <QP>\n");
    fprintf(stderr, "\t--fps <FPS>      specify the frame rate\n");
    fprintf(stderr, "\t--mode <MODE>    specify the mode 0 (I), 1 (I/P) and 2 (I/P/B)\n");
    fprintf(stderr, "\t--profile <PROFILE>      specify the profile 0(Simple), or 1(Main, default)\n");
    fprintf(stderr, "\t--level <LEVEL>  specify the level 0(Low), 1(Main, default) or 2(High)\n");    
}

void
mpeg2_profile_level(struct mpeg2enc_context *ctx,
                    int profile,
                    int level)
{
    int l = 2, p;

    for (p = profile; p < 2; p++) {
        for (l = level; l < 3; l++) {
            if (ctx->width <= mpeg2_upper_samplings[p][l].samplers_per_line &&
                ctx->height <= mpeg2_upper_samplings[p][l].line_per_frame &&
                ctx->fps <= mpeg2_upper_samplings[p][l].frame_per_sec) {
                
                goto __find;
                break;
            }
        }
    }

    if (p == 2) {
        fprintf(stderr, "Warning: can't find a proper profile and level for the specified width/height/fps\n");
        p = 1;
        l = 2;
    }

__find:    
    ctx->profile = mpeg2_va_profiles[p];
    ctx->level = l;
}

static void 
parse_args(struct mpeg2enc_context *ctx, int argc, char **argv)
{
    int c, tmp;
    int option_index = 0;
    long file_size;
    int profile = 1, level = 1;

    static struct option long_options[] = {
        {"help",        no_argument,            0,      'h'},
        {"cqp",         required_argument,      0,      'c'},
        {"fps",         required_argument,      0,      'f'},
        {"mode",        required_argument,      0,      'm'},
        {"profile",     required_argument,      0,      'p'},
        {"level",       required_argument,      0,      'l'},
        { NULL,         0,                      NULL,   0 }
    };

    if ((argc == 2 && strcmp(argv[1], "--help") == 0) ||
        (argc < 5))
        goto print_usage;

    ctx->width = atoi(argv[1]);
    ctx->height = atoi(argv[2]);

    if (ctx->width <= 0 || ctx->height <= 0) {
        fprintf(stderr, "<width> and <height> must be greater than 0\n");
        goto err_exit;
    }

    ctx->ifp = fopen(argv[3], "rb");

    if (ctx->ifp == NULL) {
        fprintf(stderr, "Can't open the input file\n");
        goto err_exit;
    }

    fseek(ctx->ifp, 0l, SEEK_END);
    file_size = ftell(ctx->ifp);
    ctx->frame_size = ctx->width * ctx->height * 3 / 2;

    if ((file_size < ctx->frame_size) ||
        (file_size % ctx->frame_size)) {
        fprintf(stderr, "The input file size %ld isn't a multiple of the frame size %d\n", file_size, ctx->frame_size);
        goto err_exit;
    }

    ctx->num_pictures = file_size / ctx->frame_size;
    fseek(ctx->ifp, 0l, SEEK_SET);
    
    ctx->ofp = fopen(argv[4], "wb");
    
    if (ctx->ofp == NULL) {
        fprintf(stderr, "Can't create the output file\n");
        goto err_exit;
    }

    opterr = 0;
    ctx->fps = 30;
    ctx->qp = 8;
    ctx->rate_control_mode = VA_RC_CQP;
    ctx->mode = MPEG2_MODE_IP;
    ctx->profile = VAProfileMPEG2Main;
    ctx->level = MPEG2_LEVEL_MAIN;

    optind = 5;

    while((c = getopt_long(argc, argv,
                           "",
                           long_options, 
                           &option_index)) != -1) {
        switch(c) {
        case 'c':
            tmp = atoi(optarg);

            /* only support q_scale_type = 0 */
            if (tmp > 62 || tmp < 2) {
                fprintf(stderr, "Warning: QP must be in [2, 62]\n");

                if (tmp > 62)
                    tmp = 62;

                if (tmp < 2)
                    tmp = 2;
            }

            ctx->qp = tmp & 0xFE;
            ctx->rate_control_mode = VA_RC_CQP;

            break;

        case 'f':
            tmp = atoi(optarg);

            if (tmp <= 0)
                fprintf(stderr, "Warning: FPS must be greater than 0\n");
            else
                ctx->fps = tmp;

            ctx->rate_control_mode = VA_RC_CBR;

            break;

        case 'm':
            tmp = atoi(optarg);
            
            if (tmp < MPEG2_MODE_I || tmp > MPEG2_MODE_IPB)
                fprintf(stderr, "Waning: MODE must be 0, 1, or 2\n");
            else
                ctx->mode = tmp;

            break;

        case 'p':
            tmp = atoi(optarg);
            
            if (tmp < 0 || tmp > 1)
                fprintf(stderr, "Waning: PROFILE must be 0 or 1\n");
            else
                profile = tmp;

            break;

        case 'l':
            tmp = atoi(optarg);
            
            if (tmp < MPEG2_LEVEL_LOW || tmp > MPEG2_LEVEL_HIGH)
                fprintf(stderr, "Waning: LEVEL must be 0, 1, or 2\n");
            else
                level = tmp;

            break;

        case '?':
            fprintf(stderr, "Error: unkown command options\n");

        case 'h':
            goto print_usage;
        }
    }

    mpeg2_profile_level(ctx, profile, level);

    return;

print_usage:    
    usage(argv[0]);
err_exit:
    mpeg2enc_exit(ctx, 1);
}

/*
 * init
 */
void
mpeg2enc_init_sequence_parameter(struct mpeg2enc_context *ctx,
                                VAEncSequenceParameterBufferMPEG2 *seq_param)
{
    int profile = 4, level = 8;

    switch (ctx->profile) {
    case VAProfileMPEG2Simple:
        profile = 5;
        break;

    case VAProfileMPEG2Main:
        profile = 4;
        break;

    default:
        assert(0);
        break;
    }

    switch (ctx->level) {
    case MPEG2_LEVEL_LOW:
        level = 10;
        break;

    case MPEG2_LEVEL_MAIN:
        level = 8;
        break;

    case MPEG2_LEVEL_HIGH:
        level = 4;
        break;

    default:
        assert(0);
        break;
    }
        
    seq_param->intra_period = ctx->intra_period;
    seq_param->ip_period = ctx->ip_period;   /* FIXME: ??? */
    seq_param->picture_width = ctx->width;
    seq_param->picture_height = ctx->height;

    if (ctx->bit_rate > 0)
        seq_param->bits_per_second = 1024 * ctx->bit_rate; /* use kbps as input */
    else
        seq_param->bits_per_second = 0x3FFFF * 400;

    seq_param->frame_rate = ctx->fps;
    seq_param->aspect_ratio_information = 1;
    seq_param->vbv_buffer_size = 3; /* B = 16 * 1024 * vbv_buffer_size */

    seq_param->sequence_extension.bits.profile_and_level_indication = profile << 4 | level;
    seq_param->sequence_extension.bits.progressive_sequence = 1; /* progressive frame-pictures */
    seq_param->sequence_extension.bits.chroma_format = CHROMA_FORMAT_420; /* 4:2:0 */
    seq_param->sequence_extension.bits.low_delay = 0; /* FIXME */
    seq_param->sequence_extension.bits.frame_rate_extension_n = 0;
    seq_param->sequence_extension.bits.frame_rate_extension_d = 0;

    seq_param->gop_header.bits.time_code = (1 << 12); /* bit12: marker_bit */
    seq_param->gop_header.bits.closed_gop = 0;
    seq_param->gop_header.bits.broken_link = 0;    
}

static void
mpeg2enc_init_picture_parameter(struct mpeg2enc_context *ctx,
                               VAEncPictureParameterBufferMPEG2 *pic_param)
{
    pic_param->forward_reference_picture = VA_INVALID_ID;
    pic_param->backward_reference_picture = VA_INVALID_ID;
    pic_param->reconstructed_picture = VA_INVALID_ID;
    pic_param->coded_buf = VA_INVALID_ID;
    pic_param->picture_type = VAEncPictureTypeIntra;

    pic_param->temporal_reference = 0;
    pic_param->f_code[0][0] = 0xf;
    pic_param->f_code[0][1] = 0xf;
    pic_param->f_code[1][0] = 0xf;
    pic_param->f_code[1][1] = 0xf;

    pic_param->picture_coding_extension.bits.intra_dc_precision = 0; /* 8bits */
    pic_param->picture_coding_extension.bits.picture_structure = 3; /* frame picture */
    pic_param->picture_coding_extension.bits.top_field_first = 0; 
    pic_param->picture_coding_extension.bits.frame_pred_frame_dct = 1; /* FIXME */
    pic_param->picture_coding_extension.bits.concealment_motion_vectors = 0;
    pic_param->picture_coding_extension.bits.q_scale_type = 0;
    pic_param->picture_coding_extension.bits.intra_vlc_format = 0;
    pic_param->picture_coding_extension.bits.alternate_scan = 0;
    pic_param->picture_coding_extension.bits.repeat_first_field = 0;
    pic_param->picture_coding_extension.bits.progressive_frame = 1;
    pic_param->picture_coding_extension.bits.composite_display_flag = 0;
}

static void 
mpeg2enc_alloc_va_resources(struct mpeg2enc_context *ctx)
{
    VAEntrypoint *entrypoint_list;
    VAConfigAttrib attrib_list[2];
    VAStatus va_status;
    int max_entrypoints, num_entrypoints, entrypoint;
    int major_ver, minor_ver;

    ctx->va_dpy = va_open_display();
    va_status = vaInitialize(ctx->va_dpy,
                             &major_ver,
                             &minor_ver);
    CHECK_VASTATUS(va_status, "vaInitialize");

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
        assert(0);
    }

    /* find out the format for the render target, and rate control mode */
    attrib_list[0].type = VAConfigAttribRTFormat;
    attrib_list[1].type = VAConfigAttribRateControl;
    vaGetConfigAttributes(ctx->va_dpy,
                          ctx->profile,
                          VAEntrypointEncSlice,
                          &attrib_list[0],
                          2);

    if ((attrib_list[0].value & VA_RT_FORMAT_YUV420) == 0) {
        /* not find desired YUV420 RT format */
        assert(0);
    }

    if ((attrib_list[1].value & ctx->rate_control_mode) == 0) {
        /* Can't find matched RC mode */
        fprintf(stderr, "RC mode %d isn't found, exit\n", ctx->rate_control_mode);
        assert(0);
    }

    attrib_list[0].value = VA_RT_FORMAT_YUV420; /* set to desired RT format */
    attrib_list[1].value = ctx->rate_control_mode; /* set to desired RC mode */

    va_status = vaCreateConfig(ctx->va_dpy,
                               ctx->profile,
                               VAEntrypointEncSlice,
                               attrib_list,
                               2,
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
                                 surface_ids,
                                 SID_NUMBER,
                                 NULL,
                                 0);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");
}

static void 
mpeg2enc_init(struct mpeg2enc_context *ctx)
{
    int i;

    ctx->frame_data_buffer = (unsigned char *)malloc(ctx->frame_size);
    ctx->seq_param_buf_id = VA_INVALID_ID;
    ctx->pic_param_buf_id = VA_INVALID_ID;
    ctx->packed_seq_header_param_buf_id = VA_INVALID_ID;
    ctx->packed_seq_buf_id = VA_INVALID_ID;
    ctx->packed_pic_header_param_buf_id = VA_INVALID_ID;
    ctx->packed_pic_buf_id = VA_INVALID_ID;
    ctx->codedbuf_buf_id = VA_INVALID_ID;
    ctx->codedbuf_i_size = ctx->frame_size;
    ctx->codedbuf_pb_size = 0;
    ctx->next_display_order = 0;
    ctx->next_type = VAEncPictureTypeIntra;

    if (ctx->mode == MPEG2_MODE_I) {
        ctx->intra_period = 1;
        ctx->ip_period = 0;
    } else if (ctx->mode == MPEG2_MODE_IP) {
        ctx->intra_period = 16;
        ctx->ip_period = 0;
    } else {
        ctx->intra_period = 16;
        ctx->ip_period = 2;
    }

    ctx->next_bframes = ctx->ip_period;

    ctx->new_sequence = 1;
    ctx->new_gop_header = 1;
    ctx->gop_header_in_display_order = 0;

    ctx->bit_rate = -1;

    for (i = 0; i < MAX_SLICES; i++) {
        ctx->slice_param_buf_id[i] = VA_INVALID_ID;
    }

    mpeg2enc_init_sequence_parameter(ctx, &ctx->seq_param);
    mpeg2enc_init_picture_parameter(ctx, &ctx->pic_param);
    mpeg2enc_alloc_va_resources(ctx);

    /* thread */
    ctx->current_input_surface = SID_INPUT_PICTURE_0;
    ctx->current_upload_surface = SID_INPUT_PICTURE_1;
    ctx->upload_thread_value = pthread_create(&ctx->upload_thread_id,
                                              NULL,
                                              upload_yuv_to_surface,
                                              ctx);
}

static int 
mpeg2enc_time_code(VAEncSequenceParameterBufferMPEG2 *seq_param,
                   int num_frames)
{
    int fps = (int)(seq_param->frame_rate + 0.5);
    int time_code = 0;
    int time_code_pictures, time_code_seconds, time_code_minutes, time_code_hours;
    int drop_frame_flag = 0;

    assert(fps <= 60);

    time_code_seconds = num_frames / fps;
    time_code_pictures = num_frames % fps;
    time_code |= time_code_pictures;

    time_code_minutes = time_code_seconds / 60;
    time_code_seconds = time_code_seconds % 60;
    time_code |= (time_code_seconds << 6);

    time_code_hours = time_code_minutes / 60;
    time_code_minutes = time_code_minutes % 60;

    time_code |= (1 << 12);     /* marker_bit */
    time_code |= (time_code_minutes << 13);

    time_code_hours = time_code_hours % 24;
    time_code |= (time_code_hours << 19);

    time_code |= (drop_frame_flag << 24);

    return time_code;
}

/*
 * run
 */
static void
mpeg2enc_update_sequence_parameter(struct mpeg2enc_context *ctx,
                                   VAEncPictureType picture_type,
                                   int coded_order,
                                   int display_order)
{
    VAEncSequenceParameterBufferMPEG2 *seq_param = &ctx->seq_param;

    /* update the time_code info for the new GOP */
    if (ctx->new_gop_header) {
        seq_param->gop_header.bits.time_code = mpeg2enc_time_code(seq_param, display_order);
    }
}

static void
mpeg2enc_update_picture_parameter(struct mpeg2enc_context *ctx,
                                  VAEncPictureType picture_type,
                                  int coded_order,
                                  int display_order)
{
    VAEncPictureParameterBufferMPEG2 *pic_param = &ctx->pic_param;
    uint8_t f_code_x, f_code_y;

    pic_param->picture_type = picture_type;
    pic_param->temporal_reference = (display_order - ctx->gop_header_in_display_order) & 0x3FF;
    pic_param->reconstructed_picture = surface_ids[SID_RECON_PICTURE];
    pic_param->forward_reference_picture = surface_ids[SID_REFERENCE_PICTURE_L0];
    pic_param->backward_reference_picture = surface_ids[SID_REFERENCE_PICTURE_L1];

    f_code_x = 0xf;
    f_code_y = 0xf;
    if (pic_param->picture_type != VAEncPictureTypeIntra) {
	if (ctx->level == MPEG2_LEVEL_LOW) {
		f_code_x = 7;
		f_code_y = 4;
	} else if (ctx->level == MPEG2_LEVEL_MAIN) {
		f_code_x = 8;
		f_code_y = 5;
	} else {
		f_code_x = 9;
		f_code_y = 5;
	}
    }
    
    if (pic_param->picture_type == VAEncPictureTypeIntra) {
        pic_param->f_code[0][0] = 0xf;
        pic_param->f_code[0][1] = 0xf;
        pic_param->f_code[1][0] = 0xf;
        pic_param->f_code[1][1] = 0xf;
        pic_param->forward_reference_picture = VA_INVALID_SURFACE;
        pic_param->backward_reference_picture = VA_INVALID_SURFACE;

    } else if (pic_param->picture_type == VAEncPictureTypePredictive) {
        pic_param->f_code[0][0] = f_code_x;
        pic_param->f_code[0][1] = f_code_y;
        pic_param->f_code[1][0] = 0xf;
        pic_param->f_code[1][1] = 0xf;
        pic_param->forward_reference_picture = surface_ids[SID_REFERENCE_PICTURE_L0];
        pic_param->backward_reference_picture = VA_INVALID_SURFACE;
    } else if (pic_param->picture_type == VAEncPictureTypeBidirectional) {
        pic_param->f_code[0][0] = f_code_x;
        pic_param->f_code[0][1] = f_code_y;
        pic_param->f_code[1][0] = f_code_x;
        pic_param->f_code[1][1] = f_code_y;
        pic_param->forward_reference_picture = surface_ids[SID_REFERENCE_PICTURE_L0];
        pic_param->backward_reference_picture = surface_ids[SID_REFERENCE_PICTURE_L1];
    } else {
        assert(0);
    }
}

static void
mpeg2enc_update_picture_parameter_buffer(struct mpeg2enc_context *ctx,
                                         VAEncPictureType picture_type,
                                         int coded_order,
                                         int display_order)
{
    VAEncPictureParameterBufferMPEG2 *pic_param = &ctx->pic_param;
    VAStatus va_status;

    /* update the coded buffer id */
    pic_param->coded_buf = ctx->codedbuf_buf_id;
    va_status = vaCreateBuffer(ctx->va_dpy,
                               ctx->context_id,
                               VAEncPictureParameterBufferType,
                               sizeof(*pic_param),
                               1,
                               pic_param,
                               &ctx->pic_param_buf_id);
    CHECK_VASTATUS(va_status, "vaCreateBuffer");
}

static void 
mpeg2enc_update_slice_parameter(struct mpeg2enc_context *ctx, VAEncPictureType picture_type)
{
    VAEncSequenceParameterBufferMPEG2 *seq_param;
    VAEncPictureParameterBufferMPEG2 *pic_param;
    VAEncSliceParameterBufferMPEG2 *slice_param;
    VAStatus va_status;
    int i, width_in_mbs, height_in_mbs;

    pic_param = &ctx->pic_param;
    assert(pic_param->picture_coding_extension.bits.q_scale_type == 0);

    seq_param = &ctx->seq_param;
    width_in_mbs = (seq_param->picture_width + 15) / 16;
    height_in_mbs = (seq_param->picture_height + 15) / 16;
    ctx->num_slice_groups = 1;

    for (i = 0; i < height_in_mbs; i++) {
        slice_param = &ctx->slice_param[i];
        slice_param->macroblock_address = i * width_in_mbs;
        slice_param->num_macroblocks = width_in_mbs;
        slice_param->is_intra_slice = (picture_type == VAEncPictureTypeIntra);
        slice_param->quantiser_scale_code = ctx->qp / 2;
    }

    va_status = vaCreateBuffer(ctx->va_dpy,
                               ctx->context_id,
                               VAEncSliceParameterBufferType,
                               sizeof(*slice_param),
                               height_in_mbs,
                               ctx->slice_param,
                               ctx->slice_param_buf_id);
    CHECK_VASTATUS(va_status, "vaCreateBuffer");;
}

static int 
begin_picture(struct mpeg2enc_context *ctx,
              int coded_order,
              int display_order,
              VAEncPictureType picture_type)
{
    VAStatus va_status;
    int tmp;
    VAEncPackedHeaderParameterBuffer packed_header_param_buffer;
    unsigned int length_in_bits;
    unsigned char *packed_seq_buffer = NULL, *packed_pic_buffer = NULL;

    if (ctx->upload_thread_value != 0) {
        fprintf(stderr, "FATAL error!!!\n");
        exit(1);
    }
    
    pthread_join(ctx->upload_thread_id, NULL);

    ctx->upload_thread_value = -1;
    tmp = ctx->current_input_surface;
    ctx->current_input_surface = ctx->current_upload_surface;
    ctx->current_upload_surface = tmp;

    mpeg2enc_update_sequence_parameter(ctx, picture_type, coded_order, display_order);
    mpeg2enc_update_picture_parameter(ctx, picture_type, coded_order, display_order);

    if (ctx->new_sequence || ctx->new_gop_header) {
        assert(picture_type == VAEncPictureTypeIntra);
        length_in_bits = build_packed_seq_buffer(ctx, &ctx->seq_param, &packed_seq_buffer);
        packed_header_param_buffer.type = VAEncPackedHeaderMPEG2_SPS;
        packed_header_param_buffer.has_emulation_bytes = 0;
        packed_header_param_buffer.bit_length = length_in_bits;
        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncPackedHeaderParameterBufferType,
                                   sizeof(packed_header_param_buffer), 1, &packed_header_param_buffer,
                                   &ctx->packed_seq_header_param_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncPackedHeaderDataBufferType,
                                   (length_in_bits + 7) / 8, 1, packed_seq_buffer,
                                   &ctx->packed_seq_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        free(packed_seq_buffer);
    }

    length_in_bits = build_packed_pic_buffer(&ctx->seq_param, &ctx->pic_param, &packed_pic_buffer);
    packed_header_param_buffer.type = VAEncPackedHeaderMPEG2_PPS;
    packed_header_param_buffer.has_emulation_bytes = 0;
    packed_header_param_buffer.bit_length = length_in_bits;

    va_status = vaCreateBuffer(ctx->va_dpy,
                               ctx->context_id,
                               VAEncPackedHeaderParameterBufferType,
                               sizeof(packed_header_param_buffer), 1, &packed_header_param_buffer,
                               &ctx->packed_pic_header_param_buf_id);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");

    va_status = vaCreateBuffer(ctx->va_dpy,
                               ctx->context_id,
                               VAEncPackedHeaderDataBufferType,
                               (length_in_bits + 7) / 8, 1, packed_pic_buffer,
                               &ctx->packed_pic_buf_id);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");

    free(packed_pic_buffer);

    /* sequence parameter set */
    VAEncSequenceParameterBufferMPEG2 *seq_param = &ctx->seq_param;
    va_status = vaCreateBuffer(ctx->va_dpy,
                               ctx->context_id,
                               VAEncSequenceParameterBufferType,
                               sizeof(*seq_param),
                               1,
                               seq_param,
                               &ctx->seq_param_buf_id);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");;

    /* slice parameter */
    mpeg2enc_update_slice_parameter(ctx, picture_type);

    return 0;
}

static int 
mpeg2enc_render_picture(struct mpeg2enc_context *ctx)
{
    VAStatus va_status;
    VABufferID va_buffers[16];
    unsigned int num_va_buffers = 0;

    va_buffers[num_va_buffers++] = ctx->seq_param_buf_id;
    va_buffers[num_va_buffers++] = ctx->pic_param_buf_id;

    if (ctx->packed_seq_header_param_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->packed_seq_header_param_buf_id;

    if (ctx->packed_seq_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->packed_seq_buf_id;

    if (ctx->packed_pic_header_param_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->packed_pic_header_param_buf_id;

    if (ctx->packed_pic_buf_id != VA_INVALID_ID)
        va_buffers[num_va_buffers++] = ctx->packed_pic_buf_id;

    va_status = vaBeginPicture(ctx->va_dpy,
                               ctx->context_id,
                               surface_ids[ctx->current_input_surface]);
    CHECK_VASTATUS(va_status,"vaBeginPicture");

    va_status = vaRenderPicture(ctx->va_dpy,
                                ctx->context_id,
                                va_buffers,
                                num_va_buffers);
    CHECK_VASTATUS(va_status,"vaRenderPicture");

    va_status = vaRenderPicture(ctx->va_dpy,
                                ctx->context_id,
                                &ctx->slice_param_buf_id[0],
                                ctx->num_slice_groups);
    CHECK_VASTATUS(va_status,"vaRenderPicture");

    va_status = vaEndPicture(ctx->va_dpy, ctx->context_id);
    CHECK_VASTATUS(va_status,"vaEndPicture");

    return 0;
}

static int 
mpeg2enc_destroy_buffers(struct mpeg2enc_context *ctx, VABufferID *va_buffers, unsigned int num_va_buffers)
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

static void
end_picture(struct mpeg2enc_context *ctx, VAEncPictureType picture_type, int next_is_bpic)
{
    VABufferID tempID;

    /* Prepare for next picture */
    tempID = surface_ids[SID_RECON_PICTURE];  

    if (picture_type != VAEncPictureTypeBidirectional) {
        if (next_is_bpic) {
            surface_ids[SID_RECON_PICTURE] = surface_ids[SID_REFERENCE_PICTURE_L1]; 
            surface_ids[SID_REFERENCE_PICTURE_L1] = tempID;	
        } else {
            surface_ids[SID_RECON_PICTURE] = surface_ids[SID_REFERENCE_PICTURE_L0]; 
            surface_ids[SID_REFERENCE_PICTURE_L0] = tempID;
        }
    } else {
        if (!next_is_bpic) {
            surface_ids[SID_RECON_PICTURE] = surface_ids[SID_REFERENCE_PICTURE_L0]; 
            surface_ids[SID_REFERENCE_PICTURE_L0] = surface_ids[SID_REFERENCE_PICTURE_L1];
            surface_ids[SID_REFERENCE_PICTURE_L1] = tempID;
        }
    }

    mpeg2enc_destroy_buffers(ctx, &ctx->seq_param_buf_id, 1);
    mpeg2enc_destroy_buffers(ctx, &ctx->pic_param_buf_id, 1);
    mpeg2enc_destroy_buffers(ctx, &ctx->packed_seq_header_param_buf_id, 1);
    mpeg2enc_destroy_buffers(ctx, &ctx->packed_seq_buf_id, 1);
    mpeg2enc_destroy_buffers(ctx, &ctx->packed_pic_header_param_buf_id, 1);
    mpeg2enc_destroy_buffers(ctx, &ctx->packed_pic_buf_id, 1);
    mpeg2enc_destroy_buffers(ctx, &ctx->slice_param_buf_id[0], ctx->num_slice_groups);
    mpeg2enc_destroy_buffers(ctx, &ctx->codedbuf_buf_id, 1);
    memset(ctx->slice_param, 0, sizeof(ctx->slice_param));
    ctx->num_slice_groups = 0;
}

static int
store_coded_buffer(struct mpeg2enc_context *ctx, VAEncPictureType picture_type)
{
    VACodedBufferSegment *coded_buffer_segment;
    unsigned char *coded_mem;
    int slice_data_length;
    VAStatus va_status;
    VASurfaceStatus surface_status;
    size_t w_items;

    va_status = vaSyncSurface(ctx->va_dpy, surface_ids[ctx->current_input_surface]);
    CHECK_VASTATUS(va_status,"vaSyncSurface");

    surface_status = 0;
    va_status = vaQuerySurfaceStatus(ctx->va_dpy, surface_ids[ctx->current_input_surface], &surface_status);
    CHECK_VASTATUS(va_status,"vaQuerySurfaceStatus");

    va_status = vaMapBuffer(ctx->va_dpy, ctx->codedbuf_buf_id, (void **)(&coded_buffer_segment));
    CHECK_VASTATUS(va_status,"vaMapBuffer");
    coded_mem = coded_buffer_segment->buf;

    if (coded_buffer_segment->status & VA_CODED_BUF_STATUS_SLICE_OVERFLOW_MASK) {
        if (picture_type == VAEncPictureTypeIntra)
            ctx->codedbuf_i_size *= 2;
        else
            ctx->codedbuf_pb_size *= 2;

        vaUnmapBuffer(ctx->va_dpy, ctx->codedbuf_buf_id);
        return -1;
    }

    slice_data_length = coded_buffer_segment->size;

    do {
        w_items = fwrite(coded_mem, slice_data_length, 1, ctx->ofp);
    } while (w_items != 1);

    if (picture_type == VAEncPictureTypeIntra) {
        if (ctx->codedbuf_i_size > slice_data_length * 3 / 2) {
            ctx->codedbuf_i_size = slice_data_length * 3 / 2;
        }
        
        if (ctx->codedbuf_pb_size < slice_data_length) {
            ctx->codedbuf_pb_size = slice_data_length;
        }
    } else {
        if (ctx->codedbuf_pb_size > slice_data_length * 3 / 2) {
            ctx->codedbuf_pb_size = slice_data_length * 3 / 2;
        }
    }

    vaUnmapBuffer(ctx->va_dpy, ctx->codedbuf_buf_id);

    return 0;
}

static void
encode_picture(struct mpeg2enc_context *ctx,
               int coded_order,
               int display_order,
               VAEncPictureType picture_type,
               int next_is_bpic,
               int next_display_order)
{
    VAStatus va_status;
    int ret = 0, codedbuf_size;
    
    begin_picture(ctx, coded_order, display_order, picture_type);

    if (1) {
        /* upload YUV data to VA surface for next frame */
        if (next_display_order >= ctx->num_pictures)
            next_display_order = ctx->num_pictures - 1;

        fseek(ctx->ifp, ctx->frame_size * next_display_order, SEEK_SET);
        ctx->upload_thread_value = pthread_create(&ctx->upload_thread_id,
                                                  NULL,
                                                  upload_yuv_to_surface,
                                                  ctx);
    }

    do {
        mpeg2enc_destroy_buffers(ctx, &ctx->codedbuf_buf_id, 1);
        mpeg2enc_destroy_buffers(ctx, &ctx->pic_param_buf_id, 1);


        if (VAEncPictureTypeIntra == picture_type) {
            codedbuf_size = ctx->codedbuf_i_size;
        } else {
            codedbuf_size = ctx->codedbuf_pb_size;
        }

        /* coded buffer */
        va_status = vaCreateBuffer(ctx->va_dpy,
                                   ctx->context_id,
                                   VAEncCodedBufferType,
                                   codedbuf_size, 1, NULL,
                                   &ctx->codedbuf_buf_id);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");

        /* picture parameter set */
        mpeg2enc_update_picture_parameter_buffer(ctx, picture_type, coded_order, display_order);

        mpeg2enc_render_picture(ctx);

        ret = store_coded_buffer(ctx, picture_type);
    } while (ret);

    end_picture(ctx, picture_type, next_is_bpic);
}

static void
update_next_frame_info(struct mpeg2enc_context *ctx,
                       VAEncPictureType curr_type,
                       int curr_coded_order,
                       int curr_display_order)
{
    if (((curr_coded_order + 1) % ctx->intra_period) == 0) {
        ctx->next_type = VAEncPictureTypeIntra;
        ctx->next_display_order = curr_coded_order + 1;
        
        return;
    }

    if (curr_type == VAEncPictureTypeIntra) {
        assert(curr_display_order == curr_coded_order);
        ctx->next_type = VAEncPictureTypePredictive;
        ctx->next_bframes = ctx->ip_period;
        ctx->next_display_order = curr_display_order + ctx->next_bframes + 1;
    } else if (curr_type == VAEncPictureTypePredictive) {
        if (ctx->ip_period == 0) {
            assert(curr_display_order == curr_coded_order);
            ctx->next_type = VAEncPictureTypePredictive;
            ctx->next_display_order = curr_display_order + 1;
        } else {
            ctx->next_type = VAEncPictureTypeBidirectional;
            ctx->next_display_order = curr_display_order - ctx->next_bframes;
            ctx->next_bframes--;
        }
    } else if (curr_type == VAEncPictureTypeBidirectional) {
        if (ctx->next_bframes == 0) {
            ctx->next_type = VAEncPictureTypePredictive;
            ctx->next_bframes = ctx->ip_period;
            ctx->next_display_order = curr_display_order + ctx->next_bframes + 2;
        } else {
            ctx->next_type = VAEncPictureTypeBidirectional;
            ctx->next_display_order = curr_display_order + 1;
            ctx->next_bframes--;
        }
    }

    if (ctx->next_display_order >= ctx->num_pictures) {
        int rtmp = ctx->next_display_order - (ctx->num_pictures - 1);
        ctx->next_display_order = ctx->num_pictures - 1;
        ctx->next_bframes -= rtmp;
    }
}

static void
mpeg2enc_run(struct mpeg2enc_context *ctx)
{
    int display_order = 0, coded_order = 0;
    VAEncPictureType type;

    ctx->new_sequence = 1;
    ctx->new_gop_header = 1;
    ctx->gop_header_in_display_order = display_order;

    while (coded_order < ctx->num_pictures) {
        type = ctx->next_type;
        display_order = ctx->next_display_order;
        /* follow the IPBxxBPBxxB mode */
        update_next_frame_info(ctx, type, coded_order, display_order);
        encode_picture(ctx,
                       coded_order,
                       display_order,
                       type,
                       ctx->next_type == VAEncPictureTypeBidirectional,
                       ctx->next_display_order);

        /* update gop_header */
        ctx->new_sequence = 0;
        ctx->new_gop_header = ctx->next_type == VAEncPictureTypeIntra;

        if (ctx->new_gop_header)
            ctx->gop_header_in_display_order += ctx->intra_period;

        coded_order++;

        fprintf(stderr, "\r %d/%d ...", coded_order, ctx->num_pictures);
        fflush(stdout);
    }
}

/*
 * end
 */
static void
mpeg2enc_release_va_resources(struct mpeg2enc_context *ctx)
{
    vaDestroySurfaces(ctx->va_dpy, surface_ids, SID_NUMBER);	
    vaDestroyContext(ctx->va_dpy, ctx->context_id);
    vaDestroyConfig(ctx->va_dpy, ctx->config_id);
    vaTerminate(ctx->va_dpy);
    va_close_display(ctx->va_dpy);
}

static void
mpeg2enc_end(struct mpeg2enc_context *ctx)
{
    pthread_join(ctx->upload_thread_id, NULL);
    mpeg2enc_release_va_resources(ctx);
}

int 
main(int argc, char *argv[])
{
    struct mpeg2enc_context ctx;
    struct timeval tpstart, tpend; 
    float timeuse;

    gettimeofday(&tpstart, NULL);

    memset(&ctx, 0, sizeof(ctx));
    parse_args(&ctx, argc, argv);
    mpeg2enc_init(&ctx);
    mpeg2enc_run(&ctx);
    mpeg2enc_end(&ctx);

    gettimeofday(&tpend, NULL);
    timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
    timeuse /= 1000000;
    fprintf(stderr, "\ndone!\n");
    fprintf(stderr, "encode %d frames in %f secondes, FPS is %.1f\n", ctx.num_pictures, timeuse, ctx.num_pictures / timeuse);

    mpeg2enc_exit(&ctx, 0);

    return 0;
}
