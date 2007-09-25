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

#define TEST_DESCRIPTION	"Get config attributes from configs"

#include "test_common.c"

int max_entrypoints;
VAEntrypoint *entrypoints;

VAConfigID *configs;
int config_count = 0;



void pre()
{
    int i, j, k;

    test_init();
    test_profiles();

    max_entrypoints = vaMaxNumEntrypoints(va_dpy);
    ASSERT(max_entrypoints > 0);
    entrypoints = malloc(max_entrypoints * sizeof(VAEntrypoint));
    ASSERT(entrypoints);

    configs = malloc(max_entrypoints * num_profiles * sizeof(VAConfigID));
    ASSERT(configs);

    // Create configs
    for(i = 0; i < num_profiles; i++)
    {
        int num_entrypoints;
        va_status = vaQueryConfigEntrypoints(va_dpy, profiles[i], entrypoints, &num_entrypoints);
        ASSERT( VA_STATUS_SUCCESS == va_status );
        
        for(j = 0; j < num_entrypoints; j++)
        {
            va_status = vaCreateConfig(va_dpy, profiles[i], entrypoints[j], NULL, 0, &(configs[config_count]));
            ASSERT( VA_STATUS_SUCCESS == va_status );
            config_count++;
        }
    }
}

void test()
{
    int i, j, k;
    int max_attribs;

    max_attribs = vaMaxNumConfigAttributes(va_dpy);
    ASSERT(max_attribs > 0);

    VAConfigAttrib *attrib_list = malloc(max_attribs * sizeof(VAConfigAttrib));

    config_count = 0;
    for(i = 0; i < num_profiles; i++)
    {
        int num_entrypoints;

        va_status = vaQueryConfigEntrypoints(va_dpy, profiles[i], entrypoints, &num_entrypoints);
        ASSERT( VA_STATUS_SUCCESS == va_status );
        for(j = 0; j < num_entrypoints; j++)
        {
            VAProfile profile= -1;
            VAEntrypoint entrypoint = -1;
            int num_attribs = -1;
            
            status("Checking vaQueryConfigAttributes for %s, %s\n",  profile2string(profiles[i]), entrypoint2string(entrypoints[j]));
            memset(attrib_list, 0xff, max_attribs * sizeof(VAConfigAttrib));
            
            va_status = vaQueryConfigAttributes(va_dpy, configs[config_count], &profile, &entrypoint, attrib_list, &num_attribs);
            config_count++;
            ASSERT( VA_STATUS_SUCCESS == va_status );
            ASSERT( profile == profiles[i] );
            ASSERT( entrypoint == entrypoints[j] );
            ASSERT( num_attribs >= 0 );
            for(k = 0; k < num_attribs; k++)
            {
                status("  %d -> %08x\n", attrib_list[k].type, attrib_list[k].value);
                ASSERT(attrib_list[k].value != VA_ATTRIB_NOT_SUPPORTED);
            }
        }
    }

    free(attrib_list);
}

void post()
{
    int i;
    for(i = 0; i < config_count; i++)
    {
        va_status = vaDestroyConfig( va_dpy, configs[i] );
        ASSERT( VA_STATUS_SUCCESS == va_status );
    }
    
    free(configs);
    free(entrypoints);
    test_terminate();
}
