#ifndef _VA_EGL_H_
#define _VA_EGL_H_

#include <va/va.h>
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
 * Return maximum number of EGL targets supported by the implementation
 *
 * @param[in] dpy the VADisplay
 * @return the maximum number of EGL Target
 */
int vaMaxNumSurfaceTargetsEGL(
    VADisplay dpy
);

/**
 * Return maximum number of EGL surface attributes supported by the implementation
 *
 * @param[in] dpy the VADisplay
 * @return the maximum number of EGL surface attributes
 */
int vaMaxNumSurfaceAttributesEGL(
    VADisplay dpy
);

/**
 * Query supported EGL targets for eglCreateImageKHR(). 
 *
 * The caller must provide a "target_list" array that can hold at
 * least vaMaxNumSurfaceTargetsEGL() entries. The actual number of
 * targets returned in "target_list" is returned in "num_targets".
 *
 * @param[in]] dpy              the VADisplay
 * @param[out] target_list      the array to hold target entries
 * @param[out] num_targets      the actual number of targets
 * @return VA_STATUS_SUCCESS if successful
 */
VAStatus vaQuerySurfaceTargetsEGL(
    VADisplay dpy,
    EGLenum *target_list,       /* out */
    int *num_targets		/* out */
);

/**
 * Creates a VA/EGL surface with the specified target
 *
 * If target is 0, this means the best efficient target by default.
 *
 * @param[in] dpy               the VADisplay
 * @param[in] target            the specified EGL target
 * @param[in] width             the surface width
 * @param[in] height            the surface height
 * @param[out] gl_surface the VA/EGL surface
 * @return VA_STATUS_SUCCESS if successful
 */
VAStatus vaCreateSurfaceEGL(
    VADisplay dpy,
    EGLenum target,
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
 * Associate a EGL surface with a VA surface
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
 * @param[in] dpy         the VA display
 * @param[in] egl_surface the VA/EGL surface that has been associated with a VA surface
 * @return VA_STATUS_SUCCESS if successful
 */
VAStatus vaSyncSurfaceEGL(
    VADisplay dpy,
    VASurfaceEGL egl_surface
);

/**
 * Get the necessary information for eglCreateImageKHR()
 *
 * The caller must provide a "attrib_list" array that can hold at
 * least (2 * vaMaxNumSurfaceAttributesEGL()) entries. The last attribute 
 * specified in attrib_list must be EGL_NONE
 *
 * @param[in]  dpy         the VA display
 * @param[in]  egl_surface the VA/EGL surface that has been associated with a VA surface
 * @param[out] target      the type of <buffer> for eglCreateImageKHR()
 * @param[out] buffer      the EGLClientBuffer for eglCreateImageKHR()
 * @param[out] attrib_list the list of attribute-value pairs for eglCreateImageKHR()
 * @param[in/out] num_attribs input: the number of allocated attribute-value pairs in attrib_list; output: the actual number of attribute-value pairs
 * @return VA_STATUS_SUCCESS if successful
 */
VAStatus vaGetSurfaceInfoEGL(
    VADisplay dpy,
    VASurfaceEGL egl_surface,
    EGLenum *target,            /* out, the type of <buffer> */
    EGLClientBuffer *buffer,    /* out */
    EGLint *attrib_list,        /* out, the last attribute must be EGL_NONE */
    int *num_attribs            /* in/out, the number of attribute-value pairs */
);

/**
 * Deassociate a EGL surface
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
