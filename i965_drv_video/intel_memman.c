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

#include "intel_driver.h"

Bool 
intel_memman_init(struct intel_driver_data *intel)
{
    intel->bufmgr = intel_bufmgr_gem_init(intel->fd, BATCH_SIZE);
    assert(intel->bufmgr);
    intel_bufmgr_gem_enable_reuse(intel->bufmgr);

    return True;
}

Bool 
intel_memman_terminate(struct intel_driver_data *intel)
{
    drm_intel_bufmgr_destroy(intel->bufmgr);
    return True;
}
