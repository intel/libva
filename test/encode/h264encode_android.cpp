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

using namespace android;
#include "../android_winsys.cpp"
#include "h264encode_common.c"

sp<SurfaceComposerClient> client;
sp<Surface> android_surface;
sp<ISurface> android_isurface;
sp<SurfaceControl> surface_ctrl;

static int display_surface(int frame_id, int *exit_encode)
{
    VAStatus va_status;

    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    printf("Create window0 for thread0\n");
    SURFACE_CREATE(client,surface_ctrl,android_surface, android_isurface, 0, 0, win_width, win_height);
    va_status = vaPutSurface(va_dpy, surface_id[frame_id], android_isurface,
            0,0, frame_width, frame_height,
            0,0, win_width, win_height,
            NULL,0,0);

    *exit_encode = 0;
    return 0;
}

