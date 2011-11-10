#ifndef _VA_EGL_H_
#define _VA_EGL_H_

#include <va/va.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *VASurfaceEGL;

/*This function is used to get EGLClientBuffer
 * (lower 16bits is buffer index, upper 16bits 
 * is BC device id.) from surface id. Application
 * should maintain EGLClientBuffer itself.*/

VAStatus vaGetEGLClientBufferFromSurface (
    VADisplay dpy,
    VASurfaceID surface,	
    EGLClientBuffer *buffer     /* out*/
);

/**
 * Return a suitable VADisplay for VA API
 *
 * @param[in] native_dpy the native display
 * @param[in] egl_dpy the EGL display
 * @return a VADisplay
 */
VADisplay vaGetDisplayEGL(
    VANativeDisplay native_dpy,
    EGLDisplay egl_dpy
);

/**
 * Create a surface used for display to OpenGL ES
 *
 * The application shall maintain the live EGL context itself.
 *
 * @param[in]  dpy        the VA display
 * @param[in]  target     the GL target to which the texture needs to be bound, must be GL_TEXTURE_2D
 * @param[in]  texture    the GL texture
 * @param[in]  width      the surface width
 * @param[in]  height     the surface height
 * @param[out] gl_surface the VA/EGL surface
 * @return VA_STATUS_SUCCESS if successful
 */
VAStatus vaCreateSurfaceEGL(
    VADisplay dpy,
    GLenum target,
    GLuint texture,
    unsigned int width,
    unsigned int height,
    VASurfaceEGL *gl_surface
);

/**
 * Destroy a VA/EGL surface
 *
 * The application shall maintain the live EGL context itself.
 *
 * @param[in]  dpy        the VA display
 * @param[in]  gl_surface the VA surface
 * @return VA_STATUS_SUCCESS if successful
 */
VAStatus vaDestroySurfaceEGL(
    VADisplay dpy,
    VASurfaceEGL gl_surface
);

/**
 * Associate a EGLClientBuffer with a VA surface
 *
 * @param[in]  dpy         the VA display
 * @param[in]  egl_surface the VA/EGL destination surface
 * @param[in]  surface     the VA surface
 * @param[in]  flags       the flags to PutSurface
 * @return VA_STATUS_SUCCESS if successful
 */
VAStatus vaAssociateSurfaceEGL(
    VADisplay dpy,
    VASurfaceEGL egl_surface,
    VASurfaceID surface,
    unsigned int flags
);

/**
 * Update the content of a VA/EGL surface
 *
 * Changes to VA surface are committed to VA/EGL surface at this point.
 *
 * @param[in]  dpy         the VA display
 * @param[in]  egl_surface the VA/EGL destination surface
 * @return VA_STATUS_SUCCESS if successful
 */
VAStatus vaUpdateAssociatedSurfaceEGL(
    VADisplay dpy,
    VASurfaceEGL egl_surface
);

/**
 * Deassociate a EGLClientBuffer
 *
 * @param[in]  dpy         the VA display
 * @param[in]  egl_surface the VA/EGL destination surface
 * @return VA_STATUS_SUCCESS if successful
 */
VAStatus vaDeassociateSurfaceEGL(
    VADisplay dpy,
    VASurfaceEGL egl_surface
);

#ifdef __cplusplus
}
#endif

#endif /* _VA_EGL_H_ */
