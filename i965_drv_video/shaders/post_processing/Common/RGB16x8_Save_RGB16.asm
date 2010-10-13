/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: RGB16x8_Save_RGB16.asm
//
// Save packed RGB565 frame data block of size 16x8
//
// To save 16x8 block (32x8 byte layout for RGB565) we need 1 send instruction
//  -----
//  | 1 |
//  ----- 

#include "RGB16x8_Save_RGB16.inc"

//convert 32 bit RGB to 16 bit RGB
    // Truncate A8R8G8B8 to A6R5G6B5 within byte.
    // That is keeping 5 MSB of R and B, and 6 MSB of G.

    $for (0, 0; <nY_NUM_OF_ROWS; 1, 2) {
        shr     uwCSC_TEMP(%1,0)<1>    ubDEST_ARGB(%2,0)<32;8,4>   3:w                // B >> 3

        shl (16) uwTEMP_RGB16(0)<1>    uwDEST_ARGB(%2,1)<16;8,2>   8:w                // R << 8
        and (16) uwTEMP_RGB16(0)<1>    uwTEMP_RGB16(0)             0xF800:uw
        or  (16) uwCSC_TEMP(%1,0)<1>   uwCSC_TEMP(%1,0)<16;16,1>   uwTEMP_RGB16(0)

        shr (16) uwTEMP_RGB16(0)<1>    uwDEST_ARGB(%2,0)<16;8,2>   5:w                // G >> 5
        and (16) uwTEMP_RGB16(0)<1>    uwTEMP_RGB16(0)             0x07E0:uw
        or  (16) uwCSC_TEMP(%1,0)<1>   uwCSC_TEMP(%1,0)<16;16,1>   uwTEMP_RGB16(0)
    }

    mov (2) rMSGSRC.0<1>:d      wORIX<2;2,1>:w                      // Block origin (1st quadrant)
    shl (1) rMSGSRC.0<1>:d      wORIX<0;1,0>:w              1:w     // H. block origin need to be doubled for byte offset
    mov (1) rMSGSRC.2<1>:ud     nDPW_BLOCK_SIZE_RGB16:ud            // Block width and height (32x8)
    mov (8) mMSGHDR<1>:ud       rMSGSRC<8;8,1>:ud

//Use the mask to determine which pixels shouldn't be over-written
    and (1)        acc0.0<1>:ud udBLOCK_MASK<0;1,0>:ud   0x00FFFFFF:ud
    cmp.ge.f0.0(1) dNULLREG     acc0.0<0;1,0>:ud         0x00FFFFFF:ud   //Check if all pixels in the block need to be modified
    (f0.0)  jmpi WriteRGB16ToDataPort

    //If mask is not all 1's, then load the entire 32x8 block
    //so that only those bytes may be modified that need to be (using the mask)

    // Load 32x8 packed RGB565 -----------------------------------------------------
    send (8) udSRC_RGB16(0)<1>  mMSGHDR     udDUMMY_NULL    nDATAPORT_READ    nDPMR_MSGDSC+nDPR_MSG_SIZE_RGB16+nBI_DESTINATION_RGB:ud
    mov (8) mMSGHDR<1>:ud       rMSGSRC<8;8,1>:ud

    //Merge the data
    mov (1)           f0.0:uw             ubBLOCK_MASK_V:ub    //Load the mask on flag reg
    (f0.0)  mov (8)   rMASK_TEMP<1>:uw    uwBLOCK_MASK_H:uw    //use sel instruction - vK
    (-f0.0) mov (8)   rMASK_TEMP<1>:uw    0:uw

    $for(0; <nY_NUM_OF_ROWS; 1) {
        mov (1)             f0.1:uw                   uwMASK_TEMP(0,%1)<0;1,0>
        (-f0.1) mov (16)    uwCSC_TEMP(%1)<1>         uwSRC_RGB16(%1)
    }

WriteRGB16ToDataPort:
    // Move packed data to MRF and output
    $for(0; <nY_NUM_OF_ROWS; 1) {
        mov (8) mudMSGPAYLOAD(%1)<1>       udCSC_TEMP(%1)
    }
    send (8)    dNULLREG    mMSGHDR   udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPW_MSG_SIZE_RGB16+nBI_DESTINATION_RGB:ud

// End of RGB16x8_Save_RGB16
