/*
 * Copyright (c) 2007 Intel Corporation. All Rights Reserved.
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

#include "va_backend.h"

#include "dummy_drv_video.h"

#include "assert.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define ASSERT	assert

#define INIT_DRIVER_DATA	struct dummy_driver_data *driver_data = (struct dummy_driver_data *) ctx->pDriverData;

#define CONFIG(id)  ((object_config_p) object_heap_lookup( &driver_data->config_heap, id ))
#define CONTEXT(id) ((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))
#define SURFACE(id)	((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &driver_data->buffer_heap, id ))

#define CONFIG_ID_OFFSET		0x01000000
#define CONTEXT_ID_OFFSET		0x02000000
#define SURFACE_ID_OFFSET		0x04000000
#define BUFFER_ID_OFFSET		0x08000000

static void dummy__error_message(const char *msg, ...)
{
    va_list args;

    fprintf(stderr, "dummy_drv_video error: ");
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

static void dummy__information_message(const char *msg, ...)
{
    va_list args;

    fprintf(stderr, "dummy_drv_video: ");
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

VAStatus dummy_QueryConfigProfiles(
		VADriverContextP ctx,
		VAProfile *profile_list,	/* out */
		int *num_profiles			/* out */
	)
{
    INIT_DRIVER_DATA
    int i = 0;
    
    profile_list[i++] = VAProfileMPEG2Simple;
    profile_list[i++] = VAProfileMPEG2Main;
    profile_list[i++] = VAProfileMPEG4Simple;
    profile_list[i++] = VAProfileMPEG4AdvancedSimple;
    profile_list[i++] = VAProfileMPEG4Main;
    profile_list[i++] = VAProfileH264Baseline;
    profile_list[i++] = VAProfileH264Main;
    profile_list[i++] = VAProfileH264High;
    profile_list[i++] = VAProfileVC1Simple;
    profile_list[i++] = VAProfileVC1Main;
    profile_list[i++] = VAProfileVC1Advanced;

    /* If the assert fails then DUMMY_MAX_PROFILES needs to be bigger */
    ASSERT(i <= DUMMY_MAX_PROFILES);
    *num_profiles = i;

	return VA_STATUS_SUCCESS;
}

VAStatus dummy_QueryConfigEntrypoints(
		VADriverContextP ctx,
		VAProfile profile,
		VAEntrypoint  *entrypoint_list,	/* out */
		int *num_entrypoints		/* out */
	)
{
    INIT_DRIVER_DATA
    
    switch (profile) {
        case VAProfileMPEG2Simple:
        case VAProfileMPEG2Main:
                *num_entrypoints = 2;
                entrypoint_list[0] = VAEntrypointVLD;
                entrypoint_list[1] = VAEntrypointMoComp;
                break;

        case VAProfileMPEG4Simple:
        case VAProfileMPEG4AdvancedSimple:
        case VAProfileMPEG4Main:
                *num_entrypoints = 1;
                entrypoint_list[0] = VAEntrypointVLD;
                break;

        case VAProfileH264Baseline:
        case VAProfileH264Main:
        case VAProfileH264High:
                *num_entrypoints = 1;
                entrypoint_list[0] = VAEntrypointVLD;
                break;

        case VAProfileVC1Simple:
        case VAProfileVC1Main:
        case VAProfileVC1Advanced:
                *num_entrypoints = 1;
                entrypoint_list[0] = VAEntrypointVLD;
                break;

        default:
                *num_entrypoints = 0;
                break;
    }

    /* If the assert fails then DUMMY_MAX_ENTRYPOINTS needs to be bigger */
    ASSERT(*num_entrypoints <= DUMMY_MAX_ENTRYPOINTS);
	return VA_STATUS_SUCCESS;
}

VAStatus dummy_QueryConfigAttributes(
		VADriverContextP ctx,
		VAProfile profile,
		VAEntrypoint entrypoint,
		VAConfigAttrib *attrib_list,	/* in/out */
		int num_attribs
	)
{
    INIT_DRIVER_DATA

    int i;

    /* Other attributes don't seem to be defined */
    /* What to do if we don't know the attribute? */
    for (i = 0; i < num_attribs; i++)
    {
        switch (attrib_list[i].type)
        {
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

static VAStatus dummy__update_attribute(object_config_p obj_config, VAConfigAttrib *attrib)
{
    int i;
    /* Check existing attrbiutes */
    for(i = 0; obj_config->attrib_count < i; i++)
    {
        if (obj_config->attrib_list[i].type == attrib->type)
        {
            /* Update existing attribute */
            obj_config->attrib_list[i].value = attrib->value;
            return VA_STATUS_SUCCESS;
        }
    }
    if (obj_config->attrib_count < DUMMY_MAX_CONFIG_ATTRIBUTES)
    {
        i = obj_config->attrib_count;
        obj_config->attrib_list[i].type = attrib->type;
        obj_config->attrib_list[i].value = attrib->value;
        obj_config->attrib_count++;
        return VA_STATUS_SUCCESS;
    }
    return VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
}

VAStatus dummy_CreateConfig(
		VADriverContextP ctx,
		VAProfile profile, 
		VAEntrypoint entrypoint, 
		VAConfigAttrib *attrib_list,
		int num_attribs,
		VAConfigID *config_id		/* out */
	)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus;
    int configID;
    object_config_p obj_config;
    int i;
    
    /* Validate profile & entrypoint */
    switch (profile) {
        case VAProfileMPEG2Simple:
        case VAProfileMPEG2Main:
                if ((VAEntrypointVLD == entrypoint) ||
                    (VAEntrypointMoComp == entrypoint))
                {
                    vaStatus = VA_STATUS_SUCCESS;
                }
                else
                {
                    vaStatus = VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
                }
                break;

        case VAProfileMPEG4Simple:
        case VAProfileMPEG4AdvancedSimple:
        case VAProfileMPEG4Main:
                if (VAEntrypointVLD == entrypoint)
                {
                    vaStatus = VA_STATUS_SUCCESS;
                }
                else
                {
                    vaStatus = VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
                }
                break;

        case VAProfileH264Baseline:
        case VAProfileH264Main:
        case VAProfileH264High:
                if (VAEntrypointVLD == entrypoint)
                {
                    vaStatus = VA_STATUS_SUCCESS;
                }
                else
                {
                    vaStatus = VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
                }
                break;

        case VAProfileVC1Simple:
        case VAProfileVC1Main:
        case VAProfileVC1Advanced:
                if (VAEntrypointVLD == entrypoint)
                {
                    vaStatus = VA_STATUS_SUCCESS;
                }
                else
                {
                    vaStatus = VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
                }
                break;

        default:
                vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
                break;
    }

    if (VA_STATUS_SUCCESS != vaStatus)
    {
        return vaStatus;
    }

    configID = object_heap_allocate( &driver_data->config_heap );
    obj_config = CONFIG(configID);
    if (NULL == obj_config)
    {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        return vaStatus;
    }

    obj_config->profile = profile;
    obj_config->entrypoint = entrypoint;
    obj_config->attrib_list[0].type = VAConfigAttribRTFormat;
    obj_config->attrib_list[0].value = VA_RT_FORMAT_YUV420;
    obj_config->attrib_count = 1;

    for(i = 0; i < num_attribs; i++)
    {
        vaStatus = dummy__update_attribute(obj_config, &(attrib_list[i]));
        if (VA_STATUS_SUCCESS != vaStatus)
        {
            break;
        }
    }
    
    /* Error recovery */
    if (VA_STATUS_SUCCESS != vaStatus)
    {
        object_heap_free( &driver_data->config_heap, (object_base_p) obj_config);
    }
    else
    {
        *config_id = configID;
    }

	return vaStatus;
}
	
VAStatus dummy_GetConfigAttributes(
		VADriverContextP ctx,
		VAConfigID config_id, 
		VAProfile *profile,		/* out */
		VAEntrypoint *entrypoint, 	/* out */
		VAConfigAttrib *attrib_list,	/* out */
		int *num_attribs		/* out */
	)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_config_p obj_config;
    int i;

    obj_config = CONFIG(config_id);
    ASSERT(obj_config);
    
    *profile = obj_config->profile;
    *entrypoint = obj_config->entrypoint;
    *num_attribs =  obj_config->attrib_count;
    for(i = 0; i < obj_config->attrib_count; i++)
    {
        attrib_list[i] = obj_config->attrib_list[i];
    }
    
	return vaStatus;
}

VAStatus dummy_CreateSurfaces(
		VADriverContextP ctx,
		int width,
		int height,
		int format,
		int num_surfaces,
		VASurface *surfaces		/* out */
	)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int i;
    
    /* We only support one format */
    if (VA_RT_FORMAT_YUV420 != format)
    {
        return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
    }
    
    for (i = 0; i < num_surfaces; i++)
    {
        int surfaceID = object_heap_allocate( &driver_data->surface_heap );
        object_surface_p obj_surface = SURFACE(surfaceID);
        if (NULL == obj_surface)
        {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            break;
        }
        obj_surface->surface = &(surfaces[i]);
        obj_surface->surface->surface_id = surfaceID;
        obj_surface->surface->context_id = -1;
        obj_surface->surface->width = width;
        obj_surface->surface->height = height;
        obj_surface->surface->format = format;
        obj_surface->surface->privData = NULL;
    }

    /* Error recovery */
    if (VA_STATUS_SUCCESS != vaStatus)
    {
        /* surfaces[i-1] was the last successful allocation */
        for(; i--; )
        {
            object_surface_p obj_surface = SURFACE(surfaces[i].surface_id);
            surfaces[i].surface_id = -1;
            ASSERT(obj_surface);
            object_heap_free( &driver_data->surface_heap, (object_base_p) obj_surface);
        }
    }

	return vaStatus;
}
	
VAStatus dummy_DestroySurface(
		VADriverContextP ctx,
		VASurface *surface_list,
		int num_surfaces
	)
{
    INIT_DRIVER_DATA
    int i;
    for(i = num_surfaces; i--; )
    {
        object_surface_p obj_surface = SURFACE(surface_list[i].surface_id);
        ASSERT(obj_surface);
        object_heap_free( &driver_data->surface_heap, (object_base_p) obj_surface);
    }
	return VA_STATUS_SUCCESS;
}

VAStatus dummy_CreateContext(
		VADriverContextP ctx,
		VAConfigID config_id,
		int picture_width,
		int picture_height,
		int flag,
		VASurface *render_targets,
		int num_render_targets,
		VAContext *context		/* out */
	)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_config_p obj_config;
    int i;

    obj_config = CONFIG(config_id);
    if (NULL == obj_config)
    {
        vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
        return vaStatus;
    }
    
    /* Validate flag */
    /* Validate picture dimensions */

    int contextID = object_heap_allocate( &driver_data->context_heap );
    object_context_p obj_context = CONTEXT(contextID);
    if (NULL == obj_context)
    {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        return vaStatus;
    }

    obj_context->context = context;
    obj_context->current_render_target = -1;
    
    obj_context->context->context_id = contextID;
    obj_context->context->config_id = config_id;
    obj_context->context->picture_width = picture_width;
    obj_context->context->picture_height = picture_height;
    obj_context->context->num_render_targets = num_render_targets;
    obj_context->context->render_targets = (VASurfaceID *) malloc(num_render_targets * sizeof(VASurfaceID));
    for(i = 0; i < num_render_targets; i++)
    {
        if (NULL == SURFACE(render_targets[i].surface_id))
        {
            vaStatus = VA_STATUS_ERROR_INVALID_SURFACE;
            break;
        }
        obj_context->context->render_targets[i] = render_targets[i].surface_id;
    }
    obj_context->context->flags = flag;
    obj_context->context->privData = NULL;

    /* Error recovery */
    if (VA_STATUS_SUCCESS != vaStatus)
    {
        free(obj_context->context->render_targets);
        obj_context->context->render_targets = NULL;
        obj_context->context->context_id = -1;
        obj_context->context->config_id = -1;
        obj_context->context->picture_width = 0;
        obj_context->context->picture_height = 0;
        free(obj_context->context->render_targets);
        obj_context->context->render_targets = NULL;
        obj_context->context->num_render_targets = 0;
        obj_context->context->flags = 0;
        obj_context->context->privData = NULL;
        object_heap_free( &driver_data->context_heap, (object_base_p) obj_context);
    }

	return vaStatus;
}


VAStatus dummy_DestroyContext(
		VADriverContextP ctx,
		VAContext *context
	)
{
    INIT_DRIVER_DATA
    object_context_p obj_context = CONTEXT(context->context_id);
    ASSERT(obj_context);
    
    obj_context->context->context_id = -1;
    obj_context->context->config_id = -1;
    obj_context->context->picture_width = 0;
    obj_context->context->picture_height = 0;
    if (obj_context->context->render_targets)
    {
        free(obj_context->context->render_targets);
    }
    obj_context->context->render_targets = NULL;
    obj_context->context->num_render_targets = 0;
    obj_context->context->flags = 0;
    obj_context->context->privData = NULL;

    obj_context->context = NULL;
    obj_context->current_render_target = -1;

    object_heap_free( &driver_data->context_heap, (object_base_p) obj_context);

	return VA_STATUS_SUCCESS;
}


VAStatus dummy_CreateBuffer(
		VADriverContextP ctx,
		VABufferType type,  /* in */
		VABufferID *buf_desc	/* out */
	)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    int bufferID;
    object_buffer_p obj_buffer;
    
    /* Validate type */
    switch (type)
    {
        case VAPictureParameterBufferType:
        case VAIQMatrixBufferType:
        case VASliceParameterBufferType:
        case VASliceDataBufferType:
        case VAMacroblockParameterBufferType:
        case VAResidualDataBufferType:
        case VADeblockingParameterBufferType:
            /* Ok */
            break;
        default:
            vaStatus = VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
            return vaStatus;
    }

    bufferID = object_heap_allocate( &driver_data->buffer_heap );
    obj_buffer = BUFFER(bufferID);
    if (NULL == obj_buffer)
    {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        return vaStatus;
    }

    obj_buffer->buffer_data = NULL;
    
    *buf_desc = bufferID;

	return vaStatus;
}

static VAStatus dummy__allocate_buffer(object_buffer_p obj_buffer, int size)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    obj_buffer->buffer_data = realloc(obj_buffer->buffer_data, size);
    if (NULL == obj_buffer->buffer_data)
    {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
    }
    return vaStatus;
}

VAStatus dummy_BufferData(
		VADriverContextP ctx,
		VABufferID buf_id,	/* in */
        unsigned int size,	/* in */
        unsigned int num_elements,	/* in */
        void *data		/* in */
	)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_buffer_p obj_buffer = BUFFER(buf_id);
    ASSERT(obj_buffer);
    
    vaStatus = dummy__allocate_buffer(obj_buffer, size * num_elements);
    if (VA_STATUS_SUCCESS == vaStatus)
    {
        obj_buffer->max_num_elements = num_elements;
        obj_buffer->num_elements = num_elements;
        if (data)
        {
            memcpy(obj_buffer->buffer_data, data, size * num_elements);
        }
    }

	return vaStatus;
}

VAStatus dummy_BufferSetNumElements(
		VADriverContextP ctx,
		VABufferID buf_id,	/* in */
        unsigned int num_elements	/* in */
	)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_buffer_p obj_buffer = BUFFER(buf_id);
    ASSERT(obj_buffer);
    
    if ((num_elements < 0) || (num_elements > obj_buffer->max_num_elements))
    {
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
    }
    if (VA_STATUS_SUCCESS == vaStatus)
    {
        obj_buffer->num_elements = num_elements;
    }

	return vaStatus;
}

VAStatus dummy_MapBuffer(
		VADriverContextP ctx,
		VABufferID buf_id,	/* in */
		void **pbuf         /* out */
	)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
    object_buffer_p obj_buffer = BUFFER(buf_id);
    ASSERT(obj_buffer);
    if (NULL == obj_buffer)
    {
        vaStatus = VA_STATUS_ERROR_INVALID_BUFFER;
        return vaStatus;
    }
    
    if (NULL != obj_buffer->buffer_data)
    {
        *pbuf = obj_buffer->buffer_data;
        vaStatus = VA_STATUS_SUCCESS;
    }
	return vaStatus;
}

VAStatus dummy_UnmapBuffer(
		VADriverContextP ctx,
		VABufferID buf_id	/* in */
	)
{
    /* Do nothing */
	return VA_STATUS_SUCCESS;
}

static void dummy__destroy_buffer(struct dummy_driver_data *driver_data, object_buffer_p obj_buffer)
{
    if (NULL != obj_buffer->buffer_data)
    {
        free(obj_buffer->buffer_data);
        obj_buffer->buffer_data = NULL;
    }
    
    object_heap_free( &driver_data->buffer_heap, (object_base_p) obj_buffer);
}

VAStatus dummy_DestroyBuffer(
		VADriverContextP ctx,
		VABufferID buffer_id
	)
{
    INIT_DRIVER_DATA
    object_buffer_p obj_buffer = BUFFER(buffer_id);
    ASSERT(obj_buffer);

    dummy__destroy_buffer(driver_data, obj_buffer);
	return VA_STATUS_SUCCESS;
}

VAStatus dummy_BeginPicture(
		VADriverContextP ctx,
		VAContext *context,
		VASurface *render_target
	)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_context_p obj_context;
    object_surface_p obj_surface;
    
    obj_context = CONTEXT(context->context_id);
    ASSERT(obj_context);

    obj_surface = SURFACE(render_target->surface_id);
    ASSERT(obj_surface);

    obj_context->current_render_target = obj_surface->base.id;
    
	return vaStatus;
}

VAStatus dummy_RenderPicture(
		VADriverContextP ctx,
		VAContext *context,
		VABufferID *buffers,
		int num_buffers
	)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_context_p obj_context;
    object_surface_p obj_surface;
    int i;
    
    obj_context = CONTEXT(context->context_id);
    ASSERT(obj_context);

    obj_surface = SURFACE(obj_context->current_render_target);
    ASSERT(obj_surface);
    
    /* verify that we got valid buffer references */
    for(i = 0; i < num_buffers; i++)
    {
        object_buffer_p obj_buffer = BUFFER(buffers[i]);
        ASSERT(obj_buffer);
        if (NULL == obj_buffer)
        {
            vaStatus = VA_STATUS_ERROR_INVALID_BUFFER;
            break;
        }
    }

	return vaStatus;
}

VAStatus dummy_EndPicture(
		VADriverContextP ctx,
		VAContext *context
	)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_context_p obj_context;
    object_surface_p obj_surface;
    
    obj_context = CONTEXT(context->context_id);
    ASSERT(obj_context);

    obj_surface = SURFACE(obj_context->current_render_target);
    ASSERT(obj_surface);
    
    // For now, assume that we are done with rendering right away
    obj_context->current_render_target = -1;

	return vaStatus;
}


VAStatus dummy_SyncSurface(
		VADriverContextP ctx,
		VAContext *context,
		VASurface *render_target
	)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_context_p obj_context;
    object_surface_p obj_surface;
    
    obj_context = CONTEXT(context->context_id);
    ASSERT(obj_context);

    obj_surface = SURFACE(render_target->surface_id);
    ASSERT(obj_surface);

    /* Assume that this shouldn't be called before vaEndPicture() */
    ASSERT( obj_context->current_render_target != obj_surface->base.id );
    
	return vaStatus;
}

VAStatus dummy_QuerySurfaceStatus(
		VADriverContextP ctx,
		VAContext *context,
		VASurface *render_target,
		VASurfaceStatus *status	/* out */
	)
{
    INIT_DRIVER_DATA
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_context_p obj_context;
    object_surface_p obj_surface;
    
    obj_context = CONTEXT(context->context_id);
    ASSERT(obj_context);

    obj_surface = SURFACE(render_target->surface_id);
    ASSERT(obj_surface);

    /* Assume that we are busy until vaEndPicture() is called */
    if ( obj_context->current_render_target == obj_surface->base.id )
    {
        *status = VASurfaceRendering;
    }
    else
    {
        *status = VASurfaceReady;
    }
    
	return vaStatus;
}

VAStatus dummy_PutSurface(
   		VADriverContextP ctx,
		VASurface *surface,
		Drawable draw, /* X Drawable */
		short srcx,
		short srcy,
		unsigned short srcw,
		unsigned short srch,
		short destx,
		short desty,
		unsigned short destw,
		unsigned short desth,
		int flags /* de-interlacing flags */
	)
{
    /* TODO */
	return VA_STATUS_ERROR_UNKNOWN;
}

VAStatus dummy_DbgCopySurfaceToBuffer(
		VADriverContextP ctx,
		VASurface *surface,
		void **buffer, /* out */
		unsigned int *stride /* out */
	)
{
    /* TODO */
	return VA_STATUS_ERROR_UNKNOWN;
}

VAStatus dummy_Terminate( VADriverContextP ctx )
{
    INIT_DRIVER_DATA
    object_buffer_p obj_buffer;
    object_surface_p obj_surface;
    object_context_p obj_context;
    object_config_p obj_config;
    object_heap_iterator iter;

    /* Clean up left over buffers */
    obj_buffer = (object_buffer_p) object_heap_first( &driver_data->buffer_heap, &iter);
    while (obj_buffer)
    {
        dummy__information_message("vaTerminate: bufferID %08x still allocated, destroying\n", obj_buffer->base.id);
        dummy__destroy_buffer(driver_data, obj_buffer);
        obj_buffer = (object_buffer_p) object_heap_next( &driver_data->buffer_heap, &iter);
    }
    object_heap_destroy( &driver_data->buffer_heap );

    /* TODO cleanup */
    object_heap_destroy( &driver_data->surface_heap );
    
    /* TODO cleanup */
    object_heap_destroy( &driver_data->context_heap );

    /* Clean up configIDs */
    obj_config = (object_config_p) object_heap_first( &driver_data->config_heap, &iter);
    while (obj_config)
    {
        object_heap_free( &driver_data->config_heap, (object_base_p) obj_config);
        obj_config = (object_config_p) object_heap_next( &driver_data->config_heap, &iter);
    }
    object_heap_destroy( &driver_data->config_heap );

    free(ctx->pDriverData);
    ctx->pDriverData = NULL;

	return VA_STATUS_SUCCESS;
}

VAStatus __vaDriverInit_0_20(  VADriverContextP ctx )
{
    object_base_p obj;
    int result;
    struct dummy_driver_data *driver_data;
    int i;
    
    ctx->version_major = 0;
    ctx->version_minor = 20;
    ctx->max_profiles = DUMMY_MAX_PROFILES;
    ctx->max_entrypoints = DUMMY_MAX_ENTRYPOINTS;
    ctx->max_attributes = DUMMY_MAX_CONFIG_ATTRIBUTES;

	ctx->vtable.vaTerminate = dummy_Terminate;
	ctx->vtable.vaQueryConfigEntrypoints = dummy_QueryConfigEntrypoints;
	ctx->vtable.vaTerminate = dummy_Terminate;
	ctx->vtable.vaQueryConfigProfiles = dummy_QueryConfigProfiles;
	ctx->vtable.vaQueryConfigEntrypoints = dummy_QueryConfigEntrypoints;
	ctx->vtable.vaQueryConfigAttributes = dummy_QueryConfigAttributes;
	ctx->vtable.vaCreateConfig = dummy_CreateConfig;
	ctx->vtable.vaGetConfigAttributes = dummy_GetConfigAttributes;
	ctx->vtable.vaCreateSurfaces = dummy_CreateSurfaces;
	ctx->vtable.vaDestroySurface = dummy_DestroySurface;
	ctx->vtable.vaCreateContext = dummy_CreateContext;
	ctx->vtable.vaDestroyContext = dummy_DestroyContext;
	ctx->vtable.vaCreateBuffer = dummy_CreateBuffer;
	ctx->vtable.vaBufferData = dummy_BufferData;
	ctx->vtable.vaBufferSetNumElements = dummy_BufferSetNumElements;
	ctx->vtable.vaMapBuffer = dummy_MapBuffer;
	ctx->vtable.vaUnmapBuffer = dummy_UnmapBuffer;
	ctx->vtable.vaDestroyBuffer = dummy_DestroyBuffer;
	ctx->vtable.vaBeginPicture = dummy_BeginPicture;
	ctx->vtable.vaRenderPicture = dummy_RenderPicture;
	ctx->vtable.vaEndPicture = dummy_EndPicture;
	ctx->vtable.vaSyncSurface = dummy_SyncSurface;
	ctx->vtable.vaQuerySurfaceStatus = dummy_QuerySurfaceStatus;
	ctx->vtable.vaPutSurface = dummy_PutSurface;
	ctx->vtable.vaDbgCopySurfaceToBuffer = dummy_DbgCopySurfaceToBuffer;

	driver_data = (struct dummy_driver_data *) malloc( sizeof(*driver_data) );
    ctx->pDriverData = (void *) driver_data;
    
    result = object_heap_init( &driver_data->config_heap, sizeof(struct object_config), CONFIG_ID_OFFSET );
    ASSERT( result == 0 );

    result = object_heap_init( &driver_data->context_heap, sizeof(struct object_context), CONTEXT_ID_OFFSET );
    ASSERT( result == 0 );

    result = object_heap_init( &driver_data->surface_heap, sizeof(struct object_surface), SURFACE_ID_OFFSET );
    ASSERT( result == 0 );

    result = object_heap_init( &driver_data->buffer_heap, sizeof(struct object_buffer), BUFFER_ID_OFFSET );
    ASSERT( result == 0 );


    return VA_STATUS_SUCCESS;
}

