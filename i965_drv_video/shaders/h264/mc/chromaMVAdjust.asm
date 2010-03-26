/*
 * Adjust chrom MV
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: ChromaMVAdjust.asm
//
//


//#if !defined(__ChromaMVAdjust__)		// Make sure this is only included once
//#define __ChromaMVAdjust__


	// Chroma MV adjustment
	add (1)		acc0:w				gPARITY:w						gREFPARITY:w
	cmp.e.f0.0 (1) null:w			acc0:w							0x1:w
	cmp.e.f0.1 (1) null:w			acc0:w							0x100:w
	mov (1)		gCHRMVADJ:w			0:w
	(f0.0) mov (1)	gCHRMVADJ:w		2:w	
	(f0.1) mov (1)	gCHRMVADJ:w		-2:w
        
//#endif	// !defined(__ChromaMVAdjust__)
