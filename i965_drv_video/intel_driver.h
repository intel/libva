#ifndef _INTEL_DRIVER_H_
#define _INTEL_DRIVER_H_

#include <pthread.h>
#include <signal.h>

#include <xf86drm.h>
#include <drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>

#include <va/va_backend.h>

#if defined(__GNUC__)
#define INLINE __inline__
#else
#define INLINE
#endif

#define BATCH_SIZE      0x10000
#define BATCH_RESERVED  0x10

#define CMD_MI                                  (0x0 << 29)
#define CMD_2D                                  (0x2 << 29)

#define MI_NOOP                                 (CMD_MI | 0)

#define MI_BATCH_BUFFER_END                     (CMD_MI | (0xA << 23))
#define MI_BATCH_BUFFER_START                   (CMD_MI | (0x31 << 23))

#define MI_FLUSH                                (CMD_MI | (0x4 << 23))
#define STATE_INSTRUCTION_CACHE_INVALIDATE      (0x1 << 0)

#define XY_COLOR_BLT_CMD                        (CMD_2D | (0x50 << 22) | 0x04)
#define XY_COLOR_BLT_WRITE_ALPHA                (1 << 21)
#define XY_COLOR_BLT_WRITE_RGB                  (1 << 20)
#define XY_COLOR_BLT_DST_TILED                  (1 << 11)

/* BR13 */
#define BR13_565                                (0x1 << 24)
#define BR13_8888                               (0x3 << 24)

struct intel_batchbuffer;

#define ALIGN(i, n)    (((i) + (n) - 1) & ~((n) - 1))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

#define SET_BLOCKED_SIGSET()   do {     \
        sigset_t bl_mask;               \
        sigfillset(&bl_mask);           \
        sigdelset(&bl_mask, SIGFPE);    \
        sigdelset(&bl_mask, SIGILL);    \
        sigdelset(&bl_mask, SIGSEGV);   \
        sigdelset(&bl_mask, SIGBUS);    \
        sigdelset(&bl_mask, SIGKILL);   \
        pthread_sigmask(SIG_SETMASK, &bl_mask, &intel->sa_mask); \
    } while (0)

#define RESTORE_BLOCKED_SIGSET() do {    \
        pthread_sigmask(SIG_SETMASK, &intel->sa_mask, NULL); \
    } while (0)

#define PPTHREAD_MUTEX_LOCK() do {             \
        SET_BLOCKED_SIGSET();                  \
        pthread_mutex_lock(&intel->ctxmutex);       \
    } while (0)

#define PPTHREAD_MUTEX_UNLOCK() do {           \
        pthread_mutex_unlock(&intel->ctxmutex);     \
        RESTORE_BLOCKED_SIGSET();              \
    } while (0)

struct intel_driver_data 
{
    int fd;
    int device_id;

    int dri2Enabled;
    drm_context_t hHWContext;
    drm_i915_sarea_t *pPrivSarea;
    drmLock *driHwLock;

    sigset_t sa_mask;
    pthread_mutex_t ctxmutex;
    int locked;

    struct intel_batchbuffer *batch;
    struct intel_batchbuffer *batch_bcs;
    dri_bufmgr *bufmgr;
};

Bool intel_driver_init(VADriverContextP ctx);
Bool intel_driver_terminate(VADriverContextP ctx);
void intel_lock_hardware(VADriverContextP ctx);
void intel_unlock_hardware(VADriverContextP ctx);

static INLINE struct intel_driver_data *
intel_driver_data(VADriverContextP ctx)
{
    return (struct intel_driver_data *)ctx->pDriverData;
}

struct intel_region
{
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    unsigned int cpp;
    unsigned int pitch;
    unsigned int tiling;
    unsigned int swizzle;
    dri_bo *bo;
};

#define PCI_CHIP_GM45_GM                0x2A42
#define PCI_CHIP_IGD_E_G                0x2E02
#define PCI_CHIP_Q45_G                  0x2E12
#define PCI_CHIP_G45_G                  0x2E22
#define PCI_CHIP_G41_G                  0x2E32

#define PCI_CHIP_IRONLAKE_D_G           0x0042
#define PCI_CHIP_IRONLAKE_M_G           0x0046

#define IS_G45(devid)           (devid == PCI_CHIP_IGD_E_G || \
                                 devid == PCI_CHIP_Q45_G || \
                                 devid == PCI_CHIP_G45_G || \
                                 devid == PCI_CHIP_G41_G)
#define IS_GM45(devid)          (devid == PCI_CHIP_GM45_GM)
#define IS_G4X(devid)		(IS_G45(devid) || IS_GM45(devid))

#define IS_IRONLAKE_D(devid)    (devid == PCI_CHIP_IRONLAKE_D_G)
#define IS_IRONLAKE_M(devid)    (devid == PCI_CHIP_IRONLAKE_M_G)
#define IS_IRONLAKE(devid)      (IS_IRONLAKE_D(devid) || IS_IRONLAKE_M(devid))

#endif /* _INTEL_DRIVER_H_ */
