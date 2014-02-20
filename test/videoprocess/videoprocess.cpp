/*
 * Copyright (c) 2014 Intel Corporation. All Rights Reserved.
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
 * Video process test case based on LibVA.
 * This test covers deinterlace, denoise, color balance, sharpening,
 * blending, scaling and several surface format conversion.
 * Usage: videoprocess process.cfg
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <assert.h>
#include <va/va.h>
#include <va/va_vpp.h>
#include "va_display.h"

#ifndef VA_FOURCC_I420
#define VA_FOURCC_I420 0x30323449
#endif

#define MAX_LEN   1024

#define CHECK_VASTATUS(va_status,func)                                      \
  if (va_status != VA_STATUS_SUCCESS) {                                     \
      fprintf(stderr,"%s:%s (%d) failed,exit\n", __func__, func, __LINE__); \
      exit(1);                                                              \
  }

static VADisplay va_dpy = NULL;
static VAContextID context_id = 0;
static VAConfigID  config_id = 0;
static VAProcFilterType g_filter_type = VAProcFilterNone;
static VASurfaceID g_in_surface_id = VA_INVALID_ID;
static VASurfaceID g_out_surface_id = VA_INVALID_ID;

static FILE* g_config_file_fd = NULL;
static FILE* g_src_file_fd = NULL;
static FILE* g_dst_file_fd = NULL;

static char g_config_file_name[MAX_LEN];
static char g_src_file_name[MAX_LEN];
static char g_dst_file_name[MAX_LEN];
static char g_filter_type_name[MAX_LEN];

static uint32_t g_in_pic_width = 352;
static uint32_t g_in_pic_height = 288;
static uint32_t g_out_pic_width = 352;
static uint32_t g_out_pic_height = 288;

static uint32_t g_in_fourcc  = VA_FOURCC('N', 'V', '1', '2');
static uint32_t g_in_format  = VA_RT_FORMAT_YUV420;
static uint32_t g_out_fourcc = VA_FOURCC('N', 'V', '1', '2');
static uint32_t g_out_format = VA_RT_FORMAT_YUV420;

static uint8_t g_blending_enabled = 0;
static uint8_t g_blending_min_luma = 1;
static uint8_t g_blending_max_luma = 254;

static uint32_t g_frame_count = 0;

static int8_t
read_value_string(FILE *fp, const char* field_name, char* value)
{
    char strLine[MAX_LEN];
    char* field;
    char* str;
    uint16_t i;

    if (!fp || !field_name || !value)  {
        printf("Invalid fuction parameters\n");
        return -1;
    }

    rewind(fp);

    while (!feof(fp)) {
        if (!fgets(strLine, MAX_LEN, fp))
            continue;

        for (i = 0; strLine[i] && i < MAX_LEN; i++)
            if (strLine[i] != ' ') break;

        if (strLine[i] == '#' || strLine[i] == '\n' || i == 1024)
            continue;

        field = strtok(&strLine[i], ":");
        if (strncmp(field, field_name, strlen(field_name)))
            continue;

        if (!(str = strtok(NULL, ":")))
            continue;

        /* skip blank space in string */
        while (*str == ' ')
            str++;

        *(str + strlen(str)-1) = '\0';
        strcpy(value, str);

        return 0;
    }

    return -1;
}

static int8_t
read_value_uint8(FILE* fp, const char* field_name, uint8_t* value)
{
    char str[MAX_LEN];

    if (read_value_string(fp, field_name, str)) {
        printf("Failed to find integer field: %s", field_name);
        return -1;
    }

    *value = (uint8_t)atoi(str);
    return 0;
}

static int8_t
read_value_uint32(FILE* fp, const char* field_name, uint32_t* value)
{
    char str[MAX_LEN];

    if (read_value_string(fp, field_name, str)) {
       printf("Failed to find integer field: %s", field_name);
       return -1;
    }

    *value = (uint32_t)atoi(str);
    return 0;
}

static int8_t
read_value_float(FILE *fp, const char* field_name, float* value)
{
    char str[MAX_LEN];
    if (read_value_string(fp, field_name, str)) {
       printf("Failed to find float field: %s \n",field_name);
       return -1;
    }

    *value = atof(str);
    return 0;
}

static float
adjust_to_range(VAProcFilterValueRange *range, float value)
{
    if (value < range->min_value || value > range->max_value){
        printf("Value: %f exceed range: (%f ~ %f), force to use default: %f \n",
                value, range->min_value, range->max_value, range->default_value);
        return range->default_value;
    }

    return value;
}

static VAStatus
create_surface(VASurfaceID * p_surface_id,
               uint32_t width, uint32_t height,
               uint32_t fourCC, uint32_t format)
{
    VAStatus va_status;
    VASurfaceAttrib    surface_attrib;
    surface_attrib.type =  VASurfaceAttribPixelFormat;
    surface_attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    surface_attrib.value.type = VAGenericValueTypeInteger;
    surface_attrib.value.value.i = fourCC;

    va_status = vaCreateSurfaces(va_dpy,
                                 format,
                                 width ,
                                 height,
                                 p_surface_id,
                                 1,
                                 &surface_attrib,
                                 1);
   return va_status;
}

static VAStatus
construct_nv12_mask_surface(VASurfaceID surface_id,
                            uint8_t min_luma,
                            uint8_t max_luma)
{
    VAStatus va_status;
    VAImage surface_image;
    void *surface_p = NULL;
    unsigned char *y_dst, *u_dst, *v_dst;
    uint32_t row, col;

    va_status = vaDeriveImage(va_dpy, surface_id, &surface_image);
    CHECK_VASTATUS(va_status, "vaDeriveImage");

    va_status = vaMapBuffer(va_dpy, surface_image.buf, &surface_p);
    CHECK_VASTATUS(va_status, "vaMapBuffer");

    y_dst = (unsigned char *)((unsigned char*)surface_p + surface_image.offsets[0]);
    u_dst = (unsigned char *)((unsigned char*)surface_p + surface_image.offsets[1]);
    v_dst = u_dst;

    /* fill Y plane, the luma values of some pixels is in the range of min_luma~max_luma,
     * and others are out side of it, in luma key blending case, the pixels with Y value
     * exceeding the range will be hided*/
    for (row = 0; row < surface_image.height; row++) {
        if (row < surface_image.height / 4 || row > surface_image.height * 3 / 4)
            memset(y_dst, max_luma + 1, surface_image.pitches[0]);
        else
            memset(y_dst, (min_luma + max_luma) / 2, surface_image.pitches[0]);

        y_dst += surface_image.pitches[0];
     }

     /* fill UV plane */
     for (row = 0; row < surface_image.height / 2; row++) {
         for (col = 0; col < surface_image.width / 2; col++) {
             u_dst[col * 2] = 128;
             u_dst[col * 2 + 1] = 128;
        }
        u_dst += surface_image.pitches[1];
     }

    vaUnmapBuffer(va_dpy, surface_image.buf);
    vaDestroyImage(va_dpy, surface_image.image_id);

    return VA_STATUS_SUCCESS;
}

/* Load yv12 frame to NV12/YV12/I420 surface*/
static VAStatus
upload_yv12_frame_to_yuv_surface(FILE *fp,
                                 VASurfaceID surface_id)
{
    VAStatus va_status;
    VAImage surface_image;
    unsigned char *y_src, *u_src, *v_src;
    unsigned char *y_dst, *u_dst, *v_dst;
    void *surface_p = NULL;
    uint32_t frame_size, i, row, col;
    size_t n_items;
    unsigned char * newImageBuffer = NULL;

    va_status = vaDeriveImage(va_dpy, surface_id, &surface_image);
    CHECK_VASTATUS(va_status, "vaDeriveImage");

    va_status = vaMapBuffer(va_dpy, surface_image.buf, &surface_p);
    CHECK_VASTATUS(va_status, "vaMapBuffer");

    if (surface_image.format.fourcc == VA_FOURCC_YV12 ||
        surface_image.format.fourcc == VA_FOURCC_I420 ||
        surface_image.format.fourcc == VA_FOURCC_NV12){

        frame_size = surface_image.width * surface_image.height * 3 / 2;
        newImageBuffer = (unsigned char*)malloc(frame_size);
        do {
            n_items = fread(newImageBuffer, frame_size, 1, fp);
        } while (n_items != 1);

        y_src = newImageBuffer;
        v_src = newImageBuffer + surface_image.width * surface_image.height;
        u_src = newImageBuffer + surface_image.width * surface_image.height * 5 / 4;

        y_dst = (unsigned char *)((unsigned char*)surface_p + surface_image.offsets[0]);

        if(surface_image.format.fourcc == VA_FOURCC_YV12){
            v_dst = (unsigned char *)((unsigned char*)surface_p + surface_image.offsets[1]);
            u_dst = (unsigned char *)((unsigned char*)surface_p + surface_image.offsets[2]);
        }else if(surface_image.format.fourcc == VA_FOURCC_I420){
            u_dst = (unsigned char *)((unsigned char*)surface_p + surface_image.offsets[1]);
            v_dst = (unsigned char *)((unsigned char*)surface_p + surface_image.offsets[2]);
        }else {
            u_dst = (unsigned char *)((unsigned char*)surface_p + surface_image.offsets[1]);
            v_dst = u_dst;
        }

        /* Y plane, directly copy */
        for (row = 0; row < surface_image.height; row++) {
            memcpy(y_dst, y_src, surface_image.width);
            y_dst += surface_image.pitches[0];
            y_src += surface_image.width;
        }

        /* UV plane */
        if (surface_image.format.fourcc == VA_FOURCC_YV12||
            surface_image.format.fourcc == VA_FOURCC_I420){
            /* UV plane */
            for (row = 0; row < surface_image.height /2; row ++){
                memcpy(v_dst, v_src, surface_image.width/2);
                memcpy(u_dst, u_src, surface_image.width/2);

                v_src += surface_image.width/2;
                u_src += surface_image.width/2;

                if (surface_image.format.fourcc == VA_FOURCC_YV12){
                    v_dst += surface_image.pitches[1];
                    u_dst += surface_image.pitches[2];
                } else {
                    v_dst += surface_image.pitches[2];
                    u_dst += surface_image.pitches[1];
                }
            }
        } else if (surface_image.format.fourcc == VA_FOURCC_NV12){
            for (row = 0; row < surface_image.height / 2; row++) {
                for (col = 0; col < surface_image.width / 2; col++) {
                    u_dst[col * 2] = u_src[col];
                    u_dst[col * 2 + 1] = v_src[col];
                }

                u_dst += surface_image.pitches[1];
                u_src += (surface_image.width / 2);
                v_src += (surface_image.width / 2);
            }
        }
     } else {
         printf("Not supported YUV surface fourcc !!! \n");
         return VA_STATUS_ERROR_INVALID_SURFACE;
     }

     if (newImageBuffer){
         free(newImageBuffer);
         newImageBuffer = NULL;
     }

     vaUnmapBuffer(va_dpy, surface_image.buf);
     vaDestroyImage(va_dpy, surface_image.image_id);

     return VA_STATUS_SUCCESS;
}

/* Store NV12/YV12/I420 surface to yv12 frame*/
static VAStatus
store_yuv_surface_to_yv12_frame(FILE *fp,
                            VASurfaceID surface_id)
{
    VAStatus va_status;
    VAImageFormat image_format;
    VAImage surface_image;
    void *surface_p = NULL;
    unsigned char *y_src, *u_src, *v_src;
    unsigned char *y_dst, *u_dst, *v_dst;
    uint32_t frame_size, row, col;
    int32_t  ret, n_items;
    unsigned char * newImageBuffer = NULL;

    va_status = vaDeriveImage(va_dpy, surface_id, &surface_image);
    CHECK_VASTATUS(va_status, "vaDeriveImage");

    va_status = vaMapBuffer(va_dpy, surface_image.buf, &surface_p);
    CHECK_VASTATUS(va_status, "vaMapBuffer");

    /* store the surface to one YV12 file or one bmp file*/
    if (surface_image.format.fourcc == VA_FOURCC_YV12 ||
        surface_image.format.fourcc == VA_FOURCC_I420 ||
        surface_image.format.fourcc == VA_FOURCC_NV12){

        uint32_t y_size = surface_image.width * surface_image.height;
        uint32_t u_size = y_size/4;

        newImageBuffer = (unsigned char*)malloc(y_size * 3 / 2);

        /* stored as YV12 format */
        y_dst = newImageBuffer;
        v_dst = newImageBuffer + y_size;
        u_dst = newImageBuffer + y_size + u_size;

        y_src = (unsigned char *)((unsigned char*)surface_p + surface_image.offsets[0]);
        if (surface_image.format.fourcc == VA_FOURCC_YV12){
            v_src = (unsigned char *)((unsigned char*)surface_p + surface_image.offsets[1]);
            u_src = (unsigned char *)((unsigned char*)surface_p + surface_image.offsets[2]);
        } else if(surface_image.format.fourcc == VA_FOURCC_I420){
            u_src = (unsigned char *)((unsigned char*)surface_p + surface_image.offsets[1]);
            v_src = (unsigned char *)((unsigned char*)surface_p + surface_image.offsets[2]);
        } else if(surface_image.format.fourcc == VA_FOURCC_NV12){
            u_src = (unsigned char *)((unsigned char*)surface_p + surface_image.offsets[1]);
            v_src = u_src;
        }

        /* Y plane copy */
        for (row = 0; row < surface_image.height; row++) {
            memcpy(y_dst, y_src, surface_image.width);
            y_src += surface_image.pitches[0];
            y_dst += surface_image.width;
        }

        /* UV plane copy */
        if (surface_image.format.fourcc == VA_FOURCC_YV12||
            surface_image.format.fourcc == VA_FOURCC_I420){
            for (row = 0; row < surface_image.height /2; row ++){
                memcpy(v_dst, v_src, surface_image.width/2);
                memcpy(u_dst, u_src, surface_image.width/2);

                v_dst += surface_image.width/2;
                u_dst += surface_image.width/2;

                if (surface_image.format.fourcc == VA_FOURCC_YV12){
                    v_src += surface_image.pitches[1];
                    u_src += surface_image.pitches[2];
                 } else {
                    v_src += surface_image.pitches[2];
                    u_src += surface_image.pitches[1];
                 }
             }
         } else if (surface_image.format.fourcc == VA_FOURCC_NV12){
             for (row = 0; row < surface_image.height / 2; row++) {
                 for (col = 0; col < surface_image.width /2; col++) {
                     u_dst[col] = u_src[col * 2];
                     v_dst[col] = u_src[col * 2 + 1];
                  }

                  u_src += surface_image.pitches[1];
                  u_dst += (surface_image.width / 2);
                  v_dst += (surface_image.width / 2);
             }
         }

         /* write frame to file */
         do {
             n_items = fwrite(newImageBuffer, y_size * 3 / 2, 1, fp);
         } while (n_items != 1);

     } else {
         printf("Not supported YUV surface fourcc !!! \n");
         return VA_STATUS_ERROR_INVALID_SURFACE;
     }

     if (newImageBuffer){
         free(newImageBuffer);
         newImageBuffer = NULL;
     }

     vaUnmapBuffer(va_dpy, surface_image.buf);
     vaDestroyImage(va_dpy, surface_image.image_id);

     return VA_STATUS_SUCCESS;
}

static VAStatus
denoise_filter_init(VABufferID *filter_param_buf_id)
{
    VAStatus va_status = VA_STATUS_SUCCESS;
    VAProcFilterParameterBuffer denoise_param;
    VABufferID denoise_param_buf_id;
    float intensity;

    VAProcFilterCap denoise_caps;
    uint32_t num_denoise_caps = 1;
    va_status = vaQueryVideoProcFilterCaps(va_dpy, context_id,
                                           VAProcFilterNoiseReduction,
                                           &denoise_caps, &num_denoise_caps);
    CHECK_VASTATUS(va_status,"vaQueryVideoProcFilterCaps");

    if (read_value_float(g_config_file_fd, "DENOISE_INTENSITY", &intensity)) {
        printf("Read denoise intensity failed, use default value");
        intensity = denoise_caps.range.default_value;
    }
    intensity = adjust_to_range(&denoise_caps.range, intensity);

    denoise_param.type  = VAProcFilterNoiseReduction;
    denoise_param.value = intensity;

    printf("Denoise intensity: %f\n", intensity);

    va_status = vaCreateBuffer(va_dpy, context_id,
                               VAProcFilterParameterBufferType, sizeof(denoise_param), 1,
                               &denoise_param, &denoise_param_buf_id);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");

    *filter_param_buf_id = denoise_param_buf_id;

    return va_status;
}

static VAStatus
deinterlace_filter_init(VABufferID *filter_param_buf_id)
{
    VAStatus va_status = VA_STATUS_SUCCESS;
    VAProcFilterParameterBufferDeinterlacing deinterlacing_param;
    VABufferID deinterlacing_param_buf_id;
    char algorithm_str[MAX_LEN], flags_str[MAX_LEN];
    uint32_t i;

    /* read and check whether configured deinterlace algorithm is supported */
    deinterlacing_param.algorithm  = VAProcDeinterlacingBob;
    if (!read_value_string(g_config_file_fd, "DEINTERLACING_ALGORITHM", algorithm_str)) {
        printf("Deinterlacing algorithm in config: %s \n", algorithm_str);
        if (!strcmp(algorithm_str, "VAProcDeinterlacingBob"))
            deinterlacing_param.algorithm  = VAProcDeinterlacingBob;
        else if (!strcmp(algorithm_str, "VAProcDeinterlacingWeave"))
            deinterlacing_param.algorithm  = VAProcDeinterlacingWeave;
        else if (!strcmp(algorithm_str, "VAProcDeinterlacingMotionAdaptive"))
            deinterlacing_param.algorithm  = VAProcDeinterlacingMotionAdaptive;
        else if (!strcmp(algorithm_str, "VAProcDeinterlacingMotionCompensated"))
            deinterlacing_param.algorithm  = VAProcDeinterlacingMotionCompensated;
    } else {
        printf("Read deinterlace algorithm failed, use default algorithm");
        deinterlacing_param.algorithm  = VAProcDeinterlacingBob;
    }

    VAProcFilterCapDeinterlacing deinterlacing_caps[VAProcDeinterlacingCount];
    uint32_t num_deinterlacing_caps = VAProcDeinterlacingCount;
    va_status = vaQueryVideoProcFilterCaps(va_dpy, context_id,
                                           VAProcFilterDeinterlacing,
                                           &deinterlacing_caps, &num_deinterlacing_caps);
    CHECK_VASTATUS(va_status,"vaQueryVideoProcFilterCaps");

    for (i = 0; i < VAProcDeinterlacingCount; i ++)
       if (deinterlacing_caps[i].type == deinterlacing_param.algorithm)
         break;

    if (i == VAProcDeinterlacingCount) {
        printf("Deinterlacing algorithm: %d is not supported by driver, \
                use defautl algorithm :%d \n",
                deinterlacing_param.algorithm,
                VAProcDeinterlacingBob);
        deinterlacing_param.algorithm = VAProcDeinterlacingBob;
    }

    /* read and check the deinterlace flags */
    deinterlacing_param.flags = 0;
    if (!read_value_string(g_config_file_fd, "DEINTERLACING_FLAG", flags_str)) {
        if (strstr(flags_str, "VA_DEINTERLACING_BOTTOM_FIELD_FIRST"))
            deinterlacing_param.flags |= VA_DEINTERLACING_BOTTOM_FIELD_FIRST;
        if (strstr(flags_str, "VA_DEINTERLACING_BOTTOM_FIELD"))
            deinterlacing_param.flags |= VA_DEINTERLACING_BOTTOM_FIELD;
        if (strstr(flags_str, "VA_DEINTERLACING_ONE_FIELD"))
            deinterlacing_param.flags |= VA_DEINTERLACING_ONE_FIELD;
    }

    deinterlacing_param.type  = VAProcFilterDeinterlacing;

    /* create deinterlace fitler buffer */
    va_status = vaCreateBuffer(va_dpy, context_id,
                               VAProcFilterParameterBufferType, sizeof(deinterlacing_param), 1,
                               &deinterlacing_param, &deinterlacing_param_buf_id);
    CHECK_VASTATUS(va_status, "vaCreateBuffer");

    *filter_param_buf_id = deinterlacing_param_buf_id;

    return va_status;
}

static VAStatus
sharpening_filter_init(VABufferID *filter_param_buf_id)
{
    VAStatus va_status;
    VAProcFilterParameterBuffer sharpening_param;
    VABufferID sharpening_param_buf_id;
    float intensity;

    VAProcFilterCap sharpening_caps;
    uint32_t num_sharpening_caps = 1;
    va_status = vaQueryVideoProcFilterCaps(va_dpy, context_id,
                VAProcFilterSharpening,
                &sharpening_caps, &num_sharpening_caps);
    CHECK_VASTATUS(va_status,"vaQueryVideoProcFilterCaps");

    if(read_value_float(g_config_file_fd, "SHARPENING_INTENSITY", &intensity)) {
        printf("Read sharpening intensity failed, use default value.");
        intensity = sharpening_caps.range.default_value;
    }

    intensity = adjust_to_range(&sharpening_caps.range, intensity);
    printf("Sharpening intensity: %f\n", intensity);
    sharpening_param.value = intensity;

    sharpening_param.type  = VAProcFilterSharpening;

    /* create sharpening fitler buffer */
    va_status = vaCreateBuffer(va_dpy, context_id,
                               VAProcFilterParameterBufferType, sizeof(sharpening_param), 1,
                               &sharpening_param, &sharpening_param_buf_id);

    *filter_param_buf_id = sharpening_param_buf_id;

    return va_status;
}

static VAStatus
color_balance_filter_init(VABufferID *filter_param_buf_id)
{
    VAStatus va_status;
    VAProcFilterParameterBufferColorBalance color_balance_param[4];
    VABufferID color_balance_param_buf_id;
    float value;
    uint32_t i, count;
    int8_t status;

    VAProcFilterCapColorBalance color_balance_caps[VAProcColorBalanceCount];
    unsigned int num_color_balance_caps = VAProcColorBalanceCount;
    va_status = vaQueryVideoProcFilterCaps(va_dpy, context_id,
                                           VAProcFilterColorBalance,
                                           &color_balance_caps, &num_color_balance_caps);
    CHECK_VASTATUS(va_status,"vaQueryVideoProcFilterCaps");

    count = 0;
    printf("Color balance params: ");
    for (i = 0; i < num_color_balance_caps; i++) {
        if (color_balance_caps[i].type == VAProcColorBalanceHue) {
            color_balance_param[count].attrib  = VAProcColorBalanceHue;
            status = read_value_float(g_config_file_fd, "COLOR_BALANCE_HUE", &value);
            printf("Hue: ");
        } else if (color_balance_caps[i].type == VAProcColorBalanceSaturation) {
            color_balance_param[count].attrib  = VAProcColorBalanceSaturation;
            status = read_value_float(g_config_file_fd, "COLOR_BALANCE_SATURATION", &value);
            printf("Saturation: ");
        } else if (color_balance_caps[i].type == VAProcColorBalanceBrightness) {
            color_balance_param[count].attrib  = VAProcColorBalanceBrightness;
            status = read_value_float(g_config_file_fd, "COLOR_BALANCE_BRIGHTNESS", &value);
            printf("Brightness: ");
        } else if (color_balance_caps[i].type == VAProcColorBalanceContrast) {
            color_balance_param[count].attrib  = VAProcColorBalanceContrast;
            status = read_value_float(g_config_file_fd, "COLOR_BALANCE_CONTRAST", &value);
            printf("Contrast: ");
        } else {
            continue;
        }

        if (status)
            value = color_balance_caps[i].range.default_value;
        else
            value = adjust_to_range(&color_balance_caps[i].range, value);

        color_balance_param[count].value = value;
        color_balance_param[count].type  = VAProcFilterColorBalance;
        count++;

        printf("%4f,  ", value);
    }
    printf("\n");

    va_status = vaCreateBuffer(va_dpy, context_id,
                               VAProcFilterParameterBufferType, sizeof(color_balance_param), 4,
                               color_balance_param, &color_balance_param_buf_id);

    *filter_param_buf_id = color_balance_param_buf_id;

    return va_status;
}

static VAStatus
blending_state_init(VABlendState *state)
{
    VAStatus va_status = VA_STATUS_SUCCESS;
    char blending_flags_str[MAX_LEN];
    float global_alpha;
    uint32_t min_luma, max_luma;

    /* read and check blend state */
    state->flags = 0;
    if (!read_value_string(g_config_file_fd, "BLENDING_FLAGS", blending_flags_str)){
        if (strstr(blending_flags_str, "VA_BLEND_GLOBAL_ALPHA")) {
           if (read_value_float(g_config_file_fd, "BLENDING_GLOBAL_ALPHA", &global_alpha)) {
               global_alpha = 1.0  ;
               printf("Use default global alpha : %4f \n", global_alpha);
           }
           state->flags |= VA_BLEND_GLOBAL_ALPHA;
           state->global_alpha = global_alpha;
        }
        if (strstr(blending_flags_str, "VA_BLEND_LUMA_KEY")) {
            if (read_value_uint8(g_config_file_fd, "BLENDING_MIN_LUMA", &g_blending_min_luma)) {
                g_blending_min_luma = 1;
                printf("Use default min luma : %3d \n", g_blending_min_luma);
            }
            if (read_value_uint8(g_config_file_fd, "BLENDING_MAX_LUMA", &g_blending_max_luma)) {
                g_blending_max_luma = 254;
                printf("Use default max luma : %3d \n", g_blending_max_luma);
            }
            state->flags |= VA_BLEND_LUMA_KEY;
            state->min_luma = g_blending_min_luma * 1.0 / 256;
            state->max_luma = g_blending_max_luma * 1.0 / 256;
        }

        printf("Blending type = %s, alpha = %f, min_luma = %3d, max_luma = %3d \n",
              blending_flags_str, global_alpha, min_luma, max_luma);
    }

    VAProcPipelineCaps pipeline_caps;
    va_status = vaQueryVideoProcPipelineCaps(va_dpy, context_id,
                NULL, 0, &pipeline_caps);
    CHECK_VASTATUS(va_status,"vaQueryVideoProcPipelineCaps");

    if (!pipeline_caps.blend_flags){
        printf("Blending is not supported in driver! \n");
        return VA_STATUS_ERROR_UNIMPLEMENTED;
    }

    if (! (pipeline_caps.blend_flags & state->flags)) {
        printf("Driver do not support current blending flags: %d", state->flags);
        return VA_STATUS_ERROR_UNIMPLEMENTED;
    }

    return va_status;
}

static VAStatus
video_frame_process(VAProcFilterType filter_type,
                    uint32_t frame_idx,
                    VASurfaceID in_surface_id,
                    VASurfaceID out_surface_id)
{
    VAStatus va_status;
    VAProcPipelineParameterBuffer pipeline_param;
    VARectangle surface_region, output_region;
    VABufferID pipeline_param_buf_id = VA_INVALID_ID;
    VABufferID filter_param_buf_id = VA_INVALID_ID;
    VABlendState state ;
    uint32_t filter_count = 1;

    /* create denoise_filter buffer id */
    switch(filter_type){
      case VAProcFilterNoiseReduction:
           denoise_filter_init(&filter_param_buf_id);
           break;
      case VAProcFilterDeinterlacing:
           deinterlace_filter_init(&filter_param_buf_id);
           break;
      case VAProcFilterSharpening:
           sharpening_filter_init(&filter_param_buf_id);
           break;
      case VAProcFilterColorBalance:
           color_balance_filter_init(&filter_param_buf_id);
           break;
      default :
           filter_count = 0;
         break;
    }

    /* Fill pipeline buffer */
    surface_region.x = 0;
    surface_region.y = 0;
    surface_region.width = g_in_pic_width;
    surface_region.height = g_in_pic_height;
    output_region.x = 0;
    output_region.y = 0;
    output_region.width = g_out_pic_width;
    output_region.height = g_out_pic_height;

    memset(&pipeline_param, 0, sizeof(pipeline_param));
    pipeline_param.surface = in_surface_id;
    pipeline_param.surface_region = &surface_region;
    pipeline_param.output_region = &output_region;

    pipeline_param.filter_flags = 0;
    pipeline_param.filters      = &filter_param_buf_id;
    pipeline_param.num_filters  = filter_count;

    /* Blending related state */
    if (g_blending_enabled){
        blending_state_init(&state);
        pipeline_param.blend_state = &state;
    }

    va_status = vaCreateBuffer(va_dpy,
                               context_id,
                               VAProcPipelineParameterBufferType,
                               sizeof(pipeline_param),
                               1,
                               &pipeline_param,
                               &pipeline_param_buf_id);
    CHECK_VASTATUS(va_status, "vaCreateBuffer");

    va_status = vaBeginPicture(va_dpy,
                               context_id,
                               out_surface_id);
    CHECK_VASTATUS(va_status, "vaBeginPicture");

    va_status = vaRenderPicture(va_dpy,
                                context_id,
                                &pipeline_param_buf_id,
                                1);
    CHECK_VASTATUS(va_status, "vaRenderPicture");

    va_status = vaEndPicture(va_dpy, context_id);
    CHECK_VASTATUS(va_status, "vaEndPicture");

    if (filter_param_buf_id != VA_INVALID_ID)
        vaDestroyBuffer(va_dpy,filter_param_buf_id);

    if (pipeline_param_buf_id != VA_INVALID_ID)
        vaDestroyBuffer(va_dpy,pipeline_param_buf_id);

    return va_status;
}

static VAStatus
vpp_context_create()
{
    VAStatus va_status = VA_STATUS_SUCCESS;
    uint32_t i;

    /* VA driver initialization */
    va_dpy = va_open_display();
    int32_t major_ver, minor_ver;
    va_status = vaInitialize(va_dpy, &major_ver, &minor_ver);
    assert(va_status == VA_STATUS_SUCCESS);

    /* Check whether VPP is supported by driver */
    VAEntrypoint entrypoints[5];
    int32_t num_entrypoints;
    num_entrypoints = vaMaxNumEntrypoints(va_dpy);
    va_status = vaQueryConfigEntrypoints(va_dpy,
                                         VAProfileNone,
                                         entrypoints,
                                         &num_entrypoints);
    CHECK_VASTATUS(va_status, "vaQueryConfigEntrypoints");

    for	(i = 0; i < num_entrypoints; i++) {
        if (entrypoints[i] == VAEntrypointVideoProc)
            break;
    }

    if (i == num_entrypoints) {
        printf("VPP is not supported by driver\n");
        assert(0);
    }

    /* Render target surface format check */
    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRTFormat;
    va_status = vaGetConfigAttributes(va_dpy,
                                      VAProfileNone,
                                      VAEntrypointVideoProc,
                                      &attrib,
                                     1);
    CHECK_VASTATUS(va_status, "vaGetConfigAttributes");
    if ((attrib.value != g_out_format)) {
        printf("RT format %d is not supported by VPP !\n",g_out_format);
        assert(0);
    }

    /* Create surface/config/context for VPP pipeline */
    va_status = create_surface(&g_in_surface_id, g_in_pic_width, g_in_pic_height,
                                g_in_fourcc, g_in_format);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces for input");

    va_status = create_surface(&g_out_surface_id, g_out_pic_width, g_out_pic_height,
                                g_out_fourcc, g_out_format);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces for output");

    va_status = vaCreateConfig(va_dpy,
                               VAProfileNone,
                               VAEntrypointVideoProc,
                               &attrib,
                               1,
                               &config_id);
    CHECK_VASTATUS(va_status, "vaCreateConfig");

    /* Source surface format check */
    uint32_t num_surf_attribs = VASurfaceAttribCount;
    VASurfaceAttrib * surf_attribs = (VASurfaceAttrib*)
              malloc(sizeof(VASurfaceAttrib) * num_surf_attribs);
    if (!surf_attribs)
       assert(0);

    va_status = vaQuerySurfaceAttributes(va_dpy,
                                        config_id,
                                        surf_attribs,
                                        &num_surf_attribs);

    if (va_status == VA_STATUS_ERROR_MAX_NUM_EXCEEDED) {
        surf_attribs = (VASurfaceAttrib*)realloc(surf_attribs,
                        sizeof(VASurfaceAttrib) * num_surf_attribs);
         if (!surf_attribs)
             assert(0);
         va_status = vaQuerySurfaceAttributes(va_dpy,
                                              config_id,
                                              surf_attribs,
                                              &num_surf_attribs);
    }
    CHECK_VASTATUS(va_status, "vaQuerySurfaceAttributes");

    for (i = 0; i < num_surf_attribs; i++) {
        if (surf_attribs[i].type == VASurfaceAttribPixelFormat &&
            surf_attribs[i].value.value.i == g_in_fourcc)
            break;
    }
    free(surf_attribs);

    if (i == num_surf_attribs) {
        printf("Input fourCC %d  is not supported by VPP !\n", g_in_fourcc);
        assert(0);
    }

    va_status = vaCreateContext(va_dpy,
                                config_id,
                                g_out_pic_width,
                                g_out_pic_height,
                                VA_PROGRESSIVE,
                                &g_out_surface_id,
                                1,
                                &context_id);
    CHECK_VASTATUS(va_status, "vaCreateContext");


    /* Validate  whether currect filter is supported */
    if (g_filter_type != VAProcFilterNone) {
        uint32_t supported_filter_num = VAProcFilterCount;
        VAProcFilterType supported_filter_types[VAProcFilterCount];

        va_status = vaQueryVideoProcFilters(va_dpy,
                                            context_id,
                                            supported_filter_types,
                                            &supported_filter_num);

        CHECK_VASTATUS(va_status, "vaQueryVideoProcFilters");

        for (i = 0; i < supported_filter_num; i++){
            if (supported_filter_types[i] == g_filter_type)
                break;
        }

        if (i == supported_filter_num) {
            printf("VPP filter type %s is not supported by driver !\n", g_filter_type_name);
            assert(0);
        }
    }

    return va_status;
}

static void
vpp_context_destroy()
{
    /* Release resource */
    vaDestroySurfaces(va_dpy, &g_in_surface_id, 1);
    vaDestroySurfaces(va_dpy, &g_out_surface_id, 1);
    vaDestroyContext(va_dpy, context_id);
    vaDestroyConfig(va_dpy, config_id);

    vaTerminate(va_dpy);
    va_close_display(va_dpy);
}

static int8_t
parse_fourcc_and_format(char *str, uint32_t *fourcc, uint32_t *format)
{
    if (!strcmp(str, "YV12")){
        *fourcc = VA_FOURCC('Y', 'V', '1', '2');
        *format = VA_RT_FORMAT_YUV420;
    } else if(!strcmp(str, "I420")){
        *fourcc = VA_FOURCC('I', '4', '2', '0');
        *format = VA_RT_FORMAT_YUV420;
    } else if(!strcmp(str, "NV12")){
        *fourcc = VA_FOURCC('N', 'V', '1', '2');
        *format = VA_RT_FORMAT_YUV420;
    } else{
        printf("Not supported format: %s! Currently only support following format: %s\n",
         str, "YV12, I420, NV12");
        assert(0);
    }
    return 0;
}

static int8_t
parse_basic_parameters()
{
    char str[MAX_LEN];

    /* Read src frame file information */
    read_value_string(g_config_file_fd, "SRC_FILE_NAME", g_src_file_name);
    read_value_uint32(g_config_file_fd, "SRC_FRAME_WIDTH", &g_in_pic_width);
    read_value_uint32(g_config_file_fd, "SRC_FRAME_HEIGHT", &g_in_pic_height);
    read_value_string(g_config_file_fd, "SRC_FRAME_FORMAT", str);
    parse_fourcc_and_format(str, &g_in_fourcc, &g_in_format);

    /* Read dst frame file information */
    read_value_string(g_config_file_fd, "DST_FILE_NAME", g_dst_file_name);
    read_value_uint32(g_config_file_fd, "DST_FRAME_WIDTH", &g_out_pic_width);
    read_value_uint32(g_config_file_fd, "DST_FRAME_HEIGHT",&g_out_pic_height);
    read_value_string(g_config_file_fd, "DST_FRAME_FORMAT", str);
    parse_fourcc_and_format(str, &g_out_fourcc, &g_out_format);

    read_value_uint32(g_config_file_fd, "FRAME_SUM", &g_frame_count);

    /* Read filter type */
    if (read_value_string(g_config_file_fd, "FILTER_TYPE", g_filter_type_name)){
        printf("Read filter type error !\n");
        assert(0);
    }

    if (!strcmp(g_filter_type_name, "VAProcFilterNoiseReduction"))
        g_filter_type = VAProcFilterNoiseReduction;
    else if (!strcmp(g_filter_type_name, "VAProcFilterDeinterlacing"))
        g_filter_type = VAProcFilterDeinterlacing;
    else if (!strcmp(g_filter_type_name, "VAProcFilterSharpening"))
        g_filter_type = VAProcFilterSharpening;
    else if (!strcmp(g_filter_type_name, "VAProcFilterColorBalance"))
        g_filter_type = VAProcFilterColorBalance;
    else if (!strcmp(g_filter_type_name, "VAProcFilterNone"))
        g_filter_type = VAProcFilterNone;
    else {
        printf("Unsupported filter type :%s \n", g_filter_type_name);
        return -1;
    }

    /* Check whether blending is enabled */
    if (read_value_uint8(g_config_file_fd, "BLENDING_ENABLED", &g_blending_enabled))
        g_blending_enabled = 0;

    if (g_blending_enabled)
        printf("Blending will be done \n");

    if (g_in_pic_width != g_out_pic_width ||
        g_in_pic_height != g_out_pic_height)
        printf("Scaling will be done : from %4d x %4d to %4d x %4d \n",
                g_in_pic_width, g_in_pic_height,
                g_out_pic_width, g_out_pic_height);

    if (g_in_fourcc != g_out_fourcc)
        printf("Format conversion will be done: from %d to %d \n",
               g_in_fourcc, g_out_fourcc);

    return 0;
}

int32_t main(int32_t argc, char *argv[])
{
    VAStatus va_status;
    uint32_t i;

    if (argc != 2){
        printf("Input error! please specify the configure file \n");
        return -1;
    }

    /* Parse the configure file for video process*/
    strcpy(g_config_file_name, argv[1]);
    if (NULL == (g_config_file_fd = fopen(g_config_file_name, "r"))){
        printf("Open configure file %s failed!\n",g_config_file_name);
        assert(0);
    }

    /* Parse basic parameters */
    if (parse_basic_parameters()){
        printf("Parse parameters in configure file error\n");
        assert(0);
    }

    va_status = vpp_context_create();
    if (va_status != VA_STATUS_SUCCESS) {
        printf("vpp context create failed \n");
        assert(0);
    }

    /* Video frame fetch, process and store */
    if (NULL == (g_src_file_fd = fopen(g_src_file_name, "r"))){
        printf("Open SRC_FILE_NAME: %s failed, please specify it in config file: %s !\n",
                g_src_file_name, g_config_file_name);
        assert(0);
    }

    if (NULL == (g_dst_file_fd = fopen(g_dst_file_name, "w"))){
        printf("Open DST_FILE_NAME: %s failed, please specify it in config file: %s !\n",
               g_dst_file_name, g_config_file_name);
        assert(0);
    }

    printf("\nStart to process, processing type is %s ...\n", g_filter_type_name);
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    for (i = 0; i < g_frame_count; i ++){
        if (g_blending_enabled) {
            construct_nv12_mask_surface(g_in_surface_id, g_blending_min_luma, g_blending_max_luma);
            upload_yv12_frame_to_yuv_surface(g_src_file_fd, g_out_surface_id);
        } else {
            upload_yv12_frame_to_yuv_surface(g_src_file_fd, g_in_surface_id);
        }

        video_frame_process(g_filter_type, i, g_in_surface_id, g_out_surface_id);
        store_yuv_surface_to_yv12_frame(g_dst_file_fd, g_out_surface_id);
    }

    gettimeofday(&end_time, NULL);
    float duration = (end_time.tv_sec - start_time.tv_sec) +
                     (end_time.tv_usec - start_time.tv_usec)/1000000.0;
    printf("Finish processing, performance: \n" );
    printf("%d frames processed in: %f s, ave time = %.6fs \n",g_frame_count, duration, duration/g_frame_count);

    if (g_src_file_fd)
       fclose(g_src_file_fd);

    if (g_dst_file_fd)
       fclose(g_dst_file_fd);

    if (g_config_file_fd)
       fclose(g_config_file_fd);

    vpp_context_destroy();

    return 0;
}

