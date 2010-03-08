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

#include <va/va_x11.h>

#include "assert.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#define ASSERT	assert

int main(int argc, const char* argv[])
{
  Display *dpy;
  VADisplay va_dpy;
  VAStatus va_status;
  int major_version, minor_version; 
   
  dpy = XOpenDisplay(NULL);
  ASSERT( dpy );
  printf("XOpenDisplay: dpy = %08x\n", dpy);
  
  va_dpy = vaGetDisplay(dpy);
  ASSERT( va_dpy );  
  printf("vaGetDisplay: va_dpy = %08x\n", va_dpy);
  
  va_status = vaInitialize(va_dpy, &major_version, &minor_version);
  ASSERT( VA_STATUS_SUCCESS == va_status );
  printf("vaInitialize: major = %d minor = %d\n", major_version, minor_version);

  {
      VASurfaceID surfaces[21];
      int i;
      
      surfaces[20] = -1;
      va_status = vaCreateSurfaces(va_dpy, 720, 480, VA_RT_FORMAT_YUV420, 20, surfaces);
      ASSERT( VA_STATUS_SUCCESS == va_status );
      ASSERT( -1 == surfaces[20] ); /* bounds check */
      for(i = 0; i < 20; i++)
      {
          printf("Surface %d surface_id = %08x\n", i, surfaces[i]);
      }
      Window win = XCreateSimpleWindow(dpy, RootWindow(dpy, 0), 0, 0, 720, 480, 0, 0, WhitePixel(dpy, 0));
      printf("Window = %08x\n", win); 
      XMapWindow(dpy, win);
      XSync(dpy, False);
      
      vaPutSurface(va_dpy, surfaces[0], win, 0, 0, 720, 480, 0, 0, 720, 480, 0); 

      sleep(10);
      va_status = vaDestroySurface(va_dpy, surfaces, 20);
      ASSERT( VA_STATUS_SUCCESS == va_status );
  }
  
  {
      int num_profiles;
      int i;
      VAProfile *profiles = malloc(vaMaxNumProfiles(va_dpy) * sizeof(VAProfile));
      ASSERT(profiles);
      printf("vaMaxNumProfiles = %d\n", vaMaxNumProfiles(va_dpy));
      
      va_status = vaQueryConfigProfiles(va_dpy, profiles, &num_profiles);
      ASSERT( VA_STATUS_SUCCESS == va_status );
      
      printf("vaQueryConfigProfiles reports %d profiles\n", num_profiles);
      for(i = 0; i < num_profiles; i++)
      {
          printf("Profile %d\n", profiles[i]);
      }
  }

  {
      VASurfaceID surfaces[20];
      VAContextID context;
      VAConfigAttrib attrib;
      VAConfigID config_id;
      int i;

      attrib.type = VAConfigAttribRTFormat;
      va_status = vaGetConfigAttributes(va_dpy, VAProfileMPEG2Main, VAEntrypointVLD,
                                &attrib, 1);
      ASSERT( VA_STATUS_SUCCESS == va_status );

      ASSERT(attrib.value & VA_RT_FORMAT_YUV420);
      /* Found desired RT format, keep going */ 

      va_status = vaCreateConfig(va_dpy, VAProfileMPEG2Main, VAEntrypointVLD, &attrib, 1,
                       &config_id);
      ASSERT( VA_STATUS_SUCCESS == va_status );

      va_status = vaCreateSurfaces(va_dpy, 720, 480, VA_RT_FORMAT_YUV420, 20, surfaces);
      ASSERT( VA_STATUS_SUCCESS == va_status );

      va_status = vaCreateContext(va_dpy, config_id, 720, 480, 0 /* flag */, surfaces, 20, &context);
      ASSERT( VA_STATUS_SUCCESS == va_status );

      va_status = vaDestroyContext(va_dpy, context);
      ASSERT( VA_STATUS_SUCCESS == va_status );

      va_status = vaDestroySurface(va_dpy, surfaces, 20);
      ASSERT( VA_STATUS_SUCCESS == va_status );
  }

  {
      VABufferID picture_buf[3];
      va_status = vaCreateBuffer(va_dpy, VAPictureParameterBufferType, &picture_buf[0]);
      ASSERT( VA_STATUS_SUCCESS == va_status );
      va_status = vaCreateBuffer(va_dpy, VAPictureParameterBufferType, &picture_buf[1]);
      ASSERT( VA_STATUS_SUCCESS == va_status );
      va_status = vaCreateBuffer(va_dpy, VAPictureParameterBufferType, &picture_buf[2]);
      ASSERT( VA_STATUS_SUCCESS == va_status );

      va_status = vaDestroyBuffer(va_dpy, picture_buf[0]);
      ASSERT( VA_STATUS_SUCCESS == va_status );
      va_status = vaDestroyBuffer(va_dpy, picture_buf[2]);
      ASSERT( VA_STATUS_SUCCESS == va_status );
      va_status = vaDestroyBuffer(va_dpy, picture_buf[1]);
      ASSERT( VA_STATUS_SUCCESS == va_status );
  }

  va_status = vaTerminate(va_dpy);
  ASSERT( VA_STATUS_SUCCESS == va_status );
  printf("vaTerminate\n");

  XCloseDisplay(dpy);

  return 0;
}
