/*
 * Interpolation kernel for chrominance 4x4 motion compensation
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//	Kernel name: Interpolate_C_4x4_Func.asm
//
//	Interpolation kernel for chrominance 4x4 motion compensation
//
//  $Revision: 8 $
//  $Date: 10/09/06 4:00p $
//


//#if !defined(__Interpolate_C_4x4_Func__)		// Make sure this is only included once
//#define __Interpolate_C_4x4_Func__


INTERLABEL(Interpolate_C_4x4_Func):


	// (8-xFrac) and (8-yFrac)
    add (2)		gW0<1>:w			gMVX_FRACC<2;2,1>:w				-0x08:w

	// Compute the GRF address of the starting position of the reference area
    mov (1)		pREF0:w				nOFFSET_REFC:w		{NoDDClr}
	mov (1)		pREF1:uw			nOFFSET_REFC+16:w	{NoDDChk,NoDDClr}
	mov (1)		pRESULT:uw			gpINTPC:uw			{NoDDChk}

	// gCOEFA = (8-xFrac)*(8-yFrac)
    // gCOEFB = xFrac*(8-yFrac)  
    // gCOEFC = (8-xFrac)*yFrac
    // gCOEFD = xFrac*yFrac 
    mul (1)		gCOEFD:w	        gMVX_FRACC:w					gMVY_FRACC:w	{NoDDClr}
    mul (1)		gCOEFA:w			-gW0:w							-gW1:uw		{NoDDClr,NoDDChk}
    mul (1)		gCOEFB:w			gMVX_FRACC:w					-gW1:uw		{NoDDClr,NoDDChk}
    mul (1)		gCOEFC:w		    -gW0:w							gMVY_FRACC:w {NoDDChk}

	add (2)		gW0<1>:uw			pREF0<2;2,1>:uw					16:w

    // (8-xFrac)*(8-yFrac)*A
    // ---------------------
    mul (16)	acc0<1>:uw			r[pREF0,0]<16;8,1>:ub			gCOEFA:uw
    mul (16)	acc1<1>:uw			r[pREF0,nGRFWIB]<16;8,1>:ub		gCOEFA:uw
        
    // xFrac*(8-yFrac)*B
    // -------------------
    mac (16)	acc0<1>:uw          r[pREF0,2]<16;8,1>:ub			gCOEFB:uw
    mac (16)	acc1<1>:uw          r[pREF0,nGRFWIB+2]<16;8,1>:ub	gCOEFB:uw

    // (8-xFrac)*yFrac*C
    // -------------------
    mov (2)		pREF0<1>:uw			gW0<2;2,1>:uw
    mac (16)	acc0<1>:uw          r[pREF0,0]<8,1>:ub				gCOEFC:uw
    mac (16)	acc1<1>:uw          r[pREF0,nGRFWIB]<8,1>:ub		gCOEFC:uw
            
    // xFrac*yFrac*D
    // -----------------
    mac (16)	r[pRESULT]<1>:uw	r[pREF0,2]<8,1>:ub				gCOEFD:uw
    mac (16)	r[pRESULT,GRFWIB]<1>:uw r[pREF0,nGRFWIB+2]<8,1>:ub gCOEFD:uw {SecHalf}

   
//#endif	// !defined(__Interpolate_C_4x4_Func__)
