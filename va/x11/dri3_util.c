/*
 * Copyright (c) 2018 Intel Corporation. All Rights Reserved.
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

#include <fcntl.h>
#include <X11/Xlibint.h>
#include <unistd.h>

#include "va_dri3.h"

int
va_dri3_createfd(VADriverContextP ctx, Pixmap pixmap, int *stride)
{
    return VA_DRI3_create_fd(ctx->native_dpy, pixmap, stride);
}

Pixmap
va_dri3_createPixmap(VADriverContextP ctx, Drawable draw,
                     int width, int height, int depth,
                     int fd, int bpp, int stride, int size)
{
    return VA_DRI3_create_pixmap(ctx->native_dpy,
                                 draw, width, height,
                                 depth, fd, bpp,
                                 stride, size);
}

void
va_dri3_presentPixmap(VADriverContextP ctx,
                      Drawable draw,
                      Pixmap pixmap,
                      unsigned int serial,
                      xcb_xfixes_region_t valid,
                      xcb_xfixes_region_t update,
                      unsigned short int x_off,
                      unsigned short int y_off,
                      xcb_randr_crtc_t target_crtc,
                      xcb_sync_fence_t wait_fence,
                      xcb_sync_fence_t idle_fence,
                      unsigned int options,
                      unsigned long int target_msc,
                      unsigned long int divisor,
                      unsigned long int  remainder,
                      unsigned int notifies_len,
                      const xcb_present_notify_t *notifies)
{
    VA_DRI3_present_pixmap(ctx->native_dpy, draw,
                           pixmap, serial, valid, update,
                           x_off, y_off, target_crtc, wait_fence,
                           idle_fence, options, target_msc, divisor,
                           remainder, notifies_len, notifies);
}

int
va_dri3_create_fence(VADriverContextP ctx, Pixmap pixmap,
                     struct dri3_fence *fence)
{
    return VA_DRI3_create_fence(ctx->native_dpy, pixmap, fence);
}

void va_dri3_fence_sync(VADriverContextP ctx, struct dri3_fence *fence)
{
    VA_DRI3_fence_sync(ctx->native_dpy, fence);
}

void va_dri3_fence_free(VADriverContextP ctx, struct dri3_fence *fence)
{
    VA_DRI3_fence_free(ctx->native_dpy, fence);
}

void
va_dri3_close(VADriverContextP ctx)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->drm_state;
    if(dri_state->base.fd >= 0)
        close(dri_state->base.fd);
}

Bool
va_isDRI3Connected(VADriverContextP ctx, char** driver_name)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->drm_state;
    char *device_name = NULL;
    *driver_name = NULL;
    dri_state->base.fd = -1;
    dri_state->base.auth_type = VA_DRM_AUTH_CUSTOM;

    if(!VA_DRI3Connect(ctx->native_dpy, driver_name, &device_name))
            goto err_out;

    dri_state->base.fd = open(device_name, O_RDWR);

    if(dri_state->base.fd < 0)
        goto err_out;

    dri_state->createfd         = va_dri3_createfd;
    dri_state->createPixmap     = va_dri3_createPixmap;
    dri_state->close            = va_dri3_close;
    dri_state->create_fence     = va_dri3_create_fence;
    dri_state->fence_free       = va_dri3_fence_free;
    dri_state->fence_sync       = va_dri3_fence_sync;

    Xfree(device_name);

    return True;

err_out:
    if(device_name)
        Xfree(device_name);
    if(*driver_name)
        Xfree(*driver_name);
    if(dri_state->base.fd >= 0)
        close(dri_state->base.fd);
    dri_state->base.fd = -1;

    return False;
}
