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

	// Read U+V blk
#if defined(_PROGRESSIVE) 
    mov (1)	MSGSRC.0:ud		ORIX_CUR:w				{ NoDDClr } 		// Block origin
    asr (1)	MSGSRC.1:ud		ORIY_CUR:w		1:w		{ NoDDClr, NoDDChk }	// NV12 U+V block origin y = half of Y comp
    mov (1)	MSGSRC.2:ud		0x0007000F:ud			{ NoDDChk }			// NV12 U+V block width and height (16x8 bytes)

    //send (8) SRC_UD(0)<1>	MSGHDRU		MSGSRC<8;8,1>:ud	DWBRMSGDSC_SC+0x00040000+BI_SRC_UV
    mov (1)	MSGDSC	RESP_LEN(4)+DWBRMSGDSC_SC+BI_SRC_UV:ud	
#endif

#if defined(_FIELD)
//    cmp.z.f0.0 (1)  NULLREGW 	PicTypeC:w  	0:w						// Get pic type flag
    and.nz.f0.1 (1) NULLREGW 	BitFields:w  	BotFieldFlag:w			// Get bottom field flag
	// they are used later in this file

    mov (1)	MSGSRC.0:ud		ORIX_CUR:w				{ NoDDClr } 		// Block origin
    asr (1)	MSGSRC.1:ud		ORIY_CUR:w		1:w		{ NoDDClr, NoDDChk }	// NV12 U+V block origin y = half of Y comp
    mov (1)	MSGSRC.2:ud		0x0007000F:ud			{ NoDDChk }			// NV12 U+V block width and height (16x8 bytes)

    // Set message descriptor

    // Frame picture
//    (f0.0) mov (1)	MSGDSC	RESP_LEN(4)+DWBRMSGDSC_SC+BI_SRC_UV:ud			// Read 4 GRFs from SRC_UV
//	(f0.0) jmpi		load_UV_8x8T

	// Field picture
    (f0.1) mov (1)	MSGDSC	RESP_LEN(4)+DWBRMSGDSC_SC_BF+BI_SRC_UV:ud  // Read 4 GRFs from SRC_UV bottom field
    (-f0.1) mov (1)	MSGDSC	RESP_LEN(4)+DWBRMSGDSC_SC_TF+BI_SRC_UV:ud  // Read 4 GRFs from SRC_UV top field

//load_UV_8x8T:

#endif

    send (8) SRC_UD(0)<1>	MSGHDRU		MSGSRC<8;8,1>:ud	DAPREAD	MSGDSC

//	#include "Transpose_Cur_UV_8x8.asm"

// End of load_UV_8x8T
