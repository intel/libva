/*
 * All HWMC kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Kernel name: AllAVC.asm
//
// All HWMC kernels merged into this file
//
//  $Revision: 2 $
//  $Date: 9/10/06 2:02a $
//

// Note: To enable SW scoreboard for ILK AVC kernels, simply toggle the HW_SCOREBOARD 
//		 and SW_SCOREBOARD definition as described below.
//
// ----------------------------------------------------
//  Main: ALLINTRA
// ----------------------------------------------------

#define	COMBINED_KERNEL
#define	ENABLE_ILDB

//	WA for *Stim tool issue, should be removed later

#ifdef DEV_ILK
#define INSTFACTOR	2	// 128-bit count as 2 instructions
#else
#define INSTFACTOR	1	// 128-bit is 1 instruction
#endif	// DEV_ILK

#ifdef DEV_CTG
  #define SW_SCOREBOARD		// SW Scoreboard should be enabled for CTG and earlier
  #undef HW_SCOREBOARD		// HW Scoreboard should be disabled for CTG and earlier
#else
  #define HW_SCOREBOARD		// HW Scoreboard should be enabled for ILK and beyond
  #undef SW_SCOREBOARD		// SW Scoreboard should be disabled for ILK and beyond
#endif	// DEV_CTG
#include "export.inc"
#if defined(_EXPORT)
	#include "AllAVC_Export.inc"
#elif defined(_BUILD)
	#include "AllAVC.ich"			// ISAasm dumped .exports
	#include "AllAVC_Export.inc"	// Keep jumping targets aligned, only for CTG and beyond
	#include "AllAVC_Build.inc"
#else
#endif

.kernel AllAVC

// Build all intra prediction kernels
//
#ifdef INTRA_16x16_PAD_NENOP
    $for(0; <INTRA_16x16_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef INTRA_16x16_PAD_NOP
    $for(0; <INTRA_16x16_PAD_NOP; 1) {
	nop
	}
#endif
    #include "Intra_16x16.asm"

#ifdef INTRA_8x8_PAD_NENOP
    $for(0; <INTRA_8x8_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef INTRA_8x8_PAD_NOP
    $for(0; <INTRA_8x8_PAD_NOP; 1) {
	nop
	}
#endif
    #include "Intra_8x8.asm"

#ifdef INTRA_4x4_PAD_NENOP
    $for(0; <INTRA_4x4_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef INTRA_4x4_PAD_NOP
    $for(0; <INTRA_4x4_PAD_NOP; 1) {
	nop
	}
#endif
    #include "Intra_4x4.asm"

#ifdef INTRA_PCM_PAD_NENOP
    $for(0; <INTRA_PCM_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef INTRA_PCM_PAD_NOP
    $for(0; <INTRA_PCM_PAD_NOP; 1) {
	nop
	}
#endif
    #include "Intra_PCM.asm"

// Build FrameMB_Motion kernel
//
#define FRAME

  #ifdef FRAME_MB_PAD_NENOP
    $for(0; <FRAME_MB_PAD_NENOP; 1) {
	nenop
	}
  #endif
  #ifdef FRAME_MB_PAD_NOP
    $for(0; <FRAME_MB_PAD_NOP; 1) {
	nop
	}
  #endif
    #include "AVCMCInter.asm"
#undef  FRAME

// Build FieldMB_Motion kernel
//
#define FIELD

  #ifdef FIELD_MB_PAD_NENOP
    $for(0; <FIELD_MB_PAD_NENOP; 1) {
	nenop
	}
  #endif
  #ifdef FIELD_MB_PAD_NOP
    $for(0; <FIELD_MB_PAD_NOP; 1) {
	nop
	}
  #endif
    #include "AVCMCInter.asm"
#undef  FIELD

// Build MBAff_Motion kernel
//
#define MBAFF

  #ifdef MBAFF_MB_PAD_NENOP
    $for(0; <MBAFF_MB_PAD_NENOP; 1) {
	nenop
	}
  #endif
  #ifdef MBAFF_MB_PAD_NOP
    $for(0; <MBAFF_MB_PAD_NOP; 1) {
	nop
	}
  #endif
    #include "AVCMCInter.asm"
#undef  MBAFF

#ifdef SW_SCOREBOARD    

// SW scoreboard kernel for non-MBAFF
//
#ifdef SCOREBOARD_PAD_NENOP
    $for(0; <SCOREBOARD_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef SCOREBOARD_PAD_NOP
    $for(0; <SCOREBOARD_PAD_NOP; 1) {
	nop
	}
#endif
    #include "scoreboard.asm"

//	SW scoreboard kernel for MBAFF

#ifdef SCOREBOARD_MBAFF_PAD_NENOP
    $for(0; <SCOREBOARD_MBAFF_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef SCOREBOARD_MBAFF_PAD_NOP
    $for(0; <SCOREBOARD_MBAFF_PAD_NOP; 1) {
	nop
	}
#endif
    #include "scoreboard_MBAFF.asm"

#elif defined(HW_SCOREBOARD)
 
// SetHWscoreboard kernel for non-MBAFF
//
#ifdef SETHWSCOREBOARD_PAD_NENOP
    $for(0; <SETHWSCOREBOARD_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef SETHWSCOREBOARD_PAD_NOP
    $for(0; <SETHWSCOREBOARD_PAD_NOP; 1) {
	nop
	}
#endif
    #include "SetHWScoreboard.asm"

//	SetHWscoreboard kernel for MBAFF

#ifdef SETHWSCOREBOARD_MBAFF_PAD_NENOP
    $for(0; <SETHWSCOREBOARD_MBAFF_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef SETHWSCOREBOARD_MBAFF_PAD_NOP
    $for(0; <SETHWSCOREBOARD_MBAFF_PAD_NOP; 1) {
	nop
	}
#endif
    #include "SetHWScoreboard_MBAFF.asm"

#endif	// SW_SCOREBOARD

#ifdef BSDRESET_PAD_NENOP
    $for(0; <BSDRESET_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef BSDRESET_PAD_NOP
    $for(0; <BSDRESET_PAD_NOP; 1) {
	nop
	}
#endif
    #include "BSDReset.asm"

#ifdef DCRESETDUMMY_PAD_NENOP
    $for(0; <DCRESETDUMMY_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef DCRESETDUMMY_PAD_NOP
    $for(0; <DCRESETDUMMY_PAD_NOP; 1) {
	nop
	}
#endif
    #include "DCResetDummy.asm"

#ifdef ENABLE_ILDB

// Build all ILDB kernels
//
//	Undefine some previous defined symbols since they will be re-defined/re-declared in ILDB kernels
#undef	A
#undef	B
#undef	p0
#undef	p1

#define MSGPAYLOADB MSGPAYLOADB_ILDB
#define MSGPAYLOADW MSGPAYLOADW_ILDB
#define MSGPAYLOADD MSGPAYLOADD_ILDB
#define MSGPAYLOADF MSGPAYLOADF_ILDB

//				< Frame ILDB >
#define _PROGRESSIVE
#define ILDB_LABEL(x)	x##_ILDB_FRAME
#ifdef AVC_ILDB_ROOT_Y_ILDB_FRAME_PAD_NENOP
    $for(0; <AVC_ILDB_ROOT_Y_ILDB_FRAME_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef AVC_ILDB_ROOT_Y_ILDB_FRAME_PAD_NOP
    $for(0; <AVC_ILDB_ROOT_Y_ILDB_FRAME_PAD_NOP; 1) {
	nop
	}
#endif
    #include "AVC_ILDB_Root_Y.asm"

#ifdef AVC_ILDB_CHILD_Y_ILDB_FRAME_PAD_NENOP
    $for(0; <AVC_ILDB_CHILD_Y_ILDB_FRAME_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef AVC_ILDB_CHILD_Y_ILDB_FRAME_PAD_NOP
    $for(0; <AVC_ILDB_CHILD_Y_ILDB_FRAME_PAD_NOP; 1) {
	nop
	}
#endif
    #include "AVC_ILDB_Child_Y.asm"

#ifdef AVC_ILDB_ROOT_UV_ILDB_FRAME_PAD_NENOP
    $for(0; <AVC_ILDB_ROOT_UV_ILDB_FRAME_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef AVC_ILDB_ROOT_UV_ILDB_FRAME_PAD_NOP
    $for(0; <AVC_ILDB_ROOT_UV_ILDB_FRAME_PAD_NOP; 1) {
	nop
	}
#endif
    #include "AVC_ILDB_Root_UV.asm"

#ifdef AVC_ILDB_CHILD_UV_ILDB_FRAME_PAD_NENOP
    $for(0; <AVC_ILDB_CHILD_UV_ILDB_FRAME_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef AVC_ILDB_CHILD_UV_ILDB_FRAME_PAD_NOP
    $for(0; <AVC_ILDB_CHILD_UV_ILDB_FRAME_PAD_NOP; 1) {
	nop
	}
#endif
    #include "AVC_ILDB_Child_UV.asm"
#undef ILDB_LABEL
#undef _PROGRESSIVE

//				< Field ILDB >
#define _FIELD
#define ILDB_LABEL(x)	x##_ILDB_FIELD
#ifdef AVC_ILDB_ROOT_Y_ILDB_FIELD_PAD_NENOP
    $for(0; <AVC_ILDB_ROOT_Y_ILDB_FIELD_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef AVC_ILDB_ROOT_Y_ILDB_FIELD_PAD_NOP
    $for(0; <AVC_ILDB_ROOT_Y_ILDB_FIELD_PAD_NOP; 1) {
	nop
	}
#endif
    #include "AVC_ILDB_Root_Field_Y.asm"

#ifdef AVC_ILDB_CHILD_Y_ILDB_FIELD_PAD_NENOP
    $for(0; <AVC_ILDB_CHILD_Y_ILDB_FIELD_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef AVC_ILDB_CHILD_Y_ILDB_FIELD_PAD_NOP
    $for(0; <AVC_ILDB_CHILD_Y_ILDB_FIELD_PAD_NOP; 1) {
	nop
	}
#endif
    #include "AVC_ILDB_Child_Field_Y.asm"

#ifdef AVC_ILDB_ROOT_UV_ILDB_FIELD_PAD_NENOP
    $for(0; <AVC_ILDB_ROOT_UV_ILDB_FIELD_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef AVC_ILDB_ROOT_UV_ILDB_FIELD_PAD_NOP
    $for(0; <AVC_ILDB_ROOT_UV_ILDB_FIELD_PAD_NOP; 1) {
	nop
	}
#endif
    #include "AVC_ILDB_Root_Field_UV.asm"

#ifdef AVC_ILDB_CHILD_UV_ILDB_FIELD_PAD_NENOP
    $for(0; <AVC_ILDB_CHILD_UV_ILDB_FIELD_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef AVC_ILDB_CHILD_UV_ILDB_FIELD_PAD_NOP
    $for(0; <AVC_ILDB_CHILD_UV_ILDB_FIELD_PAD_NOP; 1) {
	nop
	}
#endif
    #include "AVC_ILDB_Child_Field_UV.asm"
#undef ILDB_LABEL
#undef _FIELD

//				< MBAFF Frame ILDB >
#define _MBAFF
#define ILDB_LABEL(x)	x##_ILDB_MBAFF
#ifdef AVC_ILDB_ROOT_Y_ILDB_MBAFF_PAD_NENOP
    $for(0; <AVC_ILDB_ROOT_Y_ILDB_MBAFF_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef AVC_ILDB_ROOT_Y_ILDB_MBAFF_PAD_NOP
    $for(0; <AVC_ILDB_ROOT_Y_ILDB_MBAFF_PAD_NOP; 1) {
	nop
	}
#endif
    #include "AVC_ILDB_Root_Mbaff_Y.asm"

#ifdef AVC_ILDB_CHILD_Y_ILDB_MBAFF_PAD_NENOP
    $for(0; <AVC_ILDB_CHILD_Y_ILDB_MBAFF_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef AVC_ILDB_CHILD_Y_ILDB_MBAFF_PAD_NOP
    $for(0; <AVC_ILDB_CHILD_Y_ILDB_MBAFF_PAD_NOP; 1) {
	nop
	}
#endif
    #include "AVC_ILDB_Child_Mbaff_Y.asm"

#ifdef AVC_ILDB_ROOT_UV_ILDB_MBAFF_PAD_NENOP
    $for(0; <AVC_ILDB_ROOT_UV_ILDB_MBAFF_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef AVC_ILDB_ROOT_UV_ILDB_MBAFF_PAD_NOP
    $for(0; <AVC_ILDB_ROOT_UV_ILDB_MBAFF_PAD_NOP; 1) {
	nop
	}
#endif
    #include "AVC_ILDB_Root_Mbaff_UV.asm"

#ifdef AVC_ILDB_CHILD_UV_ILDB_MBAFF_PAD_NENOP
    $for(0; <AVC_ILDB_CHILD_UV_ILDB_MBAFF_PAD_NENOP; 1) {
	nenop
	}
#endif
#ifdef AVC_ILDB_CHILD_UV_ILDB_MBAFF_PAD_NOP
    $for(0; <AVC_ILDB_CHILD_UV_ILDB_MBAFF_PAD_NOP; 1) {
	nop
	}
#endif
    #include "AVC_ILDB_Child_Mbaff_UV.asm"
#undef ILDB_LABEL
#undef _MBAFF

#endif		// ENABLE_ILDB

AllAVC_END:
nop
// End of AllAVC

.end_code

.end_kernel

