/*
 * Copyright © 2009 Intel Corporation
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
 *    Zou Nan hai <nanhai.zou@intel.com>
 *
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "va/x11/va_dricommon.h"

#include "intel_driver.h"
#include "intel_memman.h"
#include "intel_batchbuffer.h"

#include "i965_media.h"
#include "i965_drv_video.h"
#include "i965_defines.h"

#define CONFIG_ID_OFFSET                0x01000000
#define CONTEXT_ID_OFFSET               0x02000000
#define SURFACE_ID_OFFSET               0x04000000
#define BUFFER_ID_OFFSET                0x08000000
#define IMAGE_ID_OFFSET                 0x0a000000
#define SUBPIC_ID_OFFSET                0x10000000

enum {
    I965_SURFACETYPE_RGBA = 1,
    I965_SURFACETYPE_YUV,
    I965_SURFACETYPE_INDEXED
};

/* List of supported image formats */
typedef struct {
    unsigned int        type;
    VAImageFormat       va_format;
} i965_image_format_map_t;

static const i965_image_format_map_t
i965_image_formats_map[I965_MAX_IMAGE_FORMATS + 1] = {
    { I965_SURFACETYPE_YUV,
      { VA_FOURCC('Y','V','1','2'), VA_LSB_FIRST, 12, } },
    { I965_SURFACETYPE_YUV,
      { VA_FOURCC('I','4','2','0'), VA_LSB_FIRST, 12, } },
    { I965_SURFACETYPE_YUV,
      { VA_FOURCC('N','V','1','2'), VA_LSB_FIRST, 12, } },
};

/* List of supported subpicture formats */
typedef struct {
    unsigned int        type;
    unsigned int        format;
    VAImageFormat       va_format;
    unsigned int        va_flags;
} i965_subpic_format_map_t;

static const i965_subpic_format_map_t
i965_subpic_formats_map[I965_MAX_SUBPIC_FORMATS + 1] = {
    { I965_SURFACETYPE_INDEXED, I965_SURFACEFORMAT_P4A4_UNORM,
      { VA_FOURCC('I','A','4','4'), VA_MSB_FIRST, 8, },
      0 },
    { I965_SURFACETYPE_INDEXED, I965_SURFACEFORMAT_A4P4_UNORM,
      { VA_FOURCC('A','I','4','4'), VA_MSB_FIRST, 8, },
      0 },
    { I965_SURFACETYPE_RGBA, I965_SURFACEFORMAT_B8G8R8A8_UNORM,
      { VA_FOURCC('B','G','R','A'), VA_LSB_FIRST, 32,
        32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 },
      0 },
    { I965_SURFACETYPE_RGBA, I965_SURFACEFORMAT_R8G8B8A8_UNORM,
      { VA_FOURCC('R','G','B','A'), VA_LSB_FIRST, 32,
        32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 },
      0 },
};

static const i965_subpic_format_map_t *
get_subpic_format(const VAImageFormat *va_format)
{
    unsigned int i;
    for (i = 0; i965_subpic_formats_map[i].type != 0; i++) {
        const i965_subpic_format_map_t * const m = &i965_subpic_formats_map[i];
        if (m->va_format.fourcc == va_format->fourcc &&
            (m->type == I965_SURFACETYPE_RGBA ?
             (m->va_format.byte_order == va_format->byte_order &&
              m->va_format.red_mask   == va_format->red_mask   &&
              m->va_format.green_mask == va_format->green_mask &&
              m->va_format.blue_mask  == va_format->blue_mask  &&
              m->va_format.alpha_mask == va_format->alpha_mask) : 1))
            return m;
    }
    return NULL;
}

VAStatus 
i965_QueryConfigProfiles(VADriverContextP ctx,
                         VAProfile *profile_list,       /* out */
                         int *num_profiles)             /* out */
{
    int i = 0;

    profile_list[i++] = VAProfileMPEG2Simple;
    profile_list[i++] = VAProfileMPEG2Main;
    profile_list[i++] = VAProfileH264Baseline;
    profile_list[i++] = VAProfileH264Main;
    profile_list[i++] = VAProfileH264High;

    /* If the assert fails then I965_MAX_PROFILES needs to be bigger */
    assert(i <= I965_MAX_PROFILES);
    *num_profiles = i;

    return VA_STATUS_SUCCESS;
}

VAStatus 
i965_QueryConfigEntrypoints(VADriverContextP ctx,
                            VAProfile profile,
                            VAEntrypoint *entrypoint_list,      /* out */
                            int *num_entrypoints)               /* out */
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    switch (profile) {
    case VAProfileMPEG2Simple:
    case VAProfileMPEG2Main:
        *num_entrypoints = 1;
        entrypoint_list[0] = VAEntrypointVLD;
        break;

    case VAProfileH264Baseline:
    case VAProfileH264Main:
    case VAProfileH264High:
        *num_entrypoints = 1;
        entrypoint_list[0] = VAEntrypointVLD;
        break;

    default:
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
        *num_entrypoints = 0;
        break;
    }

    /* If the assert fails then I965_MAX_ENTRYPOINTS needs to be bigger */
    assert(*num_entrypoints <= I965_MAX_ENTRYPOINTS);

    return vaStatus;
}

VAStatus 
i965_GetConfigAttributes(VADriverContextP ctx,
                         VAProfile profile,
                         VAEntrypoint entrypoint,
                         VAConfigAttrib *attrib_list,  /* in/out */
                         int num_attribs)
{
    int i;

    /* Other attributes don't seem to be defined */
    /* What to do if we don't know the attribute? */
    for (i = 0; i < num_attribs; i++) {
        switch (attrib_list[i].type) {
        case VAConfigAttribRTFormat:
            attrib_list[i].value = VA_RT_FORMAT_YUV420;
            break;

        default:
            /* Do nothing */
            attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            break;
        }
    }

    return VA_STATUS_SUCCESS;
}

static void 
i965_destroy_config(struct object_heap *heap, struct object_base *obj)
{
    object_heap_free(heap, obj);
}

static VAStatus 
i965_update_attribute(struct object_config *obj_config, VAConfigAttrib *attrib)
{
    int i;

    /* Check existing attrbiutes */
    for (i = 0; obj_config->num_attribs < i; i++) {
        if (obj_config->attrib_list[i].type == attrib->type) {
            /* Update existing attribute */
            obj_config->attrib_list[i].value = attrib->value;
            return VA_STATUS_SUCCESS;
        }
    }

    if (obj_config->num_attribs < I965_MAX_CONFIG_ATTRIBUTES) {
        i = obj_config->num_attribs;
        obj_config->attrib_list[i].type = attrib->type;
        obj_config->attrib_list[i].value = attrib->value;
        obj_config->num_attribs++;
        return VA_STATUS_SUCCESS;
    }

    return VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
}

VAStatus 
i965_CreateConfig(VADriverContextP ctx,
                  VAProfile profile,
                  VAEntrypoint entrypoint,
                  VAConfigAttrib *attrib_list,
                  int num_attribs,
                  VAConfigID *config_id)		/* out */
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_config *obj_config;
    int configID;
    int i;
    VAStatus vaStatus;

    /* Validate profile & entrypoint */
    switch (profile) {
    case VAProfileMPEG2Simple:
    case VAProfileMPEG2Main:
        if (VAEntrypointVLD == entrypoint) {
            vaStatus = VA_STATUS_SUCCESS;
        } else {
            vaStatus = VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
        }
        break;

    case VAProfileH264Baseline:
    case VAProfileH264Main:
    case VAProfileH264High:
        if (VAEntrypointVLD == entrypoint) {
            vaStatus = VA_STATUS_SUCCESS;
        } else {
            vaStatus = VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
        }

        break;

    default:
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
        break;
    }

    if (VA_STATUS_SUCCESS != vaStatus) {
        return vaStatus;
    }

    configID = NEW_CONFIG_ID();
    obj_config = CONFIG(configID);

    if (NULL == obj_config) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        return vaStatus;
    }

    obj_config->profile = profile;
    obj_config->entrypoint = entrypoint;
    obj_config->attrib_list[0].type = VAConfigAttribRTFormat;
    obj_config->attrib_list[0].value = VA_RT_FORMAT_YUV420;
    obj_config->num_attribs = 1;

    for(i = 0; i < num_attribs; i++) {
        vaStatus = i965_update_attribute(obj_config, &(attrib_list[i]));

        if (VA_STATUS_SUCCESS != vaStatus) {
            break;
        }
    }

    /* Error recovery */
    if (VA_STATUS_SUCCESS != vaStatus) {
        i965_destroy_config(&i965->config_heap, (struct object_base *)obj_config);
    } else {
        *config_id = configID;
    }

    return vaStatus;
}

VAStatus 
i965_DestroyConfig(VADriverContextP ctx, VAConfigID config_id)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_config *obj_config = CONFIG(config_id);
    VAStatus vaStatus;

    if (NULL == obj_config) {
        vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
        return vaStatus;
    }

    i965_destroy_config(&i965->config_heap, (struct object_base *)obj_config);
    return VA_STATUS_SUCCESS;
}

VAStatus i965_QueryConfigAttributes(VADriverContextP ctx,
                                    VAConfigID config_id,
                                    VAProfile *profile,                 /* out */
                                    VAEntrypoint *entrypoint,           /* out */
                                    VAConfigAttrib *attrib_list,        /* out */
                                    int *num_attribs)                   /* out */
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_config *obj_config = CONFIG(config_id);
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i;

    assert(obj_config);
    *profile = obj_config->profile;
    *entrypoint = obj_config->entrypoint;
    *num_attribs = obj_config->num_attribs;

    for(i = 0; i < obj_config->num_attribs; i++) {
        attrib_list[i] = obj_config->attrib_list[i];
    }

    return vaStatus;
}

static void 
i965_destroy_surface(struct object_heap *heap, struct object_base *obj)
{
    struct object_surface *obj_surface = (struct object_surface *)obj;

    dri_bo_unreference(obj_surface->bo);
    obj_surface->bo = NULL;
    dri_bo_unreference(obj_surface->pp_out_bo);
    obj_surface->pp_out_bo = NULL;

    if (obj_surface->free_private_data != NULL) {
        obj_surface->free_private_data(&obj_surface->private_data);
        obj_surface->private_data = NULL;
    }

    object_heap_free(heap, obj);
}

VAStatus 
i965_CreateSurfaces(VADriverContextP ctx,
                    int width,
                    int height,
                    int format,
                    int num_surfaces,
                    VASurfaceID *surfaces)      /* out */
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    int i;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    /* We only support one format */
    if (VA_RT_FORMAT_YUV420 != format) {
        return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
    }

    for (i = 0; i < num_surfaces; i++) {
        int surfaceID = NEW_SURFACE_ID();
        struct object_surface *obj_surface = SURFACE(surfaceID);

        if (NULL == obj_surface) {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            break;
        }

        surfaces[i] = surfaceID;
        obj_surface->status = VASurfaceReady;
        obj_surface->subpic = VA_INVALID_ID;
        obj_surface->orig_width = width;
        obj_surface->orig_height = height;
        obj_surface->width = ALIGN(obj_surface->orig_width, 16);
        obj_surface->height = ALIGN(obj_surface->orig_height, 16);
        obj_surface->size = SIZE_YUV420(obj_surface->width, obj_surface->height);
        obj_surface->flags = SURFACE_REFERENCED;
        obj_surface->bo = NULL;
        obj_surface->pp_out_bo = NULL;
        obj_surface->private_data = NULL;
        obj_surface->free_private_data = NULL;
    }

    /* Error recovery */
    if (VA_STATUS_SUCCESS != vaStatus) {
        /* surfaces[i-1] was the last successful allocation */
        for (; i--; ) {
            struct object_surface *obj_surface = SURFACE(surfaces[i]);

            surfaces[i] = VA_INVALID_SURFACE;
            assert(obj_surface);
            i965_destroy_surface(&i965->surface_heap, (struct object_base *)obj_surface);
        }
    }

    return vaStatus;
}

VAStatus 
i965_DestroySurfaces(VADriverContextP ctx,
                     VASurfaceID *surface_list,
                     int num_surfaces)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    int i;

    for (i = num_surfaces; i--; ) {
        struct object_surface *obj_surface = SURFACE(surface_list[i]);

        assert(obj_surface);
        i965_destroy_surface(&i965->surface_heap, (struct object_base *)obj_surface);
    }

    return VA_STATUS_SUCCESS;
}

VAStatus 
i965_QueryImageFormats(VADriverContextP ctx,
                       VAImageFormat *format_list,      /* out */
                       int *num_formats)                /* out */
{
    int n;

    for (n = 0; i965_image_formats_map[n].va_format.fourcc != 0; n++) {
        const i965_image_format_map_t * const m = &i965_image_formats_map[n];
        if (format_list)
            format_list[n] = m->va_format;
    }

    if (num_formats)
        *num_formats = n;

    return VA_STATUS_SUCCESS;
}

VAStatus 
i965_PutImage(VADriverContextP ctx,
              VASurfaceID surface,
              VAImageID image,
              int src_x,
              int src_y,
              unsigned int src_width,
              unsigned int src_height,
              int dest_x,
              int dest_y,
              unsigned int dest_width,
              unsigned int dest_height)
{
    return VA_STATUS_SUCCESS;
}

VAStatus 
i965_QuerySubpictureFormats(VADriverContextP ctx,
                            VAImageFormat *format_list,         /* out */
                            unsigned int *flags,                /* out */
                            unsigned int *num_formats)          /* out */
{
    int n;

    for (n = 0; i965_subpic_formats_map[n].va_format.fourcc != 0; n++) {
        const i965_subpic_format_map_t * const m = &i965_subpic_formats_map[n];
        if (format_list)
            format_list[n] = m->va_format;
        if (flags)
            flags[n] = m->va_flags;
    }

    if (num_formats)
        *num_formats = n;

    return VA_STATUS_SUCCESS;
}

static void 
i965_destroy_subpic(struct object_heap *heap, struct object_base *obj)
{
//    struct object_subpic *obj_subpic = (struct object_subpic *)obj;

    object_heap_free(heap, obj);
}

VAStatus 
i965_CreateSubpicture(VADriverContextP ctx,
                      VAImageID image,
                      VASubpictureID *subpicture)         /* out */
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    VASubpictureID subpicID = NEW_SUBPIC_ID()
	
        struct object_subpic *obj_subpic = SUBPIC(subpicID);
    if (!obj_subpic)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    struct object_image *obj_image = IMAGE(image);
    if (!obj_image)
        return VA_STATUS_ERROR_INVALID_IMAGE;

    const i965_subpic_format_map_t * const m = get_subpic_format(&obj_image->image.format);
    if (!m)
        return VA_STATUS_ERROR_UNKNOWN; /* XXX: VA_STATUS_ERROR_UNSUPPORTED_FORMAT? */

    *subpicture = subpicID;
    obj_subpic->image  = image;
    obj_subpic->format = m->format;
    obj_subpic->width  = obj_image->image.width;
    obj_subpic->height = obj_image->image.height;
    obj_subpic->pitch  = obj_image->image.pitches[0];
    obj_subpic->bo     = obj_image->bo;
    return VA_STATUS_SUCCESS;
}

VAStatus 
i965_DestroySubpicture(VADriverContextP ctx,
                       VASubpictureID subpicture)
{
	
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_subpic *obj_subpic = SUBPIC(subpicture);
    i965_destroy_subpic(&i965->subpic_heap, (struct object_base *)obj_subpic);
    return VA_STATUS_SUCCESS;
}

VAStatus 
i965_SetSubpictureImage(VADriverContextP ctx,
                        VASubpictureID subpicture,
                        VAImageID image)
{
    /* TODO */
    return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus 
i965_SetSubpictureChromakey(VADriverContextP ctx,
                            VASubpictureID subpicture,
                            unsigned int chromakey_min,
                            unsigned int chromakey_max,
                            unsigned int chromakey_mask)
{
    /* TODO */
    return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus 
i965_SetSubpictureGlobalAlpha(VADriverContextP ctx,
                              VASubpictureID subpicture,
                              float global_alpha)
{
    /* TODO */
    return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus 
i965_AssociateSubpicture(VADriverContextP ctx,
                         VASubpictureID subpicture,
                         VASurfaceID *target_surfaces,
                         int num_surfaces,
                         short src_x, /* upper left offset in subpicture */
                         short src_y,
                         unsigned short src_width,
                         unsigned short src_height,
                         short dest_x, /* upper left offset in surface */
                         short dest_y,
                         unsigned short dest_width,
                         unsigned short dest_height,
                         /*
                          * whether to enable chroma-keying or global-alpha
                          * see VA_SUBPICTURE_XXX values
                          */
                         unsigned int flags)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_subpic *obj_subpic = SUBPIC(subpicture);
    int i;

    obj_subpic->src_rect.x      = src_x;
    obj_subpic->src_rect.y      = src_y;
    obj_subpic->src_rect.width  = src_width;
    obj_subpic->src_rect.height = src_height;
    obj_subpic->dst_rect.x      = dest_x;
    obj_subpic->dst_rect.y      = dest_y;
    obj_subpic->dst_rect.width  = dest_width;
    obj_subpic->dst_rect.height = dest_height;

    for (i = 0; i < num_surfaces; i++) {
        struct object_surface *obj_surface = SURFACE(target_surfaces[i]);
        if (!obj_surface)
            return VA_STATUS_ERROR_INVALID_SURFACE;
        obj_surface->subpic = subpicture;
    }
    return VA_STATUS_SUCCESS;
}


VAStatus 
i965_DeassociateSubpicture(VADriverContextP ctx,
                           VASubpictureID subpicture,
                           VASurfaceID *target_surfaces,
                           int num_surfaces)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    int i;

    for (i = 0; i < num_surfaces; i++) {
        struct object_surface *obj_surface = SURFACE(target_surfaces[i]);
        if (!obj_surface)
            return VA_STATUS_ERROR_INVALID_SURFACE;
        if (obj_surface->subpic == subpicture)
            obj_surface->subpic = VA_INVALID_ID;
    }
    return VA_STATUS_SUCCESS;
}

static void
i965_reference_buffer_store(struct buffer_store **ptr, 
                            struct buffer_store *buffer_store)
{
    assert(*ptr == NULL);

    if (buffer_store) {
        buffer_store->ref_count++;
        *ptr = buffer_store;
    }
}

static void 
i965_release_buffer_store(struct buffer_store **ptr)
{
    struct buffer_store *buffer_store = *ptr;

    if (buffer_store == NULL)
        return;

    assert(buffer_store->bo || buffer_store->buffer);
    assert(!(buffer_store->bo && buffer_store->buffer));
    buffer_store->ref_count--;
    
    if (buffer_store->ref_count == 0) {
        dri_bo_unreference(buffer_store->bo);
        free(buffer_store->buffer);
        buffer_store->bo = NULL;
        buffer_store->buffer = NULL;
        free(buffer_store);
    }

    *ptr = NULL;
}

static void 
i965_destroy_context(struct object_heap *heap, struct object_base *obj)
{
    struct object_context *obj_context = (struct object_context *)obj;
    int i;

    assert(obj_context->decode_state.num_slice_params <= obj_context->decode_state.max_slice_params);
    assert(obj_context->decode_state.num_slice_datas <= obj_context->decode_state.max_slice_datas);

    i965_release_buffer_store(&obj_context->decode_state.pic_param);
    i965_release_buffer_store(&obj_context->decode_state.iq_matrix);
    i965_release_buffer_store(&obj_context->decode_state.bit_plane);

    for (i = 0; i < obj_context->decode_state.num_slice_params; i++)
        i965_release_buffer_store(&obj_context->decode_state.slice_params[i]);

    for (i = 0; i < obj_context->decode_state.num_slice_datas; i++)
        i965_release_buffer_store(&obj_context->decode_state.slice_datas[i]);

    free(obj_context->decode_state.slice_params);
    free(obj_context->decode_state.slice_datas);
    free(obj_context->render_targets);
    object_heap_free(heap, obj);
}

VAStatus 
i965_CreateContext(VADriverContextP ctx,
                   VAConfigID config_id,
                   int picture_width,
                   int picture_height,
                   int flag,
                   VASurfaceID *render_targets,
                   int num_render_targets,
                   VAContextID *context)                /* out */
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;
    struct object_config *obj_config = CONFIG(config_id);
    struct object_context *obj_context = NULL;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int contextID;
    int i;

    if (NULL == obj_config) {
        vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
        return vaStatus;
    }

    /* Validate flag */
    /* Validate picture dimensions */
    contextID = NEW_CONTEXT_ID();
    obj_context = CONTEXT(contextID);

    if (NULL == obj_context) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        return vaStatus;
    }

    switch (obj_config->profile) {
    case VAProfileH264Baseline:
    case VAProfileH264Main:
    case VAProfileH264High:
        render_state->interleaved_uv = 1;
        break;
    default:
        render_state->interleaved_uv = 0;
    }

    obj_context->context_id = contextID;
    *context = contextID;
    memset(&obj_context->decode_state, 0, sizeof(obj_context->decode_state));
    obj_context->decode_state.current_render_target = -1;
    obj_context->decode_state.max_slice_params = NUM_SLICES;
    obj_context->decode_state.max_slice_datas = NUM_SLICES;
    obj_context->decode_state.slice_params = calloc(obj_context->decode_state.max_slice_params,
                                                    sizeof(*obj_context->decode_state.slice_params));
    obj_context->decode_state.slice_datas = calloc(obj_context->decode_state.max_slice_datas,
                                                   sizeof(*obj_context->decode_state.slice_datas));
    obj_context->config_id = config_id;
    obj_context->picture_width = picture_width;
    obj_context->picture_height = picture_height;
    obj_context->num_render_targets = num_render_targets;
    obj_context->render_targets = 
        (VASurfaceID *)calloc(num_render_targets, sizeof(VASurfaceID));

    for(i = 0; i < num_render_targets; i++) {
        if (NULL == SURFACE(render_targets[i])) {
            vaStatus = VA_STATUS_ERROR_INVALID_SURFACE;
            break;
        }

        obj_context->render_targets[i] = render_targets[i];
    }

    obj_context->flags = flag;

    /* Error recovery */
    if (VA_STATUS_SUCCESS != vaStatus) {
        i965_destroy_context(&i965->context_heap, (struct object_base *)obj_context);
    }

    return vaStatus;
}

VAStatus 
i965_DestroyContext(VADriverContextP ctx, VAContextID context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_context *obj_context = CONTEXT(context);

    assert(obj_context);
    i965_destroy_context(&i965->context_heap, (struct object_base *)obj_context);

    return VA_STATUS_SUCCESS;
}

static void 
i965_destroy_buffer(struct object_heap *heap, struct object_base *obj)
{
    struct object_buffer *obj_buffer = (struct object_buffer *)obj;

    assert(obj_buffer->buffer_store);
    i965_release_buffer_store(&obj_buffer->buffer_store);
    object_heap_free(heap, obj);
}

VAStatus 
i965_CreateBuffer(VADriverContextP ctx,
                  VAContextID context,          /* in */
                  VABufferType type,            /* in */
                  unsigned int size,            /* in */
                  unsigned int num_elements,    /* in */
                  void *data,                   /* in */
                  VABufferID *buf_id)           /* out */
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_buffer *obj_buffer = NULL;
    struct buffer_store *buffer_store = NULL;
    int bufferID;

    /* Validate type */
    switch (type) {
    case VAPictureParameterBufferType:
    case VAIQMatrixBufferType:
    case VABitPlaneBufferType:
    case VASliceGroupMapBufferType:
    case VASliceParameterBufferType:
    case VASliceDataBufferType:
    case VAMacroblockParameterBufferType:
    case VAResidualDataBufferType:
    case VADeblockingParameterBufferType:
    case VAImageBufferType:
        /* Ok */
        break;

    default:
        return VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
    }

    bufferID = NEW_BUFFER_ID();
    obj_buffer = BUFFER(bufferID);

    if (NULL == obj_buffer) {
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    obj_buffer->max_num_elements = num_elements;
    obj_buffer->num_elements = num_elements;
    obj_buffer->size_element = size;
    obj_buffer->type = type;
    obj_buffer->buffer_store = NULL;
    buffer_store = calloc(1, sizeof(struct buffer_store));
    assert(buffer_store);
    buffer_store->ref_count = 1;

    if (type == VASliceDataBufferType || type == VAImageBufferType) {
        buffer_store->bo = dri_bo_alloc(i965->intel.bufmgr, 
                                        "Buffer", 
                                        size * num_elements, 64);
        assert(buffer_store->bo);

        if (data)
            dri_bo_subdata(buffer_store->bo, 0, size * num_elements, data);
    } else {
        buffer_store->buffer = malloc(size * num_elements);
        assert(buffer_store->buffer);

        if (data)
            memcpy(buffer_store->buffer, data, size * num_elements);
    }

    buffer_store->num_elements = obj_buffer->num_elements;
    i965_reference_buffer_store(&obj_buffer->buffer_store, buffer_store);
    i965_release_buffer_store(&buffer_store);
    *buf_id = bufferID;

    return VA_STATUS_SUCCESS;
}


VAStatus 
i965_BufferSetNumElements(VADriverContextP ctx,
                          VABufferID buf_id,           /* in */
                          unsigned int num_elements)   /* in */
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_buffer *obj_buffer = BUFFER(buf_id);
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    assert(obj_buffer);

    if ((num_elements < 0) || 
        (num_elements > obj_buffer->max_num_elements)) {
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
    } else {
        obj_buffer->num_elements = num_elements;
    }

    return vaStatus;
}

VAStatus 
i965_MapBuffer(VADriverContextP ctx,
               VABufferID buf_id,       /* in */
               void **pbuf)             /* out */
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_buffer *obj_buffer = BUFFER(buf_id);
    VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;

    assert(obj_buffer && obj_buffer->buffer_store);
    assert(obj_buffer->buffer_store->bo || obj_buffer->buffer_store->buffer);
    assert(!(obj_buffer->buffer_store->bo && obj_buffer->buffer_store->buffer));

    if (NULL != obj_buffer->buffer_store->bo) {
        dri_bo_map(obj_buffer->buffer_store->bo, 1);
        assert(obj_buffer->buffer_store->bo->virtual);
        *pbuf = obj_buffer->buffer_store->bo->virtual;
        vaStatus = VA_STATUS_SUCCESS;
    } else if (NULL != obj_buffer->buffer_store->buffer) {
        *pbuf = obj_buffer->buffer_store->buffer;
        vaStatus = VA_STATUS_SUCCESS;
    }

    return vaStatus;
}

VAStatus 
i965_UnmapBuffer(VADriverContextP ctx, VABufferID buf_id)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_buffer *obj_buffer = BUFFER(buf_id);
    VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;

    assert(obj_buffer && obj_buffer->buffer_store);
    assert(obj_buffer->buffer_store->bo || obj_buffer->buffer_store->buffer);
    assert(!(obj_buffer->buffer_store->bo && obj_buffer->buffer_store->buffer));

    if (NULL != obj_buffer->buffer_store->bo) {
        dri_bo_unmap(obj_buffer->buffer_store->bo);
        vaStatus = VA_STATUS_SUCCESS;
    } else if (NULL != obj_buffer->buffer_store->buffer) {
        /* Do nothing */
        vaStatus = VA_STATUS_SUCCESS;
    }

    return vaStatus;    
}

VAStatus 
i965_DestroyBuffer(VADriverContextP ctx, VABufferID buffer_id)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_buffer *obj_buffer = BUFFER(buffer_id);

    assert(obj_buffer);
    i965_destroy_buffer(&i965->buffer_heap, (struct object_base *)obj_buffer);

    return VA_STATUS_SUCCESS;
}

VAStatus 
i965_BeginPicture(VADriverContextP ctx,
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

    switch (obj_config->profile) {
    case VAProfileMPEG2Simple:
    case VAProfileMPEG2Main:
        vaStatus = VA_STATUS_SUCCESS;
        break;

    case VAProfileH264Baseline:
    case VAProfileH264Main:
    case VAProfileH264High:
        vaStatus = VA_STATUS_SUCCESS;
        break;

    default:
        assert(0);
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
        break;
    }

    obj_context->decode_state.current_render_target = render_target;

    return vaStatus;
}

static VAStatus
i965_render_picture_parameter_buffer(VADriverContextP ctx,
                                     struct object_context *obj_context,
                                     struct object_buffer *obj_buffer)
{
    assert(obj_buffer->buffer_store->bo == NULL);
    assert(obj_buffer->buffer_store->buffer);
    i965_release_buffer_store(&obj_context->decode_state.pic_param);
    i965_reference_buffer_store(&obj_context->decode_state.pic_param,
                                obj_buffer->buffer_store);

    return VA_STATUS_SUCCESS;
}

static VAStatus
i965_render_iq_matrix_buffer(VADriverContextP ctx,
                             struct object_context *obj_context,
                             struct object_buffer *obj_buffer)
{
    assert(obj_buffer->buffer_store->bo == NULL);
    assert(obj_buffer->buffer_store->buffer);
    i965_release_buffer_store(&obj_context->decode_state.iq_matrix);
    i965_reference_buffer_store(&obj_context->decode_state.iq_matrix,
                                obj_buffer->buffer_store);

    return VA_STATUS_SUCCESS;
}

static VAStatus
i965_render_bit_plane_buffer(VADriverContextP ctx,
                             struct object_context *obj_context,
                             struct object_buffer *obj_buffer)
{
    assert(obj_buffer->buffer_store->bo == NULL);
    assert(obj_buffer->buffer_store->buffer);
    i965_release_buffer_store(&obj_context->decode_state.bit_plane);
    i965_reference_buffer_store(&obj_context->decode_state.bit_plane,
                                obj_buffer->buffer_store);
    
    return VA_STATUS_SUCCESS;
}

static VAStatus
i965_render_slice_parameter_buffer(VADriverContextP ctx,
                                   struct object_context *obj_context,
                                   struct object_buffer *obj_buffer)
{
    assert(obj_buffer->buffer_store->bo == NULL);
    assert(obj_buffer->buffer_store->buffer);
    
    if (obj_context->decode_state.num_slice_params == obj_context->decode_state.max_slice_params) {
        obj_context->decode_state.slice_params = realloc(obj_context->decode_state.slice_params,
                                                         (obj_context->decode_state.max_slice_params + NUM_SLICES) * sizeof(*obj_context->decode_state.slice_params));
        memset(obj_context->decode_state.slice_params + obj_context->decode_state.max_slice_params, 0, NUM_SLICES * sizeof(*obj_context->decode_state.slice_params));
        obj_context->decode_state.max_slice_params += NUM_SLICES;
    }
        
    i965_release_buffer_store(&obj_context->decode_state.slice_params[obj_context->decode_state.num_slice_params]);
    i965_reference_buffer_store(&obj_context->decode_state.slice_params[obj_context->decode_state.num_slice_params],
                                obj_buffer->buffer_store);
    obj_context->decode_state.num_slice_params++;
    
    return VA_STATUS_SUCCESS;
}

static VAStatus
i965_render_slice_data_buffer(VADriverContextP ctx,
                              struct object_context *obj_context,
                              struct object_buffer *obj_buffer)
{
    assert(obj_buffer->buffer_store->buffer == NULL);
    assert(obj_buffer->buffer_store->bo);

    if (obj_context->decode_state.num_slice_datas == obj_context->decode_state.max_slice_datas) {
        obj_context->decode_state.slice_datas = realloc(obj_context->decode_state.slice_datas,
                                                        (obj_context->decode_state.max_slice_datas + NUM_SLICES) * sizeof(*obj_context->decode_state.slice_datas));
        memset(obj_context->decode_state.slice_datas + obj_context->decode_state.max_slice_datas, 0, NUM_SLICES * sizeof(*obj_context->decode_state.slice_datas));
        obj_context->decode_state.max_slice_datas += NUM_SLICES;
    }
        
    i965_release_buffer_store(&obj_context->decode_state.slice_datas[obj_context->decode_state.num_slice_datas]);
    i965_reference_buffer_store(&obj_context->decode_state.slice_datas[obj_context->decode_state.num_slice_datas],
                                obj_buffer->buffer_store);
    obj_context->decode_state.num_slice_datas++;
    
    return VA_STATUS_SUCCESS;
}

VAStatus 
i965_RenderPicture(VADriverContextP ctx,
                   VAContextID context,
                   VABufferID *buffers,
                   int num_buffers)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_context *obj_context;
    int i;
    VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;

    obj_context = CONTEXT(context);
    assert(obj_context);

    for (i = 0; i < num_buffers; i++) {
        struct object_buffer *obj_buffer = BUFFER(buffers[i]);
        assert(obj_buffer);

        switch (obj_buffer->type) {
        case VAPictureParameterBufferType:
            vaStatus = i965_render_picture_parameter_buffer(ctx, obj_context, obj_buffer);
            break;
            
        case VAIQMatrixBufferType:
            vaStatus = i965_render_iq_matrix_buffer(ctx, obj_context, obj_buffer);
            break;

        case VABitPlaneBufferType:
            vaStatus = i965_render_bit_plane_buffer(ctx, obj_context, obj_buffer);
            break;

        case VASliceParameterBufferType:
            vaStatus = i965_render_slice_parameter_buffer(ctx, obj_context, obj_buffer);
            break;

        case VASliceDataBufferType:
            vaStatus = i965_render_slice_data_buffer(ctx, obj_context, obj_buffer);
            break;

        default:
            break;
        }
    }

    return vaStatus;
}

VAStatus 
i965_EndPicture(VADriverContextP ctx, VAContextID context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx); 
    struct object_context *obj_context = CONTEXT(context);
    struct object_config *obj_config;
    VAContextID config;
    int i;

    assert(obj_context);
    assert(obj_context->decode_state.pic_param);
    assert(obj_context->decode_state.num_slice_params >= 1);
    assert(obj_context->decode_state.num_slice_datas >= 1);
    assert(obj_context->decode_state.num_slice_params == obj_context->decode_state.num_slice_datas);

    config = obj_context->config_id;
    obj_config = CONFIG(config);
    assert(obj_config);

    i965_media_decode_picture(ctx, obj_config->profile, &obj_context->decode_state);
    obj_context->decode_state.current_render_target = -1;
    obj_context->decode_state.num_slice_params = 0;
    obj_context->decode_state.num_slice_datas = 0;
    i965_release_buffer_store(&obj_context->decode_state.pic_param);
    i965_release_buffer_store(&obj_context->decode_state.iq_matrix);
    i965_release_buffer_store(&obj_context->decode_state.bit_plane);

    for (i = 0; i < obj_context->decode_state.num_slice_params; i++) {
        i965_release_buffer_store(&obj_context->decode_state.slice_params[i]);
        i965_release_buffer_store(&obj_context->decode_state.slice_datas[i]);
    }

    return VA_STATUS_SUCCESS;
}

VAStatus 
i965_SyncSurface(VADriverContextP ctx,
                 VASurfaceID render_target)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx); 
    struct object_surface *obj_surface = SURFACE(render_target);

    assert(obj_surface);

    return VA_STATUS_SUCCESS;
}

VAStatus 
i965_QuerySurfaceStatus(VADriverContextP ctx,
                        VASurfaceID render_target,
                        VASurfaceStatus *status)        /* out */
{
    struct i965_driver_data *i965 = i965_driver_data(ctx); 
    struct object_surface *obj_surface = SURFACE(render_target);

    assert(obj_surface);
    *status = obj_surface->status;

    return VA_STATUS_SUCCESS;
}


/* 
 * Query display attributes 
 * The caller must provide a "attr_list" array that can hold at
 * least vaMaxNumDisplayAttributes() entries. The actual number of attributes
 * returned in "attr_list" is returned in "num_attributes".
 */
VAStatus 
i965_QueryDisplayAttributes(VADriverContextP ctx,
                            VADisplayAttribute *attr_list,	/* out */
                            int *num_attributes)		/* out */
{
    if (num_attributes)
        *num_attributes = 0;

    return VA_STATUS_SUCCESS;
}

/* 
 * Get display attributes 
 * This function returns the current attribute values in "attr_list".
 * Only attributes returned with VA_DISPLAY_ATTRIB_GETTABLE set in the "flags" field
 * from vaQueryDisplayAttributes() can have their values retrieved.  
 */
VAStatus 
i965_GetDisplayAttributes(VADriverContextP ctx,
                          VADisplayAttribute *attr_list,	/* in/out */
                          int num_attributes)
{
    /* TODO */
    return VA_STATUS_ERROR_UNIMPLEMENTED;
}

/* 
 * Set display attributes 
 * Only attributes returned with VA_DISPLAY_ATTRIB_SETTABLE set in the "flags" field
 * from vaQueryDisplayAttributes() can be set.  If the attribute is not settable or 
 * the value is out of range, the function returns VA_STATUS_ERROR_ATTR_NOT_SUPPORTED
 */
VAStatus 
i965_SetDisplayAttributes(VADriverContextP ctx,
                          VADisplayAttribute *attr_list,
                          int num_attributes)
{
    /* TODO */
    return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus 
i965_DbgCopySurfaceToBuffer(VADriverContextP ctx,
                            VASurfaceID surface,
                            void **buffer,              /* out */
                            unsigned int *stride)       /* out */
{
    /* TODO */
    return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus 
i965_Init(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx); 

    if (intel_driver_init(ctx) == False)
        return VA_STATUS_ERROR_UNKNOWN;

    if (!IS_G4X(i965->intel.device_id) &&
        !IS_IRONLAKE(i965->intel.device_id))
        return VA_STATUS_ERROR_UNKNOWN;

    if (i965_media_init(ctx) == False)
        return VA_STATUS_ERROR_UNKNOWN;

    if (i965_render_init(ctx) == False)
        return VA_STATUS_ERROR_UNKNOWN;

    return VA_STATUS_SUCCESS;
}

static void
i965_destroy_heap(struct object_heap *heap, 
                  void (*func)(struct object_heap *heap, struct object_base *object))
{
    struct object_base *object;
    object_heap_iterator iter;    

    object = object_heap_first(heap, &iter);

    while (object) {
        if (func)
            func(heap, object);

        object = object_heap_next(heap, &iter);
    }

    object_heap_destroy(heap);
}


VAStatus 
i965_DestroyImage(VADriverContextP ctx, VAImageID image);

VAStatus 
i965_CreateImage(VADriverContextP ctx,
                 VAImageFormat *format,
                 int width,
                 int height,
                 VAImage *out_image)        /* out */
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_image *obj_image;
    VAStatus va_status = VA_STATUS_ERROR_OPERATION_FAILED;
    VAImageID image_id;
    unsigned int width2, height2, size2, size;

    out_image->image_id = VA_INVALID_ID;
    out_image->buf      = VA_INVALID_ID;

    image_id = NEW_IMAGE_ID();
    if (image_id == VA_INVALID_ID)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    obj_image = IMAGE(image_id);
    if (!obj_image)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    obj_image->bo         = NULL;
    obj_image->palette    = NULL;

    VAImage * const image = &obj_image->image;
    image->image_id       = image_id;
    image->buf            = VA_INVALID_ID;

    size    = width * height;
    width2  = (width  + 1) / 2;
    height2 = (height + 1) / 2;
    size2   = width2 * height2;

    image->num_palette_entries = 0;
    image->entry_bytes         = 0;
    memset(image->component_order, 0, sizeof(image->component_order));

    switch (format->fourcc) {
    case VA_FOURCC('I','A','4','4'):
    case VA_FOURCC('A','I','4','4'):
        image->num_planes = 1;
        image->pitches[0] = width;
        image->offsets[0] = 0;
        image->data_size  = image->offsets[0] + image->pitches[0] * height;
        image->num_palette_entries = 16;
        image->entry_bytes         = 3;
        image->component_order[0]  = 'R';
        image->component_order[1]  = 'G';
        image->component_order[2]  = 'B';
        break;
    case VA_FOURCC('A','R','G','B'):
    case VA_FOURCC('A','B','G','R'):
    case VA_FOURCC('B','G','R','A'):
    case VA_FOURCC('R','G','B','A'):
        image->num_planes = 1;
        image->pitches[0] = width * 4;
        image->offsets[0] = 0;
        image->data_size  = image->offsets[0] + image->pitches[0] * height;
        break;
    case VA_FOURCC('Y','V','1','2'):
        image->num_planes = 3;
        image->pitches[0] = width;
        image->offsets[0] = 0;
        image->pitches[1] = width2;
        image->offsets[1] = size + size2;
        image->pitches[2] = width2;
        image->offsets[2] = size;
        image->data_size  = size + 2 * size2;
        break;
    case VA_FOURCC('I','4','2','0'):
        image->num_planes = 3;
        image->pitches[0] = width;
        image->offsets[0] = 0;
        image->pitches[1] = width2;
        image->offsets[1] = size;
        image->pitches[2] = width2;
        image->offsets[2] = size + size2;
        image->data_size  = size + 2 * size2;
        break;
    case VA_FOURCC('N','V','1','2'):
        image->num_planes = 2;
        image->pitches[0] = width;
        image->offsets[0] = 0;
        image->pitches[1] = width;
        image->offsets[1] = size;
        image->data_size  = size + 2 * size2;
        break;
    default:
        goto error;
    }

    va_status = i965_CreateBuffer(ctx, 0, VAImageBufferType,
                                  image->data_size, 1, NULL, &image->buf);
    if (va_status != VA_STATUS_SUCCESS)
        goto error;

    obj_image->bo = BUFFER(image->buf)->buffer_store->bo;

    if (image->num_palette_entries > 0 && image->entry_bytes > 0) {
        obj_image->palette = malloc(image->num_palette_entries * sizeof(obj_image->palette));
        if (!obj_image->palette)
            goto error;
    }

    image->image_id             = image_id;
    image->format               = *format;
    image->width                = width;
    image->height               = height;

    *out_image                  = *image;
    return VA_STATUS_SUCCESS;

error:
    i965_DestroyImage(ctx, image_id);
    return va_status;
}

VAStatus i965_DeriveImage(VADriverContextP ctx,
                          VASurfaceID surface,
                          VAImage *image)        /* out */
{
    /* TODO */
    return VA_STATUS_ERROR_OPERATION_FAILED;
}

static void 
i965_destroy_image(struct object_heap *heap, struct object_base *obj)
{
    object_heap_free(heap, obj);
}


VAStatus 
i965_DestroyImage(VADriverContextP ctx, VAImageID image)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct object_image *obj_image = IMAGE(image); 

    if (!obj_image)
        return VA_STATUS_SUCCESS;

    if (obj_image->image.buf != VA_INVALID_ID) {
        i965_DestroyBuffer(ctx, obj_image->image.buf);
        obj_image->image.buf = VA_INVALID_ID;
    }

    if (obj_image->palette) {
        free(obj_image->palette);
        obj_image->palette = NULL;
    }

    i965_destroy_image(&i965->image_heap, (struct object_base *)obj_image);
	
    return VA_STATUS_SUCCESS;
}

/*
 * pointer to an array holding the palette data.  The size of the array is
 * num_palette_entries * entry_bytes in size.  The order of the components
 * in the palette is described by the component_order in VASubpicture struct
 */
VAStatus 
i965_SetImagePalette(VADriverContextP ctx,
                     VAImageID image,
                     unsigned char *palette)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    unsigned int i;

    struct object_image *obj_image = IMAGE(image);
    if (!obj_image)
        return VA_STATUS_ERROR_INVALID_IMAGE;

    if (!obj_image->palette)
        return VA_STATUS_ERROR_ALLOCATION_FAILED; /* XXX: unpaletted/error */

    for (i = 0; i < obj_image->image.num_palette_entries; i++)
        obj_image->palette[i] = (((unsigned int)palette[3*i + 0] << 16) |
                                 ((unsigned int)palette[3*i + 1] <<  8) |
                                 (unsigned int)palette[3*i + 2]);
    return VA_STATUS_SUCCESS;
}

static inline void
memcpy_pic(uint8_t *dst, unsigned int dst_stride,
           const uint8_t *src, unsigned int src_stride,
           unsigned int len, unsigned int height)
{
    unsigned int i;

    for (i = 0; i < height; i++) {
        memcpy(dst, src, len);
        dst += dst_stride;
        src += src_stride;
    }
}

static void
get_image_i420(struct object_image *obj_image, uint8_t *image_data,
               struct object_surface *obj_surface,
               const VARectangle *rect)
{
    uint8_t *dst[3], *src[3];
    const int Y = 0;
    const int U = obj_image->image.format.fourcc == VA_FOURCC_YV12 ? 2 : 1;
    const int V = obj_image->image.format.fourcc == VA_FOURCC_YV12 ? 1 : 2;

    if (!obj_surface->bo)
        return;

    dri_bo_map(obj_surface->bo, 0);

    if (!obj_surface->bo->virtual)
        return;

    /* Dest VA image has either I420 or YV12 format.
       Source VA surface alway has I420 format */
    dst[Y] = image_data + obj_image->image.offsets[Y];
    src[0] = (uint8_t *)obj_surface->bo->virtual;
    dst[U] = image_data + obj_image->image.offsets[U];
    src[1] = src[0] + obj_surface->width * obj_surface->height;
    dst[V] = image_data + obj_image->image.offsets[V];
    src[2] = src[1] + (obj_surface->width / 2) * (obj_surface->height / 2);

    /* Y plane */
    dst[Y] += rect->y * obj_image->image.pitches[Y] + rect->x;
    src[0] += rect->y * obj_surface->width + rect->x;
    memcpy_pic(dst[Y], obj_image->image.pitches[Y],
               src[0], obj_surface->width,
               rect->width, rect->height);

    /* U plane */
    dst[U] += (rect->y / 2) * obj_image->image.pitches[U] + rect->x / 2;
    src[1] += (rect->y / 2) * obj_surface->width / 2 + rect->x / 2;
    memcpy_pic(dst[U], obj_image->image.pitches[U],
               src[1], obj_surface->width / 2,
               rect->width / 2, rect->height / 2);

    /* V plane */
    dst[V] += (rect->y / 2) * obj_image->image.pitches[V] + rect->x / 2;
    src[2] += (rect->y / 2) * obj_surface->width / 2 + rect->x / 2;
    memcpy_pic(dst[V], obj_image->image.pitches[V],
               src[2], obj_surface->width / 2,
               rect->width / 2, rect->height / 2);

    dri_bo_unmap(obj_surface->bo);
}

static void
get_image_nv12(struct object_image *obj_image, uint8_t *image_data,
               struct object_surface *obj_surface,
               const VARectangle *rect)
{
    uint8_t *dst[2], *src[2];

    if (!obj_surface->bo)
        return;

    dri_bo_map(obj_surface->bo, 0);

    if (!obj_surface->bo->virtual)
        return;

    /* Both dest VA image and source surface have NV12 format */
    dst[0] = image_data + obj_image->image.offsets[0];
    src[0] = (uint8_t *)obj_surface->bo->virtual;
    dst[1] = image_data + obj_image->image.offsets[1];
    src[1] = src[0] + obj_surface->width * obj_surface->height;

    /* Y plane */
    dst[0] += rect->y * obj_image->image.pitches[0] + rect->x;
    src[0] += rect->y * obj_surface->width + rect->x;
    memcpy_pic(dst[0], obj_image->image.pitches[0],
               src[0], obj_surface->width,
               rect->width, rect->height);

    /* UV plane */
    dst[1] += (rect->y / 2) * obj_image->image.pitches[1] + (rect->x & -2);
    src[1] += (rect->y / 2) * obj_surface->width + (rect->x & -2);
    memcpy_pic(dst[1], obj_image->image.pitches[1],
               src[1], obj_surface->width,
               rect->width, rect->height / 2);

    dri_bo_unmap(obj_surface->bo);
}

VAStatus 
i965_GetImage(VADriverContextP ctx,
              VASurfaceID surface,
              int x,   /* coordinates of the upper left source pixel */
              int y,
              unsigned int width,      /* width and height of the region */
              unsigned int height,
              VAImageID image)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;

    struct object_surface *obj_surface = SURFACE(surface);
    if (!obj_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    struct object_image *obj_image = IMAGE(image);
    if (!obj_image)
        return VA_STATUS_ERROR_INVALID_IMAGE;

    if (x < 0 || y < 0)
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    if (x + width > obj_surface->orig_width ||
        y + height > obj_surface->orig_height)
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    if (x + width > obj_image->image.width ||
        y + height > obj_image->image.height)
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    /* Commit pending operations to the HW */
    intel_batchbuffer_flush(ctx);

    VAStatus va_status;
    void *image_data = NULL;

    va_status = i965_MapBuffer(ctx, obj_image->image.buf, &image_data);
    if (va_status != VA_STATUS_SUCCESS)
        return va_status;

    VARectangle rect;
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;

    switch (obj_image->image.format.fourcc) {
    case VA_FOURCC('Y','V','1','2'):
    case VA_FOURCC('I','4','2','0'):
        /* I420 is native format for MPEG-2 decoded surfaces */
        if (render_state->interleaved_uv)
            goto operation_failed;
        get_image_i420(obj_image, image_data, obj_surface, &rect);
        break;
    case VA_FOURCC('N','V','1','2'):
        /* NV12 is native format for H.264 decoded surfaces */
        if (!render_state->interleaved_uv)
            goto operation_failed;
        get_image_nv12(obj_image, image_data, obj_surface, &rect);
        break;
    default:
    operation_failed:
        va_status = VA_STATUS_ERROR_OPERATION_FAILED;
        break;
    }

    i965_UnmapBuffer(ctx, obj_image->image.buf);
    return va_status;
}

VAStatus 
i965_PutSurface(VADriverContextP ctx,
                VASurfaceID surface,
                void *draw, /* X Drawable */
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
                unsigned int flags) /* de-interlacing flags */
{
    struct i965_driver_data *i965 = i965_driver_data(ctx); 
    struct dri_state *dri_state = (struct dri_state *)ctx->dri_state;
    struct i965_render_state *render_state = &i965->render_state;
    struct dri_drawable *dri_drawable;
    union dri_buffer *buffer;
    struct intel_region *dest_region;
    struct object_surface *obj_surface; 
    int ret;
    uint32_t name;
    Bool new_region = False;
    int pp_flag = 0;
    /* Currently don't support DRI1 */
    if (dri_state->driConnectedFlag != VA_DRI2)
        return VA_STATUS_ERROR_UNKNOWN;

    /* Some broken sources such as H.264 conformance case FM2_SVA_C
     * will get here
     */
    obj_surface = SURFACE(surface);
    if (obj_surface->bo == NULL)
        return VA_STATUS_SUCCESS;

    dri_drawable = dri_get_drawable(ctx, (Drawable)draw);
    assert(dri_drawable);

    buffer = dri_get_rendering_buffer(ctx, dri_drawable);
    assert(buffer);
    
    dest_region = render_state->draw_region;

    if (dest_region) {
        assert(dest_region->bo);
        dri_bo_flink(dest_region->bo, &name);
        
        if (buffer->dri2.name != name) {
            new_region = True;
            dri_bo_unreference(dest_region->bo);
        }
    } else {
        dest_region = (struct intel_region *)calloc(1, sizeof(*dest_region));
        assert(dest_region);
        render_state->draw_region = dest_region;
        new_region = True;
    }

    if (new_region) {
        dest_region->x = dri_drawable->x;
        dest_region->y = dri_drawable->y;
        dest_region->width = dri_drawable->width;
        dest_region->height = dri_drawable->height;
        dest_region->cpp = buffer->dri2.cpp;
        dest_region->pitch = buffer->dri2.pitch;

        dest_region->bo = intel_bo_gem_create_from_name(i965->intel.bufmgr, "rendering buffer", buffer->dri2.name);
        assert(dest_region->bo);

        ret = dri_bo_get_tiling(dest_region->bo, &(dest_region->tiling), &(dest_region->swizzle));
        assert(ret == 0);
    }

    if ((flags & VA_FILTER_SCALING_MASK) == VA_FILTER_SCALING_NL_ANAMORPHIC)
        pp_flag |= I965_PP_FLAG_AVS;

    if (flags & (VA_BOTTOM_FIELD | VA_TOP_FIELD))
        pp_flag |= I965_PP_FLAG_DEINTERLACING;

    i965_render_put_surface(ctx, surface,
                            srcx, srcy, srcw, srch,
                            destx, desty, destw, desth,
                            pp_flag);

    if(obj_surface->subpic != VA_INVALID_ID) {	
	i965_render_put_subpic(ctx, surface,
                               srcx, srcy, srcw, srch,
                               destx, desty, destw, desth);
    } 

    dri_swap_buffer(ctx, dri_drawable);
    obj_surface->flags |= SURFACE_DISPLAYED;

    if (!(obj_surface->flags & SURFACE_REFERENCED)) {
        dri_bo_unreference(obj_surface->bo);
        obj_surface->bo = NULL;
        obj_surface->flags = 0;

        if (obj_surface->free_private_data)
            obj_surface->free_private_data(&obj_surface->private_data);
    }

    return VA_STATUS_SUCCESS;
}

VAStatus 
i965_Terminate(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);

    if (i965_render_terminate(ctx) == False)
	return VA_STATUS_ERROR_UNKNOWN;

    if (i965_media_terminate(ctx) == False)
	return VA_STATUS_ERROR_UNKNOWN;

    if (intel_driver_terminate(ctx) == False)
	return VA_STATUS_ERROR_UNKNOWN;

    i965_destroy_heap(&i965->buffer_heap, i965_destroy_buffer);
    i965_destroy_heap(&i965->image_heap, i965_destroy_image);
    i965_destroy_heap(&i965->subpic_heap, i965_destroy_subpic);
    i965_destroy_heap(&i965->surface_heap, i965_destroy_surface);
    i965_destroy_heap(&i965->context_heap, i965_destroy_context);
    i965_destroy_heap(&i965->config_heap, i965_destroy_config);

    free(ctx->pDriverData);
    ctx->pDriverData = NULL;

    return VA_STATUS_SUCCESS;
}

VAStatus 
__vaDriverInit_0_31(  VADriverContextP ctx )
{
    struct i965_driver_data *i965;
    int result;

    ctx->version_major = VA_MAJOR_VERSION;
    ctx->version_minor = VA_MINOR_VERSION;
    ctx->max_profiles = I965_MAX_PROFILES;
    ctx->max_entrypoints = I965_MAX_ENTRYPOINTS;
    ctx->max_attributes = I965_MAX_CONFIG_ATTRIBUTES;
    ctx->max_image_formats = I965_MAX_IMAGE_FORMATS;
    ctx->max_subpic_formats = I965_MAX_SUBPIC_FORMATS;
    ctx->max_display_attributes = I965_MAX_DISPLAY_ATTRIBUTES;
    ctx->str_vendor = I965_STR_VENDOR;

    ctx->vtable.vaTerminate = i965_Terminate;
    ctx->vtable.vaQueryConfigEntrypoints = i965_QueryConfigEntrypoints;
    ctx->vtable.vaQueryConfigProfiles = i965_QueryConfigProfiles;
    ctx->vtable.vaQueryConfigEntrypoints = i965_QueryConfigEntrypoints;
    ctx->vtable.vaQueryConfigAttributes = i965_QueryConfigAttributes;
    ctx->vtable.vaCreateConfig = i965_CreateConfig;
    ctx->vtable.vaDestroyConfig = i965_DestroyConfig;
    ctx->vtable.vaGetConfigAttributes = i965_GetConfigAttributes;
    ctx->vtable.vaCreateSurfaces = i965_CreateSurfaces;
    ctx->vtable.vaDestroySurfaces = i965_DestroySurfaces;
    ctx->vtable.vaCreateContext = i965_CreateContext;
    ctx->vtable.vaDestroyContext = i965_DestroyContext;
    ctx->vtable.vaCreateBuffer = i965_CreateBuffer;
    ctx->vtable.vaBufferSetNumElements = i965_BufferSetNumElements;
    ctx->vtable.vaMapBuffer = i965_MapBuffer;
    ctx->vtable.vaUnmapBuffer = i965_UnmapBuffer;
    ctx->vtable.vaDestroyBuffer = i965_DestroyBuffer;
    ctx->vtable.vaBeginPicture = i965_BeginPicture;
    ctx->vtable.vaRenderPicture = i965_RenderPicture;
    ctx->vtable.vaEndPicture = i965_EndPicture;
    ctx->vtable.vaSyncSurface = i965_SyncSurface;
    ctx->vtable.vaQuerySurfaceStatus = i965_QuerySurfaceStatus;
    ctx->vtable.vaPutSurface = i965_PutSurface;
    ctx->vtable.vaQueryImageFormats = i965_QueryImageFormats;
    ctx->vtable.vaCreateImage = i965_CreateImage;
    ctx->vtable.vaDeriveImage = i965_DeriveImage;
    ctx->vtable.vaDestroyImage = i965_DestroyImage;
    ctx->vtable.vaSetImagePalette = i965_SetImagePalette;
    ctx->vtable.vaGetImage = i965_GetImage;
    ctx->vtable.vaPutImage = i965_PutImage;
    ctx->vtable.vaQuerySubpictureFormats = i965_QuerySubpictureFormats;
    ctx->vtable.vaCreateSubpicture = i965_CreateSubpicture;
    ctx->vtable.vaDestroySubpicture = i965_DestroySubpicture;
    ctx->vtable.vaSetSubpictureImage = i965_SetSubpictureImage;
    ctx->vtable.vaSetSubpictureChromakey = i965_SetSubpictureChromakey;
    ctx->vtable.vaSetSubpictureGlobalAlpha = i965_SetSubpictureGlobalAlpha;
    ctx->vtable.vaAssociateSubpicture = i965_AssociateSubpicture;
    ctx->vtable.vaDeassociateSubpicture = i965_DeassociateSubpicture;
    ctx->vtable.vaQueryDisplayAttributes = i965_QueryDisplayAttributes;
    ctx->vtable.vaGetDisplayAttributes = i965_GetDisplayAttributes;
    ctx->vtable.vaSetDisplayAttributes = i965_SetDisplayAttributes;
//    ctx->vtable.vaDbgCopySurfaceToBuffer = i965_DbgCopySurfaceToBuffer;

    i965 = (struct i965_driver_data *)calloc(1, sizeof(*i965));
    assert(i965);
    ctx->pDriverData = (void *)i965;

    result = object_heap_init(&i965->config_heap, 
                              sizeof(struct object_config), 
                              CONFIG_ID_OFFSET);
    assert(result == 0);

    result = object_heap_init(&i965->context_heap, 
                              sizeof(struct object_context), 
                              CONTEXT_ID_OFFSET);
    assert(result == 0);

    result = object_heap_init(&i965->surface_heap, 
                              sizeof(struct object_surface), 
                              SURFACE_ID_OFFSET);
    assert(result == 0);

    result = object_heap_init(&i965->buffer_heap, 
                              sizeof(struct object_buffer), 
                              BUFFER_ID_OFFSET);
    assert(result == 0);

    result = object_heap_init(&i965->image_heap, 
                              sizeof(struct object_image), 
                              IMAGE_ID_OFFSET);
    assert(result == 0);
	
    result = object_heap_init(&i965->subpic_heap, 
                              sizeof(struct object_subpic), 
                              SUBPIC_ID_OFFSET);
    assert(result == 0);
    
    return i965_Init(ctx);
}
