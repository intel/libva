/*
 * Decode Intra_4x4 macroblock
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: Intra_4x4.asm
//
// Decoding of Intra_4x4 macroblock
//
//  $Revision: 12 $
//  $Date: 10/18/06 4:10p $
//

// ----------------------------------------------------
//  Main: Intra_4x4
// ----------------------------------------------------

#define	INTRA_4X4

.kernel Intra_4x4
INTRA_4x4:

#ifdef _DEBUG
// WA for FULSIM so we'll know which kernel is being debugged
mov (1) acc0:ud 0x02aa55a5:ud
#endif

#include "SetupForHWMC.asm"

#undef		PPREDBUF_Y
#define	    PPREDBUF_Y		a0.3	// Pointer to predicted Y picture

#define		REG_INTRA_PRED_AVAIL	REG_INTRA_TEMP_4
#define		REG_INTRA_4X4_PRED		REG_INTRA_TEMP_7		// Store predicted Intra_4x4 data

// Offset where 4x4 predicted data blocks are stored
#define	PREDSUBBLK0		0*GRFWIB
#define	PREDSUBBLK1		1*GRFWIB
#define	PREDSUBBLK2		2*GRFWIB
#define	PREDSUBBLK3		3*GRFWIB
#define	PREDSUBBLK4		4*GRFWIB
#define	PREDSUBBLK5		5*GRFWIB
#define	PREDSUBBLK6		6*GRFWIB
#define	PREDSUBBLK7		7*GRFWIB
#define	PREDSUBBLK8		8*GRFWIB
#define	PREDSUBBLK9		9*GRFWIB
#define	PREDSUBBLK10	10*GRFWIB
#define	PREDSUBBLK11	11*GRFWIB
#define	PREDSUBBLK12	12*GRFWIB
#define	PREDSUBBLK13	13*GRFWIB
#define	PREDSUBBLK14	14*GRFWIB
#define	PREDSUBBLK15	15*GRFWIB

// 4x4 error block byte offset within the 8x8 error block
#define ERRBLK0		0
#define ERRBLK1		8
#define ERRBLK2		64
#define ERRBLK3		72

#ifdef SW_SCOREBOARD    
    CALL(scoreboard_start_intra,1)
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
	mov	(1)	PPREDBUF_Y<1>:w	PREDBUF*GRFWIB:w	// Pointer to predicted data
	shr (2)	REG_INTRA_PRED_AVAIL<1>:w	REG_INTRA_PRED_AVAIL_FLAG_BYTE<0;1,0>:ub	0x40:v
    and.nz.f0.0 (8)	NULLREG	REG_INTRA_PRED_AVAIL_FLAG_BYTE<0;1,0>:ub	4:w	// Top-right macroblock available for intra prediction?
	(-f0.0.any8h) mov (8)	INTRA_REF_TOP(0,16)<1>	INTRA_REF_TOP(0,15)REGION(1,0)	// Extend right boundary of MB B to C

//	Intra predict Intra_4x4 luma blocks
//
//	Sub-macroblock 0 *****************
	mov	(16)	REF_TOP0(0)<1>	INTRA_REF_TOP0(0)	// Top reference data
	mov	(8)		REF_LEFT(0)<1>	INTRA_REF_LEFT(0)REGION(8,4)	// Left reference data
	shr	(4)		PRED_MODE<1>:w	INTRA_PRED_MODE(0)<1;2,0>	0x4040:v	// Expand IntraPredMode to 1 byte/block
	CALL(intra_Pred_4x4_Y_4,1)
    add (1)		PERROR<1>:w	PERROR<0;1,0>:w	0x0080:w	// Pointers to next error block

	or  (1)	REG_INTRA_PRED_AVAIL<1>:w	REG_INTRA_PRED_AVAIL<0;1,0>:w	0x1:w	// Left neighbor is available now

//	Sub-macroblock 1 *****************

	mov	(16)	REF_TOP0(0)<1>	INTRA_REF_TOP0(0,8)	// Top reference data
	mov	(4)		REF_LEFT(0)<1>	r[PPREDBUF_Y,PREDSUBBLK1+6]<8;1,0>:ub	// Left reference data (top half)
	mov	(4)		REF_LEFT(0,4)<1>	r[PPREDBUF_Y,PREDSUBBLK3+6]<8;1,0>:ub	// Left reference data (bottom half)
	shr	(4)		PRED_MODE<1>:w	INTRA_PRED_MODE(0,2)<1;2,0>	0x4040:v	// Expand IntraPredMode to 1 byte/block
	add	(1)		PPREDBUF_Y<1>:w	PPREDBUF_Y<0;1,0>:w	4*GRFWIB:w	// Pointer to predicted sub-macroblock 1
	CALL(intra_Pred_4x4_Y_4,1)
    add (1)		PERROR<1>:w	PERROR<0;1,0>:w	0x0080:w	// Pointers to next error block

	or  (1)	REG_INTRA_PRED_AVAIL<1>:w	REG_INTRA_PRED_AVAIL.1<0;1,0>:w	0x2:w	// Top neighbor is available now

//	Pack constructed data from word-aligned to byte-aligned format
//	to speed up save_4x4_Y module later
//	PPREDBUF_Y now points to sub-block #4
	mov (16)	r[PPREDBUF_Y,-PREDSUBBLK4]<1>:ub	r[PPREDBUF_Y,-PREDSUBBLK4]<32;16,2>:ub		// Sub-block 0
	mov (16)	r[PPREDBUF_Y,0-PREDSUBBLK4+16]<1>:ub	r[PPREDBUF_Y,-PREDSUBBLK3]<32;16,2>:ub	// Sub-block 1
	mov (16)	r[PPREDBUF_Y,-PREDSUBBLK2]<1>:ub	r[PPREDBUF_Y,-PREDSUBBLK2]<32;16,2>:ub		// Sub-block 2
	mov (16)	r[PPREDBUF_Y,0-PREDSUBBLK2+16]<1>:ub	r[PPREDBUF_Y,-PREDSUBBLK1]<32;16,2>:ub	// Sub-block 3

	mov (16)	r[PPREDBUF_Y,-PREDSUBBLK3]<1>:ub	r[PPREDBUF_Y]<32;16,2>:ub				// Sub-block 4
	mov (16)	r[PPREDBUF_Y,0-PREDSUBBLK3+16]<1>:ub	r[PPREDBUF_Y,PREDSUBBLK1]<32;16,2>:ub	// Sub-block 5
	mov (16)	r[PPREDBUF_Y,-PREDSUBBLK1]<1>:ub	r[PPREDBUF_Y,PREDSUBBLK2]<32;16,2>:ub		// Sub-block 6
	mov (16)	r[PPREDBUF_Y,0-PREDSUBBLK1+16]<1>:ub	r[PPREDBUF_Y,PREDSUBBLK3]<32;16,2>:ub	// Sub-block 7

//	Sub-macroblock 2 *****************

	mov	(4)		REF_TOP0(0)<1>		INTRA_REF_LEFT0(0,28)REGION(4,1)		// Top-left reference data
	mov	(8)		REF_TOP0(0,4)<1>	r[PPREDBUF_Y,0-2*GRFWIB+12]<16;4,1>:ub	// Top reference data from SB 2,3
	mov	(8)		REF_TOP0(0,12)<1>	r[PPREDBUF_Y,0-GRFWIB+12]<16;4,1>:ub	// Top reference data from SB 6,7
	mov	(8)		REF_TOP0(0,20)<1>	r[PPREDBUF_Y,0-GRFWIB+31]<0;1,0>:ub		// Top-right reference data
	mov	(16)	REG_INTRA_REF_TOP<1>:w	REF_TOP_W(0)		// Store top reference data for SubMB #2 and #3.
	mov	(8)		REF_LEFT(0)<1>		INTRA_REF_LEFT(1)REGION(8,4)	// Left reference data
	shr	(4)		PRED_MODE<1>:w		INTRA_PRED_MODE(0,4)<1;2,0>	0x4040:v	// Expand IntraPredMode to 1 byte/block
	CALL(intra_Pred_4x4_Y_4,1)
    add (1)		PERROR<1>:w	PERROR<0;1,0>:w	0x0080:w	// Pointers to next error block

	or  (1)	REG_INTRA_PRED_AVAIL<1>:w	REG_INTRA_PRED_AVAIL<0;1,0>:w	0x1:w	// Left neighbor is available now

//	Sub-macroblock 3 *****************

	mov	(16)	REF_TOP0(0)<1>	INTRA_REF_TOP0(0,8)		// Top reference data
	mov	(8)		REF_TOP0(0,16)<1>	INTRA_REF_TOP0(0,24)REGION(8,1)	// Top reference data
	mov	(4)		REF_LEFT(0)<1>	r[PPREDBUF_Y,PREDSUBBLK1+6]<8;1,0>:ub	// Left reference data (top half)
	mov	(4)		REF_LEFT(0,4)<1>	r[PPREDBUF_Y,PREDSUBBLK3+6]<8;1,0>:ub	// Left reference data (bottom half)
	shr	(4)		PRED_MODE<1>:w	INTRA_PRED_MODE(0,6)<1;2,0>	0x4040:v	// Expand IntraPredMode to 1 byte/block
	add	(1)		PPREDBUF_Y<1>:w	PPREDBUF_Y<0;1,0>:w	4*GRFWIB:w	// Pointer to predicted sub-macroblock 3
	CALL(intra_Pred_4x4_Y_4,1)

//	Pack constructed data from word-aligned to byte-aligned format
//	to speed up save_4x4_Y module later
//	PPREDBUF_Y now points to sub-block #12
	mov (16)	r[PPREDBUF_Y,-PREDSUBBLK4]<1>:ub	r[PPREDBUF_Y,-PREDSUBBLK4]<32;16,2>:ub		// Sub-block 8
	mov (16)	r[PPREDBUF_Y,0-PREDSUBBLK4+16]<1>:ub	r[PPREDBUF_Y,-PREDSUBBLK3]<32;16,2>:ub	// Sub-block 9
	mov (16)	r[PPREDBUF_Y,-PREDSUBBLK2]<1>:ub	r[PPREDBUF_Y,-PREDSUBBLK2]<32;16,2>:ub		// Sub-block 10
	mov (16)	r[PPREDBUF_Y,0-PREDSUBBLK2+16]<1>:ub	r[PPREDBUF_Y,-PREDSUBBLK1]<32;16,2>:ub	// Sub-block 11

	mov (16)	r[PPREDBUF_Y,-PREDSUBBLK3]<1>:ub	r[PPREDBUF_Y]<32;16,2>:ub				// Sub-block 12
	mov (16)	r[PPREDBUF_Y,0-PREDSUBBLK3+16]<1>:ub	r[PPREDBUF_Y,PREDSUBBLK1]<32;16,2>:ub	// Sub-block 13
	mov (16)	r[PPREDBUF_Y,-PREDSUBBLK1]<1>:ub	r[PPREDBUF_Y,PREDSUBBLK2]<32;16,2>:ub		// Sub-block 14
	mov (16)	r[PPREDBUF_Y,0-PREDSUBBLK1+16]<1>:ub	r[PPREDBUF_Y,PREDSUBBLK3]<32;16,2>:ub	// Sub-block 15

//	All 4 sub-macroblock (containing 4 intra_4x4 blocks) have be constructed
//	Save constructed Y picture
	CALL(save_4x4_Y,1)		// Save Intra_4x4 predicted luma data.
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

// End of Intra_4x4
