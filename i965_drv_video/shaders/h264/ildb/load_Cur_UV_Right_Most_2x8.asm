/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module Name: Load_Cur_UV_Right_Most_2X8.Asm

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD0:w
#endif

#if defined(_PROGRESSIVE) 
	// Read U+V, (UV MB size = 16x8)
    add (1)	MSGSRC.0:ud		ORIX_CUR:w			12:w			{ NoDDClr }		// Block origin
    asr (1)	MSGSRC.1:ud		ORIY_CUR:w			1:w				{ NoDDClr, NoDDChk }		// NV12 U+V block origin y = half of Y comp
    mov (1)	MSGSRC.2:ud		0x00070003:ud						{ NoDDChk }		// NV12 U+V block width and height (4x8)
	send (8) LEFT_TEMP_D(0)<1>	MSGHDRU	MSGSRC<8;8,1>:ud	DAPREAD	RESP_LEN(1)+DWBRMSGDSC_RC+BI_DEST_UV	
#endif

#if defined(_FIELD) || defined(_MBAFF)

    // FieldModeCurrentMbFlag determines how to access left MB
	and.z.f0.0 (1) 	null:w		r[ECM_AddrReg, BitFlags]:ub		FieldModeCurrentMbFlag:w		

    and.nz.f0.1 (1)	NULLREGW 		BitFields:w  	BotFieldFlag:w				// Get bottom field flag

	// Read U+V
    add (1)	MSGSRC.0:ud		ORIX_CUR:w			12:w				{ NoDDClr }		// Block origin
    asr (1)	MSGSRC.1:ud		ORIY_CUR:w			1:w				{ NoDDClr, NoDDChk }		// NV12 U+V block origin y = half of Y comp
    mov (1)	MSGSRC.2:ud		0x00070003:ud						{ NoDDChk }		// NV12 U+V block width and height (4x8)

	// Load NV12 U+V 
	
    // Set message descriptor

	(f0.0)	if	(1)		ILDB_LABEL(ELSE_Y_2x8T)

    // Frame picture
    mov (1)	MSGDSC	RESP_LEN(1)+DWBRMSGDSC_RC+BI_DEST_UV:ud			// Read 1 GRF from SRC_UV

	(f0.1) add (1)	MSGSRC.1:d		MSGSRC.1:d		8:w		// Add vertical offset 8 for bot MB in MBAFF mode

ILDB_LABEL(ELSE_Y_2x8T): 
	else 	(1)		ILDB_LABEL(ENDIF_Y_2x8T)

	// Field picture
    (f0.1) mov (1)	MSGDSC	RESP_LEN(1)+DWBRMSGDSC_RC_BF+BI_DEST_UV:ud  // Read 1 GRF from SRC_Y bottom field
    (-f0.1) mov (1)	MSGDSC	RESP_LEN(1)+DWBRMSGDSC_RC_TF+BI_DEST_UV:ud  // Read 1 GRF from SRC_Y top field

	endif
ILDB_LABEL(ENDIF_Y_2x8T):

	// Read 1 GRF from DEST surface as the above MB has been deblocked.
//	send (8) BUF_D(0)<1>	MSGHDRU	MSGSRC<8;8,1>:ud	MSGDSC	
	send (8) LEFT_TEMP_D(0)<1>	MSGHDRU	MSGSRC<8;8,1>:ud	DAPREAD	MSGDSC	

#endif

