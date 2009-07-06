#include "va_dricommon.h"

static struct dri_drawable *                                                                                                                                                                                   
do_drawable_hash(VADriverContextP ctx, XID drawable)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->dri_state;
    int index = drawable % DRAWABLE_HASH_SZ;
    struct dri_drawable *dri_drawable = dri_state->drawable_hash[index];

    while (dri_drawable) {
        if (dri_drawable->x_drawable == drawable)
            return dri_drawable;
        dri_drawable = dri_drawable->next;
    }

    dri_drawable = dri_state->createDrawable(ctx, drawable);
    dri_drawable->x_drawable = drawable;
    dri_drawable->next = dri_state->drawable_hash[index];
    dri_state->drawable_hash[index] = dri_drawable;

    return dri_drawable;
}

void
free_drawable_hashtable(VADriverContextP ctx)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->dri_state;
    int i;
    struct dri_drawable *dri_drawable, *prev;

    for (i = 0; i < DRAWABLE_HASH_SZ; i++) {
        dri_drawable = dri_state->drawable_hash[i];

        while (dri_drawable) {
            prev = dri_drawable;
            dri_drawable = prev->next;
            dri_state->destroyDrawable(ctx, prev);
        }
    }
}

struct dri_drawable *
dri_get_drawable(VADriverContextP ctx, XID drawable)
{
    return do_drawable_hash(ctx, drawable);
}

void 
dri_swap_buffer(VADriverContextP ctx, struct dri_drawable *dri_drawable)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->dri_state;

    dri_state->swapBuffer(ctx, dri_drawable);
}

union dri_buffer *
dri_get_rendering_buffer(VADriverContextP ctx, struct dri_drawable *dri_drawable)
{
    struct dri_state *dri_state = (struct dri_state *)ctx->dri_state;
    
    return dri_state->getRenderingBuffer(ctx, dri_drawable);
}
