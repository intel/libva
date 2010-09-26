/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */


//Module name: PL8x8_Save_P208.inc
//
// Setup for storing planar data
//

#include "undefall.inc"                 //Undefine the SRC and DEST sysmbols

#define nDPW_BLOCK_SIZE_Y   nBLOCK_WIDTH_16+nBLOCK_HEIGHT_8   // Y block size 16x8
#define nDPW_MSG_SIZE_Y     nMSGLEN_4                         // # of MRF's to hold Y block data (4)
#define nDPW_BLOCK_SIZE_UV  nBLOCK_WIDTH_16+nBLOCK_HEIGHT_8    // U/V interleaved block width and height (16x8)
#define nDPW_MSG_SIZE_UV    nMSGLEN_4                         // # of MRF's to hold U/V block data (4)

#if (nSRC_REGION==nREGION_1)
        #define udSRC_Y                 udBOT_Y_IO
        #define udSRC_U                 udBOT_U_IO
        #define udSRC_V                 udBOT_V_IO
        #define ubSRC_Y                 ubBOT_Y
        #define ubSRC_U                 ubBOT_U
        #define ubSRC_V                 ubBOT_V

        #define uwSRC_U                 uwBOT_U  //For masking operation
        #define uwSRC_V                 uwBOT_V

        #define ub2DEST_Y               ub2TOP_Y
        #define ub2DEST_U               ub2TOP_U
        #define ub2DEST_V               ub2TOP_V
        
#elif (nSRC_REGION==nREGION_2)
        #define udSRC_Y                 udTOP_Y_IO
        #define udSRC_U                 udTOP_U_IO
        #define udSRC_V                 udTOP_V_IO
        #define ubSRC_Y                 ubTOP_Y
        #define ubSRC_U                 ubTOP_U
        #define ubSRC_V                 ubTOP_V

        #define uwSRC_U                 uwTOP_U  //For masking operation
        #define uwSRC_V                 uwTOP_V

        #define ub2DEST_Y               ub2BOT_Y
        #define ub2DEST_U               ub2BOT_U
        #define ub2DEST_V               ub2BOT_V
        
#endif

///* Yoni - masking is not relevant for ILK?!? 
//#define         TEMP0   REG(r,54)
//.declare    TEMP        Base=TEMP0      ElementSize=2   SrcRegion=<8;8,1>       Type=uw
///* Yoni - masking is not relevant for ILK?!? 


