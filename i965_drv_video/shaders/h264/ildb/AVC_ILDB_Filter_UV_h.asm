/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
////////// AVC ILDB filter horizontal UV ///////////////////////////////////////////////////////
//
//	This filter code prepares the src data and control data for ILDB filtering on all horizontal edges of UV.
//
//	It sssumes the data for horizontal de-blocking is already transposed.  
//
//		Chroma:
//
//		+-------+-------+		H0 Edge
//		|		|		|
//		|		|		|
//		|		|		|
//		+-------+-------+		H1 Edge
//		|		|		|
//		|		|		|
//		|		|		|
//		+-------+-------+
//
/////////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xBBBC:w
#endif	

//=============== Chroma deblocking ================

//---------- Deblock U external top edge ----------
	and.z.f0.0  (1) null:w		r[ECM_AddrReg, BitFlags]:ub		FilterTopMbEdgeFlag:w		// Check for FilterTopMbEdgeFlag 
//    (f0.0)	jmpi	BYPASS_EXT_TOP_EDGE_UV	

	// Get horizontal border edge control data.
	
	//***** Need to take every other bit to form U maskA and mask B
	// Get Luma maskA and maskB	
	shr (16)	TempRow0(0)<1>		r[ECM_AddrReg, wEdgeCntlMapA_ExtTopHorz0]<0;1,0>:uw		RRampW(0)
	shr (16)	TempRow1(0)<1>		r[ECM_AddrReg, wEdgeCntlMapB_ExtTopHorz0]<0;1,0>:uw		RRampW(0)
		
    (f0.0)	jmpi	ILDB_LABEL(BYPASS_EXT_TOP_EDGE_UV)			

	// Extract UV MaskA and MaskB from every other bit of Y masks
	and.nz.f0.0 (8) null:w			TempRow0(0)<16;8,2>		1:w
	and.nz.f0.1 (8) null:w			TempRow1(0)<16;8,2>		1:w

//---------- Deblock U external edge ----------
	//	p1 = Prev MB U row 0
	//	p0 = Prev MB U row 1
	// 	q0 = Cur MB U row 0
	//	q1 = Cur MB U row 1
//	mov (1)	P_AddrReg:w		PREV_MB_U_BASE:w									{ NoDDClr }
	mov (1)	P_AddrReg:w		TOP_MB_U_BASE:w										{ NoDDClr }
	mov (1)	Q_AddrReg:w		SRC_MB_U_BASE:w										{ NoDDChk }

	// alpha = bAlphaTop0_Cb, beta = bBetaTop0_Cb
	mov	(2)	alpha<1>:w	r[ECM_AddrReg, bAlphaTop0_Cb]<2;2,1>:ub					{ NoDDClr } 
	// tc0 has bTc0_h03_0_Cb + bTc0_h02_0_Cb + bTc0_h01_0_Cb + bTc0_h00_0_Cb
	mov (4)	tc0<1>:ub	r[ECM_AddrReg, bTc0_h00_0_Cb]<4;4,1>:ub					{ NoDDChk } 
		
	// UV MaskA and MaskB
	mov (2)		MaskA<1>:uw			f0.0<2;2,1>:uw

	CALL(FILTER_UV, 1)	

//---------- Deblock V external top edge ----------
	//	p1 = Prev MB V row 0
	//	p0 = Prev MB V row 1
	// 	q0 = Cur MB V row 0
	//	q1 = Cur MB V row 1
//	mov (1)	P_AddrReg:w		PREV_MB_V_BASE:w		{ NoDDClr }
	mov (1)	P_AddrReg:w		TOP_MB_V_BASE:w		{ NoDDClr }
	mov (1)	Q_AddrReg:w		SRC_MB_V_BASE:w			{ NoDDChk }

	// alpha = bAlphaTop0_Cr, beta = bBetaTop0_Cr
	mov	(2)	alpha<1>:w	r[ECM_AddrReg, bAlphaTop0_Cr]<2;2,1>:ub		{ NoDDClr }
	
	// tc0 has bTc0_h03_0_Cr + bTc0_h02_0_Cr + bTc0_h01_0_Cr + bTc0_h00_0_Cr
	mov (4)	tc0<1>:ub	r[ECM_AddrReg, bTc0_h00_0_Cr]<4;4,1>:ub		{ NoDDChk }

	// UV MaskA and MaskB
	mov (2)		f0.0<1>:uw		MaskA<2;2,1>:uw

	CALL(FILTER_UV, 1)	

ILDB_LABEL(BYPASS_EXT_TOP_EDGE_UV):

	// Set EdgeCntlMap2 = 0, so it always uses bS < 4 algorithm.

	// Bypass deblocking if FilterInternal4x4EdgesFlag = 0  
//	and.z.f0.0 (1) null:w	r[ECM_AddrReg, BitFlags]:ub		FilterInternal4x4EdgesFlag:w		// Check for FilterInternal4x4EdgesFlag 
//    (f0.0)	jmpi	BYPASS_4x4_DEBLOCK_H

//---------- Deblock U internal horz middle edge ----------

	//***** Need to take every other bit to form U maskA
	// Get Luma maskA and maskB	
	shr (16)	TempRow0(0)<1>		r[ECM_AddrReg, wEdgeCntlMap_IntMidHorz]<0;1,0>:uw		RRampW(0)

	//	p1 = Cur MB U row 2
	//	p0 = Cur MB U row 3
	// 	q0 = Cur MB U row 4
	//	q1 = Cur MB U row 5
	mov (1)	P_AddrReg:w		4*UV_ROW_WIDTH+SRC_MB_U_BASE:w					{ NoDDClr }		// Skip 2 U rows and 2 V rows
	mov (1)	Q_AddrReg:w		8*UV_ROW_WIDTH+SRC_MB_U_BASE:w					{ NoDDChk }

	// alpha = bAlphaInternal_Cb, beta = bBetaInternal_Cb
	mov	(2)	alpha<1>:w	r[ECM_AddrReg, bAlphaInternal_Cb]<2;2,1>:ub 		{ NoDDClr }
	// tc0 has bTc0_h23_Cb + bTc0_h22_Cb + bTc0_h21_Cb + bTc0_h20_Cb		
	mov (4)	tc0<1>:ub	r[ECM_AddrReg, bTc0_h20_Cb]<4;4,1>:ub				{ NoDDChk }

	// Extract UV MaskA and MaskB from every other bit of Y masks
	and.nz.f0.0 (8) null:w			TempRow0(0)<16;8,2>		1:w

	// UV MaskA and MaskB
	mov (1) f0.1:uw		0:w
	mov (1)	MaskB:uw	0:w													{ NoDDClr }
	mov (1)	MaskA:uw	f0.0:uw												{ NoDDChk }

	CALL(FILTER_UV, 1)	

//---------- Deblock V internal horz middle edge ----------
	//	p1 = Cur MB V row 2
	//	p0 = Cur MB V row 3
	// 	q0 = Cur MB V row 4
	//	q1 = Cur MB V row 5
	mov (1)	P_AddrReg:w		4*UV_ROW_WIDTH+SRC_MB_V_BASE:w					{ NoDDClr }		// Skip 2 U rows and 2 V rows
	mov (1)	Q_AddrReg:w		8*UV_ROW_WIDTH+SRC_MB_V_BASE:w					{ NoDDChk }

	// alpha = bAlphaInternal_Cr, beta = bBetaInternal_Cr
	mov	(2)	alpha<1>:w	r[ECM_AddrReg, bAlphaInternal_Cr]<2;2,1>:ub 		{ NoDDClr }
	// tc0 has bTc0_h23_Cr + bTc0_h22_Cr + bTc0_h21_Cr + bTc0_h20_Cr
	mov (4)	tc0<1>:ub	r[ECM_AddrReg, bTc0_h20_Cr]<4;4,1>:ub				{ NoDDChk }

	// UV MaskA and MaskB
	mov (2)		f0.0<1>:uw		MaskA<2;2,1>:uw

	CALL(FILTER_UV, 1)	

//BYPASS_4x4_DEBLOCK_H:
