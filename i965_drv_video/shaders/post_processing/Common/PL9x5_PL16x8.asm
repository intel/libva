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

#define EXPAND_9x5
#include "Expansion.inc"

//------------------------------ Horizontal Upconversion -----------------------------
    $for (nUV_NUM_OF_ROWS-2; >-1; -1) {
        avg.sat (16) uwDEST_U(0, %1*16)<1>    uwDEST_U(0, %1*16)<1;2,0>    uwDEST_U(0, %1*16)<1;2,1>
        avg.sat (16) uwDEST_V(0, %1*16)<1>    uwDEST_V(0, %1*16)<1;2,0>    uwDEST_V(0, %1*16)<1;2,1>
    }

#undef 	nUV_NUM_OF_ROWS
#define nUV_NUM_OF_ROWS		8	//use packed version of all post-processing kernels

//------------------------------- Vertical Upconversion ------------------------------
    avg.sat (16) uwDEST_U(0, 3*32+16)<1>   uwDEST_U(0, 3*16)    uwDEST_U(0, (1+3)*16)
    avg.sat (16) uwDEST_V(0, 3*32+16)<1>   uwDEST_V(0, 3*16)    uwDEST_V(0, (1+3)*16)

    $for(nUV_NUM_OF_ROWS/2-2; >-1; -1) {
        mov     (16) uwDEST_U(0, (1+%1)*32)<1>    uwDEST_U(0, (1+%1)*16)
        avg.sat (16) uwDEST_U(0, %1*32+16)<1>   uwDEST_U(0, %1*16)    uwDEST_U(0, (1+%1)*16)

        mov     (16) uwDEST_V(0, (1+%1)*32)<1>    uwDEST_V(0, (1+%1)*16)
        avg.sat (16) uwDEST_V(0, %1*32+16)<1>   uwDEST_V(0, %1*16)    uwDEST_V(0, (1+%1)*16)
    }

// End of PL9x5_PL16x8
