/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

/////////////////////////////////////////////////////////////////////////////////
// Multiple_Loop.asm


// This lable is for satisfying component kernel build.
// DL will remove this label and reference the real one in Multiple_Loop_Head.asm.
#if defined(COMPONENT)
VIDEO_PROCESSING_LOOP:
#endif


//===== Possible build flags for component kernels
// 1) INC_SCALING
// 2) INC_BLENDING
// 3) INC_BLENDING and INC_SCALING
// 4) (no flags)


#define MxN_MULTIPLE_BLOCKS

//------------------------------------------------------------------------------
#if defined(MxN_MULTIPLE_BLOCKS)
// Do Multiple Block Processing ------------------------------------------------

	// The 1st block has been processed before entering the loop

	// Processed all blocks?
	add.z.f0.0	(1)	wNUM_BLKS:w	wNUM_BLKS:w	-1:w

	// Reached multi-block width?
	add			(1)	wORIX:w		wORIX:w		16:w
	cmp.l.f0.1	(1)	null:w		wORIX:w	wFRAME_ENDX:w	// acc0.0 has wORIX

	#if defined(INC_SCALING)
	// Update SRC_VID_H_ORI for scaling
		mul	(1)	REG(r,nTEMP0):f		fVIDEO_STEP_X:f		16.0:f
		add	(1)	fSRC_VID_H_ORI:f	REG(r,nTEMP0):f		fSRC_VID_H_ORI:f
	#endif

	#if defined(INC_BLENDING)
	// Update SRC_ALPHA_H_ORI for blending
		mul	(1)	REG(r,nTEMP0):f		fALPHA_STEP_X:f		16.0:f
		add	(1)	fSRC_ALPHA_H_ORI:f	REG(r,nTEMP0):f		fSRC_ALPHA_H_ORI:f
	#endif

	(f0.0)jmpi	(1)	END_VIDEO_PROCESSING	// All blocks are done - Exit loop

	(f0.1)jmpi	(1)	VIDEO_PROCESSING_LOOP	// If not the end of row, goto the beginning of the loop

	//If end of row, restart Horizontal offset and calculate Vertical offsets next row.
	mov	(1)		wORIX:w		wCOPY_ORIX:w
	add	(1)		wORIY:w		wORIY:w			8:w

	#if defined(INC_SCALING)
	// Update SRC_VID_H_ORI and SRC_VID_V_ORI for scaling
		mov	(1)		fSRC_VID_H_ORI:f	fFRAME_VID_ORIX:f	// Reset normalised X origin to 0 for video and alpha
		mul	(1)		REG(r,nTEMP0):f		fVIDEO_STEP_Y:f		8.0:f
		add	(1)		fSRC_VID_V_ORI:f	REG(r,nTEMP0):f		fSRC_VID_V_ORI:f
	#endif

	#if defined(INC_BLENDING)
	// Update SRC_ALPHA_H_ORI and SRC_ALPHA_V_ORI for blending
		mov	(1)		fSRC_ALPHA_H_ORI:f	fFRAME_ALPHA_ORIX:f	// Reset normalised X origin to 0 for video and alpha
		mul	(1)		REG(r,nTEMP0):f		fALPHA_STEP_Y:f		8.0:f
		add	(1)		fSRC_ALPHA_V_ORI:f	REG(r,nTEMP0):f		fSRC_ALPHA_V_ORI:f
	#endif

	jmpi (1)	VIDEO_PROCESSING_LOOP	// Continue Loop

END_VIDEO_PROCESSING:
	nop

#endif
END_THREAD	// End of Thread