/*
 * Common module to end current intra thread
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: EndIntraThread.asm
//
// Common module to end current intra thread
//
#ifndef SW_SCOREBOARD
// Check for write commit first if SW scoreboard is disabled
	mov	(1)	REG_WRITE_COMMIT_Y<1>:ud	REG_WRITE_COMMIT_Y<0;1,0>:ud		// Make sure Y write is committed
	mov	(1)	REG_WRITE_COMMIT_UV<1>:ud	REG_WRITE_COMMIT_UV<0;1,0>:ud		// Make sure U/V write is committed
#endif

	END_THREAD

    #include "Intra_funcLib.asm"

#ifndef COMBINED_KERNEL		// For standalone kernel only
.end_code

.end_kernel
#endif	// COMBINED_KERNEL

// End of EndIntraThread
