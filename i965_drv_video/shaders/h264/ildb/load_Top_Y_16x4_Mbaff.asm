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
    // FieldModeCurrentMbFlag determines how to access above MB
	and.z.f0.0 (1) 	null:w		r[ECM_AddrReg, BitFlags]:ub		FieldModeCurrentMbFlag:w		

    and.nz.f0.1 (1) NULLREGW 	BitFields:w   BotFieldFlag:w

	// Read Y
    mov (2)	MSGSRC.0<1>:ud	ORIX_TOP<2;2,1>:w		{ NoDDClr }		// Block origin
    mov (1)	MSGSRC.2<1>:ud	0x0003000F:ud			{ NoDDChk }		// Block width and height (16x4)
   
    // Set message descriptor

	(f0.0)	if	(1)		ELSE_Y_16x4

    // Frame picture
    mov (1)	MSGDSC	RESP_LEN(2)+DWBRMSGDSC_RC+BI_DEST_Y:ud			// Read 2 GRFs from SRC_Y

	// Add vertical offset 16 for bot MB in MBAFF mode
	(f0.1) add (1)	MSGSRC.1:d	MSGSRC.1:d		16:w		
	
	// Dual field mode setup
	and.z.f0.1 (1) NULLREGW		DualFieldMode:w		1:w
	(f0.1) jmpi NOT_DUAL_FIELD

    add (1)	MSGSRC.1:d		MSGSRC.1:d		-4:w	{ NoDDClr }		// Load 8 lines in above MB
	mov (1)	MSGSRC.2:ud		0x0007000F:ud			{ NoDDChk }		// New block width and height (16x8)
	
	add (1) MSGDSC			MSGDSC			RESP_LEN(2):ud	// 2 more GRF to receive

NOT_DUAL_FIELD:

ELSE_Y_16x4: 
	else 	(1)		ENDIF_Y_16x4

	asr (1)	MSGSRC.1:d		ORIY_CUR:w		1:w		// Reduce y by half in field access mode

	// Field picture
    (f0.1) mov (1)	MSGDSC	RESP_LEN(2)+DWBRMSGDSC_RC_BF+BI_DEST_Y:ud  // Read 2 GRFs from SRC_Y bottom field
    (-f0.1) mov (1)	MSGDSC	RESP_LEN(2)+DWBRMSGDSC_RC_TF+BI_DEST_Y:ud  // Read 2 GRFs from SRC_Y top field

	add (1)	MSGSRC.1:d		MSGSRC.1:d		-4:w	// for last 4 rows of above MB

	endif
ENDIF_Y_16x4:
        
    // Read 2 GRFs from DEST surface, as the above MB has been deblocked
    send (8) PREV_MB_YD(0)<1>	MSGHDRY	MSGSRC<8;8,1>:ud	DAPREAD	MSGDSC

// End of load_Y_16x4.asm
