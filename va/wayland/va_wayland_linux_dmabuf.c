/*
 * va_wayland_drm.c - Wayland/linux-dmabuf helpers
 *
 * Copyright (c) 2024 Simon Ser
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
 * IN NO EVENT SHALL INTEL AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "sysdeps.h"
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <xf86drm.h>
#include "va_drmcommon.h"
#include "drm/va_drm_utils.h"
#include "va_wayland_linux_dmabuf.h"
#include "va_wayland_private.h"
#include "linux-dmabuf-v1-client-protocol.h"

typedef struct va_wayland_linux_dmabuf_context {
    struct va_wayland_context base;
    bool                      has_linux_dmabuf;
    bool                      default_feedback_done;
} VADisplayContextWaylandLinuxDmabuf;

static void
feedback_handle_done(
    void                                *data,
    struct zwp_linux_dmabuf_feedback_v1 *feedback
)
{
    VADisplayContextP const pDisplayContext = data;
    struct va_wayland_linux_dmabuf_context *wl_linux_dmabuf_ctx = pDisplayContext->opaque;

    wl_linux_dmabuf_ctx->default_feedback_done = true;

    zwp_linux_dmabuf_feedback_v1_destroy(feedback);
}

static void
feedback_handle_format_table(
    void                                *data,
    struct zwp_linux_dmabuf_feedback_v1 *feedback,
    int                                  fd,
    uint32_t                             size
)
{
    close(fd);
}

/* XXX: replace with drmGetDeviceFromDevId() */
static drmDevice *
get_drm_device_from_dev_id(dev_t dev_id)
{
    uint32_t flags = 0;
    int devices_len, i, node_type;
    drmDevice *match = NULL, *dev;
    struct stat statbuf;

    devices_len = drmGetDevices2(flags, NULL, 0);
    if (devices_len < 0) {
        return NULL;
    }
    drmDevice **devices = calloc(devices_len, sizeof(*devices));
    if (devices == NULL) {
        return NULL;
    }
    devices_len = drmGetDevices2(flags, devices, devices_len);
    if (devices_len < 0) {
        free(devices);
        return NULL;
    }

    for (i = 0; i < devices_len; i++) {
        dev = devices[i];
        for (node_type = 0; node_type < DRM_NODE_MAX; node_type++) {
            if (!(dev->available_nodes & (1 << node_type)))
                continue;

            if (stat(dev->nodes[node_type], &statbuf) != 0) {
                va_wayland_error("stat() failed for %s", dev->nodes[node_type]);
                continue;
            }

            if (statbuf.st_rdev == dev_id) {
                match = dev;
                break;
            }
        }
    }

    for (i = 0; i < devices_len; i++) {
        dev = devices[i];
        if (dev != match)
            drmFreeDevice(&dev);
    }
    free(devices);

    return match;
}

static void
feedback_handle_main_device(
    void                                *data,
    struct zwp_linux_dmabuf_feedback_v1 *feedback,
    struct wl_array                     *device_array
)
{
    dev_t dev_id;
    drmDevice *dev;
    const char *dev_path;
    VADisplayContextP const pDisplayContext = data;
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    struct drm_state * const drm_state = ctx->drm_state;

    assert(device_array->size == sizeof(dev_id));
    memcpy(&dev_id, device_array->data, sizeof(dev_id));

    dev = get_drm_device_from_dev_id(dev_id);
    if (!dev) {
        va_wayland_error("failed to get DRM device from device ID");
        return;
    }

    if (!(dev->available_nodes & (1 << DRM_NODE_RENDER)))
        goto end;

    dev_path = dev->nodes[DRM_NODE_RENDER];
    drm_state->fd = open(dev_path, O_RDWR | O_CLOEXEC);
    if (drm_state->fd < 0) {
        va_wayland_error("failed to open %s", dev_path);
        goto end;
    }

    drm_state->auth_type = VA_DRM_AUTH_CUSTOM;

end:
    drmFreeDevice(&dev);
}

static void
feedback_handle_tranche_done(
    void                                *data,
    struct zwp_linux_dmabuf_feedback_v1 *feedback
)
{
}

static void
feedback_handle_tranche_target_device(
    void                                *data,
    struct zwp_linux_dmabuf_feedback_v1 *feedback,
    struct wl_array                     *device_array
)
{
}

static void
feedback_handle_tranche_formats(
    void                                *data,
    struct zwp_linux_dmabuf_feedback_v1 *feedback,
    struct wl_array                     *indices_array
)
{
}

static void
feedback_handle_tranche_flags(
    void                                *data,
    struct zwp_linux_dmabuf_feedback_v1 *feedback,
    uint32_t                             flags
)
{
}

static const struct zwp_linux_dmabuf_feedback_v1_listener feedback_listener = {
    .done = feedback_handle_done,
    .format_table = feedback_handle_format_table,
    .main_device = feedback_handle_main_device,
    .tranche_done = feedback_handle_tranche_done,
    .tranche_target_device = feedback_handle_tranche_target_device,
    .tranche_formats = feedback_handle_tranche_formats,
    .tranche_flags = feedback_handle_tranche_flags,
};

static void
registry_handle_global(
    void               *data,
    struct wl_registry *registry,
    uint32_t            name,
    const char         *interface,
    uint32_t            version
)
{
    VADisplayContextP const pDisplayContext = data;
    struct va_wayland_linux_dmabuf_context *wl_linux_dmabuf_ctx = pDisplayContext->opaque;
    struct zwp_linux_dmabuf_v1 *linux_dmabuf;
    struct zwp_linux_dmabuf_feedback_v1 *feedback;

    if (strcmp(interface, zwp_linux_dmabuf_v1_interface.name) == 0 &&
        version >= 4) {
        wl_linux_dmabuf_ctx->has_linux_dmabuf = true;
        linux_dmabuf =
            wl_registry_bind(registry, name, &zwp_linux_dmabuf_v1_interface, 4);
        feedback = zwp_linux_dmabuf_v1_get_default_feedback(linux_dmabuf);
        zwp_linux_dmabuf_feedback_v1_add_listener(feedback, &feedback_listener, data);
        zwp_linux_dmabuf_v1_destroy(linux_dmabuf);
    }
}

static void
registry_handle_global_remove(
    void               *data,
    struct wl_registry *registry,
    uint32_t            name
)
{
}

static const struct wl_registry_listener registry_listener = {
    .global        = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};

static VAStatus
va_DisplayContextGetDriverNames(
    VADisplayContextP pDisplayContext,
    char            **drivers,
    unsigned         *num_drivers
)
{
    VADriverContextP const ctx = pDisplayContext->pDriverContext;

    return VA_DRM_GetDriverNames(ctx, drivers, num_drivers);
}

bool
va_wayland_linux_dmabuf_create(VADisplayContextP pDisplayContext)
{
    bool result = false;
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    struct VADriverVTableWayland *vtable = ctx->vtable_wayland;
    struct va_wayland_linux_dmabuf_context *wl_linux_dmabuf_ctx;
    struct drm_state *drm_state;
    struct wl_event_queue *queue = NULL;
    struct wl_display *display = NULL;
    struct wl_registry *registry = NULL;

    wl_linux_dmabuf_ctx = calloc(1, sizeof(*wl_linux_dmabuf_ctx));
    if (!wl_linux_dmabuf_ctx) {
        va_wayland_error("could not allocate wl_linux_dmabuf_ctx");
        goto end;
    }
    wl_linux_dmabuf_ctx->base.destroy = va_wayland_linux_dmabuf_destroy;
    pDisplayContext->opaque           = wl_linux_dmabuf_ctx;
    pDisplayContext->vaGetDriverNames = va_DisplayContextGetDriverNames;

    drm_state = calloc(1, sizeof(*drm_state));
    if (!drm_state) {
        va_wayland_error("could not allocate drm_state");
        goto end;
    }
    drm_state->fd        = -1;
    drm_state->auth_type = 0;
    ctx->drm_state       = drm_state;

    vtable->has_prime_sharing = 0;

    /* Use wrapped wl_display with private event queue to prevent
     * thread safety issues with applications that e.g. run an event pump
     * parallel to libva initialization.
     * Using the default queue, events might get lost and crashes occur
     * because wl_display_roundtrip is not thread-safe with respect to the
     * same queue.
     */
    queue = wl_display_create_queue(ctx->native_dpy);
    if (!queue) {
        va_wayland_error("could not create Wayland event queue");
        goto end;
    }

    display = wl_proxy_create_wrapper(ctx->native_dpy);
    if (!display) {
        va_wayland_error("could not create Wayland proxy wrapper");
        goto end;
    }
    wl_proxy_set_queue((struct wl_proxy *) display, queue);

    registry = wl_display_get_registry(display);
    if (!registry) {
        va_wayland_error("could not create wl_registry");
        goto end;
    }
    wl_registry_add_listener(registry, &registry_listener, pDisplayContext);

    if (wl_display_roundtrip_queue(ctx->native_dpy, queue) < 0) {
        va_wayland_error("failed to roundtrip Wayland queue");
        goto end;
    }

    if (!wl_linux_dmabuf_ctx->has_linux_dmabuf)
        goto end;

    while (!wl_linux_dmabuf_ctx->default_feedback_done) {
        if (wl_display_dispatch_queue(ctx->native_dpy, queue) < 0) {
            va_wayland_error("failed to dispatch Wayland queue");
            goto end;
        }
    }

    if (drm_state->fd < 0)
        goto end;

    result = true;
    vtable->has_prime_sharing = true;

end:
    if (registry)
        wl_registry_destroy(registry);
    if (display)
        wl_proxy_wrapper_destroy(display);
    if (queue)
        wl_event_queue_destroy(queue);
    return result;
}

void
va_wayland_linux_dmabuf_destroy(VADisplayContextP pDisplayContext)
{
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    struct drm_state * const drm_state = ctx->drm_state;
    struct VADriverVTableWayland *vtable = ctx->vtable_wayland;

    vtable->has_prime_sharing = 0;

    if (drm_state) {
        if (drm_state->fd >= 0) {
            close(drm_state->fd);
            drm_state->fd = -1;
        }
        free(ctx->drm_state);
        ctx->drm_state = NULL;
    }
}
