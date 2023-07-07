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
#include "va_x11.h"
#include "va_dri2.h"
#include "va_dri2_priv.h"
#include "va_dri3.h"
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

    if (dri_state && dri_state->base.fd != -1)
        close(dri_state->base.fd);

    free(pDisplayContext->pDriverContext->drm_state);
    free(pDisplayContext->pDriverContext);
    free(pDisplayContext);
}

static VAStatus va_DisplayContextGetDriverNames(
    VADisplayContextP pDisplayContext,
    char **drivers, unsigned *num_drivers
)
{
    VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;

    const char* dri3_disable = getenv("LIBVA_DRI3_DISABLE");
    if (!dri3_disable || !atoi(dri3_disable)) {
        unsigned old_num_drivers = *num_drivers;

        vaStatus = va_DRI3_GetDriverNames(pDisplayContext, drivers, num_drivers);
        /* As of 8 July 2023, i965 and iHD drivers lack DRI3 support.
         *
         * Requests by the community were raised as early as 29 July 2017,
         * with DRI3 support landing in libva on the 27 September 2022. At of
         * time of writing it's unknown if/when that would materialise.
         *
         * To handle this on libva level, we are explicitly disabling DRI3
         * support on said drivers - it scales better than having every user
         * to set the environment override listed above.
         *
         * Omit them by default, set LIBVA_DRI3_DISABLE=0 to bypass.
         */
        if (vaStatus == VA_STATUS_SUCCESS && dri3_disable && !atoi(dri3_disable)) {
            for (unsigned i = 0; i < *num_drivers; i++) {
                if (drivers[i] && (!strcmp(drivers[i], "iHD") ||
                                   !strcmp(drivers[i], "i965")))
                    vaStatus = VA_STATUS_ERROR_UNKNOWN;
            }
            if (vaStatus == VA_STATUS_ERROR_UNKNOWN) {
                for (unsigned i = 0; i < *num_drivers; i++)
                    free(drivers[i]);
                *num_drivers = old_num_drivers;
            }
        }
    }

    if (vaStatus != VA_STATUS_SUCCESS)
        vaStatus = va_DRI2_GetDriverNames(pDisplayContext, drivers, num_drivers);
#ifdef HAVE_NVCTRL
    if (vaStatus != VA_STATUS_SUCCESS)
        vaStatus = va_NVCTRL_GetDriverNames(pDisplayContext, drivers, num_drivers);
#endif
#ifdef HAVE_FGLRX
    if (vaStatus != VA_STATUS_SUCCESS)
        vaStatus = va_FGLRX_GetDriverNames(pDisplayContext, drivers, num_drivers);
#endif

    return vaStatus;
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

    pDisplayContext->vaDestroy       = va_DisplayContextDestroy;
    pDisplayContext->vaGetDriverNames = va_DisplayContextGetDriverNames;

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

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_LOG(va_TracePutSurface, dpy, surface, (void *)draw, srcx, srcy, srcw, srch,
                 destx, desty, destw, desth,
                 cliprects, number_cliprects, flags);

    return ctx->vtable->vaPutSurface(ctx, surface, (void *)draw, srcx, srcy, srcw, srch,
                                     destx, desty, destw, desth,
                                     cliprects, number_cliprects, flags);
}
