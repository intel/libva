/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */


// Module name: PL8x4_Save_NV12.asm
//
// Save entire current planar frame data block of size 16x8
//---------------------------------------------------------------
//  Symbols needed to be defined before including this module
//
//      DWORD_ALIGNED_DEST:     only if DEST_Y, DEST_U, DEST_V data are DWord aligned
//      ORIX:
//---------------------------------------------------------------

#include "PL8x4_Save_NV12.inc"

    mov  (8) mMSGHDR<1>:ud      rMSGSRC<8;8,1>:ud

#if !defined(SAVE_UV_ONLY)
// Save current planar frame Y block data (16x8) -------------------------------

    mov  (2) mMSGHDR.0<1>:d     wORIX<2;2,1>:w          // Block origin
    mov  (1) mMSGHDR.2<1>:ud    nDPW_BLOCK_SIZE_Y:ud    // Block width and height (16x8)
#endif

//Use the mask to determine which pixels shouldn't be over-written
	and	(1)		acc0<1>:ud		udBLOCK_MASK<0;1,0>:ud		0x00FFFFFF:ud
	cmp.ge.f0.0	(1)		dNULLREG		acc0<0;1,0>:ud		0x00FFFFFF:ud	//Check if all pixels in the block need to be modified
	(f0.0)	jmpi WritePlanarToDataPort

//If mask is not all 1's, then load the entire 16x8 block
//so that only those bytes may be modified that need to be (using the mask)	
  send (8)	udSRC_Y(0)<1>	mMSGHDR	udDUMMY_NULL nDATAPORT_READ	nDPMR_MSGDSC+nDPR_MSG_SIZE_Y+nBI_DESTINATION_Y:ud		//16x8  
    
  asr  (1)	rMSGSRC.1<1>:ud	wORIY<0;1,0>:w	1:w	{ NoDDClr }	// U/V block origin should be half of Y's
  mov  (1)	rMSGSRC.2<1>:ud	nDPW_BLOCK_SIZE_UV:ud		{ NoDDChk }	// Block width and height (16x4)
  mov (8)  mMSGHDR<1>:ud       rMSGSRC<8;8,1>:ud //move message desrcptor to the message header
  send (8)	udSRC_U(0)<1>	mMSGHDR	udDUMMY_NULL nDATAPORT_READ nDPMR_MSGDSC+nDPR_MSG_SIZE_UV+nBI_DESTINATION_UV:ud     	                                                   
    	
//Restore the origin information
  mov (2)	rMSGSRC.0<1>:ud	wORIX<2;2,1>:w		// Block origin
  mov (1)	rMSGSRC.2<1>:ud	nDPW_BLOCK_SIZE_Y:ud		// Block width and height (16x8)
  mov (8) mMSGHDR<1>:ud       rMSGSRC<8;8,1>:ud //move message desrcptor to the message header	
	
//Merge the data
	mov  (1)	f0.1:uw			ubBLOCK_MASK_V:ub			//Load the mask on flag reg
	(f0.1)	mov	(8)	rMASK_TEMP<1>:uw	uwBLOCK_MASK_H:uw
	(-f0.1)	mov	(8)	rMASK_TEMP<1>:uw	0:uw  
    
//convert the mask from 16bits to 8bits by selecting every other bit
	mov (1) udMASK_TEMP1(0,0)<1> 0x00040001:ud 
	mov (1) udMASK_TEMP1(0,1)<1> 0x00400010:ud
	mov (1) udMASK_TEMP1(0,2)<1> 0x04000100:ud
	mov (1) udMASK_TEMP1(0,3)<1> 0x40001000:ud

//merge the loaded block with the current block
  $for(0,0; <nY_NUM_OF_ROWS; 2,1) {
  	mov	(1)	f0.1:uw		uwMASK_TEMP(0, %1)<0;1,0>
		(-f0.1)	mov  (16)	ubDEST_Y(0,%1*32)<2>		ubSRC_Y(0,%1*16)		

	  and.nz.f0.1 (8) wNULLREG uwMASK_TEMP(0,%1)<0;1,0> uwMASK_TEMP1(0,0) //change the mask by selecting every other bit
		(-f0.1)	mov  (8)	ubDEST_U(0, %2*16)<2>		ub2SRC_U(0, %1*8)<16;8,2>
		(-f0.1)	mov  (8)	ubDEST_V(0, %2*16)<2>		ub2SRC_U(0, %1*8+1)<16;8,2>
		
		mov	(1)	f0.1:uw		uwMASK_TEMP(0,1+%1)<0;1,0>
		(-f0.1)	mov  (16)	ubDEST_Y(0, (1+%1)*32)<2>	ubSRC_Y(0, (1+%1)*16)		
  
  }	 

WritePlanarToDataPort:
#if !defined(SAVE_UV_ONLY)
    $for(0,0; <nY_NUM_OF_ROWS; 2,1) {
            mov (16) mubMSGPAYLOAD(%2,0)<1>     ub2DEST_Y(%1)REGION(16,2)
            mov (16) mubMSGPAYLOAD(%2,16)<1>    ub2DEST_Y(%1+1)REGION(16,2)
    } 
    send (8)    dNULLREG    mMSGHDR   udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPW_MSG_SIZE_Y+nBI_DESTINATION_Y:ud
#endif
    
//** Save  8x4 packed U and V -----------------------------------------------------
// we could write directly wORIX to mMSGHDR and then execute asr on it, that way we could
// avoid using rMSGSRC as a buffer and have one command less in code, but it is unknown whether
//it is possible to do asr on mMSGHDR so we use rMSGSRC.
    mov (2)  rMSGSRC.0<1>:d    wORIX<2;2,1>:w             // Block origin
    asr (1)  rMSGSRC.1<1>:d    rMSGSRC.1<0;1,0>:d    1:w  // U/V block origin should be half of Y's
    mov (1)  rMSGSRC.2<1>:ud   nDPW_BLOCK_SIZE_UV:ud      // U/V block width and height (16x4)
    mov (8)  mMSGHDR<1>:ud     rMSGSRC<8;8,1>:ud

    $for(0,0; <nY_NUM_OF_ROWS;4,1) {
        mov (16) mubMSGPAYLOAD(%2,0)<2>     ub2DEST_U(%2)REGION(16,2) 
        mov (16) mubMSGPAYLOAD(%2,1)<2>     ub2DEST_V(%2)REGION(16,2) 
    }
    send (8)    dNULLREG    mMSGHDR    udDUMMY_NULL    nDATAPORT_WRITE    nDPMW_MSGDSC+nDPW_MSG_SIZE_UV+nBI_DESTINATION_UV:ud

// End of PL8x4_Save_NV12  

