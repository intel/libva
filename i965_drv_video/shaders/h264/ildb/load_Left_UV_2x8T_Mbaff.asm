/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module Name: Load_Left_UV_2X8T.Asm
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
	mov		(1)		EntrySignatureC:w			0xDDD0:w
#endif

    // FieldModeCurrentMbFlag determines how to access left MB
	and.z.f0.0 (1) 	null:w		r[ECM_AddrReg, BitFlags]:ub		FieldModeCurrentMbFlag:w		

    and.nz.f0.1 (1)	NULLREGW 		BitFields:w  	BotFieldFlag:w				// Get bottom field flag

	// Read U+V
    mov (1)	MSGSRC.0:ud		ORIX_LEFT:w							{ NoDDClr }		// Block origin
    asr (1)	MSGSRC.1:ud		ORIY_LEFT:w			1:w				{ NoDDClr, NoDDChk }		// NV12 U+V block origin y = half of Y comp
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

	asr (1)	MSGSRC.1:d		MSGSRC.1:d		1:w					// Reduce y by half in field access mode

	endif
ILDB_LABEL(ENDIF_Y_2x8T):

	// Read 1 GRF from DEST surface as the above MB has been deblocked.
//	send (8) BUF_D(0)<1>	MSGHDRU	MSGSRC<8;8,1>:ud	MSGDSC	
	send (8) LEFT_TEMP_D(0)<1>	MSGHDRU	MSGSRC<8;8,1>:ud	DAPREAD	MSGDSC	


//	Input from dport for transpose:	
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|71 71 70 70 61 61 60 60 51 51 50 50 41 41 40 40 31 31 30 30 21 21 20 20 11 11 10 10 01 01 00 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//
//	Output of transpose:	<1>	<=== <16;8,2>:w
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|71 71 61 61 51 51 41 41 31 31 21 21 11 11 01 01 70 70 60 60 50 50 40 40 30 30 20 20 10 10 00 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
/*
	mov (8)	PREV_MB_UW(0,0)<1>		BUF_W(0,0)<16;8,2>		{ NoDDClr }
	mov (8)	PREV_MB_UW(0,8)<1>		BUF_W(0,1)<16;8,2>		{ NoDDChk }
*/
// End of load_Left_UV_2x8T.asm
