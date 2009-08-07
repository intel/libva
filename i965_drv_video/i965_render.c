/*
 * Copyright © 2006 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *    Keith Packard <keithp@keithp.com>
 *    Xiang Haihao <haihao.xiang@intel.com>
 *
 */

/*
 * Most of rendering codes are ported from xf86-video-intel/src/i965_video.c
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "va_backend.h"
#include "va_dricommon.h"

#include "intel_batchbuffer.h"
#include "intel_driver.h"

#include "i965_defines.h"
#include "i965_render.h"
#include "i965_drv_video.h"

#define SF_KERNEL_NUM_GRF       16
#define SF_MAX_THREADS          1

static const unsigned int sf_kernel_static[][4] = 
{
#include "shaders/render/exa_sf.g4b"
};

#define PS_KERNEL_NUM_GRF       32
#define PS_MAX_THREADS          32

#define I965_GRF_BLOCKS(nreg)	((nreg + 15) / 16 - 1)

static const unsigned int ps_kernel_static[][4] = 
{
#include "shaders/render/exa_wm_xy.g4b"
#include "shaders/render/exa_wm_src_affine.g4b"
#include "shaders/render/exa_wm_src_sample_planar.g4b"
#include "shaders/render/exa_wm_yuv_rgb.g4b"
#include "shaders/render/exa_wm_write.g4b"
};
static const unsigned int ps_subpic_kernel_static[][4] = 
{
#include "shaders/render/exa_wm_xy.g4b"
#include "shaders/render/exa_wm_src_affine.g4b"
#include "shaders/render/exa_wm_blend_subpicture.g4b"
#include "shaders/render/exa_wm_write.g4b"
};

/* On IGDNG */
static const unsigned int sf_kernel_static_gen5[][4] = 
{
#include "shaders/render/exa_sf.g4b.gen5"
};

static const unsigned int ps_kernel_static_gen5[][4] = 
{
#include "shaders/render/exa_wm_xy.g4b.gen5"
#include "shaders/render/exa_wm_src_affine.g4b.gen5"
#include "shaders/render/exa_wm_src_sample_planar.g4b.gen5"
#include "shaders/render/exa_wm_yuv_rgb.g4b.gen5"
#include "shaders/render/exa_wm_write.g4b.gen5"
};
static const unsigned int ps_subpic_kernel_static_gen5[][4] = 
{
#include "shaders/render/exa_wm_xy.g4b.gen5"
#include "shaders/render/exa_wm_src_affine.g4b.gen5"
#include "shaders/render/exa_wm_blend_subpicture.g4b.gen5"
#include "shaders/render/exa_wm_write.g4b.gen5"
};

static uint32_t float_to_uint (float f) 
{
    union {
        uint32_t i; 
        float f;
    } x;

    x.f = f;
    return x.i;
}

enum 
{
    SF_KERNEL = 0,
    PS_KERNEL,
    PS_SUBPIC_KERNEL
};

struct render_kernel
{
    char *name;
    const unsigned int (*bin)[4];
    int size;
    dri_bo *bo;
};

static struct render_kernel render_kernels_gen4[] = {
    {
        "SF",
        sf_kernel_static,
        sizeof(sf_kernel_static),
        NULL
    },
    {
        "PS",
        ps_kernel_static,
        sizeof(ps_kernel_static),
        NULL
    },

    {
        "PS_SUBPIC",
        ps_subpic_kernel_static,
        sizeof(ps_subpic_kernel_static),
        NULL
    }
};

static struct render_kernel render_kernels_gen5[] = {
    {
        "SF",
        sf_kernel_static_gen5,
        sizeof(sf_kernel_static_gen5),
        NULL
    },
    {
        "PS",
        ps_kernel_static_gen5,
        sizeof(ps_kernel_static_gen5),
        NULL
    },

    {
        "PS_SUBPIC",
        ps_subpic_kernel_static_gen5,
        sizeof(ps_subpic_kernel_static_gen5),
        NULL
    }
};

static struct render_kernel *render_kernels = NULL;

#define NUM_RENDER_KERNEL (sizeof(render_kernels_gen4)/sizeof(render_kernels_gen4[0]))

#define URB_VS_ENTRIES	      8
#define URB_VS_ENTRY_SIZE     1

#define URB_GS_ENTRIES	      0
#define URB_GS_ENTRY_SIZE     0

#define URB_CLIP_ENTRIES      0
#define URB_CLIP_ENTRY_SIZE   0

#define URB_SF_ENTRIES	      1
#define URB_SF_ENTRY_SIZE     2

#define URB_CS_ENTRIES	      0
#define URB_CS_ENTRY_SIZE     0

static void
i965_render_vs_unit(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;
    struct i965_vs_unit_state *vs_state;

    dri_bo_map(render_state->vs.state, 1);
    assert(render_state->vs.state->virtual);
    vs_state = render_state->vs.state->virtual;
    memset(vs_state, 0, sizeof(*vs_state));

    if (IS_IGDNG(i965->intel.device_id))
        vs_state->thread4.nr_urb_entries = URB_VS_ENTRIES >> 2;
    else
        vs_state->thread4.nr_urb_entries = URB_VS_ENTRIES;

    vs_state->thread4.urb_entry_allocation_size = URB_VS_ENTRY_SIZE - 1;
    vs_state->vs6.vs_enable = 0;
    vs_state->vs6.vert_cache_disable = 1;
    
    dri_bo_unmap(render_state->vs.state);
}

static void
i965_render_sf_unit(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;
    struct i965_sf_unit_state *sf_state;

    dri_bo_map(render_state->sf.state, 1);
    assert(render_state->sf.state->virtual);
    sf_state = render_state->sf.state->virtual;
    memset(sf_state, 0, sizeof(*sf_state));

    sf_state->thread0.grf_reg_count = I965_GRF_BLOCKS(SF_KERNEL_NUM_GRF);
    sf_state->thread0.kernel_start_pointer = render_kernels[SF_KERNEL].bo->offset >> 6;

    sf_state->sf1.single_program_flow = 1; /* XXX */
    sf_state->sf1.binding_table_entry_count = 0;
    sf_state->sf1.thread_priority = 0;
    sf_state->sf1.floating_point_mode = 0; /* Mesa does this */
    sf_state->sf1.illegal_op_exception_enable = 1;
    sf_state->sf1.mask_stack_exception_enable = 1;
    sf_state->sf1.sw_exception_enable = 1;

    /* scratch space is not used in our kernel */
    sf_state->thread2.per_thread_scratch_space = 0;
    sf_state->thread2.scratch_space_base_pointer = 0;

    sf_state->thread3.const_urb_entry_read_length = 0; /* no const URBs */
    sf_state->thread3.const_urb_entry_read_offset = 0; /* no const URBs */
    sf_state->thread3.urb_entry_read_length = 1; /* 1 URB per vertex */
    sf_state->thread3.urb_entry_read_offset = 0;
    sf_state->thread3.dispatch_grf_start_reg = 3;

    sf_state->thread4.max_threads = SF_MAX_THREADS - 1;
    sf_state->thread4.urb_entry_allocation_size = URB_SF_ENTRY_SIZE - 1;
    sf_state->thread4.nr_urb_entries = URB_SF_ENTRIES;
    sf_state->thread4.stats_enable = 1;

    sf_state->sf5.viewport_transform = 0; /* skip viewport */

    sf_state->sf6.cull_mode = I965_CULLMODE_NONE;
    sf_state->sf6.scissor = 0;

    sf_state->sf7.trifan_pv = 2;

    sf_state->sf6.dest_org_vbias = 0x8;
    sf_state->sf6.dest_org_hbias = 0x8;

    dri_bo_emit_reloc(render_state->sf.state,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      sf_state->thread0.grf_reg_count << 1,
                      offsetof(struct i965_sf_unit_state, thread0),
                      render_kernels[SF_KERNEL].bo);

    dri_bo_unmap(render_state->sf.state);
}

static void 
i965_render_sampler(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;
    struct i965_sampler_state *sampler_state;
    int i;
    
    assert(render_state->wm.sampler_count > 0);
    assert(render_state->wm.sampler_count <= MAX_SAMPLERS);

    dri_bo_map(render_state->wm.sampler, 1);
    assert(render_state->wm.sampler->virtual);
    sampler_state = render_state->wm.sampler->virtual;
    for (i = 0; i < render_state->wm.sampler_count; i++) {
        memset(sampler_state, 0, sizeof(*sampler_state));
        sampler_state->ss0.min_filter = I965_MAPFILTER_LINEAR;
        sampler_state->ss0.mag_filter = I965_MAPFILTER_LINEAR;
        sampler_state->ss1.r_wrap_mode = I965_TEXCOORDMODE_CLAMP;
        sampler_state->ss1.s_wrap_mode = I965_TEXCOORDMODE_CLAMP;
        sampler_state->ss1.t_wrap_mode = I965_TEXCOORDMODE_CLAMP;
        sampler_state++;
    }

    dri_bo_unmap(render_state->wm.sampler);
}
static void
i965_subpic_render_wm_unit(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;
    struct i965_wm_unit_state *wm_state;

    assert(render_state->wm.sampler);

    dri_bo_map(render_state->wm.state, 1);
    assert(render_state->wm.state->virtual);
    wm_state = render_state->wm.state->virtual;
    memset(wm_state, 0, sizeof(*wm_state));

    wm_state->thread0.grf_reg_count = I965_GRF_BLOCKS(PS_KERNEL_NUM_GRF);
    wm_state->thread0.kernel_start_pointer = render_kernels[PS_SUBPIC_KERNEL].bo->offset >> 6;

    wm_state->thread1.single_program_flow = 1; /* XXX */

    if (IS_IGDNG(i965->intel.device_id))
        wm_state->thread1.binding_table_entry_count = 0; /* hardware requirement */
    else
        wm_state->thread1.binding_table_entry_count = 7;

    wm_state->thread2.scratch_space_base_pointer = 0;
    wm_state->thread2.per_thread_scratch_space = 0; /* 1024 bytes */

    wm_state->thread3.dispatch_grf_start_reg = 3; /* XXX */
    wm_state->thread3.const_urb_entry_read_length = 0;
    wm_state->thread3.const_urb_entry_read_offset = 0;
    wm_state->thread3.urb_entry_read_length = 1; /* XXX */
    wm_state->thread3.urb_entry_read_offset = 0; /* XXX */

    wm_state->wm4.stats_enable = 0;
    wm_state->wm4.sampler_state_pointer = render_state->wm.sampler->offset >> 5; 

    if (IS_IGDNG(i965->intel.device_id))
        wm_state->wm4.sampler_count = 0;        /* hardware requirement */
    else
        wm_state->wm4.sampler_count = (render_state->wm.sampler_count + 3) / 4;

    wm_state->wm5.max_threads = PS_MAX_THREADS - 1;
    wm_state->wm5.thread_dispatch_enable = 1;
    wm_state->wm5.enable_16_pix = 1;
    wm_state->wm5.enable_8_pix = 0;
    wm_state->wm5.early_depth_test = 1;

    dri_bo_emit_reloc(render_state->wm.state,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      wm_state->thread0.grf_reg_count << 1,
                      offsetof(struct i965_wm_unit_state, thread0),
                      render_kernels[PS_SUBPIC_KERNEL].bo);

    dri_bo_emit_reloc(render_state->wm.state,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      wm_state->wm4.sampler_count << 2,
                      offsetof(struct i965_wm_unit_state, wm4),
                      render_state->wm.sampler);

    dri_bo_unmap(render_state->wm.state);
}


static void
i965_render_wm_unit(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;
    struct i965_wm_unit_state *wm_state;

    assert(render_state->wm.sampler);

    dri_bo_map(render_state->wm.state, 1);
    assert(render_state->wm.state->virtual);
    wm_state = render_state->wm.state->virtual;
    memset(wm_state, 0, sizeof(*wm_state));

    wm_state->thread0.grf_reg_count = I965_GRF_BLOCKS(PS_KERNEL_NUM_GRF);
    wm_state->thread0.kernel_start_pointer = render_kernels[PS_KERNEL].bo->offset >> 6;

    wm_state->thread1.single_program_flow = 1; /* XXX */

    if (IS_IGDNG(i965->intel.device_id))
        wm_state->thread1.binding_table_entry_count = 0;        /* hardware requirement */
    else
        wm_state->thread1.binding_table_entry_count = 7;

    wm_state->thread2.scratch_space_base_pointer = 0;
    wm_state->thread2.per_thread_scratch_space = 0; /* 1024 bytes */

    wm_state->thread3.dispatch_grf_start_reg = 3; /* XXX */
    wm_state->thread3.const_urb_entry_read_length = 0;
    wm_state->thread3.const_urb_entry_read_offset = 0;
    wm_state->thread3.urb_entry_read_length = 1; /* XXX */
    wm_state->thread3.urb_entry_read_offset = 0; /* XXX */

    wm_state->wm4.stats_enable = 0;
    wm_state->wm4.sampler_state_pointer = render_state->wm.sampler->offset >> 5; 

    if (IS_IGDNG(i965->intel.device_id))
        wm_state->wm4.sampler_count = 0;        /* hardware requirement */
    else 
        wm_state->wm4.sampler_count = (render_state->wm.sampler_count + 3) / 4;

    wm_state->wm5.max_threads = PS_MAX_THREADS - 1;
    wm_state->wm5.thread_dispatch_enable = 1;
    wm_state->wm5.enable_16_pix = 1;
    wm_state->wm5.enable_8_pix = 0;
    wm_state->wm5.early_depth_test = 1;

    dri_bo_emit_reloc(render_state->wm.state,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      wm_state->thread0.grf_reg_count << 1,
                      offsetof(struct i965_wm_unit_state, thread0),
                      render_kernels[PS_KERNEL].bo);

    dri_bo_emit_reloc(render_state->wm.state,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      wm_state->wm4.sampler_count << 2,
                      offsetof(struct i965_wm_unit_state, wm4),
                      render_state->wm.sampler);

    dri_bo_unmap(render_state->wm.state);
}

static void 
i965_render_cc_viewport(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;
    struct i965_cc_viewport *cc_viewport;

    dri_bo_map(render_state->cc.viewport, 1);
    assert(render_state->cc.viewport->virtual);
    cc_viewport = render_state->cc.viewport->virtual;
    memset(cc_viewport, 0, sizeof(*cc_viewport));
    
    cc_viewport->min_depth = -1.e35;
    cc_viewport->max_depth = 1.e35;

    dri_bo_unmap(render_state->cc.viewport);
}

static void 
i965_subpic_render_cc_unit(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;
    struct i965_cc_unit_state *cc_state;

    assert(render_state->cc.viewport);

    dri_bo_map(render_state->cc.state, 1);
    assert(render_state->cc.state->virtual);
    cc_state = render_state->cc.state->virtual;
    memset(cc_state, 0, sizeof(*cc_state));

    cc_state->cc0.stencil_enable = 0;   /* disable stencil */
    cc_state->cc2.depth_test = 0;       /* disable depth test */
    cc_state->cc2.logicop_enable = 0;   /* disable logic op */
    cc_state->cc3.ia_blend_enable = 0 ;  /* blend alpha just like colors */
    cc_state->cc3.blend_enable = 1;     /* enable color blend */
    cc_state->cc3.alpha_test = 0;       /* disable alpha test */
    cc_state->cc3.alpha_test_format = 0;//0:ALPHATEST_UNORM8;       /*store alpha value with UNORM8 */
    cc_state->cc3.alpha_test_func = 5;//COMPAREFUNCTION_LESS;       /*pass if less than the reference */
    cc_state->cc4.cc_viewport_state_offset = render_state->cc.viewport->offset >> 5;

    cc_state->cc5.dither_enable = 0;    /* disable dither */
    cc_state->cc5.logicop_func = 0xc;   /* WHITE */
    cc_state->cc5.statistics_enable = 1;
    cc_state->cc5.ia_blend_function = I965_BLENDFUNCTION_ADD;
    cc_state->cc5.ia_src_blend_factor = I965_BLENDFACTOR_DST_ALPHA;
    cc_state->cc5.ia_dest_blend_factor = I965_BLENDFACTOR_DST_ALPHA;

    cc_state->cc6.clamp_post_alpha_blend = 0; 
    cc_state->cc6.clamp_pre_alpha_blend  =0; 
    
    /*final color = src_color*src_blend_factor +/- dst_color*dest_color_blend_factor*/
    cc_state->cc6.blend_function = I965_BLENDFUNCTION_ADD;
    cc_state->cc6.src_blend_factor = I965_BLENDFACTOR_SRC_ALPHA;
    cc_state->cc6.dest_blend_factor = I965_BLENDFACTOR_INV_SRC_ALPHA;
   
    /*alpha test reference*/
    cc_state->cc7.alpha_ref.f =0.0 ;


    dri_bo_emit_reloc(render_state->cc.state,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      0,
                      offsetof(struct i965_cc_unit_state, cc4),
                      render_state->cc.viewport);

    dri_bo_unmap(render_state->cc.state);
}


static void 
i965_render_cc_unit(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;
    struct i965_cc_unit_state *cc_state;

    assert(render_state->cc.viewport);

    dri_bo_map(render_state->cc.state, 1);
    assert(render_state->cc.state->virtual);
    cc_state = render_state->cc.state->virtual;
    memset(cc_state, 0, sizeof(*cc_state));

    cc_state->cc0.stencil_enable = 0;   /* disable stencil */
    cc_state->cc2.depth_test = 0;       /* disable depth test */
    cc_state->cc2.logicop_enable = 1;   /* enable logic op */
    cc_state->cc3.ia_blend_enable = 0;  /* blend alpha just like colors */
    cc_state->cc3.blend_enable = 0;     /* disable color blend */
    cc_state->cc3.alpha_test = 0;       /* disable alpha test */
    cc_state->cc4.cc_viewport_state_offset = render_state->cc.viewport->offset >> 5;

    cc_state->cc5.dither_enable = 0;    /* disable dither */
    cc_state->cc5.logicop_func = 0xc;   /* WHITE */
    cc_state->cc5.statistics_enable = 1;
    cc_state->cc5.ia_blend_function = I965_BLENDFUNCTION_ADD;
    cc_state->cc5.ia_src_blend_factor = I965_BLENDFACTOR_ONE;
    cc_state->cc5.ia_dest_blend_factor = I965_BLENDFACTOR_ONE;

    dri_bo_emit_reloc(render_state->cc.state,
                      I915_GEM_DOMAIN_INSTRUCTION, 0,
                      0,
                      offsetof(struct i965_cc_unit_state, cc4),
                      render_state->cc.viewport);

    dri_bo_unmap(render_state->cc.state);
}

static void
i965_render_src_surface_state(VADriverContextP ctx, 
                              int index,
                              dri_bo *region,
                              unsigned long offset,
                              int w, int h)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);  
    struct i965_render_state *render_state = &i965->render_state;
    struct i965_surface_state *ss;
    dri_bo *ss_bo;

    ss_bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "surface state", 
                      sizeof(struct i965_surface_state), 32);
    assert(ss_bo);
    dri_bo_map(ss_bo, 1);
    assert(ss_bo->virtual);
    ss = ss_bo->virtual;
    memset(ss, 0, sizeof(*ss));
    ss->ss0.surface_type = I965_SURFACE_2D;
    ss->ss0.surface_format = I965_SURFACEFORMAT_R8_UNORM;
    ss->ss0.writedisable_alpha = 0;
    ss->ss0.writedisable_red = 0;
    ss->ss0.writedisable_green = 0;
    ss->ss0.writedisable_blue = 0;
    ss->ss0.color_blend = 1;
    ss->ss0.vert_line_stride = 0;
    ss->ss0.vert_line_stride_ofs = 0;
    ss->ss0.mipmap_layout_mode = 0;
    ss->ss0.render_cache_read_mode = 0;

    ss->ss1.base_addr = region->offset + offset;

    ss->ss2.width = w - 1;
    ss->ss2.height = h - 1;
    ss->ss2.mip_count = 0;
    ss->ss2.render_target_rotation = 0;

    ss->ss3.pitch = w - 1;

    dri_bo_emit_reloc(ss_bo,
                      I915_GEM_DOMAIN_SAMPLER, 0,
                      offset,
                      offsetof(struct i965_surface_state, ss1),
                      region);

    dri_bo_unmap(ss_bo);

    assert(index < MAX_RENDER_SURFACES);
    assert(render_state->wm.surface[index] == NULL);
    render_state->wm.surface[index] = ss_bo;
    render_state->wm.sampler_count++;
}
static void
i965_subpic_render_src_surface_state(VADriverContextP ctx, 
                              int index,
                              dri_bo *region,
                              unsigned long offset,
                              int w, int h)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);  
    struct i965_render_state *render_state = &i965->render_state;
    struct i965_surface_state *ss;
    dri_bo *ss_bo;

    ss_bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "surface state", 
                      sizeof(struct i965_surface_state), 32);
    assert(ss_bo);
    dri_bo_map(ss_bo, 1);
    assert(ss_bo->virtual);
    ss = ss_bo->virtual;
    memset(ss, 0, sizeof(*ss));
    ss->ss0.surface_type = I965_SURFACE_2D;
    ss->ss0.surface_format = I965_SURFACEFORMAT_R8_UNORM;
    ss->ss0.writedisable_alpha = 0;
    ss->ss0.writedisable_red = 0;
    ss->ss0.writedisable_green = 0;
    ss->ss0.writedisable_blue = 0;
    ss->ss0.color_blend = 1;
    ss->ss0.vert_line_stride = 0;
    ss->ss0.vert_line_stride_ofs = 0;
    ss->ss0.mipmap_layout_mode = 0;
    ss->ss0.render_cache_read_mode = 0;

    ss->ss1.base_addr = region->offset + offset;

    ss->ss2.width = w - 1;
    ss->ss2.height = h - 1;
    ss->ss2.mip_count = 0;
    ss->ss2.render_target_rotation = 0;

    ss->ss3.pitch = w - 1;

    dri_bo_emit_reloc(ss_bo,
                      I915_GEM_DOMAIN_SAMPLER, 0,
                      offset,
                      offsetof(struct i965_surface_state, ss1),
                      region);

    dri_bo_unmap(ss_bo);

    assert(index < MAX_RENDER_SURFACES);
    assert(render_state->wm.surface[index] == NULL);
    render_state->wm.surface[index] = ss_bo;
    render_state->wm.sampler_count++;
}

static void
i965_render_src_surfaces_state(VADriverContextP ctx,
                              VASurfaceID surface)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);  
    struct object_surface *obj_surface;
    int w, h;
    dri_bo *region;

    obj_surface = SURFACE(surface);
    assert(obj_surface);
    assert(obj_surface->bo);
    w = obj_surface->width;
    h = obj_surface->height;
    region = obj_surface->bo;

    i965_render_src_surface_state(ctx, 1, region, 0, w, h);     /* Y */
    i965_render_src_surface_state(ctx, 2, region, 0, w, h);
    i965_render_src_surface_state(ctx, 3, region, w * h + w * h / 4, w / 2, h / 2);     /* V */
    i965_render_src_surface_state(ctx, 4, region, w * h + w * h / 4, w / 2, h / 2);
    i965_render_src_surface_state(ctx, 5, region, w * h, w / 2, h / 2); /* U */
    i965_render_src_surface_state(ctx, 6, region, w * h, w / 2, h / 2);
}

static void
i965_subpic_render_src_surfaces_state(VADriverContextP ctx,
                              VASurfaceID surface)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);  
    struct object_surface *obj_surface = SURFACE(surface);
    int w, h;
    dri_bo *region;
    dri_bo *subpic_region;
    struct object_subpic *obj_subpic = SUBPIC(obj_surface->subpic);
    struct object_image *obj_image = IMAGE(obj_subpic->image);
    assert(obj_surface);
    assert(obj_surface->bo);
    w = obj_surface->width;
    h = obj_surface->height;
    region = obj_surface->bo;
    subpic_region = obj_image->bo;
    /*subpicture surface*/
    i965_subpic_render_src_surface_state(ctx, 1, subpic_region, 0, obj_image->width, obj_image->height);     
    i965_subpic_render_src_surface_state(ctx, 2, subpic_region, 0, obj_image->width, obj_image->height);
}

static void
i965_render_set_surface_tiling(struct i965_surface_state *ss, unsigned int tiling)
{
    switch (tiling) {
    case I915_TILING_NONE:
        ss->ss3.tiled_surface = 0;
        ss->ss3.tile_walk = 0;
        break;
    case I915_TILING_X:
        ss->ss3.tiled_surface = 1;
        ss->ss3.tile_walk = I965_TILEWALK_XMAJOR;
        break;
    case I915_TILING_Y:
        ss->ss3.tiled_surface = 1;
        ss->ss3.tile_walk = I965_TILEWALK_YMAJOR;
        break;
    }
}

static void
i965_render_dest_surface_state(VADriverContextP ctx, int index)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);  
    struct i965_render_state *render_state = &i965->render_state;
    struct intel_region *dest_region = render_state->draw_region;
    struct i965_surface_state *ss;
    dri_bo *ss_bo;

    ss_bo = dri_bo_alloc(i965->intel.bufmgr, 
                      "surface state", 
                      sizeof(struct i965_surface_state), 32);
    assert(ss_bo);
    dri_bo_map(ss_bo, 1);
    assert(ss_bo->virtual);
    ss = ss_bo->virtual;
    memset(ss, 0, sizeof(*ss));

    ss->ss0.surface_type = I965_SURFACE_2D;
    ss->ss0.data_return_format = I965_SURFACERETURNFORMAT_FLOAT32;

    if (dest_region->cpp == 2) {
	ss->ss0.surface_format = I965_SURFACEFORMAT_B5G6R5_UNORM;
	} else {
	ss->ss0.surface_format = I965_SURFACEFORMAT_B8G8R8A8_UNORM;
    }

    ss->ss0.writedisable_alpha = 0;
    ss->ss0.writedisable_red = 0;
    ss->ss0.writedisable_green = 0;
    ss->ss0.writedisable_blue = 0;
    ss->ss0.color_blend = 1;
    ss->ss0.vert_line_stride = 0;
    ss->ss0.vert_line_stride_ofs = 0;
    ss->ss0.mipmap_layout_mode = 0;
    ss->ss0.render_cache_read_mode = 0;

    ss->ss1.base_addr = dest_region->bo->offset;

    ss->ss2.width = dest_region->width - 1;
    ss->ss2.height = dest_region->height - 1;
    ss->ss2.mip_count = 0;
    ss->ss2.render_target_rotation = 0;
    ss->ss3.pitch = dest_region->pitch - 1;
    i965_render_set_surface_tiling(ss, dest_region->tiling);

    dri_bo_emit_reloc(ss_bo,
                      I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                      0,
                      offsetof(struct i965_surface_state, ss1),
                      dest_region->bo);

    dri_bo_unmap(ss_bo);

    assert(index < MAX_RENDER_SURFACES);
    assert(render_state->wm.surface[index] == NULL);
    render_state->wm.surface[index] = ss_bo;
}

static void
i965_render_binding_table(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;
    int i;
    unsigned int *binding_table;

    dri_bo_map(render_state->wm.binding_table, 1);
    assert(render_state->wm.binding_table->virtual);
    binding_table = render_state->wm.binding_table->virtual;
    memset(binding_table, 0, render_state->wm.binding_table->size);

    for (i = 0; i < MAX_RENDER_SURFACES; i++) {
        if (render_state->wm.surface[i]) {
            binding_table[i] = render_state->wm.surface[i]->offset;
            dri_bo_emit_reloc(render_state->wm.binding_table,
                              I915_GEM_DOMAIN_INSTRUCTION, 0,
                              0,
                              i * sizeof(*binding_table),
                              render_state->wm.surface[i]);
        }
    }

    dri_bo_unmap(render_state->wm.binding_table);
}

static void 
i965_subpic_render_upload_vertex(VADriverContextP ctx,
                          VASurfaceID surface,
                          short srcx,
                          short srcy,
                          unsigned short srcw,
                          unsigned short srch,
                          short destx,
                          short desty,
                          unsigned short destw,
                          unsigned short desth)
{    
	struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;
    struct intel_region *dest_region = render_state->draw_region;
    struct object_surface *obj_surface;
    struct object_subpic *obj_subpic;
    float *vb;
    float src_scale_x, src_scale_y;
    int i, width, height;
    obj_surface = SURFACE(surface);
    obj_subpic = SUBPIC(obj_surface->subpic);
    assert(obj_surface);
    assert(obj_subpic);
    
    int box_x1 = dest_region->x + obj_subpic->dstx;
    int box_y1 = dest_region->y + obj_subpic->dsty;
    int box_x2 = box_x1 + obj_subpic->width;
    int box_y2 = box_y1 + obj_subpic->height;

    width = obj_surface->width;
    height = obj_surface->height;
    src_scale_x = ((float)srcw / width) / (float)destw;
    src_scale_y = ((float)srch / height) / (float)desth;

    dri_bo_map(render_state->vb.vertex_buffer, 1);
    assert(render_state->vb.vertex_buffer->virtual);
    vb = render_state->vb.vertex_buffer->virtual;
    /*vertex covers the full subpicture*/
    i = 0;
    vb[i++] = 1;
    vb[i++] = 1;
    vb[i++] = (float)box_x2;
    vb[i++] = (float)box_y2;
    
    vb[i++] = 0.0;
    vb[i++] = 1;
    vb[i++] = (float)box_x1;
    vb[i++] = (float)box_y2;

    vb[i++] = 0.0;
    vb[i++] = 0.0;
    vb[i++] = (float)box_x1;
    vb[i++] = (float)box_y1;
    dri_bo_unmap(render_state->vb.vertex_buffer);
}

static void 
i965_render_upload_vertex(VADriverContextP ctx,
                          VASurfaceID surface,
                          short srcx,
                          short srcy,
                          unsigned short srcw,
                          unsigned short srch,
                          short destx,
                          short desty,
                          unsigned short destw,
                          unsigned short desth)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;
    struct intel_region *dest_region = render_state->draw_region;
    struct object_surface *obj_surface;
    float *vb;

    float u1, v1, u2, v2;
    int i, width, height;
    int box_x1 = dest_region->x + destx;
    int box_y1 = dest_region->y + desty;
    int box_x2 = box_x1 + destw;
    int box_y2 = box_y1 + desth;

    obj_surface = SURFACE(surface);
    assert(surface);
    width = obj_surface->width;
    height = obj_surface->height;

    u1 = (float)srcx / width;
    v1 = (float)srcy / height;
    u2 = (float)(srcx + srcw) / width;
    v2 = (float)(srcy + srch) / height;

    dri_bo_map(render_state->vb.vertex_buffer, 1);
    assert(render_state->vb.vertex_buffer->virtual);
    vb = render_state->vb.vertex_buffer->virtual;

    i = 0;
    vb[i++] = u2;
    vb[i++] = v2;
    vb[i++] = (float)box_x2;
    vb[i++] = (float)box_y2;
    
    vb[i++] = u1;
    vb[i++] = v2;
    vb[i++] = (float)box_x1;
    vb[i++] = (float)box_y2;

    vb[i++] = u1;
    vb[i++] = v1;
    vb[i++] = (float)box_x1;
    vb[i++] = (float)box_y1;

    dri_bo_unmap(render_state->vb.vertex_buffer);
}

static void
i965_surface_render_state_setup(VADriverContextP ctx,
                        VASurfaceID surface,
                        short srcx,
                        short srcy,
                        unsigned short srcw,
                        unsigned short srch,
                        short destx,
                        short desty,
                        unsigned short destw,
                        unsigned short desth)
{
    i965_render_vs_unit(ctx);
    i965_render_sf_unit(ctx);
    i965_render_dest_surface_state(ctx, 0);
    i965_render_src_surfaces_state(ctx, surface);
    i965_render_sampler(ctx);
    i965_render_wm_unit(ctx);
    i965_render_cc_viewport(ctx);
    i965_render_cc_unit(ctx);
    i965_render_binding_table(ctx);
    i965_render_upload_vertex(ctx, surface,
                              srcx, srcy, srcw, srch,
                              destx, desty, destw, desth);
}
static void
i965_subpic_render_state_setup(VADriverContextP ctx,
                        VASurfaceID surface,
                        short srcx,
                        short srcy,
                        unsigned short srcw,
                        unsigned short srch,
                        short destx,
                        short desty,
                        unsigned short destw,
                        unsigned short desth)
{
    i965_render_vs_unit(ctx);
    i965_render_sf_unit(ctx);
    i965_render_dest_surface_state(ctx, 0);
    i965_subpic_render_src_surfaces_state(ctx, surface);
    i965_render_sampler(ctx);
    i965_subpic_render_wm_unit(ctx);
    i965_render_cc_viewport(ctx);
    i965_subpic_render_cc_unit(ctx);
    i965_render_binding_table(ctx);
    i965_subpic_render_upload_vertex(ctx, surface,
	    srcx, srcy, srcw, srch,
	    destx, desty, destw, desth);
}


static void
i965_render_pipeline_select(VADriverContextP ctx)
{
    BEGIN_BATCH(ctx, 1);
    OUT_BATCH(ctx, CMD_PIPELINE_SELECT | PIPELINE_SELECT_3D);
    ADVANCE_BATCH(ctx);
}

static void
i965_render_state_sip(VADriverContextP ctx)
{
    BEGIN_BATCH(ctx, 2);
    OUT_BATCH(ctx, CMD_STATE_SIP | 0);
    OUT_BATCH(ctx, 0);
    ADVANCE_BATCH(ctx);
}

static void
i965_render_state_base_address(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);

    if (IS_IGDNG(i965->intel.device_id)) {
        BEGIN_BATCH(ctx, 8);
        OUT_BATCH(ctx, CMD_STATE_BASE_ADDRESS | 6);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        ADVANCE_BATCH(ctx);
    } else {
        BEGIN_BATCH(ctx, 6);
        OUT_BATCH(ctx, CMD_STATE_BASE_ADDRESS | 4);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        OUT_BATCH(ctx, 0 | BASE_ADDRESS_MODIFY);
        ADVANCE_BATCH(ctx);
    }
}

static void
i965_render_binding_table_pointers(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;

    BEGIN_BATCH(ctx, 6);
    OUT_BATCH(ctx, CMD_BINDING_TABLE_POINTERS | 4);
    OUT_BATCH(ctx, 0); /* vs */
    OUT_BATCH(ctx, 0); /* gs */
    OUT_BATCH(ctx, 0); /* clip */
    OUT_BATCH(ctx, 0); /* sf */
    OUT_RELOC(ctx, render_state->wm.binding_table, I915_GEM_DOMAIN_INSTRUCTION, 0, 0); /* wm */
    ADVANCE_BATCH(ctx);
}

static void 
i965_render_constant_color(VADriverContextP ctx)
{
    BEGIN_BATCH(ctx, 5);
    OUT_BATCH(ctx, CMD_CONSTANT_COLOR | 3);
    OUT_BATCH(ctx, float_to_uint(1.0));
    OUT_BATCH(ctx, float_to_uint(0.0));
    OUT_BATCH(ctx, float_to_uint(1.0));
    OUT_BATCH(ctx, float_to_uint(1.0));
    ADVANCE_BATCH(ctx);
}

static void
i965_render_pipelined_pointers(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;

    BEGIN_BATCH(ctx, 7);
    OUT_BATCH(ctx, CMD_PIPELINED_POINTERS | 5);
    OUT_RELOC(ctx, render_state->vs.state, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
    OUT_BATCH(ctx, 0);  /* disable GS */
    OUT_BATCH(ctx, 0);  /* disable CLIP */
    OUT_RELOC(ctx, render_state->sf.state, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
    OUT_RELOC(ctx, render_state->wm.state, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
    OUT_RELOC(ctx, render_state->cc.state, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
    ADVANCE_BATCH(ctx);
}

static void
i965_render_urb_layout(VADriverContextP ctx)
{
    int urb_vs_start, urb_vs_size;
    int urb_gs_start, urb_gs_size;
    int urb_clip_start, urb_clip_size;
    int urb_sf_start, urb_sf_size;
    int urb_cs_start, urb_cs_size;

    urb_vs_start = 0;
    urb_vs_size = URB_VS_ENTRIES * URB_VS_ENTRY_SIZE;
    urb_gs_start = urb_vs_start + urb_vs_size;
    urb_gs_size = URB_GS_ENTRIES * URB_GS_ENTRY_SIZE;
    urb_clip_start = urb_gs_start + urb_gs_size;
    urb_clip_size = URB_CLIP_ENTRIES * URB_CLIP_ENTRY_SIZE;
    urb_sf_start = urb_clip_start + urb_clip_size;
    urb_sf_size = URB_SF_ENTRIES * URB_SF_ENTRY_SIZE;
    urb_cs_start = urb_sf_start + urb_sf_size;
    urb_cs_size = URB_CS_ENTRIES * URB_CS_ENTRY_SIZE;

    BEGIN_BATCH(ctx, 3);
    OUT_BATCH(ctx, 
              CMD_URB_FENCE |
              UF0_CS_REALLOC |
              UF0_SF_REALLOC |
              UF0_CLIP_REALLOC |
              UF0_GS_REALLOC |
              UF0_VS_REALLOC |
              1);
    OUT_BATCH(ctx, 
              ((urb_clip_start + urb_clip_size) << UF1_CLIP_FENCE_SHIFT) |
              ((urb_gs_start + urb_gs_size) << UF1_GS_FENCE_SHIFT) |
              ((urb_vs_start + urb_vs_size) << UF1_VS_FENCE_SHIFT));
    OUT_BATCH(ctx,
              ((urb_cs_start + urb_cs_size) << UF2_CS_FENCE_SHIFT) |
              ((urb_sf_start + urb_sf_size) << UF2_SF_FENCE_SHIFT));
    ADVANCE_BATCH(ctx);
}

static void 
i965_render_cs_urb_layout(VADriverContextP ctx)
{
    BEGIN_BATCH(ctx, 2);
    OUT_BATCH(ctx, CMD_CS_URB_STATE | 0);
    OUT_BATCH(ctx,
              ((URB_CS_ENTRY_SIZE - 1) << 4) |          /* URB Entry Allocation Size */
              (URB_CS_ENTRIES << 0));                /* Number of URB Entries */
    ADVANCE_BATCH(ctx);
}

static void
i965_render_drawing_rectangle(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);  
    struct i965_render_state *render_state = &i965->render_state;
    struct intel_region *dest_region = render_state->draw_region;

    BEGIN_BATCH(ctx, 4);
    OUT_BATCH(ctx, CMD_DRAWING_RECTANGLE | 2);
    OUT_BATCH(ctx, 0x00000000);
    OUT_BATCH(ctx, (dest_region->width - 1) | (dest_region->height - 1) << 16);
    OUT_BATCH(ctx, 0x00000000);         
    ADVANCE_BATCH(ctx);
}

static void
i965_render_vertex_elements(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);  

    if (IS_IGDNG(i965->intel.device_id)) {
        BEGIN_BATCH(ctx, 5);
        OUT_BATCH(ctx, CMD_VERTEX_ELEMENTS | 3);
        /* offset 0: X,Y -> {X, Y, 1.0, 1.0} */
        OUT_BATCH(ctx, (0 << VE0_VERTEX_BUFFER_INDEX_SHIFT) |
                  VE0_VALID |
                  (I965_SURFACEFORMAT_R32G32_FLOAT << VE0_FORMAT_SHIFT) |
                  (0 << VE0_OFFSET_SHIFT));
        OUT_BATCH(ctx, (I965_VFCOMPONENT_STORE_SRC << VE1_VFCOMPONENT_0_SHIFT) |
                  (I965_VFCOMPONENT_STORE_SRC << VE1_VFCOMPONENT_1_SHIFT) |
                  (I965_VFCOMPONENT_STORE_1_FLT << VE1_VFCOMPONENT_2_SHIFT) |
                  (I965_VFCOMPONENT_STORE_1_FLT << VE1_VFCOMPONENT_3_SHIFT));
        /* offset 8: S0, T0 -> {S0, T0, 1.0, 1.0} */
        OUT_BATCH(ctx, (0 << VE0_VERTEX_BUFFER_INDEX_SHIFT) |
                  VE0_VALID |
                  (I965_SURFACEFORMAT_R32G32_FLOAT << VE0_FORMAT_SHIFT) |
                  (8 << VE0_OFFSET_SHIFT));
        OUT_BATCH(ctx, (I965_VFCOMPONENT_STORE_SRC << VE1_VFCOMPONENT_0_SHIFT) |
                  (I965_VFCOMPONENT_STORE_SRC << VE1_VFCOMPONENT_1_SHIFT) |
                  (I965_VFCOMPONENT_STORE_1_FLT << VE1_VFCOMPONENT_2_SHIFT) |
                  (I965_VFCOMPONENT_STORE_1_FLT << VE1_VFCOMPONENT_3_SHIFT));
        ADVANCE_BATCH(ctx);
    } else {
        BEGIN_BATCH(ctx, 5);
        OUT_BATCH(ctx, CMD_VERTEX_ELEMENTS | 3);
        /* offset 0: X,Y -> {X, Y, 1.0, 1.0} */
        OUT_BATCH(ctx, (0 << VE0_VERTEX_BUFFER_INDEX_SHIFT) |
                  VE0_VALID |
                  (I965_SURFACEFORMAT_R32G32_FLOAT << VE0_FORMAT_SHIFT) |
                  (0 << VE0_OFFSET_SHIFT));
        OUT_BATCH(ctx, (I965_VFCOMPONENT_STORE_SRC << VE1_VFCOMPONENT_0_SHIFT) |
                  (I965_VFCOMPONENT_STORE_SRC << VE1_VFCOMPONENT_1_SHIFT) |
                  (I965_VFCOMPONENT_STORE_1_FLT << VE1_VFCOMPONENT_2_SHIFT) |
                  (I965_VFCOMPONENT_STORE_1_FLT << VE1_VFCOMPONENT_3_SHIFT) |
                  (0 << VE1_DESTINATION_ELEMENT_OFFSET_SHIFT));
        /* offset 8: S0, T0 -> {S0, T0, 1.0, 1.0} */
        OUT_BATCH(ctx, (0 << VE0_VERTEX_BUFFER_INDEX_SHIFT) |
                  VE0_VALID |
                  (I965_SURFACEFORMAT_R32G32_FLOAT << VE0_FORMAT_SHIFT) |
                  (8 << VE0_OFFSET_SHIFT));
        OUT_BATCH(ctx, (I965_VFCOMPONENT_STORE_SRC << VE1_VFCOMPONENT_0_SHIFT) |
                  (I965_VFCOMPONENT_STORE_SRC << VE1_VFCOMPONENT_1_SHIFT) |
                  (I965_VFCOMPONENT_STORE_1_FLT << VE1_VFCOMPONENT_2_SHIFT) |
                  (I965_VFCOMPONENT_STORE_1_FLT << VE1_VFCOMPONENT_3_SHIFT) |
                  (4 << VE1_DESTINATION_ELEMENT_OFFSET_SHIFT));
        ADVANCE_BATCH(ctx);
    }
}

void
i965_render_upload_palette(VADriverContextP ctx)
{
    BEGIN_BATCH(ctx, 17);
    OUT_BATCH(ctx, CMD_SAMPLER_PALETTE_LOAD | 15);
    /*fill palette*/
    //int32_t out[16]; //0-23:color 23-31:alpha
    int32_t i,c;
    for(i = 0; i < 16; i ++){
        c = i*16; //16 colors
        OUT_BATCH(ctx,c<<24/*alpha*/|c<<16/*R*/|c<<8/*G*/|c/*B*/);//c<<24/*alpha*/|c<<16/*R*/|c<<8/*G*/|c/*B*/);
    }

    ADVANCE_BATCH(ctx);
}
static void
i965_render_startup(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;

    BEGIN_BATCH(ctx, 11);
    OUT_BATCH(ctx, CMD_VERTEX_BUFFERS | 3);
    OUT_BATCH(ctx, 
              (0 << VB0_BUFFER_INDEX_SHIFT) |
              VB0_VERTEXDATA |
              ((4 * 4) << VB0_BUFFER_PITCH_SHIFT));
    OUT_RELOC(ctx, render_state->vb.vertex_buffer, I915_GEM_DOMAIN_VERTEX, 0, 0);

    if (IS_IGDNG(i965->intel.device_id))
        OUT_RELOC(ctx, render_state->vb.vertex_buffer, I915_GEM_DOMAIN_VERTEX, 0, 12 * 4);
    else
        OUT_BATCH(ctx, 3);

    OUT_BATCH(ctx, 0);

    OUT_BATCH(ctx, 
              CMD_3DPRIMITIVE |
              _3DPRIMITIVE_VERTEX_SEQUENTIAL |
              (_3DPRIM_RECTLIST << _3DPRIMITIVE_TOPOLOGY_SHIFT) |
              (0 << 9) |
              4);
    OUT_BATCH(ctx, 3); /* vertex count per instance */
    OUT_BATCH(ctx, 0); /* start vertex offset */
    OUT_BATCH(ctx, 1); /* single instance */
    OUT_BATCH(ctx, 0); /* start instance location */
    OUT_BATCH(ctx, 0); /* index buffer offset, ignored */
    ADVANCE_BATCH(ctx);
}

static void
i965_surface_render_pipeline_setup(VADriverContextP ctx)
{
    intel_batchbuffer_start_atomic(ctx, 0x1000);
    intel_batchbuffer_emit_mi_flush(ctx);
    i965_render_pipeline_select(ctx);
    i965_render_state_sip(ctx);
    i965_render_state_base_address(ctx);
    i965_render_binding_table_pointers(ctx);
    i965_render_constant_color(ctx);
    i965_render_pipelined_pointers(ctx);
    i965_render_urb_layout(ctx);
    i965_render_cs_urb_layout(ctx);
    i965_render_drawing_rectangle(ctx);
    i965_render_vertex_elements(ctx);
    i965_render_startup(ctx);
    intel_batchbuffer_end_atomic(ctx);
}

static void
i965_subpic_render_pipeline_setup(VADriverContextP ctx)
{
    intel_batchbuffer_start_atomic(ctx, 0x1000);
    intel_batchbuffer_emit_mi_flush(ctx);
    i965_render_pipeline_select(ctx);
    i965_render_state_sip(ctx);
    i965_render_state_base_address(ctx);
    i965_render_binding_table_pointers(ctx);
    i965_render_constant_color(ctx);
    i965_render_pipelined_pointers(ctx);
    //i965_render_upload_palette(ctx);
    i965_render_urb_layout(ctx);
    i965_render_cs_urb_layout(ctx);
    i965_render_drawing_rectangle(ctx);
    i965_render_vertex_elements(ctx);
    i965_render_startup(ctx);
    intel_batchbuffer_end_atomic(ctx);
}


static void 
i965_render_initialize(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;
    int i;
    dri_bo *bo;

    /* VERTEX BUFFER */
    dri_bo_unreference(render_state->vb.vertex_buffer);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "vertex buffer",
                      4096,
                      4096);
    assert(bo);
    render_state->vb.vertex_buffer = bo;

    /* VS */
    dri_bo_unreference(render_state->vs.state);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "vs state",
                      sizeof(struct i965_vs_unit_state),
                      64);
    assert(bo);
    render_state->vs.state = bo;

    /* GS */
    /* CLIP */
    /* SF */
    dri_bo_unreference(render_state->sf.state);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "sf state",
                      sizeof(struct i965_sf_unit_state),
                      64);
    assert(bo);
    render_state->sf.state = bo;

    /* WM */
    for (i = 0; i < MAX_RENDER_SURFACES; i++) {
        dri_bo_unreference(render_state->wm.surface[i]);
        render_state->wm.surface[i] = NULL;
    }

    dri_bo_unreference(render_state->wm.binding_table);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "binding table",
                      MAX_RENDER_SURFACES * sizeof(unsigned int),
                      64);
    assert(bo);
    render_state->wm.binding_table = bo;

    dri_bo_unreference(render_state->wm.sampler);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "sampler state",
                      MAX_SAMPLERS * sizeof(struct i965_sampler_state),
                      64);
    assert(bo);
    render_state->wm.sampler = bo;
    render_state->wm.sampler_count = 0;

    dri_bo_unreference(render_state->wm.state);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "wm state",
                      sizeof(struct i965_wm_unit_state),
                      64);
    assert(bo);
    render_state->wm.state = bo;

    /* COLOR CALCULATOR */
    dri_bo_unreference(render_state->cc.state);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "color calc state",
                      sizeof(struct i965_cc_unit_state),
                      64);
    assert(bo);
    render_state->cc.state = bo;

    dri_bo_unreference(render_state->cc.viewport);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "cc viewport",
                      sizeof(struct i965_cc_viewport),
                      64);
    assert(bo);
    render_state->cc.viewport = bo;
}

void
i965_render_put_surface(VADriverContextP ctx,
                        VASurfaceID surface,
                        short srcx,
                        short srcy,
                        unsigned short srcw,
                        unsigned short srch,
                        short destx,
                        short desty,
                        unsigned short destw,
                        unsigned short desth)
{
    i965_render_initialize(ctx);
    i965_surface_render_state_setup(ctx, surface,
                            srcx, srcy, srcw, srch,
                            destx, desty, destw, desth);
    i965_surface_render_pipeline_setup(ctx);
    intel_batchbuffer_flush(ctx);
}

void
i965_render_put_subpic(VADriverContextP ctx,
                        VASurfaceID surface,
                        short srcx,
                        short srcy,
                        unsigned short srcw,
                        unsigned short srch,
                        short destx,
                        short desty,
                        unsigned short destw,
                        unsigned short desth)
{
    i965_render_initialize(ctx);
    i965_subpic_render_state_setup(ctx, surface,
	    srcx, srcy, srcw, srch,
	    destx, desty, destw, desth);
    i965_subpic_render_pipeline_setup(ctx);
    intel_batchbuffer_flush(ctx);
}


Bool 
i965_render_init(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    int i;

    /* kernel */
    assert(NUM_RENDER_KERNEL == (sizeof(render_kernels_gen5) / 
                                 sizeof(render_kernels_gen5[0])));

    if (IS_IGDNG(i965->intel.device_id))
        render_kernels = render_kernels_gen5;
    else
        render_kernels = render_kernels_gen4;

    for (i = 0; i < NUM_RENDER_KERNEL; i++) {
        struct render_kernel *kernel = &render_kernels[i];
        kernel->bo = dri_bo_alloc(i965->intel.bufmgr, 
                                  kernel->name, 
                                  kernel->size, 64);
        assert(kernel->bo);
        dri_bo_subdata(kernel->bo, 0, kernel->size, kernel->bin);
    }

    return True;
}

Bool 
i965_render_terminate(VADriverContextP ctx)
{
    int i;
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct i965_render_state *render_state = &i965->render_state;

    for (i = 0; i < NUM_RENDER_KERNEL; i++) {
        struct render_kernel *kernel = &render_kernels[i];
        
        dri_bo_unreference(kernel->bo);
        kernel->bo = NULL;
    }

    dri_bo_unreference(render_state->vb.vertex_buffer);
    render_state->vb.vertex_buffer = NULL;
    dri_bo_unreference(render_state->vs.state);
    render_state->vs.state = NULL;
    dri_bo_unreference(render_state->sf.state);
    render_state->sf.state = NULL;
    dri_bo_unreference(render_state->wm.binding_table);
    render_state->wm.binding_table = NULL;
    dri_bo_unreference(render_state->wm.sampler);
    render_state->wm.sampler = NULL;
    dri_bo_unreference(render_state->wm.state);
    render_state->wm.state = NULL;

    for (i = 0; i < MAX_RENDER_SURFACES; i++) {
        dri_bo_unreference(render_state->wm.surface[i]);
        render_state->wm.surface[i] = NULL;
    }

    dri_bo_unreference(render_state->cc.viewport);
    render_state->cc.viewport = NULL;
    dri_bo_unreference(render_state->cc.state);
    render_state->cc.state = NULL;

    if (render_state->draw_region) {
        dri_bo_unreference(render_state->draw_region->bo);
        free(render_state->draw_region);
        render_state->draw_region = NULL;
    }

    return True;
}

