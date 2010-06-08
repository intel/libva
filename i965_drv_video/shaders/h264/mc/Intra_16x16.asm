/*
 * Decode Intra_16x16 macroblock
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: Intra_16x16.asm
//
// Decoding of Intra_16x16 macroblock
//
//  $Revision: 8 $
//  $Date: 10/18/06 4:10p $
//

// ----------------------------------------------------
//  Main: Intra_16x16
// ----------------------------------------------------

#define	INTRA_16X16

.kernel Intra_16x16
INTRA_16x16:

#ifdef _DEBUG
// WA for FULSIM so we'll know which kernel is being debugged
mov (1) acc0:ud 0x00aa55a5:ud
#endif

#include "SetupForHWMC.asm"

#ifdef SW_SCOREBOARD    
    CALL(scoreboard_start_intra,1)
#endif

#ifdef SW_SCOREBOARD 
	wait	n0:ud		//	Now wait for scoreboard to response
#endif

//
//  Decode Y blocks
//
//	Load reference data from neighboring macroblocks
    CALL(load_Intra_Ref_Y,1)

//	Intra predict Intra_16x16 luma block
	#include "intra_pred_16x16_Y.asm"

//	Add error data to predicted intra data
	#include "add_Error_16x16_Y.asm"

//	Save decoded Y picture
	CALL(save_16x16_Y,1)
//
//  Decode U/V blocks
//
//	Note: The decoding for chroma blocks will be the same for all intra prediction mode
//
	CALL(decode_Chroma_Intra,1)

#ifdef SW_SCOREBOARD
    #include "scoreboard_update.asm"
#endif

// Terminate the thread
//
    #include "EndIntraThread.asm"

// End of Intra_16x16
