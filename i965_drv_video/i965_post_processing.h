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

#define NUM_PP_MODULES                  5

struct pp_load_save_context
{
    int dest_w;
    int dest_h;
};

struct pp_scaling_context
{
    int dest_x; /* in pixel */
    int dest_y; /* in pixel */
    int dest_w;
    int dest_h;
    int src_normalized_x;
    int src_normalized_y;
};

struct pp_avs_context
{
    int dest_x; /* in pixel */
    int dest_y; /* in pixel */
    int dest_w;
    int dest_h;
    int src_normalized_x;
    int src_normalized_y;
    int src_w;
    int src_h;
};

struct pp_dndi_context
{
    int dest_w;
    int dest_h;
};

struct pp_module
{
    struct i965_kernel kernel;
    
    /* others */
    void (*initialize)(VADriverContextP ctx, 
                       VASurfaceID in_surface_id, VASurfaceID out_surface_id,
                       const VARectangle *src_rect, const VARectangle *dst_rect);
};

struct pp_static_parameter
{
    struct {
        /* Procamp r1.0 */
        float procamp_constant_c0;
        
        /* Load and Same r1.1 */
        unsigned int source_packed_y_offset:8;
        unsigned int source_packed_u_offset:8;
        unsigned int source_packed_v_offset:8;
        unsigned int pad0:8;

        union {
            /* Load and Save r1.2 */
            struct {
                unsigned int destination_packed_y_offset:8;
                unsigned int destination_packed_u_offset:8;
                unsigned int destination_packed_v_offset:8;
                unsigned int pad0:8;
            } load_and_save;

            /* CSC r1.2 */
            struct {
                unsigned int destination_rgb_format:8;
                unsigned int pad0:24;
            } csc;
        } r1_2;
        
        /* Procamp r1.3 */
        float procamp_constant_c1;

        /* Procamp r1.4 */
        float procamp_constant_c2;

        /* DI r1.5 */
        unsigned int statistics_surface_picth:16;  /* Devided by 2 */
        unsigned int pad1:16;

        union {
            /* DI r1.6 */
            struct {
                unsigned int pad0:24;
                unsigned int top_field_first:8;
            } di;

            /* AVS/Scaling r1.6 */
            float normalized_video_y_scaling_step;
        } r1_6;

        /* Procamp r1.7 */
        float procamp_constant_c5;
    } grf1;
    
    struct {
        /* Procamp r2.0 */
        float procamp_constant_c3;

        /* MBZ r2.1*/
        unsigned int pad0;

        /* WG+CSC r2.2 */
        float wg_csc_constant_c4;

        /* WG+CSC r2.3 */
        float wg_csc_constant_c8;

        /* Procamp r2.4 */
        float procamp_constant_c4;

        /* MBZ r2.5 */
        unsigned int pad1;

        /* MBZ r2.6 */
        unsigned int pad2;

        /* WG+CSC r2.7 */
        float wg_csc_constant_c9;
    } grf2;

    struct {
        /* WG+CSC r3.0 */
        float wg_csc_constant_c0;

        /* Blending r3.1 */
        float scaling_step_ratio;

        /* Blending r3.2 */
        float normalized_alpha_y_scaling;
        
        /* WG+CSC r3.3 */
        float wg_csc_constant_c4;

        /* WG+CSC r3.4 */
        float wg_csc_constant_c1;

        /* ALL r3.5 */
        int horizontal_origin_offset:16;
        int vertical_origin_offset:16;

        /* Shared r3.6*/
        union {
            /* Color filll */
            unsigned int color_pixel;

            /* WG+CSC */
            float wg_csc_constant_c2;
        } r3_6;

        /* WG+CSC r3.7 */
        float wg_csc_constant_c3;
    } grf3;

    struct {
        /* WG+CSC r4.0 */
        float wg_csc_constant_c6;

        /* ALL r4.1 MBZ ???*/
        unsigned int pad0;

        /* Shared r4.2 */
        union {
            /* AVS */
            struct {
                unsigned int pad1:15;
                unsigned int nlas:1;
                unsigned int pad2:16;
            } avs;

            /* DI */
            struct {
                unsigned int motion_history_coefficient_m2:8;
                unsigned int motion_history_coefficient_m1:8;
                unsigned int pad0:16;
            } di;
        } r4_2;

        /* WG+CSC r4.3 */
        float wg_csc_constant_c7;

        /* WG+CSC r4.4 */
        float wg_csc_constant_c10;

        /* AVS r4.5 */
        float source_video_frame_normalized_horizontal_origin;

        /* MBZ r4.6 */
        unsigned int pad1;

        /* WG+CSC r4.7 */
        float wg_csc_constant_c11;
    } grf4;
};

struct pp_inline_parameter
{
    struct {
        /* ALL r5.0 */
        int destination_block_horizontal_origin:16;
        int destination_block_vertical_origin:16;

        /* Shared r5.1 */
        union {
            /* AVS/Scaling */
            float source_surface_block_normalized_horizontal_origin;

            /* FMD */
            struct {
                unsigned int variance_surface_vertical_origin:16;
                unsigned int pad0:16;
            } fmd;
        } r5_1; 

        /* AVS/Scaling r5.2 */
        float source_surface_block_normalized_vertical_origin;

        /* Alpha r5.3 */
        float alpha_surface_block_normalized_horizontal_origin;

        /* Alpha r5.4 */
        float alpha_surface_block_normalized_vertical_origin;

        /* Alpha r5.5 */
        unsigned int alpha_mask_x:16;
        unsigned int alpha_mask_y:8;
        unsigned int block_count_x:8;

        /* r5.6 */
        unsigned int block_horizontal_mask:16;
        unsigned int block_vertical_mask:8;
        unsigned int number_blocks:8;

        /* AVS/Scaling r5.7 */
        float normalized_video_x_scaling_step;
    } grf5;

    struct {
        /* AVS r6.0 */
        float video_step_delta;

        /* r6.1-r6.7 */
        unsigned int padx[7];
    } grf6;
};

struct i965_post_processing_context
{
    int current_pp;
    struct pp_module pp_modules[NUM_PP_MODULES];
    struct pp_static_parameter pp_static_parameter;
    struct pp_inline_parameter pp_inline_parameter;

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
    int (*pp_set_block_parameter)(struct i965_post_processing_context *pp_context, int x, int y);
};

VASurfaceID
i965_post_processing(
    VADriverContextP   ctx,
    VASurfaceID        surface,
    const VARectangle *src_rect,
    const VARectangle *dst_rect,
    unsigned int       flags,
    int                *has_done_scaling 
);

Bool
i965_post_processing_terminate(VADriverContextP ctx);
Bool
i965_post_processing_init(VADriverContextP ctx);

#endif /* __I965_POST_PROCESSING_H__ */
