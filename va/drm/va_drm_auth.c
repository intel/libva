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

#include "sysdeps.h"
#include <unistd.h>
#include <xf86drm.h>
#include "va_drm_auth.h"
#include "va_drm_auth_x11.h"

#if defined __linux__
# include <sys/syscall.h>
#endif

/* Checks whether the thread id is the current thread */
static bool
is_local_tid(pid_t tid)
{
#if defined __linux__
    /* On Linux systems, drmGetClient() would return the thread ID
       instead of the actual process ID */
    return syscall(SYS_gettid) == tid;
#else
    return false;
#endif
}

/* Checks whether DRM connection is authenticated */
bool
va_drm_is_authenticated(int fd)
{
    pid_t client_pid;
    int i, auth, pid, uid;
    unsigned long magic, iocs;
    bool is_authenticated = false;

    client_pid = getpid();
    for (i = 0; !is_authenticated; i++) {
        if (drmGetClient(fd, i, &auth, &pid, &uid, &magic, &iocs) != 0)
            break;
        is_authenticated = auth && (pid == client_pid || is_local_tid(pid));
    }
    return is_authenticated;
}

/* Try to authenticate the DRM connection with the supplied magic id */
bool
va_drm_authenticate(int fd, uint32_t magic)
{
    /* XXX: try to authenticate through Wayland, etc. */
#ifdef HAVE_VA_X11
    if (va_drm_authenticate_x11(fd, magic))
        return true;
#endif

    /* Default: root + master privs are needed for the following call */
    return drmAuthMagic(fd, magic) == 0;
}
