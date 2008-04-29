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

#ifndef _DUMMY_DRV_VIDEO_H_
#define _DUMMY_DRV_VIDEO_H_

#include "va.h"
#include "object_heap.h"

#define DUMMY_MAX_PROFILES			11
#define DUMMY_MAX_ENTRYPOINTS			5
#define DUMMY_MAX_CONFIG_ATTRIBUTES		10
#define DUMMY_MAX_IMAGE_FORMATS			10
#define DUMMY_MAX_SUBPIC_FORMATS		4
#define DUMMY_MAX_DISPLAY_ATTRIBUTES		4
#define DUMMY_STR_VENDOR			"Dummy Driver 1.0"

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
    VAContextID context_id;
    VAConfigID config_id;
    VASurfaceID current_render_target;
    int picture_width;
    int picture_height;
    int num_render_targets;
    int flags;
    VASurfaceID *render_targets;
};

struct object_surface {
    struct object_base base;
    VASurfaceID surface_id;
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
