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

/*
 * Initial EGL backend, and subject to change
 *
 * Gstreamer gst-gltexture has a framework to support associating a buffer
 * to a texture via EGL_KHR_image_base and GL_OES_EGL_image_external.
 *
 * EGL_KHR_image_base:
 *   EGLImageKHR eglCreateImageKHR(
 *                           EGLDisplay dpy,
 *                           EGLContext ctx,
 *                           EGLenum target,
 *                           EGLClientBuffer buffer,
 *                           const EGLint *attrib_list)
 *
 * GL_OES_EGL_image_external:
 * This extension provides a mechanism for creating EGLImage texture targets
 * from EGLImages.  This extension defines a new texture target TEXTURE_EXTERNAL_OES.
 * This texture target can only be specified using an EGLImage.
 * The first eglCreateImageKHR will create an EGLImage from an EGLClientBufferm, and with
 * an EGLImage, gst-gltexture can use GL_OES_EGL_image_external extension to create textures.
 *
 * eglCreateImageKHR and GL_OES_EGL_image_external are all called directly from gst-gltexture,
 * thus the simplest way to support gst-gltexture is defining a new API to pass EGLClientBuffer
 * to gst-gltexture.
 *
 * EGLClientBuffer is gfx/video driver implementation specific (?). It means we need to pass up
 * the low-level buffer ID (or handle) of the decoded surface to gst-gltexture, and gst-gltexture
 * then pass down it to gfx driver.  
 *
 * Bellow API vaGetEGLClientBufferFromSurface is for this purpose
 */
#include "va_egl_private.h"
#include "va_egl_impl.h"

#define CTX(dpy) (((VADisplayContextP)dpy)->pDriverContext)
#define CHECK_DISPLAY(dpy) if( !vaDisplayIsValid(dpy) ) { return VA_STATUS_ERROR_INVALID_DISPLAY; }

#define INIT_CONTEXT(ctx, dpy) do {                             \
        if (!vaDisplayIsValid(dpy))                             \
            return VA_STATUS_ERROR_INVALID_DISPLAY;             \
                                                                \
        ctx = ((VADisplayContextP)(dpy))->pDriverContext;       \
        if (!(ctx))                                             \
            return VA_STATUS_ERROR_INVALID_DISPLAY;             \
                                                                \
        status = va_egl_init_context(dpy);                      \
        if (status != VA_STATUS_SUCCESS)                        \
            return status;                                      \
    } while (0)

#define INVOKE(ctx, func, args) do {                            \
        VADriverVTablePrivEGLP vtable;                          \
        vtable = &VA_DRIVER_CONTEXT_EGL(ctx)->vtable;           \
        if (!vtable->va##func##EGL)                             \
            return VA_STATUS_ERROR_UNIMPLEMENTED;               \
        status = vtable->va##func##EGL args;                    \
    } while (0)


VAStatus vaGetEGLClientBufferFromSurface (
    VADisplay dpy,
    VASurfaceID surface,
    EGLClientBuffer *buffer /* out*/
)
{
  VADriverContextP ctx;
  struct VADriverVTableEGL *va_egl;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  va_egl = (struct VADriverVTableEGL *)ctx->vtable_egl;
  if (va_egl && va_egl->vaGetEGLClientBufferFromSurface) {
      return va_egl->vaGetEGLClientBufferFromSurface(ctx, surface, buffer);
  } else
      return VA_STATUS_ERROR_UNIMPLEMENTED;
}
  
// Destroy VA/EGL display context
static void va_DisplayContextDestroy(VADisplayContextP pDisplayContext)
{
    VADisplayContextEGLP pDisplayContextEGL;
    VADriverContextP     pDriverContext;
    VADriverContextEGLP  pDriverContextEGL;

    if (!pDisplayContext)
        return;

    pDriverContext     = pDisplayContext->pDriverContext;
    pDriverContextEGL  = pDriverContext->egl;
    if (pDriverContextEGL) {
        free(pDriverContextEGL);
        pDriverContext->egl = NULL;
    }

    pDisplayContextEGL = pDisplayContext->opaque;
    if (pDisplayContextEGL) {
        vaDestroyFunc vaDestroy = pDisplayContextEGL->vaDestroy;
        free(pDisplayContextEGL);
        pDisplayContext->opaque = NULL;
        if (vaDestroy)
            vaDestroy(pDisplayContext);
    }
}

// Return a suitable VADisplay for VA API
VADisplay vaGetDisplayEGL(VANativeDisplay native_dpy,
                          EGLDisplay egl_dpy)
{
    VADisplay            dpy                = NULL;
    VADisplayContextP    pDisplayContext    = NULL;
    VADisplayContextEGLP pDisplayContextEGL = NULL;
    VADriverContextP     pDriverContext;
    VADriverContextEGLP  pDriverContextEGL  = NULL;

    dpy = vaGetDisplay(native_dpy);

    if (!dpy)
        return NULL;

    if (egl_dpy == EGL_NO_DISPLAY)
        goto error;

    pDisplayContext = (VADisplayContextP)dpy;
    pDriverContext  = pDisplayContext->pDriverContext;

    pDisplayContextEGL = calloc(1, sizeof(*pDisplayContextEGL));
    if (!pDisplayContextEGL)
        goto error;

    pDriverContextEGL = calloc(1, sizeof(*pDriverContextEGL));
    if (!pDriverContextEGL)
        goto error;

    pDisplayContextEGL->vaDestroy = pDisplayContext->vaDestroy;
    pDisplayContext->vaDestroy = va_DisplayContextDestroy;
    pDisplayContext->opaque = pDisplayContextEGL;
    pDriverContextEGL->egl_display = egl_dpy;
    pDriverContext->egl = pDriverContextEGL;
    return dpy;

error:
    free(pDriverContextEGL);
    free(pDisplayContextEGL);
    pDisplayContext->vaDestroy(pDisplayContext);
    return NULL;
}

int vaMaxNumSurfaceTargetsEGL(
    VADisplay dpy
)
{
    VADriverContextP ctx;
    struct VADriverVTableEGL *va_egl;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    va_egl = (struct VADriverVTableEGL *)ctx->vtable_egl;

    if (va_egl)
        return va_egl->max_egl_surface_targets;
    else
        return 2;
}

int vaMaxNumSurfaceAttributesEGL(
    VADisplay dpy
)
{
    VADriverContextP ctx;
    struct VADriverVTableEGL *va_egl;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    va_egl = (struct VADriverVTableEGL *)ctx->vtable_egl;

    if (va_egl)
        return va_egl->max_egl_surface_attributes;
    else
        return 2;
}

VAStatus vaQuerySurfaceTargetsEGL(
    VADisplay dpy,
    EGLenum *target_list,       /* out */
    int *num_targets		/* out */
)
{
    VADriverContextP ctx;
    VAStatus status;

    INIT_CONTEXT(ctx, dpy);

    INVOKE(ctx, QuerySurfaceTargets, (dpy, target_list, num_targets));
    return status;
}

VAStatus vaCreateSurfaceEGL(
    VADisplay dpy,
    EGLenum target,
    unsigned int width,
    unsigned int height,
    VASurfaceEGL *gl_surface
)
{
    VADriverContextP ctx;
    VAStatus status;

    INIT_CONTEXT(ctx, dpy);

    INVOKE(ctx, CreateSurface, (dpy, target, width, height, gl_surface));
    return status;
}

// Destroy a VA/EGL surface
VAStatus vaDestroySurfaceEGL(
    VADisplay dpy,
    VASurfaceEGL egl_surface
)
{
    VADriverContextP ctx;
    VAStatus status;

    INIT_CONTEXT(ctx, dpy);

    INVOKE(ctx, DestroySurface, (dpy, egl_surface));
    return status;
}

VAStatus vaAssociateSurfaceEGL(
    VADisplay dpy,
    VASurfaceEGL egl_surface,
    VASurfaceID surface,
    unsigned int flags
)
{
    VADriverContextP ctx;
    VAStatus status;

    INIT_CONTEXT(ctx, dpy);

    INVOKE(ctx, AssociateSurface, (dpy, egl_surface, surface, flags));
    return status;
}

VAStatus vaSyncSurfaceEGL(
    VADisplay dpy,
    VASurfaceEGL egl_surface
)
{
    VADriverContextP ctx;
    VAStatus status;

    INIT_CONTEXT(ctx, dpy);

    INVOKE(ctx, SyncSurface, (dpy, egl_surface));
    return status;
}

VAStatus vaGetSurfaceInfoEGL(
    VADisplay dpy,
    VASurfaceEGL egl_surface,
    EGLenum *target,            /* out, the type of <buffer> */
    EGLClientBuffer *buffer,    /* out */
    EGLint *attrib_list,        /* out, the last attribute must be EGL_NONE */
    int *num_attribs            /* in/out */
)
{
    VADriverContextP ctx;
    VAStatus status;

    INIT_CONTEXT(ctx, dpy);

    INVOKE(ctx, GetSurfaceInfo, (dpy, egl_surface, target, buffer, attrib_list, num_attribs));
    return status;
}

VAStatus vaDeassociateSurfaceEGL(
    VADisplay dpy,
    VASurfaceEGL egl_surface
)
{
    VADriverContextP ctx;
    VAStatus status;

    INIT_CONTEXT(ctx, dpy);

    INVOKE(ctx, DeassociateSurface, (dpy, egl_surface));
    return status;
}
  
