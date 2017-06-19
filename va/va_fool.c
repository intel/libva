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
#include "sysdeps.h"
#include "va.h"
#include "va_backend.h"
#include "va_internal.h"
#include "va_trace.h"
#include "va_fool.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

/*
 * Do dummy decode/encode, ignore the input data
 * In order to debug memory leak or low performance issues, we need to isolate driver problems
 * We export env "VA_FOOL", with which, we can do fake decode/encode:
 *
 * LIBVA_FOOL_DECODE:
 * . if set, decode does nothing
 * LIBVA_FOOL_ENCODE=<framename>:
 * . if set, encode does nothing, but fill in the coded buffer from the content of files with
 *   name framename.0,framename.1,..., framename.N, framename.0,..., framename.N,...repeatly
 *   Use file name to determine h264 or vp8
 * LIBVA_FOOL_JPEG=<framename>:fill the content of filename to codedbuf for jpeg encoding
 * LIBVA_FOOL_POSTP:
 * . if set, do nothing for vaPutSurface
 */


/* global settings */
int va_fool_codec = 0;
int va_fool_postp  = 0;

#define FOOL_BUFID_MAGIC   0x12345600
#define FOOL_BUFID_MASK    0xffffff00

struct fool_context {
    int enabled; /* va_fool_codec is global, and it is for concurent encode/decode */
    char *fn_enc;/* file pattern with codedbuf content for encode */
    char *segbuf_enc; /* the segment buffer of coded buffer, load frome fn_enc */
    int file_count;

    char *fn_jpg;/* file name of JPEG fool with codedbuf content */
    char *segbuf_jpg; /* the segment buffer of coded buffer, load frome fn_jpg */

    VAEntrypoint entrypoint; /* current entrypoint */
    
    /* all buffers with same type share one malloc-ed memory
     * bufferID = (buffer numbers with the same type << 8) || type
     * the malloc-ed memory can be find by fool_buf[bufferID & 0xff]
     * the size is ignored here
     */
    char *fool_buf[VABufferTypeMax]; /* memory of fool buffers */
    unsigned int fool_buf_size[VABufferTypeMax]; /* size of memory of fool buffers */
    unsigned int fool_buf_element[VABufferTypeMax]; /* element count of created buffers */
    unsigned int fool_buf_count[VABufferTypeMax]; /* count of created buffers */
    VAContextID context;
};

#define FOOL_CTX(dpy) ((struct fool_context *)((VADisplayContextP)dpy)->vafool)

#define DPY2FOOLCTX(dpy)                                 \
    struct fool_context *fool_ctx = FOOL_CTX(dpy);       \
    if (fool_ctx == NULL)                                \
        return 0; /* no fool for the context */          \

#define DPY2FOOLCTX_CHK(dpy)                             \
    struct fool_context *fool_ctx = FOOL_CTX(dpy);       \
    if ((fool_ctx == NULL) || (fool_ctx->enabled == 0))  \
        return 0; /* no fool for the context */          \


void va_FoolInit(VADisplay dpy)
{
    char env_value[1024];

    struct fool_context *fool_ctx = calloc(sizeof(struct fool_context), 1);
    
    if (fool_ctx == NULL)
        return;
    
    if (va_parseConfig("LIBVA_FOOL_POSTP", NULL) == 0) {
        va_fool_postp = 1;
        va_infoMessage(dpy, "LIBVA_FOOL_POSTP is on, dummy vaPutSurface\n");
    }
    
    if (va_parseConfig("LIBVA_FOOL_DECODE", NULL) == 0) {
        va_fool_codec  |= VA_FOOL_FLAG_DECODE;
        va_infoMessage(dpy, "LIBVA_FOOL_DECODE is on, dummy decode\n");
    }
    if (va_parseConfig("LIBVA_FOOL_ENCODE", &env_value[0]) == 0) {
        va_fool_codec  |= VA_FOOL_FLAG_ENCODE;
        fool_ctx->fn_enc = strdup(env_value);
        va_infoMessage(dpy, "LIBVA_FOOL_ENCODE is on, load encode data from file with patten %s\n",
                       fool_ctx->fn_enc);
    }
    if (va_parseConfig("LIBVA_FOOL_JPEG", &env_value[0]) == 0) {
        va_fool_codec  |= VA_FOOL_FLAG_JPEG;
        fool_ctx->fn_jpg = strdup(env_value);
        va_infoMessage(dpy, "LIBVA_FOOL_JPEG is on, load encode data from file with patten %s\n",
                       fool_ctx->fn_jpg);
    }
    
    ((VADisplayContextP)dpy)->vafool = fool_ctx;
}


int va_FoolEnd(VADisplay dpy)
{
    int i;
    DPY2FOOLCTX(dpy);

    for (i = 0; i < VABufferTypeMax; i++) {/* free memory */
        if (fool_ctx->fool_buf[i])
            free(fool_ctx->fool_buf[i]);
    }
    if (fool_ctx->segbuf_enc)
        free(fool_ctx->segbuf_enc);
    if (fool_ctx->segbuf_jpg)
        free(fool_ctx->segbuf_jpg);
    if (fool_ctx->fn_enc)
        free(fool_ctx->fn_enc);
    if (fool_ctx->fn_jpg)
        free(fool_ctx->fn_jpg);

    free(fool_ctx);
    ((VADisplayContextP)dpy)->vafool = NULL;
    
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
    DPY2FOOLCTX(dpy);

    fool_ctx->entrypoint = entrypoint;
    
    /*
     * check va_fool_codec to align with current context
     * e.g. va_fool_codec = decode then for encode, the
     * vaBegin/vaRender/vaEnd also run into fool path
     * which is not desired
     */
    if (((va_fool_codec & VA_FOOL_FLAG_DECODE) && (entrypoint == VAEntrypointVLD)) ||
        ((va_fool_codec & VA_FOOL_FLAG_JPEG) && (entrypoint == VAEntrypointEncPicture)))
        fool_ctx->enabled = 1;
    else if ((va_fool_codec & VA_FOOL_FLAG_ENCODE) && (entrypoint == VAEntrypointEncSlice)) {
        /* H264 is desired */
        if (((profile == VAProfileH264Baseline ||
              profile == VAProfileH264Main ||
              profile == VAProfileH264High ||
              profile == VAProfileH264ConstrainedBaseline)) &&
            strstr(fool_ctx->fn_enc, "h264"))
            fool_ctx->enabled = 1;

        /* vp8 is desired */
        if ((profile == VAProfileVP8Version0_3) &&
            strstr(fool_ctx->fn_enc, "vp8"))
            fool_ctx->enabled = 1;
    }
    if (fool_ctx->enabled)
        va_infoMessage(dpy, "FOOL is enabled for this context\n");
    else
        va_infoMessage(dpy, "FOOL is not enabled for this context\n");

    
    return 0; /* continue */
}


VAStatus va_FoolCreateBuffer(
    VADisplay dpy,
    VAContextID context,	/* in */
    VABufferType type,		/* in */
    unsigned int size,		/* in */
    unsigned int num_elements,	/* in */
    void *data,			/* in */
    VABufferID *buf_id		/* out */
)
{
    unsigned int new_size = size * num_elements;
    unsigned int old_size;
    DPY2FOOLCTX_CHK(dpy);

    old_size = fool_ctx->fool_buf_size[type] * fool_ctx->fool_buf_element[type];

    if (old_size < new_size)
        fool_ctx->fool_buf[type] = realloc(fool_ctx->fool_buf[type], new_size);
    
    fool_ctx->fool_buf_size[type] = size;
    fool_ctx->fool_buf_element[type] = num_elements;
    fool_ctx->fool_buf_count[type]++;
    /* because we ignore the vaRenderPicture, 
     * all buffers with same type share same real memory
     * bufferID = (magic number) | type
     */
    *buf_id = FOOL_BUFID_MAGIC | type;

    return 1; /* don't call into driver */
}

VAStatus va_FoolBufferInfo(
    VADisplay dpy,
    VABufferID buf_id,  /* in */
    VABufferType *type, /* out */
    unsigned int *size,         /* out */
    unsigned int *num_elements /* out */
)
{
    unsigned int magic;
    
    DPY2FOOLCTX_CHK(dpy);

    magic = buf_id & FOOL_BUFID_MASK;
    if (magic != FOOL_BUFID_MAGIC)
        return 0; /* could be VAImageBufferType from vaDeriveImage */
    
    *type = buf_id & 0xff;
    *size = fool_ctx->fool_buf_size[*type];
    *num_elements = fool_ctx->fool_buf_element[*type];;
    
    return 1; /* fool is valid */
}

static int va_FoolFillCodedBufEnc(struct fool_context *fool_ctx)
{
    char file_name[1024];
    struct stat file_stat = {0};
    VACodedBufferSegment *codedbuf;
    int i, fd = -1;
    ssize_t ret;

    /* try file_name.file_count, if fail, try file_name.file_count-- */
    for (i=0; i<=1; i++) {
        snprintf(file_name, 1024, "%s.%d",
                 fool_ctx->fn_enc,
                 fool_ctx->file_count);

        if ((fd = open(file_name, O_RDONLY)) != -1) {
            fstat(fd, &file_stat);
            fool_ctx->file_count++; /* open next file */
            break;
        } else /* fall back to the first file file */
            fool_ctx->file_count = 0;
    }
    if (fd != -1) {
        fool_ctx->segbuf_enc = realloc(fool_ctx->segbuf_enc, file_stat.st_size);
        ret = read(fd, fool_ctx->segbuf_enc, file_stat.st_size);
        if (ret < file_stat.st_size)
            va_errorMessage("Reading file %s failed.\n", file_name);
        close(fd);
    } else
        va_errorMessage("Open file %s failed:%s\n", file_name, strerror(errno));

    codedbuf = (VACodedBufferSegment *)fool_ctx->fool_buf[VAEncCodedBufferType];
    codedbuf->size = file_stat.st_size;
    codedbuf->bit_offset = 0;
    codedbuf->status = 0;
    codedbuf->reserved = 0;
    codedbuf->buf = fool_ctx->segbuf_enc;
    codedbuf->next = NULL;

    return 0;
}

static int va_FoolFillCodedBufJPG(struct fool_context *fool_ctx)
{
    struct stat file_stat = {0};
    VACodedBufferSegment *codedbuf;
    int fd = -1;
    ssize_t ret;

    if ((fd = open(fool_ctx->fn_jpg, O_RDONLY)) != -1) {
        fstat(fd, &file_stat);
        fool_ctx->segbuf_jpg = realloc(fool_ctx->segbuf_jpg, file_stat.st_size);
        ret = read(fd, fool_ctx->segbuf_jpg, file_stat.st_size);
        if (ret < file_stat.st_size)
            va_errorMessage("Reading file %s failed.\n", fool_ctx->fn_jpg);
        close(fd);
    } else
        va_errorMessage("Open file %s failed:%s\n", fool_ctx->fn_jpg, strerror(errno));

    codedbuf = (VACodedBufferSegment *)fool_ctx->fool_buf[VAEncCodedBufferType];
    codedbuf->size = file_stat.st_size;
    codedbuf->bit_offset = 0;
    codedbuf->status = 0;
    codedbuf->reserved = 0;
    codedbuf->buf = fool_ctx->segbuf_jpg;
    codedbuf->next = NULL;

    return 0;
}


static int va_FoolFillCodedBuf(struct fool_context *fool_ctx)
{
    if (fool_ctx->entrypoint == VAEntrypointEncSlice)
        va_FoolFillCodedBufEnc(fool_ctx);
    else if (fool_ctx->entrypoint == VAEntrypointEncPicture)
        va_FoolFillCodedBufJPG(fool_ctx);
        
    return 0;
}


VAStatus va_FoolMapBuffer(
    VADisplay dpy,
    VABufferID buf_id,	/* in */
    void **pbuf 	/* out */
)
{
    unsigned int magic, buftype;
    DPY2FOOLCTX_CHK(dpy);

    magic = buf_id & FOOL_BUFID_MASK;
    if (magic != FOOL_BUFID_MAGIC)
        return 0; /* could be VAImageBufferType from vaDeriveImage */
    
    buftype = buf_id & 0xff;
    *pbuf = fool_ctx->fool_buf[buftype];

    /* it is coded buffer, fill coded segment from file */
    if (*pbuf && (buftype == VAEncCodedBufferType))
        va_FoolFillCodedBuf(fool_ctx);
    
    return 1; /* fool is valid */
}

VAStatus va_FoolCheckContinuity(VADisplay dpy)
{
    DPY2FOOLCTX_CHK(dpy);

    return 1; /* fool is valid */
}

