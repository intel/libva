/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//////////////////////////////////////////////////////////////////////////////////////////
//	Module name: Transpose_Y_16x16.asm
//	
//	Transpose Y 16x16 block.
//
//----------------------------------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Source region is :ub
//	SRC_YB:			SRC_YB Base=rxx ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// 8 GRFs
//
//  Temp buffer:
//	CUR_TEMP_B:		BUF_B Base=rxx ElementSize=1 SrcRegion=REGION(16,1) Type=ub		// 8 GRFs
//
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDDA:w
#endif


// Transpose Y (16x16 bytes)

// The first step
mov (16)	CUR_TEMP_B(0,0)<1>		SRC_YB(0,0)<16;4,1>		{ NoDDClr } 
mov (16)	CUR_TEMP_B(0,16)<1>		SRC_YB(2,0)<16;4,1>		{ NoDDChk }
mov (16)	CUR_TEMP_B(1,0)<1>		SRC_YB(4,0)<16;4,1>		{ NoDDClr }
mov (16)	CUR_TEMP_B(1,16)<1>		SRC_YB(6,0)<16;4,1>		{ NoDDChk }

mov (16)	CUR_TEMP_B(2,0)<1>		SRC_YB(0,4)<16;4,1>		{ NoDDClr }
mov (16)	CUR_TEMP_B(2,16)<1>		SRC_YB(2,4)<16;4,1>		{ NoDDChk }
mov (16)	CUR_TEMP_B(3,0)<1>		SRC_YB(4,4)<16;4,1>		{ NoDDClr }
mov (16)	CUR_TEMP_B(3,16)<1>		SRC_YB(6,4)<16;4,1>		{ NoDDChk }

mov (16)	CUR_TEMP_B(4,0)<1>		SRC_YB(0,8)<16;4,1>		{ NoDDClr }
mov (16)	CUR_TEMP_B(4,16)<1>		SRC_YB(2,8)<16;4,1>		{ NoDDChk }
mov (16)	CUR_TEMP_B(5,0)<1>		SRC_YB(4,8)<16;4,1>		{ NoDDClr }
mov (16)	CUR_TEMP_B(5,16)<1>		SRC_YB(6,8)<16;4,1>		{ NoDDChk }

mov (16)	CUR_TEMP_B(6,0)<1>		SRC_YB(0,12)<16;4,1>	{ NoDDClr }
mov (16)	CUR_TEMP_B(6,16)<1>		SRC_YB(2,12)<16;4,1>	{ NoDDChk }
mov (16)	CUR_TEMP_B(7,0)<1>		SRC_YB(4,12)<16;4,1>	{ NoDDClr }
mov (16)	CUR_TEMP_B(7,16)<1>		SRC_YB(6,12)<16;4,1>	{ NoDDChk }

// The second step
mov (16)	SRC_YB(0,0)<1>		CUR_TEMP_B(0,0)<32;8,4>		{ NoDDClr }
mov (16)	SRC_YB(0,16)<1>		CUR_TEMP_B(0,1)<32;8,4>		{ NoDDChk }
mov (16)	SRC_YB(1,0)<1>		CUR_TEMP_B(0,2)<32;8,4>		{ NoDDClr }
mov (16)	SRC_YB(1,16)<1>		CUR_TEMP_B(0,3)<32;8,4>		{ NoDDChk }

mov (16)	SRC_YB(2,0)<1>		CUR_TEMP_B(2,0)<32;8,4>		{ NoDDClr }
mov (16)	SRC_YB(2,16)<1>		CUR_TEMP_B(2,1)<32;8,4>		{ NoDDChk }
mov (16)	SRC_YB(3,0)<1>		CUR_TEMP_B(2,2)<32;8,4>		{ NoDDClr }
mov (16)	SRC_YB(3,16)<1>		CUR_TEMP_B(2,3)<32;8,4>		{ NoDDChk }

mov (16)	SRC_YB(4,0)<1>		CUR_TEMP_B(4,0)<32;8,4>		{ NoDDClr }
mov (16)	SRC_YB(4,16)<1>		CUR_TEMP_B(4,1)<32;8,4>		{ NoDDChk }
mov (16)	SRC_YB(5,0)<1>		CUR_TEMP_B(4,2)<32;8,4>		{ NoDDClr }
mov (16)	SRC_YB(5,16)<1>		CUR_TEMP_B(4,3)<32;8,4>		{ NoDDChk }

mov (16)	SRC_YB(6,0)<1>		CUR_TEMP_B(6,0)<32;8,4>		{ NoDDClr }
mov (16)	SRC_YB(6,16)<1>		CUR_TEMP_B(6,1)<32;8,4>		{ NoDDChk }
mov (16)	SRC_YB(7,0)<1>		CUR_TEMP_B(6,2)<32;8,4>		{ NoDDClr }
mov (16)	SRC_YB(7,16)<1>		CUR_TEMP_B(6,3)<32;8,4>		{ NoDDChk }

// Y is transposed.
