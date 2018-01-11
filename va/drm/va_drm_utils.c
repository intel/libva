/*
 * va_drm_utils.c - VA/DRM Utilities
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

#include "sysdeps.h"
#include <xf86drm.h>
#include <sys/stat.h>
#include "va_drm_utils.h"
#include "va_drmcommon.h"

struct driver_name_map {
    const char *key;
    int         key_len;
    const char *name;
};

static const struct driver_name_map g_driver_name_map[] = {
    { "i915",       4, "i965"   }, // Intel OTC GenX driver
    { "pvrsrvkm",   8, "pvr"    }, // Intel UMG PVR driver
    { "emgd",       4, "emgd"   }, // Intel ECG PVR driver
    { "hybrid",     6, "hybrid" }, // Intel OTC Hybrid driver
    { "nouveau",    7, "nouveau"  }, // Mesa Gallium driver
    { "radeon",     6, "r600"     }, // Mesa Gallium driver
    { "amdgpu",     6, "radeonsi" }, // Mesa Gallium driver
    { NULL, }
};

/* Returns the VA driver name for the active display */
VAStatus
VA_DRM_GetDriverName(VADriverContextP ctx, char **driver_name_ptr)
{
    struct drm_state * const drm_state = ctx->drm_state;
    drmVersionPtr drm_version;
    char *driver_name = NULL;
    const struct driver_name_map *m;

    *driver_name_ptr = NULL;

    if (!drm_state || drm_state->fd < 0)
        return VA_STATUS_ERROR_INVALID_DISPLAY;

    drm_version = drmGetVersion(drm_state->fd);
    if (!drm_version)
        return VA_STATUS_ERROR_UNKNOWN;

    for (m = g_driver_name_map; m->key != NULL; m++) {
        if (drm_version->name_len >= m->key_len &&
            strncmp(drm_version->name, m->key, m->key_len) == 0)
            break;
    }
    drmFreeVersion(drm_version);

    if (!m->name)
        return VA_STATUS_ERROR_UNKNOWN;

    driver_name = strdup(m->name);
    if (!driver_name)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    *driver_name_ptr = driver_name;
    return VA_STATUS_SUCCESS;
}

/* Checks whether the file descriptor is a DRM Render-Nodes one */
int
VA_DRM_IsRenderNodeFd(int fd)
{
    struct stat st;
    const char *name;

    /* Check by device node */
    if (fstat(fd, &st) == 0)
        return S_ISCHR(st.st_mode) && (st.st_rdev & 0x80);

    /* Check by device name */
    name = drmGetDeviceNameFromFd(fd);
    if (name)
        return strncmp(name, "/dev/dri/renderD", 16) == 0;

    /* Unrecoverable error */
    return -1;
}
