/*
 * Copyright (C) 2009 Splitted-Desktop Systems. All Rights Reserved.
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

#include <stdlib.h>
#include "va_glx_private.h"
#include "va_glx_impl.h"

#define INIT_CONTEXT(ctx, dpy) do {                             \
        if (!vaDisplayIsValid(dpy))                             \
            return VA_STATUS_ERROR_INVALID_DISPLAY;             \
                                                                \
        ctx = ((VADisplayContextP)(dpy))->pDriverContext;       \
        if (!(ctx))                                             \
            return VA_STATUS_ERROR_INVALID_DISPLAY;             \
                                                                \
        VAStatus status = va_glx_init_context(ctx);             \
        if (status != VA_STATUS_SUCCESS)                        \
            return status;                                      \
    } while (0)

#define INVOKE(ctx, func, args) do {                            \
        VADriverVTableGLXP vtable;                              \
        vtable = &VA_DRIVER_CONTEXT_GLX(ctx)->vtable;           \
        if (!vtable->va##func##GLX)                             \
            return VA_STATUS_ERROR_UNIMPLEMENTED;               \
        status = vtable->va##func##GLX args;                    \
    } while (0)


// Destroy VA/GLX display context
static void va_DisplayContextDestroy(VADisplayContextP pDisplayContext)
{
    VADisplayContextGLXP pDisplayContextGLX;
    VADriverContextP     pDriverContext;
    VADriverContextGLXP  pDriverContextGLX;

    if (!pDisplayContext)
        return;

    pDriverContext     = pDisplayContext->pDriverContext;
    pDriverContextGLX  = pDriverContext->glx;
    if (pDriverContextGLX) {
        free(pDriverContextGLX);
        pDriverContext->glx = NULL;
    }

    pDisplayContextGLX = pDisplayContext->opaque;
    if (pDisplayContextGLX) {
        vaDestroyFunc vaDestroy = pDisplayContextGLX->vaDestroy;
        free(pDisplayContextGLX);
        pDisplayContext->opaque = NULL;
        if (vaDestroy)
            vaDestroy(pDisplayContext);
    }
}

// Return a suitable VADisplay for VA API
VADisplay vaGetDisplayGLX(Display *native_dpy)
{
    VADisplay            dpy                = NULL;
    VADisplayContextP    pDisplayContext    = NULL;
    VADisplayContextGLXP pDisplayContextGLX = NULL;
    VADriverContextP     pDriverContext;
    VADriverContextGLXP  pDriverContextGLX  = NULL;

    dpy = vaGetDisplay(native_dpy);
    if (!dpy)
        return NULL;
    pDisplayContext = (VADisplayContextP)dpy;
    pDriverContext  = pDisplayContext->pDriverContext;

    pDisplayContextGLX = calloc(1, sizeof(*pDisplayContextGLX));
    if (!pDisplayContextGLX)
        goto error;

    pDriverContextGLX = calloc(1, sizeof(*pDriverContextGLX));
    if (!pDriverContextGLX)
        goto error;

    pDriverContext->display_type  = VA_DISPLAY_GLX;
    pDisplayContextGLX->vaDestroy = pDisplayContext->vaDestroy;
    pDisplayContext->vaDestroy    = va_DisplayContextDestroy;
    pDisplayContext->opaque       = pDisplayContextGLX;
    pDriverContext->glx           = pDriverContextGLX;
    return dpy;

error:
    free(pDriverContextGLX);
    free(pDisplayContextGLX);
    pDisplayContext->vaDestroy(pDisplayContext);
    return NULL;
}

// Create a surface used for display to OpenGL
VAStatus vaCreateSurfaceGLX(
    VADisplay dpy,
    GLenum    target,
    GLuint    texture,
    void    **gl_surface
)
{
    VADriverContextP ctx;
    VAStatus status;

    /* Make sure it is a valid GL texture object */
    if (!glIsTexture(texture))
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    INIT_CONTEXT(ctx, dpy);

    INVOKE(ctx, CreateSurface, (ctx, target, texture, gl_surface));
    return status;
}

// Destroy a VA/GLX surface
VAStatus vaDestroySurfaceGLX(
    VADisplay dpy,
    void     *gl_surface
)
{
    VADriverContextP ctx;
    VAStatus status;

    INIT_CONTEXT(ctx, dpy);

    INVOKE(ctx, DestroySurface, (ctx, gl_surface));
    return status;
}

// Copy a VA surface to a VA/GLX surface
VAStatus vaCopySurfaceGLX(
    VADisplay    dpy,
    void        *gl_surface,
    VASurfaceID  surface,
    unsigned int flags
)
{
    VADriverContextP ctx;
    VAStatus status;

    INIT_CONTEXT(ctx, dpy);

    INVOKE(ctx, CopySurface, (ctx, gl_surface, surface, flags));
    return status;
}
