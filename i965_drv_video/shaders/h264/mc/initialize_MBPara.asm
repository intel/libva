/*
 * Initialize parameters
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: Initialize_MBPara.asm
//


//#if !defined(__INITIALIZE_MBPARA__)		// Make sure this is only included once
//#define __INITIALIZE_MBPARA__


// WA for weighted prediction - 2007/09/06		// shlee
//	mov (1)		guwW128(0)<1>			guwR1(0)<0;1,0>		// Copy the unique number indicating weight/offset=(128,0)



//	MB Type		Category
//	1			B_L0_16x16
//	2			B_L1_16x16
//	3			B_Bi_16x16
//	4			B_L0_L0_16x8
//	5			B_L0_L0_8x16
//	6			B_L1_L1_16x8
//	7			B_L1_L1_8x16
//	8			B_L0_L1_16x8
//	9			B_L0_L1_8x16
//	10			B_L1_L0_16x8
//	11			B_L1_L0_8x16
//	12			B_L0_Bi_16x8
//	13			B_L0_Bi_8x16
//	14			B_L1_Bi_16x8
//	15			B_L1_Bi_8x16
//	16			B_Bi_L0_16x8
//	17			B_Bi_L0_8x16
//	18			B_Bi_L1_16x8
//	19			B_Bi_L1_8x16
//	20			B_Bi_Bi_16x8
//	21			B_Bi_Bi_8x16
//	22			B_8x8

	// TODO:
	// Initialize interpolation area to eliminate uninitialized registers making the results of mac instructions XX.
	// This issue was reported by Sharath on 5/25/2006, and why multiplication by zero still yields XX has not been understood yet.
#if 0
	mov (16)	gudINTPY0(0)<1>		0:ud	{Compr}
	mov (16)	gudINTPY0(2)<1>		0:ud	{Compr}
	mov (16)	gudINTPY1(0)<1>		0:ud	{Compr}
	mov (16)	gudINTPY1(2)<1>		0:ud	{Compr}
	mov (16)	gudINTPC0(0)<1>		0:ud	{Compr}
	mov (16)	gudINTPC1(0)<1>		0:ud	{Compr}
#endif

	mov (1)		gMVSTEP:w			0:w								// Address increament for MV read

	cmp.e.f0.0 (1) null:w			gwMBTYPE<0;1,0>					22:w
	(-f0.0)		jmpi				INTERLABEL(NOT_8x8_MODE)

	//--- 8x8 mode
		
	// Starting address of error data blocks
	cmp.e.f0.1 (2) null<1>:w		gSUBMB_SHAPE<0;1,0>:ub			0:w
	(f0.1) jmpi INTERLABEL(CONVERT_MVS)

	// Note: MVs and Weights/Offsets are already expanded by HW or driver
	
	// MV conversion - Convert each MV to absolute coord. (= MV + MB org. + block offset) 
	shl (16)	gwTEMP(0)<1>		gX<0;2,1>:w						2:w // Convert MB origin to 1/4-pel unit
	mov (1)		gMVSTEP:w			24:w							// Address increament for MV read
	add (2)		gwTEMP(0,4)<2>		gwTEMP(0,4)<4;2,2>				16:w
	add (2)		gwTEMP(0,9)<2>		gwTEMP(0,9)<4;2,2>				16:w
	add (4)		gwTEMP(0,12)<1>		gwTEMP(0,12)<4;4,1>				16:w
	
	add (16)	gMV<1>:w			gMV<16;16,1>:w					gwTEMP(0)<16;16,1>
	add (8)		gwTEMP(0)<2>		gwTEMP(0)<16;8,2>				32:w
	add (16)	gwMV(1,0)<1>		gwMV(1,0)<16;16,1>				gwTEMP(0)<16;16,1>	
	add (8)		gwTEMP(0,1)<2>		gwTEMP(0,1)<16;8,2>				32:w
	add (16)	gwMV(3,0)<1>		gwMV(3,0)<16;16,1>				gwTEMP(0)<16;16,1>	
	add (8)		gwTEMP(0)<2>		gwTEMP(0)<16;8,2>				-32:w
	add (16)	gwMV(2,0)<1>		gwMV(2,0)<16;16,1>				gwTEMP(0)<16;16,1>	

	jmpi INTERLABEL(INIT_ADDRESS_REGS)

INTERLABEL(NOT_8x8_MODE):

	//--- !8x8 mode (16x16, 16x8, 8x16)

	// MVs and Weights/Offsets are expanded
	cmp.le.f0.1 (8) null<1>:w		gwMBTYPE<0;1,0>					3:w // Check 16x16
	mov (1)		gSUBMB_SHAPE:ub		0:uw							// subMB shape
	(f0.1) mov (8)	gMV<1>:d		gMV<0;2,1>:d					
	(f0.1) mov (8)	gdWGT(1,0)<1>	gWGT<0;4,1>:d					
	(f0.1) mov (4)	gdWGT(0,4)<1>	gWGT<4;4,1>:d					
	
INTERLABEL(CONVERT_MVS):
	// MV conversion - Convert each MV to absolute coord. (= MV + MB org. + block offset)
	shl (2)		gwTEMP(0)<1>		gX<2;2,1>:w						2:w // Convert MB origin to 1/4-pel unit
	add (16)	gMV<1>:w			gMV<16;16,1>:w					gwTEMP(0)<0;2,1>
	add (2)		gwMV(0,4)<2>		gwMV(0,4)<4;2,2>				32:w	//{NoDDClr}
	add (2)		gwMV(0,9)<2>		gwMV(0,9)<4;2,2>				32:w	//{NoDDChk,NoDDClr}
	add (4)		gwMV(0,12)<1>		gwMV(0,12)<4;4,1>				32:w	//{NoDDChk}
		
INTERLABEL(INIT_ADDRESS_REGS):
	// Initialize the address registers
	mov (2)		pERRORYC:ud			nOFFSET_ERROR:ud				{NoDDClr} // Address of Y and C error blocks
	mov (1)		pRECON_MV:ud		nOFFSET_RECON_MV:ud				{NoDDChk,NoDDClr} // Address of recon area and motion vectors
	mov (1)		pWGT_BIDX:ud		nOFFSET_WGT_BIDX:ud				{NoDDChk} // Address of weights/offsets and binding tbl idx
	
	// Read the parity of the current field (gPARITY - 0:top, 1:bottom, 3:frame)
	// and set message descriptor for frame/field write
#if defined(MBAFF)
	and.nz.f0.0 (1) null:uw			gFIELDMBFLAG:ub					nFIELDMB_MASK:uw
	(f0.0) and (1)	gPARITY:uw		gMBPARITY:ub					nMBPARITY_MASK:uw
	(-f0.0) mov (1) gPARITY:uw		3:uw
#elif defined(FIELD)
	and (1)		gPARITY:uw			gMBPARITY:ub					nMBPARITY_MASK:uw
#endif
	
        
//#endif	// !defined(__INITIALIZE_MBPARA__)
