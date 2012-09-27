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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
* Example based on Simple AVC encoder.
* http://cgit.freedesktop.org/libva/tree/test/encode/avcenc.c
*
*/

#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cassert>
#include <va/va_x11.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <iostream>
#include <cstdlib>

#include "TCPSocketClient.h"

#define MY_Y 0x22
#define MY_U 0xff
#define MY_V 0x55


extern TCPSocketClient *sock_ptr;
extern int  g_Debug;
extern char *device_settings;
Window  win2;
int win2_width = 640;
int win2_height = 480;
int g_PX = -1;
int g_PY = -1;


bool g_LiveView = true;
bool g_Force_P_Only = false;
bool g_ShowNumber = true;




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
        std::cerr << __func__ << ':' << func << '(' << __LINE__ << ") failed, exit\n"; \
        exit(1);                                                        \
    }

static Display *x11_display;
static VADisplay va_dpy;
static VAContextID context_id;
static VAConfigID config_id;

static int picture_width, picture_width_in_mbs;
static int picture_height, picture_height_in_mbs;
static int frame_size;
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
    vaQueryConfigEntrypoints(va_dpy, VAProfileH264Baseline, entrypoints, &num_entrypoints);

    for (slice_entrypoint = 0; slice_entrypoint < num_entrypoints; slice_entrypoint++) {
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
    vaGetConfigAttributes(va_dpy, VAProfileH264Baseline, VAEntrypointEncSlice, &attrib[0], 2);

    if ((attrib[0].value & VA_RT_FORMAT_YUV420) == 0) {
        /* not find desired YUV420 RT format */
        assert(0);
    }

    if ((attrib[1].value & VA_RC_VBR) == 0) {
        /* Can't find matched RC mode */
        std::cerr << "VBR mode not found, exit\n";
        assert(0);
    }

    attrib[0].value = VA_RT_FORMAT_YUV420; /* set to desired RT format */
    attrib[1].value = VA_RC_VBR; /* set to desired RC mode */

    va_status = vaCreateConfig(va_dpy, VAProfileH264Baseline, VAEntrypointEncSlice, &attrib[0], 2,&config_id);
    CHECK_VASTATUS(va_status, "vaCreateConfig");

    /* Create a context for this decode pipe */
    va_status = vaCreateContext(va_dpy, config_id, picture_width, picture_height, VA_PROGRESSIVE, 0, 0, &context_id);
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
static VABufferID coded_buf = VA_INVALID_ID;                    /*Output buffer, compressed data*/

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
        VAEncSequenceParameterBufferH264 seq_h264 = {0};
        seq_h264.level_idc = 30;
        seq_h264.picture_width_in_mbs = picture_width_in_mbs;
        seq_h264.picture_height_in_mbs = picture_height_in_mbs;
        seq_h264.bits_per_second = 384*1000;
        seq_h264.initial_qp = qp_value;
        seq_h264.min_qp = 3;
        va_status = vaCreateBuffer(va_dpy, context_id, VAEncSequenceParameterBufferType,
            sizeof(seq_h264),1,&seq_h264,&seq_parameter);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");;
    }
    //2. Create surface
    va_status = vaCreateSurfaces(va_dpy, picture_width, picture_height, VA_RT_FORMAT_YUV420, SID_NUMBER, &surface_ids[0]);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");
    //3. Create coded buffer
    {
        va_status = vaCreateBuffer(va_dpy,context_id,VAEncCodedBufferType, codedbuf_size, 1, NULL, &coded_buf);
        CHECK_VASTATUS(va_status,"vaBeginPicture");
    }
}

static void release_encode_resource()
{
    //-3 Relese coded buffer
    if (coded_buf != VA_INVALID_ID)
        vaDestroyBuffer(va_dpy, coded_buf);
    //-2 Release all the surfaces resource
    vaDestroySurfaces(va_dpy, &surface_ids[0], SID_NUMBER);
    //-1 Destory the sequence level parameter
    if (seq_parameter != VA_INVALID_ID)
        vaDestroyBuffer(va_dpy, seq_parameter);
}


static int get_coded_bitsteam_length(unsigned char *buffer, int buffer_length)
{
    int i;
    for (i = buffer_length - 1; i >= 0; i--) {
        if (buffer[i])
            break;
    }
    return i + 1;
}



void SetWindowTitle(const char* title, ...)
{
    va_list args;
    va_start(args, title);
    char buf[256];
    vsprintf(buf, title, args);
    va_end(args);
    XSetStandardProperties(x11_display,win2, buf, buf, None, NULL, 0, NULL);
}



int encoder_init(int width, int height)
{
    picture_width = width;
    picture_height = height;
    picture_width_in_mbs = (picture_width + 15) / 16;
    picture_height_in_mbs = (picture_height + 15) / 16;
    qp_value = 26;
    frame_size = picture_width * picture_height +  ((picture_width * picture_height) >> 1) ;
    codedbuf_size = picture_width * picture_height * 1.5;
    create_encode_pipe();
    alloc_encode_resource();
    sock_ptr->send(picture_width);
    sock_ptr->send(picture_height);
    sock_ptr->send(picture_width_in_mbs-1);
    sock_ptr->send(picture_height_in_mbs-1);
    if (g_LiveView) {
        win2 = XCreateSimpleWindow(x11_display, RootWindow(x11_display, 0), 0, 0, win2_width, win2_height, 0, 0, WhitePixel(x11_display, 0));
        XMapWindow(x11_display, win2);
        if ((g_PX !=-1) && (g_PY !=-1)) {
            XMoveWindow(x11_display,  win2, g_PX, g_PY);
        }
        SetWindowTitle("Input: %dx%d [TCP] %s",picture_width,picture_height, device_settings);
        XSync(x11_display, False);
    }
    return 0;
}

void encoder_close()
{
    release_encode_resource();
    destory_encode_pipe();
}


/* 8x8 font 0-9 only - asm type format */
unsigned char mydigits[80] = {
        // 0
        0x0E,0x11,0x13,0x15,0x19,0x11,0x0E,0x00,
        // 1
        0x04,0x0C,0x04,0x04,0x04,0x04,0x0E,0x00,
        // 2
        0x0E,0x11,0x01,0x02,0x04,0x08,0x1F,0x00,
        // 3
        0x1F,0x02,0x04,0x02,0x01,0x11,0x0E,0x00,
        // 4
        0x02,0x06,0x0A,0x12,0x1F,0x02,0x02,0x00,
        // 5
        0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E,0x00,
        // 6
        0x06,0x08,0x10,0x1E,0x11,0x11,0x0E,0x00,
        // 7
        0x1F,0x01,0x02,0x04,0x04,0x04,0x04,0x00,
        // 8
        0x1E,0x11,0x11,0x0E,0x11,0x11,0x0E,0x00,
        //9
        0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C,0x00
};

#define INTERSIZE 16

static void ShowNumber(int num, unsigned char *buffer, VAImage *image)
{
    int j;
    char buf[20];
    unsigned char *dst_y;
    unsigned char *dst_uv_line;
    unsigned char *digits_ptr;
    assert(image);
    int maxlen = sprintf(buf, "%d", num);
    assert(maxlen<20);
    for (int a=0; a<maxlen;a++) {
        digits_ptr = &mydigits[(buf[a]-'0')*8];
        for (int i=0; i<8; i++) {
            unsigned char current = digits_ptr[i];
            dst_y = (buffer+ image->offsets[0]) + ((i*2)*image->pitches[0])+(a*INTERSIZE);
            dst_uv_line = (buffer + image->offsets[1]) + (i*image->pitches[1])+(a*INTERSIZE);
            for (j=7; j>=0;j--) {
                if ((current >>j) & 1) {
                    *dst_y ++ = MY_Y;
                    *dst_y ++ = MY_Y;
                    *dst_uv_line++ = MY_U;
                    *dst_uv_line++ = MY_V;
                } else  {
                    dst_y += 2;
                    dst_uv_line +=2;
                }
            }
            dst_y = (buffer+ image->offsets[0]) + (((i*2)+1)*image->pitches[0])+(a*INTERSIZE);
            for (j=7; j>=0;j--) {
                if ((current >>j) & 1) {
                    *dst_y ++ = MY_Y;
                    *dst_y ++ = MY_Y;
                } else  {
                    dst_y += 2;
                }
            }
        }
    }
}



static void upload_yuv_to_surface(unsigned char *inbuf, VASurfaceID surface_id, unsigned int frame)
{
    VAImage image;
    VAStatus va_status;
    void *pbuffer=NULL;
    unsigned char *psrc = inbuf;
    unsigned char *pdst = NULL;
    unsigned char *dst_y, *dst_uv;
    unsigned char *src_u, *src_v;
    unsigned char *dst_uv_line = NULL;
    int i,j;
    va_status = vaDeriveImage(va_dpy, surface_id, &image);
    va_status = vaMapBuffer(va_dpy, image.buf, &pbuffer);
    pdst = (unsigned char *)pbuffer;
    dst_uv_line = pdst + image.offsets[1];
    dst_uv = dst_uv_line;
    for (i=0; i<picture_height; i+=2) {
        dst_y = (pdst + image.offsets[0]) + i*image.pitches[0];
        for (j=0; j<(picture_width/2); ++j) {
            *(dst_y++) = psrc[0];//y1;
            *(dst_uv++) = psrc[1];//u;
            *(dst_y++) = psrc[2];//y1;
            *(dst_uv++) = psrc[3];//v;
            psrc+=4;
        }
        dst_y = (pdst + image.offsets[0]) + (i+1)*image.pitches[0];
        for (j=0; j<picture_width/2; ++j) {
            *(dst_y++) = psrc[0];//y1;
            *(dst_y++) = psrc[2];//y2;
            psrc+=4;
        }
        dst_uv_line += image.pitches[1];
        dst_uv = dst_uv_line;
    }
    if (g_ShowNumber) {
        ShowNumber(frame, (unsigned char *)pbuffer, &image);
    }
    va_status = vaUnmapBuffer(va_dpy, image.buf);
    CHECK_VASTATUS(va_status,"vaUnmapBuffer");
    va_status = vaDestroyImage(va_dpy, image.image_id);
    CHECK_VASTATUS(va_status,"vaDestroyImage");
    if (g_LiveView) {
        va_status = vaPutSurface(va_dpy, surface_id, win2, 0, 0, picture_width,picture_height, 0, 0, win2_width, win2_height, NULL, 0, VA_FRAME_PICTURE);
        CHECK_VASTATUS(va_status,"vaPutSurface");
    }
}


static void prepare_input(unsigned char *buffer, int intra_slice, unsigned int frame)
{
    static VAEncPictureParameterBufferH264 pic_h264;
    static VAEncSliceParameterBuffer slice_h264;
    VAStatus va_status;
    VABufferID tempID;
    VACodedBufferSegment *coded_buffer_segment = NULL;
    unsigned char *coded_mem;
    // Sequence level
    va_status = vaRenderPicture(va_dpy, context_id, &seq_parameter, 1);
    CHECK_VASTATUS(va_status,"vaRenderPicture");;
    // Copy Image to target surface according input YUV data.
    upload_yuv_to_surface(buffer, surface_ids[SID_INPUT_PICTURE], frame);
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
    coded_mem = (unsigned char*)coded_buffer_segment->buf;
    memset(coded_mem, 0, coded_buffer_segment->size);
    vaUnmapBuffer(va_dpy, coded_buf);
    // Slice level
    slice_h264.start_row_number = 0;
    slice_h264.slice_height = picture_height/16; /* Measured by MB */
    slice_h264.slice_flags.bits.is_intra = intra_slice;
    slice_h264.slice_flags.bits.disable_deblocking_filter_idc = 0;
    if ( slice_parameter != VA_INVALID_ID) {
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

static void send_slice_data(unsigned int frcount, int slice_type)
{
    VACodedBufferSegment *coded_buffer_segment;
    unsigned char *coded_mem;
    int i, slice_data_length;
    int mm;
    VAStatus va_status;
    VASurfaceStatus surface_status;
    int is_cabac = (entropy_coding_mode_flag == ENTROPY_MODE_CABAC);
    va_status = vaSyncSurface(va_dpy, surface_ids[SID_INPUT_PICTURE]);
    CHECK_VASTATUS(va_status,"vaSyncSurface");
    surface_status = (VASurfaceStatus)0;
    va_status = vaQuerySurfaceStatus(va_dpy, surface_ids[SID_INPUT_PICTURE], &surface_status);
    CHECK_VASTATUS(va_status,"vaQuerySurfaceStatus");
    va_status = vaMapBuffer(va_dpy, coded_buf, (void **)(&coded_buffer_segment));
    CHECK_VASTATUS(va_status,"vaMapBuffer");
    coded_mem = (unsigned char*)coded_buffer_segment->buf;
    sock_ptr->send(frcount);
    sock_ptr->send(slice_type);

    if (is_cabac) {
        if (!coded_buffer_segment->next) {
            slice_data_length = get_coded_bitsteam_length(coded_mem, codedbuf_size);
        } else {
            /* Fixme me - to do: loop to each block and calculate the real data_lenght */
            assert(0);
        }
        if (g_Debug) {
            printf("T=%d BS=%8d SZ=%8d C=%d\n", slice_type, codedbuf_size, slice_data_length, frcount);
        }sock_ptr->send(slice_data_length);
        sock_ptr->send((unsigned char*)coded_mem,  slice_data_length);
    } else {
        /* FIXME */
        assert(0);
    }
    vaUnmapBuffer(va_dpy, coded_buf);
}


int encode_frame(unsigned char *inbuf)
{
    static unsigned int framecount = 0;
    int is_intra = (framecount % 30 == 0);
    if (g_Force_P_Only) {
        is_intra = 1;
    }
    VAStatus va_status;
    va_status = vaBeginPicture(va_dpy, context_id, surface_ids[SID_INPUT_PICTURE]);
    CHECK_VASTATUS(va_status,"vaBeginPicture");
    prepare_input(inbuf, is_intra, framecount);
    va_status = vaEndPicture(va_dpy,context_id);
    CHECK_VASTATUS(va_status,"vaRenderPicture");
    send_slice_data(framecount, is_intra ? SLICE_TYPE_I : SLICE_TYPE_P);
    framecount++;
    return 1;
}

