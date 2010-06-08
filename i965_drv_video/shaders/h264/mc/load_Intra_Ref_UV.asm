/*
 * Load all reference U/V samples from neighboring macroblocks
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__LOAD_INTRA_REF_UV__)		// Make sure this is only included once
#define __LOAD_INTRA_REF_UV__

// Module name: load_Intra_Ref_UV.asm
//
// Load all reference U/V samples from neighboring macroblocks
//
// Note: Since loading of U/V data always follows writing of Y, the message descriptor is manipulated
// to avoid recalculating due to frame/field variztions.

// First load top 20x1 row U/V reference samples
// 4 from macroblock D (actually use 2), 16 from macroblock B
//
    shr	(1)	I_ORIY<1>:w		I_ORIY<0;1,0>:w	1:w		// Adjust I_ORIY for NV12 format
    add	(2)	MSGSRC.0<1>:d	I_ORIX<2;2,1>:w	TOP_REF_OFFSET<2;2,1>:b	{NoDDClr}	// Reference samples positioned at (-4, -1)
    mov (1)	MSGSRC.2:ud		0x00000013:ud {NoDDChk}			// Block width and height (20x1)

//  Update message descriptor based on previous Y block write
//
#ifdef DEV_ILK
    add (1)	MSGDSC	MSGDSC	RESP_LEN(1)+DWBRMSGDSC_RC+DESTUV-DWBWMSGDSC_WC-0x10000000-DESTY:ud  // Set message descriptor
#else
    add (1)	MSGDSC	MSGDSC	RESP_LEN(1)+DWBRMSGDSC_RC+DESTUV-DWBWMSGDSC_WC-0x00800000-DESTY:ud  // Set message descriptor
#endif	// DEV_ILK

    send (8)	INTRA_REF_TOP_D(0)	MSGHDR	MSGSRC<8;8,1>:ud	DAPREAD	MSGDSC

// Then load left 4x8 reference samples (actually use 1x8 column)
//
    add	(1)	MSGSRC.1<1>:d	MSGSRC.1<0;1,0>:d	1:w {NoDDClr}	// Reference samples positioned next row
    mov (1)	MSGSRC.2:ud		0x00070003:ud {NoDDChk}			// Block width and height (4x8)
    send (8)	INTRA_REF_LEFT_D(0)	MSGHDRUV	MSGSRC<8;8,1>:ud	DAPREAD	MSGDSC

// End of load_Intra_Ref_UV
#endif	// !defined(__LOAD_INTRA_REF_UV__)
