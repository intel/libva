/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: WriteRecon_YC.asm
//
//  $Revision: 10 $
//  $Date: 10/03/06 5:28p $
//


//#if !defined(__WRITERECON_YC__)		// Make sure this is only included once
//#define __WRITERECON_YC__

    // TODO: Merge two inst to one.
    mov (1)		p0:w					nOFFSET_ERRORY:w
    mov (1)		p1:w					nOFFSET_ERRORY+128:w
    
	$for(0; <4; 1) {
    mov (16)	mbMSGPAYLOADY(%1,0)<1>	r[p0,%1*32+0]<8,2>:ub		{NoDDClr}
    mov (16)	mbMSGPAYLOADY(%1,16)<1>	r[p0,%1*32+16]<8,2>:ub		{NoDDChk}
    }    
	$for(0; <4; 1) {
    mov (16)	mbMSGPAYLOADY(%1+4,0)<1>	r[p0,%1*32+256]<8,2>:ub			{NoDDClr}
    mov (16)	mbMSGPAYLOADY(%1+4,16)<1>	r[p0,%1*32+16+256]<8,2>:ub		{NoDDChk}
    }    
    
 
 #if defined(MBAFF)
	add (1)		pMSGDSC:ud				gFIELDFLAGS:uw			MSG_LEN(8)+nDWBWMSGDSC+nBDIX_DESTY+ENWRCOM:ud
 #elif defined(FIELD)
	add (1)		pMSGDSC:ud				gFIELDFLAGS:uw			MSG_LEN(8)+nDWBWMSGDSC_TF+nBDIX_DESTY+ENWRCOM:ud
 #endif

    mov	(2)		gMSGSRC.0<1>:d			gX<2;2,1>:w		{NoDDClr}
    mov (1)		gMSGSRC.2:ud			0x000f000f:ud	{NoDDChk}
    
#if defined(FRAME)
    send (8)	gREG_WRITE_COMMIT_Y<1>:ud		mMSGHDRYW				gMSGSRC<8;8,1>:ud		DAPWRITE	MSG_LEN(8)+nDWBWMSGDSC+nBDIX_DESTY+ENWRCOM
#else
    send (8)	gREG_WRITE_COMMIT_Y<1>:ud		mMSGHDRYW				gMSGSRC<8;8,1>:ud		DAPWRITE	pMSGDSC:ud
#endif

#ifndef MONO
	// TODO: Why did I use p0?
    mov (1)		p0:w					nOFFSET_ERRORC:w
	mov (16)	mbMSGPAYLOADC(0,0)<2>	r[p0,0]<32;16,2>:ub				{NoDDClr}
	mov (16)	mbMSGPAYLOADC(0,1)<2>	r[p0,128]<32;16,2>:ub			{NoDDChk}
	mov (16)	mbMSGPAYLOADC(1,0)<2>	r[p0,32]<32;16,2>:ub			{NoDDClr}
	mov (16)	mbMSGPAYLOADC(1,1)<2>	r[p0,128+32]<32;16,2>:ub		{NoDDChk}
	mov (16)	mbMSGPAYLOADC(2,0)<2>	r[p0,64]<32;16,2>:ub			{NoDDClr}
	mov (16)	mbMSGPAYLOADC(2,1)<2>	r[p0,128+64]<32;16,2>:ub		{NoDDChk}
	mov (16)	mbMSGPAYLOADC(3,0)<2>	r[p0,96]<32;16,2>:ub			{NoDDClr}
	mov (16)	mbMSGPAYLOADC(3,1)<2>	r[p0,128+96]<32;16,2>:ub		{NoDDChk}


 #if defined(MBAFF)
	add (1)		pMSGDSC:ud				gFIELDFLAGS:uw			MSG_LEN(4)+nDWBWMSGDSC+nBDIX_DESTC+ENWRCOM:ud
 #elif defined(FIELD)
	add (1)		pMSGDSC:ud				gFIELDFLAGS:uw			MSG_LEN(4)+nDWBWMSGDSC_TF+nBDIX_DESTC+ENWRCOM:ud
 #endif

    asr (1)		gMSGSRC.1:d				gMSGSRC.1:d					1:w	{NoDDClr}
    mov (1)		gMSGSRC.2:ud			0x0007000f:ud					{NoDDChk} // NV12 (16x4)

#if defined(FRAME)
    send (8)	gREG_WRITE_COMMIT_UV<1>:ud		mMSGHDRCW				gMSGSRC<8;8,1>:ud		DAPWRITE	MSG_LEN(4)+nDWBWMSGDSC+nBDIX_DESTC+ENWRCOM
#else
    send (8)	gREG_WRITE_COMMIT_UV<1>:ud		mMSGHDRCW				gMSGSRC<8;8,1>:ud		DAPWRITE	pMSGDSC:ud
#endif	// defined(FRAME)

#endif	// !defined(MONO)


//#endif	// !defined(__WRITERECON_YC__)
