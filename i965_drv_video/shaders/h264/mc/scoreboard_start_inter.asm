/*
 * Scoreboard function for starting inter prediction kernels
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__SCOREBOARD_START_INTER__)
#define __SCOREBOARD_START_INTER__
//
// Module name: scoreboard_start_inter.asm
//
//	Scoreboard function for starting inter prediction kernels
//	This function is only used by inter prediction kernels to send message to
//	scoreboard in order to announce the inter kernel has started
//
//  $Revision: 5 $
//  $Date: 10/18/06 4:11p $
//
scoreboard_start_inter:

// First open message gateway since intra kernels need wake-up message to resume
// 
    mov (8)	MSGHDRY0<1>:ud	0x00000000:ud			// Initialize message header payload with 0

    // Send a message with register base RegBase = r0 (0x0) and Size = 0x0
    // 000 00000000 00000 00000 000 00000000 ==> 0000 0000 0000 0000 0000 0000 0000 0000
    // ---------------------------------------------------------------------------------
	send (8)	NULLREG  MSGHDRY0	null:ud    MSG_GW	OGWMSGDSC

//	Derive the scoreboard location where the inter thread writes to
//
    mov (8)		MSGHDRY1<1>:ud	0x00000000:ud			// Initialize message header payload with 0

	// Compose M0.5:ud
	#include "set_SB_offset.asm"

	// Compose M0.0:ud, i.e. message payload
	or	(1)		MSGHDRY1.1<1>:uw	sr0.0<0;1,0>:uw		0x0000:uw	// Set EUID/TID bits + inter start bit

	send (8)	NULLREG  MSGHDRY1	null:ud    MSG_GW	FWDMSGDSC+NOTIFYMSG	// Send "Inter start" message to scoreboard kernel

    RETURN

#endif	// !defined(__SCOREBOARD_START_INTER__)
