/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: save_Cur_Y_16x16.asm
//
// Save a Y 16x16 block 
//
//----------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Source region in :ud
//	SRC_YD:			SRC_YD Base=rxx ElementSize=4 SrcRegion=REGION(8,1) Type=ud			// 8 GRFs
//
//	Binding table index: 
//	BI_DEST_Y:		Binding table index of Y surface
//
//----------------------------------------------------------------

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD4:w
#endif

	and.z.f0.0 (1) 	null:w		r[ECM_AddrReg, BitFlags]:ub		FieldModeCurrentMbFlag:w		

    and.nz.f0.1 (1)	NULLREGW 		BitFields:w  	BotFieldFlag:w

    mov (2)	MSGSRC.0<1>:ud	ORIX_CUR<2;2,1>:w		{ NoDDClr }		// Block origin
    mov (1)	MSGSRC.2<1>:ud	0x000F000F:ud			{ NoDDChk }		// Block width and height (16x16 or 12x16)

	// Pack Y    
	mov	(16)	MSGPAYLOADD(0)<1>		SRC_YD(0)		// Compressed inst
	mov (16)	MSGPAYLOADD(2)<1>		SRC_YD(2)
	mov (16)	MSGPAYLOADD(4)<1>		SRC_YD(4)
	mov (16)	MSGPAYLOADD(6)<1>		SRC_YD(6)
    
    // Set message descriptor

	(f0.0)	if	(1)		ELSE_Y_16x16
    
    // Frame picture
    mov (1)	MSGDSC	MSG_LEN(8)+DWBWMSGDSC+BI_DEST_Y:ud			// Write 8 GRFs to DEST_Y

	(f0.1) add (1)	MSGSRC.1:d		MSGSRC.1:d		16:w		// Add vertical offset 16 for bot MB in MBAFF mode

ELSE_Y_16x16: 
	else 	(1)		ENDIF_Y_16x16

	// Field picture
    (f0.1) mov (1)	MSGDSC	MSG_LEN(8)+DWBWMSGDSC+ENMSGDSCBF+BI_DEST_Y:ud  // Write 8 GRFs to DEST_Y bottom field
    (-f0.1) mov (1)	MSGDSC	MSG_LEN(8)+DWBWMSGDSC+ENMSGDSCTF+BI_DEST_Y:ud  // Write 8 GRFs to DEST_Y top field

	asr (1)	MSGSRC.1:d		MSGSRC.1:d		1:w					// Reduce y by half in field access mode

	endif
ENDIF_Y_16x16:
    
    send (8)	null:ud		MSGHDR		MSGSRC<8;8,1>:ud	DAPWRITE	MSGDSC

// End of save_Cur_Y_16x16.asm
