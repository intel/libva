#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>

#include "va_egl_private.h"
#include "va_egl_impl.h"

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
check_pixmap_extensions(VADriverContextP ctx, EGLDisplay egl_display)
{
    const char *exts;

    exts = (const char *)eglQueryString(egl_display, EGL_EXTENSIONS);

    if (!check_extension("EGL_KHR_image_pixmap", exts))
        return 0;

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
vaQuerySurfaceTargetsEGL_impl_driver(VADisplay dpy,
                                     EGLenum *target_list,
                                     int *num_targets)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;

    INVOKE(ctx, QuerySurfaceTargets, (ctx, target_list, num_targets));

    return VA_STATUS_SUCCESS;
}

static VAStatus 
vaCreateSurfaceEGL_impl_driver(VADisplay dpy,
                               EGLenum target,
                               unsigned int width,
                               unsigned int height,
                               VASurfaceEGL *gl_surface)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;

    INVOKE(ctx, CreateSurface, (ctx, target, width, height, gl_surface));

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
vaSyncSurfaceEGL_impl_driver(VADisplay dpy,
                             VASurfaceEGL egl_surface)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;

    INVOKE(ctx, SyncSurface, (ctx, egl_surface));

    return VA_STATUS_SUCCESS;
}

static VAStatus
vaGetSurfaceInfoEGL_impl_driver(VADisplay dpy,
                                VASurfaceEGL egl_surface,
                                EGLenum *target,
                                EGLClientBuffer *buffer,
                                EGLint *attrib_list,
                                int *num_attribs)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;

    INVOKE(ctx, GetSurfaceInfo, (ctx, egl_surface, target, buffer, attrib_list, num_attribs));

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
#define VA_SURFACE_IMPL_EGL_MAGIC VA_FOURCC('V','E','G','L')

struct VASurfaceImplEGL {
    uint32_t            magic;      ///< Magic number identifying a VASurfaceImplEGL
    VASurfaceID         surface;    ///< Associated VA surface
    EGLenum             target;     ///< EGL target
    EGLClientBuffer     buffer;
    unsigned int        width;
    unsigned int        height;
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
sync_associated_surface(VADriverContextP ctx, VASurfaceImplEGLP pSurfaceImplEGL)
{
    VAStatus status;

    status = sync_surface(ctx, pSurfaceImplEGL);

    if (status != VA_STATUS_SUCCESS)
        return status;

    if (pSurfaceImplEGL->target != EGL_NATIVE_PIXMAP_KHR)
        return VA_STATUS_ERROR_UNIMPLEMENTED;

    status = ctx->vtable->vaPutSurface(
        ctx,
        pSurfaceImplEGL->surface,
        (void *)pSurfaceImplEGL->buffer,
        0, 0, pSurfaceImplEGL->width, pSurfaceImplEGL->height,
        0, 0, pSurfaceImplEGL->width, pSurfaceImplEGL->height,
        NULL, 0,
        pSurfaceImplEGL->flags
        );

    if (status == VA_STATUS_SUCCESS) {
        eglWaitNative(EGL_CORE_NATIVE_ENGINE);
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
vaQuerySurfaceTargetsEGL_impl_libva(VADisplay dpy,
                                    EGLenum *target_list,
                                    int *num_targets)
{
    int i = 0;

    target_list[i++] = EGL_NATIVE_PIXMAP_KHR;
    *num_targets = i;

    return VA_STATUS_SUCCESS;
}

static VAStatus 
vaCreateSurfaceEGL_impl_libva(VADisplay dpy,
                              EGLenum target,
                              unsigned int width,
                              unsigned int height,
                              VASurfaceEGL *egl_surface)
{
    VASurfaceImplEGLP pSurfaceImplEGL = NULL;

    pSurfaceImplEGL = calloc(1, sizeof(*pSurfaceImplEGL));

    if (!pSurfaceImplEGL) {
        *egl_surface = 0;
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    pSurfaceImplEGL->magic = VA_SURFACE_IMPL_EGL_MAGIC;
    pSurfaceImplEGL->surface = VA_INVALID_SURFACE;
    pSurfaceImplEGL->target = target == 0 ? EGL_NATIVE_PIXMAP_KHR : target;
    pSurfaceImplEGL->buffer = 0;
    pSurfaceImplEGL->width = width;
    pSurfaceImplEGL->height = height;
    *egl_surface = (VASurfaceEGL)pSurfaceImplEGL;

    return VA_STATUS_SUCCESS;
}

static VAStatus
vaDestroySurfaceEGL_impl_libva(VADisplay dpy, VASurfaceEGL egl_surface)
{
    VASurfaceImplEGLP pSurfaceImplEGL;

    INIT_SURFACE(pSurfaceImplEGL, egl_surface);

    if (pSurfaceImplEGL->target == EGL_NATIVE_PIXMAP_KHR) {
        if (pSurfaceImplEGL->buffer) {
            destroy_native_pixmap(dpy, pSurfaceImplEGL->buffer);
            pSurfaceImplEGL->buffer = 0;
        }
    }

    free(pSurfaceImplEGL);

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

    if (surface == VA_INVALID_SURFACE)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    if (pSurfaceImplEGL->target == EGL_NATIVE_PIXMAP_KHR) {
        if (pSurfaceImplEGL->buffer)
            destroy_native_pixmap(dpy, pSurfaceImplEGL->buffer);

        pSurfaceImplEGL->buffer = create_native_pixmap(dpy, pSurfaceImplEGL->width, pSurfaceImplEGL->height);
    }

    pSurfaceImplEGL->surface = surface;
    pSurfaceImplEGL->flags = flags;

    if (pSurfaceImplEGL->buffer)
        return VA_STATUS_SUCCESS;
    
    return VA_STATUS_ERROR_UNKNOWN;
}

static VAStatus
vaSyncSurfaceEGL_impl_libva(VADisplay dpy,
                            VASurfaceEGL egl_surface)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;
    VASurfaceImplEGLP pSurfaceImplEGL;
    VAStatus status;

    INIT_SURFACE(pSurfaceImplEGL, egl_surface);

    status = sync_associated_surface(ctx, pSurfaceImplEGL);

    return status;
}

static VAStatus
vaGetSurfaceInfoEGL_impl_libva(VADisplay dpy,
                               VASurfaceEGL egl_surface,
                               EGLenum *target,
                               EGLClientBuffer *buffer,
                               EGLint *attrib_list,
                               int *num_attribs)
{
    VADriverContextP ctx = ((VADisplayContextP)(dpy))->pDriverContext;
    VASurfaceImplEGLP pSurfaceImplEGL;
    VAStatus status;
    int i = 0;

    INIT_SURFACE(pSurfaceImplEGL, egl_surface);

    if (pSurfaceImplEGL->surface == VA_INVALID_SURFACE)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    *target = pSurfaceImplEGL->target;
    *buffer = pSurfaceImplEGL->buffer;

    if (pSurfaceImplEGL->target == EGL_NATIVE_PIXMAP_KHR) {
        attrib_list[i++] = EGL_IMAGE_PRESERVED_KHR;
        attrib_list[i++] = EGL_TRUE;
        attrib_list[i++] = EGL_NONE;
    } else {
        /* FIXME later */
        attrib_list[i++] = EGL_NONE;
    }

    return VA_STATUS_SUCCESS;
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

    if (pSurfaceImplEGL->target == EGL_NATIVE_PIXMAP_KHR) {
        if (pSurfaceImplEGL->buffer)
            destroy_native_pixmap(dpy, pSurfaceImplEGL->buffer);

        pSurfaceImplEGL->buffer = 0;
    }

    pSurfaceImplEGL->surface = VA_INVALID_SURFACE;

    return VA_STATUS_SUCCESS;
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
        vtable->vaQuerySurfaceTargetsEGL = vaQuerySurfaceTargetsEGL_impl_driver;
        vtable->vaCreateSurfaceEGL = vaCreateSurfaceEGL_impl_driver;
        vtable->vaDestroySurfaceEGL = vaDestroySurfaceEGL_impl_driver;
        vtable->vaAssociateSurfaceEGL = vaAssociateSurfaceEGL_impl_driver;
        vtable->vaSyncSurfaceEGL = vaSyncSurfaceEGL_impl_driver;
        vtable->vaGetSurfaceInfoEGL = vaGetSurfaceInfoEGL_impl_driver;
        vtable->vaDeassociateSurfaceEGL = vaDeassociateSurfaceEGL_impl_driver;
    }
    else {
        if (pDisplayContext->vaCreateNativePixmap == NULL ||
            pDisplayContext->vaFreeNativePixmap == NULL)
            return VA_STATUS_ERROR_UNIMPLEMENTED;

        if (!check_pixmap_extensions(ctx, egl_ctx->egl_display))
            return VA_STATUS_ERROR_UNIMPLEMENTED;

        vtable->vaQuerySurfaceTargetsEGL = vaQuerySurfaceTargetsEGL_impl_libva;
        vtable->vaCreateSurfaceEGL = vaCreateSurfaceEGL_impl_libva;
        vtable->vaDestroySurfaceEGL = vaDestroySurfaceEGL_impl_libva;
        vtable->vaAssociateSurfaceEGL = vaAssociateSurfaceEGL_impl_libva;
        vtable->vaSyncSurfaceEGL = vaSyncSurfaceEGL_impl_libva;
        vtable->vaGetSurfaceInfoEGL = vaGetSurfaceInfoEGL_impl_libva;
        vtable->vaDeassociateSurfaceEGL = vaDeassociateSurfaceEGL_impl_libva;
    }

    egl_ctx->is_initialized = 1;

    return VA_STATUS_SUCCESS;
}
