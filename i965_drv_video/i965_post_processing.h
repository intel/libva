/*
 * Copyright © 2010 Intel Corporation
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
 *
 * Authors:
 *    Xiang Haihao <haihao.xiang@intel.com>
 *
 */

#ifndef __I965_POST_PROCESSING_H__
#define __I965_POST_PROCESSING_H__

#define MAX_PP_SURFACES 32

#define I965_PP_FLAG_DEINTERLACING      1
#define I965_PP_FLAG_AVS                2

enum
{
    PP_NULL = 0,
    PP_NV12_LOAD_SAVE,
    PP_NV12_SCALING,
    PP_NV12_AVS,
    PP_NV12_DNDI,
};

struct pp_load_save_context
{
    int dest_w;
    int dest_h;
};

struct pp_scaling_context
{
    int dest_w;
    int dest_h;
};

struct pp_avs_context
{
    int dest_w;
    int dest_h;
    int src_w;
    int src_h;
};

struct pp_dndi_context
{
    int dest_w;
    int dest_h;

};

struct i965_post_processing_context
{
    int current_pp;

    struct {
        dri_bo *bo;
    } curbe;

    struct {
        dri_bo *ss_bo;
        dri_bo *s_bo;
    } surfaces[MAX_PP_SURFACES];

    struct {
        dri_bo *bo;
    } binding_table;

    struct {
        dri_bo *bo;
        int num_interface_descriptors;
    } idrt;

    struct {
        dri_bo *bo;
    } vfe_state;

    struct {
        dri_bo *bo;
        dri_bo *bo_8x8;
        dri_bo *bo_8x8_uv;
    } sampler_state_table;

    struct {
        unsigned int size;

        unsigned int vfe_start;
        unsigned int cs_start;

        unsigned int num_vfe_entries;
        unsigned int num_cs_entries;

        unsigned int size_vfe_entry;
        unsigned int size_cs_entry;
    } urb;

    struct {
        dri_bo *bo;
    } stmm;

    union {
        struct pp_load_save_context pp_load_save_context;
        struct pp_scaling_context pp_scaling_context;
        struct pp_avs_context pp_avs_context;
        struct pp_dndi_context pp_dndi_context;
    } private_context;

    int (*pp_x_steps)(void *private_context);
    int (*pp_y_steps)(void *private_context);
    int (*pp_set_block_parameter)(void *private_context, int x, int y);
};

void
i965_post_processing(VADriverContextP ctx,
                     VASurfaceID surface,
                     short srcx,
                     short srcy,
                     unsigned short srcw,
                     unsigned short srch,
                     short destx,
                     short desty,
                     unsigned short destw,
                     unsigned short desth,
                     unsigned int pp_index);
Bool
i965_post_processing_terminate(VADriverContextP ctx);
Bool
i965_post_processing_init(VADriverContextP ctx);

#endif /* __I965_POST_PROCESSING_H__ */
