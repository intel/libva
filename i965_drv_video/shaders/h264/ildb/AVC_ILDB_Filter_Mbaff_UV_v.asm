/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
////////// AVC LDB filter vertical Mbaff UV ///////////////////////////////////////////////////////
//
//	This filter code prepares the src data and control data for ILDB filtering on all vertical edges of UV.
//
//	It sssumes the data for vertical de-blocking is already transposed.  
//
//		Chroma:
//
//		+-------+-------+
//		|		|		|
//		|		|		|
//		|		|		|
//		+-------+-------+
//		|		|		|
//		|		|		|
//		|		|		|
//		+-------+-------+
//
//		V0		V1		
//		Edge	Edge	
//
/////////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0xBBBC:w
#endif	

//=============== Chroma deblocking ================

//---------- Deblock U external left edge ----------

	and.z.f0.0  (1) null:w		r[ECM_AddrReg, BitFlags]:ub		FilterLeftMbEdgeFlag:w		// Check for FilterLeftMbEdgeFlag 

	cmp.z.f0.1	(1)	null:w	VertEdgePattern:uw		LEFT_FIELD_CUR_FRAME:w

	// Get Luma maskA and maskB	
	shr (16)	TempRow0(0)<1>		r[ECM_AddrReg, wEdgeCntlMapA_ExtLeftVert0]<0;1,0>:uw		RRampW(0)
	shr (16)	TempRow1(0)<1>		r[ECM_AddrReg, wEdgeCntlMapB_ExtLeftVert0]<0;1,0>:uw		RRampW(0)
	
    (f0.0)	jmpi	BYPASS_V0_UV	// Do not deblock Left ext edge

	cmp.z.f0.0	(1)	null:w	VertEdgePattern:uw		LEFT_FRAME_CUR_FIELD:w

	(-f0.1) jmpi V0_U_NEXT1	// Jump if not LEFT_FIELD_CUR_FRAME

	//----- For LEFT_FIELD_CUR_FRAME
	
	// Extract UV MaskA and MaskB from every other 2 bits of Y masks
	and.nz.f0.0 (8) null:w			TempRow0(0)<4;2,1>		1:w
	and.nz.f0.1 (8) null:w			TempRow1(0)<4;2,1>		1:w

	// For FieldModeLeftMbFlag=1 && FieldModeCurrentMbFlag=0 
	mov	(4)	Mbaff_ALPHA(0,0)<2>		r[ECM_AddrReg, bAlphaLeft0_Cb]<0;1,0>:ub	{ NoDDClr }
	mov	(4)	Mbaff_ALPHA(0,1)<2>		r[ECM_AddrReg, bAlphaLeft1_Cb]<0;1,0>:ub	{ NoDDChk }
	mov	(4)	Mbaff_BETA(0,0)<2>		r[ECM_AddrReg, bBetaLeft0_Cb]<0;1,0>:ub		{ NoDDClr }
	mov	(4)	Mbaff_BETA(0,1)<2>		r[ECM_AddrReg, bBetaLeft1_Cb]<0;1,0>:ub		{ NoDDChk }
	mov (4)	Mbaff_TC0(0,0)<2>		r[ECM_AddrReg, bTc0_v00_0_Cb]<4;4,1>:ub		{ NoDDClr }
	mov (4)	Mbaff_TC0(0,1)<2>		r[ECM_AddrReg, bTc0_v00_1_Cb]<4;4,1>:ub		{ NoDDChk }

	jmpi	V0_U_NEXT3

V0_U_NEXT1:
	
	(-f0.0) jmpi V0_U_NEXT2			// Jump if not LEFT_FRAME_CUR_FIELD
	
	//----- For LEFT_FRAME_CUR_FIELD
		
	// Extract UV MaskA and MaskB from every other bit of Y masks
	and.nz.f0.0 (8) null:w			TempRow0(0)<16;8,2>		1:w
	and.nz.f0.1 (8) null:w			TempRow1(0)<16;8,2>		1:w

	// For FieldModeLeftMbFlag=0 && FieldModeCurrentMbFlag=1
	mov	(4)	Mbaff_ALPHA(0,0)<1>		r[ECM_AddrReg, bAlphaLeft0_Cb]<0;1,0>:ub	{ NoDDClr }
	mov	(4)	Mbaff_ALPHA(0,4)<1>		r[ECM_AddrReg, bAlphaLeft1_Cb]<0;1,0>:ub	{ NoDDChk }
	mov	(4)	Mbaff_BETA(0,0)<1>		r[ECM_AddrReg, bBetaLeft0_Cb]<0;1,0>:ub		{ NoDDClr }
	mov	(4)	Mbaff_BETA(0,4)<1>		r[ECM_AddrReg, bBetaLeft1_Cb]<0;1,0>:ub		{ NoDDChk }
	mov (4)	Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_v00_0_Cb]<4;4,1>:ub		{ NoDDClr }
	mov (4)	Mbaff_TC0(0,4)<1>		r[ECM_AddrReg, bTc0_v00_1_Cb]<4;4,1>:ub		{ NoDDChk }

	jmpi	V0_U_NEXT3
	
V0_U_NEXT2:

	// Extract UV MaskA and MaskB from every other bit of Y masks
	and.nz.f0.0 (8) null:w			TempRow0(0)<16;8,2>		1:w
	and.nz.f0.1 (8) null:w			TempRow1(0)<16;8,2>		1:w
	
	// Both are frames or fields
	mov	(8) Mbaff_ALPHA(0,0)<1>		r[ECM_AddrReg, bAlphaLeft0_Cb]<0;1,0>:ub
	mov	(8) Mbaff_BETA(0,0)<1>		r[ECM_AddrReg, bBetaLeft0_Cb]<0;1,0>:ub
	mov (8) Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_v00_0_Cb]<1;2,0>:ub

V0_U_NEXT3:	

	//	p1 = Prev MB U row 0
	//	p0 = Prev MB U row 1
	// 	q0 = Cur MB U row 0
	//	q1 = Cur MB U row 1
	mov (1)	P_AddrReg:w		PREV_MB_U_BASE:w	{ NoDDClr }
	mov (1)	Q_AddrReg:w		SRC_MB_U_BASE:w		{ NoDDChk }

	// Store UV MaskA and MaskB
	mov (2)		MaskA<1>:uw			f0.0<2;2,1>:uw

	CALL(FILTER_UV_MBAFF, 1)	
//-----------------------------------------------

//---------- Deblock V external left edge ----------

	// No change to MaskA and MaskB

	cmp.z.f0.0	(4)	null:w	VertEdgePattern:uw		LEFT_FIELD_CUR_FRAME:w
	cmp.z.f0.1	(4)	null:w	VertEdgePattern:uw		LEFT_FRAME_CUR_FIELD:w

	// both are frame or field
	mov	(8) Mbaff_ALPHA(0,0)<1>		r[ECM_AddrReg, bAlphaLeft0_Cr]<0;1,0>:ub
	mov	(8) Mbaff_BETA(0,0)<1>		r[ECM_AddrReg, bBetaLeft0_Cr]<0;1,0>:ub
	mov (8) Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_v00_0_Cr]<1;2,0>:ub
				
	//	p1 = Prev MB V row 0
	//	p0 = Prev MB V row 1
	// 	q0 = Cur MB V row 0
	//	q1 = Cur MB V row 1
	mov (1)	P_AddrReg:w		PREV_MB_V_BASE:w	{ NoDDClr }
	mov (1)	Q_AddrReg:w		SRC_MB_V_BASE:w		{ NoDDChk }
				
	// For FieldModeLeftMbFlag=1 && FieldModeCurrentMbFlag=0 
	(f0.0) mov (4)	Mbaff_ALPHA(0,0)<2>		r[ECM_AddrReg, bAlphaLeft0_Cr]<0;1,0>:ub	{ NoDDClr }
	(f0.0) mov (4)	Mbaff_ALPHA(0,1)<2>		r[ECM_AddrReg, bAlphaLeft1_Cr]<0;1,0>:ub	{ NoDDChk }	
	(f0.0) mov (4)	Mbaff_BETA(0,0)<2>		r[ECM_AddrReg, bBetaLeft0_Cr]<0;1,0>:ub		{ NoDDClr }
	(f0.0) mov (4)	Mbaff_BETA(0,1)<2>		r[ECM_AddrReg, bBetaLeft1_Cr]<0;1,0>:ub		{ NoDDChk }
	(f0.0) mov (4)	Mbaff_TC0(0,0)<2>		r[ECM_AddrReg, bTc0_v00_0_Cr]<4;4,1>:ub		{ NoDDClr }
	(f0.0) mov (4)	Mbaff_TC0(0,1)<2>		r[ECM_AddrReg, bTc0_v00_1_Cr]<4;4,1>:ub		{ NoDDChk }

	// For FieldModeLeftMbFlag=0 && FieldModeCurrentMbFlag=1
	(f0.1) mov (4)	Mbaff_ALPHA(0,0)<1>		r[ECM_AddrReg, bAlphaLeft0_Cr]<0;1,0>:ub	{ NoDDClr }
	(f0.1) mov (4)	Mbaff_ALPHA(0,4)<1>		r[ECM_AddrReg, bAlphaLeft1_Cr]<0;1,0>:ub	{ NoDDChk }
	(f0.1) mov (4)	Mbaff_BETA(0,0)<1>		r[ECM_AddrReg, bBetaLeft0_Cr]<0;1,0>:ub		{ NoDDClr }
	(f0.1) mov (4)	Mbaff_BETA(0,4)<1>		r[ECM_AddrReg, bBetaLeft1_Cr]<0;1,0>:ub		{ NoDDChk }
	(f0.1) mov (4)	Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_v00_0_Cr]<4;4,1>:ub		{ NoDDClr }
	(f0.1) mov (4)	Mbaff_TC0(0,4)<1>		r[ECM_AddrReg, bTc0_v00_1_Cr]<4;4,1>:ub		{ NoDDChk }

	// Set UV MaskA and MaskB
	mov (2)		f0.0<1>:uw		MaskA<2;2,1>:uw

	CALL(FILTER_UV_MBAFF, 1)	
//-----------------------------------------------

BYPASS_V0_UV:
	// Set EdgeCntlMap2 = 0, so it always uses bS < 4 algorithm.
	// Same alpha and beta for all internal vert and horiz edges 

//---------- Deblock U internal vert middle edge ----------

	//***** Need to take every other bit to form U or V maskA
	shr (16) TempRow0(0)<1>			r[ECM_AddrReg, wEdgeCntlMap_IntMidVert]<0;1,0>:uw		RRampW(0)

	//	p1 = Cur MB U row 2
	//	p0 = Cur MB U row 3
	// 	q0 = Cur MB U row 4
	//	q1 = Cur MB U row 5
	mov (1)	P_AddrReg:w		4*UV_ROW_WIDTH+SRC_MB_U_BASE:w		{ NoDDClr }		// Skip 2 U rows and 2 V rows
	mov (1)	Q_AddrReg:w		8*UV_ROW_WIDTH+SRC_MB_U_BASE:w		{ NoDDChk }

	mov	(8) Mbaff_ALPHA(0,0)<1>		r[ECM_AddrReg, bAlphaInternal_Cb]<0;1,0>:ub
	mov	(8) Mbaff_BETA(0,0)<1>		r[ECM_AddrReg, bBetaInternal_Cb]<0;1,0>:ub
	mov (8) Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_v02_Cb]<1;2,0>:ub

	and.nz.f0.0 (8) null:w			TempRow0(0)<16;8,2>		1:w

	// Store MaskA and MaskB
	mov (1)	f0.1:uw		0:w			
	mov (1)	MaskB:uw	0:w			{ NoDDClr }
	mov (1)	MaskA:uw	f0.0:uw		{ NoDDChk }

	CALL(FILTER_UV_MBAFF, 1)	
	
//-----------------------------------------------


//---------- Deblock V internal vert middle edge ----------

	//	P1 = Cur MB V row 2
	//	P0 = Cur MB V row 3
	// 	Q0 = Cur MB V row 4
	//	Q1 = Cur MB V row 5
	mov (1)	P_AddrReg:w		4*UV_ROW_WIDTH+SRC_MB_V_BASE:w		{ NoDDClr }		// Skip 2 U rows and 2 V rows
	mov (1)	Q_AddrReg:w		8*UV_ROW_WIDTH+SRC_MB_V_BASE:w		{ NoDDChk }

	// Put MaskA into f0.0
	// Put MaskB into f0.1
	mov (2)	f0.0<1>:uw		MaskA<2;2,1>:uw

	mov	(8) Mbaff_ALPHA(0,0)<1>		r[ECM_AddrReg, bAlphaInternal_Cr]<0;1,0>:ub
	mov	(8) Mbaff_BETA(0,0)<1>		r[ECM_AddrReg, bBetaInternal_Cr]<0;1,0>:ub
	mov (8) Mbaff_TC0(0,0)<1>		r[ECM_AddrReg, bTc0_v02_Cr]<1;2,0>:ub

	CALL(FILTER_UV_MBAFF, 1)	

//-----------------------------------------------

