/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//////////////////////////////////////////////////////////////////////////////////////////
//	Module name: TransposeNV12_4x16.asm
//	
//	Transpose a 4x16 internal planar to 16x4 internal planar block
//
//----------------------------------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Source region is :ub
//	SRC_YB:			SRC_YB Base=rxx ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// 8 GRFs
//	SRC_UW:			SRC_UB Base=rxx ElementSize=2 SrcRegion=REGION(8,1) Type=uw		// 4 GRFs
//
//  Temp buffer:
//	BUF_B:			BUF_B Base=rxx ElementSize=1 SrcRegion=REGION(16,1) Type=ub		// 8 GRFs
//	BUF_W:			BUF_W Base=rxx ElementSize=2 SrcRegion=REGION(8,1) Type=uw		// 4 GRFs
//
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDDB:w
#endif

// Transpose Y (4x16) right most 4 columns

// The first step
mov (16)	BUF_B(0,0)<1>		SRC_YB(0,0)<16;4,1>		// Read 2 rows, write 1 row
mov (16)	BUF_B(0,16)<1>		SRC_YB(2,0)<16;4,1>
mov (16)	BUF_B(1,0)<1>		SRC_YB(4,0)<16;4,1>
mov (16)	BUF_B(1,16)<1>		SRC_YB(6,0)<16;4,1>

// The second step
mov (16)	BUF_B(2,0)<1>		BUF_B(0,0)<32;8,4> 		// Read 2 rows, write 1 row
mov (16)	BUF_B(2,16)<1>		BUF_B(0,1)<32;8,4>
mov (16)	BUF_B(3,0)<1>		BUF_B(0,2)<32;8,4>
mov (16)	BUF_B(3,16)<1>		BUF_B(0,3)<32;8,4>

// Y is now transposed. the result is in BUF_B(2) and BUF_B(3).



// Transpose UV (4x8),  right most 2 columns in word
// Use BUF_W(0) as temp buf

// Src U 8x8 and V 8x8 are mixed. (each pix is specified as yx)
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|17 17 16 16 15 15 14 14 13 13 12 12 11 11 10 10 07 07 06 06 05 05 04 04 03 03 02 02 01 01 00 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|37 37 36 36 35 35 34 34 33 33 32 32 31 31 30 30 27 27 26 26 25 25 24 24 23 23 22 22 21 21 20 20|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|57 57 56 56 55 55 54 54 53 53 52 52 51 51 50 50 47 47 46 46 45 45 44 44 43 43 42 42 41 41 40 40|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|77 77 76 76 75 75 74 74 73 73 72 72 71 71 70 70 67 67 66 66 65 65 64 64 63 63 62 62 61 61 60 60|
//	+-----------------------+-----------------------+-----------------------+-----------------------+

//  First step 		(8)	<1>:w <==== <8;2,1>:w
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|71 71 70 70 61 61 60 60 51 51 50 50 41 41 40 40 31 31 30 30 21 21 20 20 11 11 10 10 01 01 00 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
mov (8)		BUF_W(0,0)<1>		SRC_UW(0,0)<8;2,1>
mov (8)		BUF_W(0,8)<1>		SRC_UW(2,0)<8;2,1>

//	Second step		(16) <1>:w <==== <1;8,2>:w
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|71 71 61 61 51 51 41 41 31 31 21 21 11 11 01 01 70 70 60 60 50 50 40 40 30 30 20 20 10 10 00 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
mov (16)	BUF_W(1,0)<1>		BUF_W(0,0)<1;8,2>

// UV are now transposed.  the result is in BUF_W(1).



//The first step
//mov (16)	BUF_B(0,0)<1>		SRC_UW(0,0)<8;2,1>		// Read 2 rows, write 1 row
// The second step
//mov (8)		SRC_UB(4,0)<1>		BUF_B(0,0)<16;8,2> 		// Read 1 row, write 1 row
//mov (8)		SRC_UB(4,8)<1>		BUF_B(0,1)<16;8,2> 		// Read 1 row, write 1 row

// Transpose V (8x8),  right most 2 columns
// The first step
//mov (16)	BUF_B(0,0)<1>		SRC_VB(0,1)<8;2,1>		// Read 2 rows, write 1 row
// The second step
//mov (8)		SRC_UB(4,16)<1>		BUF_B(0,0)<16;8,2> 		// Read 1 row, write 1 row
//mov (8)		SRC_UB(4,24)<1>		BUF_B(0,1)<16;8,2> 		// Read 1 row, write 1 row

// U and V are now transposed.  the result is in BUF_B(4).

