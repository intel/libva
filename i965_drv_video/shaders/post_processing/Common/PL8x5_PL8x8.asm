/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: PL8x5_PL8x8.asm

#include "Expansion.inc"

//------------------------------- Vertical Upconversion ------------------------------
    avg.sat (8) uwDEST_U(0, 3*16+8)<1>   uwDEST_U(0, 3*8)    uwDEST_U(0, (1+3)*8)    // Optimization
    avg.sat (8) uwDEST_V(0, 3*16+8)<1>   uwDEST_V(0, 3*8)    uwDEST_V(0, (1+3)*8)    // Optimization

    $for(nUV_NUM_OF_ROWS/2-2; >-1; -1) {
        mov     (8) uwDEST_U(0, (1+%1)*16)<1>    uwDEST_U(0, (1+%1)*8)
        avg.sat (8) uwDEST_U(0, %1*16+8)<1>   uwDEST_U(0, %1*8)    uwDEST_U(0, (1+%1)*8)

        mov     (8) uwDEST_V(0, (1+%1)*16)<1>    uwDEST_V(0, (1+%1)*8)
        avg.sat (8) uwDEST_V(0, %1*16+8)<1>   uwDEST_V(0, %1*8)    uwDEST_V(0, (1+%1)*8)
    }

// End of PL8x5_PL8x8
