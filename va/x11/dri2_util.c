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
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include <xf86drm.h>

#include <X11/Xlibint.h>
#include <X11/Xlib.h>
#include "va.h"
#include "va_backend.h"

#include "va_dri2.h"
#include "va_dri2tokens.h"
#include "va_dricommon.h"

#define __DRI_BUFFER_FRONT_LEFT         0
#define __DRI_BUFFER_BACK_LEFT          1
#define __DRI_BUFFER_FRONT_RIGHT        2
#define __DRI_BUFFER_BACK_RIGHT         3
#define __DRI_BUFFER_DEPTH              4
#define __DRI_BUFFER_STENCIL            5
#define __DRI_BUFFER_ACCUM              6
#define __DRI_BUFFER_FAKE_FRONT_LEFT    7
#define __DRI_BUFFER_FAKE_FRONT_RIGHT   8

struct dri2_drawable 
{
    struct dri_drawable base;
    union dri_buffer buffers[5];
    int width;
    int height;
    int has_backbuffer;
    int back_index;
    int front_index;
};

static int gsDRI2SwapAvailable;

static struct dri_drawable * 
dri2CreateDrawable(VADriverContextP ctx, XID x_drawable)
{
    struct dri2_drawable *dri2_drawable;

    dri2_drawable = calloc(1, sizeof(*dri2_drawable));

    if (!dri2_drawable)
        return NULL;

    dri2_drawable->base.x_drawable = x_drawable;
    dri2_drawable->base.x = 0;
    dri2_drawable->base.y = 0;
    VA_DRI2CreateDrawable(ctx->native_dpy, x_drawable);

    return &dri2_drawable->base;
}

static void 
dri2DestroyDrawable(VADriverContextP ctx, struct dri_drawable *dri_drawable)
{
    VA_DRI2DestroyDrawable(ctx->native_dpy, dri_drawable->x_drawable);
    free(dri_drawable);
}

static void 
dri2SwapBuffer(VADriverContextP ctx, struct dri_drawable *dri_drawable)
{
    struct dri2_drawable *dri2_drawable = (struct dri2_drawable *)dri_drawable;
    XRectangle xrect;
    XserverRegion region;

    if (dri2_drawable->has_backbuffer) {
        if (gsDRI2SwapAvailable) {
            CARD64 ret;
            VA_DRI2SwapBuffers(ctx->native_dpy, dri_drawable->x_drawable, 0, 0,
                               0, &ret);
        } else {
            xrect.x = 0;
            xrect.y = 0;
            xrect.width = dri2_drawable->width;
            xrect.height = dri2_drawable->height;

            region = XFixesCreateRegion(ctx->native_dpy, &xrect, 1);
            VA_DRI2CopyRegion(ctx->native_dpy, dri_drawable->x_drawable, region,
                              DRI2BufferFrontLeft, DRI2BufferBackLeft);
            XFixesDestroyRegion(ctx->native_dpy, region);
        }
    }
}

static union dri_buffer *
dri2GetRenderingBuffer(VADriverContextP ctx, struct dri_drawable *dri_drawable)
{
    struct dri2_drawable *dri2_drawable = (struct dri2_drawable *)dri_drawable;
    int i;
    int count;
    unsigned int attachments[5];
    VA_DRI2Buffer *buffers;
    
    i = 0;
    if (dri_drawable->is_window)
        attachments[i++] = __DRI_BUFFER_BACK_LEFT;
    else
        attachments[i++] = __DRI_BUFFER_FRONT_LEFT;

    buffers = VA_DRI2GetBuffers(ctx->native_dpy, dri_drawable->x_drawable,
			     &dri2_drawable->width, &dri2_drawable->height, 
                             attachments, i, &count);
    assert(buffers);
    if (buffers == NULL)
        return NULL;

    dri2_drawable->has_backbuffer = 0;

    for (i = 0; i < count; i++) {
        dri2_drawable->buffers[i].dri2.attachment = buffers[i].attachment;
        dri2_drawable->buffers[i].dri2.name = buffers[i].name;
        dri2_drawable->buffers[i].dri2.pitch = buffers[i].pitch;
        dri2_drawable->buffers[i].dri2.cpp = buffers[i].cpp;
        dri2_drawable->buffers[i].dri2.flags = buffers[i].flags;
        
        if (buffers[i].attachment == __DRI_BUFFER_BACK_LEFT) {
            dri2_drawable->has_backbuffer = 1;
            dri2_drawable->back_index = i;
        }

        if (buffers[i].attachment == __DRI_BUFFER_FRONT_LEFT)
            dri2_drawable->front_index = i;
    }
    
    dri_drawable->width = dri2_drawable->width;
    dri_drawable->height = dri2_drawable->height;
    Xfree(buffers);

    if (dri2_drawable->has_backbuffer)
        return &dri2_drawable->buffers[dri2_drawable->back_index];

    return &dri2_drawable->buffers[dri2_drawable->front_index];
}

void
dri2Close(VADriverContextP ctx)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->drm_state;

    free_drawable_hashtable(ctx);

    if (dri_state->base.fd >= 0);
	close(dri_state->base.fd);
}

Bool 
isDRI2Connected(VADriverContextP ctx, char **driver_name)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->drm_state;
    int major, minor;
    int error_base;
    int event_base;
    char *device_name = NULL;
    drm_magic_t magic;        
    *driver_name = NULL;
    dri_state->base.fd = -1;
    dri_state->base.auth_type = VA_NONE;
    if (!VA_DRI2QueryExtension(ctx->native_dpy, &event_base, &error_base))
        goto err_out;

    if (!VA_DRI2QueryVersion(ctx->native_dpy, &major, &minor))
        goto err_out;


    if (!VA_DRI2Connect(ctx->native_dpy, RootWindow(ctx->native_dpy, ctx->x11_screen),
                     driver_name, &device_name) || !device_name)
        goto err_out;

    dri_state->base.fd = open(device_name, O_RDWR);
    assert(dri_state->base.fd >= 0);

    if (dri_state->base.fd < 0)
        goto err_out;

    if (drmGetMagic(dri_state->base.fd, &magic))
        goto err_out;

    if (!VA_DRI2Authenticate(ctx->native_dpy, RootWindow(ctx->native_dpy, ctx->x11_screen),
                          magic))
        goto err_out;

    dri_state->base.auth_type = VA_DRI2;
    dri_state->createDrawable = dri2CreateDrawable;
    dri_state->destroyDrawable = dri2DestroyDrawable;
    dri_state->swapBuffer = dri2SwapBuffer;
    dri_state->getRenderingBuffer = dri2GetRenderingBuffer;
    dri_state->close = dri2Close;
    gsDRI2SwapAvailable = (minor >= 2);

    Xfree(device_name);

    return True;

err_out:
    if (device_name)
        Xfree(device_name);

    if (*driver_name)
        Xfree(*driver_name);

    if (dri_state->base.fd >= 0)
        close(dri_state->base.fd);

    *driver_name = NULL;
    dri_state->base.fd = -1;
    
    return False;
}

