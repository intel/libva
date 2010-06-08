/*
 * Save scoreboard data before content switching
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: scoreboard_save_AS.asm
//
// Save scoreboard data before content switching
//
//
	//	r1 - r35 need to be saved
	// They are saved in a 2D surface with width of 32 and height of 64.
	// Each row corresponds to one GRF register in the following order
	// r4 - r35	: Scoreboard message
	// r1 - r3  : Scoreboard kernel control data

    mov (8)	MSGHDR<1>:ud	r0.0<8;8,1>:ud	// Initialize message header payload with r0
    mov (1)	MSGHDR.2:ud		0x0007001f:ud	// for 8 registers

    mov (2)	MSGHDR.0:ud		0:ud
	$for(0; <8; 2) {
	mov (16)	MSGPAYLOADD(%1)<1>	CMD_SB(%1)REGION(8,1) {Compr}
	}
    send (8)	NULLREG	MSGHDR	null:ud	DWBWMSGDSC+0x00800000+AS_SAVE	// Save r4 - r11

    mov (1)	MSGHDR.1:ud		8:ud
	$for(0; <8; 2) {
	mov (16)	MSGPAYLOADD(%1)<1>	CMD_SB(%1+8)REGION(8,1) {Compr}
	}
    send (8)	NULLREG	MSGHDR	null:ud	DWBWMSGDSC+0x00800000+AS_SAVE	// Save r12 - r19

    mov (1)	MSGHDR.1:ud		16:ud
	$for(0; <8; 2) {
	mov (16)	MSGPAYLOADD(%1)<1>	CMD_SB(%1+16)REGION(8,1) {Compr}
	}
    send (8)	NULLREG	MSGHDR	null:ud	DWBWMSGDSC+0x00800000+AS_SAVE	// Save r20 - r27

    mov (1)	MSGHDR.1:ud		24:ud
	$for(0; <8; 2) {
	mov (16)	MSGPAYLOADD(%1)<1>	CMD_SB(%1+24)REGION(8,1) {Compr}
	}
    send (8)	NULLREG	MSGHDR	null:ud	DWBWMSGDSC+0x00800000+AS_SAVE	// Save r28 - r35

    mov (1)	MSGHDR.1:ud		32:ud
	$for(0; <8; 2) {
	mov (16)	MSGPAYLOADD(%1)<1>	CMD_SB(%1+32)REGION(8,1) {Compr}
	}
    send (8)	NULLREG	MSGHDR	null:ud	DWBWMSGDSC+0x00800000+AS_SAVE	// Save r36 - r43

    mov (1)	MSGHDR.1:ud		40:ud
	$for(0; <8; 2) {
	mov (16)	MSGPAYLOADD(%1)<1>	CMD_SB(%1+40)REGION(8,1) {Compr}
	}
    send (8)	NULLREG	MSGHDR	null:ud	DWBWMSGDSC+0x00800000+AS_SAVE	// Save r44 - r51

    mov (1)	MSGHDR.1:ud		48:ud
	$for(0; <8; 2) {
	mov (16)	MSGPAYLOADD(%1)<1>	CMD_SB(%1+48)REGION(8,1) {Compr}
	}
    send (8)	NULLREG	MSGHDR	null:ud	DWBWMSGDSC+0x00800000+AS_SAVE	// Save r52 - r59

    mov (1)	MSGHDR.1:ud		56:ud
	$for(0; <8; 2) {
	mov (16)	MSGPAYLOADD(%1)<1>	CMD_SB(%1+56)REGION(8,1) {Compr}
	}
    send (8)	NULLREG	MSGHDR	null:ud	DWBWMSGDSC+0x00800000+AS_SAVE	// Save r60 - r67

// End of scoreboard_save_AS
