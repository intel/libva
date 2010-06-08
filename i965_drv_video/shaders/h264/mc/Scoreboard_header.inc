/*
 * Common header file for both scoreboard and scoreboard_MBAFF kernels
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__SCOREBOARD_HEADER__)	// Make sure this file is only included once
#define __SCOREBOARD_HEADER__

// Module name: scoreboard_header.inc
//
// Common header file for both scoreboard and scoreboard_MBAFF kernels
//

#define ONE_MB_WA						// Enable WA for 1-MB wide pictures. To disable WA, simply comment out this line.

#define	INLINE_REG_OFF	1
#define INLINE_REG		r1
#define INLINE_REG1		r2

#define DONEFLAG	0x40		// Bit mask of "completed" thread flag

// GRF r1 map
//
#define WIDTHINMB_1	INLINE_REG.0		// :uw type. Picture width in MB - 1
#define HEIGHTINMB_1	INLINE_REG.1	// :uw type. Picture height in MB - 1
#define TotalMB		INLINE_REG.2		// :uw type. Total number of macroblocks
#define WFLen_B		INLINE_REG.3		// :uw type. Bottom MB Wavefront length (Reserved for MBAFF scoreboard)
#define WFLen		INLINE_REG.4		// :uw type. Wavefront length (used as loop counter)
#define WFLenY		INLINE_REG.5		// :uw type. Wavefront length (vertical component)
#define StartX		INLINE_REG.6		// :uw type. Start X of current wavefront
#define StartY		INLINE_REG.7		// :uw type. Start Y of current wavefront
#define StartXD		INLINE_REG.3		// :ud type. Start (X,Y) of current wavefront
#define	CASE00PTR	INLINE_REG.4		// :ud type. Pointer to "inter start" handler
#define WFLen_Save	INLINE_REG.10		// :uw type. Saved Wavefront length (Reserved for MBAFF scoreboard)
#define	CASE10PTR	INLINE_REG.6		// :ud type. Pointer to "intra start" handler
#define	CASE11PTR	INLINE_REG.7		// :ud type. Pointer to "inter complete" handler

// GRF r2 map
//
.declare    WFStart	Base=GRF(2) ElementSize=2 SrcRegion=REGION(4,1) Type=w	// Start MB of recent 4 wavefronts, actually use 5 WORDs
.declare    WFStart_T	Base=GRF(2) ElementSize=2 SrcRegion=REGION(4,1) Type=w		// Start MB of recent 4 wavefronts
.declare    WFStart_B	Base=GRF(2).4 ElementSize=2 SrcRegion=REGION(4,1) Type=w	// Start MB of recent 4 wavefronts

#define NewWFOffsetD	INLINE_REG1.5	// :d type. Offsets used for new wavefront = 0x01ffff00 (0, -1, -1, 1)
#define NewWFOffset	INLINE_REG1.20		// :b type. Offsets used for new wavefront = 0x01ffff00 (0, -1, -1, 1)

#define AVAILFLAGD	INLINE_REG1.6		// :ud type. Neighbor available flags = 0x08020401 (in ACBD order)
#define AVAILFLAG	INLINE_REG1.24		// :ub type. Neighbor available flags as above
#define AVAILFLAG1D	INLINE_REG1.7		// :ud type. Top-half neighbor available flags = 0x80402010 (in A_Bxxx order)

.declare    MBINDEX	Base=GRF(3) ElementSize=2 SrcRegion=REGION(16,1) Type=w	// MB order # of current MB group (Cur, ACBD and AC_B_D_)
#define AR_SAVE		r3.8	// :uw type. Saved Address Register information

#define	CMDPTR		a0.0	// :uw type. DWORD Pointer to the scoreboard
#define DEPPTR		a0.0	// :uw type. Pointer to the dependency scoreboard - Current MB
#define DEPPTRL		a0.1	// :uw type. Pointer to the dependency scoreboard - Left MB
#define DEPPTRTR	a0.2	// :uw type. Pointer to the dependency scoreboard - Top right MB
#define DEPPTRT		a0.3	// :uw type. Pointer to the dependency scoreboard - Top MB
#define DEPPTRTL    a0.4    // :uw type. Pointer to the dependency scoreboard - Top left MB
#define DEPPTRLB	a0.5	// :uw type. Pointer to the dependency scoreboard - Left bottom-half MB

#define	PMSGSEL		a0.7	// :uw type. Pointer to current message in message handling table

#define	CMD_SB_REG_OFF		4
.declare    CMD_SB		Base=GRF(4) ElementSize=4 SrcRegion=REGION(8,1) Type=ud	// Command scoreboard (64 GRF)

#ifdef	AS_ENABLED
//	Definitions for Advanced Scheduler support
#define AS_INT		BIT23	// "Preemption Exception Status" bit in cr0.1:ud control register
#define AS_INT_EN	BIT10	// "Preemption Exception Enable" bit in cr0.1:ud control register
#define TH_INT		BIT2	// "Thread Interrupted" bit in message descriptor
#define TH_RES		BIT0	// "Thread Restart Enable" bit in R0 header r0.2

#define AS_SAVE		34		// Surface state for saving scoreboard contents
							// Ensure not to conflict with existing binding table entries
#endif	// End AS_ENABLED

// End of scoreboard_header

#endif	// !defined(__SCOREBOARD_HEADER__)

