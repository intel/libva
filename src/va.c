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

#define VA_MAJOR_VERSION	0
#define VA_MINOR_VERSION	29
#define DRIVER_INIT_FUNC	"__vaDriverInit_0_29"

#define DEFAULT_DRIVER_DIR	"/usr/X11R6/lib/modules/dri"
#define DRIVER_EXTENSION	"_drv_video.so"

#define CTX(dpy) ((VADriverContextP) dpy );
#define CHECK_CONTEXT(dpy) if( !vaContextIsValid(dpy) ) { return VA_STATUS_ERROR_INVALID_DISPLAY; }
#define ASSERT		assert
#define CHECK_VTABLE(s, ctx, func) if (!va_checkVtable(ctx->vtable.va##func, #func)) s = VA_STATUS_ERROR_UNKNOWN;
#define CHECK_MAXIMUM(s, ctx, var) if (!va_checkMaximum(ctx->max_##var, #var)) s = VA_STATUS_ERROR_UNKNOWN;
#define CHECK_STRING(s, ctx, var) if (!va_checkString(ctx->str_##var, #var)) s = VA_STATUS_ERROR_UNKNOWN;

#define TRACE(func) if (va_debug_trace) va_infoMessage("[TR] %s\n", #func);

static VADriverContextP pDriverContexts = NULL;
static int va_debug_trace = 0;

static Bool vaContextIsValid(VADriverContextP arg_ctx)
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

  if (!native_dpy)
  {
      return NULL;
  }

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

static Bool va_checkString(const char* value, char *variable)
{
    if (!value)
    {
        va_errorMessage("Failed to define str_%s in init\n", variable);
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
    char *search_path = NULL;
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
                    CHECK_VTABLE(vaStatus, ctx, PutImage2);
                    CHECK_VTABLE(vaStatus, ctx, QuerySubpictureFormats);
                    CHECK_VTABLE(vaStatus, ctx, CreateSubpicture);
                    CHECK_VTABLE(vaStatus, ctx, DestroySubpicture);
                    CHECK_VTABLE(vaStatus, ctx, SetSubpictureImage);
                    CHECK_VTABLE(vaStatus, ctx, SetSubpicturePalette);
                    CHECK_VTABLE(vaStatus, ctx, SetSubpictureChromakey);
                    CHECK_VTABLE(vaStatus, ctx, SetSubpictureGlobalAlpha);
                    CHECK_VTABLE(vaStatus, ctx, AssociateSubpicture);
                    CHECK_VTABLE(vaStatus, ctx, AssociateSubpicture2);
                    CHECK_VTABLE(vaStatus, ctx, DeassociateSubpicture);
                    CHECK_VTABLE(vaStatus, ctx, QueryDisplayAttributes);
                    CHECK_VTABLE(vaStatus, ctx, GetDisplayAttributes);
                    CHECK_VTABLE(vaStatus, ctx, SetDisplayAttributes);
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

VAPrivFunc vaGetLibFunc(VADisplay dpy, const char *func)
{
    VADriverContextP ctx = CTX(dpy);
    if( !vaContextIsValid(ctx) )
        return NULL;

    if (NULL == ctx->handle)
        return NULL;
        
    return (VAPrivFunc) dlsym(ctx->handle, func);
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
        case VA_STATUS_ERROR_UNKNOWN:
            return "unknown libva error";
    }
    return "unknown libva error / description missing";
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
  
  CHECK_CONTEXT(ctx);

  va_debug_trace = (getenv("LIBVA_DEBUG_TRACE") != NULL);

  vaStatus = va_getDriverName(ctx, &driver_name);
  va_infoMessage("va_getDriverName() returns %d\n", vaStatus);
  
  if (VA_STATUS_SUCCESS == vaStatus)
  {
      vaStatus = va_openDriver(ctx, driver_name);
      va_infoMessage("va_openDriver() returns %d\n", vaStatus);
      
      *major_version = VA_MAJOR_VERSION;
      *minor_version = VA_MINOR_VERSION;
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
  CHECK_CONTEXT(old_ctx);

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
  VADriverContextP ctx = CTX(dpy);
  if( !vaContextIsValid(ctx) )
  {
      return NULL;
  }
  
  return ctx->str_vendor;
}


/* Get maximum number of profiles supported by the implementation */
int vaMaxNumProfiles (
    VADisplay dpy
)
{
  VADriverContextP ctx = CTX(dpy);
  if( !vaContextIsValid(ctx) )
  {
      return 0;
  }
  
  return ctx->max_profiles;
}

/* Get maximum number of entrypoints supported by the implementation */
int vaMaxNumEntrypoints (
    VADisplay dpy
)
{
  VADriverContextP ctx = CTX(dpy);
  if( !vaContextIsValid(ctx) )
  {
      return 0;
  }
  
  return ctx->max_entrypoints;
}


/* Get maximum number of attributs supported by the implementation */
int vaMaxNumConfigAttributes (
    VADisplay dpy
)
{
  VADriverContextP ctx = CTX(dpy);
  if( !vaContextIsValid(ctx) )
  {
      return 0;
  }
  
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
  CHECK_CONTEXT(ctx);

  TRACE(vaQueryConfigEntrypoints);
  return ctx->vtable.vaQueryConfigEntrypoints ( ctx, profile, entrypoints, num_entrypoints);
}

VAStatus vaGetConfigAttributes (
    VADisplay dpy,
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list, /* in/out */
    int num_attribs
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaGetConfigAttributes);
  return ctx->vtable.vaGetConfigAttributes ( ctx, profile, entrypoint, attrib_list, num_attribs );
}

VAStatus vaQueryConfigProfiles (
    VADisplay dpy,
    VAProfile *profile_list,	/* out */
    int *num_profiles		/* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

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
  CHECK_CONTEXT(ctx);

  TRACE(vaCreateConfig);
  return ctx->vtable.vaCreateConfig ( ctx, profile, entrypoint, attrib_list, num_attribs, config_id );
}

VAStatus vaDestroyConfig (
    VADisplay dpy,
    VAConfigID config_id
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaDestroyConfig);
  return ctx->vtable.vaDestroyConfig ( ctx, config_id );
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaQueryConfigAttributes);
  return ctx->vtable.vaQueryConfigAttributes( ctx, config_id, profile, entrypoint, attrib_list, num_attribs);
}

VAStatus vaCreateSurfaces (
    VADisplay dpy,
    int width,
    int height,
    int format,
    int num_surfaces,
    VASurfaceID *surfaces	/* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaCreateSurfaces);
  return ctx->vtable.vaCreateSurfaces( ctx, width, height, format, num_surfaces, surfaces );
}

VAStatus vaDestroySurfaces (
    VADisplay dpy,
    VASurfaceID *surface_list,
    int num_surfaces
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaDestroySurfaces);
  return ctx->vtable.vaDestroySurfaces( ctx, surface_list, num_surfaces );
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaCreateContext);
  return ctx->vtable.vaCreateContext( ctx, config_id, picture_width, picture_height,
                                      flag, render_targets, num_render_targets, context );
}

VAStatus vaDestroyContext (
    VADisplay dpy,
    VAContextID context
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaDestroyContext);
  return ctx->vtable.vaDestroyContext( ctx, context );
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaCreateBuffer);
  return ctx->vtable.vaCreateBuffer( ctx, context, type, size, num_elements, data, buf_id);
}

VAStatus vaBufferSetNumElements (
    VADisplay dpy,
    VABufferID buf_id,	/* in */
    unsigned int num_elements /* in */
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

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
  CHECK_CONTEXT(ctx);

  TRACE(vaMapBuffer);
  return ctx->vtable.vaMapBuffer( ctx, buf_id, pbuf );
}

VAStatus vaUnmapBuffer (
    VADisplay dpy,
    VABufferID buf_id	/* in */
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaUnmapBuffer);
  return ctx->vtable.vaUnmapBuffer( ctx, buf_id );
}

VAStatus vaDestroyBuffer (
    VADisplay dpy,
    VABufferID buffer_id
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaDestroyBuffer);
  return ctx->vtable.vaDestroyBuffer( ctx, buffer_id );
}

VAStatus vaBeginPicture (
    VADisplay dpy,
    VAContextID context,
    VASurfaceID render_target
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaBeginPicture);
  return ctx->vtable.vaBeginPicture( ctx, context, render_target );
}

VAStatus vaRenderPicture (
    VADisplay dpy,
    VAContextID context,
    VABufferID *buffers,
    int num_buffers
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaRenderPicture);
  return ctx->vtable.vaRenderPicture( ctx, context, buffers, num_buffers );
}

VAStatus vaEndPicture (
    VADisplay dpy,
    VAContextID context
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaEndPicture);
  return ctx->vtable.vaEndPicture( ctx, context );
}

VAStatus vaSyncSurface (
    VADisplay dpy,
    VAContextID context,
    VASurfaceID render_target
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaSyncSurface);
  return ctx->vtable.vaSyncSurface( ctx, context, render_target );
}

VAStatus vaQuerySurfaceStatus (
    VADisplay dpy,
    VASurfaceID render_target,
    VASurfaceStatus *status	/* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaQuerySurfaceStatus);
  return ctx->vtable.vaQuerySurfaceStatus( ctx, render_target, status );
}

VAStatus vaPutSurface (
    VADisplay dpy,
    VASurfaceID surface,
    Drawable draw, /* X Drawable */
    short srcx,
    short srcy,
    unsigned short srcw,
    unsigned short srch,
    short destx,
    short desty,
    unsigned short destw,
    unsigned short desth,
    VARectangle *cliprects, /* client supplied clip list */
    unsigned int number_cliprects, /* number of clip rects in the clip list */
    unsigned int flags /* de-interlacing flags */
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaPutSurface);
  return ctx->vtable.vaPutSurface( ctx, surface, draw, srcx, srcy, srcw, srch,
                                   destx, desty, destw, desth,
                                   cliprects, number_cliprects, flags );
}

/* Get maximum number of image formats supported by the implementation */
int vaMaxNumImageFormats (
    VADisplay dpy
)
{
  VADriverContextP ctx = CTX(dpy);
  if( !vaContextIsValid(ctx) )
  {
      return 0;
  }
  
  return ctx->max_image_formats;
}

VAStatus vaQueryImageFormats (
    VADisplay dpy,
    VAImageFormat *format_list,	/* out */
    int *num_formats		/* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaQueryImageFormats);
  return ctx->vtable.vaQueryImageFormats ( ctx, format_list, num_formats);
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaCreateImage);
  return ctx->vtable.vaCreateImage ( ctx, format, width, height, image);
}

/*
 * Should call DestroyImage before destroying the surface it is bound to
 */
VAStatus vaDestroyImage (
    VADisplay dpy,
    VAImageID image
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaDestroyImage);
  return ctx->vtable.vaDestroyImage ( ctx, image);
}

VAStatus vaSetImagePalette (
    VADisplay dpy,
    VAImageID image,
    unsigned char *palette
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaSetImagePalette);
  return ctx->vtable.vaSetImagePalette ( ctx, image, palette);
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaGetImage);
  return ctx->vtable.vaGetImage ( ctx, surface, x, y, width, height, image);
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
    unsigned int width,
    unsigned int height,
    int dest_x,
    int dest_y
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaPutImage);
  return ctx->vtable.vaPutImage ( ctx, surface, image, src_x, src_y, width, height, dest_x, dest_y );
}

/*
 * Similar to vaPutImage but with additional destination width
 * and height arguments to enable scaling
 */
VAStatus vaPutImage2 (
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaPutImage2);
  return ctx->vtable.vaPutImage2 ( ctx, surface, image, src_x, src_y, src_width, src_height, dest_x, dest_y, dest_width, dest_height );
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaDeriveImage);
  return ctx->vtable.vaDeriveImage ( ctx, surface, image );
}


/* Get maximum number of subpicture formats supported by the implementation */
int vaMaxNumSubpictureFormats (
    VADisplay dpy
)
{
  VADriverContextP ctx = CTX(dpy);
  if( !vaContextIsValid(ctx) )
  {
      return 0;
  }
  
  return ctx->max_subpic_formats;
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaQuerySubpictureFormats);
  return ctx->vtable.vaQuerySubpictureFormats ( ctx, format_list, flags, num_formats);
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaCreateSubpicture);
  return ctx->vtable.vaCreateSubpicture ( ctx, image, subpicture );
}

/*
 * Destroy the subpicture before destroying the image it is assocated to
 */
VAStatus vaDestroySubpicture (
    VADisplay dpy,
    VASubpictureID subpicture
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaDestroySubpicture);
  return ctx->vtable.vaDestroySubpicture ( ctx, subpicture);
}

VAStatus vaSetSubpictureImage (
    VADisplay dpy,
    VASubpictureID subpicture,
    VAImageID image
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaSetSubpictureImage);
  return ctx->vtable.vaSetSubpictureImage ( ctx, subpicture, image);
}

#warning TODO: Remove vaSetSubpicturePalette in rev 0.29
VAStatus vaSetSubpicturePalette (
    VADisplay dpy,
    VASubpictureID subpicture,
    /* 
     * pointer to an array holding the palette data.  The size of the array is 
     * num_palette_entries * entry_bytes in size.  The order of the components
     * in the palette is described by the component_order in VASubpicture struct
     */
    unsigned char *palette 
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaSetSubpicturePalette);
  return ctx->vtable.vaSetSubpicturePalette ( ctx, subpicture, palette);
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaSetSubpictureChromakey);
  return ctx->vtable.vaSetSubpictureChromakey ( ctx, subpicture, chromakey_min, chromakey_max, chromakey_mask );
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaSetSubpictureGlobalAlpha);
  return ctx->vtable.vaSetSubpictureGlobalAlpha ( ctx, subpicture, global_alpha );
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
    short dest_x, /* upper left offset in surface */
    short dest_y,
    unsigned short width,
    unsigned short height,
    /*
     * whether to enable chroma-keying or global-alpha
     * see VA_SUBPICTURE_XXX values
     */
    unsigned int flags
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaAssociateSubpicture);
  return ctx->vtable.vaAssociateSubpicture ( ctx, subpicture, target_surfaces, num_surfaces, src_x, src_y, dest_x, dest_y, width, height, flags );
}

VAStatus vaAssociateSubpicture2 (
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaAssociateSubpicture2);
  return ctx->vtable.vaAssociateSubpicture2 ( ctx, subpicture, target_surfaces, num_surfaces, src_x, src_y, src_width, src_height, dest_x, dest_y, dest_width, dest_height, flags );
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaDeassociateSubpicture);
  return ctx->vtable.vaDeassociateSubpicture ( ctx, subpicture, target_surfaces, num_surfaces );
}


/* Get maximum number of display attributes supported by the implementation */
int vaMaxNumDisplayAttributes (
    VADisplay dpy
)
{
  VADriverContextP ctx = CTX(dpy);
  if( !vaContextIsValid(ctx) )
  {
      return 0;
  }
  
  return ctx->max_display_attributes;
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaQueryDisplayAttributes);
  return ctx->vtable.vaQueryDisplayAttributes ( ctx, attr_list, num_attributes );
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaGetDisplayAttributes);
  return ctx->vtable.vaGetDisplayAttributes ( ctx, attr_list, num_attributes );
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
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaSetDisplayAttributes);
  return ctx->vtable.vaSetDisplayAttributes ( ctx, attr_list, num_attributes );
}


#warning TODO: Remove vaDbgCopySurfaceToBuffer in rev 0.29
VAStatus vaDbgCopySurfaceToBuffer(VADisplay dpy,
    VASurfaceID surface,
    void **buffer, /* out */
    unsigned int *stride /* out */
)
{
  VADriverContextP ctx = CTX(dpy);
  CHECK_CONTEXT(ctx);

  TRACE(vaDbgCopySurfaceToBuffer);
  return ctx->vtable.vaDbgCopySurfaceToBuffer( ctx, surface, buffer, stride );
}

