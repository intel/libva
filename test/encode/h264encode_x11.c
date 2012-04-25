/*
 * Copyright (c) 2007-2008 Intel Corporation. All Rights Reserved.
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

/*
 * it is a real program to show how VAAPI encoding work,
 * It does H264 element stream level encoding on auto-generated YUV data
 *
 * gcc -o  h264encode  h264encode -lva -lva-x11
 * ./h264encode -w <width> -h <height> -n <frame_num>
 *
 */  
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <va/va.h>
#include <X11/Xlib.h>
#include <va/va_x11.h>

#define SURFACE_NUM 18 /* 16 surfaces for src, 2 surface for reconstructed/reference */

static  Display *x11_display;
static  VADisplay va_dpy;
static  VASurfaceID surface_id[SURFACE_NUM];
static  Window display_win = 0;
static  int win_width;
static  int win_height;

static int display_surface(int frame_id, int *exit_encode);

#include "h264encode_common.c"


static int display_surface(int frame_id, int *exit_encode)
{
    Window win = display_win;
    XEvent event;
    VAStatus va_status;
    
    if (win == 0) { /* display reconstructed surface */
        win_width = frame_width;
        win_height = frame_height;
        
        win = XCreateSimpleWindow(x11_display, RootWindow(x11_display, 0), 0, 0,
                                  frame_width, frame_height, 0, 0, WhitePixel(x11_display, 0));
        XMapWindow(x11_display, win);
        XSync(x11_display, False);

        display_win = win;
    }

    va_status = vaPutSurface(va_dpy, surface_id[frame_id], win,
                             0,0, frame_width, frame_height,
                             0,0, win_width, win_height,
                             NULL,0,0);

    *exit_encode = 0;
    while(XPending(x11_display)) {
        XNextEvent(x11_display, &event);
            
        /* bail on any focused key press */
        if(event.type == KeyPress) {  
            *exit_encode = 1;
            break;
        }
            
        /* rescale the video to fit the window */
        if(event.type == ConfigureNotify) { 
            win_width = event.xconfigure.width;
            win_height = event.xconfigure.height;
        }	
    }	

    return 0;
}

