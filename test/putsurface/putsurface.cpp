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


/* gcc -o putsurface putsurface.c -lva -lva-x11 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include <sys/time.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <va/va.h>

#ifndef ANDROID
#include <va/va_x11.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#else
#include <va/va_android.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>
#include <surfaceflinger/ISurfaceComposer.h>
#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurface.h>
#include <surfaceflinger/SurfaceComposerClient.h>
#include <binder/MemoryHeapBase.h>
#define Display unsigned int
#endif

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

#include "loadsurface.h"


#define SURFACE_NUM 5
static  VASurfaceID surface_id[SURFACE_NUM];
static  int surface_width=352, surface_height=288;
static  int win_width=352, win_height=288;
#ifndef ANDROID
static  Window win_thread0, win_thread1;
static  Pixmap pixmap_thread0, pixmap_thread1;
static  GC context_thread0, context_thread1;
static  pthread_mutex_t gmutex;
static  int check_event = 1;
#else
static  int win_thread0 = 0, win_thread1 = 0;
using namespace android;

sp<SurfaceComposerClient> client;
sp<Surface> android_surface;
sp<ISurface> android_isurface;
sp<SurfaceControl> surface_ctrl;

sp<SurfaceComposerClient> client1;
sp<Surface> android_surface1;
sp<ISurface> android_isurface1;
sp<SurfaceControl> surface_ctrl1;

namespace android {
    class Test {
        public:
            static const sp<ISurface>& getISurface(const sp<Surface>& s) {
                return s->getISurface();
            }
    };
};
#endif

static  Display *x11_display;
static  VADisplay va_dpy;
static  int multi_thread=0;
static  int put_pixmap = 0;
static  int test_clip = 0;
static  int display_field = VA_FRAME_PICTURE;

static  int verbose=0;
static  pthread_mutex_t surface_mutex[SURFACE_NUM];

static  int box_width=32;

#ifndef ANDROID
static Pixmap create_pixmap(int width, int height)
{
    int screen = DefaultScreen(x11_display);
    Window root;
    Pixmap pixmap;
    XWindowAttributes attr;

    root = RootWindow(x11_display, screen);

    XGetWindowAttributes (x11_display, root, &attr);

    printf("Create a pixmap from ROOT window %dx%d, pixmap size %dx%d\n\n", attr.width, attr.height, width, height);
    pixmap = XCreatePixmap(x11_display, root, width, height,
            DefaultDepth(x11_display, DefaultScreen(x11_display)));

    return pixmap;
}


static int create_window(int width, int height)
{
    int screen = DefaultScreen(x11_display);
    Window root, win;

    root = RootWindow(x11_display, screen);

    printf("Create window0 for thread0\n");
    win_thread0 = win = XCreateSimpleWindow(x11_display, root, 0, 0, width, height,
            0, 0, WhitePixel(x11_display, 0));
    if (win) {
        XSizeHints sizehints;
        sizehints.width  = width;
        sizehints.height = height;
        sizehints.flags = USSize;
        XSetNormalHints(x11_display, win, &sizehints);
        XSetStandardProperties(x11_display, win, "Thread 0", "Thread 0",
                None, (char **)NULL, 0, &sizehints);

        XMapWindow(x11_display, win);
    }
    context_thread0 = XCreateGC(x11_display, win, 0, 0);
    XSelectInput(x11_display, win, KeyPressMask | StructureNotifyMask);
    XSync(x11_display, False);

    if (put_pixmap)
        pixmap_thread0 = create_pixmap(width, height);

    if (multi_thread == 0)
        return 0;

    printf("Create window1 for thread1\n");

    win_thread1 = win = XCreateSimpleWindow(x11_display, root, width, 0, width, height,
            0, 0, WhitePixel(x11_display, 0));
    if (win) {
        XSizeHints sizehints;
        sizehints.width  = width;
        sizehints.height = height;
        sizehints.flags = USSize;
        XSetNormalHints(x11_display, win, &sizehints);
        XSetStandardProperties(x11_display, win, "Thread 1", "Thread 1",
                None, (char **)NULL, 0, &sizehints);

        XMapWindow(x11_display, win);
    }
    if (put_pixmap)
        pixmap_thread1 = create_pixmap(width, height);

    context_thread1 = XCreateGC(x11_display, win, 0, 0);
    XSelectInput(x11_display, win, KeyPressMask | StructureNotifyMask);
    XSync(x11_display, False);

    return 0;
}
#else
static int create_window(int width, int height)
{
    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    printf("Create window0 for thread0\n");
    client = new SurfaceComposerClient();
    surface_ctrl = client->createSurface(getpid(), 0, width, height, PIXEL_FORMAT_RGB_565, ISurfaceComposer::ePushBuffers);
    android_surface = surface_ctrl->getSurface();
    android_isurface = Test::getISurface(android_surface);

    client->openTransaction();
    surface_ctrl->setPosition(0, 0);
    client->closeTransaction();

    client->openTransaction();
    surface_ctrl->setSize(width, height);
    client->closeTransaction();

    client->openTransaction();
    surface_ctrl->setLayer(0x100000);
    client->closeTransaction();

    win_thread0 = 1;
    if (multi_thread == 0)
        return 0;

    printf("Create window1 for thread1\n");
    client1 = new SurfaceComposerClient();
    surface_ctrl1 = client1->createSurface(getpid(), 0, width, height, PIXEL_FORMAT_RGB_565, ISurfaceComposer::ePushBuffers);
    android_surface1 = surface_ctrl1->getSurface();
    android_isurface1 = Test::getISurface(android_surface1);

    client1->openTransaction();
    surface_ctrl1->setPosition(width, 0);
    client1->closeTransaction();

    client1->openTransaction();
    surface_ctrl1->setSize(width, height);
    client1->closeTransaction();

    client1->openTransaction();
    surface_ctrl1->setLayer(0x100000);
    client1->closeTransaction();

    win_thread1 = 2;
    return 0;

}
#endif

static VASurfaceID get_next_free_surface(int *index)
{
    VASurfaceStatus surface_status;
    int i;

    assert(index);

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

static void* putsurface_thread(void *data)
{
    int width=win_width, height=win_height;

#ifndef ANDROID
    Drawable draw;
    Window win = (Window)data;
    Pixmap pixmap = 0;
    GC context = NULL;
    Bool is_event; 
    XEvent event;
#else
    sp<ISurface> draw;
    int win = (int)data;
#endif

    int quit = 0;
    VAStatus vaStatus;
    int row_shift = 0;
    int index = 0;

    unsigned int frame_num=0, start_time, putsurface_time;
    VARectangle cliprects[2]; /* client supplied clip list */

    if (win == win_thread0) {
        printf("Enter into thread0\n\n");
#ifndef ANDROID
        pixmap = pixmap_thread0;
        context = context_thread0;
#else
        draw = android_isurface;    
#endif
    }

    if (win == win_thread1) {
        printf("Enter into thread1\n\n");
#ifndef ANDROID
        pixmap = pixmap_thread1;
        context = context_thread1;
#else
        draw = android_isurface1;    
#endif
    }
#ifndef ANDROID
    if (put_pixmap) {
        printf("vaPutSurface into a Pixmap, then copy into the Window\n\n");
        draw = pixmap;
    } else {
        printf("vaPutSurface into a Window directly\n\n");
        draw = win;
    }
#endif

    putsurface_time = 0;
    while (!quit) {
        VASurfaceID surface_id = VA_INVALID_SURFACE;

        while (surface_id == VA_INVALID_SURFACE)
            surface_id = get_next_free_surface(&index);

        if (verbose) printf("Thread %x Display surface 0x%p,\n", (unsigned int)win, (void *)surface_id);

        upload_surface(va_dpy, surface_id, box_width, row_shift, display_field);

        start_time = get_tick_count();
        vaStatus = vaPutSurface(va_dpy, surface_id, draw,
                0,0,surface_width,surface_height,
                0,0,width,height,
                (test_clip==0)?NULL:&cliprects[0],
                (test_clip==0)?0:2,
                display_field);
        CHECK_VASTATUS(vaStatus,"vaPutSurface");
        putsurface_time += (get_tick_count() - start_time);

        if ((frame_num % 0xff) == 0) {
            fprintf(stderr, "%.2f FPS             \r", 256000.0 / (float)putsurface_time);
            putsurface_time = 0;

            if (test_clip) {
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
        }

#ifndef ANDROID
        if (put_pixmap)
            XCopyArea(x11_display, pixmap, win,  context, 0, 0, width, height, 0, 0);
#endif

        pthread_mutex_unlock(&surface_mutex[index]);

#ifndef ANDROID
        if (check_event) {
            pthread_mutex_lock(&gmutex);
            is_event = XCheckWindowEvent(x11_display, win, StructureNotifyMask|KeyPressMask,&event);
            pthread_mutex_unlock(&gmutex);
            if (is_event) {
                /* bail on any focused key press */
                if(event.type == KeyPress) {  
                    quit = 1;
                    break;
                }

                /* rescale the video to fit the window */
                if(event.type == ConfigureNotify) { 
                    width = event.xconfigure.width;
                    height = event.xconfigure.height;
                    printf("Scale window to %dx%d\n", width, height);
                }	
            }
        }
#endif

        row_shift++;
        if (row_shift==(2*box_width)) row_shift= 0;

        frame_num++;
#ifdef ANDROID
        if( frame_num == 0x200 )
            quit = 1;
#endif
    }
    printf("\n\n");
#ifdef ANDROID
    if(win == win_thread1)
#endif
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

    while ((c =getopt(argc,argv,"w:h:d:f:tcep?nv") ) != EOF) {
        switch (c) {
            case '?':
                printf("putsurface <options>\n");
                printf("           -d the dimension of black/write square box, default is 32\n");
                printf("           -t multi-threads\n");
#ifndef ANDROID
                printf("           -p output to pixmap\n");
                printf("           -e don't check X11 event\n");
#endif
                printf("           -c test clipbox\n");
                printf("           -f <1/2> top field, or bottom field\n");
                printf("           -v verbose output\n");
                exit(0);
                break;
            case 'w':
                win_width = atoi(optarg);
                break;
            case 'h':
                win_height = atoi(optarg);
                break;
            case 'd':
                box_width = atoi(optarg);
                break;
            case 't':
                multi_thread = 1;
                printf("Two threads to do vaPutSurface\n");
                break;
#ifndef ANDROID
            case 'e':
                check_event = 0;
                break;
            case 'p':
                put_pixmap = 1;
                break;
#endif
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

#ifndef ANDROID
    x11_display = XOpenDisplay(":0.0");
#else
    x11_display = (Display*)malloc(sizeof(Display));
    *(x11_display) = 0x18c34078;
#endif

    if (x11_display == NULL) {
        fprintf(stderr, "Can't connect X server!\n");
        exit(-1);
    }

    create_window(win_width, win_height);

    va_dpy = vaGetDisplay(x11_display);
    va_status = vaInitialize(va_dpy, &major_ver, &minor_ver);
    CHECK_VASTATUS(va_status, "vaInitialize");

    surface_width = win_width;
    surface_height = win_height;
    va_status = vaCreateSurfaces(va_dpy,surface_width, surface_height,
            VA_RT_FORMAT_YUV420, SURFACE_NUM, &surface_id[0]);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");

#ifndef ANDROID
    if (check_event)
        pthread_mutex_init(&gmutex, NULL);
#endif

    for(i = 0; i< SURFACE_NUM; i++)
        pthread_mutex_init(&surface_mutex[i], NULL);

    if (multi_thread == 1) 
        ret = pthread_create(&thread1, NULL, putsurface_thread, (void*)win_thread1);

    putsurface_thread((void *)win_thread0);

    if (multi_thread == 1) 
        pthread_join(thread1, (void **)&ret);

    vaDestroySurfaces(va_dpy,&surface_id[0],SURFACE_NUM);    
    vaTerminate(va_dpy);

    return 0;
}
