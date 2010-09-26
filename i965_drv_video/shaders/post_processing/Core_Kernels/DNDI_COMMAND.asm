/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Activate the DNDI send command
mov (8)     mudMSG_SMPL(0)<1>        rMSGSRC.0<8;8,1>:ud    NODDCLR         // message header
mov (1)     muwMSG_SMPL(1,4)<1>      wORIX<0;1,0>:w         NODDCLR_NODDCHK// horizontal origin
mov (1)     muwMSG_SMPL(1,12)<1>     wORIY<0;1,0>:w         NODDCLR_NODDCHK         // vertical origin
//mov (2)     muwMSG_SMPL(1,4)<2>      wORIX<2;2,1>:w       NODDCHK// problem during compile !! when using this line

send (8)    udRESP(0)<1>    mMSG_SMPL  udDUMMY_NULL   nSMPL_ENGINE    nSMPL_DI_MSGDSC+nSMPL_RESP_LEN+nBI_CURRENT_SRC_YUV_HW_DI:ud
