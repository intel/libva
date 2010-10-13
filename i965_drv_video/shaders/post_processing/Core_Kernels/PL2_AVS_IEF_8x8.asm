/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

//---------- PL2_AVS_IEF_8x8.asm ----------

#include "AVS_IEF.inc"

//------------------------------------------------------------------------------
// 2 sampler reads for 8x8 Y each
// 1 sampler read for 8x8 U and 8x8 V (NV11\NV12 input surface)
//------------------------------------------------------------------------------

    // 1st 8x8 setup
    #include "AVS_SetupFirstBlock.asm"

    // Enable green channel only
    mov (1) rAVS_8x8_HDR.2:ud      nAVS_GREEN_CHANNEL_ONLY:ud               

    mov (16) mAVS_8x8_HDR.0:ud      rAVS_8x8_HDR.0<8;8,1>:ud    // Copy msg header and payload mirrors to MRFs
    send (1) uwAVS_RESPONSE(0)<1>   mAVS_8x8_HDR   udDUMMY_NULL      nSMPL_ENGINE        nAVS_MSG_DSC_1CH+nSI_SRC_Y+nBI_CURRENT_SRC_Y
    // Return Y in 4 GRFs

    // 8x8 U and V sampling 
    // Enable red and blue channels    
    mov (1) rAVS_8x8_HDR.2:ud  nAVS_RED_BLUE_CHANNELS:ud                   

    // Calculate Chroma Step Size:
    // for H direction: 16 Luma samples are covered by 8 Chroma samples. Thus Chroma_Step_X = 2 * Luma_Step_X 
    // for V direction: 8  Luma samples are covered by 8 Chroma samples. Thus Chroma_Step_Y = Luma_Step_Y
    mul  (1)  rAVS_PAYLOAD.1:f      fVIDEO_STEP_X:f    2.0:f             // Step X for chroma

    mov (16) mAVS_8x8_HDR_UV.0:ud      rAVS_8x8_HDR.0<8;8,1>:ud    // Copy msg header and payload mirrors to MRFs
    send (1) uwAVS_RESPONSE(4)<1> mAVS_8x8_HDR_UV   udDUMMY_NULL  nSMPL_ENGINE    nAVS_MSG_DSC_2CH+nSI_SRC_UV+nBI_CURRENT_SRC_UV
    // Return U and V in 8 GRFs

    // 2nd 8x8 setup
    #include "AVS_SetupSecondBlock.asm"

    // 2nd 8x8 Y sampling
    // Enable green channel only
    mov (1) rAVS_8x8_HDR.2:ud      nAVS_GREEN_CHANNEL_ONLY:ud                           

    mov (16) mAVS_8x8_HDR.0:ud      rAVS_8x8_HDR.0<8;8,1>:ud    // Copy msg header and payload mirrors to MRFs
    send (1) uwAVS_RESPONSE_2(0)<1>    mAVS_8x8_HDR    udDUMMY_NULL    nSMPL_ENGINE    nAVS_MSG_DSC_1CH+nSI_SRC_Y+nBI_CURRENT_SRC_Y 

//------------------------------------------------------------------------------
// Unpacking sampler reads to 4:2:2 internal planar 
//------------------------------------------------------------------------------
    #include "PL2_AVS_IEF_Unpack_8x8.asm"

