#ifndef _VA_EGL_H_
#define _VA_EGL_H_

#include <va/va.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void*   EGLClientBuffer;

VAStatus vaGetEGLClientBufferFromSurface (
    VADisplay dpy,
    VASurfaceID surface,	
    EGLClientBuffer *buffer     /* out*/
);

#ifdef __cplusplus
}
#endif

#endif /* _VA_EGL_H_ */
