/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: load_Y_4x16T.asm
//
// Load luma left MB 4x16 and transpose 4x16 to 16x4.
//
//----------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Source region in :ud
//	PREV_MB_YD:		PREV_MB_YD Base=rxx ElementSize=4 SrcRegion=REGION(8,1) Type=ud			// 2 GRFs
//
//	Binding table index: 
//	BI_SRC_Y:		Binding table index of Y surface
//
//----------------------------------------------------------------

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD0:w
#endif


#if defined(_PROGRESSIVE) 
	// Read Y
    mov (2)	MSGSRC.0<1>:ud	ORIX_LEFT<2;2,1>:w		{ NoDDClr }		// Block origin
    mov (1)	MSGSRC.2<1>:ud	0x000F0003:ud			{ NoDDChk }		// Block width and height (4x16)
    
//    mov (1)	MSGDSC	DWBRMSGDSC_RC+0x00020000+BI_DEST_Y:ud			// Read 2 GRFs from DEST_Y
    send (8) LEFT_TEMP_D(0)<1>	MSGHDRL		MSGSRC<8;8,1>:ud	DAPREAD	RESP_LEN(2)+DWBRMSGDSC_RC+BI_DEST_Y	
#endif


#if defined(_FIELD) || defined(_MBAFF)

    // FieldModeCurrentMbFlag determines how to access left MB
	and.z.f0.0 (1) 	null:w		r[ECM_AddrReg, BitFlags]:ub		FieldModeCurrentMbFlag:w		

    and.nz.f0.1 (1)	NULLREGW 		BitFields:w  	BotFieldFlag:w	// Get bottom field flag

	// Read Y
    mov (2)	MSGSRC.0<1>:ud	ORIX_LEFT<2;2,1>:w		{ NoDDClr }		// Block origin
    mov (1)	MSGSRC.2<1>:ud	0x000F0003:ud			{ NoDDChk }		// Block width and height (4x16)
    
    // Set message descriptor, etc.
    
	(f0.0)	if	(1)		ILDB_LABEL(ELSE_Y_4x16T)

    // Frame picture
    mov (1)	MSGDSC	RESP_LEN(2)+DWBRMSGDSC_RC+BI_DEST_Y:ud			// Read 2 GRFs from DEST_Y
    
	(f0.1) add (1)	MSGSRC.1:d		MSGSRC.1:d		16:w		// Add vertical offset 16 for bot MB in MBAFF mode
    
ILDB_LABEL(ELSE_Y_4x16T): 
	else 	(1)		ILDB_LABEL(ENDIF_Y_4x16T)

	// Field picture
    (f0.1) mov (1)	MSGDSC	RESP_LEN(2)+DWBRMSGDSC_RC_BF+BI_DEST_Y:ud  // Read 2 GRFs from DEST_Y bottom field
    (-f0.1) mov (1)	MSGDSC	RESP_LEN(2)+DWBRMSGDSC_RC_TF+BI_DEST_Y:ud  // Read 2 GRFs from DEST_Y top field

	endif
ILDB_LABEL(ENDIF_Y_4x16T):

//    send (8) BUF_D(0)<1>	MSGHDRY	MSGSRC<8;8,1>:ud	MSGDSC
    send (8) LEFT_TEMP_D(0)<1>	MSGHDRL		MSGSRC<8;8,1>:ud	DAPREAD	MSGDSC
#endif

//	Transpose 4x16 to 16x4

//	Input received from dport:
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|73 72 71 70 63 62 61 60 53 52 51 50 43 42 41 40 33 32 31 30 23 22 21 20 13 12 11 10 03 02 01 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|f3 f2 f1 f0 e3 e2 e1 e0 d3 d2 d1 d0 c3 c2 c1 c0 b3 b2 b1 b0 a3 a2 a1 a0 93 92 91 90 83 82 81 80|
//	+-----------------------+-----------------------+-----------------------+-----------------------+

//	Output of transpose:		<1>	<= <32;8,4>
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|f1 e1 d1 c1 b1 a1 91 81 71 61 51 41 31 21 11 01 f0 e0 d0 c0 b0 a0 90 80 70 60 50 40 30 20 10 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|f3 e3 d3 c3 b3 a3 93 83 73 63 53 43 33 23 13 03 f2 e2 d2 c2 b2 a2 92 82 72 62 52 42 32 22 12 02|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
/*
	// Transpose the data, also occupy 2 GRFs
	mov (16)	PREV_MB_YB(0)<1>			BUF_B(0, 0)<32;8,4>		{ NoDDClr }
	mov (16)	PREV_MB_YB(0, 16)<1>		BUF_B(0, 1)<32;8,4>		{ NoDDChk }
	mov (16)	PREV_MB_YB(1)<1>			BUF_B(0, 2)<32;8,4>		{ NoDDClr }
	mov (16)	PREV_MB_YB(1, 16)<1>		BUF_B(0, 3)<32;8,4>		{ NoDDChk }
*/
// End of load_Y_4x16T

