/*
 * Small jpeg decoder library
 *
 * Copyright (c) 2006, Luc Saillard <luc@saillard.org>
 * Copyright (c) 2012 Intel Corporation.
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * - Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * - Neither the name of the author nor the names of its contributors may be
 *  used to endorse or promote products derived from this software without
 *  specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "tinyjpeg.h"
#include "tinyjpeg-internal.h"

// for libva
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <va/va.h>
#include <va/va_dec_jpeg.h>
#include "va_display.h"


#define cY	0
#define cCb	1
#define cCr	2

#define BLACK_Y 0
#define BLACK_U 127
#define BLACK_V 127

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#define ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

#if DEBUG
#define trace(fmt, args...) do { \
   fprintf(stderr, fmt, ## args); \
   fflush(stderr); \
} while(0)
#else
#define trace(fmt, args...) do { } while (0)
#endif
#define error(fmt, args...) do { \
   snprintf(error_string, sizeof(error_string), fmt, ## args); \
   return -1; \
} while(0)
/* The variables for different image scans */
static int scan_num=0;
static int next_image_found=0;
/* Global variable to return the last error found while deconding */
static char error_string[256];
static VAHuffmanTableBufferJPEGBaseline default_huffman_table_param={
    huffman_table:
    {
        // lumiance component
        {
            num_dc_codes:{0,1,5,1,1,1,1,1,1,0,0,0}, // 12 bits is ok for baseline profile
            dc_values:{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b},
            num_ac_codes:{0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,125},
            ac_values:{
              0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
              0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
              0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
              0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
              0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
              0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
              0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
              0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
              0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
              0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
              0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
              0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
              0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
              0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
              0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
              0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
              0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
              0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
              0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
              0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
              0xf9, 0xfa
           },/*,0xonly,0xthe,0xfirst,0x162,0xbytes,0xare,0xavailable,0x*/
        },
        // chrom component
        {
            num_dc_codes:{0,3,1,1,1,1,1,1,1,1,1,0}, // 12 bits is ok for baseline profile
            dc_values:{0,1,2,3,4,5,6,7,8,9,0xa,0xb},
            num_ac_codes:{0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,119},
            ac_values:{
              0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
              0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
              0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
              0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
              0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
              0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
              0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
              0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
              0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
              0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
              0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
              0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
              0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
              0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
              0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
              0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
              0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
              0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
              0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
              0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
              0xf9, 0xfa
            },/*,0xonly,0xthe,0xfirst,0x162,0xbytes,0xare,0xavailable,0x*/
        },
    }
};

#define be16_to_cpu(x) (((x)[0]<<8)|(x)[1])


static int build_default_huffman_tables(struct jdec_private *priv)
{
    int i = 0;
	if (priv->default_huffman_table_initialized)
		return 0;

    for (i = 0; i < 4; i++) {
        priv->HTDC_valid[i] = 1;
        memcpy(priv->HTDC[i].bits, default_huffman_table_param.huffman_table[i].num_dc_codes, 16);
        memcpy(priv->HTDC[i].values, default_huffman_table_param.huffman_table[i].dc_values, 16);
        priv->HTAC_valid[i] = 1;
        memcpy(priv->HTAC[i].bits, default_huffman_table_param.huffman_table[i].num_ac_codes, 16);
        memcpy(priv->HTAC[i].values, default_huffman_table_param.huffman_table[i].ac_values, 256);
    }
	priv->default_huffman_table_initialized = 1;
	return 0;
}


static void print_SOF(const unsigned char *stream)
{
  int width, height, nr_components, precision;
#if DEBUG
  const char *nr_components_to_string[] = {
     "????",
     "Grayscale",
     "????",
     "YCbCr",
     "CYMK"
  };
#endif

  precision = stream[2];
  height = be16_to_cpu(stream+3);
  width  = be16_to_cpu(stream+5);
  nr_components = stream[7];

  trace("> SOF marker\n");
  trace("Size:%dx%d nr_components:%d (%s)  precision:%d\n", 
      width, height,
      nr_components, nr_components_to_string[nr_components],
      precision);
}

static int parse_DQT(struct jdec_private *priv, const unsigned char *stream)
{
  int qi;
  const unsigned char *dqt_block_end;

  trace("> DQT marker\n");
  dqt_block_end = stream + be16_to_cpu(stream);
  stream += 2;	/* Skip length */

  while (stream < dqt_block_end)
   {
     qi = *stream++;
#if SANITY_CHECK
     if (qi>>4)
       error("16 bits quantization table is not supported\n");
     if (qi>4)
       error("No more 4 quantization table is supported (got %d)\n", qi);
#endif
     memcpy(priv->Q_tables[qi&0x0F], stream, 64);
     priv->Q_tables_valid[qi & 0x0f] = 1;
     stream += 64;
   }
  trace("< DQT marker\n");
  return 0;
}

static int parse_SOF(struct jdec_private *priv, const unsigned char *stream)
{
  int i, width, height, nr_components, cid, sampling_factor;
  unsigned char Q_table;
  struct component *c;

  trace("> SOF marker\n");
  print_SOF(stream);

  height = be16_to_cpu(stream+3);
  width  = be16_to_cpu(stream+5);
  nr_components = stream[7];
  priv->nf_components = nr_components;
#if SANITY_CHECK
  if (stream[2] != 8)
    error("Precision other than 8 is not supported\n");
  if (width>JPEG_MAX_WIDTH || height>JPEG_MAX_HEIGHT)
    printf("WARNING:Width and Height (%dx%d) seems suspicious\n", width, height);
  if (nr_components != 3)
    printf("ERROR:We only support YUV images\n");
  if (height%16)
    printf("WARNING:Height need to be a multiple of 16 (current height is %d)\n", height);
  if (width%16)
    printf("WARNING:Width need to be a multiple of 16 (current Width is %d)\n", width);
#endif
  stream += 8;
  for (i=0; i<nr_components; i++) {
     cid = *stream++;
     sampling_factor = *stream++;
     Q_table = *stream++;
     c = &priv->component_infos[i];
     c->cid = cid;
     if (Q_table >= COMPONENTS)
       error("Bad Quantization table index (got %d, max allowed %d)\n", Q_table, COMPONENTS-1);
     c->Vfactor = sampling_factor&0xf;
     c->Hfactor = sampling_factor>>4;
     c->quant_table_index = Q_table;
     trace("Component:%d  factor:%dx%d  Quantization table:%d\n",
           cid, c->Hfactor, c->Vfactor, Q_table );

  }
  priv->width[scan_num] = width;
  priv->height[scan_num] = height;

  trace("< SOF marker\n");

  return 0;
}

static int parse_SOS(struct jdec_private *priv, const unsigned char *stream)
{
  unsigned int i, cid, table;
  unsigned int nr_components = stream[2];

  trace("> SOS marker\n");

  priv->cur_sos.nr_components= nr_components;

  stream += 3;
  for (i=0;i<nr_components;i++) {
     cid = *stream++;
     table = *stream++;
     priv->cur_sos.components[i].component_id = cid;
     priv->cur_sos.components[i].dc_selector = ((table>>4)&0x0F);
     priv->cur_sos.components[i].ac_selector = (table&0x0F);
#if SANITY_CHECK
     if ((table&0xf)>=4)
	error("We do not support more than 2 AC Huffman table\n");
     if ((table>>4)>=4)
	error("We do not support more than 2 DC Huffman table\n");
     if (cid != priv->component_infos[i].cid)
        error("SOS cid order (%d:%d) isn't compatible with the SOF marker (%d:%d)\n",
	      i, cid, i, priv->component_infos[i].cid);
     trace("ComponentId:%d  tableAC:%d tableDC:%d\n", cid, table&0xf, table>>4);
#endif
  }
  priv->stream = stream+3;
  trace("< SOS marker\n");
  return 0;
}

int tinyjpeg_parse_SOS(struct jdec_private *priv, const unsigned char *stream)
{
    return parse_SOS(priv, stream);
}


static int parse_DHT(struct jdec_private *priv, const unsigned char *stream)
{
  unsigned int count, i;
  int length, index;
  unsigned char Tc, Th;

  length = be16_to_cpu(stream) - 2;
  stream += 2;	/* Skip length */

  trace("> DHT marker (length=%d)\n", length);

  while (length>0) {
     index = *stream++;

     Tc = index & 0xf0; // it is not important to <<4
     Th = index & 0x0f;
     if (Tc) {
        memcpy(priv->HTAC[index & 0xf].bits, stream, 16);
     }
     else {
         memcpy(priv->HTDC[index & 0xf].bits, stream, 16);
     }

     count = 0;
     for (i=0; i<16; i++) {
        count += *stream++;
     }

#if SANITY_CHECK
     if (count >= HUFFMAN_BITS_SIZE)
       error("No more than %d bytes is allowed to describe a huffman table", HUFFMAN_BITS_SIZE);
     if ( (index &0xf) >= HUFFMAN_TABLES)
       error("No more than %d Huffman tables is supported (got %d)\n", HUFFMAN_TABLES, index&0xf);
     trace("Huffman table %s[%d] length=%d\n", (index&0xf0)?"AC":"DC", index&0xf, count);
#endif

     if (Tc) {
        memcpy(priv->HTAC[index & 0xf].values, stream, count);
        priv->HTAC_valid[index & 0xf] = 1;
     }
     else {
        memcpy(priv->HTDC[index & 0xf].values, stream, count);
        priv->HTDC_valid[index & 0xf] = 1;
     }

     length -= 1;
     length -= 16;
     length -= count;
     stream += count;
  }
  trace("< DHT marker\n");
  return 0;
}
static int parse_DRI(struct jdec_private *priv, const unsigned char *stream)
{
  unsigned int length;

  trace("> DRI marker\n");

  length = be16_to_cpu(stream);

#if SANITY_CHECK
  if (length != 4)
    error("Length of DRI marker need to be 4\n");
#endif

  priv->restart_interval = be16_to_cpu(stream+2);

#if DEBUG
  trace("Restart interval = %d\n", priv->restart_interval);
#endif

  trace("< DRI marker\n");

  return 0;
}

static int findEOI(struct jdec_private *priv,const unsigned char *stream)
{
   while (!(*stream == 0xff  && *(stream+1) == 0xd9 )&& stream<=priv->stream_end) //searching for the end of image marker
   {
      stream++;
      continue;
   }
  priv->stream_scan=stream;
  return 0;
}

static int findSOI(struct jdec_private *priv,const unsigned char *stream)
{
   while (!(*stream == 0xff  && *(stream+1) == 0xd8 ) ) //searching for the start of image marker
   {
      if(stream<=priv->stream_end)
        {
           stream++;
           continue;
         }
      else
         return 0;  // No more images in the file.
   }
   priv->stream=stream+2;
   return 1;
}

static int parse_JFIF(struct jdec_private *priv, const unsigned char *stream)
{
  int chuck_len;
  int marker;
  int sos_marker_found = 0;
  int dht_marker_found = 0;
  int dqt_marker_found = 0;
  const unsigned char *next_chunck;

  next_image_found = findSOI(priv,stream);
  stream=priv->stream;

   while (!sos_marker_found  && stream<=priv->stream_end)
   {
     while((*stream == 0xff))
        stream++;    
            	    
     marker = *stream++;
     chuck_len = be16_to_cpu(stream);
     next_chunck = stream + chuck_len;
     switch (marker)
      {
       case SOF:
	 if (parse_SOF(priv, stream) < 0)
	   return -1;
	 break;
       case DQT:
	 if (parse_DQT(priv, stream) < 0)
	   return -1;
	 dqt_marker_found = 1;
	 break;
       case SOS:
	 if (parse_SOS(priv, stream) < 0)
	   return -1;
	 sos_marker_found = 1;
	 break;
       case DHT:
	 if (parse_DHT(priv, stream) < 0)
	   return -1;
	 dht_marker_found = 1;
	 break;
       case DRI:
	 if (parse_DRI(priv, stream) < 0)
	   return -1;
	 break;
       default:
	 trace("> Unknown marker %2.2x\n", marker);
	 break;
      }

     stream = next_chunck;
   }

   if(next_image_found){
      if (!dht_marker_found) {
        trace("No Huffman table loaded, using the default one\n");
        build_default_huffman_tables(priv);
      }
      if (!dqt_marker_found) {
        error("ERROR:No Quantization table loaded, using the default one\n");
      }
   } 
#ifdef SANITY_CHECK
  if (   (priv->component_infos[cY].Hfactor < priv->component_infos[cCb].Hfactor)
      || (priv->component_infos[cY].Hfactor < priv->component_infos[cCr].Hfactor))
    error("Horizontal sampling factor for Y should be greater than horitontal sampling factor for Cb or Cr\n");
  if (   (priv->component_infos[cY].Vfactor < priv->component_infos[cCb].Vfactor)
      || (priv->component_infos[cY].Vfactor < priv->component_infos[cCr].Vfactor))
    error("Vertical sampling factor for Y should be greater than vertical sampling factor for Cb or Cr\n");
  if (   (priv->component_infos[cCb].Hfactor!=1) 
      || (priv->component_infos[cCr].Hfactor!=1)
      || (priv->component_infos[cCb].Vfactor!=1)
      || (priv->component_infos[cCr].Vfactor!=1))
    printf("ERROR:Sampling other than 1x1 for Cr and Cb is not supported");
#endif
  findEOI(priv,stream);
  return next_image_found;
}

/*******************************************************************************
 *
 * Functions exported of the library.
 *
 * Note: Some applications can access directly to internal pointer of the
 * structure. It's is not recommended, but if you have many images to
 * uncompress with the same parameters, some functions can be called to speedup
 * the decoding.
 *
 ******************************************************************************/

/**
 * Allocate a new tinyjpeg decoder object.
 *
 * Before calling any other functions, an object need to be called.
 */
struct jdec_private *tinyjpeg_init(void)
{
  struct jdec_private *priv;
 
  priv = (struct jdec_private *)calloc(1, sizeof(struct jdec_private));
  if (priv == NULL)
    return NULL;
  return priv;
}

/**
 * Free a tinyjpeg object.
 *
 * No others function can be called after this one.
 */
void tinyjpeg_free(struct jdec_private *priv)
{
  free(priv);
}

/**
 * Initialize the tinyjpeg object and prepare the decoding of the stream.
 *
 * Check if the jpeg can be decoded with this jpeg decoder.
 * Fill some table used for preprocessing.
 */
int tinyjpeg_parse_header(struct jdec_private *priv, const unsigned char *buf, unsigned int size)
{
  int ret;

  /* Identify the file */
  if ((buf[0] != 0xFF) || (buf[1] != SOI))
    error("Not a JPG file ?\n");

  priv->stream_begin = buf;
  priv->stream_length = size;
  priv->stream_end = priv->stream_begin + priv->stream_length;

  priv->stream = priv->stream_begin;
  ret = parse_JFIF(priv, priv->stream);
  return ret;
}


int tinyjpeg_decode(struct jdec_private *priv)
{
#define CHECK_VASTATUS(va_status,func)                                  \
    if (va_status != VA_STATUS_SUCCESS) {                                   \
        fprintf(stderr,"%s:%s (%d) failed,exit\n", __func__, func, __LINE__); \
        exit(1);                                                            \
    }

    VAEntrypoint entrypoints[5];
    int num_entrypoints,vld_entrypoint;
    VAConfigAttrib attrib;
    VAConfigID config_id;
    VASurfaceID surface_id;
    VAContextID context_id;
    VABufferID pic_param_buf,iqmatrix_buf,huffmantable_buf,slice_param_buf,slice_data_buf;
    int major_ver, minor_ver;
    VADisplay	va_dpy;
    VAStatus va_status;
    int max_h_factor, max_v_factor;
    int putsurface=1;
    unsigned int i, j;

    int surface_type;
    char *type;
    int ChromaTypeIndex;

    VASurfaceAttrib forcc;
    forcc.type =VASurfaceAttribPixelFormat;
    forcc.flags=VA_SURFACE_ATTRIB_SETTABLE;
    forcc.value.type=VAGenericValueTypeInteger;
     

    va_dpy = va_open_display();
    va_status = vaInitialize(va_dpy, &major_ver, &minor_ver);
    assert(va_status == VA_STATUS_SUCCESS);
    
    va_status = vaQueryConfigEntrypoints(va_dpy, VAProfileJPEGBaseline, entrypoints, 
                             &num_entrypoints);
    CHECK_VASTATUS(va_status, "vaQueryConfigEntrypoints");

    for	(vld_entrypoint = 0; vld_entrypoint < num_entrypoints; vld_entrypoint++) {
        if (entrypoints[vld_entrypoint] == VAEntrypointVLD)
            break;
    }
    if (vld_entrypoint == num_entrypoints) {
        /* not find VLD entry point */
        assert(0);
    }

    /* Assuming finding VLD, find out the format for the render target */
    attrib.type = VAConfigAttribRTFormat;
    vaGetConfigAttributes(va_dpy, VAProfileJPEGBaseline, VAEntrypointVLD,
                          &attrib, 1);
    if ((attrib.value & VA_RT_FORMAT_YUV420) == 0) {
        /* not find desired YUV420 RT format */
        assert(0);
    }
    
    va_status = vaCreateConfig(va_dpy, VAProfileJPEGBaseline, VAEntrypointVLD,
                              &attrib, 1,&config_id);
    CHECK_VASTATUS(va_status, "vaQueryConfigEntrypoints");

    while (next_image_found){  
       VAPictureParameterBufferJPEGBaseline pic_param;
       memset(&pic_param, 0, sizeof(pic_param));
       pic_param.picture_width = priv->width[scan_num];
       pic_param.picture_height = priv->height[scan_num];
       pic_param.num_components = priv->nf_components;


       for (i=0; i<pic_param.num_components; i++) { // tinyjpeg support 3 components only, does it match va?
           pic_param.components[i].component_id = priv->component_infos[i].cid;
           pic_param.components[i].h_sampling_factor = priv->component_infos[i].Hfactor;
           pic_param.components[i].v_sampling_factor = priv->component_infos[i].Vfactor;
           pic_param.components[i].quantiser_table_selector = priv->component_infos[i].quant_table_index;
       }
       int h1, h2, h3, v1, v2, v3;
       h1 = pic_param.components[0].h_sampling_factor;
       h2 = pic_param.components[1].h_sampling_factor;
       h3 = pic_param.components[2].h_sampling_factor;
       v1 = pic_param.components[0].v_sampling_factor;
       v2 = pic_param.components[1].v_sampling_factor;
       v3 = pic_param.components[2].v_sampling_factor;

       if (h1 == 2 && h2 == 1 && h3 == 1 &&
               v1 == 2 && v2 == 1 && v3 == 1) {
           //surface_type = VA_RT_FORMAT_IMC3;
           surface_type = VA_RT_FORMAT_YUV420;
           forcc.value.value.i = VA_FOURCC_IMC3;
           ChromaTypeIndex = 1;
           type = "VA_FOURCC_IMC3";
       }
       else if (h1 == 2 && h2 == 1 && h3 == 1 &&
               v1 == 1 && v2 == 1 && v3 == 1) {
           //surface_type = VA_RT_FORMAT_YUV422H;
           surface_type = VA_RT_FORMAT_YUV422;
           forcc.value.value.i = VA_FOURCC_422H;
           ChromaTypeIndex = 2;
           type = "VA_FOURCC_422H";
       }
       else if (h1 == 1 && h2 == 1 && h3 == 1 &&
               v1 == 1 && v2 == 1 && v3 == 1) {
           surface_type = VA_RT_FORMAT_YUV444;
           forcc.value.value.i = VA_FOURCC_444P;
           //forcc.value.value.i = VA_FOURCC_RGBP;
           ChromaTypeIndex = 3;
           type = "VA_FOURCC_444P";
       }
       else if (h1 == 4 && h2 == 1 && h3 == 1 &&
               v1 == 1 && v2 == 1 && v3 == 1) {
           surface_type = VA_RT_FORMAT_YUV411;
           forcc.value.value.i = VA_FOURCC_411P;
           ChromaTypeIndex = 4;
           type = "VA_FOURCC_411P";
       }
       else if (h1 == 1 && h2 == 1 && h3 == 1 &&
               v1 == 2 && v2 == 1 && v3 == 1) {
           //surface_type = VA_RT_FORMAT_YUV422V;
           surface_type = VA_RT_FORMAT_YUV422;
           forcc.value.value.i = VA_FOURCC_422V;
           ChromaTypeIndex = 5;
           type = "VA_FOURCC_422V";
       }
       else if (h1 == 2 && h2 == 1 && h3 == 1 &&
               v1 == 2 && v2 == 2 && v3 == 2) {
           //surface_type = VA_RT_FORMAT_YUV422H;
           surface_type = VA_RT_FORMAT_YUV422;
           forcc.value.value.i = VA_FOURCC_422H;
           ChromaTypeIndex = 6;
           type = "VA_FOURCC_422H";
       }
       else if (h2 == 2 && h2 == 2 && h3 == 2 &&
               v1 == 2 && v2 == 1 && v3 == 1) {
           //surface_type = VA_RT_FORMAT_YUV422V;
           surface_type = VA_RT_FORMAT_YUV422;
           forcc.value.value.i = VA_FOURCC_422V;
           ChromaTypeIndex = 7;
           type = "VA_FOURCC_422V";
       }
       else
       {
           surface_type = VA_RT_FORMAT_YUV400;
           forcc.value.value.i = VA_FOURCC('Y','8','0','0');
           ChromaTypeIndex = 0;
           type = "Format_400P";
       }

       va_status = vaCreateSurfaces(va_dpy,surface_type,
                                    priv->width[scan_num],priv->height[scan_num], //alignment?
                                    &surface_id, 1, &forcc, 1);
       CHECK_VASTATUS(va_status, "vaCreateSurfaces");
  
       /* Create a context for this decode pipe */
       va_status = vaCreateContext(va_dpy, config_id,
                                  priv->width[scan_num], priv->height[scan_num], // alignment?
                                  VA_PROGRESSIVE,
                                  &surface_id,
                                  1,
                                  &context_id);
       CHECK_VASTATUS(va_status, "vaCreateContext");

       va_status = vaCreateBuffer(va_dpy, context_id,
                                 VAPictureParameterBufferType, // VAPictureParameterBufferJPEGBaseline?
                                 sizeof(VAPictureParameterBufferJPEGBaseline),
                                 1, &pic_param,
                                 &pic_param_buf);
       CHECK_VASTATUS(va_status, "vaCreateBuffer");

       VAIQMatrixBufferJPEGBaseline iq_matrix;
       const unsigned int num_quant_tables =
           MIN(COMPONENTS, ARRAY_ELEMS(iq_matrix.load_quantiser_table));
       // todo, only mask it if non-default quant matrix is used. do we need build default quant matrix?
       memset(&iq_matrix, 0, sizeof(VAIQMatrixBufferJPEGBaseline));
       for (i = 0; i < num_quant_tables; i++) {
           if (!priv->Q_tables_valid[i])
               continue;
           iq_matrix.load_quantiser_table[i] = 1;
           for (j = 0; j < 64; j++)
               iq_matrix.quantiser_table[i][j] = priv->Q_tables[i][j];
       }
       va_status = vaCreateBuffer(va_dpy, context_id,
                                 VAIQMatrixBufferType, // VAIQMatrixBufferJPEGBaseline?
                                 sizeof(VAIQMatrixBufferJPEGBaseline),
                                 1, &iq_matrix,
                                 &iqmatrix_buf );
       CHECK_VASTATUS(va_status, "vaCreateBuffer");

       VAHuffmanTableBufferJPEGBaseline huffman_table;
       const unsigned int num_huffman_tables =
           MIN(COMPONENTS, ARRAY_ELEMS(huffman_table.load_huffman_table));
       memset(&huffman_table, 0, sizeof(VAHuffmanTableBufferJPEGBaseline));
       assert(sizeof(huffman_table.huffman_table[0].num_dc_codes) ==
           sizeof(priv->HTDC[0].bits));
          assert(sizeof(huffman_table.huffman_table[0].dc_values[0]) ==
           sizeof(priv->HTDC[0].values[0]));
       for (i = 0; i < num_huffman_tables; i++) {
           if (!priv->HTDC_valid[i] || !priv->HTAC_valid[i])
               continue;
           huffman_table.load_huffman_table[i] = 1;
           memcpy(huffman_table.huffman_table[i].num_dc_codes, priv->HTDC[i].bits,
                  sizeof(huffman_table.huffman_table[i].num_dc_codes));
           memcpy(huffman_table.huffman_table[i].dc_values, priv->HTDC[i].values,
                  sizeof(huffman_table.huffman_table[i].dc_values));
           memcpy(huffman_table.huffman_table[i].num_ac_codes, priv->HTAC[i].bits,
                  sizeof(huffman_table.huffman_table[i].num_ac_codes));   
           memcpy(huffman_table.huffman_table[i].ac_values, priv->HTAC[i].values,
                  sizeof(huffman_table.huffman_table[i].ac_values));
           memset(huffman_table.huffman_table[i].pad, 0,
                  sizeof(huffman_table.huffman_table[i].pad));
       }
       va_status = vaCreateBuffer(va_dpy, context_id,
                                 VAHuffmanTableBufferType, // VAHuffmanTableBufferJPEGBaseline?
                                 sizeof(VAHuffmanTableBufferJPEGBaseline),
                                 1, &huffman_table,
                                 &huffmantable_buf );
       CHECK_VASTATUS(va_status, "vaCreateBuffer");
    
       // one slice for whole image?
       max_h_factor = priv->component_infos[0].Hfactor;
       max_v_factor = priv->component_infos[0].Vfactor;
       static VASliceParameterBufferJPEGBaseline slice_param;
       slice_param.slice_data_size = (priv->stream_scan - priv->stream);
       slice_param.slice_data_offset = 0;
       slice_param.slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
       slice_param.slice_horizontal_position = 0;    
       slice_param.slice_vertical_position = 0;    
       slice_param.num_components = priv->cur_sos.nr_components;
       for (i = 0; i < slice_param.num_components; i++) {
           slice_param.components[i].component_selector = priv->cur_sos.components[i].component_id; /* FIXME: set to values specified in SOS  */
           slice_param.components[i].dc_table_selector = priv->cur_sos.components[i].dc_selector;  /* FIXME: set to values specified in SOS  */
           slice_param.components[i].ac_table_selector = priv->cur_sos.components[i].ac_selector;  /* FIXME: set to values specified in SOS  */
       }
       slice_param.restart_interval = priv->restart_interval;
       slice_param.num_mcus = ((priv->width[scan_num]+max_h_factor*8-1)/(max_h_factor*8))*
                             ((priv->height[scan_num]+max_v_factor*8-1)/(max_v_factor*8)); // ?? 720/16? 
 
       va_status = vaCreateBuffer(va_dpy, context_id,
                                 VASliceParameterBufferType, // VASliceParameterBufferJPEGBaseline?
                                 sizeof(VASliceParameterBufferJPEGBaseline),
                                 1,
                                 &slice_param, &slice_param_buf);
       CHECK_VASTATUS(va_status, "vaCreateBuffer");

       va_status = vaCreateBuffer(va_dpy, context_id,
                                 VASliceDataBufferType,
                                 priv->stream_scan - priv->stream,
                                 1,
                                 (void*)priv->stream, // jpeg_clip,
                                 &slice_data_buf);
       CHECK_VASTATUS(va_status, "vaCreateBuffer");

       va_status = vaBeginPicture(va_dpy, context_id, surface_id);
       CHECK_VASTATUS(va_status, "vaBeginPicture");   

       va_status = vaRenderPicture(va_dpy,context_id, &pic_param_buf, 1);
       CHECK_VASTATUS(va_status, "vaRenderPicture");
   
       va_status = vaRenderPicture(va_dpy,context_id, &iqmatrix_buf, 1);
       CHECK_VASTATUS(va_status, "vaRenderPicture");

       va_status = vaRenderPicture(va_dpy,context_id, &huffmantable_buf, 1);
       CHECK_VASTATUS(va_status, "vaRenderPicture");
   
       va_status = vaRenderPicture(va_dpy,context_id, &slice_param_buf, 1);
       CHECK_VASTATUS(va_status, "vaRenderPicture");
    
       va_status = vaRenderPicture(va_dpy,context_id, &slice_data_buf, 1);
       CHECK_VASTATUS(va_status, "vaRenderPicture");
    
       va_status = vaEndPicture(va_dpy,context_id);
       CHECK_VASTATUS(va_status, "vaEndPicture");

       va_status = vaSyncSurface(va_dpy, surface_id);
       CHECK_VASTATUS(va_status, "vaSyncSurface");

       if (putsurface) {
           VARectangle src_rect, dst_rect;

           src_rect.x      = 0;
           src_rect.y      = 0;
           src_rect.width  = priv->width[scan_num];
           src_rect.height = priv->height[scan_num];
           dst_rect        = src_rect;

           va_status = va_put_surface(va_dpy, surface_id, &src_rect, &dst_rect);
           CHECK_VASTATUS(va_status, "vaPutSurface");
       }
       scan_num++;

       vaDestroySurfaces(va_dpy,&surface_id,1);
       vaDestroyConfig(va_dpy,config_id);
       vaDestroyContext(va_dpy,context_id);
    
       parse_JFIF(priv,priv->stream);
       if(priv->width[scan_num] == 0 && priv->height[scan_num] == 0)
          break;
    }
   // va_close_display(va_dpy);
    vaTerminate(va_dpy);
    printf("press any key to exit23\n");
    getchar();
    return 0;
}
const char *tinyjpeg_get_errorstring(struct jdec_private *priv)
{
  /* FIXME: the error string must be store in the context */
  priv = priv;
  return error_string;
}
void tinyjpeg_get_size(struct jdec_private *priv, unsigned int *width, unsigned int *height)
{
  *width = priv->width[scan_num];
  *height = priv->height[scan_num];
}


