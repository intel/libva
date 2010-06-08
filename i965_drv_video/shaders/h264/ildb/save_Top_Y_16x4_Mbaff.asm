/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: save_Top_Y_16x4.asm
//
// Save a Y 16x4 block 
//
//----------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Source region in :ud
//	SRC_YD:			SRC_YD Base=rxx ElementSize=4 SrcRegion=REGION(8,1) Type=ud			// 2 GRFs
//
//	Binding table index: 
//	BI_DEST_Y:		Binding table index of Y surface
//
//----------------------------------------------------------------

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD5:w
#endif

	and.z.f0.1 (16) NULLREGW		DualFieldMode<0;1,0>:w		1:w

    // FieldModeCurrentMbFlag determines how to access above MB
	and.z.f0.0 (1) 	null:w		r[ECM_AddrReg, BitFlags]:ub		FieldModeCurrentMbFlag:w		

    mov (2)	MSGSRC.0<1>:ud	ORIX_TOP<2;2,1>:w		{ NoDDClr }			// Block origin
    mov (1)	MSGSRC.2<1>:ud	0x0003000F:ud			{ NoDDChk }			// Block width and height (16x4)

	// Pack Y
	// Dual field mode
	(f0.1) mov	(16) MSGPAYLOADD(0)<1>		PREV_MB_YD(0)				// Compressed inst
    (-f0.1)  mov (16) MSGPAYLOADD(0)<1>		PREV_MB_YD(2)				// for dual field mode, write last 4 rows
    
    // Set message descriptor

    and.nz.f0.1 (1) NULLREGW 		BitFields:w   BotFieldFlag:w

	(f0.0)	if	(1)		ELSE_Y_16x4_SAVE
    
    // Frame picture
    mov (1)	MSGDSC	MSG_LEN(2)+DWBWMSGDSC_WC+BI_DEST_Y:ud			// Write 2 GRFs to DEST_Y

	// Add vertical offset 16 for bot MB in MBAFF mode
	(f0.1) add (1)	MSGSRC.1:d		MSGSRC.1:d		16:w		

ELSE_Y_16x4_SAVE: 
	else 	(1)		ENDIF_Y_16x4_SAVE

	asr (1)	MSGSRC.1:d		ORIY_CUR:w		1:w					// Reduce y by half in field access mode

	// Field picture
    (f0.1) mov (1)	MSGDSC	MSG_LEN(2)+DWBWMSGDSC_WC+ENMSGDSCBF+BI_DEST_Y:ud  // Write 2 GRFs to DEST_Y bottom field
    (-f0.1) mov (1)	MSGDSC	MSG_LEN(2)+DWBWMSGDSC_WC+ENMSGDSCTF+BI_DEST_Y:ud  // Write 2 GRFs to DEST_Y top field

	add (1)	MSGSRC.1:d		MSGSRC.1:d		-4:w	// for last 4 rows of above MB

	endif
ENDIF_Y_16x4_SAVE:
    
    send (8)	WritebackResponse(0)<1>		MSGHDR		MSGSRC<8;8,1>:ud	DAPWRITE	MSGDSC

// End of save_Top_Y_16x4.asm
