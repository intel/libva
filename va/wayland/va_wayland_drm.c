/*
 * va_wayland_drm.c - Wayland/DRM helpers
 *
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

/* XXX: Wayland/DRM support currently lives in Mesa libEGL.so.* library */
#define LIBWAYLAND_DRM_NAME "libEGL.so.1"

typedef struct va_wayland_drm_context {
    struct va_wayland_context   base;
    void                       *handle;
    struct wl_drm              *drm;
    struct wl_registry         *registry;
    void                       *drm_interface;
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

    if (stat(device, &st) < 0) {
        va_wayland_error("failed to identify %s: %s (errno %d)",
                         device, strerror(errno), errno);
        return;
    }

    if (!S_ISCHR(st.st_mode)) {
        va_wayland_error("%s is not a device", device);
        return;
    }

    drm_state->fd = open(device, O_RDWR);
    if (drm_state->fd < 0) {
        va_wayland_error("failed to open %s: %s (errno %d)",
                         device, strerror(errno), errno);
        return;
    }

    drmGetMagic(drm_state->fd, &magic);
    wl_drm_authenticate(wl_drm_ctx->drm, magic);
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

static const struct wl_drm_listener drm_listener = {
    drm_handle_device,
    drm_handle_format,
    drm_handle_authenticated
};

static VAStatus
va_DisplayContextGetDriverName(
    VADisplayContextP pDisplayContext,
    char            **driver_name_ptr
)
{
    VADriverContextP const ctx = pDisplayContext->pDriverContext;

    return VA_DRM_GetDriverName(ctx, driver_name_ptr);
}

void
va_wayland_drm_destroy(VADisplayContextP pDisplayContext)
{
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    struct va_wayland_drm_context * const wl_drm_ctx = pDisplayContext->opaque;
    struct drm_state * const drm_state = ctx->drm_state;

    if (wl_drm_ctx->drm) {
        wl_drm_destroy(wl_drm_ctx->drm);
        wl_drm_ctx->drm = NULL;
    }
    wl_drm_ctx->is_authenticated = 0;

    if (wl_drm_ctx->handle) {
        dlclose(wl_drm_ctx->handle);
        wl_drm_ctx->handle = NULL;
    }

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
    uint32_t            id,
    const char         *interface,
    uint32_t            version
)
{
    struct va_wayland_drm_context *wl_drm_ctx = data;

    if (strcmp(interface, "wl_drm") == 0) {
        wl_drm_ctx->drm =
            wl_registry_bind(wl_drm_ctx->registry, id, wl_drm_ctx->drm_interface, 1);
    }
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    NULL,
};

bool
va_wayland_drm_create(VADisplayContextP pDisplayContext)
{
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    struct va_wayland_drm_context *wl_drm_ctx;
    struct drm_state *drm_state;
    uint32_t id;

    wl_drm_ctx = malloc(sizeof(*wl_drm_ctx));
    if (!wl_drm_ctx)
        return false;
    wl_drm_ctx->base.destroy            = va_wayland_drm_destroy;
    wl_drm_ctx->handle                  = NULL;
    wl_drm_ctx->drm                     = NULL;
    wl_drm_ctx->drm_interface           = NULL;
    wl_drm_ctx->is_authenticated        = 0;
    pDisplayContext->opaque             = wl_drm_ctx;
    pDisplayContext->vaGetDriverName    = va_DisplayContextGetDriverName;

    drm_state = calloc(1, sizeof(struct drm_state));
    if (!drm_state)
        return false;
    drm_state->fd        = -1;
    drm_state->auth_type = 0;
    ctx->drm_state       = drm_state;

    wl_drm_ctx->handle = dlopen(LIBWAYLAND_DRM_NAME, RTLD_LAZY|RTLD_LOCAL);
    if (!wl_drm_ctx->handle)
        return false;

    wl_drm_ctx->drm_interface =
        dlsym(wl_drm_ctx->handle, "wl_drm_interface");
    if (!wl_drm_ctx->drm_interface)
        return false;

    wl_drm_ctx->registry = wl_display_get_registry(ctx->native_dpy);
    wl_registry_add_listener(wl_drm_ctx->registry, &registry_listener, wl_drm_ctx);
    wl_display_roundtrip(ctx->native_dpy);

    /* registry_handle_global should have been called by the
     * wl_display_roundtrip above
     */

    if (!wl_drm_ctx->drm)
        return false;

    wl_drm_add_listener(wl_drm_ctx->drm, &drm_listener, pDisplayContext);
    wl_display_roundtrip(ctx->native_dpy);
    if (drm_state->fd < 0)
        return false;

    wl_display_roundtrip(ctx->native_dpy);
    if (!wl_drm_ctx->is_authenticated)
        return false;
    return true;
}
