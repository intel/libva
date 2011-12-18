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

        if (verbose) printf("Thread %x Display surface 0x%p,\n", (unsigned int)drawable, (void *)surface_id);

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
        vaStatus = vaPutSurface(va_dpy, surface_id, CAST_DRAWABLE(drawable),
                                0,0,surface_width,surface_height,
                                0,0,width,height,
                                (test_clip==0)?NULL:&cliprects[0],
                                (test_clip==0)?0:2,
                                display_field);
        CHECK_VASTATUS(vaStatus,"vaPutSurface");
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

    while ((c =getopt(argc,argv,"w:h:g:r:d:f:tcep?n:v") ) != EOF) {
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
            case 'v':
                verbose = 1;
                printf("Enable verbose output\n");
                break;
        }
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

    va_status = vaCreateSurfaces(
        va_dpy,
        VA_RT_FORMAT_YUV420, surface_width, surface_height,
        &surface_id[0], SURFACE_NUM,
        NULL, 0
    );
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
    
    vaDestroySurfaces(va_dpy,&surface_id[0],SURFACE_NUM);    
    vaTerminate(va_dpy);

    close_display(win_display);
    
    return 0;
}
