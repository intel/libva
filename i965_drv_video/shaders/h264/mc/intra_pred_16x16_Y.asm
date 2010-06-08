/*
 * Intra predict 16x16 luma block
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: intra_Pred_16x16_Y.asm
//
// Intra predict 16x16 luma block
//
	and	(1)	PINTRAPRED_Y<1>:w	INTRA_PRED_MODE(0)REGION(1,0)	0x0F:w
	// WA for "jmpi" restriction
	mov (1)	REG_INTRA_TEMP_1<1>:ud	r[PINTRAPRED_Y, INTRA_16X16_OFFSET]:ub
	jmpi (1) REG_INTRA_TEMP_1<0;1,0>:d

// Mode 0
INTRA_16x16_VERTICAL:
    $for(0; <16; 2) {
	mov (32)	PRED_YW(%1)<1>	INTRA_REF_TOP(0) {Compr}
	}
	jmpi (1) End_intra_Pred_16x16_Y

// Mode 1
INTRA_16x16_HORIZONTAL:
	mov (1)		PREF_LEFT_UD<1>:ud	INTRA_REF_LEFT_ID*GRFWIB*0x00010001+0x00040000:ud	// Set address registers for instruction compression
    $for(0,0; <16; 2,8) {
	mov (32)	PRED_YW(%1)<1>	r[PREF_LEFT,%2+3]<0;1,0>:ub {Compr}	// Actual left column reference data start at offset 3
	}
	jmpi (1) End_intra_Pred_16x16_Y

// Mode 2
INTRA_16x16_DC:
    and.nz.f0.0 (8)	NULLREG		REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_UP_AVAIL_FLAG:ud	// Top macroblock available for intra prediction?
    and (8)			acc0<1>:ud	REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_LEFT_TH_AVAIL_FLAG+INTRA_PRED_LEFT_BH_AVAIL_FLAG:ud	// Left macroblock available for intra prediction?
    xor.z.f0.1 (8)	NULLREG		acc0:ud	INTRA_PRED_LEFT_TH_AVAIL_FLAG+INTRA_PRED_LEFT_BH_AVAIL_FLAG:ud	// Left macroblock available for intra prediction?
// Rearrange reference samples for unified DC prediction code
//
	(-f0.0.any8h)	mov (8)	INTRA_REF_TOP_W(0)<1>	0x8080:uw
	(-f0.1.any8h)	mov (8)	INTRA_REF_LEFT(0)<4>	INTRA_REF_TOP(0)REGION(8,1)
	(-f0.1.any8h)	mov (8)	INTRA_REF_LEFT(1)<4>	INTRA_REF_TOP(0,8)REGION(8,1)
	(-f0.0.any8h)	mov (8)	INTRA_REF_TOP(0)<1>		INTRA_REF_LEFT(0)REGION(8,4)
	(-f0.0.any8h)	mov (8)	INTRA_REF_TOP(0,8)<1>	INTRA_REF_LEFT(1)REGION(8,4)	// Split due to HW limitation
// Perform DC prediction
//
	add (16)	PRED_YW(15)<1>	INTRA_REF_LEFT(0)REGION(8,4)	INTRA_REF_TOP(0)REGION(16,1)
	add (8)		PRED_YW(15)<1>	PRED_YW(15)REGION(8,1)	PRED_YW(15,8)REGION(8,1)
	add (4)		PRED_YW(15)<1>	PRED_YW(15)REGION(4,1)	PRED_YW(15,4)REGION(4,1)
	add (2)		PRED_YW(15)<1>	PRED_YW(15)REGION(2,1)	PRED_YW(15,2)REGION(2,1)
	add (32)	acc0<1>:w		PRED_YW(15)REGION(1,0)	PRED_YW(15,1)REGION(1,0) {Compr}	// Set up both acc0 and acc1
	add	(32)	acc0<1>:w		acc0:w	16:w {Compr}

    $for(0; <16; 2) {
	shr (32)	PRED_YW(%1)<1>	acc0:w	5:w {Compr}
	}
	jmpi (1) End_intra_Pred_16x16_Y

// Mode 3
INTRA_16x16_PLANE:
// Refer to H.264/AVC spec Section 8.3.3.4

#define A		REG_INTRA_TEMP_2.0		// All are WORD type
#define B		REG_INTRA_TEMP_3.0
#define C		REG_INTRA_TEMP_3.1
#define YP		REG_INTRA_TEMP_0		// Store intermediate results of c*(y-7). Make sure it's an even GRF
#define YP1		REG_INTRA_TEMP_1		// Store intermediate results of c*(y-7). Make sure it's an odd GRF, used in {Comp}
#define XP		REG_INTRA_TEMP_5		// Store intermediate results of a+b*(x-7)+16. Make sure it's an odd GRF

// First Calculate constants H and V
//	H1 = sum((-x'-1)*p[8+x',-1]), x'=0,1,...7
//	H2 =  sum((-x'-1)*p[6-x',-1]), x'=7,6,...0
//	H = -H1 + H2
//	The same calculation holds for V
//
	mul (8)	H1(0)<1>	INTRA_REF_TOP(0,8)REGION(8,1)		0x89ABCDEF:v
	mul (8)	H2(0)<1>	INTRA_REF_TOP(0,-1)REGION(8,1)		0xFEDCBA98:v

	mul (8)	V1(0)<1>	INTRA_REF_LEFT(0,8*4)REGION(8,4)	0x89ABCDEF:v
	mul (8)	V2(0)<1>	INTRA_REF_LEFT(0)REGION(8,4)		0x0FEDCBA9:v
	mul (1)	V2(0,7)<1>	INTRA_REF_TOP(0,-1)<0;1,0>	-8:w		// Replace 0*p[-1,7] with -8*p[-1,-1]
	// Now, REG_INTRA_TEMP_0 holds [H2, -H1] and REG_INTRA_TEMP_1 holds [V2, -V1]

	// Sum up [H2, -H1] and [V2, -V1] using instruction compression
	// ExecSize = 16 is restricted by B-spec for instruction compression
	// Actual intermediate results are in lower sub-registers after each summing step
	add	(16)	H1(0)<1>	-H1(0)	H2(0)	{Compr}	// Results in lower 8 WORDs
	add	(16)	H1(0)<1>	H1(0)	H1(0,4) {Compr}	// Results in lower 4 WORDs
	add	(16)	H1(0)<1>	H1(0)	H1(0,2) {Compr}	// Results in lower 2 WORDs
	add	(16)	H1(0)<1>	H1(0)	H1(0,1) {Compr}	// Results in lower 1 WORD

//	Calculate a, b, c and further derivations
	mov	(16)	acc0<1>:w	32:w
	mac	(2)		acc0<1>:w	H1(0)<16;1,0>	5:w
	shr	(2)		B<1>:w		acc0:w	6:w		// Done b,c
	mov	(16)	acc0<1>:w	16:w
	mac	(16)	acc0<1>:w	INTRA_REF_TOP(0,15)<0;1,0>	16:w
	mac	(16)	A<1>:w		INTRA_REF_LEFT(0,15*4)<0;1,0>	16:w	// A = a+16
	mac (16)	XP<1>:w		B<0;1,0>:w		XY_7<16;16,1>:b			// XP = A+b*(x-7)
	mul	(8)		YP<1>:w		C<0;1,0>:w		XY_7<16;8,2>:b			// YP = c*(y-7), even portion
	mul	(8)		YP1<1>:w	C<0;1,0>:w		XY_7_1<16;8,2>:b		// YP = c*(y-7), odd portion

//	Finally the intra_16x16 plane prediction
    $for(0,0; <16; 2,1) {
	add (32)	acc0<1>:w		XP<16;16,1>:w	YP.%2<16;16,0>:w {Compr}	// Set Width!= 1 to trick EU to use YP_1.%2 for 2nd instruction
	shr.sat (32)	PRED_Y(%1)<2>	acc0<16;16,1>:w	5:w {Compr}
	}

End_intra_Pred_16x16_Y:
// End of intra_Pred_16x16_Y
