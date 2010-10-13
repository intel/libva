/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

//---------- PA_AVS_IEF_Unpack_8x8.asm ----------

// Yoni: In order to optimize unpacking, 3 methods are being checked:
//  1. AVS_ORIGINAL
//  2. AVS_ROUND_TO_8_BITS  
//  3. AVS_INDIRECT_ACCESS  
//
// Only 1 method should stay in the code 


//#define AVS_ROUND_TO_8_BITS
//#define AVS_INDIRECT_ACCESS


    // Move first 8x8 words of Y to dest GRF
    mov (8)  uwDEST_Y(0)<1>     ubAVS_RESPONSE(2,1)<16;4,2>                 
    mov (8)  uwDEST_Y(1)<1>     ubAVS_RESPONSE(2,8+1)<16;4,2>               
    mov (8)  uwDEST_Y(2)<1>     ubAVS_RESPONSE(3,1)<16;4,2>                 
    mov (8)  uwDEST_Y(3)<1>     ubAVS_RESPONSE(3,8+1)<16;4,2>               
    mov (8)  uwDEST_Y(4)<1>     ubAVS_RESPONSE(8,1)<16;4,2>                 
    mov (8)  uwDEST_Y(5)<1>     ubAVS_RESPONSE(8,8+1)<16;4,2>               
    mov (8)  uwDEST_Y(6)<1>     ubAVS_RESPONSE(9,1)<16;4,2>                 
    mov (8)  uwDEST_Y(7)<1>     ubAVS_RESPONSE(9,8+1)<16;4,2>               

    // Move first 4x8 words of V to dest GRF  
    mov (4) uwDEST_V(0)<1>      ubAVS_RESPONSE(0,1)<16;2,4>                 
    mov (4) uwDEST_V(0,8)<1>    ubAVS_RESPONSE(0,8+1)<16;2,4>               
    mov (4) uwDEST_V(1)<1>      ubAVS_RESPONSE(1,1)<16;2,4>                 
    mov (4) uwDEST_V(1,8)<1>    ubAVS_RESPONSE(1,8+1)<16;2,4>               
    mov (4) uwDEST_V(2)<1>      ubAVS_RESPONSE(6,1)<16;2,4>                 
    mov (4) uwDEST_V(2,8)<1>    ubAVS_RESPONSE(6,8+1)<16;2,4>               
    mov (4) uwDEST_V(3)<1>      ubAVS_RESPONSE(7,1)<16;2,4>                 
    mov (4) uwDEST_V(3,8)<1>    ubAVS_RESPONSE(7,8+1)<16;2,4>               

    // Move first 4x8 words of U to dest GRF        
    mov (4) uwDEST_U(0)<1>      ubAVS_RESPONSE(4,1)<16;2,4>           
    mov (4) uwDEST_U(0,8)<1>    ubAVS_RESPONSE(4,8+1)<16;2,4>                 
    mov (4) uwDEST_U(1)<1>      ubAVS_RESPONSE(5,1)<16;2,4>           
    mov (4) uwDEST_U(1,8)<1>    ubAVS_RESPONSE(5,8+1)<16;2,4>                 
    mov (4) uwDEST_U(2)<1>      ubAVS_RESPONSE(10,1)<16;2,4>          
    mov (4) uwDEST_U(2,8)<1>    ubAVS_RESPONSE(10,8+1)<16;2,4>                
    mov (4) uwDEST_U(3)<1>      ubAVS_RESPONSE(11,1)<16;2,4>          
    mov (4) uwDEST_U(3,8)<1>    ubAVS_RESPONSE(11,8+1)<16;2,4>                

    // Move second 8x8 words of Y to dest GRF
    mov (8) uwDEST_Y(0,8)<1>    ubAVS_RESPONSE_2(2,1)<16;4,2>    
    mov (8) uwDEST_Y(1,8)<1>    ubAVS_RESPONSE_2(2,8+1)<16;4,2>
    mov (8) uwDEST_Y(2,8)<1>    ubAVS_RESPONSE_2(3,1)<16;4,2>     
    mov (8) uwDEST_Y(3,8)<1>    ubAVS_RESPONSE_2(3,8+1)<16;4,2>
    mov (8) uwDEST_Y(4,8)<1>    ubAVS_RESPONSE_2(8,1)<16;4,2>  
    mov (8) uwDEST_Y(5,8)<1>    ubAVS_RESPONSE_2(8,8+1)<16;4,2>
    mov (8) uwDEST_Y(6,8)<1>    ubAVS_RESPONSE_2(9,1)<16;4,2>     
    mov (8) uwDEST_Y(7,8)<1>    ubAVS_RESPONSE_2(9,8+1)<16;4,2>

    // Move second 4x8 words of V to dest GRF        
    mov (4) uwDEST_V(0,4)<1>    ubAVS_RESPONSE_2(0,1)<16;2,4>           
    mov (4) uwDEST_V(0,12)<1>   ubAVS_RESPONSE_2(0,8+1)<16;2,4>                 
    mov (4) uwDEST_V(1,4)<1>    ubAVS_RESPONSE_2(1,1)<16;2,4>           
    mov (4) uwDEST_V(1,12)<1>   ubAVS_RESPONSE_2(1,8+1)<16;2,4>                 
    mov (4) uwDEST_V(2,4)<1>    ubAVS_RESPONSE_2(6,1)<16;2,4>           
    mov (4) uwDEST_V(2,12)<1>   ubAVS_RESPONSE_2(6,8+1)<16;2,4>                 
    mov (4) uwDEST_V(3,4)<1>    ubAVS_RESPONSE_2(7,1)<16;2,4>           
    mov (4) uwDEST_V(3,12)<1>   ubAVS_RESPONSE_2(7,8+1)<16;2,4>                 

    // Move second 4x8 words of U to dest GRF        
    mov (4) uwDEST_U(0,4)<1>    ubAVS_RESPONSE_2(4,1)<16;2,4>             
    mov (4) uwDEST_U(0,12)<1>   ubAVS_RESPONSE_2(4,8+1)<16;2,4>           
    mov (4) uwDEST_U(1,4)<1>    ubAVS_RESPONSE_2(5,1)<16;2,4>             
    mov (4) uwDEST_U(1,12)<1>   ubAVS_RESPONSE_2(5,8+1)<16;2,4>           
    mov (4) uwDEST_U(2,4)<1>    ubAVS_RESPONSE_2(10,1)<16;2,4>            
    mov (4) uwDEST_U(2,12)<1>   ubAVS_RESPONSE_2(10,8+1)<16;2,4>          
    mov (4) uwDEST_U(3,4)<1>    ubAVS_RESPONSE_2(11,1)<16;2,4>            
    mov (4) uwDEST_U(3,12)<1>   ubAVS_RESPONSE_2(11,8+1)<16;2,4>          

//------------------------------------------------------------------------------

       // Re-define new number of lines
       #undef nUV_NUM_OF_ROWS
       #undef nY_NUM_OF_ROWS
       
       #define nY_NUM_OF_ROWS      8
       #define nUV_NUM_OF_ROWS     8

