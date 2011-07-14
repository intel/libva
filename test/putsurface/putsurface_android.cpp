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
#include <va/va.h>
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
#include <assert.h>
#include <pthread.h>

static  int android_display=0;

using namespace android;
#include "../android_winsys.cpp"

sp<SurfaceComposerClient> client;
sp<Surface> android_surface;
sp<ISurface> android_isurface;
sp<SurfaceControl> surface_ctrl;

sp<SurfaceComposerClient> client1;
sp<Surface> android_surface1;
sp<ISurface> android_isurface1;
sp<SurfaceControl> surface_ctrl1;

static void *open_display(void);
static void close_display(void *win_display);
static int create_window(void *win_display, int x, int y, int width, int height);
static int check_window_event(void *x11_display, void *win, int *width, int *height, int *quit);

#define CAST_DRAWABLE(a)  static_cast<ISurface*>((void *)(*(unsigned int *)a))
#include "putsurface_common.c"

static void *open_display()
{
    return &android_display;
}

static void close_display(void *win_display)
{
    return;
}

static int create_window(void *win_display, int x, int y, int width, int height)
{
    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    printf("Create window0 for thread0\n");
    SURFACE_CREATE(client,surface_ctrl,android_surface, android_isurface, x, y, width, height);

    drawable_thread0 = static_cast<void*>(&android_isurface);
    if (multi_thread == 0)
        return 0;

    printf("Create window1 for thread1\n");
    /* need to modify here jgl*/
    SURFACE_CREATE(client1,surface_ctrl1,android_surface1, android_isurface1, x, y, width, height);
    drawable_thread1 = static_cast<void *>(&android_isurface);
    
    return 0;
}

int check_window_event(void *win_display, void *drawble, int *width, int *height, int *quit)
{
    return 0;
}


