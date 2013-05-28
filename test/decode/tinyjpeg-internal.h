/*
 * Small jpeg decoder library (Internal header)
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


#ifndef __TINYJPEG_INTERNAL_H_
#define __TINYJPEG_INTERNAL_H_

#include <setjmp.h>

#define SANITY_CHECK 1

struct jdec_private;

#define HUFFMAN_BITS_SIZE  256

#define HUFFMAN_TABLES	   4
#define COMPONENTS	   4
#define JPEG_MAX_WIDTH	   2048
#define JPEG_MAX_HEIGHT	   2048
#define JPEG_SCAN_MAX	   4

enum std_markers {
   DQT  = 0xDB, /* Define Quantization Table */
   SOF  = 0xC0, /* Start of Frame (size information) */
   DHT  = 0xC4, /* Huffman Table */
   SOI  = 0xD8, /* Start of Image */
   SOS  = 0xDA, /* Start of Scan */
   RST  = 0xD0, /* Reset Marker d0 -> .. */
   RST7 = 0xD7, /* Reset Marker .. -> d7 */
   EOI  = 0xD9, /* End of Image */
   DRI  = 0xDD, /* Define Restart Interval */
   APP0 = 0xE0,
};


struct huffman_table
{
  /*bits and values*/
	unsigned char bits[16];
	unsigned char values[256];
};

struct component 
{
  unsigned int Hfactor;
  unsigned int Vfactor;
  unsigned char quant_table_index;
  unsigned int cid;
};


typedef void (*decode_MCU_fct) (struct jdec_private *priv);
typedef void (*convert_colorspace_fct) (struct jdec_private *priv);

struct jpeg_sos
{
  unsigned int nr_components;
  struct {
    unsigned int component_id;
    unsigned int dc_selector;
    unsigned int ac_selector;
  }components[4];
};

struct jdec_private
{
  /* Public variables */
  unsigned int width[JPEG_SCAN_MAX], height[JPEG_SCAN_MAX];	/* Size of the image */

  /* Private variables */
  const unsigned char *stream_begin, *stream_end,*stream_scan;
  unsigned int stream_length;

  const unsigned char *stream;	/* Pointer to the current stream */

  struct component component_infos[COMPONENTS];
  unsigned int nf_components;
  unsigned char Q_tables[COMPONENTS][64];		/* quantization tables, zigzag*/
  unsigned char Q_tables_valid[COMPONENTS];
  struct huffman_table HTDC[HUFFMAN_TABLES];	/* DC huffman tables   */
  unsigned char HTDC_valid[HUFFMAN_TABLES];
  struct huffman_table HTAC[HUFFMAN_TABLES];	/* AC huffman tables   */
  unsigned char HTAC_valid[HUFFMAN_TABLES];
  struct jpeg_sos cur_sos;  /* current sos values*/
  int default_huffman_table_initialized;
  int restart_interval;
};

#endif

