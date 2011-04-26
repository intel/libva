#ifndef _INTEL_BATCHBUFFER_H_
#define _INTEL_BATCHBUFFER_H_

#include <xf86drm.h>
#include <drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>

#include "intel_driver.h"

struct intel_batchbuffer 
{
    struct intel_driver_data *intel;
    dri_bo *buffer;
    unsigned int size;
    unsigned char *map;
    unsigned char *ptr;
    int atomic;
    int flag;

    int emit_total;
    unsigned char *emit_start;

    int (*run)(drm_intel_bo *bo, int used,
               drm_clip_rect_t *cliprects, int num_cliprects,
               int DR4, int ring_flag);
};

Bool intel_batchbuffer_init(struct intel_driver_data *intel);
Bool intel_batchbuffer_terminate(struct intel_driver_data *intel);

void intel_batchbuffer_emit_dword(VADriverContextP ctx, unsigned int x);
void intel_batchbuffer_emit_reloc(VADriverContextP ctx, dri_bo *bo, 
                                  uint32_t read_domains, uint32_t write_domains, 
                                  uint32_t delta);
void intel_batchbuffer_require_space(VADriverContextP ctx, unsigned int size);
void intel_batchbuffer_data(VADriverContextP ctx, void *data, unsigned int size);
void intel_batchbuffer_emit_mi_flush(VADriverContextP ctx);
void intel_batchbuffer_start_atomic(VADriverContextP ctx, unsigned int size);
void intel_batchbuffer_end_atomic(VADriverContextP ctx);
Bool intel_batchbuffer_flush(VADriverContextP ctx);

void intel_batchbuffer_begin_batch(VADriverContextP ctx, int total);
void intel_batchbuffer_advance_batch(VADriverContextP ctx);

void intel_batchbuffer_emit_dword_bcs(VADriverContextP ctx, unsigned int x);
void intel_batchbuffer_emit_reloc_bcs(VADriverContextP ctx, dri_bo *bo, 
                                      uint32_t read_domains, uint32_t write_domains, 
                                      uint32_t delta);
void intel_batchbuffer_require_space_bcs(VADriverContextP ctx, unsigned int size);
void intel_batchbuffer_data_bcs(VADriverContextP ctx, void *data, unsigned int size);
void intel_batchbuffer_emit_mi_flush_bcs(VADriverContextP ctx);
void intel_batchbuffer_start_atomic_bcs(VADriverContextP ctx, unsigned int size);
void intel_batchbuffer_end_atomic_bcs(VADriverContextP ctx);
Bool intel_batchbuffer_flush_bcs(VADriverContextP ctx);

void intel_batchbuffer_begin_batch_bcs(VADriverContextP ctx, int total);
void intel_batchbuffer_advance_batch_bcs(VADriverContextP ctx);

void intel_batchbuffer_check_batchbuffer_flag(VADriverContextP ctx, int flag);

int intel_batchbuffer_check_free_space(VADriverContextP ctx, int size);
int intel_batchbuffer_check_free_space_bcs(VADriverContextP ctx, int size);

#define __BEGIN_BATCH(ctx, n, flag) do {                        \
        intel_batchbuffer_check_batchbuffer_flag(ctx, flag);    \
        intel_batchbuffer_require_space(ctx, (n) * 4);          \
        intel_batchbuffer_begin_batch(ctx, (n));                \
    } while (0)

#define BEGIN_BATCH(ctx, n)             __BEGIN_BATCH(ctx, n, I915_EXEC_RENDER)
#define BEGIN_BLT_BATCH(ctx, n)         __BEGIN_BATCH(ctx, n, I915_EXEC_BLT)

#define OUT_BATCH(ctx, d) do {                                  \
   intel_batchbuffer_emit_dword(ctx, d);                        \
} while (0)

#define OUT_RELOC(ctx, bo, read_domains, write_domain, delta) do {      \
   assert((delta) >= 0);                                                \
   intel_batchbuffer_emit_reloc(ctx, bo,                                \
                                read_domains, write_domain, delta);     \
} while (0)

#define ADVANCE_BATCH(ctx) do {                                         \
    intel_batchbuffer_advance_batch(ctx);                               \
} while (0)

#define BEGIN_BCS_BATCH(ctx, n) do {                                    \
   intel_batchbuffer_require_space_bcs(ctx, (n) * 4);                   \
   intel_batchbuffer_begin_batch_bcs(ctx, (n));                         \
} while (0)

#define OUT_BCS_BATCH(ctx, d) do {                                      \
   intel_batchbuffer_emit_dword_bcs(ctx, d);                            \
} while (0)

#define OUT_BCS_RELOC(ctx, bo, read_domains, write_domain, delta) do {  \
   assert((delta) >= 0);                                                \
   intel_batchbuffer_emit_reloc_bcs(ctx, bo,                            \
                                    read_domains, write_domain, delta); \
} while (0)

#define ADVANCE_BCS_BATCH(ctx) do {                                     \
    intel_batchbuffer_advance_batch_bcs(ctx);                           \
} while (0)

#endif /* _INTEL_BATCHBUFFER_H_ */
