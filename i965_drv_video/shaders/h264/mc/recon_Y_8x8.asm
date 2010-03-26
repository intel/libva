/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: Recon_Y_8x8.asm
//
//  $Revision: 10 $
//  $Date: 9/22/06 2:50p $
//


//#if !defined(__RECON_Y_8x8__)		// Make sure this is only included once
//#define __RECON_Y_8x8__


	add.sat (16)		r[pERRORY,0]<2>:ub			r[pERRORY,0]<16;16,1>:w				gubYPRED(0)
	add.sat (16)		r[pERRORY,nGRFWIB]<2>:ub	r[pERRORY,nGRFWIB]<16;16,1>:w		gubYPRED(1)
	add.sat (16)		r[pERRORY,nGRFWIB*2]<2>:ub	r[pERRORY,nGRFWIB*2]<16;16,1>:w		gubYPRED(2)
	add.sat (16)		r[pERRORY,nGRFWIB*3]<2>:ub	r[pERRORY,nGRFWIB*3]<16;16,1>:w		gubYPRED(3)
	
	add (1)				pERRORY:w					pERRORY:w							128:w

//#endif	// !defined(__RECON_Y_8x8__)
