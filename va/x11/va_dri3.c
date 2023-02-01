/*
 * Copyright © 2022 Collabora Ltd.
 * Copyright (c) 2023 Emil Velikov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Soft-
 * ware"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, provided that the above copyright
 * notice(s) and this permission notice appear in all copies of the Soft-
 * ware and that both the above copyright notice(s) and this permission
 * notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
 * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
 * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
 * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
 * MANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization of
 * the copyright holder.
 *
 * Authors:
 *   Emil Velikov (emil.velikov@collabora.com)
 */

#include "sysdeps.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/dri3.h>

#include <X11/Xlib-xcb.h>
#include <xf86drm.h>

#include "va_backend.h"
#include "va_drmcommon.h"
#include "drm/va_drm_utils.h"

static xcb_screen_t *
va_DRI3GetXCBScreen(xcb_connection_t *conn, int screen)
{
    xcb_screen_iterator_t iter;

    iter = xcb_setup_roots_iterator(xcb_get_setup(conn));
    for (; iter.rem; --screen, xcb_screen_next(&iter))
        if (screen == 0)
            return iter.data;
    return NULL;
}

static int
va_isDRI3Connected(VADriverContextP ctx, int *outfd)
{
    xcb_connection_t *conn = XGetXCBConnection(ctx->native_dpy);
    xcb_screen_t *screen;
    xcb_window_t root;
    const xcb_query_extension_reply_t *ext;
    xcb_dri3_open_cookie_t cookie;
    xcb_dri3_open_reply_t *reply;
    int fd;
    char *render_node;

    if (!conn)
        return -1;

    screen = va_DRI3GetXCBScreen(conn, ctx->x11_screen);
    if (!screen)
        return -1;

    root = screen->root;

    xcb_prefetch_extension_data(conn, &xcb_dri3_id);
    ext = xcb_get_extension_data(conn, &xcb_dri3_id);
    if (!ext || !ext->present)
        return -1;

    /* We don't require any of the ancy stuff, so there's no point in checking
     * the version.
     */

    cookie = xcb_dri3_open(conn, root, 0 /* provider */);
    reply = xcb_dri3_open_reply(conn, cookie, NULL /* error */);

    if (!reply || reply->nfd != 1) {
        free(reply);
        return -1;
    }

    fd = xcb_dri3_open_reply_fds(conn, reply)[0];
    free(reply);

    /* The server can give us primary or a render node.
     * In case of the former we need to swap it for the latter.
     */
    switch (drmGetNodeTypeFromFd(fd)) {
    case DRM_NODE_PRIMARY:
        render_node = drmGetRenderDeviceNameFromFd(fd);
        close(fd);
        if (!render_node)
            return -1;

        fd = open(render_node, O_RDWR | O_CLOEXEC);
        free(render_node);
        if (fd == -1)
            return -1;

        break;
    case DRM_NODE_RENDER:
        fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
        break;
    default:
        close(fd);
        return -1;
    }

    *outfd = fd;
    return 0;
}

VAStatus va_DRI3_GetDriverNames(
    VADisplayContextP pDisplayContext,
    char **drivers,
    unsigned *num_drivers
)
{
    VADriverContextP const ctx = pDisplayContext->pDriverContext;
    struct drm_state * const drm_state = ctx->drm_state;
    int fd = -1;

    if (va_isDRI3Connected(ctx, &fd) && fd != -1)
        return VA_STATUS_ERROR_UNKNOWN;

    drm_state->fd = fd;
    drm_state->auth_type = VA_DRM_AUTH_CUSTOM;
    return VA_DRM_GetDriverNames(ctx, drivers, num_drivers);
}
