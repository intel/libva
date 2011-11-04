/*
 * Copyright (c) 2007-2008 Intel Corporation. All Rights Reserved.
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
 * it is a real program to show how VAAPI encoding work,
 * It does H264 element stream level encoding on auto-generated YUV data
 *
 * gcc -o  h264encode  h264encode -lva -lva-x11
 * ./h264encode -w <width> -h <height> -n <frame_num>
 *
 */  
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <va/va.h>
#ifdef ANDROID
#include <va/va_android.h>
#else
#include <va/va_x11.h>
#endif

#define CHECK_VASTATUS(va_status,func)                                  \
if (va_status != VA_STATUS_SUCCESS) {                                   \
    fprintf(stderr,"%s:%s (%d) failed,exit\n", __func__, func, __LINE__); \
    exit(1);                                                            \
}

#include "../loadsurface.h"
#define SURFACE_NUM 18 /* 16 surfaces for src, 2 surface for reconstructed/reference */
#define CODEDBUF_NUM 5
static  VADisplay va_dpy;
static  VASurfaceID surface_id[SURFACE_NUM];
static  VABufferID coded_buf[CODEDBUF_NUM];
static  VAContextID context_id;
static  Display *x11_display;
static  int coded_fd;
static  char coded_file[256];
static  int frame_width,  frame_height;
static  int win_width;
static  int win_height;
static  int frame_display = 0; /* display the frame during encoding */
static  int frame_rate = 30;
static  int frame_count = 400;
static  int intra_count = 30;
static  int frame_bitrate = 8000000; /* 8M */
static  int initial_qp = 15;
static  int minimal_qp = 0;

static int display_surface(int frame_id, int *exit_encode);

static int upload_source_YUV_once_for_all()
{
    void *surface_p=NULL, *U_start,*V_start;
    VAStatus va_status;
    int box_width=8;
    int row_shift=0;
    int i;
    
    for (i=0; i<SURFACE_NUM-2; i++) {
        printf("\rLoading data into surface %d.....", i);
        upload_surface(va_dpy, surface_id[i], box_width, row_shift, 0);
        
        row_shift++;
        if (row_shift==(2*box_width)) row_shift= 0;
    }
    printf("\n");

    return 0;
}


static int save_coded_buf(VABufferID coded_buf, int current_frame, int frame_skipped)
{    
    void *coded_p=NULL;
    VACodedBufferSegment *buf_list = NULL;
    VAStatus va_status;
    unsigned int coded_size = 0;
    
    va_status = vaMapBuffer(va_dpy,coded_buf,(void **)(&buf_list));
    CHECK_VASTATUS(va_status,"vaMapBuffer");
    while (buf_list != NULL) {
        printf("Write %d bytes", buf_list->size);
        coded_size += write(coded_fd, buf_list->buf, buf_list->size);
        buf_list = (VACodedBufferSegment *) buf_list->next;
    }
    vaUnmapBuffer(va_dpy,coded_buf);

    printf("\r      "); /* return back to startpoint */
    switch (current_frame % 4) {
        case 0:
            printf("|");
            break;
        case 1:
            printf("/");
            break;
        case 2:
            printf("-");
            break;
        case 3:
            printf("\\");
            break;
    }
    printf("%08d", current_frame);
    if (current_frame % intra_count == 0)
        printf("(I)");
    else
        printf("(P)");
    
    printf("(%06d bytes coded)",coded_size);
    if (frame_skipped)
        printf("(SKipped)");
    printf("                                    ");

    return 0;
}


enum {
    SH_LEVEL_1=10,
    SH_LEVEL_1B=11,
    SH_LEVEL_2=20,
    SH_LEVEL_3=30,
    SH_LEVEL_31=31,
    SH_LEVEL_32=32,
    SH_LEVEL_4=40,
    SH_LEVEL_5=50
};

static int do_h264_encoding(void)
{
    VAEncPictureParameterBufferH264Baseline pic_h264;
    VAEncSliceParameterBuffer slice_h264;
    VAStatus va_status;
    VABufferID seq_param_buf, pic_param_buf, slice_param_buf;
    int codedbuf_size;
    VASurfaceStatus surface_status;
    int src_surface, dst_surface, ref_surface;
    int codedbuf_idx = 0;
    int frame_skipped = 0;
    int i;


    va_status = vaCreateSurfaces(va_dpy,frame_width, frame_height,
                                 VA_RT_FORMAT_YUV420, SURFACE_NUM, &surface_id[0]);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");
    
    /* upload RAW YUV data into all surfaces */
    upload_source_YUV_once_for_all();
    
    codedbuf_size = (frame_width * frame_height * 400) / (16*16);

    for (i = 0; i < CODEDBUF_NUM; i++) {
        /* create coded buffer once for all
         * other VA buffers which won't be used again after vaRenderPicture.
         * so APP can always vaCreateBuffer for every frame
         * but coded buffer need to be mapped and accessed after vaRenderPicture/vaEndPicture
         * so VA won't maintain the coded buffer
         */
        va_status = vaCreateBuffer(va_dpy,context_id,VAEncCodedBufferType,
                                   codedbuf_size, 1, NULL, &coded_buf[i]);
        CHECK_VASTATUS(va_status,"vaBeginPicture");
    }

    src_surface = 0;
    /* the last two frames are reference/reconstructed frame */
    dst_surface = SURFACE_NUM - 1;
    ref_surface = SURFACE_NUM - 2;
    
    for (i = 0; i < frame_count; i++) {
        va_status = vaBeginPicture(va_dpy, context_id, surface_id[src_surface]);
        CHECK_VASTATUS(va_status,"vaBeginPicture");

        if (i == 0) {
            VAEncSequenceParameterBufferH264Baseline seq_h264;
            VABufferID seq_param_buf;
            
            seq_h264.level_idc = SH_LEVEL_3;
            seq_h264.picture_width_in_mbs = frame_width / 16;
            seq_h264.picture_height_in_mbs = frame_height / 16;
            seq_h264.bits_per_second = frame_bitrate;
            seq_h264.frame_rate = frame_rate;
            seq_h264.initial_qp = initial_qp;
            seq_h264.min_qp = minimal_qp;
            seq_h264.basic_unit_size = 0;
            seq_h264.intra_period = intra_count;
            
            va_status = vaCreateBuffer(va_dpy, context_id,
                                       VAEncSequenceParameterBufferType,
                                       sizeof(seq_h264),1,&seq_h264,&seq_param_buf);
            CHECK_VASTATUS(va_status,"vaCreateBuffer");;

            va_status = vaRenderPicture(va_dpy,context_id, &seq_param_buf, 1);
            CHECK_VASTATUS(va_status,"vaRenderPicture");;
        }


        pic_h264.reference_picture = surface_id[ref_surface];
        pic_h264.reconstructed_picture= surface_id[dst_surface];
        pic_h264.coded_buf = coded_buf[codedbuf_idx];
        pic_h264.picture_width = frame_width;
        pic_h264.picture_height = frame_height;
        pic_h264.last_picture = (i==frame_count);
        
        va_status = vaCreateBuffer(va_dpy, context_id,VAEncPictureParameterBufferType,
                                   sizeof(pic_h264),1,&pic_h264,&pic_param_buf);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");;

        va_status = vaRenderPicture(va_dpy,context_id, &pic_param_buf, 1);
        CHECK_VASTATUS(va_status,"vaRenderPicture");

        /* one frame, one slice */
        slice_h264.start_row_number = 0;
        slice_h264.slice_height = frame_height/16; /* Measured by MB */
        slice_h264.slice_flags.bits.is_intra = ((i % intra_count) == 0);
        slice_h264.slice_flags.bits.disable_deblocking_filter_idc = 0;
        va_status = vaCreateBuffer(va_dpy,context_id,VAEncSliceParameterBufferType,
                                   sizeof(slice_h264),1,&slice_h264,&slice_param_buf);
        CHECK_VASTATUS(va_status,"vaCreateBuffer");;
        
        va_status = vaRenderPicture(va_dpy,context_id, &slice_param_buf, 1);
        CHECK_VASTATUS(va_status,"vaRenderPicture");
        
        va_status = vaEndPicture(va_dpy,context_id);
        CHECK_VASTATUS(va_status,"vaEndPicture");;

        va_status = vaSyncSurface(va_dpy, surface_id[src_surface]);
        CHECK_VASTATUS(va_status,"vaSyncSurface");

        surface_status = (VASurfaceStatus) 0;
        va_status = vaQuerySurfaceStatus(va_dpy, surface_id[src_surface],&surface_status);
        frame_skipped = (surface_status & VASurfaceSkipped);

        save_coded_buf(coded_buf[codedbuf_idx], i, frame_skipped);
        
        /* should display reconstructed frame, but just diplay source frame */
        if (frame_display) {
            int exit_encode = 0;

            display_surface(src_surface, &exit_encode);
            if (exit_encode)
                frame_count = i;
        }
        
        /* use next surface */
        src_surface++;
        if (src_surface == (SURFACE_NUM - 2))
            src_surface = 0;

        /* use next codedbuf */
        codedbuf_idx++;
        if (codedbuf_idx == (CODEDBUF_NUM - 1))
            codedbuf_idx = 0;
        
        /* if a frame is skipped, current frame still use last reference frame */
        if (frame_skipped == 0) {
            /* swap ref/dst */
            int tmp = dst_surface;
            dst_surface = ref_surface;
            ref_surface = tmp;
        } 
    }

    return 0;
}

int main(int argc,char **argv)
{
    VAEntrypoint entrypoints[5];
    int num_entrypoints,slice_entrypoint;
    VAConfigAttrib attrib[2];
    VAConfigID config_id;
    int major_ver, minor_ver;
    VAStatus va_status;
    char c;

    strcpy(coded_file, "/tmp/demo.264");
    while ((c =getopt(argc,argv,"w:h:n:p:f:r:q:s:o:d?") ) != EOF) {
        switch (c) {
                case 'w':
                    frame_width = atoi(optarg);
                    break;
                case 'h':
                    frame_height = atoi(optarg);
                    break;
                case 'n':
                    frame_count = atoi(optarg);
                    break;
                case 'p':
                    intra_count = atoi(optarg);
                    break;
                case 'f':
                    frame_rate = atoi(optarg);
                    break;
                case 'b':
                    frame_bitrate = atoi(optarg);
                    break;
                case 'q':
                    initial_qp = atoi(optarg);
                    break;
                case 's':
                    minimal_qp = atoi(optarg);
                    break;
                case 'd':
                    frame_display = 1;
                    break;
                case 'o':
                    strcpy(coded_file, optarg);
                    break;
                case ':':
                case '?':
                    printf("./h264encode <options>\n");
                    printf("   -w -h: resolution\n");
                    printf("   -n frame number\n"); 
                    printf("   -d display the source frame\n");
                    printf("   -p P frame count between two I frames\n");
                    printf("   -f frame rate\n");
                    printf("   -r bit rate\n");
                    printf("   -q initial QP\n");
                    printf("   -s maximum QP\n");
                    printf("   -o coded file\n");
                    exit(0);
        }
    }
    
#ifdef ANDROID
    x11_display = (Display*)malloc(sizeof(Display));
    *(x11_display) = 0x18c34078;
#else
    x11_display = XOpenDisplay(":0.0");
#endif
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
    
    va_status = vaCreateSurfaces(va_dpy,frame_width, frame_height,
                                 VA_RT_FORMAT_YUV420, SURFACE_NUM, &surface_id[0]);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");
    
    /* Create a context for this decode pipe */
    va_status = vaCreateContext(va_dpy, config_id,
                                frame_width, ((frame_height+15)/16)*16,
                                VA_PROGRESSIVE,&surface_id[0],SURFACE_NUM,&context_id);
    CHECK_VASTATUS(va_status, "vaCreateContext");

    /* store coded data into a file */
    coded_fd = open(coded_file,O_CREAT|O_RDWR, 0);
    if (coded_fd == -1) {
        printf("Open file %s failed, exit\n", coded_file);
        exit(1);
    }

    printf("Coded %d frames, %dx%d, save the coded file into %s\n",
           frame_count, frame_width, frame_height, coded_file);
    do_h264_encoding();

    printf("\n\n");
    
    vaDestroySurfaces(va_dpy,&surface_id[0],SURFACE_NUM);
    vaDestroyContext(va_dpy,context_id);
    vaDestroyConfig(va_dpy,config_id);
    
    vaTerminate(va_dpy);
    
#ifdef ANDROID
    free(x11_display);
#else
    XCloseDisplay(x11_display);
#endif
    
    return 0;
}
