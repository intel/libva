/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: P208_Load_9x8.asm
//----------------------------------------------------------------

#define  P208_LOAD_9x8
#include "PL2_Load.inc"

    add  (2) rMSGSRC.0<1>:d     wORIX<2;2,1>:w    wSRC_H_ORI_OFFSET<2;2,1>:w       // Source Y Block origin

// Load 16x8 P208 Y ------------------------------------------------------------
#if !defined(LOAD_UV_ONLY)
    mov  (1) rMSGSRC.2<1>:ud    nDPR_BLOCK_SIZE_Y:ud                               // Y block width and height (16x8)
    mov  (8) mMSGHDRY<1>:ud     rMSGSRC<8;8,1>:ud
    send (8) udSRC_Y(0)<1>      mMSGHDRY    udDUMMY_NULL    nDATAPORT_READ    nDPMR_MSGDSC+nDPR_MSG_SIZE_Y+nBI_CURRENT_SRC_Y:ud
#endif

	// Load 16x8 planar UV -----------------------------------------------------
    mov  (1) rMSGSRC.2<1>:ud	nDPR_BLOCK_SIZE_UV:ud          // U/V block width and height (20x8)
    mov  (8) mMSGHDRU<1>:ud     rMSGSRC<8;8,1>:ud
    send (8) udSRC_U(0)<1>      mMSGHDRU    udDUMMY_NULL    nDATAPORT_READ    nDPMR_MSGDSC+nDPR_MSG_SIZE_UV+nBI_CURRENT_SRC_UV:ud

// Convert to word-aligned format ----------------------------------------------
#if !defined(LOAD_UV_ONLY)
    $for (0; <nY_NUM_OF_ROWS; 1) {
        mov	(16)	uwDEST_Y(0,%1*16)	ubSRC_Y(0,%1*16)
    }
#endif
    $for (0; <nUV_NUM_OF_ROWS; 1) {
        mov	(16)	uwDEST_U(0,%1*16)	ubSRC_U(0,%1*32)<32;16,2>
        mov	(16)	uwDEST_V(0,%1*16)	ubSRC_U(0,%1*32+1)<32;16,2>
    }

// End of P208_Load_9x8.asm
