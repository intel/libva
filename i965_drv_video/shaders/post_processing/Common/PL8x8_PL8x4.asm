/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: PL8x8_PL8x4.asm
//
// Convert PL 8x8 to PL8x4 in GRF
//---------------------------------------------------------------
//  Symbols needed to be defined before including this module
//
//	DWORD_ALIGNED_DEST:	only if DEST_Y, DEST_U, DEST_V data are DWord aligned
//	ORIX:
//---------------------------------------------------------------

#include "PL8x8_PL8x4.inc"
  
// Convert PL8x8 to PL8x4 ---------------------------------------------------------

  mov (8) ubDEST_U(0,16)<2> ubDEST_U(1)<16;8,2> //selecting U every other row
  mov (16) ubDEST_U(0,32)<2> ubDEST_U(2)<32;8,2> //selecting U every other row
  mov (8) ubDEST_V(0,16)<2> ubDEST_V(1)<16;8,2> //selecting V every other row
  mov (16) ubDEST_V(0,32)<2> ubDEST_V(2)<32;8,2> //selecting V every other row
  
// End of PL8x8_PL8x4.asm -------------------------------------------------------