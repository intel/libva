/*
 * Dummy kernel
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: DCResetDummy.asm
//
// Dummy kernel used by driver for debug-counter reset SW WA
//

// ----------------------------------------------------
//  Main: DCResetDummy
// ----------------------------------------------------

.kernel DCResetDummy
DCRESETDUMMY:

#include "header.inc"

.code

// Terminate the thread
//
    END_THREAD

#if !defined(COMBINED_KERNEL)		// For standalone kernel only
.end_code

.end_kernel
#endif	// !defined(COMBINED_KERNEL)
