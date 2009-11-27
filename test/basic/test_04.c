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

#define TEST_DESCRIPTION	"Get config attributes for all profiles / entrypoints"

#include "test_common.c"

void pre()
{
    test_init();
    test_profiles();
}

#define DEADVALUE 0xdeaddead

void test()
{
    VAConfigAttrib attributes[] = { 
      { type: VAConfigAttribRTFormat, value: DEADVALUE },
      { type: VAConfigAttribSpatialResidual, value: DEADVALUE },
      { type: VAConfigAttribSpatialClipping, value: DEADVALUE },
      { type: VAConfigAttribIntraResidual, value: DEADVALUE },
      { type: VAConfigAttribEncryption, value: DEADVALUE }
    };
    int max_entrypoints;
    int num_entrypoints;
    int num_attribs = sizeof(attributes) / sizeof(VAConfigAttrib);
    int i, j, k;
    max_entrypoints = vaMaxNumEntrypoints(va_dpy);
    ASSERT(max_entrypoints > 0);
    VAEntrypoint *entrypoints = malloc(max_entrypoints * sizeof(VAEntrypoint));
    ASSERT(entrypoints);

    VAConfigAttrib *attrib_list = (VAConfigAttrib *) malloc(sizeof(attributes));
    ASSERT(attrib_list);

    for(i = 0; i < num_profiles; i++)
    {
        va_status = vaQueryConfigEntrypoints(va_dpy, profiles[i], entrypoints, &num_entrypoints);
        ASSERT( VA_STATUS_SUCCESS == va_status );
        
        for(j = 0; j < num_entrypoints; j++)
        {
            memcpy(attrib_list, attributes, sizeof(attributes));
            status("vaGetConfigAttributes for %s, %s\n",  profile2string(profiles[i]), entrypoint2string(entrypoints[j]));
            va_status = vaGetConfigAttributes(va_dpy, profiles[i], entrypoints[j], attrib_list, num_attribs);
            ASSERT( VA_STATUS_SUCCESS == va_status );
            for(k = 0; k < num_attribs; k++)
            {
                status("  %d -> %08x\n", attrib_list[k].type, attrib_list[k].value);
                ASSERT(attrib_list[k].value != DEADVALUE);
            }
        }
    }
    
    free(attrib_list);
    free(entrypoints);
}

void post()
{
    test_terminate();
}
