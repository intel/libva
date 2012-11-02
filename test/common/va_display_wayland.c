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
#include <string.h>
#ifdef IN_LIBVA
# include "va/wayland/va_wayland.h"
#else
# include <va/va_wayland.h>
#endif
#include "va_display.h"

struct display {
    struct wl_display          *display;
    struct wl_registry         *registry;
    struct wl_compositor       *compositor;
    struct wl_shell            *shell;
    struct wl_shell_surface    *shell_surface;
    struct wl_surface          *surface;
    unsigned int                ref_count;
    int                         event_fd;
};

static struct display *g_display;

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

static VADisplay
va_open_display_wayland(void)
{
    struct display *d;

    if (g_display) {
        d = g_display;
        d->ref_count++;
    }
    else {
        d = calloc(1, sizeof(*d));
        if (!d)
            return NULL;
        d->event_fd = -1;

        d->display = wl_display_connect(NULL);
        if (!d->display) {
            free(d);
            return NULL;
        }
        wl_display_set_user_data(d->display, d);
        d->registry = wl_display_get_registry(d->display);
        wl_registry_add_listener(d->registry, &registry_listener, d);
        d->event_fd = wl_display_get_fd(d->display);
        wl_display_dispatch(d->display);

        d->ref_count = 1;
        g_display = d;
    }
    return vaGetDisplayWl(d->display);
}

static void
va_close_display_wayland(VADisplay va_dpy)
{
    struct display * const d = g_display;

    if (!d || --d->ref_count > 0)
        return;

    if (d->surface) {
        wl_surface_destroy(d->surface);
        d->surface = NULL;
    }

    if (d->shell_surface) {
        wl_shell_surface_destroy(d->shell_surface);
        d->shell_surface = NULL;
    }

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
    free(g_display);
    g_display = NULL;
}

static int
ensure_window(VADisplay va_dpy, unsigned int width, unsigned int height)
{
    struct display * const d = g_display;

    if (!d->surface) {
        d->surface = wl_compositor_create_surface(d->compositor);
        if (!d->surface)
            return 0;
    }

    if (!d->shell_surface) {
        d->shell_surface = wl_shell_get_shell_surface(d->shell, d->surface);
        if (!d->shell_surface)
            return 0;
        wl_shell_surface_set_toplevel(d->shell_surface);
    }
    return 1;
}

static VAStatus
va_put_surface_wayland(
    VADisplay          va_dpy,
    VASurfaceID        surface,
    const VARectangle *src_rect,
    const VARectangle *dst_rect
)
{
    struct display * const d = g_display;
    VAStatus va_status;
    struct wl_buffer *buffer;

    if (!ensure_window(va_dpy, dst_rect->width, dst_rect->height))
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    va_status = vaGetSurfaceBufferWl(va_dpy, surface, VA_FRAME_PICTURE, &buffer);
    if (va_status != VA_STATUS_SUCCESS)
        return va_status;

    wl_surface_attach(d->surface, buffer, 0, 0);
    wl_surface_damage(
         d->surface,
         dst_rect->x, dst_rect->y, dst_rect->width, dst_rect->height
     );

    wl_surface_commit(d->surface);
    wl_display_flush(d->display);
    return VA_STATUS_SUCCESS;
}

const VADisplayHooks va_display_hooks_wayland = {
    "wayland",
    va_open_display_wayland,
    va_close_display_wayland,
    va_put_surface_wayland,
};
