/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//=============== Spawn a child thread for Luma or Chroma ===============

	//----- Create child thread R0 header -----
#if defined(_DEBUG) 
	mov		(1)		EntrySignature:w			0xAAAA:w
#endif

	//***** Set CT_R0Hdr fields that change for every thread 
	
	// Restore CT_R0Hdr.4:ud to r0.4:ud 
	mov (1) CT_R0Hdr.4:ud		r0.4:ud

	// R0.2: Interface Discriptor Ptr.  Add a child offset for child kernel
//	add (1) CT_R0Hdr.2:ud 		r0.2:ud 		CHILD_OFFSET:w

	// Assign a new Thread Count for this child
	mov (1) CT_R0Hdr.6:ud 		ChildThreadsID:uw

	//----- Prepare URB for launching a child thread -----
	mov (16) m2.0:w		ChildParam<16;16,1>:w

	shr (1)	 MRF0.0:uw	URBOffset:uw	1:w

	add	(1)	ChildThreadsID:uw		ChildThreadsID:uw	2:uw	// Luma child=even, chroma child=odd
		
	//--------------------------------------------------
//	#include "writeURB.asm"
	send  null:uw 	MRF0	 null:ud	URBWRITE	URBWriteMsgDesc:ud		// URB write	

	//--------------------------------------------------
	// Set URB handle for child thread launching:
	// URB handle Length	 	(bit 15:10) - 0000 0000 0000 0000  yyyy yy00 0000 0000
	// URB handle offset  		(bit 9:0) 	- 0000 0000 0000 0000  0000 00xx xxxx xxxx

	or  (1) CT_R0Hdr.4:ud		URB_EntriesPerMB_2:w	URBOffset:uw
	
	// 2 URB entries:
	// Entry 0 - CT_R0Hdr
	// Entry 1 - input parameter to child kernel

	//----- Spawn a child now -----
	send (8) null:ud 	CT_R0Hdr	  null:ud    TS	TSMSGDSC
//	send (8) null:ud 	CT_Spawn_Reg	null:ud    0x07100001


	// Restore CT_R0Hdr.4:ud to r0.4:ud for next use
//	mov (1) CT_R0Hdr.4:ud		r0.4:ud
