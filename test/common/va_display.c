/*
 * Copyright (c) 2012 Intel Corporation. All Rights Reserved.
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stddef.h>
#include <va/va.h>
#include "va_display.h"

extern const VADisplayHooks va_display_hooks_android;
extern const VADisplayHooks va_display_hooks_x11;

static const VADisplayHooks *g_display_hooks;
static const VADisplayHooks *g_display_hooks_available[] = {
#ifdef ANDROID
    &va_display_hooks_android,
#else
    &va_display_hooks_x11,
#endif
    NULL
};

VADisplay
va_open_display(void)
{
    VADisplay va_dpy = NULL;
    unsigned int i;

    for (i = 0; !va_dpy && g_display_hooks_available[i]; i++) {
        g_display_hooks = g_display_hooks_available[i];
        if (!g_display_hooks->open_display)
            continue;
        va_dpy = g_display_hooks->open_display();
    }
    return va_dpy;
}

void
va_close_display(VADisplay va_dpy)
{
    if (!va_dpy)
        return;

    if (g_display_hooks && g_display_hooks->close_display)
        g_display_hooks->close_display(va_dpy);
}

VAStatus
va_put_surface(
    VADisplay          va_dpy,
    VASurfaceID        surface,
    const VARectangle *src_rect,
    const VARectangle *dst_rect
)
{
    if (!va_dpy)
        return VA_STATUS_ERROR_INVALID_DISPLAY;

    if (g_display_hooks && g_display_hooks->put_surface)
        return g_display_hooks->put_surface(va_dpy, surface, src_rect, dst_rect);
    return VA_STATUS_ERROR_UNIMPLEMENTED;
}
