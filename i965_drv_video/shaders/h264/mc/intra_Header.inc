/*
 * Header file for all AVC intra prediction kernels
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__INTRA_HEADER__)	// Make sure this file is only included once
#define __INTRA_HEADER__

// Module name: intra_header.inc
//
// Header file for all AVC intra prediction kernels
//
//	This header file defines everything that's specific to intra macroblock kernels


//  ----------- Various data buffers and pointers ------------
//
//	I_PCM data buffer
//
#define		I_PCM_BUF_Y			4
#define		I_PCM_BUF_UV		12

#define		REG_I_PCM_BUF_Y		r4
#define		REG_I_PCM_BUF_UV	r12

.declare    I_PCM_Y  Base=REG_I_PCM_BUF_Y  ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// 8-bit I_PCM Y data
.declare    I_PCM_UV Base=REG_I_PCM_BUF_UV ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// 8-bit I_PCM U/V data

//	Intra macroblock error data blocks
//
#define	    ERRBUF		4		// Starting GRF index for error data
#define		REG_ERRBUF	r4
.declare    MBBLOCKW Base=REG_ERRBUF ElementSize=2 SrcRegion=REGION(16,1) Type=w	// For 16-bit inter MB
.declare    MBBLOCKD Base=REG_ERRBUF ElementSize=2 SrcRegion=REGION(16,1) Type=uw	// For use in "send" command

#define	    PERROR		a0.2	// Pointer to macroblock error data
#define	    PERROR1		a0.3	// Pointer to macroblock error data used by instruction compression
#define	    PERROR_UD	a0.1	// Pointer to macroblock error data in DWORD unit

//	Intra macroblock reference data
//
#define		REG_INTRA_REF_TOP	r49		// Must be an odd numbered GRF register
.declare    INTRA_REF_TOP0		Base=REG_INTRA_REF_TOP	 ElementSize=1 SrcRegion=REGION(16,1) Type=ub
.declare    INTRA_REF_TOP		Base=REG_INTRA_REF_TOP.4 ElementSize=1 SrcRegion=REGION(16,1) Type=ub
										// Actual top row reference data start at offset 4 in BYTE
.declare    INTRA_REF_TOP_W		Base=REG_INTRA_REF_TOP.2 ElementSize=2 SrcRegion=REGION(16,1) Type=uw
										// Actual top row reference data start at offset 2 in WORD
.declare    INTRA_REF_TOP_D		Base=REG_INTRA_REF_TOP ElementSize=4 DstRegion=<1> Type=ud	// Only used in "send" instruction

#define		INTRA_REF_LEFT_ID	50
#define		REG_INTRA_REF_LEFT	r50
.declare    INTRA_REF_LEFT0		Base=REG_INTRA_REF_LEFT ElementSize=1 SrcRegion=REGION(8,4) Type=ub
.declare    INTRA_REF_LEFT		Base=REG_INTRA_REF_LEFT.3 ElementSize=1 SrcRegion=REGION(8,4) Type=ub
										// Actual left column reference data are located at offset 3 in BYTE
.declare    INTRA_REF_LEFT_UV	Base=REG_INTRA_REF_LEFT.2 ElementSize=1 SrcRegion=REGION(8,4) Type=ub
										// Actual left column U/V reference data are located at offset 2 in BYTE
.declare    INTRA_REF_LEFT_W	Base=REG_INTRA_REF_LEFT.1 ElementSize=2 SrcRegion=REGION(8,2) Type=uw
										// Actual left column reference data are located at offset 1 in WORD
.declare    INTRA_REF_LEFT_D	Base=REG_INTRA_REF_LEFT ElementSize=4 DstRegion=<1> Type=ud	// Only used in "send" instruction

#define		PREF_LEFT		a0.4	// Pointer to left reference data
#define		PREF_LEFT_UD	a0.2	// Pointer in DWORD to left reference data

#define		INTRA_TEMP_0	52
#define		INTRA_TEMP_1	53
#define		INTRA_TEMP_2	54
#define		INTRA_TEMP_3	55
#define		INTRA_TEMP_4	56
#define		INTRA_TEMP_5	57
#define		INTRA_TEMP_6	58

#define		REG_INTRA_TEMP_0	r52
#define		REG_INTRA_TEMP_1	r53
#define		REG_INTRA_TEMP_2	r54
#define		REG_INTRA_TEMP_3	r55
#define		REG_INTRA_TEMP_4	r56
#define		REG_INTRA_TEMP_5	r57
#define		REG_INTRA_TEMP_6	r58
#define		REG_INTRA_TEMP_7	r59
#define		REG_INTRA_TEMP_8	r60

// Destination registers for write commit
#define		REG_WRITE_COMMIT_Y	r60.0
#define		REG_WRITE_COMMIT_UV	r61.0

//  ----------- Various data buffers and pointers ------------
//  R32 - R47 for predicted picture buffer (for both Y and U/V blocks)
//
#define	    PREDBUF		32		// Starting GRF index for predicted buffer
#define		REG_PREDBUF	r32

.declare    PRED_Y		Base=REG_PREDBUF ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// Predicted Y picture
.declare    PRED_YW		Base=REG_PREDBUF ElementSize=2 SrcRegion=REGION(16,1) Type=uw	// Predicted Y picture stored in WORD
.declare    PRED_Y_FM	Base=REG_PREDBUF ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// Predicted Y picture frame
.declare    PRED_Y_TF	Base=REG_PREDBUF ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// Predicted Y picture Top field

.declare    PRED_UV		Base=REG_PREDBUF ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// Predicted U/V picture
.declare    PRED_UVW	Base=REG_PREDBUF ElementSize=2 SrcRegion=REGION(16,1) Type=uw	// Predicted U/V picture stored in WORD
.declare    PRED_UV_FM	Base=REG_PREDBUF ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// Predicted U/V picture frame
.declare    PRED_UV_TF	Base=REG_PREDBUF ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// Predicted U/V picture top field
.declare    PRED_UV_BF	Base=REG_PREDBUF.16 ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// Predicted U/V picture bottom field

//  The same region will also be used as finally decoded Y blocks shared with U/V blocks
//
#define	    DECBUF		32
#define		REG_DECBUF	r32
.declare    DEC_Y		Base=REG_DECBUF ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// Decoded Y picture
.declare    DEC_UV		Base=REG_DECBUF ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// Decoded U/V P-/B-picture
.declare    DEC_UD		Base=REG_DECBUF ElementSize=4 SrcRegion=REGION(8,1) Type=ud		// Decoded buffer in UD type

//	Reference buffer for intra_NxN prediction
//
#define		PRED_MODE	REG_INTRA_TEMP_0
.declare    REF_TOP0	Base=REG_INTRA_TEMP_5	ElementSize=1 SrcRegion=REGION(16,1) Type=ub
.declare    REF_TOP		Base=REG_INTRA_TEMP_5.4 ElementSize=1 SrcRegion=REGION(16,1) Type=ub
									// Actual top reference data start from offset 3,i.e. p[-1,-1]
.declare    REF_TOP_W	Base=REG_INTRA_TEMP_5 ElementSize=2 SrcRegion=REGION(16,1) Type=uw
.declare    REF_TOP_D	Base=REG_INTRA_TEMP_5 ElementSize=4 SrcRegion=REGION(8,1) Type=ud
.declare    REF_LEFT	Base=REG_INTRA_TEMP_6 ElementSize=1 SrcRegion=REGION(16,1) Type=ub
.declare    REF_LEFT_D	Base=REG_INTRA_TEMP_6 ElementSize=4 SrcRegion=REGION(8,1) Type=ud

// For intra prediction plane mode
//
.declare    H1	Base=REG_INTRA_TEMP_0 ElementSize=2 SrcRegion=REGION(8,1) Type=w	// Make sure it's an even GRF
.declare    H2	Base=REG_INTRA_TEMP_0.8 ElementSize=2 SrcRegion=REGION(8,1) Type=w
.declare    V1	Base=REG_INTRA_TEMP_1 ElementSize=2 SrcRegion=REGION(8,1) Type=w	// Make sure it's the following odd GRF
.declare    V2	Base=REG_INTRA_TEMP_1.8 ElementSize=2 SrcRegion=REGION(8,1) Type=w

.declare	CP	Base=REG_INTRA_TEMP_2 ElementSize=2 SrcRegion=REGION(16,1) Type=w

#define		PINTRAPRED_Y	a0.7	// Used as luma intra prediction mode pointer
#define		PINTRAPRED_UV	a0.7	// Used as chroma intra prediction mode pointer
#define		PINTRA4X4_Y		a0.4	// Used as luma intra_4x4 prediction mode pointer

#define		PBWDCOPY_4		a0.4	// a0.4 - a0.7 used in intra_4x4 prediction for moving data backward
#define		PBWDCOPY_8		a0.6	// a0.6 - a0.7 used in intra_8x8 prediction for moving data backward

// For Intra_4x4 prediction mode
//
.declare    INTRA_4X4_MODE	Base=REG_INTRA_TEMP_1 ElementSize=4 SrcRegion=REGION(1,0) DstRegion=<1> Type=d	// Actually only need 1 DWORD

//  ----------- Intra CURBE constants ------------
//
#define		REG_CURBE1	r1
#define		REG_CURBE2	r2
#define		INTRA_4X4_OFFSET	1*GRFWIB		// 9 Bytes
#define		INTRA_8X8_OFFSET	1*GRFWIB+12		// 9 Bytes starting sub-register r1.3:ud
#define		INTRA_16X16_OFFSET	1*GRFWIB+24		// 4 Bytes starting sub-register r1.6:ud
#define		INTRA_CHROMA_OFFSET	1*GRFWIB+28		// 4 Bytes starting sub-register r1.7:ud

#define		TOP_REF_OFFSET		REG_CURBE1.10	// r1.5:w

//	Constants used in plane intra prediction mode
#define		XY_3	REG_CURBE2.4	// Stored BYTE constants x-3 for x=0...7, i.e. -3,-2,...3,4 for U/V, need duplicate to every other byte
#define		XY_3_1	REG_CURBE2.5	// Stored BYTE constants x-3 for x=0...7, i.e. -3,-2,...3,4 for 2nd instruction in {Comp}
#define		XY_7	REG_CURBE2.0	// Stored BYTE constants x-7 for x=0...15, i.e. -7,-6,...7,8 for Y
#define		XY_7_1	REG_CURBE2.1	// Stored BYTE constants x-7 for x=0...15, i.e. -7,-6,...7,8 for 2nd instruction in {Comp}

#define		INV_SHIFT	REG_CURBE2.16

#define		INV_TRANS4	REG_CURBE2.20	// For reverse data transfer for intra_4x4 (0x00020406)
#define		INV_TRANS48	REG_CURBE2.22	// For reverse data transfer for intra_4x4 (0x0002)
#define		INV_TRANS8	REG_CURBE1.22	// For reverse data transfer for intra_8x8 (0x0001)

#define		INTRA_MODE	REG_CURBE2.24	// Offset to intra_Pred_4x4_Y from each sub-block

//  ----------- In-line parameters ------------
//
#define REG_INLINE	r3

#define INLINE_DW0	REG_INLINE.0<0;1,0>:ud
#define INLINE_DW1	REG_INLINE.1<0;1,0>:ud
#define INLINE_DW2	REG_INLINE.2<0;1,0>:ud
#define INLINE_DW3	REG_INLINE.3<0;1,0>:ud
#define INLINE_DW4	REG_INLINE.4<0;1,0>:ud
#define INLINE_DW5	REG_INLINE.5<0;1,0>:ud
#define INLINE_DW6	REG_INLINE.6<0;1,0>:ud
#define INLINE_DW7	REG_INLINE.7<0;1,0>:ud

//	Intra macroblock in-line data
//
//	In-line DWORD 0
#define REG_MBAFF_FIELD				REG_INLINE.1	// :uw, can be added directly to lower-word of MSGDSC
#define MBAFF_FIELD					BIT26+BIT25		// Bits 26:25 - MBAFF field macroblock flag
													//  00 = Current macroblock is not an MBAFF field macroblock
													//  11 = Current macroblock is an MBAFF field macroblock

#define REG_FIELD_PARITY			INLINE_DW0
#define FIELD_PARITY				BIT24			// Bit 24 - Macroblock field parity flag
													//  0 = Current field is a top field
													//  1 = Current field is a bottom field

#define REG_FIELD_MACROBLOCK_FLAG	INLINE_DW0
#define FIELD_MACROBLOCK_FLAG		BIT14			// Bit 14 - Field macroblock flag
													//  0 = Current macroblock is not a field macroblock
													//  1 = Current macroblock is a field macroblock
#define REG_MACROBLOCK_TYPE			INLINE_DW0
#define MACROBLOCK_TYPE				BIT12+BIT11+BIT10+BIT9+BIT8		// Bit 12:8 - Intra macroblock flag

#define REG_CHROMA_FORMAT_IDC		INLINE_DW0
#define CHROMA_FORMAT_IDC			BIT3+BIT2		// Bit 3:2 - Chroma format
													// 00 = Luma only (Monochrome)
													// 01 = YUV420
													// 10 = YUV422
													// 11 = YUV444
#define	REG_MBAFF_PIC				INLINE_DW0
#define MBAFF_PIC					BIT1			// Bit 1 - MBAFF Frame picture
													// 0 = Not an MBAFF frame picture
													// 1 = An MBAFF frame picture
#define REG_INTRA_PRED_8X8_BLK2_AVAIL_FLAG	INLINE_DW0
#define INTRA_PRED_8X8_BLK2_AVAIL_FLAG	BIT4		// Bit 4: Pixel available for block 2 in an intra_8x8 MB.

//	In-line DWORD 1
#define ORIX			REG_INLINE.4	// :ub, H. origin of the macroblock in macroblock unit
#define ORIY			REG_INLINE.5	// :ub, V. origin of the macroblock in macroblock unit

//	In-line DWORD 2
#define	REG_CBPCYB					REG_INLINE.9	// :ub, Coded block pattern
#define	REG_CBPCY					INLINE_DW2		// Bits 13:8 - Coded block pattern
													// reflect Y0, Y1, Y2, Y3, Cb4, Cr5
													// Bit 13 - Y0
													// Bit 12 - Y1
													// Bit 11 - Y2
													// Bit 10 - Y3
													// Bit 9 - U4
													// Bit 8 - V5
#define CBP_MASK					0x3F00:ud		// Bit mask for all CBP bits
#define CBP_Y_MASK					0x3C00:ud		// Bit mask for CBP Y bits
#define CBP_UV_MASK					0x0300:ud		// Bit mask for CBP U/V bits

#define CBP_Y0_MASK					BIT13:ud		// Bit mask for CBP Y0 bit
#define CBP_Y1_MASK					BIT12:ud		// Bit mask for CBP Y1 bit
#define CBP_Y2_MASK					BIT11:ud		// Bit mask for CBP Y2 bit
#define CBP_Y3_MASK					BIT10:ud		// Bit mask for CBP Y3 bit
#define CBP_U_MASK					BIT9:ud			// Bit mask for CBP U bit
#define CBP_V_MASK					BIT8:ud			// Bit mask for CBP V bit

//	In-line DWORD 3
#define REG_INTRA_CHROMA_PRED_MODE	REG_INLINE.12	// :ub - Intra chroma prediction mode
#define INTRA_CHROMA_PRED_MODE		BIT7+BIT6		// Bit 7:6 - Intra chroma prediction mode
													// 00 = Intra DC prediction
													// 01 = Intra horizontal prediction
													// 10 = Intra vertical prediction
													// 11 = Intra plane prediction
#define INTRA_CHROMA_PRED_MODE_SHIFT	6			// Intra chroma prediction mode shift

#define REG_INTRA_PRED_AVAIL_FLAG	INLINE_DW3
#define INTRA_PRED_AVAIL_FLAG		BIT4+BIT3+BIT2+BIT1+BIT0	// Bits 4:0 - Intra prediction available flag
													// Bit 0: Macroblock A (the left neighbor) entire or top half
													// Bit 1: Macroblock B (the upper neighbor)
													// Bit 2: Macroblock C (the above-right neighbor)
													// Bit 3: Macroblock D (the above-left neighbor)
													// Bit 4: Macroblock A (the left neighbor) bottom half
													// Each bit is defined below
													// 0 = The macroblock is not available for intra prediction
													// 1 = The macroblock is available for intra prediction
#define INTRA_PRED_LEFT_TH_AVAIL_FLAG	BIT0		// Bit 0: Macroblock A (the left neighbor) entire or top half
#define INTRA_PRED_UP_AVAIL_FLAG		BIT1		// Bit 1: Macroblock B (the upper neighbor)
#define INTRA_PRED_UP_RIGHT_AVAIL_FLAG	BIT2		// Bit 2: Macroblock C (the above-right neighbor)
#define INTRA_PRED_UP_LEFT_AVAIL_FLAG	BIT3		// Bit 3: Macroblock D (the above-left neighbor)
#define INTRA_PRED_LEFT_BH_AVAIL_FLAG	BIT4		// Bit 4: Macroblock A (the left neighbor) bottom half
//#define INTRA_PRED_8X8_BLK2_AVAIL_FLAG	BIT5		// Bit 5: Pixel available for block 2 in an intra_8x8 MB.
#define REG_INTRA_PRED_AVAIL_FLAG_BYTE	REG_INLINE.12	// Byte location of Intra_Pred_Avail_Flag
#define REG_INTRA_PRED_AVAIL_FLAG_WORD	REG_INLINE.6	// Word location of Intra_Pred_Avail_Flag


.declare    INTRA_PRED_MODE  Base=REG_INLINE.16 ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// Intra prediction mode

// End of intra_header.inc

#endif	// !defined(__INTRA_HEADER__)

