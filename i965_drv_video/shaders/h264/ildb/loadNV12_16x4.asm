/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module Name: Loadnv12_16X4.Asm
//
// Load Nv12 16X4 Block 
//
//----------------------------------------------------------------
//  Symbols Need To Be Defined Before Including This Module
//
//	Source Region In :Ud
//	Src_Yd:			Src_Yd Base=Rxx Elementsize=4 Srcregion=Region(8,1) Type=Ud			// 3 Grfs (2 For Y, 1 For U+V)
//
//	Source Region Is :Ub.  The Same Region As :Ud Region
//	Src_Yb:			Src_Yb Base=Rxx Elementsize=1 Srcregion=Region(16,1) Type=Ub		// 2 Grfs
//	Src_Ub:			Src_Ub Base=Rxx Elementsize=1 Srcregion=Region(16,1) Type=Ub		// 0.5 Grf
//	Src_Vb:			Src_Vb Base=Rxx Elementsize=1 Srcregion=Region(16,1) Type=Ub		// 0.5 Grf
//
//	Binding Table Index: 
//	Bi_Src_Y:		Binding Table Index Of Y Surface
//	Bi_Src_UV:		Binding Table Index Of UV Surface (Nv12)
//
//	Temp Buffer:
//	Buf_D:			Buf_D Base=Rxx Elementsize=4 Srcregion=Region(8,1) Type=Ud
//	Buf_B:			Buf_B Base=Rxx Elementsize=1 Srcregion=Region(16,1) Type=Ub
//
//----------------------------------------------------------------

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD2:w
#endif

	// Read Y
    mov (2)	MSGSRC.0<1>:ud	ORIX<2;2,1>:w		// Block origin
    mov (1)	MSGSRC.2<1>:ud	0x0003000F:ud		// Block width and height (16x4)
    send (8) PREV_MB_YD(0)<1>	MSGHDRY	MSGSRC<8;8,1>:ud	DAPREAD	RESP_LEN(2)+DWBRMSGDSC_RC+BI_SRC_Y	// Read 2 GRFs

	// Read U+V
    asr (1)	MSGSRC.1:ud		MSGSRC.1:ud			1:w						// NV12 U+V block origin y = half of Y comp
    mov (1)	MSGSRC.2<1>:ud	0x0001000F:ud		// NV12 U+V block width and height (16x2)

	// Load NV12 U+V tp a temp buf  
	send (8) BUF_D(0)<1>	MSGHDRU	MSGSRC<8;8,1>:ud	DAPREAD	RESP_LEN(1)+DWBRMSGDSC_RC+BI_SRC_UV	// Read 1 GRF

	// Convert NV12 U+V to internal planar U and V and place them right after Y.
//	mov (16)	SRC_UB(0,0)<1>		BUF_B(0,0)<32;16,2>
//	mov (16)	SRC_VB(0,0)<1>		BUF_B(0,1)<32;16,2>	
	
// End of loadNV12_16x4.asm
