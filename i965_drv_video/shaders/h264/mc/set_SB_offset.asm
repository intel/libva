/*
 * Common module to set offset into the scoreboard
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//
// Module name: set_SB_offset.asm
//
// Common module to set offset into the scoreboard
//	Note: This is to encapsulate the way M0.5:ud in ForwardMsg is filled.
//
//  $Revision: 2 $
//  $Date: 10/16/06 5:19p $
//
	add (1)		MSGHDRY1.10<1>:uw r0.20:ub	0x0200:uw			// Message length = 1 DWORD

	add	(16)	acc0<1>:w	r0.12<0;1,0>:uw	-LEADING_THREAD:w	// 0-based thread count derived from r0.6:ud
	shl (1)		M05_STORE<1>:uw		acc0<0;1,0>:uw	0x2:uw		// Store for future "update" use, in DWORD unit
	and	(16)	acc0<1>:w	acc0<16;16,1>:uw	SB_MASK:uw		// Wrap around scoreboard
	shl (1)		MSGHDRY1.11<1>:uw	acc0<0;1,0>:uw	0x2:uw		// Convert to DWORD offset

// End of set_SB_offset