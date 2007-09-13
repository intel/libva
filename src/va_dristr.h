/* $XFree86: xc/lib/GL/dri/xf86dristr.h,v 1.10 2002/10/30 12:51:25 alanh Exp $ */
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

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Jens Owen <jens@tungstengraphics.com>
 *   Rickard E. (Rik) Fiath <faith@valinux.com>
 *
 */

#ifndef _VA_DRISTR_H_
#define _VA_DRISTR_H_

#include "va_dri.h"

#define VA_DRINAME "XFree86-DRI"

/* The DRI version number.  This was originally set to be the same of the
 * XFree86 version number.  However, this version is really indepedent of
 * the XFree86 version.
 *
 * Version History:
 *    4.0.0: Original
 *    4.0.1: Patch to bump clipstamp when windows are destroyed, 28 May 02
 *    4.1.0: Add transition from single to multi in DRMInfo rec, 24 Jun 02
 */
#define VA_DRI_MAJOR_VERSION	4
#define VA_DRI_MINOR_VERSION	1
#define VA_DRI_PATCH_VERSION	0

typedef struct _VA_DRIQueryVersion {
    CARD8	reqType;		/* always DRIReqCode */
    CARD8	driReqType;		/* always X_DRIQueryVersion */
    CARD16	length B16;
} xVA_DRIQueryVersionReq;
#define sz_xVA_DRIQueryVersionReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	majorVersion B16;	/* major version of DRI protocol */
    CARD16	minorVersion B16;	/* minor version of DRI protocol */
    CARD32	patchVersion B32;       /* patch version of DRI protocol */
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xVA_DRIQueryVersionReply;
#define sz_xVA_DRIQueryVersionReply	32

typedef struct _VA_DRIQueryDirectRenderingCapable {
    CARD8	reqType;		/* always DRIReqCode */
    CARD8	driReqType;		/* X_DRIQueryDirectRenderingCapable */
    CARD16	length B16;
    CARD32	screen B32;
} xVA_DRIQueryDirectRenderingCapableReq;
#define sz_xVA_DRIQueryDirectRenderingCapableReq	8

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    BOOL	isCapable;
    BOOL	pad2;
    BOOL	pad3;
    BOOL	pad4;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
    CARD32	pad7 B32;
    CARD32	pad8 B32;
    CARD32	pad9 B32;
} xVA_DRIQueryDirectRenderingCapableReply;
#define sz_xVA_DRIQueryDirectRenderingCapableReply	32

typedef struct _VA_DRIOpenConnection {
    CARD8	reqType;		/* always DRIReqCode */
    CARD8	driReqType;		/* always X_DRIOpenConnection */
    CARD16	length B16;
    CARD32	screen B32;
} xVA_DRIOpenConnectionReq;
#define sz_xVA_DRIOpenConnectionReq	8

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	hSAREALow B32;
    CARD32	hSAREAHigh B32;
    CARD32	busIdStringLength B32;
    CARD32	pad6 B32;
    CARD32	pad7 B32;
    CARD32	pad8 B32;
} xVA_DRIOpenConnectionReply;
#define sz_xVA_DRIOpenConnectionReply	32

typedef struct _VA_DRIAuthConnection {
    CARD8	reqType;		/* always DRIReqCode */
    CARD8	driReqType;		/* always X_DRICloseConnection */
    CARD16	length B16;
    CARD32	screen B32;
    CARD32      magic B32;
} xVA_DRIAuthConnectionReq;
#define sz_xVA_DRIAuthConnectionReq	12

typedef struct {
    BYTE        type;
    BOOL        pad1;
    CARD16      sequenceNumber B16;
    CARD32      length B32;
    CARD32      authenticated B32;
    CARD32      pad2 B32;
    CARD32      pad3 B32;
    CARD32      pad4 B32;
    CARD32      pad5 B32;
    CARD32      pad6 B32;
} xVA_DRIAuthConnectionReply;
#define zx_xVA_DRIAuthConnectionReply  32

typedef struct _VA_DRICloseConnection {
    CARD8	reqType;		/* always DRIReqCode */
    CARD8	driReqType;		/* always X_DRICloseConnection */
    CARD16	length B16;
    CARD32	screen B32;
} xVA_DRICloseConnectionReq;
#define sz_xVA_DRICloseConnectionReq	8

typedef struct _VA_DRIGetClientDriverName {
    CARD8	reqType;		/* always DRIReqCode */
    CARD8	driReqType;		/* always X_DRIGetClientDriverName */
    CARD16	length B16;
    CARD32	screen B32;
} xVA_DRIGetClientDriverNameReq;
#define sz_xVA_DRIGetClientDriverNameReq	8

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	ddxDriverMajorVersion B32;
    CARD32	ddxDriverMinorVersion B32;
    CARD32	ddxDriverPatchVersion B32;
    CARD32	clientDriverNameLength B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xVA_DRIGetClientDriverNameReply;
#define sz_xVA_DRIGetClientDriverNameReply	32

typedef struct _VA_DRICreateContext {
    CARD8	reqType;		/* always DRIReqCode */
    CARD8	driReqType;		/* always X_DRICreateContext */
    CARD16	length B16;
    CARD32	screen B32;
    CARD32	visual B32;
    CARD32	context B32;
} xVA_DRICreateContextReq;
#define sz_xVA_DRICreateContextReq	16

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	hHWContext B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xVA_DRICreateContextReply;
#define sz_xVA_DRICreateContextReply	32

typedef struct _VA_DRIDestroyContext {
    CARD8	reqType;		/* always DRIReqCode */
    CARD8	driReqType;		/* always X_DRIDestroyContext */
    CARD16	length B16;
    CARD32	screen B32;
    CARD32	context B32;
} xVA_DRIDestroyContextReq;
#define sz_xVA_DRIDestroyContextReq	12

typedef struct _VA_DRICreateDrawable {
    CARD8	reqType;		/* always DRIReqCode */
    CARD8	driReqType;		/* always X_DRICreateDrawable */
    CARD16	length B16;
    CARD32	screen B32;
    CARD32	drawable B32;
} xVA_DRICreateDrawableReq;
#define sz_xVA_DRICreateDrawableReq	12

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	hHWDrawable B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xVA_DRICreateDrawableReply;
#define sz_xVA_DRICreateDrawableReply	32

typedef struct _VA_DRIDestroyDrawable {
    CARD8	reqType;		/* always DRIReqCode */
    CARD8	driReqType;		/* always X_DRIDestroyDrawable */
    CARD16	length B16;
    CARD32	screen B32;
    CARD32	drawable B32;
} xVA_DRIDestroyDrawableReq;
#define sz_xVA_DRIDestroyDrawableReq	12

typedef struct _VA_DRIGetDrawableInfo {
    CARD8	reqType;		/* always DRIReqCode */
    CARD8	driReqType;		/* always X_DRIGetDrawableInfo */
    CARD16	length B16;
    CARD32	screen B32;
    CARD32	drawable B32;
} xVA_DRIGetDrawableInfoReq;
#define sz_xVA_DRIGetDrawableInfoReq	12

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	drawableTableIndex B32;
    CARD32	drawableTableStamp B32;
    INT16	drawableX B16;
    INT16	drawableY B16;
    INT16	drawableWidth B16;
    INT16	drawableHeight B16;
    CARD32	numClipRects B32;
    INT16       backX B16;
    INT16       backY B16;
    CARD32      numBackClipRects B32;
} xVA_DRIGetDrawableInfoReply;

#define sz_xVA_DRIGetDrawableInfoReply	36


typedef struct _VA_DRIGetDeviceInfo {
    CARD8	reqType;		/* always DRIReqCode */
    CARD8	driReqType;		/* always X_DRIGetDeviceInfo */
    CARD16	length B16;
    CARD32	screen B32;
} xVA_DRIGetDeviceInfoReq;
#define sz_xVA_DRIGetDeviceInfoReq	8

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	hFrameBufferLow B32;
    CARD32	hFrameBufferHigh B32;
    CARD32	framebufferOrigin B32;
    CARD32	framebufferSize B32;
    CARD32	framebufferStride B32;
    CARD32	devPrivateSize B32;
} xVA_DRIGetDeviceInfoReply;
#define sz_xVA_DRIGetDeviceInfoReply	32

typedef struct _VA_DRIOpenFullScreen {
    CARD8       reqType;	/* always DRIReqCode */
    CARD8       driReqType;	/* always X_DRIOpenFullScreen */
    CARD16      length B16;
    CARD32      screen B32;
    CARD32      drawable B32;
} xVA_DRIOpenFullScreenReq;
#define sz_xVA_DRIOpenFullScreenReq    12

typedef struct {
    BYTE        type;
    BOOL        pad1;
    CARD16      sequenceNumber B16;
    CARD32      length B32;
    CARD32      isFullScreen B32;
    CARD32      pad2 B32;
    CARD32      pad3 B32;
    CARD32      pad4 B32;
    CARD32      pad5 B32;
    CARD32      pad6 B32;
} xVA_DRIOpenFullScreenReply;
#define sz_xVA_DRIOpenFullScreenReply  32

typedef struct _VA_DRICloseFullScreen {
    CARD8       reqType;	/* always DRIReqCode */
    CARD8       driReqType;	/* always X_DRICloseFullScreen */
    CARD16      length B16;
    CARD32      screen B32;
    CARD32      drawable B32;
} xVA_DRICloseFullScreenReq;
#define sz_xVA_DRICloseFullScreenReq   12

typedef struct {
    BYTE        type;
    BOOL        pad1;
    CARD16      sequenceNumber B16;
    CARD32      length B32;
    CARD32      pad2 B32;
    CARD32      pad3 B32;
    CARD32      pad4 B32;
    CARD32      pad5 B32;
    CARD32      pad6 B32;
    CARD32      pad7 B32;
} xVA_DRICloseFullScreenReply;
#define sz_xVA_DRICloseFullScreenReply  32


#endif /* _VA_DRISTR_H_ */
