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
#include "va_dricommon.h"

// X error trap
static int x11_error_code = 0;
static int (*old_error_handler)(Display *, XErrorEvent *);

static int 
error_handler(Display *dpy, XErrorEvent *error)
{
    x11_error_code = error->error_code;
    return 0;
}

static void 
x11_trap_errors(void)
{
    x11_error_code    = 0;
    old_error_handler = XSetErrorHandler(error_handler);
}

static int 
x11_untrap_errors(void)
{
    XSetErrorHandler(old_error_handler);
    return x11_error_code;
}

static int 
is_window(Display *dpy, Drawable drawable)
{
    XWindowAttributes wattr;

    x11_trap_errors();
    XGetWindowAttributes(dpy, drawable, &wattr);
    return x11_untrap_errors() == 0;
}

static struct dri_drawable *
do_drawable_hash(VADriverContextP ctx, XID drawable)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->drm_state;
    int index = drawable % DRAWABLE_HASH_SZ;
    struct dri_drawable *dri_drawable = dri_state->drawable_hash[index];

    while (dri_drawable) {
        if (dri_drawable->x_drawable == drawable)
            return dri_drawable;
        dri_drawable = dri_drawable->next;
    }

    dri_drawable = dri_state->createDrawable(ctx, drawable);
    dri_drawable->x_drawable = drawable;
    dri_drawable->is_window = is_window(ctx->native_dpy, drawable);
    dri_drawable->next = dri_state->drawable_hash[index];
    dri_state->drawable_hash[index] = dri_drawable;

    return dri_drawable;
}

void
free_drawable(VADriverContextP ctx, struct dri_drawable* dri_drawable)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->drm_state;
    int i = 0;

    while (i++ < DRAWABLE_HASH_SZ) {
	if (dri_drawable == dri_state->drawable_hash[i]) {
	    dri_state->destroyDrawable(ctx, dri_drawable);
	    dri_state->drawable_hash[i] = NULL;
	}
    }
}

void
free_drawable_hashtable(VADriverContextP ctx)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->drm_state;
    int i;
    struct dri_drawable *dri_drawable, *prev;

    for (i = 0; i < DRAWABLE_HASH_SZ; i++) {
        dri_drawable = dri_state->drawable_hash[i];

        while (dri_drawable) {
            prev = dri_drawable;
            dri_drawable = prev->next;
            dri_state->destroyDrawable(ctx, prev);
        }

	dri_state->drawable_hash[i] = NULL;
    }
}

struct dri_drawable *
dri_get_drawable(VADriverContextP ctx, XID drawable)
{
    return do_drawable_hash(ctx, drawable);
}

void 
dri_swap_buffer(VADriverContextP ctx, struct dri_drawable *dri_drawable)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->drm_state;

    dri_state->swapBuffer(ctx, dri_drawable);
}

union dri_buffer *
dri_get_rendering_buffer(VADriverContextP ctx, struct dri_drawable *dri_drawable)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->drm_state;
    
    return dri_state->getRenderingBuffer(ctx, dri_drawable);
}
