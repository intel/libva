/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//	Kernel name: RoundShift_C_4x4.asm
//
//	Do (...+32)>>6 to 4x4 (NV12 8x4) interpolated chrominance data
//


//#if !defined(__RoundShift_C_4x4__)		// Make sure this is only included once
//#define __RoundShift_C_4x4__


	// TODO: Optimize using instruction compression
	add (16)	acc0<1>:w					r[pRESULT,0]<16;16,1>:w			32:w
	add (16)	acc1<1>:w					r[pRESULT,nGRFWIB]<16;16,1>:w	32:w
	asr.sat (16) r[pRESULT,0]<2>:ub			acc0<16;16,1>:w					6:w
	asr.sat (16) r[pRESULT,nGRFWIB]<2>:ub	acc1<16;16,1>:w					6:w
	

//#endif	// !defined(__RoundShift_C_4x4__)
