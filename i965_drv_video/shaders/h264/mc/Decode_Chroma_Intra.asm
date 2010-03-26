/*
 * Decode both intra chroma blocks
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__DECODE_CHROMA_INTRA__)		// Make sure this is only included once
#define __DECODE_CHROMA_INTRA__

// Module name: Decode_Chroma_Intra.asm
//
// Decode both intra chroma blocks
//

decode_Chroma_Intra:
#ifndef MONO
    #include "load_Intra_Ref_UV.asm"	// Load intra U/V reference data
    #include "intra_Pred_Chroma.asm"	// Intra predict chroma blocks
    #include "add_Error_UV.asm"			// Add error data to predicted U/V data blocks
#endif	// !defined(MONO)
    #include "save_8x8_UV.asm"			// Save to destination U/V frame surface

	RETURN
// End of Decode_Chroma_Intra

#endif	// !defined(__DECODE_CHROMA_INTRA__)
