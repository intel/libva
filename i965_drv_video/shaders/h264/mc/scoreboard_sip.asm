/*
 * Scoreboard interrupt handler
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: scoreboard_sip.asm
//
// scoreboard interrupt handler
//
// Simply send a notification message to scoreboard thread

    mov (8)		m0<1>:ud	0x00000000:ud			// Initialize message header payload with 0
#ifdef	DOUBLE_SB
	mov (1)		m0.5<1>:ud	0x08000200:ud			// Message length = 1 DWORD, sent to GRF offset 64 registers
#else
	mov (1)		m0.5<1>:ud	0x04000200:ud			// Message length = 1 DWORD, sent to GRF offset 32 registers
#endif
	send (8)	null<1>:ud  m0	null:ud    0x03108002	// Send notification message to scoreboard kernel

	and (1)		cr0.1:ud	cr0.1:ud	0x00800000		// Clear preempt exception bit
	and (1)		cr0.0:ud	cr0.0:ud	0x7fffffff:ud	// Exit SIP routine
	nop													// Required by B-spec

.end_code






