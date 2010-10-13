/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: PL8x8_Save_PA.inc
//
// Setup for storing packed data
//

#include "undefall.inc"                 //Undefine the SRC and DEST sysmbols

// For saving
#define nDPW_BLOCK_SIZE_YUV      nBLOCK_WIDTH_32+nBLOCK_HEIGHT_8    // YUV block size 32x8
#define nDPW_MSG_SIZE_YUV        nMSGLEN_8                          // # of MRF's to hold YUV block data (8)

// For masking
#undef  nDPR_MSG_SIZE_YUV
#define nDPR_MSG_SIZE_YUV        nRESLEN_8                          // # of MRF's to hold YUV block data (8)
#define     rMASK_TEMP     REG(r,nTEMP0)
.declare    uwMASK_TEMP    Base=rMASK_TEMP    ElementSize=2    SrcRegion=<8;8,1>    Type=uw        //1 GRF

#if (nSRC_REGION==nREGION_1)
    // For saving
    #define udSRC_YUV        udTOP_Y_IO
    #define udDEST_YUV       udBOT_Y_IO
    #define nDEST_YUV_REG    nBOT_Y
    //For masking operation
    #define ubSRC_Y          ub2TOP_Y
    #define ubSRC_U          ub2TOP_U
    #define ubSRC_V          ub2TOP_V
    #define uwSRC_YUV        uwTOP_Y
    #define uwDEST_YUV       uwBOT_Y

#elif (nSRC_REGION==nREGION_2)
    // For saving
    #define udSRC_YUV        udBOT_Y_IO
    #define udDEST_YUV       udTOP_Y_IO
    #define nDEST_YUV_REG    nTOP_Y
    //For masking operation
    #define ubSRC_Y          ub2BOT_Y
    #define ubSRC_U          ub2BOT_U
    #define ubSRC_V          ub2BOT_V
    #define uwSRC_YUV        uwBOT_Y
    #define uwDEST_YUV       uwTOP_Y

#endif
