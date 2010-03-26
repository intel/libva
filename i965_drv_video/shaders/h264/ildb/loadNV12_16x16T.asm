/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: loadNV12_16x16T.asm
//
// Load and transpose NV12 16x16 block 
//
//----------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Source region in :ud
//	SRC_YD:			SRC_YD Base=rxx ElementSize=4 SrcRegion=REGION(8,1) Type=ud			// 8 GRFs
//	SRC_UD:			SRC_UD Base=rxx ElementSize=4 SrcRegion=REGION(8,1) Type=ud   (U+V for NV12) 	// 4 GRFs
//
//	Source region is :ub.  The same region as :ud region
//	SRC_YB:			SRC_YB Base=rxx ElementSize=1 SrcRegion=REGION(16,1) Type=ub		// 8 GRFs
//	SRC_UB:			SRC_UB Base=rxx ElementSize=1 SrcRegion=REGION(16,1) Type=ub		// 2 GRFs
//	SRC_VB:			SRC_VB Base=rxx ElementSize=1 SrcRegion=REGION(16,1) Type=ub		// 2 GRFs
//
//	Binding table index: 
//	BI_SRC_Y:		Binding table index of Y surface
//	BI_SRC_UV:		Binding table index of UV surface (NV12)
//
//	Temp buffer:
//	BUF_B:			BUF_B Base=rxx ElementSize=1 SrcRegion=REGION(16,1) Type=ub
//
//----------------------------------------------------------------

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xDDD1:w
#endif

	// Read Y
    mov (2)	MSGSRC.0<1>:ud	ORIX_CUR<2;2,1>:w		// Block origin
    mov (1)	MSGSRC.2<1>:ud	0x000F000F:ud		// Block width and height (16x16)
    send (8) SRC_YD(0)<1>	MSGHDRY	MSGSRC<8;8,1>:ud	DAPREAD	RESP_LEN(8)+DWBRMSGDSC_RC+BI_SRC_Y	// Read 8 GRFs

	// Read U+V
    asr (1)	MSGSRC.1:ud		MSGSRC.1:ud			1:w						// NV12 U+V block origin y = half of Y comp
    mov (1)	MSGSRC.2<1>:ud	0x0007000F:ud		// NV12 U+V block width and height (16x8)
    send (8) SRC_UD(0)<1>	MSGHDRU	MSGSRC<8;8,1>:ud	DAPREAD	RESP_LEN(4)+DWBRMSGDSC_RC+BI_SRC_UV	// Read 4 GRFs

	#include "TransposeNV12_16x16.asm"

//	#include "Transpose_Y_16x16.asm"	
//	#include "Transpose_NV12_UV_16x8.asm"	
		
// End of loadNV12_16x16T
