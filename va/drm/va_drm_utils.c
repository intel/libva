/*
 * va_drm_utils.c - VA/DRM Utilities
 *
 * Copyright (c) 2012 Intel Corporation. All Rights Reserved.
 * Copyright (c) 2023 Emil Velikov
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
#include <sys/utsname.h>
#include "va_drm_utils.h"
#include "va_drmcommon.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct driver_name_map {
    const char *key;
    const char *name;
};

static const struct driver_name_map g_driver_name_map[] = {
    { "i915",       "iHD"    }, // Intel Media driver
    { "i915",       "i965"   }, // Intel OTC GenX driver
    { "pvrsrvkm",   "pvr"    }, // Intel UMG PVR driver
    { "radeon",     "r600"     }, // Mesa Gallium driver
    { "radeon",     "radeonsi" }, // Mesa Gallium driver
    { "amdgpu",     "radeonsi" }, // Mesa Gallium driver
    { "WSL",        "d3d12"    }, // Mesa Gallium driver
    { "nvidia-drm", "nvidia"   }, // NVIDIA driver
    { NULL,         NULL }
};

static char *
va_DRM_GetDrmDriverName(int fd)
{
    drmVersionPtr drm_version = drmGetVersion(fd);
    char *driver_name;

    if (!drm_version)
        return NULL;

    driver_name = strdup(drm_version->name);
    drmFreeVersion(drm_version);

    return driver_name;
}

/* Returns the VA driver candidate num for the active display*/
VAStatus
VA_DRM_GetNumCandidates(VADriverContextP ctx, int * num_candidates)
{
    struct drm_state * const drm_state = ctx->drm_state;
    int count = 0;
    const struct driver_name_map *m = NULL;
    char *driver_name;

    if (!drm_state || drm_state->fd < 0)
        return VA_STATUS_ERROR_INVALID_DISPLAY;

    driver_name = va_DRM_GetDrmDriverName(drm_state->fd);
    if (!driver_name)
        return VA_STATUS_ERROR_UNKNOWN;

    for (m = g_driver_name_map; m->key != NULL; m++) {
        if (strcmp(m->key, driver_name) == 0) {
            count ++;
        }
    }

    free(driver_name);

    /*
     * If the drm driver name does not have a mapped vaapi driver name, then
     * assume they have the same name.
     */
    if (count == 0)
        count = 1;

    *num_candidates = count;
    return VA_STATUS_SUCCESS;
}

/* Returns the VA driver name for the active display */
VAStatus
VA_DRM_GetDriverName(VADriverContextP ctx, char **driver_name_ptr, int candidate_index)
{
    struct drm_state * const drm_state = ctx->drm_state;
    const struct driver_name_map *m;
    int current_index = 0;

    *driver_name_ptr = NULL;

    if (!drm_state || drm_state->fd < 0)
        return VA_STATUS_ERROR_INVALID_DISPLAY;

    *driver_name_ptr = va_DRM_GetDrmDriverName(drm_state->fd);

    if (!*driver_name_ptr)
        return VA_STATUS_ERROR_UNKNOWN;

    /* Map vgem to WSL2 for Windows subsystem for linux */
    struct utsname sysinfo = {};
    if (!strncmp(*driver_name_ptr, "vgem", 4) && uname(&sysinfo) >= 0 &&
        strstr(sysinfo.release, "WSL")) {
        free(*driver_name_ptr);
        *driver_name_ptr = strdup("WSL");
    }

    for (m = g_driver_name_map; m->key != NULL; m++) {
        if (strcmp(m->key, *driver_name_ptr) == 0) {
            if (current_index == candidate_index) {
                break;
            }
            current_index ++;
        }
    }

    /*
     * If the drm driver name does not have a mapped vaapi driver name, then
     * assume they have the same name.
     */
    if (!m->name)
        return VA_STATUS_SUCCESS;

    /* Use the mapped vaapi driver name */
    free(*driver_name_ptr);
    *driver_name_ptr = strdup(m->name);
    if (!*driver_name_ptr)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    return VA_STATUS_SUCCESS;
}

/* Returns the VA driver names and how many they are, for the active display */
VAStatus
VA_DRM_GetDriverNames(VADriverContextP ctx, char **drivers, unsigned *num_drivers)
{
#define MAX_NAMES 2 // Adjust if needed

    static const struct {
        const char * const drm_driver;
        const char * const va_driver[MAX_NAMES];
    } map[] = {
        { "i915",       { "iHD", "i965"      } }, // Intel Media and OTC GenX
        { "pvrsrvkm",   { "pvr"              } }, // Intel UMG PVR
        { "radeon",     { "r600", "radeonsi" } }, // Mesa Gallium
        { "amdgpu",     { "radeonsi"         } }, // Mesa Gallium
        { "WSL",        { "d3d12"            } }, // Mesa Gallium
        { "nvidia-drm", { "nvidia"           } }, // Unofficial NVIDIA
    };

    const struct drm_state * const drm_state = ctx->drm_state;
    char *drm_driver;
    unsigned count = 0;

    drm_driver = va_DRM_GetDrmDriverName(drm_state->fd);
    if (!drm_driver)
        return VA_STATUS_ERROR_UNKNOWN;

    /* Map vgem to WSL2 for Windows subsystem for linux */
    struct utsname sysinfo = {};
    if (!strncmp(drm_driver, "vgem", 4) && uname(&sysinfo) >= 0 &&
        strstr(sysinfo.release, "WSL")) {
        free(drm_driver);
        drm_driver = strdup("WSL");
        if (!drm_driver)
            return VA_STATUS_ERROR_UNKNOWN;
    }

    for (unsigned i = 0; i < ARRAY_SIZE(map); i++) {
        if (strcmp(map[i].drm_driver, drm_driver) == 0) {
            const char * const *va_drivers = map[i].va_driver;

            while (va_drivers[count]) {
                if (count < MAX_NAMES && count < *num_drivers)
                    drivers[count] = strdup(va_drivers[count]);
                count++;
            }
            break;
        }
    }

    /* Fallback to the drm driver, if there's no va equivalent in the map. */
    if (!count) {
        drivers[count] = drm_driver;
        count++;
    } else {
        free(drm_driver);
    }

    *num_drivers = count;

    return VA_STATUS_SUCCESS;
}
