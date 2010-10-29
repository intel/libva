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

#ifndef VA_GLX_PRIVATE_H
#define VA_GLX_PRIVATE_H

#include "sysdeps.h"
#include "va.h"
#include "va_backend.h"
#include "va_x11.h"
#include "va_glx.h"
#include "va_backend_glx.h"
#include <GL/glxext.h>

#if GLX_GLXEXT_VERSION < 18
typedef void (*PFNGLXBINDTEXIMAGEEXTPROC)(Display *, GLXDrawable, int, const int *);
typedef void (*PFNGLXRELEASETEXIMAGEEXTPROC)(Display *, GLXDrawable, int);
#endif

#if GLX_GLXEXT_VERSION < 27
/* XXX: this is not exactly that version but this is the only means to
   make sure we have the correct <GL/glx.h> with those signatures */
typedef GLXPixmap (*PFNGLXCREATEPIXMAPPROC)(Display *, GLXFBConfig, Pixmap, const int *);
typedef void (*PFNGLXDESTROYPIXMAPPROC)(Display *, GLXPixmap);
#endif

typedef struct VAOpenGLVTable *VAOpenGLVTableP;

struct VAOpenGLVTable {
    PFNGLXCREATEPIXMAPPROC              glx_create_pixmap;
    PFNGLXDESTROYPIXMAPPROC             glx_destroy_pixmap;
    PFNGLXBINDTEXIMAGEEXTPROC           glx_bind_tex_image;
    PFNGLXRELEASETEXIMAGEEXTPROC        glx_release_tex_image;
    PFNGLGENFRAMEBUFFERSEXTPROC         gl_gen_framebuffers;
    PFNGLDELETEFRAMEBUFFERSEXTPROC      gl_delete_framebuffers;
    PFNGLBINDFRAMEBUFFEREXTPROC         gl_bind_framebuffer;
    PFNGLGENRENDERBUFFERSEXTPROC        gl_gen_renderbuffers;
    PFNGLDELETERENDERBUFFERSEXTPROC     gl_delete_renderbuffers;
    PFNGLBINDRENDERBUFFEREXTPROC        gl_bind_renderbuffer;
    PFNGLRENDERBUFFERSTORAGEEXTPROC     gl_renderbuffer_storage;
    PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC gl_framebuffer_renderbuffer;
    PFNGLFRAMEBUFFERTEXTURE2DEXTPROC    gl_framebuffer_texture_2d;
    PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC  gl_check_framebuffer_status;
};

typedef struct VADisplayContextGLX *VADisplayContextGLXP;
typedef struct VADriverContextGLX  *VADriverContextGLXP;
typedef struct VASurfaceGLX        *VASurfaceGLXP;
typedef struct VADriverVTableGLX   *VADriverVTableGLXP;

typedef void (*vaDestroyFunc)(VADisplayContextP);

struct VADisplayContextGLX {
    vaDestroyFunc vaDestroy;
};

#define VA_DRIVER_CONTEXT_GLX(ctx) ((VADriverContextGLXP)((ctx)->glx))

struct VADriverContextGLX {
    struct VADriverVTableGLX    vtable;
    struct VAOpenGLVTable       gl_vtable;
    unsigned int                is_initialized  : 1;
};

#endif /* VA_GLX_PRIVATE_H */
