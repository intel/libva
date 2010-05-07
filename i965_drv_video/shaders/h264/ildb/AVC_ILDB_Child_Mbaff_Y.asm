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
//	***** MBAFF Mode *****
//	This version deblocks top MB first, followed by bottom MB.
//
//	Need variable CurMB	to indicate top MB or bottom MB (CurMB = 0 or 1).  
//	We can use BotFieldFlag in BitFields to represent it.
//
//  Usage:
// 	1) Access control data for top 
//		CntrlDataOffsetY + CurMB  * Control data block size		(64 DWs for CL, 16 DWs for BLC)
//
// 	2) Load frame/field video data based on flags: FieldModeCurrentMbFlag, FieldModeLeftMbFlag, FieldModeaboveMbFlag, 
//
//	E.g. 
//	if (pCntlData->BitField & FieldModeCurrentMbFlag)
//		cur_y = ORIX_CUR.y + CurMB * 1;				// Add field vertical offset for bot field MB .
//	else
//		cur_y = ORIX_CUR.y + CurMB * MB_Rows_Y;		// Add bottom MB vertical offset for bot MB
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define AVC_ILDB

.kernel AVC_ILDB_CHILD_MBAFF_Y
#if defined(COMBINED_KERNEL)
ILDB_LABEL(AVC_ILDB_CHILD_Y):
#endif

#include "SetupVPKernel.asm"
#include "AVC_ILDB.inc"

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xE998:w
#endif

	// Setup temp buf used by load and save code
	#define BUF_B		RTempB				
	#define BUF_D		RTempD
	
	// Init local variables
	// These coordinates are in progressive fashion
	mul (4)		ORIX_CUR<2>:w		ORIX<0;1,0>:w		16:w	{ NoDDClr }		// Expand X addr to bytes, repeat 4 times
	mul (4)		ORIY_CUR<2>:w		ORIY<0;1,0>:w		32:w	{ NoDDChk }		// Expand Y addr to bytes, repeat 4 times

	mov (2)		f0.0<1>:w		0:w
	
	mov	(1)		GateWayOffsetC:uw	ORIY:uw						// Use row # as Gateway offset

	//=== Null Kernel ===============================================================
//	jmpi POST_ILDB
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
    and (1)  	BitFields:w  		BitFields:w		TopFieldFlag:w	// Reset BotFieldFlag

RE_ENTRY:	// for bootom field

	// Load current MB control data
#if defined(DEV_CL)
	#include "Load_ILDB_Cntrl_Data_64DW.asm"	// Crestline
#else
	#include "Load_ILDB_Cntrl_Data_16DW.asm"	// Cantiga and beyond
#endif

	// Init addr register for vertical control data
	mov (1)		ECM_AddrReg<1>:w	CNTRL_DATA_BASE:w			// Init edge control map AddrReg

	// Check loaded control data
	and.z.f0.1  (16) null<1>:uw	r[ECM_AddrReg, wEdgeCntlMap_IntLeftVert]<16;16,1>:uw	0xFFFF:uw		// Skip ILDB?		
	and.nz.f0.0  (1) null:w		r[ECM_AddrReg, ExtBitFlags]:ub		DISABLE_ILDB_FLAG:w		// Skip ILDB?

	// Use free cycles here
	// Set DualFieldMode for all data read, write and deblocking
	and	(1)	CTemp1_W:uw		r[ECM_AddrReg, BitFlags]:ub		FieldModeAboveMbFlag+FieldModeCurrentMbFlag:uw

	// Get Vert Edge Pattern (frame vs. field MBs)
	and	(1)	VertEdgePattern:uw		r[ECM_AddrReg, BitFlags]:ub		FieldModeLeftMbFlag+FieldModeCurrentMbFlag:uw

	(f0.1.all16h)	jmpi 	SKIP_ILDB						// Skip ILDB
	(f0.0)			jmpi 	SKIP_ILDB						// Skip ILDB

	// Set DualFieldMode for all data read, write and deblocking
//	and	(1)	CTemp1_W:uw		r[ECM_AddrReg, BitFlags]:ub		FieldModeAboveMbFlag+FieldModeCurrentMbFlag:uw
	cmp.z.f0.0	(1)	null:w	CTemp1_W:uw		ABOVE_FIELD_CUR_FRAME:w
	and (1)		DualFieldMode:w		f0.0:w		0x0001:w

	// Load current MB 				// DDD1
	#include "load_Cur_Y_16x16T_Mbaff.asm"				// Load cur Y, 16x16, transpose
	#include "load_Left_Y_4x16T_Mbaff.asm"				// Load left MB (4x16) Y data from memory if exists

	#include "Transpose_Cur_Y_16x16.asm"
	#include "Transpose_Left_Y_4x16.asm"

	//---------- Perform vertical ILDB filting on Y----------
	#include "AVC_ILDB_Filter_Mbaff_Y_v.asm"	
	//-------------------------------------------------------

	#include "save_Left_Y_16x4T_Mbaff.asm"				// Write left MB (4x16) Y data to memory if exists
	#include "load_Top_Y_16x4_Mbaff.asm"				// Load top MB (16x4) Y data from memory if exists
	#include "Transpose_Cur_Y_16x16.asm"				// Transpose a MB for horizontal edge de-blocking 

	//---------- Perform horizontal ILDB filting on Y ----------
	#include "AVC_ILDB_Filter_Mbaff_Y_h.asm"	
	//----------------------------------------------------------

	#include "save_Cur_Y_16x16_Mbaff.asm"					// Write cur MB (16x16)
	#include "save_Top_Y_16x4_Mbaff.asm"					// Write top MB (16x4) if not the top row

SKIP_ILDB:
	//----------------------------------------------------------
	and.z.f0.0 (1) 	null:w		BitFields:w		BotFieldFlag:w

	//=========== Process Bottom MB ============
    or (1)  	BitFields:w  	BitFields:w		BotFieldFlag:w	// Set BotFieldFlag to 1
	(f0.0) jmpi		RE_ENTRY								// Loop back for bottom deblocking

	// Fall through to finish

	//=========== Check write commit of the last write ============
    mov (8)	WritebackResponse(0)<1>		WritebackResponse(0)	

POST_ILDB:
	
	//---------------------------------------------------------------------------
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
	#include "AVC_ILDB_Luma_Core_Mbaff.asm"

#if !defined(COMBINED_KERNEL)		// For standalone kernel only
.end_code

.end_kernel
#endif
