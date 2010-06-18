/*
 * Intra predict 8X8 luma block
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__INTRA_PRED_8X8_Y__)		// Make sure this is only included once
#define __INTRA_PRED_8X8_Y__

// Module name: intra_Pred_8X8_Y.asm
//
// Intra predict 8X8 luma block
//
//--------------------------------------------------------------------------
//  Input data:
//
//  REF_TOP:	Top reference data stored in BYTE with p[-1,-1] at REF_TOP(0,-1), p[-1,-1] and [15,-1] adjusted
//  REF_LEFT:	Left reference data stored in BYTE with p[-1,0] at REF_LEFT(0,2), REF_LEFT(0,1) (p[-1,-1]) adjusted
//	PRED_MODE:	Intra prediction mode stored in 4 LSBs
//	INTRA_PRED_AVAIL:	Top/Left available flag, (Bit0: Left, Bit1: Top)
//
//	Output data:
//
//	REG_INTRA_8X8_PRED: Predicted 8X8 block data
//--------------------------------------------------------------------------

#define INTRA_REF	REG_INTRA_TEMP_1
#define REF_TMP		REG_INTRA_TEMP_2

intra_Pred_8x8_Y:

//	Reference sample filtering
//
	// Set up boundary pixels for unified filtering
	mov (1)		REF_TOP(0,16)<1>	REF_TOP(0,15)REGION(1,0)	// p[16,-1] = p[15,-1]
	mov	(8)		REF_LEFT(0,2+8)<1>	REF_LEFT(0,2+7)REGION(1,0)	// p[-1,8] = p[-1,7]

	// Top reference sample filtering (!!Consider instruction compression later)
	add (16)	acc0<1>:w	REF_TOP(0,-1)REGION(16,1)	2:w		// p[x-1,-1]+2
	mac (16)	acc0<1>:w	REF_TOP(0)REGION(16,1)		2:w		// p[x-1,-1]+2*p[x,-1]+2
	mac (16)	acc0<1>:w	REF_TOP(0,1)REGION(16,1)	1:w		// p[x-1,-1]+2*p[x,-1]+p[x+1,-1]+2
	shr	(16)	REF_TMP<1>:w	acc0:w	2:w		// (p[x-1,-1]+2*p[x,-1]+p[x+1,-1]+2)>>2

	// Left reference sample filtering
	add (16)	acc0<1>:w	REF_LEFT(0)REGION(16,1)		2:w		// p[-1,y-1]+2
	mac (16)	acc0<1>:w	REF_LEFT(0,1)REGION(16,1)	2:w		// p[-1,y-1]+2*p[-1,y]+2
	mac (16)	acc0<1>:w	REF_LEFT(0,2)REGION(16,1)	1:w		// p[-1,y-1]+2*p[-1,y]+p[-1,y+1]+2
	shr	(16)	INTRA_REF<1>:w	acc0:w	2:w		// (p[-1,y-1]+2*p[-1,y]+p[-1,y+1]+2)>>2

	// Re-assign filtered reference samples
	mov	(16)	REF_TOP(0)<1>	REF_TMP<32;16,2>:ub			// p'[x,-1], x=0...15
	mov	(8)		REF_LEFT(0)<1>	INTRA_REF.2<16;8,2>:ub		// p'[-1,y], y=0...7
	mov	(1)		REF_TOP(0,-1)<1>	INTRA_REF<0;1,0>:ub		// p'[-1,-1]

//	Select intra_8x8 prediction mode
//
	and	(1)	PINTRAPRED_Y<1>:w	PRED_MODE<0;1,0>:w	0x0F:w
	// WA for "jmpi" restriction
	mov (1)	REG_INTRA_TEMP_1<1>:ud	r[PINTRAPRED_Y, INTRA_8X8_OFFSET]:ub
	jmpi (1) REG_INTRA_TEMP_1<0;1,0>:d

// Mode 0
#define	PTMP	a0.6
#define PTMP_D	a0.3
INTRA_8X8_VERTICAL:
    $for(0,0; <4; 1,32) {
	add.sat (16)	r[PPREDBUF_Y,%2]<2>:ub	r[PERROR,%2]<16;16,1>:w	REF_TOP(0)<0;8,1>
	}
	RETURN

// Mode 1
INTRA_8X8_HORIZONTAL:
    $for(0,0; <8; 2,32) {
	add.sat (16)	r[PPREDBUF_Y,%2]<2>:ub	r[PERROR,%2]<16;16,1>:w	REF_LEFT(0,%1)<1;8,0>
	}
	RETURN

// Mode 2
INTRA_8X8_DC:
// Rearrange reference samples for unified DC prediction code
//
    and.nz.f0.0 (16)	NULLREG		INTRA_PRED_AVAIL<0;1,0>:w	2:w	// Top macroblock available for intra prediction?
    and.nz.f0.1 (8)		NULLREG		INTRA_PRED_AVAIL<0;1,0>:w	1:w	// Left macroblock available for intra prediction?
	(-f0.0.any16h) mov (16)	REF_TOP_W(0)<1>	0x8080:uw
	(-f0.1.any8h) mov (8)	REF_LEFT(0)<1>	REF_TOP(0)REGION(8,1)
	(-f0.0.any8h) mov (8)	REF_TOP(0)<1>	REF_LEFT(0)REGION(8,1)

// Perform DC prediction
//
	add (8)		PRED_YW(15)<1>	REF_TOP(0)REGION(8,1)	REF_LEFT(0)REGION(8,1)
	add (4)		PRED_YW(15)<1>	PRED_YW(15)REGION(4,1)	PRED_YW(15,4)REGION(4,1)
	add (2)		PRED_YW(15)<1>	PRED_YW(15)REGION(2,1)	PRED_YW(15,2)REGION(2,1)
	add (16)	acc0<1>:w		PRED_YW(15)REGION(1,0)	PRED_YW(15,1)REGION(1,0)
	add	(16)	acc0<1>:w		acc0:w	8:w
	shr (16)	REG_INTRA_TEMP_0<1>:w	acc0:w	4:w

	// Add error block
    $for(0,0; <4; 1,32) {
	add.sat (16)	r[PPREDBUF_Y,%2]<2>:ub	r[PERROR,%2]<16;16,1>:w	REG_INTRA_TEMP_0<16;16,1>:w
	}
	RETURN

// Mode 3
INTRA_8X8_DIAG_DOWN_LEFT:
	mov	(8)		REF_TOP(0,16)<1>	REF_TOP(0,15)REGION(8,1)	// p[16,-1] = p[15,-1]
	add (16)	acc0<1>:w		REF_TOP(0,2)REGION(16,1)	2:w		// p[x+2]+2
	mac (16)	acc0<1>:w		REF_TOP(0,1)REGION(16,1)	2:w		// 2*p[x+1]+p[x+2]+2
	mac (16)	acc0<1>:w		REF_TOP(0)REGION(16,1)		1:w		// p[x]+2*p[x+1]+p[x+2]+2
	shr (16)	REG_INTRA_TEMP_0<1>:w	acc0<16;16,1>:w		2:w		// (p[x]+2*p[x+1]+p[x+2]+2)>>2

	// Add error block
    $for(0,0; <8; 2,32) {
	add.sat (16)	r[PPREDBUF_Y,%2]<2>:ub	r[PERROR,%2]<16;16,1>:w	REG_INTRA_TEMP_0.%1<1;8,1>:w
	}
	RETURN

// Mode 4
INTRA_8X8_DIAG_DOWN_RIGHT:
#define INTRA_REF	REG_INTRA_TEMP_1
#define REF_TMP		REG_INTRA_TEMP_2

//	Set inverse shift count
	shl	(4)		REF_TMP<1>:ud	REF_LEFT_D(0,1)REGION(1,0)	INV_SHIFT<4;4,1>:b	// Reverse order bottom 4 pixels of left ref.
	shl	(4)		REF_TMP.4<1>:ud	REF_LEFT_D(0)REGION(1,0)	INV_SHIFT<4;4,1>:b	// Reverse order top 4 pixels of left ref.
	mov	(8)		INTRA_REF<1>:ub	REF_TMP.3<32;8,4>:ub
	mov	(16)	INTRA_REF.8<1>:ub	REF_TOP(0,-1)REGION(16,1)	// INTRA_REF holds all reference data

	add (16)	acc0<1>:w		INTRA_REF.2<16;16,1>:ub		2:w		// p[x+2]+2
	mac (16)	acc0<1>:w		INTRA_REF.1<16;16,1>:ub		2:w		// 2*p[x+1]+p[x+2]+2
	mac (16)	acc0<1>:w		INTRA_REF<16;16,1>:ub		1:w		// p[x]+2*p[x+1]+p[x+2]+2
	shr (16)	INTRA_REF<1>:w	acc0<16;16,1>:w				2:w		// (p[x]+2*p[x+1]+p[x+2]+2)>>2

//	Store data in reversed order
	add (2)		PBWDCOPY_8<1>:w	INV_TRANS48<2;2,1>:b	INTRA_TEMP_1*GRFWIB:w	// Must match with INTRA_REF

	// Add error block
    $for(0,96; <8; 2,-32) {
	add.sat (16)	r[PPREDBUF_Y,%2]<2>:ub	r[PBWDCOPY_8,%1*2]<8,1>:w	r[PERROR,%2]<16;16,1>:w
	}
	RETURN

// Mode 5
INTRA_8X8_VERT_RIGHT:
#define INTRA_REF	REG_INTRA_TEMP_1
#define REF_TMP		REG_INTRA_TEMP_2
#define REF_TMP1	REG_INTRA_TEMP_3

//	Set inverse shift count
	shl	(4)		REF_TMP<1>:ud	REF_LEFT_D(0,1)REGION(1,0)	INV_SHIFT<4;4,1>:b	// Reverse order bottom 4 pixels of left ref.
	shl	(4)		REF_TMP.4<1>:ud	REF_LEFT_D(0)REGION(1,0)	INV_SHIFT<4;4,1>:b	// Reverse order top 4 pixels of left ref.
	mov	(8)		INTRA_REF<1>:ub	REF_TMP.3<32;8,4>:ub
	mov	(16)	INTRA_REF.8<1>:ub	REF_TOP(0,-1)REGION(16,1)	// INTRA_REF holds all reference data

	// Even rows
	avg (16)	PRED_YW(14)<1>	INTRA_REF.8<16;16,1>	INTRA_REF.9<16;16,1>	// avg(p[x-1],p[x])
	// Odd rows
	add (16)	acc0<1>:w		INTRA_REF.3<16;16,1>:ub		2:w		// p[x]+2
	mac (16)	acc0<1>:w		INTRA_REF.2<16;16,1>:ub		2:w		// 2*p[x-1]+p[x]+2
	mac (16)	acc0<1>:w		INTRA_REF.1<16;16,1>:ub		1:w		// p[x-2]+2*p[x-1]+p[x]+2
	shr (16)	REF_TMP<1>:w	acc0:w	2:w		// (p[x-2]+2*p[x-1]+p[x]+2)>>2

	mov	(8)		INTRA_REF<1>:ub		REF_TMP<16;8,2>:ub		// Keep zVR = -1,-2,-3,-4,-5,-6,-7 sequencially
	mov	(8)		INTRA_REF.6<2>:ub	REF_TMP.12<16;8,2>:ub	// Keep zVR = -1,1,3,5,7,9,11,13 at even byte
	mov	(8)		INTRA_REF.7<2>:ub	PRED_Y(14)REGION(8,2)	// Combining zVR = 0,2,4,6,8,10,12,14 at odd byte

	add (2)		PBWDCOPY_8<1>:w	INV_TRANS8<2;2,1>:b	INTRA_TEMP_1*GRFWIB:w	// Must match with INTRA_REF

	// Add error block
    $for(0,96; <8; 2,-32) {
	add.sat (16)	r[PPREDBUF_Y,%2]<2>:ub	r[PBWDCOPY_8,%1]<8,2>:ub	r[PERROR,%2]<16;16,1>:w
	}
	RETURN

// Mode 6
INTRA_8X8_HOR_DOWN:
//	Set inverse shift count
	shl	(4)		REF_TMP<1>:ud	REF_LEFT_D(0,1)REGION(1,0)	INV_SHIFT<4;4,1>:b	// Reverse order bottom 4 pixels of left ref.
	shl	(4)		REF_TMP.4<1>:ud	REF_LEFT_D(0)REGION(1,0)	INV_SHIFT<4;4,1>:b	// Reverse order top 4 pixels of left ref.
	mov	(8)		INTRA_REF<1>:ub	REF_TMP.3<16;4,4>:ub
	mov	(16)	INTRA_REF.8<1>:ub	REF_TOP(0,-1)REGION(16,1)	// INTRA_REF holds all reference data

	// Odd pixels
	add (16)	acc0<1>:w	INTRA_REF.2<16;16,1>:ub		2:w		// p[y]+2
	mac (16)	acc0<1>:w	INTRA_REF.1<16;16,1>:ub		2:w		// 2*p[y-1]+p[y]+2
	mac (16)	acc0<1>:w	INTRA_REF.0<16;16,1>:ub		1:w		// p[y-2]+2*p[y-1]+p[y]+2
	shr (16)	PRED_YW(14)<1>	acc0:w	2:w		// (p[y-2]+2*p[y-1]+p[y]+2)>>2
	// Even pixels
	avg (16)	INTRA_REF<1>:w	INTRA_REF<16;16,1>:ub	INTRA_REF.1<16;16,1>:ub	// avg(p[y-1],p[y])

	mov	(8)		INTRA_REF.1<2>:ub	PRED_Y(14)REGION(8,2)		// Combining odd pixels to form byte type
	mov	(8)		INTRA_REF.16<1>:ub	PRED_Y(14,16)REGION(8,2)	// Keep zVR = -2,-3,-4,-5,-6,-7 unchanged
	// Now INTRA_REF.0 - INTRA_REF.21 contain predicted data

	add (2)		PBWDCOPY_8<1>:w	INV_TRANS48<2;2,1>:b	INTRA_TEMP_1*GRFWIB:w	// Must match with INTRA_REF

	// Add error block
    $for(0,96; <13; 4,-32) {
	add.sat (16)	r[PPREDBUF_Y,%2]<2>:ub	r[PBWDCOPY_8,%1]<8,1>:ub	r[PERROR,%2]<16;16,1>:w
	}
	RETURN

// Mode 7
INTRA_8X8_VERT_LEFT:
	// Even rows
	avg (16)		PRED_YW(14)<1>	REF_TOP(0)REGION(16,1)	REF_TOP(0,1)REGION(16,1)	// avg(p[x],p[x+1])
	// Odd rows
	add (16)		acc0<1>:w		REF_TOP(0,2)REGION(16,1)	2:w		// p[x+2]+2
	mac (16)		acc0<1>:w		REF_TOP(0,1)REGION(16,1)	2:w		// 2*p[x+1]+p[x+2]+2
	mac (16)		acc0<1>:w		REF_TOP(0)REGION(16,1)		1:w		// p[x]+2*p[x+1]+p[x+2]+2
	shr (16)		PRED_YW(15)<1>	acc0<1;8,1>:w	2:w		// (p[x]+2*p[x+1]+p[x+2]+2)>>2

	// Add error block
    $for(0,0; <4; 1,32) {
	add.sat (16)	r[PPREDBUF_Y,%2]<2>:ub	PRED_YW(14,%1)<16;8,1>	r[PERROR,%2]<16;16,1>:w
	}
	RETURN

// Mode 8
INTRA_8X8_HOR_UP:
//	Set extra left reference pixels for unified prediction
	mov	(8)		REF_LEFT(0,8)<1>	REF_LEFT(0,7)REGION(1,0)	// Copy p[-1,7] to p[-1,y],y=8...15

	// Even pixels
	avg (16)	PRED_YW(14)<1>	REF_LEFT(0)REGION(16,1)	REF_LEFT(0,1)REGION(16,1)	// avg(p[y],p[y+1])
	// Odd pixels
	add (16)	acc0<1>:w		REF_LEFT(0,2)REGION(16,1)	2:w		// p[y+2]+2
	mac (16)	acc0<1>:w		REF_LEFT(0,1)REGION(16,1)	2:w		// 2*p[y+1]+p[y+2]+2
	mac (16)	acc0<1>:w		REF_LEFT(0)REGION(16,1)		1:w		// p[y]+2*p[y+1]+p[y+2]+2
	shr (16)	PRED_YW(15)<1>	acc0<1;8,1>:w	2:w		// (p[y]+2*p[y+1]+p[y+2]+2)>>2

	// Merge even/odd pixels
	// The predicted data need to be stored in byte type (22 bytes are required)
	mov (16)	PRED_Y(14,1)<2>	PRED_Y(15)REGION(16,2)

	// Add error block
    $for(0,0; <4; 1,32) {
	add.sat (16)	r[PPREDBUF_Y,%2]<2>:ub	PRED_Y(14,%1*4)<2;8,1>	r[PERROR,%2]<16;16,1>:w
	}
	RETURN

// End of intra_Pred_8X8_Y

#endif	// !defined(__INTRA_PRED_8X8_Y__)
