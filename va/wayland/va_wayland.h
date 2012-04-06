/*
 * va_wayland.h - Wayland API
 *
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
 * IN NO EVENT SHALL INTEL AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef VA_WAYLAND_H
#define VA_WAYLAND_H

#include <va/va.h>
#include <wayland-client.h>

/**
 * \file va_wayland.h
 * \brief The Wayland rendering API
 *
 * This file contains the \ref api_wayland "Wayland rendering API".
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup api_wayland Wayland rendering API
 *
 * @{
 *
 * Theory of operations:
 * - Create a VA display for an active Wayland display ;
 * - Perform normal VA-API operations, e.g. decode to a VA surface ;
 * - Get wl_buffer associated to the VA surface ;
 * - Attach wl_buffer to wl_surface ;
 */

/**
 * \brief Returns a VA display wrapping the specified Wayland display.
 *
 * This functions returns a (possibly cached) VA display from the
 * specified Wayland @display.
 *
 * @param[in]   display         the native Wayland display
 * @return the VA display
 */
VADisplay
vaGetDisplayWl(struct wl_display *display);

/**
 * \brief Returns the Wayland buffer associated with a VA surface.
 *
 * This function returns a wl_buffer handle that can be used as an
 * argument to wl_surface_attach(). This buffer references the
 * underlying VA @surface. As such, the VA @surface and Wayland
 * @out_buffer have the same size and color format. Should specific
 * color conversion be needed, then VA/VPP API can fulfill this
 * purpose.
 *
 * @param[in]   dpy         the VA display
 * @param[in]   surface     the VA surface
 * @param[out]  out_buffer  a wl_buffer wrapping the VA @surface
 * @return VA_STATUS_SUCCESS if successful
 */
VAStatus
vaGetSurfaceBufferWl(
    VADisplay           dpy,
    VASurfaceID         surface,
    struct wl_buffer  **out_buffer
);

/**
 * \brief Returns the Wayland buffer associated with a VA image.
 *
 * This function returns a wl_buffer handle that can be used as an
 * argument to wl_surface_attach(). This buffer references the
 * underlying VA @image. As such, the VA @image and Wayland
 * @out_buffer have the same size and color format. Should specific
 * color conversion be needed, then VA/VPP API can fulfill this
 * purpose.
 *
 * @param[in]   dpy         the VA display
 * @param[in]   image       the VA image
 * @param[out]  out_buffer  a wl_buffer wrapping the VA @image
 * @return VA_STATUS_SUCCESS if successful
 */
VAStatus
vaGetImageBufferWl(
    VADisplay           dpy,
    VAImageID           image,
    struct wl_buffer  **out_buffer
);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* VA_WAYLAND_H */
