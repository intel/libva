/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AVC Child Kernel (Vertical and horizontal de-block a 4:2:0 MB UV comp)
//
// First de-block vertical edges from left to right.
// Second de-block horizontal edge from top to bottom.
// 
// For 4:2:0, chroma is always de-blocked at 8x8.
// NV12 format allows to filter U and V together.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define AVC_ILDB

.kernel AVC_ILDB_CHILD_UV
#if defined(COMBINED_KERNEL)
ILDB_LABEL(AVC_ILDB_CHILD_UV):
#endif

#include "SetupVPKernel.asm"
#include "AVC_ILDB.inc"

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0x9997:w
#endif

	// Init local variables
	shl (8)		ORIX_CUR<1>:w		ORIX<0;2,1>:w		4:w		// Expand addr to bytes, repeat (x,y) 4 times

	// Init addr register for vertical control data
	mov (1)		ECM_AddrReg<1>:w		CNTRL_DATA_BASE:w		// Init ECM_AddrReg

	//=== Null Kernel ===============================================================
//	jmpi ILDB_LABEL(POST_ILDB_UV_UV)
	//===============================================================================

#if defined(DEV_CL)	
	mov	(1)		acc0.0:w		240:w	
#else
	//====================================================================================
	// For BearLake-C, 64 bytes are stored in memory and dataport expands to 256 bytes.  Need to use a special read command on BL-C.
	// MB_offset = MBsCntX * CurRow + CurCol
	// MBCntrlDataOffsetY = globel_byte_offset = MB_offset * 64
	mul (1) CntrlDataOffsetY:ud		MBsCntX:w 				ORIY:w
	add (1) CntrlDataOffsetY:ud		CntrlDataOffsetY:ud		ORIX:w

	// Assign to MSGSRC.2:ud for memory access
	// mul (1) CntrlDataOffsetY:ud		CntrlDataOffsetY:ud		64:uw
	mul (1) MSGSRC.2:ud		CntrlDataOffsetY:ud		64:uw
		
	mov	(1)		acc0.0:w		320:w	
#endif
	mac (1)		URBOffsetC:w	ORIY:w			4:w				// UV URB entries are right after Y entries		


	// Init local variables
//	shl (8)		ORIX_CUR<1>:w		ORIX<0;2,1>:w		4:w		// Expand addr to bytes, repeat (x,y) 4 times
	add (1)		ORIX_LEFT:w			ORIX_LEFT:w			-4:w
	add (1)		ORIY_TOP:w			ORIY_TOP:w			-4:w

	// Build a ramp from 0 to 15
	mov	(16)	RRampW(0)<1>		RampConstC<0;8,1>:ub
	add (8)		RRampW(0,8)<1>		RRampW(0,8)			8:w		// RRampW = ramp 15-0

	// Load current MB control data
#if defined(DEV_CL)
	#if defined(_APPLE)
		#include "Load_ILDB_Cntrl_Data_22DW.asm"	// Crestline for Apple, progressive only
	#else
		#include "Load_ILDB_Cntrl_Data_64DW.asm"	// Crestline
	#endif	
#else
	#include "Load_ILDB_Cntrl_Data_16DW.asm"	// Cantiga and beyond
#endif

	// Check loaded control data
	#if defined(_APPLE)
		and.z.f0.1  (8) null<1>:uw	r[ECM_AddrReg, wEdgeCntlMap_IntLeftVert]<8;8,1>:uw		0xFFFF:uw		// Skip ILDB?
		(f0.1) and.z.f0.1 (2) null<1>:uw	r[ECM_AddrReg, wEdgeCntlMapA_ExtTopHorz0]<2;2,1>:uw		0xFFFF:uw		// Skip ILDB?
	#else
		and.z.f0.1  (16) null<1>:uw	r[ECM_AddrReg, wEdgeCntlMap_IntLeftVert]<16;16,1>:uw	0xFFFF:uw		// Skip ILDB?		
	#endif	
		
	and.nz.f0.0  (1) null:w		r[ECM_AddrReg, ExtBitFlags]:ub		DISABLE_ILDB_FLAG:w		// Skip ILDB?
	
	mov	(1)		GateWayOffsetC:uw	ORIY:uw		// Use row # as Gateway offset

	#if defined(_APPLE)
		(f0.1.all8h)	jmpi 	ILDB_LABEL(READ_FOR_URB_UV)				// Skip ILDB
	#else
		(f0.1.all16h)	jmpi 	ILDB_LABEL(READ_FOR_URB_UV)				// Skip ILDB
	#endif	

	(f0.0)			jmpi 	ILDB_LABEL(READ_FOR_URB_UV)					// Skip ILDB



	#include "load_Cur_UV_8x8T.asm"				// Load transposed data 8x8
//	#include "load_Left_UV_2x8T.asm"
	#include "load_Top_UV_8x2.asm"				// Load top MB (8x2) Y data from memory if exists

	#include "Transpose_Cur_UV_8x8.asm"
//	#include "Transpose_Left_UV_2x8.asm"


	//---------- Perform vertical ILDB filting on UV ----------
	#include "AVC_ILDB_Filter_UV_v.asm"	
	//---------------------------------------------------------

	#include "save_Left_UV_8x2T.asm"			// Write left MB (2x8) Y data to memory if exists
	#include "Transpose_Cur_UV_8x8.asm"			// Transpose a MB for horizontal edge de-blocking 

	//---------- Perform horizontal ILDB filting on UV ----------
	#include "AVC_ILDB_Filter_UV_h.asm"	
	//-----------------------------------------------------------

	#include "save_Cur_UV_8x8.asm"				// Write 8x8
	#include "save_Top_UV_8x2.asm"				// Write top MB (8x2) if not the top row

	//---------- Write right most 4 columns of cur MB to URB ----------
	// Transpose the right most 2 cols 2x8 (word) in GRF to 8x2 in BUF_D.  It is 2 left most cols in cur MB.
	#include "Transpose_Cur_UV_2x8.asm"						
		
ILDB_LABEL(WRITE_URB_UV):
	mov (8)		m1<1>:ud		LEFT_TEMP_D(1)<8;8,1>			// Copy 1 GRF to 1 URB entry (U+V)
	
	#include "writeURB_UV_Child.asm"	
	//-----------------------------------------------------------------

	//=========== Check write commit of the last write ============
    mov (8)	WritebackResponse(0)<1>		WritebackResponse(0)	

ILDB_LABEL(POST_ILDB_UV):
	//---------------------------------		
	
	// Send notification thru Gateway to root thread, update chroma Status[CurRow]
	#include "AVC_ILDB_ForwardMsg.asm"

#if !defined(GW_DCN)		// For non-ILK chipsets
	//child send EOT : Request type = 1
	END_CHILD_THREAD
#endif	// !defined(DEV_ILK)
	
	// The thread finishs here
	//------------------------------------------------------------------------------
	
ILDB_LABEL(READ_FOR_URB_UV):
	// Still need to prepare URB data for the right neighbor MB
	#include "load_Cur_UV_Right_Most_2x8.asm"		// Load cur MB ( right most 4x16) Y data from memory
	#include "Transpose_Cur_UV_Right_Most_2x8.asm"						
//	jmpi ILDB_LABEL(WRITE_URB_UV)

	mov (8)		m1<1>:ud		LEFT_TEMP_D(1)<8;8,1>			// Copy 1 GRF to 1 URB entry (U+V)
	
	#include "writeURB_UV_Child.asm"	
	//-----------------------------------------------------------------

	// Send notification thru Gateway to root thread, update chroma Status[CurRow]
	#include "AVC_ILDB_ForwardMsg.asm"

#if !defined(GW_DCN)		// For non-ILK chipsets
	//child send EOT : Request type = 1
	END_CHILD_THREAD
#endif	// !defined(DEV_ILK)
	
	// The thread finishs here
	//------------------------------------------------------------------------------
	
	
	////////////////////////////////////////////////////////////////////////////////
	// Include other subrutines being called
//	#include "AVC_ILDB_Luma_Core.asm"
	#include "AVC_ILDB_Chroma_Core.asm"

	
#if !defined(COMBINED_KERNEL)		// For standalone kernel only
.end_code

.end_kernel
#endif
