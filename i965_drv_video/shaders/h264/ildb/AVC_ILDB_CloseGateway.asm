/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//----- Close a Message Gateway -----

#if defined(_DEBUG) 
	mov		(1)		EntrySignature:b			0x4444:w
#endif

// Message descriptor
// bit 31	EOD
// 27:24	FFID = 0x0011 for msg gateway
// 23:20	msg length = 1 MRF
// 19:16	Response length	= 0
// 1:0		SubFuncID = 01 for CloseGateway
// Message descriptor: 0 000 0011 0001 0000 + 0 0 000000000000 01 ==> 0000 0011 0001 0000 0000 0000 0000 0001
send (8)	null:ud 	m7	  r0.0<0;1,0>:ud    MSG_GW	CGWMSGDSC 
