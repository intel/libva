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

#ifdef __cplusplus
extern "C" {
#endif

extern int fool_codec;
extern int fool_postp;

#define VA_FOOL_FLAG_DECODE  0x1
#define VA_FOOL_FLAG_ENCODE  0x2
#define VA_FOOL_FLAG_JPEG    0x4

#define VA_FOOL_FUNC(fool_func,...)            \
    if (fool_codec) {                          \
        ret = fool_func(__VA_ARGS__);          \
    }
#define VA_FOOL_RETURN()                       \
    if (fool_codec) {                          \
        return VA_STATUS_SUCCESS;              \
    }

void va_FoolInit(VADisplay dpy);
int va_FoolEnd(VADisplay dpy);

int va_FoolCreateConfig(
        VADisplay dpy,
        VAProfile profile, 
        VAEntrypoint entrypoint, 
        VAConfigAttrib *attrib_list,
        int num_attribs,
        VAConfigID *config_id /* out */
);


VAStatus va_FoolCreateBuffer(
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

VAStatus va_FoolUnmapBuffer(
        VADisplay dpy,
        VABufferID buf_id	/* in */
);

VAStatus va_FoolBufferInfo (
    VADisplay dpy,
    VABufferID buf_id,  /* in */
    VABufferType *type, /* out */
    unsigned int *size,         /* out */
    unsigned int *num_elements /* out */
);
    
    
#ifdef __cplusplus
}
#endif

#endif
