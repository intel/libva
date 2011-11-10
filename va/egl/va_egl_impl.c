#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>

#include "va_egl_private.h"
#include "va_egl_impl.h"

// Returns the OpenGL ES VTable
static inline VAOpenGLESVTableP 
gles_get_vtable(VADriverContextP ctx)
{
    return &VA_DRIVER_CONTEXT_EGL(ctx)->gles_vtable;
}

// Lookup for a EGL function
typedef void (*EGLFuncPtr)(void);
typedef EGLFuncPtr (*EGLGetProcAddressProc)(const char *);

static EGLFuncPtr 
get_proc_address_default(const char *name)
{
    return NULL;
}

static EGLGetProcAddressProc 
get_proc_address_func(void)
{
    EGLGetProcAddressProc get_proc_func;

    dlerror();
    get_proc_func = (EGLGetProcAddressProc)
        dlsym(RTLD_DEFAULT, "eglGetProcAddress");

    if (!dlerror())
        return get_proc_func;

    return get_proc_address_default;
}

static inline EGLFuncPtr 
get_proc_address(const char *name)
{
    static EGLGetProcAddressProc get_proc_func = NULL;

    get_proc_func = get_proc_address_func();

    return get_proc_func(name);
}

// Check for GLES extensions (TFP, FBO)
static int 
check_extension(const char *name, const char *exts)
{
    const char *end;
    int name_len, n;

    if (!name || !exts)
        return 0;

    end = exts + strlen(exts);
    name_len = strlen(name);

    while (exts < end) {
        n = strcspn(exts, " ");

        if (n == name_len && strncmp(name, exts, n) == 0)
            return 1;

        exts += (n + 1);
    }

    return 0;
}

static int 
check_extensions(VADriverContextP ctx, EGLDisplay egl_display)
{
    const char *exts;

    exts = (const char *)eglQueryString(egl_display, EGL_EXTENSIONS);

    if (!check_extension("EGL_KHR_image_pixmap", exts))
        return 0;

    exts = (const char *)glGetString(GL_EXTENSIONS);

    if (!check_extension("GL_OES_EGL_image", exts))
        return 0;

#if 0
    if (!check_extension("GL_OES_texture_npot", exts))
        return 0;
#endif

    return 1;
}

static int 
init_extensions(VADriverContextP ctx)
{
    VAOpenGLESVTableP pOpenGLESVTable = gles_get_vtable(ctx);

    /* EGL_KHR_image_pixmap */
    pOpenGLESVTable->egl_create_image_khr = 
        (PFNEGLCREATEIMAGEKHRPROC)get_proc_address("eglCreateImageKHR");
    pOpenGLESVTable->egl_destroy_image_khr =
        (PFNEGLDESTROYIMAGEKHRPROC)get_proc_address("eglDestroyImageKHR");

    /* GL_OES_EGL_image */
    pOpenGLESVTable->gles_egl_image_target_texture_2d =
        (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)get_proc_address("glEGLImageTargetTexture2DOES");

    return 1;
}

/* ========================================================================= */
/* === VA/EGL implementation from the driver (fordward calls)            === */
/* ========================================================================= */
#ifdef INVOKE
#undef INVOKE
#endif

#define INVOKE(ctx, func, args) do {                    \
        VADriverVTableEGLP vtable = (ctx)->vtable_egl;  \
        if (!vtable->va##func##EGL)                     \
            return VA_STATUS_ERROR_UNIMPLEMENTED;       \
                                                        \
        VAStatus status = vtable->va##func##EGL args;   \
        if (status != VA_STATUS_SUCCESS)                \
            return status;                              \
    } while (0)

static VAStatus
vaCreateSurfaceEGL_impl_driver(VADisplay dpy,
                               GLenum target,
                               GLuint texture,
                               unsigned int width,
                               unsigned int height,
                               VASurfaceEGL *egl_surface)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;
    INVOKE(ctx, CreateSurface, (ctx, target, texture, width, height, egl_surface));
    return VA_STATUS_SUCCESS;
}

static VAStatus
vaDestroySurfaceEGL_impl_driver(VADisplay dpy, VASurfaceEGL egl_surface)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;
    INVOKE(ctx, DestroySurface, (ctx, egl_surface));
    return VA_STATUS_SUCCESS;
}

static VAStatus
vaAssociateSurfaceEGL_impl_driver(VADisplay dpy,
                                  VASurfaceEGL egl_surface,
                                  VASurfaceID surface,
                                  unsigned int flags)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;
    INVOKE(ctx, AssociateSurface, (ctx, egl_surface, surface, flags));

    return VA_STATUS_SUCCESS;
}

static VAStatus
vaUpdateAssociatedSurfaceEGL_impl_driver(VADisplay dpy,
                                         VASurfaceEGL egl_surface)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;
    INVOKE(ctx, UpdateAssociatedSurface, (ctx, egl_surface));

    return VA_STATUS_SUCCESS;
}

static VAStatus
vaDeassociateSurfaceEGL_impl_driver(VADisplay dpy,
                                    VASurfaceEGL egl_surface)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;
    INVOKE(ctx, DeassociateSurface, (ctx, egl_surface));

    return VA_STATUS_SUCCESS;
}

#undef INVOKE

/* ========================================================================= */
/* === VA/EGL helpers                                                    === */
/* ========================================================================= */
/** Unique VASurfaceImplEGL identifier */
#define VA_SURFACE_IMPL_EGL_MAGIC VA_FOURCC('V','I','E','L')

struct VASurfaceImplEGL {
    uint32_t            magic;      ///< Magic number identifying a VASurfaceImplEGL
    VASurfaceID         surface;    ///< Associated VA surface
    GLenum              target;     ///< GL target to which the texture is bound
    GLuint              texture;    ///< GL texture
    unsigned int        width;
    unsigned int        height;
    void                *pixmap;
    EGLImageKHR         img_khr;
    unsigned int        flags;
};

static void *
create_native_pixmap(VADisplay dpy, unsigned int width, unsigned int height)
{
    VADisplayContextP pDisplayContext = (VADisplayContextP)dpy;
    VAStatus status;
    void *native_pixmap = NULL;

    status = pDisplayContext->vaCreateNativePixmap(pDisplayContext, width, height, &native_pixmap);

    if (status != VA_STATUS_SUCCESS)
        native_pixmap = NULL;

    return native_pixmap;
}

static void
destroy_native_pixmap(VADisplay dpy, void *native_pixmap)
{
    VADisplayContextP pDisplayContext = (VADisplayContextP)dpy;
    
    pDisplayContext->vaFreeNativePixmap(pDisplayContext, native_pixmap);
}

// Destroy VA/EGL surface
static void
destroy_surface(VADisplay dpy, VASurfaceImplEGLP pSurfaceImplEGL)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;
    VADriverContextEGLP egl_ctx = VA_DRIVER_CONTEXT_EGL(ctx);
    VAOpenGLESVTableP const pOpenGLESVTable = gles_get_vtable(ctx);

    if (pSurfaceImplEGL->img_khr) {
        pOpenGLESVTable->egl_destroy_image_khr(egl_ctx->egl_display,
                                               pSurfaceImplEGL->img_khr);
        pSurfaceImplEGL->img_khr = EGL_NO_IMAGE_KHR;
    }

    if (pSurfaceImplEGL->pixmap) {
        destroy_native_pixmap(dpy, pSurfaceImplEGL->pixmap);
        pSurfaceImplEGL->pixmap = 0;
    }

    free(pSurfaceImplEGL);
}

// Create VA/EGL surface
static VASurfaceImplEGLP
create_surface(VADisplay dpy,
               GLenum target,
               GLuint texture,
               unsigned int width,
               unsigned int height)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;
    VADriverContextEGLP egl_ctx = VA_DRIVER_CONTEXT_EGL(ctx);
    VAOpenGLESVTableP const pOpenGLESVTable = gles_get_vtable(ctx);
    VASurfaceImplEGLP pSurfaceImplEGL = NULL;
    int is_error = 1;
    const EGLint img_attribs[] = {
        EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
        EGL_NONE
    };

    pSurfaceImplEGL = calloc(1, sizeof(*pSurfaceImplEGL));

    if (!pSurfaceImplEGL)
        goto end;

    pSurfaceImplEGL->magic = VA_SURFACE_IMPL_EGL_MAGIC;
    pSurfaceImplEGL->surface = VA_INVALID_SURFACE;
    pSurfaceImplEGL->target = target;
    pSurfaceImplEGL->texture = texture;
    pSurfaceImplEGL->width = width;
    pSurfaceImplEGL->height = height;
    pSurfaceImplEGL->pixmap = create_native_pixmap(dpy, width, height);

    if (!pSurfaceImplEGL->pixmap)
        goto end;

    pSurfaceImplEGL->img_khr = pOpenGLESVTable->egl_create_image_khr(egl_ctx->egl_display,
                                                                 EGL_NO_CONTEXT,
                                                                 EGL_NATIVE_PIXMAP_KHR,
                                                                 (EGLClientBuffer)pSurfaceImplEGL->pixmap,
                                                                 img_attribs);
    
    if (!pSurfaceImplEGL->img_khr)
        goto end;

    is_error = 0;

end:
    if (is_error && pSurfaceImplEGL) {
        destroy_surface(dpy, pSurfaceImplEGL);
        pSurfaceImplEGL = NULL;
    }

    return pSurfaceImplEGL;
}

// Check VASurfaceImplEGL is valid
static inline int check_surface(VASurfaceImplEGLP pSurfaceImplEGL)
{
    return pSurfaceImplEGL && pSurfaceImplEGL->magic == VA_SURFACE_IMPL_EGL_MAGIC;
}

static inline VAStatus
deassociate_surface(VADriverContextP ctx, VASurfaceImplEGLP pSurfaceImplEGL)
{
    pSurfaceImplEGL->surface = VA_INVALID_SURFACE;

    return VA_STATUS_SUCCESS;
}

static VAStatus
associate_surface(VADriverContextP ctx,
                  VASurfaceImplEGLP pSurfaceImplEGL,
                  VASurfaceID surface,
                  unsigned int flags)
{
    VAStatus status;

    status = deassociate_surface(ctx, pSurfaceImplEGL);

    if (status != VA_STATUS_SUCCESS)
        return status;

    pSurfaceImplEGL->surface = surface;
    pSurfaceImplEGL->flags = flags;

    return VA_STATUS_SUCCESS;
}

static inline VAStatus
sync_surface(VADriverContextP ctx, VASurfaceImplEGLP pSurfaceImplEGL)
{
    if (pSurfaceImplEGL->surface == VA_INVALID_SURFACE)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    return ctx->vtable->vaSyncSurface(ctx, pSurfaceImplEGL->surface);
}

static VAStatus
update_accociated_surface(VADriverContextP ctx, VASurfaceImplEGLP pSurfaceImplEGL)
{
    VAOpenGLESVTableP pOpenGLESVTable = gles_get_vtable(ctx);
    VAStatus status;

    status = sync_surface(ctx, pSurfaceImplEGL);

    if (status != VA_STATUS_SUCCESS)
        return status;

    status = ctx->vtable->vaPutSurface(
        ctx,
        pSurfaceImplEGL->surface,
        (void *)pSurfaceImplEGL->pixmap,
        0, 0, pSurfaceImplEGL->width, pSurfaceImplEGL->height,
        0, 0, pSurfaceImplEGL->width, pSurfaceImplEGL->height,
        NULL, 0,
        pSurfaceImplEGL->flags
    );

    if (status == VA_STATUS_SUCCESS) {
        eglWaitNative(EGL_CORE_NATIVE_ENGINE);
        pOpenGLESVTable->gles_egl_image_target_texture_2d(pSurfaceImplEGL->target,
                                                          (GLeglImageOES)pSurfaceImplEGL->img_khr);
    }

    return status;
}

/* ========================================================================= */
/* === VA/EGL implementation from libVA (generic and suboptimal path)    === */
/* ========================================================================= */
#ifdef INIT_SURFACE
#undef INIT_SURFACE
#endif

#define INIT_SURFACE(surface, egl_surface) do {         \
        surface = (VASurfaceImplEGLP)(egl_surface);     \
        if (!check_surface(surface))                    \
            return VA_STATUS_ERROR_INVALID_SURFACE;     \
    } while (0)

static VAStatus
vaCreateSurfaceEGL_impl_libva(VADisplay dpy,
                              GLenum target,
                              GLuint texture,
                              unsigned int width,
                              unsigned int height,
                              VASurfaceEGL *egl_surface)
{
    VASurfaceEGL pSurfaceEGL;

    pSurfaceEGL = create_surface(dpy, target, texture, width, height);

    if (!pSurfaceEGL)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    *egl_surface = pSurfaceEGL;

    return VA_STATUS_SUCCESS;
}

static VAStatus
vaDestroySurfaceEGL_impl_libva(VADisplay dpy, VASurfaceEGL egl_surface)
{
    VASurfaceImplEGLP pSurfaceImplEGL;

    INIT_SURFACE(pSurfaceImplEGL, egl_surface);

    destroy_surface(dpy, pSurfaceImplEGL);

    return VA_STATUS_SUCCESS;
}

static VAStatus
vaAssociateSurfaceEGL_impl_libva(
    VADisplay dpy,
    VASurfaceEGL egl_surface,
    VASurfaceID surface,
    unsigned int flags
)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;
    VASurfaceImplEGLP pSurfaceImplEGL;
    VAStatus status;

    INIT_SURFACE(pSurfaceImplEGL, egl_surface);

    status = associate_surface(ctx, egl_surface, surface, flags);

    return status;
}

static VAStatus
vaUpdateAssociatedSurfaceEGL_impl_libva(
    VADisplay dpy,
    VASurfaceEGL egl_surface
)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;
    VASurfaceImplEGLP pSurfaceImplEGL;
    VAStatus status;

    INIT_SURFACE(pSurfaceImplEGL, egl_surface);

    status = update_accociated_surface(ctx, egl_surface);

    return status;
}

static VAStatus
vaDeassociateSurfaceEGL_impl_libva(
    VADisplay dpy,
    VASurfaceEGL egl_surface
)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;
    VASurfaceImplEGLP pSurfaceImplEGL;
    VAStatus status;

    INIT_SURFACE(pSurfaceImplEGL, egl_surface);

    status = deassociate_surface(ctx, egl_surface);

    return status;
}

#undef INIT_SURFACE

/* ========================================================================= */
/* === Private VA/EGL vtable initialization                              === */
/* ========================================================================= */

// Initialize EGL driver context
VAStatus va_egl_init_context(VADisplay dpy)
{
    VADisplayContextP pDisplayContext = (VADisplayContextP)dpy;
    VADriverContextP ctx = pDisplayContext->pDriverContext;
    VADriverContextEGLP egl_ctx = VA_DRIVER_CONTEXT_EGL(ctx);
    VADriverVTablePrivEGLP  vtable  = &egl_ctx->vtable;

    if (egl_ctx->is_initialized)
        return VA_STATUS_SUCCESS;

    if (ctx->vtable_egl && ctx->vtable_egl->vaCreateSurfaceEGL) {
        vtable->vaCreateSurfaceEGL = vaCreateSurfaceEGL_impl_driver;
        vtable->vaDestroySurfaceEGL = vaDestroySurfaceEGL_impl_driver;
        vtable->vaAssociateSurfaceEGL = vaAssociateSurfaceEGL_impl_driver;
        vtable->vaUpdateAssociatedSurfaceEGL = vaUpdateAssociatedSurfaceEGL_impl_driver;
        vtable->vaDeassociateSurfaceEGL = vaDeassociateSurfaceEGL_impl_driver;
    }
    else {
        if (pDisplayContext->vaCreateNativePixmap == NULL ||
            pDisplayContext->vaFreeNativePixmap == NULL)
            return VA_STATUS_ERROR_UNIMPLEMENTED;

        vtable->vaCreateSurfaceEGL = vaCreateSurfaceEGL_impl_libva;
        vtable->vaDestroySurfaceEGL = vaDestroySurfaceEGL_impl_libva;
        vtable->vaAssociateSurfaceEGL = vaAssociateSurfaceEGL_impl_libva;
        vtable->vaUpdateAssociatedSurfaceEGL = vaUpdateAssociatedSurfaceEGL_impl_libva;
        vtable->vaDeassociateSurfaceEGL = vaDeassociateSurfaceEGL_impl_libva;

        if (!check_extensions(ctx, egl_ctx->egl_display) || !init_extensions(ctx))
            return VA_STATUS_ERROR_UNIMPLEMENTED;
    }

    egl_ctx->is_initialized = 1;

    return VA_STATUS_SUCCESS;
}
