/*
 * Save I_PCM Y samples to Y picture buffer
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: save_I_PCM.asm
//
// First save I_PCM Y samples to Y picture buffer
//
    mov (1) MSGSRC.2:ud		0x000F000F:ud {NoDDClr}			// Block width and height (16x16)
	shl (2) MSGSRC.0:ud		ORIX<2;2,1>:ub	4:w	{NoDDChk}	// Convert MB origin in pixel unit

    add (1)	MSGDSC	REG_MBAFF_FIELD<0;1,0>:uw	MSG_LEN(8)+DWBWMSGDSC_WC+DESTY:ud  // Set message descriptor

    $for(0; <8; 2) {
	mov (32)	MSGPAYLOAD(%1)<1>		I_PCM_Y(%1)REGION(16,1) {Compr,NoDDClr}
	mov (32)	MSGPAYLOAD(%1,16)<1>	I_PCM_Y(%1,16)REGION(16,1) {Compr,NoDDChk}
    }

    send (8)	REG_WRITE_COMMIT_Y<1>:ud	MSGHDR	MSGSRC<8;8,1>:ud	DAPWRITE	MSGDSC

// Then save I_PCM U/V samples to U/V picture buffer
//
    mov (1) MSGHDR.2:ud		0x0007000F:ud			{NoDDClr}	// Block width and height (16x8)
    asr (1) MSGHDR.1:ud		MSGSRC.1<0;1,0>:ud	1:w {NoDDChk}	// Y offset should be halved
    add (1)	MSGDSC			MSGDSC			0x0-MSG_LEN(4)+0x1:d	// Set message descriptor for U/V

#if 0
    and.z.f0.0 (1)  NULLREG REG_CHROMA_FORMAT_IDC  CHROMA_FORMAT_IDC:ud
	(f0.0) jmpi (1) MONOCHROME_I_PCM
#endif

#ifndef MONO
// Non-monochrome picture
//
    $for(0,0; <4; 2,1) {
	mov (16)	MSGPAYLOAD(%1)<2>		I_PCM_UV(%2)REGION(16,1)		// U data
	mov (16)	MSGPAYLOAD(%1,1)<2>		I_PCM_UV(%2+2)REGION(16,1)		// V data
	mov (16)	MSGPAYLOAD(%1+1)<2>		I_PCM_UV(%2,16)REGION(16,1)		// U data
	mov (16)	MSGPAYLOAD(%1+1,1)<2>	I_PCM_UV(%2+2,16)REGION(16,1)	// V data
	}
#else	// defined(MONO)
MONOCHROME_I_PCM:
    $for(0; <4; 2) {
	mov (16)	MSGPAYLOADD(%1)<1>		0x80808080:ud {Compr}
	}

#endif	// !defined(MONO)

    send (8)	REG_WRITE_COMMIT_UV<1>:ud	MSGHDR	null:ud	DAPWRITE	MSGDSC

// End of save_I_PCM
