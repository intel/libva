/*
 * Copyright © 2009 Intel Corporation
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
 *
 * Authors:
 *    Xiang Haihao <haihao.xiang@intel.com>
 *    Zou Nan hai <nanhai.zou@intel.com>
 *
 */

#include <assert.h>

#include "va_dricommon.h"

#include "intel_batchbuffer.h"
#include "intel_memman.h"
#include "intel_driver.h"

static Bool
intel_driver_get_param(struct intel_driver_data *intel, int param, int *value)
{
   struct drm_i915_getparam gp;

   gp.param = param;
   gp.value = value;

   return drmCommandWriteRead(intel->fd, DRM_I915_GETPARAM, &gp, sizeof(gp)) == 0;
}

Bool 
intel_driver_init(VADriverContextP ctx)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    struct dri_state *dri_state = (struct dri_state *)ctx->dri_state;
    int has_exec2, has_bsd, has_blt;

    assert(dri_state);
    assert(dri_state->driConnectedFlag == VA_DRI2 || 
           dri_state->driConnectedFlag == VA_DRI1);

    intel->fd = dri_state->fd;
    intel->dri2Enabled = (dri_state->driConnectedFlag == VA_DRI2);

    if (!intel->dri2Enabled) {
        drm_sarea_t *pSAREA;

        pSAREA = (drm_sarea_t *)dri_state->pSAREA;
        intel->hHWContext = dri_state->hwContext;
        intel->driHwLock = (drmLock *)(&pSAREA->lock);
        intel->pPrivSarea = (void *)pSAREA + sizeof(drm_sarea_t);
    }

    intel->locked = 0;
    pthread_mutex_init(&intel->ctxmutex, NULL);

    intel_driver_get_param(intel, I915_PARAM_CHIPSET_ID, &intel->device_id);
    if (intel_driver_get_param(intel, I915_PARAM_HAS_EXECBUF2, &has_exec2))
        intel->has_exec2 = has_exec2;
    if (intel_driver_get_param(intel, I915_PARAM_HAS_BSD, &has_bsd))
        intel->has_bsd = has_bsd;
    if (intel_driver_get_param(intel, I915_PARAM_HAS_BLT, &has_blt))
        intel->has_blt = has_blt;

    intel_memman_init(intel);
    return True;
}

Bool 
intel_driver_terminate(VADriverContextP ctx)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    intel_memman_terminate(intel);
    pthread_mutex_destroy(&intel->ctxmutex);

    return True;
}

void 
intel_lock_hardware(VADriverContextP ctx)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    char __ret = 0;

    PPTHREAD_MUTEX_LOCK();

    assert(!intel->locked);

    if (!intel->dri2Enabled) {
        DRM_CAS(intel->driHwLock, 
                intel->hHWContext,
                (DRM_LOCK_HELD|intel->hHWContext),
                __ret);

        if (__ret) {
            drmGetLock(intel->fd, intel->hHWContext, 0);
        }	
    }

    intel->locked = 1;
}

void 
intel_unlock_hardware(VADriverContextP ctx)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    if (!intel->dri2Enabled) {
        DRM_UNLOCK(intel->fd, 
                   intel->driHwLock,
                   intel->hHWContext);
    }

    intel->locked = 0;
    PPTHREAD_MUTEX_UNLOCK();
}
