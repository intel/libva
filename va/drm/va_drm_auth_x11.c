/*
 * Copyright (c) 2012 Intel Corporation. All Rights Reserved.
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
 */

#define _GNU_SOURCE 1
#include "sysdeps.h"
#include <dlfcn.h>
#include <X11/Xlib.h>
#include "va_drm_auth_x11.h"

#define LIBVA_MAJOR_VERSION 1

typedef struct drm_auth_x11             DRMAuthX11;
typedef struct drm_auth_x11_vtable      DRMAuthX11VTable;

typedef void (*VAGenericFunc)(void);
typedef Display *(*X11OpenDisplayFunc)(const char *display_name);
typedef int (*X11CloseDisplayFunc)(Display *display);
typedef Bool (*VADRI2QueryExtensionFunc)(
    Display *display, int *event_base, int *error_base);
typedef Bool (*VADRI2QueryVersionFunc)(
    Display *display, int *major, int *minor);
typedef Bool (*VADRI2AuthenticateFunc)(
    Display *display, XID window, uint32_t magic);

struct drm_auth_x11_vtable {
    X11OpenDisplayFunc          x11_open_display;
    X11CloseDisplayFunc         x11_close_display;
    VADRI2QueryExtensionFunc    va_dri2_query_extension;
    VADRI2QueryVersionFunc      va_dri2_query_version;
    VADRI2AuthenticateFunc      va_dri2_authenticate;
};

struct drm_auth_x11 {
    void                       *handle; /* libva-x11.so.1 */
    DRMAuthX11VTable            vtable;
    Display                    *display;
    Window                      window;
};

static bool
get_symbol(void *handle, void *func_vptr, const char *name)
{
    VAGenericFunc func, *func_ptr = func_vptr;
    const char *error;

    dlerror();
    func = (VAGenericFunc)dlsym(handle, name);
    error = dlerror();

    if (error) {
        fprintf(stderr, "error: failed to resolve %s() function: %s\n",
                name, error);
        return false;
    }

    *func_ptr = func;
    return true;
}

static bool
drm_auth_x11_init(DRMAuthX11 *auth)
{
    struct drm_auth_x11_vtable *vtable;
    char libva_x11_name[16];
    int ret;

    ret = snprintf(
        libva_x11_name, sizeof(libva_x11_name),
        "libva-x11.so.%d", LIBVA_MAJOR_VERSION
    );
    if (ret < 0 || ret >= sizeof(libva_x11_name))
        return false;

    auth->handle = dlopen(libva_x11_name, RTLD_LAZY | RTLD_GLOBAL);
    if (!auth->handle) {
        perror("open lib");
        return false;
    }

    vtable = &auth->vtable;
    if (!get_symbol(RTLD_DEFAULT, &vtable->x11_open_display, "XOpenDisplay"))
        return false;
    if (!get_symbol(RTLD_DEFAULT, &vtable->x11_close_display, "XCloseDisplay"))
        return false;
    if (!get_symbol(auth->handle, &vtable->va_dri2_query_extension,
                    "VA_DRI2QueryExtension"))
        return false;
    if (!get_symbol(auth->handle, &vtable->va_dri2_query_version,
                    "VA_DRI2QueryVersion"))
        return false;
    if (!get_symbol(auth->handle, &vtable->va_dri2_authenticate,
                    "VA_DRI2Authenticate"))
        return false;

    auth->display = vtable->x11_open_display(NULL);
    if (!auth->display)
        return false;

    auth->window = DefaultRootWindow(auth->display);
    return true;
}

static void
drm_auth_x11_terminate(DRMAuthX11 *auth)
{
    if (!auth)
        return;

    if (auth->display) {
        auth->vtable.x11_close_display(auth->display);
        auth->display = NULL;
        auth->window  = None;
    }

    if (auth->handle) {
        dlclose(auth->handle);
        auth->handle = NULL;
    }
}

static bool
drm_auth_x11_authenticate(DRMAuthX11 *auth, int fd, uint32_t magic)
{
    DRMAuthX11VTable * const vtable = &auth->vtable;
    int evt_base, err_base, v_major, v_minor;

    if (!vtable->va_dri2_query_extension(auth->display, &evt_base, &err_base))
        return false;
    if (!vtable->va_dri2_query_version(auth->display, &v_major, &v_minor))
        return false;
    if (!vtable->va_dri2_authenticate(auth->display, auth->window, magic))
        return false;
    return true;
}

/* Try to authenticate the DRM connection with the supplied magic through X11 */
bool
va_drm_authenticate_x11(int fd, uint32_t magic)
{
    DRMAuthX11 auth;
    bool success = false;

    memset(&auth, 0, sizeof(auth));
    if (!drm_auth_x11_init(&auth))
        goto end;
    success = drm_auth_x11_authenticate(&auth, fd, magic);

end:
    drm_auth_x11_terminate(&auth);
    return success;
}
