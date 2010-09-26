/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: RGB16x8_Save_RGB.inc
//

#include "undefall.inc"                 //Undefine the SRC and DEST sysmbols

// For saving
#define nDPW_BLOCK_SIZE_ARGB     nBLOCK_WIDTH_32+nBLOCK_HEIGHT_8    // ARGB block size 32x8
#define nDPW_MSG_SIZE_ARGB       nMSGLEN_8                          // # of MRF's to hold ARGB block data (8)

// For masking
#undef  nDPR_MSG_SIZE_ARGB
#define nDPR_MSG_SIZE_ARGB       nRESLEN_8                          // # of MRF's to hold ARGB block data (8)
#define     rMASK_TEMP     REG(r,nTEMP0)
.declare    uwMASK_TEMP    Base=rMASK_TEMP    ElementSize=2    SrcRegion=<8;8,1>    Type=uw        //1 GRF

#if (nSRC_REGION==nREGION_1)
    // For saving
    #define udDEST_ARGB      udTOP_Y_IO    //The output of previous stage is stored here; This is modified and is written to render cache
    //For masking operation
    #define udSRC_ARGB       udBOT_Y_IO    //To hold the destination data that shouldn't be modified

#elif (nSRC_REGION==nREGION_2)
    // For saving
    #define udDEST_ARGB      udBOT_Y_IO    //The output of previous stage is stored here; This is modified and is written to render cache
    //For masking operation
    #define udSRC_ARGB       udTOP_Y_IO    //To hold the destination data that shouldn't be modified

#endif
