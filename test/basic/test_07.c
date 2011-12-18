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

#define TEST_DESCRIPTION	"Create and destory surfaces"

#include "test_common.c"

void pre()
{
    test_init();
}

#define DEAD_SURFACE_ID 	(VASurfaceID) 0xbeefdead

void test_unique_surfaces(VASurfaceID *surface_list1, int surface_count1, VASurfaceID *surface_list2, int surface_count2)
{
    int i,j;
    
    for(i = 0; i < surface_count1; i++)
    {
        for(j = 0; j < surface_count2; j++)
        {
            if ((surface_list1 == surface_list2) && (i == j)) continue;
            ASSERT(surface_list1[i] != VA_INVALID_SURFACE);
            ASSERT(surface_list2[j] != VA_INVALID_SURFACE);
            ASSERT(surface_list1[i] != surface_list2[j]);
        }
    }
}


void test()
{
    VASurfaceID surfaces_1[1+1];
    VASurfaceID surfaces_4[4+1];
    VASurfaceID surfaces_16[16+1];
    VASurfaceID surfaces_6[6+1];
    
    memset(surfaces_1, 0xff, sizeof(surfaces_1));
    memset(surfaces_4, 0xff, sizeof(surfaces_4));
    memset(surfaces_16, 0xff, sizeof(surfaces_16));
    memset(surfaces_6, 0xff, sizeof(surfaces_6));

    status("vaCreateSurfaces 1 surface\n");
    surfaces_1[1] = DEAD_SURFACE_ID;
    va_status = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_YUV420, 352, 288, surfaces_1, 1, NULL, 0);
    ASSERT( VA_STATUS_SUCCESS == va_status );
    ASSERT( DEAD_SURFACE_ID == surfaces_1[1] ); /* bounds check */

    status("vaCreateSurfaces 4 surfaces\n");
    surfaces_4[4] = DEAD_SURFACE_ID;
    va_status = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_YUV420, 352, 288, surfaces_4, 4, NULL, 0);
    ASSERT( VA_STATUS_SUCCESS == va_status );
    ASSERT( DEAD_SURFACE_ID == surfaces_4[4] ); /* bounds check */

    status("vaCreateSurfaces 16 surfaces\n");
    surfaces_16[16] = DEAD_SURFACE_ID;
    va_status = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_YUV420, 352, 288, surfaces_16, 16, NULL, 0);
    ASSERT( VA_STATUS_SUCCESS == va_status );
    ASSERT( DEAD_SURFACE_ID == surfaces_16[16] ); /* bounds check */
    
    test_unique_surfaces(surfaces_1, 1, surfaces_4, 4);
    test_unique_surfaces(surfaces_4, 4, surfaces_16, 4);
    test_unique_surfaces(surfaces_4, 4, surfaces_16, 16);
    test_unique_surfaces(surfaces_4, 1, surfaces_16, 16);
    test_unique_surfaces(surfaces_1, 16, surfaces_16, 16);

    status("vaDestroySurface 4 surfaces\n");
    va_status = vaDestroySurfaces(va_dpy, surfaces_4, 4);
    ASSERT( VA_STATUS_SUCCESS == va_status );
    
    status("vaCreateSurfaces 6 surfaces\n");
    surfaces_6[6] = DEAD_SURFACE_ID;
    va_status = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_YUV420, 352, 288, surfaces_6, 6, NULL, 0);
    ASSERT( VA_STATUS_SUCCESS == va_status );
    ASSERT( DEAD_SURFACE_ID == surfaces_6[6] ); /* bounds check */

    test_unique_surfaces(surfaces_1, 1, surfaces_6, 6);
    test_unique_surfaces(surfaces_6, 6, surfaces_16, 16);
    test_unique_surfaces(surfaces_1, 6, surfaces_16, 6);

    status("vaDestroySurface 16 surfaces\n");
    va_status = vaDestroySurfaces(va_dpy, surfaces_16, 16);
    ASSERT( VA_STATUS_SUCCESS == va_status );
    
    status("vaDestroySurface 1 surface\n");
    va_status = vaDestroySurfaces(va_dpy, surfaces_1, 1);
    ASSERT( VA_STATUS_SUCCESS == va_status );

    status("vaDestroySurface 6 surfaces\n");
    va_status = vaDestroySurfaces(va_dpy, surfaces_6, 6);
    ASSERT( VA_STATUS_SUCCESS == va_status );
}

void post()
{
    test_terminate();
}
