/*
 * Copyright © 2010 Intel Corporation
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
 *    Zhou Chang <chang.zhou@intel.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <va/va_backend.h>

#include "intel_batchbuffer.h"
#include "intel_driver.h"

#include "i965_defines.h"
#include "i965_drv_video.h"
#include "i965_encoder.h"

static void 
gen6_encoder_end_picture(VADriverContextP ctx, 
                         VAProfile profile, 
                         union codec_state *codec_state,
                         struct hw_context *hw_context)
{
    struct gen6_encoder_context *gen6_encoder_context = (struct gen6_encoder_context *)hw_context;
    struct encode_state *encode_state = &codec_state->enc;
    VAStatus vaStatus;

    vaStatus = gen6_vme_pipeline(ctx, profile, encode_state, gen6_encoder_context);

    if (vaStatus == VA_STATUS_SUCCESS)
        gen6_mfc_pipeline(ctx, profile, encode_state, gen6_encoder_context);
}
static void
gen6_encoder_context_destroy(void *hw_context)
{
    struct gen6_encoder_context *gen6_encoder_context = (struct gen6_encoder_context *)hw_context;

    gen6_mfc_context_destroy(&gen6_encoder_context->mfc_context);
    gen6_vme_context_destroy(&gen6_encoder_context->vme_context);
    free(gen6_encoder_context);
}

struct hw_context *
gen6_enc_hw_context_init(VADriverContextP ctx, VAProfile profile)
{
    struct gen6_encoder_context *gen6_encoder_context = calloc(1, sizeof(struct gen6_encoder_context));

    gen6_encoder_context->base.destroy = gen6_encoder_context_destroy;
    gen6_encoder_context->base.run = gen6_encoder_end_picture;
    gen6_vme_context_init(ctx, &gen6_encoder_context->vme_context);
    gen6_mfc_context_init(ctx, &gen6_encoder_context->mfc_context);

    return (struct hw_context *)gen6_encoder_context;
}
