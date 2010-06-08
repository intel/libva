/*
 * Interpolation kernel for chrominance 2x2 motion compensation
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//	Kernel name: Interpolate_C_2x2.asm
//
//	Interpolation kernel for chrominance 2x2 motion compensation
//
//  $Revision: 8 $
//  $Date: 10/09/06 4:00p $
//


//#if !defined(__Interpolate_C_2x2__)		// Make sure this is only included once
//#define __Interpolate_C_2x2__

	
	// (8-xFrac) and (8-yFrac)
    add (2)		gW0<1>:w			gMVX_FRACC<2;2,1>:w				-0x08:w
    
	// Compute the GRF address of the starting position of the reference area
    mov (1)		pREF0:w				nOFFSET_REFC:w		{NoDDClr}
	mov (1)		pRESULT:uw			gpINTPC:uw			{NoDDChk}

	// gCOEFA = (8-xFrac)*(8-yFrac)
    // gCOEFB = xFrac*(8-yFrac)  
    // gCOEFC = (8-xFrac)*yFrac
    // gCOEFD = xFrac*yFrac 
    mul (1)		gCOEFD:w	        gMVX_FRACC:w					gMVY_FRACC:w	{NoDDClr}
    mul (1)		gCOEFA:w			-gW0:w							-gW1:uw		{NoDDClr,NoDDChk}
    mul (1)		gCOEFB:w			gMVX_FRACC:w					-gW1:uw		{NoDDClr,NoDDChk}
    mul (1)		gCOEFC:w		    -gW0:w							gMVY_FRACC:w {NoDDChk}
    
    // (8-xFrac)*(8-yFrac)*A
    // ---------------------
    mul (8)		acc0<1>:uw			r[pREF0,0]<8;4,1>:ub			gCOEFA:uw
        
    // xFrac*(8-yFrac)*B
    // -------------------
    mac (8)		acc0<1>:uw			r[pREF0,2]<8;4,1>:ub			gCOEFB:uw
          
    // (8-xFrac)*yFrac*C
    // -------------------
    mac (8)		acc0<1>:uw			r[pREF0,8]<8;4,1>:ub			gCOEFC:uw
            
    // xFrac*yFrac*D
    // -----------------
    mac (8)		gwINTERIM_BUF2(0)<1>	r[pREF0,10]<8;4,1>:ub		gCOEFD:uw
    mov (4)		r[pRESULT]<1>:uw		gwINTERIM_BUF2(0)<4;4,1>		{NoDDClr}
	mov (4)		r[pRESULT,16]<1>:uw		gwINTERIM_BUF2(0,4)<4;4,1>		{NoDDChk}
    
//#endif	// !defined(__Interpolate_C_2x2__)
