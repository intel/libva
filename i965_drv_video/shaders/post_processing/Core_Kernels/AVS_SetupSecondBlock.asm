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
// AVS_SetupSecondBlock.asm
//------------------------------------------------------------------------------
        
    //NLAS calculations for 2nd block of Media Sampler 8x8: 
    // X(i) = X0 + dx*i + ddx*i*(i-1)/2   ==>  X(8) = X0 + dx*8 +ddx*28
    // dx(i)= dx(0) + ddx*i               ==>  dx(8)= dx + ddx*8

    // Calculating X(8)
    mov (1)   acc0.2<1>:f           fSRC_VID_H_ORI:f                         
    mac (1)   acc0.2<1>:f           fVIDEO_STEP_X:f          8.0:f           
    mac (1)   rAVS_PAYLOAD.2:f      fVIDEO_STEP_DELTA:f      28.0:f                    
    
    // Calculating dx(8)
    mov (1)   acc0.1<1>:f           fVIDEO_STEP_X:f                         
    mac (1)   rAVS_PAYLOAD.1:f      fVIDEO_STEP_DELTA:f      8.0:f
		
