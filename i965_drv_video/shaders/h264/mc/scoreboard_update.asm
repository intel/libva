/*
 * Scoreboard update function for decoding kernels
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//
// Module name: scoreboard_update.asm
//
//	Scoreboard update function for decoding kernels
//
//	This module is used by decoding kernels to send message to scoreboard to update the
//	"complete" status, thus the dependency of the MB can be cleared.
//
//  $Revision: 6 $
//  $Date: 10/16/06 5:19p $
//
    mov (8)		MSGHDRY1<1>:ud	0x00000000:ud				// Initialize message header payload with 0

	// Compose M0.5:ud information
	add (1)	MSGHDRY1.10<1>:uw	r0.20:ub	0x0200:uw				// Message length = 1 DWORD
	and (1) MSGHDRY1.11<1>:uw	M05_STORE<0;1,0>:uw	SB_MASK*4:uw	// Retrieve stored value and wrap around scoreboard

	or (1)	MSGHDRY1.0<1>:ud	M05_STORE<0;1,0>:uw	0xc0000000:ud	// Set "Completed" bits

#ifndef BSDRESET_ENABLE
#ifdef INTER_KERNEL
	mov	(1)	gREG_WRITE_COMMIT_Y<1>:ud	gREG_WRITE_COMMIT_Y<0;1,0>:ud		// Make sure Y write is committed
	mov	(1)	gREG_WRITE_COMMIT_UV<1>:ud	gREG_WRITE_COMMIT_UV<0;1,0>:ud		// Make sure U/V write is committed
#else
	mov	(1)	REG_WRITE_COMMIT_Y<1>:ud	REG_WRITE_COMMIT_Y<0;1,0>:ud		// Make sure Y write is committed
	mov	(1)	REG_WRITE_COMMIT_UV<1>:ud	REG_WRITE_COMMIT_UV<0;1,0>:ud		// Make sure U/V write is committed
#endif	// INTER_KERNEL
#endif	// BSDRESET_ENABLE

	send (8)	NULLREG  MSGHDRY1	null:ud    MSG_GW	FWDMSGDSC

// End of scoreboard_update
