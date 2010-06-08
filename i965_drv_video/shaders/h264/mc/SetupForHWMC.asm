/*
 * Initial setup for running HWMC kernels in HWMC-Only decoding mode
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: SetupForHWMC.asm
//
// Initial setup for running HWMC kernels in HWMC-Only decoding mode
//
#include "header.inc"
#include "intra_Header.inc"

#if !defined(__SETUPFORHWMC__)	// Make sure the following are only included once
#define __SETUPFORHWMC__

.reg_count_total    64
.reg_count_payload  2

//
//  Now, begin source code....
//

.code
#endif	// !defined(__SETUPFORHWMC__)

    mov (8)	MSGSRC<1>:ud	r0.0<8;8,1>:ud			// Initialize message header payload with R0
	shl (2) I_ORIX<1>:uw	ORIX<2;2,1>:ub	4:w		// Convert MB origin to pixel unit

// End of SetupForHWMC
