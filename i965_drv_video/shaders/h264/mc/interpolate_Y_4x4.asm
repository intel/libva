/*
 * Interpolation kernel for luminance motion compensation
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: Interpolate_Y_4x4.asm
//
// Interpolation kernel for luminance motion compensation
//
//  $Revision: 10 $
//  $Date: 10/09/06 4:00p $
//


	// Compute the GRF address of the starting position of the reference area
#if 1
    (-f0.1) mov (1)	pREF:w			nOFFSET_REF+2+nGRFWIB:w
    (f0.1) mov (1)	pREF:w			nOFFSET_REF+2:w		
	mov (1)		pRESULT:uw			gpINTPY:uw											
#else
    mov (1)		pREF:w				nOFFSET_REF+2+nGRFWIB:w	{NoDDClr}
	mov (1)		pRESULT:uw			gpINTPY:uw				{NoDDChk}
#endif
	
	/*
	 *			 |		 |
	 *		 - - 0 1 2 3 + - 
	 *			 4 5 6 7
	 *			 8 9 A B
	 *			 C D E F
	 *		 - - + - - - + -
     *			 |		 |
	 */
	
	// Case 0
	or.z.f0.1 (16) null:w			gMVY_FRAC<0;1,0>:w				gMVX_FRAC<0;1,0>:w	
	(f0.1) mov (4)	r[pRESULT]<1>:uw	r[pREF0]<4;4,1>:ub
	(f0.1) mov (4)	r[pRESULT,16]<1>:uw	r[pREF0,16]<4;4,1>:ub
	(f0.1) mov (4)	r[pRESULT,32]<1>:uw r[pREF0,32]<4;4,1>:ub
	(f0.1) mov (4)	r[pRESULT,48]<1>:uw r[pREF0,48]<4;4,1>:ub
	(f0.1) jmpi INTERLABEL(Exit_Interpolate_Y_4x4)
	
	// Store all address registers
	mov (8)		gpADDR<1>:w			a0<8;8,1>:w
	
	mul.z.f0.0 (1) gW4:w			gMVY_FRAC:w						gMVX_FRAC:w
	and.nz.f0.1 (1) null			gW4:w							1:w

	add (1)		pREF1:uw			pREF0:uw						nGRFWIB/2:uw
	add (2)		pREF2<1>:uw			pREF0<2;2,1>:uw					nGRFWIB:uw
	mov (4)		gW0<1>:uw			pREF0<4;4,1>:uw

	(f0.0) jmpi INTERLABEL(Interpolate_Y_H_4x4)
	(f0.1) jmpi INTERLABEL(Interpolate_Y_H_4x4)	
	
	//-----------------------------------------------------------------------
	// CASE: A69BE (H/V interpolation)
	//-----------------------------------------------------------------------

	// Compute interim horizontal intepolation 
	add (1)		pREF0<1>:uw			pREF0<0;1,0>:uw					-34:w 
	add (1)		pREF1<1>:uw			pREF1<0;1,0>:uw					-18:w {NoDDClr}
	mov (1)		pRESD:ud			nOFFSET_INTERIM4x4_5:ud				{NoDDChk} // Case 69be
	
	// Check whether this position is 'A'
	cmp.e.f0.0 (1) null				gW4:w							4:w

    $for(0;<2;1) {
	add (16)	acc0<1>:w			r[pREF0,nGRFWIB*2*%1]<16;4,1>:ub			r[pREF0,nGRFWIB*2*%1+5]<16;4,1>:ub		{Compr}
	mac (16)	acc0<1>:w			r[pREF0,nGRFWIB*2*%1+1]<16;4,1>:ub			-5:w	{Compr}
	mac (16)	acc0<1>:w			r[pREF0,nGRFWIB*2*%1+2]<16;4,1>:ub			20:w	{Compr}
	mac (16)	acc0<1>:w			r[pREF0,nGRFWIB*2*%1+3]<16;4,1>:ub			20:w	{Compr}
	mac (16)	r[pRES,nGRFWIB*%1]<1>:w		r[pREF0,nGRFWIB*2*%1+4]<16;4,1>:ub	-5:w	{Compr}
	}
	// last line
	add (4)		acc0<1>:w			r[pREF0,nGRFWIB*2*2]<4;4,1>:ub				r[pREF0,nGRFWIB*2*2+5]<4;4,1>:ub
	mac (4)		acc0<1>:w			r[pREF0,nGRFWIB*2*2+1]<4;4,1>:ub			-5:w
	mac (4)		acc0<1>:w			r[pREF0,nGRFWIB*2*2+2]<4;4,1>:ub			20:w
	mac (4)		acc0<1>:w			r[pREF0,nGRFWIB*2*2+3]<4;4,1>:ub			20:w
	mac (4)		r[pRES,nGRFWIB*2]<1>:w		r[pREF0,nGRFWIB*2*2+4]<4;4,1>:ub	-5:w
	
    // Compute interim/output vertical interpolation 
	mov (1)		pREF6D:ud			nOFFSET_INTERIM4x4_4:ud		{NoDDClr}
	mov (1)		pREF0D:ud			nOFFSET_INTERIM4x4_7:ud		{NoDDChk,NoDDClr}
	mov (1)		pREF2D:ud			nOFFSET_INTERIM4x4_8:ud		{NoDDChk,NoDDClr}
	mov (1)		pREF4D:ud			nOFFSET_INTERIM4x4_9:ud		{NoDDChk}

 	add (16)	acc0<1>:w			gwINTERIM4x4_BUF(0)<16;16,1>		512:w
	mac (16)	acc0<1>:w			gwINTERIM4x4_BUF(1)<16;16,1>		-5:w
	mac (16)	acc0<1>:w			r[pREF6,0]<8,1>:w				20:w
	
	(f0.0) mov (1) pRES:uw			nOFFSET_RES:uw					// Case a
	(-f0.0) mov (1) pRES:uw			nOFFSET_INTERIM4x4:uw				// Case 69be
	
	mac (16)	acc0<1>:w			r[pREF0,0]<4,1>:w				-5:w
	mac (16)	acc0<1>:w			r[pREF0,nGRFWIB]<4,1>:w			1:w
	mac (16)	acc0<1>:w			r[pREF2,0]<4,1>:w				20:w	
	asr.sat (16) r[pRES]<2>:ub		acc0<16;16,1>:w					10:w  
	
	(f0.0) jmpi INTERLABEL(Return_Interpolate_Y_4x4)

INTERLABEL(Interpolate_Y_H_4x4):
	
	cmp.e.f0.0 (1) null				gMVX_FRAC:w						0:w
	cmp.e.f0.1 (1) null				gMVY_FRAC:w						2:w
	(f0.0) jmpi INTERLABEL(Interpolate_Y_V_4x4)
	(f0.1) jmpi INTERLABEL(Interpolate_Y_V_4x4)

	//-----------------------------------------------------------------------
	// CASE: 123567DEF (H interpolation)
	//-----------------------------------------------------------------------
	
	add (4)		pREF0<1>:uw			gW0<4;4,1>:uw					-2:w		
	cmp.g.f0.0 (4) null:w			gMVY_FRAC<0;1,0>:w				2:w
	cmp.e.f0.1 (1) null				gMVX_FRAC:w						2:w
	(f0.0) add (4) pREF0<1>:uw		pREF0<4;4,1>:uw					nGRFWIB/2:uw
	
	cmp.e.f0.0 (1) null:w			gMVY_FRAC<0;1,0>:w				0:w

	(f0.1) mov (1) pRESULT:uw		nOFFSET_RES:uw					// Case 26E
	(-f0.1) mov (1) pRESULT:uw		nOFFSET_INTERIM4x4:uw			// Case 1357DF
	
	// Compute interim/output horizontal interpolation
	add (16)	acc0<1>:w			r[pREF0]<4,1>:ub				16:w
	mac (16)	acc0<1>:w			r[pREF0,1]<4,1>:ub				-5:w
	mac (16)	acc0<1>:w			r[pREF0,2]<4,1>:ub				20:w
	mac (16)	acc0<1>:w			r[pREF0,3]<4,1>:ub				20:w
	mac (16)	acc0<1>:w			r[pREF0,4]<4,1>:ub				-5:w
	mac (16)	acc0<1>:w			r[pREF0,5]<4,1>:ub				1:w
	asr.sat (16) r[pRESULT]<2>:ub	acc0<16;16,1>:w					5:w
	
	(-f0.1) jmpi INTERLABEL(Interpolate_Y_V_4x4)
	(-f0.0) jmpi INTERLABEL(Average_4x4)
	
	jmpi INTERLABEL(Return_Interpolate_Y_4x4)

INTERLABEL(Interpolate_Y_V_4x4):

	cmp.e.f0.0 (1) null				gMVY_FRAC:w						0:w
	(f0.0) jmpi INTERLABEL(Interpolate_Y_I_4x4)
	
	//-----------------------------------------------------------------------
	// CASE: 48C59D7BF (V interpolation)
	//-----------------------------------------------------------------------
	
	cmp.g.f0.1 (8) null:w			gMVX_FRAC<0;1,0>:w				2:w

	mov (4)		pREF0<1>:uw			gW0<4;4,1>:uw							{NoDDClr}
	add (4)		pREF4<1>:w			gW0<4;4,1>:uw					16:w	{NoDDChk}
	
	(f0.1) add (8) pREF0<1>:uw		pREF0<4;4,1>:uw					1:uw

	cmp.e.f0.0 (1) null:w			gMVX_FRAC<0;1,0>:w				0:w
	cmp.e.f0.1 (1) null				gMVY_FRAC:w						2:w

	// Compute interim/output vertical interpolation
	add (16)	acc0<1>:w			r[pREF0,-nGRFWIB]<4,1>:ub		16:w
	mac (16)	acc0<1>:w			r[pREF0]<4,1>:ub				20:w
	mac (16)	acc0<1>:w			r[pREF0,nGRFWIB]<4,1>:ub		-5:w
	mac (16)	acc0<1>:w			r[pREF4,-nGRFWIB]<4,1>:ub		-5:w
	mac (16)	acc0<1>:w			r[pREF4]<4,1>:ub				20:w	
	mac (16)	acc0<1>:w			r[pREF4,nGRFWIB]<4,1>:ub		1:w
	
	mov (1)		pRESULT:uw			nOFFSET_RES:uw
	(-f0.0) jmpi INTERLABEL(VFILTER_4x4)
	(-f0.1) mov (1) pRESULT:uw		nOFFSET_INTERIM4x4:uw
 
 INTERLABEL(VFILTER_4x4):
 
	asr.sat (16) r[pRESULT]<2>:ub	acc0<16;16,1>:w					5:w
	
	(-f0.0) jmpi INTERLABEL(Average_4x4)
	(f0.1) jmpi INTERLABEL(Return_Interpolate_Y_4x4	)

INTERLABEL(Interpolate_Y_I_4x4):

	//-----------------------------------------------------------------------
	// CASE: 134C (Integer position)
	//-----------------------------------------------------------------------
	
	mov (4)		pREF0<1>:uw			gW0<4;4,1>:uw
		
	cmp.e.f0.0 (4) null:w			gMVX_FRAC<0;1,0>:w				3:w
	cmp.e.f0.1 (4) null:w			gMVY_FRAC<0;1,0>:w				3:w
	(f0.0) add (4) pREF0<1>:uw		pREF0<4;4,1>:uw					1:uw 
	(f0.1) add (4) pREF0<1>:uw		pREF0<4;4,1>:uw					nGRFWIB/2:uw
	
	mov (16)	guwINTERIM_BUF2(0)<1>	r[pREF0]<4,1>:ub
	
INTERLABEL(Average_4x4):

	//-----------------------------------------------------------------------
	// CASE: 13456789BCDEF (Average)
	//-----------------------------------------------------------------------
	
	// Average two interim results
	avg.sat (16) gubINTERIM_BUF2(0)<2>	gubINTERIM_BUF2(0)<32;16,2>		gINTERIM4x4<32;16,2>:ub

INTERLABEL(Return_Interpolate_Y_4x4):
	// Move result
	mov (1)		pRES:uw				gpINTPY:uw
	mov (4)		r[pRES,0]<2>:ub		gubINTERIM_BUF2(0,0)
	mov (4)		r[pRES,16]<2>:ub	gubINTERIM_BUF2(0,8)
	mov (4)		r[pRES,32]<2>:ub	gubINTERIM_BUF2(0,16)
	mov (4)		r[pRES,48]<2>:ub	gubINTERIM_BUF2(0,24)

	// Restore all address registers
	mov (8)		a0<1>:w					gpADDR<8;8,1>:w
	
INTERLABEL(Exit_Interpolate_Y_4x4):
	
        
// end of file
