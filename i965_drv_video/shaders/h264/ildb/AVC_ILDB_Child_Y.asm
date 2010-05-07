/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AVC Child Kernel (Vertical and horizontal de-block a 4:2:0 MB Y comp)
//
// First, de-block vertical edges from left to right.
// Second, de-block horizontal edge from top to bottom.
// 
// If transform_size_8x8_flag = 1, luma is de-blocked at 8x8.  Otherwise, luma is de-blocked at 4x4.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define AVC_ILDB

.kernel AVC_ILDB_CHILD_Y
#if defined(COMBINED_KERNEL)
ILDB_LABEL(AVC_ILDB_CHILD_Y):
#endif

#include "SetupVPKernel.asm"
#include "AVC_ILDB.inc"

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0x9998:w
#endif

	// Init local variables
	shl (8)		ORIX_CUR<1>:w		ORIX<0;2,1>:w		4:w		// Expand addr to bytes, repeat (x,y) 4 times

	// Init addr register for vertical control data
	mov (1)		ECM_AddrReg<1>:w	CNTRL_DATA_BASE:w			// Init edge control map AddrReg

	//=== Null Kernel ===============================================================
//	jmpi ILDB_LABEL(POST_ILDB_Y)
	//===============================================================================

	mul	(1)		URBOffsetC:uw	ORIY:uw		4:w	
	
#if !defined(DEV_CL)	
	//====================================================================================
	// For BearLake-C, 64 bytes are stored in memory and dataport expands to 256 bytes.  Need to use a special read command on BL-C.
	// MB_offset = MBsCntX * CurRow + CurCol
	// MBCntrlDataOffsetY = globel_byte_offset = MB_offset * 64
	mul (1) CntrlDataOffsetY:ud		MBsCntX:w 				ORIY:w
	add (1) CntrlDataOffsetY:ud		CntrlDataOffsetY:ud		ORIX:w
		
	// Assign to MSGSRC.2:ud for memory access
	// mul (1) CntrlDataOffsetY:ud		CntrlDataOffsetY:ud		64:uw
	mul (1) MSGSRC.2:ud		CntrlDataOffsetY:ud		64:uw		
	
#endif

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

	// Use free cycles here
	add (1)		ORIX_LEFT:w			ORIX_LEFT:w			-4:w
//	add (1)		ORIY_TOP:w			ORIY_TOP:w			-4:w
	mov	(1)		GateWayOffsetC:uw	ORIY:uw						// Use row # as Gateway offset

	#if defined(_APPLE)
		(f0.1.all8h)	jmpi 	ILDB_LABEL(READ_FOR_URB_Y)				// Skip ILDB
	#else
		(f0.1.all16h)	jmpi 	ILDB_LABEL(READ_FOR_URB_Y)				// Skip ILDB
	#endif

	(f0.0)			jmpi 	ILDB_LABEL(READ_FOR_URB_Y)					// Skip ILDB

	add (1)		ORIY_TOP:w			ORIY_TOP:w			-4:w

	// Bettr performance is observed if boundary MBs are not checked and skipped.
	
	#include "load_Cur_Y_16x16T.asm"				// Load cur MB Y, 16x16, transpose
//	#include "load_Left_Y_4x16T.asm"				// Load left MB (4x16) Y data from memory
	#include "load_Top_Y_16x4.asm"					// Load top MB (16x4) Y data from memory

	#include "Transpose_Cur_Y_16x16.asm"
//	#include "Transpose_Left_Y_4x16.asm"

	//---------- Perform vertical ILDB filting on Y ---------
	#include "AVC_ILDB_Filter_Y_v.asm"	
	//-------------------------------------------------------

	#include "save_Left_Y_16x4T.asm"				// Write left MB (4x16) Y data to memory
	#include "Transpose_Cur_Y_16x16.asm"			// Transpose a MB for horizontal edge de-blocking 

	//---------- Perform horizontal ILDB filting on Y -------
	#include "AVC_ILDB_Filter_Y_h.asm"	
	//-------------------------------------------------------

	#include "save_Cur_Y_16x16.asm"					// Write cur MB (16x16)
	#include "save_Top_Y_16x4.asm"					// Write top MB (16x4)

	//---------- Write right most 4 columns of cur MB to URB ----------
	// Transpose the right most 4 cols 4x16 in GRF to 16x4 in LEFT_TEMP_B.  It is 4 left most cols in cur MB.	
	#include "Transpose_Cur_Y_4x16.asm"						
	
ILDB_LABEL(WRITE_URB_Y):
	// Note: LEFT_TEMP_B(2) = TOP_TEMP_B(0), TOP_TEMP_B must be avail
	mov (16)	m1<1>:ud		LEFT_TEMP_D(2)<8;8,1>		// Copy 2 GRFs to 2 URB entries (Y)
	
	#include "writeURB_Y_Child.asm"	
	//-----------------------------------------------------------------

	//=========== Check write commit of the last write ============
    mov (8)	WritebackResponse(0)<1>		WritebackResponse(0)	

ILDB_LABEL(POST_ILDB_Y):
	// Send notification thru Gateway to root thread, update luma Status[CurRow]
	#include "AVC_ILDB_ForwardMsg.asm"	

#if !defined(GW_DCN)		// For non-ILK chipsets
	//child send EOT : Request type = 1
	END_CHILD_THREAD
#endif	// !defined(DEV_ILK)
	
	// The thread finishs here
	//------------------------------------------------------------------------------

ILDB_LABEL(READ_FOR_URB_Y):
	// Still need to prepare URB data for the right neighbor MB
	#include "load_Cur_Y_Right_Most_4x16.asm"		// Load cur MB ( right most 4x16) Y data from memory
	#include "Transpose_Cur_Y_Right_Most_4x16.asm"						
//	jmpi ILDB_LABEL(WRITE_URB_Y)

	// Note: LEFT_TEMP_B(2) = TOP_TEMP_B(0), TOP_TEMP_B must be avail
	mov (16)	m1<1>:ud		LEFT_TEMP_D(2)<8;8,1>		// Copy 2 GRFs to 2 URB entries (Y)
	
	#include "writeURB_Y_Child.asm"	
	//-----------------------------------------------------------------

	// Send notification thru Gateway to root thread, update luma Status[CurRow]
	#include "AVC_ILDB_ForwardMsg.asm"	

#if !defined(GW_DCN)		// For non-ILK chipsets
	//child send EOT : Request type = 1
	END_CHILD_THREAD
#endif	// !defined(DEV_ILK)
	
	// The thread finishs here
	//------------------------------------------------------------------------------
	
	////////////////////////////////////////////////////////////////////////////////
	// Include other subrutines being called
	#include "AVC_ILDB_Luma_Core.asm"
//	#include "AVC_ILDB_Chroma_Core.asm"

	
#if !defined(COMBINED_KERNEL)		// For standalone kernel only
.end_code

.end_kernel
#endif
