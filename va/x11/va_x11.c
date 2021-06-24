/*
 * Copyright (c) 2007 Intel Corporation. All Rights Reserved.
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

#define _GNU_SOURCE 1
#include "sysdeps.h"
#include "va.h"
#include "va_backend.h"
#include "va_internal.h"
#include "va_trace.h"
#include "va_fool.h"
#include "va_x11.h"
#include "va_dri2.h"
#include "va_dricommon.h"
#include "va_nvctrl.h"
#include "va_fglrx.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

struct driver_name_map {
    const char *key;
    int         key_len;
    const char *name;
};

static const struct driver_name_map g_dri2_driver_name_map[] = {
    { "i965",       4, "iHD"    }, // Intel iHD  VAAPI driver with i965 DRI driver
    { "i965",       4, "i965"   }, // Intel i965 VAAPI driver with i965 DRI driver
    { "iris",       4, "iHD"    }, // Intel iHD  VAAPI driver with iris DRI driver
    { "iris",       4, "i965"   }, // Intel i965 VAAPI driver with iris DRI driver
    { NULL,         0, NULL }
};

static int va_DisplayContextIsValid(
    VADisplayContextP pDisplayContext
)
{
    return (pDisplayContext != NULL &&
            pDisplayContext->pDriverContext != NULL);
}

static void va_DisplayContextDestroy(
    VADisplayContextP pDisplayContext
)
{
    VADriverContextP ctx;
    struct dri_state *dri_state;

    if (pDisplayContext == NULL)
        return;

    ctx = pDisplayContext->pDriverContext;
    dri_state = ctx->drm_state;

    if (dri_state && dri_state->close)
        dri_state->close(ctx);

    free(pDisplayContext->pDriverContext->drm_state);
    free(pDisplayContext->pDriverContext);
    free(pDisplayContext);
}

static VAStatus va_DRI2_GetNumCandidates(
    VADisplayContextP pDisplayContext,
    int *num_candidates
)
{
    char *driver_name = NULL;
    const struct driver_name_map *m = NULL;
    VADriverContextP ctx = pDisplayContext->pDriverContext;

    *num_candidates = 0;

    if (!(va_isDRI2Connected(ctx, &driver_name) && driver_name))
        return VA_STATUS_ERROR_UNKNOWN;

    for (m = g_dri2_driver_name_map; m->key != NULL; m++) {
        if (strlen(driver_name) >= m->key_len &&
            strncmp(driver_name, m->key, m->key_len) == 0) {
            (*num_candidates)++;
        }
    }

    free(driver_name);

    /*
     * If the dri2 driver name does not have a mapped vaapi driver name, then
     * assume they have the same name.
     */
    if (*num_candidates == 0)
        *num_candidates = 1;

    return VA_STATUS_SUCCESS;
}

static VAStatus va_DRI2_GetDriverName(
    VADisplayContextP pDisplayContext,
    char **driver_name_ptr,
    int candidate_index
)
{
    const struct driver_name_map *m = NULL;
    int current_index = 0;
    VADriverContextP ctx = pDisplayContext->pDriverContext;

    *driver_name_ptr = NULL;

    if (!(va_isDRI2Connected(ctx, driver_name_ptr) && *driver_name_ptr))
        return VA_STATUS_ERROR_UNKNOWN;

    for (m = g_dri2_driver_name_map; m->key != NULL; m++) {
        if (strlen(*driver_name_ptr) >= m->key_len &&
            strncmp(*driver_name_ptr, m->key, m->key_len) == 0) {
            if (current_index == candidate_index) {
                break;
            }
            current_index++;
        }
    }

    /*
     * If the dri2 driver name does not have a mapped vaapi driver name, then
     * assume they have the same name.
     */
    if (!m->name)
        return VA_STATUS_SUCCESS;

    /* Use the mapped vaapi driver name */
    free(*driver_name_ptr);
    *driver_name_ptr = strdup(m->name);
    if (!*driver_name_ptr)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    return VA_STATUS_SUCCESS;
}

static VAStatus va_NVCTRL_GetDriverName(
    VADisplayContextP pDisplayContext,
    char **driver_name,
    int candidate_index
)
{
    VADriverContextP ctx = pDisplayContext->pDriverContext;
    int direct_capable, driver_major, driver_minor, driver_patch;
    Bool result;

    if (candidate_index != 0)
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    result = VA_NVCTRLQueryDirectRenderingCapable(ctx->native_dpy, ctx->x11_screen,
             &direct_capable);
    if (!result || !direct_capable)
        return VA_STATUS_ERROR_UNKNOWN;

    result = VA_NVCTRLGetClientDriverName(ctx->native_dpy, ctx->x11_screen,
                                          &driver_major, &driver_minor,
                                          &driver_patch, driver_name);
    if (!result)
        return VA_STATUS_ERROR_UNKNOWN;

    return VA_STATUS_SUCCESS;
}

static VAStatus va_FGLRX_GetDriverName(
    VADisplayContextP pDisplayContext,
    char **driver_name,
    int candidate_index
)
{
    VADriverContextP ctx = pDisplayContext->pDriverContext;
    int driver_major, driver_minor, driver_patch;
    Bool result;

    if (candidate_index != 0)
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    result = VA_FGLRXGetClientDriverName(ctx->native_dpy, ctx->x11_screen,
                                         &driver_major, &driver_minor,
                                         &driver_patch, driver_name);
    if (!result)
        return VA_STATUS_ERROR_UNKNOWN;

    return VA_STATUS_SUCCESS;
}

static VAStatus va_DisplayContextGetDriverName(
    VADisplayContextP pDisplayContext,
    char **driver_name, int candidate_index
)
{
    VAStatus vaStatus;

    if (driver_name)
        *driver_name = NULL;
    else
        return VA_STATUS_ERROR_UNKNOWN;

    vaStatus = va_DRI2_GetDriverName(pDisplayContext, driver_name, candidate_index);
    if (vaStatus != VA_STATUS_SUCCESS)
        vaStatus = va_NVCTRL_GetDriverName(pDisplayContext, driver_name, candidate_index);
    if (vaStatus != VA_STATUS_SUCCESS)
        vaStatus = va_FGLRX_GetDriverName(pDisplayContext, driver_name, candidate_index);

    return vaStatus;
}

static VAStatus va_DisplayContextGetNumCandidates(
    VADisplayContextP pDisplayContext,
    int *num_candidates
)
{
    VAStatus vaStatus;

    vaStatus = va_DRI2_GetNumCandidates(pDisplayContext, num_candidates);

    /* A call to va_DisplayContextGetDriverName will fallback to other
     * methods (i.e. NVCTRL, FGLRX) when DRI2 is unsuccessful.  All of those
     * fallbacks only have 1 candidate driver.
     */
    if (vaStatus != VA_STATUS_SUCCESS)
        *num_candidates = 1;

    return VA_STATUS_SUCCESS;
}

VADisplay vaGetDisplay(
    Display *native_dpy /* implementation specific */
)
{
    VADisplayContextP pDisplayContext;
    VADriverContextP  pDriverContext;
    struct dri_state *dri_state;

    if (!native_dpy)
        return NULL;

    pDisplayContext = va_newDisplayContext();
    if (!pDisplayContext)
        return NULL;

    pDisplayContext->vaIsValid       = va_DisplayContextIsValid;
    pDisplayContext->vaDestroy       = va_DisplayContextDestroy;
    pDisplayContext->vaGetNumCandidates = va_DisplayContextGetNumCandidates;
    pDisplayContext->vaGetDriverNameByIndex = va_DisplayContextGetDriverName;

    pDriverContext = va_newDriverContext(pDisplayContext);
    if (!pDriverContext) {
        free(pDisplayContext);
        return NULL;
    }

    pDriverContext->native_dpy   = (void *)native_dpy;
    pDriverContext->x11_screen   = XDefaultScreen(native_dpy);
    pDriverContext->display_type = VA_DISPLAY_X11;

    dri_state = calloc(1, sizeof(*dri_state));
    if (!dri_state) {
        free(pDisplayContext);
        free(pDriverContext);
        return NULL;
    }

    dri_state->base.fd = -1;
    dri_state->base.auth_type = VA_NONE;

    pDriverContext->drm_state = dri_state;

    return (VADisplay)pDisplayContext;
}

void va_TracePutSurface(
    VADisplay dpy,
    VASurfaceID surface,
    void *draw, /* the target Drawable */
    short srcx,
    short srcy,
    unsigned short srcw,
    unsigned short srch,
    short destx,
    short desty,
    unsigned short destw,
    unsigned short desth,
    VARectangle *cliprects, /* client supplied clip list */
    unsigned int number_cliprects, /* number of clip rects in the clip list */
    unsigned int flags /* de-interlacing flags */
);

VAStatus vaPutSurface(
    VADisplay dpy,
    VASurfaceID surface,
    Drawable draw, /* X Drawable */
    short srcx,
    short srcy,
    unsigned short srcw,
    unsigned short srch,
    short destx,
    short desty,
    unsigned short destw,
    unsigned short desth,
    VARectangle *cliprects, /* client supplied clip list */
    unsigned int number_cliprects, /* number of clip rects in the clip list */
    unsigned int flags /* de-interlacing flags */
)
{
    VADriverContextP ctx;

    if (va_fool_postp)
        return VA_STATUS_SUCCESS;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_LOG(va_TracePutSurface, dpy, surface, (void *)draw, srcx, srcy, srcw, srch,
                 destx, desty, destw, desth,
                 cliprects, number_cliprects, flags);

    return ctx->vtable->vaPutSurface(ctx, surface, (void *)draw, srcx, srcy, srcw, srch,
                                     destx, desty, destw, desth,
                                     cliprects, number_cliprects, flags);
}
