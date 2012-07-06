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

#define TEST_DESCRIPTION	"Create and destroy buffers"

#include "test_common.c"

VAConfigID config;
VAContextID context;
VASurfaceID *surfaces;
int total_surfaces;

void pre()
{
    test_init();

    va_status = vaCreateConfig(va_dpy, VAProfileMPEG2Main, VAEntrypointVLD, NULL, 0, &config);
    ASSERT( VA_STATUS_SUCCESS == va_status );
    status("vaCreateConfig returns %08x\n", config);

    int width = 352;
    int height = 288;
    int surface_count = 4;
    total_surfaces = surface_count;
    
    surfaces = malloc(total_surfaces * sizeof(VASurfaceID));

    // TODO: Don't assume VA_RT_FORMAT_YUV420 is supported / needed for each config
    va_status = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_YUV420, width, height, surfaces, total_surfaces, NULL, 0);
    ASSERT( VA_STATUS_SUCCESS == va_status );
    
    status("vaCreateContext with config %08x\n", config);
    int flags = 0;
    va_status = vaCreateContext( va_dpy, config, width, height, flags, surfaces, surface_count, &context );
    ASSERT( VA_STATUS_SUCCESS == va_status );
}

void test_unique_buffers(VABufferID *buffer_list, int buffer_count)
{
    int i,j;
    
    for(i = 0; i < buffer_count; i++)
    {
        for(j = 0; j < i; j++)
        {
            ASSERT(buffer_list[i] != buffer_list[j]);
        }
    }
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
};


#define NUM_BUFFER_TYPES 	(sizeof(buffer_types) / sizeof(VABufferType))

#define DEAD_BUFFER_ID	((VABufferID) 0x1234ffff)

void test()
{
    VABufferID buffer_ids[NUM_BUFFER_TYPES+1];
    unsigned int i;
    memset(buffer_ids, 0xff, sizeof(buffer_ids));
    for(i=0; i < NUM_BUFFER_TYPES; i++)
    {
        buffer_ids[i+1] = DEAD_BUFFER_ID;
        va_status = vaCreateBuffer(va_dpy, context, buffer_types[i], buffer_sizes[i], 1, NULL, &buffer_ids[i]);
        ASSERT( VA_STATUS_SUCCESS == va_status );
        ASSERT( DEAD_BUFFER_ID == buffer_ids[i+1] ); /* Bounds check */
    }
    test_unique_buffers(buffer_ids, NUM_BUFFER_TYPES);
    
    for(i=0; i < NUM_BUFFER_TYPES; i++)
    {
        va_status = vaDestroyBuffer(va_dpy, buffer_ids[i]);
        ASSERT( VA_STATUS_SUCCESS == va_status );
    }
}

void post()
{
    status("vaDestroyContext for context %08x\n", context);
    va_status = vaDestroyContext( va_dpy, context );
    ASSERT( VA_STATUS_SUCCESS == va_status );

    status("vaDestroyConfig for config %08x\n", config);
    va_status = vaDestroyConfig( va_dpy, config );
    ASSERT( VA_STATUS_SUCCESS == va_status );
    
    va_status = vaDestroySurfaces(va_dpy, surfaces, total_surfaces);
    ASSERT( VA_STATUS_SUCCESS == va_status );
    
    free(surfaces);

    test_terminate();
}
