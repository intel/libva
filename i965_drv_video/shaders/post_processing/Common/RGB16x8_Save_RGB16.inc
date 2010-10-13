/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: RGB16x8_Save_RGB16.inc
//

#include "undefall.inc"                 //Undefine the SRC and DEST sysmbols

// For saving
#define nDPW_BLOCK_SIZE_RGB16    nBLOCK_WIDTH_32+nBLOCK_HEIGHT_8    // RGB16 block size 32x8
#define nDPW_MSG_SIZE_RGB16      nMSGLEN_8                          // # of MRF's to hold RGB16 block data (8)

// For conversion to 16bit
.declare	uwTEMP_RGB16    Base=REG(r,nTEMP1)	ElementSize=2 SrcRegion=<16;16,1>	Type=uw		//1 GRF

// For masking
#undef  nDPR_MSG_SIZE_RGB16
#define nDPR_MSG_SIZE_RGB16      nRESLEN_8                          // # of MRF's to hold ARGB block data (8)
#define     rMASK_TEMP     REG(r,nTEMP0)
.declare    uwMASK_TEMP    Base=rMASK_TEMP    ElementSize=2    SrcRegion=<8;8,1>    Type=uw        //1 GRF

#if (nSRC_REGION==nREGION_1)
    // For saving
    #define ubDEST_ARGB      ubTOP_Y       //Data from previous module
    #define uwDEST_ARGB      uwTOP_Y       //Data from previous module
    #define udCSC_TEMP       udBOT_Y_IO    //Data Converted to 16 bits
    #define uwCSC_TEMP       uwBOT_Y
    //For masking operation
    #define udSRC_RGB16      udTOP_Y_IO    //To hold the destination data that shouldn't be modified
    #define uwSRC_RGB16      uwTOP_Y

#elif (nSRC_REGION==nREGION_2)
    // For saving
    #define ubDEST_ARGB      ubBOT_Y       //Data from previous module
    #define uwDEST_ARGB      uwBOT_Y       //Data from previous module
    #define udCSC_TEMP       udTOP_Y_IO    //Data Converted to 16 bits
    #define uwCSC_TEMP       uwTOP_Y
    //For masking operation
    #define udSRC_RGB16      udBOT_Y_IO    //To hold the destination data that shouldn't be modified
    #define uwSRC_RGB16      uwBOT_Y

#endif
