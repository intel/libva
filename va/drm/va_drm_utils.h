/*
 * va_drm_utils.h - VA/DRM Utilities
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

#ifndef VA_DRM_UTILS_H
#define VA_DRM_UTILS_H

#include <va/va_backend.h>

/**
 * \file va_drm_utils.h
 * \brief VA/DRM Utilities
 *
 * This file contains VA/DRM utility functions. The API herein defined is
 * internal to libva. Users include the VA/DRM API itself or VA/Android,
 * should it be based on DRM.
 */

#ifdef __cplusplus
extern "C" {
#endif

DLL_HIDDEN
VAStatus
VA_DRM_GetDriverNames(VADriverContextP ctx, char **drivers, unsigned *num_drivers);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* VA_DRM_UTILS_H */
