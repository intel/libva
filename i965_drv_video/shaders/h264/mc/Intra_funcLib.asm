/*
 * Library of common modules shared among different intra prediction kernels
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: Intra_funcLib.asm
//
// Library of common modules shared among different intra prediction kernels
//
//  Note: Any sub-modules, if they are #included in more than one kernel,
//	  should be moved to this module.
//
#if defined(INTRA_16X16)
#undef INTRA_16X16
    #include "load_Intra_Ref_Y.asm"		// Load intra Y reference data
    #include "Decode_Chroma_Intra.asm"	// Decode chroma blocks
    #include "save_16x16_Y.asm"			// Save to destination Y frame surface
#elif defined(INTRA_8X8)
#undef INTRA_8X8
    #include "load_Intra_Ref_Y.asm"		// Load intra Y reference data
    #include "Decode_Chroma_Intra.asm"	// Decode chroma blocks
    #include "intra_Pred_8x8_Y.asm"		// Intra predict Intra_4x4 blocks
    #include "save_8x8_Y.asm"			// Save to destination Y frame surface
#elif defined(INTRA_4X4)
#undef INTRA_4X4
    #include "load_Intra_Ref_Y.asm"		// Load intra Y reference data
    #include "Decode_Chroma_Intra.asm"	// Decode chroma blocks
    #include "intra_Pred_4x4_Y_4.asm"	// Intra predict Intra_4x4 blocks
    #include "save_4x4_Y.asm"			// Save to destination Y frame surface
#else								// For all merged kernels
#endif

#ifdef SW_SCOREBOARD    
    #include "scoreboard_start_intra.asm"	// scorboard intra start function	
    #include "scoreboard_start_inter.asm"	// scorboard inter start function	
#endif	// SW_SCOREBOARD

// End of Intra_funcLib
