/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: PL8x8_PL8x4.inc
//
// Setup module for convert PL8x8 to PL8x4 
//
// 

// Source/destination region definitions
//
#include "undefall.inc"				//Undefine the SRC and DEST sysmbols

#if (nSRC_REGION==nREGION_1)

	//REGION_1 selected
	#define ubDEST_Y		ubTOP_Y		
	#define ubDEST_U		ubTOP_U	
  #define ubDEST_V		ubTOP_V
	
#elif (nSRC_REGION==nREGION_2)

	//REGION_2 selected	
	#define ubDEST_Y		ubBOT_Y	
	#define ubDEST_U		ubBOT_U	
	#define ubDEST_V		ubBOT_V
  
	
#endif
