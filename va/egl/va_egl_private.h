#ifndef _VA_EGL_PRIVATE_H_
#define _VA_EGL_PRIVATE_H_

#include "sysdeps.h"
#include "va.h"
#include "va_backend.h"
#include "va_egl.h"
#include "va_backend_egl.h"

typedef struct VAOpenGLESVTable *VAOpenGLESVTableP;

struct VAOpenGLESVTable {
    /* EGL_KHR_image_pixmap */
    PFNEGLCREATEIMAGEKHRPROC            egl_create_image_khr;
    PFNEGLDESTROYIMAGEKHRPROC           egl_destroy_image_khr;

    /* GL_OES_EGL_image */
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC gles_egl_image_target_texture_2d;
};

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
    VAStatus (*vaCreateSurfaceEGL)(
        VADisplay dpy,
        GLenum target,
        GLuint texture,
        unsigned int width,
        unsigned int height,
        VASurfaceEGL *egl_surface
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

    VAStatus (*vaUpdateAssociatedSurfaceEGL)(
        VADisplay dpy,
        VASurfaceEGL egl_surface
    );

    VAStatus (*vaDeassociateSurfaceEGL)(
        VADisplay dpy,
        VASurfaceEGL egl_surface
    );
};

struct VADriverContextEGL {
    struct VADriverVTablePrivEGL vtable;
    struct VAOpenGLESVTable gles_vtable;
    unsigned int is_initialized : 1;
    EGLDisplay  egl_display;
};

#endif /* _VA_EGL_PRIVATE_H_ */
