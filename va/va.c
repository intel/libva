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
#include "va_backend_vpp.h"
#include "va_trace.h"
#include "va_fool.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#ifdef ANDROID
#include <cutils/log.h>
/* support versions < JellyBean */
#ifndef ALOGE
#define ALOGE LOGE
#endif
#ifndef ALOGI
#define ALOGI LOGI
#endif
#endif

#define DRIVER_EXTENSION	"_drv_video.so"

#define CTX(dpy) (((VADisplayContextP)dpy)->pDriverContext)
#define CHECK_DISPLAY(dpy) if( !vaDisplayIsValid(dpy) ) { return VA_STATUS_ERROR_INVALID_DISPLAY; }

#define ASSERT		assert
#define CHECK_VTABLE(s, ctx, func) if (!va_checkVtable(ctx->vtable->va##func, #func)) s = VA_STATUS_ERROR_UNKNOWN;
#define CHECK_MAXIMUM(s, ctx, var) if (!va_checkMaximum(ctx->max_##var, #var)) s = VA_STATUS_ERROR_UNKNOWN;
#define CHECK_STRING(s, ctx, var) if (!va_checkString(ctx->str_##var, #var)) s = VA_STATUS_ERROR_UNKNOWN;

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
    FILE *fp=NULL;

    if (env == NULL)
        return 1;
    
    fp = fopen("/etc/libva.conf", "r");
    while (fp && (fgets(oneline, 1024, fp) != NULL)) {
        if (strlen(oneline) == 1)
            continue;
        token = strtok_r(oneline, "=\n", &saveptr);
        value = strtok_r(NULL, "=\n", &saveptr);

        if (NULL == token || NULL == value)
            continue;

        if (strcmp(token, env) == 0) {
            if (env_value) {
                strncpy(env_value,value, 1024);
                env_value[1023] = '\0';
            }

            fclose(fp);

            return 0;
        }
    }
    if (fp)
        fclose(fp);

    /* no setting in config file, use env setting */
    value = getenv(env);
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
    return pDisplayContext && (pDisplayContext->vadpy_magic == VA_DISPLAY_MAGIC) && pDisplayContext->vaIsValid(pDisplayContext);
}

static void default_log_error(const char *buffer)
{
# ifdef ANDROID
    ALOGE("%s", buffer);
# else
    fprintf(stderr, "libva error: %s", buffer);
# endif
}

static void default_log_info(const char *buffer)
{
# ifdef ANDROID
    ALOGI("%s", buffer);
# else
    fprintf(stderr, "libva info: %s", buffer);
# endif
}

static vaMessageCallback va_log_error = default_log_error;
static vaMessageCallback va_log_info = default_log_info;

/**
 * Set the callback for error messages, or NULL for no logging.
 * Returns the previous one, or NULL if it was disabled.
 */
vaMessageCallback vaSetErrorCallback(vaMessageCallback callback)
{
    vaMessageCallback old_callback = va_log_error;
    va_log_error = callback;
    return old_callback;
}

/**
 * Set the callback for info messages, or NULL for no logging.
 * Returns the previous one, or NULL if it was disabled.
 */
vaMessageCallback vaSetInfoCallback(vaMessageCallback callback)
{
    vaMessageCallback old_callback = va_log_info;
    va_log_info = callback;
    return old_callback;
}

void va_MessagingInit()
{
#if ENABLE_VA_MESSAGING
    char env_value[1024];

    if (va_parseConfig("LIBVA_MESSAGING_LEVEL", &env_value[0]) == 0) {
        if (strcmp(env_value, "0") == 0) {
            vaSetInfoCallback(NULL);
            vaSetErrorCallback(NULL);
        }

        if (strcmp(env_value, "1") == 0) {
            vaSetInfoCallback(NULL);
        }
    }
#endif
}

void va_errorMessage(const char *msg, ...)
{
#if ENABLE_VA_MESSAGING
    char buf[512], *dynbuf;
    va_list args;
    int n, len;

    if (va_log_error == NULL)
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
            va_log_error(dynbuf);
        free(dynbuf);
    }
    else if (len > 0)
        va_log_error(buf);
#endif
}

void va_infoMessage(const char *msg, ...)
{
#if ENABLE_VA_MESSAGING
    char buf[512], *dynbuf;
    va_list args;
    int n, len;

    if (va_log_info == NULL)
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
            va_log_info(dynbuf);
        free(dynbuf);
    }
    else if (len > 0)
        va_log_info(buf);
#endif
}

static bool va_checkVtable(void *ptr, char *function)
{
    if (!ptr) {
        va_errorMessage("No valid vtable entry for va%s\n", function);
        return false;
    }
    return true;
}

static bool va_checkMaximum(int value, char *variable)
{
    if (!value) {
        va_errorMessage("Failed to define max_%s in init\n", variable);
        return false;
    }
    return true;
}

static bool va_checkString(const char* value, char *variable)
{
    if (!value) {
        va_errorMessage("Failed to define str_%s in init\n", variable);
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

static VAStatus va_getDriverName(VADisplay dpy, char **driver_name)
{
    VADisplayContextP pDisplayContext = (VADisplayContextP)dpy;

    return pDisplayContext->vaGetDriverName(pDisplayContext, driver_name);
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
        search_path = getenv("LIBVA_DRIVERS_PATH");
    if (!search_path)
        search_path = VA_DRIVERS_PATH;

    search_path = strdup((const char *)search_path);
    driver_dir = strtok_r(search_path, ":", &saveptr);
    while (driver_dir) {
        void *handle = NULL;
        char *driver_path = (char *) malloc( strlen(driver_dir) +
                                             strlen(driver_name) +
                                             strlen(DRIVER_EXTENSION) + 2 );
        if (!driver_path) {
            va_errorMessage("%s L%d Out of memory!n",
                                __FUNCTION__, __LINE__);
            free(search_path);    
            return VA_STATUS_ERROR_ALLOCATION_FAILED;
        }

        strncpy( driver_path, driver_dir, strlen(driver_dir) + 1);
        strncat( driver_path, "/", strlen("/") );
        strncat( driver_path, driver_name, strlen(driver_name) );
        strncat( driver_path, DRIVER_EXTENSION, strlen(DRIVER_EXTENSION) );
        
        va_infoMessage("Trying to open %s\n", driver_path);
#ifndef ANDROID
        handle = dlopen( driver_path, RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE );
#else
        handle = dlopen( driver_path, RTLD_NOW| RTLD_GLOBAL);
#endif
        if (!handle) {
            /* Don't give errors for non-existing files */
            if (0 == access( driver_path, F_OK))
                va_errorMessage("dlopen of %s failed: %s\n", driver_path, dlerror());
        } else {
            VADriverInit init_func = NULL;
            char init_func_s[256];
            int i;

            static const struct {
                int major;
                int minor;
            } compatible_versions[] = {
                { VA_MAJOR_VERSION, VA_MINOR_VERSION },
                { -1, }
            };

            for (i = 0; compatible_versions[i].major >= 0; i++) {
                if (va_getDriverInitName(init_func_s, sizeof(init_func_s),
                                         compatible_versions[i].major,
                                         compatible_versions[i].minor)) {
                    init_func = (VADriverInit)dlsym(handle, init_func_s);
                    if (init_func) {
                        va_infoMessage("Found init function %s\n", init_func_s);
                        break;
                    }
                }
            }

            if (compatible_versions[i].major < 0) {
                va_errorMessage("%s has no function %s\n",
                                driver_path, init_func_s);
                dlclose(handle);
            } else {
                struct VADriverVTable *vtable = ctx->vtable;
                struct VADriverVTableVPP *vtable_vpp = ctx->vtable_vpp;

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

                if (init_func && VA_STATUS_SUCCESS == vaStatus)
                    vaStatus = (*init_func)(ctx);

                if (VA_STATUS_SUCCESS == vaStatus) {
                    CHECK_MAXIMUM(vaStatus, ctx, profiles);
                    CHECK_MAXIMUM(vaStatus, ctx, entrypoints);
                    CHECK_MAXIMUM(vaStatus, ctx, attributes);
                    CHECK_MAXIMUM(vaStatus, ctx, image_formats);
                    CHECK_MAXIMUM(vaStatus, ctx, subpic_formats);
                    CHECK_MAXIMUM(vaStatus, ctx, display_attributes);
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
                    CHECK_VTABLE(vaStatus, ctx, PutSurface);
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
                    va_errorMessage("%s init failed\n", driver_path);
                    dlclose(handle);
                }
                if (VA_STATUS_SUCCESS == vaStatus)
                    ctx->handle = handle;
                free(driver_path);
                break;
            }
        }
        free(driver_path);
        
        driver_dir = strtok_r(NULL, ":", &saveptr);
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
    switch(error_status) {
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
        case VA_STATUS_ERROR_INVALID_VALUE:
            return "an invalid/unsupported value was supplied";
        case VA_STATUS_ERROR_UNSUPPORTED_FILTER:
            return "the requested filter is not supported";
        case VA_STATUS_ERROR_INVALID_FILTER_CHAIN:
            return "an invalid filter chain was supplied";
        case VA_STATUS_ERROR_UNKNOWN:
            return "unknown libva error";
    }
    return "unknown libva error / description missing";
}

const static char *prefer_driver_list[4] = {
    "i965",
    "hybrid",
    "pvr",
    "iHD",
};

VAStatus vaSetDriverName(
    VADisplay dpy,
    char *driver_name
)
{
    VADriverContextP ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    char *override_driver_name = NULL;
    int i, found;
    ctx = CTX(dpy);

    if (geteuid() != getuid()) {
        vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
        va_errorMessage("no permission to vaSetDriverName\n");
        return vaStatus;
    }

    if (strlen(driver_name) == 0 || strlen(driver_name) >=256) {
        vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        va_errorMessage("vaSetDriverName returns %s\n",
                         vaErrorStr(vaStatus));
        return vaStatus;
    }

    found = 0;
    for (i = 0; i < sizeof(prefer_driver_list) / sizeof(char *); i++) {
        if (strlen(prefer_driver_list[i]) != strlen(driver_name))
            continue;
        if (!strncmp(prefer_driver_list[i], driver_name, strlen(driver_name))) {
            found = 1;
            break;
        }
    }

    if (!found) {
        vaStatus = VA_STATUS_ERROR_INVALID_PARAMETER;
        va_errorMessage("vaSetDriverName returns %s. Incorrect parameter\n",
                         vaErrorStr(vaStatus));
        return vaStatus;
    }

    override_driver_name = strdup(driver_name);

    if (!override_driver_name) {
        vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
        va_errorMessage("vaSetDriverName returns %s. Out of Memory\n",
                         vaErrorStr(vaStatus));
        return vaStatus;
    }

    ctx->override_driver_name = override_driver_name;
    return VA_STATUS_SUCCESS;
}

VAStatus vaInitialize (
    VADisplay dpy,
    int *major_version,	 /* out */
    int *minor_version 	 /* out */
)
{
    const char *driver_name_env = NULL;
    char *driver_name = NULL;
    VAStatus vaStatus;
    VADriverContextP ctx;

    CHECK_DISPLAY(dpy);

    ctx = CTX(dpy);

    va_TraceInit(dpy);

    va_FoolInit(dpy);

    va_MessagingInit();

    va_infoMessage("VA-API version %s\n", VA_VERSION_S);

    vaStatus = va_getDriverName(dpy, &driver_name);

    if (!ctx->override_driver_name) {
        va_infoMessage("va_getDriverName() returns %d\n", vaStatus);

        driver_name_env = getenv("LIBVA_DRIVER_NAME");
    } else if (vaStatus == VA_STATUS_SUCCESS) {
        if (driver_name)
            free(driver_name);

        driver_name = strdup(ctx->override_driver_name);
        if (!driver_name) {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            va_errorMessage("vaInitialize() failed with %s, out of memory\n",
                        vaErrorStr(vaStatus));
            return vaStatus;
        }
        va_infoMessage("User requested driver '%s'\n", driver_name);
    }

    if (driver_name_env && (geteuid() == getuid())) {
        /* Don't allow setuid apps to use LIBVA_DRIVER_NAME */
        if (driver_name) /* memory is allocated in va_getDriverName */
            free(driver_name);
        
        driver_name = strdup(driver_name_env);
        vaStatus = VA_STATUS_SUCCESS;
        va_infoMessage("User requested driver '%s'\n", driver_name);
    }

    if ((VA_STATUS_SUCCESS == vaStatus) && (driver_name != NULL)) {
        vaStatus = va_openDriver(dpy, driver_name);
        va_infoMessage("va_openDriver() returns %d\n", vaStatus);

        *major_version = VA_MAJOR_VERSION;
        *minor_version = VA_MINOR_VERSION;
    } else
        va_errorMessage("va_getDriverName() failed with %s,driver_name=%s\n",
                        vaErrorStr(vaStatus), driver_name);

    if (driver_name)
        free(driver_name);
    
    VA_TRACE_LOG(va_TraceInitialize, dpy, major_version, minor_version);

    return vaStatus;
}


/*
 * After this call, all library internal resources will be cleaned up
 */ 
VAStatus vaTerminate (
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

  if (old_ctx->override_driver_name) {
      free(old_ctx->override_driver_name);
      old_ctx->override_driver_name = NULL;
  }

  VA_TRACE_LOG(va_TraceTerminate, dpy);

  va_TraceEnd(dpy);

  va_FoolEnd(dpy);

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
const char *vaQueryVendorString (
    VADisplay dpy
)
{
  if (!vaDisplayIsValid(dpy))
      return NULL;
  
  return CTX(dpy)->str_vendor;
}


/* Get maximum number of profiles supported by the implementation */
int vaMaxNumProfiles (
    VADisplay dpy
)
{
  if (!vaDisplayIsValid(dpy))
      return 0;
  
  return CTX(dpy)->max_profiles;
}

/* Get maximum number of entrypoints supported by the implementation */
int vaMaxNumEntrypoints (
    VADisplay dpy
)
{
  if (!vaDisplayIsValid(dpy))
      return 0;
  
  return CTX(dpy)->max_entrypoints;
}


/* Get maximum number of attributs supported by the implementation */
int vaMaxNumConfigAttributes (
    VADisplay dpy
)
{
  if (!vaDisplayIsValid(dpy))
      return 0;
  
  return CTX(dpy)->max_attributes;
}

VAStatus vaQueryConfigEntrypoints (
    VADisplay dpy,
    VAProfile profile,
    VAEntrypoint *entrypoints,	/* out */
    int *num_entrypoints	/* out */
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaQueryConfigEntrypoints ( ctx, profile, entrypoints, num_entrypoints);
}

VAStatus vaGetConfigAttributes (
    VADisplay dpy,
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list, /* in/out */
    int num_attribs
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaGetConfigAttributes ( ctx, profile, entrypoint, attrib_list, num_attribs );
}

VAStatus vaQueryConfigProfiles (
    VADisplay dpy,
    VAProfile *profile_list,	/* out */
    int *num_profiles		/* out */
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaQueryConfigProfiles ( ctx, profile_list, num_profiles );
}

VAStatus vaCreateConfig (
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

  vaStatus = ctx->vtable->vaCreateConfig ( ctx, profile, entrypoint, attrib_list, num_attribs, config_id );

  /* record the current entrypoint for further trace/fool determination */
  VA_TRACE_ALL(va_TraceCreateConfig, dpy, profile, entrypoint, attrib_list, num_attribs, config_id);
  VA_FOOL_FUNC(va_FoolCreateConfig, dpy, profile, entrypoint, attrib_list, num_attribs, config_id);
  
  return vaStatus;
}

VAStatus vaDestroyConfig (
    VADisplay dpy,
    VAConfigID config_id
)
{
  VADriverContextP ctx;
  VAStatus vaStatus = VA_STATUS_SUCCESS;

  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  vaStatus = ctx->vtable->vaDestroyConfig ( ctx, config_id );

  VA_TRACE_ALL(va_TraceDestroyConfig, dpy, config_id);

  return vaStatus;
}

VAStatus vaQueryConfigAttributes (
    VADisplay dpy,
    VAConfigID config_id, 
    VAProfile *profile, 	/* out */
    VAEntrypoint *entrypoint, 	/* out */
    VAConfigAttrib *attrib_list,/* out */
    int *num_attribs		/* out */
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaQueryConfigAttributes( ctx, config_id, profile, entrypoint, attrib_list, num_attribs);
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
        { VASurfaceAttribNone, }
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
        attrib->flags = VA_SURFACE_ATTRIB_GETTABLE|VA_SURFACE_ATTRIB_SETTABLE;
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

    if (!ctx->vtable->vaQuerySurfaceAttributes)
        vaStatus = va_impl_query_surface_attributes(ctx, config,
                                                    attrib_list, num_attribs);
    else
        vaStatus = ctx->vtable->vaQuerySurfaceAttributes(ctx, config,
                                                         attrib_list, num_attribs);

    VA_TRACE_LOG(va_TraceQuerySurfaceAttributes, dpy, config, attrib_list, num_attribs);

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

    return vaStatus;
}


VAStatus vaDestroySurfaces (
    VADisplay dpy,
    VASurfaceID *surface_list,
    int num_surfaces
)
{
  VADriverContextP ctx;
  VAStatus vaStatus;
  
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  VA_TRACE_LOG(va_TraceDestroySurfaces,
               dpy, surface_list, num_surfaces);
  
  vaStatus = ctx->vtable->vaDestroySurfaces( ctx, surface_list, num_surfaces );
  
  return vaStatus;
}

VAStatus vaCreateContext (
    VADisplay dpy,
    VAConfigID config_id,
    int picture_width,
    int picture_height,
    int flag,
    VASurfaceID *render_targets,
    int num_render_targets,
    VAContextID *context		/* out */
)
{
  VADriverContextP ctx;
  VAStatus vaStatus;
  
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  vaStatus = ctx->vtable->vaCreateContext( ctx, config_id, picture_width, picture_height,
                                      flag, render_targets, num_render_targets, context );

  /* keep current encode/decode resoluton */
  VA_TRACE_ALL(va_TraceCreateContext, dpy, config_id, picture_width, picture_height, flag, render_targets, num_render_targets, context);

  return vaStatus;
}

VAStatus vaDestroyContext (
    VADisplay dpy,
    VAContextID context
)
{
  VADriverContextP ctx;
  VAStatus vaStatus;

  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  vaStatus = ctx->vtable->vaDestroyContext( ctx, context );

  VA_TRACE_ALL(va_TraceDestroyContext, dpy, context);

  return vaStatus;
}

VAStatus vaCreateBuffer (
    VADisplay dpy,
    VAContextID context,	/* in */
    VABufferType type,		/* in */
    unsigned int size,		/* in */
    unsigned int num_elements,	/* in */
    void *data,			/* in */
    VABufferID *buf_id		/* out */
)
{
  VADriverContextP ctx;
  VAStatus vaStatus;
  
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  VA_FOOL_FUNC(va_FoolCreateBuffer, dpy, context, type, size, num_elements, data, buf_id);

  vaStatus = ctx->vtable->vaCreateBuffer( ctx, context, type, size, num_elements, data, buf_id);

  VA_TRACE_LOG(va_TraceCreateBuffer,
               dpy, context, type, size, num_elements, data, buf_id);
  
  return vaStatus;
}

VAStatus vaBufferSetNumElements (
    VADisplay dpy,
    VABufferID buf_id,	/* in */
    unsigned int num_elements /* in */
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  VA_FOOL_FUNC(va_FoolCheckContinuity, dpy);
  
  return ctx->vtable->vaBufferSetNumElements( ctx, buf_id, num_elements );
}


VAStatus vaMapBuffer (
    VADisplay dpy,
    VABufferID buf_id,	/* in */
    void **pbuf 	/* out */
)
{
  VADriverContextP ctx;
  VAStatus va_status;
  
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);
  
  VA_FOOL_FUNC(va_FoolMapBuffer, dpy, buf_id, pbuf);
  
  va_status = ctx->vtable->vaMapBuffer( ctx, buf_id, pbuf );

  VA_TRACE_ALL(va_TraceMapBuffer, dpy, buf_id, pbuf);
  
  return va_status;
}

VAStatus vaUnmapBuffer (
    VADisplay dpy,
    VABufferID buf_id	/* in */
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  VA_FOOL_FUNC(va_FoolCheckContinuity, dpy);

  return ctx->vtable->vaUnmapBuffer( ctx, buf_id );
}

VAStatus vaDestroyBuffer (
    VADisplay dpy,
    VABufferID buffer_id
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  VA_FOOL_FUNC(va_FoolCheckContinuity, dpy);

  VA_TRACE_LOG(va_TraceDestroyBuffer,
               dpy, buffer_id);
  
  return ctx->vtable->vaDestroyBuffer( ctx, buffer_id );
}

VAStatus vaBufferInfo (
    VADisplay dpy,
    VAContextID context,	/* in */
    VABufferID buf_id,		/* in */
    VABufferType *type,		/* out */
    unsigned int *size,		/* out */
    unsigned int *num_elements	/* out */
)
{
  VADriverContextP ctx;
  
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  VA_FOOL_FUNC(va_FoolBufferInfo, dpy, buf_id, type, size, num_elements);
  
  return ctx->vtable->vaBufferInfo( ctx, buf_id, type, size, num_elements );
}

/* Locks buffer for external API usage */
VAStatus
vaAcquireBufferHandle(VADisplay dpy, VABufferID buf_id, VABufferInfo *buf_info)
{
    VADriverContextP ctx;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    if (!ctx->vtable->vaAcquireBufferHandle)
        return VA_STATUS_ERROR_UNIMPLEMENTED;
    return ctx->vtable->vaAcquireBufferHandle(ctx, buf_id, buf_info);
}

/* Unlocks buffer after usage from external API */
VAStatus
vaReleaseBufferHandle(VADisplay dpy, VABufferID buf_id)
{
    VADriverContextP ctx;

    CHECK_DISPLAY(dpy);
    ctx = CTX(dpy);

    if (!ctx->vtable->vaReleaseBufferHandle)
        return VA_STATUS_ERROR_UNIMPLEMENTED;
    return ctx->vtable->vaReleaseBufferHandle(ctx, buf_id);
}

VAStatus vaBeginPicture (
    VADisplay dpy,
    VAContextID context,
    VASurfaceID render_target
)
{
  VADriverContextP ctx;
  VAStatus va_status;

  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  VA_TRACE_ALL(va_TraceBeginPicture, dpy, context, render_target);
  VA_FOOL_FUNC(va_FoolCheckContinuity, dpy);
  
  va_status = ctx->vtable->vaBeginPicture( ctx, context, render_target );
  
  return va_status;
}

VAStatus vaRenderPicture (
    VADisplay dpy,
    VAContextID context,
    VABufferID *buffers,
    int num_buffers
)
{
  VADriverContextP ctx;

  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  VA_TRACE_LOG(va_TraceRenderPicture, dpy, context, buffers, num_buffers);
  VA_FOOL_FUNC(va_FoolCheckContinuity, dpy);

  return ctx->vtable->vaRenderPicture( ctx, context, buffers, num_buffers );
}

VAStatus vaEndPicture (
    VADisplay dpy,
    VAContextID context
)
{
  VAStatus va_status = VA_STATUS_SUCCESS;
  VADriverContextP ctx;

  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  VA_FOOL_FUNC(va_FoolCheckContinuity, dpy);

  va_status = ctx->vtable->vaEndPicture( ctx, context );

  /* dump surface content */
  VA_TRACE_ALL(va_TraceEndPicture, dpy, context, 1);

  return va_status;
}

VAStatus vaSyncSurface (
    VADisplay dpy,
    VASurfaceID render_target
)
{
  VAStatus va_status;
  VADriverContextP ctx;

  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  va_status = ctx->vtable->vaSyncSurface( ctx, render_target );
  VA_TRACE_LOG(va_TraceSyncSurface, dpy, render_target);

  return va_status;
}

VAStatus vaQuerySurfaceStatus (
    VADisplay dpy,
    VASurfaceID render_target,
    VASurfaceStatus *status	/* out */
)
{
  VAStatus va_status;
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  va_status = ctx->vtable->vaQuerySurfaceStatus( ctx, render_target, status );

  VA_TRACE_LOG(va_TraceQuerySurfaceStatus, dpy, render_target, status);

  return va_status;
}

VAStatus vaQuerySurfaceError (
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

  va_status = ctx->vtable->vaQuerySurfaceError( ctx, surface, error_status, error_info );

  VA_TRACE_LOG(va_TraceQuerySurfaceError, dpy, surface, error_status, error_info);

  return va_status;
}

/* Get maximum number of image formats supported by the implementation */
int vaMaxNumImageFormats (
    VADisplay dpy
)
{
  if (!vaDisplayIsValid(dpy))
      return 0;
  
  return CTX(dpy)->max_image_formats;
}

VAStatus vaQueryImageFormats (
    VADisplay dpy,
    VAImageFormat *format_list,	/* out */
    int *num_formats		/* out */
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaQueryImageFormats ( ctx, format_list, num_formats);
}

/* 
 * The width and height fields returned in the VAImage structure may get 
 * enlarged for some YUV formats. The size of the data buffer that needs
 * to be allocated will be given in the "data_size" field in VAImage.
 * Image data is not allocated by this function.  The client should
 * allocate the memory and fill in the VAImage structure's data field
 * after looking at "data_size" returned from the library.
 */
VAStatus vaCreateImage (
    VADisplay dpy,
    VAImageFormat *format,
    int width,
    int height,
    VAImage *image	/* out */
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaCreateImage ( ctx, format, width, height, image);
}

/*
 * Should call DestroyImage before destroying the surface it is bound to
 */
VAStatus vaDestroyImage (
    VADisplay dpy,
    VAImageID image
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaDestroyImage ( ctx, image);
}

VAStatus vaSetImagePalette (
    VADisplay dpy,
    VAImageID image,
    unsigned char *palette
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaSetImagePalette ( ctx, image, palette);
}

/*
 * Retrieve surface data into a VAImage
 * Image must be in a format supported by the implementation
 */
VAStatus vaGetImage (
    VADisplay dpy,
    VASurfaceID surface,
    int x,	/* coordinates of the upper left source pixel */
    int y,
    unsigned int width, /* width and height of the region */
    unsigned int height,
    VAImageID image
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaGetImage ( ctx, surface, x, y, width, height, image);
}

/*
 * Copy data from a VAImage to a surface
 * Image must be in a format supported by the implementation
 */
VAStatus vaPutImage (
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
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaPutImage ( ctx, surface, image, src_x, src_y, src_width, src_height, dest_x, dest_y, dest_width, dest_height );
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
VAStatus vaDeriveImage (
    VADisplay dpy,
    VASurfaceID surface,
    VAImage *image	/* out */
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaDeriveImage ( ctx, surface, image );
}


/* Get maximum number of subpicture formats supported by the implementation */
int vaMaxNumSubpictureFormats (
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
VAStatus vaQuerySubpictureFormats (
    VADisplay dpy,
    VAImageFormat *format_list,	/* out */
    unsigned int *flags,	/* out */
    unsigned int *num_formats	/* out */
)
{
  VADriverContextP ctx;

  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaQuerySubpictureFormats ( ctx, format_list, flags, num_formats);
}

/* 
 * Subpictures are created with an image associated. 
 */
VAStatus vaCreateSubpicture (
    VADisplay dpy,
    VAImageID image,
    VASubpictureID *subpicture	/* out */
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaCreateSubpicture ( ctx, image, subpicture );
}

/*
 * Destroy the subpicture before destroying the image it is assocated to
 */
VAStatus vaDestroySubpicture (
    VADisplay dpy,
    VASubpictureID subpicture
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaDestroySubpicture ( ctx, subpicture);
}

VAStatus vaSetSubpictureImage (
    VADisplay dpy,
    VASubpictureID subpicture,
    VAImageID image
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaSetSubpictureImage ( ctx, subpicture, image);
}


/*
 * If chromakey is enabled, then the area where the source value falls within
 * the chromakey [min, max] range is transparent
 */
VAStatus vaSetSubpictureChromakey (
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

  return ctx->vtable->vaSetSubpictureChromakey ( ctx, subpicture, chromakey_min, chromakey_max, chromakey_mask );
}


/*
 * Global alpha value is between 0 and 1. A value of 1 means fully opaque and 
 * a value of 0 means fully transparent. If per-pixel alpha is also specified then
 * the overall alpha is per-pixel alpha multiplied by the global alpha
 */
VAStatus vaSetSubpictureGlobalAlpha (
    VADisplay dpy,
    VASubpictureID subpicture,
    float global_alpha 
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaSetSubpictureGlobalAlpha ( ctx, subpicture, global_alpha );
}

/*
  vaAssociateSubpicture associates the subpicture with the target_surface.
  It defines the region mapping between the subpicture and the target 
  surface through source and destination rectangles (with the same width and height).
  Both will be displayed at the next call to vaPutSurface.  Additional
  associations before the call to vaPutSurface simply overrides the association.
*/
VAStatus vaAssociateSubpicture (
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

  return ctx->vtable->vaAssociateSubpicture ( ctx, subpicture, target_surfaces, num_surfaces, src_x, src_y, src_width, src_height, dest_x, dest_y, dest_width, dest_height, flags );
}

/*
 * vaDeassociateSubpicture removes the association of the subpicture with target_surfaces.
 */
VAStatus vaDeassociateSubpicture (
    VADisplay dpy,
    VASubpictureID subpicture,
    VASurfaceID *target_surfaces,
    int num_surfaces
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaDeassociateSubpicture ( ctx, subpicture, target_surfaces, num_surfaces );
}


/* Get maximum number of display attributes supported by the implementation */
int vaMaxNumDisplayAttributes (
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
VAStatus vaQueryDisplayAttributes (
    VADisplay dpy,
    VADisplayAttribute *attr_list,	/* out */
    int *num_attributes			/* out */
)
{
  VADriverContextP ctx;
  VAStatus va_status;
  
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);
  va_status = ctx->vtable->vaQueryDisplayAttributes ( ctx, attr_list, num_attributes );

  VA_TRACE_LOG(va_TraceQueryDisplayAttributes, dpy, attr_list, num_attributes);

  return va_status;
  
}

/* 
 * Get display attributes 
 * This function returns the current attribute values in "attr_list".
 * Only attributes returned with VA_DISPLAY_ATTRIB_GETTABLE set in the "flags" field
 * from vaQueryDisplayAttributes() can have their values retrieved.  
 */
VAStatus vaGetDisplayAttributes (
    VADisplay dpy,
    VADisplayAttribute *attr_list,	/* in/out */
    int num_attributes
)
{
  VADriverContextP ctx;
  VAStatus va_status;

  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);
  va_status = ctx->vtable->vaGetDisplayAttributes ( ctx, attr_list, num_attributes );

  VA_TRACE_LOG(va_TraceGetDisplayAttributes, dpy, attr_list, num_attributes);
  
  return va_status;
}

/* 
 * Set display attributes 
 * Only attributes returned with VA_DISPLAY_ATTRIB_SETTABLE set in the "flags" field
 * from vaQueryDisplayAttributes() can be set.  If the attribute is not settable or 
 * the value is out of range, the function returns VA_STATUS_ERROR_ATTR_NOT_SUPPORTED
 */
VAStatus vaSetDisplayAttributes (
    VADisplay dpy,
    VADisplayAttribute *attr_list,
    int num_attributes
)
{
  VADriverContextP ctx;
  VAStatus va_status;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  va_status = ctx->vtable->vaSetDisplayAttributes ( ctx, attr_list, num_attributes );
  VA_TRACE_LOG(va_TraceSetDisplayAttributes, dpy, attr_list, num_attributes);
  
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
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaLockSurface( ctx, surface, fourcc, luma_stride, chroma_u_stride, chroma_v_stride, luma_offset, chroma_u_offset, chroma_v_offset, buffer_name, buffer);
}


VAStatus vaUnlockSurface(VADisplay dpy,
    VASurfaceID surface
)
{
  VADriverContextP ctx;
  CHECK_DISPLAY(dpy);
  ctx = CTX(dpy);

  return ctx->vtable->vaUnlockSurface( ctx, surface );
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
    return status;
}
