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

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>
#include <surfaceflinger/ISurfaceComposer.h>
#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurface.h>
#include <surfaceflinger/SurfaceComposerClient.h>
#include <binder/MemoryHeapBase.h>

static unsigned int fake_display = 0xdeada01d;

using namespace android;
sp<SurfaceComposerClient> client;
sp<Surface> android_surface;
sp<ISurface> android_isurface;
sp<SurfaceControl> surface_ctrl;
#include "../android_winsys.cpp"

static VADisplay
va_open_display_android(void)
{
    return vaGetDisplay(&fake_display);
}

static void
va_close_display_android(VADisplay va_dpy)
{
}

static VAStatus
va_put_surface_android(
    VADisplay          va_dpy,
    VASurfaceID        surface,
    const VARectangle *src_rect,
    const VARectangle *dst_rect
)
{
    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    printf("Create window0 for thread0\n");
    SURFACE_CREATE(
        client,
        surface_ctrl,
        android_surface,
        android_isurface,
        dst_rect->x, dst_rect->y, dst_rect->width, dst_rect->height);

    return vaPutSurface(va_dpy, surface, android_isurface,
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
