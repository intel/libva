/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: WriteRecon_Y_16x8.asm
//
//  $Revision: 10 $
//  $Date: 10/03/06 5:28p $
//


//#if !defined(__WRITERECON_Y_16x8__)		// Make sure this is only included once
//#define __WRITERECON_Y_16x8__


    add (1)		p0:w					pERRORY:w				-256:w
    add (1)		p1:w					pERRORY:w				-128:w
    
	$for(0; <4; 1) {
    mov (16)	mbMSGPAYLOADY(%1,0)<1>	r[p0,%1*32+0]<8,2>:ub		{NoDDClr}
    mov (16)	mbMSGPAYLOADY(%1,16)<1>	r[p0,%1*32+16]<8,2>:ub		{NoDDChk}
    }    
 
 #if defined(MBAFF)
	add (1)		pMSGDSC:ud				gFIELDFLAGS:uw			MSG_LEN(4)+nDWBWMSGDSC+nBDIX_DESTY+ENWRCOM:ud
 #elif defined(FIELD)
	add (1)		pMSGDSC:ud				gFIELDFLAGS:uw			MSG_LEN(4)+nDWBWMSGDSC_TF+nBDIX_DESTY+ENWRCOM:ud
 #endif

    mov	(2)		gMSGSRC.0<1>:d			gX<2;2,1>:w		{NoDDClr}
    mov (1)		gMSGSRC.2:ud			0x0007000f:ud	{NoDDChk}
    
#if defined(FRAME)
    send (8)	gREG_WRITE_COMMIT_Y<1>:ud		mMSGHDRYW				gMSGSRC<8;8,1>:ud		DAPWRITE	MSG_LEN(4)+nDWBWMSGDSC+nBDIX_DESTY+ENWRCOM
#else
    send (8)	gREG_WRITE_COMMIT_Y<1>:ud		mMSGHDRYW				gMSGSRC<8;8,1>:ud		DAPWRITE	pMSGDSC:ud
#endif

//#endif	// !defined(__WRITERECON_Y_16x8__)
