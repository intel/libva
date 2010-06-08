/*
 * Save Intra_4x4 decoded Y picture data to frame buffer
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__SAVE_4X4_Y__)		// Make sure this is only included once
#define __SAVE_4X4_Y__

// Module name: save_4x4_Y.asm
//
// Save Intra_4x4 decoded Y picture data to frame buffer
// Note: Each 4x4 block is stored in 1 GRF register in the order of block raster scan order,
// i.e. 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15

save_4x4_Y:

    mov (1) MSGSRC.2:ud		0x000F000F:ud {NoDDClr}		// Block width and height (16x16)
    mov (2) MSGSRC.0:ud		I_ORIX<2;2,1>:w {NoDDChk}	// X, Y offset
#ifdef DEV_ILK
    add (1)		MSGDSC	MSGDSC MSG_LEN(8)+DWBWMSGDSC_WC-DWBRMSGDSC_RC-0x00200000:ud  // Set message descriptor
#else
    add (1)		MSGDSC	MSGDSC MSG_LEN(8)+DWBWMSGDSC_WC-DWBRMSGDSC_RC-0x00020000:ud  // Set message descriptor
#endif	// DEV_ILK

    $for(0; <8; 2) {
	mov (16)	MSGPAYLOAD(%1)<1>		DEC_Y(%1)<16;4,1>
	mov (16)	MSGPAYLOAD(%1,16)<1>	DEC_Y(%1,4)<16;4,1>
	mov (16)	MSGPAYLOAD(%1+1)<1>		DEC_Y(%1,8)<16;4,1>
	mov (16)	MSGPAYLOAD(%1+1,16)<1>	DEC_Y(%1,12)<16;4,1>
    }

//  Update message descriptor based on previous read setup
//
    send (8)	REG_WRITE_COMMIT_Y<1>:ud	MSGHDR	MSGSRC<8;8,1>:ud	DAPWRITE	MSGDSC

    RETURN
// End of save_4x4_Y

#endif	// !defined(__SAVE_4X4_Y__)
