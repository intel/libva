/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: PL8x8_Save_PA.asm
//
// Save planar YUV422 to packed YUV422 format data
//
// Note: SRC_* must reference to regions with data type "BYTE"
//               in order to save to byte-aligned byte location

#include "PL8x8_Save_PA.inc"

    add (4) pCF_Y_OFFSET<1>:uw   ubDEST_CF_OFFSET<4;4,1>:ub   nDEST_YUV_REG*nGRFWIB:w    // Initial Y,U,V offset in YUV422 block

    // Pack Y
    $for(0; <nY_NUM_OF_ROWS; 1) {
        mov (16) r[pCF_Y_OFFSET, %1*nGRFWIB]<2>    ubSRC_Y(0,%1*32)
    }

    // Pack U/V
    $for(0; <nUV_NUM_OF_ROWS; 1) {
        mov (8)  r[pCF_U_OFFSET, %1*nGRFWIB]<4>    ubSRC_U(0, %1*16)
        mov (8)  r[pCF_V_OFFSET, %1*nGRFWIB]<4>    ubSRC_V(0, %1*16)
    }

    shl (1) rMSGSRC.0<1>:d      wORIX<0;1,0>:w            1:w  { NoDDClr }             // H. block origin need to be doubled
    mov (1) rMSGSRC.1<1>:d      wORIY<0;1,0>:w                 { NoDDClr, NoDDChk }    // Block origin
    mov (1) rMSGSRC.2<1>:ud     nDPW_BLOCK_SIZE_YUV:ud         { NoDDChk }             // Block width and height (32x8)

    mov (8) mMSGHDR<1>:ud       rMSGSRC<8;8,1>:ud

//Use the mask to determine which pixels shouldn't be over-written
    and (1)        acc0.0<1>:ud udBLOCK_MASK<0;1,0>:ud   0x00FFFFFF:ud
    cmp.ge.f0.0(1) dNULLREG     acc0.0<0;1,0>:ud         0x00FFFFFF:ud   //Check if all pixels in the block need to be modified
    (f0.0)  jmpi WritePackedToDataPort

    //If mask is not all 1's, then load the entire 32x8 block
    //so that only those bytes may be modified that need to be (using the mask)

    // Load 32x8 packed YUV 422 ----------------------------------------------------
    send (8) udSRC_YUV(0)<1>    mMSGHDR     udDUMMY_NULL    nDATAPORT_READ    nDPMR_MSGDSC+nDPR_MSG_SIZE_YUV+nBI_DESTINATION_YUV:ud
    mov  (8) mMSGHDR<1>:ud      rMSGSRC<8;8,1>:ud

    //Merge the data
    mov (1)           f0.0:uw             ubBLOCK_MASK_V:ub    //Load the mask on flag reg
    (f0.0)  mov (8)   rMASK_TEMP<1>:uw    uwBLOCK_MASK_H:uw
    (-f0.0) mov (8)   rMASK_TEMP<1>:uw    0:uw

    // Destination is Byte aligned
    $for(0; <nY_NUM_OF_ROWS; 1) {
        mov (1)             f0.1:uw                   uwMASK_TEMP(0,%1)<0;1,0>
        (-f0.1) mov (16)    uwDEST_YUV(%1)<1>         uwSRC_YUV(%1)        //check the UV merge - vK
    }

WritePackedToDataPort:
    //  Packed YUV data are stored in one of the I/O regions before moving to MRF
    //  Note: This is necessary since indirect addressing is not supported for MRF. 
    //  Packed data block should be saved as 32x8 pixel block
    $for(0; <nY_NUM_OF_ROWS; 1) {
        mov (8) mudMSGPAYLOAD(%1)<1>       udDEST_YUV(%1)REGION(8,1)
    }
    send (8)    dNULLREG    mMSGHDR   udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPW_MSG_SIZE_YUV+nBI_DESTINATION_YUV:ud

// End of PL8x8_Save_PA
