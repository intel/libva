/*
 * Load reference 6x3 area for chroma NV12 4x4 MC
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: LoadRef_C_6x3.asm
//
// Load reference 6x3 area for chroma NV12 4x4 MC


//#if !defined(__LOADREF_C_6x3__)		// Make sure this is only included once
//#define __LOADREF_C_6x3__


#ifdef DEV_ILK
	add (1)		pMSGDSC:ud			gMSGDSC_R:ud					0x00100010:ud
#else
	add (1)		pMSGDSC:ud			gMSGDSC_R:ud					0x00010010:ud
#endif	// DEV_ILK

	// Compute integer and fractional components of MV
    asr (2)		gMVX_INT<1>:w		r[pMV,0]<2;2,1>:w				0x03:w {NoDDClr}
    and (2)		gMVX_FRACC<1>:w		r[pMV,0]<2;2,1>:w				0x07:w {NoDDChk}
	
	// Compute top-left corner position to be loaded
    mov (2)		gMSGSRC.0<1>:d		gMVX_INT<2;2,1>:w
	shl (1)		gMSGSRC.0:d			gMSGSRC.0:d						1:w

    // Read 8x3 pixels
    mov (1)		gMSGSRC.2:ud		0x00020005:ud
    send (8)	gudREFC(0)<1>	    mMSGHDRC						gMSGSRC<8;8,1>:ud	DAPREAD	pMSGDSC:ud

        
//#endif	// !defined(__LOADREF_C_6x3__)
