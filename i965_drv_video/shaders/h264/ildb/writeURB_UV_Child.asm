/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: WriteURB_Child.asm
//
// General purpose module to write data to URB using the URB handle/offset in r0
//
//----------------------------------------------------------------
//	Assume:
//	- a0.0 and a0.1 is meg desc, has been assign with URB offset and msg size
//	- MRFs are alrady assigned with data.
//----------------------------------------------------------------
//
//  16x16 byte pixel block can be saved using just 1 "send" instruction.

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0x3535:w
#endif

// URB write header:
//mov (8) MSGSRC.0:ud 	r0.0<8;8,1>:ud			// Copy parent R0 header

//shr (1)	Temp2_W:uw	URBOffsetC:uw	1:w	// divide by 2, because URB entry is counted by 512bits.  Offset is counted by 256bits.
//add (1) MSGSRC.0:uw		r0.0:uw		Temp2_W:uw	

shr (1)	MSGSRC.0:uw		URBOffsetC:uw	1:w	// divide by 2, because URB entry is counted by 512bits.  Offset is counted by 256bits.

//mov (1) MSGSRC.1:ud 	0:ud					// Reset Handle 1

	// URB write 1 MRFs, 
	// Current MB offset is in URBOffset, use it as write origin
	// Add 2 to offset to store data be be passed to the right MB

send  null:uw 	m0	  MSGSRC<8;8,1>:uw		URBWRITE	MSG_LEN(1)+URBWMSGDSC+0x20 // URB write
