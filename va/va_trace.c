/*
 * Copyright (c) 2009 Intel Corporation. All Rights Reserved.
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

#define _GNU_SOURCE 1
#include "va.h"
#include "va_backend.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>

unsigned int trace_flag = 0;

static const char *trace_file = 0;
static FILE *trace_fp = 0;

static VASurfaceID  trace_rendertarget; /* current render target */
static VAProfile trace_profile; /* current entrypoint for buffers */

static unsigned int trace_frame;
static unsigned int trace_slice;

static unsigned int trace_width;
static unsigned int trace_height;

int va_TraceInit(void)
{
    trace_file = (const char *)getenv("LIBVA_TRACE");
    if (trace_file) {
	trace_fp = fopen(trace_file, "w");
        if (trace_fp)
            trace_flag = 1;
    }
}

int va_TraceEnd(void)
{
    if (trace_file && trace_fp) {
        fclose(trace_fp);

        trace_file = NULL;
        trace_fp = NULL;

        trace_flag = 0;

        trace_width = 0;
        trace_height = 0;
    }
}

int va_TraceMsg(const char *msg, ...)
{
    va_list args;
    
    if (msg)  {
        va_start(args, msg);
        vfprintf(trace_fp, msg, args);
        va_end(args);
    } else  {
        fflush(trace_fp);
    }
}


int va_TraceCreateConfig(
    VADisplay dpy,
    VAProfile profile, 
    VAEntrypoint entrypoint, 
    VAConfigAttrib *attrib_list,
    int num_attribs,
    VAConfigID *config_id /* out */
                         )
{
    int i;
    
    va_TraceMsg("\tprofile = %d\n", profile);
    va_TraceMsg("\tentrypoint = %d\n", entrypoint);
    va_TraceMsg("\tnum_attribs = %d\n", num_attribs);
    for (i = 0; i < num_attribs; i++) {
	va_TraceMsg("\t\tattrib_list[%d].type = 0x%08x\n", i, attrib_list[i].type);
        va_TraceMsg("\t\tattrib_list[%d].value = 0x%08x\n", i, attrib_list[i].value);
    }

    trace_profile = profile;
}


int va_TraceCreateSurface(
    VADisplay dpy,
    int width,
    int height,
    int format,
    int num_surfaces,
    VASurfaceID *surfaces	/* out */
                          )
{
    int i;
    
    va_TraceMsg("\twidth = %d\n", width);
    va_TraceMsg("\theight = %d\n", height);
    va_TraceMsg("\tformat = %d\n", format);
    va_TraceMsg("\tnum_surfaces = %d\n", num_surfaces);

    for (i = 0; i < num_surfaces; i++)
        va_TraceMsg("\t\tsurfaces[%d] = 0x%08x\n", i, surfaces[i]);
}


int va_TraceCreateContext(
    VADisplay dpy,
    VAConfigID config_id,
    int picture_width,
    int picture_height,
    int flag,
    VASurfaceID *render_targets,
    int num_render_targets,
    VAContextID *context		/* out */
                          )
{
    int i;

    va_TraceMsg("\twidth = %d\n", picture_width);
    va_TraceMsg("\theight = %d\n", picture_height);
    va_TraceMsg("\tflag = 0x%08x\n", flag);
    va_TraceMsg("\tnum_render_targets = %d\n", num_render_targets);
    for (i=0; i<num_render_targets; i++)
        va_TraceMsg("\t\trender_targets[%d] = 0x%08x\n", i, render_targets[i]);
    va_TraceMsg("\tcontext = 0x%08x\n", context);


    trace_frame = 0;
    trace_slice = 0;

    trace_width = picture_width;
    trace_height = picture_height;
}


static char * buffer_type_to_string(int type)
{
    switch (type) {
    case VAPictureParameterBufferType: return "VAPictureParameterBufferType";
    case VAIQMatrixBufferType: return "VAIQMatrixBufferType";
    case VABitPlaneBufferType: return "VABitPlaneBufferType";
    case VASliceGroupMapBufferType: return "VASliceGroupMapBufferType";
    case VASliceParameterBufferType: return "VASliceParameterBufferType";
    case VASliceDataBufferType: return "VASliceDataBufferType";
    case VAProtectedSliceDataBufferType: return "VAProtectedSliceDataBufferType";
    case VAMacroblockParameterBufferType: return "VAMacroblockParameterBufferType";
    case VAResidualDataBufferType: return "VAResidualDataBufferType";
    case VADeblockingParameterBufferType: return "VADeblockingParameterBufferType";
    case VAImageBufferType: return "VAImageBufferType";
    case VAEncCodedBufferType: return "VAEncCodedBufferType";
    case VAEncSequenceParameterBufferType: return "VAEncSequenceParameterBufferType";
    case VAEncPictureParameterBufferType: return "VAEncPictureParameterBufferType";
    case VAEncSliceParameterBufferType: return "VAEncSliceParameterBufferType";
    default: return "UnknowBuffer";
    }
}


static int va_TraceVABuffers(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *pbuf
                             )
{
    int i;
    unsigned char *p = pbuf;
    unsigned int *pi = (unsigned int *)pbuf;

    va_TraceMsg("***Buffer Data***");
    for (i=0; i<size; i++) {
        if ((i%16) == 0)
            va_TraceMsg("\n0x%08x:", i);
        va_TraceMsg(" %02x", p[i]);
    }

    va_TraceMsg("\n");
    return 0;
}


static void va_TraceVAPictureParameterBufferMPEG2(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, data);
    
    return;
}


static void va_TraceVAIQMatrixBufferMPEG2(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, data);
    
    return;
}


static void va_TraceVASliceParameterBufferMPEG2(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, data);
    
    return;
}



static void va_TraceVAPictureParameterBufferMPEG4(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, data);
    
    return;
}


static void va_TraceVAIQMatrixBufferMPEG4(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, data);
    
    return;
}


static void va_TraceVASliceParameterBufferMPEG4(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, data);
    
    return;
}


static void va_TraceVAPictureParameterBufferH264(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    int i;

    VAPictureParameterBufferH264 *p = (VAPictureParameterBufferH264*)data;

    va_TraceMsg  ("==========H264PicParameterBuffer============\n");

#if 0    
    if (p->num_ref_frames > 4)
    {
        int num = 0;
        for (i = 15; i >= 0; i--)
        {
            if (p->ReferenceFrames[i].flags != VA_PICTURE_H264_INVALID)
            {
                num++;
            }
            if (num > 4)
            {
                p->ReferenceFrames[i].flags = VA_PICTURE_H264_INVALID;
            }
        }  	
        p->num_ref_frames  = 4;
    }
#endif
    
#if 1
    va_TraceMsg("picture id: %d\n", p->CurrPic.picture_id);
    va_TraceMsg("frame idx: %d\n", p->CurrPic.frame_idx);
    va_TraceMsg("picture flags: %d\n", p->CurrPic.flags);
    va_TraceMsg("top field order count: %d\n", p->CurrPic.TopFieldOrderCnt);
    va_TraceMsg("bottom field order count: %d\n", p->CurrPic.BottomFieldOrderCnt);


    va_TraceMsg("Reference frames: \n");
    for (i = 0; i < 16; i++)
    {
        if (p->ReferenceFrames[i].flags != VA_PICTURE_H264_INVALID)
        {
            //va_TraceMsg("%d-%d; ", p->ReferenceFrames[i].TopFieldOrderCnt, p->ReferenceFrames[i].BottomFieldOrderCnt);
            va_TraceMsg("%d-%d-%d-%d; ", p->ReferenceFrames[i].TopFieldOrderCnt, p->ReferenceFrames[i].BottomFieldOrderCnt, p->ReferenceFrames[i].picture_id, p->ReferenceFrames[i].frame_idx);
        }
    }
    va_TraceMsg("\n");
#endif
    va_TraceMsg("picture_width_in_mbs_minus1: %d\n", p->picture_width_in_mbs_minus1);
    va_TraceMsg("picture_height_in_mbs_minus1: %d\n", p->picture_height_in_mbs_minus1);
    va_TraceMsg("bit_depth_luma_minus8: %d\n", p->bit_depth_luma_minus8);
    va_TraceMsg("bit_depth_chroma_minus8: %d\n", p->bit_depth_chroma_minus8);
    va_TraceMsg("num_ref_frames: %d\n", p->num_ref_frames);
    va_TraceMsg("seq fields: %d\n", p->seq_fields.value);
    va_TraceMsg("\t chroma_format_idc: %d\n", p->seq_fields.bits.chroma_format_idc);
    va_TraceMsg("\t residual_colour_transform_flag: %d\n", p->seq_fields.bits.residual_colour_transform_flag);
    va_TraceMsg("\t frame_mbs_only_flag: %d\n", p->seq_fields.bits.frame_mbs_only_flag);
    va_TraceMsg("\t mb_adaptive_frame_field_flag: %d\n", p->seq_fields.bits.mb_adaptive_frame_field_flag);
    va_TraceMsg("\t direct_8x8_inference_flag: %d\n", p->seq_fields.bits.direct_8x8_inference_flag);
    va_TraceMsg("\t MinLumaBiPredSize8x8: %d\n", p->seq_fields.bits.MinLumaBiPredSize8x8);
    va_TraceMsg("num_slice_groups_minus1: %d\n", p->num_slice_groups_minus1);
    va_TraceMsg("slice_group_map_type: %d\n", p->slice_group_map_type);
    va_TraceMsg("slice_group_change_rate_minus1: %d\n", p->slice_group_change_rate_minus1);
    va_TraceMsg("pic_init_qp_minus26: %d\n", p->pic_init_qp_minus26);
    va_TraceMsg("pic_init_qs_minus26: %d\n", p->pic_init_qs_minus26);
    va_TraceMsg("chroma_qp_index_offset: %d\n", p->chroma_qp_index_offset);
    va_TraceMsg("second_chroma_qp_index_offset: %d\n", p->second_chroma_qp_index_offset);
    va_TraceMsg("pic_fields: %d\n", p->pic_fields.value);
    va_TraceMsg("\t entropy_coding_mode_flag: %d\n", p->pic_fields.bits.entropy_coding_mode_flag);
    va_TraceMsg("\t weighted_pred_flag: %d\n", p->pic_fields.bits.weighted_pred_flag);
    va_TraceMsg("\t weighted_bipred_idc: %d\n", p->pic_fields.bits.weighted_bipred_idc);
    va_TraceMsg("\t transform_8x8_mode_flag: %d\n", p->pic_fields.bits.transform_8x8_mode_flag);
    va_TraceMsg("\t field_pic_flag: %d\n", p->pic_fields.bits.field_pic_flag);
    va_TraceMsg("\t constrained_intra_pred_flag: %d\n", p->pic_fields.bits.constrained_intra_pred_flag);
    va_TraceMsg("frame_num: %d\n", p->frame_num);

    return;
}

static void va_TraceVASliceParameterBufferH264(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data)
{
    int i;
    VASliceParameterBufferH264* p = (VASliceParameterBufferH264*)data;

    va_TraceMsg  ("========== SLICE HEADER ============.\n");
    va_TraceMsg("slice_data_size: %d\n", p->slice_data_size);
    va_TraceMsg("slice_data_offset: %d\n", p->slice_data_offset);
    va_TraceMsg("slice_data_flag: %d\n", p->slice_data_flag);
    va_TraceMsg("slice_data_bit_offset: %d\n", p->slice_data_bit_offset);
    va_TraceMsg("first_mb_in_slice: %d\n", p->first_mb_in_slice);
    va_TraceMsg("slice_type: %d\n", p->slice_type);
    va_TraceMsg("direct_spatial_mv_pred_flag: %d\n", p->direct_spatial_mv_pred_flag);
    va_TraceMsg("num_ref_idx_l0_active_minus1: %d\n", p->num_ref_idx_l0_active_minus1);
    va_TraceMsg("num_ref_idx_l1_active_minus1: %d\n", p->num_ref_idx_l1_active_minus1);
    va_TraceMsg("cabac_init_idc: %d\n", p->cabac_init_idc);
    va_TraceMsg("slice_qp_delta: %d\n", p->slice_qp_delta);
    va_TraceMsg("disable_deblocking_filter_idc: %d\n", p->disable_deblocking_filter_idc);
    va_TraceMsg("slice_alpha_c0_offset_div2: %d\n", p->slice_alpha_c0_offset_div2);
    va_TraceMsg("slice_beta_offset_div2: %d\n", p->slice_beta_offset_div2);	

#if 1
    if (p->slice_type == 0 || p->slice_type == 1)
    {
        va_TraceMsg("RefPicList0:\n");
        for (i = 0; i < p->num_ref_idx_l0_active_minus1 + 1; i++)
        {
            //va_TraceMsg("%d-%d; ", p->RefPicList0[i].TopFieldOrderCnt, p->RefPicList0[i].BottomFieldOrderCnt);
            va_TraceMsg("%d-%d-%d-%d; ", p->RefPicList0[i].TopFieldOrderCnt, p->RefPicList0[i].BottomFieldOrderCnt, p->RefPicList0[i].picture_id, p->RefPicList0[i].frame_idx);
        }
        va_TraceMsg("\n");
        if (p->slice_type == 1)
        {
            va_TraceMsg("RefPicList1:\n");
            for (i = 0; i < p->num_ref_idx_l1_active_minus1 + 1; i++)
            {
                //va_TraceMsg("%d-%d; ", p->RefPicList1[i].TopFieldOrderCnt, p->RefPicList1[i].BottomFieldOrderCnt);
                va_TraceMsg("%d-%d-%d-%d; ", p->RefPicList1[i].TopFieldOrderCnt, p->RefPicList1[i].BottomFieldOrderCnt, p->RefPicList1[i].picture_id, p->RefPicList1[i].frame_idx);
            }
        }
        va_TraceMsg("\n");
    }
#endif

    va_TraceMsg("luma_log2_weight_denom: %d\n", p->luma_log2_weight_denom);
    va_TraceMsg("chroma_log2_weight_denom: %d\n", p->chroma_log2_weight_denom);
    va_TraceMsg("luma_weight_l0_flag: %d\n", p->luma_weight_l0_flag);
    if (p->luma_weight_l0_flag)
    {
        for (i = 0; i <=  p->num_ref_idx_l0_active_minus1; i++)
        {
            va_TraceMsg("%d ", p->luma_weight_l0[i]);
            va_TraceMsg("%d ", p->luma_offset_l0[i]);
        }
        va_TraceMsg("\n");
    }
	
		
    va_TraceMsg("chroma_weight_l0_flag: %d\n", p->chroma_weight_l0_flag);
    if (p->chroma_weight_l0_flag)
    {
        for (i = 0; i <= p->num_ref_idx_l0_active_minus1; i++)
        {
            va_TraceMsg("%d ", p->chroma_weight_l0[i][0]);
            va_TraceMsg("%d ", p->chroma_offset_l0[i][0]);
            va_TraceMsg("%d ", p->chroma_weight_l0[i][1]);
            va_TraceMsg("%d ", p->chroma_offset_l0[i][1]);
        }
        va_TraceMsg("\n");			
    }
    va_TraceMsg("luma_weight_l1_flag: %d\n", p->luma_weight_l1_flag);
    if (p->luma_weight_l1_flag)
    {
        for (i = 0; i <=  p->num_ref_idx_l1_active_minus1; i++)
        {
            va_TraceMsg("%d ", p->luma_weight_l1[i]);
            va_TraceMsg("%d ", p->luma_offset_l1[i]);
        }
        va_TraceMsg("\n");
    }
    va_TraceMsg("chroma_weight_l1_flag: %d\n", p->chroma_weight_l1_flag);
    if (p->chroma_weight_l1_flag)
    {
        for (i = 0; i <= p->num_ref_idx_l1_active_minus1; i++)
        {
            va_TraceMsg("%d ", p->chroma_weight_l1[i][0]);
            va_TraceMsg("%d ", p->chroma_offset_l1[i][0]);
            va_TraceMsg("%d ", p->chroma_weight_l1[i][1]);
            va_TraceMsg("%d ", p->chroma_offset_l1[i][1]);
        }
        va_TraceMsg("\n");			
    }	
}

static void va_TraceVAIQMatrixBufferH264(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data
)
{
    va_TraceMsg("========== IQMatrix ============.\n");
    VAIQMatrixBufferH264* p = (VAIQMatrixBufferH264* )data;
    int i, j;
    for (i = 0; i < 6; i++)
    {
        for (j = 0; j < 16; j++)
        {
            va_TraceMsg("%d\t", p->ScalingList4x4[i][j]);
            if ((j + 1) % 8 == 0)
                va_TraceMsg("\n");
        }
    }

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 64; j++)
        {
            va_TraceMsg("%d\t", p->ScalingList8x8[i][j]);
            if ((j + 1) % 8 == 0)
                va_TraceMsg("\n");
        }		
    }
}

static void va_TraceVAPictureParameterBufferVC1(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data
)
{
    VAPictureParameterBufferVC1* p = (VAPictureParameterBufferVC1*)data;

    va_TraceMsg("\tforward_reference_picture = 0x%08x\n", p->forward_reference_picture);
    va_TraceMsg("\tbackward_reference_picture = 0x%08x\n", p->backward_reference_picture);
    va_TraceMsg("\tinloop_decoded_picture = 0x%08x\n", p->inloop_decoded_picture);
    
    va_TraceMsg("\tpulldown = %d\n", p->sequence_fields.bits.pulldown);
    va_TraceMsg("\tinterlace = %d\n", p->sequence_fields.bits.interlace);
    va_TraceMsg("\ttfcntrflag = %d\n", p->sequence_fields.bits.tfcntrflag);
    va_TraceMsg("\tfinterpflag = %d\n", p->sequence_fields.bits.finterpflag);
    va_TraceMsg("\tpsf                           = %d.\n",
                p->sequence_fields.bits.psf);
    va_TraceMsg("\tmultires                      = %d.\n",
                p->sequence_fields.bits.multires);
    va_TraceMsg("\toverlap                       = %d.\n",
                p->sequence_fields.bits.overlap);
    va_TraceMsg("\tsyncmarker                    = %d.\n",
                p->sequence_fields.bits.syncmarker);
    va_TraceMsg("\trangered                      = %d.\n",
                p->sequence_fields.bits.rangered);
    va_TraceMsg("\tmax_b_frames                  = %d.\n",
                p->sequence_fields.bits.max_b_frames);
    va_TraceMsg("\tcoded_width                   = %d.\n",
                p->coded_width);
    va_TraceMsg("\tcoded_height                  = %d.\n",
                p->coded_height);
    va_TraceMsg("\tclosed_entry                  = %d.\n",
                p->entrypoint_fields.bits.closed_entry);
    va_TraceMsg("\tbroken_link                   = %d.\n",
                p->entrypoint_fields.bits.broken_link);
    va_TraceMsg("\tclosed_entry                  = %d.\n",
                p->entrypoint_fields.bits.closed_entry);
    va_TraceMsg("\tpanscan_flag                  = %d.\n",
                p->entrypoint_fields.bits.panscan_flag);
    va_TraceMsg("\tloopfilter                    = %d.\n",
                p->entrypoint_fields.bits.loopfilter);
    va_TraceMsg("\tconditional_overlap_flag      = %d.\n",
                p->conditional_overlap_flag);
    va_TraceMsg("\tfast_uvmc_flag                = %d.\n",
                p->fast_uvmc_flag);
    va_TraceMsg("\trange_mapping_luma_flag       = %d.\n",
                p->range_mapping_fields.bits.luma_flag);
    va_TraceMsg("\trange_mapping_luma            = %d.\n",
                p->range_mapping_fields.bits.luma);
    va_TraceMsg("\trange_mapping_chroma_flag     = %d.\n",
                p->range_mapping_fields.bits.chroma_flag);
    va_TraceMsg("\trange_mapping_chroma          = %d.\n",
                p->range_mapping_fields.bits.chroma);
    va_TraceMsg("\tb_picture_fraction            = %d.\n",
                p->b_picture_fraction);
    va_TraceMsg("\tcbp_table                     = %d.\n",
                p->cbp_table);
    va_TraceMsg("\tmb_mode_table                 = %d.\n",
                p->mb_mode_table);
    va_TraceMsg("\trange_reduction_frame         = %d.\n",
                p->range_reduction_frame);
    va_TraceMsg("\trounding_control              = %d.\n",
                p->rounding_control);
    va_TraceMsg("\tpost_processing               = %d.\n",
                p->post_processing);
    va_TraceMsg("\tpicture_resolution_index      = %d.\n",
                p->picture_resolution_index);
    va_TraceMsg("\tluma_scale                    = %d.\n",
                p->luma_scale);
    va_TraceMsg("\tluma_shift                    = %d.\n",
                p->luma_shift);
    va_TraceMsg("\tpicture_type                  = %d.\n",
                p->picture_fields.bits.picture_type);
    va_TraceMsg("\tframe_coding_mode             = %d.\n",
                p->picture_fields.bits.frame_coding_mode);
    va_TraceMsg("\ttop_field_first               = %d.\n",
                p->picture_fields.bits.top_field_first);
    va_TraceMsg("\tis_first_field                = %d.\n",
                p->picture_fields.bits.is_first_field);
    va_TraceMsg("\tintensity_compensation        = %d.\n",
                p->picture_fields.bits.intensity_compensation);
    va_TraceMsg("  ---------------------------------\n");
    va_TraceMsg("\tmv_type_mb                    = %d.\n",
                p->raw_coding.flags.mv_type_mb);
    va_TraceMsg("\tdirect_mb                     = %d.\n",
                p->raw_coding.flags.direct_mb);
    va_TraceMsg("\tskip_mb                       = %d.\n",
                p->raw_coding.flags.skip_mb);
    va_TraceMsg("\tfield_tx                      = %d.\n",
                p->raw_coding.flags.field_tx);
    va_TraceMsg("\tforward_mb                    = %d.\n",
                p->raw_coding.flags.forward_mb);
    va_TraceMsg("\tac_pred                       = %d.\n",
                p->raw_coding.flags.ac_pred);
    va_TraceMsg("\toverflags                     = %d.\n",
                p->raw_coding.flags.overflags);
    va_TraceMsg("  ---------------------------------\n");
    va_TraceMsg("\tbp_mv_type_mb                 = %d.\n",
                p->bitplane_present.flags.bp_mv_type_mb);
    va_TraceMsg("\tbp_direct_mb                  = %d.\n",
                p->bitplane_present.flags.bp_direct_mb);
    va_TraceMsg("\tbp_skip_mb                    = %d.\n",
                p->bitplane_present.flags.bp_skip_mb);
    va_TraceMsg("\tbp_field_tx                   = %d.\n",
                p->bitplane_present.flags.bp_field_tx);
    va_TraceMsg("\tbp_forward_mb                 = %d.\n",
                p->bitplane_present.flags.bp_forward_mb);
    va_TraceMsg("\tbp_ac_pred                    = %d.\n",
                p->bitplane_present.flags.bp_ac_pred);
    va_TraceMsg("\tbp_overflags                  = %d.\n",
                p->bitplane_present.flags.bp_overflags);
    va_TraceMsg("  ---------------------------------\n");
    va_TraceMsg("\treference_distance_flag       = %d.\n",
                p->reference_fields.bits.reference_distance_flag);
    va_TraceMsg("\treference_distance            = %d.\n",
                p->reference_fields.bits.reference_distance);
    va_TraceMsg("\tnum_reference_pictures        = %d.\n",
                p->reference_fields.bits.num_reference_pictures);
    va_TraceMsg("\treference_field_pic_indicator = %d.\n",
                p->reference_fields.bits.reference_field_pic_indicator);
    va_TraceMsg("\tmv_mode                       = %d.\n",
                p->mv_fields.bits.mv_mode);
    va_TraceMsg("\tmv_mode2                      = %d.\n",
                p->mv_fields.bits.mv_mode2);
    va_TraceMsg("\tmv_table                      = %d.\n",
                p->mv_fields.bits.mv_table);
    va_TraceMsg("\ttwo_mv_block_pattern_table    = %d.\n",
                p->mv_fields.bits.two_mv_block_pattern_table);
    va_TraceMsg("\tfour_mv_switch                = %d.\n",
                p->mv_fields.bits.four_mv_switch);
    va_TraceMsg("\tfour_mv_block_pattern_table   = %d.\n",
                p->mv_fields.bits.four_mv_block_pattern_table);
    va_TraceMsg("\textended_mv_flag              = %d.\n",
                p->mv_fields.bits.extended_mv_flag);
    va_TraceMsg("\textended_mv_range             = %d.\n",
                p->mv_fields.bits.extended_mv_range);
    va_TraceMsg("\textended_dmv_flag             = %d.\n",
                p->mv_fields.bits.extended_dmv_flag);
    va_TraceMsg("\textended_dmv_range            = %d.\n",
                p->mv_fields.bits.extended_dmv_range);
    va_TraceMsg("\tdquant                        = %d.\n",
                p->pic_quantizer_fields.bits.dquant);
    va_TraceMsg("\tquantizer                     = %d.\n",
                p->pic_quantizer_fields.bits.quantizer);
    va_TraceMsg("\thalf_qp                       = %d.\n",
                p->pic_quantizer_fields.bits.half_qp);
    va_TraceMsg("\tpic_quantizer_scale           = %d.\n",
                p->pic_quantizer_fields.bits.pic_quantizer_scale);
    va_TraceMsg("\tpic_quantizer_type            = %d.\n",
                p->pic_quantizer_fields.bits.pic_quantizer_type);
    va_TraceMsg("\tdq_frame                      = %d.\n",
                p->pic_quantizer_fields.bits.dq_frame);
    va_TraceMsg("\tdq_profile                    = %d.\n",
                p->pic_quantizer_fields.bits.dq_profile);
    va_TraceMsg("\tdq_sb_edge                    = %d.\n",
                p->pic_quantizer_fields.bits.dq_sb_edge);
    va_TraceMsg("\tdq_db_edge                    = %d.\n",
                p->pic_quantizer_fields.bits.dq_db_edge);
    va_TraceMsg("\tdq_binary_level               = %d.\n",
                p->pic_quantizer_fields.bits.dq_binary_level);
    va_TraceMsg("\talt_pic_quantizer             = %d.\n",
                p->pic_quantizer_fields.bits.alt_pic_quantizer);
    va_TraceMsg("\tvariable_sized_transform_flag = %d.\n",
                p->transform_fields.bits.variable_sized_transform_flag);
    va_TraceMsg("\tmb_level_transform_type_flag  = %d.\n",
                p->transform_fields.bits.mb_level_transform_type_flag);
    va_TraceMsg("\tframe_level_transform_type    = %d.\n",
                p->transform_fields.bits.frame_level_transform_type);
    va_TraceMsg("\ttransform_ac_codingset_idx1   = %d.\n",
                p->transform_fields.bits.transform_ac_codingset_idx1);
    va_TraceMsg("\ttransform_ac_codingset_idx2   = %d.\n",
                p->transform_fields.bits.transform_ac_codingset_idx2);
    va_TraceMsg("\tintra_transform_dc_table      = %d.\n",
                p->transform_fields.bits.intra_transform_dc_table);
}

static void va_TraceVASliceParameterBufferVC1(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void* data
)
{
    VASliceParameterBufferVC1 *p = (VASliceParameterBufferVC1*)data;

    va_TraceMsg ("========== SLICE NUMBER ==========\n");
    va_TraceMsg ("    slice_data_size               = %d\n", p->slice_data_size);
    va_TraceMsg ("    slice_data_offset             = %d\n", p->slice_data_offset);
    va_TraceMsg ("    slice_data_flag               = %d\n", p->slice_data_flag);
    va_TraceMsg ("    macroblock_offset             = %d\n", p->macroblock_offset);
    va_TraceMsg ("    slice_vertical_position       = %d\n", p->slice_vertical_position);
}

int va_TraceBeginPicture(
    VADisplay dpy,
    VAContextID context,
    VASurfaceID render_target
)
{
    int i;

    va_TraceMsg("\tcontext = 0x%08x\n", context);
    va_TraceMsg("\t\trender_targets = 0x%08x\n", render_target);

    trace_rendertarget = render_target; /* for surface data dump after vaEndPicture */

    trace_frame++;
    trace_slice = 0;
}

VAStatus vaBufferInfo (
    VADisplay dpy,
    VAContextID context,	/* in */
    VABufferID buf_id,		/* in */
    VABufferType *type,		/* out */
    unsigned int *size,		/* out */
    unsigned int *num_elements	/* out */
);

static int va_TraceMPEG2Buf(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *pbuf
)
{
    switch (type) {
    case VAPictureParameterBufferType:
        va_TraceVAPictureParameterBufferMPEG2(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAIQMatrixBufferType:
        va_TraceVAIQMatrixBufferMPEG2(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VABitPlaneBufferType:
        break;
    case VASliceGroupMapBufferType:
        break;
    case VASliceParameterBufferType:
        trace_slice++;
        va_TraceVASliceParameterBufferMPEG2(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAMacroblockParameterBufferType:
        break;
    case VAResidualDataBufferType:
        break;
    case VADeblockingParameterBufferType:
        break;
    case VAImageBufferType:
        break;
    case VAProtectedSliceDataBufferType:
        break;
    case VAEncCodedBufferType:
        break;
    case VAEncSequenceParameterBufferType:
        break;
    case VAEncPictureParameterBufferType:
        break;
    case VAEncSliceParameterBufferType:
        break;
    case VAEncH264VUIBufferType:
        break;
    case VAEncH264SEIBufferType:
        break;
    }
    
    return 0;
}


static int va_TraceMPEG4Buf(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *pbuf
)
{
    switch (type) {
    case VAPictureParameterBufferType:
        va_TraceVAPictureParameterBufferMPEG4(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAIQMatrixBufferType:
        va_TraceVAIQMatrixBufferMPEG4(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VABitPlaneBufferType:
        break;
    case VASliceGroupMapBufferType:
        break;
    case VASliceParameterBufferType:
        va_TraceVASliceParameterBufferMPEG4(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAMacroblockParameterBufferType:
        break;
    case VAResidualDataBufferType:
        break;
    case VADeblockingParameterBufferType:
        break;
    case VAImageBufferType:
        break;
    case VAProtectedSliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncCodedBufferType:
        break;
    case VAEncSequenceParameterBufferType:
        break;
    case VAEncPictureParameterBufferType:
        break;
    case VAEncSliceParameterBufferType:
        break;
    case VAEncH264VUIBufferType:
        break;
    case VAEncH264SEIBufferType:
        break;
    default:
        break;
    }
    
    
    return 0;
}


static int va_TraceH264Buf(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *pbuf
)
{
    switch (type) {
    case VAPictureParameterBufferType:
        va_TraceVAPictureParameterBufferMPEG2(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAIQMatrixBufferType:
        va_TraceVAIQMatrixBufferH264(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VABitPlaneBufferType:
        break;
    case VASliceGroupMapBufferType:
        break;
    case VASliceParameterBufferType:
        va_TraceVASliceParameterBufferH264(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAMacroblockParameterBufferType:
        break;
    case VAResidualDataBufferType:
        break;
    case VADeblockingParameterBufferType:
        break;
    case VAImageBufferType:
        break;
    case VAProtectedSliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncCodedBufferType:
        break;
    case VAEncSequenceParameterBufferType:
        break;
    case VAEncPictureParameterBufferType:
        break;
    case VAEncSliceParameterBufferType:
        break;
    case VAEncH264VUIBufferType:
        break;
    case VAEncH264SEIBufferType:
        break;
    default:
        break;
    }
    
    
    return 0;
}


static int va_TraceVC1Buf(
    VADisplay dpy,
    VAContextID context,
    VABufferID buffer,
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *pbuf
)
{
    switch (type) {
    case VAPictureParameterBufferType:
        va_TraceVAPictureParameterBufferVC1(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAIQMatrixBufferType:
        break;
    case VABitPlaneBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceGroupMapBufferType:
        break;
    case VASliceParameterBufferType:
        va_TraceVASliceParameterBufferVC1(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VASliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAMacroblockParameterBufferType:
        break;
    case VAResidualDataBufferType:
        break;
    case VADeblockingParameterBufferType:
        break;
    case VAImageBufferType:
        break;
    case VAProtectedSliceDataBufferType:
        va_TraceVABuffers(dpy, context, buffer, type, size, num_elements, pbuf);
        break;
    case VAEncCodedBufferType:
        break;
    case VAEncSequenceParameterBufferType:
        break;
    case VAEncPictureParameterBufferType:
        break;
    case VAEncSliceParameterBufferType:
        break;
    case VAEncH264VUIBufferType:
        break;
    case VAEncH264SEIBufferType:
        break;
    default:
        break;
    }
    
    return 0;
}

int va_TraceRenderPicture(
    VADisplay dpy,
    VAContextID context,
    VABufferID *buffers,
    int num_buffers
)
{
    VABufferType type;
    unsigned int size;
    unsigned int num_elements;
    int i;

    va_TraceMsg("\tcontext = 0x%08x\n", context);
    va_TraceMsg("\tnum_buffers = %d\n", num_buffers);
    for (i = 0; i < num_buffers; i++) {
        void *pbuf;

        /* get buffer type information */
        vaBufferInfo(dpy, context, buffers[i], &type, &size, &num_elements);
        
        va_TraceMsg("\t\tbuffers[%d] = 0x%08x\n", i, buffers[i]);
        va_TraceMsg("\t\t\ttype = %s\n", buffer_type_to_string(type));
        va_TraceMsg("\t\t\tsize = %d\n", size);
        va_TraceMsg("\t\t\tnum_elements = %d\n", num_elements);


        vaMapBuffer(dpy, buffers[i], &pbuf);

        switch (trace_profile) {
        case VAProfileMPEG2Simple:
        case VAProfileMPEG2Main:
            va_TraceMPEG2Buf(dpy, context, buffers[i], type, size, num_elements, pbuf);
            break;
        case VAProfileMPEG4Simple:
        case VAProfileMPEG4AdvancedSimple:
        case VAProfileMPEG4Main:
            va_TraceMPEG4Buf(dpy, context, buffers[i], type, size, num_elements, pbuf);
            break;
        case VAProfileH264Baseline:
        case VAProfileH264Main:
        case VAProfileH264High:
            va_TraceH264Buf(dpy, context, buffers[i], type, size, num_elements, pbuf);
            break;
        case VAProfileVC1Simple:
        case VAProfileVC1Main:
        case VAProfileVC1Advanced:
            va_TraceVC1Buf(dpy, context, buffers[i], type, size, num_elements, pbuf);
            break;
        case VAProfileH263Baseline:
            break;
        }

        vaUnmapBuffer(dpy, buffers[i]);
    }
}


int va_TraceEndPicture(
    VADisplay dpy,
    VAContextID context
)
{
    int i, j;
    unsigned int fourcc; /* following are output argument */
    unsigned int luma_stride;
    unsigned int chroma_u_stride;
    unsigned int chroma_v_stride;
    unsigned int luma_offset;
    unsigned int chroma_u_offset;
    unsigned int chroma_v_offset;
    unsigned int buffer_name;
    void *buffer;
    char *Y_data, *UV_data, *tmp;
    
    VAStatus va_status;
    
    va_TraceMsg("\tcontext = 0x%08x\n", context);
    va_TraceMsg("\t\trender_targets = 0x%08x\n", trace_rendertarget);
    
    /* force the pipleline finish rendering */
    vaSyncSurface(dpy, trace_rendertarget);

    va_TraceMsg("***dump surface data***\n", trace_rendertarget);

    va_status = vaLockSurface(dpy, trace_rendertarget, &fourcc, &luma_stride, &chroma_u_stride, &chroma_v_stride,
                                      &luma_offset, &chroma_u_offset, &chroma_v_offset, &buffer_name, &buffer);

    if (va_status != VA_STATUS_SUCCESS)
        return va_status;
    
    va_TraceMsg("\tfourcc=0x%08x\n", fourcc);
    va_TraceMsg("\twidth=%d\n", trace_width);
    va_TraceMsg("\theight=%d\n", trace_height);
    va_TraceMsg("\tluma_stride=%d\n", luma_stride);
    va_TraceMsg("\tchroma_u_stride=%d\n", chroma_u_stride);
    va_TraceMsg("\tchroma_v_stride=%d\n", chroma_v_stride);
    va_TraceMsg("\tluma_offset=%d\n", luma_offset);
    va_TraceMsg("\tchroma_u_offset=%d\n", chroma_u_offset);
    va_TraceMsg("\tchroma_v_offset=%d\n", chroma_v_offset);

    if (!buffer)
        return;

    Y_data = buffer;
    UV_data = buffer + luma_offset;

    tmp = Y_data;
    va_TraceMsg("**Y data**\n");
    for (i=0; i<trace_height; i++) {
        for (j=0; j<trace_width; j++) {
            if ((j%16) == 0)
                va_TraceMsg("\n0x%08x:", j + i*trace_width);
            va_TraceMsg(" %02x", tmp[j]);
        }

        va_TraceMsg("\n");
        tmp = Y_data + i * luma_stride;
    }

    tmp = UV_data;
    if (fourcc == VA_FOURCC_NV12) {
        va_TraceMsg("**UV data**\n");
        
        for (i=0; i<trace_height/2; i++) {
            for (j=0; j<trace_width; j++) {
                if ((j%16) == 0)
                    va_TraceMsg("\n0x%08x:", j + i*trace_width);
                va_TraceMsg(" %02x", tmp[j]);
            }

            va_TraceMsg("\n");
            tmp = UV_data + i * chroma_u_stride;
        }
    }
}
