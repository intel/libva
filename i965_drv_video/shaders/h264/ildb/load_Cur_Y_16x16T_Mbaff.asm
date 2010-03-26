/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: load_Y_16x16T.asm
//
// Load and transpose Y 16x16 block 
//
//----------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Source region in :ud
//	SRC_YD:			SRC_YD Base=rxx ElementSize=4 SrcRegion=REGION(8,1) Type=ud			// 8 GRFs
//
//	Binding table index: 
//	BI_SRC_Y:		Binding table index of Y surface
//
//----------------------------------------------------------------

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD1:w
#endif

    // FieldModeCurrentMbFlag determines how to access left MB
	and.z.f0.0 (1) 	null:w		r[ECM_AddrReg, BitFlags]:ub		FieldModeCurrentMbFlag:w		
	
    and.nz.f0.1 (1)	NULLREGW 	BitFields:w  	BotFieldFlag:w		// Get bottom field flag

	// Read Y
    mov (2)	MSGSRC.0<1>:d	ORIX_CUR<2;2,1>:w		{ NoDDClr }		// Block origin
    mov (1)	MSGSRC.2<1>:ud	0x000F000F:ud			{ NoDDChk }		// Block width and height (16x16)
    
    // Set message descriptor, etc.
    
	(f0.0)	if	(1)		ILDB_LABEL(ELSE_Y_16x16T)

    // Frame picture
    mov (1)	MSGDSC	RESP_LEN(8)+DWBRMSGDSC_SC+BI_SRC_Y:ud			// Read 8 GRFs from SRC_Y
    
	(f0.1) add (1)	MSGSRC.1:d	MSGSRC.1:d		16:w		// Add vertical offset 16 for bot MB in MBAFF mode
    
ILDB_LABEL(ELSE_Y_16x16T): 
	else 	(1)		ILDB_LABEL(ENDIF_Y_16x16T)

	// Field picture
    (f0.1) mov (1)	MSGDSC	RESP_LEN(8)+DWBRMSGDSC_SC_BF+BI_SRC_Y:ud  // Read 8 GRFs from SRC_Y bottom field
    (-f0.1) mov (1)	MSGDSC	RESP_LEN(8)+DWBRMSGDSC_SC_TF+BI_SRC_Y:ud  // Read 8 GRFs from SRC_Y top field

	asr (1)	MSGSRC.1:d		MSGSRC.1:d		1:w					// Reduce y by half in field access mode

	endif
ILDB_LABEL(ENDIF_Y_16x16T):

    send (8) SRC_YD(0)<1>	MSGHDRY	MSGSRC<8;8,1>:ud	DAPREAD	MSGDSC

//	#include "Transpose_Cur_Y_16x16.asm"

// End of load_Y_16x16T
