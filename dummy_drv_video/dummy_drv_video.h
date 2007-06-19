/*
 * @COPYRIGHT@ Intel Confidential - Unreleased Software
 */

#ifndef _DUMMY_DRV_VIDEO_H_
#define _DUMMY_DRV_VIDEO_H_

#include "va.h"
#include "object_heap.h"

#define DUMMY_MAX_PROFILES				11
#define DUMMY_MAX_ENTRYPOINTS			5
#define DUMMY_MAX_CONFIG_ATTRIBUTES		10

struct dummy_driver_data {
    struct object_heap	config_heap;
    struct object_heap	context_heap;
    struct object_heap	surface_heap;
    struct object_heap	buffer_heap;
};

struct object_config {
    struct object_base base;
    VAProfile profile;
    VAEntrypoint entrypoint;
    VAConfigAttrib attrib_list[DUMMY_MAX_CONFIG_ATTRIBUTES];
    int attrib_count;
};

struct object_context {
    struct object_base base;
    VAContext *context;
    VAConfigID config;
    VASurfaceID current_render_target;
};

struct object_surface {
    struct object_base base;
    VASurface *surface;
};

struct object_buffer {
    struct object_base base;
    void *buffer_data;
    int max_num_elements;
    int num_elements;
};

typedef struct object_config *object_config_p;
typedef struct object_context *object_context_p;
typedef struct object_surface *object_surface_p;
typedef struct object_buffer *object_buffer_p;

#endif /* _DUMMY_DRV_VIDEO_H_ */
