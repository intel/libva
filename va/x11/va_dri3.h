/*
 * Copyright (c) 2018 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef VA_DRI3_H
#define VA_DRI3_H

#include <X11/Xlib-xcb.h>
#include <xcb/dri3.h>
#include <xcb/sync.h>
#include <X11/xshmfence.h>

#include "va_dricommon.h"

int VA_DRI3_open(Display *dpy, Window root, unsigned provider);
Pixmap VA_DRI3_create_pixmap(Display *dpy, Drawable draw, int width,
                             int height, int depth, int fd, int bpp,
                             int stride, int size);
int VA_DRI3_create_fd(Display *dpy, Pixmap pixmap, int *stride);
Bool VA_DRI3Connect(Display *dpy, char** driver_name, char** device_name);

int
VA_DRI3_create_fence(Display *dpy, Pixmap pixmap, struct dri3_fence *fence);
void VA_DRI3_fence_sync(Display *dpy, struct dri3_fence *fence);
void VA_DRI3_fence_free(Display *dpy, struct dri3_fence *fence);
void
VA_DRI3_present_pixmap(Display *dpy,
                       xcb_window_t window,
                       xcb_pixmap_t pixmap,
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
                       const xcb_present_notify_t *notifies);
#endif /* VA_DRI3_H */
