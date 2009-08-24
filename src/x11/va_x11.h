#ifndef _VA_X11_H_
#define _VA_X11_H_

#ifdef IN_LIBVA
#include "va.h"
#else
#include <va/va.h>
#endif
#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Returns a suitable VADisplay for VA API
 */
VADisplay vaGetDisplay (
    Display *dpy
);

/*
 * Output rendering
 * Following is the rendering interface for X windows, 
 * to get the decode output surface to a X drawable
 * It basically performs a de-interlacing (if needed), 
 * color space conversion and scaling to the destination
 * rectangle
 */
/* de-interlacing flags for vaPutSurface */
#define VA_FRAME_PICTURE	0x00000000 
#define VA_TOP_FIELD		0x00000001
#define VA_BOTTOM_FIELD		0x00000002

/* 
 * clears the drawable with background color.
 * for hardware overlay based implementation this flag
 * can be used to turn off the overlay
 */
#define VA_CLEAR_DRAWABLE	0x00000008 

/* color space conversion flags for vaPutSurface */
#define VA_SRC_BT601		0x00000010
#define VA_SRC_BT709		0x00000020

VAStatus vaPutSurface (
    VADisplay dpy,
    VASurfaceID surface,	
    Drawable draw, /* X Drawable */
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

#ifdef __cplusplus
}
#endif

#endif /* _VA_X11_H_ */
