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
#ifndef _VA_DRICOMMON_H_
#define _VA_DRICOMMON_H_

#ifndef ANDROID
#include <X11/Xlib.h>
#include <xf86drm.h>
#include <drm.h>
#include <drm_sarea.h>
#endif

#include <va/va_backend.h>
#include <va/va_drmcommon.h>
#include <xcb/present.h>

#ifdef ANDROID
#define XID unsigned int
#define Bool int
#endif

enum {
    /* Compatibility. Do not use for newly-written code. */
    VA_NONE     = VA_DRM_AUTH_NONE,
    VA_DRI1     = VA_DRM_AUTH_DRI1,
    VA_DRI2     = VA_DRM_AUTH_DRI2,
    VA_DUMMY    = VA_DRM_AUTH_CUSTOM
};

union dri_buffer 
{
    struct {
        unsigned int attachment;
        unsigned int name;
        unsigned int pitch;
        unsigned int cpp;
        unsigned int flags;
        /** \brief Reserved bytes for future use, must be zero */
        unsigned int va_reserved[8];
    } dri2;
};

struct dri_drawable 
{
    XID x_drawable;
    int is_window;
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    struct dri_drawable *next;
};

struct dri3_fence {
    XID xid;
    void *addr;
};

#define DRAWABLE_HASH_SZ 32
struct dri_state 
{
    struct drm_state base;
#ifndef ANDROID
    struct dri_drawable *drawable_hash[DRAWABLE_HASH_SZ];

    struct dri_drawable *(*createDrawable)(VADriverContextP ctx, XID x_drawable);
    void (*destroyDrawable)(VADriverContextP ctx, struct dri_drawable *dri_drawable);
    void (*swapBuffer)(VADriverContextP ctx, struct dri_drawable *dri_drawable);
    union dri_buffer *(*getRenderingBuffer)(VADriverContextP ctx, struct dri_drawable *dri_drawable);
    void (*close)(VADriverContextP ctx);
    int (*createfd)(VADriverContextP ctx, Pixmap pixmap, int *stride);
    Pixmap
    (*createPixmap)(VADriverContextP ctx, Drawable draw,
                    int width, int height, int depth,
                    int fd, int bpp, int stride, int size);
    void (*presentPixmap)(VADriverContextP ctx, Drawable draw,
                          Pixmap pixmap, unsigned int serial,
                          xcb_xfixes_region_t valid,
                          xcb_xfixes_region_t update,
                          unsigned short int x_off,
                          unsigned short int y_off,
                          xcb_randr_crtc_t target_crtc,
                          xcb_sync_fence_t wait_fence,
                          xcb_sync_fence_t idle_fence,
                          unsigned int options,
                          unsigned long int target_msc,
                          unsigned long int divisor,
                          unsigned long int  remainder,
                          unsigned int notifies_len,
                          const xcb_present_notify_t *notifies);
    int
    (*create_fence)(VADriverContextP ctx, Pixmap pixmap,
                         struct dri3_fence *fence);
    void (*fence_sync)(VADriverContextP ctx, struct dri3_fence *fence);
    void (*fence_free)(VADriverContextP ctx, struct dri3_fence *fence);
#endif
    /** \brief Reserved bytes for future use, must be zero */
    unsigned long  va_reserved[16];
};

Bool va_isDRI2Connected(VADriverContextP ctx, char **driver_name);
void va_dri_free_drawable(VADriverContextP ctx, struct dri_drawable* dri_drawable);
void va_dri_free_drawable_hashtable(VADriverContextP ctx);
struct dri_drawable *va_dri_get_drawable(VADriverContextP ctx, XID drawable);
void va_dri_swap_buffer(VADriverContextP ctx, struct dri_drawable *dri_drawable);
union dri_buffer *va_dri_get_rendering_buffer(VADriverContextP ctx, struct dri_drawable *dri_drawable);

#ifdef HAVE_VA_DRI3
Bool va_isDRI3Connected(VADriverContextP ctx, char **driver_name);
Pixmap
va_dri3_createPixmap(VADriverContextP ctx, Drawable draw,
                     int width, int height, int depth,
                     int fd, int bpp, int stride, int size);
int va_dri3_createfd(VADriverContextP ctx, Pixmap pixmap, int *stride);
void
va_dri3_presentPixmap(VADriverContextP ctx, Drawable draw,
                      Pixmap pixmap, unsigned int serial,
                      xcb_xfixes_region_t valid,
                      xcb_xfixes_region_t update,
                      unsigned short int x_off,
                      unsigned short int y_off,
                      xcb_randr_crtc_t target_crtc,
                      xcb_sync_fence_t wait_fence,
                      xcb_sync_fence_t idle_fence,
                      unsigned int options,
                      unsigned long int target_msc,
                      unsigned long int divisor,
                      unsigned long int  remainder,
                      unsigned int notifies_len,
                      const xcb_present_notify_t *notifies);
void va_dri3_close(VADriverContextP ctx);
int
va_dri3_create_fence(VADriverContextP ctx, Pixmap pixmap,
                     struct dri3_fence *fence);
void va_dri3_fence_sync(VADriverContextP ctx, struct dri3_fence *fence);
void va_dri3_fence_free(VADriverContextP ctx, struct dri3_fence *fence);
#endif

#endif /* _VA_DRICOMMON_H_ */
