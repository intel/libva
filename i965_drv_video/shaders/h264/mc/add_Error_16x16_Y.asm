/*
 * Add macroblock correction Y data blocks to predicted picture
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
        
// Module name: add_Error_16x16_Y.asm
//
// Add macroblock correction Y data blocks to predicted picture
//

//  Every line of predicted Y data is added to Y error data if CBP bit is set

    mov (1) PERROR_UD<1>:ud	0x10001*ERRBUF*GRFWIB+0x00100000:ud	// Pointers to first and second row of error block

    and.z.f0.1 (1)	NULLREG    REG_CBPCY	CBP_Y_MASK
    (f0.1) jmpi (1) End_add_Error_16x16_Y	// Skip all blocks

//  Block Y0
//
    $for(0,0; <8; 2,1) {
	add.sat (16)	DEC_Y(%1)<2>	r[PERROR,%2*GRFWIB]REGION(8,1):w	PRED_Y(%1)REGION(8,2) {Compr}
    }

//  Block Y1
//
    $for(0,0; <8; 2,1) {
	add.sat (16)	DEC_Y(%1,16)<2>		r[PERROR,%2*GRFWIB+0x80]REGION(8,1):w	PRED_Y(%1,16)REGION(8,2) {Compr}
    }

//  Block Y2
//
    $for(8,0; <16; 2,1) {
	add.sat (16)	DEC_Y(%1)<2>	r[PERROR,%2*GRFWIB+0x100]REGION(8,1):w	PRED_Y(%1)REGION(8,2) {Compr}
    }

//  Block Y3
//
    $for(8,0; <16; 2,1) {
	add.sat (16)	DEC_Y(%1,16)<2>		r[PERROR,%2*GRFWIB+0x180]REGION(8,1):w	PRED_Y(%1,16)REGION(8,2) {Compr}
    }

End_add_Error_16x16_Y:
    add (1) PERROR_UD<1>:ud	PERROR_UD:ud	0x01800180:ud	// Pointers to Y3 error block

//  End of add_Error_16x16_Y

