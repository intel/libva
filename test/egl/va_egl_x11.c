#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <va/va_x11.h>
#include <va/va_egl.h>

struct va_egl_context
{
    Display *x11_dpy;
    Window win;

    EGLDisplay egl_dpy;
    EGLContext egl_ctx;
    EGLSurface egl_surf;
    unsigned int egl_target;
    EGLClientBuffer egl_buffer;
    EGLImageKHR egl_image;
    PFNEGLCREATEIMAGEKHRPROC egl_create_image_khr;
    PFNEGLDESTROYIMAGEKHRPROC egl_destroy_image_hkr;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glegl_image_target_texture2d_oes;

    VADisplay va_dpy;
    VASurfaceID va_surface;
    VASurfaceEGL va_egl_surface;

    int x, y;
    unsigned int width, height;
    GLuint texture;
    GLfloat ar;
    unsigned int box_width;
    unsigned char ydata;
};

static void
va_egl_fini_egl(struct va_egl_context *ctx)
{
    eglMakeCurrent(ctx->egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(ctx->egl_dpy);
}

static int
va_egl_init_egl(struct va_egl_context *ctx)
{
    EGLint egl_major, egl_minor;
    const char *s;

    ctx->egl_dpy = eglGetDisplay(ctx->x11_dpy);

    if (!ctx->egl_dpy) {
        printf("Error: eglGetDisplay() failed\n");
        return -1;
    }

    if (!eglInitialize(ctx->egl_dpy, &egl_major, &egl_minor)) {
        printf("Error: eglInitialize() failed\n");
        return -1;
    }

    s = eglQueryString(ctx->egl_dpy, EGL_VERSION);
    printf("EGL_VERSION = %s\n", s);

    return 0;
}

static int 
yuvgen_planar(int width, int height,
              unsigned char *Y_start, int Y_pitch,
              unsigned char *U_start, int U_pitch,
              unsigned char *V_start, int V_pitch,
              int UV_interleave, int box_width, unsigned char ydata)
{
    int row;

    /* copy Y plane */
    for (row = 0; row < height; row++) {
        unsigned char *Y_row = Y_start + row * Y_pitch;
        int jj, xpos, ypos;

        ypos = (row / box_width) & 0x1;

        for (jj = 0; jj < width; jj++) {
            xpos = ((jj) / box_width) & 0x1;
                        
            if ((xpos == 0) && (ypos == 0))
                Y_row[jj] = ydata;
            if ((xpos == 1) && (ypos == 1))
                Y_row[jj] = ydata;
                        
            if ((xpos == 1) && (ypos == 0))
                Y_row[jj] = 0xff - ydata;
            if ((xpos == 0) && (ypos == 1))
                Y_row[jj] = 0xff - ydata;
        }
    }
  
    /* copy UV data */
    for( row = 0; row < height/2; row++) {
        unsigned short value = 0x80;

        if (UV_interleave) {
            unsigned short *UV_row = (unsigned short *)(U_start + row * U_pitch);

            memset(UV_row, value, width);
        } else {
            unsigned char *U_row = U_start + row * U_pitch;
            unsigned char *V_row = V_start + row * V_pitch;
            
            memset(U_row, value, width / 2);
            memset(V_row, value, width / 2);
        }
    }

    return 0;
}

static int 
va_egl_upload_surface(struct va_egl_context *ctx)
{
    VAImage surface_image;
    void *surface_p = NULL, *U_start, *V_start;
    
    vaDeriveImage(ctx->va_dpy, ctx->va_surface, &surface_image);

    vaMapBuffer(ctx->va_dpy, surface_image.buf, &surface_p);
        
    U_start = (char *)surface_p + surface_image.offsets[1];
    V_start = (char *)surface_p + surface_image.offsets[2];

    /* assume surface is planar format */
    yuvgen_planar(surface_image.width, surface_image.height,
                  (unsigned char *)surface_p, surface_image.pitches[0],
                  (unsigned char *)U_start, surface_image.pitches[1],
                  (unsigned char *)V_start, surface_image.pitches[2],
                  (surface_image.format.fourcc==VA_FOURCC_NV12),
                  ctx->box_width, ctx->ydata);
        
    vaUnmapBuffer(ctx->va_dpy,surface_image.buf);

    vaDestroyImage(ctx->va_dpy,surface_image.image_id);

    return 0;
}

static void
va_egl_fini_va(struct va_egl_context *ctx)
{
    vaDestroySurfaces(ctx->va_dpy, &ctx->va_surface, 1);    
    vaTerminate(ctx->va_dpy);
}

static int
va_egl_init_va(struct va_egl_context *ctx)
{
    VAStatus va_status;
    int major_ver, minor_ver;

    ctx->va_dpy = vaGetDisplayEGL(ctx->x11_dpy, ctx->egl_dpy);

    if (!ctx->va_dpy) {
        printf("Error: vaGetDisplayEGL() failed\n");
        return -1;
    }

    va_status = vaInitialize(ctx->va_dpy, &major_ver, &minor_ver);

    if (va_status != VA_STATUS_SUCCESS) {
        printf("Error: vaInitialize() failed\n");
        return -1;
    }

    va_status = vaCreateSurfaces(ctx->va_dpy,
                                 ctx->width, ctx->height,
                                 VA_RT_FORMAT_YUV420, 
                                 1, &ctx->va_surface);

    if (va_status != VA_STATUS_SUCCESS) {
        printf("Error: vaCreateSurfaces() failed\n");
        return -1;
    }

    va_egl_upload_surface(ctx);

    return 0;
}

static void
va_egl_make_window(struct va_egl_context *ctx, const char *title)
{
    int scrnum;
    XSetWindowAttributes attr;
    unsigned long mask;
    Window root;
    XVisualInfo *visInfo, visTemplate;
    int num_visuals;
    EGLConfig config;
    EGLint num_configs, vid;
    const EGLint attribs[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
        EGL_NONE
    };

    scrnum = DefaultScreen(ctx->x11_dpy);
    root = RootWindow(ctx->x11_dpy, scrnum);

    if (!eglChooseConfig(ctx->egl_dpy, attribs, &config, 1, &num_configs) ||
        !num_configs) {
        printf("Error: couldn't get an EGL visual config\n");

        return;
    }

    if (!eglGetConfigAttrib(ctx->egl_dpy, config, EGL_NATIVE_VISUAL_ID, &vid)) {
        printf("Error: eglGetConfigAttrib() failed\n");

        return;
    }

    /* The X window visual must match the EGL config */
    visTemplate.visualid = vid;
    visInfo = XGetVisualInfo(ctx->x11_dpy, VisualIDMask, &visTemplate, &num_visuals);

    if (!visInfo) {
        printf("Error: couldn't get X visual\n");

        return;
    }

    /* window attributes */
    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(ctx->x11_dpy, root, visInfo->visual, AllocNone);
    attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
    attr.override_redirect = 0;
    mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect;

    ctx->win = XCreateWindow(ctx->x11_dpy,
                             root,
                             ctx->x, ctx->y,
                             ctx->width, ctx->height,
                             0, visInfo->depth, InputOutput,
                             visInfo->visual, mask, &attr);

    /* set hints and properties */
    {
        XSizeHints sizehints;
        sizehints.x = ctx->x;
        sizehints.y = ctx->y;
        sizehints.width  = ctx->width;
        sizehints.height = ctx->height;
        sizehints.flags = USSize | USPosition;
        XSetNormalHints(ctx->x11_dpy, ctx->win, &sizehints);
        XSetStandardProperties(ctx->x11_dpy, ctx->win, title, title,
                               None, (char **)NULL, 0, &sizehints);
    }

    eglBindAPI(EGL_OPENGL_ES_API);

    ctx->egl_ctx = eglCreateContext(ctx->egl_dpy, config, EGL_NO_CONTEXT, NULL);

    if (!ctx->egl_ctx) {
        printf("Error: eglCreateContext() failed\n");
        
        return;
    }

    ctx->egl_surf = eglCreateWindowSurface(ctx->egl_dpy, config, ctx->win, NULL);
    eglMakeCurrent(ctx->egl_dpy, ctx->egl_surf, ctx->egl_surf, ctx->egl_ctx);
    XFree(visInfo);
}

static int
va_egl_init_extension(struct va_egl_context *ctx)
{
   const char *exts;

   exts = eglQueryString(ctx->egl_dpy, EGL_EXTENSIONS);
   ctx->egl_create_image_khr =
       (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
   ctx->egl_destroy_image_hkr =
       (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");

   if (!exts ||
       !strstr(exts, "EGL_KHR_image_base") ||
       !ctx->egl_create_image_khr ||
       !ctx->egl_destroy_image_hkr) {
       printf("EGL does not support EGL_KHR_image_base\n");
       return -1;
   }

   exts = (const char *)glGetString(GL_EXTENSIONS);
   ctx->glegl_image_target_texture2d_oes =
       (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");

   if (!exts ||
       !strstr(exts, "GL_OES_EGL_image") ||
       !ctx->glegl_image_target_texture2d_oes) {
       printf("OpenGL ES does not support GL_OES_EGL_image\n");
       return -1;
   }

   return 0;
}

static void
va_egl_fini_gles(struct va_egl_context *ctx)
{
    glDeleteTextures(1, &ctx->texture);
}

static int
va_egl_init_gles(struct va_egl_context *ctx)
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor4f(1.0, 1.0, 1.0, 1.0);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    glGenTextures(1, &ctx->texture);
    glBindTexture(GL_TEXTURE_2D, ctx->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);

    return 0;
}

static void
va_egl_fini_va_egl(struct va_egl_context *ctx)
{
    if (ctx->egl_image)
        ctx->egl_destroy_image_hkr(ctx->egl_dpy, ctx->egl_image);

    vaDeassociateSurfaceEGL(ctx->va_dpy, ctx->va_egl_surface);
    vaDestroySurfaceEGL(ctx->va_dpy, ctx->va_egl_surface);
}

static int
va_egl_init_va_egl(struct va_egl_context *ctx)
{
    VAStatus va_status;
    int num_max_targets = 0, num_targets = 0;
    int num_max_attributes = 0, num_attribs = 0;
    unsigned int *target_list = NULL;
    EGLint *img_attribs = NULL;

    num_targets = num_max_targets = vaMaxNumSurfaceTargetsEGL(ctx->va_dpy);
    
    if (num_max_targets < 1) {
        printf("Error: vaMaxNumSurfaceTargetsEGL() returns %d\n", num_max_targets);
        return -1;
    }

    num_attribs = num_max_attributes = vaMaxNumSurfaceAttributesEGL(ctx->va_dpy);

    if (num_max_attributes < 1) {
        printf("Error: vaMaxNumSurfaceAttributesEGL() returns %d\n", num_max_attributes);
        return -1;
    }

    target_list = malloc(num_max_targets * sizeof(unsigned int));
    va_status = vaQuerySurfaceTargetsEGL(ctx->va_dpy,
                                         target_list,
                                         &num_targets);
    
    if (va_status != VA_STATUS_SUCCESS || num_targets < 1) {
        printf("Error: vaQuerySurfaceTargetsEGL() failed\n");
        return -1;
    }
    
    va_status = vaCreateSurfaceEGL(ctx->va_dpy,
                                   target_list[0],
                                   ctx->width, ctx->height,
                                   &ctx->va_egl_surface);

    if (va_status != VA_STATUS_SUCCESS) {
        printf("Error: vaCreateSurfaceEGL() failed\n");
        return -1;
    }

    va_status = vaAssociateSurfaceEGL(ctx->va_dpy,
                                      ctx->va_egl_surface,
                                      ctx->va_surface,
                                      0);
 
    if (va_status != VA_STATUS_SUCCESS) {
        printf("Error: vaAssociateSurfaceEGL() failed\n");
        return -1;
    }

    img_attribs = malloc(2 * num_max_attributes * sizeof(EGLint));
    va_status = vaGetSurfaceInfoEGL(ctx->va_dpy,
                                    ctx->va_egl_surface,
                                    &ctx->egl_target,
                                    &ctx->egl_buffer,
                                    img_attribs,
                                    &num_attribs);

    if (va_status != VA_STATUS_SUCCESS) {
        printf("Error: vaGetSurfaceInfoEGL() failed\n");
        return -1;
    }

    ctx->egl_image = ctx->egl_create_image_khr(ctx->egl_dpy,
                                               EGL_NO_CONTEXT,
                                               ctx->egl_target,
                                               ctx->egl_buffer,
                                               img_attribs);

    vaSyncSurfaceEGL(ctx->va_dpy, ctx->va_egl_surface);
    ctx->glegl_image_target_texture2d_oes(GL_TEXTURE_2D,
                                          (GLeglImageOES)ctx->egl_image);

    return 0;
}

static void
va_egl_fini(struct va_egl_context *ctx)
{
    va_egl_fini_gles(ctx);
    va_egl_fini_va(ctx);
    va_egl_fini_egl(ctx);
    va_egl_fini_gles(ctx);
    va_egl_fini_va_egl(ctx);

    // XDestroyWindow(ctx->x11_dpy, ctx->win);
    XCloseDisplay(ctx->x11_dpy);
}

static int
va_egl_init(struct va_egl_context *ctx, int argc, char **argv)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->x11_dpy = XOpenDisplay(NULL);
    ctx->width = 320;
    ctx->height = 320;
    ctx->ar = 1.0;
    ctx->box_width = 16;
    ctx->ydata = 0xff;

    if (!ctx->x11_dpy) {
        printf("Error: couldn't open display %s\n", getenv("DISPLAY"));
        return -1;
    }

    if (va_egl_init_egl(ctx) != 0)
        return -1;

    if (va_egl_init_va(ctx) != 0)
        return -1;

    va_egl_make_window(ctx, "VA/EGL");
    va_egl_init_extension(ctx);
    va_egl_init_gles(ctx);
    va_egl_init_va_egl(ctx);

    return 0;
}

static void
va_egl_reshape(struct va_egl_context *ctx, int width, int height)
{
    GLfloat ar = (GLfloat) width / (GLfloat) height;

    ctx->width = width;
    ctx->height = height;
    ctx->ar = ar;

    glViewport(0, 0, (GLint) width, (GLint) height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(-ar, ar, -ar, ar, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void
va_egl_draw(struct va_egl_context *ctx)
{
    const GLfloat verts[][3] = {
        { -ctx->ar, -ctx->ar, 0 },
        {  ctx->ar, -ctx->ar, 0 },
        {  ctx->ar,  ctx->ar, 0 },
        { -ctx->ar,  ctx->ar, 0 }
    };
    const GLfloat texs[][2] = {
        { 0, 0 },
        { 1, 0 },
        { 1, 1 },
        { 0, 1 }
    };

    glClear(GL_COLOR_BUFFER_BIT);

    glVertexPointer(3, GL_FLOAT, 0, verts);
    glTexCoordPointer(2, GL_FLOAT, 0, texs);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

static void
va_egl_event_loop(struct va_egl_context *ctx)
{
    while (1) {
        int redraw = 0;

        if (XPending(ctx->x11_dpy) > 0) {
            XEvent event;
            XNextEvent(ctx->x11_dpy, &event);

            switch (event.type) {
            case Expose:
                redraw = 1;
                break;

            case ConfigureNotify:
                va_egl_reshape(ctx, event.xconfigure.width, event.xconfigure.height);
                redraw = 1;
                break;

            case KeyPress:
            {
                char buffer[10];
                int code;
                code = XLookupKeysym(&event.xkey, 0);

                if (code == XK_y) {
                    ctx->ydata += 0x10;
                    va_egl_upload_surface(ctx);
                    vaSyncSurfaceEGL(ctx->va_dpy, ctx->va_egl_surface);
                    ctx->glegl_image_target_texture2d_oes(GL_TEXTURE_2D,
                                                          (GLeglImageOES)ctx->egl_image);
                    redraw = 1;
                } else {
                    XLookupString(&event.xkey, buffer, sizeof(buffer),
                                  NULL, NULL);
                    
                    if (buffer[0] == 27) {
                        /* escape */
                        return;
                    }
                }
            }
                
            break;

            default:
                ; /*no-op*/
            }
        }

        if (redraw) {
            va_egl_draw(ctx);
            eglSwapBuffers(ctx->egl_dpy, ctx->egl_surf);
        }
    }
}

static void
va_egl_run(struct va_egl_context *ctx)
{
    XMapWindow(ctx->x11_dpy, ctx->win);
    va_egl_reshape(ctx, ctx->width, ctx->height);
    va_egl_event_loop(ctx);
}

int
main(int argc, char *argv[])
{
    struct va_egl_context ctx;

    printf("Usage: press 'y' to change Y plane \n\n");

    if (va_egl_init(&ctx, argc, argv) == 0) {
        va_egl_run(&ctx);
        va_egl_fini(&ctx);
    }

    return 0;
}
