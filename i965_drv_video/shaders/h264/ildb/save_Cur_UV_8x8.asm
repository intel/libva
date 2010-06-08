/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: save_Cur_UV_8x8.asm
//
// Save UV 8x8 block (8x8U + 8x8V in NV12)
//
//----------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Source region in :ud
//	SRC_UD:			SRC_UD Base=rxx ElementSize=4 SrcRegion=REGION(8,1) Type=ud			// 4 GRF
//
//	Binding table index: 
//	BI_DEST_UV:		Binding table index of UV surface (NV12)
//
//----------------------------------------------------------------

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD4:w
#endif

#if defined(_FIELD)
    and.nz.f0.1 (1) NULLREGW 	BitFields:w  	BotFieldFlag:w			// Get bottom field flag
#endif

    mov (1)	MSGSRC.0:ud		ORIX_CUR:w					{ NoDDClr } 	// Block origin
    asr (1)	MSGSRC.1:ud		ORIY_CUR:w			1:w		{ NoDDClr, NoDDChk }	// NV12 U+V block origin y = half of Y comp
    mov (1)	MSGSRC.2:ud		0x0007000F:ud				{ NoDDChk }		// NV12 U+V block width and height (16x8)

	mov (16)	MSGPAYLOADD(0)<1>		SRC_UD(0) 		// Compressed inst
	mov (16)	MSGPAYLOADD(2)<1>		SRC_UD(2) 
	
#if defined(_PROGRESSIVE) 
	mov (1)		MSGDSC	MSG_LEN(4)+DWBWMSGDSC+BI_DEST_UV:ud
//    send (8)	NULLREG		MSGHDR		MSGSRC<8;8,1>:ud	DWBWMSGDSC+0x00400000+BI_DEST_UV
#endif

#if defined(_FIELD)
	// Field picture
    (f0.1) mov (1)	MSGDSC	MSG_LEN(4)+DWBWMSGDSC+ENMSGDSCBF+BI_DEST_UV:ud  // Write 4 GRFs to DEST_UV bottom field
    (-f0.1) mov (1)	MSGDSC	MSG_LEN(4)+DWBWMSGDSC+ENMSGDSCTF+BI_DEST_UV:ud  // Write 4 GRFs to DEST_UV top field

#endif

    send (8)	null:ud		MSGHDR		MSGSRC<8;8,1>:ud	DAPWRITE	MSGDSC

// End of save_Cur_UV_8x8.asm
