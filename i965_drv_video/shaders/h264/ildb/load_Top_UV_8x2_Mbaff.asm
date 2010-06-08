/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module Name: Load_Top_UV_8X2.Asm
//
// Load UV 8X2 Block 
//
//----------------------------------------------------------------
//  Symbols ceed To be defined before including this module
//
//	Source Region Is :UB
//	BUF_D:			BUF_D Base=Rxx Elementsize=4 Srcregion=Region(8,1) Type=UD

//	Binding Table Index: 
//	BI_SRC_UV:		Binding Table Index Of UV Surface (NV12)
//
//----------------------------------------------------------------

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD2:w
#endif

    // FieldModeCurrentMbFlag determines how to access above MB
	and.z.f0.0 (1) 	null:w		r[ECM_AddrReg, BitFlags]:ub		FieldModeCurrentMbFlag:w		

    and.nz.f0.1 (1) NULLREGW 	BitFields:w   	BotFieldFlag:w

	// Read U+V
    mov (1)	MSGSRC.0:ud		ORIX_TOP:w						{ NoDDClr }			// Block origin
    asr (1)	MSGSRC.1:d		ORIY_TOP:w			1:w			{ NoDDClr, NoDDChk } 	// NV12 U+V block origin y = half of Y comp
    mov (1)	MSGSRC.2:ud		0x0001000F:ud					{ NoDDChk }			// NV12 U+V block width and height (16x2)

	// Load NV12 U+V 
	
    // Set message descriptor
    
	(f0.0)	if	(1)		ELSE_UV_8X2

    // Frame picture
    mov (1)	MSGDSC	RESP_LEN(1)+DWBRMSGDSC_RC+BI_DEST_UV:ud			// Read 1 GRF from SRC_UV

	// Add vertical offset 8 for bot MB in MBAFF mode
	(f0.1) add (1)	MSGSRC.1:d	MSGSRC.1:d		8:w		
    
	// Dual field mode setup
	and.z.f0.1 (1) NULLREGW		DualFieldMode:w		1:w
	(f0.1) jmpi NOT_DUAL_FIELD_UV

    add (1)	MSGSRC.1:d		MSGSRC.1:d		-2:w			{ NoDDClr }			// Load 4 lines in stead of 2
	mov (1)	MSGSRC.2:ud		0x0003000F:ud					{ NoDDChk }			// New block width and height (16x8)

	add (1) MSGDSC			MSGDSC			RESP_LEN(1):ud	// 1 more GRF to receive

NOT_DUAL_FIELD_UV:    
    
ELSE_UV_8X2: 
	else 	(1)		ENDIF_UV_8X2

	// Field picture
	asr (1)	MSGSRC.1:d		ORIY_CUR:w		2:w			// asr 1: NV12 U+V block origin y = half of Y comp
														// asr 1: Reduce y by half in field access mode
	
    (f0.1) mov (1)	MSGDSC	RESP_LEN(1)+DWBRMSGDSC_RC_BF+BI_DEST_UV:ud  // Read 1 GRF from SRC_Y bottom field
    (-f0.1) mov (1)	MSGDSC	RESP_LEN(1)+DWBRMSGDSC_RC_TF+BI_DEST_UV:ud  // Read 1 GRF from SRC_Y top field

	add (1) MSGSRC.1:d		MSGSRC.1:d		-2:w				// for last 2 rows of above MB

	endif
ENDIF_UV_8X2:

	// Read 1 GRF from DEST surface as the above MB has been deblocked.
	send (8) PREV_MB_UD(0)<1>	MSGHDRU	MSGSRC<8;8,1>:ud	DAPREAD	MSGDSC	

// End of load_Top_UV_8x2.asm
