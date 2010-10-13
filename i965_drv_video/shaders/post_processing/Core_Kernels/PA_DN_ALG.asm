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
#undef  nUV_NUM_OF_ROWS
#define nUV_NUM_OF_ROWS        8                                 // Number of U/V rows per block

#undef   nSMPL_RESP_LEN
#define  nSMPL_RESP_LEN        nSMPL_RESP_LEN_DN_PA              // Set the Number of GRFs in DNDI response 
#undef   nDPW_BLOCK_SIZE_DN
#define  nDPW_BLOCK_SIZE_DN    nBLOCK_WIDTH_32+nBLOCK_HEIGHT_8   // DN Curr Block Size for Write is 32x8
#undef   nDPW_BLOCK_SIZE_HIST
#define  nDPW_BLOCK_SIZE_HIST  nBLOCK_WIDTH_4+nBLOCK_HEIGHT_2    // HIST Block Size for Write is 4x2

////////////////////////////////////// Run the DN Algorithm ///////////////////////////////////////
#include "DNDI_COMMAND.asm"

////////////////////////////////////// Save the History Data for Next Run /////////////////////////
#include "DNDI_Hist_Save.asm"

////////////////////////////////////// Pack and Save the DN Curr Frame for Next Run ///////////////
add (4)     pCF_Y_OFFSET<1>:uw    ubDEST_CF_OFFSET<4;4,1>:ub    npDN_YUV:w 
$for (0; <nY_NUM_OF_ROWS; 1) {
    mov (16)    r[pCF_Y_OFFSET,  %1*32]<2>:ub   ubRESP(nNODI_LUMA_OFFSET,%1*16)<16;16,1>       // copy line of Y
}
$for (0; <nUV_NUM_OF_ROWS; 1) {
    mov (8)     r[pCF_U_OFFSET,  %1*32]<4>:ub   ubRESP(nNODI_CHROMA_OFFSET,%1*16+1)<16;8,2>    // copy line of U
    mov (8)     r[pCF_V_OFFSET,  %1*32]<4>:ub   ubRESP(nNODI_CHROMA_OFFSET,%1*16)<16;8,2>      // copy line of V
}

shl (1)     rMSGSRC.0<1>:ud     wORIX<0;1,0>:w     1:w       // X origin * 2 (422 output)
mov (1)     rMSGSRC.1<1>:ud     wORIY<0;1,0>:w               // Y origin
mov (1)     rMSGSRC.2<1>:ud     nDPW_BLOCK_SIZE_DN:ud        // block width and height (32x8)
mov (8)     mMSGHDR_DN<1>:ud    rMSGSRC<8;8,1>:ud            // message header   

$for(0; <nY_NUM_OF_ROWS; 2) {
        mov (16) mudMSGHDR_DN(1+%1)<1>  udDN_YUV(%1)REGION(8,1)    // Move DN Curr to MRF
}
send (8)    dNULLREG    mMSGHDR_DN   udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPMW_MSG_LEN_PA_DN_NODI+nBI_DESTINATION_YUV:ud     



