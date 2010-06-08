/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
/////////////////////////////////////////////////////////////////////////////////////
// Kernel name: AVC_ILDB_Root_Mbaff.asm
//
//  Root kernel serves as a scheduler for child threads.
//
//
//	***** Note *****
//	Initial design bundle MB pair for each thread, and share AVC_ILDB_MB_Dep_Check.asm
//	with non mbaff kernels.
//
//	Optimization will be done later, putting top and bottom MBs on separate threads.
//
//
/////////////////////////////////////////////////////////////////////////////////////
//
//  $Revision: 1 $
//  $Date: 10/19/06 5:06p $
//

// ----------------------------------------------------
//  AVC_ILDB_ROOT_MBAFF_UV
// ----------------------------------------------------
#define AVC_ILDB

.kernel AVC_ILDB_ROOT_MBAFF_UV
#if defined(COMBINED_KERNEL)
ILDB_LABEL(AVC_ILDB_ROOT_UV):
#endif

#include "SetupVPKernel.asm"
#include "AVC_ILDB.inc"


#if defined(_DEBUG) 

/////////////////////////////////////////////////////////////////////////////////////
// Init URB space for running on RTL.  It satisfies reading an unwritten URB entries.  
// Will remove it for production release.


//mov (8) m1:ud 		0x11111111:ud
//mov (8) m2:ud 		0x22222222:ud 
//mov (8) m3:ud 		0x33333333:ud
//mov (8) m4:ud 		0x44444444:ud 

//mov (1)	Temp1_W:w	0:w

//ILDB_INIT_URB:
//mul (1)	URBOffset:w				Temp1_W:w		4:w
//shl (1) URBWriteMsgDescLow:uw 	URBOffset:w		4:w		// Msg descriptor: URB write dest offset (9:4)
//mov (1) URBWriteMsgDescHigh:uw 	0x0650:uw				// Msg descriptor: URB write 5 MRFs (m0 - m4)
//#include "writeURB.asm"

//add		(1)		Temp1_W:w	Temp1_W:w	1:w				// Increase block count
//cmp.l.f0.0 (1) 	null		Temp1_W:w	MBsCntY:w		// Check the block count limit
//(f0.0) jmpi		ILDB_INIT_URB							// Loop back


/////////////////////////////////////////////////////////////////////////////////////


mov		(1)		EntrySignature:w			0xEFF0:w

#endif
//----------------------------------------------------------------------------------------------------------------

// Set global variable
mov (32) 	ChildParam:uw			0:uw								// Reset local variables
//mul	(1)	 	TotalBlocks:w			MBsCntX:w		MBsCntY:w			// Total # of MB pairs
//add	(1)	 	GatewayApertureE:w		MBsCntY:w 		GatewayApertureB:w	// Aperture End = aperture Head + BlockCntY


// 2 URB entries for Y:
// Entry 0 - Child thread R0Hdr
// Entry 1 - input parameter to child kernel (child r1)

#define 	URB_ENTRIES_PER_MB 	2

// URB_ENTRIES_PER_MB in differnt form, the final desired format is (URB_ENTRIES_PER_MB-1) << 10
mov (1) URB_EntriesPerMB_2:w		URB_ENTRIES_PER_MB-1:w
shl (1) URB_EntriesPerMB_2:w		URB_EntriesPerMB_2:w	10:w

#define 	CHROMA_ROOT														// Compiling flag for chroma only
//mul (1)		URBOffsetUVBase:w		MBsCntY:w		URB_ENTRIES_PER_MB:w	// Right after Y entries

// URB base for UV kernels
#if defined(DEV_CL)	
	mov (1)		URBOffsetUVBase:w		240:w
#else
	mov (1)		URBOffsetUVBase:w		320:w
#endif


mov	(1)		ChildThreadsID:uw		3:uw

shr (1)		ThreadLimit:w		MaxThreads:w		1:w		// Initial luma thread limit to 50%
mul	(1)	 	TotalBlocks:w		MBsCntX:w		MBsCntY:w	// MBs to be processed count down from TotalBlocks

//***** Init CT_R0Hdr fields that are common to all threads *************************
mov (8)	CT_R0Hdr.0:ud			r0.0<8;8,1>:ud				// Init to root R0 header
mov (1)	CT_R0Hdr.7:ud			r0.6:ud						// Copy Parent Thread Cnt; JJ did the change on 06/20/2006
mov (1) CT_R0Hdr.31:ub			0:w							// Reset the highest byte
mov (1) CT_R0Hdr.3:ud 			0x00000000	 
mov (1) CT_R0Hdr.6:uw 			sr0.0:uw					// sr0.0: state reg contains general thread states, e.g. EUID/TID.

//***** Init ChildParam fields that are common to all threads ***********************
mov (8) 	ChildParam<1>:ud	RootParam<8;8,1>:ud		// Copy all root parameters
mov (4)		CurCol<1>:w			0:w						// Reset CurCol, CurRow
add	(2)		LastCol<1>:w		MBsCntX<2;2,1>:w		-1:w	// Get LastCol and LastRow

mov (1) URBWriteMsgDesc:ud		MSG_LEN(2)+URBWMSGDSC:ud

//===================================================================================

#include "AVC_ILDB_OpenGateway.asm"		// Open root thread gateway for receiving notification 

#include "AVC_ILDB_Dep_Check.asm"  	// Check dependency and spawn all MBs

//#include "AVC_ILDB_UpdateThrdLimit.asm"	// Update thread limit in luma root thread via gateway

#include "AVC_ILDB_CloseGateway.asm"	// Close root thread gateway 

// Chroma root EOT = child send EOT : Request type = 1
	END_CHILD_THREAD

#undef 	CHROMA_ROOT

#if !defined(COMBINED_KERNEL)		// For standalone kernel only
.end_code

.end_kernel
#endif
