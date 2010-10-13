/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: Expansion.inc
// Number of U/V rows per block definition
#undef 	nUV_NUM_OF_ROWS
#ifdef EXPAND_9x5
	#define nUV_NUM_OF_ROWS     6
#else
	#define nUV_NUM_OF_ROWS     8
#endif

// Source/destination region definitions
#undef uwDEST_U
#undef uwDEST_V
#if (nSRC_REGION==nREGION_1)
	#define uwDEST_U          uwTOP_U
	#define uwDEST_V          uwTOP_V
#elif (nSRC_REGION==nREGION_2)
	#define uwDEST_U          uwBOT_U
	#define uwDEST_V          uwBOT_V
#endif

// End of Expansion.inc
