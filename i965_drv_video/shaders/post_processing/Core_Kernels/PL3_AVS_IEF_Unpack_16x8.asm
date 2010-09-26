/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

//---------- PL3_AVS_IEF_Unpack_16x8.asm ----------
        
#ifdef AVS_OUTPUT_16_BIT	//Output is packed in AVYU format
// Move first 8x8 words of Y to dest GRF (as packed)
    mov (4) uwDEST_Y(0,1)<4>       uwAVS_RESPONSE(0,0)<4;4,1>                                      
    mov (4) uwDEST_Y(1,1)<4>       uwAVS_RESPONSE(0,8)<4;4,1>                                      
    mov (4) uwDEST_Y(4,1)<4>       uwAVS_RESPONSE(0,4)<4;4,1>                                    
    mov (4) uwDEST_Y(5,1)<4>       uwAVS_RESPONSE(0,12)<4;4,1>                                    
    mov (4) uwDEST_Y(8,1)<4>       uwAVS_RESPONSE(1,0)<4;4,1>                                      
    mov (4) uwDEST_Y(9,1)<4>       uwAVS_RESPONSE(1,8)<4;4,1>                                      
    mov (4) uwDEST_Y(12,1)<4>      uwAVS_RESPONSE(1,4)<4;4,1>                                    
    mov (4) uwDEST_Y(13,1)<4>      uwAVS_RESPONSE(1,12)<4;4,1>                                    
    mov (4) uwDEST_Y(16,1)<4>      uwAVS_RESPONSE(2,0)<4;4,1>                                     
    mov (4) uwDEST_Y(17,1)<4>      uwAVS_RESPONSE(2,8)<4;4,1>                                     
    mov (4) uwDEST_Y(20,1)<4>      uwAVS_RESPONSE(2,4)<4;4,1>                                   
    mov (4) uwDEST_Y(21,1)<4>      uwAVS_RESPONSE(2,12)<4;4,1>                                   
    mov (4) uwDEST_Y(24,1)<4>      uwAVS_RESPONSE(3,0)<4;4,1>                                     
    mov (4) uwDEST_Y(25,1)<4>      uwAVS_RESPONSE(3,8)<4;4,1>                                     
    mov (4) uwDEST_Y(28,1)<4>      uwAVS_RESPONSE(3,4)<4;4,1>                                   
    mov (4) uwDEST_Y(29,1)<4>      uwAVS_RESPONSE(3,12)<4;4,1>                                   

// Move first 8x8 words of U to dest GRF (as packed)
    mov (4) uwDEST_Y(0,0)<4>       uwAVS_RESPONSE(4,0)<4;4,1>                                      
    mov (4) uwDEST_Y(1,0)<4>       uwAVS_RESPONSE(4,8)<4;4,1>                                      
    mov (4) uwDEST_Y(4,0)<4>       uwAVS_RESPONSE(4,4)<4;4,1>                                    
    mov (4) uwDEST_Y(5,0)<4>       uwAVS_RESPONSE(4,12)<4;4,1>                                    
    mov (4) uwDEST_Y(8,0)<4>       uwAVS_RESPONSE(5,0)<4;4,1>                                      
    mov (4) uwDEST_Y(9,0)<4>       uwAVS_RESPONSE(5,8)<4;4,1>                                      
    mov (4) uwDEST_Y(12,0)<4>      uwAVS_RESPONSE(5,4)<4;4,1>                                    
    mov (4) uwDEST_Y(13,0)<4>      uwAVS_RESPONSE(5,12)<4;4,1>                                    
    mov (4) uwDEST_Y(16,0)<4>      uwAVS_RESPONSE(6,0)<4;4,1>                                     
    mov (4) uwDEST_Y(17,0)<4>      uwAVS_RESPONSE(6,8)<4;4,1>                                     
    mov (4) uwDEST_Y(20,0)<4>      uwAVS_RESPONSE(6,4)<4;4,1>                                   
    mov (4) uwDEST_Y(21,0)<4>      uwAVS_RESPONSE(6,12)<4;4,1>                                   
    mov (4) uwDEST_Y(24,0)<4>      uwAVS_RESPONSE(7,0)<4;4,1>                                     
    mov (4) uwDEST_Y(25,0)<4>      uwAVS_RESPONSE(7,8)<4;4,1>                                     
    mov (4) uwDEST_Y(28,0)<4>      uwAVS_RESPONSE(7,4)<4;4,1>                                   
    mov (4) uwDEST_Y(29,0)<4>      uwAVS_RESPONSE(7,12)<4;4,1>                                   

// Move first 8x8 words of V to dest GRF (as packed)
    mov (4) uwDEST_Y(0,2)<4>       uwAVS_RESPONSE(8,0)<4;4,1>                                      
    mov (4) uwDEST_Y(1,2)<4>       uwAVS_RESPONSE(8,8)<4;4,1>                                      
    mov (4) uwDEST_Y(4,2)<4>       uwAVS_RESPONSE(8,4)<4;4,1>                                    
    mov (4) uwDEST_Y(5,2)<4>       uwAVS_RESPONSE(8,12)<4;4,1>                                    
    mov (4) uwDEST_Y(8,2)<4>       uwAVS_RESPONSE(9,0)<4;4,1>                                      
    mov (4) uwDEST_Y(9,2)<4>       uwAVS_RESPONSE(9,8)<4;4,1>                                      
    mov (4) uwDEST_Y(12,2)<4>      uwAVS_RESPONSE(9,4)<4;4,1>                                    
    mov (4) uwDEST_Y(13,2)<4>      uwAVS_RESPONSE(9,12)<4;4,1>                                    
    mov (4) uwDEST_Y(16,2)<4>      uwAVS_RESPONSE(10,0)<4;4,1>                                     
    mov (4) uwDEST_Y(17,2)<4>      uwAVS_RESPONSE(10,8)<4;4,1>                                     
    mov (4) uwDEST_Y(20,2)<4>      uwAVS_RESPONSE(10,4)<4;4,1>                                   
    mov (4) uwDEST_Y(21,2)<4>      uwAVS_RESPONSE(10,12)<4;4,1>                                   
    mov (4) uwDEST_Y(24,2)<4>      uwAVS_RESPONSE(11,0)<4;4,1>                                     
    mov (4) uwDEST_Y(25,2)<4>      uwAVS_RESPONSE(11,8)<4;4,1>                                     
    mov (4) uwDEST_Y(28,2)<4>      uwAVS_RESPONSE(11,4)<4;4,1>                                   
    mov (4) uwDEST_Y(29,2)<4>      uwAVS_RESPONSE(11,12)<4;4,1>                                   

// Move first 8x8 words of A to dest GRF (as packed)
    mov (4) uwDEST_Y(0,3)<4>       0:uw                                    
    mov (4) uwDEST_Y(1,3)<4>       0:uw                                    
    mov (4) uwDEST_Y(4,3)<4>       0:uw                                  
    mov (4) uwDEST_Y(5,3)<4>       0:uw                                   
    mov (4) uwDEST_Y(8,3)<4>       0:uw                                    
    mov (4) uwDEST_Y(9,3)<4>       0:uw                                    
    mov (4) uwDEST_Y(12,3)<4>      0:uw                                  
    mov (4) uwDEST_Y(13,3)<4>      0:uw                                   
    mov (4) uwDEST_Y(16,3)<4>      0:uw                                    
    mov (4) uwDEST_Y(17,3)<4>      0:uw                                    
    mov (4) uwDEST_Y(20,3)<4>      0:uw                                  
    mov (4) uwDEST_Y(21,3)<4>      0:uw                                   
    mov (4) uwDEST_Y(24,3)<4>      0:uw                                    
    mov (4) uwDEST_Y(25,3)<4>      0:uw                                    
    mov (4) uwDEST_Y(28,3)<4>      0:uw                                  
    mov (4) uwDEST_Y(29,3)<4>      0:uw                                   

// Move second 8x8 words of Y to dest GRF
    mov (4) uwDEST_Y(2,1)<4>       uwAVS_RESPONSE_2(0,0)<4;4,1>                                      
    mov (4) uwDEST_Y(3,1)<4>       uwAVS_RESPONSE_2(0,8)<4;4,1>                                      
    mov (4) uwDEST_Y(6,1)<4>       uwAVS_RESPONSE_2(0,4)<4;4,1>                                    
    mov (4) uwDEST_Y(7,1)<4>       uwAVS_RESPONSE_2(0,12)<4;4,1>                                    
    mov (4) uwDEST_Y(10,1)<4>      uwAVS_RESPONSE_2(1,0)<4;4,1>                                      
    mov (4) uwDEST_Y(11,1)<4>      uwAVS_RESPONSE_2(1,8)<4;4,1>                                      
    mov (4) uwDEST_Y(14,1)<4>      uwAVS_RESPONSE_2(1,4)<4;4,1>                                    
    mov (4) uwDEST_Y(15,1)<4>      uwAVS_RESPONSE_2(1,12)<4;4,1>                                    
    mov (4) uwDEST_Y(18,1)<4>      uwAVS_RESPONSE_2(2,0)<4;4,1>                                     
    mov (4) uwDEST_Y(19,1)<4>      uwAVS_RESPONSE_2(2,8)<4;4,1>                                     
    mov (4) uwDEST_Y(22,1)<4>      uwAVS_RESPONSE_2(2,4)<4;4,1>                                   
    mov (4) uwDEST_Y(23,1)<4>      uwAVS_RESPONSE_2(2,12)<4;4,1>                                   
    mov (4) uwDEST_Y(26,1)<4>      uwAVS_RESPONSE_2(3,0)<4;4,1>                                     
    mov (4) uwDEST_Y(27,1)<4>      uwAVS_RESPONSE_2(3,8)<4;4,1>                                     
    mov (4) uwDEST_Y(30,1)<4>      uwAVS_RESPONSE_2(3,4)<4;4,1>                                   
    mov (4) uwDEST_Y(31,1)<4>      uwAVS_RESPONSE_2(3,12)<4;4,1>                                   

// Move second 8x8 words of U to dest GRF
    mov (4) uwDEST_Y(2,0)<4>       uwAVS_RESPONSE_2(4,0)<4;4,1>                                      
    mov (4) uwDEST_Y(3,0)<4>       uwAVS_RESPONSE_2(4,8)<4;4,1>                                      
    mov (4) uwDEST_Y(6,0)<4>       uwAVS_RESPONSE_2(4,4)<4;4,1>                                    
    mov (4) uwDEST_Y(7,0)<4>       uwAVS_RESPONSE_2(4,12)<4;4,1>                                    
    mov (4) uwDEST_Y(10,0)<4>      uwAVS_RESPONSE_2(5,0)<4;4,1>                                      
    mov (4) uwDEST_Y(11,0)<4>      uwAVS_RESPONSE_2(5,8)<4;4,1>                                      
    mov (4) uwDEST_Y(14,0)<4>      uwAVS_RESPONSE_2(5,4)<4;4,1>                                    
    mov (4) uwDEST_Y(15,0)<4>      uwAVS_RESPONSE_2(5,12)<4;4,1>                                    
    mov (4) uwDEST_Y(18,0)<4>      uwAVS_RESPONSE_2(6,0)<4;4,1>                                     
    mov (4) uwDEST_Y(19,0)<4>      uwAVS_RESPONSE_2(6,8)<4;4,1>                                     
    mov (4) uwDEST_Y(22,0)<4>      uwAVS_RESPONSE_2(6,4)<4;4,1>                                   
    mov (4) uwDEST_Y(23,0)<4>      uwAVS_RESPONSE_2(6,12)<4;4,1>                                   
    mov (4) uwDEST_Y(26,0)<4>      uwAVS_RESPONSE_2(7,0)<4;4,1>                                     
    mov (4) uwDEST_Y(27,0)<4>      uwAVS_RESPONSE_2(7,8)<4;4,1>                                     
    mov (4) uwDEST_Y(30,0)<4>      uwAVS_RESPONSE_2(7,4)<4;4,1>                                   
    mov (4) uwDEST_Y(31,0)<4>      uwAVS_RESPONSE_2(7,12)<4;4,1>                                   

// Move second 8x8 words of V to dest GRF
    mov (4) uwDEST_Y(2,2)<4>       uwAVS_RESPONSE_2(8,0)<4;4,1>                                      
    mov (4) uwDEST_Y(3,2)<4>       uwAVS_RESPONSE_2(8,8)<4;4,1>                                      
    mov (4) uwDEST_Y(6,2)<4>       uwAVS_RESPONSE_2(8,4)<4;4,1>                                    
    mov (4) uwDEST_Y(7,2)<4>       uwAVS_RESPONSE_2(8,12)<4;4,1>                                    
    mov (4) uwDEST_Y(10,2)<4>      uwAVS_RESPONSE_2(9,0)<4;4,1>                                      
    mov (4) uwDEST_Y(11,2)<4>      uwAVS_RESPONSE_2(9,8)<4;4,1>                                      
    mov (4) uwDEST_Y(14,2)<4>      uwAVS_RESPONSE_2(9,4)<4;4,1>                                    
    mov (4) uwDEST_Y(15,2)<4>      uwAVS_RESPONSE_2(9,12)<4;4,1>                                    
    mov (4) uwDEST_Y(18,2)<4>      uwAVS_RESPONSE_2(10,0)<4;4,1>                                     
    mov (4) uwDEST_Y(19,2)<4>      uwAVS_RESPONSE_2(10,8)<4;4,1>                                     
    mov (4) uwDEST_Y(22,2)<4>      uwAVS_RESPONSE_2(10,4)<4;4,1>                                   
    mov (4) uwDEST_Y(23,2)<4>      uwAVS_RESPONSE_2(10,12)<4;4,1>                                   
    mov (4) uwDEST_Y(26,2)<4>      uwAVS_RESPONSE_2(11,0)<4;4,1>                                     
    mov (4) uwDEST_Y(27,2)<4>      uwAVS_RESPONSE_2(11,8)<4;4,1>                                     
    mov (4) uwDEST_Y(30,2)<4>      uwAVS_RESPONSE_2(11,4)<4;4,1>                                   
    mov (4) uwDEST_Y(31,2)<4>      uwAVS_RESPONSE_2(11,12)<4;4,1>                                   

// Move second 8x8 words of A to dest GRF
    mov (4) uwDEST_Y(2,3)<4>       0:uw                                    
    mov (4) uwDEST_Y(3,3)<4>       0:uw                                    
    mov (4) uwDEST_Y(6,3)<4>       0:uw                                  
    mov (4) uwDEST_Y(7,3)<4>       0:uw                                   
    mov (4) uwDEST_Y(10,3)<4>      0:uw                                    
    mov (4) uwDEST_Y(11,3)<4>      0:uw                                    
    mov (4) uwDEST_Y(14,3)<4>      0:uw                                  
    mov (4) uwDEST_Y(15,3)<4>      0:uw                                   
    mov (4) uwDEST_Y(18,3)<4>      0:uw                                    
    mov (4) uwDEST_Y(19,3)<4>      0:uw                                    
    mov (4) uwDEST_Y(22,3)<4>      0:uw                                  
    mov (4) uwDEST_Y(23,3)<4>      0:uw                                   
    mov (4) uwDEST_Y(26,3)<4>      0:uw                                    
    mov (4) uwDEST_Y(27,3)<4>      0:uw                                    
    mov (4) uwDEST_Y(30,3)<4>      0:uw                                  
    mov (4) uwDEST_Y(31,3)<4>      0:uw                                   

/*	This section will be used if 16-bit output is needed in planar format -vK
    // Move 1st 8x8 words of Y to dest GRF at lower 8 words of each RGF.
    $for(0; <8/2; 1) {
        mov (8) uwDEST_Y(%1*2)<1>          uwAVS_RESPONSE(%1)<8;4,1>        
        mov (8) uwDEST_Y(%1*2+1)<1>        uwAVS_RESPONSE(%1,8)<8;4,1>      
    } 

    // Move 8x8 words of U to dest GRF  
    $for(0; <8/2; 1) {
        mov (8) uwDEST_U(%1*2)<1>          uwAVS_RESPONSE(%1+4)<8;4,1>  
        mov (8) uwDEST_U(%1*2+1)<1>        uwAVS_RESPONSE(%1+4,8)<8;4,1> 
    } 

    // Move 8x8 words of V to dest GRF  
    $for(0; <8/2; 1) {
        mov (8) uwDEST_V(%1*2)<1>          uwAVS_RESPONSE(%1+8)<8;4,1>      
        mov (8) uwDEST_V(%1*2+1)<1>        uwAVS_RESPONSE(%1+8,8)<8;4,1>    
    } 

    // Move 2nd 8x8 words of Y to dest GRF  
    $for(0; <8/2; 1) {
        mov (8) uwDEST_Y(%1*2,8)<1>          uwAVS_RESPONSE_2(%1)<8;4,1>        
        mov (8) uwDEST_Y(%1*2+1,8)<1>        uwAVS_RESPONSE_2(%1,8)<8;4,1>      
    } 

    // Move 2nd 8x8 words of U to dest GRF  
    $for(0; <8/2; 1) {
        mov (8) uwDEST_U(%1*2,8)<1>          uwAVS_RESPONSE_2(%1+4)<8;4,1>  
        mov (8) uwDEST_U(%1*2+1,8)<1>        uwAVS_RESPONSE_2(%1+4,8)<8;4,1> 
    } 

    // Move 2nd 8x8 words of V to dest GRF  
    $for(0; <8/2; 1) {
        mov (8) uwDEST_V(%1*2,8)<1>          uwAVS_RESPONSE_2(%1+8)<8;4,1>      
        mov (8) uwDEST_V(%1*2+1,8)<1>        uwAVS_RESPONSE_2(%1+8,8)<8;4,1>    
    } 
*/
#else /* OUTPUT_8_BIT */
    // Move 1st 8x8 words of Y to dest GRF at lower 8 words of each RGF.
    $for(0; <8/2; 1) {
        mov (8) uwDEST_Y(%1*2)<1>          ubAVS_RESPONSE(%1,1)<16;4,2>        // Copy high byte in a word
        mov (8) uwDEST_Y(%1*2+1)<1>        ubAVS_RESPONSE(%1,8+1)<16;4,2>      // Copy high byte in a word
    } 

    // Move 8x8 words of U to dest GRF  
    $for(0; <8/2; 1) {
        mov (8) uwDEST_U(%1*2)<1>          ubAVS_RESPONSE(%1+4,1)<16;4,2>      // Copy high byte in a word
        mov (8) uwDEST_U(%1*2+1)<1>        ubAVS_RESPONSE(%1+4,8+1)<16;4,2>    // Copy high byte in a word
    } 

    // Move 8x8 words of V to dest GRF  
    $for(0; <8/2; 1) {
        mov (8) uwDEST_V(%1*2)<1>          ubAVS_RESPONSE(%1+8,1)<16;4,2>      // Copy high byte in a word
        mov (8) uwDEST_V(%1*2+1)<1>        ubAVS_RESPONSE(%1+8,8+1)<16;4,2>    // Copy high byte in a word
    } 

    // Move 2nd 8x8 words of Y to dest GRF at higher 8 words of each RGF.
    $for(0; <8/2; 1) {
        mov (8) uwDEST_Y(%1*2,8)<1>          ubAVS_RESPONSE_2(%1,1)<16;4,2>     // Copy high byte in a word
        mov (8) uwDEST_Y(%1*2+1,8)<1>        ubAVS_RESPONSE_2(%1,8+1)<16;4,2>   // Copy high byte in a word
    } 

    // Move 2nd 8x8 words of U to dest GRF  
    $for(0; <8/2; 1) {
        mov (8) uwDEST_U(%1*2,8)<1>          ubAVS_RESPONSE_2(%1+4,1)<16;4,2>   // Copy high byte in a word
        mov (8) uwDEST_U(%1*2+1,8)<1>        ubAVS_RESPONSE_2(%1+4,8+1)<16;4,2> // Copy high byte in a word
    } 

    // Move 2nd 8x8 words of V to dest GRF  
    $for(0; <8/2; 1) {
        mov (8) uwDEST_V(%1*2,8)<1>          ubAVS_RESPONSE_2(%1+8,1)<16;4,2>   // Copy high byte in a word
        mov (8) uwDEST_V(%1*2+1,8)<1>        ubAVS_RESPONSE_2(%1+8,8+1)<16;4,2> // Copy high byte in a word
    } 
#endif
//------------------------------------------------------------------------------
    // Re-define new # of lines
    #undef nUV_NUM_OF_ROWS
    #undef nY_NUM_OF_ROWS
      
    #define nY_NUM_OF_ROWS      8
    #define nUV_NUM_OF_ROWS     8
                   

