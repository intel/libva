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
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <assert.h>
#include <pthread.h>

using namespace android;

static  int android_display=0;

static sp<SurfaceComposerClient> client0 = NULL;
static sp<SurfaceControl> surface_ctrl0 = NULL;
static sp<ANativeWindow> anw0 = NULL;

static sp<SurfaceComposerClient> client1 = NULL;
static sp<SurfaceControl> surface_ctrl1 = NULL;
static sp<ANativeWindow> anw1 = NULL;

static void *open_display(void);
static void close_display(void *win_display);
static int create_window(void *win_display, int x, int y, int width, int height);
static int check_window_event(void *x11_display, void *win, int *width, int *height, int *quit);

#define CAST_DRAWABLE(a)  static_cast<ANativeWindow *>((void *)(*(unsigned int *)a))
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
    client0 = new SurfaceComposerClient();
    
    surface_ctrl0 = client1->createSurface(
        String8("Test Surface"),
        width, height,
        PIXEL_FORMAT_RGB_888, 0);

    SurfaceComposerClient::openGlobalTransaction();
    surface_ctrl0->setLayer(0x7FFFFFFF);
    surface_ctrl0->show();
    SurfaceComposerClient::closeGlobalTransaction();
    
    SurfaceComposerClient::openGlobalTransaction();
    surface_ctrl0->setPosition(x, y);
    SurfaceComposerClient::closeGlobalTransaction();
    
    SurfaceComposerClient::openGlobalTransaction();
    surface_ctrl0->setSize(width, height);
    SurfaceComposerClient::closeGlobalTransaction();
    
    anw0 = surface_ctrl0->getSurface();

    drawable_thread0 = static_cast<void*>(&anw0);
    if (multi_thread == 0)
        return 0;

    printf("Create window1 for thread1\n");
    client1 = new SurfaceComposerClient();
    
    surface_ctrl1 = client1->createSurface(
        String8("Test Surface"),
        width, height,
        PIXEL_FORMAT_RGB_888, 0);

    SurfaceComposerClient::openGlobalTransaction();
    surface_ctrl1->setLayer(0x7FFFFFFF);
    surface_ctrl1->show();
    SurfaceComposerClient::closeGlobalTransaction();
    
    SurfaceComposerClient::openGlobalTransaction();
    surface_ctrl1->setPosition(x*2, y*2);
    SurfaceComposerClient::closeGlobalTransaction();
    
    SurfaceComposerClient::openGlobalTransaction();
    surface_ctrl1->setSize(width, height);
    SurfaceComposerClient::closeGlobalTransaction();
    
    anw1 = surface_ctrl1->getSurface();

    drawable_thread1 = static_cast<void *>(&anw1);
    
    return 0;
}

int check_window_event(void *win_display, void *drawble, int *width, int *height, int *quit)
{
    return 0;
}


