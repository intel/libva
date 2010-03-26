/*
 * Set dependency control HW scoreboard kernel
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: SetHWScoreboard.asm
//
// Set dependency control HW scoreboard kernel
//

// ----------------------------------------------------
//  Main: SetHWScoreboard
// ----------------------------------------------------

.kernel SetHWScoreboard

SETHWSCOREBOARD:

#ifdef _DEBUG
// WA for FULSIM so we'll know which kernel is being debugged
mov (1) acc0:ud 0xf0aa55a5:ud
#endif

#include "header.inc"
#include "SetHWScoreboard_header.inc"

//
//  Now, begin source code....
//

.code

//	Separate the TotalMB so TotalMB will be multiple of 8
//	and RemainderMB will hold the TotalMB%8
//
	and.z.f0.1 (1)	RemainderMB<1>:uw	TotalMB<0;1,0>:uw	0x0007:uw	// number of %8 commands
	and.z.f0.0 (1)	TotalMB<1>:uw		TotalMB<0;1,0>:uw	0xfff8:uw	// Number of 8-command blocks

	mov (1)	MB_SHIFT_MASK_W<1>:uw		0x100*16+12:w					// Set up shift values (12, 16)

//	Initialize common DAP read header
//
	mov (8)	MRF_READ_HEADER_SRC<1>:ud	r0.0<8;8,1>:ud
	shl (1) MRF_READ_HEADER_SRC.2<1>:ud	StartingMB<0;1,0>:uw	6:uw	// Byte-aligned offset being read

//	Initialize Inter DAP write header
 	mov (8)	MRF_INTER_WRITE_HEADER<1>:ud	r0.0<8;8,1>:ud

	(f0.0) jmpi (1)	SetHWScoreboard_Remainder							// Jump if TotalMB < 8

//------------------------------------------------------------------------
//	Command buffer parsing loop
//	Each loop will handle 8 commands
//------------------------------------------------------------------------
//
SetHWScoreboard_Loop:
//	Load block 0 (Commands 0/1)
	mov (8)	MRF_READ_HEADER0.0<1>:ud	MRF_READ_HEADER_SRC.0<8;8,1>:ud
	send (16)	CMD_BUFFER_W(0)<1>	MRF_READ_HEADER0	null:uw	DAPREAD	RESP_LEN(4)+OWBRMSGDSC_SC+OWORD_8+BI_CMD_BUFFER

//	Load block 1  (Commands 2/3)
	mov (8)	MRF_READ_HEADER1.0<1>:ud	MRF_READ_HEADER_SRC.0<8;8,1>:ud
	add	(1)	MRF_READ_HEADER1.2<1>:ud	MRF_READ_HEADER_SRC.2<0;1,0>:ud		128:ud		// Point to next 2-command block
	send (16)	CMD_BUFFER_W(4)<1>	MRF_READ_HEADER1	null:uw	DAPREAD	RESP_LEN(4)+OWBRMSGDSC_SC+OWORD_8+BI_CMD_BUFFER

//	Load block 2  (Commands 4/5)
	mov (8)	MRF_READ_HEADER2.0<1>:ud	MRF_READ_HEADER_SRC.0<8;8,1>:ud
	add	(1)	MRF_READ_HEADER2.2<1>:ud	MRF_READ_HEADER_SRC.2<0;1,0>:ud		256:ud		// Point to next 2-command block
	send (16)	CMD_BUFFER_W(8)<1>	MRF_READ_HEADER2	null:uw	DAPREAD	RESP_LEN(4)+OWBRMSGDSC_SC+OWORD_8+BI_CMD_BUFFER

//	Load block 3  (Commands 6/7)
	mov (8)	MRF_READ_HEADER3.0<1>:ud	MRF_READ_HEADER_SRC.0<8;8,1>:ud
	add	(1)	MRF_READ_HEADER3.2<1>:ud	MRF_READ_HEADER_SRC.2<0;1,0>:ud		384:ud		// Point to next 2-command block
	send (16)	CMD_BUFFER_W(12)<1>	MRF_READ_HEADER3	null:uw	DAPREAD	RESP_LEN(4)+OWBRMSGDSC_SC+OWORD_8+BI_CMD_BUFFER

//	Start parsing commands
    $for(0; <16; 2) {
	and.nz.f0.1 (8)	NULLREG	CMD_BUFFER_D(%1,4)<0;1,0>	IS_INTRA_MB:ud		// Is it an "Intra" MB?
	or (1)	CMD_BUFFER_D(%1,2)<1>	CMD_BUFFER_D(%1,2)<0;1,0>	BIT21:ud	// Set "Use Scoreboard" for every MB
	shl	(2)	CMD_BUFFER_W(%1,2)<1>	CMD_BUFFER_W(%1,14)<0;1,0>	MB_SHIFT_MASK_B<2;2,1>:b	// Set HW SB masks
	mov (2)	CMD_BUFFER_B(%1,4)<2>	CMD_BUFFER_B(%1,20)<2;2,1>				// Set scoreboard (X,Y) for intra MB
	(-f0.1) mov (2)	CMD_BUFFER_W(%1,2)<1>	CMD_BUFFER_B(%1,20)<2;2,1>		// Set scoreboard (X,Y) for inter MB
	(f0.1) jmpi (1) Parse_8_Loop_%1

//	Inter Macroblock
//	Output MEDIA_OBJECT command in raster scan order
	mul (16) acc0<1>:uw	CMD_BUFFER_B(%1,21)<0;1,0>	PicWidthMB<0;1,0>:uw	// MB offset = Y*W
	add (16) acc0<1>:uw	acc0<8;8,1>:uw	CMD_BUFFER_B(%1,20)<0;1,0>			// MB offset = Y*W+X
 	shl (1)	MRF_INTER_WRITE_HEADER.2<1>:ud	acc0.2<0;1,0>:uw	6:uw		// Byte-aligned MB offset
 	mov (16)	MRF_INTER_WRITE_DATA0<1>:ud	CMD_BUFFER_D(%1)<8;8,1>	{Compr}	// Copy entire command to inter buffer
	mov	(16)	CMD_BUFFER_D(%1)<1>		0:ud	{Compr}						// Clear original command
	send (16)	NULLREGW	MRF_INTER_WRITE_HEADER	null:uw	DAPWRITE	MSG_LEN(2)+OWBWMSGDSC+OWORD_4+BI_CMD_BUFFER

Parse_8_Loop_%1:
	}

	add.z.f0.0 (1)	TotalMB<1>:w	TotalMB<0;1,0>:w	-8:w				// Update remaining number of 8-command blocks

//	Output modified intra commands
//	Write block 0
	mov (8)	MRF_INTRA_WRITE_HEADER.0<1>:ud	MRF_READ_HEADER_SRC.0<8;8,1>:ud
    $for(0; <4; 2) {
	mov (16)	MRF_CMD_BUF_D(%1)<1>	CMD_BUFFER_D(%1)<8;8,1>	{Compr}
	}
	send (16)	NULLREGW	MRF_INTRA_WRITE_HEADER	null:uw	DAPWRITE	MSG_LEN(4)+OWBWMSGDSC+OWORD_8+BI_CMD_BUFFER

//	Write block 1
	mov (8)	m1.0<1>:ud	MRF_READ_HEADER_SRC.0<8;8,1>:ud
	add	(1)	m1.2<1>:ud	MRF_READ_HEADER_SRC.2<0;1,0>:ud		128:ud		// Point to next 2-command block
	mov (16)	m2<1>:ud	CMD_BUFFER_D(4)<8;8,1>	{Compr}
	mov (16)	m4<1>:ud	CMD_BUFFER_D(6)<8;8,1>	{Compr}
	send (16)	NULLREGW	m1	null:uw	DAPWRITE	MSG_LEN(4)+OWBWMSGDSC+OWORD_8+BI_CMD_BUFFER

//	Write block 2
	add	(1)	MRF_INTRA_WRITE_HEADER.2<1>:ud	MRF_READ_HEADER_SRC.2<0;1,0>:ud		256:ud		// Point to next 2-command block
    $for(0; <4; 2) {
	mov (16)	MRF_CMD_BUF_D(%1)<1>	CMD_BUFFER_D(%1+8)<8;8,1>	{Compr}
	}
	send (16)	NULLREGW	MRF_INTRA_WRITE_HEADER	null:uw	DAPWRITE	MSG_LEN(4)+OWBWMSGDSC+OWORD_8+BI_CMD_BUFFER

//	Write block 3
	add	(1)	m1.2<1>:ud	MRF_READ_HEADER_SRC.2<0;1,0>:ud		384:ud		// Point to next 2-command block
	mov (16)	m2<1>:ud	CMD_BUFFER_D(12)<8;8,1>	{Compr}
	mov (16)	m4<1>:ud	CMD_BUFFER_D(14)<8;8,1>	{Compr}
	send (16)	NULLREGW	m1	null:uw	DAPWRITE	MSG_LEN(4)+OWBWMSGDSC+OWORD_8+BI_CMD_BUFFER

//	Update message header for next DAP read
	add (1) MRF_READ_HEADER_SRC.2<1>:ud	MRF_READ_HEADER_SRC.2<0;1,0>:ud	512:ud	// Point to next block of 8-commands

	cmp.z.f0.1 (1)	NULLREG	RemainderMB<0;1,0>:w	0:uw	// Check if remainder MB = 0
	(-f0.0) jmpi (1)	SetHWScoreboard_Loop			// Continue if more command blocks remain

SetHWScoreboard_Remainder:
//	f0.1 should have been set to indicate if RemainderMB = 0
//
	(f0.1) jmpi (1) SetHWScoreboard_Done				// Stop if all commands have been updated

//	Blindly load next 8 commands anyway
//
//	Load block 0 (Commands 0/1)
	mov (8)	MRF_READ_HEADER0.0<1>:ud	MRF_READ_HEADER_SRC.0<8;8,1>:ud
	send (16)	CMD_BUFFER_W(0)<1>	MRF_READ_HEADER0	null:uw	DAPREAD	RESP_LEN(4)+OWBRMSGDSC_SC+OWORD_8+BI_CMD_BUFFER

//	Load block 1  (Commands 2/3)
	mov (8)	MRF_READ_HEADER1.0<1>:ud	MRF_READ_HEADER_SRC.0<8;8,1>:ud
	add	(1)	MRF_READ_HEADER1.2<1>:ud	MRF_READ_HEADER_SRC.2<0;1,0>:ud		128:ud		// Point to next 2-command block
	send (16)	CMD_BUFFER_W(4)<1>	MRF_READ_HEADER1	null:uw	DAPREAD	RESP_LEN(4)+OWBRMSGDSC_SC+OWORD_8+BI_CMD_BUFFER

//	Load block 2  (Commands 4/5)
	mov (8)	MRF_READ_HEADER2.0<1>:ud	MRF_READ_HEADER_SRC.0<8;8,1>:ud
	add	(1)	MRF_READ_HEADER2.2<1>:ud	MRF_READ_HEADER_SRC.2<0;1,0>:ud		256:ud		// Point to next 2-command block
	send (16)	CMD_BUFFER_W(8)<1>	MRF_READ_HEADER2	null:uw	DAPREAD	RESP_LEN(4)+OWBRMSGDSC_SC+OWORD_8+BI_CMD_BUFFER

//	Load block 3  (Commands 6/7)
	mov (8)	MRF_READ_HEADER3.0<1>:ud	MRF_READ_HEADER_SRC.0<8;8,1>:ud
	add	(1)	MRF_READ_HEADER3.2<1>:ud	MRF_READ_HEADER_SRC.2<0;1,0>:ud		384:ud		// Point to next 2-command block
	send (16)	CMD_BUFFER_W(12)<1>	MRF_READ_HEADER3	null:uw	DAPREAD	RESP_LEN(4)+OWBRMSGDSC_SC+OWORD_8+BI_CMD_BUFFER

//	Initialize necessary pointers
	mov (1)	a0.1<1>:ud	((CMD_BUFFER_REG_OFF+1)*0x10000+CMD_BUFFER_REG_OFF)*32	// a0.2:w points to command buffer (first half)
																			// a0.3:w points to command buffer (second half)
//	Initialize Inter DAP write header
 	mov (8)	MRF_INTER_WRITE_HEADER<1>:ud	r0.0<8;8,1>:ud

SetHWScoreboard_Remainder_Loop:
	and.nz.f0.1 (8)	NULLREG	r[a0.2,4*4]<0;1,0>:ud	IS_INTRA_MB:ud		// Is it an "Intra" MB?
	add.z.f0.0 (1)	RemainderMB<1>:w	RemainderMB<0;1,0>:w	-1:w	// Decrement MB #
	or (1)	r[a0.2,2*4]<1>:ud	r[a0.2,2*4]<0;1,0>:ud	BIT21:ud		// Set "Use Scoreboard" for every MB
	shl	(2)	r[a0.2,2*2]<1>:uw	r[a0.2,14*2]<0;1,0>:uw	MB_SHIFT_MASK_B<2;2,1>:b	// Set HW SB masks
	mov (2)	r[a0.2,4*1]<2>:ub	r[a0.2,5*4]<2;2,1>:ub					// Set scoreboard (X,Y) for intra MB

	(-f0.1) mov (2)	r[a0.2,4*1]<1>:uw	r[a0.2,5*4]<2;2,1>:ub			// Set scoreboard (X,Y) for inter MB
	(f0.1) jmpi (1)	Output_Remainder_Intra

//	Inter Macroblock
//	Output MEDIA_OBJECT command in raster scan order
	mul (16) acc0<1>:uw	r[a0.2,21]<0;1,0>:ub	PicWidthMB<0;1,0>:uw	// MB offset = Y*W
	add (16) acc0<1>:uw	acc0<8;8,1>:uw	r[a0.2,20]<0;1,0>:ub			// MB offset = Y*W+X
 	shl (1)	MRF_INTER_WRITE_HEADER.2<1>:ud	acc0.2<0;1,0>:uw	6:uw	// Byte-aligned MB offset
 	mov (16)	MRF_INTER_WRITE_DATA0<1>:ud	r[a0.2]<8;8,1>:ud	{Compr}	// Copy entire command to inter buffer
	mov	(16)	r[a0.2]<1>:ud		0:ud	{Compr}						// Clear original command
	send (16)	NULLREGW	MRF_INTER_WRITE_HEADER	null:uw	DAPWRITE	MSG_LEN(2)+OWBWMSGDSC+OWORD_4+BI_CMD_BUFFER

Output_Remainder_Intra:
//	Intra MB command always output
	mov (8)	MRF_INTRA_WRITE_HEADER.0<1>:ud	MRF_READ_HEADER_SRC.0<8;8,1>:ud
 	mov (16)	MRF_CMD_BUF_D(0)<1>		r[a0.2]<8;8,1>:ud	{Compr}		// Copy entire command to intra buffer
	send (16)	NULLREGW	MRF_INTRA_WRITE_HEADER	null:uw	DAPWRITE	MSG_LEN(2)+OWBWMSGDSC+OWORD_4+BI_CMD_BUFFER

	add	(1)	MRF_READ_HEADER_SRC.2<1>:ud		MRF_READ_HEADER_SRC.2<0;1,0>:ud		64:ud	// Point to next command
	add (1)	a0.1<1>:ud	a0.1<0;1,0>:ud	0x00400040:ud					// Update pointers
	(-f0.0) jmpi (1)	SetHWScoreboard_Remainder_Loop

// All MBs have been decoded. Terminate the thread now
//
SetHWScoreboard_Done:
    END_THREAD

#if !defined(COMBINED_KERNEL)		// For standalone kernel only
.end_code

.end_kernel
#endif

// End of SetHWScoreboard
