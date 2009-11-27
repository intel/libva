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

#define TEST_DESCRIPTION	"Query entrypoints for all profiles"

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
    int i, j;
    max_entrypoints = vaMaxNumEntrypoints(va_dpy);
    status("vaMaxEntryPoints = %d\n", max_entrypoints);
    ASSERT(max_entrypoints > 0);
    
    VAEntrypoint *entrypoints = malloc(max_entrypoints * sizeof(VAEntrypoint));
    ASSERT(entrypoints);
    
    for(i = 0; i < num_profiles; i++)
    {
        memset(entrypoints, 0xff, max_entrypoints * sizeof(VAEntrypoint));
        va_status = vaQueryConfigEntrypoints(va_dpy, profiles[i], entrypoints, &num_entrypoints);
        ASSERT( VA_STATUS_SUCCESS == va_status );
        status("vaQueryConfigEntryPoints(%s) reports %d entrypoints\n", profile2string(profiles[i]), num_entrypoints);
        ASSERT(num_entrypoints <= max_entrypoints);
        ASSERT(num_entrypoints > 0);
        for(j = 0; j < num_entrypoints; j++)
        {
            status("  entrypoint %d [%s]\n", entrypoints[j], entrypoint2string(entrypoints[j]));
        }
    }
    
    free(entrypoints);
}

void post()
{
    test_terminate();
}
