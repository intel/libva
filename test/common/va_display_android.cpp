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

#include <va/va_android.h>
#include "va_display.h"

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>

static unsigned int fake_display = 0xdeada01d;

using namespace android;

static sp<SurfaceComposerClient> client = NULL;
static sp<SurfaceControl> surface_ctr = NULL;
static sp<ANativeWindow> anw = NULL;

static VADisplay
va_open_display_android(void)
{
    return vaGetDisplay(&fake_display);
}

static void
va_close_display_android(VADisplay va_dpy)
{
}

static int create_window(int x, int y, int width, int height)
{
    client = new SurfaceComposerClient();
    
    surface_ctr = client->createSurface(
        String8("Test Surface"),
        width, height,
        PIXEL_FORMAT_RGB_888, 0);

    SurfaceComposerClient::openGlobalTransaction();
    surface_ctr->setLayer(0x7FFFFFFF);
    surface_ctr->show();
    SurfaceComposerClient::closeGlobalTransaction();
    
    SurfaceComposerClient::openGlobalTransaction();
    surface_ctr->setPosition(x, y);
    SurfaceComposerClient::closeGlobalTransaction();
    
    SurfaceComposerClient::openGlobalTransaction();
    surface_ctr->setSize(width, height);
    SurfaceComposerClient::closeGlobalTransaction();
    
    anw = surface_ctr->getSurface();
    
    return 0;
}


static VAStatus
va_put_surface_android(
    VADisplay          va_dpy,
    VASurfaceID        surface,
    const VARectangle *src_rect,
    const VARectangle *dst_rect
)
{
    if (anw == NULL)
        create_window(dst_rect->x, dst_rect->y, dst_rect->width, dst_rect->height);

    return vaPutSurface(va_dpy, surface, anw,
                        src_rect->x, src_rect->y,
                        src_rect->width, src_rect->height,
                        dst_rect->x, dst_rect->y,
                        dst_rect->width, dst_rect->height,
                        NULL, 0,
                        VA_FRAME_PICTURE);
}

extern "C"
const VADisplayHooks va_display_hooks_android = {
    "android",
    va_open_display_android,
    va_close_display_android,
    va_put_surface_android
};
