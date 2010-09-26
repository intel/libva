/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */


// Write denoise history to memory
shr (2)    rMSGSRC.0<1>:ud    wORIX<2;2,1>:w            2:w                       NODDCLR         // X,Y origin / 4
add (1)    rMSGSRC.0<1>:ud    rMSGSRC.0<0;1,0>:ud       uwSPITCH_DIV2<0;1,0>:uw   NODDCLR_NODDCHK// Add pitch to X origin
mov (1)    rMSGSRC.2<1>:ud    nDPW_BLOCK_SIZE_HIST:ud                             NODDCHK         // block width and height (4x2)

mov (8)    mMSGHDR_HIST<1>:ud      rMSGSRC.0<8;8,1>:ud                   // message header   
mov (2)    mudMSGHDR_HIST(1)<1>    udRESP(nNODI_HIST_OFFSET,0)<2;2,1>    // Move denoise history to MRF

send (8)   dNULLREG    mMSGHDR_HIST    udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPMW_MSG_LEN_HIST+nBI_STMM_HISTORY_OUTPUT:ud
