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


#ifndef VA_FOOL_H
#define VA_FOOL_H

#include <stdio.h>

void va_FoolInit(VADisplay dpy);

int va_FoolEnd(VADisplay dpy);


int va_FoolGetFrame(FILE *input_fp, char *frame_buf);

int va_FoolCodedBuf(VADisplay dpy);


int va_FoolCreateConfig(
    VADisplay dpy,
    VAProfile profile, 
    VAEntrypoint entrypoint, 
    VAConfigAttrib *attrib_list,
    int num_attribs,
    VAConfigID *config_id /* out */
);

int va_FoolCreateSurfaces(
    VADisplay dpy,
    int width,
    int height,
    int format,
    int num_surfaces,
    VASurfaceID *surfaces	/* out */
);

VAStatus va_FoolCreateBuffer (
    VADisplay dpy,
    VAContextID context,	/* in */
    VABufferType type,		/* in */
    unsigned int size,		/* in */
    unsigned int num_elements,	/* in */
    void *data,			/* in */
    VABufferID *buf_id		/* out */
);

VAStatus va_FoolMapBuffer (
    VADisplay dpy,
    VABufferID buf_id,	/* in */
    void **pbuf 	/* out */
);

int va_FoolBeginPicture(
    VADisplay dpy,
    VAContextID context,
    VASurfaceID render_target
);

int va_FoolRenderPicture(
    VADisplay dpy,
    VAContextID context,
    VABufferID *buffers,
    int num_buffers
);

int va_FoolEndPicture(
    VADisplay dpy,
    VAContextID context
);

VAStatus va_FoolUnmapBuffer (
    VADisplay dpy,
    VABufferID buf_id  /* in */
);


VAStatus va_FoolQuerySubpictureFormats (
    VADisplay dpy,
    VAImageFormat *format_list,
    unsigned int *flags,
    unsigned int *num_formats
);
int va_FoolSyncSurface(
    VADisplay dpy, 
    VASurfaceID render_target
);



#endif
