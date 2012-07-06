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

#define TEST_DESCRIPTION	"Create and destory surfaces of different sizes"

#include "test_common.c"

void pre()
{
    test_init();
}

#define DEAD_SURFACE_ID 	(VASurfaceID) 0xbeefdead

void test_unique_surfaces(VASurfaceID *surface_list, int surface_count)
{
    int i,j;
    
    for(i = 0; i < surface_count; i++)
    {
        ASSERT(surface_list[i] != VA_INVALID_SURFACE);
        for(j = 0; j < i; j++)
        {
            if (i == j) continue;
            ASSERT(surface_list[i] != surface_list[j]);
        }
    }
}

typedef struct test_size { int w; int h; } test_size_t;

test_size_t test_sizes[] = { 
  {  10, 10 }, 
  {  128, 128 }, 
  {  176, 144 }, 
  {  144, 176 }, 
  {  352, 288 }, 
  {  399, 299 }, 
  {  640, 480 }, 
  {  1280, 720 }
};

#define NUM_SIZES 	(sizeof(test_sizes) / sizeof(test_size_t))

void test()
{
    VASurfaceID surfaces[NUM_SIZES+1];
    unsigned int i;
    
    memset(surfaces, 0xff, sizeof(surfaces));

    for(i = 0; i < NUM_SIZES; i++)
    {
        status("vaCreateSurfaces create %dx%d surface\n", test_sizes[i].w, test_sizes[i].h);
        surfaces[i+1] = DEAD_SURFACE_ID;
        va_status = vaCreateSurfaces(va_dpy,  VA_RT_FORMAT_YUV420, test_sizes[i].w, test_sizes[i].h, &surfaces[i], 1, NULL, 0);
        ASSERT( VA_STATUS_SUCCESS == va_status );
        ASSERT( DEAD_SURFACE_ID == surfaces[i+1] );
    }
    
    test_unique_surfaces(surfaces, NUM_SIZES);

    status("vaDestroySurface all surfaces\n");
    va_status = vaDestroySurfaces(va_dpy, surfaces, NUM_SIZES);
    ASSERT( VA_STATUS_SUCCESS == va_status );
}

void post()
{
    test_terminate();
}
