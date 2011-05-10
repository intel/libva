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

#ifndef _GEN6_MFC_H_
#define _GEN6_MFC_H_

#include <xf86drm.h>
#include <drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>

struct encode_state;

#define MAX_MFC_REFERENCE_SURFACES        16
#define NUM_MFC_DMV_BUFFERS				  34

struct gen6_mfc_context
{
    struct {
        unsigned int width;
        unsigned int height;
        unsigned int w_pitch;
        unsigned int h_pitch;
    } surface_state;


    //MFX_PIPE_BUF_ADDR_STATE
    struct {
        dri_bo *bo;                            	
    } post_deblocking_output;                   		//OUTPUT: reconstructed picture                           
    
    struct {  
        dri_bo *bo;							   	
    } pre_deblocking_output;                    		//OUTPUT: reconstructed picture with deblocked                           

    struct {
        dri_bo *bo;
    } uncompressed_picture_source;						//INPUT: original compressed image

    struct {
        dri_bo *bo;							  	
    } intra_row_store_scratch_buffer;					//INTERNAL:

    struct {
        dri_bo *bo;								
    } deblocking_filter_row_store_scratch_buffer;		//INTERNAL:

    struct {                                    
       dri_bo *bo; 
    } reference_surfaces[MAX_MFC_REFERENCE_SURFACES];	//INTERNAL: refrence surfaces

    //MFX_IND_OBJ_BASE_ADDR_STATE
    struct{
        dri_bo *bo;
    } mfc_indirect_mv_object;							//INPUT: the blocks' mv info

    struct {
        dri_bo *bo;
        int offset;
    } mfc_indirect_pak_bse_object;						//OUTPUT: the compressed bitstream 

    //MFX_BSP_BUF_BASE_ADDR_STATE
    struct {
        dri_bo *bo;
    }bsd_mpc_row_store_scratch_buffer;					//INTERNAL:
	
    //MFX_AVC_DIRECTMODE_STATE
    struct {
        dri_bo *bo;
    }direct_mv_buffers[NUM_MFC_DMV_BUFFERS];				//INTERNAL:	0-31 as input,32 and 33 as output
};

VAStatus 
gen6_mfc_pipeline(VADriverContextP ctx,
                  VAProfile profile,
                  struct encode_state *encode_state,
                  struct gen6_encoder_context *gen6_encoder_context);
Bool gen6_mfc_context_init(VADriverContextP ctx, struct gen6_mfc_context *mfc_context);
Bool gen6_mfc_context_destroy(struct gen6_mfc_context *mfc_context);

#endif	/* _GEN6_MFC_BCS_H_ */
