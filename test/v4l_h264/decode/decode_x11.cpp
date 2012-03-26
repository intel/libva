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

//#define XT_DEBUG

#include <cstdio>
#include <getopt.h>
#include <csignal>
#include <cstring>
#include <cstdarg>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cassert>
#include <va/va_x11.h>
#include <iostream>
#include <cstdlib>

#include "TCPSocketServer.h"
using std::string;

#define MYPROF  VAProfileH264High

int   g_Debug = 0;
int   ip_port = 8888;
TCPSocketServer *sock_ptr = NULL;
#define SURFACE_NUM 7
static  Display *win_display;
static  VADisplay va_dpy;
Window  win;
int g_PX = 50;
int g_PY = 0;
bool g_LiveView = true;
int pwm;
int phm;
VAContextID context_id;
VASurfaceID surface_id[SURFACE_NUM];
int win_width = 0, win_height = 0;
int surface_width = 0, surface_height = 0;
static int time_to_quit = 0;

static void SignalHandler(int a_Signal)
{
    time_to_quit = 1;
    signal(SIGINT, SIG_DFL);
}


void InitSock()
{
    try {
    sock_ptr = new TCPSocketServer(ip_port);
    }
    catch (const std::exception& e)
    {
    std::cerr << e.what() << '\n';
    exit(1);
    }
}




/*currently, if XCheckWindowEvent was called  in more than one thread, it would cause
* XIO:  fatal IO error 11 (Resource temporarily unavailable) on X server ":0.0"
*   after 87 requests (83 known processed) with 0 events remaining.
*
*   X Error of failed request:  BadGC (invalid GC parameter)
*   Major opcode of failed request:  60 (X_FreeGC)
*   Resource id in failed request:  0x600034
*   Serial number of failed request:  398
*   Current serial number in output stream:  399
* The root cause is unknown. */

VAStatus gva_status;
VASurfaceStatus gsurface_status;
#define CHECK_SURF(X) \
    gva_status = vaQuerySurfaceStatus(va_dpy, X, &gsurface_status); \
    if (gsurface_status != 4) printf("ss: %d\n", gsurface_status);



#define CHECK_VASTATUS(va_status,func)                  \
    if (va_status != VA_STATUS_SUCCESS) {                   \
    fprintf(stderr,"%s:%s (%d) failed,exit\n", __func__, func, __LINE__); \
    exit(1);                                \
    } else  { \
    /*    fprintf(stderr,">> SUCCESS for: %s:%s (%d)\n", __func__, func, __LINE__); */ \
    }

void SetWindowTitle(const char* title, ...)
{
    char buf[256];
    va_list args;
    va_start(args, title);
    vsprintf(buf, title, args);
    va_end(args);
    XSetStandardProperties(win_display,win, buf, buf, None, NULL, 0, NULL);
}


#ifdef XT_DEBUG
static inline void PrintFlagIfNotZero(
    const char *name,   /* in */
    unsigned int flag   /* in */
    )
{
    if (flag != 0) {
    printf("%s = %x\n", name, flag);
    }
}

void DumpVAPictureH264(VAPictureH264 *p, char *s)
{
    printf("%s:\n", s);
    printf(" picture_id=0x%x\n", p->picture_id);
    printf(" frame_idx=%d\n", p->frame_idx);
    printf(" flags=%d\n", p->flags);
    printf(" TopFieldOrderCnt=%d\n", p->TopFieldOrderCnt);
    printf(" BottomFieldOrderCnt=%d\n", p->BottomFieldOrderCnt);
}


void DumpVAPictureParameterBufferH264(VAPictureParameterBufferH264 *p)
{
    int i;
    printf("VAPictureParameterBufferH264\n");
    printf("\tCurrPic.picture_id = 0x%08x\n", p->CurrPic.picture_id);
    printf("\tCurrPic.frame_idx = %d\n", p->CurrPic.frame_idx);
    printf("\tCurrPic.flags = %d\n", p->CurrPic.flags);
    printf("\tCurrPic.TopFieldOrderCnt = %d\n", p->CurrPic.TopFieldOrderCnt);
    printf("\tCurrPic.BottomFieldOrderCnt = %d\n", p->CurrPic.BottomFieldOrderCnt);
    printf("\tReferenceFrames (TopFieldOrderCnt-BottomFieldOrderCnt-picture_id-frame_idx:\n");
    for (i = 0; i < 16; i++) {
    if (p->ReferenceFrames[i].flags != VA_PICTURE_H264_INVALID) {
        printf("\t\t%d-%d-0x%08x-%d\n",
        p->ReferenceFrames[i].TopFieldOrderCnt,
        p->ReferenceFrames[i].BottomFieldOrderCnt,
        p->ReferenceFrames[i].picture_id,
        p->ReferenceFrames[i].frame_idx);
    } else {
#ifdef EXTRA_LOGS
        printf("\t\tinv-inv-inv-inv\n");
#endif
    }
    }
    printf("\n");
    printf("\tpicture_width_in_mbs_minus1 = %d\n", p->picture_width_in_mbs_minus1);
    printf("\tpicture_height_in_mbs_minus1 = %d\n", p->picture_height_in_mbs_minus1);
    printf("\tbit_depth_luma_minus8 = %d\n", p->bit_depth_luma_minus8);
    printf("\tbit_depth_chroma_minus8 = %d\n", p->bit_depth_chroma_minus8);
    printf("\tnum_ref_frames = %d\n", p->num_ref_frames);
    printf("\tseq fields = %d\n", p->seq_fields.value);
    printf("\tchroma_format_idc = %d\n", p->seq_fields.bits.chroma_format_idc);
    printf("\tresidual_colour_transform_flag = %d\n", p->seq_fields.bits.residual_colour_transform_flag);
    printf("\tframe_mbs_only_flag = %d\n", p->seq_fields.bits.frame_mbs_only_flag);
    printf("\tmb_adaptive_frame_field_flag = %d\n", p->seq_fields.bits.mb_adaptive_frame_field_flag);
    printf("\tdirect_8x8_inference_flag = %d\n", p->seq_fields.bits.direct_8x8_inference_flag);
    printf("\tMinLumaBiPredSize8x8 = %d\n", p->seq_fields.bits.MinLumaBiPredSize8x8);
    printf("\tnum_slice_groups_minus1 = %d\n", p->num_slice_groups_minus1);
    printf("\tslice_group_map_type = %d\n", p->slice_group_map_type);
    printf("\tslice_group_change_rate_minus1 = %d\n", p->slice_group_change_rate_minus1);
    printf("\tpic_init_qp_minus26 = %d\n", p->pic_init_qp_minus26);
    printf("\tpic_init_qs_minus26 = %d\n", p->pic_init_qs_minus26);
    printf("\tchroma_qp_index_offset = %d\n", p->chroma_qp_index_offset);
    printf("\tsecond_chroma_qp_index_offset = %d\n", p->second_chroma_qp_index_offset);
    printf("\tpic_fields = 0x%03x\n", p->pic_fields.value);
#ifdef EXTRA_LOGS
    PrintFlagIfNotZero("\t\tentropy_coding_mode_flag", p->pic_fields.bits.entropy_coding_mode_flag);
    PrintFlagIfNotZero("\t\tweighted_pred_flag", p->pic_fields.bits.weighted_pred_flag);
    PrintFlagIfNotZero("\t\tweighted_bipred_idc", p->pic_fields.bits.weighted_bipred_idc);
    PrintFlagIfNotZero("\t\ttransform_8x8_mode_flag", p->pic_fields.bits.transform_8x8_mode_flag);
    PrintFlagIfNotZero("\t\tfield_pic_flag", p->pic_fields.bits.field_pic_flag);
    PrintFlagIfNotZero("\t\tconstrained_intra_pred_flag", p->pic_fields.bits.constrained_intra_pred_flag);
    PrintFlagIfNotZero("\t\tpic_order_present_flag", p->pic_fields.bits.pic_order_present_flag);
    PrintFlagIfNotZero("\t\tdeblocking_filter_control_present_flag", p->pic_fields.bits.deblocking_filter_control_present_flag);
    PrintFlagIfNotZero("\t\tredundant_pic_cnt_present_flag", p->pic_fields.bits.redundant_pic_cnt_present_flag);
    PrintFlagIfNotZero("\t\treference_pic_flag", p->pic_fields.bits.reference_pic_flag);
#endif
    printf("\tframe_num = %d\n", p->frame_num);
}
#endif


void SetVAPictureParameterBufferH264(VAPictureParameterBufferH264 *p)
{
    int i;
    memset(p, 0, sizeof(VAPictureParameterBufferH264));
    p->picture_width_in_mbs_minus1 = pwm;
    p->picture_height_in_mbs_minus1 = phm;
    p->num_ref_frames = 1;
    p->seq_fields.value = 145;
    /*
    p->seq_fields.bits.chroma_format_idc = 1;
    p->seq_fields.bits.frame_mbs_only_flag = 1;
    p->seq_fields.bits.MinLumaBiPredSize8x8 = 1;
    */
    p->pic_fields.value = 0x501;
    for (i = 0; i < 16; i++) {
    p->ReferenceFrames[i].flags = VA_PICTURE_H264_INVALID;
    p->ReferenceFrames[i].picture_id = 0xffffffff;

    }
}



void SetVASliceParameterBufferH264(VASliceParameterBufferH264 *p)
{
    int i;
    memset(p, 0, sizeof(VASliceParameterBufferH264));
    p->slice_data_size = 0;
    p->slice_data_bit_offset = 64;
    p->slice_alpha_c0_offset_div2 = 2;
    p->slice_beta_offset_div2 = 2;
    p->chroma_weight_l0_flag = 1;
    p->chroma_weight_l0[0][0]=1;
    p->chroma_offset_l0[0][0]=0;
    p->chroma_weight_l0[0][1]=1;
    p->chroma_offset_l0[0][1]=0;
    p->luma_weight_l1_flag = 1;
    p->chroma_weight_l1_flag = 1;
    p->luma_weight_l0[0]=0x01;
    for (i = 0; i < 32; i++) {
    p->RefPicList0[i].flags = VA_PICTURE_H264_INVALID;
    p->RefPicList1[i].flags = VA_PICTURE_H264_INVALID;
    //  p->ReferenceFrames[i].picture_id = 0xffffffff;
    }
    p->RefPicList1[0].picture_id = 0xffffffff; //0xaa0000bb;
}



void SetVASliceParameterBufferH264_T2(VASliceParameterBufferH264 *p, int first)
{
    int i;
    memset(p, 0, sizeof(VASliceParameterBufferH264));
    p->slice_data_size = 0;
    p->slice_data_bit_offset = 64;
    p->slice_alpha_c0_offset_div2 = 2;
    p->slice_beta_offset_div2 = 2;
    p->slice_type = 2;
    if (first) {
    p->luma_weight_l0_flag = 1;
    p->chroma_weight_l0_flag = 1;
    p->luma_weight_l1_flag = 1;
    p->chroma_weight_l1_flag = 1;
    } else {
    p->chroma_weight_l0_flag = 1;
    p->chroma_weight_l0[0][0]=1;
    p->chroma_offset_l0[0][0]=0;
    p->chroma_weight_l0[0][1]=1;
    p->chroma_offset_l0[0][1]=0;
    p->luma_weight_l1_flag = 1;
    p->chroma_weight_l1_flag = 1;
    p->luma_weight_l0[0]=0x01;
    }
    for (i = 0; i < 32; i++) {
    p->RefPicList0[i].flags = VA_PICTURE_H264_INVALID;
    p->RefPicList1[i].flags = VA_PICTURE_H264_INVALID;
    //  p->ReferenceFrames[i].picture_id = 0xffffffff;
    }
    p->RefPicList1[0].picture_id = 0xffffffff;
    p->RefPicList0[0].picture_id = 0xffffffff;
}


unsigned char m_MatrixBufferH264[]= {
    //ScalingList4x4[6][16]
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    //ScalingList8x8[2][64]
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

#ifdef XT_DEBUG
void DumpVASliceParameterBufferH264(VASliceParameterBufferH264 *p)
{
    int i;
    printf("\telement[0] = VASliceParameterBufferH264\n");
    printf("\tslice_data_size = %d\n", p->slice_data_size);
    printf("\tslice_data_offset = %d\n", p->slice_data_offset);
    printf("\tslice_data_flag = %d\n", p->slice_data_flag);
    printf("\tslice_data_bit_offset = %d\n", p->slice_data_bit_offset);
    printf("\tfirst_mb_in_slice = %d\n", p->first_mb_in_slice);
    printf("\tslice_type = %d\n", p->slice_type);
    printf("\tdirect_spatial_mv_pred_flag = %d\n", p->direct_spatial_mv_pred_flag);
    printf("\tnum_ref_idx_l0_active_minus1 = %d\n", p->num_ref_idx_l0_active_minus1);
    printf("\tnum_ref_idx_l1_active_minus1 = %d\n", p->num_ref_idx_l1_active_minus1);
    printf("\tcabac_init_idc = %d\n", p->cabac_init_idc);
    printf("\tslice_qp_delta = %d\n", p->slice_qp_delta);
    printf("\tdisable_deblocking_filter_idc = %d\n", p->disable_deblocking_filter_idc);
    printf("\tslice_alpha_c0_offset_div2 = %d\n", p->slice_alpha_c0_offset_div2);
    printf("\tslice_beta_offset_div2 = %d\n", p->slice_beta_offset_div2);
    if (p->slice_type == 0 || p->slice_type == 1) {
    printf("\tRefPicList0 =");
    for (i = 0; i < p->num_ref_idx_l0_active_minus1 + 1; i++) {
        printf("%d-%d-0x%08x-%d\n", p->RefPicList0[i].TopFieldOrderCnt, p->RefPicList0[i].BottomFieldOrderCnt, p->RefPicList0[i].picture_id, p->RefPicList0[i].frame_idx);
    }
    if (p->slice_type == 1) {
        printf("\tRefPicList1 =");
        for (i = 0; i < p->num_ref_idx_l1_active_minus1 + 1; i++)
        {
        printf("%d-%d-0x%08x-%d\n", p->RefPicList1[i].TopFieldOrderCnt, p->RefPicList1[i].BottomFieldOrderCnt, p->RefPicList1[i].picture_id, p->RefPicList1[i].frame_idx);
        }
    }
    }
    printf("\tluma_log2_weight_denom = %d\n", p->luma_log2_weight_denom);
    printf("\tchroma_log2_weight_denom = %d\n", p->chroma_log2_weight_denom);
    printf("\tluma_weight_l0_flag = %d\n", p->luma_weight_l0_flag);
    if (p->luma_weight_l0_flag) {
    for (i = 0; i <=  p->num_ref_idx_l0_active_minus1; i++) {
        printf("\t%d ", p->luma_weight_l0[i]);
        printf("\t%d ", p->luma_offset_l0[i]);
    }
    printf("\n");
    }
    printf("\tchroma_weight_l0_flag = %d\n", p->chroma_weight_l0_flag);
    if (p->chroma_weight_l0_flag) {
    for (i = 0; i <= p->num_ref_idx_l0_active_minus1; i++) {
        printf("\t\t%d ", p->chroma_weight_l0[i][0]);
        printf("\t\t%d ", p->chroma_offset_l0[i][0]);
        printf("\t\t%d ", p->chroma_weight_l0[i][1]);
        printf("\t\t%d ", p->chroma_offset_l0[i][1]);
    }
    printf("\n");
    }
    printf("\tluma_weight_l1_flag = %d\n", p->luma_weight_l1_flag);
    if (p->luma_weight_l1_flag) {
    for (i = 0; i <=  p->num_ref_idx_l1_active_minus1; i++) {
        printf("\t\t%d ", p->luma_weight_l1[i]);
        printf("\t\t%d ", p->luma_offset_l1[i]);
    }
    printf("\n");
    }
    printf("\tchroma_weight_l1_flag = %d\n", p->chroma_weight_l1_flag);
    if (p->chroma_weight_l1_flag) {
    for (i = 0; i <= p->num_ref_idx_l1_active_minus1; i++) {
        printf("\t\t%d ", p->chroma_weight_l1[i][0]);
        printf("\t\t%d ", p->chroma_offset_l1[i][0]);
        printf("\t\t%d ", p->chroma_weight_l1[i][1]);
        printf("\t\t%d ", p->chroma_offset_l1[i][1]);
    }
    printf("\n");
    }
}
#endif


static void usage (FILE * fp, int argc, char ** argv)
{
    fprintf (fp,
        "Usage: %s [options]\n"
        "\n"
        "Options:\n"
        "-?, --help             Print this message\n"
        "-p, --port=PORT        Listen port [%d]\n"
        "-l, --liveview-off     Live View off\n"
        "-x, --posx=POS_X       X position [WM handles placement]\n"
        "-y, --posy=POS_Y       Y position [WM handles placement]\n"
        "-w, --width=WIDTH      Window width [same as Surface]\n"
        "-h, --height=HEIGHT    Window height [same as Surface]\n"
        "-d, --debug=LEVEL      Debug level [%d]\n"
        "\n",
        argv[0], ip_port, g_Debug);
}

static const char short_options [] = "?p:lx:y:w:h:d:";

static const struct option
    long_options [] = {
        { "help",           no_argument,        NULL, '?' },
        { "port",           required_argument,  NULL, 'p' },
        { "liveview-off",   no_argument,        NULL, 'l' },
        { "posx",           required_argument,  NULL, 'x' },
        { "posy",           required_argument,  NULL, 'y' },
        { "width",          required_argument,  NULL, 'w' },
        { "height",         required_argument,  NULL, 'h' },
        { "debug",          required_argument,  NULL, 'd' },
        { 0, 0, 0, 0 }
};


int main(int argc,char **argv)
{
    int t2first = 1;
    int real_frame = 0;
    int slice_type = 2;
    int FieldOrderCnt = 0;
    int major_ver, minor_ver;
    int i;
    unsigned char frid = 0;
    int z;
    int sid = 0;
    int newsid = 0;
    unsigned int data_size = 0;
    unsigned int frame_count = 0;
    std::string remoteAddr;
    unsigned short remotePort;
    char *dh264 = NULL;
    int num_entrypoints,vld_entrypoint;
    VAStatus              va_status;
    VAIQMatrixBufferH264     *mh264 = NULL;
    VAPictureParameterBufferH264 *ph264 = NULL;
    VASliceParameterBufferH264   *sh264 = NULL;
    VABufferID            bufids[10];
    VAEntrypoint          entrypoints[5];
    VAConfigAttrib        attrib;
    VAConfigID            config_id;
    VABufferID            pic_param_buf_id[SURFACE_NUM];
    VABufferID            mat_param_buf_id[SURFACE_NUM];
    VABufferID            sp_param_buf_id[SURFACE_NUM];
    VABufferID            d_param_buf_id[SURFACE_NUM];
    VAPictureH264         my_VAPictureH264;
    VAPictureH264         my_old_VAPictureH264;


    for (;;) {
    int index;
    int c;

    c = getopt_long (argc, argv,
        short_options, long_options,
        &index);

    if (-1 == c)
        break;

    switch (c) {
    case 0: /* getopt_long() flag */
        break;

    case '?':
        usage (stdout, argc, argv);
        exit (EXIT_SUCCESS);
    case 'p':
        ip_port = atoi(optarg);
        break;
    case 'l':
        g_LiveView = false;
        break;
    case 'x':
        g_PX = atoi(optarg);
        break;
    case 'y':
        g_PY = atoi(optarg);
        break;
    case 'w':
        win_width = atoi(optarg);
        break;
    case 'h':
        win_height = atoi(optarg);
        break;
    case 'd':
        g_Debug = atoi(optarg);
        break;
    default:
        usage (stderr, argc, argv);
        exit (EXIT_FAILURE);
    }
    }

    InitSock();

    printf("Accept - start\n");
    sock_ptr->accept(remoteAddr, remotePort);
    printf("Accept - done (%s:%d)\n", remoteAddr.c_str(), remotePort);

    surface_width = sock_ptr->recv_uint32();
    surface_height = sock_ptr->recv_uint32();
    if (!win_width) {
    win_width = surface_width;
    }
    if (!win_height) {
    win_height = surface_height;
    }
    pwm = sock_ptr->recv_uint32();
    phm = sock_ptr->recv_uint32();

    win_display = (Display *)XOpenDisplay(":0.0");
    if (win_display == NULL) {
    fprintf(stderr, "Can't open the connection of display!\n");
    exit(-1);
    }
    if (g_LiveView) {
    win = XCreateSimpleWindow(win_display, RootWindow(win_display, 0), 0, 0, win_width, win_height, 0, 0, WhitePixel(win_display, 0));
    XMapWindow(win_display, win);
    SetWindowTitle("Decode H264 (%dx%d in %dx%d) TCP", surface_width,surface_height, win_width, win_height);
    if ((g_PX !=-1) && (g_PY !=-1)) {
        XMoveWindow(win_display,  win, g_PX, g_PY);
    }
    XSync(win_display, False);
    }
    if(signal(SIGINT, SignalHandler) == SIG_ERR) {
    printf("signal() failed\n");
    time_to_quit = 1;
    exit(-1);
    }
    va_dpy = vaGetDisplay(win_display);
    va_status = vaInitialize(va_dpy, &major_ver, &minor_ver);
    CHECK_VASTATUS(va_status, "vaInitialize");

    va_status = vaQueryConfigEntrypoints(va_dpy, MYPROF, entrypoints, &num_entrypoints);
    CHECK_VASTATUS(va_status, "vaQueryConfigEntrypoints");
    for (vld_entrypoint = 0; vld_entrypoint < num_entrypoints; vld_entrypoint++) {
    if (entrypoints[vld_entrypoint] == VAEntrypointVLD)
        break;
    }
    if (vld_entrypoint == num_entrypoints) {
    /* not find VLD entry point */
    assert(0);
    }
    /* Assuming finding VLD, find out the format for the render target */
    attrib.type = VAConfigAttribRTFormat;
    vaGetConfigAttributes(va_dpy, MYPROF, VAEntrypointVLD, &attrib, 1);
    if ((attrib.value & VA_RT_FORMAT_YUV420) == 0) {
    /* not find desired YUV420 RT format */
    assert(0);
    }
    CHECK_VASTATUS(va_status, "vaGetConfigAttributes");
    va_status = vaCreateConfig(va_dpy, MYPROF, VAEntrypointVLD, &attrib, 1,&config_id);
    CHECK_VASTATUS(va_status, "vaCreateConfig");
    va_status = vaCreateSurfaces(va_dpy,surface_width,surface_height,VA_RT_FORMAT_YUV420, SURFACE_NUM, &surface_id[0]);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");
    va_status = vaCreateContext(va_dpy, config_id, surface_width,surface_height, 0/*VA_PROGRESSIVE*/,  &surface_id[0], SURFACE_NUM, &context_id);
    CHECK_VASTATUS(va_status, "vaCreateContext");
    for(i=0; i<SURFACE_NUM; i++) {
    pic_param_buf_id[i] = VA_INVALID_ID;
    mat_param_buf_id[i] = VA_INVALID_ID;
    sp_param_buf_id[i] = VA_INVALID_ID;
    d_param_buf_id[i] = VA_INVALID_ID;
    }
    va_status = vaBeginPicture(va_dpy, context_id, surface_id[sid]);
    CHECK_VASTATUS(va_status, "vaBeginPicture");
    if (g_Debug) {
      printf("--- Loop start here....\n");
    }
    while(!time_to_quit) {
    frame_count = sock_ptr->recv_uint32();
    slice_type = sock_ptr->recv_uint32();
    switch(slice_type) {
    case 0:
    case 2:
        break;
    default:
        printf("Wrong type: %d\n", slice_type);
        exit(-1);
        break;
    }
    data_size = sock_ptr->recv_uint32();
    if (g_Debug) {
      printf("T=%d S=%8d [%8d]\n", slice_type, data_size, frame_count);
    }
    my_VAPictureH264.picture_id = surface_id[sid];
    my_VAPictureH264.frame_idx = frid;
    my_VAPictureH264.flags = 0;
    my_VAPictureH264.BottomFieldOrderCnt = FieldOrderCnt;
    my_VAPictureH264.TopFieldOrderCnt = FieldOrderCnt;
    if (pic_param_buf_id[sid] == VA_INVALID_ID) {
        va_status = vaCreateBuffer(va_dpy, context_id, VAPictureParameterBufferType, sizeof(VAPictureParameterBufferH264), 1, NULL, &pic_param_buf_id[sid]);
    }
    CHECK_VASTATUS(va_status, "vaCreateBuffer");
    CHECK_SURF(surface_id[sid]);
    va_status = vaMapBuffer(va_dpy,pic_param_buf_id[sid],(void **)&ph264);
    CHECK_VASTATUS(va_status, "vaMapBuffer");
    SetVAPictureParameterBufferH264(ph264);
    memcpy(&ph264->CurrPic, &my_VAPictureH264, sizeof(VAPictureH264));
    if (slice_type == 2) {
    } else {
        memcpy(&ph264->ReferenceFrames[0], &my_old_VAPictureH264, sizeof(VAPictureH264));
        ph264->ReferenceFrames[0].flags = 0;
    }
    ph264->frame_num = frid;

#ifdef XT_DEBUG
    DumpVAPictureParameterBufferH264(ph264);
#endif
    va_status = vaUnmapBuffer(va_dpy,pic_param_buf_id[sid]);
    CHECK_VASTATUS(va_status, "vaUnmapBuffer");

    if (mat_param_buf_id[sid] == VA_INVALID_ID) {
        va_status = vaCreateBuffer(va_dpy, context_id, VAIQMatrixBufferType, sizeof(VAIQMatrixBufferH264), 1, NULL, &mat_param_buf_id[sid]);
        CHECK_VASTATUS(va_status, "vaCreateBuffer");
    }
    CHECK_SURF(surface_id[sid]);
    va_status = vaMapBuffer(va_dpy, mat_param_buf_id[sid], (void **)&mh264);
    CHECK_VASTATUS(va_status, "vaMapBuffer");
    memcpy(mh264, m_MatrixBufferH264, 224);
    va_status = vaUnmapBuffer(va_dpy, mat_param_buf_id[sid]);
    CHECK_VASTATUS(va_status, "vaUnmapBuffer");
    bufids[0] = pic_param_buf_id[sid];
    bufids[1] = mat_param_buf_id[sid];
    CHECK_SURF(surface_id[sid]);
    va_status = vaRenderPicture(va_dpy, context_id, bufids, 2);
    CHECK_VASTATUS(va_status, "vaRenderPicture");
    if (sp_param_buf_id[sid] == VA_INVALID_ID) {
        va_status = vaCreateBuffer(va_dpy, context_id, VASliceParameterBufferType, sizeof(VASliceParameterBufferH264), 1, NULL, &sp_param_buf_id[sid]);
        CHECK_VASTATUS(va_status, "vaCreateBuffer");
    }
    CHECK_SURF(surface_id[sid]);
    va_status = vaMapBuffer(va_dpy, sp_param_buf_id[sid], (void **)&sh264);
    CHECK_VASTATUS(va_status, "vaMapBuffer");
    if (slice_type == 2) {
        SetVASliceParameterBufferH264_T2(sh264, t2first);
        t2first = 0;
    } else {
        SetVASliceParameterBufferH264(sh264);
        memcpy(&sh264->RefPicList0[0], &my_old_VAPictureH264, sizeof(VAPictureH264));
        sh264->RefPicList0[0].flags = 0;
    }
    sh264->slice_data_bit_offset = 0;
    sh264->slice_data_size = data_size;
#ifdef XT_DEBUG
    DumpVASliceParameterBufferH264(sh264);
#endif
    va_status = vaUnmapBuffer(va_dpy, sp_param_buf_id[sid]);
    CHECK_VASTATUS(va_status, "vaUnmapBuffer");
    CHECK_SURF(surface_id[sid]);
    if (d_param_buf_id[sid] == VA_INVALID_ID) {
        va_status = vaCreateBuffer(va_dpy, context_id, VASliceDataBufferType, 4177920, 1, NULL, &d_param_buf_id[sid]); // 1080p size
        CHECK_VASTATUS(va_status, "vaCreateBuffer");
    }
    va_status = vaMapBuffer(va_dpy, d_param_buf_id[sid], (void **)&dh264);
    CHECK_VASTATUS(va_status, "vaMapBuffer");
    sock_ptr->recv_data((unsigned char*)dh264, data_size);
    CHECK_SURF(surface_id[sid]);
    va_status = vaUnmapBuffer(va_dpy, d_param_buf_id[sid]);
    CHECK_VASTATUS(va_status, "vaUnmapBuffer");
    bufids[0] = sp_param_buf_id[sid];
    bufids[1] = d_param_buf_id[sid];
    CHECK_SURF(surface_id[sid]);
    va_status = vaRenderPicture(va_dpy, context_id, bufids, 2);
    CHECK_VASTATUS(va_status, "vaRenderPicture");
    va_status = vaEndPicture(va_dpy, context_id);
    CHECK_VASTATUS(va_status, "vaEndPicture");
    newsid = sid+1;
    if (newsid==SURFACE_NUM) {
        newsid = 0;
    }
    va_status = vaBeginPicture(va_dpy, context_id, surface_id[newsid]);
    CHECK_VASTATUS(va_status, "vaBeginPicture");
    va_status = vaSyncSurface(va_dpy, surface_id[sid]);
    CHECK_VASTATUS(va_status, "vaSyncSurface");
    CHECK_SURF(surface_id[sid]);
    if (g_LiveView) {
        va_status = vaPutSurface(va_dpy, surface_id[sid], win, 0, 0, surface_width, surface_height, 0, 0, win_width, win_height, NULL, 0, VA_FRAME_PICTURE);
        CHECK_VASTATUS(va_status, "vaPutSurface");
    }
    sid = newsid;
    frid++;
    if (frid>15) frid = 0;
    FieldOrderCnt+=2;
    memcpy(&my_old_VAPictureH264, &my_VAPictureH264, sizeof(VAPictureH264));
    real_frame ++;
    }
    if (g_Debug) {
      printf("Final !\n");
    }
    vaDestroySurfaces(va_dpy,&surface_id[0],SURFACE_NUM);
    vaTerminate(va_dpy);
    XCloseDisplay(win_display);
    delete sock_ptr;

    return 0;
}
