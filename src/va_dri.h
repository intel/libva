/* $XFree86: xc/lib/GL/dri/xf86dri.h,v 1.8 2002/10/30 12:51:25 alanh Exp $ */
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
Copyright 2000 VA Linux Systems, Inc.
Copyright 2007 Intel Corporation
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/**
 * \file xf86dri.h
 * Protocol numbers and function prototypes for DRI X protocol.
 *
 * \author Kevin E. Martin <martin@valinux.com>
 * \author Jens Owen <jens@tungstengraphics.com>
 * \author Rickard E. (Rik) Faith <faith@valinux.com>
 */

#ifndef _VA_DRI_H_
#define _VA_DRI_H_

#include <X11/Xfuncproto.h>
#include <xf86drm.h>

#define X_VA_DRIQueryVersion			0
#define X_VA_DRIQueryDirectRenderingCapable	1
#define X_VA_DRIOpenConnection			2
#define X_VA_DRICloseConnection		3
#define X_VA_DRIGetClientDriverName		4
#define X_VA_DRICreateContext			5
#define X_VA_DRIDestroyContext			6
#define X_VA_DRICreateDrawable			7
#define X_VA_DRIDestroyDrawable		8
#define X_VA_DRIGetDrawableInfo		9
#define X_VA_DRIGetDeviceInfo			10
#define X_VA_DRIAuthConnection                 11
#define X_VA_DRIOpenFullScreen                 12   /* Deprecated */
#define X_VA_DRICloseFullScreen                13   /* Deprecated */

#define VA_DRINumberEvents		0

#define VA_DRIClientNotLocal		0
#define VA_DRIOperationNotSupported	1
#define VA_DRINumberErrors		(VA_DRIOperationNotSupported + 1)

typedef unsigned long __DRIid;
typedef void __DRInativeDisplay;

_XFUNCPROTOBEGIN

Bool VA_DRIQueryExtension( Display *dpy, int *event_base, int *error_base );

Bool VA_DRIQueryVersion( Display *dpy, int *majorVersion, int *minorVersion,
    int *patchVersion );

Bool VA_DRIQueryDirectRenderingCapable( Display *dpy, int screen,
    Bool *isCapable );

Bool VA_DRIOpenConnection( Display *dpy, int screen, drm_handle_t *hSAREA,
    char **busIDString );

Bool VA_DRIAuthConnection( Display *dpy, int screen, drm_magic_t magic );

Bool VA_DRICloseConnection( Display *dpy, int screen );

Bool VA_DRIGetClientDriverName( Display *dpy, int screen,
    int *ddxDriverMajorVersion, int *ddxDriverMinorVersion,
    int *ddxDriverPatchVersion, char **clientDriverName );

Bool VA_DRICreateContext( Display *dpy, int screen, Visual *visual,
    XID *ptr_to_returned_context_id, drm_context_t *hHWContext );

Bool VA_DRICreateContextWithConfig( Display *dpy, int screen, int configID,
    XID *ptr_to_returned_context_id, drm_context_t *hHWContext );

Bool VA_DRIDestroyContext( __DRInativeDisplay *dpy, int screen,
    __DRIid context_id );

Bool VA_DRICreateDrawable( __DRInativeDisplay *dpy, int screen,
    __DRIid drawable, drm_drawable_t *hHWDrawable );

Bool VA_DRIDestroyDrawable( __DRInativeDisplay *dpy, int screen, 
    __DRIid drawable);

Bool VA_DRIGetDrawableInfo( Display *dpy, int screen, Drawable drawable,
    unsigned int *index, unsigned int *stamp, 
    int *X, int *Y, int *W, int *H,
    int *numClipRects, drm_clip_rect_t ** pClipRects,
    int *backX, int *backY,
    int *numBackClipRects, drm_clip_rect_t **pBackClipRects );

Bool VA_DRIGetDeviceInfo( Display *dpy, int screen,
    drm_handle_t *hFrameBuffer, int *fbOrigin, int *fbSize,
    int *fbStride, int *devPrivateSize, void **pDevPrivate );

_XFUNCPROTOEND

#endif /* _VA_DRI_H_ */

