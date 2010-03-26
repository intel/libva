/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//=============== Spawn a child thread for a vertical child ===============

#if defined(_DEBUG) 
	mov	(1)		EntrySignature:w	0x6666:w
#endif
	
	mul	(1)		URBOffset:uw		CurRow:uw		2:w // 5:w			// Each row uses 5 URB entries (R0, child R0, 3 GRFs of data from left MB)

	mov (8)		CT_R0Hdr.0:ud		r0.0<8;8,1>:ud				// Init to root R0 header
	
	// R0.2: Interface Discriptor Ptr.  Add offset 16 for next Interface Discriptor for child kernel
	add (1) 	CT_R0Hdr.2:ud 		r0.2:ud 		IDesc_Child_Offset:w
	
	#include "AVC_ILDB_SpawnChild.asm"
