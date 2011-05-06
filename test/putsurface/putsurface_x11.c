/*
 * Copyright (c) 2008-2009 Intel Corporation. All Rights Reserved.
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <va/va_x11.h>

static  Window win_thread0, win_thread1;
static  int multi_thread = 0;
static  Pixmap pixmap_thread0, pixmap_thread1;
static  GC context_thread0, context_thread1;
static pthread_mutex_t gmutex;

#include "putsurface_common.c"

static Pixmap create_pixmap(int width, int height)
{
    int screen = DefaultScreen(x11_display);
    Window root;
    Pixmap pixmap;
    XWindowAttributes attr;
    
    root = RootWindow(x11_display, screen);

    XGetWindowAttributes (x11_display, root, &attr);
    
    printf("Create a pixmap from ROOT window %dx%d, pixmap size %dx%d\n\n", attr.width, attr.height, width, height);
    pixmap = XCreatePixmap(x11_display, root, width, height,
                           DefaultDepth(x11_display, DefaultScreen(x11_display)));

    return pixmap;
}

static int create_window(int width, int height)
{
    int screen = DefaultScreen(x11_display);
    Window root, win;

    root = RootWindow(x11_display, screen);

    printf("Create window0 for thread0\n");
    win_thread0 = win = XCreateSimpleWindow(x11_display, root, 0, 0, width, height,
                                            0, 0, WhitePixel(x11_display, 0));
    if (win) {
        XSizeHints sizehints;
        sizehints.width  = width;
        sizehints.height = height;
        sizehints.flags = USSize;
        XSetNormalHints(x11_display, win, &sizehints);
        XSetStandardProperties(x11_display, win, "Thread 0", "Thread 0",
                               None, (char **)NULL, 0, &sizehints);

        XMapWindow(x11_display, win);
    }
    context_thread0 = XCreateGC(x11_display, win, 0, 0);
    XSelectInput(x11_display, win, KeyPressMask | StructureNotifyMask);
    XSync(x11_display, False);

    if (put_pixmap)
        pixmap_thread0 = create_pixmap(width, height);
    
    if (multi_thread == 0)
        return 0;

    printf("Create window1 for thread1\n");
    
    win_thread1 = win = XCreateSimpleWindow(x11_display, root, width, 0, width, height,
                                            0, 0, WhitePixel(x11_display, 0));
    if (win) {
        XSizeHints sizehints;
        sizehints.width  = width;
        sizehints.height = height;
        sizehints.flags = USSize;
        XSetNormalHints(x11_display, win, &sizehints);
        XSetStandardProperties(x11_display, win, "Thread 1", "Thread 1",
                               None, (char **)NULL, 0, &sizehints);

        XMapWindow(x11_display, win);
    }
    if (put_pixmap)
        pixmap_thread1 = create_pixmap(width, height);

    context_thread1 = XCreateGC(x11_display, win, 0, 0);
    XSelectInput(x11_display, win, KeyPressMask | StructureNotifyMask);
    XSync(x11_display, False);
    
    return 0;
}
