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

#include "undefall.inc"

//---------------------------------------------------------------------------
// Message descriptors
//---------------------------------------------------------------------------
// Extended message descriptor
          // Message descriptor for sampler read
//        //                      = 000 0010 (message len 2) 00000 (resp len - set later, 12 or 5 or 11)  
//        //                        1 (header present 1) 0 11 (SIMD32/64 mode) 
//        //                        1000 (message type) 0000 (DI state index) 
//        //                        00000000 (binding table index - set later)
//        //                      = 0x040b8000
#define nSMPL_DI_MSGDSC           0x040b8000

#define nSMPL_RESP_LEN_DNDI      nRESLEN_12      // 12 - for DN + DI Alg
#define nSMPL_RESP_LEN_DN_PL     nRESLEN_5       // 5  - for DN Planar Alg
#define nSMPL_RESP_LEN_DN_PA     nRESLEN_9       // 9  - for DN Packed Alg
#define nSMPL_RESP_LEN_DI        nRESLEN_9       // 9  - for DI Only Alg
#define nSMPL_RESP_LEN_PDI       nRESLEN_11      // 11 - for Partial DI Alg

// Attention: The Message Length is The Number of GRFs with Data Only, without the Header
#define nDPMW_MSG_LEN_STMM       nMSGLEN_1       // 1 - For STMM Save
#define nDPMW_MSG_LEN_HIST       nMSGLEN_1       // 1 - For Denoise History Save
#define nDPMW_MSG_LEN_PA_DN_DI   nMSGLEN_4       // 4 - For DN Curr Save
#define nDPMW_MSG_LEN_PA_DN_NODI nMSGLEN_8       // 8 - For DN Curr Save (denoise only - DI disabled)
#define nDPMW_MSG_LEN_PL_DN_DI   nMSGLEN_2       // 2 - For DN Curr Save
#define nDPMW_MSG_LEN_PL_DN_NODI nMSGLEN_4       // 4 - For DN Curr Save (denoise only - DI disabled)

#define nDPW_BLOCK_SIZE_STMM   nBLOCK_WIDTH_8+nBLOCK_HEIGHT_4   // Y block size 8x4

#undef  nDPW_BLOCK_SIZE_DI
#undef  nDPW_MSG_SIZE_DI
#define nDPW_BLOCK_SIZE_DI  nBLOCK_WIDTH_32+nBLOCK_HEIGHT_4    
#define nDPW_MSG_SIZE_DI    nMSGLEN_4


//---------------------------------------------------------------------------
// Kernel GRF variables 
//---------------------------------------------------------------------------
// Defines for DI enabled
#define nDI_PREV_FRAME_LUMA_OFFSET          0
#define nDI_PREV_FRAME_CHROMA_OFFSET        2
#define nDI_CURR_FRAME_LUMA_OFFSET          4
#define nDI_CURR_FRAME_CHROMA_OFFSET        6
#define nDI_STMM_OFFSET                     8
#define nDI_HIST_OFFSET                     9
#define nDI_CURR_2ND_FIELD_LUMA_OFFSET     10
#define nDI_CURR_2ND_FIELD_CHROMA_OFFSET   11

// Defines for DI disabled
#define nNODI_LUMA_OFFSET                   0
#define nNODI_HIST_OFFSET                   4
#define nNODI_CHROMA_OFFSET                 5

#ifdef DI_ENABLE
    #define nHIST_OFFSET    nDI_HIST_OFFSET
    #undef  nY_NUM_OF_ROWS
    #define nY_NUM_OF_ROWS      8       // Number of Y rows per block (4 rows for each frame) 
    #undef  nUV_NUM_OF_ROWS
    #define nUV_NUM_OF_ROWS     8       // Number of U/V rows per block
	
#endif

#ifdef DI_DISABLE
    #define nHIST_OFFSET    nNODI_HIST_OFFSET
#endif

#if (nSRC_REGION==nREGION_2)
    #define ub2SRC_Y      ub2BOT_Y
    #define ub2SRC_U      ub2BOT_U
    #define ub2SRC_V      ub2BOT_V
    #define uwDEST_Y      uwBOT_Y
    #define uwDEST_U      uwBOT_U
    #define uwDEST_V      uwBOT_V
    #define nDEST_YUV_REG nTOP_Y
    #define udDEST_YUV    udTOP_Y_IO

    #define nRESP         nTEMP0         // DI return message requires 12 GRFs
    #define nDN_YUV       nTOP_Y         // Space for Packing DN for next run requires 8 GRFs

    #undef  nSRC_REGION
    #define nSRC_REGION   nREGION_2

#else
    #define ub2SRC_Y      ub2TOP_Y
    #define ub2SRC_U      ub2TOP_U
    #define ub2SRC_V      ub2TOP_V
    #define uwDEST_Y      uwTOP_Y
    #define uwDEST_U      uwTOP_U
    #define uwDEST_V      uwTOP_V
    #define nDEST_YUV_REG nBOT_Y
    #define udDEST_YUV    udBOT_Y_IO
    #define nRESP         nTEMP0         // DI return message requires 12 GRFs
    #define nDN_YUV       nBOT_Y         // Space for Packing DN for next run requires 8 GRFs

    #undef  nSRC_REGION
    #define nSRC_REGION   nREGION_1    // REGION_1 will be the source region for first kernel

#endif


    






// Message response (Denoised & DI-ed pixels & statistics)
.declare udRESP      Base=REG(r,nRESP) ElementSize=4 SrcRegion=REGION(8,1) DstRegion=<1> Type=ud
.declare ubRESP      Base=REG(r,nRESP) ElementSize=1 SrcRegion=REGION(16,1) DstRegion=<1> Type=ub

// For Denoised Curr Output (Used as Priv in Next Run)
.declare ubDN_YUV           Base=REG(r,nDN_YUV)    ElementSize=1 Type=ub
.declare udDN_YUV           Base=REG(r,nDN_YUV)    ElementSize=4 Type=ud
#define  npDN_YUV           nDN_YUV*nGRFWIB                                 

// For DI Process Output (1st and 2nd Frames Output)
//.declare udDI_YUV_PRIV      Base=REG(r,nTEMP0)    ElementSize=4 Type=ud   // Previous frame DI output
//.declare udDI_YUV_CURR      Base=REG(r,nTEMP0)    ElementSize=4 Type=ud   // Current frame DI output
//#define  npDI_YUV           nTEMP0*nGRFWIB                                  

//---------------------------------------------------------------------------
// Kernel MRF variables 
//---------------------------------------------------------------------------
#define  mMSG_SMPL           m1                                              // Sampler Command is in: m1~m2
.declare mudMSG_SMPL         Base=mMSG_SMPL         ElementSize=4 Type=ud
.declare muwMSG_SMPL         Base=mMSG_SMPL         ElementSize=2 Type=uw

#define mMSGHDR_DN           m1                                              // Denoise Output: m1~m9 for PA, m3~m5 for PL
.declare mudMSGHDR_DN        Base=mMSGHDR_DN        ElementSize=4 Type=ud
.declare mubMSGHDR_DN        Base=mMSGHDR_DN        ElementSize=1 Type=ub

#define mMSGHDR_STMM         m11                                             // STMM Output: m11~m12
.declare mudMSGHDR_STMM      Base=mMSGHDR_STMM      ElementSize=4 Type=ud
#define mMSGHDR_HIST         m13                                             // HIST Output: m13~m14
.declare mudMSGHDR_HIST      Base=mMSGHDR_HIST      ElementSize=1 Type=ud

#define mMSGHDR_DI_1ST       m1                                              // DI output: m1~m5
.declare mudMSGHDR_DI_1ST    Base=mMSGHDR_DI_1ST    ElementSize=4 Type=ud
#define mMSGHDR_DI_2ND       m6                                              // DI output: m6~m10
.declare mudMSGHDR_DI_2ND    Base=mMSGHDR_DI_2ND    ElementSize=4 Type=ud

// end of DNDI.inc
