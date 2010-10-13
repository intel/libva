/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

//---------- PL3_AVS_IEF_8x8.asm ----------

#include "AVS_IEF.inc"

//------------------------------------------------------------------------------
// 2 sampler reads for 8x8 Y surface 
// 1 sampler read  for 8x8 U surface 
// 1 sampler read  for 8x8 V surface
//------------------------------------------------------------------------------

    // 1st 8x8 setup
    #include "AVS_SetupFirstBlock.asm"

    // 1st 8x8 Y sampling                                                       
    mov (1) rAVS_8x8_HDR.2:ud       nAVS_GREEN_CHANNEL_ONLY:ud   // Enable green channel
    mov (16) mAVS_8x8_HDR.0:ud      rAVS_8x8_HDR.0<8;8,1>:ud    // Copy msg header and payload mirrors to MRFs
    send (1) uwAVS_RESPONSE(0)<1>   mAVS_8x8_HDR    udDUMMY_NULL    nSMPL_ENGINE        nAVS_MSG_DSC_1CH+nSI_SRC_Y+nBI_CURRENT_SRC_Y
    // Return Y in 4 GRFs

    // 8x8 U sampling 
    mov (1)  rAVS_8x8_HDR.2:ud      nAVS_RED_CHANNEL_ONLY:ud     // Enable red channel
    mul (1)  rAVS_PAYLOAD.1:f       fVIDEO_STEP_X:f    2.0:f    // Calculate Step X for chroma
    mov (16) mAVS_8x8_HDR_UV.0:ud      rAVS_8x8_HDR.0<8;8,1>:ud    // Copy msg header and payload mirrors to MRFs
    send (1) uwAVS_RESPONSE(4)<1>   mAVS_8x8_HDR_UV   udDUMMY_NULL      nSMPL_ENGINE        nAVS_MSG_DSC_1CH+nSI_SRC_U+nBI_CURRENT_SRC_U
    // Return U in 4 GRFs

    // 8x8 V sampling 
    mov (1)  rAVS_8x8_HDR.2:ud      nAVS_RED_CHANNEL_ONLY:ud     // Dummy instruction just in order to avoid back-2-back send instructions!
    mov (16) mAVS_8x8_HDR_UV.0:ud      rAVS_8x8_HDR.0<8;8,1>:ud    // Copy msg header and payload mirrors to MRFs
    send (1) uwAVS_RESPONSE(8)<1>   mAVS_8x8_HDR_UV   udDUMMY_NULL      nSMPL_ENGINE        nAVS_MSG_DSC_1CH+nSI_SRC_V+nBI_CURRENT_SRC_V
    // Return V in 4 GRFs

    // 2nd 8x8 setup
    #include "AVS_SetupSecondBlock.asm"

    // 2nd 8x8 Y sampling
    mov (1)  rAVS_8x8_HDR.2:ud      nAVS_GREEN_CHANNEL_ONLY:ud   // Enable green channel
    mov (1)  rAVS_PAYLOAD.1:f       fVIDEO_STEP_X:f             // Restore Step X for luma
    mov (16) mAVS_8x8_HDR.0:ud      rAVS_8x8_HDR.0<8;8,1>:ud    // Copy msg header and payload mirrors to MRFs
    send (1) uwAVS_RESPONSE(12)<1>  mAVS_8x8_HDR   udDUMMY_NULL      nSMPL_ENGINE        nAVS_MSG_DSC_1CH+nSI_SRC_Y+nBI_CURRENT_SRC_Y 
    // Return Y in 4 GRFs

//------------------------------------------------------------------------------
// Unpacking sampler reads to 4:2:2 internal planar 
//------------------------------------------------------------------------------
    #include "PL3_AVS_IEF_Unpack_8x8.asm"
    

                    

