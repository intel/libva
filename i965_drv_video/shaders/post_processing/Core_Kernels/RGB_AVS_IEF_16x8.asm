/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

//---------- RGB_AVS_IEF_16x8.asm ----------

#include "AVS_IEF.inc"

//------------------------------------------------------------------------------
// 2 sampler reads for 8x8 ARGB packed
//------------------------------------------------------------------------------

    // 1st 8x8 setup
    #include "AVS_SetupFirstBlock.asm"

    mov (1)  rAVS_8x8_HDR.2:ud      nAVS_ALL_CHANNELS:ud   // Enable ARGB channels
    mov (16) mAVS_8x8_HDR.0:ud      rAVS_8x8_HDR.0<8;8,1>:ud    // Copy msg header and payload mirrors to MRFs
    send (1) uwAVS_RESPONSE(0)<1>   mAVS_8x8_HDR    udDUMMY_NULL    nSMPL_ENGINE    nAVS_MSG_DSC_4CH+nSI_SRC_RGB+nBI_CURRENT_SRC_YUV
    // Return ARGB in 16 GRFs

    // 2nd 8x8 setup
    #include "AVS_SetupSecondBlock.asm"
    mov (16) mAVS_8x8_HDR_2.0:ud      rAVS_8x8_HDR.0<8;8,1>:ud    // Copy msg header and payload mirrors to MRFs
    send (1) uwAVS_RESPONSE_2(0)<1> mAVS_8x8_HDR_2    udDUMMY_NULL    nSMPL_ENGINE    nAVS_MSG_DSC_4CH+nSI_SRC_RGB+nBI_CURRENT_SRC_YUV
    // Return ARGB in 16 GRFs

        
