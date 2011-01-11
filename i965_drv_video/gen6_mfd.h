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

#ifndef _GEN6_MFD_H_
#define _GEN6_MFD_H_

#include <xf86drm.h>
#include <drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>

struct gen6_mfd_surface
{
    dri_bo *dmv_top;
    dri_bo *dmv_bottom;
    int dmv_bottom_flag;
};

#define MAX_MFX_REFERENCE_SURFACES        16
struct gen6_mfd_context
{
    struct {
        VASurfaceID surface_id;
        int frame_store_id;
    } reference_surface[MAX_MFX_REFERENCE_SURFACES];

    struct {
        dri_bo *bo;
        int valid;
    } post_deblocking_output;

    struct {
        dri_bo *bo;
        int valid;
    } pre_deblocking_output;

    struct {
        dri_bo *bo;
        int valid;
    } intra_row_store_scratch_buffer;

    struct {
        dri_bo *bo;
        int valid;
    } deblocking_filter_row_store_scratch_buffer;

    struct {
        dri_bo *bo;
        int valid;
    } bsd_mpc_row_store_scratch_buffer;

    struct {
        dri_bo *bo;
        int valid;
    } mpr_row_store_scratch_buffer;

    struct {
        dri_bo *bo;
        int valid;
    } bitplane_read_buffer;
};

struct decode_state;

Bool gen6_mfd_init(VADriverContextP ctx);
Bool gen6_mfd_terminate(VADriverContextP ctx);
void gen6_mfd_decode_picture(VADriverContextP ctx, 
                             VAProfile profile, 
                             struct decode_state *decode_state);
#endif /* _GEN6_MFD_H_ */
