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
#define Display unsigned int
Display *dpy;
VADisplay va_dpy;
VAStatus va_status;
VAProfile *profiles ;
int major_version, minor_version;

void test_init()
{
    dpy = (Display*)malloc(sizeof(Display));
    *(dpy) = 0x18c34078;
    ASSERT( dpy );
    status("malloc Display: dpy = %08x\n", dpy);

    va_dpy = vaGetDisplay(dpy);
    ASSERT( va_dpy );  
    status("vaGetDisplay: va_dpy = %08x\n", va_dpy);

    va_status = vaInitialize(va_dpy, &major_version, &minor_version);
    ASSERT( VA_STATUS_SUCCESS == va_status );
    status("vaInitialize: major = %d minor = %d\n", major_version, minor_version);
}

void test_terminate()
{
    va_status = vaTerminate(va_dpy);
    ASSERT( VA_STATUS_SUCCESS == va_status );
    status("vaTerminate\n");

    free(dpy);
    status("free Display\n");

    if (profiles)
    {
        free(profiles);
        profiles = NULL;
    }
}

