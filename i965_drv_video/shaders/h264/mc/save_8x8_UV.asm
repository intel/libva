/*
 * Save decoded U/V picture data to frame buffer
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__SAVE_8x8_UV__)		// Make sure this is only included once
#define __SAVE_8x8_UV__

// Module name: save_8x8_UV.asm
//
// Save decoded U/V picture data to frame buffer
//

    mov (1) MSGSRC.2:ud	    0x0007000F:ud {NoDDClr}		// Block width and height (16x8)
    mov (2) MSGSRC.0<1>:ud  I_ORIX<2;2,1>:w	{NoDDChk}	// I_ORIX has already been adjusted for NV12

//  Update message descriptor based on previous read setup
//
#ifdef DEV_ILK
    add (1)		MSGDSC	MSGDSC MSG_LEN(4)+DWBWMSGDSC_WC-DWBRMSGDSC_RC-0x00100000:ud  // Set message descriptor
#else
    add (1)		MSGDSC	MSGDSC MSG_LEN(4)+DWBWMSGDSC_WC-DWBRMSGDSC_RC-0x00010000:ud  // Set message descriptor
#endif	// DEV_ILK

// Write U/V picture data
//
#ifndef MONO
    mov	    MSGPAYLOAD(0,0)<1>	DEC_UV(0)REGION(16,2)	// U/V row 0
    mov	    MSGPAYLOAD(0,16)<1>	DEC_UV(1)REGION(16,2)	// U/V row 1
    mov	    MSGPAYLOAD(1,0)<1>	DEC_UV(2)REGION(16,2)	// U/V row 2
    mov	    MSGPAYLOAD(1,16)<1>	DEC_UV(3)REGION(16,2)	// U/V row 3
    mov	    MSGPAYLOAD(2,0)<1>	DEC_UV(4)REGION(16,2)	// U/V row 4
    mov	    MSGPAYLOAD(2,16)<1>	DEC_UV(5)REGION(16,2)	// U/V row 5
    mov	    MSGPAYLOAD(3,0)<1>	DEC_UV(6)REGION(16,2)	// U/V row 6
    mov	    MSGPAYLOAD(3,16)<1>	DEC_UV(7)REGION(16,2)	// U/V row 7
#else	// defined(MONO)
    $for(0; <4; 2) {
	mov (16)	MSGPAYLOADD(%1)<1>		0x80808080:ud {Compr}
	}

#endif	// !defined(MONO)

	send (8)	REG_WRITE_COMMIT_UV<1>:ud	MSGHDR	MSGSRC<8;8,1>:ud	DAPWRITE	MSGDSC

// End of save_8x8_UV

#endif	// !defined(__SAVE_8x8_UV__)
