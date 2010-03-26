/*
 * Intra predict 4 Intra_4x4 luma blocks
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__INTRA_PRED_4X4_Y_4__)		// Make sure this is only included once
#define __INTRA_PRED_4X4_Y_4__

// Module name: intra_Pred_4x4_Y_4.asm
//
// Intra predict 4 Intra_4x4 luma blocks
//
//--------------------------------------------------------------------------
//  Input data:
//
//  REF_TOP:	Top reference data stored in BYTE with p[-1,-1] at REF_TOP(0,-1)
//  REF_LEFT:	Left reference data stored in BYTE with p[-1,0] at REF_LEFT(0,0)
//	PRED_MODE:	Intra prediction mode stored in 4 words (4 LSB)
//	REG_INTRA_PRED_AVAIL:	Top/Left available flag, (Bit0: Left, Bit1: Top)
//
//--------------------------------------------------------------------------

#undef	INTRA_PRED_AVAIL
#undef	INTRA_REF
#undef	REF_LEFT_BACK
#undef	REF_TMP
#undef	REF_TMP1

#define	INTRA_PRED_AVAIL	REG_INTRA_TEMP_2.8
#define INTRA_REF			REG_INTRA_TEMP_2
#define	REF_LEFT_BACK		REG_INTRA_TEMP_8
#define	REF_TMP				REG_INTRA_TEMP_3
#define REF_TMP1			REG_INTRA_TEMP_4

intra_Pred_4x4_Y_4:

	mov	(8)		REF_LEFT_BACK<1>:ub	REF_LEFT(0)REGION(8,1)	// Store left referece data
//	Set up pointers to each intra_4x4 prediction mode
//
	and	(4)		PINTRA4X4_Y<1>:w	PRED_MODE<4;4,1>:w	0x0F:w
	add (4)		INTRA_4X4_MODE(0)	r[PINTRA4X4_Y, INTRA_4X4_OFFSET]<1,0>:ub	INTRA_MODE<4;4,1>:ub

//	Sub-block 0 *****************
	mov (1)		INTRA_PRED_AVAIL<1>:w	REG_INTRA_PRED_AVAIL<0;1,0>:w		// Top/Left neighbor available flags
	CALL_1(INTRA_4X4_MODE(0),1)

//	Add error data to predicted intra data
ADD_ERROR_SB0:
	add.sat (8)	r[PPREDBUF_Y,PREDSUBBLK0]<2>:ub	r[PERROR,ERRBLK0]<8;4,1>:w		REG_INTRA_4X4_PRED<8;8,1>:w		// Too bad indexed src can't
	add.sat (8)	r[PPREDBUF_Y,PREDSUBBLK0+16]<2>:ub	r[PERROR,ERRBLK0+32]<8;4,1>:w	REG_INTRA_4X4_PRED.8<8;8,1>:w	// cross 2 GRFs

//	Sub-block 1 *****************
	mov	(16)	REF_TOP0(0)<1>	REF_TOP0(0,4)REGION(8,1)		// Top reference data
	mov	(4)		REF_LEFT(0)<1>	r[PPREDBUF_Y,PREDSUBBLK0+6]<8;1,0>:ub	// New left referece data from sub-block 0
	or (1)		INTRA_PRED_AVAIL<1>:w	REG_INTRA_PRED_AVAIL<0;1,0>:w	1:w		// Left neighbor is available
	CALL_1(INTRA_4X4_MODE(0,1),1)

//	Add error data to predicted intra data
ADD_ERROR_SB1:
	add.sat (8)	r[PPREDBUF_Y,PREDSUBBLK1]<2>:ub	r[PERROR,ERRBLK1]<8;4,1>:w		REG_INTRA_4X4_PRED<8;8,1>:w		// Too bad indexed src can't
	add.sat (8)	r[PPREDBUF_Y,PREDSUBBLK1+16]<2>:ub	r[PERROR,ERRBLK1+32]<8;4,1>:w	REG_INTRA_4X4_PRED.8<8;8,1>:w	// cross 2 GRFs

//	Sub-block 2 *****************
	mov	(1)		REF_TOP0(0,3)<1>	REF_LEFT_BACK.3<0;1,0>:ub		// Top-left reference data from stored left referece data
	mov	(4)		REF_TOP0(0,4)<1>	r[PPREDBUF_Y,PREDSUBBLK0+24]REGION(4,2):ub	// Top reference data
	mov	(4)		REF_TOP0(0,8)<1>	r[PPREDBUF_Y,PREDSUBBLK0+24+32]REGION(4,2):ub	// Too bad indexed src can't cross 2 GRFs
	mov	(4)		REF_TOP0(0,12)<1>	r[PPREDBUF_Y,PREDSUBBLK0+30+32]REGION(1,0):ub	// Extended top-right reference data
	mov	(4)		REF_LEFT(0)<1>		REF_LEFT_BACK.4<4;4,1>:ub	// From stored left referece data
	or (1)		INTRA_PRED_AVAIL<1>:w	REG_INTRA_PRED_AVAIL<0;1,0>:w	2:w		// Top neighbor is available
	CALL_1(INTRA_4X4_MODE(0,2),1)

//	Add error data to predicted intra data
ADD_ERROR_SB2:
	add.sat (8)	r[PPREDBUF_Y,PREDSUBBLK2]<2>:ub	r[PERROR,ERRBLK2]<8;4,1>:w		REG_INTRA_4X4_PRED<8;8,1>:w		// Too bad indexed src can't
	add.sat (8)	r[PPREDBUF_Y,PREDSUBBLK2+16]<2>:ub	r[PERROR,ERRBLK2+32]<8;4,1>:w	REG_INTRA_4X4_PRED.8<8;8,1>:w	// cross 2 GRFs

//	Sub-block 3 *****************
	mov	(16)	REF_TOP0(0)<1>		REF_TOP0(0,4)REGION(8,1)	// Top reference data
	mov	(8)		REF_TOP0(0,8)<1>	REF_TOP0(0,7)<0;1,0>	// Extended top-right reference data
	mov	(4)		REF_LEFT(0)<1>	r[PPREDBUF_Y,PREDSUBBLK2+6]<8;1,0>:ub	// Left referece data from sub-block 0
	or (1)		INTRA_PRED_AVAIL<1>:w	REG_INTRA_PRED_AVAIL<0;1,0>:w	3:w		// Top/Left neighbor are available
	CALL_1(INTRA_4X4_MODE(0,3),1)

//	Add error data to predicted intra data
ADD_ERROR_SB3:
	add.sat (8)	r[PPREDBUF_Y,PREDSUBBLK3]<2>:ub	r[PERROR,ERRBLK3]<8;4,1>:w		REG_INTRA_4X4_PRED<8;8,1>:w		// Too bad indexed src can't
	add.sat (8)	r[PPREDBUF_Y,PREDSUBBLK3+16]<2>:ub	r[PERROR,ERRBLK3+32]<8;4,1>:w	REG_INTRA_4X4_PRED.8<8;8,1>:w	// cross 2 GRFs

	RETURN

//--------------------------------------------------------------------------
//  Actual module that performs Intra_4x4 prediction and construction
//
//  REF_TOP:		Top reference data stored in BYTE with p[-1,-1] at REF_TOP(0,-1)
//  REF_LEFT:		Left reference data stored in BYTE with p[-1,0] at REF_LEFT(0,0)
//	PINTRA4X4_Y:	Intra prediction mode
//	INTRA_PRED_AVAIL:	Top/Left available flag, (Bit0: Left, Bit1: Top)
//
//	Output data:
//
//	REG_INTRA_4X4_PRED: Predicted 4x4 block data stored in 1 GRF register
//--------------------------------------------------------------------------
intra_Pred_4x4_Y:
// Mode 0
INTRA_4X4_VERTICAL:
	mov (16)	REG_INTRA_4X4_PRED<1>:w	REF_TOP(0)<0;4,1>
	RETURN_1

// Mode 1
INTRA_4X4_HORIZONTAL:
	mov (16)	REG_INTRA_4X4_PRED<1>:w	REF_LEFT(0)<1;4,0>
	RETURN_1

// Mode 2
INTRA_4X4_DC:
// Rearrange reference samples for unified DC prediction code
//
    and.nz.f0.0 (16)	NULLREG		INTRA_PRED_AVAIL<0;1,0>:w	2:w  {Compr}
    and.nz.f0.1 (16)	NULLREG		INTRA_PRED_AVAIL<0;1,0>:w	1:w  {Compr}
	(-f0.0.any16h) mov (16)	REF_TOP_W(0)<1>	0x8080:uw				// Top macroblock not available for intra prediction
	(-f0.1.any8h) mov (8)	REF_LEFT(0)<1>	REF_TOP(0)REGION(8,1)	// Left macroblock not available for intra prediction
	(-f0.0.any8h) mov (8)	REF_TOP(0)<1>	REF_LEFT(0)REGION(8,1)	// Top macroblock not available for intra prediction
// Perform DC prediction
//
	add (4)		PRED_YW(15)<1>	REF_TOP(0)REGION(4,1)	REF_LEFT(0)REGION(4,1)
	add (2)		PRED_YW(15)<1>	PRED_YW(15)REGION(2,1)	PRED_YW(15,2)REGION(2,1)
	add (16)	acc0<1>:w		PRED_YW(15)REGION(1,0)	PRED_YW(15,1)REGION(1,0)
	add	(16)	acc0<1>:w		acc0:w	4:w
	shr (16)	REG_INTRA_4X4_PRED<1>:w	acc0:w	3:w
	RETURN_1

// Mode 3
INTRA_4X4_DIAG_DOWN_LEFT:
	mov	(8)		INTRA_REF<1>:ub	REF_TOP(0)REGION(8,1)		// Keep REF_TOP untouched for future use
	mov	(4)		INTRA_REF.8<1>:ub	REF_TOP(0,7)REGION(4,1)	// p[8,-1] = p[7,-1]
	add (8)		acc0<1>:w		INTRA_REF.2<8;8,1>	2:w		// p[x+2]+2
	mac (8)		acc0<1>:w		INTRA_REF.1<8;8,1>	2:w		// 2*p[x+1]+p[x+2]+2
	mac (8)		PRED_YW(15)<1>	INTRA_REF.0<8;8,1>	1:w		// p[x]+2*p[x+1]+p[x+2]+2

	shr (16)	REG_INTRA_4X4_PRED<1>:w	PRED_YW(15)<1;4,1>	2:w		// (p[x]+2*p[x+1]+p[x+2]+2)>>2
	RETURN_1

// Mode 4
INTRA_4X4_DIAG_DOWN_RIGHT:

//	Set inverse shift count
	shl	(4)		REF_TMP<1>:ud	REF_LEFT_D(0)REGION(1,0)	INV_SHIFT<4;4,1>:b
	mov	(8)		INTRA_REF.4<1>:ub	REF_TOP(0,-1)REGION(8,1)	// INTRA_REF holds all reference data
	mov	(4)		INTRA_REF<1>:ub	REF_TMP.3<16;4,4>:ub

	add (8)		acc0<1>:w		INTRA_REF.2<8;8,1>:ub	2:w		// p[x+2]+2
	mac (8)		acc0<1>:w		INTRA_REF.1<8;8,1>:ub	2:w		// 2*p[x+1]+p[x+2]+2
	mac (8)		INTRA_REF<1>:w	INTRA_REF<8;8,1>:ub		1:w		// p[x]+2*p[x+1]+p[x+2]+2

//	Store data in reversed order
	add (4)		PBWDCOPY_4<1>:w	INV_TRANS4<4;4,1>:b	INTRA_TEMP_2*GRFWIB:w	// Must match with INTRA_REF
	shr (16)	REG_INTRA_4X4_PRED<1>:w	r[PBWDCOPY_4]<4,1>:w	2:w
	RETURN_1

// Mode 5
INTRA_4X4_VERT_RIGHT:

//	Set inverse shift count
	shl	(4)		REF_TMP<1>:ud	REF_LEFT_D(0)REGION(1,0)	INV_SHIFT<4;4,1>:b
	mov	(8)		INTRA_REF.4<1>:ub	REF_TOP(0,-1)REGION(8,1)	// INTRA_REF holds all reference data
	mov	(4)		INTRA_REF<1>:ub	REF_TMP.3<16;4,4>:ub

	// Even rows
	avg (8)		PRED_YW(14)<1>	INTRA_REF.4<8;8,1>	INTRA_REF.5<8;8,1>	// avg(p[x-1],p[x])
	// Odd rows
	add (8)		acc0<1>:w		INTRA_REF.3<8;8,1>:ub		2:w		// p[x]+2
	mac (8)		acc0<1>:w		INTRA_REF.2<8;8,1>:ub		2:w		// 2*p[x-1]+p[x]+2
	mac (8)		acc0<1>:w		INTRA_REF.1<8;8,1>:ub		1:w		// p[x-2]+2*p[x-1]+p[x]+2
	shr (8)		INTRA_REF<1>:w	acc0:w	2:w		// (p[x-2]+2*p[x-1]+p[x]+2)>>2

	mov	(4)		INTRA_REF.2<2>:w	INTRA_REF.2<4;4,1>:w	// Keep zVR = -2,-3 unchanged
	mov	(4)		INTRA_REF.3<2>:w	PRED_YW(14)REGION(4,1)	// Combining even rows

	add (4)		PBWDCOPY_4<1>:w	INV_TRANS4<4;4,1>:b	INTRA_TEMP_2*GRFWIB:w	// Must match with INTRA_REF
	mov (16)	REG_INTRA_4X4_PRED<1>:w	r[PBWDCOPY_4]<4,2>:w
	RETURN_1

// Mode 6
INTRA_4X4_HOR_DOWN:
//	Set inverse shift count
	shl	(4)		REF_TMP<1>:ud	REF_LEFT_D(0)REGION(1,0)	INV_SHIFT<4;4,1>:b
	mov	(8)		INTRA_REF.4<1>:ub	REF_TOP(0,-1)REGION(8,1)	// INTRA_REF holds all reference data
	mov	(4)		INTRA_REF<1>:ub	REF_TMP.3<16;4,4>:ub

	// Even pixels
	avg (8)		PRED_YW(14)<1>	INTRA_REF<8;8,1>	INTRA_REF.1<8;8,1>	// avg(p[y-1],p[y])
	// Odd pixels
	add (8)		acc0<1>:w		INTRA_REF.2<8;8,1>:ub	2:w		// p[y]+2
	mac (8)		acc0<1>:w		INTRA_REF.1<8;8,1>:ub	2:w		// 2*p[y-1]+p[y]+2
	mac (8)		REF_TMP<1>:w	INTRA_REF.0<8;8,1>:ub	1:w		// p[y-2]+2*p[y-1]+p[y]+2
	shr (4)		INTRA_REF.1<2>:w	REF_TMP<4;4,1>:w	2:w		// (p[y-2]+2*p[y-1]+p[y]+2)>>2

	shr	(2)		INTRA_REF.8<1>:w	REF_TMP.4<2;2,1>:w	2:w		// Keep zVR = -2,-3 unchanged
	mov	(4)		INTRA_REF.0<2>:w	PRED_YW(14)REGION(4,1)	// Combining even pixels

	shl (4)		PBWDCOPY_4<1>:w	INV_TRANS4<4;4,1>:b	1:w		// Convert to WORD offset
	add (4)		PBWDCOPY_4<1>:w	PBWDCOPY_4<4;4,1>:w	INTRA_TEMP_2*GRFWIB:w	// Must match with INTRA_REF
	mov (16)	REG_INTRA_4X4_PRED<1>:w	r[PBWDCOPY_4]<4,1>:w
	RETURN_1

// Mode 7
INTRA_4X4_VERT_LEFT:
	// Even rows
	avg (8)		PRED_YW(14)<2>	REF_TOP(0)REGION(8,1)	REF_TOP(0,1)REGION(8,1)	// avg(p[x],p[x+1])
	// Odd rows
	add (8)		acc0<1>:w		REF_TOP(0,2)REGION(8,1)	2:w		// p[x+2]+2
	mac (8)		acc0<1>:w		REF_TOP(0,1)REGION(8,1)	2:w		// 2*p[x+1]+p[x+2]+2
	mac (8)		PRED_YW(15)<1>	REF_TOP(0)REGION(8,1)		1:w		// p[x]+2*p[x+1]+p[x+2]+2
	shr (8)		PRED_YW(14,1)<2>	PRED_YW(15)REGION(8,1)	2:w

	mov (16)	REG_INTRA_4X4_PRED<1>:w	PRED_YW(14)<1;4,2>
	RETURN_1

// Mode 8
INTRA_4X4_HOR_UP:
//	Set extra left reference pixels for unified prediction
	mov	(8)		REF_LEFT(0,4)<1>	REF_LEFT(0,3)REGION(1,0)	// Copy p[-1,3] to p[-1,y],y=4...7
	// Even pixels
	avg (8)		PRED_YW(14)<2>	REF_LEFT(0)REGION(8,1)	REF_LEFT(0,1)REGION(8,1)	// avg(p[y],p[y+1])
	// Odd pixels
	add (8)		acc0<1>:w		REF_LEFT(0,2)REGION(8,1)	2:w		// p[y+2]+2
	mac (8)		acc0<1>:w		REF_LEFT(0,1)REGION(8,1)	2:w		// 2*p[y+1]+p[y+2]+2
	mac (8)		PRED_YW(15)<1>	REF_LEFT(0)REGION(8,1)		1:w		// p[y]+2*p[y+1]+p[y+2]+2
	shr (8)		PRED_YW(14,1)<2>	PRED_YW(15)REGION(8,1)	2:w		// (p[y]+2*p[y+1]+p[y+2]+2)>>2

	mov (16)	REG_INTRA_4X4_PRED<1>:w	PRED_YW(14)<2;4,1>
	RETURN_1

// End of intra_Pred_4x4_Y_4

#endif	// !defined(__INTRA_PRED_4X4_Y_4__)
