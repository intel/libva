/*
 * Copyright (c) 2008 NVIDIA, Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _GNU_SOURCE 1
#include <string.h>

#define NEED_REPLIES
#include <stdlib.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xext.h>
#include <X11/extensions/extutil.h>
#include "va_nvctrl.h"

#define NV_CONTROL_ERRORS 0
#define NV_CONTROL_EVENTS 5
#define NV_CONTROL_NAME "NV-CONTROL"

#define NV_CTRL_TARGET_TYPE_X_SCREEN   0
#define NV_CTRL_TARGET_TYPE_GPU        1
#define NV_CTRL_TARGET_TYPE_FRAMELOCK  2
#define NV_CTRL_TARGET_TYPE_VCSC       3 /* Visual Computing System */

#define NV_CTRL_STRING_NVIDIA_DRIVER_VERSION                    3  /* R--G */

#define X_nvCtrlQueryExtension                      0
#define X_nvCtrlIsNv                                1
#define X_nvCtrlQueryStringAttribute                4

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
} xnvCtrlQueryExtensionReq;
#define sz_xnvCtrlQueryExtensionReq 4

typedef struct {
    BYTE type;   /* X_Reply */
    CARD8 padb1;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD16 major B16;
    CARD16 minor B16;
    CARD32 padl4 B32;
    CARD32 padl5 B32;
    CARD32 padl6 B32;
    CARD32 padl7 B32;
    CARD32 padl8 B32;
} xnvCtrlQueryExtensionReply;
#define sz_xnvCtrlQueryExtensionReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
} xnvCtrlIsNvReq;
#define sz_xnvCtrlIsNvReq 8

typedef struct {
    BYTE type;   /* X_Reply */
    CARD8 padb1;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 isnv B32;
    CARD32 padl4 B32;
    CARD32 padl5 B32;
    CARD32 padl6 B32;
    CARD32 padl7 B32;
    CARD32 padl8 B32;
} xnvCtrlIsNvReply;
#define sz_xnvCtrlIsNvReply 32

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD16 target_id B16;    /* X screen number or GPU number */
    CARD16 target_type B16;  /* X screen or GPU */
    CARD32 display_mask B32;
    CARD32 attribute B32;
} xnvCtrlQueryStringAttributeReq;
#define sz_xnvCtrlQueryStringAttributeReq 16

typedef struct {
    BYTE type;
    BYTE pad0;
    CARD16 sequenceNumber B16;
    CARD32 length B32;
    CARD32 flags B32;
    CARD32 n B32;    /* Length of string */
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
} xnvCtrlQueryStringAttributeReply;
#define sz_xnvCtrlQueryStringAttributeReply 32

#define NVCTRL_EXT_NEED_CHECK          (XPointer)(~0)
#define NVCTRL_EXT_NEED_NOTHING        (XPointer)(0)
#define NVCTRL_EXT_NEED_TARGET_SWAP    (XPointer)(1)

static XExtensionInfo _nvctrl_ext_info_data;
static XExtensionInfo *nvctrl_ext_info = &_nvctrl_ext_info_data;
static /* const */ char *nvctrl_extension_name = NV_CONTROL_NAME;

#define XNVCTRLCheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, nvctrl_extension_name, val)
#define XNVCTRLSimpleCheckExtension(dpy,i) \
  XextSimpleCheckExtension (dpy, i, nvctrl_extension_name)

static int close_display();
static /* const */ XExtensionHooks nvctrl_extension_hooks = {
    NULL,                               /* create_gc */
    NULL,                               /* copy_gc */
    NULL,                               /* flush_gc */
    NULL,                               /* free_gc */
    NULL,                               /* create_font */
    NULL,                               /* free_font */
    close_display,                      /* close_display */
    NULL,                               /* wire_to_event */
    NULL,                               /* event_to_wire */
    NULL,                               /* error */
    NULL,                               /* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY (find_display, nvctrl_ext_info,
                                   nvctrl_extension_name, 
                                   &nvctrl_extension_hooks,
                                   NV_CONTROL_EVENTS, NVCTRL_EXT_NEED_CHECK)

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, nvctrl_ext_info)

static Bool XNVCTRLQueryVersion (Display *dpy, int *major, int *minor);

/*
 * NV-CONTROL versions 1.8 and 1.9 pack the target_type and target_id
 * fields in reversed order.  In order to talk to one of these servers,
 * we need to swap these fields.
 */
static void XNVCTRLCheckTargetData(Display *dpy, XExtDisplayInfo *info,
                                   int *target_type, int *target_id)
{
    /* Find out what the server's NV-CONTROL version is and
     * setup for swapping if we need to.
     */
    if (info->data == NVCTRL_EXT_NEED_CHECK) {
        int major, minor;

        if (XNVCTRLQueryVersion(dpy, &major, &minor)) {
            if (major == 1 &&
                (minor == 8 || minor == 9)) {
                info->data = NVCTRL_EXT_NEED_TARGET_SWAP;
            } else {
                info->data = NVCTRL_EXT_NEED_NOTHING;
            }
        } else {
            info->data = NVCTRL_EXT_NEED_NOTHING;
        }
    }

    /* We need to swap the target_type and target_id */
    if (info->data == NVCTRL_EXT_NEED_TARGET_SWAP) {
        int tmp;
        tmp = *target_type;
        *target_type = *target_id;
        *target_id = tmp;
    }
}


static Bool XNVCTRLQueryExtension (
    Display *dpy,
    int *event_basep,
    int *error_basep
){
    XExtDisplayInfo *info = find_display (dpy);

    if (XextHasExtension(info)) {
        if (event_basep) *event_basep = info->codes->first_event;
        if (error_basep) *error_basep = info->codes->first_error;
        return True;
    } else {
        return False;
    }
}


static Bool XNVCTRLQueryVersion (
    Display *dpy,
    int *major,
    int *minor
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryExtensionReply rep;
    xnvCtrlQueryExtensionReq   *req;

    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlQueryExtension, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryExtension;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    if (major) *major = rep.major;
    if (minor) *minor = rep.minor;
    UnlockDisplay (dpy);
    SyncHandle ();
    return True;
}


static Bool XNVCTRLIsNvScreen (
    Display *dpy,
    int screen
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlIsNvReply rep;
    xnvCtrlIsNvReq   *req;
    Bool isnv;

    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (nvCtrlIsNv, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlIsNv;
    req->screen = screen;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    isnv = rep.isnv;
    UnlockDisplay (dpy);
    SyncHandle ();
    return isnv;
}


static Bool XNVCTRLQueryTargetStringAttribute (
    Display *dpy,
    int target_type,
    int target_id,
    unsigned int display_mask,
    unsigned int attribute,
    char **ptr
){
    XExtDisplayInfo *info = find_display (dpy);
    xnvCtrlQueryStringAttributeReply rep;
    xnvCtrlQueryStringAttributeReq   *req;
    Bool exists;
    int length, numbytes, slop;

    if (!ptr) return False;

    if(!XextHasExtension(info))
        return False;

    XNVCTRLCheckExtension (dpy, info, False);
    XNVCTRLCheckTargetData(dpy, info, &target_type, &target_id);

    LockDisplay (dpy);
    GetReq (nvCtrlQueryStringAttribute, req);
    req->reqType = info->codes->major_opcode;
    req->nvReqType = X_nvCtrlQueryStringAttribute;
    req->target_type = target_type;
    req->target_id = target_id;
    req->display_mask = display_mask;
    req->attribute = attribute;
    if (!_XReply (dpy, (xReply *) &rep, 0, False)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    }
    length = rep.length;
    numbytes = rep.n;
    slop = numbytes & 3;
    *ptr = (char *) Xmalloc(numbytes);
    if (! *ptr) {
        _XEatData(dpy, length);
        UnlockDisplay (dpy);
        SyncHandle ();
        return False;
    } else {
        _XRead(dpy, (char *) *ptr, numbytes);
        if (slop) _XEatData(dpy, 4-slop);
    }
    exists = rep.flags;
    UnlockDisplay (dpy);
    SyncHandle ();
    return exists;
}

static Bool XNVCTRLQueryStringAttribute (
    Display *dpy,
    int screen,
    unsigned int display_mask,
    unsigned int attribute,
    char **ptr
){
    return XNVCTRLQueryTargetStringAttribute(dpy, NV_CTRL_TARGET_TYPE_X_SCREEN,
                                             screen, display_mask,
                                             attribute, ptr);
}


Bool VA_NVCTRLQueryDirectRenderingCapable( Display *dpy, int screen,
    Bool *isCapable )
{
    int event_base;
    int error_base;

    if (isCapable)
        *isCapable = False;

    if (!XNVCTRLQueryExtension(dpy, &event_base, &error_base))
        return False;

    if (isCapable && XNVCTRLIsNvScreen(dpy, screen))
        *isCapable = True;

    return True;
}

Bool VA_NVCTRLGetClientDriverName( Display *dpy, int screen,
    int *ddxDriverMajorVersion, int *ddxDriverMinorVersion,
    int *ddxDriverPatchVersion, char **clientDriverName )
{
    if (ddxDriverMajorVersion)
        *ddxDriverMajorVersion = 0;
    if (ddxDriverMinorVersion)
        *ddxDriverMinorVersion = 0;
    if (ddxDriverPatchVersion)
        *ddxDriverPatchVersion = 0;
    if (clientDriverName)
        *clientDriverName = NULL;

    char *nvidia_driver_version = NULL;
    if (!XNVCTRLQueryStringAttribute(dpy, screen, 0, NV_CTRL_STRING_NVIDIA_DRIVER_VERSION, &nvidia_driver_version))
        return False;

    char *end, *str = nvidia_driver_version;
    unsigned long v = strtoul(str, &end, 10);
    if (end && end != str) {
        if (ddxDriverMajorVersion)
            *ddxDriverMajorVersion = v;
        if (*(str = end) == '.') {
            v = strtoul(str + 1, &end, 10);
            if (end && end != str && (*end == '.' || *end == '\0')) {
                if (ddxDriverMinorVersion)
                    *ddxDriverMinorVersion = v;
                if (*(str = end) == '.') {
                    v = strtoul(str + 1, &end, 10);
                    if (end && end != str && *end == '\0') {
                        if (ddxDriverPatchVersion)
                            *ddxDriverPatchVersion = v;
                    }
                }
            }
        }
    }
    Xfree(nvidia_driver_version);

    if (clientDriverName)
        *clientDriverName = strdup("nvidia");

    return True;
}
