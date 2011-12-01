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
