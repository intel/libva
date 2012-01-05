#ifndef _VA_DRICOMMON_H_
#define _VA_DRICOMMON_H_

#ifndef ANDROID
#include <X11/Xlib.h>
#include <xf86drm.h>
#include <drm.h>
#include <drm_sarea.h>
#endif

#include <va/va_backend.h>

#ifdef ANDROID
#define XID unsigned int
#define Bool int
#endif

enum
{
    VA_NONE = 0,
    VA_DRI1 = 1,
    VA_DRI2 = 2,
    VA_DUMMY = 3
};

union dri_buffer 
{
    struct {
        unsigned int attachment;
        unsigned int name;
        unsigned int pitch;
        unsigned int cpp;
        unsigned int flags;
    } dri2;
};

struct dri_drawable 
{
    XID x_drawable;
    int is_window;
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    struct dri_drawable *next;
};

#define DRAWABLE_HASH_SZ 32
struct dri_state 
{
    int fd;
    int driConnectedFlag; /* 0: disconnected, 1: DRI, 2: DRI2 */
#ifndef ANDROID
    struct dri_drawable *drawable_hash[DRAWABLE_HASH_SZ];

    struct dri_drawable *(*createDrawable)(VADriverContextP ctx, XID x_drawable);
    void (*destroyDrawable)(VADriverContextP ctx, struct dri_drawable *dri_drawable);
    void (*swapBuffer)(VADriverContextP ctx, struct dri_drawable *dri_drawable);
    union dri_buffer *(*getRenderingBuffer)(VADriverContextP ctx, struct dri_drawable *dri_drawable);
    void (*close)(VADriverContextP ctx);
#endif
};

Bool isDRI2Connected(VADriverContextP ctx, char **driver_name);
void free_drawable(VADriverContextP ctx, struct dri_drawable* dri_drawable);
void free_drawable_hashtable(VADriverContextP ctx);
struct dri_drawable *dri_get_drawable(VADriverContextP ctx, XID drawable);
void dri_swap_buffer(VADriverContextP ctx, struct dri_drawable *dri_drawable);
union dri_buffer *dri_get_rendering_buffer(VADriverContextP ctx, struct dri_drawable *dri_drawable);

#endif /* _VA_DRICOMMON_H_ */
