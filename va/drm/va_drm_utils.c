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
    const char *name;
};

static const struct driver_name_map g_driver_name_map[] = {
    { "i915",       "iHD"    }, // Intel Media driver
    { "i915",       "i965"   }, // Intel OTC GenX driver
    { "pvrsrvkm",   "pvr"    }, // Intel UMG PVR driver
    { "emgd",       "emgd"   }, // Intel ECG PVR driver
    { "hybrid",     "hybrid" }, // Intel OTC Hybrid driver
    { "nouveau",    "nouveau"  }, // Mesa Gallium driver
    { "radeon",     "r600"     }, // Mesa Gallium driver
    { "amdgpu",     "radeonsi" }, // Mesa Gallium driver
    { "nvidia-drm", "nvidia"   }, // NVIDIA driver
    { NULL,         NULL }
};

/* Returns the VA driver candidate num for the active display*/
VAStatus
VA_DRM_GetNumCandidates(VADriverContextP ctx, int * num_candidates)
{
    struct drm_state * const drm_state = ctx->drm_state;
    drmVersionPtr drm_version;
    int count = 0;
    const struct driver_name_map *m = NULL;
    if (!drm_state || drm_state->fd < 0)
        return VA_STATUS_ERROR_INVALID_DISPLAY;
    drm_version = drmGetVersion(drm_state->fd);
    if (!drm_version)
        return VA_STATUS_ERROR_UNKNOWN;
    for (m = g_driver_name_map; m->key != NULL; m++) {
        if (strcmp(m->key, drm_version->name) == 0) {
            count ++;
        }
    }
    drmFreeVersion(drm_version);
    *num_candidates = count;
    return count ? VA_STATUS_SUCCESS : VA_STATUS_ERROR_UNKNOWN;
}

/* Returns the VA driver name for the active display */
VAStatus
VA_DRM_GetDriverName(VADriverContextP ctx, char **driver_name_ptr, int candidate_index)
{
    struct drm_state * const drm_state = ctx->drm_state;
    drmVersionPtr drm_version;
    char *driver_name = NULL;
    const struct driver_name_map *m;
    int current_index = 0;

    *driver_name_ptr = NULL;

    if (!drm_state || drm_state->fd < 0)
        return VA_STATUS_ERROR_INVALID_DISPLAY;

    drm_version = drmGetVersion(drm_state->fd);
    if (!drm_version)
        return VA_STATUS_ERROR_UNKNOWN;

    for (m = g_driver_name_map; m->key != NULL; m++) {
        if (strcmp(m->key, drm_version->name) == 0) {
            if (current_index == candidate_index) {
                break;
            }
            current_index ++;
        }
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
