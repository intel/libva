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

#include <stdio.h>
#include <stdbool.h>
#include <va/va_x11.h>
#include "va_display.h"

static Display *x11_display;
static Window   x11_window;

static VADisplay
va_open_display_x11(void)
{
    x11_display = XOpenDisplay(NULL);
    if (!x11_display) {
        fprintf(stderr, "error: can't connect to X server!\n");
        return NULL;
    }
    return vaGetDisplay(x11_display);
}

static void
va_close_display_x11(VADisplay va_dpy)
{
    if (!x11_display)
        return;

    if (x11_window) {
        XUnmapWindow(x11_display, x11_window);
        XDestroyWindow(x11_display, x11_window);
        x11_window = None;
    }
    XCloseDisplay(x11_display);
    x11_display = NULL;
}

static int
ensure_window(unsigned int width, unsigned int height)
{
    Window win, rootwin;
    unsigned int black_pixel, white_pixel;
    int screen;

    if (!x11_display)
        return 0;

    if (x11_window) {
        XResizeWindow(x11_display, x11_window, width, height);
        return 1;
    }

    screen      = DefaultScreen(x11_display);
    rootwin     = RootWindow(x11_display, screen);
    black_pixel = BlackPixel(x11_display, screen);
    white_pixel = WhitePixel(x11_display, screen);

    win = XCreateSimpleWindow(
        x11_display,
        rootwin,
        0, 0, width, height,
        1, black_pixel, white_pixel
    );
    if (!win)
        return 0;
    x11_window = win;

    XMapWindow(x11_display, x11_window);
    XSync(x11_display, False);
    return 1;
}

static inline bool
validate_rect(const VARectangle *rect)
{
    return (rect            &&
            rect->x >= 0    &&
            rect->y >= 0    &&
            rect->width > 0 &&
            rect->height > 0);
}

static VAStatus
va_put_surface_x11(
    VADisplay          va_dpy,
    VASurfaceID        surface,
    const VARectangle *src_rect,
    const VARectangle *dst_rect
)
{
    unsigned int win_width, win_height;

    if (!va_dpy)
        return VA_STATUS_ERROR_INVALID_DISPLAY;
    if (surface == VA_INVALID_SURFACE)
        return VA_STATUS_ERROR_INVALID_SURFACE;
    if (!validate_rect(src_rect) || !validate_rect(dst_rect))
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    win_width  = dst_rect->x + dst_rect->width;
    win_height = dst_rect->y + dst_rect->height;
    if (!ensure_window(win_width, win_height))
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    return vaPutSurface(va_dpy, surface, x11_window,
                        src_rect->x, src_rect->y,
                        src_rect->width, src_rect->height,
                        dst_rect->x, dst_rect->y,
                        dst_rect->width, dst_rect->height,
                        NULL, 0,
                        VA_FRAME_PICTURE);
}

const VADisplayHooks va_display_hooks_x11 = {
    "x11",
    va_open_display_x11,
    va_close_display_x11,
    va_put_surface_x11,
};
