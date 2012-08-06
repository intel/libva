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

/**
 * \brief Returns the VA driver name for the active display.
 *
 * This functions returns a newly allocated buffer in @driver_name_ptr that
 * contains the VA driver name for the active display. Active display means
 * the display obtained with any of the vaGetDisplay*() functions.
 *
 * The VADriverContext.drm_state structure must be valid, i.e. allocated
 * and containing an open DRM connection descriptor. The DRM connection
 * does *not* need to be authenticated as it only performs a call to
 * drmGetVersion().
 *
 * @param[in]   ctx     the pointer to a VADriverContext
 * @param[out]  driver_name_ptr the newly allocated buffer containing
 *     the VA driver name
 * @return VA_STATUS_SUCCESS if operation is successful, or another
 *     #VAStatus value otherwise.
 */
DLL_HIDDEN
VAStatus
VA_DRM_GetDriverName(VADriverContextP ctx, char **driver_name_ptr);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* VA_DRM_UTILS_H */
