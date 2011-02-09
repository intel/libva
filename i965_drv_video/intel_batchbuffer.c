/**************************************************************************                                                                                  
 *                                                                                                                                                           
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.                                                                                                
 * All Rights Reserved.                                                                                                                                      
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR                                                                                    
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,                                                                                  
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE                                                                                         
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                                                                                    
 *                                                                                                                                                           
 **************************************************************************/      

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <va/va_backend.h>

#include "intel_batchbuffer.h"

static void 
intel_batchbuffer_reset(struct intel_batchbuffer *batch)
{
    struct intel_driver_data *intel = batch->intel; 
    int batch_size = BATCH_SIZE;

    assert(batch->flag == I915_EXEC_RENDER ||
           batch->flag == I915_EXEC_BLT ||
           batch->flag == I915_EXEC_BSD);

    dri_bo_unreference(batch->buffer);
    batch->buffer = dri_bo_alloc(intel->bufmgr, 
                                 batch->flag == I915_EXEC_RENDER ? "render batch buffer" : "bsd batch buffer", 
                                 batch_size,
                                 0x1000);
    assert(batch->buffer);
    dri_bo_map(batch->buffer, 1);
    assert(batch->buffer->virtual);
    batch->map = batch->buffer->virtual;
    batch->size = batch_size;
    batch->ptr = batch->map;
    batch->atomic = 0;
}

Bool 
intel_batchbuffer_init(struct intel_driver_data *intel)
{
    intel->batch = calloc(1, sizeof(*(intel->batch)));
    assert(intel->batch);
    intel->batch->intel = intel;
    intel->batch->flag = I915_EXEC_RENDER;
    intel->batch->run = drm_intel_bo_mrb_exec;
    intel_batchbuffer_reset(intel->batch);

    if (intel->has_bsd) {
        intel->batch_bcs = calloc(1, sizeof(*(intel->batch_bcs)));
        assert(intel->batch_bcs);
        intel->batch_bcs->intel = intel;
        intel->batch_bcs->flag = I915_EXEC_BSD;
        intel->batch_bcs->run = drm_intel_bo_mrb_exec;
        intel_batchbuffer_reset(intel->batch_bcs);
    }

    return True;
}

Bool
intel_batchbuffer_terminate(struct intel_driver_data *intel)
{
    if (intel->batch) {
        if (intel->batch->map) {
            dri_bo_unmap(intel->batch->buffer);
            intel->batch->map = NULL;
        }

        dri_bo_unreference(intel->batch->buffer);
        free(intel->batch);
        intel->batch = NULL;
    }

    if (intel->batch_bcs) {
        if (intel->batch_bcs->map) {
            dri_bo_unmap(intel->batch_bcs->buffer);
            intel->batch_bcs->map = NULL;
        }

        dri_bo_unreference(intel->batch_bcs->buffer);
        free(intel->batch_bcs);
        intel->batch_bcs = NULL;
    }

    return True;
}

static Bool
intel_batchbuffer_flush_helper(VADriverContextP ctx,
                               struct intel_batchbuffer *batch)
{
    struct intel_driver_data *intel = batch->intel;
    unsigned int used = batch->ptr - batch->map;
    int is_locked = intel->locked;

    if (used == 0) {
        return True;
    }

    if ((used & 4) == 0) {
        *(unsigned int*)batch->ptr = 0;
        batch->ptr += 4;
    }

    *(unsigned int*)batch->ptr = MI_BATCH_BUFFER_END;
    batch->ptr += 4;
    dri_bo_unmap(batch->buffer);
    used = batch->ptr - batch->map;

    if (!is_locked)
        intel_lock_hardware(ctx);

    batch->run(batch->buffer, used, 0, 0, 0, batch->flag);

    if (!is_locked)
        intel_unlock_hardware(ctx);

    intel_batchbuffer_reset(batch);

    return True;
}

Bool 
intel_batchbuffer_flush(VADriverContextP ctx)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    
    return intel_batchbuffer_flush_helper(ctx, intel->batch);
}

Bool 
intel_batchbuffer_flush_bcs(VADriverContextP ctx)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    
    return intel_batchbuffer_flush_helper(ctx, intel->batch_bcs);
}

static unsigned int
intel_batchbuffer_space_helper(struct intel_batchbuffer *batch)
{
    return (batch->size - BATCH_RESERVED) - (batch->ptr - batch->map);
}

static void
intel_batchbuffer_emit_dword_helper(struct intel_batchbuffer *batch, 
                                    unsigned int x)
{
    assert(intel_batchbuffer_space_helper(batch) >= 4);
    *(unsigned int *)batch->ptr = x;
    batch->ptr += 4;
}

void 
intel_batchbuffer_emit_dword(VADriverContextP ctx, unsigned int x)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    intel_batchbuffer_emit_dword_helper(intel->batch, x);
}

void 
intel_batchbuffer_emit_dword_bcs(VADriverContextP ctx, unsigned int x)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    intel_batchbuffer_emit_dword_helper(intel->batch_bcs, x);
}

static void 
intel_batchbuffer_emit_reloc_helper(VADriverContextP ctx, 
                                    struct intel_batchbuffer *batch,
                                    dri_bo *bo, 
                                    uint32_t read_domains, uint32_t write_domains, 
                                    uint32_t delta)
{
    assert(batch->ptr - batch->map < batch->size);
    dri_bo_emit_reloc(batch->buffer, read_domains, write_domains,
                      delta, batch->ptr - batch->map, bo);
    intel_batchbuffer_emit_dword_helper(batch, bo->offset + delta);
}

void 
intel_batchbuffer_emit_reloc(VADriverContextP ctx, dri_bo *bo, 
                             uint32_t read_domains, uint32_t write_domains, 
                             uint32_t delta)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    intel_batchbuffer_emit_reloc_helper(ctx, intel->batch,
                                        bo, read_domains, write_domains,
                                        delta);
}

void 
intel_batchbuffer_emit_reloc_bcs(VADriverContextP ctx, dri_bo *bo, 
                                 uint32_t read_domains, uint32_t write_domains, 
                                 uint32_t delta)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    intel_batchbuffer_emit_reloc_helper(ctx, intel->batch_bcs,
                                        bo, read_domains, write_domains,
                                        delta);
}

static void 
intel_batchbuffer_require_space_helper(VADriverContextP ctx, 
                                struct intel_batchbuffer *batch,
                                unsigned int size)
{
    assert(size < batch->size - 8);

    if (intel_batchbuffer_space_helper(batch) < size) {
        intel_batchbuffer_flush_helper(ctx, batch);
    }
}

void 
intel_batchbuffer_require_space(VADriverContextP ctx, unsigned int size)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    intel_batchbuffer_require_space_helper(ctx, intel->batch, size);
}

void 
intel_batchbuffer_require_space_bcs(VADriverContextP ctx, unsigned int size)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    intel_batchbuffer_require_space_helper(ctx, intel->batch_bcs, size);
}

static void 
intel_batchbuffer_data_helper(VADriverContextP ctx, 
                              struct intel_batchbuffer *batch,
                              void *data,
                              unsigned int size)
{
    assert((size & 3) == 0);
    intel_batchbuffer_require_space_helper(ctx, batch, size);

    assert(batch->ptr);
    memcpy(batch->ptr, data, size);
    batch->ptr += size;
}

void 
intel_batchbuffer_data(VADriverContextP ctx, void *data, unsigned int size)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    intel_batchbuffer_data_helper(ctx, intel->batch, data, size);
}

void 
intel_batchbuffer_data_bcs(VADriverContextP ctx, void *data, unsigned int size)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    intel_batchbuffer_data_helper(ctx, intel->batch_bcs, data, size);
}

void
intel_batchbuffer_emit_mi_flush(VADriverContextP ctx)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    if (intel->batch->flag == I915_EXEC_BLT) {
        BEGIN_BLT_BATCH(ctx, 4);
        OUT_BATCH(ctx, MI_FLUSH_DW);
        OUT_BATCH(ctx, 0);
        OUT_BATCH(ctx, 0);
        OUT_BATCH(ctx, 0);
        ADVANCE_BATCH(ctx);
    } else if (intel->batch->flag == I915_EXEC_RENDER) {
        BEGIN_BATCH(ctx, 1);
        OUT_BATCH(ctx, MI_FLUSH | MI_FLUSH_STATE_INSTRUCTION_CACHE_INVALIDATE);
        ADVANCE_BATCH(ctx);
    }
}

void
intel_batchbuffer_emit_mi_flush_bcs(VADriverContextP ctx)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    if (IS_GEN6(intel->device_id)) {
        BEGIN_BCS_BATCH(ctx, 4);
        OUT_BCS_BATCH(ctx, MI_FLUSH_DW | MI_FLUSH_DW_VIDEO_PIPELINE_CACHE_INVALIDATE);
        OUT_BCS_BATCH(ctx, 0);
        OUT_BCS_BATCH(ctx, 0);
        OUT_BCS_BATCH(ctx, 0);
        ADVANCE_BCS_BATCH(ctx);
    } else {
        BEGIN_BCS_BATCH(ctx, 1);
        OUT_BCS_BATCH(ctx, MI_FLUSH | MI_FLUSH_STATE_INSTRUCTION_CACHE_INVALIDATE);
        ADVANCE_BCS_BATCH(ctx);
    }
}

void
intel_batchbuffer_start_atomic_helper(VADriverContextP ctx, 
                                      struct intel_batchbuffer *batch,
                                      unsigned int size)
{
    assert(!batch->atomic);
    intel_batchbuffer_require_space_helper(ctx, batch, size);
    batch->atomic = 1;
}

void
intel_batchbuffer_start_atomic(VADriverContextP ctx, unsigned int size)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    intel_batchbuffer_check_batchbuffer_flag(ctx, I915_EXEC_RENDER);
    intel_batchbuffer_start_atomic_helper(ctx, intel->batch, size);
}

void
intel_batchbuffer_start_atomic_bcs(VADriverContextP ctx, unsigned int size)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    intel_batchbuffer_start_atomic_helper(ctx, intel->batch_bcs, size);
}

void
intel_batchbuffer_end_atomic_helper(struct intel_batchbuffer *batch)
{
    assert(batch->atomic);
    batch->atomic = 0;
}

void
intel_batchbuffer_end_atomic(VADriverContextP ctx)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    intel_batchbuffer_end_atomic_helper(intel->batch);
}

void
intel_batchbuffer_end_atomic_bcs(VADriverContextP ctx)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    intel_batchbuffer_end_atomic_helper(intel->batch_bcs);
}

static void
intel_batchbuffer_begin_batch_helper(struct intel_batchbuffer *batch, int total)
{
    batch->emit_total = total * 4;
    batch->emit_start = batch->ptr;
}

void
intel_batchbuffer_begin_batch(VADriverContextP ctx, int total)
{
   struct intel_driver_data *intel = intel_driver_data(ctx);

   intel_batchbuffer_begin_batch_helper(intel->batch, total);
}

void
intel_batchbuffer_begin_batch_bcs(VADriverContextP ctx, int total)
{
   struct intel_driver_data *intel = intel_driver_data(ctx);

   intel_batchbuffer_begin_batch_helper(intel->batch_bcs, total);
}

static void
intel_batchbuffer_advance_batch_helper(struct intel_batchbuffer *batch)
{
    assert(batch->emit_total == (batch->ptr - batch->emit_start));
}

void
intel_batchbuffer_advance_batch(VADriverContextP ctx)
{
   struct intel_driver_data *intel = intel_driver_data(ctx);

   intel_batchbuffer_advance_batch_helper(intel->batch);
}

void
intel_batchbuffer_advance_batch_bcs(VADriverContextP ctx)
{
   struct intel_driver_data *intel = intel_driver_data(ctx);

   intel_batchbuffer_advance_batch_helper(intel->batch_bcs);
}

void
intel_batchbuffer_check_batchbuffer_flag(VADriverContextP ctx, int flag)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);

    if (flag != I915_EXEC_RENDER &&
        flag != I915_EXEC_BLT &&
        flag != I915_EXEC_BSD)
        return;

    if (intel->batch->flag == flag)
        return;

    intel_batchbuffer_flush_helper(ctx, intel->batch);
    intel->batch->flag = flag;
}
