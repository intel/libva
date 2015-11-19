/*
 * Copyright (c) 2008-2009 Intel Corporation. All Rights Reserved.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>

#include <sys/time.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>

/*currently, if XCheckWindowEvent was called  in more than one thread, it would cause
 * XIO:  fatal IO error 11 (Resource temporarily unavailable) on X server ":0.0"
 *       after 87 requests (83 known processed) with 0 events remaining.
 *
 *       X Error of failed request:  BadGC (invalid GC parameter)
 *       Major opcode of failed request:  60 (X_FreeGC)
 *       Resource id in failed request:  0x600034
 *       Serial number of failed request:  398
 *       Current serial number in output stream:  399
 * The root cause is unknown. */

#define CHECK_VASTATUS(va_status,func)                                  \
if (va_status != VA_STATUS_SUCCESS) {                                   \
    fprintf(stderr,"%s:%s (%d) failed,exit\n", __func__, func, __LINE__); \
    exit(1);                                                            \
}
#include "../loadsurface.h"

#define SURFACE_NUM 16

static  void *win_display;
static  VADisplay va_dpy;
static  VAImageFormat *va_image_formats;
static  int va_num_image_formats = -1;
static  VAConfigID vpp_config_id = VA_INVALID_ID;
static  VASurfaceAttrib *va_surface_attribs;
static  int va_num_surface_attribs = -1;
static  VASurfaceID surface_id[SURFACE_NUM];
static  pthread_mutex_t surface_mutex[SURFACE_NUM];

static  void *drawable_thread0, *drawable_thread1;
static  int surface_width = 352, surface_height = 288;
static  int win_x = 0, win_y = 0;
static  int win_width = 352, win_height = 288;
static  int frame_rate = 0;
static  unsigned long long frame_num_total = ~0;
static  int check_event = 1;
static  int put_pixmap = 0;
static  int test_clip = 0;
static  int display_field = VA_FRAME_PICTURE;
static  pthread_mutex_t gmutex;
static  int box_width = 32;
static  int multi_thread = 0;
static  int verbose = 0;
static  int test_color_conversion = 0;
static  unsigned int csc_src_fourcc = 0, csc_dst_fourcc = 0;
static  VAImage csc_dst_fourcc_image;
static  VASurfaceID csc_render_surface;


typedef struct {
    const char * fmt_str;
    unsigned int fourcc;
} fourcc_map;
fourcc_map va_fourcc_map[] = {
    {"YUYV", VA_FOURCC_YUY2},
    {"YUY2", VA_FOURCC_YUY2},
    {"NV12", VA_FOURCC_NV12},
    {"YV12", VA_FOURCC_YV12},
    {"BGRA", VA_FOURCC_BGRA},
    {"RGBA", VA_FOURCC_RGBA},
    {"BGRX", VA_FOURCC_BGRX},
    {"RGBX", VA_FOURCC_RGBX},
};
unsigned int map_str_to_vafourcc (char * str)
{
    unsigned int i;
    for (i=0; i< sizeof(va_fourcc_map)/sizeof(fourcc_map); i++) {
        if (!strcmp(va_fourcc_map[i].fmt_str, str)) {
            return va_fourcc_map[i].fourcc;
        }
    }

    return 0;

}
const char* map_vafourcc_to_str (unsigned int format)
{
    static char unknown_format[] = "unknown-format";
    unsigned int i;
    for (i=0; i< sizeof(va_fourcc_map)/sizeof(fourcc_map); i++) {
        if (va_fourcc_map[i].fourcc == format) {
            return va_fourcc_map[i].fmt_str;
        }
    }

    return unknown_format;

}

static int
va_value_equals(const VAGenericValue *v1, const VAGenericValue *v2)
{
    if (v1->type != v2->type)
        return 0;

    switch (v1->type) {
    case VAGenericValueTypeInteger:
        return v1->value.i == v2->value.i;
    case VAGenericValueTypeFloat:
        return v1->value.f == v2->value.f;
    case VAGenericValueTypePointer:
        return v1->value.p == v2->value.p;
    case VAGenericValueTypeFunc:
        return v1->value.fn == v2->value.fn;
    }
    return 0;
}

static int
ensure_image_formats(void)
{
    VAStatus va_status;
    VAImageFormat *image_formats;
    int num_image_formats;

    if (va_num_image_formats >= 0)
        return va_num_image_formats;

    num_image_formats = vaMaxNumImageFormats(va_dpy);
    if (num_image_formats == 0)
        return 0;

    image_formats = (VAImageFormat *) malloc(num_image_formats * sizeof(*image_formats));
    if (!image_formats)
        return 0;

    va_status = vaQueryImageFormats(va_dpy, image_formats, &num_image_formats);
    CHECK_VASTATUS(va_status, "vaQuerySurfaceAttributes()");

    va_image_formats = image_formats;
    va_num_image_formats = num_image_formats;
    return num_image_formats;
}

static const VAImageFormat *
lookup_image_format(uint32_t fourcc)
{
    int i;

    if (!ensure_image_formats())
        return NULL;

    for (i = 0; i < va_num_image_formats; i++) {
        const VAImageFormat * const image_format = &va_image_formats[i];
        if (image_format->fourcc == fourcc)
            return image_format;
    }
    return NULL;
}

static int
ensure_surface_attribs(void)
{
    VAStatus va_status;
    VASurfaceAttrib *surface_attribs;
    unsigned int num_image_formats, num_surface_attribs;

    if (va_num_surface_attribs >= 0)
        return va_num_surface_attribs;

    num_image_formats = vaMaxNumImageFormats(va_dpy);
    if (num_image_formats == 0)
        return 0;

    va_status = vaCreateConfig(va_dpy, VAProfileNone, VAEntrypointVideoProc,
        NULL, 0, &vpp_config_id);
    CHECK_VASTATUS(va_status, "vaCreateConfig()");

    /* Guess the number of surface attributes, thus including any
       pixel-format supported by the VA driver */
    num_surface_attribs = VASurfaceAttribCount + num_image_formats;
    surface_attribs = (VASurfaceAttrib *) malloc(num_surface_attribs * sizeof(*surface_attribs));
    if (!surface_attribs)
        return 0;

    va_status = vaQuerySurfaceAttributes(va_dpy, vpp_config_id,
        surface_attribs, &num_surface_attribs);
    if (va_status == VA_STATUS_SUCCESS)
        va_surface_attribs =  surface_attribs;
    else if (va_status == VA_STATUS_ERROR_MAX_NUM_EXCEEDED) {
        va_surface_attribs = (VASurfaceAttrib *) realloc(surface_attribs,
            num_surface_attribs * sizeof(*va_surface_attribs));
        if (!va_surface_attribs) {
            free(surface_attribs);
            return 0;
        }
        va_status = vaQuerySurfaceAttributes(va_dpy, vpp_config_id,
            va_surface_attribs, &num_surface_attribs);
    }
    CHECK_VASTATUS(va_status, "vaQuerySurfaceAttributes()");
    va_num_surface_attribs = num_surface_attribs;
    return num_surface_attribs;
}

static const VASurfaceAttrib *
lookup_surface_attrib(VASurfaceAttribType type, const VAGenericValue *value)
{
    int i;

    if (!ensure_surface_attribs())
        return NULL;

    for (i = 0; i < va_num_surface_attribs; i++) {
        const VASurfaceAttrib * const surface_attrib = &va_surface_attribs[i];
        if (surface_attrib->type != type)
            continue;
        if (!(surface_attrib->flags & VA_SURFACE_ATTRIB_SETTABLE))
            continue;
        if (va_value_equals(&surface_attrib->value, value))
            return surface_attrib;
    }
    return NULL;
}

int csc_preparation ()
{
    VAStatus va_status;
    VASurfaceAttrib surface_attribs[1], * const s_attrib = &surface_attribs[0];

    // 1. make sure dst fourcc is supported for vaImage
    if (!lookup_image_format(csc_dst_fourcc)) {
        test_color_conversion = 0;
        printf("VA driver doesn't support %s image, skip additional color conversion\n",  map_vafourcc_to_str(csc_dst_fourcc));
        goto cleanup;
    }

    // 2. make sure src_fourcc is supported for vaSurface
    s_attrib->type = VASurfaceAttribPixelFormat;
    s_attrib->flags = VA_SURFACE_ATTRIB_SETTABLE;
    s_attrib->value.type = VAGenericValueTypeInteger;
    s_attrib->value.value.i = csc_src_fourcc;

    if (!lookup_surface_attrib(VASurfaceAttribPixelFormat, &s_attrib->value)) {
        printf("VA driver doesn't support %s surface, skip additional color conversion\n",  map_vafourcc_to_str(csc_src_fourcc));
        test_color_conversion = 0;
        goto cleanup;
    }

    // 3 create all objs required by csc
    // 3.1 vaSurface with src fourcc
    va_status = vaCreateSurfaces(
        va_dpy,
        VA_RT_FORMAT_YUV420, surface_width, surface_height,
        &surface_id[0], SURFACE_NUM,
        surface_attribs, 1
    );
    CHECK_VASTATUS(va_status,"vaCreateSurfaces");

    // 3.2 vaImage with dst fourcc
    VAImageFormat image_format;
    image_format.fourcc = csc_dst_fourcc;
    image_format.byte_order = VA_LSB_FIRST;
    image_format.bits_per_pixel = 16;

    va_status = vaCreateImage(va_dpy, &image_format,
                    surface_width, surface_height,
                    &csc_dst_fourcc_image);
    CHECK_VASTATUS(va_status,"vaCreateImage");


    // 3.3 create a temp VASurface for final rendering(vaPutSurface)
    s_attrib->value.value.i = VA_FOURCC_NV12;
    va_status = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_YUV420,
                                 surface_width, surface_height,
                                 &csc_render_surface, 1,
                                 surface_attribs, 1);
    CHECK_VASTATUS(va_status,"vaCreateSurfaces");


cleanup:
    return test_color_conversion;
}

static VASurfaceID get_next_free_surface(int *index)
{
    VASurfaceStatus surface_status;
    int i;

    assert(index);

    if (multi_thread == 0) {
        i = *index;
        i++;
        if (i == SURFACE_NUM)
            i = 0;
        *index = i;

        return surface_id[i];
    }

    for (i=0; i<SURFACE_NUM; i++) {
        surface_status = (VASurfaceStatus)0;
        vaQuerySurfaceStatus(va_dpy, surface_id[i], &surface_status);
        if (surface_status == VASurfaceReady)
        {
            if (0 == pthread_mutex_trylock(&surface_mutex[i]))
            {
                *index = i;
                break;
            }
        }
    }

    if (i==SURFACE_NUM)
        return VA_INVALID_SURFACE;
    else
        return surface_id[i];
}

static int upload_source_YUV_once_for_all()
{
    VAImage surface_image;
    void *surface_p=NULL, *U_start,*V_start;
    VAStatus va_status;
    int box_width_loc=8;
    int row_shift_loc=0;
    int i;

    for (i=0; i<SURFACE_NUM; i++) {
        printf("\rLoading data into surface %d.....", i);
        upload_surface(va_dpy, surface_id[i], box_width_loc, row_shift_loc, 0);

        row_shift_loc++;
        if (row_shift_loc==(2*box_width_loc)) row_shift_loc= 0;
    }
    printf("\n");

    return 0;
}

/*
 * Helper function for profiling purposes
 */
static unsigned long get_tick_count(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL))
        return 0;
    return tv.tv_usec/1000+tv.tv_sec*1000;
}

static void update_clipbox(VARectangle *cliprects, int width, int height)
{
    if (test_clip == 0)
        return;

    srand((unsigned)time(NULL));

    cliprects[0].x = (rand() % width);
    cliprects[0].y = (rand() % height);
    cliprects[0].width = (rand() % (width - cliprects[0].x));
    cliprects[0].height = (rand() % (height - cliprects[0].y));

    cliprects[1].x = (rand() % width);
    cliprects[1].y = (rand() % height);
    cliprects[1].width = (rand() % (width - cliprects[1].x));
    cliprects[1].height = (rand() % (height - cliprects[1].y));
    printf("\nTest clip (%d,%d, %d x %d) and (%d,%d, %d x %d) \n",
           cliprects[0].x, cliprects[0].y, cliprects[0].width, cliprects[0].height,
           cliprects[1].x, cliprects[1].y, cliprects[1].width, cliprects[1].height);
}

static void* putsurface_thread(void *data)
{
    int width=win_width, height=win_height;
    void *drawable = data;
    int quit = 0;
    VAStatus vaStatus;
    int row_shift = 0;
    int index = 0;
    unsigned int frame_num=0, start_time, putsurface_time;
    VARectangle cliprects[2]; /* client supplied clip list */
    int continue_display = 0;

    if (drawable == drawable_thread0)
        printf("Enter into thread0\n\n");
    if (drawable == drawable_thread1)
        printf("Enter into thread1\n\n");

    putsurface_time = 0;
    while (!quit) {
        VASurfaceID surface_id = VA_INVALID_SURFACE;

        while (surface_id == VA_INVALID_SURFACE)
            surface_id = get_next_free_surface(&index);

        if (verbose) printf("Thread: %p Display surface 0x%x,\n", drawable, surface_id);

        if (multi_thread)
            upload_surface(va_dpy, surface_id, box_width, row_shift, display_field);

        if (check_event)
            pthread_mutex_lock(&gmutex);

        start_time = get_tick_count();
        if ((continue_display == 0) && getenv("FRAME_STOP")) {
            char c;
            printf("Press any key to display frame %d...(c/C to continue)\n", frame_num);
            c = getchar();
            if (c == 'c' || c == 'C')
                continue_display = 1;
        }
        if (test_color_conversion) {
            static int _put_surface_count = 0;
            if (_put_surface_count++ %50 == 0) {
                printf("do additional colorcoversion from %s to %s\n", map_vafourcc_to_str(csc_src_fourcc), map_vafourcc_to_str(csc_dst_fourcc));
            }
            // get image from surface, csc_src_fourcc to csc_dst_fourcc conversion happens
            vaStatus = vaGetImage(va_dpy, surface_id, 0, 0,
                surface_width, surface_height, csc_dst_fourcc_image.image_id);
            CHECK_VASTATUS(vaStatus,"vaGetImage");

            // render csc_dst_fourcc image to temp surface
            vaStatus = vaPutImage(va_dpy, csc_render_surface, csc_dst_fourcc_image.image_id,
                                    0, 0, surface_width, surface_height,
                                    0, 0, surface_width, surface_height);
            CHECK_VASTATUS(vaStatus,"vaPutImage");

            // render the temp surface, it should be same with original surface without color conversion test
            vaStatus = vaPutSurface(va_dpy, csc_render_surface, CAST_DRAWABLE(drawable),
                                    0,0,surface_width,surface_height,
                                    0,0,width,height,
                                    (test_clip==0)?NULL:&cliprects[0],
                                    (test_clip==0)?0:2,
                                    display_field);
            CHECK_VASTATUS(vaStatus,"vaPutSurface");
        }
        else {
            vaStatus = vaPutSurface(va_dpy, surface_id, CAST_DRAWABLE(drawable),
                                    0,0,surface_width,surface_height,
                                    0,0,width,height,
                                    (test_clip==0)?NULL:&cliprects[0],
                                    (test_clip==0)?0:2,
                                    display_field);
            CHECK_VASTATUS(vaStatus,"vaPutSurface");
        }

        putsurface_time += (get_tick_count() - start_time);

        if (check_event)
            pthread_mutex_unlock(&gmutex);

        pthread_mutex_unlock(&surface_mutex[index]); /* locked in get_next_free_surface */

        if ((frame_num % 0xff) == 0) {
            fprintf(stderr, "%.2f FPS             \r", 256000.0 / (float)putsurface_time);
            putsurface_time = 0;
            update_clipbox(cliprects, width, height);
        }

        if (check_event)
            check_window_event(win_display, drawable, &width, &height, &quit);

        if (multi_thread) { /* reload surface content */
            row_shift++;
            if (row_shift==(2*box_width)) row_shift= 0;
        }

        if (frame_rate != 0) /* rough framerate control */
            usleep(1000/frame_rate*1000);

        frame_num++;
        if (frame_num >= frame_num_total)
            quit = 1;
    }

    if (drawable == drawable_thread1)
        pthread_exit(NULL);

    return 0;
}
int main(int argc,char **argv)
{
    int major_ver, minor_ver;
    VAStatus va_status;
    pthread_t thread1;
    int ret;
    char c;
    int i;
    char str_src_fmt[5], str_dst_fmt[5];

    static struct option long_options[] =
                 {
                   {"fmt1",  required_argument,       NULL, '1'},
                   {"fmt2",  required_argument,       NULL, '2'},
                   {0, 0, 0, 0}
                 };

    while ((c =getopt_long(argc,argv,"w:h:g:r:d:f:tcep?n:1:2:v", long_options, NULL)) != EOF) {
        switch (c) {
            case '?':
                printf("putsurface <options>\n");
                printf("           -g <widthxheight+x_location+y_location> window geometry\n");
                printf("           -w/-h resolution of surface\n");
                printf("           -r <framerate>\n");
                printf("           -d the dimension of black/write square box, default is 32\n");
                printf("           -t multi-threads\n");
                printf("           -c test clipbox\n");
                printf("           -f <1/2> top field, or bottom field\n");
                printf("           -1 source format (fourcc) for color conversion test\n");
                printf("           -2 dest   format (fourcc) for color conversion test\n");
                printf("           --fmt1 same to -1\n");
                printf("           --fmt2 same to -2\n");
                printf("           -v verbose output\n");
                exit(0);
                break;
            case 'g':
                ret = sscanf(optarg, "%dx%d+%d+%d", &win_width, &win_height, &win_x, &win_y);
                if (ret != 4) {
                    printf("invalid window geometry, must be widthxheight+x_location+y_location\n");
                    exit(0);
                } else
                    printf("Create window at (%d, %d), width = %d, height = %d\n",
                           win_x, win_y, win_width, win_height);
                break;
            case 'r':
                frame_rate = atoi(optarg);
                break;
            case 'w':
                surface_width = atoi(optarg);
                break;
            case 'h':
                surface_height = atoi(optarg);
                break;
            case 'n':
                frame_num_total = atoi(optarg);
                break;
            case 'd':
                box_width = atoi(optarg);
                break;
            case 't':
                multi_thread = 1;
                printf("Two threads to do vaPutSurface\n");
                break;
            case 'e':
                check_event = 0;
                break;
            case 'p':
                put_pixmap = 1;
                break;
            case 'c':
                test_clip = 1;
                break;
            case 'f':
                if (atoi(optarg) == 1) {
                    printf("Display TOP field\n");
                    display_field = VA_TOP_FIELD;
                } else if (atoi(optarg) == 2) {
                    printf("Display BOTTOM field\n");
                    display_field = VA_BOTTOM_FIELD;
                } else
                    printf("The validate input for -f is: 1(top field)/2(bottom field)\n");
                break;
            case '1':
                sscanf(optarg, "%s", str_src_fmt);
                csc_src_fourcc = map_str_to_vafourcc (str_src_fmt);

                                if (!csc_src_fourcc) {
                    printf("invalid fmt1: %s\n", str_src_fmt );
                    exit(0);
                }
                break;
            case '2':
                sscanf(optarg, "%s", str_dst_fmt);
                csc_dst_fourcc = map_str_to_vafourcc (str_dst_fmt);

                                if (!csc_dst_fourcc) {
                    printf("invalid fmt1: %s\n", str_dst_fmt );
                    exit(0);
                }
                break;
            case 'v':
                verbose = 1;
                printf("Enable verbose output\n");
                break;
        }
    }

    if (csc_src_fourcc && csc_dst_fourcc) {
        test_color_conversion = 1;
    }

    win_display = (void *)open_display();
    if (win_display == NULL) {
        fprintf(stderr, "Can't open the connection of display!\n");
        exit(-1);
    }
    create_window(win_display, win_x, win_y, win_width, win_height);

    va_dpy = vaGetDisplay(win_display);
    va_status = vaInitialize(va_dpy, &major_ver, &minor_ver);
    CHECK_VASTATUS(va_status, "vaInitialize");

    if (test_color_conversion) {
        ret = csc_preparation();
    }
    if (!test_color_conversion || !ret ) {
        va_status = vaCreateSurfaces(
            va_dpy,
            VA_RT_FORMAT_YUV420, surface_width, surface_height,
            &surface_id[0], SURFACE_NUM,
            NULL, 0
        );
        }
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");
    if (multi_thread == 0) /* upload the content for all surfaces */
        upload_source_YUV_once_for_all();

    if (check_event)
        pthread_mutex_init(&gmutex, NULL);

    for(i = 0; i< SURFACE_NUM; i++)
        pthread_mutex_init(&surface_mutex[i], NULL);

    if (multi_thread == 1)
        ret = pthread_create(&thread1, NULL, putsurface_thread, (void*)drawable_thread1);

    putsurface_thread((void *)drawable_thread0);

    if (multi_thread == 1)
        pthread_join(thread1, (void **)&ret);
    printf("thread1 is free\n");

    if (test_color_conversion) {
        // destroy temp surface/image
        va_status = vaDestroySurfaces(va_dpy, &csc_render_surface, 1);
        CHECK_VASTATUS(va_status,"vaDestroySurfaces");

        va_status = vaDestroyImage(va_dpy, csc_dst_fourcc_image.image_id);
        CHECK_VASTATUS(va_status,"vaDestroyImage");
    }

    if (vpp_config_id != VA_INVALID_ID) {
        vaDestroyConfig (va_dpy, vpp_config_id);
        vpp_config_id = VA_INVALID_ID;
    }

    vaDestroySurfaces(va_dpy,&surface_id[0],SURFACE_NUM);
    vaTerminate(va_dpy);

    free(va_image_formats);
    free(va_surface_attribs);
    close_display(win_display);

    return 0;
}
