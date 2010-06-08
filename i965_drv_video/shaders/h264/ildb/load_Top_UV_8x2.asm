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

#if defined(_PROGRESSIVE) 
	// Read U+V
    mov (1)	MSGSRC.0:ud		ORIX_TOP:w						{ NoDDClr }			// Block origin
    asr (1)	MSGSRC.1:ud		ORIY_TOP:w			1:w			{ NoDDClr, NoDDChk }	// NV12 U+V block origin y = half of Y comp
    mov (1)	MSGSRC.2:ud		0x0001000F:ud					{ NoDDChk }			// NV12 U+V block width and height (16x2)

	// Read 1 GRF from DEST surface as the above MB has been deblocked.
	//send (8) TOP_MB_UD(0)<1>	MSGHDRU		MSGSRC<8;8,1>:ud	RESP_LEN(1)+DWBRMSGDSC_RC+BI_DEST_UV	
	mov (1)	MSGDSC	RESP_LEN(1)+DWBRMSGDSC_RC+BI_DEST_UV:ud	
#endif

#if defined(_FIELD)

//    cmp.z.f0.0 (1)  NULLREGW PicTypeC:w  	0:w							// Get pic type flag
    and.nz.f0.1 (1)  NULLREGW 	BitFields:w  	BotFieldFlag:w			// Get bottom field flag
	// They are used later in this file

	// Read U+V
    mov (1)	MSGSRC.0:ud		ORIX_TOP:w						{ NoDDClr }			// Block origin
    asr (1)	MSGSRC.1:ud		ORIY_TOP:w			1:w			{ NoDDClr, NoDDChk }	// NV12 U+V block origin y = half of Y comp
    mov (1)	MSGSRC.2:ud		0x0001000F:ud					{ NoDDChk }			// NV12 U+V block width and height (16x2)

	// Load NV12 U+V 
	
    // Set message descriptor
    // Frame picture
//    (f0.0) mov (1)	MSGDSC	DWBRMSGDSC_RC+0x00010000+BI_DEST_UV:ud			// Read 1 GRF from SRC_UV
//	(f0.0) jmpi		Load_Top_UV_8x2

	// Field picture
    (f0.1) mov (1)	MSGDSC	RESP_LEN(1)+DWBRMSGDSC_RC_BF+BI_DEST_UV:ud  // Read 1 GRF from SRC_Y bottom field
    (-f0.1) mov (1)	MSGDSC	RESP_LEN(1)+DWBRMSGDSC_RC_TF+BI_DEST_UV:ud  // Read 1 GRF from SRC_Y top field

//Load_Top_UV_8x2:

	// Read 1 GRF from DEST surface as the above MB has been deblocked.
//	send (8) PREV_MB_UD(0)<1>	MSGHDRU		MSGSRC<8;8,1>:ud	MSGDSC	

#endif

	send (8) TOP_MB_UD(0)<1>	MSGHDRU		MSGSRC<8;8,1>:ud	DAPREAD	MSGDSC	
		
// End of load_Top_UV_8x2.asm
