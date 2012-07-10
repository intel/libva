/*
 * Small jpeg decoder library - testing application
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

#include "tinyjpeg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "va_display.h"

static void exitmessage(const char *message) __attribute__((noreturn));
static void exitmessage(const char *message)
{
  printf("%s\n", message);
  exit(0);
}

static int filesize(FILE *fp)
{
  long pos;
  fseek(fp, 0, SEEK_END);
  pos = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  return pos;
}

/**
 * Load one jpeg image, and decompress it, and save the result.
 */
int convert_one_image(const char *infilename)
{
  FILE *fp;
  unsigned int length_of_file;
  unsigned int width, height;
  unsigned char *buf;
  struct jdec_private *jdec;

  /* Load the Jpeg into memory */
  fp = fopen(infilename, "rb");
  if (fp == NULL)
    exitmessage("Cannot open filename\n");
  length_of_file = filesize(fp);
  buf = (unsigned char *)malloc(length_of_file + 4);
  if (buf == NULL)
    exitmessage("Not enough memory for loading file\n");
  fread(buf, length_of_file, 1, fp);
  fclose(fp);

  /* Decompress it */
  jdec = tinyjpeg_init();
  if (jdec == NULL)
    exitmessage("Not enough memory to alloc the structure need for decompressing\n");

  if (tinyjpeg_parse_header(jdec, buf, length_of_file)<0)
    exitmessage(tinyjpeg_get_errorstring(jdec));

  /* Get the size of the image */
  tinyjpeg_get_size(jdec, &width, &height);

  printf("Decoding JPEG image %dx%d...\n", width, height);
  if (tinyjpeg_decode(jdec) < 0)
    exitmessage(tinyjpeg_get_errorstring(jdec));

  tinyjpeg_free(jdec);

  free(buf);
  return 0;
}

static void usage(void)
{
    fprintf(stderr, "Usage: loadjpeg <input_filename.jpeg> \n");
    exit(1);
}

/**
 * main
 *
 */
int main(int argc, char *argv[])
{
  char *input_filename;
  clock_t start_time, finish_time;
  unsigned int duration;
  int current_argument;

  va_init_display_args(&argc, argv);

  if (argc < 2)
    usage();

  current_argument = 1;
  input_filename = argv[current_argument];

  start_time = clock();
  convert_one_image(input_filename);
  finish_time = clock();
  duration = finish_time - start_time;
  printf("Decoding finished in %u ticks\n", duration);

  return 0;
}




