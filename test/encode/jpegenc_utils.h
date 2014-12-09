/*
 * Copyright (c) 2014 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * This file is a utilities file which supports JPEG Encode process
 */ 

#include <sys/types.h>
#include <stdio.h>

#define MAX_JPEG_COMPONENTS 3 //only for Y, U and V
#define JPEG_Y 0
#define JPEG_U 1
#define JPEG_V 2
#define NUM_QUANT_ELEMENTS 64
#define NUM_MAX_HUFFTABLE 2
#define NUM_AC_RUN_SIZE_BITS 16
#define NUM_AC_CODE_WORDS_HUFFVAL 162
#define NUM_DC_RUN_SIZE_BITS 16
#define NUM_DC_CODE_WORDS_HUFFVAL 12

#define BITSTREAM_ALLOCATE_STEPPING     4096

struct __bitstream {
    unsigned int *buffer;
    int bit_offset;
    int max_size_in_dword;
};

typedef struct __bitstream bitstream;

static unsigned int 
swap32(unsigned int val)
{
    unsigned char *pval = (unsigned char *)&val;

    return ((pval[0] << 24)     |
            (pval[1] << 16)     |
            (pval[2] << 8)      |
            (pval[3] << 0));
}

static void
bitstream_start(bitstream *bs)
{
    bs->max_size_in_dword = BITSTREAM_ALLOCATE_STEPPING;
    bs->buffer = calloc(bs->max_size_in_dword * sizeof(int), 1);
    bs->bit_offset = 0;
}

static void
bitstream_end(bitstream *bs)
{
    int pos = (bs->bit_offset >> 5);
    int bit_offset = (bs->bit_offset & 0x1f);
    int bit_left = 32 - bit_offset;

    if (bit_offset) {
        bs->buffer[pos] = swap32((bs->buffer[pos] << bit_left));
    }
}
 
static void
bitstream_put_ui(bitstream *bs, unsigned int val, int size_in_bits)
{
    int pos = (bs->bit_offset >> 5);
    int bit_offset = (bs->bit_offset & 0x1f);
    int bit_left = 32 - bit_offset;

    if (!size_in_bits)
        return;

    if (size_in_bits < 32)
        val &= ((1 << size_in_bits) - 1);

    bs->bit_offset += size_in_bits;

    if (bit_left > size_in_bits) {
        bs->buffer[pos] = (bs->buffer[pos] << size_in_bits | val);
    } else {
        size_in_bits -= bit_left;
        bs->buffer[pos] = (bs->buffer[pos] << bit_left) | (val >> size_in_bits);
        bs->buffer[pos] = swap32(bs->buffer[pos]);

        if (pos + 1 == bs->max_size_in_dword) {
            bs->max_size_in_dword += BITSTREAM_ALLOCATE_STEPPING;
            bs->buffer = realloc(bs->buffer, bs->max_size_in_dword * sizeof(unsigned int));
        }

        bs->buffer[pos + 1] = val;
    }
}

//As per Jpeg Spec ISO/IEC 10918-1, below values are assigned
enum jpeg_markers {

 //Define JPEG markers as 0xFFXX if you are adding the value directly to the buffer
 //Else define marker as 0xXXFF if you are assigning the marker to a structure variable.
 //This is needed because of the little-endedness of the IA 

 SOI  = 0xFFD8, //Start of Image 
 EOI  = 0xFFD9, //End of Image
 SOS  = 0xFFDA, //Start of Scan
 DQT  = 0xFFDB, //Define Quantization Table
 DRI  = 0xFFDD, //Define restart interval
 RST0 = 0xFFD0, //Restart interval termination
 DHT  = 0xFFC4, //Huffman table
 SOF0 = 0xFFC0, //Baseline DCT   
 APP0 = 0xFFE0, //Application Segment
 COM  = 0xFFFE  //Comment segment    
};

typedef struct _JPEGFrameHeader {
    
    uint16_t SOF;    //Start of Frame Header
    uint16_t Lf;     //Length of Frame Header
    uint8_t  P;      //Sample precision
    uint16_t Y;      //Number of lines
    uint16_t X;      //Number of samples per line
    uint8_t  Nf;     //Number of image components in frame
    
    struct _JPEGComponent {        
        uint8_t Ci;    //Component identifier
        uint8_t Hi:4;  //Horizontal sampling factor
        uint8_t Vi:4;  //Vertical sampling factor
        uint8_t Tqi;   //Quantization table destination selector        
    } JPEGComponent[MAX_JPEG_COMPONENTS];
    
} JPEGFrameHeader;


typedef struct _JPEGScanHeader {
    
    uint16_t SOS;  //Start of Scan
    uint16_t Ls;   //Length of Scan
    uint8_t  Ns;   //Number of image components in the scan
        
    struct _ScanComponent {
        uint8_t Csj;   //Scan component selector
        uint8_t Tdj:4; //DC Entropy coding table destination selector(Tdj:4 bits) 
        uint8_t Taj:4; //AC Entropy coding table destination selector(Taj:4 bits)       
    } ScanComponent[MAX_JPEG_COMPONENTS];
    
    uint8_t Ss;    //Start of spectral or predictor selection, 0 for Baseline
    uint8_t Se;    //End of spectral or predictor selection, 63 for Baseline
    uint8_t Ah:4;  //Successive approximation bit position high, 0 for Baseline
    uint8_t Al:4;  //Successive approximation bit position low, 0 for Baseline
    
} JPEGScanHeader;


typedef struct _JPEGQuantSection {
    
    uint16_t DQT;    //Quantization table marker
    uint16_t Lq;     //Length of Quantization table definition
    uint8_t  Tq:4;   //Quantization table destination identifier
    uint8_t  Pq:4;   //Quatization table precision. Should be 0 for 8-bit samples
    uint8_t  Qk[NUM_QUANT_ELEMENTS]; //Quantization table elements    
    
} JPEGQuantSection;

typedef struct _JPEGHuffSection {
    
        uint16_t DHT;                            //Huffman table marker
        uint16_t Lh;                             //Huffman table definition length
        uint8_t  Tc:4;                           //Table class- 0:DC, 1:AC
        uint8_t  Th:4;                           //Huffman table destination identifier
        uint8_t  Li[NUM_AC_RUN_SIZE_BITS];       //Number of Huffman codes of length i
        uint8_t  Vij[NUM_AC_CODE_WORDS_HUFFVAL]; //Value associated with each Huffman code
    
} JPEGHuffSection;


typedef struct _JPEGRestartSection {
    
    uint16_t DRI;  //Restart interval marker
    uint16_t Lr;   //Legth of restart interval segment
    uint16_t Ri;   //Restart interval
    
} JPEGRestartSection;


typedef struct _JPEGCommentSection {
    
    uint16_t COM;  //Comment marker
    uint16_t Lc;   //Comment segment length
    uint8_t  Cmi;  //Comment byte
    
} JPEGCommentSection;


typedef struct _JPEGAppSection {
    
    uint16_t APPn;  //Application data marker
    uint16_t Lp;    //Application data segment length
    uint8_t  Api;   //Application data byte
    
} JPEGAppSection;

//Luminance quantization table
//Source: Jpeg Spec ISO/IEC 10918-1, Annex K, Table K.1
uint8_t jpeg_luma_quant[NUM_QUANT_ELEMENTS] = {
    16, 11, 10, 16, 24,  40,  51,  61,
    12, 12, 14, 19, 26,  58,  60,  55,
    14, 13, 16, 24, 40,  57,  69,  56,
    14, 17, 22, 29, 51,  87,  80,  62,
    18, 22, 37, 56, 68,  109, 103, 77,
    24, 35, 55, 64, 81,  104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99    
};

//Luminance quantization table
//Source: Jpeg Spec ISO/IEC 10918-1, Annex K, Table K.2
uint8_t jpeg_chroma_quant[NUM_QUANT_ELEMENTS] = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};


//Zigzag scan order of the the Luma and Chroma components
//Note: Jpeg Spec ISO/IEC 10918-1, Figure A.6 shows the zigzag order differently.
//The Spec is trying to show the zigzag pattern with number positions. The below
//table will use the patter shown by A.6 and map the postion of the elements in the array
uint8_t jpeg_zigzag[] = {
    0,   1,   8,   16,  9,   2,   3,   10,
    17,  24,  32,  25,  18,  11,  4,   5,
    12,  19,  26,  33,  40,  48,  41,  34,
    27,  20,  13,  6,   7,   14,  21,  28,
    35,  42,  49,  56,  57,  50,  43,  36,
    29,  22,  15,  23,  30,  37,  44,  51,
    58,  59,  52,  45,  38,  31,  39,  46,
    53,  60,  61,  54,  47,  55,  62,  63
};


//Huffman table for Luminance DC Coefficients
//Reference Jpeg Spec ISO/IEC 10918-1, K.3.3.1
//K.3.3.1 is the summarized version of Table K.3
uint8_t jpeg_hufftable_luma_dc[] = {
    //TcTh (Tc=0 since 0:DC, 1:AC; Th=0)
    0x00,
    //Li
    0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    //Vi
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B
};

//Huffman table for Chrominance DC Coefficients
//Reference Jpeg Spec ISO/IEC 10918-1, K.3.3.1
//K.3.3.1 is the summarized version of Table K.4
uint8_t jpeg_hufftable_chroma_dc[] = {
    //TcTh (Tc=0 since 0:DC, 1:AC; Th=1)
    0x01,
    //Li
    0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    //Vi
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B    
};


//Huffman table for Luminance AC Coefficients
//Reference Jpeg Spec ISO/IEC 10918-1, K.3.3.2
//K.3.3.2 is the summarized version of Table K.5
uint8_t jpeg_hufftable_luma_ac[] = {
    //TcTh (Tc=1 since 0:DC, 1:AC; Th=0)
    0x10,
    //Li
    0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D,
    //Vi
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 
    0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 
    0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 
    0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 
    0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
    0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 
    0xF9, 0xFA
};

//Huffman table for Chrominance AC Coefficients
//Reference Jpeg Spec ISO/IEC 10918-1, K.3.3.2
//K.3.3.2 is the summarized version of Table K.6
uint8_t jpeg_hufftable_chroma_ac[] = {
    //TcTh (Tc=1 since 0:DC, 1:AC; Th=1)
    0x11,
    //Li
    0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77,
    //Vi
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0, 
    0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
    0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 
    0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 
    0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 
    0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 
    0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
    0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 
    0xF9, 0xFA
};

typedef struct _YUVComponentSpecs {
    //One of 0(I420)/1(NV12)/2(UYVY)/3(YUY2)/4(Y8)/5(RGBA)>
    unsigned int yuv_type;
    // One of VA_RT_FORMAT_YUV420, VA_RT_FORMAT_YUV422, VA_RT_FORMAT_YUV400, VA_RT_FORMAT_YUV444, VA_RT_FORMAT_RGB32
    unsigned int va_surface_format;
    //One of VA_FOURCC_I420, VA_FOURCC_NV12, VA_FOURCC_UYVY, VA_FOURCC_YUY2, VA_FOURCC_Y800, VA_FOURCC_444P, VA_FOURCC_RGBA
    unsigned int fourcc_val; //Using this field to evaluate the input file type.
    //no.of. components
    unsigned int num_components;
    //Y horizontal subsample
    unsigned int y_h_subsample;
    //Y vertical subsample
    unsigned int y_v_subsample;
    //U horizontal subsample
    unsigned int u_h_subsample;
    //U vertical subsample
    unsigned int u_v_subsample;
    //V horizontal subsample
    unsigned int v_h_subsample;
    //V vertical subsample
    unsigned int v_v_subsample;    
} YUVComponentSpecs;
