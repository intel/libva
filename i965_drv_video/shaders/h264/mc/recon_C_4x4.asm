/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: Recon_C_4x4.asm
//
//  $Revision: 11 $
//  $Date: 10/03/06 5:28p $
//


//#if !defined(__RECON_C_4x4__)		// Make sure this is only included once
//#define __RECON_C_4x4__


	// TODO: Use instruction compression
	add.sat (4) r[pERRORC,0]<2>:ub			r[pERRORC,0]<4;4,1>:w			gubCPRED(0)<16;4,4>
	add.sat (4) r[pERRORC,128]<2>:ub		r[pERRORC,128]<4;4,1>:w			gubCPRED(0,2)<16;4,4>
	add.sat (4) r[pERRORC,32]<2>:ub			r[pERRORC,32]<4;4,1>:w			gubCPRED(1)<16;4,4>
	add.sat (4) r[pERRORC,128+32]<2>:ub		r[pERRORC,128+32]<4;4,1>:w		gubCPRED(1,2)<16;4,4>
	
	add.sat (4) r[pERRORC,16]<2>:ub			r[pERRORC,16]<4;4,1>:w			gubCPRED(0,16)<16;4,4>
	add.sat (4) r[pERRORC,128+16]<2>:ub		r[pERRORC,128+16]<4;4,1>:w		gubCPRED(0,18)<16;4,4>
	add.sat (4) r[pERRORC,48]<2>:ub			r[pERRORC,48]<4;4,1>:w			gubCPRED(1,16)<16;4,4>
	add.sat (4) r[pERRORC,128+48]<2>:ub		r[pERRORC,128+48]<4;4,1>:w		gubCPRED(1,18)<16;4,4>

	// Increase chroma error block offset	
#ifndef MONO
	add (1)		pERRORC:w			pERRORC:w						8:w
#endif

        
//#endif	// !defined(__RECON_C_4x4__)
