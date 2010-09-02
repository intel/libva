/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: PA_Load.inc
//
// YUV422 data are first loaded to bottom I/O REGION_2, then unpacked to planar data
// and stored in top I/O REGION_1

#undef 	nY_NUM_OF_ROWS
#undef 	nUV_NUM_OF_ROWS

#define nY_NUM_OF_ROWS      8       // Number of Y rows per block
#define nUV_NUM_OF_ROWS     8       // Number of U/V rows per block

#if defined(PA_LOAD_8x8)
        #define nDPR_BLOCK_SIZE_YUV           nBLOCK_WIDTH_32+nBLOCK_HEIGHT_8   // Y block size 32x8
        #define nDPR_MSG_SIZE_YUV             nRESLEN_8                         // # of MRF's to hold Y block data (8)
#endif
#if defined(PA_LOAD_9x8)
        #define nDPR_BLOCK_SIZE_YUV_MAIN      nBLOCK_WIDTH_32+nBLOCK_HEIGHT_8   // Main YUV block size 32x8
        #define nDPR_MSG_SIZE_YUV_MAIN        nRESLEN_8                         // # of MRF's to hold Y block data (8)
        #define nDPR_BLOCK_SIZE_YUV_ADDITION  nBLOCK_WIDTH_4+nBLOCK_HEIGHT_8    // Additional YUV block size 4x8
        #define nDPR_MSG_SIZE_YUV_ADDITION    nRESLEN_1                         // # of MRF's to hold Y block data (8)
#endif

#define udSRC_YUV               udBOT_Y_IO
#define nSRC_YUV_REG            nBOT_Y

#define uwDEST_Y                uwTOP_Y
#define uwDEST_U                uwTOP_U
#define uwDEST_V                uwTOP_V

#define nSRC_REGION nREGION_1    // REGION_1 will be the source region for first kernel

// End of PA_Load.inc
