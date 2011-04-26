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
 *    Zhou Chang <chang.zhou@intel.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <va/va_backend.h>

#include "intel_batchbuffer.h"
#include "intel_driver.h"

#include "i965_defines.h"
#include "i965_drv_video.h"
#include "i965_encoder.h"

extern void i965_reference_buffer_store(struct buffer_store **ptr, 
                                        struct buffer_store *buffer_store);
extern void i965_release_buffer_store(struct buffer_store **ptr);

static VAStatus i965_encoder_render_squence_parameter_buffer(VADriverContextP ctx, 
                                                             struct object_context *obj_context,
                                                             struct object_buffer *obj_buffer)
{
    assert(obj_buffer->buffer_store->bo == NULL);
    assert(obj_buffer->buffer_store->buffer);
    i965_release_buffer_store(&obj_context->encode_state.seq_param);
    i965_reference_buffer_store(&obj_context->encode_state.seq_param,
                                obj_buffer->buffer_store);

    return VA_STATUS_SUCCESS;
}


static VAStatus i965_encoder_render_picture_parameter_buffer(VADriverContextP ctx, 
                                                             struct object_context *obj_context,
                                                             struct object_buffer *obj_buffer)
{
    assert(obj_buffer->buffer_store->bo == NULL);
    assert(obj_buffer->buffer_store->buffer);
    i965_release_buffer_store(&obj_context->encode_state.pic_param);
    i965_reference_buffer_store(&obj_context->encode_state.pic_param,
                                obj_buffer->buffer_store);

    return VA_STATUS_SUCCESS;
}

static VAStatus i965_encoder_render_slice_parameter_buffer(VADriverContextP ctx, 
                                                           struct object_context *obj_context,
                                                           struct object_buffer *obj_buffer)
{
    if (obj_context->encode_state.num_slice_params == obj_context->encode_state.max_slice_params) {
        obj_context->encode_state.slice_params = realloc(obj_context->encode_state.slice_params,
                                                         (obj_context->encode_state.max_slice_params + NUM_SLICES) * sizeof(*obj_context->encode_state.slice_params));
        memset(obj_context->encode_state.slice_params + obj_context->encode_state.max_slice_params, 0, NUM_SLICES * sizeof(*obj_context->encode_state.slice_params));
        obj_context->encode_state.max_slice_params += NUM_SLICES;
    }

    i965_release_buffer_store(&obj_context->encode_state.slice_params[obj_context->encode_state.num_slice_params]);
    i965_reference_buffer_store(&obj_context->encode_state.slice_params[obj_context->encode_state.num_slice_params],
                                obj_buffer->buffer_store);
    obj_context->encode_state.num_slice_params++;
    
    return VA_STATUS_SUCCESS;
}

static void i965_encoder_render_picture_control_buffer(VADriverContextP ctx, 
                                                       struct object_context *obj_context,
                                                       struct object_buffer *obj_buffer)
{
    assert(obj_buffer->buffer_store->bo == NULL);
    assert(obj_buffer->buffer_store->buffer);
    i965_release_buffer_store(&obj_context->encode_state.pic_control);
    i965_reference_buffer_store(&obj_context->encode_state.pic_control,
                                obj_buffer->buffer_store);
}

static void i965_encoder_render_qmatrix_buffer(VADriverContextP ctx, 
                                               struct object_context *obj_context,
                                               struct object_buffer *obj_buffer)
{
    assert(obj_buffer->buffer_store->bo == NULL);
    assert(obj_buffer->buffer_store->buffer);
    i965_release_buffer_store(&obj_context->encode_state.q_matrix);
    i965_reference_buffer_store(&obj_context->encode_state.iq_matrix,
                                obj_buffer->buffer_store);
}

static void i965_encoder_render_iqmatrix_buffer(VADriverContextP ctx, 
                                                struct object_context *obj_context,
                                                struct object_buffer *obj_buffer)
{
    assert(obj_buffer->buffer_store->bo == NULL);
    assert(obj_buffer->buffer_store->buffer);
    i965_release_buffer_store(&obj_context->encode_state.iq_matrix);
    i965_reference_buffer_store(&obj_context->encode_state.iq_matrix,
                                obj_buffer->buffer_store);
}

VAStatus i965_encoder_create_context(VADriverContextP ctx,
                                     VAConfigID config_id,
                                     int picture_width,
                                     int picture_height,
                                     int flag,
                                     VASurfaceID *render_targets,
                                     int num_render_targets,
                                     struct object_context *obj_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_config *obj_config = CONFIG(config_id);
    VAStatus vaStatus = VA_STATUS_SUCCESS;


    if (NULL == obj_config) {
        vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
        return vaStatus;
    }

    if (NULL == obj_context) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        return vaStatus;
    }

    if( VAProfileH264Baseline != obj_config->profile || 
        VAEntrypointEncSlice != obj_config->entrypoint) {
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
        return vaStatus;
    }

    /*encdoe_state init */
    obj_context->encode_state.current_render_target = VA_INVALID_ID;
    obj_context->encode_state.max_slice_params = NUM_SLICES;
    obj_context->encode_state.slice_params = calloc(obj_context->encode_state.max_slice_params,
                                                    sizeof(*obj_context->encode_state.slice_params));

    return vaStatus;
}


VAStatus i965_encoder_begin_picture(VADriverContextP ctx,
                                    VAContextID context,
                                    VASurfaceID render_target)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx); 
    struct object_context *obj_context = CONTEXT(context);
    struct object_surface *obj_surface = SURFACE(render_target);
    struct object_config *obj_config;
    VAContextID config;
    VAStatus vaStatus;

    assert(obj_context);
    assert(obj_surface);

    config = obj_context->config_id;
    obj_config = CONFIG(config);
    assert(obj_config);

    if( VAProfileH264Baseline != obj_config->profile || 
        VAEntrypointEncSlice != obj_config->entrypoint){
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
    }else{
        vaStatus = VA_STATUS_SUCCESS;
    }

    obj_context->encode_state.current_render_target = render_target;	/*This is input new frame*/

    return vaStatus;
}

VAStatus i965_encoder_render_picture(VADriverContextP ctx,
                                     VAContextID context,
                                     VABufferID *buffers,
                                     int num_buffers)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx); 
    struct object_context *obj_context = CONTEXT(context);
    struct object_config *obj_config;
    VAContextID config;
    VAStatus vaStatus;
    int i;

    assert(obj_context);
    config = obj_context->config_id;
    obj_config = CONFIG(config);
    assert(obj_config);


    for (i = 0; i < num_buffers; i++) {  
        struct object_buffer *obj_buffer = BUFFER(buffers[i]);
        assert(obj_buffer);

        switch (obj_buffer->type) {
        case VAEncSequenceParameterBufferType:
            i965_encoder_render_squence_parameter_buffer(ctx, obj_context, obj_buffer);
            break;

        case VAEncPictureParameterBufferType:
            i965_encoder_render_picture_parameter_buffer(ctx, obj_context, obj_buffer);
            break;		

        case VAEncSliceParameterBufferType:
            i965_encoder_render_slice_parameter_buffer(ctx, obj_context, obj_buffer);
            break;

        case VAPictureParameterBufferType:
            i965_encoder_render_picture_control_buffer(ctx, obj_context, obj_buffer);
            break;

        case VAQMatrixBufferType:
            i965_encoder_render_qmatrix_buffer(ctx, obj_context, obj_buffer);
            break;

        case VAIQMatrixBufferType:
            i965_encoder_render_iqmatrix_buffer(ctx, obj_context, obj_buffer);
            break;

        default:
            break;
        }
    }	

    vaStatus = VA_STATUS_SUCCESS;
    return vaStatus;
}

static VAStatus 
gen6_encoder_end_picture(VADriverContextP ctx, 
                     VAContextID context,                              
                     struct mfc_encode_state *encode_state)
{
    VAStatus vaStatus;

    vaStatus = gen6_vme_media_pipeline(ctx, context, encode_state);

    if (vaStatus == VA_STATUS_SUCCESS)
        vaStatus = gen6_mfc_pipeline(ctx, context, encode_state);

    return vaStatus;
}

VAStatus i965_encoder_end_picture(VADriverContextP ctx, VAContextID context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx); 
    struct object_context *obj_context = CONTEXT(context);
    struct object_config *obj_config;
    VAContextID config;
    VAStatus vaStatus;
    int i;

    assert(obj_context);
    config = obj_context->config_id;
    obj_config = CONFIG(config);
    assert(obj_config);

    assert(obj_context->encode_state.pic_param);
    assert(obj_context->encode_state.num_slice_params >= 1);

    if (IS_GEN6(i965->intel.device_id)) {
        vaStatus = gen6_encoder_end_picture(ctx, context, &(obj_context->encode_state));
    } else {
        /* add for other chipset */
        assert(0);
    }

    obj_context->encode_state.current_render_target = VA_INVALID_SURFACE;
    obj_context->encode_state.num_slice_params = 0;
    i965_release_buffer_store(&obj_context->encode_state.pic_param);

    for (i = 0; i < obj_context->encode_state.num_slice_params; i++) {
        i965_release_buffer_store(&obj_context->encode_state.slice_params[i]);
    }

    return VA_STATUS_SUCCESS;
}


void i965_encoder_destroy_context(struct object_heap *heap, struct object_base *obj)
{
    struct object_context *obj_context = (struct object_context *)obj;

    assert(obj_context->encode_state.num_slice_params <= obj_context->encode_state.max_slice_params);

    i965_release_buffer_store(&obj_context->encode_state.pic_param);
    i965_release_buffer_store(&obj_context->encode_state.seq_param);

    free(obj_context->render_targets);
    object_heap_free(heap, obj);
}

Bool i965_encoder_init(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx); 

    if (IS_GEN6(i965->intel.device_id)) {
        gen6_vme_init(ctx);
    }

    return True;
}

Bool i965_encoder_terminate(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen6_media_state *media_state = &i965->gen6_media_state;
    struct gen6_mfc_bcs_state *bcs_state = &i965->gen6_mfc_bcs_state;
    int i;

    if (IS_GEN6(i965->intel.device_id)) {
        gen6_vme_terminate(ctx);
    }

    for (i = 0; i < MAX_MEDIA_SURFACES_GEN6; i++) {
        dri_bo_unreference(media_state->surface_state[i].bo);
        media_state->surface_state[i].bo = NULL;
    }
    
    dri_bo_unreference(media_state->idrt.bo);
    media_state->idrt.bo = NULL;

    dri_bo_unreference(media_state->binding_table.bo);
    media_state->binding_table.bo = NULL;

    dri_bo_unreference(media_state->curbe.bo);
    media_state->curbe.bo = NULL;

    dri_bo_unreference(media_state->vme_output.bo);
    media_state->vme_output.bo = NULL;

    dri_bo_unreference(media_state->vme_state.bo);
    media_state->vme_state.bo = NULL;

    dri_bo_unreference(bcs_state->post_deblocking_output.bo);
    bcs_state->post_deblocking_output.bo = NULL;

    dri_bo_unreference(bcs_state->pre_deblocking_output.bo);
    bcs_state->pre_deblocking_output.bo = NULL;

    dri_bo_unreference(bcs_state->uncompressed_picture_source.bo);
    bcs_state->uncompressed_picture_source.bo = NULL;

    dri_bo_unreference(bcs_state->mfc_indirect_pak_bse_object.bo); 
    bcs_state->mfc_indirect_pak_bse_object.bo = NULL;

    for (i = 0; i < NUM_MFC_DMV_BUFFERS; i++){
        dri_bo_unreference(bcs_state->direct_mv_buffers[i].bo);
        bcs_state->direct_mv_buffers[i].bo = NULL;
    }

    dri_bo_unreference(bcs_state->intra_row_store_scratch_buffer.bo);
    bcs_state->intra_row_store_scratch_buffer.bo = NULL;

    dri_bo_unreference(bcs_state->deblocking_filter_row_store_scratch_buffer.bo);
    bcs_state->deblocking_filter_row_store_scratch_buffer.bo = NULL;

    dri_bo_unreference(bcs_state->bsd_mpc_row_store_scratch_buffer.bo);
    bcs_state->bsd_mpc_row_store_scratch_buffer.bo = NULL;

    return True;
}
