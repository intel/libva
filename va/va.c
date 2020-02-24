/*
 * Copyright (c) 2007 Intel Corporation. All Rights Reserved.
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
#include "va.h"
#include "va_backend.h"
#include "va_backend_prot.h"
#include "va_backend_vpp.h"
#include "va_internal.h"
#include "va_trace.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include "compat_win32.h"
#define DRIVER_EXTENSION    "_drv_video.dll"
#define DRIVER_PATH_STRING  "%s\\%s%s"
#define ENV_VAR_SEPARATOR ";"
#else
#include <dlfcn.h>
#include <unistd.h>
#define DRIVER_EXTENSION    "_drv_video.so"
#define DRIVER_PATH_STRING  "%s/%s%s"
#define ENV_VAR_SEPARATOR ":"
#endif
#ifdef ANDROID
#include <log/log.h>
#endif

#define ASSERT      assert
#define CHECK_VTABLE(s, ctx, func) if (!va_checkVtable(dpy, ctx->vtable->va##func, #func)) s = VA_STATUS_ERROR_UNIMPLEMENTED;
#define CHECK_MAXIMUM(s, ctx, var) if (!va_checkMaximum(dpy, ctx->max_##var, #var)) s = VA_STATUS_ERROR_UNKNOWN;
#define CHECK_STRING(s, ctx, var) if (!va_checkString(dpy, ctx->str_##var, #var)) s = VA_STATUS_ERROR_UNKNOWN;

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifndef HAVE_SECURE_GETENV
static char * secure_getenv(const char *name)
{
#if defined(__MINGW32__) || defined(__MINGW64__)
    if (getuid() == geteuid())
#else
    if (getuid() == geteuid() && getgid() == getegid())
#endif
        return getenv(name);
    else
        return NULL;
}
#endif

/*
 * read a config "env" for libva.conf or from environment setting
 * libva.conf has higher priority
 * return 0: the "env" is set, and the value is copied into env_value
 *        1: the env is not set
 */
int va_parseConfig(char *env, char *env_value)
{
    char *token, *value, *saveptr;
    char oneline[1024];
    FILE *fp = NULL;

    if (env == NULL)
        return 1;

    fp = fopen(SYSCONFDIR "/libva.conf", "r");
    while (fp && (fgets(oneline, 1024, fp) != NULL)) {
        if (strlen(oneline) == 1)
            continue;
        token = strtok_r(oneline, "=\n", &saveptr);
        value = strtok_r(NULL, "=\n", &saveptr);

        if (NULL == token || NULL == value)
            continue;

        if (strcmp(token, env) == 0) {
            if (env_value) {
                strncpy(env_value, value, 1024);
                env_value[1023] = '\0';
            }

            fclose(fp);

            return 0;
        }
    }
    if (fp)
        fclose(fp);

    /* no setting in config file, use env setting */
    value = secure_getenv(env);
    if (value) {
        if (env_value) {
            strncpy(env_value, value, 1024);
            env_value[1023] = '\0';
        }
        return 0;
    }

    return 1;
}

int vaDisplayIsValid(VADisplay dpy)
{
    VADisplayContextP pDisplayContext = (VADisplayContextP)dpy;
    return pDisplayContext &&
           pDisplayContext->vadpy_magic == VA_DISPLAY_MAGIC &&
           pDisplayContext->pDriverContext;
}

/*
 * Global log level configured from the config file or environment, which sets
 * whether default logging appears or not (always overridden by explicitly
 * user-configured logging).
 */
static int default_log_level = 2;

static void default_log_error(void *user_context, const char *buffer)
{
    if (default_log_level < 1)
        return;
# ifdef ANDROID
    ALOGE("%s", buffer);
# else
    fprintf(stderr, "libva error: %s", buffer);
# endif
}

static void default_log_info(void *user_context, const char *buffer)
{
    if (default_log_level < 2)
        return;
# ifdef ANDROID
    ALOGI("%s", buffer);
# else
    fprintf(stderr, "libva info: %s", buffer);
# endif
}

/**
 * Set the callback for error messages, or NULL for no logging.
 * Returns the previous one, or NULL if it was disabled.
 */
VAMessageCallback vaSetErrorCallback(VADisplay dpy, VAMessageCallback callback, void *user_context)
{
    VADisplayContextP dctx;
    VAMessageCallback old_callback;

    if (!vaDisplayIsValid(dpy))
        return NULL;

    dctx = (VADisplayContextP)dpy;
    old_callback = dctx->error_callback;

    dctx->error_callback = callback;
    dctx->error_callback_user_context = user_context;

    return old_callback;
}

/**
 * Set the callback for info messages, or NULL for no logging.
 * Returns the previous one, or NULL if it was disabled.
 */
VAMessageCallback vaSetInfoCallback(VADisplay dpy, VAMessageCallback callback, void *user_context)
{
    VADisplayContextP dctx;
    VAMessageCallback old_callback;

    if (!vaDisplayIsValid(dpy))
        return NULL;

    dctx = (VADisplayContextP)dpy;
    old_callback = dctx->info_callback;

    dctx->info_callback = callback;
    dctx->info_callback_user_context = user_context;

    return old_callback;
}

static void va_MessagingInit()
{
    char env_value[1024];
    int ret;

    if (va_parseConfig("LIBVA_MESSAGING_LEVEL", &env_value[0]) == 0) {
        ret = sscanf(env_value, "%d", &default_log_level);
        if (ret < 1 || default_log_level < 0 || default_log_level > 2)
            default_log_level = 2;
    }
}

void va_errorMessage(VADisplay dpy, const char *msg, ...)
{
    VADisplayContextP dctx = (VADisplayContextP)dpy;
    char buf[512], *dynbuf;
    va_list args;
    int n, len;

    if (dctx->error_callback == NULL)
        return;

    va_start(args, msg);
    len = vsnprintf(buf, sizeof(buf), msg, args);
    va_end(args);

    if (len >= (int)sizeof(buf)) {
        dynbuf = malloc(len + 1);
        if (!dynbuf)
            return;
        va_start(args, msg);
        n = vsnprintf(dynbuf, len + 1, msg, args);
        va_end(args);
        if (n == len)
            dctx->error_callback(dctx->error_callback_user_context, dynbuf);
        free(dynbuf);
    } else if (len > 0)
        dctx->error_callback(dctx->error_callback_user_context, buf);
}

void va_infoMessage(VADisplay dpy, const char *msg, ...)
{
    VADisplayContextP dctx = (VADisplayContextP)dpy;
    char buf[512], *dynbuf;
    va_list args;
    int n, len;

    if (dctx->info_callback == NULL)
        return;

    va_start(args, msg);
    len = vsnprintf(buf, sizeof(buf), msg, args);
    va_end(args);

    if (len >= (int)sizeof(buf)) {
        dynbuf = malloc(len + 1);
        if (!dynbuf)
            return;
        va_start(args, msg);
        n = vsnprintf(dynbuf, len + 1, msg, args);
        va_end(args);
        if (n == len)
            dctx->info_callback(dctx->info_callback_user_context, dynbuf);
        free(dynbuf);
    } else if (len > 0)
        dctx->info_callback(dctx->info_callback_user_context, buf);
}

static void va_driverErrorCallback(VADriverContextP ctx,
                                   const char *message)
{
    VADisplayContextP dctx = ctx->pDisplayContext;
    if (!dctx)
        return;
    dctx->error_callback(dctx->error_callback_user_context, message);
}

static void va_driverInfoCallback(VADriverContextP ctx,
                                  const char *message)
{
    VADisplayContextP dctx = ctx->pDisplayContext;
    if (!dctx)
        return;
    dctx->info_callback(dctx->info_callback_user_context, message);
}

VADisplayContextP va_newDisplayContext(void)
{
    VADisplayContextP dctx = calloc(1, sizeof(*dctx));
    if (!dctx)
        return NULL;

    dctx->vadpy_magic = VA_DISPLAY_MAGIC;

    dctx->error_callback = default_log_error;
    dctx->info_callback  = default_log_info;

    return dctx;
}

VADriverContextP va_newDriverContext(VADisplayContextP dctx)
{
    VADriverContextP ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
        return NULL;

    dctx->pDriverContext = ctx;
    ctx->pDisplayContext = dctx;

    ctx->error_callback = va_driverErrorCallback;
    ctx->info_callback  = va_driverInfoCallback;

    return ctx;
}

static bool va_checkVtable(VADisplay dpy, void *ptr, char *function)
{
    if (!ptr) {
        va_errorMessage(dpy, "No valid vtable entry for va%s\n", function);
        return false;
    }
    return true;
}

static bool va_checkMaximum(VADisplay dpy, int value, char *variable)
{
    if (!value) {
        va_errorMessage(dpy, "Failed to define max_%s in init\n", variable);
        return false;
    }
    return true;
}

static bool va_checkString(VADisplay dpy, const char* value, char *variable)
{
    if (!value) {
        va_errorMessage(dpy, "Failed to define str_%s in init\n", variable);
        return false;
    }
    return true;
}

static inline int
va_getDriverInitName(char *name, int namelen, int major, int minor)
{
    int ret = snprintf(name, namelen, "__vaDriverInit_%d_%d", major, minor);
    return ret > 0 && ret < namelen;
}

static char *va_getDriverPath(const char *driver_dir, const char *driver_name)
{
    int n = snprintf(0, 0, DRIVER_PATH_STRING, driver_dir, driver_name, DRIVER_EXTENSION);
    if (n < 0)
        return NULL;
    char *driver_path = (char *) malloc(n + 1);
    if (!driver_path)
        return NULL;
    n = snprintf(driver_path, n + 1, DRIVER_PATH_STRING,
                 driver_dir, driver_name, DRIVER_EXTENSION);
    if (n < 0) {
        free(driver_path);
        return NULL;
    }
    return driver_path;
}

static VAStatus va_openDriver(VADisplay dpy, char *driver_name)
{
    VADriverContextP ctx = CTX(dpy);
    VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
    char *search_path = NULL;
    char *saveptr;
    char *driver_dir;

    if (geteuid() == getuid())
        /* don't allow setuid apps to use LIBVA_DRIVERS_PATH */
        search_path = secure_getenv("LIBVA_DRIVERS_PATH");
    if (!search_path)
        search_path = VA_DRIVERS_PATH;

    search_path = strdup((const char *)search_path);
    if (!search_path) {
        va_errorMessage(dpy, "%s L%d Out of memory\n",
                        __FUNCTION__, __LINE__);
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }
    driver_dir = strtok_r(search_path, ENV_VAR_SEPARATOR, &saveptr);
    while (driver_dir) {
        void *handle = NULL;
        char *driver_path = va_getDriverPath(driver_dir, driver_name);
        if (!driver_path) {
            va_errorMessage(dpy, "%s L%d Out of memory\n",
                            __FUNCTION__, __LINE__);
            free(search_path);
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        va_infoMessage(dpy, "Trying to open %s\n", driver_path);
#if defined(RTLD_NODELETE) && !defined(ANDROID)
        handle = dlopen(driver_path, RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
#else
        handle = dlopen(driver_path, RTLD_NOW | RTLD_GLOBAL);
#endif
        if (!handle) {
            /* Don't give errors for non-existing files */
            if (0 == access(driver_path, F_OK))
                va_errorMessage(dpy, "dlopen of %s failed: %s\n", driver_path, dlerror());
        } else {
            VADriverInit init_func = NULL;
            char init_func_s[256];
            int i;

            struct {
                int major;
                int minor;
            } compatible_versions[VA_MINOR_VERSION + 2];
            for (i = 0; i <= VA_MINOR_VERSION; i ++) {
                compatible_versions[i].major = VA_MAJOR_VERSION;
                compatible_versions[i].minor = VA_MINOR_VERSION - i;
            }
            compatible_versions[i].major = -1;
            compatible_versions[i].minor = -1;

            for (i = 0; compatible_versions[i].major >= 0; i++) {
                if (va_getDriverInitName(init_func_s, sizeof(init_func_s),
                                         compatible_versions[i].major,
                                         compatible_versions[i].minor)) {
                    init_func = (VADriverInit)dlsym(handle, init_func_s);
                    if (init_func) {
                        va_infoMessage(dpy, "Found init function %s\n", init_func_s);
                        break;
                    }
                }
            }

            if (compatible_versions[i].major < 0) {
                va_errorMessage(dpy, "%s has no function %s\n",
                                driver_path, init_func_s);
                dlclose(handle);
            } else {
                struct VADriverVTable *vtable = ctx->vtable;
                struct VADriverVTableVPP *vtable_vpp = ctx->vtable_vpp;
                struct VADriverVTableProt *vtable_prot = ctx->vtable_prot;

                vaStatus = VA_STATUS_SUCCESS;
                if (!vtable) {
                    vtable = calloc(1, sizeof(*vtable));
                    if (!vtable)
                        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
                }
                ctx->vtable = vtable;

                if (!vtable_vpp) {
                    vtable_vpp = calloc(1, sizeof(*vtable_vpp));
                    if (vtable_vpp)
                        vtable_vpp->version = VA_DRIVER_VTABLE_VPP_VERSION;
                    else
                        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
                }
                ctx->vtable_vpp = vtable_vpp;

                if (!vtable_prot) {
                    vtable_prot = calloc(1, sizeof(*vtable_prot));
                    if (vtable_prot)
                        vtable_prot->version = VA_DRIVER_VTABLE_PROT_VERSION;
                    else
                        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
                }
                ctx->vtable_prot = vtable_prot;

                if (init_func && VA_STATUS_SUCCESS == vaStatus)
                    vaStatus = (*init_func)(ctx);

                if (VA_STATUS_SUCCESS == vaStatus) {
                    CHECK_MAXIMUM(vaStatus, ctx, profiles);
                    CHECK_MAXIMUM(vaStatus, ctx, entrypoints);
                    CHECK_MAXIMUM(vaStatus, ctx, attributes);
                    CHECK_MAXIMUM(vaStatus, ctx, image_formats);
                    CHECK_MAXIMUM(vaStatus, ctx, subpic_formats);
                    CHECK_STRING(vaStatus, ctx, vendor);
                    CHECK_VTABLE(vaStatus, ctx, Terminate);
                    CHECK_VTABLE(vaStatus, ctx, QueryConfigProfiles);
                    CHECK_VTABLE(vaStatus, ctx, QueryConfigEntrypoints);
                    CHECK_VTABLE(vaStatus, ctx, QueryConfigAttributes);
                    CHECK_VTABLE(vaStatus, ctx, CreateConfig);
                    CHECK_VTABLE(vaStatus, ctx, DestroyConfig);
                    CHECK_VTABLE(vaStatus, ctx, GetConfigAttributes);
                    CHECK_VTABLE(vaStatus, ctx, CreateSurfaces);
                    CHECK_VTABLE(vaStatus, ctx, DestroySurfaces);
                    CHECK_VTABLE(vaStatus, ctx, CreateContext);
                    CHECK_VTABLE(vaStatus, ctx, DestroyContext);
                    CHECK_VTABLE(vaStatus, ctx, CreateBuffer);
                    CHECK_VTABLE(vaStatus, ctx, BufferSetNumElements);
                    CHECK_VTABLE(vaStatus, ctx, MapBuffer);
                    CHECK_VTABLE(vaStatus, ctx, UnmapBuffer);
                    CHECK_VTABLE(vaStatus, ctx, DestroyBuffer);
                    CHECK_VTABLE(vaStatus, ctx, BeginPicture);
                    CHECK_VTABLE(vaStatus, ctx, RenderPicture);
                    CHECK_VTABLE(vaStatus, ctx, EndPicture);
                    CHECK_VTABLE(vaStatus, ctx, SyncSurface);
                    CHECK_VTABLE(vaStatus, ctx, QuerySurfaceStatus);
                    CHECK_VTABLE(vaStatus, ctx, QueryImageFormats);
                    CHECK_VTABLE(vaStatus, ctx, CreateImage);
                    CHECK_VTABLE(vaStatus, ctx, DeriveImage);
                    CHECK_VTABLE(vaStatus, ctx, DestroyImage);
                    CHECK_VTABLE(vaStatus, ctx, SetImagePalette);
                    CHECK_VTABLE(vaStatus, ctx, GetImage);
                    CHECK_VTABLE(vaStatus, ctx, PutImage);
                    CHECK_VTABLE(vaStatus, ctx, QuerySubpictureFormats);
                    CHECK_VTABLE(vaStatus, ctx, CreateSubpicture);
                    CHECK_VTABLE(vaStatus, ctx, DestroySubpicture);
                    CHECK_VTABLE(vaStatus, ctx, SetSubpictureImage);
                    CHECK_VTABLE(vaStatus, ctx, SetSubpictureChromakey);
                    CHECK_VTABLE(vaStatus, ctx, SetSubpictureGlobalAlpha);
                    CHECK_VTABLE(vaStatus, ctx, AssociateSubpicture);
                    CHECK_VTABLE(vaStatus, ctx, DeassociateSubpicture);
                    CHECK_VTABLE(vaStatus, ctx, QueryDisplayAttributes);
                    CHECK_VTABLE(vaStatus, ctx, GetDisplayAttributes);
                    CHECK_VTABLE(vaStatus, ctx, SetDisplayAttributes);
                }
                if (VA_STATUS_SUCCESS != vaStatus) {
                    va_errorMessage(dpy, "%s init failed\n", driver_path);
                    dlclose(handle);
                }
                if (VA_STATUS_SUCCESS == vaStatus)
                    ctx->handle = handle;
                free(driver_path);
                break;
            }
        }
        free(driver_path);

        driver_dir = strtok_r(NULL, ENV_VAR_SEPARATOR, &saveptr);
    }

    free(search_path);

    return vaStatus;
}

VAPrivFunc vaGetLibFunc(VADisplay dpy, const char *func)
{
    VADriverContextP ctx;
    if (!vaDisplayIsValid(dpy))
        return NULL;
    ctx = CTX(dpy);

    if (NULL == ctx->handle)
        return NULL;

    return (VAPrivFunc) dlsym(ctx->handle, func);
}


/*
 * Returns a short english description of error_status
 */
const char *vaErrorStr(VAStatus error_status)
{
    switch (error_status) {
    case VA_STATUS_SUCCESS:
        return "success (no error)";
    case VA_STATUS_ERROR_OPERATION_FAILED:
        return "operation failed";
    case VA_STATUS_ERROR_ALLOCATION_FAILED:
        return "resource allocation failed";
    case VA_STATUS_ERROR_INVALID_DISPLAY:
        return "invalid VADisplay";
    case VA_STATUS_ERROR_INVALID_CONFIG:
        return "invalid VAConfigID";
    case VA_STATUS_ERROR_INVALID_CONTEXT:
        return "invalid VAContextID";
    case VA_STATUS_ERROR_INVALID_SURFACE:
        return "invalid VASurfaceID";
    case VA_STATUS_ERROR_INVALID_BUFFER:
        return "invalid VABufferID";
    case VA_STATUS_ERROR_INVALID_IMAGE:
        return "invalid VAImageID";
    case VA_STATUS_ERROR_INVALID_SUBPICTURE:
        return "invalid VASubpictureID";
    case VA_STATUS_ERROR_ATTR_NOT_SUPPORTED:
        return "attribute not supported";
    case VA_STATUS_ERROR_MAX_NUM_EXCEEDED:
        return "list argument exceeds maximum number";
    case VA_STATUS_ERROR_UNSUPPORTED_PROFILE:
        return "the requested VAProfile is not supported";
    case VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT:
        return "the requested VAEntryPoint is not supported";
    case VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT:
        return "the requested RT Format is not supported";
    case VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE:
        return "the requested VABufferType is not supported";
    case VA_STATUS_ERROR_SURFACE_BUSY:
        return "surface is in use";
    case VA_STATUS_ERROR_FLAG_NOT_SUPPORTED:
        return "flag not supported";
    case VA_STATUS_ERROR_INVALID_PARAMETER:
        return "invalid parameter";
    case VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED:
        return "resolution not supported";
    case VA_STATUS_ERROR_UNIMPLEMENTED:
        return "the requested function is not implemented";
    case VA_STATUS_ERROR_SURFACE_IN_DISPLAYING:
        return "surface is in displaying (may by overlay)" ;
    case VA_STATUS_ERROR_INVALID_IMAGE_FORMAT:
        return "invalid VAImageFormat";
    case VA_STATUS_ERROR_DECODING_ERROR:
        return "internal decoding error";
    case VA_STATUS_ERROR_ENCODING_ERROR:
        return "internal encoding error";
    case VA_STATUS_ERROR_INVALID_VALUE:
        return "an invalid/unsupported value was supplied";
    case VA_STATUS_ERROR_UNSUPPORTED_FILTER:
        return "the requested filter is not supported";
    case VA_STATUS_ERROR_INVALID_FILTER_CHAIN:
        return "an invalid filter chain was supplied";
    case VA_STATUS_ERROR_HW_BUSY:
        return "HW busy now";
    case VA_STATUS_ERROR_UNSUPPORTED_MEMORY_TYPE:
        return "an unsupported memory type was supplied";
    case VA_STATUS_ERROR_NOT_ENOUGH_BUFFER:
        return "allocated memory size is not enough for input or output";
    case VA_STATUS_ERROR_UNKNOWN:
        return "unknown libva error";
    }
    return "unknown libva error / description missing";
}

VAStatus vaSetDriverName(
    VADisplay dpy,
    char *driver_name
)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    char *override_driver_name = NULL;
    ctx = CTX(dpy);

    if (strlen(driver_name) == 0 || strlen(driver_name) >= 256) {
        vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        va_errorMessage(dpy, "vaSetDriverName returns %s\n",
                        vaErrorStr(vaStatus));
        return vaStatus;
    }

    override_driver_name = strdup(driver_name);
    if (!override_driver_name) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        va_errorMessage(dpy, "vaSetDriverName returns %s. Out of Memory\n",
                        vaErrorStr(vaStatus));
        return vaStatus;
    }

    ctx->override_driver_name = override_driver_name;
    return VA_STATUS_SUCCESS;
}

static VAStatus va_new_opendriver(VADisplay dpy)
{
    VADisplayContextP pDisplayContext = (VADisplayContextP)dpy;
    /* In the extreme case we can get up-to 5ish names. Pad that out to be on
     * the safe side. In the worst case, the DRIVER BUG below will print and
     * we'll be capped to the current selection.
     */
    char *drivers[20] = { 0, };
    unsigned int num_drivers = ARRAY_SIZE(drivers);
    VAStatus vaStatus;
    const char *driver_name_env;
    VADriverContextP ctx;

    /* XXX: The order is bonkers - env var should take highest priority, then
     * override (which ought to be nuked) than native. It's not possible atm,
     * since the DPY connect/init happens during the GetDriverNames.
     */
    vaStatus = pDisplayContext->vaGetDriverNames(pDisplayContext, drivers, &num_drivers);
    if (vaStatus != VA_STATUS_SUCCESS) {
        /* Print and error yet continue, as per the above ordering note */
        va_errorMessage(dpy, "vaGetDriverNames() failed with %s\n", vaErrorStr(vaStatus));
        num_drivers = 0;
    }

    ctx = CTX(dpy);
    driver_name_env = secure_getenv("LIBVA_DRIVER_NAME");

    if ((ctx->override_driver_name) || (driver_name_env && (geteuid() == getuid()))) {
        const char *driver = ctx->override_driver_name ?
                             ctx->override_driver_name : driver_name_env;

        for (unsigned int i = 0; i < num_drivers; i++)
            free(drivers[i]);

        drivers[0] = strdup(driver);
        num_drivers = 1;

        va_infoMessage(dpy, "User %srequested driver '%s'\n",
                       ctx->override_driver_name ? "" : "environment variable ",
                       driver);
    }

    for (unsigned int i = 0; i < num_drivers; i++) {
        /* The strdup() may have failed. Check here instead of a dozen+ places */
        if (!drivers[i]) {
            va_errorMessage(dpy, "%s:%d: Out of memory\n", __func__, __LINE__);
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            break;
        }

        vaStatus = va_openDriver(dpy, drivers[i]);
        va_infoMessage(dpy, "va_openDriver() returns %d\n", vaStatus);

        if (vaStatus == VA_STATUS_SUCCESS)
            break;
    }

    for (unsigned int i = 0; i < num_drivers; i++)
        free(drivers[i]);

    return vaStatus;
}

VAStatus vaInitialize(
    VADisplay dpy,
    int *major_version,  /* out */
    int *minor_version   /* out */
)
{
    VAStatus vaStatus;

    CHECK_DISPLAY(dpy);

    va_TraceInit(dpy);

    va_MessagingInit();

    va_infoMessage(dpy, "VA-API version %s\n", VA_VERSION_S);

    vaStatus = va_new_opendriver(dpy);

    *major_version = VA_MAJOR_VERSION;
    *minor_version = VA_MINOR_VERSION;

    VA_TRACE_LOG(va_TraceInitialize, dpy, major_version, minor_version);
    VA_TRACE_RET(dpy, vaStatus);
    return vaStatus;
}


/*
 * After this call, all library internal resources will be cleaned up
 */
VAStatus vaTerminate(
    VADisplay dpy
)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VADisplayContextP pDisplayContext = (VADisplayContextP)dpy;
    VADriverContextP old_ctx;

    CHECK_DISPLAY(dpy);
    old_ctx = CTX(dpy);

    if (old_ctx->handle) {
        vaStatus = old_ctx->vtable->vaTerminate(old_ctx);
        dlclose(old_ctx->handle);
        old_ctx->handle = NULL;
    }
    free(old_ctx->vtable);
    old_ctx->vtable = NULL;
    free(old_ctx->vtable_vpp);
    old_ctx->vtable_vpp = NULL;
    free(old_ctx->vtable_prot);
    old_ctx->vtable_prot = NULL;

    if (old_ctx->override_driver_name) {
        free(old_ctx->override_driver_name);
        old_ctx->override_driver_name = NULL;
    }

    VA_TRACE_LOG(va_TraceTerminate, dpy);
    VA_TRACE_RET(dpy, vaStatus);

    va_TraceEnd(dpy);

    if (VA_STATUS_SUCCESS == vaStatus)
        pDisplayContext->vaDestroy(pDisplayContext);

    return vaStatus;
}

/*
 * vaQueryVendorString returns a pointer to a zero-terminated string
 * describing some aspects of the VA implemenation on a specific
 * hardware accelerator. The format of the returned string is:
 * <vendorname>-<major_version>-<minor_version>-<addtional_info>
 * e.g. for the Intel GMA500 implementation, an example would be:
 * "IntelGMA500-1.0-0.2-patch3
 */
const char *vaQueryVendorString(
    VADisplay dpy
)
{
    if (!vaDisplayIsValid(dpy))
        return NULL;

    return CTX(dpy)->str_vendor;
}


/* Get maximum number of profiles supported by the implementation */
int vaMaxNumProfiles(
    VADisplay dpy
)
{
    if (!vaDisplayIsValid(dpy))
        return 0;

    return CTX(dpy)->max_profiles;
}

/* Get maximum number of entrypoints supported by the implementation */
int vaMaxNumEntrypoints(
    VADisplay dpy
)
{
    if (!vaDisplayIsValid(dpy))
        return 0;

    return CTX(dpy)->max_entrypoints;
}


/* Get maximum number of attributs supported by the implementation */
int vaMaxNumConfigAttributes(
    VADisplay dpy
)
{
    if (!vaDisplayIsValid(dpy))
        return 0;

    return CTX(dpy)->max_attributes;
}

VAStatus vaQueryConfigEntrypoints(
    VADisplay dpy,
    VAProfile profile,
    VAEntrypoint *entrypoints,  /* out */
    int *num_entrypoints    /* out */
)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    vaStatus = ctx->vtable->vaQueryConfigEntrypoints(ctx, profile, entrypoints, num_entrypoints);
    VA_TRACE_RET(dpy, vaStatus);
    return vaStatus;
}

VAStatus vaGetConfigAttributes(
    VADisplay dpy,
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list, /* in/out */
    int num_attribs
)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    vaStatus = ctx->vtable->vaGetConfigAttributes(ctx, profile, entrypoint, attrib_list, num_attribs);
    VA_TRACE_RET(dpy, vaStatus);
    return vaStatus;
}

VAStatus vaQueryConfigProfiles(
    VADisplay dpy,
    VAProfile *profile_list,    /* out */
    int *num_profiles       /* out */
)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    vaStatus =  ctx->vtable->vaQueryConfigProfiles(ctx, profile_list, num_profiles);
    VA_TRACE_RET(dpy, vaStatus);
    return vaStatus;
}

VAStatus vaCreateConfig(
    VADisplay dpy,
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs,
    VAConfigID *config_id /* out */
)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_VVVA(dpy, CREATE_CONFIG, TRACE_BEGIN, profile, entrypoint, num_attribs, attrib_list);
    vaStatus = ctx->vtable->vaCreateConfig(ctx, profile, entrypoint, attrib_list, num_attribs, config_id);

    /* record the current entrypoint for further trace/fool determination */
    VA_TRACE_ALL(va_TraceCreateConfig, dpy, profile, entrypoint, attrib_list, num_attribs, config_id);
    VA_TRACE_RET(dpy, vaStatus);
    VA_TRACE_PV(dpy, CREATE_CONFIG, TRACE_END, config_id, vaStatus);
    return vaStatus;
}

VAStatus vaDestroyConfig(
    VADisplay dpy,
    VAConfigID config_id
)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_V(dpy, DESTROY_CONFIG, TRACE_BEGIN, config_id);
    vaStatus = ctx->vtable->vaDestroyConfig(ctx, config_id);

    VA_TRACE_ALL(va_TraceDestroyConfig, dpy, config_id);
    VA_TRACE_RET(dpy, vaStatus);
    VA_TRACE_V(dpy, DESTROY_CONFIG, TRACE_END, vaStatus);

    return vaStatus;
}

VAStatus vaQueryConfigAttributes(
    VADisplay dpy,
    VAConfigID config_id,
    VAProfile *profile,     /* out */
    VAEntrypoint *entrypoint,   /* out */
    VAConfigAttrib *attrib_list,/* out */
    int *num_attribs        /* out */
)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    vaStatus = ctx->vtable->vaQueryConfigAttributes(ctx, config_id, profile, entrypoint, attrib_list, num_attribs);
    VA_TRACE_RET(dpy, vaStatus);
    return vaStatus;
}

VAStatus vaQueryProcessingRate(
    VADisplay dpy,
    VAConfigID config_id,
    VAProcessingRateParameter *proc_buf,
    unsigned int *processing_rate   /* out */
)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);
    if (!ctx->vtable->vaQueryProcessingRate)
        vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
    else
        vaStatus = ctx->vtable->vaQueryProcessingRate(ctx, config_id, proc_buf, processing_rate);
    VA_TRACE_RET(dpy, vaStatus);
    return vaStatus;
}

/* XXX: this is a slow implementation that will be removed */
static VAStatus
va_impl_query_surface_attributes(
    VADriverContextP    ctx,
    VAConfigID          config,
    VASurfaceAttrib    *out_attribs,
    unsigned int       *out_num_attribs_ptr
)
{
    VASurfaceAttrib *attribs = NULL;
    unsigned int num_attribs, n;
    VASurfaceAttrib *out_attrib;
    unsigned int out_num_attribs;
    VAImageFormat *image_formats = NULL;
    int num_image_formats, i;
    VAStatus va_status;

    /* List of surface attributes to query */
    struct va_surface_attrib_map {
        VASurfaceAttribType type;
        VAGenericValueType  value_type;
    };
    static const struct va_surface_attrib_map attribs_map[] = {
        { VASurfaceAttribMinWidth,      VAGenericValueTypeInteger },
        { VASurfaceAttribMaxWidth,      VAGenericValueTypeInteger },
        { VASurfaceAttribMinHeight,     VAGenericValueTypeInteger },
        { VASurfaceAttribMaxHeight,     VAGenericValueTypeInteger },
        { VASurfaceAttribMemoryType,    VAGenericValueTypeInteger },
        { VASurfaceAttribNone,          VAGenericValueTypeInteger }
    };

    if (!out_attribs || !out_num_attribs_ptr)
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    if (!ctx->vtable->vaGetSurfaceAttributes)
        return VA_STATUS_ERROR_UNIMPLEMENTED;

    num_image_formats = ctx->max_image_formats;
    image_formats = malloc(num_image_formats * sizeof(*image_formats));
    if (!image_formats) {
        va_status = VA_STATUS_ERROR_ALLOCATION_FAILED;
        goto end;
    }

    va_status = ctx->vtable->vaQueryImageFormats(
                    ctx, image_formats, &num_image_formats);
    if (va_status != VA_STATUS_SUCCESS)
        goto end;

    num_attribs = VASurfaceAttribCount + num_image_formats;
    attribs = malloc(num_attribs * sizeof(*attribs));
    if (!attribs) {
        va_status = VA_STATUS_ERROR_ALLOCATION_FAILED;
        goto end;
    }

    /* Initialize with base surface attributes, except pixel-formats */
    for (n = 0; attribs_map[n].type != VASurfaceAttribNone; n++) {
        VASurfaceAttrib * const attrib = &attribs[n];
        attrib->type = attribs_map[n].type;
        attrib->flags = VA_SURFACE_ATTRIB_GETTABLE;
        attrib->value.type = attribs_map[n].value_type;
    }

    /* Append image formats */
    for (i = 0; i < num_image_formats; i++) {
        VASurfaceAttrib * const attrib = &attribs[n];
        attrib->type = VASurfaceAttribPixelFormat;
        attrib->flags = VA_SURFACE_ATTRIB_GETTABLE | VA_SURFACE_ATTRIB_SETTABLE;
        attrib->value.type = VAGenericValueTypeInteger;
        attrib->value.value.i = image_formats[i].fourcc;
        if (++n == num_attribs) {
            va_status = VA_STATUS_ERROR_ALLOCATION_FAILED;
            goto end;
        }
    }
    num_attribs = n;

    va_status = ctx->vtable->vaGetSurfaceAttributes(
                    ctx, config, attribs, num_attribs);
    if (va_status != VA_STATUS_SUCCESS)
        goto end;

    /* Remove invalid entries */
    out_num_attribs = 0;
    for (n = 0; n < num_attribs; n++) {
        VASurfaceAttrib * const attrib = &attribs[n];

        if (attrib->flags == VA_SURFACE_ATTRIB_NOT_SUPPORTED)
            continue;

        // Accept all surface attributes that are not pixel-formats
        if (attrib->type != VASurfaceAttribPixelFormat) {
            out_num_attribs++;
            continue;
        }

        // Drop invalid pixel-format attribute
        if (!attrib->value.value.i) {
            attrib->flags = VA_SURFACE_ATTRIB_NOT_SUPPORTED;
            continue;
        }

        // Check for duplicates
        int is_duplicate = 0;
        for (i = n - 1; i >= 0 && !is_duplicate; i--) {
            const VASurfaceAttrib * const prev_attrib = &attribs[i];
            if (prev_attrib->type != VASurfaceAttribPixelFormat)
                break;
            is_duplicate = prev_attrib->value.value.i == attrib->value.value.i;
        }
        if (is_duplicate)
            attrib->flags = VA_SURFACE_ATTRIB_NOT_SUPPORTED;
        else
            out_num_attribs++;
    }

    if (*out_num_attribs_ptr < out_num_attribs) {
        *out_num_attribs_ptr = out_num_attribs;
        va_status = VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
        goto end;
    }

    out_attrib = out_attribs;
    for (n = 0; n < num_attribs; n++) {
        const VASurfaceAttrib * const attrib = &attribs[n];
        if (attrib->flags == VA_SURFACE_ATTRIB_NOT_SUPPORTED)
            continue;
        *out_attrib++ = *attrib;
    }

end:
    free(attribs);
    free(image_formats);
    return va_status;
}

VAStatus
vaQuerySurfaceAttributes(
    VADisplay           dpy,
    VAConfigID          config,
    VASurfaceAttrib    *attrib_list,
    unsigned int       *num_attribs
)
{
    VADriverContextP ctx;
    VAStatus vaStatus;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);
    if (!ctx)
        return VA_STATUS_ERROR_INVALID_DISPLAY;

    VA_TRACE_V(dpy, QUERY_SURFACE_ATTR, TRACE_BEGIN, config);
    if (!ctx->vtable->vaQuerySurfaceAttributes)
        vaStatus = va_impl_query_surface_attributes(ctx, config,
                   attrib_list, num_attribs);
    else
        vaStatus = ctx->vtable->vaQuerySurfaceAttributes(ctx, config,
                   attrib_list, num_attribs);

    VA_TRACE_LOG(va_TraceQuerySurfaceAttributes, dpy, config, attrib_list, num_attribs);
    VA_TRACE_RET(dpy, vaStatus);
    VA_TRACE_PA(dpy, QUERY_SURFACE_ATTR, TRACE_END, num_attribs, attrib_list);

    return vaStatus;
}

VAStatus
vaCreateSurfaces(
    VADisplay           dpy,
    unsigned int        format,
    unsigned int        width,
    unsigned int        height,
    VASurfaceID        *surfaces,
    unsigned int        num_surfaces,
    VASurfaceAttrib    *attrib_list,
    unsigned int        num_attribs
)
{
    VADriverContextP ctx;
    VAStatus vaStatus;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);
    if (!ctx)
        return VA_STATUS_ERROR_INVALID_DISPLAY;

    VA_TRACE_VVVVA(dpy, CREATE_SURFACE, TRACE_BEGIN, width, height, format, num_attribs, attrib_list);
    if (ctx->vtable->vaCreateSurfaces2)
        vaStatus = ctx->vtable->vaCreateSurfaces2(ctx, format, width, height,
                   surfaces, num_surfaces,
                   attrib_list, num_attribs);
    else if (attrib_list && num_attribs > 0)
        vaStatus = VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;
    else
        vaStatus = ctx->vtable->vaCreateSurfaces(ctx, width, height, format,
                   num_surfaces, surfaces);
    VA_TRACE_LOG(va_TraceCreateSurfaces,
                 dpy, width, height, format, num_surfaces, surfaces,
                 attrib_list, num_attribs);
    VA_TRACE_RET(dpy, vaStatus);
    VA_TRACE_VVA(dpy, CREATE_SURFACE, TRACE_END, vaStatus, num_surfaces, surfaces);

    return vaStatus;
}


VAStatus vaDestroySurfaces(
    VADisplay dpy,
    VASurfaceID *surface_list,
    int num_surfaces
)
{
    VADriverContextP ctx;
    VAStatus vaStatus;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_VA(dpy, DESTROY_SURFACE, TRACE_BEGIN, num_surfaces, surface_list);
    VA_TRACE_LOG(va_TraceDestroySurfaces,
                 dpy, surface_list, num_surfaces);

    vaStatus = ctx->vtable->vaDestroySurfaces(ctx, surface_list, num_surfaces);
    VA_TRACE_RET(dpy, vaStatus);
    VA_TRACE_V(dpy, DESTROY_SURFACE, TRACE_END, vaStatus);

    return vaStatus;
}

VAStatus vaCreateContext(
    VADisplay dpy,
    VAConfigID config_id,
    int picture_width,
    int picture_height,
    int flag,
    VASurfaceID *render_targets,
    int num_render_targets,
    VAContextID *context        /* out */
)
{
    VADriverContextP ctx;
    VAStatus vaStatus;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_VVVVVA(dpy, CREATE_CONTEXT, TRACE_BEGIN, config_id, picture_width, picture_height, flag, num_render_targets, render_targets);
    vaStatus = ctx->vtable->vaCreateContext(ctx, config_id, picture_width, picture_height,
                                            flag, render_targets, num_render_targets, context);

    /* keep current encode/decode resoluton */
    VA_TRACE_ALL(va_TraceCreateContext, dpy, config_id, picture_width, picture_height, flag, render_targets, num_render_targets, context);
    VA_TRACE_RET(dpy, vaStatus);
    VA_TRACE_PV(dpy, CREATE_CONTEXT, TRACE_END, context, vaStatus);

    return vaStatus;
}

VAStatus vaDestroyContext(
    VADisplay dpy,
    VAContextID context
)
{
    VADriverContextP ctx;
    VAStatus vaStatus;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_V(dpy, DESTROY_CONTEXT, TRACE_BEGIN, context);
    vaStatus = ctx->vtable->vaDestroyContext(ctx, context);

    VA_TRACE_ALL(va_TraceDestroyContext, dpy, context);
    VA_TRACE_RET(dpy, vaStatus);
    VA_TRACE_V(dpy, DESTROY_CONTEXT, TRACE_END, vaStatus);

    return vaStatus;
}

VAStatus vaCreateMFContext(
    VADisplay dpy,
    VAMFContextID *mf_context    /* out */
)
{
    VADriverContextP ctx;
    VAStatus vaStatus;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);
    if (ctx->vtable->vaCreateMFContext == NULL)
        vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
    else {
        vaStatus = ctx->vtable->vaCreateMFContext(ctx, mf_context);
        VA_TRACE_ALL(va_TraceCreateMFContext, dpy, mf_context);
    }

    VA_TRACE_RET(dpy, vaStatus);
    return vaStatus;
}

VAStatus vaMFAddContext(
    VADisplay dpy,
    VAMFContextID mf_context,
    VAContextID context
)
{
    VADriverContextP ctx;
    VAStatus vaStatus;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    if (ctx->vtable->vaMFAddContext == NULL)
        vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
    else {
        vaStatus = ctx->vtable->vaMFAddContext(ctx, context, mf_context);
        VA_TRACE_ALL(va_TraceMFAddContext, dpy, context, mf_context);
    }

    VA_TRACE_RET(dpy, vaStatus);
    return vaStatus;
}

VAStatus vaMFReleaseContext(
    VADisplay dpy,
    VAMFContextID mf_context,
    VAContextID context
)
{
    VADriverContextP ctx;
    VAStatus vaStatus;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);
    if (ctx->vtable->vaMFReleaseContext == NULL)
        vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
    else {
        vaStatus = ctx->vtable->vaMFReleaseContext(ctx, context, mf_context);
        VA_TRACE_ALL(va_TraceMFReleaseContext, dpy, context, mf_context);
    }
    VA_TRACE_RET(dpy, vaStatus);

    return vaStatus;
}

VAStatus vaMFSubmit(
    VADisplay dpy,
    VAMFContextID mf_context,
    VAContextID *contexts,
    int num_contexts
)
{
    VADriverContextP ctx;
    VAStatus vaStatus;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);
    CHECK_VTABLE(vaStatus, ctx, MFSubmit);
    if (ctx->vtable->vaMFSubmit == NULL)
        vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
    else {
        vaStatus = ctx->vtable->vaMFSubmit(ctx, mf_context, contexts, num_contexts);
        VA_TRACE_ALL(va_TraceMFSubmit, dpy, mf_context, contexts, num_contexts);
    }
    VA_TRACE_RET(dpy, vaStatus);

    return vaStatus;
}

VAStatus vaCreateBuffer(
    VADisplay dpy,
    VAContextID context,    /* in */
    VABufferType type,      /* in */
    unsigned int size,      /* in */
    unsigned int num_elements,  /* in */
    void *data,         /* in */
    VABufferID *buf_id      /* out */
)
{
    VADriverContextP ctx;
    VAStatus vaStatus;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_VVVV(dpy, CREATE_BUFFER, TRACE_BEGIN, context, type, size, num_elements);
    vaStatus = ctx->vtable->vaCreateBuffer(ctx, context, type, size, num_elements, data, buf_id);

    VA_TRACE_LOG(va_TraceCreateBuffer,
                 dpy, context, type, size, num_elements, data, buf_id);

    VA_TRACE_RET(dpy, vaStatus);
    VA_TRACE_PV(dpy, CREATE_BUFFER, TRACE_END, buf_id, vaStatus);
    return vaStatus;
}

VAStatus vaCreateBuffer2(
    VADisplay dpy,
    VAContextID context,
    VABufferType type,
    unsigned int width,
    unsigned int height,
    unsigned int *unit_size,
    unsigned int *pitch,
    VABufferID *buf_id
)
{
    VADriverContextP ctx;
    VAStatus vaStatus;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);
    if (!ctx->vtable->vaCreateBuffer2)
        return VA_STATUS_ERROR_UNIMPLEMENTED;

    vaStatus = ctx->vtable->vaCreateBuffer2(ctx, context, type, width, height, unit_size, pitch, buf_id);

    VA_TRACE_LOG(va_TraceCreateBuffer,
                 dpy, context, type, *pitch, height, NULL, buf_id);
    VA_TRACE_RET(dpy, vaStatus);

    return vaStatus;
}

VAStatus vaBufferSetNumElements(
    VADisplay dpy,
    VABufferID buf_id,  /* in */
    unsigned int num_elements /* in */
)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    vaStatus = ctx->vtable->vaBufferSetNumElements(ctx, buf_id, num_elements);
    VA_TRACE_RET(dpy, vaStatus);
    return vaStatus;
}


VAStatus vaMapBuffer(
    VADisplay dpy,
    VABufferID buf_id,  /* in */
    void **pbuf     /* out */
)
{
    VADriverContextP ctx;
    VAStatus va_status = VA_STATUS_SUCCESS;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    if (ctx->vtable->vaMapBuffer2) {
        va_status = ctx->vtable->vaMapBuffer2(ctx, buf_id, pbuf, VA_MAPBUFFER_FLAG_DEFAULT);
    } else if (ctx->vtable->vaMapBuffer) {
        va_status = ctx->vtable->vaMapBuffer(ctx, buf_id, pbuf);
    }

    VA_TRACE_ALL(va_TraceMapBuffer, dpy, buf_id, pbuf, VA_MAPBUFFER_FLAG_DEFAULT);
    VA_TRACE_RET(dpy, va_status);

    return va_status;
}

VAStatus vaMapBuffer2(
    VADisplay dpy,
    VABufferID buf_id,  /* in */
    void **pbuf,    /* out */
    uint32_t flags      /*in */
)
{
    VADriverContextP ctx;
    VAStatus va_status = VA_STATUS_SUCCESS;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    if (ctx->vtable->vaMapBuffer2) {
        va_status = ctx->vtable->vaMapBuffer2(ctx, buf_id, pbuf, flags);
    } else if (ctx->vtable->vaMapBuffer) {
        va_status = ctx->vtable->vaMapBuffer(ctx, buf_id, pbuf);
    }

    VA_TRACE_ALL(va_TraceMapBuffer, dpy, buf_id, pbuf, flags);
    VA_TRACE_RET(dpy, va_status);

    return va_status;
}

VAStatus vaUnmapBuffer(
    VADisplay dpy,
    VABufferID buf_id   /* in */
)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    vaStatus = ctx->vtable->vaUnmapBuffer(ctx, buf_id);
    VA_TRACE_RET(dpy, vaStatus);
    return vaStatus;
}

VAStatus vaDestroyBuffer(
    VADisplay dpy,
    VABufferID buffer_id
)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_V(dpy, DESTROY_BUFFER, TRACE_BEGIN, buffer_id);
    VA_TRACE_LOG(va_TraceDestroyBuffer,
                 dpy, buffer_id);

    vaStatus = ctx->vtable->vaDestroyBuffer(ctx, buffer_id);
    VA_TRACE_RET(dpy, vaStatus);
    VA_TRACE_V(dpy, DESTROY_BUFFER, TRACE_END, vaStatus);
    return vaStatus;
}

VAStatus vaBufferInfo(
    VADisplay dpy,
    VAContextID context,    /* in */
    VABufferID buf_id,      /* in */
    VABufferType *type,     /* out */
    unsigned int *size,     /* out */
    unsigned int *num_elements  /* out */
)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    vaStatus = ctx->vtable->vaBufferInfo(ctx, buf_id, type, size, num_elements);
    VA_TRACE_RET(dpy, vaStatus);
    return vaStatus;
}

/* Locks buffer for external API usage */
VAStatus
vaAcquireBufferHandle(VADisplay dpy, VABufferID buf_id, VABufferInfo *buf_info)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    if (!ctx->vtable->vaAcquireBufferHandle)
        vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
    else
        vaStatus = ctx->vtable->vaAcquireBufferHandle(ctx, buf_id, buf_info);
    VA_TRACE_RET(dpy, vaStatus);
    return vaStatus;
}

/* Unlocks buffer after usage from external API */
VAStatus
vaReleaseBufferHandle(VADisplay dpy, VABufferID buf_id)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    if (!ctx->vtable->vaReleaseBufferHandle)
        vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
    else
        vaStatus = ctx->vtable->vaReleaseBufferHandle(ctx, buf_id);
    VA_TRACE_RET(dpy, vaStatus);
    return vaStatus;
}

VAStatus
vaExportSurfaceHandle(VADisplay dpy, VASurfaceID surface_id,
                      uint32_t mem_type, uint32_t flags,
                      void *descriptor)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    if (!ctx->vtable->vaExportSurfaceHandle)
        vaStatus = VA_STATUS_ERROR_UNIMPLEMENTED;
    else
        vaStatus = ctx->vtable->vaExportSurfaceHandle(ctx, surface_id,
                   mem_type, flags,
                   descriptor);
    VA_TRACE_LOG(va_TraceExportSurfaceHandle, dpy, surface_id, mem_type, flags, descriptor);

    VA_TRACE_RET(dpy, vaStatus);
    return vaStatus;
}

VAStatus vaBeginPicture(
    VADisplay dpy,
    VAContextID context,
    VASurfaceID render_target
)
{
    VADriverContextP ctx;
    VAStatus va_status;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_VV(dpy, BEGIN_PICTURE, TRACE_BEGIN, context, render_target);
    VA_TRACE_ALL(va_TraceBeginPicture, dpy, context, render_target);

    va_status = ctx->vtable->vaBeginPicture(ctx, context, render_target);
    VA_TRACE_RET(dpy, va_status);
    VA_TRACE_V(dpy, BEGIN_PICTURE, TRACE_END, va_status);

    return va_status;
}

VAStatus vaRenderPicture(
    VADisplay dpy,
    VAContextID context,
    VABufferID *buffers,
    int num_buffers
)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_VVA(dpy, RENDER_PICTURE, TRACE_BEGIN, context, num_buffers, buffers);
    VA_TRACE_BUFFERS(dpy, context, num_buffers, buffers);
    VA_TRACE_LOG(va_TraceRenderPicture, dpy, context, buffers, num_buffers);

    vaStatus = ctx->vtable->vaRenderPicture(ctx, context, buffers, num_buffers);
    VA_TRACE_RET(dpy, vaStatus);
    VA_TRACE_V(dpy, RENDER_PICTURE, TRACE_END, vaStatus);
    return vaStatus;
}

VAStatus vaEndPicture(
    VADisplay dpy,
    VAContextID context
)
{
    VAStatus va_status = VA_STATUS_SUCCESS;
    VADriverContextP ctx;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_V(dpy, END_PICTURE, TRACE_BEGIN, context);
    VA_TRACE_ALL(va_TraceEndPicture, dpy, context, 0);
    va_status = ctx->vtable->vaEndPicture(ctx, context);
    VA_TRACE_RET(dpy, va_status);
    /* dump surface content */
    VA_TRACE_ALL(va_TraceEndPictureExt, dpy, context, 1);
    VA_TRACE_V(dpy, END_PICTURE, TRACE_END, va_status);

    return va_status;
}

VAStatus vaSyncSurface(
    VADisplay dpy,
    VASurfaceID render_target
)
{
    VAStatus va_status;
    VADriverContextP ctx;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_V(dpy, SYNC_SURFACE, TRACE_BEGIN, render_target);
    va_status = ctx->vtable->vaSyncSurface(ctx, render_target);
    VA_TRACE_LOG(va_TraceSyncSurface, dpy, render_target);
    VA_TRACE_RET(dpy, va_status);
    VA_TRACE_V(dpy, SYNC_SURFACE, TRACE_END, va_status);

    return va_status;
}

VAStatus vaSyncSurface2(
    VADisplay dpy,
    VASurfaceID surface,
    uint64_t timeout_ns
)
{
    VAStatus va_status;
    VADriverContextP ctx;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_VV(dpy, SYNC_SURFACE2, TRACE_BEGIN, surface, timeout_ns);
    if (ctx->vtable->vaSyncSurface2)
        va_status = ctx->vtable->vaSyncSurface2(ctx, surface, timeout_ns);
    else
        va_status = VA_STATUS_ERROR_UNIMPLEMENTED;
    VA_TRACE_LOG(va_TraceSyncSurface2, dpy, surface, timeout_ns);
    VA_TRACE_RET(dpy, va_status);
    VA_TRACE_V(dpy, SYNC_SURFACE2, TRACE_END, va_status);

    return va_status;
}

VAStatus vaQuerySurfaceStatus(
    VADisplay dpy,
    VASurfaceID render_target,
    VASurfaceStatus *status /* out */
)
{
    VAStatus va_status;
    VADriverContextP ctx;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    va_status = ctx->vtable->vaQuerySurfaceStatus(ctx, render_target, status);

    VA_TRACE_LOG(va_TraceQuerySurfaceStatus, dpy, render_target, status);
    VA_TRACE_RET(dpy, va_status);

    return va_status;
}

VAStatus vaQuerySurfaceError(
    VADisplay dpy,
    VASurfaceID surface,
    VAStatus error_status,
    void **error_info /*out*/
)
{
    VAStatus va_status;
    VADriverContextP ctx;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    va_status = ctx->vtable->vaQuerySurfaceError(ctx, surface, error_status, error_info);

    VA_TRACE_LOG(va_TraceQuerySurfaceError, dpy, surface, error_status, error_info);
    VA_TRACE_RET(dpy, va_status);

    return va_status;
}

VAStatus vaSyncBuffer(
    VADisplay dpy,
    VABufferID buf_id,
    uint64_t timeout_ns
)
{
    VAStatus va_status;
    VADriverContextP ctx;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    VA_TRACE_LOG(va_TraceSyncBuffer, dpy, buf_id, timeout_ns);

    if (ctx->vtable->vaSyncBuffer)
        va_status = ctx->vtable->vaSyncBuffer(ctx, buf_id, timeout_ns);
    else
        va_status = VA_STATUS_ERROR_UNIMPLEMENTED;
    VA_TRACE_RET(dpy, va_status);

    return va_status;
}

/* Get maximum number of image formats supported by the implementation */
int vaMaxNumImageFormats(
    VADisplay dpy
)
{
    if (!vaDisplayIsValid(dpy))
        return 0;

    return CTX(dpy)->max_image_formats;
}

VAStatus vaQueryImageFormats(
    VADisplay dpy,
    VAImageFormat *format_list, /* out */
    int *num_formats        /* out */
)
{
    VADriverContextP ctx;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    return ctx->vtable->vaQueryImageFormats(ctx, format_list, num_formats);
}

/*
 * The width and height fields returned in the VAImage structure may get
 * enlarged for some YUV formats. The size of the data buffer that needs
 * to be allocated will be given in the "data_size" field in VAImage.
 * Image data is not allocated by this function.  The client should
 * allocate the memory and fill in the VAImage structure's data field
 * after looking at "data_size" returned from the library.
 */
VAStatus vaCreateImage(
    VADisplay dpy,
    VAImageFormat *format,
    int width,
    int height,
    VAImage *image  /* out */
)
{
    VADriverContextP ctx;
    VAStatus va_status = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    va_status = ctx->vtable->vaCreateImage(ctx, format, width, height, image);
    VA_TRACE_RET(dpy, va_status);
    return va_status;
}

/*
 * Should call DestroyImage before destroying the surface it is bound to
 */
VAStatus vaDestroyImage(
    VADisplay dpy,
    VAImageID image
)
{
    VADriverContextP ctx;
    VAStatus va_status = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    va_status = ctx->vtable->vaDestroyImage(ctx, image);
    VA_TRACE_RET(dpy, va_status);
    return va_status;
}

VAStatus vaSetImagePalette(
    VADisplay dpy,
    VAImageID image,
    unsigned char *palette
)
{
    VADriverContextP ctx;
    VAStatus va_status = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    va_status = ctx->vtable->vaSetImagePalette(ctx, image, palette);
    VA_TRACE_RET(dpy, va_status);
    return va_status;
}

/*
 * Retrieve surface data into a VAImage
 * Image must be in a format supported by the implementation
 */
VAStatus vaGetImage(
    VADisplay dpy,
    VASurfaceID surface,
    int x,  /* coordinates of the upper left source pixel */
    int y,
    unsigned int width, /* width and height of the region */
    unsigned int height,
    VAImageID image
)
{
    VADriverContextP ctx;
    VAStatus va_status = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    va_status = ctx->vtable->vaGetImage(ctx, surface, x, y, width, height, image);
    VA_TRACE_RET(dpy, va_status);
    return va_status;
}

/*
 * Copy data from a VAImage to a surface
 * Image must be in a format supported by the implementation
 */
VAStatus vaPutImage(
    VADisplay dpy,
    VASurfaceID surface,
    VAImageID image,
    int src_x,
    int src_y,
    unsigned int src_width,
    unsigned int src_height,
    int dest_x,
    int dest_y,
    unsigned int dest_width,
    unsigned int dest_height
)
{
    VADriverContextP ctx;
    VAStatus va_status = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    va_status = ctx->vtable->vaPutImage(ctx, surface, image, src_x, src_y, src_width, src_height, dest_x, dest_y, dest_width, dest_height);
    VA_TRACE_RET(dpy, va_status);
    return va_status;
}

/*
 * Derive an VAImage from an existing surface.
 * This interface will derive a VAImage and corresponding image buffer from
 * an existing VA Surface. The image buffer can then be mapped/unmapped for
 * direct CPU access. This operation is only possible on implementations with
 * direct rendering capabilities and internal surface formats that can be
 * represented with a VAImage. When the operation is not possible this interface
 * will return VA_STATUS_ERROR_OPERATION_FAILED. Clients should then fall back
 * to using vaCreateImage + vaPutImage to accomplish the same task in an
 * indirect manner.
 *
 * Implementations should only return success when the resulting image buffer
 * would be useable with vaMap/Unmap.
 *
 * When directly accessing a surface special care must be taken to insure
 * proper synchronization with the graphics hardware. Clients should call
 * vaQuerySurfaceStatus to insure that a surface is not the target of concurrent
 * rendering or currently being displayed by an overlay.
 *
 * Additionally nothing about the contents of a surface should be assumed
 * following a vaPutSurface. Implementations are free to modify the surface for
 * scaling or subpicture blending within a call to vaPutImage.
 *
 * Calls to vaPutImage or vaGetImage using the same surface from which the image
 * has been derived will return VA_STATUS_ERROR_SURFACE_BUSY. vaPutImage or
 * vaGetImage with other surfaces is supported.
 *
 * An image created with vaDeriveImage should be freed with vaDestroyImage. The
 * image and image buffer structures will be destroyed; however, the underlying
 * surface will remain unchanged until freed with vaDestroySurfaces.
 */
VAStatus vaDeriveImage(
    VADisplay dpy,
    VASurfaceID surface,
    VAImage *image  /* out */
)
{
    VADriverContextP ctx;
    VAStatus va_status = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    va_status = ctx->vtable->vaDeriveImage(ctx, surface, image);
    VA_TRACE_RET(dpy, va_status);
    return va_status;
}


/* Get maximum number of subpicture formats supported by the implementation */
int vaMaxNumSubpictureFormats(
    VADisplay dpy
)
{
    if (!vaDisplayIsValid(dpy))
        return 0;

    return CTX(dpy)->max_subpic_formats;
}

/*
 * Query supported subpicture formats
 * The caller must provide a "format_list" array that can hold at
 * least vaMaxNumSubpictureFormats() entries. The flags arrary holds the flag
 * for each format to indicate additional capabilities for that format. The actual
 * number of formats returned in "format_list" is returned in "num_formats".
 */
VAStatus vaQuerySubpictureFormats(
    VADisplay dpy,
    VAImageFormat *format_list, /* out */
    unsigned int *flags,    /* out */
    unsigned int *num_formats   /* out */
)
{
    VADriverContextP ctx;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    return ctx->vtable->vaQuerySubpictureFormats(ctx, format_list, flags, num_formats);
}

/*
 * Subpictures are created with an image associated.
 */
VAStatus vaCreateSubpicture(
    VADisplay dpy,
    VAImageID image,
    VASubpictureID *subpicture  /* out */
)
{
    VADriverContextP ctx;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    return ctx->vtable->vaCreateSubpicture(ctx, image, subpicture);
}

/*
 * Destroy the subpicture before destroying the image it is assocated to
 */
VAStatus vaDestroySubpicture(
    VADisplay dpy,
    VASubpictureID subpicture
)
{
    VADriverContextP ctx;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    return ctx->vtable->vaDestroySubpicture(ctx, subpicture);
}

VAStatus vaSetSubpictureImage(
    VADisplay dpy,
    VASubpictureID subpicture,
    VAImageID image
)
{
    VADriverContextP ctx;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    return ctx->vtable->vaSetSubpictureImage(ctx, subpicture, image);
}


/*
 * If chromakey is enabled, then the area where the source value falls within
 * the chromakey [min, max] range is transparent
 */
VAStatus vaSetSubpictureChromakey(
    VADisplay dpy,
    VASubpictureID subpicture,
    unsigned int chromakey_min,
    unsigned int chromakey_max,
    unsigned int chromakey_mask
)
{
    VADriverContextP ctx;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    return ctx->vtable->vaSetSubpictureChromakey(ctx, subpicture, chromakey_min, chromakey_max, chromakey_mask);
}


/*
 * Global alpha value is between 0 and 1. A value of 1 means fully opaque and
 * a value of 0 means fully transparent. If per-pixel alpha is also specified then
 * the overall alpha is per-pixel alpha multiplied by the global alpha
 */
VAStatus vaSetSubpictureGlobalAlpha(
    VADisplay dpy,
    VASubpictureID subpicture,
    float global_alpha
)
{
    VADriverContextP ctx;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    return ctx->vtable->vaSetSubpictureGlobalAlpha(ctx, subpicture, global_alpha);
}

/*
  vaAssociateSubpicture associates the subpicture with the target_surface.
  It defines the region mapping between the subpicture and the target
  surface through source and destination rectangles (with the same width and height).
  Both will be displayed at the next call to vaPutSurface.  Additional
  associations before the call to vaPutSurface simply overrides the association.
*/
VAStatus vaAssociateSubpicture(
    VADisplay dpy,
    VASubpictureID subpicture,
    VASurfaceID *target_surfaces,
    int num_surfaces,
    short src_x, /* upper left offset in subpicture */
    short src_y,
    unsigned short src_width,
    unsigned short src_height,
    short dest_x, /* upper left offset in surface */
    short dest_y,
    unsigned short dest_width,
    unsigned short dest_height,
    /*
     * whether to enable chroma-keying or global-alpha
     * see VA_SUBPICTURE_XXX values
     */
    unsigned int flags
)
{
    VADriverContextP ctx;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    return ctx->vtable->vaAssociateSubpicture(ctx, subpicture, target_surfaces, num_surfaces, src_x, src_y, src_width, src_height, dest_x, dest_y, dest_width, dest_height, flags);
}

/*
 * vaDeassociateSubpicture removes the association of the subpicture with target_surfaces.
 */
VAStatus vaDeassociateSubpicture(
    VADisplay dpy,
    VASubpictureID subpicture,
    VASurfaceID *target_surfaces,
    int num_surfaces
)
{
    VADriverContextP ctx;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    return ctx->vtable->vaDeassociateSubpicture(ctx, subpicture, target_surfaces, num_surfaces);
}


/* Get maximum number of display attributes supported by the implementation */
int vaMaxNumDisplayAttributes(
    VADisplay dpy
)
{
    int tmp;

    if (!vaDisplayIsValid(dpy))
        return 0;

    tmp = CTX(dpy)->max_display_attributes;

    VA_TRACE_LOG(va_TraceMaxNumDisplayAttributes, dpy, tmp);

    return tmp;
}

/*
 * Query display attributes
 * The caller must provide a "attr_list" array that can hold at
 * least vaMaxNumDisplayAttributes() entries. The actual number of attributes
 * returned in "attr_list" is returned in "num_attributes".
 */
VAStatus vaQueryDisplayAttributes(
    VADisplay dpy,
    VADisplayAttribute *attr_list,  /* out */
    int *num_attributes         /* out */
)
{
    VADriverContextP ctx;
    VAStatus va_status;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);
    va_status = ctx->vtable->vaQueryDisplayAttributes(ctx, attr_list, num_attributes);

    VA_TRACE_LOG(va_TraceQueryDisplayAttributes, dpy, attr_list, num_attributes);
    VA_TRACE_RET(dpy, va_status);

    return va_status;

}

/*
 * Get display attributes
 * This function returns the current attribute values in "attr_list".
 * Only attributes returned with VA_DISPLAY_ATTRIB_GETTABLE set in the "flags" field
 * from vaQueryDisplayAttributes() can have their values retrieved.
 */
VAStatus vaGetDisplayAttributes(
    VADisplay dpy,
    VADisplayAttribute *attr_list,  /* in/out */
    int num_attributes
)
{
    VADriverContextP ctx;
    VAStatus va_status;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);
    va_status = ctx->vtable->vaGetDisplayAttributes(ctx, attr_list, num_attributes);

    VA_TRACE_LOG(va_TraceGetDisplayAttributes, dpy, attr_list, num_attributes);
    VA_TRACE_RET(dpy, va_status);

    return va_status;
}

/*
 * Set display attributes
 * Only attributes returned with VA_DISPLAY_ATTRIB_SETTABLE set in the "flags" field
 * from vaQueryDisplayAttributes() can be set.  If the attribute is not settable or
 * the value is out of range, the function returns VA_STATUS_ERROR_ATTR_NOT_SUPPORTED
 */
VAStatus vaSetDisplayAttributes(
    VADisplay dpy,
    VADisplayAttribute *attr_list,
    int num_attributes
)
{
    VADriverContextP ctx;
    VAStatus va_status;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    va_status = ctx->vtable->vaSetDisplayAttributes(ctx, attr_list, num_attributes);
    VA_TRACE_LOG(va_TraceSetDisplayAttributes, dpy, attr_list, num_attributes);
    VA_TRACE_RET(dpy, va_status);

    return va_status;
}

VAStatus vaLockSurface(VADisplay dpy,
                       VASurfaceID surface,
                       unsigned int *fourcc, /* following are output argument */
                       unsigned int *luma_stride,
                       unsigned int *chroma_u_stride,
                       unsigned int *chroma_v_stride,
                       unsigned int *luma_offset,
                       unsigned int *chroma_u_offset,
                       unsigned int *chroma_v_offset,
                       unsigned int *buffer_name,
                       void **buffer
                      )
{
    VADriverContextP ctx;
    VAStatus va_status = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    va_status = ctx->vtable->vaLockSurface(ctx, surface, fourcc, luma_stride, chroma_u_stride, chroma_v_stride, luma_offset, chroma_u_offset, chroma_v_offset, buffer_name, buffer);
    VA_TRACE_RET(dpy, va_status);

    return va_status;
}


VAStatus vaUnlockSurface(VADisplay dpy,
                         VASurfaceID surface
                        )
{
    VADriverContextP ctx;
    VAStatus va_status = VA_STATUS_SUCCESS;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    va_status = ctx->vtable->vaUnlockSurface(ctx, surface);
    VA_TRACE_RET(dpy, va_status);

    return va_status;
}

/* Video Processing */
#define VA_VPP_INIT_CONTEXT(ctx, dpy) do {              \
        CHECK_DISPLAY(dpy);                             \
        ctx = CTX(dpy);                                 \
        if (!ctx)                                       \
            return VA_STATUS_ERROR_INVALID_DISPLAY;     \
    } while (0)

#define VA_VPP_INVOKE(dpy, func, args) do {             \
        if (!ctx->vtable_vpp->va##func)                 \
            return VA_STATUS_ERROR_UNIMPLEMENTED;       \
        status = ctx->vtable_vpp->va##func args;        \
    } while (0)

VAStatus
vaQueryVideoProcFilters(
    VADisplay           dpy,
    VAContextID         context,
    VAProcFilterType   *filters,
    unsigned int       *num_filters
)
{
    VADriverContextP ctx;
    VAStatus status;

    VA_VPP_INIT_CONTEXT(ctx, dpy);
    VA_VPP_INVOKE(
        ctx,
        QueryVideoProcFilters,
        (ctx, context, filters, num_filters)
    );
    VA_TRACE_RET(dpy, status);

    return status;
}

VAStatus
vaQueryVideoProcFilterCaps(
    VADisplay           dpy,
    VAContextID         context,
    VAProcFilterType    type,
    void               *filter_caps,
    unsigned int       *num_filter_caps
)
{
    VADriverContextP ctx;
    VAStatus status;

    VA_VPP_INIT_CONTEXT(ctx, dpy);
    VA_VPP_INVOKE(
        ctx,
        QueryVideoProcFilterCaps,
        (ctx, context, type, filter_caps, num_filter_caps)
    );
    VA_TRACE_RET(dpy, status);
    return status;
}

VAStatus
vaQueryVideoProcPipelineCaps(
    VADisplay           dpy,
    VAContextID         context,
    VABufferID         *filters,
    unsigned int        num_filters,
    VAProcPipelineCaps *pipeline_caps
)
{
    VADriverContextP ctx;
    VAStatus status;

    VA_VPP_INIT_CONTEXT(ctx, dpy);
    VA_VPP_INVOKE(
        ctx,
        QueryVideoProcPipelineCaps,
        (ctx, context, filters, num_filters, pipeline_caps)
    );
    VA_TRACE_RET(dpy, status);
    return status;
}

VAStatus
vaCopy(
    VADisplay         dpy,
    VACopyObject      *dst,
    VACopyObject      *src,
    VACopyOption      option
)
{
    VAStatus va_status;
    VADriverContextP ctx;
    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    if (ctx->vtable->vaCopy  == NULL)
        va_status = VA_STATUS_ERROR_UNIMPLEMENTED;
    else
        va_status = ctx->vtable->vaCopy(ctx, dst, src, option);
    return va_status;
}

/* Protected content */
#define VA_PROT_INIT_CONTEXT(ctx, dpy) do {              \
        CHECK_DISPLAY(dpy);                             \
        ctx = CTX(dpy);                                 \
        if (!ctx)                                       \
            return VA_STATUS_ERROR_INVALID_DISPLAY;     \
    } while (0)

#define VA_PROT_INVOKE(dpy, func, args) do {             \
        if (!ctx->vtable_prot->va##func)                 \
            return VA_STATUS_ERROR_UNIMPLEMENTED;       \
        status = ctx->vtable_prot->va##func args;        \
    } while (0)

VAStatus vaCreateProtectedSession(
    VADisplay dpy,
    VAConfigID config_id,
    VAProtectedSessionID *protected_session
)
{
    VADriverContextP ctx;
    VAStatus status;

    VA_PROT_INIT_CONTEXT(ctx, dpy);
    VA_PROT_INVOKE(
        ctx,
        CreateProtectedSession,
        (ctx, config_id, protected_session)
    );
    VA_TRACE_RET(dpy, status);

    return status;
}

VAStatus vaDestroyProtectedSession(
    VADisplay dpy,
    VAProtectedSessionID protected_session
)
{
    VADriverContextP ctx;
    VAStatus status;

    VA_PROT_INIT_CONTEXT(ctx, dpy);
    VA_PROT_INVOKE(
        ctx,
        DestroyProtectedSession,
        (ctx, protected_session)
    );
    VA_TRACE_RET(dpy, status);

    return status;
}

VAStatus vaAttachProtectedSession(
    VADisplay dpy,
    VAContextID context,
    VAProtectedSessionID protected_session
)
{
    VADriverContextP ctx;
    VAStatus status;

    VA_PROT_INIT_CONTEXT(ctx, dpy);
    VA_PROT_INVOKE(
        ctx,
        AttachProtectedSession,
        (ctx, context, protected_session)
    );
    VA_TRACE_RET(dpy, status);

    return status;
}

VAStatus vaDetachProtectedSession(
    VADisplay dpy,
    VAContextID context
)
{
    VADriverContextP ctx;
    VAStatus status;

    VA_PROT_INIT_CONTEXT(ctx, dpy);
    VA_PROT_INVOKE(
        ctx,
        DetachProtectedSession,
        (ctx, context)
    );
    VA_TRACE_RET(dpy, status);

    return status;
}

VAStatus vaProtectedSessionExecute(
    VADisplay dpy,
    VAProtectedSessionID protected_session,
    VABufferID data
)
{
    VADriverContextP ctx;
    VAStatus status;

    VA_PROT_INIT_CONTEXT(ctx, dpy);
    VA_PROT_INVOKE(
        ctx,
        ProtectedSessionExecute,
        (ctx, protected_session, data)
    );
    VA_TRACE_RET(dpy, status);

    return status;
}

