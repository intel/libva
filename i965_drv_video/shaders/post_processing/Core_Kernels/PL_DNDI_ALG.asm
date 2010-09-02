/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

#define DI_ENABLE

    #include "DNDI.inc"

    #ifdef DI_ONLY
		#undef  nSMPL_RESP_LEN
		#define nSMPL_RESP_LEN          nSMPL_RESP_LEN_DI               // set the number of GRF 
	#else
		#undef  nSMPL_RESP_LEN
		#define nSMPL_RESP_LEN          nSMPL_RESP_LEN_DNDI               // set the number of GRF 
	#endif
	
    #undef  nDPW_BLOCK_SIZE_HIST
    #define nDPW_BLOCK_SIZE_HIST    nBLOCK_WIDTH_4+nBLOCK_HEIGHT_1    // HIST Block Size for Write is 4x2
    #undef  nDPW_BLOCK_SIZE_DN
    #define nDPW_BLOCK_SIZE_DN      nBLOCK_WIDTH_16+nBLOCK_HEIGHT_4   // DN Block Size for Write is 16x4
    
////////////////////////////////////// Run the DN Algorithm ///////////////////////////////////////
    #include "DNDI_Command.asm"

////////////////////////////////////// Rearrange for Internal Planar //////////////////////////////

////////////////////////////////////// Save the STMM Data for Next Run /////////////////////////
    // Write STMM to memory
    shr (1)     rMSGSRC.0<1>:ud        wORIX<0;1,0>:w            1:w  NODDCLR_NODDCHK             // X origin / 2
    mov (1)     rMSGSRC.1<1>:ud        wORIY<0;1,0>:w                 NODDCLR_NODDCHK    // Y origin
    mov (1)     rMSGSRC.2<1>:ud        nDPW_BLOCK_SIZE_STMM:ud        NODDCHK             // block width and height (8x4)
    mov (8)     mudMSGHDR_STMM(0)<1>   rMSGSRC.0<8;8,1>:ud               // message header   
    mov (8)     mudMSGHDR_STMM(1)<1>   udRESP(nDI_STMM_OFFSET,0)         // Move STMM to MRF 
    send (8)    dNULLREG               mMSGHDR_STMM              udDUMMY_NULL    nDATAPORT_WRITE     nDPMW_MSGDSC+nDPMW_MSG_LEN_STMM+nBI_STMM_HISTORY_OUTPUT:ud      

#ifdef DI_ONLY
#else

////////////////////////////////////// Save the History Data for Next Run /////////////////////////
    #include "DI_Hist_Save.asm"

////////////////////////////////////// Save the DN Curr Frame for Next Run ////////////////////////
    
	//set the save DN parameters
    mov (2)     rMSGSRC.0<1>:ud        wORIX<2;2,1>:w              NODDCLR             // X origin and Y origin
    mov (1)     rMSGSRC.2<1>:ud        nDPW_BLOCK_SIZE_DN:ud       NODDCLR_NODDCHK     // block width and height (16x4)
    mov (8)     mudMSGHDR_DN(0)<1>     rMSGSRC.0<8;8,1>:ud                     
	
    // check top/bottom field first
    cmp.e.f0.0 (1)  null<1>:w               ubTFLD_FIRST<0;1,0>:ub     1:w
    (f0.0) jmpi (1) TOP_FIELD_FIRST

BOTTOM_FIELD_FIRST:
    $for (0,0; <nY_NUM_OF_ROWS/2; 2,1) {
        mov (4)     mudMSGHDR_DN(1,%1*4)<1>     udRESP(nDI_CURR_2ND_FIELD_LUMA_OFFSET,%2*4)<4;4,1> // 2nd field luma from current frame (line 0,2)
    }
    $for (0,0; <nY_NUM_OF_ROWS/2; 2,1) {
        mov (4)     mudMSGHDR_DN(1,%1*4+4)<1>   udRESP(nDI_CURR_FRAME_LUMA_OFFSET+%2,4)<4;4,1> // 1st field luma from current frame (line 1,3)
    }
	
    jmpi (1) SAVE_DN_CURR
    
TOP_FIELD_FIRST:
    $for (0,0; <nY_NUM_OF_ROWS/2; 2,1) {
        mov (4)     mudMSGHDR_DN(1,%1*4)<1>     udRESP(nDI_CURR_FRAME_LUMA_OFFSET+%2,0)<4;4,1> // 2nd field luma from current frame (line 0,2)
    }
    $for (0,0; <nY_NUM_OF_ROWS/2; 2,1) {
        mov (4)     mudMSGHDR_DN(1,%1*4+4)<1>   udRESP(nDI_CURR_2ND_FIELD_LUMA_OFFSET,%2*4)<4;4,1> // 1st field luma from current frame (line 1,3)
    }
	
SAVE_DN_CURR:
    send (8)    dNULLREG    mMSGHDR_DN   udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPMW_MSG_LEN_PL_DN_DI+nBI_DESTINATION_Y:ud
#endif

// Save Processed frames
#include "DI_Save_PA.asm"      



