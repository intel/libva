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
 *    Zhou Chang <chang.zhou@intel.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "assert.h"
#include "intel_batchbuffer.h"
#include "i965_defines.h"
#include "i965_structs.h"
#include "i965_drv_video.h"

static void
gen6_mfc_pipe_mode_select(VADriverContextP ctx)
{
    BEGIN_BCS_BATCH(ctx,4);

    OUT_BCS_BATCH(ctx, MFX_PIPE_MODE_SELECT | (4 - 2));
    OUT_BCS_BATCH(ctx,
                  (0 << 10) | /* disable Stream-Out */
                  (1 << 9)  | /* Post Deblocking Output */
                  (0 << 8)  | /* Pre Deblocking Output */
                  (0 << 7)  | /* disable TLB prefectch */
                  (0 << 5)  | /* not in stitch mode */
                  (1 << 4)  | /* encoding mode */
                  (2 << 0));  /* Standard Select: AVC */
    OUT_BCS_BATCH(ctx,
                  (0 << 20) | /* round flag in PB slice */
                  (0 << 19) | /* round flag in Intra8x8 */
                  (0 << 7)  | /* expand NOA bus flag */
                  (1 << 6)  | /* must be 1 */
                  (0 << 5)  | /* disable clock gating for NOA */
                  (0 << 4)  | /* terminate if AVC motion and POC table error occurs */
                  (0 << 3)  | /* terminate if AVC mbdata error occurs */
                  (0 << 2)  | /* terminate if AVC CABAC/CAVLC decode error occurs */
                  (0 << 1)  | /* AVC long field motion vector */
                  (0 << 0));  /* always calculate AVC ILDB boundary strength */
    OUT_BCS_BATCH(ctx, 0);

    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfc_surface_state(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen6_mfc_bcs_state *bcs_state = &i965->gen6_mfc_bcs_state;

    BEGIN_BCS_BATCH(ctx, 6);

    OUT_BCS_BATCH(ctx, MFX_SURFACE_STATE | (6 - 2));
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx,
                  ((bcs_state->surface_state.height - 1) << 19) |
                  ((bcs_state->surface_state.width - 1) << 6));
    OUT_BCS_BATCH(ctx,
                  (MFX_SURFACE_PLANAR_420_8 << 28) | /* 420 planar YUV surface */
                  (1 << 27) | /* must be 1 for interleave U/V, hardware requirement */
                  (0 << 22) | /* surface object control state, FIXME??? */
                  ((bcs_state->surface_state.w_pitch - 1) << 3) | /* pitch */
                  (0 << 2)  | /* must be 0 for interleave U/V */
                  (1 << 1)  | /* must be y-tiled */
                  (I965_TILEWALK_YMAJOR << 0));  			/* tile walk, TILEWALK_YMAJOR */
    OUT_BCS_BATCH(ctx,
                  (0 << 16) | 								/* must be 0 for interleave U/V */
                  (bcs_state->surface_state.h_pitch)); 		/* y offset for U(cb) */
    OUT_BCS_BATCH(ctx, 0);
    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfc_pipe_buf_addr_state(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen6_mfc_bcs_state *bcs_state = &i965->gen6_mfc_bcs_state;
    int i;

    BEGIN_BCS_BATCH(ctx, 24);

    OUT_BCS_BATCH(ctx, MFX_PIPE_BUF_ADDR_STATE | (24 - 2));

    OUT_BCS_BATCH(ctx, 0);											/* pre output addr   */

    OUT_BCS_RELOC(ctx, bcs_state->post_deblocking_output.bo,
                  I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                  0);											/* post output addr  */	

    OUT_BCS_RELOC(ctx, bcs_state->uncompressed_picture_source.bo,
                  I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                  0);											/* uncompressed data */

    OUT_BCS_BATCH(ctx, 0);											/* StreamOut data*/
    OUT_BCS_RELOC(ctx, bcs_state->intra_row_store_scratch_buffer.bo,
                  I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                  0);	
    OUT_BCS_RELOC(ctx, bcs_state->deblocking_filter_row_store_scratch_buffer.bo,
                  I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                  0);
    /* 7..22 Reference pictures*/
    for (i = 0; i < ARRAY_ELEMS(bcs_state->reference_surfaces); i++) {
		if ( bcs_state->reference_surfaces[i].bo != NULL) {
			OUT_BCS_RELOC(ctx, bcs_state->reference_surfaces[i].bo,
					I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
					0);			
		} else {
			OUT_BCS_BATCH(ctx, 0);
		}
    }
    OUT_BCS_BATCH(ctx, 0);   											/* no block status  */

    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfc_ind_obj_base_addr_state(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen6_media_state *media_state = &i965->gen6_media_state;

    BEGIN_BCS_BATCH(ctx, 11);

    OUT_BCS_BATCH(ctx, MFX_IND_OBJ_BASE_ADDR_STATE | (11 - 2));
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    /* MFX Indirect MV Object Base Address */
    OUT_BCS_RELOC(ctx, media_state->vme_output.bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
    OUT_BCS_BATCH(ctx, 0);	
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    /*MFC Indirect PAK-BSE Object Base Address for Encoder*/	
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);

    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfc_bsp_buf_base_addr_state(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen6_mfc_bcs_state *bcs_state = &i965->gen6_mfc_bcs_state;

    BEGIN_BCS_BATCH(ctx, 4);

    OUT_BCS_BATCH(ctx, MFX_BSP_BUF_BASE_ADDR_STATE | (4 - 2));
    OUT_BCS_RELOC(ctx, bcs_state->bsd_mpc_row_store_scratch_buffer.bo,
                  I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                  0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);

    ADVANCE_BCS_BATCH(ctx);
}

static void
gen6_mfc_avc_img_state(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen6_mfc_bcs_state *bcs_state = &i965->gen6_mfc_bcs_state;

    int width_in_mbs = (bcs_state->surface_state.width + 15) / 16;
    int height_in_mbs = (bcs_state->surface_state.height + 15) / 16;

    BEGIN_BCS_BATCH(ctx, 13);
    OUT_BCS_BATCH(ctx, MFX_AVC_IMG_STATE | (13 - 2));
    OUT_BCS_BATCH(ctx, 
                  ((width_in_mbs * height_in_mbs) & 0xFFFF));
    OUT_BCS_BATCH(ctx, 
                  (height_in_mbs << 16) | 
                  (width_in_mbs << 0));
    OUT_BCS_BATCH(ctx, 
                  (0 << 24) |	  /*Second Chroma QP Offset*/
                  (0 << 16) |	  /*Chroma QP Offset*/
                  (0 << 14) |   /*Max-bit conformance Intra flag*/
                  (0 << 13) |   /*Max Macroblock size conformance Inter flag*/
                  (1 << 12) |   /*Should always be written as "1" */
                  (0 << 10) |   /*QM Preset FLag */
                  (0 << 8)  |   /*Image Structure*/
                  (0 << 0) );   /*Current Decoed Image Frame Store ID, reserved in Encode mode*/
    OUT_BCS_BATCH(ctx,
                  (0 << 16) |   /*Mininum Frame size*/	
                  (0 << 15) |	  /*Disable reading of Macroblock Status Buffer*/
                  (0 << 14) |   /*Load BitStream Pointer only once, 1 slic 1 frame*/
                  (0 << 13) |   /*CABAC 0 word insertion test enable*/
                  (1 << 12) |   /*MVUnpackedEnable,compliant to DXVA*/
                  (1 << 10) |   /*Chroma Format IDC, 4:2:0*/
                  (1 << 7)  |   /*0:CAVLC encoding mode,1:CABAC*/
                  (0 << 6)  |   /*Only valid for VLD decoding mode*/
                  (0 << 5)  |   /*Constrained Intra Predition Flag, from PPS*/
                  (0 << 4)  |   /*Direct 8x8 inference flag*/
                  (0 << 3)  |   /*Only 8x8 IDCT Transform Mode Flag*/
                  (1 << 2)  |   /*Frame MB only flag*/
                  (0 << 1)  |   /*MBAFF mode is in active*/
                  (0 << 0) );   /*Field picture flag*/
    OUT_BCS_BATCH(ctx, 0);		/*Mainly about MB rate control and debug, just ignoring*/
    OUT_BCS_BATCH(ctx, 			/*Inter and Intra Conformance Max size limit*/
                  (0xBB8 << 16) |		/*InterMbMaxSz*/
                  (0xEE8) );			/*IntraMbMaxSz*/
    OUT_BCS_BATCH(ctx, 0);		/*Reserved*/
    OUT_BCS_BATCH(ctx, 0);		/*Slice QP Delta for bitrate control*/
    OUT_BCS_BATCH(ctx, 0);		/*Slice QP Delta for bitrate control*/	
    OUT_BCS_BATCH(ctx, 0x8C000000);
	OUT_BCS_BATCH(ctx, 0x00010000);
    OUT_BCS_BATCH(ctx, 0);

    ADVANCE_BCS_BATCH(ctx);
}


static void gen6_mfc_avc_directmode_state(VADriverContextP ctx)
{
    int i;

    BEGIN_BCS_BATCH(ctx, 69);

    OUT_BCS_BATCH(ctx, MFX_AVC_DIRECTMODE_STATE | (69 - 2));
    //TODO: reference DMV
    for(i = 0; i < 16; i++){
        OUT_BCS_BATCH(ctx, 0);
        OUT_BCS_BATCH(ctx, 0);
    }

    //TODO: current DMV just for test
#if 0
    OUT_BCS_RELOC(ctx, bcs_state->direct_mv_buffers[0].bo,
                  I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                  0);
#else
    //drm_intel_bo_pin(bcs_state->direct_mv_buffers[0].bo, 0x1000);
    //OUT_BCS_BATCH(ctx, bcs_state->direct_mv_buffers[0].bo->offset);
    OUT_BCS_BATCH(ctx, 0);
#endif


    OUT_BCS_BATCH(ctx, 0);

    //TODO: POL list
    for(i = 0; i < 34; i++) {
        OUT_BCS_BATCH(ctx, 0);
    }

    ADVANCE_BCS_BATCH(ctx);
}

static void gen6_mfc_avc_slice_state(VADriverContextP ctx, int intra_slice)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen6_mfc_bcs_state *bcs_state = &i965->gen6_mfc_bcs_state;

    BEGIN_BCS_BATCH(ctx, 11);;

    OUT_BCS_BATCH(ctx, MFX_AVC_SLICE_STATE | (11 - 2) );

    if ( intra_slice )
        OUT_BCS_BATCH(ctx, 2);			/*Slice Type: I Slice*/
    else
        OUT_BCS_BATCH(ctx, 0);			/*Slice Type: P Slice*/

    if ( intra_slice )
        OUT_BCS_BATCH(ctx, 0);			/*no reference frames and pred_weight_table*/
    else 
        OUT_BCS_BATCH(ctx, 0x00010000); 	/*1 reference frame*/

    OUT_BCS_BATCH(ctx, (0<<24) |                /*Enable deblocking operation*/
                  (26<<16) | 			/*Slice Quantization Parameter*/
                  0x0202 );
    OUT_BCS_BATCH(ctx, 0);			/*First MB X&Y , the postion of current slice*/
    OUT_BCS_BATCH(ctx, ( ((bcs_state->surface_state.height+15)/16) << 16) );

    OUT_BCS_BATCH(ctx, 
                  (0<<31) |		/*RateControlCounterEnable = disable*/
                  (1<<30) |		/*ResetRateControlCounter*/
                  (2<<28) |		/*RC Triggle Mode = Loose Rate Control*/
                  (1<<19) | 	        /*IsLastSlice*/
                  (0<<18) | 	        /*BitstreamOutputFlag Compressed BitStream Output Disable Flag 0:enable 1:disable*/
                  (0<<17) |	        /*HeaderPresentFlag*/	
                  (1<<16) |	        /*SliceData PresentFlag*/
                  (0<<15) |	        /*TailPresentFlag*/
                  (1<<13) |	        /*RBSP NAL TYPE*/	
                  (0<<12) );	        /*CabacZeroWordInsertionEnable*/
	
    OUT_BCS_RELOC(ctx, bcs_state->mfc_indirect_pak_bse_object.bo,
                  I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                  bcs_state->mfc_indirect_pak_bse_object.offset);

    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);

    ADVANCE_BCS_BATCH(ctx);
}
static void gen6_mfc_avc_qm_state(VADriverContextP ctx)
{
	int i;

    BEGIN_BCS_BATCH(ctx, 58);

	OUT_BCS_BATCH(ctx, MFX_AVC_QM_STATE | 56);
	OUT_BCS_BATCH(ctx, 0xFF ) ; 
	for( i = 0; i < 56; i++) {
		OUT_BCS_BATCH(ctx, 0x10101010); 
	}   

	ADVANCE_BCS_BATCH(ctx);
}

static void gen6_mfc_avc_fqm_state(VADriverContextP ctx)
{
	int i;

	BEGIN_BCS_BATCH(ctx, 113);
	OUT_BCS_BATCH(ctx, MFC_AVC_FQM_STATE | (113 - 2));

	for(i = 0; i < 112;i++) {
			OUT_BCS_BATCH(ctx, 0x10001000);
	}   

	ADVANCE_BCS_BATCH(ctx);	
}

static void gen6_mfc_avc_ref_idx_state(VADriverContextP ctx)
{
	int i;

	BEGIN_BCS_BATCH(ctx, 10);

	OUT_BCS_BATCH(ctx, MFX_AVC_REF_IDX_STATE | 8);
	OUT_BCS_BATCH(ctx, 0);                  //Select L0

	OUT_BCS_BATCH(ctx, 0x80808000);         //Only 1 reference
	for(i = 0; i < 7; i++) {
		OUT_BCS_BATCH(ctx, 0x80808080);
	}

	ADVANCE_BCS_BATCH(ctx);
}
	
	
static void
gen6_mfc_avc_insert_object(VADriverContextP ctx, int flush_data)
{
    BEGIN_BCS_BATCH(ctx, 4);

    OUT_BCS_BATCH(ctx, MFC_AVC_INSERT_OBJECT | (4 -2 ) );
    OUT_BCS_BATCH(ctx, (32<<8) | 
                  (1 << 3) |
                  (1 << 2) |
                  (flush_data << 1) |
                  (1<<0) );
    OUT_BCS_BATCH(ctx, 0x00000003);
    OUT_BCS_BATCH(ctx, 0xABCD1234);

    ADVANCE_BCS_BATCH(ctx);
}

static int
gen6_mfc_avc_pak_object_intra(VADriverContextP ctx, int x, int y, int end_mb, int qp,unsigned int *msg)
{
    int len_in_dwords = 11;

    BEGIN_BCS_BATCH(ctx, len_in_dwords);

    OUT_BCS_BATCH(ctx, MFC_AVC_PAK_OBJECT | (len_in_dwords - 2));
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 0);
    OUT_BCS_BATCH(ctx, 
                  (0 << 24) |		/* PackedMvNum, Debug*/
                  (0 << 20) | 		/* No motion vector */
                  (1 << 19) |		/* CbpDcY */
                  (1 << 18) |		/* CbpDcU */
                  (1 << 17) |		/* CbpDcV */
                  (msg[0] & 0xFFFF) );

    OUT_BCS_BATCH(ctx, (0xFFFF<<16) | (y << 8) | x);		/* Code Block Pattern for Y*/
    OUT_BCS_BATCH(ctx, 0x000F000F);							/* Code Block Pattern */		
    OUT_BCS_BATCH(ctx, (0 << 27) | (end_mb << 26) | qp);	/* Last MB */

    /*Stuff for Intra MB*/
    OUT_BCS_BATCH(ctx, msg[1]);			/* We using Intra16x16 no 4x4 predmode*/	
    OUT_BCS_BATCH(ctx, msg[2]);	
    OUT_BCS_BATCH(ctx, msg[3]&0xFC);		

    OUT_BCS_BATCH(ctx, 0x8040000);	/*MaxSizeInWord and TargetSzieInWord*/

    ADVANCE_BCS_BATCH(ctx);

	return len_in_dwords;
}

static int gen6_mfc_avc_pak_object_inter(VADriverContextP ctx, int x, int y, int end_mb, int qp, unsigned int offset)
{
	 int len_in_dwords = 11;

    BEGIN_BCS_BATCH(ctx, len_in_dwords);

    OUT_BCS_BATCH(ctx, MFC_AVC_PAK_OBJECT | (len_in_dwords - 2));

	OUT_BCS_BATCH(ctx, 32);         /* 32 MV*/
	OUT_BCS_BATCH(ctx, offset);

	OUT_BCS_BATCH(ctx, 
			(1 << 24) |     /* PackedMvNum, Debug*/
			(4 << 20) |     /* 8 MV, SNB don't use it*/
			(1 << 19) |     /* CbpDcY */
			(1 << 18) |     /* CbpDcU */
			(1 << 17) |     /* CbpDcV */
			(0 << 15) |     /* Transform8x8Flag = 0*/
			(0 << 14) |     /* Frame based*/
			(0 << 13) |     /* Inter MB */
			(1 << 8)  |     /* MbType = P_L0_16x16 */   
			(0 << 7)  |     /* MBZ for frame */
			(0 << 6)  |     /* MBZ */
			(2 << 4)  |     /* MBZ for inter*/
			(0 << 3)  |     /* MBZ */
			(0 << 2)  |     /* SkipMbFlag */
			(0 << 0));      /* InterMbMode */

	OUT_BCS_BATCH(ctx, (0xFFFF<<16) | (y << 8) | x);        /* Code Block Pattern for Y*/
	OUT_BCS_BATCH(ctx, 0x000F000F);                         /* Code Block Pattern */    
	OUT_BCS_BATCH(ctx, (0 << 27) | (end_mb << 26) | qp);    /* Last MB */

	/*Stuff for Inter MB*/
	OUT_BCS_BATCH(ctx, 0x0);        
	OUT_BCS_BATCH(ctx, 0x0);    
	OUT_BCS_BATCH(ctx, 0x0);        

	OUT_BCS_BATCH(ctx, 0xF0020000); /*MaxSizeInWord and TargetSzieInWord*/

	ADVANCE_BCS_BATCH(ctx);

	return len_in_dwords;
}

static void gen6_mfc_init(VADriverContextP ctx)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen6_mfc_bcs_state *bcs_state = &i965->gen6_mfc_bcs_state;
    dri_bo *bo;
    int i;

    /*Encode common setup for MFC*/
    dri_bo_unreference(bcs_state->post_deblocking_output.bo);
    bcs_state->post_deblocking_output.bo = NULL;

    dri_bo_unreference(bcs_state->pre_deblocking_output.bo);
    bcs_state->pre_deblocking_output.bo = NULL;

    dri_bo_unreference(bcs_state->uncompressed_picture_source.bo);
    bcs_state->uncompressed_picture_source.bo = NULL;

    dri_bo_unreference(bcs_state->mfc_indirect_pak_bse_object.bo); 
    bcs_state->mfc_indirect_pak_bse_object.bo = NULL;

    for (i = 0; i < NUM_MFC_DMV_BUFFERS; i++){
        dri_bo_unreference(bcs_state->direct_mv_buffers[i].bo);
        bcs_state->direct_mv_buffers[i].bo = NULL;
    }

    for (i = 0; i < MAX_MFC_REFERENCE_SURFACES; i++){
       if ( bcs_state->reference_surfaces[i].bo != NULL)
			dri_bo_unreference( bcs_state->reference_surfaces[i].bo );
        bcs_state->reference_surfaces[i].bo = NULL;  
    }

    dri_bo_unreference(bcs_state->intra_row_store_scratch_buffer.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "Buffer",
                      128 * 64,
                      64);
    assert(bo);
    bcs_state->intra_row_store_scratch_buffer.bo = bo;

    dri_bo_unreference(bcs_state->deblocking_filter_row_store_scratch_buffer.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "Buffer",
                      49152,  /* 6 * 128 * 64 */
                      64);
    assert(bo);
    bcs_state->deblocking_filter_row_store_scratch_buffer.bo = bo;

    dri_bo_unreference(bcs_state->bsd_mpc_row_store_scratch_buffer.bo);
    bo = dri_bo_alloc(i965->intel.bufmgr,
                      "Buffer",
                      12288, /* 1.5 * 128 * 64 */
                      0x1000);
    assert(bo);
    bcs_state->bsd_mpc_row_store_scratch_buffer.bo = bo;
}

void gen6_mfc_avc_pipeline_programing(VADriverContextP ctx, void *obj)
{
    struct mfc_encode_state *encode_state = obj;
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen6_mfc_bcs_state *bcs_state = &i965->gen6_mfc_bcs_state;
    int width_in_mbs = (bcs_state->surface_state.width + 15) / 16;
    int height_in_mbs = (bcs_state->surface_state.height + 15) / 16;
    int x,y;
    struct gen6_media_state *media_state = &i965->gen6_media_state;
    VAEncSequenceParameterBufferH264 *pSequenceParameter = (VAEncSequenceParameterBufferH264 *)encode_state->seq_param->buffer;
    VAEncSliceParameterBuffer *pSliceParameter = (VAEncSliceParameterBuffer *)encode_state->slice_params[0]->buffer;
    unsigned int *msg = NULL, offset = 0;
    int emit_new_state = 1, object_len_in_bytes;
    int is_intra = pSliceParameter->slice_flags.bits.is_intra;

    intel_batchbuffer_start_atomic_bcs(ctx, 0x1000); 

    if (is_intra) {
        dri_bo_map(media_state->vme_output.bo , 1);
        msg = (unsigned int *)media_state->vme_output.bo->virtual;
    }

    for (y = 0; y < height_in_mbs; y++) {
        for (x = 0; x < width_in_mbs; x++) { 
            int last_mb = (y == (height_in_mbs-1)) && ( x == (width_in_mbs-1) );
            int qp = pSequenceParameter->initial_qp;

            if (emit_new_state) {
                intel_batchbuffer_emit_mi_flush_bcs(ctx);
                gen6_mfc_pipe_mode_select(ctx);
                gen6_mfc_surface_state(ctx);
                gen6_mfc_pipe_buf_addr_state(ctx);
                gen6_mfc_ind_obj_base_addr_state(ctx);
                gen6_mfc_bsp_buf_base_addr_state(ctx);
                gen6_mfc_avc_img_state(ctx);
                gen6_mfc_avc_qm_state(ctx);
                gen6_mfc_avc_fqm_state(ctx);
                gen6_mfc_avc_ref_idx_state(ctx);
                /*gen6_mfc_avc_directmode_state(ctx);*/
                gen6_mfc_avc_slice_state(ctx, is_intra);
                /*gen6_mfc_avc_insert_object(ctx, 0);*/
                emit_new_state = 0;
            }
			
            if (is_intra) {
                assert(msg);
                object_len_in_bytes = gen6_mfc_avc_pak_object_intra(ctx, x, y, last_mb, qp, msg);
                msg += 4;
            } else {
                object_len_in_bytes = gen6_mfc_avc_pak_object_inter(ctx, x, y, last_mb, qp, offset);
                offset += 64;
            }

            if (intel_batchbuffer_check_free_space_bcs(ctx, object_len_in_bytes) == 0) {
                intel_batchbuffer_end_atomic_bcs(ctx);
                intel_batchbuffer_flush_bcs(ctx);
                emit_new_state = 1;
                intel_batchbuffer_start_atomic_bcs(ctx, 0x1000);
            }
        }
    }

    if (is_intra)
        dri_bo_unmap(media_state->vme_output.bo);
	
    intel_batchbuffer_end_atomic_bcs(ctx);
}

static VAStatus gen6_mfc_avc_prepare(VADriverContextP ctx, 
                                         VAContextID context, 
                                         struct mfc_encode_state *encode_state)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen6_mfc_bcs_state *bcs_state = &i965->gen6_mfc_bcs_state;
    struct object_surface *obj_surface;	
    struct object_buffer *obj_buffer;
    dri_bo *bo;
    VAEncPictureParameterBufferH264 *pPicParameter = (VAEncPictureParameterBufferH264 *)encode_state->pic_param->buffer;

    /*Setup all the input&output object*/
    obj_surface = SURFACE(pPicParameter->reconstructed_picture);
    assert(obj_surface);

    if (!obj_surface->bo) {
        uint32_t tiling_mode = I915_TILING_Y;
        unsigned long pitch;

        obj_surface->bo = drm_intel_bo_alloc_tiled(i965->intel.bufmgr, 
                                                   "vaapi surface",
                                                   obj_surface->width, 
                                                   obj_surface->height + obj_surface->height / 2,
                                                   1,
                                                   &tiling_mode,
                                                   &pitch,
                                                   0);
        assert(obj_surface->bo);
        assert(tiling_mode == I915_TILING_Y);
        assert(pitch == obj_surface->width);
    }

    bcs_state->post_deblocking_output.bo = obj_surface->bo;
    dri_bo_reference(bcs_state->post_deblocking_output.bo);

    bcs_state->surface_state.width = obj_surface->orig_width;
    bcs_state->surface_state.height = obj_surface->orig_height;
    bcs_state->surface_state.w_pitch = obj_surface->width;
    bcs_state->surface_state.h_pitch = obj_surface->height;

    obj_surface = SURFACE(pPicParameter->reference_picture);
	assert(obj_surface);
	if ( obj_surface->bo != NULL) {
		bcs_state->reference_surfaces[0].bo = obj_surface->bo;
		dri_bo_reference(obj_surface->bo);
	}
	
    obj_surface = SURFACE(encode_state->current_render_target);
    assert(obj_surface && obj_surface->bo);
    bcs_state->uncompressed_picture_source.bo = obj_surface->bo;
    dri_bo_reference(bcs_state->uncompressed_picture_source.bo);

    obj_buffer = BUFFER (pPicParameter->coded_buf); /* FIXME: fix this later */
    bo = obj_buffer->buffer_store->bo;
    assert(bo);
    bcs_state->mfc_indirect_pak_bse_object.bo = bo;
    bcs_state->mfc_indirect_pak_bse_object.offset = ALIGN(sizeof(VACodedBufferSegment), 64);
    dri_bo_reference(bcs_state->mfc_indirect_pak_bse_object.bo);

    /*Programing bcs pipeline*/
    gen6_mfc_avc_pipeline_programing(ctx, encode_state);	//filling the pipeline
	
    return vaStatus;
}

static VAStatus gen6_mfc_run(VADriverContextP ctx, 
                                     VAContextID context,                              
                                     struct mfc_encode_state *encode_state)
{
    intel_batchbuffer_flush_bcs(ctx);		//run the pipeline

    return VA_STATUS_SUCCESS;
}

static VAStatus gen6_mfc_stop(VADriverContextP ctx, 
                                      VAContextID context,                              
                                      struct mfc_encode_state *encode_state)
{
#if 0
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen6_media_state *media_state = &i965->gen6_media_state;
	
    VAEncPictureParameterBufferH264 *pPicParameter = (VAEncPictureParameterBufferH264 *)encode_state->pic_param->buffer;
	
    struct object_surface *obj_surface = SURFACE(pPicParameter->reconstructed_picture);
    //struct object_surface *obj_surface = SURFACE(pPicParameter->reference_picture[0]);
    //struct object_surface *obj_surface = SURFACE(encode_state->current_render_target);
    my_debug(obj_surface);

#endif

    return VA_STATUS_SUCCESS;
}

static VAStatus 
gen6_mfc_avc_encode_picture(VADriverContextP ctx, 
                                     VAContextID context,                              
                                     struct mfc_encode_state *encode_state)
{
    gen6_mfc_init(ctx);					
    gen6_mfc_avc_prepare(ctx, context, encode_state);
    gen6_mfc_run(ctx, context, encode_state);
    gen6_mfc_stop(ctx, context, encode_state);

    return VA_STATUS_SUCCESS;
}

VAStatus 
gen6_mfc_pipeline(VADriverContextP ctx, 
                           VAContextID context,                              
                           struct mfc_encode_state *encode_state)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx); 
    struct object_context *obj_context = CONTEXT(context);
    struct object_config *obj_config;
    VAContextID config;
    VAStatus vaStatus;

    assert(obj_context);
    config = obj_context->config_id;
    obj_config = CONFIG(config);
    assert(obj_config);

    switch( obj_config->profile) {
    case VAProfileH264Baseline:
        vaStatus = gen6_mfc_avc_encode_picture(ctx, context, &(obj_context->encode_state)); 
        break;

        /* FIXME: add for other profile */
    default:
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
        break;
    }

    return VA_STATUS_SUCCESS;
}

