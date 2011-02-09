#ifndef _VA_EGL_H_
#define _VA_EGL_H_

#include <va/va.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void*   EGLClientBuffer;

/*This function is used to get EGLClientBuffer
 * (lower 16bits is buffer index, upper 16bits 
 * is BC device id.) from surface id. Application
 * should maintain EGLClientBuffer itself.*/

VAStatus vaGetEGLClientBufferFromSurface (
    VADisplay dpy,
    VASurfaceID surface,	
    EGLClientBuffer *buffer     /* out*/
);

#ifdef __cplusplus
}
#endif

#endif /* _VA_EGL_H_ */
