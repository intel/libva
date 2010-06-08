/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module Name: Load_Y_16X4.asm
//
// Load Y 16X4 Block to PREV_MB_YD
//
//----------------------------------------------------------------
//  Symbols Need To Be Defined Before Including This Module
//
//	Source Region In :Ud
//	Src_YD:			Src_Yd Base=Rxx Elementsize=4 Srcregion=Region(8,1) Type=Ud			// 3 Grfs (2 For Y, 1 For U+V)
//
//	Source Region Is :Ub.  The Same Region As :Ud Region
//	Src_YB:			Src_Yb Base=Rxx Elementsize=1 Srcregion=Region(16,1) Type=Ub		// 2 Grfs
//
//	Binding Table Index: 
//	Bi_Src_Y:		Binding Table Index Of Y Surface
//
//	Temp Buffer:
//	Buf_D:			Buf_D Base=Rxx Elementsize=4 Srcregion=Region(8,1) Type=Ud
//	Buf_B:			Buf_B Base=Rxx Elementsize=1 Srcregion=Region(16,1) Type=Ub
//
//----------------------------------------------------------------

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD2:w
#endif

#if defined(_PROGRESSIVE) 
	// Read Y
    mov (2)	MSGSRC.0<1>:ud	ORIX_TOP<2;2,1>:w		{ NoDDClr }		// Block origin
    mov (1)	MSGSRC.2<1>:ud	0x0003000F:ud			{ NoDDChk }		// Block width and height (16x4)

    mov (1)	MSGDSC	RESP_LEN(2)+DWBRMSGDSC_RC+BI_DEST_Y:ud			// Read 2 GRFs from SRC_Y
#endif

#if defined(_FIELD)

//    cmp.z.f0.0 (1)  NULLREGW 	PicTypeC:w  	0:w						// Get pic type flag
    and.nz.f0.1 (1) NULLREGW 	BitFields:w  	BotFieldFlag:w			// Get bottom field flag
	// they are used later in this file
	
    mov (2)	MSGSRC.0<1>:ud	ORIX_TOP<2;2,1>:w		{ NoDDClr }		// Block origin
    mov (1)	MSGSRC.2<1>:ud	0x0003000F:ud			{ NoDDChk }		// Block width and height (16x4)
   
    // Set message descriptor

    // Frame picture
//	(f0.0) mov (1)	MSGDSC	DWBRMSGDSC_RC+0x00020000+BI_DEST_Y:ud			// Read 2 GRFs from SRC_Y
//	(f0.0) jmpi		load_Y_16x4

	// Field picture
    (f0.1) mov (1)	MSGDSC	RESP_LEN(2)+DWBRMSGDSC_RC_BF+BI_DEST_Y:ud  // Read 2 GRFs from SRC_Y bottom field
    (-f0.1) mov (1)	MSGDSC	RESP_LEN(2)+DWBRMSGDSC_RC_TF+BI_DEST_Y:ud  // Read 2 GRFs from SRC_Y top field

//load_Y_16x4:
    // Read 2 GRFs from DEST surface, as the above MB has been deblocked
//    send (8) PREV_MB_YD(0)<1>	MSGHDRY		MSGSRC<8;8,1>:ud	MSGDSC
    
#endif
    
    send (8) TOP_MB_YD(0)<1>	MSGHDRT		MSGSRC<8;8,1>:ud	DAPREAD	MSGDSC
    	    
// End of load_Y_16x4.asm
