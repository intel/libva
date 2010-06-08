/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
////////// AVC ILDB filter vertical Mbaff Y ///////////////////////////////////////////////////////
//
//	This filter code prepares the src data and control data for ILDB filtering on all vertical edges of Y.
//
//	It sssumes the data for vertical de-blocking is already transposed.  
//
//		Luma:
//
//		+-------+-------+-------+-------+
//		|		|		|		|		|
//		|		|		|		|		|
//		|		|		|		|		|
//		+-------+-------+-------+-------+
//		|		|		|		|		|
//		|		|		|		|		|
//		|		|		|		|		|
//		+-------+-------+-------+-------+
//		|		|		|		|		|
//		|		|		|		|		|
//		|		|		|		|		|
//		+-------+-------+-------+-------+
//		|		|		|		|		|
//		|		|		|		|		|
//		|		|		|		|		|
//		+-------+-------+-------+-------+
//
//		V0		V1		V2		V3
//		Edge	Edge	Edge	Edge
//
/////////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xBBBB:w
#endif	
	

//========== Luma deblocking ==========


//---------- Deblock Y external left edge (V0) ----------	

	cmp.z.f0.0	(8)	null:w	VertEdgePattern:uw		LEFT_FIELD_CUR_FRAME:w
	cmp.z.f0.1	(8)	null:w	VertEdgePattern:uw		LEFT_FRAME_CUR_FIELD:w

	// Intial set for both are frame or field
	mov	(16) Mbaff_ALPHA(0,0)<1>	r[ECM_AddrReg, bAlphaLeft0_Y]<0;1,0>:ub
	mov	(16) Mbaff_BETA(0,0)<1>		r[ECM_AddrReg, bBetaLeft0_Y]<0;1,0>:ub
	mov (16) Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_v00_0_Y]<1;4,0>:ub
		
	// For FieldModeCurrentMbFlag=1 && FieldModeLeftMbFlag=0
	(f0.0) mov (8)	Mbaff_ALPHA(0,0)<2>		r[ECM_AddrReg, bAlphaLeft0_Y]<0;1,0>:ub		{ NoDDClr }
	(f0.0) mov (8)	Mbaff_ALPHA(0,1)<2>		r[ECM_AddrReg, bAlphaLeft1_Y]<0;1,0>:ub		{ NoDDChk }
	(f0.0) mov (8)	Mbaff_BETA(0,0)<2>		r[ECM_AddrReg, bBetaLeft0_Y]<0;1,0>:ub		{ NoDDClr }
	(f0.0) mov (8)	Mbaff_BETA(0,1)<2>		r[ECM_AddrReg, bBetaLeft1_Y]<0;1,0>:ub		{ NoDDChk }
	(f0.0) mov (8)	Mbaff_TC0(0,0)<2>		r[ECM_AddrReg, bTc0_v00_0_Y]<1;2,0>:ub		{ NoDDClr }
	(f0.0) mov (8)	Mbaff_TC0(0,1)<2>		r[ECM_AddrReg, bTc0_v00_1_Y]<1;2,0>:ub		{ NoDDChk }

	and.z.f0.0  (1) null:w		r[ECM_AddrReg, BitFlags]:ub		FilterLeftMbEdgeFlag:w		// Check for FilterLeftMbEdgeFlag 

	// For FieldModeCurrentMbFlag=0 && FieldModeLeftMbFlag=1
	(f0.1) mov (8)	Mbaff_ALPHA(0,0)<1>		r[ECM_AddrReg, bAlphaLeft0_Y]<0;1,0>:ub		{ NoDDClr }
	(f0.1) mov (8)	Mbaff_ALPHA(0,8)<1>		r[ECM_AddrReg, bAlphaLeft1_Y]<0;1,0>:ub		{ NoDDChk }
	(f0.1) mov (8)	Mbaff_BETA(0,0)<1>		r[ECM_AddrReg, bBetaLeft0_Y]<0;1,0>:ub		{ NoDDClr }
	(f0.1) mov (8)	Mbaff_BETA(0,8)<1>		r[ECM_AddrReg, bBetaLeft1_Y]<0;1,0>:ub		{ NoDDChk }
	(f0.1) mov (8)	Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_v00_0_Y]<1;2,0>:ub		{ NoDDClr }
	(f0.1) mov (8)	Mbaff_TC0(0,8)<1>		r[ECM_AddrReg, bTc0_v00_1_Y]<1;2,0>:ub		{ NoDDChk }

	// Get (alpha >> 2) + 2
	shr (16) Mbaff_ALPHA2(0,0)<1>	Mbaff_ALPHA(0)		2:w			// alpha >> 2

	//	p3 = Prev MB Y row 0 = r[P_AddrReg, 0]<16;16,1>
	//	p2 = Prev MB Y row 1 = r[P_AddrReg, 16]<16;16,1>
	//	p1 = Prev MB Y row 2 = r[P_AddrReg, 32]<16;16,1>
	//	p0 = Prev MB Y row 3 = r[P_AddrReg, 48]<16;16,1>
	// 	q0 = Cur MB Y row 0  = r[Q_AddrReg, 0]<16;16,1>
	//	q1 = Cur MB Y row 1  = r[Q_AddrReg, 16]<16;16,1>
	//	q2 = Cur MB Y row 2  = r[Q_AddrReg, 32]<16;16,1>
	//	q3 = Cur MB Y row 3  = r[Q_AddrReg, 48]<16;16,1>
	mov (1)	P_AddrReg:w		PREV_MB_Y_BASE:w	{ NoDDClr }
	mov (1)	Q_AddrReg:w		SRC_MB_Y_BASE:w		{ NoDDChk }

	// Set MaskA and MaskB	
	mov (2)	MaskA<1>:uw		r[ECM_AddrReg, wEdgeCntlMapA_ExtLeftVert0]<2;2,1>:uw

	add (16) Mbaff_ALPHA2(0,0)<1>		Mbaff_ALPHA2(0,0)<16;16,1>		2:w					// alpha2 = (alpha >> 2) + 2  

//	CALL(FILTER_Y_MBAFF, 1)
	PRED_CALL(-f0.0, FILTER_Y_MBAFF, 1)
	
//BYPASS_V0_Y:
//------------------------------------------------------------------


/*
//---------- Deblock Y external left edge (V0) ----------	

	and.z.f0.0  (1) null:w		r[ECM_AddrReg, BitFlags]:ub		FilterLeftMbEdgeFlag:w		// Check for FilterLeftMbEdgeFlag 
    (f0.0)	jmpi	ILDB_LABEL(BYPASS_EXT_LEFT_EDGE_Y)	

	// Get vertical border edge control data  

//	mov	(1)	f0.0		0:w
	and	(1)	CTemp1_W:uw		r[ECM_AddrReg, BitFlags]:ub		FieldModeLeftMbFlag+FieldModeCurrentMbFlag:uw
	cmp.z.f0.0	(1)	null:w	CTemp1_W:uw		LEFT_FIELD_CUR_FRAME:w
	(-f0.0) jmpi LEFT_EDGE_Y_NEXT1

	// For FieldModeCurrentMbFlag=1 && FieldModeLeftMbFlag=0
	mov	(8)	Mbaff_ALPHA(0,0)<2>		r[ECM_AddrReg, bAlphaLeft0_Y]<0;1,0>:ub		{ NoDDClr }
	mov	(8)	Mbaff_ALPHA(0,1)<2>		r[ECM_AddrReg, bAlphaLeft1_Y]<0;1,0>:ub		{ NoDDChk }
	mov	(8)	Mbaff_BETA(0,0)<2>		r[ECM_AddrReg, bBetaLeft0_Y]<0;1,0>:ub		{ NoDDClr }
	mov	(8)	Mbaff_BETA(0,1)<2>		r[ECM_AddrReg, bBetaLeft1_Y]<0;1,0>:ub		{ NoDDChk }
	mov (8)	Mbaff_TC0(0,0)<2>		r[ECM_AddrReg, bTc0_v00_0_Y]<1;2,0>:ub		{ NoDDClr }
	mov (8)	Mbaff_TC0(0,1)<2>		r[ECM_AddrReg, bTc0_v00_1_Y]<1;2,0>:ub		{ NoDDChk }

	jmpi	LEFT_EDGE_Y_ALPHA_BETA_TC0_SELECTED

LEFT_EDGE_Y_NEXT1:
	cmp.z.f0.0	(1)	null:w	CTemp1_W:uw		LEFT_FRAME_CUR_FIELD:w
	(-f0.0) jmpi LEFT_EDGE_Y_NEXT2


	// For FieldModeCurrentMbFlag=0 && FieldModeLeftMbFlag=1
	mov	(8)	Mbaff_ALPHA(0,0)<1>		r[ECM_AddrReg, bAlphaLeft0_Y]<0;1,0>:ub		{ NoDDClr }
	mov	(8)	Mbaff_ALPHA(0,8)<1>		r[ECM_AddrReg, bAlphaLeft1_Y]<0;1,0>:ub		{ NoDDChk }
	mov	(8)	Mbaff_BETA(0,0)<1>		r[ECM_AddrReg, bBetaLeft0_Y]<0;1,0>:ub		{ NoDDClr }
	mov	(8)	Mbaff_BETA(0,8)<1>		r[ECM_AddrReg, bBetaLeft1_Y]<0;1,0>:ub		{ NoDDChk }
	mov (8)	Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_v00_0_Y]<1;2,0>:ub		{ NoDDClr }
	mov (8)	Mbaff_TC0(0,8)<1>		r[ECM_AddrReg, bTc0_v00_1_Y]<1;2,0>:ub		{ NoDDChk }

	jmpi	LEFT_EDGE_Y_ALPHA_BETA_TC0_SELECTED
	
LEFT_EDGE_Y_NEXT2:
	// both are frame or field
	mov	(16) Mbaff_ALPHA(0,0)<1>	r[ECM_AddrReg, bAlphaLeft0_Y]<0;1,0>:ub
	mov	(16) Mbaff_BETA(0,0)<1>		r[ECM_AddrReg, bBetaLeft0_Y]<0;1,0>:ub
	mov (16) Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_v00_0_Y]<1;4,0>:ub

LEFT_EDGE_Y_ALPHA_BETA_TC0_SELECTED:

	mov (2)	MaskA<1>:uw		r[ECM_AddrReg, wEdgeCntlMapA_ExtLeftVert0]<2;2,1>:uw

	//	p3 = Prev MB Y row 0 = r[P_AddrReg, 0]<16;16,1>
	//	p2 = Prev MB Y row 1 = r[P_AddrReg, 16]<16;16,1>
	//	p1 = Prev MB Y row 2 = r[P_AddrReg, 32]<16;16,1>
	//	p0 = Prev MB Y row 3 = r[P_AddrReg, 48]<16;16,1>
	// 	q0 = Cur MB Y row 0  = r[Q_AddrReg, 0]<16;16,1>
	//	q1 = Cur MB Y row 1  = r[Q_AddrReg, 16]<16;16,1>
	//	q2 = Cur MB Y row 2  = r[Q_AddrReg, 32]<16;16,1>
	//	q3 = Cur MB Y row 3  = r[Q_AddrReg, 48]<16;16,1>
	mov (1)	P_AddrReg:w		PREV_MB_Y_BASE:w	{ NoDDClr }
	mov (1)	Q_AddrReg:w		SRC_MB_Y_BASE:w		{ NoDDChk }
	
	// Get (alpha >> 2) + 2
	shr (16) Mbaff_ALPHA2(0,0)<1>	r[ECM_AddrReg, bAlphaLeft0_Y]<0;1,0>:ub		2:w			// alpha >> 2
	add (16) Mbaff_ALPHA2(0,0)<1>		Mbaff_ALPHA2(0,0)<16;16,1>		2:w					// alpha2 = (alpha >> 2) + 2  
	
	CALL(FILTER_Y_MBAFF, 1)

ILDB_LABEL(BYPASS_EXT_LEFT_EDGE_Y):
//------------------------------------------------------------------
*/

	// Same alpha, alpha2, beta and MaskB for all internal edges 
	
	// Get (alpha >> 2) + 2
	shr (16) Mbaff_ALPHA2(0,0)<1>	r[ECM_AddrReg, bAlphaInternal_Y]<0;1,0>:ub		2:w			// alpha >> 2
	
	// alpha = bAlphaInternal_Y
	// beta = bBetaInternal_Y
	mov	(16) Mbaff_ALPHA(0,0)<1>	r[ECM_AddrReg, bAlphaInternal_Y]<0;1,0>:ub
	mov	(16) Mbaff_BETA(0,0)<1>		r[ECM_AddrReg, bBetaInternal_Y]<0;1,0>:ub

	mov (1) MaskB:uw	0:w						// Set MaskB = 0 for all 3 edges, so it always uses bS < 4 algorithm.

	add (16) Mbaff_ALPHA2(0,0)<1>		Mbaff_ALPHA2(0,0)<16;16,1>		2:w						// alpha2 = (alpha >> 2) + 2  

//---------- Deblock Y internal left edge (V1) ----------

	// Bypass deblocking if FilterInternal4x4EdgesFlag = 0  
	and.z.f0.0  (1) null:w		r[ECM_AddrReg, BitFlags]:ub		FilterInternal4x4EdgesFlag:w		// Check for FilterInternal4x4EdgesFlag 
//    (f0.0)	jmpi	BYPASS_V1_Y

	//	p3 = Cur MB Y row 0 = r[P_AddrReg, 0]<16;16,1>  
	//	p2 = Cur MB Y row 1 = r[P_AddrReg, 16]<16;16,1>
	//	p1 = Cur MB Y row 2 = r[P_AddrReg, 32]<16;16,1>
	//	p0 = Cur MB Y row 3 = r[P_AddrReg, 48]<16;16,1>
	// 	q0 = Cur MB Y row 4 = r[Q_AddrReg, 0]<16;16,1> 
	//	q1 = Cur MB Y row 5 = r[Q_AddrReg, 16]<16;16,1>
	//	q2 = Cur MB Y row 6 = r[Q_AddrReg, 32]<16;16,1>
	//	q3 = Cur MB Y row 7 = r[Q_AddrReg, 48]<16;16,1>
	mov (1)	P_AddrReg:w		SRC_MB_Y_BASE:w						{ NoDDClr }
	mov (1)	Q_AddrReg:w		4*Y_ROW_WIDTH+SRC_MB_Y_BASE:w		{ NoDDChk }

	mov (1)	MaskA:uw	r[ECM_AddrReg, wEdgeCntlMap_IntLeftVert]:uw

	// tc0 has bTc0_v31_Y + bTc0_v21_Y + bTc0_v11_Y + bTc0_v01_Y	
	mov (16) Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_v01_Y]<1;4,0>:ub

//	CALL(FILTER_Y_MBAFF, 1)
	PRED_CALL(-f0.0, FILTER_Y_MBAFF, 1)

BYPASS_V1_Y:
//------------------------------------------------------------------


//---------- Deblock Y internal mid vert edge (V2) ----------

	// Bypass deblocking if FilterInternal8x8EdgesFlag = 0  
	and.z.f0.0	(1)	null:w	r[ECM_AddrReg, BitFlags]:ub		FilterInternal8x8EdgesFlag:w		// Check for FilterInternal4x4EdgesFlag 
//    (f0.0)	jmpi	BYPASS_V2_Y

	//	p3 = Cur MB Y row 4  = r[P_AddrReg, 0]<16;16,1>  
	//	p2 = Cur MB Y row 5  = r[P_AddrReg, 16]<16;16,1> 
	//	p1 = Cur MB Y row 6  = r[P_AddrReg, 32]<16;16,1> 
	//	p0 = Cur MB Y row 7  = r[P_AddrReg, 48]<16;16,1> 
	// 	q0 = Cur MB Y row 8  = r[Q_AddrReg, 0]<16;16,1>  
	//	q1 = Cur MB Y row 9  = r[Q_AddrReg, 16]<16;16,1> 
	//	q2 = Cur MB Y row 10 = r[Q_AddrReg, 32]<16;16,1> 
	//	q3 = Cur MB Y row 11 = r[Q_AddrReg, 48]<16;16,1> 
	mov (1)	P_AddrReg:w		4*Y_ROW_WIDTH+SRC_MB_Y_BASE:w		{ NoDDClr }
	mov (1)	Q_AddrReg:w		8*Y_ROW_WIDTH+SRC_MB_Y_BASE:w		{ NoDDChk }

	mov (1)	MaskA:uw	r[ECM_AddrReg, wEdgeCntlMap_IntMidVert]:uw

	// tc0 has bTc0_v32_Y + bTc0_v22_Y + bTc0_v12_Y + bTc0_v02_Y	
	mov (16) Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_v02_Y]<1;4,0>:ub

//	CALL(FILTER_Y_MBAFF, 1)
	PRED_CALL(-f0.0, FILTER_Y_MBAFF, 1)

BYPASS_V2_Y:
//-----------------------------------------------


//---------- Deblock Y interal right edge (V3) ----------	 

	// Bypass deblocking if FilterInternal4x4EdgesFlag = 0  
	and.z.f0.0	(1)	null:w	r[ECM_AddrReg, BitFlags]:ub		FilterInternal4x4EdgesFlag:w		// Check for FilterInternal4x4EdgesFlag 
//    (f0.0)	jmpi	BYPASS_V3_Y

	//	p3 = Cur MB Y row 8  = r[P_AddrReg, 0]<16;16,1> 
	//	p2 = Cur MB Y row 9  = r[P_AddrReg, 16]<16;16,1>
	//	p1 = Cur MB Y row 10 = r[P_AddrReg, 32]<16;16,1>
	//	p0 = Cur MB Y row 11 = r[P_AddrReg, 48]<16;16,1>
	// 	q0 = Cur MB Y row 12 = r[Q_AddrReg, 0]<16;16,1> 
	//	q1 = Cur MB Y row 13 = r[Q_AddrReg, 16]<16;16,1>
	//	q2 = Cur MB Y row 14 = r[Q_AddrReg, 32]<16;16,1>
	//	q3 = Cur MB Y row 15 = r[Q_AddrReg, 48]<16;16,1>
	mov (1)	P_AddrReg:w		8*Y_ROW_WIDTH+SRC_MB_Y_BASE:w		{ NoDDClr }
	mov (1)	Q_AddrReg:w		12*Y_ROW_WIDTH+SRC_MB_Y_BASE:w		{ NoDDChk }
	
	mov (1)	MaskA:uw	r[ECM_AddrReg, wEdgeCntlMap_IntRightVert]:uw

	// tc0 has bTc0_v33_Y + bTc0_v23_Y + bTc0_v13_Y + bTc0_v03_Y
	mov (16) Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_v03_Y]<1;4,0>:ub

//	CALL(FILTER_Y_MBAFF, 1)
	PRED_CALL(-f0.0, FILTER_Y_MBAFF, 1)

BYPASS_V3_Y:
//-----------------------------------------------
