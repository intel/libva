/*
 * va_wayland_emgd.c - Wayland/EMGD helpers
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
#include <dlfcn.h>
#include "va_drmcommon.h"
#include "va_wayland_emgd.h"
#include "va_wayland_private.h"

/* XXX: Wayland/EMGD support currently lives in libwayland-emgd.so.* library */
#define LIBWAYLAND_EMGD_NAME "libwayland-emgd.so.1"

typedef struct va_wayland_emgd_context {
    struct va_wayland_context   base;
    void                       *handle;
    struct wl_emgd             *emgd;
    void                       *emgd_interface;
    unsigned int                is_created      : 1;
    struct wl_registry         *registry;
} VADisplayContextWaylandEMGD;

static inline void
wl_emgd_destroy(struct wl_emgd *emgd)
{
    wl_proxy_destroy((struct wl_proxy *)emgd);
}

static VAStatus
va_DisplayContextGetDriverName(
    VADisplayContextP pDisplayContext,
    char            **driver_name_ptr
)
{
    *driver_name_ptr = strdup("emgd");
    return VA_STATUS_SUCCESS;
}

void
va_wayland_emgd_destroy(VADisplayContextP pDisplayContext)
{
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    VADisplayContextWaylandEMGD * const wl_emgd_ctx = pDisplayContext->opaque;
    struct drm_state * const drm_state = ctx->drm_state;

    if (wl_emgd_ctx->emgd) {
        wl_emgd_destroy(wl_emgd_ctx->emgd);
        wl_emgd_ctx->emgd = NULL;
    }
    wl_emgd_ctx->is_created = 0;

    if (wl_emgd_ctx->handle) {
        dlclose(wl_emgd_ctx->handle);
        wl_emgd_ctx->handle = NULL;
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
    VADisplayContextWaylandEMGD *wl_emgd_ctx = data;

    if (strcmp(interface, "wl_emgd") == 0) {
        wl_emgd_ctx->emgd =
            wl_registry_bind(registry, id, wl_emgd_ctx->emgd_interface, 1);
    }
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    NULL,
};

bool
va_wayland_emgd_create(VADisplayContextP pDisplayContext)
{
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    VADisplayContextWaylandEMGD *wl_emgd_ctx;
    struct drm_state *drm_state;

    wl_emgd_ctx = malloc(sizeof(*wl_emgd_ctx));
    if (!wl_emgd_ctx)
        return false;
    wl_emgd_ctx->base.destroy           = va_wayland_emgd_destroy;
    wl_emgd_ctx->handle                 = NULL;
    wl_emgd_ctx->emgd                   = NULL;
    wl_emgd_ctx->emgd_interface         = NULL;
    wl_emgd_ctx->is_created             = 0;
    pDisplayContext->opaque             = wl_emgd_ctx;
    pDisplayContext->vaGetDriverName    = va_DisplayContextGetDriverName;

    drm_state = calloc(1, sizeof(struct drm_state));
    if (!drm_state)
        return false;
    drm_state->fd        = -1;
    drm_state->auth_type = 0;
    ctx->drm_state       = drm_state;

    wl_emgd_ctx->handle = dlopen(LIBWAYLAND_EMGD_NAME, RTLD_LAZY|RTLD_LOCAL);
    if (!wl_emgd_ctx->handle)
        return false;

    wl_emgd_ctx->emgd_interface =
        dlsym(wl_emgd_ctx->handle, "wl_emgd_interface");
    if (!wl_emgd_ctx->emgd_interface)
        return false;

    wl_emgd_ctx->registry = wl_display_get_registry(ctx->native_dpy);
    wl_registry_add_listener(wl_emgd_ctx->registry, &registry_listener, wl_emgd_ctx);
    wl_display_roundtrip(ctx->native_dpy);

    /* registry_handle_global should have been called by the
     * wl_display_roundtrip above
     */
    if (!wl_emgd_ctx->emgd)
        return false;
    return true;
}
