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

#define _GNU_SOURCE 1
#include "va.h"
#include "va_backend.h"
#include "va_trace.h"
#include "va_fool.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

/*
 * Do dummy decode/encode, ignore the input data
 * In order to debug memory leak or low performance issues, we need to isolate driver problems
 * We export env "VA_FOOL", with which, we can do fake decode/encode:
 *
 * LIBVA_FOOL_DECODE:
 * . if set, decode does nothing, but fill in some YUV data
 * LIBVA_FOOL_ENCODE=<clip name>:
 * . if set, encode does nothing, but fill in the coded buffer from a H264 clip.
 * . VA CONTEXT/CONFIG/SURFACE will call into drivers, but VA Buffer creation is done in here
 * . Bypass all "vaBeginPic/vaRenderPic/vaEndPic"
 * LIBVA_FOOL_POSTP:
 * . if set, do nothing for vaPutSurface
 */


/* global settings */

/* LIBVA_FOOL_DECODE/LIBVA_FOOL_ENCODE/LIBVA_FOOL_POSTP */
int fool_decode = 0;
int fool_encode = 0;
int fool_postp  = 0;



#define NAL_BUF_SIZE  65536  // maximum NAL unit size
#define RING_BUF_SIZE  8192  // input ring buffer size, MUST be a power of two!
#define MAX_FRAME 16
#define SLICE_NUM 4

#define FOOL_CONTEXT_MAX 4
/* per context settings */
static struct _fool_context {
    VADisplay dpy; /* should use context as the key */

    VAProfile fool_profile; /* current profile for buffers */
    VAEntrypoint fool_entrypoint; /* current entrypoint */

    FILE *fool_fp_codedclip; /* load a clip from disk for fooling encode*/
    char *frame_buf;
    VACodedBufferSegment *codebuf;

    /* all buffers with same type share one malloc-ed memory
     * bufferID = (buffer numbers with the same type << 8) || type
     * the malloc-ed memory can be find by fool_buf[bufferID & 0xff]
     * the size is ignored here
     */
    char *fool_buf[VABufferTypeMax]; /* memory of fool buffers */
    unsigned int fool_buf_size[VABufferTypeMax]; /* size of memory of fool buffers */
    unsigned int fool_buf_count[VABufferTypeMax]; /* count of created buffers */
    VAContextID context;
} fool_context[FOOL_CONTEXT_MAX] = { {0} }; /* trace five context at the same time */

#define FOOL_DECODE(idx) (fool_decode && (fool_context[idx].fool_entrypoint == VAEntrypointVLD))
#define FOOL_ENCODE(idx)                                                \
    (fool_encode                                                        \
     && (fool_context[idx].fool_entrypoint == VAEntrypointEncSlice)     \
     && (fool_context[idx].fool_profile >= VAProfileH264Baseline)       \
     && (fool_context[idx].fool_profile <= VAProfileH264High))



#define DPY2INDEX(dpy)                                  \
    int idx;                                            \
                                                        \
    for (idx = 0; idx < FOOL_CONTEXT_MAX; idx++)        \
        if (fool_context[idx].dpy == dpy)               \
            break;                                      \
                                                        \
    if (idx == FOOL_CONTEXT_MAX)                        \
        return 0;  /* let driver go */


/* Prototype declarations (functions defined in va.c) */

void va_errorMessage(const char *msg, ...);
void va_infoMessage(const char *msg, ...);

int va_parseConfig(char *env, char *env_value);

VAStatus vaBufferInfo(
        VADisplay dpy,
        VAContextID context,	/* in */
        VABufferID buf_id,		/* in */
        VABufferType *type,		/* out */
        unsigned int *size,		/* out */
        unsigned int *num_elements	/* out */
);

VAStatus vaLockSurface(VADisplay dpy,
        VASurfaceID surface,
        unsigned int *fourcc, /* following are output argument */
        unsigned int *luma_stride,
        unsigned int *chroma_u_stride,
        unsigned int *chroma_v_stride,
        unsigned int *luma_offset,
        unsigned int *chroma_u_offset,
        unsigned int *chroma_v_offset,
        unsigned int *buffer_name,
        void **buffer
);

VAStatus vaUnlockSurface(VADisplay dpy,
        VASurfaceID surface
);


void va_FoolInit(VADisplay dpy)
{
    char env_value[1024];
    int fool_index = 0;

    for (fool_index = 0; fool_index < FOOL_CONTEXT_MAX; fool_index++)
        if (fool_context[fool_index].dpy == 0)
            break;

    if (fool_index == FOOL_CONTEXT_MAX)
        return;

    if (va_parseConfig("LIBVA_FOOL_POSTP", NULL) == 0) {
        fool_postp = 1;
        va_infoMessage("LIBVA_FOOL_POSTP is on, dummy vaPutSurface\n");
    }


    if (va_parseConfig("LIBVA_FOOL_DECODE", NULL) == 0) {
        fool_decode = 1;
        va_infoMessage("LIBVA_FOOL_DECODE is on, dummy decode\n");
    }


    if (va_parseConfig("LIBVA_FOOL_ENCODE", &env_value[0]) == 0) {
        fool_context[fool_index].fool_fp_codedclip = fopen(env_value, "r");

        if (fool_context[fool_index].fool_fp_codedclip) {
            fool_encode = 1;
        } else
            fool_encode = 0;

        if (fool_encode) /* malloc the buffer for fake clip */
        {
            fool_context[fool_index].frame_buf = malloc(MAX_FRAME*SLICE_NUM*NAL_BUF_SIZE*sizeof(char));
            fool_context[fool_index].codebuf = malloc(sizeof(VACodedBufferSegment));
        }

        if (fool_context[fool_index].frame_buf == NULL)
            fool_encode = 0;

        if (fool_encode)
            va_infoMessage("LIBVA_FOOL_ENCODE is on, dummy encode\n");

    }

    if (fool_encode || fool_decode)
        fool_context[fool_index].dpy = dpy;
}


int va_FoolEnd(VADisplay dpy)
{
    int i;

    DPY2INDEX(dpy);

    for (i = 0; i < VABufferTypeMax; i++) {/* free memory */
        if (fool_context[idx].fool_buf[i])
            free(fool_context[idx].fool_buf[i]);
    }
    if (fool_context[idx].fool_fp_codedclip)
        fclose(fool_context[idx].fool_fp_codedclip);

    if (fool_context[idx].frame_buf)
        free(fool_context[idx].frame_buf);
    
    if (fool_context[idx].codebuf)
        free(fool_context[idx].codebuf);

    memset(&fool_context[idx], sizeof(struct _fool_context), 0);
    return 0;
}

int va_FoolCodedBuf(VADisplay dpy)
{
    /* do nothing */
    return 0;
}


int va_FoolCreateConfig(
        VADisplay dpy,
        VAProfile profile, 
        VAEntrypoint entrypoint, 
        VAConfigAttrib *attrib_list,
        int num_attribs,
        VAConfigID *config_id /* out */
)
{
    DPY2INDEX(dpy);

    /* call into driver level to allocate real context/surface/buffers, etc */
    fool_context[idx].fool_profile = profile;
    fool_context[idx].fool_entrypoint = entrypoint;
    return 0;
}

static int yuvgen_planar(
        int width, int height,
        unsigned char *Y_start, int Y_pitch,
        unsigned char *U_start, int U_pitch,
        unsigned char *V_start, int V_pitch,
        int UV_interleave, int box_width, int row_shift,
        int field
)
{
    int row;

    /* copy Y plane */
    for (row=0;row<height;row++) {
        unsigned char *Y_row = Y_start + row * Y_pitch;
        int jj, xpos, ypos;

        ypos = (row / box_width) & 0x1;

        /* fill garbage data into the other field */
        if (((field == VA_TOP_FIELD) && (row &1))
                || ((field == VA_BOTTOM_FIELD) && ((row &1)==0))) {
            memset(Y_row, 0xff, width);
            continue;
        }

        for (jj=0; jj<width; jj++) {
            xpos = ((row_shift + jj) / box_width) & 0x1;

            if ((xpos == 0) && (ypos == 0))
                Y_row[jj] = 0xeb;
            if ((xpos == 1) && (ypos == 1))
                Y_row[jj] = 0xeb;

            if ((xpos == 1) && (ypos == 0))
                Y_row[jj] = 0x10;
            if ((xpos == 0) && (ypos == 1))
                Y_row[jj] = 0x10;
        }
    }

    /* copy UV data */
    for( row =0; row < height/2; row++) {
        unsigned short value = 0x80;

        /* fill garbage data into the other field */
        if (((field == VA_TOP_FIELD) && (row &1))
                || ((field == VA_BOTTOM_FIELD) && ((row &1)==0))) {
            value = 0xff;
        }

        if (UV_interleave) {
            unsigned short *UV_row = (unsigned short *)(U_start + row * U_pitch);

            memset(UV_row, value, width);
        } else {
            unsigned char *U_row = U_start + row * U_pitch;
            unsigned char *V_row = V_start + row * V_pitch;

            memset (U_row,value,width/2);
            memset (V_row,value,width/2);
        }
    }

    return 0;
}


int va_FoolCreateSurfaces(
        VADisplay dpy,
        int width,
        int height,
        int format,
        int num_surfaces,
        VASurfaceID *surfaces	/* out */
)
{
    int i;
    unsigned int fourcc; /* following are output argument */
    unsigned int luma_stride;
    unsigned int chroma_u_stride;
    unsigned int chroma_v_stride;
    unsigned int luma_offset;
    unsigned int chroma_u_offset;
    unsigned int chroma_v_offset;
    unsigned int buffer_name;
    void *buffer = NULL;
    unsigned char *Y_data, *U_data, *V_data;

    int box_width = num_surfaces/2;
    int row_shift = 0;
    VAStatus va_status;

    DPY2INDEX(dpy);

    if (FOOL_DECODE(idx)) { 
        /* call into driver level to allocate real context/surface/buffers, etc
         * fill in the YUV data, will be overwrite if it is encode context
         */
        for (i = 0; i < num_surfaces; i++) {
            /* fool decoder: fill with auto-generated YUV data */
            va_status = vaLockSurface(dpy, surfaces[i], &fourcc,
                    &luma_stride, &chroma_u_stride, &chroma_v_stride,
                    &luma_offset, &chroma_u_offset, &chroma_v_offset,
                    &buffer_name, &buffer);

            if (va_status != VA_STATUS_SUCCESS)
                return 0;

            if (!buffer) {
                vaUnlockSurface(dpy, surfaces[i]);
                return 0;
            }

            Y_data = buffer;

            /* UV should be same for NV12 */
            U_data = buffer + chroma_u_offset;
            V_data = buffer + chroma_v_offset;

            yuvgen_planar(width, height,
                    Y_data, luma_stride,
                    U_data, chroma_v_stride,
                    V_data, chroma_v_stride,
                    (fourcc==VA_FOURCC_NV12),
                    box_width, row_shift, 0);

            vaUnlockSurface(dpy, surfaces[i]);

            row_shift++;
            if (row_shift==(2*box_width))
                row_shift= 0;
        }
        return 0; /* the return value is ignored */
    }
    return 0; /* the return value is ignored */
}

VAStatus va_FoolCreateBuffer (
        VADisplay dpy,
        VAContextID context,	/* in */
        VABufferType type,		/* in */
        unsigned int size,		/* in */
        unsigned int num_elements,	/* in */
        void *data,			/* in */
        VABufferID *buf_id		/* out */
)
{
    DPY2INDEX(dpy);

    if (FOOL_ENCODE(idx) || FOOL_DECODE(idx)) { /* fool buffer creation */
        int new_size = size * num_elements;

        if (type == VAEncCodedBufferType) /* only a VACodedBufferSegment */
            new_size = sizeof(VACodedBufferSegment);

        if (fool_context[idx].fool_buf_size[type] == 0)
            fool_context[idx].fool_buf[type] = calloc(1, new_size);
        else if (fool_context[idx].fool_buf_size[type] <= new_size)
            fool_context[idx].fool_buf[type] = realloc(fool_context[idx].fool_buf, new_size);

        if (fool_context[idx].fool_buf[type] == NULL) {
            va_FoolEnd(dpy);
            return 0; /* let driver go */
        }

        /* because we ignore the vaRenderPicture, 
         * all buffers with same type share same real memory
         * bufferID = (buffer count << 8) | type
         */
        fool_context[idx].fool_buf_count[type]++;
        *buf_id = (fool_context[idx].fool_buf_count[type] << 8) | type;

        return 1; /* don't call into driver */
    }

    return 0; /* let driver go ... */
}

VAStatus va_FoolMapBuffer (
        VADisplay dpy,
        VABufferID buf_id,	/* in */
        void **pbuf 	/* out */
)
{
    VABufferType type;
    unsigned int size,frame_size = 0;
    unsigned int num_elements;
    DPY2INDEX(dpy);

    if (FOOL_ENCODE(idx) || FOOL_DECODE(idx)) { /* fool buffer creation */
        unsigned int buf_idx = buf_id & 0xff;

        /* Image buffer? */
        vaBufferInfo(dpy, fool_context[idx].context, buf_id, &type, &size, &num_elements);
        if (type == VAImageBufferType  && FOOL_ENCODE(idx))
            return 0;

        /* buf_id is the buffer type */
        if (fool_context[idx].fool_buf[buf_idx] != NULL)
            *pbuf = fool_context[idx].fool_buf[buf_idx];
        else
            *pbuf = NULL;

        /* expect APP to MapBuffer when get the the coded data */
        if (*pbuf && (buf_idx == VAEncCodedBufferType)) { /* it is coded buffer */
            /* read from a clip */
            frame_size = va_FoolGetFrame(fool_context[idx].fool_fp_codedclip,
                    fool_context[idx].frame_buf);

            memset(fool_context[idx].codebuf,0,sizeof(VACodedBufferSegment));
            fool_context[idx].codebuf->size = frame_size;
            fool_context[idx].codebuf->bit_offset = 0;
            fool_context[idx].codebuf->status = 0;
            fool_context[idx].codebuf->reserved = 0;
            fool_context[idx].codebuf->buf = fool_context[idx].frame_buf;
            fool_context[idx].codebuf->next = NULL;
            *pbuf = fool_context[idx].codebuf;
        }
        return 1; /* don't call into driver */
    }

    return 0; /* let driver go ... */
}


int va_FoolBeginPicture(
        VADisplay dpy,
        VAContextID context,
        VASurfaceID render_target
)
{
    DPY2INDEX(dpy);

    if (FOOL_ENCODE(idx) || FOOL_DECODE(idx)) {
        if (fool_context[idx].context == 0)
            fool_context[idx].context = context;
        return 1; /* don't call into driver level */
    }

    return 0; /* let driver go ... */
}

int va_FoolRenderPicture(
        VADisplay dpy,
        VAContextID context,
        VABufferID *buffers,
        int num_buffers
)
{
    DPY2INDEX(dpy);

    if (FOOL_ENCODE(idx) || FOOL_DECODE(idx))
        return 1; /* don't call into driver level */

    return 0;  /* let driver go ... */
}


int va_FoolEndPicture(
        VADisplay dpy,
        VAContextID context
)
{
    DPY2INDEX(dpy);

    /* don't call into driver level */

    /* do real fooling operation here */

    /* only support H264 encoding currently */
    if (FOOL_ENCODE(idx)) {
        /* expect vaMapBuffer will handle it
         * or else, need to save the codedbuf ID,
         * and fool encode it here
         */
        /* va_FoolCodedBuf(dpy); */
        return 1; /* don't call into driver level */
    }

    if (FOOL_DECODE(idx))
        return 1;  /* don't call into driver level */

    return 0; /* let driver go ... */
}

int va_FoolSyncSurface(
        VADisplay dpy, 
        VASurfaceID render_target
)
{
    DPY2INDEX(dpy);

    /*Fill in black and white squares. */
    if (FOOL_DECODE(idx) || FOOL_DECODE(idx))
        return 1;

    return 0;

}

VAStatus va_FoolUnmapBuffer(
        VADisplay dpy,
        VABufferID buf_id	/* in */
)
{
    DPY2INDEX(dpy);

    if (FOOL_ENCODE(idx) || FOOL_DECODE(idx))
        return 1; /* fool buffer creation */

    return 0;
}

VAStatus va_FoolQuerySubpictureFormats(
        VADisplay dpy,
        VAImageFormat *format_list,
        unsigned int *flags,
        unsigned int *num_formats
)
{
    DPY2INDEX(dpy);

    if (FOOL_ENCODE(idx) || FOOL_DECODE(idx)) { 
        if (num_formats)
            *num_formats = 0;
        return 1;
    }
    return 0;
}

