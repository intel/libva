#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>

#include <xf86drm.h>

#include "X11/Xlib.h"
#include "va.h"
#include "va_backend.h"

#include "va_dri.h"
#include "va_dricommon.h"

struct dri1_drawable 
{
    struct dri_drawable base;
    union dri_buffer buffer;
    int width;
    int height;
};

static struct dri_drawable * 
dri1CreateDrawable(VADriverContextP ctx, XID x_drawable)
{
    struct dri1_drawable *dri1_drawable;

    dri1_drawable = calloc(1, sizeof(*dri1_drawable));

    if (!dri1_drawable)
        return NULL;

    dri1_drawable->base.x_drawable = x_drawable;

    return &dri1_drawable->base;
}

static void 
dri1DestroyDrawable(VADriverContextP ctx, struct dri_drawable *dri_drawable)
{
    free(dri_drawable);
}

static void 
dri1SwapBuffer(VADriverContextP ctx, struct dri_drawable *dri_drawable)
{

}

static union dri_buffer *
dri1GetRenderingBuffer(VADriverContextP ctx, struct dri_drawable *dri_drawable)
{
    struct dri1_drawable *dri1_drawable = (struct dri1_drawable *)dri_drawable;

    return &dri1_drawable->buffer;
}

static void
dri1Close(VADriverContextP ctx)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->drm_state;

    free_drawable_hashtable(ctx);
    VA_DRIDestroyContext(ctx->native_dpy, ctx->x11_screen, dri_state->hwContextID);
    assert(dri_state->pSAREA != MAP_FAILED);
    drmUnmap(dri_state->pSAREA, SAREA_MAX);
    assert(dri_state->base.fd >= 0);
    drmCloseOnce(dri_state->base.fd);
    VA_DRICloseConnection(ctx->native_dpy, ctx->x11_screen);
}

Bool 
isDRI1Connected(VADriverContextP ctx, char **driver_name)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->drm_state;
    int direct_capable;
    int driver_major;
    int driver_minor;
    int driver_patch;
    int newlyopened;
    char *BusID;
    drm_magic_t magic;        

    *driver_name = NULL;
    dri_state->base.fd = -1;
    dri_state->pSAREA = MAP_FAILED;
    dri_state->base.auth_type = VA_NONE;

    if (!VA_DRIQueryDirectRenderingCapable(ctx->native_dpy, 
                                           ctx->x11_screen, 
                                           &direct_capable))
        goto err_out0;

    if (!direct_capable)
        goto err_out0;

    if (!VA_DRIGetClientDriverName(ctx->native_dpy, ctx->x11_screen, 
                                   &driver_major, &driver_minor,
                                   &driver_patch, driver_name))
        goto err_out0;

    if (!VA_DRIOpenConnection(ctx->native_dpy, ctx->x11_screen, 
                              &dri_state->hSAREA, &BusID))
        goto err_out0;

    
    dri_state->base.fd = drmOpenOnce(NULL, BusID, &newlyopened);
    XFree(BusID);

    if (dri_state->base.fd < 0)
        goto err_out1;


    if (drmGetMagic(dri_state->base.fd, &magic))
        goto err_out1;

    if (newlyopened && !VA_DRIAuthConnection(ctx->native_dpy, ctx->x11_screen, magic))
        goto err_out1;

    if (drmMap(dri_state->base.fd, dri_state->hSAREA, SAREA_MAX, &dri_state->pSAREA))
        goto err_out1;

    if (!VA_DRICreateContext(ctx->native_dpy, ctx->x11_screen,
                             DefaultVisual(ctx->native_dpy, ctx->x11_screen),
                             &dri_state->hwContextID, &dri_state->hwContext))
        goto err_out1;

    dri_state->base.auth_type = VA_DRI1;
    dri_state->createDrawable = dri1CreateDrawable;
    dri_state->destroyDrawable = dri1DestroyDrawable;
    dri_state->swapBuffer = dri1SwapBuffer;
    dri_state->getRenderingBuffer = dri1GetRenderingBuffer;
    dri_state->close = dri1Close;

    return True;

err_out1:
    if (dri_state->pSAREA != MAP_FAILED)
        drmUnmap(dri_state->pSAREA, SAREA_MAX);

    if (dri_state->base.fd >= 0)
        drmCloseOnce(dri_state->base.fd);

    VA_DRICloseConnection(ctx->native_dpy, ctx->x11_screen);

err_out0:
    if (*driver_name)
        XFree(*driver_name);

    dri_state->pSAREA = MAP_FAILED;
    dri_state->base.fd = -1;
    *driver_name = NULL;
    
    return False;
}

