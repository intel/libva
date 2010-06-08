/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//////////////////////////////////////////////////////////////////////////////////////////
//	Module name: Transpose_Y_4x16.asm
//	
//	Transpose a 4x16 internal planar to 16x4 internal planar block.
//	The src block is 16x16.  Right moft 4 columns are transposed.
//
//----------------------------------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Source region is :ub
//	SRC_YB:			SRC_YB Base=rxx ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// 8 GRFs
//
//  Temp buffer:
//	BUF_B:			BUF_B Base=rxx ElementSize=1 SrcRegion=REGION(16,1) Type=ub		// 8 GRFs
//
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDDB:w
#endif

// Transpose Y (4x16) right most 4 columns
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|1f 1e 1d 1c 1b 1a 19 18 17 16 15 14 13 12 11 10 0f 0e 0d 0c 0b 0a 09 08 07 06 05 04 03 02 01 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|3f 3e 3d 3c 3b 3a 39 38 37 36 35 34 33 32 31 30 2f 2e 2d 2c 2b 2a 29 28 27 26 25 24 23 22 21 20|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|5f 5e 5d 5c 5b 5a 59 58 57 56 55 54 53 52 51 50 4f 4e 4d 4c 4b 4a 49 48 47 46 45 44 43 42 41 40|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|7f 7e 7d 7c 7b 7a 79 78 77 76 75 74 73 72 71 70 6f 6e 6d 6c 6b 6a 69 68 67 66 65 64 63 62 61 60|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|9f 9e 9d 9c 9b 9a 99 98 97 96 95 94 93 92 91 90 8f 8e 8d 8c 8b 8a 89 88 87 86 85 84 83 82 81 80|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|bf be bd bc bb ba b9 b8 b7 b6 b5 b4 b3 b2 b1 b0 af ae ad ac ab aa a9 a8 a7 a6 a5 a4 a3 a2 a1 a0|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|df de dd dc db da d9 d8 d7 d6 d5 d4 d3 d2 d1 d0 cf ce cd cc cb ca c9 c8 c7 c6 c5 c4 c3 c2 c1 c0|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|ff fe fd fc fb fa f9 f8 f7 f6 f5 f4 f3 f2 f1 f0 ef ee ed ec eb ea e9 e8 e7 e6 e5 e4 e3 e2 e1 e0|
//	+-----------------------+-----------------------+-----------------------+-----------------------+

// The first step
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|7f 7e 7d 7c 6f 6e 6d 6c 5f 5e 5d 5c 4f 4e 4d 4c 3f 3e 3d 3c 2f 2e 2d 2c 1f 1e 1d 1c 0f 0e 0d 0c|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|ff fe fd fc ef ee ed ec df de dd dc cf ce cd cc bf be bd bc af ae ad ac 9f 9e 9d 9c 8f 8e 8d 8c|
//	+-----------------------+-----------------------+-----------------------+-----------------------+

// The second step
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|fd ed dd cd bd ad 9d 8d 7d 6d 5d 4d 3d 2d 1d 0d fc ec dc cc bc ac 9c 8c 7c 6c 5c 4c 3c 2c 1c 0c|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|ff ef df cf bf af 9f 8f 7f 6f 5f 4f 3f 2f 1f 0f fe ee de ce be ae 9e 8e 7e 6e 5e 4e 3e 2e 1e 0e|
//	+-----------------------+-----------------------+-----------------------+-----------------------+


mov (16)	LEFT_TEMP_B(0,0)<1>		SRC_YB(0,12)<16;4,1>		{ NoDDClr }	
mov (16)	LEFT_TEMP_B(0,16)<1>	SRC_YB(2,12)<16;4,1>		{ NoDDChk }
mov (16)	LEFT_TEMP_B(1,0)<1>		SRC_YB(4,12)<16;4,1>		{ NoDDClr }
mov (16)	LEFT_TEMP_B(1,16)<1>	SRC_YB(6,12)<16;4,1>		{ NoDDChk }

// The second step
mov (16)	LEFT_TEMP_B(2,0)<1>		LEFT_TEMP_B(0,0)<32;8,4> 		{ NoDDClr }	
mov (16)	LEFT_TEMP_B(2,16)<1>	LEFT_TEMP_B(0,1)<32;8,4>		{ NoDDChk }
mov (16)	LEFT_TEMP_B(3,0)<1>		LEFT_TEMP_B(0,2)<32;8,4>		{ NoDDClr }
mov (16)	LEFT_TEMP_B(3,16)<1>	LEFT_TEMP_B(0,3)<32;8,4>		{ NoDDChk }

// Y is now transposed. the result is in LEFT_TEMP_B(2) and LEFT_TEMP_B(3).
