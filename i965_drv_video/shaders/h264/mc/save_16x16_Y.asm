/*
 * Save decoded Y picture data to frame buffer
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__SAVE_16X16_Y__)		// Make sure this is only included once
#define __SAVE_16X16_Y__

// Module name: save_16x16_Y.asm
//
// Save decoded Y picture data to frame buffer
//

save_16x16_Y:

    mov (1) MSGSRC.2:ud		0x000F000F:ud {NoDDClr}		// Block width and height (16x16)
    mov (2) MSGSRC.0:ud		I_ORIX<2;2,1>:w {NoDDChk}	// X, Y offset
#ifdef DEV_ILK
    add (1)		MSGDSC	MSGDSC MSG_LEN(8)+DWBWMSGDSC_WC-DWBRMSGDSC_RC-0x00200000:ud  // Set message descriptor
#else
    add (1)		MSGDSC	MSGDSC MSG_LEN(8)+DWBWMSGDSC_WC-DWBRMSGDSC_RC-0x00020000:ud  // Set message descriptor
#endif	// DEV_ILK

    mov (1) PDECBUF_UD<1>:ud	0x10001*DECBUF*GRFWIB+0x00400000:ud	// Pointers to row 0 and 2 of decoded data

    $for(0,0; <8; 2,4) {
	mov (32)	MSGPAYLOAD(%1)<1>	r[PDECBUF, %2*GRFWIB]REGION(16,2) {Compr}		// Block Y0/Y2
	mov (32)	MSGPAYLOAD(%1,16)<1>	r[PDECBUF, (1+%2)*GRFWIB]REGION(16,2) {Compr}	// Block Y1/Y3
    }

//  Update message descriptor based on previous read setup
//
    send (8)	REG_WRITE_COMMIT_Y<1>:ud	MSGHDR	MSGSRC<8;8,1>:ud	DAPWRITE	MSGDSC

    RETURN
// End of save_16x16_Y

#endif	// !defined(__SAVE_16X16_Y__)
