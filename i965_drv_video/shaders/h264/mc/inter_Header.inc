/*
 * Header file for all AVC INTER prediction kernels
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__INTER_HEADER__)	// Make sure this file is only included once
#define __INTER_HEADER__

// Module name: inter_header.inc
//
// Header file for all AVC INTER prediction kernels
//

#define INTER_KERNEL

//-------------------------------------------------------------------------------------------
// TODO: The followings will be merged with the above definitions later
//-------------------------------------------------------------------------------------------


//------------ Input parameters & bit masks

// SW WA for weighted prediction - 2007/09/06	
//.declare	guwR1			Base=r1 ElementSize=2 Type=uw	
//.declare	guwW128			Base=r63.13 ElementSize=2 Type=uw

#ifdef DEV_ILK
// #define SW_W_128		// Enable SW WA for special Weight=128 case. Can be commented to disable it
#else	// Pre DEV_ILK
#define SW_W_128		// Enable SW WA for special Weight=128 case.
#endif	// DEV_ILK

#ifdef	SW_W_128
.declare	gudW128			Base=r1.0 ElementSize=4 Type=ud
#else
#endif	// SW_W_128

#define		gORIX			r3.4				// :ub, X origin
#define		gORIY			r3.5				// :ub, Y origin

#define		gCBP			r3.9				// :ub, CBP (0, 0, Y0, Y1, Y2, Y3, Cb, Cr)
#define		nCBPY_MASK		0x3c
#define		nCBPU_MASK		0x2
#define		nCBPV_MASK		0x1

#define		gFIELDFLAGS		r3.1				// :uw - To compute message descriptor for write

#define		gMBTYPE			r3.1				// :ub, MB type
#define		nMBTYPE_MASK	0x1f
#define		gFIELDMBFLAG	r3.1				// :ub, Field MB flag
#define		nFIELDMB_MASK	0x40
#define		gMBPARITY		r3.3				// :ub, Bottom field flag
#define		nMBPARITY_MASK	0x01

#define		gWPREDFLAG		r3.0				// :ub, Weighted pred flag
#define		nWBIDIR_MASK	0xc0

#define		gSUBMB_SHAPE	r3.12				// :ub, Sub-MB shape
#define		gSUBMB_MODE		r3.13				// :ub, Sub-MB prediction mode
.declare	guwSUBMB_SHAPE_MODE	Base=r3.6 ElementSize=2 Type=uw

#define		gYWDENOM		r3.14				// :ub, Luma log2 weight denom
#define		gCWDENOM		r3.15				// :ub, Chroma log2 weight denom

#define		gADDR			r3.24				// :ub, Register addresses of error data / MV

.declare	gubBIDX			Base=r3.16 ElementSize=1 Type=ub

#define		gWGT			r8					// Weights/offsets
.declare    gdWGT			Base=r8  ElementSize=4 Type=d
.declare    gwWGT			Base=r8  ElementSize=2 Type=w
#define		gMV				r4					// MVs
.declare    gwMV			Base=r4  ElementSize=2 Type=w
.declare    gdMV			Base=r4  ElementSize=4 Type=d

.declare	gwERRORY		Base=r10 ElementSize=2 Type=w		// 16 GRFs
.declare	gubERRORY		Base=r10 ElementSize=1 Type=ub
.declare	gwERRORC		Base=r26 ElementSize=2 Type=w		// 8 GRFs
.declare	gubERRORC		Base=r26 ElementSize=2 Type=ub

//------------ Address registers
#define		pMSGDSC			a0.0				// ud: Must be the leading dword of the address register
#define		pREF			a0.0

#define		pBIDX			a0.2				
#define		pWGT			a0.3
#define		pERRORYC		a0.2				// :ud	
#define		pERRORY			a0.4
#define		pERRORC			a0.5
#define		pMV				a0.6	

#define		pWGT_BIDX		a0.1				// :ud, WGT & BIDX
#define		pRECON_MV		a0.3				// :ud, RECON & MV

#define		pREF0			a0.0				// :uw
#define		pREF0D			a0.0				// :ud
#define		pREF1			a0.1
#define		pREF2			a0.2
#define		pREF2D			a0.1				// :ud
#define		pREF3			a0.3
#define		pREF4			a0.4
#define		pREF4D			a0.2				// :ud
#define		pREF5			a0.5
#define		pREF6			a0.6
#define		pREF6D			a0.3				// :ud
#define		pREF7			a0.7

#define		pRES			a0.6
#define		pRESD			a0.3				// :ud
#define		pRESULT			a0.7

#define		p0				a0.0
#define		p1				a0.1

//------------ Constants for static/inline/indirect
#define		nOFFSET_BIDX	112					// = 32*3+4*4

#define		nOFFSET_WGT		256					// = 32*8
#define		nOFFSET_WGT_BIDX 0x01000070			// = (256<<16)+112
#define		nOFFSET_ERROR	0x03400140			// = (320+128*4)<<16+320=0x03400140
#define		nOFFSET_ERRORY	0x0140
#define		nOFFSET_ERRORC	0x0340
#define		nOFFSET_MV		128					// = 32*4
#define		nOFFSET_RECON_MV 0x04400080			// = (1088<<16)+128		// TODO: OFFSET_RECON is obsolete

//------------ Constants for kernel internal variables
#define		nOFFSET_INTPY0	0x0640				// = 32*50
#define		nOFFSET_INTPY1	0x0780				// = 32*60
#define		nOFFSET_INTPC0	0x06c0				// = 32*54
#define		nOFFSET_INTPC1	0x0480				// = 32*36
#define		nOFFSET_INTP0	0x06c00640
#define		nOFFSET_INTP1	0x04800780

#define		nOFFSET_INTERIM		0x0480				// = 32*36
#define		nOFFSET_INTERIM2	0x04A00480			// = ((32*37)<<16)|(32*36)
#define		nOFFSET_INTERIM3	0x04A00480			// = ((32*36+32)<<16)|(32*36)
#define		nOFFSET_INTERIM4	0x04A00490			// = ((32*37)<<16)|(32*36+16)

#define		nOFFSET_INTERIM4x4		0x04C0			// = 32*38
#define		nOFFSET_INTERIM4x4_4	0x04E004D0		// = ((32*38+32)<<16)|(32*38+16)
#define		nOFFSET_INTERIM4x4_5	0x04D004C0		// = ((32*38+16)<<16)|(32*38)
#define		nOFFSET_INTERIM4x4_6	0x04E004C0		// = ((32*38+32)<<16)|(32*38)
#define		nOFFSET_INTERIM4x4_7	0x04D004C8		// = ((32*38+16)<<16)|(32*38+8)
#define		nOFFSET_INTERIM4x4_8	0x04E004D8		// = ((32*38+32)<<16)|(32*38+24)
#define		nOFFSET_INTERIM4x4_9	0x04F004E8		// = ((32*38+48)<<16)|(32*38+40)

#define		nOFFSET_RES		0x540				// = 32*42
#define		nOFFSET_REF		0x560				// = 32*43
#define		nOFFSET_REFC	0x700				// = 32*56

			// Binding table index
#define		nBDIX_DESTY		0
#define		nBDIX_DESTC		1
#define		nBI_LC_DIFF		0x10				// Binding table index diff between luma and chroma

#define		nGRFWIB			32
#define		nGRFHWIB		16

//------------ Regions

.declare    gudREF			Base=r43 ElementSize=4 SrcRegion=<16;16,1> Type=ud
.declare    gubREF			Base=r43 ElementSize=1 Type=ub
.declare    gudREFC			Base=r56 ElementSize=4 SrcRegion=<16;16,1> Type=ud

// 16x16 handling
.declare    gudREF21x21		Base=r58 ElementSize=4 SrcRegion=<16;16,1> Type=ud
.declare    gudREF18x10		Base=r66 ElementSize=4 SrcRegion=<16;16,1> Type=ud
.declare    gubREF18x10		Base=r66 ElementSize=1 SrcRegion=<16;16,1> Type=ub



.declare    gudREF16x16		Base=r38 ElementSize=4 Type=ud			// 8 GRFs
.declare    gubREF16x16		Base=r38 ElementSize=1 Type=ub
.declare    gudREFC16x8		Base=r46 ElementSize=4 Type=ud			// 4 GRFs
.declare    gubREFC16x8		Base=r46 ElementSize=1 Type=ub

// TODO
.declare    gubAVG			Base=r56 ElementSize=1 Type=ub
.declare	gubREFY_BWD		Base=r64 ElementSize=1 Type=ub
.declare	gubREFC_BWD		Base=r72 ElementSize=1 Type=ub


.declare    guwINTPY0		Base=r50 ElementSize=2 SrcRegion=<16;16,1> Type=uw
.declare	gudINTPY0		Base=r50 ElementSize=4 Type=ud
.declare    gubINTPY0		Base=r50 ElementSize=1 SrcRegion=<32;16,2> Type=ub
.declare    guwINTPY1		Base=r60 ElementSize=2 SrcRegion=<16;16,1> Type=uw
.declare	gudINTPY1		Base=r60 ElementSize=4 Type=ud
.declare    gubINTPY1		Base=r60 ElementSize=1 SrcRegion=<32;16,2> Type=ub
.declare    guwYPRED		Base=r50 ElementSize=2 SrcRegion=<8;8,1> Type=uw
.declare    gubYPRED		Base=r50 ElementSize=1 SrcRegion=<32;16,2> Type=ub

.declare    guwINTPC0		Base=r54 ElementSize=2 SrcRegion=<16;16,1> Type=uw
.declare    gwINTPC0		Base=r54 ElementSize=2 SrcRegion=<16;16,1> Type=w
.declare	gudINTPC0		Base=r54 ElementSize=4 Type=ud
.declare    gubINTPC0		Base=r54 ElementSize=1 SrcRegion=<32;16,2> Type=ub
.declare    guwINTPC1		Base=r36 ElementSize=2 SrcRegion=<16;16,1> Type=uw
.declare	gudINTPC1		Base=r36 ElementSize=4 Type=ud
.declare    gubINTPC1		Base=r36 ElementSize=1 SrcRegion=<32;16,2> Type=ub
.declare    guwCPRED		Base=r54 ElementSize=2 SrcRegion=<16;8,2> Type=uw
.declare    gubCPRED		Base=r54 ElementSize=1 SrcRegion=<32;8,4> Type=ub

#define		gINTERIM		r36
.declare	gubINTERIM_BUF	Base=r36 ElementSize=1 SrcRegion=<32;16,2> Type=ub
#define		gINTERIM4x4		r38
.declare	gubINTERIM4x4_BUF Base=r38 ElementSize=1 SrcRegion=<32;16,2> Type=ub
.declare	gwINTERIM4x4_BUF Base=r38 ElementSize=2  Type=w

.declare	gubINTERIM_BUF2	Base=r42 ElementSize=1 SrcRegion=<8;4,2> Type=ub
.declare	gwINTERIM_BUF2	Base=r42 ElementSize=2 SrcRegion=<16;16,1> Type=w	
.declare	guwINTERIM_BUF2	Base=r42 ElementSize=2 Type=uw	

.declare	gwINTERIM_BUF3	Base=r38 ElementSize=2 SrcRegion=<16;16,1> Type=w		// 2 GRFs
.declare	gubINTERIM_BUF3	Base=r38 ElementSize=1 Type=ub							

.declare	gwTEMP			Base=r42 ElementSize=2 SrcRegion=<16;16,1> Type=w

//------------ General registers

#define		gX				r3.2				// w
#define		gY				r3.3				// w

#define		gMSGDSC_R		r3.6				// ud
#define		gMSGDSC_W		r3.7				// ud

#ifdef	SW_W_128
.declare	gwMBTYPE		Base=r8.6 ElementSize=2 Type=w			// Shared with gLOOP_SUBMB

// TODO
#define		gLOOP_SUBMB		r8.6
#define		gLOOP_SUBMBPT	r8.7
#define		gLOOP_DIR		r9.6
#define		gLOOPCNT		r9.7				// Loop counter for submodules
#else
.declare	gwMBTYPE		Base=r1.0 ElementSize=2 Type=w			// Shared with gLOOP_SUBMB

// TODO
#define		gLOOP_SUBMB		r1.0
#define		gLOOP_SUBMBPT	r1.1
#define		gLOOP_DIR		r8.7
#define		gLOOPCNT		r9.7				// Loop counter for submodules
#endif	// SW_W_128

#define		gW0				r34.6				// Temporary WORD 
#define		gW1				r34.7				// Temporary WORD 
#define		gW2				r34.8				// Temporary WORD 
#define		gW3				r34.9				// Temporary WORD 
#define		gD0				r34.3				// Temporary DWORD

#define		gW4				r34.15

//

#define		gMVX_INT		r34.0				// :w
#define		gMVY_INT		r34.1				// :w
#define		gMVX_FRAC		r34.2				// :w
#define		gMVY_FRAC		r34.3				// :w
#define		gMVX_FRACC		r34.4				// :w
#define		gMVY_FRACC		r34.5				// :w

#define		gpINTPY			r34.10
#define		gpINTPC			r34.11
#define		gpINTP			r34.5				// DW

#define		gPREDFLAG		r34.12
#define		gBIDX			r34.13
#define		gREFPARITY		r34.14
#define		gCHRMVADJ		r1.14
#define		gPARITY			r1.15
#define		gCBP_MASK		r1.1

#define		gMVSTEP			r1.13

#define		gpADDR			r1.2				// :uw (8 words)

#define		gSHAPETEMP		r8.15				// :uw

#define		gCOEFA			r42.0				
#define		gCOEFB			r42.1				
#define		gCOEFC			r42.2				
#define		gCOEFD			r42.3

// Weighted prediction
#define		gPREDFLAG0		r46.0
#define		gPREDFLAG1		r46.2

#define		gWEIGHTFLAG		r43.2
#define		gBIPRED			r43.3
#define		gYADD			r43.4
#define		gCADD			r43.5
#define		gYSHIFT			r43.6
#define		gCSHIFT			r43.7

#define		gOFFSET			r44.0
#define		gUOFFSET		r44.1
#define		gVOFFSET		r44.2

#define		gWT0			r45.0
#define		gO0				r45.1
#define		gWT1			r45.2
#define		gO1				r45.3
#define		gUW0			r45.4
#define		gUO0			r45.5
#define		gUW1			r45.6
#define		gUO1			r45.7	
#define		gVW0			r45.8	
#define		gVO0			r45.9	
#define		gVW1			r45.10	
#define		gVO1			r45.11

#define		gWT0_D			r45.0	
#define		gUW0_D			r45.2	

//------------ Message-related Registers & constants
#define		gMSGSRC			r2					// Message Source

#define		mMSGHDR			m1		
#define		mMSGHDRY		m1		
#define		mMSGHDRC		m2		
#define		mMSGHDR1		m1		
#define		mMSGHDR2		m2		
#define		mMSGHDR3		m3		
#define		mMSGHDR4		m4		
#define		mMSGHDRYW		m1		
#define		mMSGHDRCW		m10		

#ifdef DEV_ILK
	// 0000 0100(read)  0001(msg len) xxxx(resp len) 1010 (sampler cache) xxxx (field/frame) xxxx xxxx (bidx)
#define		nDWBRMSGDSC_SC		0x0208A002	// DWORD Block Read Message Descriptor through Data Port, Sampler Cache
#define		nDWBRMSGDSC_SC_TF	0x0208E602	// DWORD Block Read Message Descriptor through Data Port, Sampler Cache
#define		nDWBRMSGDSC_SC_BF	0x0208E702	// DWORD Block Read Message Descriptor through Data Port, Sampler Cache
	// 0000 0101(write) 0001(msg len) xxxx(resp len) 0010 (render cache)  xxxx (field/frame) xxxx xxxx (bidx)										
#define		nDWBWMSGDSC		0x02082000  // DWORD Block Write Message Descriptor through Data Port, Render Cache
#define		nDWBWMSGDSC_TF	0x02082600  // DWORD Block Write Message Descriptor through Data Port, Render Cache
#define		nDWBWMSGDSC_BF	0x02082700  // DWORD Block Write Message Descriptor through Data Port, Render Cache										

#else	// Pre DEV_ILK
	// 0000 0100(read)  0001(msg len) xxxx(resp len) 1010 (sampler cache) xxxx (field/frame) xxxx xxxx (bidx)
#define		nDWBRMSGDSC_SC		0x0410A002	// DWORD Block Read Message Descriptor through Data Port, Sampler Cache
#define		nDWBRMSGDSC_SC_TF	0x0410A602	// DWORD Block Read Message Descriptor through Data Port, Sampler Cache
#define		nDWBRMSGDSC_SC_BF	0x0410A702	// DWORD Block Read Message Descriptor through Data Port, Sampler Cache
	// 0000 0101(write) 0001(msg len) xxxx(resp len) 0010 (render cache)  xxxx (field/frame) xxxx xxxx (bidx)										
#define		nDWBWMSGDSC		0x05102000  // DWORD Block Write Message Descriptor through Data Port, Render Cache
#define		nDWBWMSGDSC_TF	0x05102600  // DWORD Block Write Message Descriptor through Data Port, Render Cache
#define		nDWBWMSGDSC_BF	0x05102700  // DWORD Block Write Message Descriptor through Data Port, Render Cache										
#endif	// DEV_ILK

#define		nDWB_FIELD_MASK	0x0600
										
// message data payload
.declare    mbMSGPAYLOADY	Base=m2  ElementSize=1 SrcRegion=REGION(16,1) Type=b
.declare    mbMSGPAYLOADC	Base=m11 ElementSize=1 SrcRegion=REGION(16,1) Type=b

// Destination registers for write commit
#define		gREG_WRITE_COMMIT_Y		r10.0
#define		gREG_WRITE_COMMIT_UV	r11.0

#define RETURN_REG_INTER	r1.5		// Return pointer for all sub-routine calls (type DWORD)

#define CALL_INTER(subFunc, skipInst)	add (1) RETURN_REG_INTER<1>:ud   ip:ud	1+skipInst*INST_SIZE \n\
				jmpi (1) subFunc
#define	RETURN_INTER		mov (1)	ip:ud	RETURN_REG_INTER<0;1,0>:ud		// Return to calling module


// End of inter_header.inc

#endif	// !defined(__INTER_HEADER__)

