/*
 * All MBAFF frame picture HWMC kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//	2857702934	// 0xAA551616 - GUID for Intra_16x16 luma prediction mode offsets
//    0    // Offset to Intra_16x16 luma prediction mode 0
//    9    // Offset to Intra_16x16 luma prediction mode 1
//   19    // Offset to Intra_16x16 luma prediction mode 2
//   42    // Offset to Intra_16x16 luma prediction mode 3
//	2857699336	// 0xAA550808 - GUID for Intra_8x8 luma prediction mode offsets
//    0    // Offset to Intra_8x8 luma prediction mode 0
//    5    // Offset to Intra_8x8 luma prediction mode 1
//   10    // Offset to Intra_8x8 luma prediction mode 2
//   26    // Offset to Intra_8x8 luma prediction mode 3
//   36    // Offset to Intra_8x8 luma prediction mode 4
//   50    // Offset to Intra_8x8 luma prediction mode 5
//   68    // Offset to Intra_8x8 luma prediction mode 6
//   85    // Offset to Intra_8x8 luma prediction mode 7
//   95    // Offset to Intra_8x8 luma prediction mode 8
//	2857698308	// 0xAA550404 - GUID for Intra_4x4 luma prediction mode offsets
//    0    // Offset to Intra_4x4 luma prediction mode 0
//    2    // Offset to Intra_4x4 luma prediction mode 1
//    4    // Offset to Intra_4x4 luma prediction mode 2
//   16    // Offset to Intra_4x4 luma prediction mode 3
//   23    // Offset to Intra_4x4 luma prediction mode 4
//   32    // Offset to Intra_4x4 luma prediction mode 5
//   45    // Offset to Intra_4x4 luma prediction mode 6
//   59    // Offset to Intra_4x4 luma prediction mode 7
//   66    // Offset to Intra_4x4 luma prediction mode 8
//	2857700364	// 0xAA550C0C - GUID for intra chroma prediction mode offsets
//    0    // Offset to intra chroma prediction mode 0
//   30    // Offset to intra chroma prediction mode 1
//   36    // Offset to intra chroma prediction mode 2
//   41    // Offset to intra chroma prediction mode 3

// Kernel name: AllAVCMBAFF.asm
//
// All MBAFF frame picture HWMC kernels merged into this file
//
//  $Revision: 1 $
//  $Date: 4/13/06 4:35p $
//

// ----------------------------------------------------
//  Main: AllAVCMBAFF
// ----------------------------------------------------

#define	ALLHWMC
#define	COMBINED_KERNEL

.kernel AllAVCMBAFF

    #include "Intra_PCM.asm"
    #include "Intra_16x16.asm"
    #include "Intra_8x8.asm"
    #include "Intra_4x4.asm"
    #include "scoreboard.asm"

	#define MBAFF
	#include "AVCMCInter.asm"

// End of AllAVCMBAFF

.end_kernel

