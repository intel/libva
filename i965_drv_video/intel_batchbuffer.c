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

#include <string.h>
#include <assert.h>

#include "va_backend.h"

#include "intel_batchbuffer.h"

static void 
intel_batchbuffer_reset(struct intel_batchbuffer *batch)
{
    struct intel_driver_data *intel = batch->intel; 

    if (batch->buffer != NULL) {
        dri_bo_unreference(batch->buffer);
        batch->buffer = NULL;
    }

    batch->buffer = dri_bo_alloc(intel->bufmgr, "batch buffer", 
                                 BATCH_SIZE, 0x1000);

    assert(batch->buffer);
    dri_bo_map(batch->buffer, 1);
    batch->map = batch->buffer->virtual;
    batch->size = BATCH_SIZE;
    batch->ptr = batch->map;
    batch->atomic = 0;
}

Bool 
intel_batchbuffer_init(struct intel_driver_data *intel)
{
    intel->batch = calloc(1, sizeof(*(intel->batch)));

    assert(intel->batch);
    intel->batch->intel = intel;
    intel_batchbuffer_reset(intel->batch);

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

    return True;
}

Bool 
intel_batchbuffer_flush(VADriverContextP ctx)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    struct intel_batchbuffer *batch = intel->batch;
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

    dri_bo_exec(batch->buffer, used, 0, 0, 0);

    if (!is_locked)
        intel_unlock_hardware(ctx);

    intel_batchbuffer_reset(intel->batch);

    return True;
}

static unsigned int
intel_batchbuffer_space(struct intel_batchbuffer *batch)
{
    return (batch->size - BATCH_RESERVED) - (batch->ptr - batch->map);
}

void 
intel_batchbuffer_emit_dword(VADriverContextP ctx, unsigned int x)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    struct intel_batchbuffer *batch = intel->batch;

    assert(intel_batchbuffer_space(batch) >= 4);
    *(unsigned int*)batch->ptr = x;
    batch->ptr += 4;
}

void 
intel_batchbuffer_emit_reloc(VADriverContextP ctx, dri_bo *bo, 
                             uint32_t read_domains, uint32_t write_domains, 
                             uint32_t delta)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    struct intel_batchbuffer *batch = intel->batch;

    assert(batch->ptr - batch->map < batch->size);
    dri_bo_emit_reloc(batch->buffer, read_domains, write_domains,
                      delta, batch->ptr - batch->map, bo);
    intel_batchbuffer_emit_dword(ctx, bo->offset + delta);
}

void 
intel_batchbuffer_require_space(VADriverContextP ctx, unsigned int size)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    struct intel_batchbuffer *batch = intel->batch;

    assert(size < batch->size - 8);

    if (intel_batchbuffer_space(batch) < size) {
        intel_batchbuffer_flush(ctx);
    }
}

void 
intel_batchbuffer_data(VADriverContextP ctx, void *data, unsigned int size)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    struct intel_batchbuffer *batch = intel->batch;

    assert((size & 3) == 0);
    intel_batchbuffer_require_space(ctx, size);

    assert(batch->ptr);
    memcpy(batch->ptr, data, size);
    batch->ptr += size;
}

void
intel_batchbuffer_emit_mi_flush(VADriverContextP ctx)
{
    intel_batchbuffer_require_space(ctx, 4);
    intel_batchbuffer_emit_dword(ctx, MI_FLUSH | STATE_INSTRUCTION_CACHE_INVALIDATE);
}

void
intel_batchbuffer_start_atomic(VADriverContextP ctx, unsigned int size)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    struct intel_batchbuffer *batch = intel->batch;

    assert(!batch->atomic);
    intel_batchbuffer_require_space(ctx, size);
    batch->atomic = 1;
}

void
intel_batchbuffer_end_atomic(VADriverContextP ctx)
{
    struct intel_driver_data *intel = intel_driver_data(ctx);
    struct intel_batchbuffer *batch = intel->batch;

    assert(batch->atomic);
    batch->atomic = 0;
}
