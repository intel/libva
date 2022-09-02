pkg = import('pkgconfig')

vars = ['libva_version=' + libva_version, 'va_api_version=' + va_api_version]

va_vars = vars + ['driverdir=' + driverdir]
pkg.generate(libva,
  description: 'Userspace Video Acceleration (VA) core interface',
  filebase: 'libva',
  variables: va_vars,
  version: va_api_version)

if WITH_DRM
  drm_vars = vars + ['display=drm']
  pkg.generate(libva_drm,
    description: 'Userspace Video Acceleration (VA) DRM interface',
    filebase: 'libva-drm',
    requires: 'libva',
    variables: drm_vars,
    version: va_api_version)
endif

if WITH_X11
  x11_vars = vars + ['display=x11']
  pkg.generate(libva_x11,
    description: 'Userspace Video Acceleration (VA) X11 interface',
    filebase: 'libva-x11',
    requires: 'libva',
    variables: x11_vars,
    version: va_api_version)
endif

if WITH_GLX
  glx_vars = vars + ['display=glx']
  pkg.generate(libva_glx,
    description: 'Userspace Video Acceleration (VA) GLX interface',
    filebase: 'libva-glx',
    requires: 'libva',
    variables: glx_vars,
    version: va_api_version)
endif

if WITH_WAYLAND
  wayland_vars = vars + ['display=wayland']
  pkg.generate(libva_wayland,
    description: 'Userspace Video Acceleration (VA) Wayland interface',
    filebase: 'libva-wayland',
    requires: 'libva, wayland-client',
    variables: wayland_vars,
    version: va_api_version)
endif

if WITH_WIN32
  win32_vars = vars + ['display=win32']
  pkg.generate(libva_win32,
    description: 'Userspace Video Acceleration (VA) Win32 interface',
    filebase: 'libva-win32',
    requires: 'libva',
    variables: win32_vars,
    version: va_api_version)
endif
