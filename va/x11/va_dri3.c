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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "va_dri3.h"

Pixmap VA_DRI3_create_pixmap(Display *dpy,
                             Drawable draw,
                             int width, int height, int depth,
                             int fd, int bpp, int stride, int size)
{
    xcb_connection_t *c = XGetXCBConnection(dpy);
    if (fd >= 0) {
        xcb_pixmap_t pixmap = xcb_generate_id(c);
        xcb_dri3_pixmap_from_buffer(c, pixmap, draw, size, width, height,
                                    stride, depth, bpp, fd);
        return pixmap;
    }
    return 0;
}

int VA_DRI3_create_fd(Display *dpy, Pixmap pixmap, int *stride)
{
    xcb_connection_t *c = XGetXCBConnection(dpy);
    xcb_dri3_buffer_from_pixmap_cookie_t cookie;
    xcb_dri3_buffer_from_pixmap_reply_t *reply;

    cookie = xcb_dri3_buffer_from_pixmap(c, pixmap);
    reply = xcb_dri3_buffer_from_pixmap_reply(c, cookie, NULL);
    if (!reply) {
        return -1;
    }

    if (reply->nfd != 1) {
        free(reply);
        return -1;
    }

    *stride = reply->stride;
    return xcb_dri3_buffer_from_pixmap_reply_fds(c, reply)[0];
}

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
                       const xcb_present_notify_t *notifies)
{
    xcb_connection_t *c = XGetXCBConnection(dpy);
    xcb_present_pixmap(c, window, pixmap, serial, valid, update, x_off,
                       y_off, target_crtc, wait_fence, idle_fence, options,
                       target_msc, divisor, remainder, notifies_len, notifies);
}

static void VA_DRI3_query_version(xcb_connection_t *c, int *major, int *minor)
{
    xcb_dri3_query_version_reply_t *reply;

    reply = xcb_dri3_query_version_reply(c,
                                         xcb_dri3_query_version(c,
                                         XCB_DRI3_MAJOR_VERSION,
                                         XCB_DRI3_MINOR_VERSION),
                NULL);
    if (reply != NULL) {
        *major = reply->major_version;
        *minor = reply->minor_version;
        free(reply);
    }
}

int
VA_DRI3_create_fence(Display *dpy, Pixmap pixmap, struct dri3_fence *fence)
{
    xcb_connection_t *c = XGetXCBConnection(dpy);
    struct dri3_fence f;
    int fd;

    fd = xshmfence_alloc_shm();
    if (fd < 0) {
        return -1;
    }

    f.addr = xshmfence_map_shm(fd);
    if (f.addr == NULL) {
        close(fd);
        return -1;
    }

    f.xid = xcb_generate_id(c);
    xcb_dri3_fence_from_fd(c, pixmap, f.xid, 0, fd);

    *fence = f;
    return 0;
}

void VA_DRI3_fence_sync(Display *dpy, struct dri3_fence *fence)
{
    xcb_connection_t *c = XGetXCBConnection(dpy);

    xshmfence_reset(fence->addr);

    xcb_sync_trigger_fence(c, fence->xid);
    xcb_flush(c);

    xshmfence_await(fence->addr);
}

void VA_DRI3_fence_free(Display *dpy, struct dri3_fence *fence)
{
    xcb_connection_t *c = XGetXCBConnection(dpy);

    xshmfence_unmap_shm(fence->addr);
    xcb_sync_destroy_fence(c, fence->xid);
}

static int VA_DRI3_exists(xcb_connection_t *c)
{
    const xcb_query_extension_reply_t *ext;
    int major, minor;

    major = minor = -1;

    ext = xcb_get_extension_data(c, &xcb_dri3_id);

    if (ext != NULL && ext->present)
        VA_DRI3_query_version(c, &major, &minor);

    return major >= 0;
}

static Bool VA_DRI3_has_present(Display *dpy)
{
    xcb_connection_t *c = XGetXCBConnection(dpy);
    xcb_generic_error_t *error = NULL;
    void *reply;

    reply = xcb_present_query_version_reply(c,
                                            xcb_present_query_version(c,
                                                  XCB_PRESENT_MAJOR_VERSION,
                                                  XCB_PRESENT_MINOR_VERSION),
                                            &error);
    if (reply == NULL)
        return 0;

    free(reply);
    free(error);
    return 1;
}

int VA_DRI3_open(Display *dpy, Window root, unsigned provider)
{
    xcb_connection_t *c = XGetXCBConnection(dpy);
    xcb_dri3_open_cookie_t cookie;
    xcb_dri3_open_reply_t *reply;

    if (!VA_DRI3_exists(c)) {
        return -1;
    }

    if (!VA_DRI3_has_present(dpy))
        return -1;

    cookie = xcb_dri3_open(c, root, provider);
    reply = xcb_dri3_open_reply(c, cookie, NULL);

    if (!reply) {
        return -1;
    }

    if (reply->nfd != 1) {
        free(reply);
        return -1;
    }

    return xcb_dri3_open_reply_fds(c, reply)[0];
}

struct driver_name_map {
    const char *key;
    int         key_len;
    const char *name;
};

static const struct driver_name_map g_driver_name_map[] = {
    { "i915",       4, "i965"   },   // Intel OTC GenX driver
    { "pvrsrvkm",   8, "pvr"    },   // Intel UMG PVR driver
    { "emgd",       4, "emgd"   },   // Intel ECG PVR driver
    { "hybrid",     6, "hybrid" },   // Intel OTC Hybrid driver
    { "nouveau",    7, "nouveau"  }, // Mesa Gallium driver
    { "radeon",     6, "r600"     }, // Mesa Gallium driver
    { "amdgpu",     6, "radeonsi" }, // Mesa Gallium driver
    { NULL, }
};

Bool
VA_DRI3Connect(Display *dpy, char** driver_name, char** device_name)
{
    const struct driver_name_map *m;
    drmVersionPtr drm_version;
    int fd = VA_DRI3_open(dpy,
                          RootWindow(dpy, DefaultScreen(dpy)),
                          None);
    if(fd != -1) {
        *device_name = drmGetRenderDeviceNameFromFd(fd);

        drm_version = drmGetVersion(fd);
        if (!drm_version)
            return 0;

        for (m = g_driver_name_map; m->key != NULL; m++) {
            if (drm_version->name_len >= m->key_len &&
                strncmp(drm_version->name, m->key, m->key_len) == 0)
                break;
        }
        drmFreeVersion(drm_version);

        if (!m->name)
            return 0;
    }
    else
        return 0;

    *driver_name = strdup(m->name);
    fcntl(fd, F_SETFD, FD_CLOEXEC);

    return 1;
}
