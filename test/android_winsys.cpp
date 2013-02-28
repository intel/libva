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
#include <ui/DisplayInfo.h>

namespace android {
};

#define min(a,b) (a<b?a:b)
#define SURFACE_CREATE(client,surface_ctrl,android_surface, android_isurface, x, y, win_width, win_height) \
do {                                                                    \
    client = new SurfaceComposerClient();                               \
    android::DisplayInfo info;                                          \
    int w, h;                                                           \
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(        \
                  ISurfaceComposer::eDisplayIdMain));                   \
    client->getDisplayInfo(dtoken, &info);                              \
    /*w = min(win_width, info.w);*/                                     \
    /*h = min(win_height, info.h);*/                                    \
    w = win_width, h = win_height;                                      \
                                                                        \
    surface_ctrl = client->createSurface(String8("libVA"), w, h, PIXEL_FORMAT_RGB_888); \
    android_surface = surface_ctrl->getSurface();                       \
                                                                        \
    SurfaceComposerClient::openGlobalTransaction();                     \
    surface_ctrl->setLayer(0x7FFFFFFF);                                 \
    surface_ctrl->show();                                               \
    SurfaceComposerClient::closeGlobalTransaction();                    \
                                                                        \
    SurfaceComposerClient::openGlobalTransaction();                     \
    surface_ctrl->setPosition(0, 0);                                    \
    SurfaceComposerClient::closeGlobalTransaction();                    \
                                                                        \
    SurfaceComposerClient::openGlobalTransaction();                     \
    surface_ctrl->setSize(w, h);                                        \
    SurfaceComposerClient::closeGlobalTransaction();                    \
} while (0)


