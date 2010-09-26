/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

//////////////////////////////////////////////////////////////////////////////////
// Multiple_Loop_Head.asm
// This code sets up the loop control for multiple blocks per thread

	mul (1)	wFRAME_ENDX:w	ubBLK_CNT_X:ub	16:uw	{ NoDDClr }				// Build multi-block loop counters
	mov (1) wNUM_BLKS:w		ubNUM_BLKS:ub			{ NoDDClr, NoDDChk }	// Copy num blocks to word variable
	mov (1) wCOPY_ORIX:w	wORIX:w					{ NoDDChk }				// Copy multi-block origin in pixel 
	mov (2) fFRAME_VID_ORIX<1>:f			fSRC_VID_H_ORI<4;2,2>:f			// Copy src video origin for scaling, and alpha origin for blending
	add (1)	wFRAME_ENDX:w	wFRAME_ENDX:w	wORIX:w							// Continue building multi-block loop counters

VIDEO_PROCESSING_LOOP:		// Loop back entry point as the biginning of the loop for multiple blocks
	
// Beginning of the loop
