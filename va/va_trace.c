/*
 * Copyright (c) 2009-2011 Intel Corporation. All Rights Reserved.
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
#include <errno.h>

/*
 * Env. to debug some issue, e.g. the decode/encode issue in a video conference scenerio:
 * .LIBVA_TRACE=log_file: general VA parameters saved into log_file
 * .LIBVA_TRACE_BUFDATA: dump VA buffer data into log_file (if not set, just calculate a checksum)
 * .LIBVA_TRACE_CODEDBUF=coded_clip_file: save the coded clip into file coded_clip_file
 * .LIBVA_TRACE_SURFACE=yuv_file: save surface YUV into file yuv_file. Use file name to determine
 *                                decode/encode or jpeg surfaces
 * .LIBVA_TRACE_LOGSIZE=numeric number: truncate the log_file or coded_clip_file, or decoded_yuv_file
 *                                      when the size is bigger than the number
 */

/* global settings */

/* LIBVA_TRACE */
int trace_flag = 0;

/* LIBVA_TRACE_LOGSIZE */
static unsigned int trace_logsize = 0xffffffff; /* truncate the log when the size is bigger than it */

#define TRACE_CONTEXT_MAX 4
/* per context settings */
static struct _trace_context {
    VADisplay dpy; /* should use context as the key */
    
    /* LIBVA_TRACE */
    FILE *trace_fp_log; /* save the log into a file */
    char *trace_log_fn; /* file name */
    
    /* LIBVA_TRACE_CODEDBUF */
    FILE *trace_fp_codedbuf; /* save the encode result into a file */
    char *trace_codedbuf_fn; /* file name */
    
    /* LIBVA_TRACE_SURFACE */
    FILE *trace_fp_surface; /* save the surface YUV into a file */
    char *trace_surface_fn; /* file name */

    VAContextID  trace_context; /* current context */
    
    VASurfaceID  trace_rendertarget; /* current render target */
    VAProfile trace_profile; /* current profile for buffers */
    VAEntrypoint trace_entrypoint; /* current entrypoint */
    VABufferID trace_codedbuf;
    
    unsigned int trace_frame_no; /* current frame NO */
    unsigned int trace_slice_no; /* current slice NO */
    unsigned int trace_slice_size; /* current slice buffer size */

    unsigned int trace_frame_width; /* current frame width */
    unsigned int trace_frame_height; /* current frame height */
    unsigned int trace_sequence_start; /* get a new sequence for encoding or not */
} trace_context[TRACE_CONTEXT_MAX]; /* trace five context at the same time */

#define DPY2INDEX(dpy)                                  \
    int idx;                                            \
                                                        \
    for (idx = 0; idx < TRACE_CONTEXT_MAX; idx++)       \
        if (trace_context[idx].dpy == dpy)              \
            break;                                      \
                                                        \
    if (idx == TRACE_CONTEXT_MAX)                       \
        return;

#define TRACE_FUNCNAME(idx)    va_TraceMsg(idx, "==========%s\n", __func__); 

/* Prototype declarations (functions defined in va.c) */

void va_errorMessage(const char *msg, ...);
void va_infoMessage(const char *msg, ...);

int va_parseConfig(char *env, char *env_value);

VAStatus vaBufferInfo(
    VADisplay dpy,
    VAContextID context,        /* in */
    VABufferID buf_id,          /* in */
    VABufferType *type,         /* out */
    unsigned int *size,         /* out */
    unsigned int *num_elements  /* out */
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

#define FILE_NAME_SUFFIX(env_value)                      \
do {                                                    \
    int tmp = strnlen(env_value, sizeof(env_value));    \
    int left = sizeof(env_value) - tmp;                 \
                                                        \
    snprintf(env_value+tmp,                             \
             left,                                      \
             ".%04d.%05d",                              \
             trace_index,                               \
             suffix);                                   \
} while (0)

void va_TraceInit(VADisplay dpy)
{
    char env_value[1024];
    unsigned short suffix = 0xffff & ((unsigned int)time(NULL));
    int trace_index = 0;
    FILE *tmp;    
    
    for (trace_index = 0; trace_index < TRACE_CONTEXT_MAX; trace_index++)
        if (trace_context[trace_index].dpy == 0)
            break;

    if (trace_index == TRACE_CONTEXT_MAX)
        return;

    memset(&trace_context[trace_index], 0, sizeof(struct _trace_context));
    if (va_parseConfig("LIBVA_TRACE", &env_value[0]) == 0) {
        FILE_NAME_SUFFIX(env_value);
        trace_context[trace_index].trace_log_fn = strdup(env_value);
        
        tmp = fopen(env_value, "w");
        if (tmp) {
            trace_context[trace_index].trace_fp_log = tmp;
            va_infoMessage("LIBVA_TRACE is on, save log into %s\n", trace_context[trace_index].trace_log_fn);
            trace_flag = VA_TRACE_FLAG_LOG;
        } else
            va_errorMessage("Open file %s failed (%s)\n", env_value, strerror(errno));
    }

    /* may re-get the global settings for multiple context */
    if (va_parseConfig("LIBVA_TRACE_LOGSIZE", &env_value[0]) == 0) {
        trace_logsize = atoi(env_value);
        va_infoMessage("LIBVA_TRACE_LOGSIZE is on, size is %d\n", trace_logsize);
    }

    if ((trace_flag & VA_TRACE_FLAG_LOG) && (va_parseConfig("LIBVA_TRACE_BUFDATA", NULL) == 0)) {
        trace_flag |= VA_TRACE_FLAG_BUFDATA;
        va_infoMessage("LIBVA_TRACE_BUFDATA is on, dump buffer into log file\n");
    }

    /* per-context setting */
    if (va_parseConfig("LIBVA_TRACE_CODEDBUF", &env_value[0]) == 0) {
        FILE_NAME_SUFFIX(env_value);
        trace_context[trace_index].trace_codedbuf_fn = strdup(env_value);
        va_infoMessage("LIBVA_TRACE_CODEDBUF is on, save codedbuf into log file %s\n",
                       trace_context[trace_index].trace_codedbuf_fn);
        trace_flag |= VA_TRACE_FLAG_CODEDBUF;
    }

    if (va_parseConfig("LIBVA_TRACE_SURFACE", &env_value[0]) == 0) {
        FILE_NAME_SUFFIX(env_value);
        trace_context[trace_index].trace_surface_fn = strdup(env_value);
        
        va_infoMessage("LIBVA_TRACE_SURFACE is on, save surface into %s\n",
                       trace_context[trace_index].trace_surface_fn);

        /* for surface data dump, it is time-consume, and may
         * cause some side-effect, so only trace the needed surfaces
         * to trace encode surface, set the trace file name to sth like *enc*
         * to trace decode surface, set the trace file name to sth like *dec*
         * if no dec/enc in file name, set both
         */
        if (strstr(env_value, "dec"))
            trace_flag |= VA_TRACE_FLAG_SURFACE_DECODE;
        if (strstr(env_value, "enc"))
            trace_flag |= VA_TRACE_FLAG_SURFACE_ENCODE;
        if (strstr(env_value, "jpeg") || strstr(env_value, "jpg"))
            trace_flag |= VA_TRACE_FLAG_SURFACE_JPEG;
    }

    trace_context[trace_index].dpy = dpy;
}


void va_TraceEnd(VADisplay dpy)
{
    DPY2INDEX(dpy);
    
    if (trace_context[idx].trace_fp_log)
        fclose(trace_context[idx].trace_fp_log);
    
    if (trace_context[idx].trace_fp_codedbuf)
        fclose(trace_context[idx].trace_fp_codedbuf);
    
    if (trace_context[idx].trace_fp_surface)
        fclose(trace_context[idx].trace_fp_surface);

    if (trace_context[idx].trace_log_fn)
        free(trace_context[idx].trace_log_fn);
    
    if (trace_context[idx].trace_codedbuf_fn)
        free(trace_context[idx].trace_codedbuf_fn);
    
    if (trace_context[idx].trace_surface_fn)
        free(trace_context[idx].trace_surface_fn);
    
    memset(&trace_context[idx], 0, sizeof(struct _trace_context));
}


static unsigned int file_size(FILE *fp)
{
    struct stat buf;

    fstat(fileno(fp), &buf);

    return buf.st_size;
}


static void truncate_file(FILE *fp)
{
    ftruncate(fileno(fp), 0);
    rewind(fp);
}

void va_TraceMsg(int idx, const char *msg, ...)
{
    va_list args;

    if (!(trace_flag & VA_TRACE_FLAG_LOG))
        return;

    if (file_size(trace_context[idx].trace_fp_log) >= trace_logsize)
        truncate_file(trace_context[idx].trace_fp_log);
    if (msg)  {
        va_start(args, msg);
        vfprintf(trace_context[idx].trace_fp_log, msg, args);
        va_end(args);
    } else
        fflush(trace_context[idx].trace_fp_log);
}

void va_TraceCodedBuf(VADisplay dpy)
{
    VACodedBufferSegment *buf_list = NULL;
    VAStatus va_status;
    unsigned char check_sum = 0;
    DPY2INDEX(dpy);
    
    /* can only truncate at a sequence boudary */
    if (((file_size(trace_context[idx].trace_fp_log) >= trace_logsize))
        && trace_context[idx].trace_sequence_start) {
        va_TraceMsg(idx, "==========truncate file %s\n", trace_context[idx].trace_codedbuf_fn);
        truncate_file(trace_context[idx].trace_fp_log);
    }
    

    trace_context[idx].trace_sequence_start = 0; /* only truncate coded file when meet next new sequence */
    
    va_status = vaMapBuffer(dpy, trace_context[idx].trace_codedbuf, (void **)(&buf_list));
    if (va_status != VA_STATUS_SUCCESS)
        return;

    va_TraceMsg(idx, "==========dump codedbuf into file %s\n", trace_context[idx].trace_codedbuf_fn);
    
    while (buf_list != NULL) {
        unsigned int i;
        
        va_TraceMsg(idx, "\tsize = %d\n", buf_list->size);
        if (trace_context[idx].trace_fp_log)
            fwrite(buf_list->buf, buf_list->size, 1, trace_context[idx].trace_fp_codedbuf);

        for (i=0; i<buf_list->size; i++)
            check_sum ^= *((unsigned char *)buf_list->buf + i);

        buf_list = buf_list->next;
    }
    vaUnmapBuffer(dpy,trace_context[idx].trace_codedbuf);
    
    va_TraceMsg(idx, "\tchecksum = 0x%02x\n", check_sum);
    va_TraceMsg(idx, NULL);
}


void va_TraceSurface(VADisplay dpy)
{
    unsigned int i, j;
    unsigned int fourcc; /* following are output argument */
    unsigned int luma_stride;
    unsigned int chroma_u_stride;
    unsigned int chroma_v_stride;
    unsigned int luma_offset;
    unsigned int chroma_u_offset;
    unsigned int chroma_v_offset;
    unsigned int buffer_name;
    void *buffer = NULL;
    unsigned char *Y_data, *UV_data, *tmp;
    VAStatus va_status;
    unsigned char check_sum = 0;
    DPY2INDEX(dpy);

    va_TraceMsg(idx, "==========dump surface data in file %s\n", trace_context[idx].trace_surface_fn);

    if ((file_size(trace_context[idx].trace_fp_surface) >= trace_logsize)) {
        va_TraceMsg(idx, "==========truncate file %s\n", trace_context[idx].trace_surface_fn);
        truncate_file(trace_context[idx].trace_fp_surface);
    }
    va_TraceMsg(idx, NULL);

    va_status = vaLockSurface(
        dpy,
        trace_context[idx].trace_rendertarget,
        &fourcc,
        &luma_stride, &chroma_u_stride, &chroma_v_stride,
        &luma_offset, &chroma_u_offset, &chroma_v_offset,
        &buffer_name, &buffer);

    if (va_status != VA_STATUS_SUCCESS) {
        va_TraceMsg(idx, "Error:vaLockSurface failed\n");
        return;
    }

    va_TraceMsg(idx, "\tfourcc = 0x%08x\n", fourcc);
    va_TraceMsg(idx, "\twidth = %d\n", trace_context[idx].trace_frame_width);
    va_TraceMsg(idx, "\theight = %d\n", trace_context[idx].trace_frame_height);
    va_TraceMsg(idx, "\tluma_stride = %d\n", luma_stride);
    va_TraceMsg(idx, "\tchroma_u_stride = %d\n", chroma_u_stride);
    va_TraceMsg(idx, "\tchroma_v_stride = %d\n", chroma_v_stride);
    va_TraceMsg(idx, "\tluma_offset = %d\n", luma_offset);
    va_TraceMsg(idx, "\tchroma_u_offset = %d\n", chroma_u_offset);
    va_TraceMsg(idx, "\tchroma_v_offset = %d\n", chroma_v_offset);

    if (buffer == NULL) {
        va_TraceMsg(idx, "Error:vaLockSurface return NULL buffer\n");
        va_TraceMsg(idx, NULL);

        vaUnlockSurface(dpy, trace_context[idx].trace_rendertarget);
        return;
    }
    va_TraceMsg(idx, "\tbuffer location = 0x%08x\n", buffer);
    va_TraceMsg(idx, NULL);

    Y_data = (unsigned char*)buffer;
    UV_data = (unsigned char*)buffer + chroma_u_offset;

    tmp = Y_data;
    for (i=0; i<trace_context[idx].trace_frame_height; i++) {
        if (trace_context[idx].trace_fp_surface)
            fwrite(tmp, trace_context[idx].trace_frame_width, 1, trace_context[idx].trace_fp_surface);
        
        tmp = Y_data + i * luma_stride;
    }
    tmp = UV_data;
    if (fourcc == VA_FOURCC_NV12) {
        for (i=0; i<trace_context[idx].trace_frame_height/2; i++) {
            if (trace_context[idx].trace_fp_surface)
                fwrite(tmp, trace_context[idx].trace_frame_width, 1, trace_context[idx].trace_fp_surface);
            
            tmp = UV_data + i * chroma_u_stride;
        }
    }

    vaUnlockSurface(dpy, trace_context[idx].trace_rendertarget);

    va_TraceMsg(idx, NULL);
}


void va_TraceInitialize (
    VADisplay dpy,
    int *major_version,     /* out */
    int *minor_version      /* out */
)
{
    DPY2INDEX(dpy);    
    TRACE_FUNCNAME(idx);
}

void va_TraceTerminate (
    VADisplay dpy
)
{
    DPY2INDEX(dpy);    
    TRACE_FUNCNAME(idx);
}


void va_TraceCreateConfig(
    VADisplay dpy,
    VAProfile profile, 
    VAEntrypoint entrypoint, 
    VAConfigAttrib *attrib_list,
    int num_attribs,
    VAConfigID *config_id /* out */
)
{
    int i;
    int encode, decode, jpeg;
    DPY2INDEX(dpy);

    TRACE_FUNCNAME(idx);
    
    va_TraceMsg(idx, "\tprofile = %d\n", profile);
    va_TraceMsg(idx, "\tentrypoint = %d\n", entrypoint);
    va_TraceMsg(idx, "\tnum_attribs = %d\n", num_attribs);
    for (i = 0; i < num_attribs; i++) {
        va_TraceMsg(idx, "\t\tattrib_list[%d].type = 0x%08x\n", i, attrib_list[i].type);
        va_TraceMsg(idx, "\t\tattrib_list[%d].value = 0x%08x\n", i, attrib_list[i].value);
    }
    va_TraceMsg(idx, NULL);

    trace_context[idx].trace_profile = profile;
    trace_context[idx].trace_entrypoint = entrypoint;

    /* avoid to create so many empty files */
    encode = (trace_context[idx].trace_entrypoint == VAEntrypointEncSlice);
    decode = (trace_context[idx].trace_entrypoint == VAEntrypointVLD);
    jpeg = (trace_context[idx].trace_entrypoint == VAEntrypointEncPicture);
    if ((encode && (trace_flag & VA_TRACE_FLAG_SURFACE_ENCODE)) ||
        (decode && (trace_flag & VA_TRACE_FLAG_SURFACE_DECODE)) ||
        (jpeg && (trace_flag & VA_TRACE_FLAG_SURFACE_JPEG))) {
        FILE *tmp = fopen(trace_context[idx].trace_surface_fn, "w");
        
        if (tmp)
            trace_context[idx].trace_fp_surface = tmp;
        else {
            va_errorMessage("Open file %s failed (%s)\n",
                            trace_context[idx].trace_surface_fn,
                            strerror(errno));
            trace_context[idx].trace_fp_surface = NULL;
            trace_flag &= ~(VA_TRACE_FLAG_SURFACE);
        }
    }

    if (encode && (trace_flag & VA_TRACE_FLAG_CODEDBUF)) {
        FILE *tmp = fopen(trace_context[idx].trace_codedbuf_fn, "w");
        
        if (tmp)
            trace_context[idx].trace_fp_codedbuf = tmp;
        else {
            va_errorMessage("Open file %s failed (%s)\n",
                            trace_context[idx].trace_codedbuf_fn,
                            strerror(errno));
            trace_context[idx].trace_fp_codedbuf = NULL;
            trace_flag &= ~VA_TRACE_FLAG_CODEDBUF;
        }
    }
}


void va_TraceCreateSurface(
    VADisplay dpy,
    int width,
    int height,
    int format,
    int num_surfaces,
    VASurfaceID *surfaces    /* out */
)
{
    int i;
    DPY2INDEX(dpy);

    TRACE_FUNCNAME(idx);
    
    va_TraceMsg(idx, "\twidth = %d\n", width);
    va_TraceMsg(idx, "\theight = %d\n", height);
    va_TraceMsg(idx, "\tformat = %d\n", format);
    va_TraceMsg(idx, "\tnum_surfaces = %d\n", num_surfaces);

    for (i = 0; i < num_surfaces; i++)
        va_TraceMsg(idx, "\t\tsurfaces[%d] = 0x%08x\n", i, surfaces[i]);

    va_TraceMsg(idx, NULL);
}


void va_TraceCreateContext(
    VADisplay dpy,
    VAConfigID config_id,
    int picture_width,
    int picture_height,
    int flag,
    VASurfaceID *render_targets,
    int num_render_targets,
    VAContextID *context        /* out */
)
{
    int i;
    DPY2INDEX(dpy);

    TRACE_FUNCNAME(idx);
    
    va_TraceMsg(idx, "\twidth = %d\n", picture_width);
    va_TraceMsg(idx, "\theight = %d\n", picture_height);
    va_TraceMsg(idx, "\tflag = 0x%08x\n", flag);
    va_TraceMsg(idx, "\tnum_render_targets = %d\n", num_render_targets);
    for (i=0; i<num_render_targets; i++)
        va_TraceMsg(idx, "\t\trender_targets[%d] = 0x%08x\n", i, render_targets[i]);
    va_TraceMsg(idx, "\tcontext = 0x%08x\n", *context);
    va_TraceMsg(idx, NULL);

    trace_context[idx].trace_context = *context;

    trace_context[idx].trace_frame_no = 0;
    trace_context[idx].trace_slice_no = 0;

    trace_context[idx].trace_frame_width = picture_width;
    trace_context[idx].trace_frame_height = picture_height;
}


static char * buffer_type_to_string(int type)
{
    switch (type) {
    case VAPictureParameterBufferType: return "VAPictureParameterBufferType";
    case VAIQMatrixBufferType: return "VAIQMatrixBufferType";
    case VABitPlaneBufferType: return "VABitPlaneBufferType";
    case VASliceGroupMapBufferType: return "VASliceGroupMapBufferType";
    case VASliceParameterBufferType: return "VASliceParameterBufferType";
    case VASliceDataBufferType: return "VASliceDataBufferType";
    case VAProtectedSliceDataBufferType: return "VAProtectedSliceDataBufferType";
    case VAMacroblockParameterBufferType: return "VAMacroblockParameterBufferType";
    case VAResidualDataBufferType: return "VAResidualDataBufferType";
    case VADeblockingParameterBufferType: return "VADeblockingParameterBufferType";
    case VAImageBufferType: return "VAImageBufferType";
    case VAEncCodedBufferType: return "VAEncCodedBufferType";
    case VAEncSequenceParameterBufferType: return "VAEncSequenceParameterBufferType";
    case VAEncPictureParameterBufferType: return "VAEncPictureParameterBufferType";
    case VAEncSliceParameterBufferType: return "VAEncSliceParameterBufferType";
    case VAEncMiscParameterBufferType: return "VAEncMiscParameterBufferType";
    default: return "UnknowBuffer";
    }
}

void va_TraceMapBuffer (
    VADisplay dpy,
    VABufferID buf_id,    /* in */
    void **pbuf           /* out */
)
{
    VABufferType type;
    unsigned int size;
    unsigned int num_elements;
    
    VACodedBufferSegment *buf_list;
    int i = 0;
    
    DPY2INDEX(dpy);

    vaBufferInfo(dpy, trace_context[idx].trace_context, buf_id, &type, &size, &num_elements);    
    /*
      va_TraceMsg(idx, "\tbuf_id=0x%x\n", buf_id);
      va_TraceMsg(idx, "\tbuf_type=%s\n", buffer_type_to_string(type));
      va_TraceMsg(idx, "\tbuf_size=%s\n", size);
      va_TraceMsg(idx, "\tbuf_elements=%s\n", &num_elements);
    */
    
    /* only trace CodedBuffer */
    if (type != VAEncCodedBufferType)
        return;
    
    buf_list = (VACodedBufferSegment *)(*pbuf);
    while (buf_list != NULL) {
        va_TraceMsg(idx, "\tCodedbuf[%d] =\n", i++);
        
        va_TraceMsg(idx, "\t   size = %d\n", buf_list->size);
        va_TraceMsg(idx, "\t   bit_offset = %d\n", buf_list->bit_offset);
        va_TraceMsg(idx, "\t   status = 0x%08x\n", buf_list->status);
        va_TraceMsg(idx, "\t   reserved = 0x%08x\n", buf_list->reserved);
        va_TraceMsg(idx, "\t   buf = 0x%08x\n", buf_list->buf);

        buf_list = buf_list->next;
    }
    va_TraceMsg(idx, NULL);
}

static void va_TraceVABuffers(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *pbuf
)
{
    unsigned int i;
    unsigned char *p = pbuf;
    unsigned char  check_sum = 0;
    DPY2INDEX(dpy);
    
    va_TraceMsg(idx, "%s\n",  buffer_type_to_string(type));

    for (i=0; i<size; i++) {
        unsigned char value =  p[i];
            
        if ((trace_flag & VA_TRACE_FLAG_BUFDATA) && ((i%16) == 0))
            va_TraceMsg(idx, "\n0x%08x:", i);

        if (trace_flag & VA_TRACE_FLAG_BUFDATA)
            va_TraceMsg(idx, " %02x", value);

        check_sum ^= value;
    }

    va_TraceMsg(idx, "\tchecksum = 0x%02x\n", check_sum & 0xff);
    va_TraceMsg(idx, NULL);

    return;
}


static void va_TraceVAPictureParameterBufferMPEG2(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    VAPictureParameterBufferMPEG2 *p=(VAPictureParameterBufferMPEG2 *)data;
    DPY2INDEX(dpy);

    va_TraceMsg(idx,"VAPictureParameterBufferMPEG2\n");

    va_TraceMsg(idx,"\thorizontal size= %d\n", p->horizontal_size);
    va_TraceMsg(idx,"\tvertical size= %d\n", p->vertical_size);
    va_TraceMsg(idx,"\tforward reference picture= %d\n", p->forward_reference_picture);
    va_TraceMsg(idx,"\tbackward reference picture= %d\n", p->backward_reference_picture);
    va_TraceMsg(idx,"\tpicture coding type= %d\n", p->picture_coding_type);
    va_TraceMsg(idx,"\tf mode= %d\n", p->f_code);

    va_TraceMsg(idx,"\tpicture coding extension = %d\n", p->picture_coding_extension.value);
    va_TraceMsg(idx,"\tintra_dc_precision= %d\n", p->picture_coding_extension.bits.intra_dc_precision);
    va_TraceMsg(idx,"\tpicture_structure= %d\n", p->picture_coding_extension.bits.picture_structure);
    va_TraceMsg(idx,"\ttop_field_first= %d\n", p->picture_coding_extension.bits.top_field_first);
    va_TraceMsg(idx,"\tframe_pred_frame_dct= %d\n", p->picture_coding_extension.bits.frame_pred_frame_dct);
    va_TraceMsg(idx,"\tconcealment_motion_vectors= %d\n", p->picture_coding_extension.bits.concealment_motion_vectors);
    va_TraceMsg(idx,"\tq_scale_type= %d\n", p->picture_coding_extension.bits.q_scale_type);
    va_TraceMsg(idx,"\tintra_vlc_format= %d\n", p->picture_coding_extension.bits.intra_vlc_format);
    va_TraceMsg(idx,"\talternate_scan= %d\n", p->picture_coding_extension.bits.alternate_scan);
    va_TraceMsg(idx,"\trepeat_first_field= %d\n", p->picture_coding_extension.bits.repeat_first_field);
    va_TraceMsg(idx,"\tprogressive_frame= %d\n", p->picture_coding_extension.bits.progressive_frame);
    va_TraceMsg(idx,"\tis_first_field= %d\n", p->picture_coding_extension.bits.is_first_field);
    va_TraceMsg(idx, NULL);

    return;
}


static void va_TraceVAIQMatrixBufferMPEG2(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    VAIQMatrixBufferMPEG2 *p=(VAIQMatrixBufferMPEG2 *)data;
    DPY2INDEX(dpy);

    va_TraceMsg(idx,"VAIQMatrixBufferMPEG2\n");

    va_TraceMsg(idx,"\tload_intra_quantiser_matrix = %d\n", p->load_intra_quantiser_matrix);
    va_TraceMsg(idx,"\tload_non_intra_quantiser_matrix = %d\n", p->load_non_intra_quantiser_matrix);
    va_TraceMsg(idx,"\tload_chroma_intra_quantiser_matrix = %d\n", p->load_chroma_intra_quantiser_matrix);
    va_TraceMsg(idx,"\tload_chroma_non_intra_quantiser_matrix = %d\n", p->load_chroma_non_intra_quantiser_matrix);
    va_TraceMsg(idx,"\tintra_quantiser_matrix = %d\n", p->intra_quantiser_matrix);
    va_TraceMsg(idx,"\tnon_intra_quantiser_matrix = %d\n", p->non_intra_quantiser_matrix);
    va_TraceMsg(idx,"\tchroma_intra_quantiser_matrix = %d\n", p->chroma_intra_quantiser_matrix);
    va_TraceMsg(idx,"\tchroma_non_intra_quantiser_matrix = %d\n", p->chroma_non_intra_quantiser_matrix);
    va_TraceMsg(idx, NULL);

    return;
}


static void va_TraceVASliceParameterBufferMPEG2(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    VASliceParameterBufferMPEG2 *p=(VASliceParameterBufferMPEG2 *)data;

    DPY2INDEX(dpy);

    trace_context[idx].trace_slice_no++;
    
    trace_context[idx].trace_slice_size = p->slice_data_size;

    va_TraceMsg(idx,"VASliceParameterBufferMPEG2\n");

    va_TraceMsg(idx,"\tslice_data_size = %d\n", p->slice_data_size);
    va_TraceMsg(idx,"\tslice_data_offset = %d\n", p->slice_data_offset);
    va_TraceMsg(idx,"\tslice_data_flag = %d\n", p->slice_data_flag);
    va_TraceMsg(idx,"\tmacroblock_offset = %d\n", p->macroblock_offset);
    va_TraceMsg(idx,"\tslice_horizontal_position = %d\n", p->slice_horizontal_position);
    va_TraceMsg(idx,"\tslice_vertical_position = %d\n", p->slice_vertical_position);
    va_TraceMsg(idx,"\tquantiser_scale_code = %d\n", p->quantiser_scale_code);
    va_TraceMsg(idx,"\tintra_slice_flag = %d\n", p->intra_slice_flag);
    va_TraceMsg(idx, NULL);

    return;
}


static void va_TraceVAPictureParameterBufferMPEG4(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    int i;
    VAPictureParameterBufferMPEG4 *p=(VAPictureParameterBufferMPEG4 *)data;
    
    DPY2INDEX(dpy);

    va_TraceMsg(idx,"*VAPictureParameterBufferMPEG4\n");
    va_TraceMsg(idx,"\tvop_width = %d\n", p->vop_width);
    va_TraceMsg(idx,"\tvop_height = %d\n", p->vop_height);
    va_TraceMsg(idx,"\tforward_reference_picture = %d\n", p->forward_reference_picture);
    va_TraceMsg(idx,"\tbackward_reference_picture = %d\n", p->backward_reference_picture);
    va_TraceMsg(idx,"\tvol_fields value = %d\n", p->vol_fields.value);
    va_TraceMsg(idx,"\tshort_video_header= %d\n", p->vol_fields.bits.short_video_header);
    va_TraceMsg(idx,"\tchroma_format= %d\n", p->vol_fields.bits.chroma_format);
    va_TraceMsg(idx,"\tinterlaced= %d\n", p->vol_fields.bits.interlaced);
    va_TraceMsg(idx,"\tobmc_disable= %d\n", p->vol_fields.bits.obmc_disable);
    va_TraceMsg(idx,"\tsprite_enable= %d\n", p->vol_fields.bits.sprite_enable);
    va_TraceMsg(idx,"\tsprite_warping_accuracy= %d\n", p->vol_fields.bits.sprite_warping_accuracy);
    va_TraceMsg(idx,"\tquant_type= %d\n", p->vol_fields.bits.quant_type);
    va_TraceMsg(idx,"\tquarter_sample= %d\n", p->vol_fields.bits.quarter_sample);
    va_TraceMsg(idx,"\tdata_partitioned= %d\n", p->vol_fields.bits.data_partitioned);
    va_TraceMsg(idx,"\treversible_vlc= %d\n", p->vol_fields.bits.reversible_vlc);
    va_TraceMsg(idx,"\tresync_marker_disable= %d\n", p->vol_fields.bits.resync_marker_disable);
    va_TraceMsg(idx,"\tno_of_sprite_warping_points = %d\n", p->no_of_sprite_warping_points);
    va_TraceMsg(idx,"\tsprite_trajectory_du =");
    for(i=0;i<3;i++)
        va_TraceMsg(idx,"\t%d", p->sprite_trajectory_du[i]);

    va_TraceMsg(idx,"\n");
    va_TraceMsg(idx,"\tsprite_trajectory_dv =");
    for(i=0;i<3;i++)
        va_TraceMsg(idx,"\t%d", p->sprite_trajectory_dv[i]);
    va_TraceMsg(idx,"\n");
    va_TraceMsg(idx,"\tvop_fields value = %d\n", p->vop_fields.value);
    va_TraceMsg(idx,"\tvop_coding_type= %d\n", p->vop_fields.bits.vop_coding_type);
    va_TraceMsg(idx,"\tbackward_reference_vop_coding_type= %d\n", p->vop_fields.bits.backward_reference_vop_coding_type);
    va_TraceMsg(idx,"\tvop_rounding_type= %d\n", p->vop_fields.bits.vop_rounding_type);
    va_TraceMsg(idx,"\tintra_dc_vlc_thr= %d\n", p->vop_fields.bits.intra_dc_vlc_thr);
    va_TraceMsg(idx,"\ttop_field_first= %d\n", p->vop_fields.bits.top_field_first);
    va_TraceMsg(idx,"\talternate_vertical_scan_flag= %d\n", p->vop_fields.bits.alternate_vertical_scan_flag);
    va_TraceMsg(idx,"\tvop_fcode_forward = %d\n", p->vop_fcode_forward);
    va_TraceMsg(idx,"\tvop_fcode_backward = %d\n", p->vop_fcode_backward);
    va_TraceMsg(idx,"\tnum_gobs_in_vop = %d\n", p->num_gobs_in_vop);
    va_TraceMsg(idx,"\tnum_macroblocks_in_gob = %d\n", p->num_macroblocks_in_gob);
    va_TraceMsg(idx,"\tTRB = %d\n", p->TRB);
    va_TraceMsg(idx,"\tTRD = %d\n", p->TRD);
    va_TraceMsg(idx, NULL);

    return;
}


static void va_TraceVAIQMatrixBufferMPEG4(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    int i;
    VAIQMatrixBufferMPEG4 *p=(VAIQMatrixBufferMPEG4 *)data;
    DPY2INDEX(dpy);

    va_TraceMsg(idx,"VAIQMatrixBufferMPEG4\n");

    va_TraceMsg(idx,"\tload_intra_quant_mat = %d\n", p->load_intra_quant_mat);
    va_TraceMsg(idx,"\tload_non_intra_quant_mat = %d\n", p->load_non_intra_quant_mat);
    va_TraceMsg(idx,"\tintra_quant_mat =\n");
    for(i=0;i<64;i++)
        va_TraceMsg(idx,"\t\t%d\n", p->intra_quant_mat[i]);

    va_TraceMsg(idx,"\tnon_intra_quant_mat =\n");
    for(i=0;i<64;i++)
        va_TraceMsg(idx,"\t\t%d\n", p->non_intra_quant_mat[i]);
    va_TraceMsg(idx, NULL);

    return;
}

static void va_TraceVAEncSequenceParameterBufferMPEG4(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    VAEncSequenceParameterBufferMPEG4 *p = (VAEncSequenceParameterBufferMPEG4 *)data;
    DPY2INDEX(dpy);
    
    va_TraceMsg(idx, "VAEncSequenceParameterBufferMPEG4\n");
    
    va_TraceMsg(idx, "\tprofile_and_level_indication = %d\n", p->profile_and_level_indication);
    va_TraceMsg(idx, "\tintra_period = %d\n", p->intra_period);
    va_TraceMsg(idx, "\tvideo_object_layer_width = %d\n", p->video_object_layer_width);
    va_TraceMsg(idx, "\tvideo_object_layer_height = %d\n", p->video_object_layer_height);
    va_TraceMsg(idx, "\tvop_time_increment_resolution = %d\n", p->vop_time_increment_resolution);
    va_TraceMsg(idx, "\tfixed_vop_rate = %d\n", p->fixed_vop_rate);
    va_TraceMsg(idx, "\tfixed_vop_time_increment = %d\n", p->fixed_vop_time_increment);
    va_TraceMsg(idx, "\tbits_per_second = %d\n", p->bits_per_second);
    va_TraceMsg(idx, "\tframe_rate = %d\n", p->frame_rate);
    va_TraceMsg(idx, "\tinitial_qp = %d\n", p->initial_qp);
    va_TraceMsg(idx, "\tmin_qp = %d\n", p->min_qp);
    va_TraceMsg(idx, NULL);

    /* start a new sequce, coded log file can be truncated */
    trace_context[idx].trace_sequence_start = 1;

    return;
}

static void va_TraceVAEncPictureParameterBufferMPEG4(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    VAEncPictureParameterBufferMPEG4 *p = (VAEncPictureParameterBufferMPEG4 *)data;
    DPY2INDEX(dpy);
    
    va_TraceMsg(idx, "VAEncPictureParameterBufferMPEG4\n");
    va_TraceMsg(idx, "\treference_picture = 0x%08x\n", p->reference_picture);
    va_TraceMsg(idx, "\treconstructed_picture = 0x%08x\n", p->reconstructed_picture);
    va_TraceMsg(idx, "\tcoded_buf = %08x\n", p->coded_buf);
    va_TraceMsg(idx, "\tpicture_width = %d\n", p->picture_width);
    va_TraceMsg(idx, "\tpicture_height = %d\n", p->picture_height);
    va_TraceMsg(idx, "\tmodulo_time_base = %d\n", p->modulo_time_base);
    va_TraceMsg(idx, "\tvop_time_increment = %d\n", p->vop_time_increment);
    va_TraceMsg(idx, "\tpicture_type = %d\n", p->picture_type);
    va_TraceMsg(idx, NULL);

    trace_context[idx].trace_codedbuf =  p->coded_buf;
    
    return;
}


static void va_TraceVASliceParameterBufferMPEG4(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    VASliceParameterBufferMPEG4 *p=(VASliceParameterBufferMPEG4 *)data;
    
    DPY2INDEX(dpy);

    trace_context[idx].trace_slice_no++;

    trace_context[idx].trace_slice_size = p->slice_data_size;

    va_TraceMsg(idx,"VASliceParameterBufferMPEG4\n");

    va_TraceMsg(idx,"\tslice_data_size = %d\n", p->slice_data_size);
    va_TraceMsg(idx,"\tslice_data_offset = %d\n", p->slice_data_offset);
    va_TraceMsg(idx,"\tslice_data_flag = %d\n", p->slice_data_flag);
    va_TraceMsg(idx,"\tmacroblock_offset = %d\n", p->macroblock_offset);
    va_TraceMsg(idx,"\tmacroblock_number = %d\n", p->macroblock_number);
    va_TraceMsg(idx,"\tquant_scale = %d\n", p->quant_scale);
    va_TraceMsg(idx, NULL);

    return;
}


static inline void va_TraceFlagIfNotZero(
    int idx,            /* in */
    const char *name,   /* in */
    unsigned int flag   /* in */
)
{
    if (flag != 0) {
        va_TraceMsg(idx, "%s = %x\n", name, flag);
    }
}


static void va_TraceVAPictureParameterBufferH264(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    int i;
    VAPictureParameterBufferH264 *p = (VAPictureParameterBufferH264*)data;
    
    DPY2INDEX(dpy);

    va_TraceMsg(idx, "VAPictureParameterBufferH264\n");

    va_TraceMsg(idx, "\tCurrPic.picture_id = 0x%08x\n", p->CurrPic.picture_id);
    va_TraceMsg(idx, "\tCurrPic.frame_idx = %d\n", p->CurrPic.frame_idx);
    va_TraceMsg(idx, "\tCurrPic.flags = %d\n", p->CurrPic.flags);
    va_TraceMsg(idx, "\tCurrPic.TopFieldOrderCnt = %d\n", p->CurrPic.TopFieldOrderCnt);
    va_TraceMsg(idx, "\tCurrPic.BottomFieldOrderCnt = %d\n", p->CurrPic.BottomFieldOrderCnt);

    va_TraceMsg(idx, "\tReferenceFrames (TopFieldOrderCnt-BottomFieldOrderCnt-picture_id-frame_idx:\n");
    for (i = 0; i < 16; i++)
    {
        if (p->ReferenceFrames[i].flags != VA_PICTURE_H264_INVALID) {
            va_TraceMsg(idx, "\t\t%d-%d-0x%08x-%d\n",
                        p->ReferenceFrames[i].TopFieldOrderCnt,
                        p->ReferenceFrames[i].BottomFieldOrderCnt,
                        p->ReferenceFrames[i].picture_id,
                        p->ReferenceFrames[i].frame_idx);
        } else
            va_TraceMsg(idx, "\t\tinv-inv-inv-inv\n");
    }
    va_TraceMsg(idx, "\n");
    
    va_TraceMsg(idx, "\tpicture_width_in_mbs_minus1 = %d\n", p->picture_width_in_mbs_minus1);
    va_TraceMsg(idx, "\tpicture_height_in_mbs_minus1 = %d\n", p->picture_height_in_mbs_minus1);
    va_TraceMsg(idx, "\tbit_depth_luma_minus8 = %d\n", p->bit_depth_luma_minus8);
    va_TraceMsg(idx, "\tbit_depth_chroma_minus8 = %d\n", p->bit_depth_chroma_minus8);
    va_TraceMsg(idx, "\tnum_ref_frames = %d\n", p->num_ref_frames);
    va_TraceMsg(idx, "\tseq fields = %d\n", p->seq_fields.value);
    va_TraceMsg(idx, "\tchroma_format_idc = %d\n", p->seq_fields.bits.chroma_format_idc);
    va_TraceMsg(idx, "\tresidual_colour_transform_flag = %d\n", p->seq_fields.bits.residual_colour_transform_flag);
    va_TraceMsg(idx, "\tframe_mbs_only_flag = %d\n", p->seq_fields.bits.frame_mbs_only_flag);
    va_TraceMsg(idx, "\tmb_adaptive_frame_field_flag = %d\n", p->seq_fields.bits.mb_adaptive_frame_field_flag);
    va_TraceMsg(idx, "\tdirect_8x8_inference_flag = %d\n", p->seq_fields.bits.direct_8x8_inference_flag);
    va_TraceMsg(idx, "\tMinLumaBiPredSize8x8 = %d\n", p->seq_fields.bits.MinLumaBiPredSize8x8);
    va_TraceMsg(idx, "\tnum_slice_groups_minus1 = %d\n", p->num_slice_groups_minus1);
    va_TraceMsg(idx, "\tslice_group_map_type = %d\n", p->slice_group_map_type);
    va_TraceMsg(idx, "\tslice_group_change_rate_minus1 = %d\n", p->slice_group_change_rate_minus1);
    va_TraceMsg(idx, "\tpic_init_qp_minus26 = %d\n", p->pic_init_qp_minus26);
    va_TraceMsg(idx, "\tpic_init_qs_minus26 = %d\n", p->pic_init_qs_minus26);
    va_TraceMsg(idx, "\tchroma_qp_index_offset = %d\n", p->chroma_qp_index_offset);
    va_TraceMsg(idx, "\tsecond_chroma_qp_index_offset = %d\n", p->second_chroma_qp_index_offset);
    va_TraceMsg(idx, "\tpic_fields = 0x%03x\n", p->pic_fields.value);
    va_TraceFlagIfNotZero(idx, "\t\tentropy_coding_mode_flag", p->pic_fields.bits.entropy_coding_mode_flag);
    va_TraceFlagIfNotZero(idx, "\t\tweighted_pred_flag", p->pic_fields.bits.weighted_pred_flag);
    va_TraceFlagIfNotZero(idx, "\t\tweighted_bipred_idc", p->pic_fields.bits.weighted_bipred_idc);
    va_TraceFlagIfNotZero(idx, "\t\ttransform_8x8_mode_flag", p->pic_fields.bits.transform_8x8_mode_flag);
    va_TraceFlagIfNotZero(idx, "\t\tfield_pic_flag", p->pic_fields.bits.field_pic_flag);
    va_TraceFlagIfNotZero(idx, "\t\tconstrained_intra_pred_flag", p->pic_fields.bits.constrained_intra_pred_flag);
    va_TraceFlagIfNotZero(idx, "\t\tpic_order_present_flag", p->pic_fields.bits.pic_order_present_flag);
    va_TraceFlagIfNotZero(idx, "\t\tdeblocking_filter_control_present_flag", p->pic_fields.bits.deblocking_filter_control_present_flag);
    va_TraceFlagIfNotZero(idx, "\t\tredundant_pic_cnt_present_flag", p->pic_fields.bits.redundant_pic_cnt_present_flag);
    va_TraceFlagIfNotZero(idx, "\t\treference_pic_flag", p->pic_fields.bits.reference_pic_flag);
    va_TraceMsg(idx, "\tframe_num = %d\n", p->frame_num);
    va_TraceMsg(idx, NULL);

    return;
}

static void va_TraceVASliceParameterBufferH264(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    int i;
    VASliceParameterBufferH264* p = (VASliceParameterBufferH264*)data;
    DPY2INDEX(dpy);

    trace_context[idx].trace_slice_no++;
    trace_context[idx].trace_slice_size = p->slice_data_size;

    va_TraceMsg(idx, "VASliceParameterBufferH264\n");
    va_TraceMsg(idx, "\tslice_data_size = %d\n", p->slice_data_size);
    va_TraceMsg(idx, "\tslice_data_offset = %d\n", p->slice_data_offset);
    va_TraceMsg(idx, "\tslice_data_flag = %d\n", p->slice_data_flag);
    va_TraceMsg(idx, "\tslice_data_bit_offset = %d\n", p->slice_data_bit_offset);
    va_TraceMsg(idx, "\tfirst_mb_in_slice = %d\n", p->first_mb_in_slice);
    va_TraceMsg(idx, "\tslice_type = %d\n", p->slice_type);
    va_TraceMsg(idx, "\tdirect_spatial_mv_pred_flag = %d\n", p->direct_spatial_mv_pred_flag);
    va_TraceMsg(idx, "\tnum_ref_idx_l0_active_minus1 = %d\n", p->num_ref_idx_l0_active_minus1);
    va_TraceMsg(idx, "\tnum_ref_idx_l1_active_minus1 = %d\n", p->num_ref_idx_l1_active_minus1);
    va_TraceMsg(idx, "\tcabac_init_idc = %d\n", p->cabac_init_idc);
    va_TraceMsg(idx, "\tslice_qp_delta = %d\n", p->slice_qp_delta);
    va_TraceMsg(idx, "\tdisable_deblocking_filter_idc = %d\n", p->disable_deblocking_filter_idc);
    va_TraceMsg(idx, "\tslice_alpha_c0_offset_div2 = %d\n", p->slice_alpha_c0_offset_div2);
    va_TraceMsg(idx, "\tslice_beta_offset_div2 = %d\n", p->slice_beta_offset_div2);

    if (p->slice_type == 0 || p->slice_type == 1) {
        va_TraceMsg(idx, "\tRefPicList0 =");
        for (i = 0; i < p->num_ref_idx_l0_active_minus1 + 1; i++) {
            va_TraceMsg(idx, "%d-%d-0x%08x-%d\n", p->RefPicList0[i].TopFieldOrderCnt, p->RefPicList0[i].BottomFieldOrderCnt, p->RefPicList0[i].picture_id, p->RefPicList0[i].frame_idx);
        }
        if (p->slice_type == 1) {
            va_TraceMsg(idx, "\tRefPicList1 =");
            for (i = 0; i < p->num_ref_idx_l1_active_minus1 + 1; i++)
            {
                va_TraceMsg(idx, "%d-%d-0x%08x-%d\n", p->RefPicList1[i].TopFieldOrderCnt, p->RefPicList1[i].BottomFieldOrderCnt, p->RefPicList1[i].picture_id, p->RefPicList1[i].frame_idx);
            }
        }
    }
    
    va_TraceMsg(idx, "\tluma_log2_weight_denom = %d\n", p->luma_log2_weight_denom);
    va_TraceMsg(idx, "\tchroma_log2_weight_denom = %d\n", p->chroma_log2_weight_denom);
    va_TraceMsg(idx, "\tluma_weight_l0_flag = %d\n", p->luma_weight_l0_flag);
    if (p->luma_weight_l0_flag) {
        for (i = 0; i <=  p->num_ref_idx_l0_active_minus1; i++) {
            va_TraceMsg(idx, "\t%d ", p->luma_weight_l0[i]);
            va_TraceMsg(idx, "\t%d ", p->luma_offset_l0[i]);
        }
    }

    va_TraceMsg(idx, "\tchroma_weight_l0_flag = %d\n", p->chroma_weight_l0_flag);
    if (p->chroma_weight_l0_flag) {
        for (i = 0; i <= p->num_ref_idx_l0_active_minus1; i++) {
            va_TraceMsg(idx, "\t\t%d ", p->chroma_weight_l0[i][0]);
            va_TraceMsg(idx, "\t\t%d ", p->chroma_offset_l0[i][0]);
            va_TraceMsg(idx, "\t\t%d ", p->chroma_weight_l0[i][1]);
            va_TraceMsg(idx, "\t\t%d ", p->chroma_offset_l0[i][1]);
        }
    }
    
    va_TraceMsg(idx, "\tluma_weight_l1_flag = %d\n", p->luma_weight_l1_flag);
    if (p->luma_weight_l1_flag) {
        for (i = 0; i <=  p->num_ref_idx_l1_active_minus1; i++) {
            va_TraceMsg(idx, "\t\t%d ", p->luma_weight_l1[i]);
            va_TraceMsg(idx, "\t\t%d ", p->luma_offset_l1[i]);
        }
    }
    
    va_TraceMsg(idx, "\tchroma_weight_l1_flag = %d\n", p->chroma_weight_l1_flag);
    if (p->chroma_weight_l1_flag) {
        for (i = 0; i <= p->num_ref_idx_l1_active_minus1; i++) {
            va_TraceMsg(idx, "\t\t%d ", p->chroma_weight_l1[i][0]);
            va_TraceMsg(idx, "\t\t%d ", p->chroma_offset_l1[i][0]);
            va_TraceMsg(idx, "\t\t%d ", p->chroma_weight_l1[i][1]);
            va_TraceMsg(idx, "\t\t%d ", p->chroma_offset_l1[i][1]);
        }
        va_TraceMsg(idx, "\n");
    }
    va_TraceMsg(idx, NULL);
}

static void va_TraceVAIQMatrixBufferH264(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data
)
{
    int i, j;    
    VAIQMatrixBufferH264* p = (VAIQMatrixBufferH264* )data;

    DPY2INDEX(dpy);

    va_TraceMsg(idx, "VAIQMatrixBufferH264\n");

    va_TraceMsg(idx, "\tScalingList4x4[6][16]=\n");
    for (i = 0; i < 6; i++) {
        for (j = 0; j < 16; j++) {
            va_TraceMsg(idx, "\t%d\t", p->ScalingList4x4[i][j]);
            if ((j + 1) % 8 == 0)
                va_TraceMsg(idx, "\n");
        }
    }

    va_TraceMsg(idx, "\tScalingList8x8[2][64]=\n");
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 64; j++) {
            va_TraceMsg(idx, "\t%d", p->ScalingList8x8[i][j]);
            if ((j + 1) % 8 == 0)
                va_TraceMsg(idx, "\n");
        }
    }

    va_TraceMsg(idx, NULL);
}

static void va_TraceVAEncSequenceParameterBufferH264Baseline(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    VAEncSequenceParameterBufferH264Baseline *p = (VAEncSequenceParameterBufferH264Baseline *)data;
    DPY2INDEX(dpy);
    
    va_TraceMsg(idx, "VAEncSequenceParameterBufferH264Baseline\n");
    
    va_TraceMsg(idx, "\tseq_parameter_set_id = %d\n", p->seq_parameter_set_id);
    va_TraceMsg(idx, "\tlevel_idc = %d\n", p->level_idc);
    va_TraceMsg(idx, "\tintra_period = %d\n", p->intra_period);
    va_TraceMsg(idx, "\tintra_idr_period = %d\n", p->intra_idr_period);
    va_TraceMsg(idx, "\tmax_num_ref_frames = %d\n", p->max_num_ref_frames);
    va_TraceMsg(idx, "\tpicture_width_in_mbs = %d\n", p->picture_width_in_mbs);
    va_TraceMsg(idx, "\tpicture_height_in_mbs = %d\n", p->picture_height_in_mbs);
    va_TraceMsg(idx, "\tbits_per_second = %d\n", p->bits_per_second);
    va_TraceMsg(idx, "\tframe_rate = %d\n", p->frame_rate);
    va_TraceMsg(idx, "\tinitial_qp = %d\n", p->initial_qp);
    va_TraceMsg(idx, "\tmin_qp = %d\n", p->min_qp);
    va_TraceMsg(idx, "\tbasic_unit_size = %d\n", p->basic_unit_size);
    va_TraceMsg(idx, "\tvui_flag = %d\n", p->vui_flag);
    va_TraceMsg(idx, NULL);

    /* start a new sequce, coded log file can be truncated */
    trace_context[idx].trace_sequence_start = 1;

    return;
}

static void va_TraceVAEncPictureParameterBufferH264Baseline(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    VAEncPictureParameterBufferH264Baseline *p = (VAEncPictureParameterBufferH264Baseline *)data;
    DPY2INDEX(dpy);
    
    va_TraceMsg(idx, "VAEncPictureParameterBufferH264Baseline\n");
    va_TraceMsg(idx, "\treference_picture = 0x%08x\n", p->reference_picture);
    va_TraceMsg(idx, "\treconstructed_picture = 0x%08x\n", p->reconstructed_picture);
    va_TraceMsg(idx, "\tcoded_buf = %08x\n", p->coded_buf);
    va_TraceMsg(idx, "\tpicture_width = %d\n", p->picture_width);
    va_TraceMsg(idx, "\tpicture_height = %d\n", p->picture_height);
    va_TraceMsg(idx, "\tlast_picture = 0x%08x\n", p->last_picture);
    va_TraceMsg(idx, NULL);

    trace_context[idx].trace_codedbuf =  p->coded_buf;
    
    return;
}


static void va_TraceVAEncSliceParameterBuffer(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    VAEncSliceParameterBuffer* p = (VAEncSliceParameterBuffer*)data;
    DPY2INDEX(dpy);
    
    va_TraceMsg(idx, "VAEncSliceParameterBuffer\n");
    
    va_TraceMsg(idx, "\tstart_row_number = %d\n", p->start_row_number);
    va_TraceMsg(idx, "\tslice_height = %d\n", p->slice_height);
    va_TraceMsg(idx, "\tslice_flags.is_intra = %d\n", p->slice_flags.bits.is_intra);
    va_TraceMsg(idx, "\tslice_flags.disable_deblocking_filter_idc = %d\n", p->slice_flags.bits.disable_deblocking_filter_idc);
    va_TraceMsg(idx, "\tslice_flags.uses_long_term_ref = %d\n", p->slice_flags.bits.uses_long_term_ref);
    va_TraceMsg(idx, "\tslice_flags.is_long_term_ref = %d\n", p->slice_flags.bits.is_long_term_ref);
    va_TraceMsg(idx, NULL);

    return;
}

static void va_TraceVAEncMiscParameterBuffer(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    VAEncMiscParameterBuffer* tmp = (VAEncMiscParameterBuffer*)data;
    DPY2INDEX(dpy);
    
    switch (tmp->type) {
    case VAEncMiscParameterTypeFrameRate:
    {
        VAEncMiscParameterFrameRate *p = (VAEncMiscParameterFrameRate *)tmp->data;
        va_TraceMsg(idx, "VAEncMiscParameterFrameRate\n");
        va_TraceMsg(idx, "\tframerate = %d\n", p->framerate);
        
        break;
    }
    case VAEncMiscParameterTypeRateControl:
    {
        VAEncMiscParameterRateControl *p = (VAEncMiscParameterRateControl *)tmp->data;

        va_TraceMsg(idx, "VAEncMiscParameterRateControl\n");
        va_TraceMsg(idx, "\tbits_per_second = %d\n", p->bits_per_second);
        va_TraceMsg(idx, "\twindow_size = %d\n", p->window_size);
        va_TraceMsg(idx, "\tinitial_qp = %d\n", p->initial_qp);
        va_TraceMsg(idx, "\tmin_qp = %d\n", p->min_qp);
        break;
    }
    case VAEncMiscParameterTypeMaxSliceSize:
    {
        VAEncMiscParameterMaxSliceSize *p = (VAEncMiscParameterMaxSliceSize *)tmp->data;
        
        va_TraceMsg(idx, "VAEncMiscParameterTypeMaxSliceSize\n");
        va_TraceMsg(idx, "\tmax_slice_size = %d\n", p->max_slice_size);
        break;
    }
    case VAEncMiscParameterTypeAIR:
    {
        VAEncMiscParameterAIR *p = (VAEncMiscParameterAIR *)tmp->data;
        
        va_TraceMsg(idx, "VAEncMiscParameterAIR\n");
        va_TraceMsg(idx, "\tair_num_mbs = %d\n", p->air_num_mbs);
        va_TraceMsg(idx, "\tair_threshold = %d\n", p->air_threshold);
        va_TraceMsg(idx, "\tair_auto = %d\n", p->air_auto);
        break;
    }
    default:
        va_TraceMsg(idx, "invalid VAEncMiscParameterBuffer type = %d\n", tmp->type);
        break;
    }
    va_TraceMsg(idx, NULL);

    return;
}


static void va_TraceVAPictureParameterBufferVC1(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data
)
{
    VAPictureParameterBufferVC1* p = (VAPictureParameterBufferVC1*)data;
    DPY2INDEX(dpy);
    
    va_TraceMsg(idx, "VAPictureParameterBufferVC1\n");
    
    va_TraceMsg(idx, "\tforward_reference_picture = 0x%08x\n", p->forward_reference_picture);
    va_TraceMsg(idx, "\tbackward_reference_picture = 0x%08x\n", p->backward_reference_picture);
    va_TraceMsg(idx, "\tinloop_decoded_picture = 0x%08x\n", p->inloop_decoded_picture);
    
    va_TraceMsg(idx, "\tpulldown = %d\n", p->sequence_fields.bits.pulldown);
    va_TraceMsg(idx, "\tinterlace = %d\n", p->sequence_fields.bits.interlace);
    va_TraceMsg(idx, "\ttfcntrflag = %d\n", p->sequence_fields.bits.tfcntrflag);
    va_TraceMsg(idx, "\tfinterpflag = %d\n", p->sequence_fields.bits.finterpflag);
    va_TraceMsg(idx, "\tpsf = %d\n", p->sequence_fields.bits.psf);
    va_TraceMsg(idx, "\tmultires = %d\n", p->sequence_fields.bits.multires);
    va_TraceMsg(idx, "\toverlap = %d\n", p->sequence_fields.bits.overlap);
    va_TraceMsg(idx, "\tsyncmarker = %d\n", p->sequence_fields.bits.syncmarker);
    va_TraceMsg(idx, "\trangered = %d\n", p->sequence_fields.bits.rangered);
    va_TraceMsg(idx, "\tmax_b_frames = %d\n", p->sequence_fields.bits.max_b_frames);
    va_TraceMsg(idx, "\tprofile = %d\n", p->sequence_fields.bits.profile);
    va_TraceMsg(idx, "\tcoded_width = %d\n", p->coded_width);
    va_TraceMsg(idx, "\tcoded_height = %d\n", p->coded_height);
    va_TraceMsg(idx, "\tclosed_entry = %d\n", p->entrypoint_fields.bits.closed_entry);
    va_TraceMsg(idx, "\tbroken_link = %d\n", p->entrypoint_fields.bits.broken_link);
    va_TraceMsg(idx, "\tclosed_entry = %d\n", p->entrypoint_fields.bits.closed_entry);
    va_TraceMsg(idx, "\tpanscan_flag = %d\n", p->entrypoint_fields.bits.panscan_flag);
    va_TraceMsg(idx, "\tloopfilter = %d\n", p->entrypoint_fields.bits.loopfilter);
    va_TraceMsg(idx, "\tconditional_overlap_flag = %d\n", p->conditional_overlap_flag);
    va_TraceMsg(idx, "\tfast_uvmc_flag = %d\n", p->fast_uvmc_flag);
    va_TraceMsg(idx, "\trange_mapping_luma_flag = %d\n", p->range_mapping_fields.bits.luma_flag);
    va_TraceMsg(idx, "\trange_mapping_luma = %d\n", p->range_mapping_fields.bits.luma);
    va_TraceMsg(idx, "\trange_mapping_chroma_flag = %d\n", p->range_mapping_fields.bits.chroma_flag);
    va_TraceMsg(idx, "\trange_mapping_chroma = %d\n", p->range_mapping_fields.bits.chroma);
    va_TraceMsg(idx, "\tb_picture_fraction = %d\n", p->b_picture_fraction);
    va_TraceMsg(idx, "\tcbp_table = %d\n", p->cbp_table);
    va_TraceMsg(idx, "\tmb_mode_table = %d\n", p->mb_mode_table);
    va_TraceMsg(idx, "\trange_reduction_frame = %d\n", p->range_reduction_frame);
    va_TraceMsg(idx, "\trounding_control = %d\n", p->rounding_control);
    va_TraceMsg(idx, "\tpost_processing = %d\n", p->post_processing);
    va_TraceMsg(idx, "\tpicture_resolution_index = %d\n", p->picture_resolution_index);
    va_TraceMsg(idx, "\tluma_scale = %d\n", p->luma_scale);
    va_TraceMsg(idx, "\tluma_shift = %d\n", p->luma_shift);
    va_TraceMsg(idx, "\tpicture_type = %d\n", p->picture_fields.bits.picture_type);
    va_TraceMsg(idx, "\tframe_coding_mode = %d\n", p->picture_fields.bits.frame_coding_mode);
    va_TraceMsg(idx, "\ttop_field_first = %d\n", p->picture_fields.bits.top_field_first);
    va_TraceMsg(idx, "\tis_first_field = %d\n", p->picture_fields.bits.is_first_field);
    va_TraceMsg(idx, "\tintensity_compensation = %d\n", p->picture_fields.bits.intensity_compensation);
    va_TraceMsg(idx, "\tmv_type_mb = %d\n", p->raw_coding.flags.mv_type_mb);
    va_TraceMsg(idx, "\tdirect_mb = %d\n", p->raw_coding.flags.direct_mb);
    va_TraceMsg(idx, "\tskip_mb = %d\n", p->raw_coding.flags.skip_mb);
    va_TraceMsg(idx, "\tfield_tx = %d\n", p->raw_coding.flags.field_tx);
    va_TraceMsg(idx, "\tforward_mb = %d\n", p->raw_coding.flags.forward_mb);
    va_TraceMsg(idx, "\tac_pred = %d\n", p->raw_coding.flags.ac_pred);
    va_TraceMsg(idx, "\toverflags = %d\n", p->raw_coding.flags.overflags);
    va_TraceMsg(idx, "\tbp_mv_type_mb = %d\n", p->bitplane_present.flags.bp_mv_type_mb);
    va_TraceMsg(idx, "\tbp_direct_mb = %d\n", p->bitplane_present.flags.bp_direct_mb);
    va_TraceMsg(idx, "\tbp_skip_mb = %d\n", p->bitplane_present.flags.bp_skip_mb);
    va_TraceMsg(idx, "\tbp_field_tx = %d\n", p->bitplane_present.flags.bp_field_tx);
    va_TraceMsg(idx, "\tbp_forward_mb = %d\n", p->bitplane_present.flags.bp_forward_mb);
    va_TraceMsg(idx, "\tbp_ac_pred = %d\n", p->bitplane_present.flags.bp_ac_pred);
    va_TraceMsg(idx, "\tbp_overflags = %d\n", p->bitplane_present.flags.bp_overflags);
    va_TraceMsg(idx, "\treference_distance_flag = %d\n", p->reference_fields.bits.reference_distance_flag);
    va_TraceMsg(idx, "\treference_distance = %d\n", p->reference_fields.bits.reference_distance);
    va_TraceMsg(idx, "\tnum_reference_pictures = %d\n", p->reference_fields.bits.num_reference_pictures);
    va_TraceMsg(idx, "\treference_field_pic_indicator = %d\n", p->reference_fields.bits.reference_field_pic_indicator);
    va_TraceMsg(idx, "\tmv_mode = %d\n", p->mv_fields.bits.mv_mode);
    va_TraceMsg(idx, "\tmv_mode2 = %d\n", p->mv_fields.bits.mv_mode2);
    va_TraceMsg(idx, "\tmv_table = %d\n", p->mv_fields.bits.mv_table);
    va_TraceMsg(idx, "\ttwo_mv_block_pattern_table = %d\n", p->mv_fields.bits.two_mv_block_pattern_table);
    va_TraceMsg(idx, "\tfour_mv_switch = %d\n", p->mv_fields.bits.four_mv_switch);
    va_TraceMsg(idx, "\tfour_mv_block_pattern_table = %d\n", p->mv_fields.bits.four_mv_block_pattern_table);
    va_TraceMsg(idx, "\textended_mv_flag = %d\n", p->mv_fields.bits.extended_mv_flag);
    va_TraceMsg(idx, "\textended_mv_range = %d\n", p->mv_fields.bits.extended_mv_range);
    va_TraceMsg(idx, "\textended_dmv_flag = %d\n", p->mv_fields.bits.extended_dmv_flag);
    va_TraceMsg(idx, "\textended_dmv_range = %d\n", p->mv_fields.bits.extended_dmv_range);
    va_TraceMsg(idx, "\tdquant = %d\n", p->pic_quantizer_fields.bits.dquant);
    va_TraceMsg(idx, "\tquantizer = %d\n", p->pic_quantizer_fields.bits.quantizer);
    va_TraceMsg(idx, "\thalf_qp = %d\n", p->pic_quantizer_fields.bits.half_qp);
    va_TraceMsg(idx, "\tpic_quantizer_scale = %d\n", p->pic_quantizer_fields.bits.pic_quantizer_scale);
    va_TraceMsg(idx, "\tpic_quantizer_type = %d\n", p->pic_quantizer_fields.bits.pic_quantizer_type);
    va_TraceMsg(idx, "\tdq_frame = %d\n", p->pic_quantizer_fields.bits.dq_frame);
    va_TraceMsg(idx, "\tdq_profile = %d\n", p->pic_quantizer_fields.bits.dq_profile);
    va_TraceMsg(idx, "\tdq_sb_edge = %d\n", p->pic_quantizer_fields.bits.dq_sb_edge);
    va_TraceMsg(idx, "\tdq_db_edge = %d\n", p->pic_quantizer_fields.bits.dq_db_edge);
    va_TraceMsg(idx, "\tdq_binary_level = %d\n", p->pic_quantizer_fields.bits.dq_binary_level);
    va_TraceMsg(idx, "\talt_pic_quantizer = %d\n", p->pic_quantizer_fields.bits.alt_pic_quantizer);
    va_TraceMsg(idx, "\tvariable_sized_transform_flag = %d\n", p->transform_fields.bits.variable_sized_transform_flag);
    va_TraceMsg(idx, "\tmb_level_transform_type_flag = %d\n", p->transform_fields.bits.mb_level_transform_type_flag);
    va_TraceMsg(idx, "\tframe_level_transform_type = %d\n", p->transform_fields.bits.frame_level_transform_type);
    va_TraceMsg(idx, "\ttransform_ac_codingset_idx1 = %d\n", p->transform_fields.bits.transform_ac_codingset_idx1);
    va_TraceMsg(idx, "\ttransform_ac_codingset_idx2 = %d\n", p->transform_fields.bits.transform_ac_codingset_idx2);
    va_TraceMsg(idx, "\tintra_transform_dc_table = %d\n", p->transform_fields.bits.intra_transform_dc_table);
    va_TraceMsg(idx, NULL);
}

static void va_TraceVASliceParameterBufferVC1(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void* data
)
{
    VASliceParameterBufferVC1 *p = (VASliceParameterBufferVC1*)data;
    DPY2INDEX(dpy);

    trace_context[idx].trace_slice_no++;
    trace_context[idx].trace_slice_size = p->slice_data_size;

    va_TraceMsg(idx, "VASliceParameterBufferVC1\n");
    va_TraceMsg(idx, "\tslice_data_size = %d\n", p->slice_data_size);
    va_TraceMsg(idx, "\tslice_data_offset = %d\n", p->slice_data_offset);
    va_TraceMsg(idx, "\tslice_data_flag = %d\n", p->slice_data_flag);
    va_TraceMsg(idx, "\tmacroblock_offset = %d\n", p->macroblock_offset);
    va_TraceMsg(idx, "\tslice_vertical_position = %d\n", p->slice_vertical_position);
    va_TraceMsg(idx, NULL);
}

void va_TraceBeginPicture(
    VADisplay dpy,
    VAContextID context,
    VASurfaceID render_target
)
{
    DPY2INDEX(dpy);

    TRACE_FUNCNAME(idx);

    va_TraceMsg(idx, "\tcontext = 0x%08x\n", context);
    va_TraceMsg(idx, "\trender_targets = 0x%08x\n", render_target);
    va_TraceMsg(idx, "\tframe_count  = #%d\n", trace_context[idx].trace_frame_no);
    va_TraceMsg(idx, NULL);

    trace_context[idx].trace_rendertarget = render_target; /* for surface data dump after vaEndPicture */

    trace_context[idx].trace_frame_no++;
    trace_context[idx].trace_slice_no = 0;
}

static void va_TraceMPEG2Buf(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *pbuf
)
{
    switch (type) {
    case VAPictureParameterBufferType:
        va_TraceVAPictureParameterBufferMPEG2(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAIQMatrixBufferType:
        va_TraceVAIQMatrixBufferMPEG2(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VABitPlaneBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceGroupMapBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceParameterBufferType:
        va_TraceVASliceParameterBufferMPEG2(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAMacroblockParameterBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAResidualDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VADeblockingParameterBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAImageBufferType:
        break;
    case VAProtectedSliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncCodedBufferType:
        break;
    case VAEncSequenceParameterBufferType:
        break;
    case VAEncPictureParameterBufferType:
        break;
    case VAEncSliceParameterBufferType:
        break;
    default:
        break;
    }
}

static void va_TraceVAEncSequenceParameterBufferH263(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    VAEncSequenceParameterBufferH263 *p = (VAEncSequenceParameterBufferH263 *)data;
    DPY2INDEX(dpy);
    
    va_TraceMsg(idx, "VAEncSequenceParameterBufferH263\n");
    
    va_TraceMsg(idx, "\tintra_period = %d\n", p->intra_period);
    va_TraceMsg(idx, "\tbits_per_second = %d\n", p->bits_per_second);
    va_TraceMsg(idx, "\tframe_rate = %d\n", p->frame_rate);
    va_TraceMsg(idx, "\tinitial_qp = %d\n", p->initial_qp);
    va_TraceMsg(idx, "\tmin_qp = %d\n", p->min_qp);
    va_TraceMsg(idx, NULL);

    /* start a new sequce, coded log file can be truncated */
    trace_context[idx].trace_sequence_start = 1;

    return;
}


static void va_TraceVAEncPictureParameterBufferH263(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    VAEncPictureParameterBufferH263 *p = (VAEncPictureParameterBufferH263 *)data;
    DPY2INDEX(dpy);
    
    va_TraceMsg(idx, "VAEncPictureParameterBufferH263\n");
    va_TraceMsg(idx, "\treference_picture = 0x%08x\n", p->reference_picture);
    va_TraceMsg(idx, "\treconstructed_picture = 0x%08x\n", p->reconstructed_picture);
    va_TraceMsg(idx, "\tcoded_buf = %08x\n", p->coded_buf);
    va_TraceMsg(idx, "\tpicture_width = %d\n", p->picture_width);
    va_TraceMsg(idx, "\tpicture_height = %d\n", p->picture_height);
    va_TraceMsg(idx, "\tpicture_type = 0x%08x\n", p->picture_type);
    va_TraceMsg(idx, NULL);

    trace_context[idx].trace_codedbuf =  p->coded_buf;
    
    return;
}

static void va_TraceVAEncPictureParameterBufferJPEG(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    VAEncPictureParameterBufferJPEG *p = (VAEncPictureParameterBufferJPEG *)data;
    DPY2INDEX(dpy);
    
    va_TraceMsg(idx, "VAEncPictureParameterBufferJPEG\n");
    va_TraceMsg(idx, "\treconstructed_picture = 0x%08x\n", p->reconstructed_picture);
    va_TraceMsg(idx, "\tcoded_buf = %08x\n", p->coded_buf);
    va_TraceMsg(idx, "\tpicture_width = %d\n", p->picture_width);
    va_TraceMsg(idx, "\tpicture_height = %d\n", p->picture_height);
    va_TraceMsg(idx, NULL);

    trace_context[idx].trace_codedbuf =  p->coded_buf;
    
    return;
}

static void va_TraceVAEncQMatrixBufferJPEG(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    VAQMatrixBufferJPEG *p = (VAQMatrixBufferJPEG *)data;
    DPY2INDEX(dpy);
    
    va_TraceMsg(idx, "VAQMatrixBufferJPEG\n");
    va_TraceMsg(idx, "\tload_lum_quantiser_matrix = %d", p->load_lum_quantiser_matrix);
    if (p->load_lum_quantiser_matrix) {
        int i;
        for (i = 0; i < 64; i++) {
            if ((i % 8) == 0)
                va_TraceMsg(idx, "\n\t");
            va_TraceMsg(idx, "\t0x%02x", p->lum_quantiser_matrix[i]);
        }
        va_TraceMsg(idx, "\n");
    }
    va_TraceMsg(idx, "\tload_chroma_quantiser_matrix = %08x\n", p->load_chroma_quantiser_matrix);
    if (p->load_chroma_quantiser_matrix) {
        int i;
        for (i = 0; i < 64; i++) {
            if ((i % 8) == 0)
                va_TraceMsg(idx, "\n\t");
            va_TraceMsg(idx, "\t0x%02x", p->chroma_quantiser_matrix[i]);
        }
        va_TraceMsg(idx, "\n");
    }
    
    va_TraceMsg(idx, NULL);
    
    return;
}

static void va_TraceH263Buf(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *pbuf
)
{
    switch (type) {
    case VAPictureParameterBufferType:/* print MPEG4 buffer */
        va_TraceVAPictureParameterBufferMPEG4(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAIQMatrixBufferType:/* print MPEG4 buffer */
        va_TraceVAIQMatrixBufferMPEG4(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VABitPlaneBufferType:/* print MPEG4 buffer */
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceGroupMapBufferType:
        break;
    case VASliceParameterBufferType:/* print MPEG4 buffer */
        va_TraceVASliceParameterBufferMPEG4(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAMacroblockParameterBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAResidualDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VADeblockingParameterBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAImageBufferType:
        break;
    case VAProtectedSliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncCodedBufferType:
        break;
    case VAEncSequenceParameterBufferType:
        va_TraceVAEncSequenceParameterBufferH263(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncPictureParameterBufferType:
        va_TraceVAEncPictureParameterBufferH263(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncSliceParameterBufferType:
        va_TraceVAEncSliceParameterBuffer(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    default:
        break;
    }
}


static void va_TraceJPEGBuf(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *pbuf
)
{
    switch (type) {
    case VAPictureParameterBufferType:/* print MPEG4 buffer */
    case VAIQMatrixBufferType:/* print MPEG4 buffer */
    case VABitPlaneBufferType:/* print MPEG4 buffer */
    case VASliceGroupMapBufferType:
    case VASliceParameterBufferType:/* print MPEG4 buffer */
    case VASliceDataBufferType:
    case VAMacroblockParameterBufferType:
    case VAResidualDataBufferType:
    case VADeblockingParameterBufferType:
    case VAImageBufferType:
    case VAProtectedSliceDataBufferType:
    case VAEncCodedBufferType:
    case VAEncSequenceParameterBufferType:
    case VAEncSliceParameterBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncPictureParameterBufferType:
        va_TraceVAEncPictureParameterBufferJPEG(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAQMatrixBufferType:
        va_TraceVAEncQMatrixBufferJPEG(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    default:
        break;
    }
}

static void va_TraceMPEG4Buf(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *pbuf
)
{
    switch (type) {
    case VAPictureParameterBufferType:
        va_TraceVAPictureParameterBufferMPEG4(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAIQMatrixBufferType:
        va_TraceVAIQMatrixBufferMPEG4(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VABitPlaneBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceGroupMapBufferType:
        break;
    case VASliceParameterBufferType:
        va_TraceVASliceParameterBufferMPEG4(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAMacroblockParameterBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAResidualDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VADeblockingParameterBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAImageBufferType:
        break;
    case VAProtectedSliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncCodedBufferType:
        break;
    case VAEncSequenceParameterBufferType:
        va_TraceVAEncSequenceParameterBufferMPEG4(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncPictureParameterBufferType:
        va_TraceVAEncPictureParameterBufferMPEG4(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncSliceParameterBufferType:
        va_TraceVAEncSliceParameterBuffer(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    default:
        break;
    }
}


static void va_TraceH264Buf(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *pbuf
)
{
    DPY2INDEX(dpy);
    
    switch (type) {
    case VAPictureParameterBufferType:
        va_TraceVAPictureParameterBufferH264(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAIQMatrixBufferType:
        va_TraceVAIQMatrixBufferH264(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VABitPlaneBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);        
        break;
    case VASliceGroupMapBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceParameterBufferType:
        va_TraceVASliceParameterBufferH264(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, trace_context[idx].trace_slice_size, num_elements, pbuf);
        break;
    case VAMacroblockParameterBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);        
        break;
    case VAResidualDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);        
        break;
    case VADeblockingParameterBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAImageBufferType:
        break;
    case VAProtectedSliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncCodedBufferType:
        break;
    case VAEncSequenceParameterBufferType:
        if (size == sizeof(VAEncSequenceParameterBufferH264Baseline))
            va_TraceVAEncSequenceParameterBufferH264Baseline(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncPictureParameterBufferType:
        if (size == sizeof(VAEncPictureParameterBufferH264Baseline))
            va_TraceVAEncPictureParameterBufferH264Baseline(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncSliceParameterBufferType:
        va_TraceVAEncSliceParameterBuffer(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncMiscParameterBufferType:
        va_TraceVAEncMiscParameterBuffer(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    default:
        break;
    }
}


static void va_TraceVC1Buf(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *pbuf
)
{
    DPY2INDEX(dpy);

    switch (type) {
    case VAPictureParameterBufferType:
        va_TraceVAPictureParameterBufferVC1(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAIQMatrixBufferType:
        break;
    case VABitPlaneBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceGroupMapBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceParameterBufferType:
        va_TraceVASliceParameterBufferVC1(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, trace_context[idx].trace_slice_size, num_elements, pbuf);
        break;
    case VAMacroblockParameterBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAResidualDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VADeblockingParameterBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAImageBufferType:
        break;
    case VAProtectedSliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncCodedBufferType:
        break;
    case VAEncSequenceParameterBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncPictureParameterBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncSliceParameterBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    default:
        break;
    }
}

void va_TraceRenderPicture(
    VADisplay dpy,
    VAContextID context,
    VABufferID *buffers,
    int num_buffers
)
{
    VABufferType type;
    unsigned int size;
    unsigned int num_elements;
    int i;
    DPY2INDEX(dpy);

    TRACE_FUNCNAME(idx);
    
    va_TraceMsg(idx, "\tcontext = 0x%08x\n", context);
    va_TraceMsg(idx, "\tnum_buffers = %d\n", num_buffers);
    for (i = 0; i < num_buffers; i++) {
        unsigned char *pbuf;
        unsigned int j;
        
        /* get buffer type information */
        vaBufferInfo(dpy, context, buffers[i], &type, &size, &num_elements);

        va_TraceMsg(idx, "\t---------------------------\n");
        va_TraceMsg(idx, "\tbuffers[%d] = 0x%08x\n", i, buffers[i]);
        va_TraceMsg(idx, "\t  type = %s\n", buffer_type_to_string(type));
        va_TraceMsg(idx, "\t  size = %d\n", size);
        va_TraceMsg(idx, "\t  num_elements = %d\n", num_elements);

        vaMapBuffer(dpy, buffers[i], (void **)&pbuf);

        switch (trace_context[idx].trace_profile) {
        case VAProfileMPEG2Simple:
        case VAProfileMPEG2Main:
            for (j=0; j<num_elements; j++) {
                va_TraceMsg(idx, "\t---------------------------\n", j);
                va_TraceMsg(idx, "\telement[%d] = ", j);
                va_TraceMPEG2Buf(dpy, context, buffers[i], type, size, num_elements, pbuf + size*j);
            }
            break;
        case VAProfileMPEG4Simple:
        case VAProfileMPEG4AdvancedSimple:
        case VAProfileMPEG4Main:
            for (j=0; j<num_elements; j++) {
                va_TraceMsg(idx, "\t---------------------------\n", j);
                va_TraceMsg(idx, "\telement[%d] = ", j);
                va_TraceMPEG4Buf(dpy, context, buffers[i], type, size, num_elements, pbuf + size*j);
            }
            break;
        case VAProfileH264Baseline:
        case VAProfileH264Main:
        case VAProfileH264High:
        case VAProfileH264ConstrainedBaseline:
            for (j=0; j<num_elements; j++) {
                va_TraceMsg(idx, "\t---------------------------\n", j);
                va_TraceMsg(idx, "\telement[%d] = ", j);
                
                va_TraceH264Buf(dpy, context, buffers[i], type, size, num_elements, pbuf + size*j);
            }
            break;
        case VAProfileVC1Simple:
        case VAProfileVC1Main:
        case VAProfileVC1Advanced:
            for (j=0; j<num_elements; j++) {
                va_TraceMsg(idx, "\t---------------------------\n", j);
                va_TraceMsg(idx, "\telement[%d] = ", j);
                
                va_TraceVC1Buf(dpy, context, buffers[i], type, size, num_elements, pbuf + size*j);
            }
            break;
        case VAProfileH263Baseline:
            for (j=0; j<num_elements; j++) {
                va_TraceMsg(idx, "\t---------------------------\n", j);
                va_TraceMsg(idx, "\telement[%d] = ", j);
                
                va_TraceH263Buf(dpy, context, buffers[i], type, size, num_elements, pbuf + size*j);
            }
            break;
        case VAProfileJPEGBaseline:
            for (j=0; j<num_elements; j++) {
                va_TraceMsg(idx, "\t---------------------------\n", j);
                va_TraceMsg(idx, "\telement[%d] = ", j);
                
                va_TraceJPEGBuf(dpy, context, buffers[i], type, size, num_elements, pbuf + size*j);
            }
            break;
        default:
            break;
        }

        vaUnmapBuffer(dpy, buffers[i]);
    }

    va_TraceMsg(idx, NULL);
}

void va_TraceEndPicture(
    VADisplay dpy,
    VAContextID context,
    int endpic_done
)
{
    int encode, decode, jpeg;
    DPY2INDEX(dpy);

    TRACE_FUNCNAME(idx);

    if (endpic_done == 0) {
        va_TraceMsg(idx, "\tcontext = 0x%08x\n", context);
        va_TraceMsg(idx, "\trender_targets = 0x%08x\n", trace_context[idx].trace_rendertarget);
    }

    encode = (trace_context[idx].trace_entrypoint == VAEntrypointEncSlice) &&
        (trace_flag & VA_TRACE_FLAG_SURFACE_ENCODE);
    decode = (trace_context[idx].trace_entrypoint == VAEntrypointVLD) &&
        (trace_flag & VA_TRACE_FLAG_SURFACE_DECODE);
    jpeg = (trace_context[idx].trace_entrypoint == VAEntrypointEncPicture) &&
        (trace_flag & VA_TRACE_FLAG_SURFACE_JPEG);
    
    /* want to trace encode source surface, do it before vaEndPicture */
    if ((encode || jpeg) && (endpic_done == 0))
        va_TraceSurface(dpy);
    
    /* want to trace encoode codedbuf, do it after vaEndPicture */
    if ((encode || jpeg) && (endpic_done == 1)) {
        /* force the pipleline finish rendering */
        vaSyncSurface(dpy, trace_context[idx].trace_rendertarget);
        va_TraceCodedBuf(dpy);
    }

    /* want to trace decode dest surface, do it after vaEndPicture */
    if (decode && (endpic_done == 1)) {
        /* force the pipleline finish rendering */
        vaSyncSurface(dpy, trace_context[idx].trace_rendertarget);
        va_TraceSurface(dpy);
    }
    va_TraceMsg(idx, NULL);
}

void va_TraceSyncSurface(
    VADisplay dpy,
    VASurfaceID render_target
)
{
    DPY2INDEX(dpy);

    TRACE_FUNCNAME(idx);

    va_TraceMsg(idx, "\trender_target = 0x%08x\n", render_target);
    va_TraceMsg(idx, NULL);
}


void va_TraceQuerySurfaceStatus(
    VADisplay dpy,
    VASurfaceID render_target,
    VASurfaceStatus *status    /* out */
)
{
    DPY2INDEX(dpy);

    TRACE_FUNCNAME(idx);

    va_TraceMsg(idx, "\trender_target = 0x%08x\n", render_target);
    va_TraceMsg(idx, "\tstatus = 0x%08x\n", *status);
    va_TraceMsg(idx, NULL);
}


void va_TraceQuerySurfaceError(
    VADisplay dpy,
    VASurfaceID surface,
    VAStatus error_status,
    void **error_info       /*out*/
)
{
    DPY2INDEX(dpy);

    TRACE_FUNCNAME(idx);
    va_TraceMsg(idx, "\tsurface = 0x%08x\n", surface);
    va_TraceMsg(idx, "\terror_status = 0x%08x\n", error_status);
    if (error_status == VA_STATUS_ERROR_DECODING_ERROR) {
        VASurfaceDecodeMBErrors *p = *error_info;
        while (p->status != -1) {
            va_TraceMsg(idx, "\t\tstatus = %d\n", p->status);
            va_TraceMsg(idx, "\t\tstart_mb = %d\n", p->start_mb);
            va_TraceMsg(idx, "\t\tend_mb = %d\n", p->end_mb);
            p++; /* next error record */
        }
    }
    va_TraceMsg(idx, NULL);
}

void va_TraceMaxNumDisplayAttributes (
    VADisplay dpy,
    int number
)
{
    DPY2INDEX(dpy);

    TRACE_FUNCNAME(idx);
    
    va_TraceMsg(idx, "\tmax_display_attributes = %d\n", number);
    va_TraceMsg(idx, NULL);
}

void va_TraceQueryDisplayAttributes (
    VADisplay dpy,
    VADisplayAttribute *attr_list,    /* out */
    int *num_attributes               /* out */
)
{
    int i;
    
    DPY2INDEX(dpy);
    
    va_TraceMsg(idx, "\tnum_attributes = %d\n", *num_attributes);

    for (i=0; i<*num_attributes; i++) {
        va_TraceMsg(idx, "\tattr_list[%d] =\n");
        va_TraceMsg(idx, "\t  typ = 0x%08x\n", attr_list[i].type);
        va_TraceMsg(idx, "\t  min_value = %d\n", attr_list[i].min_value);
        va_TraceMsg(idx, "\t  max_value = %d\n", attr_list[i].max_value);
        va_TraceMsg(idx, "\t  value = %d\n", attr_list[i].value);
        va_TraceMsg(idx, "\t  flags = %d\n", attr_list[i].flags);
    }
    va_TraceMsg(idx, NULL);
}


static void va_TraceDisplayAttributes (
    VADisplay dpy,
    VADisplayAttribute *attr_list,
    int num_attributes
)
{
    int i;
    
    DPY2INDEX(dpy);
    
    va_TraceMsg(idx, "\tnum_attributes = %d\n", num_attributes);
    for (i=0; i<num_attributes; i++) {
        va_TraceMsg(idx, "\tattr_list[%d] =\n");
        va_TraceMsg(idx, "\t  typ = 0x%08x\n", attr_list[i].type);
        va_TraceMsg(idx, "\t  min_value = %d\n", attr_list[i].min_value);
        va_TraceMsg(idx, "\t  max_value = %d\n", attr_list[i].max_value);
        va_TraceMsg(idx, "\t  value = %d\n", attr_list[i].value);
        va_TraceMsg(idx, "\t  flags = %d\n", attr_list[i].flags);
    }
    va_TraceMsg(idx, NULL);
}


void va_TraceGetDisplayAttributes (
    VADisplay dpy,
    VADisplayAttribute *attr_list,
    int num_attributes
)
{
    DPY2INDEX(dpy);

    TRACE_FUNCNAME(idx);

    va_TraceDisplayAttributes (dpy, attr_list, num_attributes);
}

void va_TraceSetDisplayAttributes (
    VADisplay dpy,
    VADisplayAttribute *attr_list,
    int num_attributes
)
{
    DPY2INDEX(dpy);

    TRACE_FUNCNAME(idx);

    va_TraceDisplayAttributes (dpy, attr_list, num_attributes);
}


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
)
{
    DPY2INDEX(dpy);

    TRACE_FUNCNAME(idx);
    
    va_TraceMsg(idx, "\tsurface = 0x%08x\n", surface);
    va_TraceMsg(idx, "\tdraw = 0x%08x\n", draw);
    va_TraceMsg(idx, "\tsrcx = %d\n", srcx);
    va_TraceMsg(idx, "\tsrcy = %d\n", srcy);
    va_TraceMsg(idx, "\tsrcw = %d\n", srcw);
    va_TraceMsg(idx, "\tsrch = %d\n", srch);
    va_TraceMsg(idx, "\tdestx = %d\n", destx);
    va_TraceMsg(idx, "\tdesty = %d\n", desty);
    va_TraceMsg(idx, "\tdestw = %d\n", destw);
    va_TraceMsg(idx, "\tdesth = %d\n", desth);
    va_TraceMsg(idx, "\tcliprects = 0x%08x\n", cliprects);
    va_TraceMsg(idx, "\tnumber_cliprects = %d\n", number_cliprects);
    va_TraceMsg(idx, "\tflags = 0x%08x\n", flags);
    va_TraceMsg(idx, NULL);
}
