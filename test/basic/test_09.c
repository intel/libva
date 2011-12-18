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

#define TEST_DESCRIPTION	"Create/destroy contexts for all profiles / entrypoints"

#include "test_common.c"

void pre()
{
    test_init();
    test_profiles();
}

void test()
{
    int max_entrypoints;
    int num_entrypoints;
    int i, j, k;
    int config_count = 0;
    max_entrypoints = vaMaxNumEntrypoints(va_dpy);
    ASSERT(max_entrypoints > 0);
    VAEntrypoint *entrypoints = malloc(max_entrypoints * sizeof(VAEntrypoint));
    ASSERT(entrypoints);

    VAConfigID *configs = malloc(max_entrypoints * num_profiles * sizeof(VAConfigID));
    VAContextID *contexts = malloc(max_entrypoints * num_profiles * sizeof(VAContextID));

    for(i = 0; i < num_profiles; i++)
    {
        va_status = vaQueryConfigEntrypoints(va_dpy, profiles[i], entrypoints, &num_entrypoints);
        ASSERT( VA_STATUS_SUCCESS == va_status );
        
        for(j = 0; j < num_entrypoints; j++)
        {
            status("vaCreateConfig for %s, %s\n",  profile2string(profiles[i]), entrypoint2string(entrypoints[j]));
            va_status = vaCreateConfig(va_dpy, profiles[i], entrypoints[j], NULL, 0, &(configs[config_count]));
            ASSERT( VA_STATUS_SUCCESS == va_status );
            status("vaCreateConfig returns %08x\n", configs[config_count]);
            config_count++;
        }
    }

    int width = 352;
    int height = 288;
    int surface_count = 4;
    int total_surfaces = config_count * surface_count;
    
    VASurfaceID *surfaces = malloc(total_surfaces * sizeof(VASurfaceID));

    // TODO: Don't assume VA_RT_FORMAT_YUV420 is supported / needed for each config
    va_status = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_YUV420, width, height, surfaces, total_surfaces, NULL, 0);
    ASSERT( VA_STATUS_SUCCESS == va_status );
    
    for(i = 0; i < config_count; i++)
    {
        status("vaCreateContext with config %08x\n", configs[i]);
        int flags = 0;
        va_status = vaCreateContext( va_dpy, configs[i], width, height, flags, surfaces + i*surface_count, surface_count, &contexts[i] );
        ASSERT( VA_STATUS_SUCCESS == va_status );
    }

    for(i = 0; i < config_count; i++)
    {
        status("vaDestroyContext for context %08x\n", contexts[i]);
        va_status = vaDestroyContext( va_dpy, contexts[i] );
        ASSERT( VA_STATUS_SUCCESS == va_status );
    }

    for(i = 0; i < config_count; i++)
    {
        status("vaDestroyConfig for config %08x\n", configs[i]);
        va_status = vaDestroyConfig( va_dpy, configs[i] );
        ASSERT( VA_STATUS_SUCCESS == va_status );
    }
    
    va_status = vaDestroySurfaces(va_dpy, surfaces, total_surfaces);
    ASSERT( VA_STATUS_SUCCESS == va_status );
    
    free(contexts);
    free(configs);
    free(surfaces);
    free(entrypoints);
}

void post()
{
    test_terminate();
}
