/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
////////// AVC ILDB filter horizontal Mbaff Y ///////////////////////////////////////////////////////
//
//	This filter code prepares the src data and control data for ILDB filtering on all horizontal edges of Y.
//
//	It sssumes the data for horizontal de-blocking is already transposed.  
//
//		Luma:
//
//		+-------+-------+-------+-------+		H0  Edge
//		|		|		|		|		|
//		|		|		|		|		|
//		|		|		|		|		|
//		+-------+-------+-------+-------+		H1 Edge
//		|		|		|		|		|
//		|		|		|		|		|
//		|		|		|		|		|
//		+-------+-------+-------+-------+		H2	Edge
//		|		|		|		|		|
//		|		|		|		|		|
//		|		|		|		|		|
//		+-------+-------+-------+-------+		H3 Edge
//		|		|		|		|		|
//		|		|		|		|		|
//		|		|		|		|		|
//		+-------+-------+-------+-------+
//
/////////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xBBBB:w
#endif	
	

//========== Luma deblocking ==========


//---------- Deblock Y external top edge (H0)  ----------	

	// Bypass deblocking if it is the top edge of the picture.  
	and.z.f0.0  (1) null:w		r[ECM_AddrReg, BitFlags]:ub		FilterTopMbEdgeFlag:w		// Check for FilterTopMbEdgeFlag 
	mov	(1)	f0.1:w		DualFieldMode:w			// Check for dual field mode
		
	// Non dual field mode	

	// Get (alpha >> 2) + 2
	shr (16) Mbaff_ALPHA2(0,0)<1>	r[ECM_AddrReg, bAlphaTop0_Y]<0;1,0>:ub		2:w			// alpha >> 2

	mov (2)	MaskA<1>:uw	r[ECM_AddrReg, wEdgeCntlMapA_ExtTopHorz0]<2;2,1>:uw

	// Ext Y
	mov	(16) Mbaff_ALPHA(0,0)<1>	r[ECM_AddrReg, bAlphaTop0_Y]<0;1,0>:ub
	mov	(16) Mbaff_BETA(0,0)<1>		r[ECM_AddrReg, bBetaTop0_Y]<0;1,0>:ub
	mov (16) Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_h00_0_Y]<1;4,0>:ub

	add (16) Mbaff_ALPHA2(0,0)<1>		Mbaff_ALPHA2(0,0)<16;16,1>		2:w					// alpha2 = (alpha >> 2) + 2  

    (f0.0) jmpi	H0_Y_DONE				// Skip Ext Y deblocking
	(f0.1) jmpi	DUAL_FIELD_Y
	
	mov (1)	P_AddrReg:w		PREV_MB_Y_BASE:w		{ NoDDClr }
	mov (1)	Q_AddrReg:w		SRC_MB_Y_BASE:w			{ NoDDChk }
	
	CALL(FILTER_Y_MBAFF, 1)			// Non dual field deblocking
		
	jmpi	H0_Y_DONE

DUAL_FIELD_Y:
	// Dual field mode, FieldModeCurrentMbFlag=0 && FieldModeAboveMbFlag=1

	mov (1)	P_AddrReg:w		ABOVE_CUR_MB_BASE:w		{ NoDDClr }
	mov (1)	Q_AddrReg:w		ABOVE_CUR_MB_BASE+64:w	{ NoDDChk }

	//  Must use PREV_MB_YW.  TOP_MB_YW is not big enough.
	// Get top field rows
	mov (16) ABOVE_CUR_MB_YW(0)<1>	PREV_MB_YW(0, 0)<16;8,1>	// Copy p3, p2
	mov (16) ABOVE_CUR_MB_YW(1)<1>	PREV_MB_YW(2, 0)<16;8,1>	// Copy p1, p0
	mov (16) ABOVE_CUR_MB_YW(2)<1>	SRC_YW(0, 0)<16;8,1>		// Copy q0, q1
	mov (16) ABOVE_CUR_MB_YW(3)<1>	SRC_YW(2, 0)<16;8,1>		// Copy q2, q3

	CALL(FILTER_Y_MBAFF, 1)				// Ext Y, top field

	// Save deblocked top field rows
	mov (8) PREV_MB_YW(1, 0)<1>		ABOVE_CUR_MB_YW(0, 8)	// Copy p2
	mov (8) PREV_MB_YW(2, 0)<1>		ABOVE_CUR_MB_YW(1, 0)	// Copy p1
	mov (8) PREV_MB_YW(3, 0)<1>		ABOVE_CUR_MB_YW(1, 8)	// Copy p0
	mov (8) SRC_YW(0, 0)<1>			ABOVE_CUR_MB_YW(2, 0)	// Copy q0
	mov (8) SRC_YW(1, 0)<1>			ABOVE_CUR_MB_YW(2, 8)	// Copy q1
	mov (8) SRC_YW(2, 0)<1>			ABOVE_CUR_MB_YW(3, 0)	// Copy q2

	//==================================================================================
	// Bottom field
	
	// Get (alpha >> 2) + 2
	shr (16) Mbaff_ALPHA2(0,0)<1>	r[ECM_AddrReg, bAlphaTop1_Y]<0;1,0>:ub		2:w			// alpha >> 2

	mov (1)	P_AddrReg:w		ABOVE_CUR_MB_BASE:w		{ NoDDClr }
	mov (1)	Q_AddrReg:w		ABOVE_CUR_MB_BASE+64:w	{ NoDDChk }
	
	// Get bot field rows
	mov (16) ABOVE_CUR_MB_YW(0)<1>	PREV_MB_YW(0, 8)<16;8,1>	// Copy p3, p2
	mov (16) ABOVE_CUR_MB_YW(1)<1>	PREV_MB_YW(2, 8)<16;8,1>	// Copy p1, p0
	mov (16) ABOVE_CUR_MB_YW(2)<1>	SRC_YW(0, 8)<16;8,1>		// Copy q0, q1
	mov (16) ABOVE_CUR_MB_YW(3)<1>	SRC_YW(2, 8)<16;8,1>		// Copy q2, q3

	mov (2)	MaskA<1>:uw	r[ECM_AddrReg, wEdgeCntlMapA_ExtTopHorz1]<2;2,1>:uw

	mov	(16) Mbaff_ALPHA(0,0)<1>	r[ECM_AddrReg, bAlphaTop1_Y]<0;1,0>:ub
	mov	(16) Mbaff_BETA(0,0)<1>		r[ECM_AddrReg, bBetaTop1_Y]<0;1,0>:ub
	mov (16) Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_h00_1_Y]<1;4,0>:ub

	add (16) Mbaff_ALPHA2(0,0)<1>		Mbaff_ALPHA2(0,0)<16;16,1>		2:w					// alpha2 = (alpha >> 2) + 2  

	CALL(FILTER_Y_MBAFF, 1)				// Ext Y, bot field

	// Save deblocked top field rows
	mov (8) PREV_MB_YW(1, 8)<1>		ABOVE_CUR_MB_YW(0, 8)	// Copy p2
	mov (8) PREV_MB_YW(2, 8)<1>		ABOVE_CUR_MB_YW(1, 0)	// Copy p1
	mov (8) PREV_MB_YW(3, 8)<1>		ABOVE_CUR_MB_YW(1, 8)	// Copy p0
	mov (8) SRC_YW(0, 8)<1>			ABOVE_CUR_MB_YW(2, 0)	// Copy q0
	mov (8) SRC_YW(1, 8)<1>			ABOVE_CUR_MB_YW(2, 8)	// Copy q1
	mov (8) SRC_YW(2, 8)<1>			ABOVE_CUR_MB_YW(3, 0)	// Copy q2
	//==================================================================================

H0_Y_DONE:

//BYPASS_H0_Y:
//------------------------------------------------------------------
	// Same alpha, alpha2, beta and MaskB for all internal edges 

	// Get (alpha >> 2) + 2
	shr (16) Mbaff_ALPHA2(0,0)<1>	r[ECM_AddrReg, bAlphaInternal_Y]<0;1,0>:ub		2:w			// alpha >> 2

	// alpha = bAlphaInternal_Y 
	// beta = bBetaInternal_Y
	mov	(16) Mbaff_ALPHA(0,0)<1>	r[ECM_AddrReg, bAlphaInternal_Y]<0;1,0>:ub
	mov	(16) Mbaff_BETA(0,0)<1>		r[ECM_AddrReg, bBetaInternal_Y]<0;1,0>:ub

	mov (1) MaskB:uw	0:w						// Set MaskB = 0 for all 3 edges, so it always uses bS < 4 algorithm.

	add (16) Mbaff_ALPHA2(0,0)<1>		Mbaff_ALPHA2(0,0)<16;16,1>		2:w					// alpha2 = (alpha >> 2) + 2  

//---------- Deblock Y internal top edge (H1)  ----------

	// Bypass deblocking if FilterInternal4x4EdgesFlag = 0  
	and.z.f0.0 (1) null:w	r[ECM_AddrReg, BitFlags]:ub		FilterInternal4x4EdgesFlag:w		// Check for FilterInternal4x4EdgesFlag 
//    (f0.0)	jmpi	BYPASS_H1_Y

	//	p3 = Cur MB Y row 0 = r[P_AddrReg, 0]<16;16,1> 
	//	p2 = Cur MB Y row 1 = r[P_AddrReg, 16]<16;16,1>
	//	p1 = Cur MB Y row 2 = r[P_AddrReg, 32]<16;16,1>
	//	p0 = Cur MB Y row 3 = r[P_AddrReg, 48]<16;16,1>
	// 	q0 = Cur MB Y row 4 = r[Q_AddrReg, 0]<16;16,1> 
	//	q1 = Cur MB Y row 5 = r[Q_AddrReg, 16]<16;16,1>
	//	q2 = Cur MB Y row 6 = r[Q_AddrReg, 32]<16;16,1>
	//	q3 = Cur MB Y row 7 = r[Q_AddrReg, 48]<16;16,1>
	mov (1)	P_AddrReg:w		SRC_MB_Y_BASE:w					{ NoDDClr }
	mov (1)	Q_AddrReg:w		4*Y_ROW_WIDTH+SRC_MB_Y_BASE:w	{ NoDDChk }

	mov (1)	MaskA:uw	r[ECM_AddrReg, wEdgeCntlMap_IntTopHorz]:uw

	// tc0 has bTc0_h13_Y + bTc0_h12_Y + bTc0_h11_Y + bTc0_h10_Y		
	mov (16) Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_h10_Y]<1;4,0>:ub

//	CALL(FILTER_Y_MBAFF, 1)
	PRED_CALL(-f0.0, FILTER_Y_MBAFF, 1)

//BYPASS_H1_Y:
//------------------------------------------------------------------


//---------- Deblock Y internal mid horizontal edge (H2) ----------

	// Bypass deblocking if FilterInternal8x8EdgesFlag = 0  
	and.z.f0.0 (1) null:w	r[ECM_AddrReg, BitFlags]:ub		FilterInternal8x8EdgesFlag:w		// Check for FilterInternal4x4EdgesFlag 
//    (f0.0)	jmpi	BYPASS_H2_Y

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

	mov (1)	MaskA:uw	r[ECM_AddrReg, wEdgeCntlMap_IntMidHorz]:uw

	// tc0 has bTc0_h23_Y + bTc0_h22_Y + bTc0_h21_Y + bTc0_h20_Y		
	mov (16) Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_h20_Y]<1;4,0>:ub

//	CALL(FILTER_Y_MBAFF, 1)
	PRED_CALL(-f0.0, FILTER_Y_MBAFF, 1)

//BYPASS_H2_Y:
//-----------------------------------------------


//---------- Deblock Y internal bottom edge (H3) ----------	 

	// Bypass deblocking if FilterInternal4x4EdgesFlag = 0  
	and.z.f0.0 (1) null:w	r[ECM_AddrReg, BitFlags]:ub		FilterInternal4x4EdgesFlag:w		// Check for FilterInternal4x4EdgesFlag 
//    (f0.0)	jmpi	BYPASS_H3_Y

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
	
	mov (1)	MaskA:uw	r[ECM_AddrReg, wEdgeCntlMap_IntBotHorz]:uw

	// tc0 has bTc0_h33_Y + bTc0_h32_Y + bTc0_h31_Y + bTc0_h30_Y
	mov (16) Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_h30_Y]<1;4,0>:ub

//	CALL(FILTER_Y_MBAFF, 1)
	PRED_CALL(-f0.0, FILTER_Y_MBAFF, 1)

//BYPASS_H3_Y:
//-----------------------------------------------
