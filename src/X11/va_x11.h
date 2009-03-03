/*
 * Copyright (c) 2007-2009 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL INTEL AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * Video Acceleration (VA) API Specification
 *
 * Rev. 0.30
 * <jonathan.bian@intel.com>
 *
 * Revision History:
 * rev 0.10 (12/10/2006 Jonathan Bian) - Initial draft
 * rev 0.11 (12/15/2006 Jonathan Bian) - Fixed some errors
 * rev 0.12 (02/05/2007 Jonathan Bian) - Added VC-1 data structures for slice level decode
 * rev 0.13 (02/28/2007 Jonathan Bian) - Added GetDisplay()
 * rev 0.14 (04/13/2007 Jonathan Bian) - Fixed MPEG-2 PictureParameter structure, cleaned up a few funcs.
 * rev 0.15 (04/20/2007 Jonathan Bian) - Overhauled buffer management
 * rev 0.16 (05/02/2007 Jonathan Bian) - Added error codes and fixed some issues with configuration
 * rev 0.17 (05/07/2007 Jonathan Bian) - Added H.264/AVC data structures for slice level decode.
 * rev 0.18 (05/14/2007 Jonathan Bian) - Added data structures for MPEG-4 slice level decode 
 *                                       and MPEG-2 motion compensation.
 * rev 0.19 (08/06/2007 Jonathan Bian) - Removed extra type for bitplane data.
 * rev 0.20 (08/08/2007 Jonathan Bian) - Added missing fields to VC-1 PictureParameter structure.
 * rev 0.21 (08/20/2007 Jonathan Bian) - Added image and subpicture support.
 * rev 0.22 (08/27/2007 Jonathan Bian) - Added support for chroma-keying and global alpha.
 * rev 0.23 (09/11/2007 Jonathan Bian) - Fixed some issues with images and subpictures.
 * rev 0.24 (09/18/2007 Jonathan Bian) - Added display attributes.
 * rev 0.25 (10/18/2007 Jonathan Bian) - Changed to use IDs only for some types.
 * rev 0.26 (11/07/2007 Waldo Bastian) - Change vaCreateBuffer semantics
 * rev 0.27 (11/19/2007 Matt Sottek)   - Added DeriveImage
 * rev 0.28 (12/06/2007 Jonathan Bian) - Added new versions of PutImage and AssociateSubpicture 
 *                                       to enable scaling
 * rev 0.29 (02/07/2008 Jonathan Bian) - VC1 parameter fixes,
 *                                       added VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED
 * rev 0.30 (03/01/2009 Jonathan Bian) - Added encoding support for H.264 BP and MPEG-4 SP and fixes
 *                                       for ISO C conformance.
 *
 * Acknowledgements:
 *  Some concepts borrowed from XvMC and XvImage.
 *  Waldo Bastian (Intel), Matt Sottek (Intel),  Austin Yuan (Intel), and Gwenole Beauchesne (SDS)
 *  contributed to various aspects of the API.
 */

#ifndef _VA_X11_H_
#define _VA_X11_H_

#include <va.h>
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
/* flags for vaPutSurface */
#define VA_FRAME_PICTURE	0x00000000 
#define VA_TOP_FIELD		0x00000001
#define VA_BOTTOM_FIELD		0x00000002
#define VA_SRC_BT601		0x00000010
#define VA_SRC_BT709		0x00000020

/* 
 * clears the drawable with background color.
 * for hardware overlay based implementation this flag
 * can be used to turn off the overlay
 */
#define VA_CLEAR_DRAWABLE	0x00000008 

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
