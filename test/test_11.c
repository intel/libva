/*
 * Copyright (c) 2007 Intel Corporation. All Rights Reserved.
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

#define TEST_DESCRIPTION	"Map and unmap buffers"

#include "test_common.c"

void pre()
{
    test_init();
}

VABufferType buffer_types[] =
{
  VAPictureParameterBufferType,
  VAIQMatrixBufferType,
  VABitPlaneBufferType,
  VASliceGroupMapBufferType,
  VASliceParameterBufferType,
  VASliceDataBufferType,
  VAMacroblockParameterBufferType,
  VAResidualDataBufferType,
  VADeblockingParameterBufferType,
  VAImageBufferType
};

unsigned int buffer_sizes[] =
{
  sizeof(VAPictureParameterBufferMPEG4), 
  sizeof(VAIQMatrixBufferH264),
  32*1024,
  48*1024,
  sizeof(VASliceParameterBufferMPEG2),
  128*1024,
  sizeof(VAMacroblockParameterBufferMPEG2),
  32*1024,
  15*1024,
  32*1024,
};

#define NUM_BUFFER_TYPES 	(sizeof(buffer_types) / sizeof(VABufferType))

void test()
{
    VABufferID buffer_ids[NUM_BUFFER_TYPES+1];
    uint32_t *input_data[NUM_BUFFER_TYPES];
    int i, j;
    memset(buffer_ids, 0xff, sizeof(buffer_ids));
    for(i=0; i < NUM_BUFFER_TYPES; i++)
    {
        uint32_t *data;
        va_status = vaCreateBuffer(va_dpy, buffer_types[i], &buffer_ids[i]);
        ASSERT( VA_STATUS_SUCCESS == va_status );
        status("vaCreateBuffer created buffer %08x of type %d\n", buffer_ids[i], buffer_types[i]);

        input_data[i] = malloc(buffer_sizes[i]+4);
        ASSERT(input_data[i]);
        
        /* Generate input data */
        for(j = buffer_sizes[i] / 4; j--;)
        {
            input_data[i][j] = random();
        }
        
        /* Copy to secondary buffer */
        data = malloc(buffer_sizes[i]);
        ASSERT(data);
        memcpy(data, input_data[i], buffer_sizes[i]);

        /* Send to VA Buffer */
        va_status = vaBufferData(va_dpy, buffer_ids[i], buffer_sizes[i], 1, data);
        ASSERT( VA_STATUS_SUCCESS == va_status );
        
        /* Wipe secondary buffer */
        memset(data, 0, buffer_sizes[i]);
        free(data);
    }

    for(i=0; i < NUM_BUFFER_TYPES; i++)
    {
        void *data = NULL;
        /* Fetch VA Buffer */
        va_status = vaBufferData(va_dpy, buffer_ids[i], buffer_sizes[i], 1, NULL);
        ASSERT( VA_STATUS_SUCCESS == va_status );
        
        va_status = vaMapBuffer(va_dpy, buffer_ids[i], &data);
        ASSERT( VA_STATUS_SUCCESS == va_status );
        status("vaMapBuffer mapped buffer %08x\n", buffer_ids[i]);

        /* Compare data */        
        ASSERT( memcmp(input_data[i], data, buffer_sizes[i]) == 0 );
    }
    
    for(i=0; i < NUM_BUFFER_TYPES; i++)
    {
        va_status = vaUnmapBuffer(va_dpy, buffer_ids[i]);
        ASSERT( VA_STATUS_SUCCESS == va_status );

        va_status = vaDestroyBuffer(va_dpy, buffer_ids[i]);
        ASSERT( VA_STATUS_SUCCESS == va_status );
        
        free(input_data[i]);
    }
}

void post()
{
    test_terminate();
}
