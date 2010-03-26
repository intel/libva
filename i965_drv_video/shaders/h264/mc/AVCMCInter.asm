/*
 * All inter-prediction macroblock kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: AVCMCInter.asm

#ifdef INTERLABEL
#undef INTERLABEL
#endif

#if defined(MBAFF)
//				< MBaff_Motion >
#define INTERLABEL(x)	x##_##MBF
#elif defined(FIELD)
//				< FieldMB_Motion >
#define INTERLABEL(x)	x##_##FLD
#else // FRAME
//				< FrameMB_Motion >
#define INTERLABEL(x)	x##_##FRM
#endif
//
// Decoding an inter-prediction macroblock (conditional compile)
//	-DMBAFF : MBAff picture MB
//	-DFRAME : Frame picture MB
//	-DFIELD : Field picture MB
//	-DMBAFF -DMONO : MBAff mono picture MB
//	-DFRAME -DMONO : Frame mono picture MB
//	-DFIELD -DMONO : Field mono picture MB


//#if !defined(__AVCMCInter__)		// Make sure this is only included once
//#define __AVCMCInter__


// TODO: header files need to be in sync with intra prediction
#include "header.inc"
#include "inter_Header.inc"

// TODO: Kernel names for mono cases
#if defined(MBAFF)
.kernel MBAff_Motion
MBAFF_MB:
#elif defined(FIELD)
.kernel FieldMB_Motion
FIELD_MB:
#else // Frame
.kernel FrameMB_Motion
FRAME_MB:
#endif

#ifdef _DEBUG
// WA for FULSIM so we'll know which kernel is being debugged
#if defined(MBAFF)
mov (1) acc0:ud 0x0aaa55a5:ud
#elif defined(FIELD)
mov (1) acc0:ud 0x0baa55a5:ud
#else // Frame
mov (1) acc0:ud 0x0caa55a5:ud
#endif
#endif


#ifdef SW_SCOREBOARD
    CALL(scoreboard_start_inter,1)
#endif

	mov (8)		gMSGSRC<1>:ud		r0.0<8;8,1>:ud		// Initialize message header payload with R0
	
	and (1)		gwMBTYPE<1>			gMBTYPE:ub						nMBTYPE_MASK:w		// MB type
	shl (2)		gX<1>:w				gORIX<2;2,1>:ub					4:w // Convert MB origin to pixel unit
	
//	#include "process_inter16x16.asm"					// Handle B_L0_16x16 case with zero MVs and weighted pred off.
	// In the case of B_L0_16x16 with zero MVs and weighted pred off, the kernel jumps to INTERLABEL(EXIT_LOOP).
	
INTERLABEL(INIT_MBPARA):
	#include "initialize_MBPara.asm"
	
	
    //========================= BEGIN - LOOP_SUBMB ===========================
	mov (1)		gLOOP_SUBMB:uw		0:uw				// 0, 2, 4, 6
INTERLABEL(LOOP_SUBMB):

	//========================== BEGIN - LOOP_DIR ============================
	// Prediction flag (gPREDFLAG - 0:Pred_L0, 1:Pred_L1, 2:BiPred)
	asr (1)		gPREDFLAG:w			gSUBMB_MODE:ub					gLOOP_SUBMB:uw
	mov (1)		gLOOP_DIR:uw		1:uw				// 1, 0
	and (1)		gPREDFLAG:w			gPREDFLAG:w						0x3:w
INTERLABEL(LOOP_DIR):

	cmp.e.f0.0 (1) null:w			gLOOP_DIR:w						gPREDFLAG:w	
	(f0.0) jmpi	INTERLABEL(LOOP_DIR_CONTINUE)	
	
    // Get binding table index 
    // & reference picture parity (gREFPARITY - 0:top, 0x100:bottom, x:frame)
    // & address of interpolation result
    cmp.e.f0.1 (1) null:w			gLOOP_DIR:w						1:w
    (f0.1) mov (1)		gpINTP:ud			nOFFSET_INTP0:ud						{NoDDClr} //
    (f0.1) and (1)		gBIDX:w				r[pBIDX]:ub						0x7f:w	{NoDDChk} //
    (-f0.1) mov (1)		gpINTP:ud			nOFFSET_INTP1:ud						{NoDDClr} //
	(-f0.1) and (1)		gBIDX:w				r[pBIDX,4]:ub					0x7f:w	{NoDDChk} //
#if defined(MBAFF) || defined(FIELD)
    (f0.1) and (1)		gREFPARITY:w				r[pBIDX]:ub						0x80:w
    (-f0.1) and (1)		gREFPARITY:w				r[pBIDX,4]:ub					0x80:w
    shl (1)		gREFPARITY:w		gREFPARITY<0;1,0>:w				1:w
#endif

	// Sub MB shape
	asr (1)		gSHAPETEMP:w		gSUBMB_SHAPE:ub					gLOOP_SUBMB:w
	
    // Chroma MV adjustment & Set message descriptor for frame/field read
#if defined(MBAFF)
	#include "chromaMVAdjust.asm" 
    and.nz.f0.0 (1) null:uw			gFIELDMBFLAG:ub					nFIELDMB_MASK:uw
    (f0.0) add (1) gD0:ud			gBIDX:uw						nDWBRMSGDSC_SC_TF:ud
    (-f0.0) add (1)	gMSGDSC_R:ud	gBIDX:uw						nDWBRMSGDSC_SC:ud
    (f0.0) add (1) gMSGDSC_R:ud		gD0:ud							gREFPARITY:uw
#elif defined(FIELD)
	#include "chromaMVAdjust.asm" 
    add (1)		gMSGDSC_R:ud		gBIDX:uw						nDWBRMSGDSC_SC_TF:ud
    add (1)		gMSGDSC_R:ud		gMSGDSC_R:ud					gREFPARITY:uw
#else // FRAME
	add (1)		gMSGDSC_R:ud		gBIDX:uw						nDWBRMSGDSC_SC:ud
#endif

	and.nz.f0.1 (1) null:w			gSHAPETEMP:w					3:w	
	(f0.1) jmpi INTERLABEL(PROCESS4x4)
	
	//======================== BEGIN - PROCESS 8x8 ===========================
	
	// Reference block load
	#include "loadRef_Y_16x13.asm"
#ifndef MONO
#if defined(MBAFF) || defined(FIELD)
	add (1)		r[pMV,2]:w			r[pMV,2]:w						gCHRMVADJ:w
#endif
	#include "loadRef_C_10x5.asm"
#endif

	// Interpolation
	//CALL_INTER(INTERLABEL(Interpolate_Y_8x8_Func), 1)
	#include "interpolate_Y_8x8.asm"
#ifndef MONO
	//CALL_INTER(INTERLABEL(Interpolate_C_4x4_Func), 1)
	#include "interpolate_C_4x4.asm"
#endif

	jmpi INTERLABEL(ROUND_SHIFT_C)
	//========================= END - PROCESS 8x8 ============================
	
	//======================== BEGIN - LOOP_SUBMBPT ==========================
INTERLABEL(PROCESS4x4):

	mov (1)		gLOOP_SUBMBPT:uw	4:uw				// 4, 3, 2, 1
INTERLABEL(LOOP_SUBMBPT):

	// Reference block load
	#include "loadRef_Y_16x9.asm"
#ifndef MONO
#if defined(MBAFF) || defined(FIELD)
	add (1)		r[pMV,2]:w			r[pMV,2]:w						gCHRMVADJ:w
#endif
	#include "loadRef_C_6x3.asm"
#endif

	// Interpolation
	#include "interpolate_Y_4x4.asm"
#ifndef MONO
	#include "interpolate_C_2x2.asm"
#endif
	
	cmp.e.f0.0 (1) null:w			gLOOP_SUBMBPT:uw				3:w
	add.z.f0.1 (1) gLOOP_SUBMBPT:uw gLOOP_SUBMBPT:uw				-1:w
	add (1)		pMV:w				pMV:w							8:w	
	(-f0.0) add (1)	gpINTP:ud		gpINTP:ud						0x00080008:ud	// 8 & 8
	(f0.0) add (1) gpINTP:ud		gpINTP:ud						0x00180038:ud	// 24 & 56
	(-f0.1) jmpi INTERLABEL(LOOP_SUBMBPT)
    
    cmp.e.f0.1	null:w				gLOOP_DIR:w						1:w
    add (1)		pMV:w				pMV:w							-32:w
    (f0.1) mov (1) gpINTP:ud		nOFFSET_INTP0:ud
    (-f0.1) mov (1) gpINTP:ud		nOFFSET_INTP1:ud

	mov (1)		pRESULT:uw					gpINTPC:uw
    
	//========================= END - LOOP_SUBMBPT ===========================
    
INTERLABEL(ROUND_SHIFT_C):
    
#ifndef MONO
	#include "roundShift_C_4x4.asm"
#endif

INTERLABEL(LOOP_DIR_CONTINUE):
	
	add.nz.f0.1 (1) gLOOP_DIR:uw	gLOOP_DIR:uw					-1:w
	add (1)		pMV:w				pMV:w							4:w
    (-f0.1) jmpi INTERLABEL(LOOP_DIR)
    //=========================== END - LOOP_DIR =============================

INTERLABEL(Weighted_Prediction):
	#include "weightedPred.asm"
	
	and.z.f0.1 (16)	null<1>:w		gLOOP_SUBMB<0;1,0>:uw			2:w

	#include "recon_Y_8x8.asm"
#ifndef MONO
	#include "recon_C_4x4.asm"

	(-f0.1) add (1)	pERRORC:w		pERRORC:w						48:w
#endif

	cmp.e.f0.1 (1) null:w			gLOOP_SUBMB:uw					6:w
	add (1)		gLOOP_SUBMB:uw		gLOOP_SUBMB:uw					2:w

	add (1)		pWGT_BIDX:ud		pWGT_BIDX:ud					0x00100001:ud	// 12 & 1
	add (1)		pMV:w				pMV:w							gMVSTEP:w

    (-f0.1) jmpi INTERLABEL(LOOP_SUBMB)
    //========================== END - LOOP_SUBMB ============================
    
INTERLABEL(EXIT_LOOP):   
	#include "writeRecon_YC.asm"    

#ifdef SW_SCOREBOARD    
	wait	n0:ud		//	Now wait for scoreboard to response
    #include "Soreboard_update.asm"	// scorboard update function
#else
// Check for write commit first if SW scoreboard is disabled
	mov	(1)	gREG_WRITE_COMMIT_Y<1>:ud	gREG_WRITE_COMMIT_Y<0;1,0>:ud		// Make sure Y write is committed
	mov	(1)	gREG_WRITE_COMMIT_UV<1>:ud	gREG_WRITE_COMMIT_UV<0;1,0>:ud		// Make sure U/V write is committed
#endif

// Terminate the thread
//
    END_THREAD


//#include "Interpolate_Y_8x8_Func.asm"
//#include "Interpolate_C_4x4_Func.asm"
//#include "WeightedPred_Y_Func.asm"	
//#include "WeightedPred_C_Func.asm"	


.end_code

.end_kernel

        
//#endif	// !defined(__AVCMCInter__)
