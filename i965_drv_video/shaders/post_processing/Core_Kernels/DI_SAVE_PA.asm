/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

    shl (1) rMSGSRC.0<1>:ud     wORIX<0;1,0>:w            1:w  NODDCLR             // H. block origin need to be doubled
    mov (1) rMSGSRC.1<1>:ud     wORIY<0;1,0>:w                 NODDCLR_NODDCHK    // Block origin
    mov (1) rMSGSRC.2<1>:ud     nDPW_BLOCK_SIZE_DI:ud          NODDCHK             // Block width and height (32x8)
    
	
	add (4) pCF_Y_OFFSET<1>:uw   ubDEST_CF_OFFSET<4;4,1>:ub   nDEST_YUV_REG*nGRFWIB:w    // Initial Y,U,V offset in YUV422 block

	// Pack 2nd field Y
    $for(0; <nY_NUM_OF_ROWS; 1) {
		mov     (16) r[pCF_Y_OFFSET, %1*nGRFWIB]<2>       ubRESP(nDI_PREV_FRAME_LUMA_OFFSET,%1*16)
    }
	// Pack 1st field Y
    $for(0; <nY_NUM_OF_ROWS; 1) {
		mov     (16) r[pCF_Y_OFFSET, %1+4*nGRFWIB]<2>       ubRESP(nDI_CURR_FRAME_LUMA_OFFSET,%1*16)
    }
	// Pack 2nd field U
    $for(0; <nUV_NUM_OF_ROWS; 1) {
        mov (8) r[pCF_U_OFFSET,   %1*nGRFWIB]<4>  ubRESP(nDI_PREV_FRAME_CHROMA_OFFSET,%1*16+1)<16;8,2>  //U pixels
    }
	 // Pack 1st field U
    $for(0; <nUV_NUM_OF_ROWS; 1) {
        mov (8) r[pCF_U_OFFSET,   %1+4*nGRFWIB]<4>  ubRESP(nDI_CURR_FRAME_CHROMA_OFFSET,%1*16+1)<16;8,2>  //U pixels
    }
	// Pack 2nd field V
    $for(0; <nUV_NUM_OF_ROWS; 1) {
        mov (8) r[pCF_V_OFFSET,   %1*nGRFWIB]<4>  ubRESP(nDI_PREV_FRAME_CHROMA_OFFSET,%1*16)<16;8,2>  //Vpixels
    }
	// Packs1st field V
    $for(0; <nUV_NUM_OF_ROWS; 1) {
        mov (8) r[pCF_V_OFFSET,   %1+4*nGRFWIB]<4>  ubRESP(nDI_CURR_FRAME_CHROMA_OFFSET,%1*16)<16;8,2>  //Vpixels
    }

    //save the previous frame
    mov (8) mMSGHDR<1>:ud       rMSGSRC<8;8,1>:ud
    $for(0; <4; 1) {
            mov (8) mudMSGPAYLOAD(%1)<1>  udDEST_YUV(%1)REGION(8,1)
    }
    send (8)    dNULLREG    mMSGHDR   udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPW_MSG_SIZE_DI+nBI_DESTINATION_1_YUV:ud

    //save the current frame
    mov (8) mMSGHDR<1>:ud       rMSGSRC<8;8,1>:ud
    $for(0; <4; 1) {
            mov (8) mudMSGPAYLOAD(%1)<1>  udDEST_YUV(%1+4)REGION(8,1)
    }
    send (8)    dNULLREG    mMSGHDR   udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPW_MSG_SIZE_DI+nBI_DESTINATION_2_YUV:ud
	
