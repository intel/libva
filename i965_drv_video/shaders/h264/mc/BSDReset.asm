/*
 * Initial kernel for filling initial BSD command buffer
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: BSDReset.asm
//
// Initial kernel for filling initial BSD command buffer
//

// ----------------------------------------------------
//  Main: BSDReset
// ----------------------------------------------------

.kernel BSDReset
BSDRESET:

#include "header.inc"

.code
#ifdef SW_SCOREBOARD
    CALL(scoreboard_start_inter,1)
	wait	n0:ud		//	Now wait for scoreboard to response

#define BSDRESET_ENABLE
	#include "scoreboard_update.asm"	// scorboard update function
#undef BSDRESET_ENABLE

#endif	// defined(SW_SCOREBOARD)

// Terminate the thread
//
    END_THREAD

#if !defined(COMBINED_KERNEL)		// For standalone kernel only
.end_code

.end_kernel
#endif	// !defined(COMBINED_KERNEL)
