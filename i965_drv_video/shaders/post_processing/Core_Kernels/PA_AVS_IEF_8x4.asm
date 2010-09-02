/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

//---------- PA_AVS_IEF_8x4.asm ----------

#include "AVS_IEF.inc"

//------------------------------------------------------------------------------
// 2 sampler reads for 8x8 YUV packed
//------------------------------------------------------------------------------
#include "PA_AVS_IEF_Sample.asm"

//------------------------------------------------------------------------------
// Unpacking sampler data to 4:2:0 internal planar 
//------------------------------------------------------------------------------
#include "PA_AVS_IEF_Unpack_8x4.asm"

//------------------------------------------------------------------------------
