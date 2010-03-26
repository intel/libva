/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//---------- Check dependency and spawn all MBs ----------

// Launch the 1st round of child threads for Vertical ILDB
#if defined(_DEBUG) 
	mov		(1)		EntrySignature:w			0x3333:w
#endif

//=====================================================================
// Jump Table 1
	// 0 0 ---> Goto ALL_SPAWNED
	// 0 1 ---> Goto ALL_SPAWNED
	// 1 0 ---> Goto SLEEP_ENTRY
	// 1 1 ---> Goto POST_SLEEP
	mov (2)		JumpTable.0<1>:d	0:d				{ NoDDClr }
#if defined(CHROMA_ROOT) 		
	mov (1)		JumpTable.2:d	SLEEP_ENTRY_UV_ILDB_FRAME_IP-ALL_SPAWNED_UV_ILDB_FRAME_IP:d		{ NoDDClr, NoDDChk }
	mov (1)		JumpTable.3:d	POST_SLEEP_UV_ILDB_FRAME_IP-ALL_SPAWNED_UV_ILDB_FRAME_IP:d		{ NoDDChk }
#else
	mov (1)		JumpTable.2:d	SLEEP_ENTRY_Y_ILDB_FRAME_IP-ALL_SPAWNED_Y_ILDB_FRAME_IP:d		{ NoDDClr, NoDDChk }
	mov (1)		JumpTable.3:d	POST_SLEEP_Y_ILDB_FRAME_IP-ALL_SPAWNED_Y_ILDB_FRAME_IP:d		{ NoDDChk }
#endif
//=====================================================================

	mov (2)		f0.0<1>:w		0:w

	// Get m0 most of fields ready for URB write
	mov	(8)			MRF0<1>:ud		MSGSRC.0<8;8,1>:ud

	// Add child kernel offset
	add (1) 	CT_R0Hdr.2:ud 			r0.2:ud 		CHILD_OFFSET:w

	// Init
	mov (1)		Col_Boundary:w			2:w
	mov (1)		Row_Boundary:w			LastRow:w
	mov (1)		TopRowForScan:w			0:w
	mov (2)		OutstandingThreads<1>:w	0:w

	// Init Scoreboard  (idle = 0x00FF, busy = 0x0000)
	// Low word is saved col.  High word is busy/idle status
	mov	(16)		GatewayAperture(0)<1>	0x00FF00FF:ud		// Init r6-r7
	mov	(16)		GatewayAperture(2)<1>	0x00FF00FF:ud		// Init r8-r9
	mov	(16)		GatewayAperture(4)<1>	0x00FF00FF:ud		// Init r10-r11
	mov	(16)		GatewayAperture(6)<1>	0x00FF00FF:ud		// Init r12-r13
	mov	(16)		GatewayAperture(8)<1>	0x00FF00FF:ud		// Init r14-r15

	mul	(1)	 		StatusAddr:w		CurRow:w		4:w		// dword to bytes offset conversion

	//=====================================================================

//SPAWN_LOOP:
	//===== OutstandingThreads < ThreadLimit ? ============================
	cmp.l.f0.1 (1)	null:w		OutstandingThreads:w	ThreadLimit:w		// Check the thread limit
#if defined(CHROMA_ROOT) 
    (f0.1) jmpi		ILDB_LABEL(POST_SLEEP_UV)
#else	// LUMA_ROOT
    (f0.1) jmpi		ILDB_LABEL(POST_SLEEP_Y)
#endif

#if defined(CHROMA_ROOT) 
ILDB_LABEL(SLEEP_ENTRY_UV):
#else	// LUMA_ROOT
ILDB_LABEL(SLEEP_ENTRY_Y):
#endif
    //===== Goto Sleep ====================================================
    // Either reached max thread limit or no child thread can be spawned due to dependency.
	add	(1)			OutstandingThreads:w	OutstandingThreads:w	-1:w // Do this before wait is faster
	wait 			n0.0:d												

#if defined(CHROMA_ROOT) 
ILDB_LABEL(POST_SLEEP_UV):
#else	// LUMA_ROOT
ILDB_LABEL(POST_SLEEP_Y):
#endif
	//===== Luma Status[CurRow] == busy ? =====
	cmp.z.f0.0 (1)	null:uw		r[StatusAddr, GatewayApertureB+ScoreBd_Idx]:uw		0:uw			// Check west neighbor
	cmp.g.f0.1 (1)	null:w		CurCol:w		LastCol:w		// Check if the curCol > LastCol

#if defined(CHROMA_ROOT) 
	mov	(16)		acc0.0<1>:w		URBOffsetUVBase<0;1,0>:w			// Add offset to UV base (MBsCntY * URB_EBTRIES_PER_MB)
	mac (1)			URBOffset:w		CurRow:w			4:w				// 4 entries per row
#else
	mul	(1)			URBOffset:w		CurRow:w			4:w				// 4 entries per row
#endif

#if defined(CHROMA_ROOT) 
	(f0.0) jmpi		ILDB_LABEL(SLEEP_ENTRY_UV)								// Current row has a child thread running, can not spawn a new child thread, go back to sleep
	(f0.1) jmpi		ILDB_LABEL(NEXT_MB_UV)									// skip MB if the curCol > LastCol 
#else	// LUMA_ROOT
	(f0.0) jmpi		ILDB_LABEL(SLEEP_ENTRY_Y)								// Current row has a child thread running, can not spawn a new child thread, go back to sleep
	(f0.1) jmpi		ILDB_LABEL(NEXT_MB_Y)									// skip MB if the curCol > LastCol 
#endif
		
	//========== Spwan a child thread ========================================
	// Save cur col and set Status[CurRow] to busy
	mov (2)			r[StatusAddr, GatewayApertureB]<1>:uw		CurColB<2;2,1>:ub		// Store the new col
			
	// Increase OutstandingThreads and ProcessedMBs by 1
	add	(2)			OutstandingThreads<1>:w		OutstandingThreads<2;2,1>:w		1:w  

	#include "AVC_ILDB_SpawnChild.asm"

	//===== Find next MB ===================================================
#if defined(CHROMA_ROOT) 
ILDB_LABEL(NEXT_MB_UV):
#else	// LUMA_ROOT
ILDB_LABEL(NEXT_MB_Y):
#endif
	// Check pic boundary, results are in f0.0 bit0 and bit1
	cmp.ge.f0.0	(2)	null<1>:w   CurCol<2;2,1>:w 	Col_Boundary<2;2,1>:w

	// Update TopRowForScan if the curCol = LastCol
	(f0.1) add (1)	TopRowForScan:w		CurRow:w		1:w	

//	cmp.l.f0.1 (1)	null<1>:w		ProcessedMBs:w		TotalBlocks:w		// Processed all blocks ?
	// 2 sets compare
	// ProcessedMBs:w < TotalBlocks:w		OutstandingThreads:w < ThreadLimit:wProcessedMBs:w
	// 0 0 ---> Goto ALL_SPAWNED
	// 0 1 ---> Goto ALL_SPAWNED
	// 1 0 ---> Goto SLEEP_ENTRY
	// 1 1 ---> Goto POST_SLEEP
	cmp.l.f0.1 (2)	null<1>:w		OutstandingThreads<2;2,1>:w	ThreadLimit<2;2,1>:w

	// Just do it in stalled cycles
	mov (1)		acc0.0:w		4:w
	mac	(1)	 	StatusAddr:w		CurRow:w		4:w						// dword to bytes offset conversion	
	add (2)		CurCol<1>:w		CurCol<2;2,1>:w		StepToNextMB<2;2,1>:b	// CurCol -= 2 and CurRow += 1
		
	// Set f0.0 if turning around is needed, assuming bit 15 - 2 are zeros for correct comparison.
	cmp.nz.f0.0 (1)	null<1>:w	f0.0:w		0x01:w
		
	mul (1) 	JumpAddr:w		f0.1:w		4:w		// byte offet in dword count
		
	// The next MB is at the row TopRowForScan
	(f0.0) mul (1)	 	StatusAddr:w	TopRowForScan:w		4:w				// dword to bytes offset conversion
	(f0.0) mov (1)		CurRow:w		TopRowForScan:w								{ NoDDClr }	// Restart from the top row that has MBs not deblocked yet.
	(f0.0) add (1)		CurCol:w		r[StatusAddr, GatewayApertureB]:uw		1:w		{ NoDDChk }
	
	//===== Processed all blocks ? =========================================
	// (f0.1) jmpi		SPAWN_LOOP

	jmpi	r[JumpAddr, JUMPTABLE_BASE]:d
//JUMP_BASE:

	//======================================================================

	// All MB are spawned at this point, check for outstanding thread count
#if defined(CHROMA_ROOT) 
ILDB_LABEL(ALL_SPAWNED_UV):
#else	// LUMA_ROOT
ILDB_LABEL(ALL_SPAWNED_Y):
#endif
	cmp.e.f0.1 (1) 	null:w		OutstandingThreads:w		0:w			// Check before goto sleep
#if defined(CHROMA_ROOT) 
	(f0.1) jmpi		ILDB_LABEL(ALL_DONE_UV)
#else	// LUMA_ROOT
	(f0.1) jmpi		ILDB_LABEL(ALL_DONE_Y)
#endif
	
	wait 			n0.0:d												// Wake up by a finished child thread
	add	(1)			OutstandingThreads:w	OutstandingThreads:w	-1:w

#if defined(CHROMA_ROOT) 
	// One thread is free and give it to luma thread limit --- Increase luma thread limit by one.
	#include "AVC_ILDB_LumaThrdLimit.asm"
#endif

#if defined(CHROMA_ROOT) 
    jmpi			ILDB_LABEL(ALL_SPAWNED_UV)							// Waked up and goto dependency check
#else	// LUMA_ROOT
    jmpi			ILDB_LABEL(ALL_SPAWNED_Y)							// Waked up and goto dependency check
#endif

	// All child threads are finsihed at this point 
#if defined(CHROMA_ROOT) 
ILDB_LABEL(ALL_DONE_UV):
#else	// LUMA_ROOT
ILDB_LABEL(ALL_DONE_Y):
#endif
