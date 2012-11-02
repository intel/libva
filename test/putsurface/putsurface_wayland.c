/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
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

#include <stddef.h>
#include <errno.h>
#include <sys/select.h>
#ifdef IN_LIBVA
# include "va/wayland/va_wayland.h"
#else
# include <va/va_wayland.h>
#endif
#include <wayland-server.h>

static void *open_display(void);
static void close_display(void *win_display);
static int create_window(void *win_display,
             int x, int y, int width, int height);
static int check_window_event(void *win_display, void *drawable,
                  int *width, int *height, int *quit);

struct display;
struct drawable;

static VAStatus
va_put_surface(
    VADisplay           dpy,
    struct drawable    *wl_drawable,
    VASurfaceID         va_surface,
    const VARectangle  *src_rect,
    const VARectangle  *dst_rect,
    const VARectangle  *cliprects,
    unsigned int        num_cliprects,
    unsigned int        flags
);

/* Glue code for the current PutSurface test design */
#define CAST_DRAWABLE(a)  (struct drawable *)(a)

static inline VADisplay
vaGetDisplay(VANativeDisplay native_dpy)
{
    return vaGetDisplayWl(native_dpy);
}

static VAStatus
vaPutSurface(
    VADisplay           dpy,
    VASurfaceID         surface,
    struct drawable    *wl_drawable,
    short               src_x,
    short               src_y,
    unsigned short      src_w,
    unsigned short      src_h,
    short               dst_x,
    short               dst_y,
    unsigned short      dst_w,
    unsigned short      dst_h,
    const VARectangle  *cliprects,
    unsigned int        num_cliprects,
    unsigned int        flags
)
{
    VARectangle src_rect, dst_rect;

    src_rect.x      = src_x;
    src_rect.y      = src_y;
    src_rect.width  = src_w;
    src_rect.height = src_h;

    dst_rect.x      = src_x;
    dst_rect.y      = src_y;
    dst_rect.width  = src_w;
    dst_rect.height = src_h;
    return va_put_surface(dpy, wl_drawable, surface, &src_rect, &dst_rect,
                          cliprects, num_cliprects, flags);
}

#include "putsurface_common.c"

struct display {
    struct wl_display  *display;
    struct wl_compositor *compositor;
    struct wl_shell    *shell;
    struct wl_registry *registry;
    int                 event_fd;
};

struct drawable {
    struct wl_display  *display;
    struct wl_surface  *surface;
    unsigned int        redraw_pending  : 1;
};

static void
frame_redraw_callback(void *data, struct wl_callback *callback, uint32_t time)
{
    struct drawable * const drawable = data;

    drawable->redraw_pending = 0;
    wl_callback_destroy(callback);
}

static const struct wl_callback_listener frame_callback_listener = {
    frame_redraw_callback
};

static VAStatus
va_put_surface(
    VADisplay           dpy,
    struct drawable    *wl_drawable,
    VASurfaceID         va_surface,
    const VARectangle  *src_rect,
    const VARectangle  *dst_rect,
    const VARectangle  *cliprects,
    unsigned int        num_cliprects,
    unsigned int        flags
)
{
    struct display *d;
    struct wl_callback *callback;
    VAStatus va_status;
    struct wl_buffer *buffer;

    if (!wl_drawable)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    d = wl_display_get_user_data(wl_drawable->display);
    if (!d)
        return VA_STATUS_ERROR_INVALID_DISPLAY;

    /* Wait for the previous frame to complete redraw */
    if (wl_drawable->redraw_pending) {
        wl_display_flush(d->display);
        while (wl_drawable->redraw_pending)
            wl_display_dispatch(wl_drawable->display);
    }

    va_status = vaGetSurfaceBufferWl(va_dpy, va_surface, VA_FRAME_PICTURE, &buffer);
    if (va_status != VA_STATUS_SUCCESS)
        return va_status;

    wl_surface_attach(wl_drawable->surface, buffer, 0, 0);
    wl_surface_damage(
        wl_drawable->surface,
        dst_rect->x, dst_rect->y, dst_rect->width, dst_rect->height
    );

    wl_display_flush(d->display);
    wl_drawable->redraw_pending = 1;
    callback = wl_surface_frame(wl_drawable->surface);
    wl_surface_commit(wl_drawable->surface);
    wl_callback_add_listener(callback, &frame_callback_listener, wl_drawable);
    return VA_STATUS_SUCCESS;
}

static void
registry_handle_global(
    void               *data,
    struct wl_registry *registry,
    uint32_t            id,
    const char         *interface,
    uint32_t            version
)
{
    struct display * const d = data;

    if (strcmp(interface, "wl_compositor") == 0)
        d->compositor =
            wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    else if (strcmp(interface, "wl_shell") == 0)
        d->shell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    NULL,
};

static void *
open_display(void)
{
    struct display *d;

    d = calloc(1, sizeof *d);
    if (!d)
        return NULL;

    d->display = wl_display_connect(NULL);
    if (!d->display)
        return NULL;

    wl_display_set_user_data(d->display, d);
    d->registry = wl_display_get_registry(d->display);
    wl_registry_add_listener(d->registry, &registry_listener, d);
    d->event_fd = wl_display_get_fd(d->display);
    wl_display_dispatch(d->display);
    return d->display;
}

static void
close_display(void *win_display)
{
    struct display * const d = wl_display_get_user_data(win_display);

    if (d->shell) {
        wl_shell_destroy(d->shell);
        d->shell = NULL;
    }

    if (d->compositor) {
        wl_compositor_destroy(d->compositor);
        d->compositor = NULL;
    }

    if (d->display) {
        wl_display_disconnect(d->display);
        d->display = NULL;
    }
    free(d);
}

static int
create_window(void *win_display, int x, int y, int width, int height)
{
    struct wl_display * const display = win_display;
    struct display * const d = wl_display_get_user_data(display);
    struct wl_surface *surface1, *surface2;
    struct wl_shell_surface *shell_surface;
    struct wl_shell_surface *shell_surface_2;
    struct drawable *drawable1, *drawable2;

    surface1 = wl_compositor_create_surface(d->compositor);
    shell_surface = wl_shell_get_shell_surface(d->shell, surface1);
    wl_shell_surface_set_toplevel(shell_surface);

    drawable1 = malloc(sizeof(*drawable1));
    drawable1->display          = display;
    drawable1->surface          = surface1;
    drawable1->redraw_pending   = 0;

    /* global out */
    drawable_thread0 = drawable1;

    if (multi_thread == 0)
        return 0;

    surface2 = wl_compositor_create_surface(d->compositor);
    shell_surface_2 = wl_shell_get_shell_surface(d->shell, surface2);
    wl_shell_surface_set_toplevel(shell_surface_2);

    drawable2 = malloc(sizeof(*drawable2));
    drawable2->display          = display;
    drawable1->surface          = surface2;
    drawable2->redraw_pending   = 0;

    /* global out */
    drawable_thread1 = drawable2;
    return 0;
}

static int
check_window_event(
    void *win_display,
    void *drawable,
    int  *width,
    int  *height,
    int  *quit
)
{
    struct wl_display * const display = win_display;
    struct display * const d = wl_display_get_user_data(display);
    struct timeval tv;
    fd_set rfds;
    int retval;

    if (check_event == 0)
        return 0;

    tv.tv_sec  = 0;
    tv.tv_usec = 0;
    do {
        FD_ZERO(&rfds);
        FD_SET(d->event_fd, &rfds);

        retval = select(d->event_fd + 1, &rfds, NULL, NULL, &tv);
        if (retval < 0) {
            perror("select");
            break;
        }
        if (retval == 1)
            wl_display_dispatch(d->display);
    } while (retval > 0);

#if 0
    /* bail on any focused key press */
    if(event.type == KeyPress) {  
        *quit = 1;
        return 0;
    }
#endif

#if 0
    /* rescale the video to fit the window */
    if(event.type == ConfigureNotify) { 
        *width = event.xconfigure.width;
        *height = event.xconfigure.height;
        printf("Scale window to %dx%d\n", width, height);
    }
#endif
    return 0;
}
