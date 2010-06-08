/*
 * Load reference 10x5 area for chroma NV12 4x4 MC
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: LoadRef_C_10x5.asm
//
// Load reference 10x5 area for chroma NV12 4x4 MC


//#if !defined(__LOADREF_C_10x5__)		// Make sure this is only included once
//#define __LOADREF_C_10x5__


#if 1

	// Compute integer and fractional components of MV
    asr (2)		gMVX_INT<1>:w		r[pMV,0]<2;2,1>:w				0x03:w {NoDDClr}
    and (2)		gMVX_FRACC<1>:w		r[pMV,0]<2;2,1>:w				0x07:w {NoDDChk}
    
    // Check whether MVY is integer
	or.z.f0.0 (8) null:w			gMVY_FRACC<0;1,0>:w				0:w
	
	// Compute top-left corner position to be loaded
    mov (2)		gMSGSRC.0<1>:d		gMVX_INT<2;2,1>:w
	shl (1)		gMSGSRC.0:d			gMSGSRC.0:d						1:w

	(f0.0) add (1)	pMSGDSC:ud		gMSGDSC_R:ud					RESP_LEN(2)+nBI_LC_DIFF:ud
	(-f0.0) add (1) pMSGDSC:ud		gMSGDSC_R:ud					RESP_LEN(3)+nBI_LC_DIFF:ud
	
    // Read 16x5 pixels - TODO: Reading 12x5 instead of 16x5 took more time on CL. Why?
    (f0.0) mov (1)	gMSGSRC.2:ud	0x00030009:ud					//{NoDDChk}
    (-f0.0) mov (1)	gMSGSRC.2:ud	0x00040009:ud					//{NoDDChk}
    send (8)	gudREFC(0)<1>	    mMSGHDRC						gMSGSRC<8;8,1>:ud	DAPREAD	pMSGDSC:ud

#else

	add (1)		pMSGDSC:ud			gMSGDSC_R:ud					RESP_LEN(3)+nBI_LC_DIFF:ud

	// Compute integer and fractional components of MV
    asr (2)		gMVX_INT<1>:w		r[pMV,0]<2;2,1>:w				0x03:w {NoDDClr}
    and (2)		gMVX_FRACC<1>:w		r[pMV,0]<2;2,1>:w				0x07:w {NoDDChk}
	
	// Compute top-left corner position to be loaded
    mov (2)		gMSGSRC.0<1>:d		gMVX_INT<2;2,1>:w
	shl (1)		gMSGSRC.0:d			gMSGSRC.0:d						1:w

    // Read 16x5 pixels
    mov (1)		gMSGSRC.2:ud		0x00040009:ud					{NoDDChk}
    send (8)	gudREFC(0)<1>	    mMSGHDRC						gMSGSRC<8;8,1>:ud	DAPREAD	pMSGDSC:ud
#endif
        
//#endif	// !defined(__LOADREF_C_10x5__)
