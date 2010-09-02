/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: IMC3_Load_9x5.asm
//
//----------------------------------------------------------------
//  This module loads 16x8 Y, 9x5 U and 9x5 V planar data blocks for CSC module
//	and stores it in byte-aligned format.
//----------------------------------------------------------------

#define  IMC3_LOAD_9x5
#include "PL3_Load.inc"

// Load 16x8 planar Y ----------------------------------------------------------
    add  (2) rMSGSRC.0<1>:d     wORIX<2;2,1>:w    wSRC_H_ORI_OFFSET<2;2,1>:w       // Source Y Block origin
#if !defined(LOAD_UV_ONLY)
    mov  (1) rMSGSRC.2<1>:ud    nDPR_BLOCK_SIZE_Y:ud                               // Block width and height (16x8)

    mov  (8) mMSGHDRY<1>:ud     rMSGSRC<8;8,1>:ud
    send (8) udSRC_Y(0)<1>      mMSGHDRY    udDUMMY_NULL    nDATAPORT_READ    nDPMR_MSGDSC+nDPR_MSG_SIZE_Y+nBI_CURRENT_SRC_Y:ud
#endif

// Load 9x5 planar U and V -----------------------------------------------------
    asr (2)  rMSGSRC.0<1>:d     rMSGSRC.0<2;2,1>:d       1:w   // U/V block origin should be half of Y's
    mov (1)  rMSGSRC.2<1>:ud    nDPR_BLOCK_SIZE_UV:ud          // U/V block width and height (12x5)

    mov  (8) mMSGHDRU<1>:ud     rMSGSRC<8;8,1>:ud
    send (8) udSRC_U(0)<1>      mMSGHDRU    udDUMMY_NULL    nDATAPORT_READ    nDPMR_MSGDSC+nDPR_MSG_SIZE_UV+nBI_CURRENT_SRC_U:ud
    mov  (8) mMSGHDRV<1>:ud     rMSGSRC<8;8,1>:ud
    send (8) udSRC_V(0)<1>      mMSGHDRU    udDUMMY_NULL    nDATAPORT_READ    nDPMR_MSGDSC+nDPR_MSG_SIZE_UV+nBI_CURRENT_SRC_V:ud

// Convert to word-aligned format ----------------------------------------------
#if !defined(LOAD_UV_ONLY)
    $for (nY_NUM_OF_ROWS-1; >-1; -1) {
        mov (16)  uwDEST_Y(0,%1*16)<1>         ubSRC_Y(0,%1*16)
    }
#endif
    $for(nUV_NUM_OF_ROWS-2; >-1; -1) {
        mov (16)  uwDEST_U(0, %1*16)<1>        ubSRC_U(0, %1*16)
        mov (16)  uwDEST_V(0, %1*16)<1>        ubSRC_V(0, %1*16)
    }

// End of IMC3_Load_9x5
