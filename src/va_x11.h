/*
 * Video Decode Acceleration API, X11 specific functions
 *
 * Rev. 0.15
 * <jonathan.bian@intel.com>
 *
 * Revision History:
 * rev 0.1 (12/10/06 Jonathan Bian) - Initial draft
 * rev 0.11 (12/15/06 Jonathan Bian) - Fixed some errors
 * rev 0.12 (02/05/07 Jonathan Bian) - Added VC-1 data structures
 * rev 0.13 (02/28/07 Jonathan Bian) - Added GetDisplay()
 * rev 0.14 (04/13/07 Jonathan Bian) - Fixed MPEG-2 PictureParameter struct, cleaned up a few funcs.
 * rev 0.15 (04/20/07 Jonathan Bian) - Overhauled buffer management  
 *
 */

#ifndef _VA_X11_H_
#define _VA_X11_H_

#include "va.h"
#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Output rendering
 * Following is the rendering interface for X windows, 
 * to get the decode output surface to a X drawable
 * It basically performs a de-interlacing (if needed), 
 * color space conversion and scaling to the destination
 * rectangle
 */

/* de-interlace flags for vaPutSurface */
#define VA_FRAME_PICTURE        0x00000000 
#define VA_TOP_FIELD            0x00000001
#define VA_BOTTOM_FIELD         0x00000002
/* 
 * clears the drawable with background color.
 * for hardware overlay based implementation this flag
 * can be used to turn off the overlay
 */
#define VA_CLEAR_DRAWABLE       0x00000008 

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
    VARectangle *cliprects, /* client supplied clip list */
    unsigned int number_cliprects, /* number of clip rects in the clip list */
    unsigned int flags /* de-interlacing flags */
);

#ifdef __cplusplus
}
#endif

#endif /* _VA_X11_H_ */
