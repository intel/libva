/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: save_Left_Y_16x4T.asm
//
// Transpose 16x4 to 4x16 Y data and write to memory 
//
//----------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Left MB region:
//	PREV_MB_YB:	  	Base=rxx 	ElementSize=1 SrcRegion=REGION(16,1) Type=ub

//	Binding table index: 
//	BI_SRC_Y:		Binding table index of Y surface
//
//	Temp buffer:
//	BUF_B:			BUF_B Base=rxx ElementSize=1 SrcRegion=REGION(16,1) Type=ub
//
//
#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD6:w
#endif

#if defined(_FIELD)
    and.nz.f0.1 (1) NULLREGW 	BitFields:w  	BotFieldFlag:w			// Get bottom field flag
#endif

    mov (2)	MSGSRC.0<1>:ud	ORIX_LEFT<2;2,1>:w		{ NoDDClr }		// Block origin
    mov (1)	MSGSRC.2<1>:ud	0x000F0003:ud			{ NoDDChk }		// 4x16
    
// Transpose Y, save them to MRFs

//	16x4 Y src in GRF (each pix is specified as yx)
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|f1 e1 d1 c1 b1 a1 91 81 71 61 51 41 31 21 11 01 f0 e0 d0 c0 b0 a0 90 80 70 60 50 40 30 20 10 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|f3 e3 d3 c3 b3 a3 93 83 73 63 53 43 33 23 13 03 f2 e2 d2 c2 b2 a2 92 82 72 62 52 42 32 22 12 02|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//
//  First step		(16)	<1>	<=== <16;4,1>
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|73 63 53 43 72 62 52 42 71 61 51 41 70 60 50 40 33 23 13 03 32 22 12 02 31 21 11 01 30 20 10 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|f3 e3 d3 c3 f2 e2 d2 c2 f1 e1 d1 c1 f0 e0 d0 c0 b3 a3 93 83 b2 a2 92 82 b1 a1 91 81 b0 a0 90 80|
//	+-----------------------+-----------------------+-----------------------+-----------------------+

	// The first step
	mov (16)	LEFT_TEMP_B(0,0)<1>			PREV_MB_YB(0,0)<16;4,1>		{ NoDDClr }	
	mov (16)	LEFT_TEMP_B(0,16)<1>		PREV_MB_YB(0,4)<16;4,1>		{ NoDDChk }
	mov (16)	LEFT_TEMP_B(1,0)<1>			PREV_MB_YB(0,8)<16;4,1>		{ NoDDClr }
	mov (16)	LEFT_TEMP_B(1,16)<1>		PREV_MB_YB(0,12)<16;4,1>	{ NoDDChk }

//
//  Second step		(16)	<1>	<=== <1;4,4>
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|73 72 71 70 63 62 61 60 53 52 51 50 43 42 41 40 33 32 31 30 23 22 21 20 13 12 11 10 03 02 01 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|f3 f2 f1 f0 e3 e2 e1 e0 d3 d2 d1 d0 c3 c2 c1 c0 b3 b2 b1 b0 a3 a2 a1 a0 93 92 91 90 83 82 81 80|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//
	// The second step
	mov	(16)	MSGPAYLOADB(0,0)<1>		LEFT_TEMP_B(0,0)<1;4,4>
	mov (16)	MSGPAYLOADB(0,16)<1>	LEFT_TEMP_B(0,16)<1;4,4>
	mov (16)	MSGPAYLOADB(1,0)<1>		LEFT_TEMP_B(1,0)<1;4,4>
	mov (16)	MSGPAYLOADB(1,16)<1>	LEFT_TEMP_B(1,16)<1;4,4>

//  Transposed Y in 4x16 is ready for writting to dataport.


#if defined(_PROGRESSIVE) 
	mov (1)	MSGDSC	MSG_LEN(2)+DWBWMSGDSC+BI_DEST_Y:ud
//    send (8)	NULLREG		MSGHDR		MSGSRC<8;8,1>:ud	DWBWMSGDSC+0x00200000+BI_DEST_Y
#endif

#if defined(_FIELD)
	// Field picture
    (f0.1) mov (1)	MSGDSC	MSG_LEN(2)+DWBWMSGDSC+ENMSGDSCBF+BI_DEST_Y:ud  // Write 2 GRFs to DEST_Y bottom field
    (-f0.1) mov (1)	MSGDSC	MSG_LEN(2)+DWBWMSGDSC+ENMSGDSCTF+BI_DEST_Y:ud  // Write 2 GRFs to DEST_Y top field

#endif

    send (8)	null:ud		MSGHDR		MSGSRC<8;8,1>:ud	DAPWRITE	MSGDSC

