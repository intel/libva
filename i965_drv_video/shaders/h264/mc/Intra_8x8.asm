/*
 * Decode Intra_8x8 macroblock
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: Intra_8x8.asm
//
// Decoding of Intra_8x8 macroblock
//
//  $Revision: 12 $
//  $Date: 10/18/06 4:10p $
//

// ----------------------------------------------------
//  Main: Intra_8x8
// ----------------------------------------------------

#define	INTRA_8X8

.kernel Intra_8x8
INTRA_8x8:

#ifdef _DEBUG
// WA for FULSIM so we'll know which kernel is being debugged
mov (1) acc0:ud 0x01aa55a5:ud
#endif

#include "SetupForHWMC.asm"

#define		REG_INTRA_PRED_AVAIL	REG_INTRA_TEMP_4
#define		INTRA_PRED_AVAIL		REG_INTRA_TEMP_4.4

// Offset where 8x8 predicted data blocks are stored
#define	PREDBLK0	0*GRFWIB
#define	PREDBLK1	4*GRFWIB
#define	PREDBLK2	8*GRFWIB
#define	PREDBLK3	12*GRFWIB

#ifdef SW_SCOREBOARD

// Update "E" flag with "F" flag information
	mov (1)	REG_INTRA_TEMP_0<1>:w	REG_INTRA_PRED_AVAIL_FLAG_WORD<0;1,0>:w		// Store original Intra_Pred_Avail_Flag
	and.nz.f0.0 (1)	NULLREG	REG_MBAFF_PIC	MBAFF_PIC	// Is current MBAFF picture
	and.z.f0.1 (1) NULLREG	REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_LEFT_TH_AVAIL_FLAG	// Is "A" not available?
	(f0.0) and.z.f0.0 (1) NULLREG	REG_FIELD_MACROBLOCK_FLAG	FIELD_MACROBLOCK_FLAG	// Is current frame MB?
	(f0.1) and.nz.f0.1 (1) NULLREG	REG_INTRA_PRED_8X8_BLK2_AVAIL_FLAG	INTRA_PRED_8X8_BLK2_AVAIL_FLAG	// Is "F" flag set?
	(f0.0.allv) or (1)	REG_INTRA_PRED_AVAIL_FLAG_WORD<1>:w	REG_INTRA_PRED_AVAIL_FLAG_WORD<0;1,0>:w	INTRA_PRED_LEFT_BH_AVAIL_FLAG	// Set "E" to 1 if all conditions meet

    CALL(scoreboard_start_intra,1)
	mov (1)	REG_INTRA_PRED_AVAIL_FLAG_WORD<1>:w	REG_INTRA_TEMP_0<0;1,0>:w		// Restore original Intra_Pred_Avail_Flag
#endif

#ifdef SW_SCOREBOARD    
	wait	n0:ud		//	Now wait for scoreboard to response
#endif

//
//  Decode Y blocks
//
//	Load reference data from neighboring macroblocks
    CALL(load_Intra_Ref_Y,1)

	mov	(1)	PERROR<1>:w	ERRBUF*GRFWIB:w			// Pointer to macroblock error data
	mov	(1)	PDECBUF_UD<1>:ud	0x00010001*PREDBUF*GRFWIB+0x00100000:ud	// Pointers to predicted data
	shr (2)	REG_INTRA_PRED_AVAIL<1>:w	REG_INTRA_PRED_AVAIL_FLAG_BYTE<0;1,0>:ub	0x40:v
	
#if 1
	mov (4) REF_LEFT_D(0,0)<1>	0:ud		// This is to make validation easier. Without it, DRAM mismatch occurs.
#endif

//	Intra predict Intra_8x8 luma blocks
//
//	Sub-macroblock 0 *****************
	mov	(16)	REF_TOP_W(0)<1>	REG_INTRA_REF_TOP<16;16,1>:w		// Copy entire top reference data
	and.nz.f0.0 (8)	NULLREG		REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_UP_LEFT_AVAIL_FLAG	// Is "D" available?
	(-f0.0) mov (1)	REF_TOP(0,-1)<1>	INTRA_REF_TOP(0)REGION(1,0)		// p[-1,-1] = p[0,-1]

	mov	(8)		REF_LEFT(0,2)<1>	INTRA_REF_LEFT(0)	// Left reference data, (leave 2 for reference filtering)
	(-f0.0) mov (1)		REF_LEFT(0,1)<1>	INTRA_REF_LEFT(0)REGION(1,0)			// p[-1,-1]=p[-1,0]
	(f0.0.any2h)  mov (2)		REF_LEFT(0,0)<1>	INTRA_REF_TOP(0,-1)REGION(1,0)	// p'[-1,y] (y=0,1) = p[-1,-1]
	and.nz.f0.1 (1)	NULLREG		REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_UP_AVAIL_FLAG	// Is "B" available?
	(f0.1) mov	(1)		REF_LEFT(0,0)<1>	INTRA_REF_TOP(0,0)REGION(1,0)	// p[0,-1] for left filtering
	and.nz.f0.1 (1)	NULLREG		REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_LEFT_TH_AVAIL_FLAG	// Is "A" available?
	(-f0.1) mov	(1)		REF_LEFT(0,2)<1>	INTRA_REF_TOP(0,-1)REGION(1,0)	// p'[-1,2] = p[-1,-1]

	and	(1)		PRED_MODE<1>:w	INTRA_PRED_MODE(0)REGION(1,0)	0x0F:w		// Intra pred mode for current block
	mov (1)		INTRA_PRED_AVAIL<1>:w	REG_INTRA_PRED_AVAIL<0;1,0>:w		// Top/Left neighbor available flags
	CALL(intra_Pred_8x8_Y,1)
    add (1)		PERROR<1>:w	PERROR<0;1,0>:w	0x0080:w	// Pointers to next error block

//	Sub-macroblock 1 *****************
	mov	(16)	REF_TOP0(0)<1>	INTRA_REF_TOP(0,4)	// Top reference data
	and.nz.f0.1 (8)	NULLREG		REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_UP_RIGHT_AVAIL_FLAG	// Is "C" available?
	(f0.1.any8h)  mov (8)	REF_TOP(0,8)<1>	INTRA_REF_TOP(0,16)<8;8,1>		// Take data from "C"
	(-f0.1.any8h) mov (8)	REF_TOP(0,8)<1>	INTRA_REF_TOP(0,15)REGION(1,0)	// Repeat last pixel from "B"

	mov	(4)		REF_LEFT(0,2)<1>	DEC_Y(0,14)<16;1,0>		// Left reference data (top half) (leave 2 for reference filtering)
	mov	(4)		REF_LEFT(0,6)<1>	DEC_Y(2,14)<16;1,0>		// Left reference data (bottom half)
	mov	(2)		REF_LEFT(0,0)<1>	INTRA_REF_TOP(0,7)REGION(1,0)		// p'[-1,y] (y=0,1) = p[-1,-1]
	and.nz.f0.1 (1)	NULLREG		REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_UP_AVAIL_FLAG	// Is "B" available?
	(f0.1)  mov	(1)		REF_LEFT(0,0)<1>	INTRA_REF_TOP(0,8)REGION(1,0)	// p[-1,-1] for left filtering
	(-f0.1) mov	(1)		REF_LEFT(0,1)<1>	DEC_Y(0,14)REGION(1,0)	// p[-1,-1] = p[-1,0]

	shr	(1)		PRED_MODE<1>:w	INTRA_PRED_MODE(0)REGION(1,0)	4:w		// Intra pred mode for current block
	add	(2)		PPREDBUF_Y<1>:w	PPREDBUF_Y<2;2,1>:w	4*GRFWIB:w			// Pointer to predicted sub-macroblock 1
	or (1)		INTRA_PRED_AVAIL<1>:w	REG_INTRA_PRED_AVAIL<0;1,0>:w	1:w		// Left neighbor is available
	CALL(intra_Pred_8x8_Y,1)
    add (1)		PERROR<1>:w	PERROR<0;1,0>:w	0x0080:w	// Pointers to next error block

//	Pack constructed data from word-aligned to byte-aligned format and interlace Y0 and Y1(every two Y rows)
//	to speed up save_8x8_Y module later
//	PPREDBUF_Y now points to sub-macroblock Y1
	mov (32)	r[PPREDBUF_Y,-PREDBLK1]<1>:ub		DEC_Y(0)<32;16,2> {Compr}	// First 4 Y0 rows
	mov (32)	r[PPREDBUF_Y,0-PREDBLK1+32]<1>:ub	DEC_Y(4)<32;16,2> {Compr}	// First 4 Y1 rows
	mov (32)	r[PPREDBUF_Y,0-PREDBLK1+64]<1>:ub	DEC_Y(2)<32;16,2> {Compr}	// Second 4 Y0 rows
	mov (32)	r[PPREDBUF_Y,0-PREDBLK1+96]<1>:ub	DEC_Y(6)<32;16,2> {Compr}	// Second 4 Y1 rows

//	Sub-macroblock 2 *****************
//	Intra_8x8 special available flag handling
	and.nz.f0.0 (1)	NULLREG	REG_MBAFF_PIC	MBAFF_PIC	// Is current MBAFF picture
	and.z.f0.1 (1) NULLREG	REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_LEFT_TH_AVAIL_FLAG	// Is "A" not available?
	(f0.0) and.z.f0.0 (1) NULLREG	REG_FIELD_MACROBLOCK_FLAG	FIELD_MACROBLOCK_FLAG	// Is current frame MB?
	(f0.1) and.nz.f0.1 (1) NULLREG	REG_INTRA_PRED_8X8_BLK2_AVAIL_FLAG	INTRA_PRED_8X8_BLK2_AVAIL_FLAG	// Is special intra_8x8 available flag set?
	(f0.0.allv) mov (1)	REF_TOP(0,-1)<1>	INTRA_REF_LEFT0(0,31)REGION(1,0)	// Top-left reference data
	(f0.0.allv) jmpi (1)	INTRA_8x8_BLK2
//	Done intra_8x8 special available flag handling

	and.nz.f0.0 (8)	NULLREG	REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_LEFT_TH_AVAIL_FLAG	// Is top-half "A" available?
	(f0.0.any4h) mov (4)	REF_TOP0(0)<1>		INTRA_REF_LEFT0(0,28)REGION(4,1)	// Top-left reference data
	(-f0.0) mov (1)	REF_TOP(0,-1)<1>	DEC_Y(2,24)REGION(1,0)	// p[-1,-1] = p[0,-1]
INTRA_8x8_BLK2:
	mov	(8)		REF_TOP(0)<1>		DEC_Y(2,24)REGION(8,1)		// Top reference data
	mov	(8)		REF_TOP(0,8)<1>		DEC_Y(3,24)REGION(8,1)		// Top reference data

	mov	(8)		REF_LEFT(0,2)<1>	INTRA_REF_LEFT(1)			// Left reference data,  (leave 2 for reference filtering)
	mov (1)		REF_LEFT(0,0)<1>	DEC_Y(2,24)REGION(1,0)		// p'[-1,0] = p[0,-1] since "B" is always available
	(f0.0) mov	(1)	REF_LEFT(0,1)<1>	INTRA_REF_LEFT(0,28)REGION(1,0)	// p[-1,1] = p[-1,-1] if top-half "A" available
	(-f0.0) mov (1)	REF_LEFT(0,1)<1>	INTRA_REF_LEFT(1)REGION(1,0)	// p[-1,1] = p[-1,0]
	and.nz.f0.1 (1)	NULLREG	REG_INTRA_PRED_AVAIL_FLAG	INTRA_PRED_LEFT_BH_AVAIL_FLAG	// Is bottom-half "A" available?
	(-f0.1) mov	(1)	REF_LEFT(0,2)<1>	INTRA_REF_LEFT(0,28)REGION(1,0)	// p'[-1,2] = p[-1,-1]

	and	(1)		PRED_MODE<1>:w			INTRA_PRED_MODE(0,1)REGION(1,0)	0x0F:w	// Intra pred mode for current block
	or (1)		INTRA_PRED_AVAIL<1>:w	REG_INTRA_PRED_AVAIL.1<0;1,0>:w	2:w		// Top neighbor is available
	CALL(intra_Pred_8x8_Y,1)
    add (1)		PERROR<1>:w	PERROR<0;1,0>:w	0x0080:w	// Pointers to next error block

//	Sub-macroblock 3 *****************
	mov	(4)		REF_TOP0(0)<1>		DEC_Y(2,28)REGION(4,1)		// Top-left reference data
	mov	(8)		REF_TOP(0)<1>		DEC_Y(3,24)REGION(8,1)		// Top reference data
	mov	(16)	REF_TOP(0,8)<1>		DEC_Y(3,31)REGION(1,0)		// Top-right reference data

	mov (4)		REF_LEFT(0,2)<1>	DEC_Y(4,14)<16;1,0>		// Left reference data (top half) (leave 2 for reference filtering)
	mov (4)		REF_LEFT(0,6)<1>	DEC_Y(6,14)<16;1,0>		// Left reference data (bottom half)
	mov (1)		REF_LEFT(0,0)<1>	DEC_Y(3,24)REGION(1,0)	// p[-1,0] = p[0,-1]
	mov (1)		REF_LEFT(0,1)<1>	DEC_Y(2,31)REGION(1,0)	// p[-1,1] = p[-1,-1]

	shr	(1)		PRED_MODE<1>:w	INTRA_PRED_MODE(0,1)REGION(1,0)	4:w		// Intra pred mode for current block
	add	(2)		PPREDBUF_Y<1>:w	PPREDBUF_Y<2;2,1>:w	4*GRFWIB:w	// Pointer to predicted sub-macroblock 3
	or (1)		INTRA_PRED_AVAIL<1>:w	REG_INTRA_PRED_AVAIL<0;1,0>:w	3:w		// Top and Left neighbor are available
	CALL(intra_Pred_8x8_Y,1)

//	Pack constructed data from word-aligned to byte-aligned format
//	to speed up save_8x8_Y module later
//	PPREDBUF_Y now points to sub-macroblock Y1
	mov (32)	r[PPREDBUF_Y,-PREDBLK1]<1>:ub		DEC_Y(4)<32;16,2> {Compr}	// First 4 Y2 rows
	mov (32)	r[PPREDBUF_Y,0-PREDBLK1+32]<1>:ub	DEC_Y(8)<32;16,2> {Compr}	// First 4 Y3 rows
	mov (32)	r[PPREDBUF_Y,0-PREDBLK1+64]<1>:ub	DEC_Y(6)<32;16,2> {Compr}	// Second 4 Y2 rows
	mov (32)	r[PPREDBUF_Y,0-PREDBLK1+96]<1>:ub	DEC_Y(10)<32;16,2> {Compr}	// Second 4 Y3 rows

//	All 4 sub-macroblock (containing 4 intra_8x8 blocks) have be constructed
//	Save constructed Y picture
	CALL(save_8x8_Y,1)		// Save Intra_8x8 predicted luma data.
//
//  Decode U/V blocks
//
//	Note: The decoding for chroma blocks will be the same for all intra prediction mode
//
	CALL(decode_Chroma_Intra,1)

#ifdef SW_SCOREBOARD
    #include "scoreboard_update.asm"
#endif

// Terminate the thread
//
    #include "EndIntraThread.asm"

// End of Intra_8x8
