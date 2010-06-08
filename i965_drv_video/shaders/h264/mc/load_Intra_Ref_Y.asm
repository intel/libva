/*
 * Load all reference Y samples from neighboring macroblocks
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__LOAD_INTRA_REF_Y__)		// Make sure this is only included once
#define __LOAD_INTRA_REF_Y__

// Module name: load_Intra_Ref_Y.asm
//
// Load all reference Y samples from neighboring macroblocks
//
load_Intra_Ref_Y:
//	shl (2) I_ORIX<1>:uw	ORIX<2;2,1>:ub	4:w		// Convert MB origin to pixel unit

// First load top 28x1 row reference samples
// 4 from macroblock D (actually use 1), 16 from macroblock B, and 8 from macroblock C
//
    add	(2)	MSGSRC.0<1>:d	I_ORIX<2;2,1>:w	TOP_REF_OFFSET<2;2,1>:b	{NoDDClr}	// Reference samples positioned at (-4, -1)
    mov (1)	MSGSRC.2:ud		0x0000001B:ud {NoDDChk}								// Block width and height (28x1)
    add (1)	MSGDSC	REG_MBAFF_FIELD<0;1,0>:uw	RESP_LEN(1)+DWBRMSGDSC_RC+DESTY:ud  // Set message descriptor
    send (8)	INTRA_REF_TOP_D(0)	MSGHDRY0	MSGSRC<8;8,1>:ud	DAPREAD	MSGDSC

// Then load left 4x16 reference samples (actually use 1x16 column)
//
    add	(1)	MSGSRC.1<1>:d	MSGSRC.1<0;1,0>:d	1:w {NoDDClr}	// Reference samples positioned next row
    mov (1)	MSGSRC.2:ud		0x00F0003:ud	{NoDDChk}			// Block width and height (4x16)
    add (1)	MSGDSC			MSGDSC	RESP_LEN(1):ud	// Need to read 1 more GRF register
    send (8)	INTRA_REF_LEFT_D(0)	MSGHDRY1	MSGSRC<8;8,1>:ud	DAPREAD	MSGDSC

	RETURN
// End of load_Intra_Ref_Y
#endif	// !defined(__LOAD_INTRA_REF_Y__)
