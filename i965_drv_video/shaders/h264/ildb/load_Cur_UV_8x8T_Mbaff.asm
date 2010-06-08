/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: load_Cur_UV_8x8T.asm
//
// Load and transpose UV 8x8 block (NV12: 8x8U and 8x8V mixed)
//
//----------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Source region in :ud
//	SRC_UD:			SRC_UD Base=rxx ElementSize=4 SrcRegion=REGION(8,1) Type=ud   (U+V for NV12) 	// 4 GRFs
//
//	Binding table index: 
//	BI_SRC_UV:		Binding table index of UV surface (NV12)
//
//----------------------------------------------------------------

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD1:w
#endif
    // FieldModeCurrentMbFlag determines how to access left MB
	and.z.f0.0 (1) 	null:w		r[ECM_AddrReg, BitFlags]:ub		FieldModeCurrentMbFlag:w		

    and.nz.f0.1 (1)	NULLREGW 	BitFields:w  	BotFieldFlag:w					// Get bottom field flag

	// Read U+V
    mov (1)	MSGSRC.0:ud		ORIX_CUR:w						{ NoDDClr } 		// Block origin
    asr (1)	MSGSRC.1:ud		ORIY_CUR:w			1:w			{ NoDDClr, NoDDChk }	// NV12 U+V block origin y = half of Y comp
    mov (1)	MSGSRC.2:ud		0x0007000F:ud					{ NoDDChk }			// NV12 U+V block width and height (16x8 bytes)

    // Set message descriptor

	(f0.0)	if	(1)		ILDB_LABEL(ELSE_UV_8X8T)

    // Frame picture
    mov (1)	MSGDSC	RESP_LEN(4)+DWBRMSGDSC_SC+BI_SRC_UV:ud			// Read 4 GRFs from SRC_UV

	(f0.1) add (1)	MSGSRC.1:d	MSGSRC.1:d		8:w		// Add vertical offset 8 for bot MB in MBAFF mode
    
ILDB_LABEL(ELSE_UV_8X8T): 
	else 	(1)		ILDB_LABEL(ENDIF_UV_8X8T)

	// Field picture
    (f0.1) mov (1)	MSGDSC	RESP_LEN(4)+DWBRMSGDSC_SC_BF+BI_SRC_UV:ud  // Read 4 GRFs from SRC_UV bottom field
    (-f0.1) mov (1)	MSGDSC	RESP_LEN(4)+DWBRMSGDSC_SC_TF+BI_SRC_UV:ud  // Read 4 GRFs from SRC_UV top field

	asr (1)	MSGSRC.1:d		MSGSRC.1:d		1:w					// Reduce y by half in field access mode

	endif
ILDB_LABEL(ENDIF_UV_8X8T):

    send (8) SRC_UD(0)<1>	MSGHDRU	MSGSRC<8;8,1>:ud	DAPREAD	MSGDSC

//	#include "Transpose_Cur_UV_8x8.asm"

// End of load_UV_8x8T
