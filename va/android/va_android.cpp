/*
 * Copyright (c) 2007 Intel Corporation. All Rights Reserved.
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
#include "va_android.h"
#include "va_drmcommon.h"
#include "va_drm_utils.h"
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>


#define CHECK_SYMBOL(func) { if (!func) printf("func %s not found\n", #func); return VA_STATUS_ERROR_UNKNOWN; }
#define DEVICE_NAME "/dev/dri/renderD128"

static void va_DisplayContextDestroy(
    VADisplayContextP pDisplayContext
)
{
    struct drm_state *drm_state;

    if (pDisplayContext == NULL)
        return;

    /* close the open-ed DRM fd */
    drm_state = (struct drm_state *)pDisplayContext->pDriverContext->drm_state;
    close(drm_state->fd);

    free(pDisplayContext->pDriverContext->drm_state);
    free(pDisplayContext->pDriverContext);
    free(pDisplayContext);
}

static VAStatus va_DisplayContextConnect(
    VADisplayContextP pDisplayContext
)
{
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    struct drm_state * const drm_state = (struct drm_state *)ctx->drm_state;

    drm_state->fd = open(DEVICE_NAME, O_RDWR | O_CLOEXEC);
    if (drm_state->fd < 0) {
        fprintf(stderr, "Cannot open DRM device '%s': %d, %s\n",
                DEVICE_NAME, errno, strerror(errno));
        return VA_STATUS_ERROR_UNKNOWN;
    }
    drm_state->auth_type = VA_DRM_AUTH_CUSTOM;
    return VA_STATUS_SUCCESS;
}

static VAStatus
va_DisplayContextGetDriverNames(
    VADisplayContextP pDisplayContext,
    char            **drivers,
    unsigned         *num_drivers
)
{
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    VAStatus status = va_DisplayContextConnect(pDisplayContext);
    if (status != VA_STATUS_SUCCESS)
        return status;

    return VA_DRM_GetDriverNames(ctx, drivers, num_drivers);
}

VADisplay vaGetDisplay(
    void *native_dpy /* implementation specific */
)
{
    VADisplayContextP pDisplayContext;
    VADriverContextP  pDriverContext;
    struct drm_state *drm_state;

    if (!native_dpy)
        return NULL;

    pDisplayContext = va_newDisplayContext();
    if (!pDisplayContext)
        return NULL;

    pDisplayContext->vaDestroy       = va_DisplayContextDestroy;
    pDisplayContext->vaGetDriverNames = va_DisplayContextGetDriverNames;

    pDriverContext = va_newDriverContext(pDisplayContext);
    if (!pDriverContext) {
        free(pDisplayContext);
        return NULL;
    }

    pDriverContext->native_dpy   = (void *)native_dpy;
    pDriverContext->display_type = VA_DISPLAY_ANDROID;

    drm_state = (struct drm_state*)calloc(1, sizeof(*drm_state));
    if (!drm_state) {
        free(pDisplayContext);
        free(pDriverContext);
        return NULL;
    }

    pDriverContext->drm_state = drm_state;

    return (VADisplay)pDisplayContext;
}
