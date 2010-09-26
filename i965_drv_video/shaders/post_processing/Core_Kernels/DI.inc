/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: DI.inc

#ifdef GT
// GT DI Kernel
#else // ILK
// ILK DI Kernel
#endif
 

//---------------------------------------------------------------------------
// Binding table indices
//---------------------------------------------------------------------------
#define nBIDX_DI_PRV		10		// Previous DI-ed frame
#define nBIDX_DI_CUR		13		// Current DI-ed frame
#define	nBIDX_DN			7		// Denoised frame
#define	nBIDX_STAT			20		// Statistics 
#define nBIDX_DI_Source  4  // Source Surface


//---------------------------------------------------------------------------
// Message descriptors
//---------------------------------------------------------------------------
// Extended message descriptor
#define nSMPL_ENGINE		0x2
#define nDATAPORT_WRITE		0x5
#define nTS_EOT				0x27	// with End-Of-Thread bit ON

		// Message descriptor for end-of-thread
		//						= 000 0001 (message len) 00000 (resp len)
		//						  0 (header present 0) 00000000000000 0 (URB dereferenced) 0000
#define nEOT_MSGDSC			0x02000000

		// Message descriptor for sampler read
		//						= 000 0010 (message len 2) 00000 (resp len - set later, 12 or 5 or 11)  
		//						  1 (header present 1) 0 11 (SIMD32/64 mode) 
		//						  1000 (message type) 0000 (DI state index) 
		//						  00000000 (binding table index - set later)
		//						= 0x040b8000

// comment begin
// The following is commented out because of walker feature
// It corresponds to the #ifdef GT #else and #endif
//#define nSMPL_MSGDSC		    0x040b8000
//#define nSMPL_RESP_LEN_DI	    0x00c00000		// 12
//#define nSMPL_RESP_LEN_NODI_PL  0x00500000		// 5
//#define nSMPL_RESP_LEN_NODI_PA  0x00900000		// 9
//#define nSMPL_RESP_LEN_NODN	    0x00900000		// 9
//#define nSMPL_RESP_LEN_PDI	    0x00b00000		// 11
// comment end

#ifdef GT

#define nSMPL_MSGDSC		    0x040b8000
#define nSMPL_RESP_LEN_DI	    0x00c00000		// 12
#define nSMPL_RESP_LEN_NODI_PL  0x00500000		// 5  //DI disable, the XY stored in 5th GRF, no impact to return length
#define nSMPL_RESP_LEN_NODI_PA  0x00900000		// 9  //DI disable, the XY stored in 5th GRF, no impact to return length
#define nSMPL_RESP_LEN_NODN	    0x00a00000		// 10 //NO DN, originally use 9, now we need use 10 to store the XY with walker
#define nSMPL_RESP_LEN_PDI	    0x00b00000		// 11

#else

#define nSMPL_MSGDSC		    0x040b8000
#define nSMPL_RESP_LEN_DI	    0x00c00000		// 12
#define nSMPL_RESP_LEN_NODI_PL  0x00500000		// 5
#define nSMPL_RESP_LEN_NODI_PA  0x00900000		// 9
#define nSMPL_RESP_LEN_NODN	    0x00900000		// 9
#define nSMPL_RESP_LEN_PDI	    0x00b00000		// 11

#endif

		// Message descriptor for dataport media write
#ifdef GT
		//						= 000 0000 (message len - set later) 00000 (resp len 0) 		
		//						  1 (header present 1) 0 0 1010 (media block write) 00000
		//						  00000000 (binding table index - set later)
		//						= 0x00094000
#define nDPMW_MSGDSC		    0x00094000
#else // ILK
		//						= 000 0000 (message len - set later) 00000 (resp len 0) 		
		//						  1 (header present 1) 000 0 010 (media block write) 0000
		//						  00000000 (binding table index - set later)
		//						= 0x00082000
#define nDPMW_MSGDSC		    0x00082000
#endif
#define nDPMW_MSG_LEN_STMM	    0x04000000		// 2 - STMM
#define nDPMW_MSG_LEN_DH	    0x04000000		// 2 - Denoise history
#define nDPMW_MSG_LEN_PA_DN	    0x0a000000		// 5 - Denoised output
#define nDPMW_MSG_LEN_PA_NODI	0x12000000		// 9 - Denoised output - denoise only - DI disabled
#define nDPMW_MSG_LEN_PL_DN	    0x06000000		// 3 - Denoised output
#define nDPMW_MSG_LEN_PL_NODI	0x0a000000		// 5 - Denoised output - denoise only - DI disabled
#define nDPMW_MSG_LEN_DI	    0x0a000000		// 5 - DI output


//---------------------------------------------------------------------------
// Static and inline parameters
//---------------------------------------------------------------------------
// Static parameters
.declare ubTFLD_FIRST		Base=r1.27	ElementSize=1 Type=ub	// top field first
.declare ubSRCYUVOFFSET		Base=r1.4	ElementSize=1 Type=ub	// source packed format
.declare ubDSTYUVOFFSET		Base=r1.8	ElementSize=1 Type=ub	// destination packed format
.declare uwSPITCH_DIV2		Base=r1.10	ElementSize=2 Type=uw	// statistics surface pitch divided by 2

// Inline parameters
.declare uwXORIGIN			Base=r5.0	ElementSize=2 Type=uw	// X and Y origin
.declare uwYORIGIN			Base=r5.1	ElementSize=2 Type=uw


//---------------------------------------------------------------------------
// Kernel GRF variables 
//---------------------------------------------------------------------------
// Message response (Denoised & DI-ed pixels & statistics)
.declare dRESP						Base=r8		ElementSize=4 Type=d	// Response message (12 or 5 or 11)
.declare ubRESP						Base=r8		ElementSize=1 Type=ub	

.declare dSTMM						Base=r16	ElementSize=4 Type=d	// STMM
.declare ubDN_HIST_NODI		Base=r12	ElementSize=1 Type=ub	// Denoise history data (DI disabled)
.declare ubDN_HIST_DI			Base=r17	ElementSize=1 Type=ub	// Denoise history data (DI enabled)
.declare uwRETURNED_POSITION_DI	Base=r17	ElementSize=2 Type=uw	// XY_Return_Data (DI enabled)
.declare uwRETURNED_POSITION_DN	Base=r12	ElementSize=2 Type=uw // XY_Return_Data (DI disabled)

.declare ub1ST_FLD_DN			Base=r12	ElementSize=1 Type=ub	// 1st field Denoised data (DI enabled)
.declare d1ST_FLD_DN			Base=r12	ElementSize=4 Type=d
.declare ub2ND_FLD_DN			Base=r18	ElementSize=1 Type=ub	// 2nd field Denoised data (DI enabled)	
.declare d2ND_FLD_DN			Base=r18	ElementSize=4 Type=d
.declare ubPRV_DI					Base=r8		ElementSize=1 Type=ub	// Previous frame DI (DI enabled)
.declare ubCUR_DI					Base=r12	ElementSize=1 Type=ub	// Previous frame DI (DI enabled)

// Packed denoised output
.declare ubDN_YUV					Base=r22	ElementSize=1 Type=ub	// Denoised YUV422
.declare dDN_YUV					Base=r22	ElementSize=4 Type=d
#define	 npDN_YUV			704									// = 22*32 = 0x280

// Packed DI output
.declare dDI_YUV_PRV			Base=r32	ElementSize=4 Type=d	// Previous frame DI output
.declare dDI_YUV_CUR			Base=r36	ElementSize=4 Type=d	// Current frame DI output
#define	 npDI_YUV			1024									// = 32*32 = 0x 

// For packed output
#define	 p422_YOFFSET		a0.2	
#define	 p422_UOFFSET		a0.3	
#define	 p422_VOFFSET		a0.4
#define	 pDN_TFLDSRC		a0.6	
#define	 pDN_BFLDSRC		a0.7	
#define	 npRESP				192									// = 6*32

// Message source
.declare udMSGSRC					Base=r70	  ElementSize=4 Type=ud
.declare uwMSGSRC					Base=r70	  ElementSize=2 Type=uw
.declare dMSGSRC          Base=r70    ElementSize=4 Type=d


//---------------------------------------------------------------------------
// Kernel MRF variables 
//---------------------------------------------------------------------------
#define	mMSGHDR_SMPL		m1									// Sampler response: m1~m2
.declare mudMSGHDR_SMPL		Base=m1		ElementSize=4 Type=ud
.declare muwMSGHDR_SMPL		Base=m1		ElementSize=2 Type=uw
#define	mMSGHDR_DN			m3									// Denoise output: m3~m7 for PA, m3~m5 for PL
.declare mdMSGHDR_DN		Base=m3		ElementSize=4 Type=d
#define	mMSGHDR_STAT		m8									// Statistics output: m8~m9
.declare mdMSGHDR_STAT		Base=m8		ElementSize=4 Type=d
.declare mubMSGHDR_STAT		Base=m8		ElementSize=1 Type=ub
#define	mMSGHDR_DI			m10									// DI output: m10~m14
.declare mdMSGHDR_DI		Base=m10	ElementSize=4 Type=d
#define	mMSGHDR_EOT			m15									// EOT

#ifdef GT
#define	MSGSRC
#else
#define MSGSRC				null:ud
#endif

        
//---------------------------------------------------------------------------
// End of thread instruction
//---------------------------------------------------------------------------
#ifdef GT
#define END_THREAD			send (8) null<1>:d mMSGHDR_EOT nTS_EOT nEOT_MSGDSC 
#else	// ILK
#define END_THREAD			send (8) null<1>:d mMSGHDR_EOT null:ud	nTS_EOT nEOT_MSGDSC
#endif


// end of DI.inc
