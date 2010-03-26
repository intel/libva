/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: save_Top_UV_8x2.asm
//
// Save UV 8x2 block (8x2U + 8x2V in NV12)
//
//----------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Source region in :ud
//	SRC_UD:			SRC_UD Base=rxx ElementSize=4 SrcRegion=REGION(8,1) Type=ud			// 1 GRF
//
//	Binding table index: 
//	BI_DEST_UV:		Binding table index of UV surface (NV12)
//
//----------------------------------------------------------------

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD5:w
#endif
	and.z.f0.1 (8) NULLREGW		DualFieldMode<0;1,0>:w		1:w

    // FieldModeCurrentMbFlag determines how to access above MB
	and.z.f0.0 (1) 	null:w		r[ECM_AddrReg, BitFlags]:ub		FieldModeCurrentMbFlag:w		
    
	// Pack U and V
    mov (1)	MSGSRC.0:ud		ORIX_TOP:w					{ NoDDClr } 			// Block origin
    asr (1)	MSGSRC.1:ud		ORIY_TOP:w			1:w		{ NoDDClr, NoDDChk }	// NV12 U+V block origin y = half of Y comp
    mov (1)	MSGSRC.2:ud		0x0001000F:ud				{ NoDDChk }				// NV12 U+V block width and height (16x2)

	// Dual field mode
	(f0.1) mov (8)	MSGPAYLOADD(0)<1>		PREV_MB_UD(0)
    (-f0.1) mov (8) MSGPAYLOADD(0)<1>		PREV_MB_UD(1)	// for dual field mode, write last 2 rows
	
    // Set message descriptor

    and.nz.f0.1 (1) NULLREGW 		BitFields:w   BotFieldFlag:w
    
	(f0.0)	if	(1)		ELSE_UV_8X2_SAVE

    // Frame picture
    mov (1)	MSGDSC	MSG_LEN(1)+DWBWMSGDSC_WC+BI_DEST_UV:ud			// Write 1 GRFs to DEST_UV

	// Add vertical offset 8 for bot MB in MBAFF mode
	(f0.1) add (1)	MSGSRC.1:d		MSGSRC.1:d		8:w		

ELSE_UV_8X2_SAVE: 
	else 	(1)		ENDIF_UV_8X2_SAVE

	asr (1)	MSGSRC.1:d		ORIY_CUR:w		2:w			// asr 1: NV12 U+V block origin y = half of Y comp
														// asr 1: Reduce y by half in field access mode
	// Field picture
    (f0.1) mov (1)	MSGDSC	MSG_LEN(1)+DWBWMSGDSC_WC+ENMSGDSCBF+BI_DEST_UV:ud  // Write 1 GRF to DEST_Y bottom field
    (-f0.1) mov (1)	MSGDSC	MSG_LEN(1)+DWBWMSGDSC_WC+ENMSGDSCTF+BI_DEST_UV:ud  // Write 1 GRF to DEST_Y top field

	add (1)	MSGSRC.1:d		MSGSRC.1:d		-2:w		// for last 4 rows of above MB

	endif
ENDIF_UV_8X2_SAVE:

    send (8)	WritebackResponse(0)<1>		MSGHDR		MSGSRC<8;8,1>:ud	DAPWRITE	MSGDSC

// End of save_Top_UV_8x2.asm
