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
 * Simple JPEG encoder based on libVA.
 *
 * Usage:
 * ./jpegenc <width> <height> <input file> <output file> <input filetype 0(I420)/1(NV12)/2(UYVY)/3(YUY2)/4(Y8)/5(RGBA)> q <quality>
 * Currently supporting only I420/NV12/UYVY/YUY2/Y8 input file formats.
 * 
 * NOTE: The intel-driver expects a packed header sent to it. So, the app is responsible to pack the header
 * and send to the driver through LibVA. This unit test also showcases how to send the header to the driver.
 */

#include "sysdeps.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>

#include <pthread.h>

#include <va/va.h>
#include <va/va_enc_jpeg.h>
#include "va_display.h"
#include "jpegenc_utils.h"

#ifndef VA_FOURCC_I420
#define VA_FOURCC_I420          0x30323449
#endif

#define CHECK_VASTATUS(va_status,func)                                  \
    if (va_status != VA_STATUS_SUCCESS) {                                   \
        fprintf(stderr,"%s:%s (%d) failed,exit\n", __func__, func, __LINE__); \
        exit(1);                                                            \
    }


void show_help()
{
    printf("Usage: ./jpegenc <width> <height> <input file> <output file> <fourcc value 0(I420)/1(NV12)/2(UYVY)/3(YUY2)/4(Y8)/5(RGBA)> q <quality>\n");
    printf("Currently supporting only I420/NV12/UYVY/YUY2/Y8 input file formats.\n");
    printf("Example: ./jpegenc 1024 768 input_file.yuv output.jpeg 0 50\n\n");
    return;
}


void jpegenc_pic_param_init(VAEncPictureParameterBufferJPEG *pic_param,int width,int height,int quality, YUVComponentSpecs yuvComp)
{
    assert(pic_param);
    
    pic_param->picture_width = width;
    pic_param->picture_height = height;
    pic_param->quality = quality;
    
    pic_param->pic_flags.bits.profile = 0;      //Profile = Baseline
    pic_param->pic_flags.bits.progressive = 0;  //Sequential encoding
    pic_param->pic_flags.bits.huffman = 1;      //Uses Huffman coding
    pic_param->pic_flags.bits.interleaved = 0;  //Input format is interleaved (YUV)
    pic_param->pic_flags.bits.differential = 0; //non-Differential Encoding
    
    pic_param->sample_bit_depth = 8; //only 8 bit sample depth is currently supported
    pic_param->num_scan = 1;
    pic_param->num_components = yuvComp.num_components; // Supporting only upto 3 components maximum
    //set component_id Ci and Tqi
    if(yuvComp.fourcc_val == VA_FOURCC_Y800) {
        pic_param->component_id[0] = 0;
        pic_param->quantiser_table_selector[0] = 0;
    } else {
        pic_param->component_id[0] = pic_param->quantiser_table_selector[0] = 0;
        pic_param->component_id[1] = pic_param->quantiser_table_selector[1] = 1;
        pic_param->component_id[2] = 2; pic_param->quantiser_table_selector[2] = 1;
    }
    
    pic_param->quality = quality;
}

void jpegenc_qmatrix_init(VAQMatrixBufferJPEG *quantization_param, YUVComponentSpecs yuvComp)
{
    int i=0;
    quantization_param->load_lum_quantiser_matrix = 1;
   
    //LibVA expects the QM in zigzag order 
    for(i=0; i<NUM_QUANT_ELEMENTS; i++) {
        quantization_param->lum_quantiser_matrix[i] = jpeg_luma_quant[jpeg_zigzag[i]];
    }
    
    
    if(yuvComp.fourcc_val == VA_FOURCC_Y800) {
        quantization_param->load_chroma_quantiser_matrix = 0;
    } else {
        quantization_param->load_chroma_quantiser_matrix = 1;
        for(i=0; i<NUM_QUANT_ELEMENTS; i++) {
            quantization_param->chroma_quantiser_matrix[i] = jpeg_chroma_quant[jpeg_zigzag[i]];
        }
    }
    
}

void jpegenc_hufftable_init(VAHuffmanTableBufferJPEGBaseline *hufftable_param, YUVComponentSpecs yuvComp)
{
    
    hufftable_param->load_huffman_table[0] = 1; //Load Luma Hufftable
    if(yuvComp.fourcc_val == VA_FOURCC_Y800) {
        hufftable_param->load_huffman_table[1] = 0; //Do not load Chroma Hufftable for Y8
    } else {
        hufftable_param->load_huffman_table[1] = 1; //Load Chroma Hufftable for other formats
    }
    
   //Load Luma hufftable values
   //Load DC codes
   memcpy(hufftable_param->huffman_table[0].num_dc_codes, jpeg_hufftable_luma_dc+1, 16);
   //Load DC Values
   memcpy(hufftable_param->huffman_table[0].dc_values, jpeg_hufftable_luma_dc+17, 12);
   //Load AC codes
   memcpy(hufftable_param->huffman_table[0].num_ac_codes, jpeg_hufftable_luma_ac+1, 16);
   //Load AC Values
   memcpy(hufftable_param->huffman_table[0].ac_values, jpeg_hufftable_luma_ac+17, 162);
   memset(hufftable_param->huffman_table[0].pad, 0, 2);
      
   
   //Load Chroma hufftable values if needed
   if(yuvComp.fourcc_val != VA_FOURCC_Y800) {
       //Load DC codes
       memcpy(hufftable_param->huffman_table[1].num_dc_codes, jpeg_hufftable_chroma_dc+1, 16);
       //Load DC Values
       memcpy(hufftable_param->huffman_table[1].dc_values, jpeg_hufftable_chroma_dc+17, 12);
       //Load AC codes
       memcpy(hufftable_param->huffman_table[1].num_ac_codes, jpeg_hufftable_chroma_ac+1, 16);
       //Load AC Values
       memcpy(hufftable_param->huffman_table[1].ac_values, jpeg_hufftable_chroma_ac+17, 162);
       memset(hufftable_param->huffman_table[1].pad, 0, 2);      
       
   }
    
}

void jpegenc_slice_param_init(VAEncSliceParameterBufferJPEG *slice_param, YUVComponentSpecs yuvComp)
{
    slice_param->restart_interval = 0;
    
    slice_param->num_components = yuvComp.num_components;
    
    slice_param->components[0].component_selector = 1;
    slice_param->components[0].dc_table_selector = 0;
    slice_param->components[0].ac_table_selector = 0;        

    if(yuvComp.num_components > 1) { 
        slice_param->components[1].component_selector = 2;
        slice_param->components[1].dc_table_selector = 1;
        slice_param->components[1].ac_table_selector = 1;        

        slice_param->components[2].component_selector = 3;
        slice_param->components[2].dc_table_selector = 1;
        slice_param->components[2].ac_table_selector = 1;        
    }
}


void populate_quantdata(JPEGQuantSection *quantVal, int type)
{
    uint8_t zigzag_qm[NUM_QUANT_ELEMENTS];
    int i;

    quantVal->DQT = DQT;
    quantVal->Pq = 0;
    quantVal->Tq = type;
    if(type == 0) {
        for(i=0; i<NUM_QUANT_ELEMENTS; i++) {
            zigzag_qm[i] = jpeg_luma_quant[jpeg_zigzag[i]];
        }

        memcpy(quantVal->Qk, zigzag_qm, NUM_QUANT_ELEMENTS);
    } else {
        for(i=0; i<NUM_QUANT_ELEMENTS; i++) {
            zigzag_qm[i] = jpeg_chroma_quant[jpeg_zigzag[i]];
        }
        memcpy(quantVal->Qk, zigzag_qm, NUM_QUANT_ELEMENTS);
    }
    quantVal->Lq = 3 + NUM_QUANT_ELEMENTS;
}

void populate_frame_header(JPEGFrameHeader *frameHdr, YUVComponentSpecs yuvComp, int picture_width, int picture_height)
{
    int i=0;
    
    frameHdr->SOF = SOF0;
    frameHdr->Lf = 8 + (3 * yuvComp.num_components); //Size of FrameHeader in bytes without the Marker SOF
    frameHdr->P = 8;
    frameHdr->Y = picture_height;
    frameHdr->X = picture_width;
    frameHdr->Nf = yuvComp.num_components;
    
    for(i=0; i<yuvComp.num_components; i++) {
        frameHdr->JPEGComponent[i].Ci = i+1;
        
        if(i == 0) {
            frameHdr->JPEGComponent[i].Hi = yuvComp.y_h_subsample;
            frameHdr->JPEGComponent[i].Vi = yuvComp.y_v_subsample;
            frameHdr->JPEGComponent[i].Tqi = 0;

        } else {
            //Analyzing the sampling factors for U/V, they are 1 for all formats except for Y8. 
            //So, it is okay to have the code below like this. For Y8, we wont reach this code.
            frameHdr->JPEGComponent[i].Hi = yuvComp.u_h_subsample;
            frameHdr->JPEGComponent[i].Vi = yuvComp.u_v_subsample;
            frameHdr->JPEGComponent[i].Tqi = 1;
        }
    }
}

void populate_huff_section_header(JPEGHuffSection *huffSectionHdr, int th, int tc)
{
    int i=0, totalCodeWords=0;
    
    huffSectionHdr->DHT = DHT;
    huffSectionHdr->Tc = tc;
    huffSectionHdr->Th = th;
    
    if(th == 0) { //If Luma

        //If AC
        if(tc == 1) {
            memcpy(huffSectionHdr->Li, jpeg_hufftable_luma_ac+1, NUM_AC_RUN_SIZE_BITS);
            memcpy(huffSectionHdr->Vij, jpeg_hufftable_luma_ac+17, NUM_AC_CODE_WORDS_HUFFVAL);
        }
               
        //If DC        
        if(tc == 0) {
            memcpy(huffSectionHdr->Li, jpeg_hufftable_luma_dc+1, NUM_DC_RUN_SIZE_BITS);
            memcpy(huffSectionHdr->Vij, jpeg_hufftable_luma_dc+17, NUM_DC_CODE_WORDS_HUFFVAL);
        }
        
        for(i=0; i<NUM_AC_RUN_SIZE_BITS; i++) {
            totalCodeWords += huffSectionHdr->Li[i];
        }
        
        huffSectionHdr->Lh = 3 + 16 + totalCodeWords;

    } else { //If Chroma
        //If AC
        if(tc == 1) {
            memcpy(huffSectionHdr->Li, jpeg_hufftable_chroma_ac+1, NUM_AC_RUN_SIZE_BITS);
            memcpy(huffSectionHdr->Vij, jpeg_hufftable_chroma_ac+17, NUM_AC_CODE_WORDS_HUFFVAL);
        }
               
        //If DC        
        if(tc == 0) {
            memcpy(huffSectionHdr->Li, jpeg_hufftable_chroma_dc+1, NUM_DC_RUN_SIZE_BITS);
            memcpy(huffSectionHdr->Vij, jpeg_hufftable_chroma_dc+17, NUM_DC_CODE_WORDS_HUFFVAL);
        }

    }
}

void populate_scan_header(JPEGScanHeader *scanHdr, YUVComponentSpecs yuvComp)
{
    
    scanHdr->SOS = SOS;
    scanHdr->Ns = yuvComp.num_components;
    
    //Y Component
    scanHdr->ScanComponent[0].Csj = 1;
    scanHdr->ScanComponent[0].Tdj = 0;
    scanHdr->ScanComponent[0].Taj = 0;
    
    if(yuvComp.num_components > 1) {
        //U Component
        scanHdr->ScanComponent[1].Csj = 2;
        scanHdr->ScanComponent[1].Tdj = 1;
        scanHdr->ScanComponent[1].Taj = 1;
        
        //V Component
        scanHdr->ScanComponent[2].Csj = 3;
        scanHdr->ScanComponent[2].Tdj = 1;
        scanHdr->ScanComponent[2].Taj = 1;
    }
    
    scanHdr->Ss = 0;  //0 for Baseline
    scanHdr->Se = 63; //63 for Baseline
    scanHdr->Ah = 0;  //0 for Baseline
    scanHdr->Al = 0;  //0 for Baseline
    
    scanHdr->Ls = 3 + (yuvComp.num_components * 2) + 3;
    
}

// This method packs the header information which is to be sent to the driver through LibVA.
// All the information that needs to be inserted in the encoded buffer should be built and sent.
// It is the responsibility of the app talking to LibVA to build this header and send it.
// This includes Markers, Quantization tables (normalized with quality factor), Huffman tables,etc.
int build_packed_jpeg_header_buffer(unsigned char **header_buffer, YUVComponentSpecs yuvComp, int picture_width, int picture_height, uint16_t restart_interval, int quality)
{
    bitstream bs;
    int i=0, j=0;
    uint32_t temp=0;
    
    bitstream_start(&bs);
    
    //Add SOI
    bitstream_put_ui(&bs, SOI, 16);
    
    //Add AppData
    bitstream_put_ui(&bs, APP0, 16);  //APP0 marker
    bitstream_put_ui(&bs, 16, 16);    //Length excluding the marker
    bitstream_put_ui(&bs, 0x4A, 8);   //J
    bitstream_put_ui(&bs, 0x46, 8);   //F
    bitstream_put_ui(&bs, 0x49, 8);   //I
    bitstream_put_ui(&bs, 0x46, 8);   //F
    bitstream_put_ui(&bs, 0x00, 8);   //0
    bitstream_put_ui(&bs, 1, 8);      //Major Version
    bitstream_put_ui(&bs, 1, 8);      //Minor Version
    bitstream_put_ui(&bs, 1, 8);      //Density units 0:no units, 1:pixels per inch, 2: pixels per cm
    bitstream_put_ui(&bs, 72, 16);    //X density
    bitstream_put_ui(&bs, 72, 16);    //Y density
    bitstream_put_ui(&bs, 0, 8);      //Thumbnail width
    bitstream_put_ui(&bs, 0, 8);      //Thumbnail height

    // Regarding Quantization matrices: As per JPEG Spec ISO/IEC 10918-1:1993(E), Pg-19:
    // "applications may specify values which customize picture quality for their particular
    // image characteristics, display devices, and viewing conditions"


    //Normalization of quality factor
    quality = (quality < 50) ? (5000/quality) : (200 - (quality*2));
    
    //Add QTable - Y
    JPEGQuantSection quantLuma;
    populate_quantdata(&quantLuma, 0);

    bitstream_put_ui(&bs, quantLuma.DQT, 16);
    bitstream_put_ui(&bs, quantLuma.Lq, 16);
    bitstream_put_ui(&bs, quantLuma.Pq, 4);
    bitstream_put_ui(&bs, quantLuma.Tq, 4);
    for(i=0; i<NUM_QUANT_ELEMENTS; i++) {
        //scale the quantization table with quality factor
        temp = (quantLuma.Qk[i] * quality)/100;
        //clamp to range [1,255]
        temp = (temp > 255) ? 255 : temp;
        temp = (temp < 1) ? 1 : temp;
        quantLuma.Qk[i] = (unsigned char)temp;
        bitstream_put_ui(&bs, quantLuma.Qk[i], 8);
    }

    //Add QTable - U/V
    if(yuvComp.fourcc_val != VA_FOURCC_Y800) {
        JPEGQuantSection quantChroma;
        populate_quantdata(&quantChroma, 1);
        
        bitstream_put_ui(&bs, quantChroma.DQT, 16);
        bitstream_put_ui(&bs, quantChroma.Lq, 16);
        bitstream_put_ui(&bs, quantChroma.Pq, 4);
        bitstream_put_ui(&bs, quantChroma.Tq, 4);
        for(i=0; i<NUM_QUANT_ELEMENTS; i++) {
            //scale the quantization table with quality factor
            temp = (quantChroma.Qk[i] * quality)/100;
            //clamp to range [1,255]
            temp = (temp > 255) ? 255 : temp;
            temp = (temp < 1) ? 1 : temp;
            quantChroma.Qk[i] = (unsigned char)temp;
            bitstream_put_ui(&bs, quantChroma.Qk[i], 8);
        }
    }
    
    //Add FrameHeader
    JPEGFrameHeader frameHdr;
    memset(&frameHdr,0,sizeof(JPEGFrameHeader));
    populate_frame_header(&frameHdr, yuvComp, picture_width, picture_height);

    bitstream_put_ui(&bs, frameHdr.SOF, 16);
    bitstream_put_ui(&bs, frameHdr.Lf, 16);
    bitstream_put_ui(&bs, frameHdr.P, 8);
    bitstream_put_ui(&bs, frameHdr.Y, 16);
    bitstream_put_ui(&bs, frameHdr.X, 16);
    bitstream_put_ui(&bs, frameHdr.Nf, 8);
    for(i=0; i<frameHdr.Nf;i++) {
        bitstream_put_ui(&bs, frameHdr.JPEGComponent[i].Ci, 8);
        bitstream_put_ui(&bs, frameHdr.JPEGComponent[i].Hi, 4);
        bitstream_put_ui(&bs, frameHdr.JPEGComponent[i].Vi, 4);
        bitstream_put_ui(&bs, frameHdr.JPEGComponent[i].Tqi, 8);
    }

    //Add HuffTable AC and DC for Y,U/V components
    JPEGHuffSection acHuffSectionHdr, dcHuffSectionHdr;
        
    for(i=0; (i<yuvComp.num_components && (i<=1)); i++) {
        //Add DC component (Tc = 0)
        populate_huff_section_header(&dcHuffSectionHdr, i, 0); 
        
        bitstream_put_ui(&bs, dcHuffSectionHdr.DHT, 16);
        bitstream_put_ui(&bs, dcHuffSectionHdr.Lh, 16);
        bitstream_put_ui(&bs, dcHuffSectionHdr.Tc, 4);
        bitstream_put_ui(&bs, dcHuffSectionHdr.Th, 4);
        for(j=0; j<NUM_DC_RUN_SIZE_BITS; j++) {
            bitstream_put_ui(&bs, dcHuffSectionHdr.Li[j], 8);
        }
        
        for(j=0; j<NUM_DC_CODE_WORDS_HUFFVAL; j++) {
            bitstream_put_ui(&bs, dcHuffSectionHdr.Vij[j], 8);
        }

        //Add AC component (Tc = 1)
        populate_huff_section_header(&acHuffSectionHdr, i, 1);
        
        bitstream_put_ui(&bs, acHuffSectionHdr.DHT, 16);
        bitstream_put_ui(&bs, acHuffSectionHdr.Lh, 16);
        bitstream_put_ui(&bs, acHuffSectionHdr.Tc, 4);
        bitstream_put_ui(&bs, acHuffSectionHdr.Th, 4);
        for(j=0; j<NUM_AC_RUN_SIZE_BITS; j++) {
            bitstream_put_ui(&bs, acHuffSectionHdr.Li[j], 8);
        }

        for(j=0; j<NUM_AC_CODE_WORDS_HUFFVAL; j++) {
            bitstream_put_ui(&bs, acHuffSectionHdr.Vij[j], 8);
        }

        if((yuvComp.fourcc_val == VA_FOURCC_Y800) )
            break;
    }
    
    //Add Restart Interval if restart_interval is not 0
    if(restart_interval != 0) {
        JPEGRestartSection restartHdr;
        restartHdr.DRI = DRI;
        restartHdr.Lr = 4;
        restartHdr.Ri = restart_interval;

        bitstream_put_ui(&bs, restartHdr.DRI, 16); 
        bitstream_put_ui(&bs, restartHdr.Lr, 16);
        bitstream_put_ui(&bs, restartHdr.Ri, 16); 
    }
    
    //Add ScanHeader
    JPEGScanHeader scanHdr;
    populate_scan_header(&scanHdr, yuvComp);
 
    bitstream_put_ui(&bs, scanHdr.SOS, 16);
    bitstream_put_ui(&bs, scanHdr.Ls, 16);
    bitstream_put_ui(&bs, scanHdr.Ns, 8);
    
    for(i=0; i<scanHdr.Ns; i++) {
        bitstream_put_ui(&bs, scanHdr.ScanComponent[i].Csj, 8);
        bitstream_put_ui(&bs, scanHdr.ScanComponent[i].Tdj, 4);
        bitstream_put_ui(&bs, scanHdr.ScanComponent[i].Taj, 4);
    }

    bitstream_put_ui(&bs, scanHdr.Ss, 8);
    bitstream_put_ui(&bs, scanHdr.Se, 8);
    bitstream_put_ui(&bs, scanHdr.Ah, 4);
    bitstream_put_ui(&bs, scanHdr.Al, 4);

    bitstream_end(&bs);
    *header_buffer = (unsigned char *)bs.buffer;
    
    return bs.bit_offset;
}

//Upload the yuv image from the file to the VASurface
void upload_yuv_to_surface(VADisplay va_dpy, FILE *yuv_fp, VASurfaceID surface_id, YUVComponentSpecs yuvComp, int picture_width, int picture_height, int frame_size)
{

    VAImage surface_image;
    VAStatus va_status;
    void *surface_p = NULL;
    unsigned char newImageBuffer[frame_size];
    unsigned char *y_src, *u_src, *v_src;
    unsigned char *y_dst, *u_dst;
    int y_size = picture_width * picture_height;
    int u_size = 0;
    int row, col;
    size_t n_items;

    //u_size is used for I420, NV12 formats only
    u_size = ((picture_width >> 1) * (picture_height >> 1));

    memset(newImageBuffer,0,frame_size);
    do {
        n_items = fread(newImageBuffer, frame_size, 1, yuv_fp);
    } while (n_items != 1);

    va_status = vaDeriveImage(va_dpy, surface_id, &surface_image);
    CHECK_VASTATUS(va_status,"vaDeriveImage");

    vaMapBuffer(va_dpy, surface_image.buf, &surface_p);
    assert(VA_STATUS_SUCCESS == va_status);

    y_src = newImageBuffer;
    u_src = newImageBuffer + y_size; /* UV offset for NV12 */
    v_src = newImageBuffer + y_size + u_size;

    y_dst = surface_p + surface_image.offsets[0];
    u_dst = surface_p + surface_image.offsets[1]; /* UV offset for NV12 */

    if((yuvComp.fourcc_val == VA_FOURCC_NV12) || (yuvComp.fourcc_val == VA_FOURCC_I420) ||
       (yuvComp.fourcc_val == VA_FOURCC_Y800) ) {

        /* Y plane */
        for (row = 0; row < surface_image.height; row++) {
            memcpy(y_dst, y_src, surface_image.width);
            y_dst += surface_image.pitches[0];
            y_src += picture_width;
        }
        
        if(yuvComp.num_components > 1) {

            switch(yuvComp.fourcc_val) {
                case VA_FOURCC_NV12: {
                    for (row = 0; row < surface_image.height/2; row++) {
                        memcpy(u_dst, u_src, surface_image.width);
                        u_dst += surface_image.pitches[1];
                        u_src += (picture_width);
                    }
                    break;
                }

                case VA_FOURCC_I420: {
                    for (row = 0; row < surface_image.height / 2; row++) {
                        for (col = 0; col < surface_image.width / 2; col++) {
                             u_dst[col * 2] = u_src[col];
                             u_dst[col * 2 + 1] = v_src[col];
                         }

                         u_dst += surface_image.pitches[1];
                         u_src += (picture_width / 2);
                         v_src += (picture_width / 2);
                    }
                    break;
                }
            }//end of switch
       }//end of if check
    } else if((yuvComp.fourcc_val == VA_FOURCC_UYVY) || (yuvComp.fourcc_val == VA_FOURCC_YUY2)) {

        for(row = 0; row < surface_image.height; row++) {
            memcpy(y_dst, y_src, surface_image.width*2);
            y_dst += surface_image.pitches[0];
            y_src += picture_width*2;
        }
             
    } else if(yuvComp.fourcc_val == VA_FOURCC_RGBA) {

        for (row = 0; row < surface_image.height; row++) {
            memcpy(y_dst, y_src, surface_image.width*4);
            y_dst += surface_image.pitches[0];
            y_src += picture_width*4;
        }
    }
    
    vaUnmapBuffer(va_dpy, surface_image.buf);
    vaDestroyImage(va_dpy, surface_image.image_id);
    
}



void init_yuv_component(YUVComponentSpecs *yuvComponent, int yuv_type, int *surface_type, VASurfaceAttrib *fourcc)
{
    
    //<fourcc value 0(I420)/1(NV12)/2(UYVY)/3(YUY2)/4(Y8)/5(RGBA)>
    switch(yuv_type)
    {
        case 0 :   //I420
        case 1 : { //NV12
            yuvComponent->va_surface_format = (*surface_type) = VA_RT_FORMAT_YUV420;
            if(yuv_type == 0) {
                yuvComponent->fourcc_val = VA_FOURCC_I420;
                fourcc->value.value.i = VA_FOURCC_NV12;
            } else {
                yuvComponent->fourcc_val = fourcc->value.value.i = VA_FOURCC_NV12;
            } 
            yuvComponent->num_components = 3;
            yuvComponent->y_h_subsample = 2;
            yuvComponent->y_v_subsample = 2;
            yuvComponent->u_h_subsample = 1;
            yuvComponent->u_v_subsample = 1;
            yuvComponent->v_h_subsample = 1;
            yuvComponent->v_v_subsample = 1;            
            break;
        }
        
        case 2: { //UYVY 
            yuvComponent->va_surface_format = (*surface_type) = VA_RT_FORMAT_YUV422;
            yuvComponent->fourcc_val = fourcc->value.value.i = VA_FOURCC_UYVY;
            yuvComponent->num_components = 3;
            yuvComponent->y_h_subsample = 2;
            yuvComponent->y_v_subsample = 1;
            yuvComponent->u_h_subsample = 1;
            yuvComponent->u_v_subsample = 1;
            yuvComponent->v_h_subsample = 1;
            yuvComponent->v_v_subsample = 1;
            break;
        }
        
        case 3: { //YUY2
            yuvComponent->va_surface_format = (*surface_type) = VA_RT_FORMAT_YUV422;
            yuvComponent->fourcc_val = fourcc->value.value.i = VA_FOURCC_YUY2;
            yuvComponent->num_components = 3;
            yuvComponent->y_h_subsample = 2;
            yuvComponent->y_v_subsample = 1;
            yuvComponent->u_h_subsample = 1;
            yuvComponent->u_v_subsample = 1;
            yuvComponent->v_h_subsample = 1;
            yuvComponent->v_v_subsample = 1;
            break;
        }
        
        case 4: { //Y8
            yuvComponent->va_surface_format = (*surface_type) = VA_RT_FORMAT_YUV400;
            yuvComponent->fourcc_val = fourcc->value.value.i = VA_FOURCC_Y800;
            yuvComponent->num_components = 1;
            yuvComponent->y_h_subsample = 1;
            yuvComponent->y_v_subsample = 1;
            yuvComponent->u_h_subsample = 0;
            yuvComponent->u_v_subsample = 0;
            yuvComponent->v_h_subsample = 0;
            yuvComponent->v_v_subsample = 0;
            break;
        }
        
        case 5: { //RGBA
            yuvComponent->va_surface_format = (*surface_type) = VA_RT_FORMAT_RGB32;
            yuvComponent->fourcc_val = fourcc->value.value.i = VA_FOURCC_RGBA;
            yuvComponent->num_components = 3;
            yuvComponent->y_h_subsample = 1;
            yuvComponent->y_v_subsample = 1;
            yuvComponent->u_h_subsample = 1;
            yuvComponent->u_v_subsample = 1;
            yuvComponent->v_h_subsample = 1;
            yuvComponent->v_v_subsample = 1;
            break;
        }
        
        default: {
            printf("Unsupported format:\n");
            show_help();
            break;
        }
        
    }
    
}

int encode_input_image(FILE *yuv_fp, FILE *jpeg_fp, int picture_width, int picture_height, int frame_size, int yuv_type, int quality)
{
    int num_entrypoints,enc_entrypoint;
    int major_ver, minor_ver;
    int surface_type;
    VAEntrypoint entrypoints[5];
    VASurfaceAttrib fourcc;
    VAConfigAttrib attrib[2];
    VADisplay   va_dpy;
    VAStatus va_status;
    VAConfigID config_id;
    VASurfaceID surface_id;
    VAContextID context_id;
    VABufferID pic_param_buf_id;                /* Picture parameter id*/
    VABufferID slice_param_buf_id;              /* Slice parameter id, only 1 slice per frame in jpeg encode */
    VABufferID codedbuf_buf_id;                 /* Output buffer id, compressed data */
    VABufferID packed_raw_header_param_buf_id;  /* Header parameter buffer id */
    VABufferID packed_raw_header_buf_id;        /* Header buffer id */
    VABufferID qmatrix_buf_id;                  /* Quantization Matrix id */
    VABufferID huffmantable_buf_id;             /* Huffman table id*/
    VAEncPictureParameterBufferJPEG pic_param;  /* Picture parameter buffer */
    VAEncSliceParameterBufferJPEG slice_param;  /* Slice parameter buffer */
    VAQMatrixBufferJPEG quantization_param;     /* Quantization Matrix buffer */
    VAHuffmanTableBufferJPEGBaseline hufftable_param; /* Huffmantable buffer */
    YUVComponentSpecs yuvComponent;
    int writeToFile = 1;
    
    //Clamp the quality factor value to [1,100]
    if(quality >= 100) quality=100;
    if(quality <= 0) quality=1;
    
    fourcc.type =VASurfaceAttribPixelFormat;
    fourcc.flags=VA_SURFACE_ATTRIB_SETTABLE;
    fourcc.value.type=VAGenericValueTypeInteger;
    
    init_yuv_component(&yuvComponent, yuv_type, &surface_type, &fourcc);
    
    /* 1. Initialize the va driver */
    va_dpy = va_open_display();
    va_status = vaInitialize(va_dpy, &major_ver, &minor_ver);
    assert(va_status == VA_STATUS_SUCCESS);
    
    /* 2. Query for the entrypoints for the JPEGBaseline profile */
    va_status = vaQueryConfigEntrypoints(va_dpy, VAProfileJPEGBaseline, entrypoints, &num_entrypoints);
    CHECK_VASTATUS(va_status, "vaQueryConfigEntrypoints");
    // We need picture level encoding (VAEntrypointEncPicture). Find if it is supported. 
    for (enc_entrypoint = 0; enc_entrypoint < num_entrypoints; enc_entrypoint++) {
        if (entrypoints[enc_entrypoint] == VAEntrypointEncPicture)
            break;
    }
    if (enc_entrypoint == num_entrypoints) {
        /* No JPEG Encode (VAEntrypointEncPicture) entry point found */
        assert(0);
    }
    
    /* 3. Query for the Render Target format supported */
    attrib[0].type = VAConfigAttribRTFormat;
    attrib[1].type = VAConfigAttribEncJPEG;
    vaGetConfigAttributes(va_dpy, VAProfileJPEGBaseline, VAEntrypointEncPicture, &attrib[0], 2);

    // RT should be one of below.
    if(!((attrib[0].value & VA_RT_FORMAT_YUV420) || (attrib[0].value & VA_RT_FORMAT_YUV422) || (attrib[0].value & VA_RT_FORMAT_RGB32)
        ||(attrib[0].value & VA_RT_FORMAT_YUV444) || (attrib[0].value & VA_RT_FORMAT_YUV400))) 
    {
        /* Did not find the supported RT format */
        assert(0);        
    }

    VAConfigAttribValEncJPEG jpeg_attrib_val;
    jpeg_attrib_val.value = attrib[1].value;

    /* Set JPEG profile attribs */
    jpeg_attrib_val.bits.arithmatic_coding_mode = 0;
    jpeg_attrib_val.bits.progressive_dct_mode = 0;
    jpeg_attrib_val.bits.non_interleaved_mode = 1;
    jpeg_attrib_val.bits.differential_mode = 0;

    attrib[1].value = jpeg_attrib_val.value;
    
    /* 4. Create Config for the profile=VAProfileJPEGBaseline, entrypoint=VAEntrypointEncPicture,
     * with RT format attribute */
    va_status = vaCreateConfig(va_dpy, VAProfileJPEGBaseline, VAEntrypointEncPicture, 
                               &attrib[0], 2, &config_id);
    CHECK_VASTATUS(va_status, "vaQueryConfigEntrypoints");
    
    /* 5. Create Surface for the input picture */
    va_status = vaCreateSurfaces(va_dpy, surface_type, picture_width, picture_height, 
                                 &surface_id, 1, &fourcc, 1);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");
    
    //Map the input yuv file to the input surface created with the surface_id
    upload_yuv_to_surface(va_dpy, yuv_fp, surface_id, yuvComponent, picture_width, picture_height, frame_size);
    
    /* 6. Create Context for the encode pipe*/
    va_status = vaCreateContext(va_dpy, config_id, picture_width, picture_height, 
                                VA_PROGRESSIVE, &surface_id, 1, &context_id);
    CHECK_VASTATUS(va_status, "vaCreateContext");

    /* Create buffer for Encoded data to be stored */
    va_status =  vaCreateBuffer(va_dpy, context_id, VAEncCodedBufferType,
                                   frame_size, 1, NULL, &codedbuf_buf_id);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");
    
    //Initialize the picture parameter buffer
    pic_param.coded_buf = codedbuf_buf_id;
    jpegenc_pic_param_init(&pic_param, picture_width, picture_height, quality, yuvComponent);
    
    /* 7. Create buffer for the picture parameter */
    va_status = vaCreateBuffer(va_dpy, context_id, VAEncPictureParameterBufferType,
                               sizeof(VAEncPictureParameterBufferJPEG), 1, &pic_param, &pic_param_buf_id);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");
    
    //Load the QMatrix 
    jpegenc_qmatrix_init(&quantization_param, yuvComponent);
    
    /* 8. Create buffer for Quantization Matrix */
    va_status = vaCreateBuffer(va_dpy, context_id, VAQMatrixBufferType, 
                               sizeof(VAQMatrixBufferJPEG), 1, &quantization_param, &qmatrix_buf_id);
    CHECK_VASTATUS(va_status, "vaCreateBuffer");
    
    //Load the Huffman Tables
    jpegenc_hufftable_init(&hufftable_param, yuvComponent);
    
    /* 9. Create buffer for Huffman Tables */
    va_status = vaCreateBuffer(va_dpy, context_id, VAHuffmanTableBufferType, 
                               sizeof(VAHuffmanTableBufferJPEGBaseline), 1, &hufftable_param, &huffmantable_buf_id);
    CHECK_VASTATUS(va_status, "vaCreateBuffer");
    
    //Initialize the slice parameter buffer
    jpegenc_slice_param_init(&slice_param, yuvComponent);
    
    /* 10. Create buffer for slice parameter */
    va_status = vaCreateBuffer(va_dpy, context_id, VAEncSliceParameterBufferType, 
                            sizeof(slice_param), 1, &slice_param, &slice_param_buf_id);
    CHECK_VASTATUS(va_status, "vaCreateBuffer");

    //Pack headers and send using Raw data buffer
    VAEncPackedHeaderParameterBuffer packed_header_param_buffer;
    unsigned int length_in_bits;
    unsigned char *packed_header_buffer = NULL;

    length_in_bits = build_packed_jpeg_header_buffer(&packed_header_buffer, yuvComponent, picture_width, picture_height, slice_param.restart_interval, quality);
    packed_header_param_buffer.type = VAEncPackedHeaderRawData;
    packed_header_param_buffer.bit_length = length_in_bits;
    packed_header_param_buffer.has_emulation_bytes = 0;
    
    /* 11. Create raw buffer for header */
    va_status = vaCreateBuffer(va_dpy,
                               context_id,
                               VAEncPackedHeaderParameterBufferType,
                               sizeof(packed_header_param_buffer), 1, &packed_header_param_buffer,
                               &packed_raw_header_param_buf_id);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");

    va_status = vaCreateBuffer(va_dpy,
                               context_id,
                               VAEncPackedHeaderDataBufferType,
                               (length_in_bits + 7) / 8, 1, packed_header_buffer,
                               &packed_raw_header_buf_id);
    CHECK_VASTATUS(va_status,"vaCreateBuffer");
    
    /* 12. Begin picture */
    va_status = vaBeginPicture(va_dpy, context_id, surface_id);
    CHECK_VASTATUS(va_status, "vaBeginPicture");   

    /* 13. Render picture for all the VA buffers created */
    va_status = vaRenderPicture(va_dpy,context_id, &pic_param_buf_id, 1);
    CHECK_VASTATUS(va_status, "vaRenderPicture");
   
    va_status = vaRenderPicture(va_dpy,context_id, &qmatrix_buf_id, 1);
    CHECK_VASTATUS(va_status, "vaRenderPicture");

    va_status = vaRenderPicture(va_dpy,context_id, &huffmantable_buf_id, 1);
    CHECK_VASTATUS(va_status, "vaRenderPicture");
   
    va_status = vaRenderPicture(va_dpy,context_id, &slice_param_buf_id, 1);
    CHECK_VASTATUS(va_status, "vaRenderPicture");
    
    va_status = vaRenderPicture(va_dpy,context_id, &packed_raw_header_param_buf_id, 1);
    CHECK_VASTATUS(va_status, "vaRenderPicture");
    
    va_status = vaRenderPicture(va_dpy,context_id, &packed_raw_header_buf_id, 1);
    CHECK_VASTATUS(va_status, "vaRenderPicture");
    
    va_status = vaEndPicture(va_dpy,context_id);
    CHECK_VASTATUS(va_status, "vaEndPicture");

    if (writeToFile) {
        VASurfaceStatus surface_status;
        size_t w_items;
        VACodedBufferSegment *coded_buffer_segment;
        unsigned char *coded_mem;
        int slice_data_length;

        va_status = vaSyncSurface(va_dpy, surface_id);
        CHECK_VASTATUS(va_status, "vaSyncSurface");
    
        surface_status = 0;
        va_status = vaQuerySurfaceStatus(va_dpy, surface_id, &surface_status);
        CHECK_VASTATUS(va_status,"vaQuerySurfaceStatus");

        va_status = vaMapBuffer(va_dpy, codedbuf_buf_id, (void **)(&coded_buffer_segment));
        CHECK_VASTATUS(va_status,"vaMapBuffer");

        coded_mem = coded_buffer_segment->buf;
  
       if (coded_buffer_segment->status & VA_CODED_BUF_STATUS_SLICE_OVERFLOW_MASK) {
            vaUnmapBuffer(va_dpy, codedbuf_buf_id);
            printf("ERROR......Coded buffer too small\n");
        }


        slice_data_length = coded_buffer_segment->size;

        do {
            w_items = fwrite(coded_mem, slice_data_length, 1, jpeg_fp);
        } while (w_items != 1);

        va_status = vaUnmapBuffer(va_dpy, codedbuf_buf_id);
        CHECK_VASTATUS(va_status, "vaUnmapBuffer");
    }
       
    vaDestroyBuffer(va_dpy, pic_param_buf_id);
    vaDestroyBuffer(va_dpy, qmatrix_buf_id);
    vaDestroyBuffer(va_dpy, slice_param_buf_id);
    vaDestroyBuffer(va_dpy, huffmantable_buf_id);
    vaDestroyBuffer(va_dpy, codedbuf_buf_id);
    vaDestroyBuffer(va_dpy, packed_raw_header_param_buf_id);
    vaDestroyBuffer(va_dpy, packed_raw_header_buf_id);
    vaDestroySurfaces(va_dpy,&surface_id,1);
    vaDestroyContext(va_dpy,context_id);
    vaDestroyConfig(va_dpy,config_id);
    vaTerminate(va_dpy);
    va_close_display(va_dpy);

    return 0;
}


int main(int argc, char *argv[])
{
    FILE *yuv_fp;
    FILE *jpeg_fp;
    off_t file_size;
    clock_t start_time, finish_time;
    unsigned int duration;
    unsigned int yuv_type = 0;
    int quality = 0;
    unsigned int picture_width = 0;
    unsigned int picture_height = 0;
    unsigned int frame_size = 0;
    
    va_init_display_args(&argc, argv);
    
    if(argc != 7) {
        show_help();
        return -1;
    }
    
    picture_width = atoi(argv[1]);
    picture_height = atoi(argv[2]);
    yuv_type = atoi(argv[5]);
    quality = atoi(argv[6]);
    
    yuv_fp = fopen(argv[3],"rb");
    if ( yuv_fp == NULL){
        printf("Can't open input YUV file\n");
        return -1;
    }
    
    fseeko(yuv_fp, (off_t)0, SEEK_END);
    file_size = ftello(yuv_fp);
    
    //<input file type: 0(I420)/1(NV12)/2(UYVY)/3(YUY2)/4(Y8)/5(RGBA)>
    switch(yuv_type)
    {
        case 0 :   //I420 
        case 1 : { //NV12
            frame_size = picture_width * picture_height +  ((picture_width * picture_height) >> 1) ; 
            break;
        }
        
        case 2:  //UYVY
        case 3: { //YUY2
           frame_size = 2 * (picture_width * picture_height); 
           break;
        }
        
        case 4: { //Y8
            frame_size = picture_width * picture_height;
            break;
        }
        
        case 5: { //RGBA
            frame_size = 4 * (picture_width * picture_height) ;
            break;
        }
        
        default: {
            printf("Unsupported format:\n");
            show_help();
            break;
        }
        
    }
    
    if ( (file_size < frame_size) || (file_size % frame_size) ) {
        fclose(yuv_fp);
        printf("The YUV file's size is not correct: file_size=%zd, frame_size=%d\n", file_size, frame_size);
        return -1;
    }
    
    fseeko(yuv_fp, (off_t)0, SEEK_SET);

    jpeg_fp = fopen(argv[4], "wb");  
    if ( jpeg_fp == NULL) {
        fclose(yuv_fp);
        printf("Can't open output destination jpeg file\n");
        return -1;
    }   
        
    start_time = clock();
    encode_input_image(yuv_fp, jpeg_fp, picture_width, picture_height, frame_size, yuv_type, quality);
    if(yuv_fp != NULL) fclose(yuv_fp);
    if(jpeg_fp != NULL) fclose(jpeg_fp);
    finish_time = clock();
    duration = finish_time - start_time;
    printf("Encoding finished in %u ticks\n", duration);
    
    return 0;   
}

