/*
 * Load reference 16x9 area for luma 4x4 MC
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: LoadRef_Y_16x9.asm
//
// Load reference 16x9 area for luma 4x4 MC


//#if !defined(__LOADREF_Y_16x9__)		// Make sure this is only included once
//#define __LOADREF_Y_16x9__

#if 1

	// Compute integer and fractional components of MV
    and (2)		gMVX_FRAC<1>:w		r[pMV,0]<2;2,1>:w				0x03:w	//{NoDDClr}
    asr (2)		gMVX_INT<1>:w		r[pMV,0]<2;2,1>:w				0x02:w	//{NoDDChk}
 
    // Check whether MVY is integer
	or.z.f0.1 (8) null:w			gMVY_FRAC<0;1,0>:w				0:w
	   
	// Set message descriptor
	(f0.1) add (1)	pMSGDSC:ud		gMSGDSC_R:ud					RESP_LEN(2):ud
	(-f0.1) add (1)	pMSGDSC:ud		gMSGDSC_R:ud					RESP_LEN(5):ud

	// Compute top-left corner position to be loaded
	// TODO: sel
    (-f0.1) add (2)	gMSGSRC.0<1>:d	gMVX_INT<2;2,1>:w				-0x02:d	//{NoDDClr}
    (-f0.1) mov (1)	gMSGSRC.2:ud	0x00080008:ud							//{NoDDChk}
    (f0.1) add (1)	gMSGSRC.0<1>:d	gMVX_INT<0;1,0>:w				-0x02:d //{NoDDClr}
	(f0.1) mov (1)	gMSGSRC.1<1>:d	gMVY_INT<0;1,0>:w						//{NoDDChk,NoDDClr}
    (f0.1) mov (1)	gMSGSRC.2:ud	0x00030008:ud							//{NoDDChk}

    // Read 16x9 pixels
    send (8)	gudREF(0)<1>	    mMSGHDRY						gMSGSRC<8;8,1>:ud	DAPREAD pMSGDSC:ud

#else

	// Compute integer and fractional components of MV
    and (2)		gMVX_FRAC<1>:w		r[pMV,0]<2;2,1>:w				0x03:w {NoDDClr} //
    asr (2)		gMVX_INT<1>:w		r[pMV,0]<2;2,1>:w				0x02:w {NoDDChk} //

	// Set message descriptor
	add (1)		pMSGDSC:ud			gMSGDSC_R:ud					RESP_LEN(5):ud
    
	// Compute top-left corner position to be loaded 
    add (2)		gMSGSRC.0<1>:d		gMVX_INT<2;2,1>:w				-0x02:d	{NoDDClr} //
    mov (1)		gMSGSRC.2:ud		0x00080008:ud							{NoDDChk} //

    // Read 16x9 pixels
    send (8)	gudREF(0)<1>	    mMSGHDRY						gMSGSRC<8;8,1>:ud	DAPREAD	pMSGDSC:ud

#endif

        
//#endif	// !defined(__LOADREF_Y_16x9__)
