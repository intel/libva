/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

//------------------------------------------------------------------------------
// AVS_SetupFirstBlock.asm
//------------------------------------------------------------------------------
        
    // Setup Message Header
//    mov (8) mAVS_8x8_HDR<1>:ud      rMSGSRC<8;8,1>:ud                                                     

    // Check  NLAS Enable bit
    and.z.f0.0	(1)	wNULLREG                uwNLAS_ENABLE:uw	BIT15:uw	
    (f0.0)mov   (1) fVIDEO_STEP_DELTA:f     0.0:f   
    
    // Setup Message Payload Header for 1st block of Media Sampler 8x8
    mov (1) rAVS_PAYLOAD.0:f        fVIDEO_STEP_DELTA:f     //NLAS dx
    mov (1) rAVS_PAYLOAD.1:f        fVIDEO_STEP_X:f         //Step X 
    mov (1) rAVS_PAYLOAD.5:f        fVIDEO_STEP_Y:f         //Step Y 
    mov (2) rAVS_PAYLOAD.2<4>:f     fSRC_VID_H_ORI<2;2,1>:f //Orig X and Y 


    





		
