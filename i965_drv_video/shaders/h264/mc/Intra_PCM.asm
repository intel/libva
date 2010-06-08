/*
 * Decode Intra_PCM macroblock
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: Intra_PCM.asm
//
// Decoding of I_PCM macroblock
//
//  $Revision: 8 $
//  $Date: 10/18/06 4:10p $
//

// ----------------------------------------------------
//  Main: Intra_PCM
// ----------------------------------------------------

.kernel Intra_PCM
INTRA_PCM:

#ifdef _DEBUG
// WA for FULSIM so we'll know which kernel is being debugged
mov (1) acc0:ud 0x03aa55a5:ud
#endif

#include "SetupForHWMC.asm"

// Not actually needed here but just want to slow down the Intra-PCM to avoid race condition
//
#ifdef SW_SCOREBOARD
	and (1)	  REG_INTRA_PRED_AVAIL_FLAG_WORD<1>:w	REG_INTRA_PRED_AVAIL_FLAG_WORD<0;1,0>:w	0xffe0:w	// Ensure all neighbor avail flags are "0"
    CALL(scoreboard_start_intra,1)
	wait	n0:ud		//	Now wait for scoreboard to response
#endif

//
//  Decoding Y blocks
//
//	In I_PCM mode, the samples are already arranged in raster scan order within the macroblock.
//	We just need to save them to picture buffers
//
    #include "save_I_PCM.asm"		    // Save to destination picture buffers

#ifdef SW_SCOREBOARD    
    #include "scoreboard_update.asm"
#endif

// Terminate the thread
//
    #include "EndIntraThread.asm"

// End of Intra_PCM
