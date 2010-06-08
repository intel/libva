/*
 * Restore previously stored scoreboard data after content switching back
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: scoreboard_restore_AS.asm
//
// Restore previously stored scoreboard data after content switching back
//
//
	// Restore scoreboard data to r4 - r67
	// They are saved in a 2D surface with width of 32 and height of 80.
	// Each row corresponds to one GRF register in the following order
	// r4 - r67	: Scoreboard message
	//
    mov (8)	MSGSRC<1>:ud	r0.0<8;8,1>:ud {NoDDClr}	// Initialize message header payload with r0

    mov (2)	MSGSRC.0:ud		0:ud {NoDDClr, NoDDChk}		// Starting r4
    mov (1)	MSGSRC.2:ud		0x0007001f:ud {NoDDChk}		// for 8 registers
    send (8)	CMD_SB(0)<1>	m1	MSGSRC<8;8,1>:ud	DWBRMSGDSC_SC+0x00080000+AS_SAVE	// Restore r4 - r11

    mov (8)	m2:ud		MSGSRC<8;8,1>:ud
    mov (1)	m2.1:ud		8:ud
    send (8)	CMD_SB(8)<1>	m2	null:ud	DWBRMSGDSC_SC+0x00080000+AS_SAVE	// Restore r12 - r19

    mov (8)	m3:ud		MSGSRC<8;8,1>:ud
    mov (1)	m3.1:ud		16:ud
    send (8)	CMD_SB(16)<1>	m3	null:ud	DWBRMSGDSC_SC+0x00080000+AS_SAVE	// Restore r20 - r27

    mov (8)	m4:ud		MSGSRC<8;8,1>:ud
    mov (1)	m4.1:ud		24:ud
    send (8)	CMD_SB(24)<1>	m4	null:ud	DWBRMSGDSC_SC+0x00080000+AS_SAVE	// Restore r28 - r35

    mov (8)	m5:ud		MSGSRC<8;8,1>:ud
    mov (1)	m5.1:ud		32:ud
    send (8)	CMD_SB(32)<1>	m5	null:ud	DWBRMSGDSC_SC+0x00080000+AS_SAVE	// Restore r36 - r43

    mov (8)	m6:ud		MSGSRC<8;8,1>:ud
    mov (1)	m6.1:ud		40:ud
    send (8)	CMD_SB(40)<1>	m6	null:ud	DWBRMSGDSC_SC+0x00080000+AS_SAVE	// Restore r44 - r51

    mov (8)	m7:ud		MSGSRC<8;8,1>:ud
    mov (1)	m7.1:ud		48:ud
    send (8)	CMD_SB(48)<1>	m7	null:ud	DWBRMSGDSC_SC+0x00080000+AS_SAVE	// Restore r52 - r59

    mov (8)	m8:ud		MSGSRC<8;8,1>:ud
    mov (1)	m8.1:ud		56:ud
    send (8)	CMD_SB(56)<1>	m8	null:ud	DWBRMSGDSC_SC+0x00080000+AS_SAVE	// Restore r60 - r67

// End of scoreboard_restore_AS
