/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */


// Module name: PL8x8_Save_P208.asm
//
// Save entire current planar frame data block of size 16x8
//---------------------------------------------------------------
//  Symbols needed to be defined before including this module
//
//      DWORD_ALIGNED_DEST:     only if DEST_Y, DEST_U, DEST_V data are DWord aligned
//      ORIX:
//---------------------------------------------------------------

#include "PL8x8_Save_P208.inc"

    mov  (8) mMSGHDR<1>:ud      rMSGSRC<8;8,1>:ud

#if !defined(SAVE_UV_ONLY)
// Save current planar frame Y block data (16x8) -------------------------------

    mov  (2) mMSGHDR.0<1>:d     wORIX<2;2,1>:w          // Block origin
    mov  (1) mMSGHDR.2<1>:ud    nDPW_BLOCK_SIZE_Y:ud    // Block width and height (16x8)

WritePlanarToDataPort:
    $for(0,0; <nY_NUM_OF_ROWS; 2,1) {
            mov (16) mubMSGPAYLOAD(%2,0)<1>     ub2DEST_Y(%1)REGION(16,2)
            mov (16) mubMSGPAYLOAD(%2,16)<1>    ub2DEST_Y(%1+1)REGION(16,2)
    } 
    send (8)    dNULLREG    mMSGHDR   udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPW_MSG_SIZE_Y+nBI_DESTINATION_Y:ud
#endif
    
//** Save  8x8 packed U and V -----------------------------------------------------
// we could write directly wORIX to mMSGHDR and then execute asr on it, that way we could
// avoid using rMSGSRC as a buffer and have one command less in code, but it is unknown whether
//it is possible to do asr on mMSGHDR so we use rMSGSRC.
    mov (2)  rMSGSRC.0<1>:d    wORIX<2;2,1>:w             // Block origin
                                                                                                        
    mov (1)  rMSGSRC.2<1>:ud   nDPW_BLOCK_SIZE_UV:ud      // U/V block width and height (16x4)
    mov (8)  mMSGHDR<1>:ud     rMSGSRC<8;8,1>:ud

    $for(0,0; <nY_NUM_OF_ROWS;2,1) {
        mov (16) mubMSGPAYLOAD(%2,0)<2>     ub2DEST_U(%2)REGION(16,2) 
        mov (16) mubMSGPAYLOAD(%2,1)<2>     ub2DEST_V(%2)REGION(16,2) 
    }
    send (8)    dNULLREG    mMSGHDR    udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPW_MSG_SIZE_UV+nBI_DESTINATION_UV:ud

//End of PL8x8_Save_P208.asm  

