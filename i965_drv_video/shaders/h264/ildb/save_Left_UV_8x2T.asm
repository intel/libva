/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: save_Left_UV_8x2T.asm
//
// Transpose 8x2 to 2x8 UV data and write to memory 
//
//----------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Left MB region:
//	PREV_MB_UW: 	Base=ryy 	ElementSize=2 SrcRegion=REGION(8,1) Type=uw

//	Binding table index: 
//	BI_SRC_UV:		Binding table index of UV surface (NV12)
//
//	Temp buffer:
//	BUF_W:			BUF_W Base=rxx ElementSize=1 SrcRegion=REGION(8,1) Type=uw
//
//
#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD6:w
#endif

#if defined(_FIELD)
    and.nz.f0.1 (1)  NULLREGW 	BitFields:w  	BotFieldFlag:w			// Get bottom field flag
#endif

	// Transpose U/V, save them to MRFs in NV12 format
    mov (1)	MSGSRC.0:ud		ORIX_LEFT:w						{ NoDDClr }			// Block origin
    asr (1)	MSGSRC.1:ud		ORIY_LEFT:w			1:w			{ NoDDClr, NoDDChk }	// NV12 U+V block origin y = half of Y comp
    mov (1)	MSGSRC.2:ud		0x00070003:ud					{ NoDDChk }			// NV12 U+V block width and height (4x8)


//	16x2 UV src in GRF (each pix is specified as yx)
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|71 71 61 61 51 51 41 41 31 31 21 21 11 11 01 01 70 70 60 60 50 50 40 40 30 30 20 20 10 10 00 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//
//	First step		(8)		<1>	<=== <8;4,1>:w
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|71 71 61 61 51 51 41 41 70 70 60 60 50 50 40 40 31 31 21 21 11 11 01 01 30 30 20 20 10 10 00 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
	mov (8)	LEFT_TEMP_W(0,0)<1>		PREV_MB_UW(0,0)<8;4,1>		{ NoDDClr }
	mov (8)	LEFT_TEMP_W(0,8)<1>		PREV_MB_UW(0,4)<8;4,1>		{ NoDDChk }

//	Second step		(8)		<1>	<=== <1;2,4>
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|71 71 70 70 61 61 60 60 51 51 50 50 41 41 40 40 31 31 30 30 21 21 20 20 11 11 10 10 01 01 00 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
	mov (8)	MSGPAYLOADW(0,0)<1>		LEFT_TEMP_W(0,0)<1;2,4>
	mov (8)	MSGPAYLOADW(0,8)<1>		LEFT_TEMP_W(0,8)<1;2,4>

//  Transposed U+V in NV12 in 4x8 is ready for writting to dataport.
 
#if defined(_PROGRESSIVE) 
	mov (1)	MSGDSC	MSG_LEN(1)+DWBWMSGDSC+BI_DEST_UV:ud
//    send (8)	NULLREG		MSGHDR		MSGSRC<8;8,1>:ud	DWBWMSGDSC+0x00100000+BI_DEST_UV
#endif

#if defined(_FIELD)
	// Field picture
    (f0.1) mov (1)	MSGDSC	MSG_LEN(1)+DWBWMSGDSC+ENMSGDSCBF+BI_DEST_UV:ud  // Write 1 GRF to DEST_UV bottom field
    (-f0.1) mov (1)	MSGDSC	MSG_LEN(1)+DWBWMSGDSC+ENMSGDSCTF+BI_DEST_UV:ud  // Write 1 GRF to DEST_UV top field

#endif
    send (8)	null:ud		MSGHDR		MSGSRC<8;8,1>:ud	DAPWRITE	MSGDSC
