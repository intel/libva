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

#ifndef _I965_MEDIA_H_
#define _I965_MEDIA_H_

#include <xf86drm.h>
#include <drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>

#include "i965_structs.h"

#define MAX_INTERFACE_DESC      16
#define MAX_MEDIA_SURFACES      34

struct decode_state;

struct i965_media_context
{
    struct hw_context base;

    struct {
        dri_bo *bo;
    } surface_state[MAX_MEDIA_SURFACES];

    struct {
        dri_bo *bo;
    } binding_table;

    struct {
        dri_bo *bo;
    } idrt;  /* interface descriptor remap table */

    struct {
        dri_bo *bo;
        int enabled;
    } extended_state;

    struct {
        dri_bo *bo;
    } vfe_state;

    struct {
        dri_bo *bo;
    } curbe;

    struct {
        dri_bo *bo;
        unsigned long offset;
    } indirect_object;

    struct {
        unsigned int vfe_start;
        unsigned int cs_start;

        unsigned int num_vfe_entries;
        unsigned int num_cs_entries;

        unsigned int size_vfe_entry;
        unsigned int size_cs_entry;
    } urb;

    void *private_context;
    void (*media_states_setup)(VADriverContextP ctx, struct decode_state *decode_state, struct i965_media_context *media_context);
    void (*media_objects)(VADriverContextP ctx, struct decode_state *decode_state, struct i965_media_context *media_context);
    void (*free_private_context)(void **data);
};

#endif /* _I965_MEDIA_H_ */
