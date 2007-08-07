/*
 * @COPYRIGHT@ Intel Confidential - Unreleased Software
 */

#include "X11/Xlib.h"
#include "va.h"
#include "va_backend.h"

#include "assert.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include "va_dri.h"

#define DEFAULT_DRIVER_DIR	"/usr/X11R6/lib/modules/dri"
#define DRIVER_EXTENSION	"_drv_video.so"
#define DRIVER_INIT_FUNC	"__vaDriverInit_0_19"

#define CTX(dpy) ((VADriverContextP) dpy );
#define ASSERT_CONTEXT(dpy) assert( vaDbgContextIsValid(dpy) )
#define ASSERT		assert
#define CHECK_VTABLE(s, ctx, func) if (!va_checkVtable(ctx->vtable.va##func, #func)) s = VA_STATUS_ERROR_UNKNOWN;
#define CHECK_MAXIMUM(s, ctx, var) if (!va_checkMaximum(ctx->max_##var, #var)) s = VA_STATUS_ERROR_UNKNOWN;

#define TRACE(func) if (va_debug_trace) va_infoMessage("[TR] %s\n", #func);

static VADriverContextP pDriverContexts = NULL;
static int va_debug_trace = 0;

static Bool vaDbgContextIsValid(VADriverContextP arg_ctx)
{
  VADriverContextP ctx = pDriverContexts;
  
  while (ctx)
  {
      if (ctx == arg_ctx)
      {
          return True;
      }
      ctx = ctx->pNext;
  }
  return False;
}

VADisplay vaGetDisplay (
    NativeDisplay native_dpy    /* implementation specific */
)
{
  VADisplay dpy = NULL;
  VADriverContextP ctx = pDriverContexts;
  
  while (ctx)
  {
      if (ctx->x11_dpy == (Display *)native_dpy)
      {
          dpy = (VADisplay) ctx;
          break;
      }
      ctx = ctx->pNext;
  }
  
  if (!dpy)
  {
      /* create new entry */
      ctx = (VADriverContextP) calloc(1, sizeof(struct VADriverContext));
      ctx->pNext = pDriverContexts;
      ctx->x11_dpy = (Display *) native_dpy;
      pDriverContexts = ctx;
      dpy = (VADisplay) ctx;
  }
  
  return dpy;
}

static void va_errorMessage(const char *msg, ...)
{
    va_list args;

    fprintf(stderr, "libva error: ");
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

static void va_infoMessage(const char *msg, ...)
{
    va_list args;

    fprintf(stderr, "libva: ");
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

static Bool va_checkVtable(void *ptr, char *function)
{
    if (!ptr)
    {
        va_errorMessage("No valid vtable entry for va%s\n", function);
        return False;
    }
    return True;
}

static Bool va_checkMaximum(int value, char *variable)
{
    if (!value)
    {
        va_errorMessage("Failed to define max_%s in init\n", variable);
        return False;
    }
    return True;
}

static VAStatus va_getDriverName(VADriverContextP ctx, char **driver_name)
{
    VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
    int direct_capable;
    int driver_major;
    int driver_minor;
    int driver_patch;
    Bool result = True;
    
    *driver_name = NULL;
    if (geteuid() == getuid())
    {
        /* don't allow setuid apps to use LIBVA_DRIVER_NAME */
        if (getenv("LIBVA_DRIVER_NAME"))
        {
            /* For easier debugging */
            *driver_name = strdup(getenv("LIBVA_DRIVER_NAME"));
            return VA_STATUS_SUCCESS;
        }
    }
    if (result)
    {
        result = VA_DRIQueryDirectRenderingCapable(ctx->x11_dpy, ctx->x11_screen, &direct_capable);
        if (!result)
        {
            va_errorMessage("VA_DRIQueryDirectRenderingCapable failed\n");
        }
    }
    if (result)
    {
        result = direct_capable;
        if (!result)
        {
            va_errorMessage("VA_DRIQueryDirectRenderingCapable returned false\n");
        }
    }
    if (result)
    {
        result = VA_DRIGetClientDriverName(ctx->x11_dpy, ctx->x11_screen, &driver_major, &driver_minor,
                                            &driver_patch, driver_name);
        if (!result)
        {
            va_errorMessage("VA_DRIGetClientDriverName returned false\n");
        }
    }
    if (result)
    {
        vaStatus = VA_STATUS_SUCCESS;
        va_infoMessage("VA_DRIGetClientDriverName: %d.%d.%d %s (screen %d)\n",
	     driver_major, driver_minor, driver_patch, *driver_name, ctx->x11_screen);
    }

    return vaStatus;
}

static VAStatus va_openDriver(VADriverContextP ctx, char *driver_name)
{
    VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
    char *search_path;
    char *saveptr;
    char *driver_dir;
    
    if (geteuid() == getuid())
    {
        /* don't allow setuid apps to use LIBVA_DRIVERS_PATH */
        search_path = getenv("LIBVA_DRIVERS_PATH");
        if (!search_path)
        {
            search_path = getenv("LIBGL_DRIVERS_PATH");
        }
    }
    if (!search_path)
    {
        search_path = DEFAULT_DRIVER_DIR;
    }

    search_path = strdup(search_path);
    driver_dir = strtok_r(search_path, ":", &saveptr);
    while(driver_dir)
    {
        void *handle = NULL;
        char *driver_path = (char *) malloc( strlen(driver_dir) +
                                             strlen(driver_name) +
                                             strlen(DRIVER_EXTENSION) + 2 );
        strcpy( driver_path, driver_dir );
        strcat( driver_path, "/" );
        strcat( driver_path, driver_name );
        strcat( driver_path, DRIVER_EXTENSION );
        
        va_infoMessage("Trying to open %s\n", driver_path);

        handle = dlopen( driver_path, RTLD_NOW | RTLD_GLOBAL );
        if (!handle)
        {
            /* Don't give errors for non-existing files */
            if (0 == access( driver_path, F_OK))
            {	
                va_errorMessage("dlopen of %s failed: %s\n", driver_path, dlerror());
            }
        }
        else
        {
            VADriverInit init_func;
            init_func = (VADriverInit) dlsym(handle, DRIVER_INIT_FUNC);
            if (!init_func)
            {
                va_errorMessage("%s has no function %s\n", driver_path, DRIVER_INIT_FUNC);
                dlclose(handle);
            }
            else
            {
                vaStatus = (*init_func)(ctx);

                if (VA_STATUS_SUCCESS == vaStatus)
                {
                    CHECK_MAXIMUM(vaStatus, ctx, profiles);
                    CHECK_MAXIMUM(vaStatus, ctx, entrypoints);
                    CHECK_MAXIMUM(vaStatus, ctx, attributes);
                    CHECK_VTABLE(vaStatus, ctx, Terminate);
                    CHECK_VTABLE(vaStatus, ctx, QueryConfigProfiles);
                    CHECK_VTABLE(vaStatus, ctx, QueryConfigEntrypoints);
                    CHECK_VTABLE(vaStatus, ctx, QueryConfigAttributes);
                    CHECK_VTABLE(vaStatus, ctx, CreateConfig);
                    CHECK_VTABLE(vaStatus, ctx, GetConfigAttributes);
                    CHECK_VTABLE(vaStatus, ctx, CreateSurfaces);
                    CHECK_VTABLE(vaStatus, ctx, DestroySurface);
                    CHECK_VTABLE(vaStatus, ctx, CreateContext);
                    CHECK_VTABLE(vaStatus, ctx, DestroyContext);
                    CHECK_VTABLE(vaStatus, ctx, CreateBuffer);
                    CHECK_VTABLE(vaStatus, ctx, BufferData);
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
                    CHECK_VTABLE(vaStatus, ctx, DbgCopySurfaceToBuffer);
                }
                if (VA_STATUS_SUCCESS != vaStatus)
                {
                    va_errorMessage("%s init failed\n", driver_path);
                    dlclose(handle);
                }
                if (VA_STATUS_SUCCESS == vaStatus)
                {
                    ctx->handle = handle;
                }
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


/*
 * Returns a short english description of error_status
 */
const char *vaErrorStr(VAStatus error_status)
{
    switch(error_status)
    {
        case VA_STATUS_SUCCESS:
            return "success (no error)";
        case VA_STATUS_ERROR_ALLOCATION_FAILED:
            return "resource allocation failed";
        case VA_STATUS_ERROR_INVALID_CONFIG:
            return "invalid VAConfigID";
        case VA_STATUS_ERROR_INVALID_CONTEXT:
            return "invalid VAContextID";
        case VA_STATUS_ERROR_INVALID_SURFACE:
            return "invalid VASurfaceID";
        case VA_STATUS_ERROR_INVALID_BUFFER:
            return "invalid VABufferID";
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
        case VA_STATUS_ERROR_UNKNOWN:
            return "unknown libva error";
    }
    return "unknwon libva error / description missing";
}
      
VAStatus vaInitialize (
    VADisplay dpy,
    int *major_version,	 /* out */
    int *minor_version 	 /* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  char *driver_name = NULL;
  VAStatus vaStatus;
  
  ASSERT_CONTEXT(ctx);

  va_debug_trace = (getenv("LIBVA_DEBUG_TRACE") != NULL);

  vaStatus = va_getDriverName(ctx, &driver_name);
  va_infoMessage("va_getDriverName() returns %d\n", vaStatus);
  
  if (VA_STATUS_SUCCESS == vaStatus)
  {
      vaStatus = va_openDriver(ctx, driver_name);
      va_infoMessage("va_openDriver() returns %d\n", vaStatus);
      
      *major_version = ctx->version_major;
      *minor_version = ctx->version_minor;
  }

  if (driver_name)
  {
      XFree(driver_name);
  }
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
  VADriverContextP old_ctx = CTX(dpy);
  ASSERT_CONTEXT(old_ctx);

  if (old_ctx->handle)
  {
      vaStatus = old_ctx->vtable.vaTerminate(old_ctx);
      dlclose(old_ctx->handle);
      old_ctx->handle = NULL;
  }

  if (VA_STATUS_SUCCESS == vaStatus)
  {
      VADriverContextP *ctx = &pDriverContexts;

      /* Throw away old_ctx */
      while (*ctx)
      {
          if (*ctx == old_ctx)
          {
              *ctx = old_ctx->pNext;
              old_ctx->pNext = NULL;
              break;
          }
          ctx = &((*ctx)->pNext);
      }
      free(old_ctx);
  }
  return vaStatus;
}

/* Get maximum number of profiles supported by the implementation */
int vaMaxNumProfiles (
    VADisplay dpy
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);
  
  return ctx->max_profiles;
}

/* Get maximum number of entrypoints supported by the implementation */
int vaMaxNumEntrypoints (
    VADisplay dpy
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);
  
  return ctx->max_entrypoints;
}

/* Get maximum number of attributs supported by the implementation */
int vaMaxNumConfigAttributes (
    VADisplay dpy
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);
  
  return ctx->max_attributes;
}


VAStatus vaQueryConfigEntrypoints (
    VADisplay dpy,
    VAProfile profile,
    VAEntrypoint *entrypoints,	/* out */
    int *num_entrypoints	/* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaQueryConfigEntrypoints);
  return ctx->vtable.vaQueryConfigEntrypoints ( ctx, profile, entrypoints, num_entrypoints);
}

VAStatus vaQueryConfigAttributes (
    VADisplay dpy,
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list, /* in/out */
    int num_attribs
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaQueryConfigAttributes);
  return ctx->vtable.vaQueryConfigAttributes ( ctx, profile, entrypoint, attrib_list, num_attribs );
}

VAStatus vaQueryConfigProfiles (
    VADisplay dpy,
    VAProfile *profile_list,	/* out */
    int *num_profiles		/* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaQueryConfigProfiles);
  return ctx->vtable.vaQueryConfigProfiles ( ctx, profile_list, num_profiles );
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
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaCreateConfig);
  return ctx->vtable.vaCreateConfig ( ctx, profile, entrypoint, attrib_list, num_attribs, config_id );
}

VAStatus vaGetConfigAttributes (
    VADisplay dpy,
    VAConfigID config_id, 
    VAProfile *profile, 	/* out */
    VAEntrypoint *entrypoint, 	/* out */
    VAConfigAttrib *attrib_list,/* out */
    int *num_attribs		/* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaGetConfigAttributes);
  return ctx->vtable.vaGetConfigAttributes( ctx, config_id, profile, entrypoint, attrib_list, num_attribs);
}

VAStatus vaCreateSurfaces (
    VADisplay dpy,
    int width,
    int height,
    int format,
    int num_surfaces,
    VASurface *surfaces	/* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaCreateSurfaces);
  return ctx->vtable.vaCreateSurfaces( ctx, width, height, format, num_surfaces, surfaces );
}

VAStatus vaDestroySurface (
    VADisplay dpy,
    VASurface *surface_list,
    int num_surfaces
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaDestroySurface);
  return ctx->vtable.vaDestroySurface( ctx, surface_list, num_surfaces );
}

VAStatus vaCreateContext (
    VADisplay dpy,
    VAConfigID config_id,
    int picture_width,
    int picture_height,
    int flag,
    VASurface *render_targets,
    int num_render_targets,
    VAContext *context		/* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaCreateContext);
  return ctx->vtable.vaCreateContext( ctx, config_id, picture_width, picture_height,
                                      flag, render_targets, num_render_targets, context );
}

VAStatus vaDestroyContext (
    VADisplay dpy,
    VAContext *context
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaDestroyContext);
  return ctx->vtable.vaDestroyContext( ctx, context );
}

VAStatus vaCreateBuffer (
    VADisplay dpy,
    VABufferType type,	/* in */
    VABufferID *buf_id	/* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaCreateBuffer);
  return ctx->vtable.vaCreateBuffer( ctx, type, buf_id);
}

VAStatus vaBufferData (
    VADisplay dpy,
    VABufferID buf_id,	/* in */
    unsigned int size,	/* in */
    unsigned int num_elements, /* in */
    void *data		/* in */
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaBufferData);
  return ctx->vtable.vaBufferData( ctx, buf_id, size, num_elements, data);
}

VAStatus vaBufferSetNumElements (
    VADisplay dpy,
    VABufferID buf_id,	/* in */
    unsigned int num_elements /* in */
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaBufferSetNumElements);
  return ctx->vtable.vaBufferSetNumElements( ctx, buf_id, num_elements );
}


VAStatus vaMapBuffer (
    VADisplay dpy,
    VABufferID buf_id,	/* in */
    void **pbuf 	/* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaMapBuffer);
  return ctx->vtable.vaMapBuffer( ctx, buf_id, pbuf );
}

VAStatus vaUnmapBuffer (
    VADisplay dpy,
    VABufferID buf_id	/* in */
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaUnmapBuffer);
  return ctx->vtable.vaUnmapBuffer( ctx, buf_id );
}

VAStatus vaDestroyBuffer (
    VADisplay dpy,
    VABufferID buffer_id
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaDestroyBuffer);
  return ctx->vtable.vaDestroyBuffer( ctx, buffer_id );
}

VAStatus vaBeginPicture (
    VADisplay dpy,
    VAContext *context,
    VASurface *render_target
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaBeginPicture);
  return ctx->vtable.vaBeginPicture( ctx, context, render_target );
}

VAStatus vaRenderPicture (
    VADisplay dpy,
    VAContext *context,
    VABufferID *buffers,
    int num_buffers
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaRenderPicture);
  return ctx->vtable.vaRenderPicture( ctx, context, buffers, num_buffers );
}

VAStatus vaEndPicture (
    VADisplay dpy,
    VAContext *context
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaEndPicture);
  return ctx->vtable.vaEndPicture( ctx, context );
}

VAStatus vaSyncSurface (
    VADisplay dpy,
    VAContext *context,
    VASurface *render_target
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaSyncSurface);
  return ctx->vtable.vaSyncSurface( ctx, context, render_target );
}

VAStatus vaQuerySurfaceStatus (
    VADisplay dpy,
    VAContext *context,
    VASurface *render_target,
    VASurfaceStatus *status	/* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaQuerySurfaceStatus);
  return ctx->vtable.vaQuerySurfaceStatus( ctx, context, render_target, status );
}

VAStatus vaPutSurface (
    VADisplay dpy,
    VASurface *surface,
    Drawable draw, /* X Drawable */
    short srcx,
    short srcy,
    unsigned short srcw,
    unsigned short srch,
    short destx,
    short desty,
    unsigned short destw,
    unsigned short desth,
    int flags /* de-interlacing flags */
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaPutSurface);
  return ctx->vtable.vaPutSurface( ctx, surface, draw, srcx, srcy, srcw, srch,
                                   destx, desty, destw, desth, flags );
}

VAStatus vaDbgCopySurfaceToBuffer(VADisplay dpy,
    VASurface *surface,
    void **buffer, /* out */
    unsigned int *stride /* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  ASSERT_CONTEXT(ctx);

  TRACE(vaDbgCopySurfaceToBuffer);
  return ctx->vtable.vaDbgCopySurfaceToBuffer( ctx, surface, buffer, stride );
}

