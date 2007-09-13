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
#define VA_TOP_FIELD            0x00000001
#define VA_BOTTOM_FIELD         0x00000002
#define VA_FRAME_PICTURE        0x00000004 /* weave */
VAStatus vaPutSurface (
    VADisplay dpy,
    VASurface *surface,
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
    int flags /* de-interlacing flags */
);

/*
  This function copies a rectangle of dimension "width" by "height" 
  from the VASurface indicated by "surface" to the GLXPbuffer
  identified by its XID "pbuffer_id".  The locations of source and
  destination rectangles are specified differently for the VASurface
  source and the GLXPbuffer destination as follows.  On the VASurface,
  the rectangle location is specified in the usual X-Window fashion
  with srcx and srcy indicating the location of the upper left hand
  corner of the rectangle relative to the VASurface origin (the upper
  left hand corner of the XvMCSurface with positive Y axis going in the
  down direction). On the GLXPbuffer the rectangle location is
  specified in the usual OpenGL fashion with the dstx and dsty
  indicating the location of the lower left hand corner of the
  rectangle relative to the GLXPbuffer origin (the lower left hand
  corner of the GLXPbuffer with the positive Y axis going in the
  up direction).

  The "draw_buffer" takes the same OpenGL enums that glDrawBuffer()
  takes (ie. GL_FRONT_LEFT, GL_FRONT_RIGHT, GL_BACK_LEFT, GL_BACK_RIGHT,
  GL_FRONT, GL_BACK, GL_LEFT, GL_RIGHT or GL_FRONT_AND_BACK).  This
  indicates which buffer of the GLXPbuffer is to be used for the
  destination of the copy.  Buffers specified in the "draw_buffer"
  that do not exist in the GLXPbuffer are ignored.

  "flags" may be VA_TOP_FIELD, VA_BOTTOM_FIELD or VA_FRAME_PICTURE.
  If flags is not VA_FRAME_PICTURE, the srcy and height are in field
  coordinates, not frame.  That is, the total copyable height is half
  the height of the VASurface.
*/
VAStatus vaCopySurfaceToGLXPbuffer (
    VADisplay dpy,
    VASurface *surface,	
    XID pbuffer_id,
    short srcx,
    short srcy,
    unsigned short width,
    unsigned short height,
    short destx,
    short desty,
    unsigned int draw_buffer,
    unsigned int flags /* de-interlacing flags */
);

    
#ifdef __cplusplus
}
#endif

#endif /* _VA_X11_H_ */
