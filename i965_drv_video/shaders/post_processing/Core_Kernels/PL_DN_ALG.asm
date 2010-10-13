/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

#define DI_DISABLE

#include "DNDI.inc"

#undef  nY_NUM_OF_ROWS
#define nY_NUM_OF_ROWS         8                                 // Number of Y rows per block

#undef   nSMPL_RESP_LEN
#define  nSMPL_RESP_LEN        nSMPL_RESP_LEN_DN_PL              // Set the Number of GRFs in DNDI response 
#undef   nDPW_BLOCK_SIZE_DN
#define  nDPW_BLOCK_SIZE_DN    nBLOCK_WIDTH_16+nBLOCK_HEIGHT_8   // DN Curr Block Size for Write is 16x8
#undef   nDPW_BLOCK_SIZE_HIST
#define  nDPW_BLOCK_SIZE_HIST  nBLOCK_WIDTH_4+nBLOCK_HEIGHT_2    // HIST Block Size for Write is 4x2

////////////////////////////////////// Run the DN Algorithm ///////////////////////////////////////
#include "DNDI_COMMAND.asm"

////////////////////////////////////// Rearrange for Internal Planar //////////////////////////////
$for (0; <nY_NUM_OF_ROWS; 1) {
    mov (16)    uwDEST_Y(0,%1*16)<1>   ubRESP(nNODI_LUMA_OFFSET,%1*16)<16;16,1>       // copy line of Y
}

////////////////////////////////////// Save the History Data for Next Run /////////////////////////
#include "DNDI_Hist_Save.asm"

