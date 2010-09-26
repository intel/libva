/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: PL2_Load.inc

#undef 	nY_NUM_OF_ROWS
#undef 	nUV_NUM_OF_ROWS

#define nY_NUM_OF_ROWS      8                                         // Number of Y rows per block
#define nDPR_BLOCK_SIZE_Y   nBLOCK_WIDTH_16+nBLOCK_HEIGHT_8           // Y block size 16x8
#define nDPR_MSG_SIZE_Y     nRESLEN_4                                 // # of MRF's to hold Y block data (4)


#if defined(NV11_LOAD_4x8)
        #define nUV_NUM_OF_ROWS     8                                 // Number of U/V rows per block
        #define nDPR_BLOCK_SIZE_UV  nBLOCK_WIDTH_8+nBLOCK_HEIGHT_8    // U/V block size 8x8
        #define nDPR_MSG_SIZE_UV    nRESLEN_2                         // # of MRF's to hold U/V block data (2)
#endif

#if defined(NV11_LOAD_5x8)
        #define nUV_NUM_OF_ROWS     8                                 // Number of U/V rows per block
        #define nDPR_BLOCK_SIZE_UV  nBLOCK_WIDTH_12+nBLOCK_HEIGHT_8   // U/V block size 12x8
        #define nDPR_MSG_SIZE_UV    nRESLEN_4                         // # of MRF's to hold U/V block data (4)
#endif
#if defined(NV12_LOAD_8x4)
        #define nUV_NUM_OF_ROWS     4                                 // Number of U/V rows per block
        #define nDPR_BLOCK_SIZE_UV  nBLOCK_WIDTH_16+nBLOCK_HEIGHT_4   // U/V block size 16x4
        #define nDPR_MSG_SIZE_UV    nRESLEN_2                         // # of MRF's to hold U/V block data (2)
#endif
#if defined(NV12_LOAD_8x5)
        #define nUV_NUM_OF_ROWS     6                                 // Number of U/V rows per block (Rounded Up to Nearest Even Number)
        #define nDPR_BLOCK_SIZE_UV  nBLOCK_WIDTH_16+nBLOCK_HEIGHT_5   // U/V block size 16x5
        #define nDPR_MSG_SIZE_UV    nRESLEN_3                         // # of MRF's to hold U/V block data (3)
#endif
#if defined(NV12_LOAD_9x5)
        #define nUV_NUM_OF_ROWS     6                                 // Number of U/V rows per block (Rounded Up to Nearest Even Number)
        #define nDPR_BLOCK_SIZE_UV  nBLOCK_WIDTH_20+nBLOCK_HEIGHT_5   // U/V block size 20x5
        #define nDPR_MSG_SIZE_UV    nRESLEN_5                         // # of MRF's to hold U/V block data (5)
#endif
#if defined(P208_LOAD_8x8)
        #define nUV_NUM_OF_ROWS     8                                 // Number of U/V rows per block
        #define nDPR_BLOCK_SIZE_UV  nBLOCK_WIDTH_16+nBLOCK_HEIGHT_8   // U/V block size 16x8
        #define nDPR_MSG_SIZE_UV    nRESLEN_4                         // # of MRF's to hold U/V block data (4)
#endif
#if defined(P208_LOAD_9x8)
        #define nUV_NUM_OF_ROWS     8                                 // Number of U/V rows per block
        #define nDPR_BLOCK_SIZE_UV  nBLOCK_WIDTH_20+nBLOCK_HEIGHT_8   // U/V block size 20x8
        #define nDPR_MSG_SIZE_UV    nRESLEN_8                         // # of MRF's to hold U/V block data (8)
#endif

// Source/destination region definitions
#if !defined(udSRC_Y)
        #define udSRC_Y  udBOT_Y_IO     // Default Y source region is top Y region
#endif

#if !defined(udSRC_U)
        #define udSRC_U  udBOT_U_IO     // Default U source region is top U region
#endif

#define ubSRC_Y           ubBOT_Y
#define nSRC_Y_REG        nBOT_Y
#define ubSRC_U           ubBOT_U
#define nSRC_U_REG        nBOT_U

#define uwDEST_Y          uwTOP_Y       // However they can be transferred to word-aligned byte if desired
#define uwDEST_U          uwTOP_U
#define uwDEST_V          uwTOP_V

#define nSRC_REGION       nREGION_1     // REGION_1 will be the source region for first kernel

// End of PL2_Load.inc
