/*
 * va_wayland_drm.c - Wayland/DRM helpers
 *
 * Copyright (c) 2012 Intel Corporation. All Rights Reserved.
 * Copyright (c) 2023 Emil Velikov
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
#include "va_wayland_drm.h"
#include "va_wayland_private.h"
#include "wayland-drm-client-protocol.h"

typedef struct va_wayland_drm_context {
    struct va_wayland_context   base;
    struct wl_event_queue      *queue;
    struct wl_drm              *drm;
    uint32_t                    drm_name;
    struct wl_registry         *registry;
    unsigned int                is_authenticated        : 1;
} VADisplayContextWaylandDRM;

static void
drm_handle_device(void *data, struct wl_drm *drm, const char *device)
{
    VADisplayContextP const pDisplayContext = data;
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    VADisplayContextWaylandDRM * const wl_drm_ctx = pDisplayContext->opaque;
    struct drm_state * const drm_state = ctx->drm_state;
    drm_magic_t magic;
    struct stat st;
    int fd = -1;

    fd = open(device, O_RDWR);
    if (fd < 0) {
        va_wayland_error("failed to open %s: %s (errno %d)",
                         device, strerror(errno), errno);
        return;
    }

    if (fstat(fd, &st) < 0) {
        va_wayland_error("failed to identify %s: %s (errno %d)",
                         device, strerror(errno), errno);
        close(fd);
        return;
    }

    if (!S_ISCHR(st.st_mode)) {
        va_wayland_error("%s is not a device", device);
        close(fd);
        return;
    }

    drm_state->fd = fd;

    int type = drmGetNodeTypeFromFd(fd);
    if (type != DRM_NODE_RENDER) {
        drmGetMagic(drm_state->fd, &magic);
        wl_drm_authenticate(wl_drm_ctx->drm, magic);
    } else {
        wl_drm_ctx->is_authenticated = 1;
        drm_state->auth_type         = VA_DRM_AUTH_CUSTOM;
    }
}

static void
drm_handle_format(void *data, struct wl_drm *drm, uint32_t format)
{
}

static void
drm_handle_authenticated(void *data, struct wl_drm *drm)
{
    VADisplayContextP const pDisplayContext = data;
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    VADisplayContextWaylandDRM * const wl_drm_ctx = pDisplayContext->opaque;
    struct drm_state * const drm_state = ctx->drm_state;

    wl_drm_ctx->is_authenticated = 1;
    drm_state->auth_type         = VA_DRM_AUTH_CUSTOM;
}

static void
drm_handle_capabilities(void *data, struct wl_drm *wl_drm, uint32_t value)
{
    VADisplayContextP const pDisplayContext = data;
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    struct VADriverVTableWayland *vtable = ctx->vtable_wayland;

    vtable->has_prime_sharing = !!(value & WL_DRM_CAPABILITY_PRIME);
}

static const struct wl_drm_listener drm_listener = {
    drm_handle_device,
    drm_handle_format,
    drm_handle_authenticated,
    drm_handle_capabilities,
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

void
va_wayland_drm_destroy(VADisplayContextP pDisplayContext)
{
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    struct va_wayland_drm_context * const wl_drm_ctx = pDisplayContext->opaque;
    struct drm_state * const drm_state = ctx->drm_state;
    struct VADriverVTableWayland *vtable = ctx->vtable_wayland;

    vtable->has_prime_sharing = 0;
    vtable->wl_interface = NULL;

    wl_drm_ctx->is_authenticated = 0;

    if (drm_state) {
        if (drm_state->fd >= 0) {
            close(drm_state->fd);
            drm_state->fd = -1;
        }
        free(ctx->drm_state);
        ctx->drm_state = NULL;
    }
}

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
    struct va_wayland_drm_context * const wl_drm_ctx = pDisplayContext->opaque;

    if (strcmp(interface, "wl_drm") == 0) {
        /* bind to at most version 2, but also support version 1 if
         * compositor does not have v2
         */
        wl_drm_ctx->drm_name = name;
        wl_drm_ctx->drm =
            wl_registry_bind(wl_drm_ctx->registry, name, &wl_drm_interface,
                             (version < 2) ? version : 2);

        if (wl_drm_ctx->drm
            && wl_drm_add_listener(wl_drm_ctx->drm, &drm_listener, pDisplayContext) != 0) {
            va_wayland_error("could not add listener to wl_drm");
            wl_drm_destroy(wl_drm_ctx->drm);
            wl_drm_ctx->drm = NULL;
        }
    }
}

static void
registry_handle_global_remove(
    void               *data,
    struct wl_registry *registry,
    uint32_t            name
)
{
    struct va_wayland_drm_context *wl_drm_ctx = data;

    if (wl_drm_ctx->drm && name == wl_drm_ctx->drm_name) {
        wl_drm_destroy(wl_drm_ctx->drm);
        wl_drm_ctx->drm = NULL;
    }
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove
};

static bool
wayland_roundtrip_queue(struct wl_display *display,
                        struct wl_event_queue *queue)
{
    if (wl_display_roundtrip_queue(display, queue) < 0) {
        int err = wl_display_get_error(display);
        va_wayland_error("Wayland roundtrip error: %s (errno %d)", strerror(err), err);
        return false;
    } else {
        return true;
    }
}

bool
va_wayland_drm_create(VADisplayContextP pDisplayContext)
{
    bool result = false;
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    struct va_wayland_drm_context *wl_drm_ctx;
    struct drm_state *drm_state;
    struct VADriverVTableWayland *vtable = ctx->vtable_wayland;
    struct wl_display *wrapped_display = NULL;

    vtable->wl_interface = NULL;

    wl_drm_ctx = malloc(sizeof(*wl_drm_ctx));
    if (!wl_drm_ctx) {
        va_wayland_error("could not allocate wl_drm_ctx");
        goto end;
    }
    wl_drm_ctx->base.destroy            = va_wayland_drm_destroy;
    wl_drm_ctx->queue                   = NULL;
    wl_drm_ctx->drm                     = NULL;
    wl_drm_ctx->registry                = NULL;
    wl_drm_ctx->is_authenticated        = 0;
    pDisplayContext->opaque             = wl_drm_ctx;
    pDisplayContext->vaGetDriverNames    = va_DisplayContextGetDriverNames;

    drm_state = calloc(1, sizeof(struct drm_state));
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
    wl_drm_ctx->queue = wl_display_create_queue(ctx->native_dpy);
    if (!wl_drm_ctx->queue) {
        va_wayland_error("could not create Wayland event queue");
        goto end;
    }

    wrapped_display = wl_proxy_create_wrapper(ctx->native_dpy);
    if (!wrapped_display) {
        va_wayland_error("could not create Wayland proxy wrapper");
        goto end;
    }

    /* All created objects will inherit this queue */
    wl_proxy_set_queue((struct wl_proxy *) wrapped_display, wl_drm_ctx->queue);
    wl_drm_ctx->registry = wl_display_get_registry(wrapped_display);
    if (!wl_drm_ctx->registry) {
        va_wayland_error("could not create wl_registry");
        goto end;
    }
    if (wl_registry_add_listener(wl_drm_ctx->registry, &registry_listener, pDisplayContext) != 0) {
        va_wayland_error("could not add listener to wl_registry");
        goto end;
    }
    if (!wayland_roundtrip_queue(ctx->native_dpy, wl_drm_ctx->queue))
        goto end;

    /* registry_handle_global should have been called by the
     * wl_display_roundtrip_queue above
     */

    /* Do not print an error, the compositor might just not support wl_drm */
    if (!wl_drm_ctx->drm)
        goto end;
    if (!wayland_roundtrip_queue(ctx->native_dpy, wl_drm_ctx->queue))
        goto end;
    if (drm_state->fd < 0) {
        va_wayland_error("did not get DRM device");
        goto end;
    }

    if (!wayland_roundtrip_queue(ctx->native_dpy, wl_drm_ctx->queue))
        goto end;

    if (!wl_drm_ctx->is_authenticated) {
        va_wayland_error("Wayland compositor did not respond to DRM authentication");
        goto end;
    }

    vtable->wl_interface = &wl_drm_interface;
    result = true;

end:
    if (wrapped_display) {
        wl_proxy_wrapper_destroy(wrapped_display);
        wrapped_display = NULL;
    }

    if (wl_drm_ctx) {
        if (wl_drm_ctx->drm) {
            wl_drm_destroy(wl_drm_ctx->drm);
            wl_drm_ctx->drm = NULL;
        }

        if (wl_drm_ctx->registry) {
            wl_registry_destroy(wl_drm_ctx->registry);
            wl_drm_ctx->registry = NULL;
        }

        if (wl_drm_ctx->queue) {
            wl_event_queue_destroy(wl_drm_ctx->queue);
            wl_drm_ctx->queue = NULL;
        }
    }

    return result;
}
