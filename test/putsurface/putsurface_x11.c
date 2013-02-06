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

static  Window window_thread0, window_thread1;
static  GC context_thread0, context_thread1;
static  pthread_mutex_t gmutex;

static void *open_display(void);
static void close_display(void *win_display);
static int create_window(void *win_display, int x, int y, int width, int height);
static int check_window_event(void *x11_display, void *drawable, int *width, int *height, int *quit);

#define CAST_DRAWABLE(a)  (Drawable)(a)

#include "putsurface_common.c"

static void *open_display(void)
{
    return XOpenDisplay(NULL);
}

static void close_display(void *win_display)
{
    XCloseDisplay(win_display);
}

static Pixmap create_pixmap(void *win_display, int width, int height)
{
    Display *x11_display = (Display *)win_display;
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

static int create_window(void *win_display, int x, int y, int width, int height)
{
    Display *x11_display = (Display *)win_display;
    int screen = DefaultScreen(x11_display);
    Window root, win;

    root = RootWindow(x11_display, screen);

    printf("Create window0 for thread0\n");
    drawable_thread0 = (void *)XCreateSimpleWindow(x11_display, root, x, y, width, height,
                                           0, 0, WhitePixel(x11_display, 0));

    win = (Window)drawable_thread0;
    if (drawable_thread0) {
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

    if (put_pixmap) {
        window_thread0 = (Window)drawable_thread0;
        drawable_thread0 = (void *)create_pixmap(x11_display, width, height);
    }
    
    if (multi_thread == 0)
        return 0;

    printf("Create window1 for thread1\n");
    
    drawable_thread1 = (void *)XCreateSimpleWindow(x11_display, root, width, 0, width, height,
                                            0, 0, WhitePixel(x11_display, 0));
    win = (Window)drawable_thread1;
    if (drawable_thread1) {
        XSizeHints sizehints;
        sizehints.width  = width;
        sizehints.height = height;
        sizehints.flags = USSize;
        XSetNormalHints(x11_display, win, &sizehints);
        XSetStandardProperties(x11_display, win, "Thread 1", "Thread 1",
                               None, (char **)NULL, 0, &sizehints);

        XMapWindow(x11_display, win);
    }
    if (put_pixmap) {
        window_thread1 = (Window)drawable_thread1;
        drawable_thread1 = (void *)create_pixmap(x11_display, width, height);
    }

    context_thread1 = XCreateGC(x11_display, win, 0, 0);
    XSelectInput(x11_display, win, KeyPressMask | StructureNotifyMask);
    XSync(x11_display, False);
    
    return 0;
}

static int check_window_event(void *win_display, void *drawable, int *width, int *height, int *quit)
{
    int is_event = 0;
    XEvent event;
    Window win = (Window)drawable;
    Display *x11_display = (Display *)win_display;
    
    
    if (check_event == 0)
        return 0;

    pthread_mutex_lock(&gmutex);
    is_event = XCheckWindowEvent(x11_display, win, StructureNotifyMask|KeyPressMask,&event);
    pthread_mutex_unlock(&gmutex);
    
    if (is_event == 0)
        return 0;

    /* bail on any focused key press */
    if(event.type == KeyPress) {  
        *quit = 1;
        return 0;
    }
    
#if 0
    /* rescale the video to fit the window */
    if(event.type == ConfigureNotify) { 
        *width = event.xconfigure.width;
        *height = event.xconfigure.height;
        printf("Scale window to %dx%d\n", width, height);
    }
#endif

    return 0;
}



