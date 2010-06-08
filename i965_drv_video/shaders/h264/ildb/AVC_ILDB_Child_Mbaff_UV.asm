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

.kernel AVC_ILDB_CHILD_MBAFF_UV
#if defined(COMBINED_KERNEL)
ILDB_LABEL(AVC_ILDB_CHILD_UV):
#endif

#include "SetupVPKernel.asm"
#include "AVC_ILDB.inc"

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xE997:w
#endif

	// Setup temp buf used by load and save code
	#define BUF_B		RTempB
	#define BUF_W		RTempW
	#define BUF_D		RTempD

	// Init local variables
	mul (4)		ORIX_CUR<2>:w		ORIX<0;1,0>:w		16:w	{ NoDDClr }		// Expand X addr to bytes, repeat 4 times
	mul (4)		ORIY_CUR<2>:w		ORIY<0;1,0>:w		32:w	{ NoDDChk }		// Expand Y addr to bytes, repeat 4 times

	mov (2)		f0.0<1>:w		0:w

	mov	(1)		GateWayOffsetC:uw	ORIY:uw						// Use row # as Gateway offset

	//=== Null Kernel ===============================================================
//	jmpi ILDB_LABEL(POST_ILDB_UV)
	//===============================================================================

	//====================================================================================
	// Assuming the MB control data is laid out in scan line order in a rectangle with width = 16 bytes.
	// Control data has dimension of X x Y = 16 x N bytes, where N = W x H / 16
	// Each MB has 256 bytes of control data

	// For CRESTLINE, 256 bytes are stored in memory and fetched into GRF.
	// MB_offset = MBsCntX * CurRow + CurCol
	// Byte_offset = MB_offset * (256 << Mbaff_flag),	Mbaff_flag = 0 or 1.
	// Base address of a control data block = (x, y) = (0, y'=y/x), region width is 16 bytes
	// where y' = Byte_offset / 16 = MB_offset * (16 << Mbaff_flag)
	// MBCntrlDataOffsetY holds y'.

	// For BearLake-C, 64 bytes are stored in memory and dataport expands to 256 bytes.  Need to use a special read command on BL-C.
	// MB_offset = MBsCntX * CurRow + CurCol
	// Byte_offset = MB_offset * (64 << Mbaff_flag),	Mbaff_flag = 0 or 1.
	// MBCntrlDataOffsetY holds globel byte offset.
	
#if !defined(DEV_CL)
	mul (1) CntrlDataOffsetY:ud		MBsCntX:w 				ORIY:w
	add (1) CntrlDataOffsetY:ud		CntrlDataOffsetY:ud		ORIX:w
	mul (1) CntrlDataOffsetY:ud		CntrlDataOffsetY:ud		128:uw
#endif
	//====================================================================================

	add (1)		ORIX_LEFT:w			ORIX_LEFT:w			-4:w
	add (1)		ORIY_TOP:w			ORIY_TOP:w			-4:w

	//=========== Process Top MB ============
    and (1)  	BitFields:w  		BitFields:w		TopFieldFlag:w		// Reset BotFieldFlag

	// Build a ramp from 0 to 15
	mov	(16)	RRampW(0)<1>		RampConstC<0;8,1>:ub
	add (8)		RRampW(0,8)<1>		RRampW(0,8)			8:w				// RRampW = ramp 15-0

ILDB_LABEL(RE_ENTRY_UV):	// for bootom field

	// Load current MB control data
#if defined(DEV_CL)
	#include "Load_ILDB_Cntrl_Data_64DW.asm"	// Crestline
#else
	#include "Load_ILDB_Cntrl_Data_16DW.asm"	// Cantiga and beyond
#endif

	// Init addr register for vertical control data
	mov (1)		ECM_AddrReg<1>:w		CNTRL_DATA_BASE:w		// Init ECM_AddrReg

	// Use free cycles here
	// Check loaded control data
	and.z.f0.1  (16) null<1>:uw	r[ECM_AddrReg, wEdgeCntlMap_IntLeftVert]<16;16,1>:uw	0xFFFF:uw		// Skip ILDB?		
	and.nz.f0.0  (1) null:w		r[ECM_AddrReg, ExtBitFlags]:ub		DISABLE_ILDB_FLAG:w		// Skip ILDB?

	// Set DualFieldMode for all data read, write and deblocking
	and	(1)	CTemp1_W:uw		r[ECM_AddrReg, BitFlags]:ub		FieldModeAboveMbFlag+FieldModeCurrentMbFlag:uw

	// Get Vert Edge Pattern (frame vs. field MBs)
	and	(1)	VertEdgePattern:uw		r[ECM_AddrReg, BitFlags]:ub		FieldModeLeftMbFlag+FieldModeCurrentMbFlag:uw

	(f0.1.all16h)	jmpi 	ILDB_LABEL(SKIP_ILDB_UV)						// Skip ILDB
	(f0.0)			jmpi 	ILDB_LABEL(SKIP_ILDB_UV)						// Skip ILDB

	// Set DualFieldMode for all data read, write and deblocking
//	and	(1)	CTemp1_W:uw		r[ECM_AddrReg, BitFlags]:ub		FieldModeAboveMbFlag+FieldModeCurrentMbFlag:uw
	cmp.z.f0.0	(1)	null:w	CTemp1_W:uw		ABOVE_FIELD_CUR_FRAME:w
	and (1)		DualFieldMode:w		f0.0:w		0x0001:w

	#include "load_Cur_UV_8x8T_Mbaff.asm"		// Load transposed data 8x8
	#include "load_Left_UV_2x8T_Mbaff.asm"				// Load left MB (2x8) UV data from memory if exists

	#include "Transpose_Cur_UV_8x8.asm"
	#include "Transpose_Left_UV_2x8.asm"
	

	//---------- Perform vertical ILDB filting on UV ----------
	#include "AVC_ILDB_Filter_Mbaff_UV_v.asm"	
	//---------------------------------------------------------

	#include "save_Left_UV_8x2T_Mbaff.asm"				// Write left MB (2x8) Y data to memory if exists
	#include "load_Top_UV_8x2_Mbaff.asm"				// Load top MB (8x2) Y data from memory if exists

	#include "Transpose_Cur_UV_8x8.asm"					// Transpose a MB for horizontal edge de-blocking 

	//---------- Perform horizontal ILDB filting on UV ----------
	#include "AVC_ILDB_Filter_Mbaff_UV_h.asm"	
	//-----------------------------------------------------------

	#include "save_Cur_UV_8x8_Mbaff.asm"				// Write 8x8
	#include "save_Top_UV_8x2_Mbaff.asm"				// Write top MB (8x2) if not the top row

	//-----------------------------------------------------------
ILDB_LABEL(SKIP_ILDB_UV):
	
	and.z.f0.0 (1) 	null:w		BitFields:w		BotFieldFlag:w

	//=========== Process Bottom MB ============
    or (1)  	BitFields:w  	BitFields:w		BotFieldFlag:w	// Set BotFieldFlag to 1
	(f0.0) jmpi		ILDB_LABEL(RE_ENTRY_UV)							// Loop back for bottom deblocking

	// Fall through to finish

	//=========== Check write commit of the last write ============
    mov (8)	WritebackResponse(0)<1>		WritebackResponse(0)	

ILDB_LABEL(POST_ILDB_UV):	
	
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
	#include "AVC_ILDB_Chroma_Core_Mbaff.asm"
	
#if !defined(COMBINED_KERNEL)		// For standalone kernel only
.end_code

.end_kernel
#endif
