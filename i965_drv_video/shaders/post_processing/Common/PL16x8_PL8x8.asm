/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: PL16x8_PL8x8.asm
//----------------------------------------------------------------

#include "common.inc"

#ifndef DEST_U

	//DEST_U, DEST_V not defined
	#if (nSRC_REGION==nREGION_1)
		#define DEST_Y		uwTOP_Y
		#define DEST_U		uwTOP_U
		#define DEST_V		uwTOP_V
	#elif (nSRC_REGION==nREGION_2)
		#define DEST_Y		uwBOT_Y
		#define DEST_U		uwBOT_U
		#define DEST_V		uwBOT_V
	#endif
	
#endif


//Convert 444 from sampler to 422
$for (0, 0; <8; 2, 1) {
	mov		DEST_U(%2)<1>	DEST_U(%1)<16;8,2>
	mov		DEST_V(%2)<1>	DEST_V(%1)<16;8,2>	
}
