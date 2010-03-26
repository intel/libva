/*
 * Add macroblock correction UV data blocks to predicted picture        
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

#if !defined(__ADD_ERROR_UV__)		// Make sure this is only included once
#define __ADD_ERROR_UV__

// Module name: add_Error_UV.asm
//
// Add macroblock correction UV data blocks to predicted picture

// PERROR points to error block Y3 after decoding Y component

//	Update address register used in instruction compression
//

//  U component
//
    add (1) PERROR1<1>:w	PERROR:w	0x00010:w	// Pointers to next error row
    $for(0,0; <8; 2,1) {
	add.sat (16)	DEC_UV(%1)<4>	r[PERROR,%2*GRFWIB+0x80]REGION(8,1):w	PRED_UV(%1)REGION(8,4) {Compr}
    }

//  V component
//
    $for(0,0; <8; 2,1) {
	add.sat (16)	DEC_UV(%1,2)<4>	r[PERROR,%2*GRFWIB+0x100]REGION(8,1):w	PRED_UV(%1,2)REGION(8,4) {Compr}
    }

//  End of add_Error_UV

#endif	// !defined(__ADD_ERROR_UV__)
