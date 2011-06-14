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
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <va/va_backend.h>

#include "intel_batchbuffer.h"
#include "intel_driver.h"
#include "i965_defines.h"
#include "i965_drv_video.h"

#include "i965_media.h"
#include "i965_media_mpeg2.h"

#define SURFACE_TARGET	    0
#define SURFACE_FORWARD	    1
#define SURFACE_BACKWARD    2
#define SURFACE_BIDIRECT    3

enum interface {
    FRAME_INTRA = 0,
    FRAME_FRAME_PRED_FORWARD,
    FRAME_FRAME_PRED_BACKWARD,
    FRAME_FRAME_PRED_BIDIRECT,
    FRAME_FIELD_PRED_FORWARD,
    FRAME_FIELD_PRED_BACKWARD,
    FRAME_FIELD_PRED_BIDIRECT,
    LIB_INTERFACE,
    FIELD_INTRA,
    FIELD_FORWARD,
    FIELD_FORWARD_16X8,
    FIELD_BACKWARD,
    FIELD_BACKWARD_16X8,
    FIELD_BIDIRECT,
    FIELD_BIDIRECT_16X8
};

/* idct table */
#define C0 23170
#define C1 22725
#define C2 21407
#define C3 19266
#define C4 16383
#define C5 12873
#define C6 8867
#define C7 4520
const uint32_t idct_table[] = {
    C4, C1, C2, C3, C4, C5, C6, C7,             //g5
    C4, C1, C2, C3, C4, C5, C6, C7,
    C4, C3, C6,-C7,-C4,-C1,-C2,-C5,
    C4, C3, C6,-C7,-C4,-C1,-C2,-C5,
    C4, C5,-C6,-C1,-C4, C7, C2, C3,
    C4, C5,-C6,-C1,-C4, C7, C2, C3,
    C4, C7,-C2,-C5, C4, C3,-C6,-C1,
    C4, C7,-C2,-C5, C4, C3,-C6,-C1,
    C4,-C7,-C2, C5, C4,-C3,-C6, C1,
    C4,-C7,-C2, C5, C4,-C3,-C6, C1,
    C4,-C5,-C6, C1,-C4,-C7, C2,-C3,
    C4,-C5,-C6, C1,-C4,-C7, C2,-C3,
    C4,-C3, C6, C7,-C4, C1,-C2, C5,
    C4,-C3, C6, C7,-C4, C1,-C2, C5,
    C4,-C1, C2,-C3, C4,-C5, C6,-C7,
    C4,-C1, C2,-C3, C4,-C5, C6,-C7              //g20
};
#undef C0
#undef C1
#undef C2
#undef C3
#undef C4
#undef C5
#undef C6
#undef C7

const uint32_t zigzag_direct[64] = {
    0,   1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

static const uint32_t frame_intra_kernel[][4] = {
   #include "shaders/mpeg2/vld/frame_intra.g4b"
};
static const uint32_t frame_frame_pred_forward_kernel[][4] = {
   #include "shaders/mpeg2/vld/frame_frame_pred_forward.g4b"
};
static const uint32_t frame_frame_pred_backward_kernel[][4] = {
   #include "shaders/mpeg2/vld/frame_frame_pred_backward.g4b"
};
static const uint32_t frame_frame_pred_bidirect_kernel[][4] = {
   #include "shaders/mpeg2/vld/frame_frame_pred_bidirect.g4b"
};
static const uint32_t frame_field_pred_forward_kernel[][4] = {
   #include "shaders/mpeg2/vld/frame_field_pred_forward.g4b"
};
static const uint32_t frame_field_pred_backward_kernel[][4] = {
   #include "shaders/mpeg2/vld/frame_field_pred_backward.g4b"
};
static const uint32_t frame_field_pred_bidirect_kernel[][4] = {
   #include "shaders/mpeg2/vld/frame_field_pred_bidirect.g4b"
};
static const uint32_t lib_kernel[][4] = {
   #include "shaders/mpeg2/vld/lib.g4b"
};
/*field picture*/
static const uint32_t field_intra_kernel[][4] = {
   #include "shaders/mpeg2/vld/field_intra.g4b"
};
static const uint32_t field_forward_kernel[][4] = {
   #include "shaders/mpeg2/vld/field_forward.g4b"
};
static const uint32_t field_forward_16x8_kernel[][4] = {
   #include "shaders/mpeg2/vld/field_forward_16x8.g4b"
};
static const uint32_t field_backward_kernel[][4] = {
   #include "shaders/mpeg2/vld/field_backward.g4b"
};
static const uint32_t field_backward_16x8_kernel[][4] = {
   #include "shaders/mpeg2/vld/field_backward_16x8.g4b"
};
static const uint32_t field_bidirect_kernel[][4] = {
   #include "shaders/mpeg2/vld/field_bidirect.g4b"
};
static const uint32_t field_bidirect_16x8_kernel[][4] = {
   #include "shaders/mpeg2/vld/field_bidirect_16x8.g4b"
};

static struct i965_kernel  mpeg2_vld_kernels_gen4[] = {
    {
        "FRAME_INTRA",
        FRAME_INTRA,
        frame_intra_kernel, 
        sizeof(frame_intra_kernel),
        NULL
    },

    {
        "FRAME_FRAME_PRED_FORWARD",
    	FRAME_FRAME_PRED_FORWARD,
        frame_frame_pred_forward_kernel, 
        sizeof(frame_frame_pred_forward_kernel),
        NULL
    },

    {
        "FRAME_FRAME_PRED_BACKWARD",
        FRAME_FRAME_PRED_BACKWARD,
        frame_frame_pred_backward_kernel, 
        sizeof(frame_frame_pred_backward_kernel),
        NULL
    },

    {   
        "FRAME_FRAME_PRED_BIDIRECT",
        FRAME_FRAME_PRED_BIDIRECT,
        frame_frame_pred_bidirect_kernel, 
        sizeof(frame_frame_pred_bidirect_kernel),
        NULL
    },

    {
        "FRAME_FIELD_PRED_FORWARD",
        FRAME_FIELD_PRED_FORWARD,
        frame_field_pred_forward_kernel, 
        sizeof(frame_field_pred_forward_kernel),
        NULL
    },

    {
        "FRAME_FIELD_PRED_BACKWARD",
        FRAME_FIELD_PRED_BACKWARD,
        frame_field_pred_backward_kernel, 
        sizeof(frame_field_pred_backward_kernel),
        NULL
    },

    {
        "FRAME_FIELD_PRED_BIDIRECT",
        FRAME_FIELD_PRED_BIDIRECT,
        frame_field_pred_bidirect_kernel, 
        sizeof(frame_field_pred_bidirect_kernel),
        NULL
    },
    
    {   
        "LIB",
        LIB_INTERFACE,
        lib_kernel, 
        sizeof(lib_kernel),
        NULL
    },

    {
        "FIELD_INTRA",
        FIELD_INTRA,
        field_intra_kernel, 
        sizeof(field_intra_kernel),
        NULL
    },

    {
        "FIELD_FORWARD",
        FIELD_FORWARD,
        field_forward_kernel, 
        sizeof(field_forward_kernel),
        NULL
    },

    {
        "FIELD_FORWARD_16X8",
        FIELD_FORWARD_16X8,
        field_forward_16x8_kernel, 
        sizeof(field_forward_16x8_kernel),
        NULL
    },

    {
        "FIELD_BACKWARD",
        FIELD_BACKWARD,
        field_backward_kernel, 
        sizeof(field_backward_kernel),
        NULL
    },

    {
        "FIELD_BACKWARD_16X8",
        FIELD_BACKWARD_16X8,
        field_backward_16x8_kernel, 
        sizeof(field_backward_16x8_kernel),
        NULL
    },

    {
        "FIELD_BIDIRECT",
        FIELD_BIDIRECT,
        field_bidirect_kernel, 
        sizeof(field_bidirect_kernel),
        NULL
    },

    {
        "FIELD_BIDIRECT_16X8",
        FIELD_BIDIRECT_16X8,
        field_bidirect_16x8_kernel, 
        sizeof(field_bidirect_16x8_kernel),
        NULL
    }
};

/* On IRONLAKE */
static const uint32_t frame_intra_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/frame_intra.g4b.gen5"
};
static const uint32_t frame_frame_pred_forward_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/frame_frame_pred_forward.g4b.gen5"
};
static const uint32_t frame_frame_pred_backward_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/frame_frame_pred_backward.g4b.gen5"
};
static const uint32_t frame_frame_pred_bidirect_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/frame_frame_pred_bidirect.g4b.gen5"
};
static const uint32_t frame_field_pred_forward_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/frame_field_pred_forward.g4b.gen5"
};
static const uint32_t frame_field_pred_backward_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/frame_field_pred_backward.g4b.gen5"
};
static const uint32_t frame_field_pred_bidirect_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/frame_field_pred_bidirect.g4b.gen5"
};
static const uint32_t lib_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/lib.g4b.gen5"
};
/*field picture*/
static const uint32_t field_intra_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/field_intra.g4b.gen5"
};
static const uint32_t field_forward_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/field_forward.g4b.gen5"
};
static const uint32_t field_forward_16x8_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/field_forward_16x8.g4b.gen5"
};
static const uint32_t field_backward_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/field_backward.g4b.gen5"
};
static const uint32_t field_backward_16x8_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/field_backward_16x8.g4b.gen5"
};
static const uint32_t field_bidirect_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/field_bidirect.g4b.gen5"
};
static const uint32_t field_bidirect_16x8_kernel_gen5[][4] = {
   #include "shaders/mpeg2/vld/field_bidirect_16x8.g4b.gen5"
};

static struct i965_kernel  mpeg2_vld_kernels_gen5[] = {
    {
        "FRAME_INTRA",
        FRAME_INTRA,
        frame_intra_kernel_gen5, 
        sizeof(frame_intra_kernel_gen5),
        NULL
    },

    {
        "FRAME_FRAME_PRED_FORWARD",
    	FRAME_FRAME_PRED_FORWARD,
        frame_frame_pred_forward_kernel_gen5, 
        sizeof(frame_frame_pred_forward_kernel_gen5),
        NULL
    },

    {
        "FRAME_FRAME_PRED_BACKWARD",
        FRAME_FRAME_PRED_BACKWARD,
        frame_frame_pred_backward_kernel_gen5, 
        sizeof(frame_frame_pred_backward_kernel_gen5),
        NULL
    },

    {   
        "FRAME_FRAME_PRED_BIDIRECT",
        FRAME_FRAME_PRED_BIDIRECT,
        frame_frame_pred_bidirect_kernel_gen5, 
        sizeof(frame_frame_pred_bidirect_kernel_gen5),
        NULL
    },

    {
        "FRAME_FIELD_PRED_FORWARD",
        FRAME_FIELD_PRED_FORWARD,
        frame_field_pred_forward_kernel_gen5, 
        sizeof(frame_field_pred_forward_kernel_gen5),
        NULL
    },

    {
        "FRAME_FIELD_PRED_BACKWARD",
        FRAME_FIELD_PRED_BACKWARD,
        frame_field_pred_backward_kernel_gen5, 
        sizeof(frame_field_pred_backward_kernel_gen5),
        NULL
    },

    {
        "FRAME_FIELD_PRED_BIDIRECT",
        FRAME_FIELD_PRED_BIDIRECT,
        frame_field_pred_bidirect_kernel_gen5, 
        sizeof(frame_field_pred_bidirect_kernel_gen5),
        NULL
    },
    
    {   
        "LIB",
        LIB_INTERFACE,
        lib_kernel_gen5, 
        sizeof(lib_kernel_gen5),
        NULL
    },

    {
        "FIELD_INTRA",
        FIELD_INTRA,
        field_intra_kernel_gen5, 
        sizeof(field_intra_kernel_gen5),
        NULL
    },

    {
        "FIELD_FORWARD",
        FIELD_FORWARD,
        field_forward_kernel_gen5, 
        sizeof(field_forward_kernel_gen5),
        NULL
    },

    {
        "FIELD_FORWARD_16X8",
        FIELD_FORWARD_16X8,
        field_forward_16x8_kernel_gen5, 
        sizeof(field_forward_16x8_kernel_gen5),
        NULL
    },

    {
        "FIELD_BACKWARD",
        FIELD_BACKWARD,
        field_backward_kernel_gen5, 
        sizeof(field_backward_kernel_gen5),
        NULL
    },

    {
        "FIELD_BACKWARD_16X8",
        FIELD_BACKWARD_16X8,
        field_backward_16x8_kernel_gen5, 
        sizeof(field_backward_16x8_kernel_gen5),
        NULL
    },

    {
        "FIELD_BIDIRECT",
        FIELD_BIDIRECT,
        field_bidirect_kernel_gen5, 
        sizeof(field_bidirect_kernel_gen5),
        NULL
    },

    {
        "FIELD_BIDIRECT_16X8",
        FIELD_BIDIRECT_16X8,
        field_bidirect_16x8_kernel_gen5, 
        sizeof(field_bidirect_16x8_kernel_gen5),
        NULL
    }
};

static void
i965_media_mpeg2_surface_state(VADriverContextP ctx, 
                               int index,
                               struct object_surface *obj_surface,
                               unsigned long offset, 
                               int w, int h,
                               Bool is_dst,
			       int vert_line_stride,
			       int vert_line_stride_ofs,
                               struct i965_media_context *media_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);  
    struct i965_surface_state *ss;
    dri_bo *bo;
    uint32_t write_domain, read_domain;

    bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "surface state", 
                      sizeof(struct i965_surface_state), 32);
    assert(bo);
    dri_bo_map(bo, 1);
    assert(bo->virtual);
    ss = bo->virtual;
    memset(ss, 0, sizeof(*ss));
    ss->ss0.surface_type = I965_SURFACE_2D;
    ss->ss0.surface_format = I965_SURFACEFORMAT_R8_SINT;
    ss->ss0.vert_line_stride = vert_line_stride;
    ss->ss0.vert_line_stride_ofs = vert_line_stride_ofs;
    ss->ss1.base_addr = obj_surface->bo->offset + offset;
    ss->ss2.width = w - 1;
    ss->ss2.height = h - 1;
    ss->ss3.pitch = w - 1;

    if (is_dst) {
        write_domain = I915_GEM_DOMAIN_RENDER;
        read_domain = I915_GEM_DOMAIN_RENDER;
    } else {
        write_domain = 0;
        read_domain = I915_GEM_DOMAIN_SAMPLER;
    }

    dri_bo_emit_reloc(bo,
                      read_domain, write_domain,
                      offset,
                      offsetof(struct i965_surface_state, ss1),
                      obj_surface->bo);
    dri_bo_unmap(bo);

    assert(index < MAX_MEDIA_SURFACES);
//    assert(media_context->surface_state[index].bo == NULL);
    media_context->surface_state[index].bo = bo;
}

static void
i965_media_mpeg2_surface_setup(VADriverContextP ctx, 
                               int base_index,
                               struct object_surface *obj_surface, 
                               Bool is_dst, 
			       int picture_structure,
			       int surface,
                               struct i965_media_context *media_context)
{
    int w = obj_surface->width;
    int h = obj_surface->height;

    i965_check_alloc_surface_bo(ctx, obj_surface, 0, VA_FOURCC('I','4','2','0'));

    if (picture_structure == MPEG_FRAME) {
	i965_media_mpeg2_surface_state(ctx, base_index + 0, obj_surface,
                                       0, w, h, 
                                       is_dst, 0, 0,
                                       media_context);
	i965_media_mpeg2_surface_state(ctx, base_index + 1, obj_surface,
                                       w * h, w / 2, h / 2, 
                                       is_dst, 0, 0,
                                       media_context);
	i965_media_mpeg2_surface_state(ctx, base_index + 2, obj_surface,
                                       w * h + w * h / 4, w / 2, h / 2, 
                                       is_dst, 0, 0,
                                       media_context);
    } else {
	if (surface == SURFACE_TARGET) {
	    i965_media_mpeg2_surface_state(ctx, 3, obj_surface,
                                           0, w, h, 
                                           False, 0, 0,
                                           media_context);
	    i965_media_mpeg2_surface_state(ctx, 10, obj_surface,
                                           w * h, w / 2, h / 2, 
                                           False, 0, 0,
                                           media_context);
	    i965_media_mpeg2_surface_state(ctx, 11, obj_surface,
                                           w * h + w * h / 4, w / 2, h / 2, 
                                           False, 0, 0,
                                           media_context);
	    if (picture_structure == MPEG_TOP_FIELD) {
		i965_media_mpeg2_surface_state(ctx, base_index + 0, obj_surface,
                                               0, w, h, 
                                               True, 1, 0,
                                               media_context);
		i965_media_mpeg2_surface_state(ctx, base_index + 1, obj_surface,
                                               w * h, w / 2, h / 2, 
                                               True, 1, 0,
                                               media_context);
		i965_media_mpeg2_surface_state(ctx, base_index + 2, obj_surface,
                                               w * h + w * h / 4, w / 2, h / 2, 
                                               True, 1, 0,
                                               media_context);
	    } else {
		assert(picture_structure == MPEG_BOTTOM_FIELD);
		i965_media_mpeg2_surface_state(ctx, base_index + 0, obj_surface,
                                               0, w, h, 
                                               True, 1, 1,
                                               media_context);
		i965_media_mpeg2_surface_state(ctx, base_index + 1, obj_surface,
                                               w * h, w / 2, h / 2, 
                                               True, 1, 1,
                                               media_context);
		i965_media_mpeg2_surface_state(ctx, base_index + 2, obj_surface,
                                               w * h + w * h / 4, w / 2, h / 2, 
                                               True, 1, 1,
                                               media_context);
	    }
	} else {
	    i965_media_mpeg2_surface_state(ctx, base_index + 0, obj_surface,
                                           0, w, h, 
                                           is_dst, 0, 0,
                                           media_context);
	    i965_media_mpeg2_surface_state(ctx, base_index + 1, obj_surface,
                                           w * h, w / 2, h / 2, 
                                           is_dst, 0, 0,
                                           media_context);
	    i965_media_mpeg2_surface_state(ctx, base_index + 2, obj_surface,
                                           w * h + w * h / 4, w / 2, h / 2, 
                                           is_dst, 0, 0,
                                           media_context);
	}
    }
}

void 
i965_media_mpeg2_surfaces_setup(VADriverContextP ctx, 
                                struct decode_state *decode_state,
                                struct i965_media_context *media_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);  
    struct object_surface *obj_surface;
    VAPictureParameterBufferMPEG2 *param;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    param = (VAPictureParameterBufferMPEG2 *)decode_state->pic_param->buffer;

    obj_surface = SURFACE(decode_state->current_render_target);
    assert(obj_surface);
    i965_media_mpeg2_surface_setup(ctx, 0, obj_surface, True,
                                   param->picture_coding_extension.bits.picture_structure,
                                   SURFACE_TARGET,
                                   media_context);

    obj_surface = SURFACE(param->forward_reference_picture);
    if (!obj_surface) {
//        assert(param->picture_coding_type == 1); /* I-picture */
    } else {
        i965_media_mpeg2_surface_setup(ctx, 4, obj_surface, False,
                                       param->picture_coding_extension.bits.picture_structure, 
                                       SURFACE_FORWARD,
                                       media_context);
        obj_surface = SURFACE(param->backward_reference_picture);
        if (!obj_surface) {
            assert(param->picture_coding_type == 2); /* P-picture */

            obj_surface = SURFACE(param->forward_reference_picture);
            i965_media_mpeg2_surface_setup(ctx, 7, obj_surface, False,
                                           param->picture_coding_extension.bits.picture_structure, 
                                           SURFACE_BACKWARD,
                                           media_context);
        } else {
            assert(param->picture_coding_type == 3); /* B-picture */
            i965_media_mpeg2_surface_setup(ctx, 7, obj_surface, False,
                                           param->picture_coding_extension.bits.picture_structure,
                                           SURFACE_BIDIRECT,
                                           media_context);
        }
    }
}

static void
i965_media_mpeg2_binding_table(VADriverContextP ctx, struct i965_media_context *media_context)
{
    int i;
    unsigned int *binding_table;
    dri_bo *bo = media_context->binding_table.bo;

    dri_bo_map(bo, 1);
    assert(bo->virtual);
    binding_table = bo->virtual;
    memset(binding_table, 0, bo->size);

    for (i = 0; i < MAX_MEDIA_SURFACES; i++) {
        if (media_context->surface_state[i].bo) {
            binding_table[i] = media_context->surface_state[i].bo->offset;
            dri_bo_emit_reloc(bo,
                              I915_GEM_DOMAIN_INSTRUCTION, 0,
                              0,
                              i * sizeof(*binding_table),
                              media_context->surface_state[i].bo);
        }
    }

    dri_bo_unmap(media_context->binding_table.bo);
}

static void
i965_media_mpeg2_vfe_state(VADriverContextP ctx, struct i965_media_context *media_context)
{
    struct i965_vfe_state *vfe_state;
    dri_bo *bo;

    bo = media_context->vfe_state.bo;
    dri_bo_map(bo, 1);
    assert(bo->virtual);
    vfe_state = bo->virtual;
    memset(vfe_state, 0, sizeof(*vfe_state));
    vfe_state->vfe0.extend_vfe_state_present = 1;
    vfe_state->vfe1.vfe_mode = VFE_VLD_MODE;
    vfe_state->vfe1.num_urb_entries = media_context->urb.num_vfe_entries;
    vfe_state->vfe1.children_present = 0;
    vfe_state->vfe1.urb_entry_alloc_size = media_context->urb.size_vfe_entry - 1;
    vfe_state->vfe1.max_threads = media_context->urb.num_vfe_entries - 1;
    vfe_state->vfe2.interface_descriptor_base = 
        media_context->idrt.bo->offset >> 4; /* reloc */
    dri_bo_emit_reloc(bo,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      0,
                      offsetof(struct i965_vfe_state, vfe2),
                      media_context->idrt.bo);
    dri_bo_unmap(bo);
}

static void 
i965_media_mpeg2_interface_descriptor_remap_table(VADriverContextP ctx, struct i965_media_context *media_context)
{
    struct i965_mpeg2_context *i965_mpeg2_context = (struct i965_mpeg2_context *)media_context->private_context;
    struct i965_interface_descriptor *desc;
    int i;
    dri_bo *bo;

    bo = media_context->idrt.bo;
    dri_bo_map(bo, 1);
    assert(bo->virtual);
    desc = bo->virtual;

    for (i = 0; i < NUM_MPEG2_VLD_KERNELS; i++) {
        memset(desc, 0, sizeof(*desc));
        desc->desc0.grf_reg_blocks = 15;
        desc->desc0.kernel_start_pointer = i965_mpeg2_context->vld_kernels[i].bo->offset >> 6; /* reloc */
        desc->desc1.const_urb_entry_read_offset = 0;
        desc->desc1.const_urb_entry_read_len = 30;
        desc->desc3.binding_table_entry_count = 0;
        desc->desc3.binding_table_pointer = 
            media_context->binding_table.bo->offset >> 5; /*reloc */

        dri_bo_emit_reloc(bo,
                          I915_GEM_DOMAIN_INSTRUCTION, 0,
                          desc->desc0.grf_reg_blocks,
                          i * sizeof(*desc) + offsetof(struct i965_interface_descriptor, desc0),
                          i965_mpeg2_context->vld_kernels[i].bo);

        dri_bo_emit_reloc(bo,
                          I915_GEM_DOMAIN_INSTRUCTION, 0,
                          desc->desc3.binding_table_entry_count,
                          i * sizeof(*desc) + offsetof(struct i965_interface_descriptor, desc3),
                          media_context->binding_table.bo);
        desc++;
    }

    dri_bo_unmap(bo);
}

void
i965_media_mpeg2_vld_state(VADriverContextP ctx,
                           struct decode_state *decode_state,
                           struct i965_media_context *media_context)
{
    struct i965_vld_state *vld_state;
    VAPictureParameterBufferMPEG2 *param;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    param = (VAPictureParameterBufferMPEG2 *)decode_state->pic_param->buffer;

    assert(media_context->extended_state.bo);
    dri_bo_map(media_context->extended_state.bo, 1);
    assert(media_context->extended_state.bo->virtual);
    vld_state = media_context->extended_state.bo->virtual;
    memset(vld_state, 0, sizeof(*vld_state));

    vld_state->vld0.f_code_0_0 = ((param->f_code >> 12) & 0xf);
    vld_state->vld0.f_code_0_1 = ((param->f_code >> 8) & 0xf);
    vld_state->vld0.f_code_1_0 = ((param->f_code >> 4) & 0xf);
    vld_state->vld0.f_code_1_1 = (param->f_code & 0xf);
    vld_state->vld0.intra_dc_precision = param->picture_coding_extension.bits.intra_dc_precision;
    vld_state->vld0.picture_structure = param->picture_coding_extension.bits.picture_structure;
    vld_state->vld0.top_field_first = param->picture_coding_extension.bits.top_field_first;
    vld_state->vld0.frame_predict_frame_dct = param->picture_coding_extension.bits.frame_pred_frame_dct;
    vld_state->vld0.concealment_motion_vector = param->picture_coding_extension.bits.concealment_motion_vectors;
    vld_state->vld0.quantizer_scale_type = param->picture_coding_extension.bits.q_scale_type;
    vld_state->vld0.intra_vlc_format = param->picture_coding_extension.bits.intra_vlc_format;
    vld_state->vld0.scan_order = param->picture_coding_extension.bits.alternate_scan;

    vld_state->vld1.picture_coding_type = param->picture_coding_type;

    if (vld_state->vld0.picture_structure == MPEG_FRAME) {
	/*frame picture*/ 
	vld_state->desc_remap_table0.index_0 = FRAME_INTRA;
	vld_state->desc_remap_table0.index_1 = FRAME_FRAME_PRED_FORWARD;
	vld_state->desc_remap_table0.index_2 = FRAME_FIELD_PRED_FORWARD;
	vld_state->desc_remap_table0.index_3 = FRAME_FIELD_PRED_BIDIRECT; /* dual prime */
	vld_state->desc_remap_table0.index_4 = FRAME_FRAME_PRED_BACKWARD;
	vld_state->desc_remap_table0.index_5 = FRAME_FIELD_PRED_BACKWARD;
	vld_state->desc_remap_table0.index_6 = FRAME_FRAME_PRED_BIDIRECT;
	vld_state->desc_remap_table0.index_7 = FRAME_FIELD_PRED_BIDIRECT;

	vld_state->desc_remap_table1.index_8 = FRAME_INTRA;
	vld_state->desc_remap_table1.index_9 = FRAME_FRAME_PRED_FORWARD;
	vld_state->desc_remap_table1.index_10 = FRAME_FIELD_PRED_FORWARD;
	vld_state->desc_remap_table1.index_11 = FRAME_FIELD_PRED_BIDIRECT;
	vld_state->desc_remap_table1.index_12 = FRAME_FRAME_PRED_BACKWARD;
	vld_state->desc_remap_table1.index_13 = FRAME_FIELD_PRED_BACKWARD;
	vld_state->desc_remap_table1.index_14 = FRAME_FRAME_PRED_BIDIRECT;
	vld_state->desc_remap_table1.index_15 = FRAME_FIELD_PRED_BIDIRECT;
    } else {
	/*field picture*/
	vld_state->desc_remap_table0.index_0 = FIELD_INTRA;
	vld_state->desc_remap_table0.index_1 = FIELD_FORWARD;
	vld_state->desc_remap_table0.index_2 = FIELD_FORWARD_16X8;
	vld_state->desc_remap_table0.index_3 = FIELD_BIDIRECT; /* dual prime */
	vld_state->desc_remap_table0.index_4 = FIELD_BACKWARD;
	vld_state->desc_remap_table0.index_5 = FIELD_BACKWARD_16X8;
	vld_state->desc_remap_table0.index_6 = FIELD_BIDIRECT;
	vld_state->desc_remap_table0.index_7 = FIELD_BIDIRECT_16X8;
    }

    dri_bo_unmap(media_context->extended_state.bo);
}

static void
i965_media_mpeg2_upload_constants(VADriverContextP ctx,
                                  struct decode_state *decode_state,
                                  struct i965_media_context *media_context)
{
    struct i965_mpeg2_context *i965_mpeg2_context = (struct i965_mpeg2_context *)media_context->private_context;
    int i, j;
    unsigned char *constant_buffer;
    unsigned char *qmx;
    unsigned int *lib_reloc;
    int lib_reloc_offset = 0;

    dri_bo_map(media_context->curbe.bo, 1);
    assert(media_context->curbe.bo->virtual);
    constant_buffer = media_context->curbe.bo->virtual;

    /* iq_matrix */
    if (decode_state->iq_matrix && decode_state->iq_matrix->buffer) {
        VAIQMatrixBufferMPEG2 *iq_matrix = (VAIQMatrixBufferMPEG2 *)decode_state->iq_matrix->buffer;

        /* Upload quantisation matrix in row-major order. The mplayer vaapi
         * patch passes quantisation matrix in zig-zag order to va library.
         * Do we need a flag in VAIQMatrixBufferMPEG2 to specify the order
         * of the quantisation matrix?
         */
        qmx = constant_buffer;
        if (iq_matrix->load_intra_quantiser_matrix) {
            for (i = 0; i < 64; i++) {
                j = zigzag_direct[i];
                qmx[j] = iq_matrix->intra_quantiser_matrix[i];
            }
        }

        qmx = constant_buffer + 64;
        if (iq_matrix->load_non_intra_quantiser_matrix) {
            for (i = 0; i < 64; i++) {
                j = zigzag_direct[i];
                qmx[j] = iq_matrix->non_intra_quantiser_matrix[i];
            }
        }

        /* no chroma quantisation matrices for 4:2:0 data */
    }

    /* idct table */
    memcpy(constant_buffer + 128, idct_table, sizeof(idct_table));

    /* idct lib reloc */
    lib_reloc_offset = 128 + sizeof(idct_table);
    lib_reloc = (unsigned int *)(constant_buffer + lib_reloc_offset);
    for (i = 0; i < 8; i++) {
        lib_reloc[i] = i965_mpeg2_context->vld_kernels[LIB_INTERFACE].bo->offset;
        dri_bo_emit_reloc(media_context->curbe.bo,
                          I915_GEM_DOMAIN_INSTRUCTION, 0,
                          0,
                          lib_reloc_offset + i * sizeof(unsigned int),
                          i965_mpeg2_context->vld_kernels[LIB_INTERFACE].bo);
    }

    dri_bo_unmap(media_context->curbe.bo);
}

static void
i965_media_mpeg2_states_setup(VADriverContextP ctx, 
                              struct decode_state *decode_state, 
                              struct i965_media_context *media_context)
{
    i965_media_mpeg2_surfaces_setup(ctx, decode_state, media_context);
    i965_media_mpeg2_binding_table(ctx, media_context);
    i965_media_mpeg2_interface_descriptor_remap_table(ctx, media_context);
    i965_media_mpeg2_vld_state(ctx, decode_state, media_context);
    i965_media_mpeg2_vfe_state(ctx, media_context);
    i965_media_mpeg2_upload_constants(ctx, decode_state, media_context);
}

static void
i965_media_mpeg2_objects(VADriverContextP ctx, 
                         struct decode_state *decode_state,
                         struct i965_media_context *media_context)
{
    struct intel_batchbuffer *batch = media_context->base.batch;
    VASliceParameterBufferMPEG2 *slice_param;
    VAPictureParameterBufferMPEG2 *pic_param;
    int i, j;

    assert(decode_state->pic_param && decode_state->pic_param->buffer);
    pic_param = (VAPictureParameterBufferMPEG2 *)decode_state->pic_param->buffer;

    for (j = 0; j < decode_state->num_slice_params; j++) {
        assert(decode_state->slice_params[j] && decode_state->slice_params[j]->buffer);
        assert(decode_state->slice_datas[j] && decode_state->slice_datas[j]->bo);
        slice_param = (VASliceParameterBufferMPEG2 *)decode_state->slice_params[j]->buffer;

        for (i = 0; i < decode_state->slice_params[j]->num_elements; i++) {
            int vpos, hpos, is_field_pic = 0;

            if (pic_param->picture_coding_extension.bits.picture_structure == MPEG_TOP_FIELD ||
                pic_param->picture_coding_extension.bits.picture_structure == MPEG_BOTTOM_FIELD)
                is_field_pic = 1;

            assert(slice_param->slice_data_flag == VA_SLICE_DATA_FLAG_ALL);
            vpos = slice_param->slice_vertical_position / (1 + is_field_pic);
            hpos = slice_param->slice_horizontal_position;

            BEGIN_BATCH(batch, 6);
            OUT_BATCH(batch, CMD_MEDIA_OBJECT | 4);
            OUT_BATCH(batch, 0);
            OUT_BATCH(batch, slice_param->slice_data_size - (slice_param->macroblock_offset >> 3));
            OUT_RELOC(batch, decode_state->slice_datas[j]->bo, 
                      I915_GEM_DOMAIN_SAMPLER, 0, 
                      slice_param->slice_data_offset + (slice_param->macroblock_offset >> 3));
            OUT_BATCH(batch, 
                      ((hpos << 24) |     
                       (vpos << 16) |
                       (127 << 8) | 
                       (slice_param->macroblock_offset & 0x7)));
            OUT_BATCH(batch, slice_param->quantiser_scale_code << 24);
            ADVANCE_BATCH(batch);          
            slice_param++;
        }
    }
}

static void
i965_media_mpeg2_free_private_context(void **data)
{
    struct i965_mpeg2_context *i965_mpeg2_context = *data;
    int i;

    if (i965_mpeg2_context == NULL)
        return;

    for (i = 0; i < NUM_MPEG2_VLD_KERNELS; i++) {
        struct i965_kernel *kernel = &i965_mpeg2_context->vld_kernels[i];

        dri_bo_unreference(kernel->bo);
        kernel->bo = NULL;
    }

    free(i965_mpeg2_context);
    *data = NULL;
}

void 
i965_media_mpeg2_decode_init(VADriverContextP ctx, 
                             struct decode_state *decode_state, 
                             struct i965_media_context *media_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    dri_bo *bo;

    dri_bo_unreference(media_context->indirect_object.bo);
    media_context->indirect_object.bo = NULL;

    media_context->extended_state.enabled = 1;
    dri_bo_unreference(media_context->extended_state.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "vld state", 
                      sizeof(struct i965_vld_state), 32);
    assert(bo);
    media_context->extended_state.bo = bo;
}

void 
i965_media_mpeg2_dec_context_init(VADriverContextP ctx, struct i965_media_context *media_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_mpeg2_context *i965_mpeg2_context;
    int i;

    i965_mpeg2_context = calloc(1, sizeof(struct i965_mpeg2_context));

    /* kernel */
    assert(NUM_MPEG2_VLD_KERNELS == (sizeof(mpeg2_vld_kernels_gen4) / 
                                     sizeof(mpeg2_vld_kernels_gen4[0])));
    assert(NUM_MPEG2_VLD_KERNELS == (sizeof(mpeg2_vld_kernels_gen5) / 
                                     sizeof(mpeg2_vld_kernels_gen5[0])));
    assert(NUM_MPEG2_VLD_KERNELS <= MAX_INTERFACE_DESC);

    if (IS_IRONLAKE(i965->intel.device_id))
        memcpy(i965_mpeg2_context->vld_kernels, mpeg2_vld_kernels_gen5, sizeof(i965_mpeg2_context->vld_kernels));
    else
        memcpy(i965_mpeg2_context->vld_kernels, mpeg2_vld_kernels_gen4, sizeof(i965_mpeg2_context->vld_kernels));

    for (i = 0; i < NUM_MPEG2_VLD_KERNELS; i++) {
        struct i965_kernel *kernel = &i965_mpeg2_context->vld_kernels[i];
        kernel->bo = dri_bo_alloc(i965->intel.bufmgr, 
                                  kernel->name, 
                                  kernel->size, 64);
        assert(kernel->bo);
        dri_bo_subdata(kernel->bo, 0, kernel->size, kernel->bin);
    }

    /* URB */
    media_context->urb.num_vfe_entries = 28;
    media_context->urb.size_vfe_entry = 13;

    media_context->urb.num_cs_entries = 1;
    media_context->urb.size_cs_entry = 16;

    media_context->urb.vfe_start = 0;
    media_context->urb.cs_start = media_context->urb.vfe_start + 
        media_context->urb.num_vfe_entries * media_context->urb.size_vfe_entry;
    assert(media_context->urb.cs_start + 
           media_context->urb.num_cs_entries * media_context->urb.size_cs_entry <= URB_SIZE((&i965->intel)));

    /* hook functions */
    media_context->media_states_setup = i965_media_mpeg2_states_setup;
    media_context->media_objects = i965_media_mpeg2_objects;
    media_context->private_context = i965_mpeg2_context;
    media_context->free_private_context = i965_media_mpeg2_free_private_context;
}
