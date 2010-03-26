/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//=============== Spawn a chroma root thread ===============

	//----- Create chroma root thread R0 header -----
#if defined(_DEBUG) 
	mov		(1)		EntrySignature:w			0xAABA:w
#endif



	// Restore CT_R0Hdr.4:ud to r0.4:ud 
//	mov (1) CT_R0Hdr.4:ud		r0.4:ud

	// R0.2: Interface Discriptor Ptr.  Add child offset for child kernel
	add (1) CT_R0Hdr.2:ud 		r0.2:ud 		CHROMA_ROOT_OFFSET:w

	// Assign a new Thread Count for this child
	mov (1) CT_R0Hdr.6:ud 		1:w		// ThreadID=1 for chroma root

	//----- Copy luma root r1 for launching chroma root thread -----
	mov (16) m2.0:w		RootParam<16;16,1>:w

	#include "writeURB.asm"

	//--------------------------------------------------
	// Set URB handle for child thread launching:
	// URB handle Length	 	(bit 15:10) - 0000 0000 0000 0000  yyyy yy00 0000 0000
	// URB handle offset  		(bit 9:0) 	- 0000 0000 0000 0000  0000 00xx xxxx xxxx

	or  (1) CT_R0Hdr.4:ud		URB_EntriesPerMB_2:w	URBOffset:uw
	
	// 2 URB entries:
	// Entry 0 - CT_R0Hdr
	// Entry 1 - input parameter to child kernel

	//----- Spawn a child now -----
	send (8) null:ud 	CT_R0Hdr	null:ud    TS	TSMSGDSC

	// Restore CT_R0Hdr.4:ud to r0.4:ud for next use 
	mov (1) CT_R0Hdr.4:ud		r0.4:ud
