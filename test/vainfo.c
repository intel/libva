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

#include "va.h"
#include "X11/Xlib.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, const char* argv[])
{
  Display *dpy;
  VADisplay va_dpy;
  VAStatus va_status;
  int major_version, minor_version;
  const char *driver;
  const char *display = getenv("DISPLAY");
  const char *name = rindex(argv[0], '/');

  if (name)
      name++;
  else
      name = argv[0];

  dpy = XOpenDisplay(NULL);
  if (NULL == dpy)
  {
      fprintf(stderr, "%s: Error, can't open display: '%s'\n", name, display ? display : "");
      return 1;
  }
  
  va_dpy = vaGetDisplay(dpy);
  if (NULL == va_dpy)
  {
      fprintf(stderr, "%s: vaGetDisplay() failed\n", name);
      return 2;
  }
  
  va_status = vaInitialize(va_dpy, &major_version, &minor_version);
  if (VA_STATUS_SUCCESS != va_status )
  {
      fprintf(stderr, "%s: vaInitialize failed with error code %d (%s)\n", 
              name, va_status, vaErrorStr(va_status));
      return 3;
  }
  printf("%s: VA API version: %d.%d\n", name, major_version, minor_version);

  driver = vaQueryVendorString(va_dpy);
  printf("%s: Driver version: %s\n", name, driver ? driver : "<unknown>");
  vaTerminate(va_dpy);
  return 0;
}
