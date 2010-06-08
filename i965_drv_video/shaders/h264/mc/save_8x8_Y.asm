/*
 * Save Intra_8x8 decoded Y picture data to frame buffer
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__SAVE_8X8_Y__)		// Make sure this is only included once
#define __SAVE_8X8_Y__

// Module name: save_8x8_Y.asm
//
// Save Intra_8x8 decoded Y picture data to frame buffer
// NotE: Every 4 rows of Y data are interleaved with the horizontal neighboring blcok
//
save_8x8_Y:

    mov (1) MSGSRC.2:ud		0x000F000F:ud {NoDDClr}		// Block width and height (16x16)
    mov (2) MSGSRC.0:ud		I_ORIX<2;2,1>:w {NoDDChk}	// X, Y offset

//  Update message descriptor based on previous read setup
//
#ifdef DEV_ILK
    add (1)		MSGDSC	MSGDSC MSG_LEN(8)+DWBWMSGDSC_WC-DWBRMSGDSC_RC-0x00200000:ud  // Set message descriptor
#else
    add (1)		MSGDSC	MSGDSC MSG_LEN(8)+DWBWMSGDSC_WC-DWBRMSGDSC_RC-0x00020000:ud  // Set message descriptor
#endif	// DEV_ILK

	mov (16)	MSGPAYLOAD(0)<1>	DEC_Y(0)<32;8,1>
	mov (16)	MSGPAYLOAD(0,16)<1>	DEC_Y(0,8)<32;8,1>
	mov (16)	MSGPAYLOAD(1,0)<1>	DEC_Y(0,16)<32;8,1>
	mov (16)	MSGPAYLOAD(1,16)<1>	DEC_Y(0,24)<32;8,1>

	mov (16)	MSGPAYLOAD(2)<1>	DEC_Y(2)<32;8,1>
	mov (16)	MSGPAYLOAD(2,16)<1>	DEC_Y(2,8)<32;8,1>
	mov (16)	MSGPAYLOAD(3,0)<1>	DEC_Y(2,16)<32;8,1>
	mov (16)	MSGPAYLOAD(3,16)<1>	DEC_Y(2,24)<32;8,1>

	mov (16)	MSGPAYLOAD(4)<1>	DEC_Y(4)<32;8,1>
	mov (16)	MSGPAYLOAD(4,16)<1>	DEC_Y(4,8)<32;8,1>
	mov (16)	MSGPAYLOAD(5,0)<1>	DEC_Y(4,16)<32;8,1>
	mov (16)	MSGPAYLOAD(5,16)<1>	DEC_Y(4,24)<32;8,1>

	mov (16)	MSGPAYLOAD(6)<1>	DEC_Y(6)<32;8,1>
	mov (16)	MSGPAYLOAD(6,16)<1>	DEC_Y(6,8)<32;8,1>
	mov (16)	MSGPAYLOAD(7,0)<1>	DEC_Y(6,16)<32;8,1>
	mov (16)	MSGPAYLOAD(7,16)<1>	DEC_Y(6,24)<32;8,1>

    send (8)	REG_WRITE_COMMIT_Y<1>:ud	MSGHDR	MSGSRC<8;8,1>:ud	DAPWRITE	MSGDSC

    RETURN
// End of save_8x8_Y

#endif	// !defined(__SAVE_8X8_Y__)
