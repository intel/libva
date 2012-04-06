#ifndef _VA_ANDROID_H_
#define _VA_ANDROID_H_

#include <va/va.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Returns a suitable VADisplay for VA API
 */
VADisplay vaGetDisplay (
    void *android_dpy
);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#ifdef ANDROID
#include <surfaceflinger/ISurface.h>
using namespace android;

/*
 * Output rendering
 * Following is the rendering interface for Android system, 
 * to get the decode output surface to an ISurface object.
 * It basically performs a de-interlacing (if needed), 
 * color space conversion and scaling to the destination
 * rectangle
 */
VAStatus vaPutSurface (
    VADisplay dpy,
    VASurfaceID surface,	
    sp<ISurface> draw, /* Android Window/Surface */
    short srcx,
    short srcy,
    unsigned short srcw,
    unsigned short srch,
    short destx,
    short desty,
    unsigned short destw,
    unsigned short desth,
    VARectangle *cliprects, /* client supplied destination clip list */
    unsigned int number_cliprects, /* number of clip rects in the clip list */
    unsigned int flags /* PutSurface flags */
);

#endif /* ANDROID */
#endif /* __cplusplus */

#endif /* _VA_ANDROID_H_ */
