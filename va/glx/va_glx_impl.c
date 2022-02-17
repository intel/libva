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

#define _GNU_SOURCE 1
#include "sysdeps.h"
#include "va_glx_private.h"
#include "va_glx_impl.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>

static void va_glx_error_message(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, "libva-glx error: ");
    vfprintf(stderr, format, args);
    va_end(args);
}

// X error trap
static int x11_error_code = 0;
static int (*old_error_handler)(Display *, XErrorEvent *);

static int error_handler(Display *dpy, XErrorEvent *error)
{
    x11_error_code = error->error_code;
    return 0;
}

static void x11_trap_errors(void)
{
    x11_error_code    = 0;
    old_error_handler = XSetErrorHandler(error_handler);
}

static int x11_untrap_errors(void)
{
    XSetErrorHandler(old_error_handler);
    return x11_error_code;
}

// Returns a string representation of an OpenGL error
static const char *gl_get_error_string(GLenum error)
{
    static const struct {
        GLenum val;
        const char *str;
    }
    gl_errors[] = {
        { GL_NO_ERROR,          "no error" },
        { GL_INVALID_ENUM,      "invalid enumerant" },
        { GL_INVALID_VALUE,     "invalid value" },
        { GL_INVALID_OPERATION, "invalid operation" },
        { GL_STACK_OVERFLOW,    "stack overflow" },
        { GL_STACK_UNDERFLOW,   "stack underflow" },
        { GL_OUT_OF_MEMORY,     "out of memory" },
#ifdef GL_INVALID_FRAMEBUFFER_OPERATION_EXT
        { GL_INVALID_FRAMEBUFFER_OPERATION_EXT, "invalid framebuffer operation" },
#endif
        { ~0, NULL }
    };

    int i;
    for (i = 0; gl_errors[i].str; i++) {
        if (gl_errors[i].val == error)
            return gl_errors[i].str;
    }
    return "unknown";
}

static inline int gl_do_check_error(int report)
{
    GLenum error;
    int is_error = 0;
    while ((error = glGetError()) != GL_NO_ERROR) {
        if (report)
            va_glx_error_message("glError: %s caught\n",
                                 gl_get_error_string(error));
        is_error = 1;
    }
    return is_error;
}

static inline void gl_purge_errors(void)
{
    gl_do_check_error(0);
}

static inline int gl_check_error(void)
{
    return gl_do_check_error(1);
}

// glGetTexLevelParameteriv() wrapper
static int gl_get_texture_param(GLenum param, unsigned int *pval)
{
    GLint val;

    gl_purge_errors();
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, param, &val);
    if (gl_check_error())
        return 0;
    if (pval)
        *pval = val;
    return 1;
}

// Returns the OpenGL VTable
static inline VAOpenGLVTableP gl_get_vtable(VADriverContextP ctx)
{
    return &VA_DRIVER_CONTEXT_GLX(ctx)->gl_vtable;
}

// Lookup for a GLX function
typedef void (*GLFuncPtr)(void);
typedef GLFuncPtr(*GLXGetProcAddressProc)(const char *);

static GLFuncPtr get_proc_address_default(const char *name)
{
    return NULL;
}

static GLXGetProcAddressProc get_proc_address_func(void)
{
    GLXGetProcAddressProc get_proc_func;

    dlerror();
    get_proc_func = (GLXGetProcAddressProc)
                    dlsym(RTLD_DEFAULT, "glXGetProcAddress");
    if (!dlerror())
        return get_proc_func;

    get_proc_func = (GLXGetProcAddressProc)
                    dlsym(RTLD_DEFAULT, "glXGetProcAddressARB");
    if (!dlerror())
        return get_proc_func;

    return get_proc_address_default;
}

static inline GLFuncPtr get_proc_address(const char *name)
{
    static GLXGetProcAddressProc get_proc_func = NULL;
    if (!get_proc_func)
        get_proc_func = get_proc_address_func();
    return get_proc_func(name);
}

// Check for GLX extensions (TFP, FBO)
static int check_extension(const char *name, const char *ext)
{
    const char *end;
    int name_len, n;

    if (!name || !ext)
        return 0;

    end = ext + strlen(ext);
    name_len = strlen(name);
    while (ext < end) {
        n = strcspn(ext, " ");
        if (n == name_len && strncmp(name, ext, n) == 0)
            return 1;
        ext += (n + 1);
    }
    return 0;
}

static int check_extension3(const char *name)
{
    int nbExtensions, i;
    PFNGLGETSTRINGIPROC glGetStringi = 0;

    glGetStringi = (PFNGLGETSTRINGIPROC) get_proc_address("glGetStringi");
    if (!glGetStringi)
        return 0;


    glGetIntegerv(GL_NUM_EXTENSIONS, &nbExtensions);
    for (i = 0; i < nbExtensions; i++) {
        const GLubyte *strExtension = glGetStringi(GL_EXTENSIONS, i);
        if (strcmp((const char *) strExtension, name) == 0)
            return 1;
    }

    return 0;
}

static int check_tfp_extensions(VADriverContextP ctx)
{
    const char *gl_extensions;
    const char *glx_extensions;

    gl_extensions = (const char *)glGetString(GL_EXTENSIONS);
    if (!check_extension("GL_ARB_texture_non_power_of_two", gl_extensions) && !check_extension3("GL_ARB_texture_non_power_of_two"))
        return 0;

    glx_extensions = glXQueryExtensionsString(ctx->native_dpy, ctx->x11_screen);
    if (!check_extension("GLX_EXT_texture_from_pixmap", glx_extensions))
        return 0;

    return 1;
}

static int check_fbo_extensions(VADriverContextP ctx)
{
    const char *gl_extensions;

    gl_extensions = (const char *)glGetString(GL_EXTENSIONS);
    if (check_extension("GL_ARB_framebuffer_object", gl_extensions) || check_extension3("GL_ARB_framebuffer_object"))
        return 1;
    if (check_extension("GL_EXT_framebuffer_object", gl_extensions) || check_extension3("GL_EXT_framebuffer_object"))
        return 1;

    return 0;
}

// Load GLX extensions
static int load_tfp_extensions(VADriverContextP ctx)
{
    VAOpenGLVTableP pOpenGLVTable = gl_get_vtable(ctx);

    pOpenGLVTable->glx_create_pixmap = (PFNGLXCREATEPIXMAPPROC)
                                       get_proc_address("glXCreatePixmap");
    if (!pOpenGLVTable->glx_create_pixmap)
        return 0;
    pOpenGLVTable->glx_destroy_pixmap = (PFNGLXDESTROYPIXMAPPROC)
                                        get_proc_address("glXDestroyPixmap");
    if (!pOpenGLVTable->glx_destroy_pixmap)
        return 0;
    pOpenGLVTable->glx_bind_tex_image = (PFNGLXBINDTEXIMAGEEXTPROC)
                                        get_proc_address("glXBindTexImageEXT");
    if (!pOpenGLVTable->glx_bind_tex_image)
        return 0;
    pOpenGLVTable->glx_release_tex_image = (PFNGLXRELEASETEXIMAGEEXTPROC)
                                           get_proc_address("glXReleaseTexImageEXT");
    if (!pOpenGLVTable->glx_release_tex_image)
        return 0;
    return 1;
}

static int load_fbo_extensions(VADriverContextP ctx)
{
    VAOpenGLVTableP pOpenGLVTable = gl_get_vtable(ctx);

    pOpenGLVTable->gl_gen_framebuffers = (PFNGLGENFRAMEBUFFERSEXTPROC)
                                         get_proc_address("glGenFramebuffersEXT");
    if (!pOpenGLVTable->gl_gen_framebuffers)
        return 0;
    pOpenGLVTable->gl_delete_framebuffers = (PFNGLDELETEFRAMEBUFFERSEXTPROC)
                                            get_proc_address("glDeleteFramebuffersEXT");
    if (!pOpenGLVTable->gl_delete_framebuffers)
        return 0;
    pOpenGLVTable->gl_bind_framebuffer = (PFNGLBINDFRAMEBUFFEREXTPROC)
                                         get_proc_address("glBindFramebufferEXT");
    if (!pOpenGLVTable->gl_bind_framebuffer)
        return 0;
    pOpenGLVTable->gl_gen_renderbuffers = (PFNGLGENRENDERBUFFERSEXTPROC)
                                          get_proc_address("glGenRenderbuffersEXT");
    if (!pOpenGLVTable->gl_gen_renderbuffers)
        return 0;
    pOpenGLVTable->gl_delete_renderbuffers = (PFNGLDELETERENDERBUFFERSEXTPROC)
            get_proc_address("glDeleteRenderbuffersEXT");
    if (!pOpenGLVTable->gl_delete_renderbuffers)
        return 0;
    pOpenGLVTable->gl_bind_renderbuffer = (PFNGLBINDRENDERBUFFEREXTPROC)
                                          get_proc_address("glBindRenderbufferEXT");
    if (!pOpenGLVTable->gl_bind_renderbuffer)
        return 0;
    pOpenGLVTable->gl_renderbuffer_storage = (PFNGLRENDERBUFFERSTORAGEEXTPROC)
            get_proc_address("glRenderbufferStorageEXT");
    if (!pOpenGLVTable->gl_renderbuffer_storage)
        return 0;
    pOpenGLVTable->gl_framebuffer_renderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)
            get_proc_address("glFramebufferRenderbufferEXT");
    if (!pOpenGLVTable->gl_framebuffer_renderbuffer)
        return 0;
    pOpenGLVTable->gl_framebuffer_texture_2d = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)
            get_proc_address("glFramebufferTexture2DEXT");
    if (!pOpenGLVTable->gl_framebuffer_texture_2d)
        return 0;
    pOpenGLVTable->gl_check_framebuffer_status = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)
            get_proc_address("glCheckFramebufferStatusEXT");
    if (!pOpenGLVTable->gl_check_framebuffer_status)
        return 0;
    return 1;
}


/* ========================================================================= */
/* === VA/GLX helpers                                                    === */
/* ========================================================================= */

// OpenGL context state
typedef struct OpenGLContextState *OpenGLContextStateP;

struct OpenGLContextState {
    Display     *display;
    Window       window;
    GLXContext   context;
};

static void
gl_destroy_context(OpenGLContextStateP cs)
{
    if (!cs)
        return;

    if (cs->display && cs->context) {
        if (glXGetCurrentContext() == cs->context)
            glXMakeCurrent(cs->display, None, NULL);
        glXDestroyContext(cs->display, cs->context);
        cs->display = NULL;
        cs->context = NULL;
    }
    free(cs);
}

static OpenGLContextStateP
gl_create_context(VADriverContextP ctx, OpenGLContextStateP parent)
{
    OpenGLContextStateP cs;
    GLXFBConfig *fbconfigs = NULL;
    int fbconfig_id, val, n, n_fbconfigs;
    Status status;

    static GLint fbconfig_attrs[] = {
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,   GLX_RGBA_BIT,
        GLX_DOUBLEBUFFER,  True,
        GLX_RED_SIZE,      8,
        GLX_GREEN_SIZE,    8,
        GLX_BLUE_SIZE,     8,
        None
    };

    cs = malloc(sizeof(*cs));
    if (!cs)
        goto error;

    if (parent) {
        cs->display = parent->display;
        cs->window  = parent->window;
    } else {
        cs->display = ctx->native_dpy;
        cs->window  = None;
    }
    cs->context = NULL;

    if (parent && parent->context) {
        status = glXQueryContext(
                     parent->display,
                     parent->context,
                     GLX_FBCONFIG_ID, &fbconfig_id
                 );
        if (status != Success)
            goto error;

        if (fbconfig_id == GLX_DONT_CARE)
            goto choose_fbconfig;

        fbconfigs = glXGetFBConfigs(
                        parent->display,
                        DefaultScreen(parent->display),
                        &n_fbconfigs
                    );
        if (!fbconfigs)
            goto error;

        /* Find out a GLXFBConfig compatible with the parent context */
        for (n = 0; n < n_fbconfigs; n++) {
            status = glXGetFBConfigAttrib(
                         cs->display,
                         fbconfigs[n],
                         GLX_FBCONFIG_ID, &val
                     );
            if (status == Success && val == fbconfig_id)
                break;
        }
        if (n == n_fbconfigs)
            goto error;
    } else {
choose_fbconfig:
        fbconfigs = glXChooseFBConfig(
                        ctx->native_dpy,
                        ctx->x11_screen,
                        fbconfig_attrs, &n_fbconfigs
                    );
        if (!fbconfigs)
            goto error;

        /* Select the first one */
        n = 0;
    }

    cs->context = glXCreateNewContext(
                      cs->display,
                      fbconfigs[n],
                      GLX_RGBA_TYPE,
                      parent ? parent->context : NULL,
                      True
                  );
    if (cs->context)
        goto end;

error:
    gl_destroy_context(cs);
    cs = NULL;
end:
    if (fbconfigs)
        XFree(fbconfigs);
    return cs;
}

static void gl_get_current_context(OpenGLContextStateP cs)
{
    cs->display = glXGetCurrentDisplay();
    cs->window  = glXGetCurrentDrawable();
    cs->context = glXGetCurrentContext();
}

static int
gl_set_current_context(OpenGLContextStateP new_cs, OpenGLContextStateP old_cs)
{
    /* If display is NULL, this could be that new_cs was retrieved from
       gl_get_current_context() with none set previously. If that case,
       the other fields are also NULL and we don't return an error */
    if (!new_cs->display)
        return !new_cs->window && !new_cs->context;

    if (old_cs) {
        if (old_cs == new_cs)
            return 1;
        gl_get_current_context(old_cs);
        if (old_cs->display == new_cs->display &&
            old_cs->window  == new_cs->window  &&
            old_cs->context == new_cs->context)
            return 1;
    }
    return glXMakeCurrent(new_cs->display, new_cs->window, new_cs->context);
}

/** Unique VASurfaceGLX identifier */
#define VA_SURFACE_GLX_MAGIC VA_FOURCC('V','A','G','L')

struct VASurfaceGLX {
    uint32_t            magic;      ///< Magic number identifying a VASurfaceGLX
    GLenum              target;     ///< GL target to which the texture is bound
    GLuint              texture;    ///< GL texture
    VASurfaceID         surface;    ///< Associated VA surface
    unsigned int        width;
    unsigned int        height;
    OpenGLContextStateP gl_context;
    int                 is_bound;
    Pixmap              pixmap;
    GLuint              pix_texture;
    GLXPixmap           glx_pixmap;
    GLuint              fbo;
};

// Create Pixmaps for GLX texture-from-pixmap extension
static int create_tfp_surface(VADriverContextP ctx, VASurfaceGLXP pSurfaceGLX)
{
    VAOpenGLVTableP const pOpenGLVTable = gl_get_vtable(ctx);
    const unsigned int    width         = pSurfaceGLX->width;
    const unsigned int    height        = pSurfaceGLX->height;
    Pixmap                pixmap        = None;
    GLXFBConfig          *fbconfig      = NULL;
    GLXPixmap             glx_pixmap    = None;
    Window                root_window;
    XWindowAttributes     wattr;
    int                  *attrib;
    int                   n_fbconfig_attrs;

    root_window = RootWindow(ctx->native_dpy, ctx->x11_screen);
    XGetWindowAttributes(ctx->native_dpy, root_window, &wattr);
    if (wattr.depth != 24 && wattr.depth != 32)
        return 0;
    pixmap = XCreatePixmap(
                 ctx->native_dpy,
                 root_window,
                 width,
                 height,
                 wattr.depth
             );
    if (!pixmap)
        return 0;
    pSurfaceGLX->pixmap = pixmap;

    int fbconfig_attrs[32] = {
        GLX_DRAWABLE_TYPE,      GLX_PIXMAP_BIT,
        GLX_DOUBLEBUFFER,       GL_TRUE,
        GLX_RENDER_TYPE,        GLX_RGBA_BIT,
        GLX_X_RENDERABLE,       GL_TRUE,
        GLX_Y_INVERTED_EXT,     GL_TRUE,
        GLX_RED_SIZE,           8,
        GLX_GREEN_SIZE,         8,
        GLX_BLUE_SIZE,          8,
        GLX_ALPHA_SIZE,         8,
        /*
         * depth test isn't enabled in the implementaion of VA GLX,
         * so depth buffer is unnecessary. However to workaround a
         * bug in older verson of xorg-server, always require a depth
         * buffer.
         *
         * See https://bugs.freedesktop.org/show_bug.cgi?id=76755
         */
        GLX_DEPTH_SIZE,         1,
        GL_NONE,
    };
    for (attrib = fbconfig_attrs; *attrib != GL_NONE; attrib += 2)
        ;
    if (wattr.depth == 32) {
        *attrib++ = GLX_BIND_TO_TEXTURE_RGBA_EXT;
        *attrib++ = GL_TRUE;
    } else {
        *attrib++ = GLX_BIND_TO_TEXTURE_RGB_EXT;
        *attrib++ = GL_TRUE;
    }
    *attrib++ = GL_NONE;

    fbconfig = glXChooseFBConfig(
                   ctx->native_dpy,
                   ctx->x11_screen,
                   fbconfig_attrs,
                   &n_fbconfig_attrs
               );
    if (!fbconfig)
        return 0;

    int pixmap_attrs[10] = {
        GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
        GLX_MIPMAP_TEXTURE_EXT, GL_FALSE,
        GL_NONE,
    };
    for (attrib = pixmap_attrs; *attrib != GL_NONE; attrib += 2)
        ;
    *attrib++ = GLX_TEXTURE_FORMAT_EXT;
    if (wattr.depth == 32)
        *attrib++ = GLX_TEXTURE_FORMAT_RGBA_EXT;
    else
        *attrib++ = GLX_TEXTURE_FORMAT_RGB_EXT;
    *attrib++ = GL_NONE;

    x11_trap_errors();
    glx_pixmap = pOpenGLVTable->glx_create_pixmap(
                     ctx->native_dpy,
                     fbconfig[0],
                     pixmap,
                     pixmap_attrs
                 );
    free(fbconfig);
    if (x11_untrap_errors() != 0)
        return 0;
    pSurfaceGLX->glx_pixmap = glx_pixmap;

    glGenTextures(1, &pSurfaceGLX->pix_texture);
    glBindTexture(GL_TEXTURE_2D, pSurfaceGLX->pix_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return 1;
}

// Destroy Pixmaps used for TFP
static void destroy_tfp_surface(VADriverContextP ctx, VASurfaceGLXP pSurfaceGLX)
{
    VAOpenGLVTableP const pOpenGLVTable = gl_get_vtable(ctx);

    if (pSurfaceGLX->pix_texture) {
        glDeleteTextures(1, &pSurfaceGLX->pix_texture);
        pSurfaceGLX->pix_texture = 0;
    }

    if (pSurfaceGLX->glx_pixmap) {
        pOpenGLVTable->glx_destroy_pixmap(ctx->native_dpy, pSurfaceGLX->glx_pixmap);
        pSurfaceGLX->glx_pixmap = None;
    }

    if (pSurfaceGLX->pixmap) {
        XFreePixmap(ctx->native_dpy, pSurfaceGLX->pixmap);
        pSurfaceGLX->pixmap = None;
    }
}

// Bind GLX Pixmap to texture
static int bind_pixmap(VADriverContextP ctx, VASurfaceGLXP pSurfaceGLX)
{
    VAOpenGLVTableP pOpenGLVTable = gl_get_vtable(ctx);

    if (pSurfaceGLX->is_bound)
        return 1;

    glBindTexture(GL_TEXTURE_2D, pSurfaceGLX->pix_texture);

    x11_trap_errors();
    pOpenGLVTable->glx_bind_tex_image(
        ctx->native_dpy,
        pSurfaceGLX->glx_pixmap,
        GLX_FRONT_LEFT_EXT,
        NULL
    );
    XSync(ctx->native_dpy, False);
    if (x11_untrap_errors() != 0) {
        va_glx_error_message("failed to bind pixmap\n");
        return 0;
    }

    pSurfaceGLX->is_bound = 1;
    return 1;
}

// Release GLX Pixmap from texture
static int unbind_pixmap(VADriverContextP ctx, VASurfaceGLXP pSurfaceGLX)
{
    VAOpenGLVTableP pOpenGLVTable = gl_get_vtable(ctx);

    if (!pSurfaceGLX->is_bound)
        return 1;

    x11_trap_errors();
    pOpenGLVTable->glx_release_tex_image(
        ctx->native_dpy,
        pSurfaceGLX->glx_pixmap,
        GLX_FRONT_LEFT_EXT
    );
    XSync(ctx->native_dpy, False);
    if (x11_untrap_errors() != 0) {
        va_glx_error_message("failed to release pixmap\n");
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    pSurfaceGLX->is_bound = 0;
    return 1;
}

// Render GLX Pixmap to texture
static void render_pixmap(VADriverContextP ctx, VASurfaceGLXP pSurfaceGLX)
{
    const unsigned int w = pSurfaceGLX->width;
    const unsigned int h = pSurfaceGLX->height;

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    {
        glTexCoord2f(0.0f, 0.0f);
        glVertex2i(0, 0);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2i(0, h);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2i(w, h);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2i(w, 0);
    }
    glEnd();
}

// Create offscreen surface
static int create_fbo_surface(VADriverContextP ctx, VASurfaceGLXP pSurfaceGLX)
{
    VAOpenGLVTableP pOpenGLVTable = gl_get_vtable(ctx);
    GLuint fbo;
    GLenum status;

    pOpenGLVTable->gl_gen_framebuffers(1, &fbo);
    pOpenGLVTable->gl_bind_framebuffer(GL_FRAMEBUFFER_EXT, fbo);
    pOpenGLVTable->gl_framebuffer_texture_2d(
        GL_FRAMEBUFFER_EXT,
        GL_COLOR_ATTACHMENT0_EXT,
        GL_TEXTURE_2D,
        pSurfaceGLX->texture,
        0
    );

    status = pOpenGLVTable->gl_check_framebuffer_status(GL_DRAW_FRAMEBUFFER_EXT);
    pOpenGLVTable->gl_bind_framebuffer(GL_FRAMEBUFFER_EXT, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
        return 0;

    pSurfaceGLX->fbo = fbo;
    return 1;
}

// Destroy offscreen surface
static void destroy_fbo_surface(VADriverContextP ctx, VASurfaceGLXP pSurfaceGLX)
{
    VAOpenGLVTableP pOpenGLVTable = gl_get_vtable(ctx);

    if (pSurfaceGLX->fbo) {
        pOpenGLVTable->gl_delete_framebuffers(1, &pSurfaceGLX->fbo);
        pSurfaceGLX->fbo = 0;
    }
}

// Setup matrices to match the FBO texture dimensions
static void fbo_enter(VADriverContextP ctx, VASurfaceGLXP pSurfaceGLX)
{
    VAOpenGLVTableP pOpenGLVTable = gl_get_vtable(ctx);
    const unsigned int width  = pSurfaceGLX->width;
    const unsigned int height = pSurfaceGLX->height;

    pOpenGLVTable->gl_bind_framebuffer(GL_FRAMEBUFFER_EXT, pSurfaceGLX->fbo);
    glPushAttrib(GL_VIEWPORT_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glViewport(0, 0, width, height);
    glTranslatef(-1.0f, -1.0f, 0.0f);
    glScalef(2.0f / width, 2.0f / height, 1.0f);
}

// Restore original OpenGL matrices
static void fbo_leave(VADriverContextP ctx)
{
    VAOpenGLVTableP pOpenGLVTable = gl_get_vtable(ctx);

    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    pOpenGLVTable->gl_bind_framebuffer(GL_FRAMEBUFFER_EXT, 0);
}

// Check internal texture format is supported
static int is_supported_internal_format(GLenum format)
{
    /* XXX: we don't support other textures than RGBA */
    switch (format) {
    case 4:
    case GL_RGBA:
    case GL_RGBA8:
        return 1;
    }
    return 0;
}

// Destroy VA/GLX surface
static void
destroy_surface(VADriverContextP ctx, VASurfaceGLXP pSurfaceGLX)
{
    unbind_pixmap(ctx, pSurfaceGLX);
    destroy_fbo_surface(ctx, pSurfaceGLX);
    destroy_tfp_surface(ctx, pSurfaceGLX);
    free(pSurfaceGLX);
}

// Create VA/GLX surface
static VASurfaceGLXP
create_surface(VADriverContextP ctx, GLenum target, GLuint texture)
{
    VASurfaceGLXP pSurfaceGLX = NULL;
    unsigned int internal_format, border_width, width, height;
    int is_error = 1;

    pSurfaceGLX = malloc(sizeof(*pSurfaceGLX));
    if (!pSurfaceGLX)
        goto end;

    pSurfaceGLX->magic          = VA_SURFACE_GLX_MAGIC;
    pSurfaceGLX->target         = target;
    pSurfaceGLX->texture        = texture;
    pSurfaceGLX->surface        = VA_INVALID_SURFACE;
    pSurfaceGLX->gl_context     = NULL;
    pSurfaceGLX->is_bound       = 0;
    pSurfaceGLX->pixmap         = None;
    pSurfaceGLX->pix_texture    = 0;
    pSurfaceGLX->glx_pixmap     = None;
    pSurfaceGLX->fbo            = 0;

    glEnable(target);
    glBindTexture(target, texture);
    if (!gl_get_texture_param(GL_TEXTURE_INTERNAL_FORMAT, &internal_format))
        goto end;
    if (!is_supported_internal_format(internal_format))
        goto end;

    /* Check texture dimensions */
    if (!gl_get_texture_param(GL_TEXTURE_BORDER, &border_width))
        goto end;
    if (!gl_get_texture_param(GL_TEXTURE_WIDTH, &width))
        goto end;
    if (!gl_get_texture_param(GL_TEXTURE_HEIGHT, &height))
        goto end;

    width  -= 2 * border_width;
    height -= 2 * border_width;
    if (width == 0 || height == 0)
        goto end;

    pSurfaceGLX->width  = width;
    pSurfaceGLX->height = height;

    /* Create TFP objects */
    if (!create_tfp_surface(ctx, pSurfaceGLX))
        goto end;

    /* Create FBO objects */
    if (!create_fbo_surface(ctx, pSurfaceGLX))
        goto end;

    is_error = 0;
end:
    if (is_error && pSurfaceGLX) {
        destroy_surface(ctx, pSurfaceGLX);
        pSurfaceGLX = NULL;
    }
    return pSurfaceGLX;
}


/* ========================================================================= */
/* === VA/GLX implementation from the driver (fordward calls)            === */
/* ========================================================================= */

#define INVOKE(ctx, func, args) do {                    \
        VADriverVTableGLXP vtable = (ctx)->vtable_glx;  \
        if (!vtable->va##func##GLX)                     \
            return VA_STATUS_ERROR_UNIMPLEMENTED;       \
                                                        \
        VAStatus status = vtable->va##func##GLX args;   \
        if (status != VA_STATUS_SUCCESS)                \
            return status;                              \
    } while (0)

static VAStatus
vaCreateSurfaceGLX_impl_driver(
    VADriverContextP    ctx,
    GLenum              target,
    GLuint              texture,
    void              **gl_surface
)
{
    INVOKE(ctx, CreateSurface, (ctx, target, texture, gl_surface));
    return VA_STATUS_SUCCESS;
}

static VAStatus
vaDestroySurfaceGLX_impl_driver(VADriverContextP ctx, void *gl_surface)
{
    INVOKE(ctx, DestroySurface, (ctx, gl_surface));
    return VA_STATUS_SUCCESS;
}

static VAStatus
vaCopySurfaceGLX_impl_driver(
    VADriverContextP    ctx,
    void               *gl_surface,
    VASurfaceID         surface,
    unsigned int        flags
)
{
    INVOKE(ctx, CopySurface, (ctx, gl_surface, surface, flags));
    return VA_STATUS_SUCCESS;
}

#undef INVOKE


/* ========================================================================= */
/* === VA/GLX implementation from libVA (generic and suboptimal path)    === */
/* ========================================================================= */

#define INIT_SURFACE(surface, surface_arg) do {         \
        surface = (VASurfaceGLXP)(surface_arg);         \
        if (!check_surface(surface))                    \
            return VA_STATUS_ERROR_INVALID_SURFACE;     \
    } while (0)

// Check VASurfaceGLX is valid
static inline int check_surface(VASurfaceGLXP pSurfaceGLX)
{
    return pSurfaceGLX && pSurfaceGLX->magic == VA_SURFACE_GLX_MAGIC;
}

static VAStatus
vaCreateSurfaceGLX_impl_libva(
    VADriverContextP    ctx,
    GLenum              target,
    GLuint              texture,
    void              **gl_surface
)
{
    VASurfaceGLXP pSurfaceGLX;
    struct OpenGLContextState old_cs, *new_cs;

    gl_get_current_context(&old_cs);
    new_cs = gl_create_context(ctx, &old_cs);
    if (!new_cs)
        goto error;
    if (!gl_set_current_context(new_cs, NULL))
        goto error;

    pSurfaceGLX = create_surface(ctx, target, texture);
    if (!pSurfaceGLX)
        goto error;

    pSurfaceGLX->gl_context = new_cs;
    *gl_surface = pSurfaceGLX;

    gl_set_current_context(&old_cs, NULL);
    return VA_STATUS_SUCCESS;

error:
    if (new_cs)
        gl_destroy_context(new_cs);

    return VA_STATUS_ERROR_ALLOCATION_FAILED;
}

static VAStatus
vaDestroySurfaceGLX_impl_libva(VADriverContextP ctx, void *gl_surface)
{
    VASurfaceGLXP pSurfaceGLX;
    struct OpenGLContextState old_cs = {0}, *new_cs;

    INIT_SURFACE(pSurfaceGLX, gl_surface);

    new_cs = pSurfaceGLX->gl_context;
    if (!gl_set_current_context(new_cs, &old_cs))
        return VA_STATUS_ERROR_OPERATION_FAILED;

    destroy_surface(ctx, pSurfaceGLX);

    gl_destroy_context(new_cs);
    gl_set_current_context(&old_cs, NULL);
    return VA_STATUS_SUCCESS;
}

static inline VAStatus
deassociate_surface(VADriverContextP ctx, VASurfaceGLXP pSurfaceGLX)
{
    if (!unbind_pixmap(ctx, pSurfaceGLX))
        return VA_STATUS_ERROR_OPERATION_FAILED;

    pSurfaceGLX->surface = VA_INVALID_SURFACE;
    return VA_STATUS_SUCCESS;
}

static VAStatus
associate_surface(
    VADriverContextP    ctx,
    VASurfaceGLXP       pSurfaceGLX,
    VASurfaceID         surface,
    unsigned int        flags
)
{
    VAStatus status;

    /* XXX: optimise case where we are associating the same VA surface
       as before an no changed occurred to it */
    status = deassociate_surface(ctx, pSurfaceGLX);
    if (status != VA_STATUS_SUCCESS)
        return status;

    x11_trap_errors();
    status = ctx->vtable->vaPutSurface(
                 ctx,
                 surface,
                 (void *)pSurfaceGLX->pixmap,
                 0, 0, pSurfaceGLX->width, pSurfaceGLX->height,
                 0, 0, pSurfaceGLX->width, pSurfaceGLX->height,
                 NULL, 0,
                 flags
             );
    XSync(ctx->native_dpy, False);
    if (x11_untrap_errors() != 0)
        return VA_STATUS_ERROR_OPERATION_FAILED;
    if (status != VA_STATUS_SUCCESS)
        return status;

    pSurfaceGLX->surface = surface;
    return VA_STATUS_SUCCESS;
}

static inline VAStatus
sync_surface(VADriverContextP ctx, VASurfaceGLXP pSurfaceGLX)
{
    if (pSurfaceGLX->surface == VA_INVALID_SURFACE)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    return ctx->vtable->vaSyncSurface(ctx, pSurfaceGLX->surface);
}

static inline VAStatus
begin_render_surface(VADriverContextP ctx, VASurfaceGLXP pSurfaceGLX)
{
    VAStatus status;

    status = sync_surface(ctx, pSurfaceGLX);
    if (status != VA_STATUS_SUCCESS)
        return status;

    if (!bind_pixmap(ctx, pSurfaceGLX))
        return VA_STATUS_ERROR_OPERATION_FAILED;

    return VA_STATUS_SUCCESS;
}

static inline VAStatus
end_render_surface(VADriverContextP ctx, VASurfaceGLXP pSurfaceGLX)
{
    if (!unbind_pixmap(ctx, pSurfaceGLX))
        return VA_STATUS_ERROR_OPERATION_FAILED;

    return VA_STATUS_SUCCESS;
}

static VAStatus
copy_surface(
    VADriverContextP    ctx,
    VASurfaceGLXP       pSurfaceGLX,
    VASurfaceID         surface,
    unsigned int        flags
)
{
    VAStatus status;

    /* Associate VA surface */
    status = associate_surface(ctx, pSurfaceGLX, surface, flags);
    if (status != VA_STATUS_SUCCESS)
        return status;

    /* Render to FBO */
    fbo_enter(ctx, pSurfaceGLX);
    status = begin_render_surface(ctx, pSurfaceGLX);
    if (status == VA_STATUS_SUCCESS) {
        render_pixmap(ctx, pSurfaceGLX);
        status = end_render_surface(ctx, pSurfaceGLX);
    }
    fbo_leave(ctx);
    if (status != VA_STATUS_SUCCESS)
        return status;

    return deassociate_surface(ctx, pSurfaceGLX);
}

static VAStatus
vaCopySurfaceGLX_impl_libva(
    VADriverContextP    ctx,
    void               *gl_surface,
    VASurfaceID         surface,
    unsigned int        flags
)
{
    VASurfaceGLXP pSurfaceGLX;
    VAStatus status;
    struct OpenGLContextState old_cs = {0};

    INIT_SURFACE(pSurfaceGLX, gl_surface);

    if (!gl_set_current_context(pSurfaceGLX->gl_context, &old_cs))
        return VA_STATUS_ERROR_OPERATION_FAILED;

    status = copy_surface(ctx, pSurfaceGLX, surface, flags);

    gl_set_current_context(&old_cs, NULL);
    return status;
}

#undef INIT_SURFACE


/* ========================================================================= */
/* === Private VA/GLX vtable initialization                              === */
/* ========================================================================= */

// Initialize GLX driver context
VAStatus va_glx_init_context(VADriverContextP ctx)
{
    VADriverContextGLXP glx_ctx = VA_DRIVER_CONTEXT_GLX(ctx);
    VADriverVTableGLXP  vtable  = &glx_ctx->vtable;
    int glx_major, glx_minor;

    if (glx_ctx->is_initialized)
        return VA_STATUS_SUCCESS;

    if (ctx->vtable_glx && ctx->vtable_glx->vaCopySurfaceGLX) {
        vtable->vaCreateSurfaceGLX      = vaCreateSurfaceGLX_impl_driver;
        vtable->vaDestroySurfaceGLX     = vaDestroySurfaceGLX_impl_driver;
        vtable->vaCopySurfaceGLX        = vaCopySurfaceGLX_impl_driver;
    } else {
        vtable->vaCreateSurfaceGLX      = vaCreateSurfaceGLX_impl_libva;
        vtable->vaDestroySurfaceGLX     = vaDestroySurfaceGLX_impl_libva;
        vtable->vaCopySurfaceGLX        = vaCopySurfaceGLX_impl_libva;

        if (!glXQueryVersion(ctx->native_dpy, &glx_major, &glx_minor))
            return VA_STATUS_ERROR_UNIMPLEMENTED;

        if (!check_tfp_extensions(ctx) || !load_tfp_extensions(ctx))
            return VA_STATUS_ERROR_UNIMPLEMENTED;

        if (!check_fbo_extensions(ctx) || !load_fbo_extensions(ctx))
            return VA_STATUS_ERROR_UNIMPLEMENTED;
    }

    glx_ctx->is_initialized = 1;
    return VA_STATUS_SUCCESS;
}
