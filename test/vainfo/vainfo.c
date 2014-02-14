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

#include "sysdeps.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "va_display.h"

#define CHECK_VASTATUS(va_status,func, ret)                             \
if (va_status != VA_STATUS_SUCCESS) {                                   \
    fprintf(stderr,"%s failed with error code %d (%s),exit\n",func, va_status, vaErrorStr(va_status)); \
    ret_val = ret;                                                      \
    goto error;                                                         \
}

static char * profile_string(VAProfile profile)
{
    switch (profile) {
            case VAProfileNone: return "VAProfileNone";
            case VAProfileMPEG2Simple: return "VAProfileMPEG2Simple";
            case VAProfileMPEG2Main: return "VAProfileMPEG2Main";
            case VAProfileMPEG4Simple: return "VAProfileMPEG4Simple";
            case VAProfileMPEG4AdvancedSimple: return "VAProfileMPEG4AdvancedSimple";
            case VAProfileMPEG4Main: return "VAProfileMPEG4Main";
            case VAProfileH264Baseline: return "VAProfileH264Baseline";
            case VAProfileH264Main: return "VAProfileH264Main";
            case VAProfileH264High: return "VAProfileH264High";
            case VAProfileVC1Simple: return "VAProfileVC1Simple";
            case VAProfileVC1Main: return "VAProfileVC1Main";
            case VAProfileVC1Advanced: return "VAProfileVC1Advanced";
            case VAProfileH263Baseline: return "VAProfileH263Baseline";
            case VAProfileH264ConstrainedBaseline: return "VAProfileH264ConstrainedBaseline";
            case VAProfileJPEGBaseline: return "VAProfileJPEGBaseline";
            case VAProfileH264MultiviewHigh: return "VAProfileH264MultiviewHigh";
            case VAProfileH264StereoHigh: return "VAProfileH264StereoHigh";
            case VAProfileVP8Version0_3: return "VAProfileVP8Version0_3";

            default:
                break;
    }
    return "<unknown profile>";
}


static char * entrypoint_string(VAEntrypoint entrypoint)
{
    switch (entrypoint) {
            case VAEntrypointVLD:return "VAEntrypointVLD";
            case VAEntrypointIZZ:return "VAEntrypointIZZ";
            case VAEntrypointIDCT:return "VAEntrypointIDCT";
            case VAEntrypointMoComp:return "VAEntrypointMoComp";
            case VAEntrypointDeblocking:return "VAEntrypointDeblocking";
            case VAEntrypointEncSlice:return "VAEntrypointEncSlice";
            case VAEntrypointEncPicture:return "VAEntrypointEncPicture";
            case VAEntrypointVideoProc:return "VAEntrypointVideoProc";
            default:
                break;
    }
    return "<unknown entrypoint>";
}

int main(int argc, const char* argv[])
{
  VADisplay va_dpy;
  VAStatus va_status;
  int major_version, minor_version;
  const char *driver;
  const char *name = strrchr(argv[0], '/'); 
  VAProfile profile, *profile_list = NULL;
  int num_profiles, max_num_profiles, i;
  VAEntrypoint entrypoint, entrypoints[10];
  int num_entrypoint;
  int ret_val = 0;
  
  if (name)
      name++;
  else
      name = argv[0];

  va_dpy = va_open_display();
  if (NULL == va_dpy)
  {
      fprintf(stderr, "%s: vaGetDisplay() failed\n", name);
      return 2;
  }
  
  va_status = vaInitialize(va_dpy, &major_version, &minor_version);
  CHECK_VASTATUS(va_status, "vaInitialize", 3);
  
  printf("%s: VA-API version: %d.%d (libva %s)\n",
         name, major_version, minor_version, LIBVA_VERSION_S);

  driver = vaQueryVendorString(va_dpy);
  printf("%s: Driver version: %s\n", name, driver ? driver : "<unknown>");

  printf("%s: Supported profile and entrypoints\n", name);
  max_num_profiles = vaMaxNumProfiles(va_dpy);
  profile_list = malloc(max_num_profiles * sizeof(VAProfile));

  if (!profile_list) {
      printf("Failed to allocate memory for profile list\n");
      ret_val = 5;
      goto error;
  }

  va_status = vaQueryConfigProfiles(va_dpy, profile_list, &num_profiles);
  CHECK_VASTATUS(va_status, "vaQueryConfigProfiles", 6);

  for (i = 0; i < num_profiles; i++) {
      char *profile_str;

      profile = profile_list[i];
      va_status = vaQueryConfigEntrypoints(va_dpy, profile, entrypoints, 
                                           &num_entrypoint);
      if (va_status == VA_STATUS_ERROR_UNSUPPORTED_PROFILE)
	continue;

      CHECK_VASTATUS(va_status, "vaQueryConfigEntrypoints", 4);

      profile_str = profile_string(profile);
      for (entrypoint = 0; entrypoint < num_entrypoint; entrypoint++)
          printf("      %-32s:	%s\n", profile_str, entrypoint_string(entrypoints[entrypoint]));
  }
  
error:
  free(profile_list);
  vaTerminate(va_dpy);
  va_close_display(va_dpy);
  
  return ret_val;
}
