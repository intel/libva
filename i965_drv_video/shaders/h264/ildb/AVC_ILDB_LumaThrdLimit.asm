/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//========== Forward message to root thread through gateway ==========

// Chroma root kenrel updates luma thread limit.

#if defined(_DEBUG) 
mov		(1)		EntrySignatureC:w			0x7788:w
#endif

// Init payload to r0
mov (8) 	GatewayPayload<1>:ud 	0:w								{ NoDDClr } 

// Forward a message:
// Offset = x relative to r50 (defiend in open gataway), x = ORIX >> 4 [bit 28:16]
// Need to shift left 16

mov	(1)		Offset_Length:ud		THREAD_LIMIT_OFFSET:ud	 			{ NoDDClr, NoDDChk }

// Length = 1 byte,	[bit 10:8 = 000]
//000 xxxxxxxxxxxxx 00000 000 00000000 ==> 000x xxxx xxxx xxxx 0000 0000 0000 0000

//mov (1) 	DispatchID:ub 			r0.20:ub		// Dispatch ID

//  Copy EUid and Thread ID that we received from the PARENT thread
mov (1) 	EUID_TID:uw 			r0.6:uw								{ NoDDClr, NoDDChk }

mov (1) 	GatewayPayloadKey:uw 	0x1212:uw							{ NoDDChk }	// Key

//mov	(4)		GatewayPayload<1>:ud	0:ud								{ NoDDClr, NoDDChk }	// Init payload low 4 dword

// Write back one byte (value = 0xFF) to root thread GRF to indicate this child thread is finished
// All lower 4 bytes must be assigned to the same byte value.
add	(1)		Temp1_W:w				MaxThreads:uw	-OutstandingThreads:uw
mov	(4)		GatewayPayload<1>:ub	Temp1_B<0;1,0>:ub 

send (8)  	GatewayResponse:ud 		m0	  		GatewayPayload<8;8,1>:ud    MSG_GW	FWDMSGDSC

//========== Forward Msg Done ========================================

