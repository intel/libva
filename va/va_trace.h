/*
 * Copyright (c) 2009 Intel Corporation. All Rights Reserved.
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
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef VA_TRACE_H
#define VA_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

extern int trace_flag;

#define VA_TRACE_FLAG_LOG             0x1
#define VA_TRACE_FLAG_BUFDATA         0x2
#define VA_TRACE_FLAG_CODEDBUF        0x4
#define VA_TRACE_FLAG_SURFACE_DECODE  0x8
#define VA_TRACE_FLAG_SURFACE_ENCODE  0x10
#define VA_TRACE_FLAG_SURFACE_JPEG    0x20
#define VA_TRACE_FLAG_SURFACE         (VA_TRACE_FLAG_SURFACE_DECODE | \
                                       VA_TRACE_FLAG_SURFACE_ENCODE | \
                                       VA_TRACE_FLAG_SURFACE_JPEG)

#define VA_TRACE_LOG(trace_func,...)            \
    if (trace_flag & VA_TRACE_FLAG_LOG) {       \
        trace_func(__VA_ARGS__);                \
    }
#define VA_TRACE_ALL(trace_func,...)            \
    if (trace_flag) {                           \
        trace_func(__VA_ARGS__);                \
    }

DLL_HIDDEN
void va_TraceInit(VADisplay dpy);
DLL_HIDDEN
void va_TraceEnd(VADisplay dpy);

DLL_HIDDEN
void va_TraceInitialize (
    VADisplay dpy,
    int *major_version,	 /* out */
    int *minor_version 	 /* out */
);

DLL_HIDDEN
void va_TraceTerminate (
    VADisplay dpy
);

DLL_HIDDEN
void va_TraceCreateConfig(
    VADisplay dpy,
    VAProfile profile, 
    VAEntrypoint entrypoint, 
    VAConfigAttrib *attrib_list,
    int num_attribs,
    VAConfigID *config_id /* out */
);

DLL_HIDDEN
void va_TraceCreateSurfaces(
    VADisplay dpy,
    int width,
    int height,
    int format,
    int num_surfaces,
    VASurfaceID *surfaces,	/* out */
    VASurfaceAttrib    *attrib_list,
    unsigned int        num_attribs
);

DLL_HIDDEN
void va_TraceDestroySurfaces(
    VADisplay dpy,
    VASurfaceID *surface_list,
    int num_surfaces
);

DLL_HIDDEN
void va_TraceCreateContext(
    VADisplay dpy,
    VAConfigID config_id,
    int picture_width,
    int picture_height,
    int flag,
    VASurfaceID *render_targets,
    int num_render_targets,
    VAContextID *context		/* out */
);

DLL_HIDDEN
void va_TraceCreateBuffer (
    VADisplay dpy,
    VAContextID context,	/* in */
    VABufferType type,		/* in */
    unsigned int size,		/* in */
    unsigned int num_elements,	/* in */
    void *data,			/* in */
    VABufferID *buf_id		/* out */
);

DLL_HIDDEN
void va_TraceDestroyBuffer (
    VADisplay dpy,
    VABufferID buf_id    /* in */
);

DLL_HIDDEN
void va_TraceMapBuffer (
    VADisplay dpy,
    VABufferID buf_id,	/* in */
    void **pbuf 	/* out */
);


DLL_HIDDEN
void va_TraceBeginPicture(
    VADisplay dpy,
    VAContextID context,
    VASurfaceID render_target
);

DLL_HIDDEN
void va_TraceRenderPicture(
    VADisplay dpy,
    VAContextID context,
    VABufferID *buffers,
    int num_buffers
);

DLL_HIDDEN
void va_TraceEndPicture(
    VADisplay dpy,
    VAContextID context,
    int endpic_done
);

DLL_HIDDEN
void va_TraceSyncSurface(
    VADisplay dpy,
    VASurfaceID render_target
);

DLL_HIDDEN
void va_TraceQuerySurfaceAttributes(
    VADisplay           dpy,
    VAConfigID          config,
    VASurfaceAttrib    *attrib_list,
    unsigned int       *num_attribs
);

DLL_HIDDEN
void va_TraceQuerySurfaceStatus(
    VADisplay dpy,
    VASurfaceID render_target,
    VASurfaceStatus *status	/* out */
);

DLL_HIDDEN
void va_TraceQuerySurfaceError(
	VADisplay dpy,
	VASurfaceID surface,
	VAStatus error_status,
	void **error_info /*out*/
);


DLL_HIDDEN
void va_TraceMaxNumDisplayAttributes (
    VADisplay dpy,
    int number
);

DLL_HIDDEN
void va_TraceQueryDisplayAttributes (
    VADisplay dpy,
    VADisplayAttribute *attr_list,	/* out */
    int *num_attributes			/* out */
);

DLL_HIDDEN
void va_TraceGetDisplayAttributes (
    VADisplay dpy,
    VADisplayAttribute *attr_list,
    int num_attributes
);

DLL_HIDDEN
void va_TraceSetDisplayAttributes (
    VADisplay dpy,
    VADisplayAttribute *attr_list,
    int num_attributes
);

/* extern function called by display side */
void va_TracePutSurface (
    VADisplay dpy,
    VASurfaceID surface,
    void *draw, /* the target Drawable */
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
    

#endif /* VA_TRACE_H */
