/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: AYUV_Load_16x8.asm
//----------------------------------------------------------------


#include "AYUV_Load_16x8.inc"

// In order to load 64x8 AYUV data (16x8 pixels), we need to divide the data 
// into two regions and load them separately. 
//
//       32 byte         32 byte
//|----------------|----------------|
//|                |                |
//|       A        |       B        |8
//|                |                |
//|                |                |
//|----------------|----------------|

// Load the first 32x8 data block
// Packed data block should be loaded as 32x8 pixel block
    add  (2) rMSGSRC.0<1>:d     wORIX<2;2,1>:w    wSRC_H_ORI_OFFSET<2;2,1>:w    // Source Block origin
    shl  (1) rMSGSRC.0<1>:d     acc0:w            2:w          { NoDDClr }      // H. block origin need to be four times larger
    mov  (1) rMSGSRC.2<1>:ud    nDPR_BLOCK_SIZE_YUV:ud         { NoDDChk }      // Block width and height (32x8)
    mov  (8) mMSGHDRY<1>:ud     rMSGSRC<8;8,1>:ud
    send (8) udSRC_YUV(0)<1>    mMSGHDRY    udDUMMY_NULL    nDATAPORT_READ    nDPMR_MSGDSC+nDPR_MSG_SIZE_YUV+nBI_CURRENT_SRC_YUV:ud

//Load the second 32x8 data block    
// Offset the origin X - move to next 32 colomns
    add (1) rMSGSRC.0<1>:d    rMSGSRC.0<0;1,0>:d    32:w                        // Increase X origin by 8 
    
// Size stays the same - 32x8
    mov  (8) mMSGHDRY<1>:ud     rMSGSRC<8;8,1>:ud                               // Copy message description to message header
    send (8) udSRC_YUV(8)<1>    mMSGHDRY    udDUMMY_NULL    nDATAPORT_READ    nDPMR_MSGDSC+nDPR_MSG_SIZE_YUV+nBI_CURRENT_SRC_YUV:ud

// Give AYUV region addresses to address register
    mov (1) SRC_YUV_OFFSET<1>:ud 0x00400038*32:ud                               //Address registers contain starting addresses of two halves 
    
//Directly move the data to destination
    $for(0; <nY_NUM_OF_ROWS; 1) {
        mov (16) uwDEST_Y(%1)<1> r[SRC_YUV_OFFSET,%1*32+2]<8,4>:ub
        mov (16) uwDEST_U(%1)<1> r[SRC_YUV_OFFSET,%1*32+1]<8,4>:ub
        mov (16) uwDEST_V(%1)<1> r[SRC_YUV_OFFSET,%1*32+0]<8,4>:ub
    }        
    