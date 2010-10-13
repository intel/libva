/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: RGB16x8_Save_RGB.asm
//
// Save packed ARGB 444 frame data block of size 16x8
//
// To save 16x8 block (64x8 byte layout for ARGB8888) we need 2 send instructions
//  ---------
//  | 1 | 2 |
//  --------- 

#include "RGB16x8_Save_RGB.inc"

    shl (1) rMSGSRC.0<1>:d      wORIX<0;1,0>:w            2:w  { NoDDClr }             // H. block origin need to be quadrupled
    mov (1) rMSGSRC.1<1>:d      wORIY<0;1,0>:w                 { NoDDClr, NoDDChk }    // Block origin (1st quadrant)
    mov (1) rMSGSRC.2<1>:ud     nDPW_BLOCK_SIZE_ARGB:ud        { NoDDChk }             // Block width and height (32x8)

    mov (8) mMSGHDR<1>:ud       rMSGSRC<8;8,1>:ud

//Use the mask to determine which pixels shouldn't be over-written
    and (1)        acc0.0<1>:ud udBLOCK_MASK<0;1,0>:ud   0x00FFFFFF:ud
    cmp.ge.f0.0(1) dNULLREG     acc0.0<0;1,0>:ud         0x00FFFFFF:ud   //Check if all pixels in the block need to be modified
    (f0.0)  jmpi WriteARGBToDataPort

    //If mask is not all 1's, then load the entire 64x8 block
    //so that only those bytes may be modified that need to be (using the mask)

    // Load first block 16x8 packed ARGB 444 ---------------------------------------
    or (1)         acc0.0<1>:ud udBLOCK_MASK<0;1,0>:ud   0xFF00FF00:ud   //Check first block
    cmp.e.f0.0 (1) dNULLREG     acc0.0<0;1,0>:ud         0xFFFFFFFF:ud   
    (f0.0)  jmpi SkipFirstBlockMerge                                     //If full mask then skip this block

    send (8) udSRC_ARGB(0)<1>   mMSGHDR     udDUMMY_NULL    nDATAPORT_READ    nDPMR_MSGDSC+nDPR_MSG_SIZE_ARGB+nBI_DESTINATION_RGB:ud
    mov  (8) mMSGHDR<1>:ud      rMSGSRC<8;8,1>:ud

    //Merge the data
    mov (1)           f0.0:uw             ubBLOCK_MASK_V:ub    //Load the mask on flag reg
    (f0.0)  mov (8)   rMASK_TEMP<1>:uw    uwBLOCK_MASK_H:uw    //use sel instruction - vK
    (-f0.0) mov (8)   rMASK_TEMP<1>:uw    0:uw

    $for(0, 0; <nY_NUM_OF_ROWS; 1, 2) {               //take care of the lines in the block, they are different in the src and dest
        mov (1)             f0.1:uw                   uwMASK_TEMP(0,%1)<0;1,0>
        (-f0.1) mov (8)     udDEST_ARGB(%2)<1>        udSRC_ARGB(%1) 
    }

SkipFirstBlockMerge:
    // Load second block 16x8 packed ARGB 444 ---------------------------------------
    or (1)         acc0.0<1>:ud udBLOCK_MASK<0;1,0>:ud   0xFF0000FF:ud   //Check second block
    cmp.e.f0.0 (1) dNULLREG     acc0.0<0;1,0>:ud         0xFFFFFFFF:ud   
    (f0.0)  jmpi WriteARGBToDataPort                                     //If full mask then skip this block

    add  (1) mMSGHDR.0<1>:d     rMSGSRC.0<0;1,0>:d       32:d     // Point to 2nd part
    send (8) udSRC_ARGB(0)<1>   mMSGHDR    udDUMMY_NULL  nDATAPORT_READ    nDPMR_MSGDSC+nDPR_MSG_SIZE_ARGB+nBI_DESTINATION_RGB:ud
    mov  (8) mMSGHDR<1>:ud      rMSGSRC<8;8,1>:ud                 // Point to 1st part again

    //Merge the data
    mov (1)           f0.0:uw             ubBLOCK_MASK_V:ub    //Load the mask on flag reg
    (f0.0)  shr (8)   rMASK_TEMP<1>:uw    uwBLOCK_MASK_H:uw    8:uw    //load the mask for second block
    (-f0.0) mov (8)   rMASK_TEMP<1>:uw    0:uw

    $for(0, 1; <nY_NUM_OF_ROWS; 1, 2) {               //take care of the lines in the block, they are different in the src and dest
        mov (1)             f0.1:uw                   uwMASK_TEMP(0,%1)<0;1,0>
        (-f0.1) mov (8)     udDEST_ARGB(%2)<1>        udSRC_ARGB(%1) 
    }

WriteARGBToDataPort:
    // Move packed data to MRF and output
    $for(0; <nY_NUM_OF_ROWS; 1) {
        mov (8) mudMSGPAYLOAD(%1)<1>       udDEST_ARGB(%1*2)
    }
    send (8)    dNULLREG    mMSGHDR   udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPW_MSG_SIZE_ARGB+nBI_DESTINATION_RGB:ud

    mov  (8)    mMSGHDR<1>:ud         rMSGSRC<8;8,1>:ud
    add  (1)    mMSGHDR.0<1>:d        rMSGSRC.0<0;1,0>:d       32:d   // Point to 2nd part
    $for(0; <nY_NUM_OF_ROWS; 1) {
        mov (8) mudMSGPAYLOAD(%1)<1>       udDEST_ARGB(%1*2+1)
    }
    send (8)    dNULLREG    mMSGHDR   udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPW_MSG_SIZE_ARGB+nBI_DESTINATION_RGB:ud

// End of RGB16x8_Save_RGB
