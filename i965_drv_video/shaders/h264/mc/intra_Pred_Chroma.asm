/*
 * Intra predict 8x8 chroma block
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__INTRA_PRED_CHROMA__)		// Make sure this is only included once
#define __INTRA_PRED_CHROMA__

// Module name: intra_Pred_Chroma.asm
//
// Intra predict 8x8 chroma block
//

	shr	(1)	PINTRAPRED_UV<1>:w	REG_INTRA_CHROMA_PRED_MODE<0;1,0>:ub	INTRA_CHROMA_PRED_MODE_SHIFT:w	// Bits 1:0 = intra chroma pred mode
	// WA for "jmpi" restriction
	mov (1)	REG_INTRA_TEMP_1<1>:d	r[PINTRAPRED_UV, INTRA_CHROMA_OFFSET]:b
	jmpi (1) REG_INTRA_TEMP_1<0;1,0>:d

// Mode 0
INTRA_CHROMA_DC:
    and.nz.f0.0 (8)		NULLREG		REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_UP_AVAIL_FLAG:ud	// Top macroblock available for intra prediction?

// Calculate DC values for sub-block 0 and 3
//
// Rearrange reference samples for unified DC prediction code
//	Need to check INTRA_PRED_LEFT_TH_AVAIL_FLAG for blk0 and INTRA_PRED_LEFT_BH_AVAIL_FLAG for blk3
// 
	(-f0.0.any8h)	mov (8)		INTRA_REF_TOP_W(0)<1>	0x8080:uw	// Up not available

    and.nz.f0.1 (4)	NULLREG		REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_LEFT_TH_AVAIL_FLAG:ud
	(-f0.1.any4h)	mov (4)		INTRA_REF_LEFT_W(0)<2>	INTRA_REF_TOP_W(0)REGION(4,1)	// Left top half macroblock not available for intra prediction
    and.nz.f0.1 (4)	NULLREG		REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_LEFT_BH_AVAIL_FLAG:ud
	(-f0.1.any4h)	mov (4)		INTRA_REF_LEFT_W(0,8)<2>	INTRA_REF_TOP_W(0,4)REGION(4,1)	// Left bottom half macroblock not available for intra prediction

	(-f0.0.any8h)	mov (8)		INTRA_REF_TOP_W(0)<1>	INTRA_REF_LEFT_W(0)REGION(8,2)	// Up not available
// Calculate DC prediction
//
	add (16)	PRED_UVW(0)<1>	INTRA_REF_TOP(0)REGION(16,1)	INTRA_REF_LEFT_UV(0)<4;2,1>	// Sum of top and left reference
	add (8)		PRED_UVW(0)<1>	PRED_UVW(0)<4;2,1>	PRED_UVW(0,2)<4;2,1>	// Sum of first half (blk #0) and second half (blk #3)

	add (8)		PRED_UVW(9)<1>	PRED_UVW(0)<0;2,1>	PRED_UVW(0,2)<0;2,1>	// Sum of blk #0
	add (8)		PRED_UVW(11,8)<1>	PRED_UVW(0,4)<0;2,1>	PRED_UVW(0,6)<0;2,1>	// Sum of blk #3

// Calculate DC values for sub-block 1 and 2
//
// Rearrange reference samples for unified DC prediction code
//
	// Blk #2
	(-f0.0.any4h)	mov (4)		INTRA_REF_TOP_W(0)<1>	0x8080:uw
	(f0.1.any4h)	mov (4)		INTRA_REF_TOP_W(0)<1>	INTRA_REF_LEFT_W(0,8)REGION(4,2)	// Always use available left reference
	(-f0.1.any4h)	mov (4)		INTRA_REF_LEFT_W(0,8)<2>	INTRA_REF_TOP_W(0)REGION(4,1)

	// Blk #1
    and.nz.f0.1 (4)	NULLREG		REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_LEFT_TH_AVAIL_FLAG:ud
	(-f0.1.any4h)	mov (4)		INTRA_REF_LEFT_W(0)<2>	0x8080:uw
	(f0.0.any4h)	mov (4)		INTRA_REF_LEFT_W(0)<2>	INTRA_REF_TOP_W(0,4)REGION(4,1)	// Always use available top reference
	(-f0.0.any4h)	mov (4)		INTRA_REF_TOP_W(0,4)<1>	INTRA_REF_LEFT_W(0)REGION(4,2)

// Calculate DC prediction
//
	add (8)	PRED_UVW(0)<1>		INTRA_REF_TOP(0)REGION(8,1)	INTRA_REF_LEFT_UV(0,16)<4;2,1>	// Sum of top and left reference for blk #2
	add (8)	PRED_UVW(0,8)<1>	INTRA_REF_LEFT_UV(0)<4;2,1>	INTRA_REF_TOP(0,8)REGION(8,1)	// Sum of top and left reference for blk #1
	add (8)	PRED_UVW(0)<1>		PRED_UVW(0)<4;2,1>		PRED_UVW(0,2)<4;2,1>	// Sum of first half (blk #2) and second half (blk #1)

	add (8)	PRED_UVW(9,8)<1>	PRED_UVW(0,4)<0;2,1>	PRED_UVW(0,6)<0;2,1>	// Sum of blk #1
	add (8)	PRED_UVW(11)<1>		PRED_UVW(0)<0;2,1>		PRED_UVW(0,2)<0;2,1>	// Sum of blk #2

// Now, PRED_UVW(9) holds sums for blks #0 and #1 and PRED_UVW(11) holds sums for blks #2 and #3
//
	add (32)	acc0<1>:w	PRED_UVW(9)REGION(16,1)		4:w {Compr}		// Add rounder
    $for(0; <4; 2) {
	shr (32)	PRED_UVW(%1)<1>	acc0:w		3:w {Compr}
	}

	add (32)	acc0<1>:w	PRED_UVW(11)REGION(16,1)	4:w {Compr}		// Add rounder
    $for(4; <8; 2) {
	shr (32)	PRED_UVW(%1)<1>	acc0:w		3:w {Compr}
	}
	jmpi (1)	End_of_intra_Pred_Chroma

// Mode 1
INTRA_CHROMA_HORIZONTAL:
	mov (1)		PREF_LEFT_UD<1>:ud	INTRA_REF_LEFT_ID*GRFWIB*0x00010001+0x00040000:ud	// Set address registers for instruction compression
    $for(0,0; <8; 2,8) {
	mov (32)	PRED_UVW(%1)<1>	r[PREF_LEFT,%2+2]<0;2,1>:ub {Compr}	// Actual left column reference data start at offset 2
	}
	jmpi (1)	End_of_intra_Pred_Chroma

// Mode 2
INTRA_CHROMA_VERTICAL:
    $for(0; <8; 2) {
	mov (32)	PRED_UVW(%1)<1>	INTRA_REF_TOP(0) {Compr}
	}
	jmpi (1)	End_of_intra_Pred_Chroma

// Mode 3
INTRA_Chroma_PLANE:
// Refer to H.264/AVC spec Section 8.3.4.4

#undef	C

#define A		REG_INTRA_TEMP_2.0		// All are WORD type
#define B		REG_INTRA_TEMP_3.0		// B[U] & B[V]
#define C		REG_INTRA_TEMP_3.2		// C[U] & C[V]
#define YP		REG_INTRA_TEMP_0		// Store intermediate results of c*(y-3). Make sure it's an even GRF
#define YP1		REG_INTRA_TEMP_1		// Store intermediate results of c*(y-3). Make sure it's an odd GRF
#define XP		REG_INTRA_TEMP_5		// Store intermediate results of a+b*(x-3)+16. Make sure it's an odd GRF

// First Calculate constants H and V
//	H1 = sum((x'+1)*p[4+x',-1]), x'=0,1,2,3
//	H2 =  sum((-x'-1)*p[2-x',-1]), x'=3,2,1,0
//	H = H1 + H2
//	The same calculation holds for V
//
	mul (8)	H1(0)<1>	INTRA_REF_TOP(0,8)REGION(8,1)	0x44332211:v
	mul (8)	H2(0)<1>	INTRA_REF_TOP(0,-2)REGION(8,1)	0xFFEEDDCC:v

	mul (8)	V1(0)<1>	INTRA_REF_LEFT_UV(0,4*4)<4;2,1>	0x44332211:v
	mul (8)	V2(0)<1>	INTRA_REF_LEFT_UV(0)<4;2,1>		0x00FFEEDD:v
	mul (2)	V2(0,6)<1>	INTRA_REF_TOP(0,-2)REGION(2,1)	-4:w		// Replace 0*p[-1,3] with -4*p[-1,-1]
	// Now, REG_INTRA_TEMP_0 holds [H2, H1] and REG_INTRA_TEMP_1 holds [V2, V1]

	// Sum up [H2, H1] and [V2, V1] using instruction compression
	// ExecSize = 16 is restricted by B-spec for instruction compression
	// Actual intermediate results are in lower sub-registers after each summing step
	add	(16)	H1(0)<1>	H1(0)	H2(0) {Compr}	// Results in lower 8 WORDs
	add	(16)	H1(0)<1>	H1(0)	H1(0,4) {Compr}	// Results in lower 4 WORDs
	add	(16)	H1(0)<1>	H1(0)	H1(0,2) {Compr}	// Results in lower 2 WORDs

//	Calculate a, b, c and further derivations
	mov	(16)	acc0<1>:w	32:w
	mac	(4)		acc0<1>:w	H1(0)<16;2,1>	34:w
	shr	(4)		B<1>:w		acc0:w	6:w		// Done b,c
	mov	(16)	acc0<1>:w	16:w
	mac	(16)	acc0<1>:w	INTRA_REF_TOP(0,7*2)<0;2,1>		16:w
	mac	(16)	A<1>:w		INTRA_REF_LEFT_UV(0,7*4)<0;2,1>	16:w	// A = a+16
	mac (16)	XP<1>:w		B<0;2,1>:w		XY_3<1;2,0>:b		// XP = A+b*(x-3)
	mul	(8)		YP<1>:w		C<0;2,1>:w		XY_3<2;2,0>:b		// YP = c*(y-3), Even portion
	mul	(8)		YP1<1>:w	C<0;2,1>:w		XY_3_1<2;2,0>:b	// YP = c*(y-3), Odd portion

//	Finally the intra_Chroma plane prediction
    $for(0; <8; 2) {
	add (32)	acc0<1>:w		XP<16;16,1>:w	YP.%1<0;2,1>:w {Compr}
	shr.sat (32)	PRED_UV(%1)<2>	acc0<16;16,1>:w	5:w {Compr}
	}

End_of_intra_Pred_Chroma:

// End of intra_Pred_Chroma

#endif	// !defined(__INTRA_PRED_CHROMA__)
