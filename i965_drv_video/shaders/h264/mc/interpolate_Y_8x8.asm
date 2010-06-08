/*
 * Interpolation kernel for luminance motion compensation
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: Interpolate_Y_8x8.asm
//
// Interpolation kernel for luminance motion compensation
//
//  $Revision: 13 $
//  $Date: 10/09/06 4:00p $
//


//---------------------------------------------------------------
// In: pMV => Source address of MV
// In: gMVX_FRAC<2;2,1>:w => MV fractional components
// In: f0.1 (1) => If 1, vertical MV is integer
// In: gpINTPY:uw => Destination address for interpolated result
// In: Reference area staring from R43
//		If horizontal/vertical MVs are all integer, 8x8 pixels are on R43~R44 (2 GRFs)
//		If only horz MV is integer, 8x13 pixels are on R43~R46 (4 GRFs)
//		If only vert MV is integer, 13x8 pixels are on R43~R46 (4 GRFs)
//		If no MVs are integer, 13x13 pixels are on R43~R49 (7 GRFs)
//---------------------------------------------------------------


INTERLABEL(Interpolate_Y_8x8_Func):



	// Check whether MVX is integer MV
	and.z.f0.0 (1) null:w			r[pMV,0]<0;1,0>:w				0x3:w
	(-f0.0)	jmpi (1) INTERLABEL(Interpolate_Y_8x8_Func2)
		
	// TODO: remove this back-to-back read - huge latency..
	mov (8)	gubREF(6,2)<1>	gubREF(3,0)<8;8,1>
    mov (8)	gubREF(5,18)<1>	gubREF(2,24)<8;8,1>		{NoDDClr}
	mov (8)	gubREF(5,2)<1>	gubREF(2,16)<8;8,1>		{NoDDChk}
	mov (8)	gubREF(4,18)<1>	gubREF(2,8)<8;8,1>		{NoDDClr}
	mov (8)	gubREF(4,2)<1>	gubREF(2,0)<8;8,1>		{NoDDChk}
	mov (8)	gubREF(3,18)<1>	gubREF(1,24)<8;8,1>		{NoDDClr}
	mov (8)	gubREF(3,2)<1>	gubREF(1,16)<8;8,1>		{NoDDChk}
	mov (8)	gubREF(2,18)<1>	gubREF(1,8)<8;8,1>		{NoDDClr}
	mov (8)	gubREF(2,2)<1>	gubREF(1,0)<8;8,1>		{NoDDChk}
	mov (8)	gubREF(1,18)<1>	gubREF(0,24)<8;8,1>		{NoDDClr}
	mov (8)	gubREF(1,2)<1>	gubREF(0,16)<8;8,1>		{NoDDChk}
	mov (8)	gubREF(0,18)<1>	gubREF(0,8)<8;8,1>	
    mov (8)	gubREF(0,2)<1>	gubREF(0,0)<8;8,1>

INTERLABEL(Interpolate_Y_8x8_Func2):

	// Compute the GRF address of the starting position of the reference area
    (-f0.1) mov (1)	pREF:w			nOFFSET_REF+2+nGRFWIB:w	
    (f0.1) mov (1)	pREF:w			nOFFSET_REF+2:w			
	mov (1)		pRESULT:uw			gpINTPY:uw	
	
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
	(f0.1) mov (16)	r[pRESULT]<1>:uw				r[pREF]<16;8,1>:ub
	(f0.1) mov (16)	r[pRESULT,nGRFWIB]<1>:uw		r[pREF,nGRFWIB]<16;8,1>:ub
	(f0.1) mov (16)	r[pRESULT,nGRFWIB*2]<1>:uw		r[pREF,nGRFWIB*2]<16;8,1>:ub
	(f0.1) mov (16)	r[pRESULT,nGRFWIB*3]<1>:uw		r[pREF,nGRFWIB*3]<16;8,1>:ub
	(f0.1) jmpi INTERLABEL(Exit_Interpolate_Y_8x8)
	
	// Store all address registers
	mov (8)		gpADDR<1>:w			a0<8;8,1>:w
	
	mul.z.f0.0 (1) gW4:w			gMVY_FRAC:w						gMVX_FRAC:w
	add (1)		pREF1:uw			pREF0:uw						nGRFWIB/2:uw
	and.nz.f0.1 (1) null			gW4:w							1:w
	add (2)		pREF2<1>:uw			pREF0<2;2,1>:uw					nGRFWIB:uw
	mov (4)		gW0<1>:uw			pREF0<4;4,1>:uw

	(f0.0) jmpi INTERLABEL(Interpolate_Y_H_8x8)
	(f0.1) jmpi INTERLABEL(Interpolate_Y_H_8x8)
	
	//-----------------------------------------------------------------------
	// CASE: A69BE (H/V interpolation)
	//-----------------------------------------------------------------------
	
	// Compute interim horizontal intepolation of 12 lines (not 9 lines)
//	add (1)		pREF0<1>:ud			pREF0<0;1,0>:ud					0xffeeffde:ud	// (-18<<16)|(-34)
	add (1)		pREF0<1>:uw			pREF0<0;1,0>:uw					-34:w	
	add (1)		pREF1<1>:uw			pREF1<0;1,0>:uw					-18:w {NoDDClr}	
	mov (1)		pRESD:ud			nOFFSET_INTERIM3:ud					{NoDDChk}			
	
	// Check whether this position is 'A'    
	cmp.e.f0.0 (1) null				gW4:w							4:w
	
	$for(0;<6;2) {
	add (32)	acc0<1>:w			r[pREF,nGRFWIB*%1]<16;8,1>:ub			r[pREF0,nGRFWIB*%1+5]<16;8,1>:ub		{Compr}
	mac (32)	acc0<1>:w			r[pREF,nGRFWIB*%1+1]<16;8,1>:ub			-5:w	{Compr}
	mac (32)	acc0<1>:w			r[pREF,nGRFWIB*%1+2]<16;8,1>:ub			20:w	{Compr}
	mac (32)	acc0<1>:w			r[pREF,nGRFWIB*%1+3]<16;8,1>:ub			20:w	{Compr}
	mac (32)	r[pRES,nGRFWIB*%1]<1>:w		r[pREF,nGRFWIB*%1+4]<16;8,1>:ub	-5:w	{Compr}
	}
	// last line
	add (8)		acc0<1>:w			r[pREF,nGRFWIB*6]<8;8,1>:ub				r[pREF,nGRFWIB*6+5]<8;8,1>:ub
	mac (8)		acc0<1>:w			r[pREF,nGRFWIB*6+1]<8;8,1>:ub			-5:w
	mac (8)		acc0<1>:w			r[pREF,nGRFWIB*6+2]<8;8,1>:ub			20:w
	mac (8)		acc0<1>:w			r[pREF,nGRFWIB*6+3]<8;8,1>:ub			20:w
	mac (8)		r[pRES,nGRFWIB*6]<1>:w		r[pREF,nGRFWIB*6+4]<8;8,1>:ub	-5:w

    // Compute interim/output vertical interpolation 
    mov (1)		pREF0:ud			nOFFSET_INTERIM2:ud	{NoDDClr}			// set pREF0 and pREF1 at the same time
	mov (1)		pREF2D:ud			nOFFSET_INTERIM4:ud	{NoDDChk,NoDDClr}	// set pREF2 and pREF3 at the same time
	(f0.0) sel (1) pRES:uw			gpINTPY:uw	nOFFSET_INTERIM:uw {NoDDChk} // Case A vs. 69BE
    
	$for(0;<4;2) {
	add (32)	acc0<1>:w			r[pREF0,nGRFWIB*%1]<16;16,1>:w				512:w	{Compr}
	mac (16)	acc0<1>:w			r[pREF2,nGRFWIB*%1]<8,1>:w					-5:w
	mac (16)	acc1<1>:w			r[pREF2,nGRFWIB*%1+nGRFWIB]<8,1>:w			-5:w
	mac (32)	acc0<1>:w			r[pREF0,nGRFWIB*%1+nGRFWIB]<16;16,1>:w		20:w	{Compr}
	mac (16)	acc0<1>:w			r[pREF2,nGRFWIB*%1+nGRFWIB]<8,1>:w			20:w	
	mac (16)	acc1<1>:w			r[pREF2,nGRFWIB*%1+nGRFWIB+nGRFWIB]<8,1>:w	20:w	
	mac (32)	acc0<1>:w			r[pREF0,(2+%1)*nGRFWIB]<16;16,1>:w			-5:w	{Compr}
	mac (16)	acc0<1>:w			r[pREF2,(2+%1)*nGRFWIB]<8,1>:w				1:w
	mac (16)	acc1<1>:w			r[pREF2,(2+%1)*nGRFWIB+nGRFWIB]<8,1>:w		1:w
	asr.sat (16) r[pRES,nGRFWIB*%1]<2>:ub			acc0<16;16,1>:w				10:w
	asr.sat (16) r[pRES,nGRFWIB*%1+nGRFWIB]<2>:ub	acc1<16;16,1>:w				10:w {SecHalf}
	}
	
	(f0.0) jmpi INTERLABEL(Return_Interpolate_Y_8x8)
	
INTERLABEL(Interpolate_Y_H_8x8):
	
	cmp.e.f0.0 (1) null				gMVX_FRAC:w						0:w
	cmp.e.f0.1 (1) null				gMVY_FRAC:w						2:w
	(f0.0) jmpi INTERLABEL(Interpolate_Y_V_8x8)
	(f0.1) jmpi INTERLABEL(Interpolate_Y_V_8x8)
	
	//-----------------------------------------------------------------------
	// CASE: 123567DEF (H interpolation)
	//-----------------------------------------------------------------------

	add (4)		pREF0<1>:uw			gW0<4;4,1>:uw					-2:w		
	cmp.g.f0.0 (4) null:w			gMVY_FRAC<0;1,0>:w				2:w
	cmp.e.f0.1 (1) null				gMVX_FRAC:w						2:w
	(f0.0) add (4) pREF0<1>:uw		pREF0<4;4,1>:uw					nGRFWIB/2:uw

	cmp.e.f0.0 (1) null:w			gMVY_FRAC<0;1,0>:w				0:w

	(f0.1) sel (1) pRES:uw			gpINTPY:uw						nOFFSET_INTERIM:uw // Case 26E vs. 1357DF
	
	// Compute interim/output horizontal interpolation
	$for(0;<4;2) {
	add (16)	acc0<1>:w			r[pREF0,nGRFWIB*%1]<8,1>:ub				16:w
	add (16)	acc1<1>:w			r[pREF0,nGRFWIB*%1+nGRFWIB]<8,1>:ub		16:w
	mac (16)	acc0<1>:w			r[pREF0,nGRFWIB*%1+1]<8,1>:ub			-5:w
	mac (16)	acc1<1>:w			r[pREF0,nGRFWIB*%1+1+nGRFWIB]<8,1>:ub	-5:w
	mac (16)	acc0<1>:w			r[pREF0,nGRFWIB*%1+2]<8,1>:ub			20:w
	mac (16)	acc1<1>:w			r[pREF0,nGRFWIB*%1+2+nGRFWIB]<8,1>:ub	20:w
	mac (16)	acc0<1>:w			r[pREF0,nGRFWIB*%1+3]<8,1>:ub			20:w
	mac (16)	acc1<1>:w			r[pREF0,nGRFWIB*%1+3+nGRFWIB]<8,1>:ub	20:w
	mac (16)	acc0<1>:w			r[pREF0,nGRFWIB*%1+4]<8,1>:ub			-5:w
	mac (16)	acc1<1>:w			r[pREF0,nGRFWIB*%1+4+nGRFWIB]<8,1>:ub	-5:w
	mac (16)	acc0<1>:w			r[pREF0,nGRFWIB*%1+5]<8,1>:ub			1:w
	mac (16)	acc1<1>:w			r[pREF0,nGRFWIB*%1+5+nGRFWIB]<8,1>:ub	1:w
	asr.sat (16) r[pRES,nGRFWIB*%1]<2>:ub			acc0<16;16,1>:w		5:w
	asr.sat (16) r[pRES,nGRFWIB*%1+nGRFWIB]<2>:ub	acc1<16;16,1>:w		5:w {SecHalf}
    }
    
    (-f0.1) jmpi INTERLABEL(Interpolate_Y_V_8x8)
	(-f0.0) jmpi INTERLABEL(Average_8x8)
	
	jmpi INTERLABEL(Return_Interpolate_Y_8x8)

INTERLABEL(Interpolate_Y_V_8x8):

	cmp.e.f0.0 (1) null				gMVY_FRAC:w						0:w
	(f0.0) jmpi INTERLABEL(Interpolate_Y_I_8x8)
	
	//-----------------------------------------------------------------------
	// CASE: 48C59D7BF (V interpolation)
	//-----------------------------------------------------------------------

	mov (2)		pREF0<1>:uw			gW0<4;2,2>:uw	{NoDDClr}
	mov (2)		pREF2<1>:uw			gW1<2;2,1>:uw	{NoDDChk,NoDDClr}
	mov (1)		pRES:uw				gpINTPY:uw		{NoDDChk}

	cmp.g.f0.1 (4) null:w			gMVX_FRAC<0;1,0>:w				2:w
	cmp.e.f0.0 (1) null:w			gMVX_FRAC<0;1,0>:w				0:w
	(f0.1) add (4) pREF0<1>:uw		pREF0<4;4,1>:uw					1:uw

	cmp.e.f0.1 (1) null				gMVY_FRAC:w						2:w
	(-f0.0) jmpi INTERLABEL(VFILTER_8x8)
	(-f0.1) mov (1) pRES:uw		nOFFSET_INTERIM:uw
	
  INTERLABEL(VFILTER_8x8): 

	// Compute interim/output vertical interpolation
	$for(0;<4;2) {
	add (32)	acc0<1>:w			r[pREF0,nGRFWIB*%1-nGRFWIB]<16;8,1>:ub			16:w {Compr}
	mac (16)	acc0<1>:w			r[pREF2,nGRFWIB*%1-nGRFWIB]<8,1>:ub				-5:w
	mac (16)	acc1<1>:w			r[pREF2,nGRFWIB*%1]<8,1>:ub						-5:w
	mac (32)	acc0<1>:w			r[pREF0,nGRFWIB*%1]<16;8,1>:ub					20:w {Compr}
	mac (16)	acc0<1>:w			r[pREF2,nGRFWIB*%1]<8,1>:ub						20:w	
	mac (16)	acc1<1>:w			r[pREF2,nGRFWIB*%1+nGRFWIB]<8,1>:ub				20:w	
	mac (32)	acc0<1>:w			r[pREF0,nGRFWIB*%1+nGRFWIB]<16;8,1>:ub			-5:w {Compr}
	mac (16)	acc0<1>:w			r[pREF2,nGRFWIB*%1+nGRFWIB]<8,1>:ub				1:w
	mac (16)	acc1<1>:w			r[pREF2,nGRFWIB*%1+nGRFWIB+nGRFWIB]<8,1>:ub		1:w
	asr.sat (16) r[pRES,nGRFWIB*%1]<2>:ub			acc0<16;16,1>:w					5:w
	asr.sat (16) r[pRES,nGRFWIB*%1+nGRFWIB]<2>:ub	acc1<16;16,1>:w					5:w	{SecHalf}
	}

	(-f0.0) jmpi INTERLABEL(Average_8x8)
	(f0.1) jmpi INTERLABEL(Return_Interpolate_Y_8x8)

INTERLABEL(Interpolate_Y_I_8x8):

	//-----------------------------------------------------------------------
	// CASE: 134C (Integer position)
	//-----------------------------------------------------------------------
	
	mov (2)		pREF0<1>:uw			gW0<2;2,1>:uw		{NoDDClr}
			
	mov (1)		pRES:uw				gpINTPY:uw			{NoDDChk}

	cmp.e.f0.0 (2) null:w			gMVX_FRAC<0;1,0>:w				3:w
	cmp.e.f0.1 (2) null:w			gMVY_FRAC<0;1,0>:w				3:w
	(f0.0) add (2) pREF0<1>:uw		pREF0<2;2,1>:uw					1:uw 
	(f0.1) add (2) pREF0<1>:uw		pREF0<2;2,1>:uw					nGRFWIB/2:uw
	
	mov (16)	r[pRES]<1>:uw			r[pREF0]<8,1>:ub
	mov (16)	r[pRES,nGRFWIB]<1>:uw	r[pREF0,nGRFWIB]<8,1>:ub
	mov (16)	r[pRES,nGRFWIB*2]<1>:uw	r[pREF0,nGRFWIB*2]<8,1>:ub
	mov (16)	r[pRES,nGRFWIB*3]<1>:uw	r[pREF0,nGRFWIB*3]<8,1>:ub
	
INTERLABEL(Average_8x8):

	//-----------------------------------------------------------------------
	// CASE: 13456789BCDEF (Average)
	//-----------------------------------------------------------------------

	// Average two interim results
	avg.sat (16) r[pRES,0]<2>:ub			r[pRES,0]<32;16,2>:ub			gubINTERIM_BUF(0)	
	avg.sat (16) r[pRES,nGRFWIB]<2>:ub		r[pRES,nGRFWIB]<32;16,2>:ub		gubINTERIM_BUF(1)	
	avg.sat (16) r[pRES,nGRFWIB*2]<2>:ub	r[pRES,nGRFWIB*2]<32;16,2>:ub	gubINTERIM_BUF(2)	
	avg.sat (16) r[pRES,nGRFWIB*3]<2>:ub	r[pRES,nGRFWIB*3]<32;16,2>:ub	gubINTERIM_BUF(3)	

INTERLABEL(Return_Interpolate_Y_8x8):
	// Restore all address registers
	mov (8)		a0<1>:w					gpADDR<8;8,1>:w
	
INTERLABEL(Exit_Interpolate_Y_8x8):
	        
// end of file
