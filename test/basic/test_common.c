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

#include <va/va.h>
#ifdef ANDROID
#include <va/va_android.h>
#else
#include <va/va_x11.h>
#endif
#include "assert.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dlfcn.h>

#define ASSERT	assert

void status(const char *msg, ...);
#ifdef ANDROID
#include "test_android.c"
#else
#include "test_x11.c"
#endif

Display *dpy;
VADisplay va_dpy;
VAStatus va_status;
int major_version, minor_version;
int print_status = 0;
int num_profiles;
VAProfile *profiles = NULL;

void pre();
void test();
void post();

void status(const char *msg, ...)
{
  if (!print_status) return;
  va_list args;
  printf("--- ");
  va_start(args, msg);
  vfprintf(stdout, msg, args);
  va_end(args);
}


int main(int argc, const char* argv[])
{
  const char *name = strrchr(argv[0], '/');
  if (name)
      name++;
  else
      name = argv[0];
  printf("*** %s: %s\n", name, TEST_DESCRIPTION);
  pre();
  print_status = 1;
  test();
  print_status = 0;
  post();
  printf("*** %s: Finished\n", name);
  return 0;
}

#define PROFILE(profile)	case VAProfile##profile:	return("VAProfile" #profile);

const char *profile2string(VAProfile profile)
{
    switch(profile)
    {
        PROFILE(None)
        PROFILE(MPEG2Simple)
        PROFILE(MPEG2Main)
        PROFILE(MPEG4Simple)
        PROFILE(MPEG4AdvancedSimple)
        PROFILE(MPEG4Main)
        PROFILE(H263Baseline)
        PROFILE(H264Baseline)
        PROFILE(H264Main)
        PROFILE(H264High)
        PROFILE(H264ConstrainedBaseline)
        PROFILE(H264MultiviewHigh)
        PROFILE(H264StereoHigh)
        PROFILE(VC1Simple)
        PROFILE(VC1Main)
        PROFILE(VC1Advanced)
        PROFILE(JPEGBaseline)
        PROFILE(VP8Version0_3)
        default:return "Unknow";
    }
    ASSERT(0);
    return "Unknown";
}

#define ENTRYPOINT(profile)	case VAEntrypoint##profile:	return("VAEntrypoint" #profile);

const char *entrypoint2string(VAEntrypoint entrypoint)
{
    switch(entrypoint)
    {
        ENTRYPOINT(VLD)
        ENTRYPOINT(IZZ)
        ENTRYPOINT(IDCT)
        ENTRYPOINT(MoComp)
        ENTRYPOINT(Deblocking)
        ENTRYPOINT(EncSlice)
        ENTRYPOINT(EncPicture)
        ENTRYPOINT(VideoProc)
        default:return "Unknow";
    }
    ASSERT(0);
    return "Unknown";
}


void test_profiles()
{
    int max_profiles;
    int i;
    max_profiles = vaMaxNumProfiles(va_dpy);
    status("vaMaxNumProfiles = %d\n", max_profiles);
    ASSERT(max_profiles > 0);
    profiles = malloc(max_profiles * sizeof(VAProfile));
    ASSERT(profiles);
      
    va_status = vaQueryConfigProfiles(va_dpy, profiles, &num_profiles);
    ASSERT( VA_STATUS_SUCCESS == va_status );
      
    status("vaQueryConfigProfiles reports %d profiles\n", num_profiles);
    ASSERT(num_profiles <= max_profiles);
    ASSERT(num_profiles > 0);
    
    if (print_status)
    {
        for(i = 0; i < num_profiles; i++)
        {
            status("  profile %d [%s]\n", profiles[i], profile2string(profiles[i]));
        }
    }
}
