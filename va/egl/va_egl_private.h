/*
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
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _VA_EGL_PRIVATE_H_
#define _VA_EGL_PRIVATE_H_

#include "va.h"
#include "va_backend.h"
#include "va_egl.h"
#include "va_backend_egl.h"

typedef struct VADisplayContextEGL *VADisplayContextEGLP;
typedef struct VADriverContextEGL  *VADriverContextEGLP;
typedef struct VASurfaceImplEGL    *VASurfaceImplEGLP;
typedef struct VADriverVTableEGL   *VADriverVTableEGLP;
typedef struct VADriverVTablePrivEGL *VADriverVTablePrivEGLP;
typedef void (*vaDestroyFunc)(VADisplayContextP);

struct VADisplayContextEGL {
    vaDestroyFunc vaDestroy;
};

#define VA_DRIVER_CONTEXT_EGL(ctx) ((VADriverContextEGLP)((ctx)->egl))

struct VADriverVTablePrivEGL {
    VAStatus (*vaQuerySurfaceTargetsEGL)(
        VADisplay dpy,
        EGLenum *target_list,           /* out */
        int *num_targets		/* out */
    );

    VAStatus (*vaCreateSurfaceEGL)(
        VADisplay dpy,
        EGLenum target,
        unsigned int width,
        unsigned int height,
        VASurfaceEGL *gl_surface
    );

    VAStatus (*vaDestroySurfaceEGL)(
        VADisplay dpy,
        VASurfaceEGL egl_surface
    );

    VAStatus (*vaAssociateSurfaceEGL)(
        VADisplay dpy,
        VASurfaceEGL egl_surface,
        VASurfaceID surface,
        unsigned int flags
    );

    VAStatus (*vaSyncSurfaceEGL)(
        VADisplay dpy,
        VASurfaceEGL egl_surface
    );

    VAStatus (*vaGetSurfaceInfoEGL)(
        VADisplay dpy,
        VASurfaceEGL egl_surface,
        EGLenum *target,
        EGLClientBuffer *buffer,
        EGLint *attrib_list,
        int *num_attribs
    );

    VAStatus (*vaDeassociateSurfaceEGL)(
        VADisplay dpy,
        VASurfaceEGL egl_surface
    );
};

struct VADriverContextEGL {
    struct VADriverVTablePrivEGL vtable;
    unsigned int is_initialized : 1;
    EGLDisplay  egl_display;
};

#endif /* _VA_EGL_PRIVATE_H_ */
