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

#include "va.h"
#include "va_backend_egl.h"
#include "va_egl.h"
#include "va_internal.h"


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
  
  
