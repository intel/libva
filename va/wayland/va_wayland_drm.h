/*
 * va_wayland_drm.h - Wayland/DRM helpers
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

#ifndef VA_WAYLAND_DRM_H
#define VA_WAYLAND_DRM_H

#include <stdbool.h>
#include "va_wayland.h"
#include "va_backend.h"
#include "va_backend_wayland.h"

/**
 * \brief Initializes Wayland/DRM layer.
 *
 * This is an internal function used to initialize the VA/DRM subsystem
 * if the application is running on a DRM-based server.
 *
 * @param[in]   pDisplayContext the VA display context
 * @return true if successful
 */
DLL_HIDDEN
bool
va_wayland_drm_create(VADisplayContextP pDisplayContext);

DLL_HIDDEN
void
va_wayland_drm_destroy(VADisplayContextP pDisplayContext);

#endif /* VA_WAYLAND_DRM_H */
