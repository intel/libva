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

static int open_device (char *dev_name)
{
    struct stat st;
    int fd;

    if (-1 == stat (dev_name, &st))
    {
        printf ("Cannot identify '%s': %d, %s\n",
                dev_name, errno, strerror (errno));
        return -1;
    }

    if (!S_ISCHR (st.st_mode))
    {
        printf ("%s is no device\n", dev_name);
        return -1;
    }

    fd = open (dev_name, O_RDWR);

    if (-1 == fd)
    {
        fprintf (stderr, "Cannot open '%s': %d, %s\n",
                 dev_name, errno, strerror (errno));
        return -1;
    }

    return fd;
}

static int va_DisplayContextIsValid (
    VADisplayContextP pDisplayContext
                                  )
{
    return (pDisplayContext != NULL &&
            pDisplayContext->pDriverContext != NULL);
}

static void va_DisplayContextDestroy (
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

static VAStatus va_DisplayContextGetDriverName (
    VADisplayContextP pDisplayContext,
    char **driver_name
)
{
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    struct drm_state * drm_state = (struct drm_state *)ctx->drm_state;

    memset(drm_state, 0, sizeof(*drm_state));
    drm_state->fd = open_device((char *)DEVICE_NAME);

    if (drm_state->fd < 0) {
        fprintf(stderr,"can't open DRM devices\n");
        return VA_STATUS_ERROR_UNKNOWN;
    }
    drm_state->auth_type = VA_DRM_AUTH_CUSTOM;

    return VA_DRM_GetDriverName(ctx, driver_name);
}


VADisplay vaGetDisplay (
    void *native_dpy /* implementation specific */
)
{
    VADisplay dpy = NULL;
    VADisplayContextP pDisplayContext;

    if (!native_dpy)
        return NULL;

    if (!dpy)
    {
        /* create new entry */
        VADriverContextP pDriverContext = 0;
        struct drm_state *drm_state = 0;
        pDisplayContext = va_newDisplayContext();
        pDriverContext  = (VADriverContextP)calloc(1, sizeof(*pDriverContext));
        drm_state       = (struct drm_state*)calloc(1, sizeof(*drm_state));
        if (pDisplayContext && pDriverContext && drm_state)
        {
            pDriverContext->native_dpy       = (void *)native_dpy;
            pDriverContext->display_type     = VA_DISPLAY_ANDROID;
            pDisplayContext->pDriverContext  = pDriverContext;
            pDisplayContext->vaIsValid       = va_DisplayContextIsValid;
            pDisplayContext->vaDestroy       = va_DisplayContextDestroy;
            pDisplayContext->vaGetDriverName = va_DisplayContextGetDriverName;
            pDriverContext->drm_state 	     = drm_state;
            dpy                              = (VADisplay)pDisplayContext;
        }
        else
        {
            if (pDisplayContext)
                free(pDisplayContext);
            if (pDriverContext)
                free(pDriverContext);
            if (drm_state)
                free(drm_state);
        }
    }
  
    return dpy;
}


extern "C"  {
    extern int fool_postp; /* do nothing for vaPutSurface if set */
    extern int trace_flag; /* trace vaPutSurface parameters */

    void va_TracePutSurface (
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
}

VAStatus vaPutSurface (
    VADisplay dpy,
    VASurfaceID surface,
    sp<ANativeWindow> draw, /* Android Native Window */
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

    if (fool_postp)
        return VA_STATUS_SUCCESS;

    if (draw == NULL)
        return VA_STATUS_ERROR_UNKNOWN;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_LOG(va_TracePutSurface, dpy, surface, static_cast<void*>(&draw), srcx, srcy, srcw, srch,
                 destx, desty, destw, desth,
                 cliprects, number_cliprects, flags );
    
    return ctx->vtable->vaPutSurface( ctx, surface, static_cast<void*>(&draw), srcx, srcy, srcw, srch, 
                                     destx, desty, destw, desth,
                                     cliprects, number_cliprects, flags );
}
