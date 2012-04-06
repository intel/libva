/*
 * va_wayland.c - Wayland API
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "va_wayland.h"
#include "va_wayland_drm.h"
#include "va_wayland_private.h"
#include "va_backend.h"
#include "va_backend_wayland.h"

static inline VADriverContextP
get_driver_context(VADisplay dpy)
{
    if (!vaDisplayIsValid(dpy))
        return NULL;
    return ((VADisplayContextP)dpy)->pDriverContext;
}

void
va_wayland_error(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    fprintf(stderr, "VA error: wayland: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

static int
va_DisplayContextIsValid(VADisplayContextP pDisplayContext)
{
    VADriverContextP const pDriverContext = pDisplayContext->pDriverContext;

    return (pDriverContext &&
            pDriverContext->display_type == VA_DISPLAY_WAYLAND);
}

static void
va_DisplayContextDestroy(VADisplayContextP pDisplayContext)
{
    VADriverContextP pDriverContext;

    if (!pDisplayContext)
        return;

    pDriverContext = pDisplayContext->pDriverContext;
    if (pDriverContext) {
        VADisplayContextWaylandP const pDisplayContextWl =
            pDisplayContext->opaque;
        if (pDisplayContextWl && pDisplayContextWl->finalize)
            pDisplayContextWl->finalize(pDisplayContext);
        free(pDriverContext->vtable_wayland);
        pDriverContext->vtable_wayland = NULL;
        free(pDriverContext);
        pDisplayContext->pDriverContext = NULL;
    }
    free(pDisplayContext->opaque);
    pDisplayContext->opaque = NULL;
    free(pDisplayContext);
}

static VAStatus
va_DisplayContextGetDriverName(VADisplayContextP pDisplayContext, char **name)
{
    *name = NULL;
    return VA_STATUS_ERROR_UNKNOWN;
}

/* -------------------------------------------------------------------------- */
/* --- Public interface                                                   --- */
/* -------------------------------------------------------------------------- */

static const VADisplayContextInitFunc g_display_context_init_funcs[] = {
    va_wayland_drm_init,
    NULL
};

VADisplay
vaGetDisplayWl(struct wl_display *display)
{
    VADisplayContextP pDisplayContext = NULL;
    VADisplayContextWaylandP pDisplayContextWl;
    VADriverContextP pDriverContext;
    VADisplayContextInitFunc init_backend;
    struct VADriverVTableWayland *vtable;
    unsigned int i;

    pDisplayContext = calloc(1, sizeof(*pDisplayContext));
    if (!pDisplayContext)
        return NULL;

    pDisplayContext->vadpy_magic        = VA_DISPLAY_MAGIC;
    pDisplayContext->vaIsValid          = va_DisplayContextIsValid;
    pDisplayContext->vaDestroy          = va_DisplayContextDestroy;
    pDisplayContext->vaGetDriverName    = va_DisplayContextGetDriverName;

    pDriverContext = calloc(1, sizeof(*pDriverContext));
    if (!pDriverContext)
        goto error;
    pDisplayContext->pDriverContext     = pDriverContext;

    pDriverContext->native_dpy          = display;
    pDriverContext->display_type        = VA_DISPLAY_WAYLAND;

    vtable = calloc(1, sizeof(*vtable));
    if (!vtable)
        goto error;
    pDriverContext->vtable_wayland      = vtable;

    vtable->version                     = VA_WAYLAND_API_VERSION;

    pDisplayContextWl = calloc(1, sizeof(*pDisplayContextWl));
    if (!pDisplayContextWl)
        goto error;
    pDisplayContext->opaque = pDisplayContextWl;

    init_backend = NULL;
    for (i = 0; g_display_context_init_funcs[i] != NULL; i++) {
        init_backend = g_display_context_init_funcs[i];
        if (init_backend(pDisplayContext))
            break;
        if (pDisplayContextWl->finalize)
            pDisplayContextWl->finalize(pDisplayContext);
        init_backend = NULL;
    }
    if (!init_backend)
        goto error;
    return (VADisplay)pDisplayContext;

error:
    va_DisplayContextDestroy(pDisplayContext);
    return NULL;
}

VAStatus
vaGetSurfaceBufferWl(
    VADisplay           dpy,
    VASurfaceID         surface,
    struct wl_buffer  **out_buffer
)
{
    VADriverContextP const ctx = get_driver_context(dpy);

    if (!ctx)
        return VA_STATUS_ERROR_INVALID_DISPLAY;
    if (!ctx->vtable_wayland || !ctx->vtable_wayland->vaGetSurfaceBufferWl)
        return VA_STATUS_ERROR_UNIMPLEMENTED;
    return ctx->vtable_wayland->vaGetSurfaceBufferWl(ctx, surface, out_buffer);
}

VAStatus
vaGetImageBufferWl(
    VADisplay           dpy,
    VAImageID           image,
    struct wl_buffer  **out_buffer
)
{
    VADriverContextP const ctx = get_driver_context(dpy);

    if (!ctx)
        return VA_STATUS_ERROR_INVALID_DISPLAY;
    if (!ctx->vtable_wayland || !ctx->vtable_wayland->vaGetImageBufferWl)
        return VA_STATUS_ERROR_UNIMPLEMENTED;
    return ctx->vtable_wayland->vaGetImageBufferWl(ctx, image, out_buffer);
}
