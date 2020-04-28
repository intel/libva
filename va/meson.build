version_cfg = configuration_data()
version_cfg.set('VA_API_MAJOR_VERSION', va_api_major_version)
version_cfg.set('VA_API_MINOR_VERSION', va_api_minor_version)
version_cfg.set('VA_API_MICRO_VERSION', va_api_micro_version)
version_cfg.set('VA_API_VERSION', va_api_version)

version_file = configure_file(
  input : 'va_version.h.in',
  output : 'va_version.h',
  configuration : version_cfg)

libva_sources = [
  'va.c',
  'va_compat.c',
  'va_fool.c',
  'va_str.c',
  'va_trace.c',
]

libva_headers = [
  'va.h',
  'va_backend.h',
  'va_backend_vpp.h',
  'va_compat.h',
  'va_dec_hevc.h',
  'va_dec_jpeg.h',
  'va_dec_vp8.h',
  'va_dec_vp9.h',
  'va_dec_av1.h',
  'va_drmcommon.h',
  'va_egl.h',
  'va_enc_hevc.h',
  'va_enc_h264.h',
  'va_enc_jpeg.h',
  'va_enc_vp8.h',
  'va_fei.h',
  'va_fei_h264.h',
  'va_fei_hevc.h',
  'va_enc_mpeg2.h',
  'va_enc_vp9.h',
  'va_str.h',
  'va_tpi.h',
  'va_vpp.h',
  version_file,
]

libva_headers_priv = [
  'sysdeps.h',
  'va_fool.h',
  'va_internal.h',
  'va_trace.h',
]

libva_sym = 'libva.syms'
libva_sym_path = '@0@/@1@'.format(meson.current_source_dir(), libva_sym)

install_headers(libva_headers, subdir : 'va')

libva = shared_library(
  'va',
  sources : libva_sources +
            libva_headers +
            libva_headers_priv,
  soversion : libva_lt_current,
  version : libva_lt_version,
  c_args : ['-DVA_DRIVERS_PATH="' + driverdir + '"'] + va_c_args,
  include_directories : configinc,
  link_args : '-Wl,-version-script,' + libva_sym_path,
  link_depends : libva_sym,
  install : true,
  dependencies : [ dl_dep ])

libva_dep = declare_dependency(
  link_with : libva,
  include_directories : configinc,
  dependencies : [ dl_dep ])

if WITH_DRM
  libva_drm_sources = [
    'drm/va_drm.c',
    'drm/va_drm_auth.c',
    'drm/va_drm_utils.c',
  ]

  libva_drm_headers = [
    'drm/va_drm.h',
  ]

  libva_drm_headers_priv = [
    'drm/va_drm_auth.h',
    'drm/va_drm_auth_x11.h',
    'drm/va_drm_utils.h',
  ]

  deps = [ libdrm_dep ]

  libva_drm_args = []
  if get_option('with_x11') != 'no' and x11_dep.found()
    libva_drm_sources += [ 'drm/va_drm_auth_x11.c' ]
    libva_drm_args += [
      '-DLIBVA_MAJOR_VERSION=@0@'.format(libva_major_version)
    ]
    deps += [ x11_dep ]
  endif

  install_headers(libva_drm_headers, subdir : 'va')

  libva_drm = shared_library(
    'va-drm',
    sources : libva_drm_sources +
              libva_drm_headers +
              libva_drm_headers_priv,
    soversion : libva_lt_current,
    version : libva_lt_version,
    install : true,
    c_args : libva_drm_args,
    dependencies : deps + [ libva_dep ])

  libva_drm_dep = declare_dependency(
    link_with : libva_drm,
    include_directories : configinc,
    dependencies : deps)
endif

if WITH_X11
  libva_x11_sources = [
    'x11/dri2_util.c',
    'x11/va_dri2.c',
    'x11/va_dricommon.c',
    'x11/va_fglrx.c',
    'x11/va_nvctrl.c',
    'x11/va_x11.c',
  ]

  libva_x11_headers = [
    'va_x11.h',
    'x11/va_dri2.h',
    'x11/va_dricommon.h',
  ]

  libva_x11_headers_priv = [
    'x11/va_dri2str.h',
    'x11/va_dri2tokens.h',
    'x11/va_fglrx.h',
    'x11/va_nvctrl.h',
  ]

  install_headers(libva_x11_headers, subdir : 'va')

  deps = [ libdrm_dep, x11_dep, xext_dep, xfixes_dep, libva_dep ]

  libva_x11 = shared_library(
    'va-x11',
    sources : libva_x11_sources +
              libva_x11_headers +
              libva_x11_headers_priv,
    soversion : libva_lt_current,
    version : libva_lt_version,
    install : true,
    dependencies : deps)

  libva_x11_dep = declare_dependency(
    link_with : libva_x11,
    include_directories : configinc,
    dependencies : deps)
endif

if WITH_GLX
  libva_glx_sources = [
    'glx/va_glx.c',
    'glx/va_glx_impl.c',
  ]

  libva_glx_headers = [
    'glx/va_backend_glx.h',
    'glx/va_glx.h',
  ]

  libva_glx_headers_priv = [
    'glx/va_glx_impl.h',
    'glx/va_glx_private.h',
  ]

  install_headers(libva_glx_headers, subdir : 'va')

  deps = [ gl_dep, libva_x11_dep ]

  libva_glx = shared_library(
    'va-glx',
    sources : libva_glx_sources +
              libva_glx_headers +
              libva_glx_headers_priv,
    soversion : libva_lt_current,
    version : libva_lt_version,
    install : true,
    dependencies : deps)

  libva_glx_dep = declare_dependency(
    link_with : libva_glx,
    include_directories : configinc,
    dependencies : deps)
endif

if WITH_WAYLAND
  libva_wayland_sources = [
    'wayland/va_wayland.c',
    'wayland/va_wayland_drm.c',
    'wayland/va_wayland_emgd.c',
    'drm/va_drm_utils.c',
  ]

  libva_wayland_headers = [
    'wayland/va_backend_wayland.h',
    'wayland/va_wayland.h',
  ]

  libva_wayland_headers_priv = [
    'wayland/va_wayland_drm.h',
    'wayland/va_wayland_emgd.h',
    'wayland/va_wayland_private.h',
  ]

  protocol_files = [
    custom_target(
      'wayland-drm-client-protocol.c',
      output : 'wayland-drm-client-protocol.c',
      input : 'wayland/wayland-drm.xml',
      command : [wl_scanner, 'code', '@INPUT@', '@OUTPUT@']),

    custom_target(
      'wayland-drm-client-protocol.h',
      output : 'wayland-drm-client-protocol.h',
      input : 'wayland/wayland-drm.xml',
      command : [wl_scanner, 'client-header', '@INPUT@', '@OUTPUT@'])
  ]

  install_headers(libva_wayland_headers, subdir : 'va')

  deps = [ libdrm_dep, wayland_dep, libva_dep ]

  libva_wayland = shared_library(
    'va-wayland',
    sources : libva_wayland_sources +
              libva_wayland_headers +
              libva_wayland_headers_priv +
              protocol_files,
    soversion : libva_lt_current,
    version : libva_lt_version,
    install : true,
    dependencies : deps)

  libva_wayland_dep = declare_dependency(
    link_with : libva_wayland,
    include_directories : configinc,
    dependencies : deps)
endif
