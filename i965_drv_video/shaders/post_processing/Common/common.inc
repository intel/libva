/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

#ifndef COMMON_INC
#define COMMON_INC

// Module name: common.inc
//
// Common header file for all Video-Processing kernels
//

.default_execution_size (16)
.default_register_type  :ub

.reg_count_total        80
.reg_count_payload      4


//========== Common constants ==========

// Bit position constants 
#define BIT0    0x01
#define BIT1    0x02
#define BIT2    0x04
#define BIT3    0x08
#define BIT4    0x10
#define BIT5    0x20
#define BIT6    0x40
#define BIT7    0x80
#define BIT8    0x0100
#define BIT9    0x0200
#define BIT10   0x0400
#define BIT11   0x0800
#define BIT12   0x1000
#define BIT13   0x2000
#define BIT14   0x4000
#define BIT15   0x8000
#define BIT16   0x00010000
#define BIT17   0x00020000
#define BIT18   0x00040000
#define BIT19   0x00080000
#define BIT20   0x00100000
#define BIT21   0x00200000
#define BIT22   0x00400000
#define BIT23   0x00800000
#define BIT24   0x01000000
#define BIT25   0x02000000
#define BIT26   0x04000000
#define BIT27   0x08000000
#define BIT28   0x10000000
#define BIT29   0x20000000
#define BIT30   0x40000000
#define BIT31   0x80000000

#define nGRFWIB             32      // GRF register width in byte
#define nGRFWIW             16      // GRF register width in word
#define nGRFWID             8       // GRF register width in dword

#define nTOP_FIELD          0
#define nBOTTOM_FIELD       1

#define nPREVIOUS_FRAME     0       // Previous frame
#define nCURRENT_FRAME      1       // Current frame
#define nNEXT_FRAME         2       // Next frame

#ifdef GT
// GT DI Kernel
#else // ILK
// ILK DI Kernel
#endif

//===================================

//========== Macros ==========
#define REGION(Width,HStride) <Width*HStride;Width,HStride> // Region definition when ExecSize = Width

#define RegFile(a) a
#define REG(r,n) _REG(RegFile(r),n)
#define _REG(r,n) __REG(r,n)
#define __REG(r,n) r##n.0
#define REG2(r,n,s) _REG2(RegFile(r),n,s)
#define _REG2(r,n,s) __REG2(r,n,s)
#define __REG2(r,n,s) r##n.##s

#define dNULLREG     null<1>:d
#define wNULLREG     null<1>:w
    
#define KERNEL_ID(kernel_ID)    mov NULLREG kernel_ID:ud


#define NODDCLR 			
#define NODDCLR_NODDCHK 	
#define NODDCHK			    

//#define NODDCLR 			{ NoDDClr }
//#define NODDCLR_NODDCHK 	{ NoDDClr, NoDDChk }
//#define NODDCHK				{ NoDDChk } 


//========== Defines ====================


//========== GRF partition ==========
// r0 header            :   r0          (1 GRF)
// Static parameters    :   r1 - r5     (5 GRFS)
// Inline parameters    :   r6 - r7     (2 GRFs)
// MSGSRC               :   r9          (1 GRF)
// Top IO region        :   r10 - r33   (24 GRFS 8 for each component Y,U,V 16X8:w)
// Free space           :   r34 - r55   (22 GRFS)
// Bottom IO region     :   r56 - r79   (24 GRFS 8 for each component Y,U,V 16X8:w) 
//===================================


//========== Static Parameters ==========
// r1
#define fPROCAMP_C0             r1.0    // DWORD 0, Procamp constant C0 in :f
#define wPROCAMP_C0             r1.0    // DWORD 0, Procamp constant C0 in :w
#define NUMBER_0002							r1.1		// DWORD 0, 0x0002 used in procamp for GT
#define udCP_MessageFormat      r1.0    // DWORD 0, bits 2:3 of DWORD. (CE)
#define udCP_StatePointer       r1.0    // DWORD 0, bits 31:5 of DWORD.(CE)

#define ubSRC_CF_OFFSET         r1.4    // DWORD 1, byte 0-2. SRC packed color format YUV offset in :ub

#define ubDEST_RGB_FORMAT        r1.8    // DWORD 2, byte 0. Dest RGB color format (0:ARGB FF:XRGB)
#define ubDEST_CF_OFFSET        r1.8    // DWORD 2, byte 0-2. Dest packed color format YUV offset in :ub

#define fPROCAMP_C1             r1.3    // DWORD 3, Procamp constant C1 in :f   
#define wPROCAMP_C1             r1.6    // DWORD 3, Procamp constant C1 in :w   
#define NUMBER_0100							r1.7		// DWORD 3, 0x0100 used in procamp for GT

#define fPROCAMP_C2             r1.4    // DWORD 4, Procamp constant C2 in :f
#define wPROCAMP_C2             r1.8    // DWORD 4, Procamp constant C2 in :w

#define uwSPITCH_DIV2           r1.10   // DWORD 5, byte 0-1. statistics surface pitch divided by 2

#define fVIDEO_STEP_Y           r1.6    // DWORD 6, :f, AVS normalized reciprocal of Y Scaling factor
#define ubSTMM_SHIFT            r1.24   // DWORD 6, byte 0. Amount of right shift for the DI blending equation
#define ubSTMM_MIN              r1.25   // DWORD 6, byte 1. Min STMM for DI blending equation
#define ubSTMM_MAX              r1.26   // DWORD 6, byte 2. Max STMM for DI blending equation
#define ubTFLD_FIRST            r1.27   // DWORD 6, byte 3. Field parity order

#define fPROCAMP_C5             r1.7    // DWORD 7, Procamp constant C3 in :f
#define wPROCAMP_C5             r1.14   // DWORD 7, Procamp constant C3 in :w

// r2
#define fPROCAMP_C3             r2.0    // DWORD 0, Procamp constant C4 in :f
#define wPROCAMP_C3             r2.0    // DWORD 0, Procamp constant C4 in :w
                    
#define fCSC_C5					r2.2	// DWORD 2. WG+CSC constant C5
#define wCSC_C5					r2.4	// DWORD 2. WG+CSC constant C5

#define fPROCAMP_C4             r2.3    // DWORD 3, Procamp constant C5 in :f
#define wPROCAMP_C4             r2.6    // DWORD 3, Procamp constant C5 in :w

#define fCSC_C8					r2.4	// DWORD 4. WG+CSC constant C8
#define wCSC_C8					r2.8	// DWORD 4. WG+CSC constant C8
#define fCSC_C9					r2.7	// DWORD 7. WG+CSC constant C9
#define wCSC_C9					r2.14	// DWORD 7. WG+CSC constant C9

// r3
#define fCSC_C0					r3.0	// DWORD 0. WG+CSC constant C0
#define wCSC_C0					r3.0	// DWORD 0. WG+CSC constant C0

#define fSCALING_STEP_RATIO     r3.1    // DWORD 1, = Alpha_X_Scaling_Step / Video_X_scaling_Step :f (blending)
#define fALPHA_STEP_X           r3.1    // DWORD 1, = 1/Scale X, 0.5 = 2x, in :f (blending)

#define fALPHA_STEP_Y           r3.2    // DWORD 2, = 1/Scale Y, in :f

#define fCSC_C4					r3.3	// DWORD 3. WG+CSC constant C4
#define wCSC_C4					r3.6	// DWORD 3. WG+CSC constant C4
#define fCSC_C1					r3.4	// DWORD 4. WG+CSC constant C1
#define wCSC_C1					r3.8	// DWORD 4. WG+CSC constant C1

#define wSRC_H_ORI_OFFSET       r3.10   // DWORD 5, bytes 0,1 :w    
#define wSRC_V_ORI_OFFSET       r3.11   // DWORD 5, bytes 2,3 :w

#define dCOLOR_PIXEL            r3.6    // DWORD 6. Color pixel for Colorfill

#define fCSC_C2					r3.6	// DWORD 6. WG+CSC constant C2
#define wCSC_C2					r3.12	// DWORD 6. WG+CSC constant C2
#define fCSC_C3					r3.7	// DWORD 7. WG+CSC constant C3
#define wCSC_C3					r3.14	// DWORD 7. WG+CSC constant C3

// r4
#define fCSC_C6					r4.0	// DWORD 0. WG+CSC constant C6
#define wCSC_C6					r4.0	// DWORD 0. WG+CSC constant C6

#define wFRAME_ENDX             r4.2    // DWORD 1, word 0. Horizontal end = Origin+Width (in pixels)(for multiple blocks)
#define wNUM_BLKS               r4.3    // DWORD 1, word 1. Number of blocks to process (for multiple blocks)

#define wCOPY_ORIX              r4.5    // DWORD 2, word 1. A copy of X origin (for multiple blocks)
#define uwNLAS_ENABLE           r4.4    // DWORD 2, bit 15, NLAS enble bit

#define fCSC_C7					r4.3	// DWORD 3. WG+CSC constant C7
#define wCSC_C7					r4.6	// DWORD 3. WG+CSC constant C7
#define fCSC_C10				r4.4	// DWORD 4. WG+CSC constant C10
#define wCSC_C10				r4.8	// DWORD 4. WG+CSC constant C10

#define fFRAME_VID_ORIX         r4.5    // DWORD 5, Frame horizontal origin normalized for scale kernel

#define fFRAME_ALPHA_ORIX       r4.6    // DWORD 6. Normalized alpha horiz origin for the frame

#define fCSC_C11				r4.7	// DWORD 7. WG+CSC constant C11
#define wCSC_C11				r4.14	// DWORD 7. WG+CSC constant C11

//========================================

//========== Inline parameters ===========
// r5
#define wORIX                   r5.0    // DWORD 0, byte 0-1. :w, Destination Block Horizontal Origin in pel
#define wORIY                   r5.1    // DWORD 0, byte 2-3. :w, Destination Block Vertical   Origin in pel

#define fSRC_VID_H_ORI          r5.1    // DWORD 1, :f, SRC Y horizontal origin normalized for scale kernel

#define fSRC_VID_V_ORI          r5.2    // DWORD 2, :f, SRC Y vertical origin normalized for scale kernel

#define fSRC_ALPHA_H_ORI        r5.3    // DWORD 3, :f, Normalized alpha horizontal origin

#define fSRC_ALPHA_V_ORI        r5.4    // DWORD 4, :f, Normalized alpha vertical origin

#define uwALPHA_MASK_X          r5.10   // DWORD 5, byte 0-1 :w, H. alpha mask
#define ubALPHA_MASK_Y          r5.22   // DWORD 5, byte 2.  :ub,V. alpha mask
#define ubBLK_CNT_X             r5.23   // DWORD 5, byte 3,  :ub, Horizontal Block Count per thread

#define udBLOCK_MASK            r5.6    // DWORD 6
#define uwBLOCK_MASK_H          r5.12   // DWORD 6, byte 0-1 :uw, Block horizontal mask used in non-DWord aligned kernels
#define ubBLOCK_MASK_V          r5.26   // DWORD 6, byte 2   :ub, Block vertical mask used in non-DWord aligned kernels
#define ubNUM_BLKS              r5.27   // DWORD 6, byte 3,  :ub, Total Block Count per thread

#define fVIDEO_STEP_X           r5.7    // DWORD 7. :f, AVS normalized reciprocal of X Scaling factor

// r6
#define fVIDEO_STEP_DELTA       r6.0    // DWORD 0. :f, AVS normalized delta between 2 adjacent scaling steps (used for non-linear scaling)


//====================== Binding table =========================================

#if defined(DNDI)
    // DNDI Surface Binding Table
    //#define nBI_SRC_CURR        0       // Current input frame surface
    //#define nBI_SRC_PRIV        1       // Denoised previous input frame surface
    //#define nBI_SRC_STAT        2       // Statistics input surface (STMM / Noise motion history)
    //#define nBI_DEST_1ST        3       // 1st deinterlaced output frame surface
//    #define nBI_DEST_YUV        3       // Dest frame YUV (for DN only)
    //#define nBI_DEST_Y          3       // Dest frame Y (for DN only)
    //#define nBI_DEST_2ND        4       // 2nd deinterlaced output frame surface
    //#define nBI_DEST_DN_CURR    6       // Denoised current output frame surface
    //#define nBI_DEST_STAT       7       // Statistics output surface (STMM / Noise motion history)
//    #define nBI_DEST_U          8       // Dest frame U (for DN only)
//    #define nBI_DEST_V          9       // Dest frame V (for DN only)
//    #define nBI_SRC_U          10       // Src frame U (for DN only)
//    #define nBI_SRC_V          11       // Src frame V (for DN only)
//    #define nBI_SRC_UV         10       // Current src frame for UV
    
#endif

#if defined(INPUT_PL3)
    // PL3 Surface Binding Table
//    #define nBI_SRC_ALPHA       0       // Alpha
//    #define nBI_SRC_Y           1       // Current src frame
//    #define nBI_SRC_U           2       // Current src frame
//    #define nBI_SRC_V           3       // Current src frame
//    #define nBI_DEST_Y         10       // Dest frame
//    #define nBI_DEST_U         11       // Dest frame
//    #define nBI_DEST_V         12       // Dest frame
//    #define nBI_DEST_YUV        7       // Dest frame
//    #define nBI_DEST_RGB        7       // same num as BI_DEST_YUV, never used at the same time
#endif

#if defined(INPUT_PL2)
    // PL2 Surface Binding Table
//    #define nBI_SRC_ALPHA       0       // Alpha
//    #define nBI_SRC_Y           1       // Current src frame for Y + offseted UV
//    #define nBI_SRC_YUV         1       // Current src frame for YUV in case of NV12_AVS
//    #define nBI_SRC_UV          2       // Current src frame for UV
//    #define nBI_DEST_YUV        7       // Current dest frame for Y + offseted UV
//    #define nBI_DEST_RGB        7       // same num as BI_DEST_YUV, never used at the same time
//    #define nBI_DEST_Y         10       // Dest frame
//    #define nBI_DEST_U         11       // Dest frame
//    #define nBI_DEST_V         12       // Dest frame
#endif

#if defined(INPUT_PA) || defined(COLORFILL)
    // Packed Surface Binding Table 
//    #define nBI_SRC_ALPHA       0       // Alpha    
//    #define nBI_SRC_YUV         1       // Current src frame
//    #define nBI_DEST_YUV        3       // Dest frame
//    #define nBI_DEST_RGB        3       // same num as BI_DEST_YUV, never used at the same time
#endif


//supper binding table
#define nBI_ALPHA_SRC                   0
#define nBI_CURRENT_SRC_YUV             1 
#define nBI_FIELD_COPY_SRC_1_YUV        1 
#define nBI_CURRENT_SRC_Y               1 
#define nBI_FIELD_COPY_SRC_1_Y          1
#define nBI_CURRENT_SRC_RGB             1  
#define nBI_CURRENT_SRC_UV              2 
#define nBI_FIELD_COPY_SRC_1_UV         2 
#define nBI_CURRENT_SRC_U               2 
#define nBI_FIELD_COPY_SRC_1_U          2 
#define nBI_CURRENT_SRC_V               3
#define nBI_FIELD_COPY_SRC_1_V          3 
#define nBI_TEMPORAL_REFERENCE_YUV      4 
#define nBI_FIELD_COPY_SRC_2_YUV        4 
#define nBI_TEMPORAL_REFERENCE_Y        4 
#define nBI_FIELD_COPY_SRC_2_Y          4 
#define nBI_CURRENT_SRC_YUV_HW_DI       4 
#define nBI_TEMPORAL_REFERENCE_UV       5 
#define nBI_FIELD_COPY_SRC_2_UV         5 
#define nBI_TEMPORAL_REFERENCE_U        5 
#define nBI_FIELD_COPY_SRC_2_U          5 
#define nBI_DENOISED_PREV_HW_DI         5 
#define nBI_TEMPORAL_REFERENCE_V        6 
#define nBI_FIELD_COPY_SRC_2_V          6 
#define nBI_STMM_HISTORY                6 
#define nBI_DESTINATION_YUV             7
#define nBI_DESTINATION_RGB             7
#define nBI_DESTINATION_Y               7
#define nBI_DESTINATION_UV              8
#define nBI_DESTINATION_U               8
#define nBI_DESTINATION_V               9
#define nBI_DESTINATION_1_YUV           10
#define nBI_DESTINATION_1_Y             10
#define nBI_DESTINATION_1_UV            11
#define nBI_DESTINATION_1_U             11
#define nBI_DESTINATION_1_V             12
#define nBI_DESTINATION_2_YUV           13
#define nBI_DESTINATION_2_Y             13
#define nBI_DESTINATION_2_UV            14
#define nBI_DESTINATION_2_U             14
#define nBI_DESTINATION_2_V             15
#define nBI_STMM_HISTORY_OUTPUT         20
#define nBI_TEMPORAL_REFERENCE_YUV_PDI  21 
#define nBI_TEMPORAL_REFERENCE_Y_PDI    21 
#define nBI_TEMPORAL_REFERENCE_UV_PDI   22 
#define nBI_TEMPORAL_REFERENCE_U_PDI    22 
#define nBI_TEMPORAL_REFERENCE_V_PDI    23 
#define nBI_SUBVIDEO_YUV                26
#define nBI_SUBVIDEO_Y                  26
#define nBI_SUBVIDEO_UV                 27
#define nBI_SUBVIDEO_U                  27
#define nBI_SUBVIDEO_V                  28
#define nBI_SUBPICTURE_YUV              29
#define nBI_SUBPICTURE_P8               29
#define nBI_SUBPICTURE_A8               30
#define nBI_GRAPHIC_YUV                 31
#define nBI_GRAPHIC_P8                  31
#define nBI_GRAPHIC_A8                  32



//========== Planar Sampler State Table Index ==========
#define nSI_SRC_ALPHA           0x000   // Sampler State for Alpha

//Sampler Index for AVS/IEF messages
#define nSI_SRC_Y               0x400   // Sampler State for Y
#define nSI_SRC_U               0x800   // Sampler State for U
#define nSI_SRC_V               0xC00   // Sampler State for V
#define nSI_SRC_UV              0x800   // For NV12 surfaces
#define nSI_SRC_YUV             0x400   // For Packed surfaces  
#define nSI_SRC_RGB             0x400   // For ARGB surfaces

//Sampler Index for SIMD16 sampler messages
#define nSI_SRC_SIMD16_Y        0x100   // Sampler State for Y
#define nSI_SRC_SIMD16_U        0x200   // Sampler State for U
#define nSI_SRC_SIMD16_V        0x300   // Sampler State for V
#define nSI_SRC_SIMD16_UV       0x200   // For NV12 surfaces
#define nSI_SRC_SIMD16_YUV      0x100   // For Packed surfaces  
#define nSI_SRC_SIMD16_RGB      0x100   // For ARGB surfaces



// Common Registers
#define pCF_Y_OFFSET            a0.4    // Address register holding Y offset
#define pCF_U_OFFSET            a0.5    // Address register holding U offset
#define pCF_V_OFFSET            a0.6    // Address register holding V offset

// #define YUV_ORI             ORIX    // Used by writing packed data to dport


//================= Message Payload Header fields ==============================
#define IDP     r0.2:ud     // Interface Descriptor Pointer

//================= Common Message Descriptor  TBD add common load and save =====
// Message descriptor for dataport media write
#ifdef GT
        // Message Descriptors
                //                = 000 0001 (min message len 1 - add later) 00000 (resp len 0)         
                //                  1 (header present 1) 0 0 1010 (media block write) 00000
                //                  00000000 (binding table index - set later)
                //                = 0x02094000
        #define nDPMW_MSGDSC      0x02094000
        #define nDPMR_MSGDSC      0x02098000  // Data Port Media Block Read Message Descriptor
        // TBD
#else // ILK
        // Message Descriptors
                //                = 000 0001 (min message len 1 - add later) 00000 (resp len 0)         
                //                  1 (header present 1) 000 0 010 (media block write) 0000
                //                  00000000 (binding table index - set later)
                //                = 0x02082000
        #define nDPMW_MSGDSC      0x02082000  // Data Port Media Block Write Message Descriptor
        #define nDPMR_MSGDSC      0x0208A000  // Data Port Media Block Read Message Descriptor
#endif

// Message Length defines
#define nMSGLEN_1      0x02000000 // Message Length of 1 GRF for Send
#define nMSGLEN_2      0x04000000 // Message Length of 2 GRF for Send
#define nMSGLEN_4      0x08000000 // Message Length of 4 GRF for Send
#define nMSGLEN_8      0x10000000 // Message Length of 8 GRF for Send

// Response Length defines
#define nRESLEN_1      0x00100000 // Message Response Length of 1  GRF from Send
#define nRESLEN_2      0x00200000 // Message Response Length of 2  GRF from Send
#define nRESLEN_3      0x00300000 // Message Response Length of 3  GRF from Send
#define nRESLEN_4      0x00400000 // Message Response Length of 4  GRF from Send
#define nRESLEN_5      0x00500000 // Message Response Length of 5  GRF from Send
#define nRESLEN_8      0x00800000 // Message Response Length of 8  GRF from Send
#define nRESLEN_9      0x00900000 // Message Response Length of 9  GRF from Send
#define nRESLEN_11     0x00B00000 // Message Response Length of 11 GRF from Send
#define nRESLEN_12     0x00C00000 // Message Response Length of 12 GRF from Send
#define nRESLEN_16     0x01000000 // Message Response Length of 16 GRF from Send

// Block Width and Height Size defines
#define nBLOCK_WIDTH_4   0x00000003      // Block Width  4
#define nBLOCK_WIDTH_5   0x00000004      // Block Width  5
#define nBLOCK_WIDTH_8   0x00000007      // Block Width  8
#define nBLOCK_WIDTH_9   0x00000008      // Block Width  9
#define nBLOCK_WIDTH_12  0x0000000B      // Block Width  12
#define nBLOCK_WIDTH_16  0x0000000F      // Block Width  16
#define nBLOCK_WIDTH_20  0x00000013      // Block Width  20
#define nBLOCK_WIDTH_32  0x0000001F      // Block Width  32
#define nBLOCK_HEIGHT_1  0x00000000      // Block Height 1
#define nBLOCK_HEIGHT_2  0x00010000      // Block Height 2
#define nBLOCK_HEIGHT_4  0x00030000      // Block Height 4
#define nBLOCK_HEIGHT_5  0x00040000      // Block Height 5
#define nBLOCK_HEIGHT_8  0x00070000      // Block Height 8

// Extended Message Descriptors
#define nEXTENDED_MATH      0x1
#define nSMPL_ENGINE        0x2
#define nMESSAGE_GATEWAY    0x3
#define nDATAPORT_READ      0x4
#define nDATAPORT_WRITE     0x5
#define nURB                0x6
#define nTS_EOT             0x27    // with End-Of-Thread bit ON

// Common message descriptors:
#ifdef GT
	#define nEOT_MSGDSC       0x02000010  // End of Thread Message Descriptor    
	#define IF_NULL						null:uw null:uw null:uw 	//for different if instructions on ILK and Gen6
#else //ILK
	#define nEOT_MSGDSC       0x02000000  // End of Thread Message Descriptor        
	#define IF_NULL
#endif    


//===================== Math Function Control ===================================
#define mfcINV                  0x1     // reciprocal
#define mfcLOG                  0x2     // log
#define mfcEXP                  0x3     // exponent
#define mfcSQRT                 0x4     // square root
#define mfcRSQ                  0x5     // reciprocal square root
#define mfcSIN                  0x6     // sine (in radians)
#define mfcCOS                  0x7     // cosine (in radians)
#define mfcSINCOS               0x8     // dst0 = sin of src0, dst1 = cosine of src0 (in radians) - GT+ ONLY
#define mfcPOW                  0xA     // abs(src0) raised to the src1 power    
#define mfcINT_DIV_QR           0xB     // return quotient and remainder
#define mfcINT_DIV_Q            0xC     // return quotient
#define mfcINT_DIV_R            0xD     // return remainder


//=================== Message related registers =================================

#ifdef GT
        #define udDUMMY_NULL
#else   // _ILK
        #define udDUMMY_NULL    null:ud         // Used in send inst as src0
#endif

 
//----------- Message Registers ------------
#define mMSGHDR      m1     // Message Payload Header
#define mMSGHDRY     m1     // Message Payload Header register for Y data
#define mMSGHDRU     m2     // Message Payload Header register for U data
#define mMSGHDRV     m3     // Message Payload Header register for V data
#define mMSGHDRYA    m4     // Second Message Payload Header register for Y data
#define mMSGHDRH     m5     // Message Payload Header register for motion history
#define mMSGHDRY1    m1     // Message Payload Header register for first  Y data
#define mMSGHDRY2    m2     // Message Payload Header register for second Y data
#define mMSGHDRY3    m3     // Message Payload Header register for third  Y data
#define mMSGHDRY4    m4     // Message Payload Header register for fourth Y data
#define mMSGHDRY5    m5     // Message Payload Header register for fifth Y data
#define mMSGHDRY6    m6     // Message Payload Header register for sixth Y data
#define mMSGHDR_EOT  m15    // Dummy Message Register for EOT

#define rMSGSRC     r8      // Message source register
#define pMSGDSC     a0.0:ud // Message Descriptor register (type DWORD)

#define udMH_ORI    rMSGSRC.0   // Data Port Media Block R/W message header block offset
#define udMH_ORIX   rMSGSRC.0   // Data Port Media Block R/W message header X offset
#define udMH_ORIY   rMSGSRC.1   // Data Port Media Block R/W message header Y offset
#define udMH_SIZE   rMSGSRC.2   // Data Port Media Block R/W message header block width & height

//  M2 - M9 for message data payload
.declare    mubMSGPAYLOAD  Base=m2 ElementSize=1 SrcRegion=REGION(16,1) Type=ub
.declare    muwMSGPAYLOAD  Base=m2 ElementSize=2 SrcRegion=REGION(16,1) Type=uw
.declare    mudMSGPAYLOAD  Base=m2 ElementSize=4 SrcRegion=REGION(8,1) Type=ud
.declare    mfMSGPAYLOAD   Base=m2 ElementSize=4 SrcRegion=REGION(8,1) Type=f

//=================== End of thread instruction ===========================
#ifdef GT
	#define END_THREAD          mov  (8) mMSGHDR_EOT<1>:ud    r0.0<8;8,1>:ud \n\
								send (1) null<1>:d mMSGHDR_EOT nTS_EOT nEOT_MSGDSC 
#else   // ILK				This should be changed to 1 instruction; I have tested it and it works - vK
	#define END_THREAD          mov  (8) mMSGHDR_EOT<1>:ud    r0.0<8;8,1>:ud \n\
                            	send (1) dNULLREG mMSGHDR_EOT udDUMMY_NULL  nTS_EOT nEOT_MSGDSC:ud
#endif


//=======================================================================
// Region declarations for SRC and DEST as TOP and BOT

// Common I/O regions
#define nREGION_1       1
#define nREGION_2       2

//*** These region base GRFs are fixed regardless planar/packed, and data alignment.
//*** Each kernel is responsible to select the correct region declaration below.
//*** YUV regions are not necessarily next to each other.
#define nTOP_Y          10      // r10 - r17  (8 GRFs)
#define nTOP_U          18      // r18 - r25 (8 GRFs)
#define nTOP_V          26      // r26 - r33 (8 GRFs)

#define nBOT_Y          56      // r56 - r63 (8 GRFs)
#define nBOT_U          64      // r64 - r71 (8 GRFs)
#define nBOT_V          72      // r72 - r79 (8 GRFs)

// Define temp space for any usages
#define nTEMP0          34
#define nTEMP1          35
#define nTEMP2          36
#define nTEMP3          37
#define nTEMP4          38
#define nTEMP5          39
#define nTEMP6          40
#define nTEMP7          41
#define nTEMP8          42
#define nTEMP10         44
#define nTEMP12         46
#define nTEMP14         48
#define nTEMP16         50
#define nTEMP17         51
#define nTEMP18         52

#define nTEMP24			58

// Common region 1
.declare ubTOP_Y        Base=REG(r,nTOP_Y) ElementSize=1 SrcRegion=REGION(16,1) DstRegion=<1> Type=ub
.declare ubTOP_U        Base=REG(r,nTOP_U) ElementSize=1 SrcRegion=REGION(8,1) DstRegion=<1> Type=ub
.declare ubTOP_V        Base=REG(r,nTOP_V) ElementSize=1 SrcRegion=REGION(8,1) DstRegion=<1> Type=ub
                        
.declare uwTOP_Y        Base=REG(r,nTOP_Y) ElementSize=2 SrcRegion=REGION(16,1) DstRegion=<1> Type=uw
.declare uwTOP_U        Base=REG(r,nTOP_U) ElementSize=2 SrcRegion=REGION(8,1) DstRegion=<1> Type=uw
.declare uwTOP_V        Base=REG(r,nTOP_V) ElementSize=2 SrcRegion=REGION(8,1) DstRegion=<1> Type=uw
.declare ub2TOP_Y       Base=REG(r,nTOP_Y) ElementSize=1 SrcRegion=REGION(16,2) DstRegion=<1> Type=ub
.declare ub2TOP_U       Base=REG(r,nTOP_U) ElementSize=1 SrcRegion=REGION(8,2) DstRegion=<1> Type=ub
.declare ub2TOP_V       Base=REG(r,nTOP_V) ElementSize=1 SrcRegion=REGION(8,2) DstRegion=<1> Type=ub

.declare ub4TOP_Y       Base=REG(r,nTOP_Y) ElementSize=1 SrcRegion=REGION(8,4) Type=ub
.declare ub4TOP_U       Base=REG(r,nTOP_U) ElementSize=1 SrcRegion=REGION(8,4) Type=ub
.declare ub4TOP_V       Base=REG(r,nTOP_V) ElementSize=1 SrcRegion=REGION(8,4) Type=ub

.declare ubTOP_ARGB     Base=REG(r,nTOP_Y) ElementSize=1 SrcRegion=REGION(8,4) Type=ub

// Used by "send" instruction
.declare udTOP_Y_IO     Base=REG(r,nTOP_Y) ElementSize=4 SrcRegion=REGION(8,1) Type=ud
.declare udTOP_U_IO     Base=REG(r,nTOP_U) ElementSize=4 SrcRegion=REGION(8,1) Type=ud
.declare udTOP_V_IO     Base=REG(r,nTOP_V) ElementSize=4 SrcRegion=REGION(8,1) Type=ud

// Common region 2
.declare ubBOT_Y        Base=REG(r,nBOT_Y) ElementSize=1 SrcRegion=REGION(16,1) DstRegion=<1> Type=ub
.declare ubBOT_U        Base=REG(r,nBOT_U) ElementSize=1 SrcRegion=REGION(8,1) DstRegion=<1> Type=ub
.declare ubBOT_V        Base=REG(r,nBOT_V) ElementSize=1 SrcRegion=REGION(8,1) DstRegion=<1> Type=ub
                        
.declare uwBOT_Y        Base=REG(r,nBOT_Y) ElementSize=2 SrcRegion=REGION(16,1) DstRegion=<1> Type=uw
.declare uwBOT_U        Base=REG(r,nBOT_U) ElementSize=2 SrcRegion=REGION(8,1) DstRegion=<1> Type=uw
.declare uwBOT_V        Base=REG(r,nBOT_V) ElementSize=2 SrcRegion=REGION(8,1) DstRegion=<1> Type=uw
.declare ub2BOT_Y       Base=REG(r,nBOT_Y) ElementSize=1 SrcRegion=REGION(16,2) DstRegion=<1> Type=ub
.declare ub2BOT_U       Base=REG(r,nBOT_U) ElementSize=1 SrcRegion=REGION(8,2) DstRegion=<1> Type=ub
.declare ub2BOT_V       Base=REG(r,nBOT_V) ElementSize=1 SrcRegion=REGION(8,2) DstRegion=<1> Type=ub

.declare ubBOT_ARGB     Base=REG(r,nBOT_Y) ElementSize=1 SrcRegion=REGION(8,4) Type=ub

// Used by "send" instruction
.declare udBOT_Y_IO     Base=REG(r,nBOT_Y) ElementSize=4 SrcRegion=REGION(8,1) Type=ud
.declare udBOT_U_IO     Base=REG(r,nBOT_U) ElementSize=4 SrcRegion=REGION(8,1) Type=ud
.declare udBOT_V_IO     Base=REG(r,nBOT_V) ElementSize=4 SrcRegion=REGION(8,1) Type=ud

// End of common.inc

#endif    // COMMON_INC
