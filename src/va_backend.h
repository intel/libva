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

/*
 * Video Decode Acceleration -Backend API
 */

#ifndef _VA_BACKEND_H_
#define _VA_BACKEND_H_

#include "va.h"
#include "va_x11.h"

#include <stdlib.h>

typedef struct VADriverContext *VADriverContextP;

struct VADriverContext
{
    VADriverContextP pNext;
    Display *x11_dpy;
    int x11_screen;
    int version_major;
    int version_minor;
    int max_profiles;
    int max_entrypoints;
    int max_attributes;
    int max_image_formats;
    int max_subpic_formats;
    int max_display_attributes;
    void *handle;			/* dlopen handle */
    void *pDriverData;
    struct
    {
	 VAStatus (*vaTerminate) ( VADriverContextP ctx );

	 VAStatus (*vaQueryConfigProfiles) (
		VADriverContextP ctx,
		VAProfile *profile_list,	/* out */
		int *num_profiles			/* out */
	);

	VAStatus (*vaQueryConfigEntrypoints) (
		VADriverContextP ctx,
		VAProfile profile,
		VAEntrypoint  *entrypoint_list,	/* out */
		int *num_entrypoints			/* out */
	);

	VAStatus (*vaQueryConfigAttributes) (
		VADriverContextP ctx,
		VAProfile profile,
		VAEntrypoint entrypoint,
		VAConfigAttrib *attrib_list,	/* in/out */
		int num_attribs
	);

	VAStatus (*vaCreateConfig) (
		VADriverContextP ctx,
		VAProfile profile, 
		VAEntrypoint entrypoint, 
		VAConfigAttrib *attrib_list,
		int num_attribs,
		VAConfigID *config_id		/* out */
	);

	VAStatus (*vaDestroyConfig) (
		VADriverContextP ctx,
		VAConfigID config_id
	);

	VAStatus (*vaGetConfigAttributes) (
		VADriverContextP ctx,
		VAConfigID config_id, 
		VAProfile *profile,		/* out */
		VAEntrypoint *entrypoint, 	/* out */
		VAConfigAttrib *attrib_list,	/* out */
		int *num_attribs		/* out */
	);

	VAStatus (*vaCreateSurfaces) (
		VADriverContextP ctx,
		int width,
		int height,
		int format,
		int num_surfaces,
		VASurface *surfaces		/* out */
	);

	VAStatus (*vaDestroySurface) (
		VADriverContextP ctx,
		VASurface *surface_list,
		int num_surfaces
	);

	VAStatus (*vaCreateContext) (
		VADriverContextP ctx,
		VAConfigID config_id,
		int picture_width,
		int picture_height,
		int flag,
		VASurface *render_targets,
		int num_render_targets,
		VAContext *context		/* out */
	);

	VAStatus (*vaDestroyContext) (
		VADriverContextP ctx,
		VAContext *context
	);

	VAStatus (*vaCreateBuffer) (
		VADriverContextP ctx,
		VABufferType type,  /* in */
		VABufferID *buf_desc	/* out */
	);

	VAStatus (*vaBufferData) (
		VADriverContextP ctx,
		VABufferID buf_id,	/* in */
        unsigned int size,	/* in */
        unsigned int num_elements,	/* in */
        void *data		/* in */
	);

	VAStatus (*vaBufferSetNumElements) (
		VADriverContextP ctx,
		VABufferID buf_id,	/* in */
        unsigned int num_elements	/* in */
	);

	VAStatus (*vaMapBuffer) (
		VADriverContextP ctx,
		VABufferID buf_id,	/* in */
		void **pbuf         /* out */
	);

	VAStatus (*vaUnmapBuffer) (
		VADriverContextP ctx,
		VABufferID buf_id	/* in */
	);

	VAStatus (*vaDestroyBuffer) (
		VADriverContextP ctx,
		VABufferID buffer_id
	);

	VAStatus (*vaBeginPicture) (
		VADriverContextP ctx,
		VAContext *context,
		VASurface *render_target
	);

	VAStatus (*vaRenderPicture) (
		VADriverContextP ctx,
		VAContext *context,
		VABufferID *buffers,
		int num_buffers
	);

	VAStatus (*vaEndPicture) (
		VADriverContextP ctx,
		VAContext *context
	);

	VAStatus (*vaSyncSurface) (
		VADriverContextP ctx,
		VAContext *context,
		VASurface *render_target
	);

	VAStatus (*vaQuerySurfaceStatus) (
		VADriverContextP ctx,
		VAContext *context,
		VASurface *render_target,
		VASurfaceStatus *status	/* out */
	);

	VAStatus (*vaPutSurface) (
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
		VARectangle *cliprects, /* client supplied clip list */
		unsigned int number_cliprects, /* number of clip rects in the clip list */
		unsigned int flags /* de-interlacing flags */
	);

	VAStatus (*vaQueryImageFormats) (
		VADriverContextP ctx,
		VAImageFormat *format_list,        /* out */
		int *num_formats           /* out */
	);

	VAStatus (*vaCreateImage) (
		VADriverContextP ctx,
		VAImageFormat *format,
		int width,
		int height,
		VAImage *image     /* out */
	);

	VAStatus (*vaDestroyImage) (
		VADriverContextP ctx,
		VAImage *image
	);

	VAStatus (*vaGetImage) (
		VADriverContextP ctx,
		VASurface *surface,
		int x,     /* coordinates of the upper left source pixel */
		int y,
		unsigned int width, /* width and height of the region */
		unsigned int height,
		VAImage *image
	);

	VAStatus (*vaPutImage) (
		VADriverContextP ctx,
		VASurface *surface,
		VAImage *image,
		int src_x,
		int src_y,
		unsigned int width,
		unsigned int height,
		int dest_x,
		int dest_y 
	);

	VAStatus (*vaQuerySubpictureFormats) (
		VADriverContextP ctx,
		VAImageFormat *format_list,        /* out */
		unsigned int *flags,       /* out */
		unsigned int *num_formats  /* out */
	);

	VAStatus (*vaCreateSubpicture) (
		VADriverContextP ctx,
		VAImage *image,
		VASubpicture *subpicture   /* out */
	);

	VAStatus (*vaDestroySubpicture) (
		VADriverContextP ctx,
		VASubpicture *subpicture
	);

        VAStatus (*vaSetSubpictureImage) (
                VADriverContextP ctx,
                VASubpicture *subpicture,
                VAImage *image
        );
        
	VAStatus (*vaSetSubpicturePalette) (
		VADriverContextP ctx,
		VASubpicture *subpicture,
		/*
		 * pointer to an array holding the palette data.  The size of the array is
		 * num_palette_entries * entry_bytes in size.  The order of the components
		 * in the palette is described by the component_order in VASubpicture struct
		 */
		unsigned char *palette
	);

	VAStatus (*vaSetSubpictureChromakey) (
		VADriverContextP ctx,
		VASubpicture *subpicture,
		unsigned int chromakey_min,
		unsigned int chromakey_max
	);

	VAStatus (*vaSetSubpictureGlobalAlpha) (
		VADriverContextP ctx,
		VASubpicture *subpicture,
		float global_alpha 
	);

	VAStatus (*vaAssociateSubpicture) (
		VADriverContextP ctx,
		VASurface *target_surface,
		VASubpicture *subpicture,
		short src_x, /* upper left offset in subpicture */
		short src_y,
		short dest_x, /* upper left offset in surface */
		short dest_y,
		unsigned short width,
		unsigned short height,
		/*
		 * whether to enable chroma-keying or global-alpha
		 * see VA_SUBPICTURE_XXX values
		 */
		unsigned int flags
	);

	VAStatus (*vaQueryDisplayAttributes) (
		VADriverContextP ctx,
		VADisplayAttribute *attr_list,	/* out */
		int *num_attributes		/* out */
        );

	VAStatus (*vaGetDisplayAttributes) (
		VADriverContextP ctx,
		VADisplayAttribute *attr_list,	/* in/out */
		int num_attributes
        );
        
        VAStatus (*vaSetDisplayAttributes) (
		VADriverContextP ctx,
                VADisplayAttribute *attr_list,
                int num_attributes
        );


	VAStatus (*vaDbgCopySurfaceToBuffer) (
		VADriverContextP ctx,
		VASurface *surface,
		void **buffer, /* out */
		unsigned int *stride /* out */
	);

    } vtable;
};

typedef VAStatus (*VADriverInit) (
    VADriverContextP driver_context
);


#endif /* _VA_BACKEND_H_ */
