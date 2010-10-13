/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: PL3_Load.inc

#undef 	nY_NUM_OF_ROWS
#undef 	nUV_NUM_OF_ROWS

#define nY_NUM_OF_ROWS      8                                     // Number of Y rows per block
#define nDPR_BLOCK_SIZE_Y   nBLOCK_WIDTH_16+nBLOCK_HEIGHT_8       // Y block size 16x8
#define nDPR_MSG_SIZE_Y     nRESLEN_4                             // # of MRF's to hold Y block data (4)

#if defined(IMC3_LOAD_8x4)
    #define nUV_NUM_OF_ROWS     4                                 // Number of U/V rows per block
    #define nDPR_BLOCK_SIZE_UV  nBLOCK_WIDTH_8+nBLOCK_HEIGHT_4    // U/V block size 8x4
    #define nDPR_MSG_SIZE_UV    nRESLEN_1                         // # of MRF's to hold U/V block data (1)
#endif
#if defined(IMC3_LOAD_8x5)
    #define nUV_NUM_OF_ROWS     6                                 // Number of U/V rows per block (Rounded Up to Nearest Even Number)
    #define nDPR_BLOCK_SIZE_UV  nBLOCK_WIDTH_8+nBLOCK_HEIGHT_5    // U/V block size 8x5
    #define nDPR_MSG_SIZE_UV    nRESLEN_2                         // # of MRF's to hold U/V block data (2)
#endif
#if defined(IMC3_LOAD_9x5)
    #define nUV_NUM_OF_ROWS     6                                 // Number of U/V rows per block (Rounded Up to Nearest Even Number)
    #define nDPR_BLOCK_SIZE_UV  nBLOCK_WIDTH_12+nBLOCK_HEIGHT_5   // U/V block size 12x5
    #define nDPR_MSG_SIZE_UV    nRESLEN_3                         // # of MRF's to hold U/V block data (3)
#endif

// Source/destination region definitions
#if !defined(udSRC_Y)
    #define udSRC_Y  udBOT_Y_IO     // Default Y source region is top Y region
#endif

#if !defined(udSRC_U)
    #define udSRC_U  udBOT_U_IO     // Default U source region is top U region
#endif

#if !defined(udSRC_V)
    #define udSRC_V  udBOT_V_IO     // Default V source region is top V region
#endif

#define ubSRC_Y      ubBOT_Y        // Loading data are always in byte type
#define ubSRC_U      ubBOT_U
#define ubSRC_V      ubBOT_V

#define uwDEST_Y     uwTOP_Y        // However they can be transferred to word-aligned byte if desired
#define uwDEST_U     uwTOP_U
#define uwDEST_V     uwTOP_V

#define nSRC_REGION  nREGION_1      // REGION_1 will be the source region for first kernel

// End of PL3_Load.inc
