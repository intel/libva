/*
 * Video Decode Acceleration -Backend API
 *
 * @COPYRIGHT@ Intel Confidential - Unreleased Software
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
		int flags /* de-interlacing flags */
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
