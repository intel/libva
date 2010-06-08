/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: WriteRecon_C_8x4.asm
//
//  $Revision: 10 $
//  $Date: 10/03/06 5:28p $
//


//#if !defined(__WRITERECON_C_8x4__)		// Make sure this is only included once
//#define __WRITERECON_C_8x4__


	// TODO: Why did I use p0?
#ifndef MONO
    add (1)		p0:w					pERRORC:w				-16:w
	mov (16)	mbMSGPAYLOADC(0,0)<2>	r[p0,0]<32;16,2>:ub				{NoDDClr}
	mov (16)	mbMSGPAYLOADC(0,1)<2>	r[p0,128]<32;16,2>:ub			{NoDDChk}
	mov (16)	mbMSGPAYLOADC(1,0)<2>	r[p0,32]<32;16,2>:ub			{NoDDClr}
	mov (16)	mbMSGPAYLOADC(1,1)<2>	r[p0,128+32]<32;16,2>:ub		{NoDDChk}
#else	// defined(MONO)
	mov (16)	mbMSGPAYLOADC(0)<1>		0x80808080:ud {Compr}
#endif	// !defined(MONO)

 #if defined(MBAFF)
	add (1)		pMSGDSC:ud				gFIELDFLAGS:uw			MSG_LEN(2)+nDWBWMSGDSC+nBDIX_DESTC+ENWRCOM:ud
 #elif defined(FIELD)
	add (1)		pMSGDSC:ud				gFIELDFLAGS:uw			MSG_LEN(2)+nDWBWMSGDSC_TF+nBDIX_DESTC+ENWRCOM:ud
 #endif

    asr (1)		gMSGSRC.1:d				gMSGSRC.1:d					1:w	{NoDDClr}
    mov (1)		gMSGSRC.2:ud			0x0003000f:ud					{NoDDChk} // NV12 (16x4)

#if defined(FRAME)
    send (8)	gREG_WRITE_COMMIT_UV<1>:ud		mMSGHDRCW				gMSGSRC<8;8,1>:ud		DAPWRITE	MSG_LEN(2)+nDWBWMSGDSC+nBDIX_DESTC+ENWRCOM
#else
    send (8)	gREG_WRITE_COMMIT_UV<1>:ud		mMSGHDRCW				gMSGSRC<8;8,1>:ud		DAPWRITE	pMSGDSC:ud
#endif	// defined(FRAME)

//#endif	// !defined(__WRITERECON_C_8x4__)
