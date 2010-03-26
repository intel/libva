/*
 * Common header file for both SetHWScoreboard and SetHWScoreboard_MBAFF kernels
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__SETHWSCOREBOARD_HEADER__)	// Make sure this file is only included once
#define __SETHWSCOREBOARD_HEADER__

// Module name: SetHWScoreboard_header.inc
//
// Common header file for both SetHWScoreboard and SetHWScoreboard_MBAFF kernels
//

#define BI_CMD_BUF			0	// Binding table index for command buffer

// GRF r1 map
//
// For use by setting HW scoreboard kernel for MBAFF picture
//
// CURBE data
#define	TOP_FM_MASK1_D	r1.0		// Bit mask for first half of top frame MB SB mask
#define	TOP_FM_MASK1_B	r1.0		// Bit mask for first half of top frame MB SB mask
#define	TOP_FM_MASK2_D	r1.1		// Bit mask for second half of top frame MB SB mask
#define	TOP_FM_MASK2_B	r1.4		// Bit mask for second half of top frame MB SB mask
#define	BOT_FM_MASK1_D	r1.2		// Bit mask for first half of bottom frame MB SB mask
#define	BOT_FM_MASK1_B	r1.8		// Bit mask for first half of bottom frame MB SB mask
#define	BOT_FM_MASK2_D	r1.3		// Bit mask for second half of bottom frame MB SB mask
#define	BOT_FM_MASK2_B	r1.12		// Bit mask for second half of bottom frame MB SB mask
#define	TOP_FD_MASK1_D	r1.4		// Bit mask for first half of top field MB SB mask
#define	TOP_FD_MASK1_B	r1.16		// Bit mask for first half of top field MB SB mask
#define	TOP_FD_MASK2_D	r1.5		// Bit mask for second half of top field MB SB mask
#define	TOP_FD_MASK2_B	r1.20		// Bit mask for second half of top field MB SB mask
#define	BOT_FD_MASK1_D	r1.6		// Bit mask for first half of bottom field MB SB mask
#define	BOT_FD_MASK1_B	r1.24		// Bit mask for first half of bottom field MB SB mask
#define	BOT_FD_MASK2_D	r1.7		// Bit mask for second half of bottom field MB SB mask
#define	BOT_FD_MASK2_B	r1.28		// Bit mask for second half of bottom field MB SB mask

// For use by setting HW scoreboard kernel for non-MBAFF picture
#define	MB_SHIFT_MASK_W		r1.0	// :w type. Shift values for two parts of the MB SB mask
#define	MB_SHIFT_MASK_B		r1.0	// :b type. Shift values for two parts of the MB SB mask

// GRF r2 map
//
// In-line data
//
#define	INLINE_REG_OFFSET	1
#define INLINE_REG			r2

#define StartingMB	INLINE_REG.0	// :uw type. Starting MB number
#define TotalMB		INLINE_REG.1	// :uw type. Total number of MB to be processed
#define PicWidthMB	INLINE_REG.2	// :uw type. Picture width in MB

// GRF r3 map
//
// Temporary variables
//
#define RemainderMB	r3.0			// :uw type. Remainder of MB (<16) to be processed

#define	TEMP_FD_X_W	r3.2			// :w type. Temporary variable for field MB X origin in MBAFF picture
#define	TEMP_FD_X_B	r3.4			// :b type. Temporary variable for field MB X origin in MBAFF picture
#define	TEMP_FD_Y_W	r3.3			// :w type. Temporary variable for field MB Y origin in MBAFF picture
#define	TEMP_FD_Y_B	r3.6			// :b type. Temporary variable for field MB Y origin in MBAFF picture

#define	TEMP_INTRA_FLAG_W	r3.4	// :uw type. Temporary intra available flag

#define MB_MASK_D	r3.4			// :ud type. Bit masks for MBAFF MB
#define MB_MASK_B	r3.16			// :ub type. Bit masks for MBAFF MB

#define	MRF_READ_HEADER_SRC	r63

// MEDIA_OBJECT_EX Command map
//
// In DW1 of each MEDIA_OBJECT_EX command (VFE DWORD)
#define	CUR_X	0	// Byte 0
#define	CUR_Y	0	// Byte 2

// In DW2 of each MEDIA_OBJECT_EX command
#define	USE_SCOREBOARD	BIT21

// In DW4 of each MEDIA_OBJECT_EX command
#define	F_FLAG		BIT4
#define	IS_INTRA_MB	BIT13
#define IS_FIELD_MB	BIT14
#define IS_INTRA8X8	BIT15
#define IS_BOT_FD	BIT24

// In DW7 of each MEDIA_OBJECT_EX command
#define	A_FLAG		BIT0
#define	B_FLAG		BIT1
#define	C_FLAG		BIT2
#define	D_FLAG		BIT3
#define	E_FLAG		BIT4

#define	CMD_BUFFER_REG_OFF		4
.declare    CMD_BUFFER_D		Base=GRF(4) ElementSize=4 SrcRegion=REGION(8,1) Type=ud		// Command buffer (32 GRF)
.declare    CMD_BUFFER_W		Base=GRF(4) ElementSize=2 SrcRegion=REGION(16,1) Type=uw	// Command buffer (32 GRF)
.declare    CMD_BUFFER_B		Base=GRF(4) ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// Command buffer (32 GRF)

#define	MRF_READ_HEADER		m1
#define	MRF_READ_HEADER0	m1
#define	MRF_READ_HEADER1	m2
#define	MRF_READ_HEADER2	m3
#define	MRF_READ_HEADER3	m4

#define	MRF_INTER_WRITE_HEADER	m5
#define	MRF_INTER_WRITE_DATA0	m6
#define	MRF_INTER_WRITE_DATA1	m7

#define	MRF_WRITE_HEADER		m11
#define	MRF_INTRA_WRITE_HEADER	m11

#define MRF_CMD_BUF_REG_OFF		12
.declare	MRF_CMD_BUF_D		Base=m12 ElementSize=4 SrcRegion=REGION(8,1) Type=ud		// Command buffer stored in MRF
.declare	MRF_CMD_BUF_W		Base=m12 ElementSize=2 SrcRegion=REGION(16,1) Type=uw		// Command buffer stored in MRF
.declare	MRF_CMD_BUF_B		Base=m12 ElementSize=1 SrcRegion=REGION(16,1) Type=ub		// Command buffer stored in MRF

#define	BI_CMD_BUFFER	0

#define OWBRMSGDSC_SC	 0x02088000	// OWORD Block Read Message Descriptor, reading from sampler cache = A.
#define OWBWMSGDSC		 0x02080000	// OWORD Block Write Message Descriptor

#define OWORD_1	0x000
#define OWORD_2	0x200
#define OWORD_4	0x300
#define OWORD_8	0x400

// End of SETHWSCOREBOARD_HEADER

#endif	// !defined(__SETHWSCOREBOARD_HEADER__)

