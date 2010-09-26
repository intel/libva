/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

//---------- PA_AVS_IEF_Sample.asm ----------

//------------------------------------------------------------------------------
// 2 sampler reads for 8x8 YUV packed
//------------------------------------------------------------------------------
        
    // 1st 8x8 setup
    #include "AVS_SetupFirstBlock.asm"

    // Enable RGB(YUV) channels
    mov (1)  rAVS_8x8_HDR.2:ud      nAVS_RGB_CHANNELS:ud   

    mov (16) mAVS_8x8_HDR.0:ud      rAVS_8x8_HDR.0<8;8,1>:ud    // Copy msg header and payload mirrors to MRFs
    send (1) uwAVS_RESPONSE(0)<1>   mAVS_8x8_HDR    udDUMMY_NULL    nSMPL_ENGINE    nAVS_MSG_DSC_3CH+nSI_SRC_YUV+nBI_CURRENT_SRC_YUV
    // Return YUV in 12 GRFs

    // 2nd 8x8 setup
    #include "AVS_SetupSecondBlock.asm"

    mov (16) mAVS_8x8_HDR_2.0:ud      rAVS_8x8_HDR.0<8;8,1>:ud    // Copy msg header and payload mirrors to MRFs
    send (1) uwAVS_RESPONSE_2(0)<1> mAVS_8x8_HDR_2    udDUMMY_NULL    nSMPL_ENGINE    nAVS_MSG_DSC_3CH+nSI_SRC_YUV+nBI_CURRENT_SRC_YUV
    // Return YUV in 12 GRFs
        

