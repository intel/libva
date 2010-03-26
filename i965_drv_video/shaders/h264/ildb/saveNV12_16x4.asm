/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: saveNV12_16x4.asm
//
// Save a NV12 16x4 block 
//
//----------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Source region in :ud
//	SRC_YD:			SRC_YD Base=rxx ElementSize=4 SrcRegion=REGION(8,1) Type=ud			// 2 GRFs
//	SRC_UD:			SRC_UD Base=rxx ElementSize=4 SrcRegion=REGION(8,1) Type=ud			// 1 GRF
//
//	Binding table index: 
//	BI_DEST_Y:		Binding table index of Y surface
//	BI_DEST_UV:		Binding table index of UV surface (NV12)
//
//----------------------------------------------------------------

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD5:w
#endif

    mov (2)	MSGSRC.0<1>:ud	ORIX_TOP<2;2,1>:w							// Block origin
    mov (1)	MSGSRC.2<1>:ud	0x0003000F:ud								// Block width and height (16x4)

	// Pack Y    
	mov	(16)	MSGPAYLOADD(0)<1>		SRC_YD(0)						// Compressed inst
    
    send (8)	NULLREG	MSGHDR	MSGSRC<8;8,1>:ud	DAPWRITE	MSG_LEN(2)+DWBWMSGDSC+BI_DEST_Y		// Write 2 GRFs


    asr (1)	MSGSRC.1:ud		MSGSRC.1:ud			1:w						// NV12 U+V block origin y = half of Y comp
    mov (1)	MSGSRC.2<1>:ud	0x0001000F:ud								// NV12 U+V block width and height (16x2)

	// Pack U and V
//	mov (16)	MSGPAYLOADB(0,0)<2>		SRC_UB(0,0)
//	mov (16)	MSGPAYLOADB(0,1)<2>		SRC_VB(0,0)
	
	mov (8)	MSGPAYLOADD(0,0)<1>		SRC_UD(0)	
	
    send (8)	NULLREG	MSGHDR	MSGSRC<8;8,1>:ud	DAPWRITE	MSG_LEN(1)+DWBWMSGDSC+BI_DEST_UV	// Write 1 GRF

// End of saveNV12_16x4.asm
