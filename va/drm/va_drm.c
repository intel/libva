/*
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
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "sysdeps.h"
#include <xf86drm.h>
#include "va_drm.h"
#include "va_backend.h"
#include "va_internal.h"
#include "va_drmcommon.h"
#include "va_drm_auth.h"
#include "va_drm_utils.h"

static void
va_DisplayContextDestroy(VADisplayContextP pDisplayContext)
{
    if (!pDisplayContext)
        return;

    free(pDisplayContext->pDriverContext->drm_state);
    free(pDisplayContext->pDriverContext);
    free(pDisplayContext);
}


static VAStatus va_DisplayContextConnect(
    VADisplayContextP pDisplayContext
)
{
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    struct drm_state * const drm_state = ctx->drm_state;
    drm_magic_t magic;
    int ret;

    /* Authentication is only needed for a legacy DRM device */
    if (ctx->display_type != VA_DISPLAY_DRM_RENDERNODES) {
        ret = drmGetMagic(drm_state->fd, &magic);
        if (ret < 0)
            return VA_STATUS_ERROR_OPERATION_FAILED;

        if (!va_drm_authenticate(drm_state->fd, magic))
            return VA_STATUS_ERROR_OPERATION_FAILED;
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

VADisplay
vaGetDisplayDRM(int fd)
{
    VADisplayContextP pDisplayContext = NULL;
    VADriverContextP  pDriverContext  = NULL;
    struct drm_state *drm_state       = NULL;
    int node_type;

    if (fd < 0 || (node_type = drmGetNodeTypeFromFd(fd)) < 0)
        return NULL;

    /* Create new entry */
    /* XXX: handle cache? */
    drm_state = calloc(1, sizeof(*drm_state));
    if (!drm_state)
        goto error;
    drm_state->fd = fd;

    pDisplayContext = va_newDisplayContext();
    if (!pDisplayContext)
        goto error;

    pDisplayContext->vaDestroy       = va_DisplayContextDestroy;
    pDisplayContext->vaGetDriverNames = va_DisplayContextGetDriverNames;

    pDriverContext = va_newDriverContext(pDisplayContext);
    if (!pDriverContext)
        goto error;

    pDriverContext->native_dpy   = NULL;
    pDriverContext->display_type = node_type == DRM_NODE_RENDER ?
                                   VA_DISPLAY_DRM_RENDERNODES : VA_DISPLAY_DRM;
    pDriverContext->drm_state    = drm_state;

    return pDisplayContext;

error:
    free(pDisplayContext);
    free(pDriverContext);
    free(drm_state);
    return NULL;
}
