/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: PL9x5_PL16x8.asm

#include "Expansion.inc"

//------------------------------ Horizontal Upconversion -----------------------------
    $for (0; <nUV_NUM_OF_ROWS; 1) {
        avg.sat (16) uwDEST_U(0, %1*16)<1>    uwDEST_U(0, %1*16)<1;2,0>    uwDEST_U(0, %1*16)<1;2,1>
        avg.sat (16) uwDEST_V(0, %1*16)<1>    uwDEST_V(0, %1*16)<1;2,0>    uwDEST_V(0, %1*16)<1;2,1>
    }

// End of PL9x5_PL16x8