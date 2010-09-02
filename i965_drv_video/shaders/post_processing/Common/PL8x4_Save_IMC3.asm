/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: PL8x4_Save_IMC3.asm
//
// Save planar YUV420 frame data block of size 16x8

#include "PL8x4_Save_IMC3.inc"

//Use the mask to determine which pixels shouldn't be over-written
    and (1)        acc0.0<1>:ud udBLOCK_MASK<0;1,0>:ud   0x00FFFFFF:ud
    cmp.ge.f0.0(1) dNULLREG     acc0.0<0;1,0>:ud         0x00FFFFFF:ud   //Check if all pixels in the block need to be modified
    (f0.0)  jmpi WritePlanarToDataPort

    //If mask is not all 1's, then load the entire 16x8 block
    //so that only those bytes may be modified that need to be (using the mask)

    // Load 16x8 planar Y ----------------------------------------------------------
    mov  (2) rMSGSRC.0<1>:d     wORIX<2;2,1>:w          // Block origin
    mov  (1) rMSGSRC.2<1>:ud    nDPW_BLOCK_SIZE_Y:ud    // Block width and height (16x8)
    mov  (8) mMSGHDR<1>:ud      rMSGSRC<8;8,1>:ud
    send (8) udSRC_Y(0)<1>      mMSGHDR     udDUMMY_NULL    nDATAPORT_READ    nDPMR_MSGDSC+nDPR_MSG_SIZE_Y+nBI_DESTINATION_Y:ud
    // Load 8x4 planar U and V -----------------------------------------------------
    asr  (2) rMSGSRC.0<1>:d     wORIX<2;2,1>:w    1:w   // U/V block origin should be half of Y's
    mov  (1) rMSGSRC.2<1>:ud    nDPW_BLOCK_SIZE_UV:ud   // Block width and height (8x4)
    mov  (8) mMSGHDR<1>:ud      rMSGSRC<8;8,1>:ud
    send (8) udSRC_U(0)<1>      mMSGHDR     udDUMMY_NULL    nDATAPORT_READ    nDPMR_MSGDSC+nDPR_MSG_SIZE_UV+nBI_DESTINATION_U:ud
    mov  (8) mMSGHDR<1>:ud      rMSGSRC<8;8,1>:ud
    send (8) udSRC_V(0)<1>      mMSGHDR     udDUMMY_NULL    nDATAPORT_READ    nDPMR_MSGDSC+nDPR_MSG_SIZE_UV+nBI_DESTINATION_V:ud

    //expand U and V to be aligned on word boundary - Y remains in bytes
    $for (nUV_NUM_OF_ROWS/2-1; >-1; -1) {
        mov (16)  uwSRC_U(0, %1*16)<1>    ubSRC_U(0, %1*16)
        mov (16)  uwSRC_V(0, %1*16)<1>    ubSRC_V(0, %1*16)
    }

    //Merge the data
    mov (1)           f0.0:uw             ubBLOCK_MASK_V:ub    //Load the mask on flag reg
    (f0.0)  mov (8)   rMASK_TEMP<1>:uw    uwBLOCK_MASK_H:uw
    (-f0.0) mov (8)   rMASK_TEMP<1>:uw    0:uw

    // Destination is Word aligned
    $for(0; <nY_NUM_OF_ROWS; 2) {
        mov (1)             f0.1:uw                   uwMASK_TEMP(0,%1)<0;1,0>
        (-f0.1) mov (16)    ub2DEST_Y(0, %1*32)<2>    ubSRC_Y(0, %1*16)
        (-f0.1) mov (16)    ub2DEST_U(0, %1*8)<1>     ubSRC_U(0, %1*8)    //only works for Word aligned Byte data
        (-f0.1) mov (16)    ub2DEST_V(0, %1*8)<1>     ubSRC_V(0, %1*8)    //only works for Word aligned Byte data

        mov (1)             f0.1:uw                   uwMASK_TEMP(0,1+%1)<0;1,0>
        (-f0.1) mov (16)    ub2DEST_Y(0, 1+%1*32)<2>  ubSRC_Y(0, 1+%1*16)
    }

WritePlanarToDataPort:
#if !defined(SAVE_UV_ONLY)
// Save current planar frame Y block data (16x8) -------------------------------
    mov (2)     rMSGSRC.0<1>:d     wORIX<2;2,1>:w          // Block origin
    mov (1)     rMSGSRC.2<1>:ud    nDPW_BLOCK_SIZE_Y:ud    // Block width and height (16x8)
    mov (8)     mMSGHDR<1>:ud      rMSGSRC<8;8,1>:ud
    $for(0,0; <nY_NUM_OF_ROWS; 2,1) {
        mov(16) mubMSGPAYLOAD(%2,0)<1>     ub2DEST_Y(%1)REGION(16,2)
        mov(16) mubMSGPAYLOAD(%2,16)<1>    ub2DEST_Y(%1+1)REGION(16,2)
    }
    send (8)    dNULLREG    mMSGHDR   udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPW_MSG_SIZE_Y+nBI_DESTINATION_Y:ud
#endif
// Save U/V data block in planar format (8x4) ----------------------------------
    asr  (2)    rMSGSRC.0<1>:d     wORIX<2;2,1>:w    1:w   // U/V block origin should be half of Y's
    mov  (1)    rMSGSRC.2<1>:ud    nDPW_BLOCK_SIZE_UV:ud   // Block width and height (8x4)
    mov  (8)    mMSGHDR<1>:ud      rMSGSRC<8;8,1>:ud

// Save U picture data ---------------------------------------------------------
    mov (16)    mubMSGPAYLOAD(0,0)<1>      ub2DEST_U(0)REGION(16,2)   // U rows 0,1
    mov (16)    mubMSGPAYLOAD(0,16)<1>     ub2DEST_U(1)REGION(16,2)   // U rows 2,3
    send (8)    dNULLREG    mMSGHDR   udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPW_MSG_SIZE_UV+nBI_DESTINATION_U:ud
    mov  (8)    mMSGHDR<1>:ud      rMSGSRC<8;8,1>:ud

// Save V picture data ---------------------------------------------------------
    mov  (16)   mubMSGPAYLOAD(0,0)<1>      ub2DEST_V(0)REGION(16,2)   // V rows 0,1
    mov  (16)   mubMSGPAYLOAD(0,16)<1>     ub2DEST_V(1)REGION(16,2)   // V rows 2,3
    send (8)    dNULLREG    mMSGHDR   udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPW_MSG_SIZE_UV+nBI_DESTINATION_V:ud

// End of PL8x4_Save_IMC3
