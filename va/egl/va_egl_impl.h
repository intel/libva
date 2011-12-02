#ifndef _VA_EGL_IMPL_H_
#define _VA_EGL_IMPL_H_

#define IMPL_MAX_EGL_SURFACE_TARGETS    4
#define IMPL_MAX_EGL_SURFACE_ATTRIBUTES 8


/**
 * Initialize EGL driver context
 *
 * @param[in]  dpy        the VA Display
 * @return VA_STATUS_SUCCESS if successful
 */
VAStatus va_egl_init_context(VADisplay dpy);

#endif /* _VA_GLX_IMPL_H_ */
