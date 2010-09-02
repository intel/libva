/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Modual name: SetupVPKernel.asm
//
// Initial setup for running video-processing kernels
//

#include "common.inc"

//
//  Now, begin source code....
//
.code

#include "Init_All_Regs.asm"

mov (8)     rMSGSRC.0<1>:ud  r0.0<8;8,1>:ud  // Initialize message payload header with R0
#if	defined (INC_BLENDING)
    mul	(1)	fALPHA_STEP_X:f   fSCALING_STEP_RATIO:f 	fVIDEO_STEP_X:f	//StepX_ratio = AlphaStepX / VideoStepX
#endif

// End of SetupVPKernel


 
       
