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
 * SOFTWAR
 *
 * Authors:
 *    Zhou Chang <chang.zhou@intel.com>
 *
 */

#ifndef _GEN6_VME_H_
#define _GEN6_VME_H_

#include <xf86drm.h>
#include <drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>


#define MAX_INTERFACE_DESC_GEN6      32
#define MAX_MEDIA_SURFACES_GEN6      34

#define GEN6_VME_KERNEL_NUMBER          2

struct encode_state;
struct gen6_encoder_context;

struct gen6_vme_context
{
    struct {
        dri_bo *bo;
    } surface_state_binding_table;

    struct {
        dri_bo *bo;
    } idrt;  /* interface descriptor remap table */

    struct {
        dri_bo *bo;
    } curbe;

    struct {
        unsigned int gpgpu_mode:1;
        unsigned int max_num_threads:16;
        unsigned int num_urb_entries:8;
        unsigned int urb_entry_size:16;
        unsigned int curbe_allocation_size:16;
    } vfe_state;

    struct {
        dri_bo *bo;
    } vme_state;

    struct {
        dri_bo *bo;
        unsigned int num_blocks;
        unsigned int size_block; /* in bytes */
        unsigned int pitch;
    } vme_output;

    struct i965_kernel vme_kernels[GEN6_VME_KERNEL_NUMBER];
};

VAStatus gen6_vme_pipeline(VADriverContextP ctx,
                           VAProfile profile,
                           struct encode_state *encode_state,
                           struct gen6_encoder_context *gen6_encoder_context);
Bool gen6_vme_context_init(VADriverContextP ctx, struct gen6_vme_context *vme_context);
Bool gen6_vme_context_destroy(struct gen6_vme_context *vme_context);

#endif /* _GEN6_VME_H_ */
