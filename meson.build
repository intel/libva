# libva package version number, (as distinct from shared library version)
# XXX: we want the package version to remain at 1.0.x for VA-API 0.32.y
#
# - major version is automatically generated from VA-API major version
# - minor version is automatically generated from VA-API minor version
# - increment micro for any library release
# - reset micro version to zero when VA-API major or minor version is changed
project(
  'libva', 'c',
  version : '2.5.0',
  meson_version : '>= 0.37.0',
  default_options : [ 'warning_level=1',
                      'buildtype=debugoptimized' ])

# VA-API version
# - increment major for any ABI change
# - increment minor for any interface change (e.g. new/modified function)
# - increment micro for any other change (new flag, new codec definition, etc.)
# - reset micro version to zero when minor version is incremented
# - reset minor version to zero when major version is incremented
va_api_major_version = 1
va_api_minor_version = 5
va_api_micro_version = 0

va_api_version = '@0@.@1@.@2@'.format(va_api_major_version,
				      va_api_minor_version,
				      va_api_micro_version)

version_arr = meson.project_version().split('.')
libva_major_version = version_arr[0]
libva_minor_version = version_arr[1]
libva_micro_version = version_arr[2]
libva_version = '@0@.@1@.@2@'.format(libva_major_version,
				     libva_minor_version,
				     libva_micro_version)
if version_arr.length() == 4
  libva_version = '@0@.pre@1@'.format(libva_version, version_arr[3])
endif


# libva library version number (generated, do not change)
# XXX: we want the SONAME to remain at libva.so.1 for VA-API major == 0
#
# The library name is generated libva.<x>.<y>.0 where
# <x> = VA-API major version + 1
# <y> = 100 * VA-API minor version + VA-API micro version
#
# For example:
# VA-API 0.32.0 generates libva.so.1.3200.0
# VA-API 0.34.1 generates libva.so.1.3401.0
# VA-API 1.2.13 generates libva.so.2.213.0
libva_interface_bias = va_api_major_version + 1
libva_interface_age = 0
libva_binary_age = 100 * va_api_minor_version + va_api_micro_version - libva_interface_age

libva_lt_current = 100 * va_api_minor_version + va_api_micro_version + libva_interface_bias
libva_lt_revision = libva_interface_age
libva_lt_age = libva_binary_age - libva_interface_age

libva_lt_current = libva_lt_current - libva_lt_age

libva_lt_version = '@0@.@1@.@2@'.format(libva_lt_current,
					libva_lt_age,
					libva_lt_revision)

driverdir = get_option('driverdir')
if driverdir == ''
  driverdir = '@0@/@1@/@2@'.format(get_option('prefix'), get_option('libdir'), 'dri')
endif

configinc = include_directories('.')

cc = meson.get_compiler('c')
dl_dep = cc.find_library('dl', required : false)

libdrm_dep = dependency('libdrm', version : '>= 2.4')

WITH_DRM = not get_option('disable_drm')

WITH_X11 = false
if get_option('with_x11') != 'no'
  x11_dep = dependency('x11', required : get_option('with_x11') == 'yes')
  xext_dep = dependency('xext', required : get_option('with_x11') == 'yes')
  xfixes_dep = dependency('xfixes', required : get_option('with_x11') == 'yes')

  WITH_X11 = (x11_dep.found() and xext_dep.found() and xfixes_dep.found())
endif

if not WITH_X11 and get_option('with_glx') == 'yes'
  error('VA/GLX explicitly enabled, but VA/X11 isn\'t built')
endif

WITH_GLX = false
if WITH_X11 and get_option('with_glx') != 'no'
  gl_dep = dependency('gl', required : get_option('with_glx') == 'yes')
  WITH_GLX = gl_dep.found()
endif

WITH_WAYLAND = false
if get_option('with_wayland') != 'no'
  wayland_dep = dependency('wayland-client', version : '>= 1.11.0',
			   required : get_option('with_wayland') == 'yes')
  if wayland_dep.found()
    prefix = wayland_dep.get_pkgconfig_variable('prefix')
    wl_scanner = find_program('wayland-scanner',
			      prefix + '/bin/wayland-scanner')
  endif
  WITH_WAYLAND = wayland_dep.found()
endif

va_c_args = []
if get_option('enable_va_messaging')
  va_c_args += ['-DENABLE_VA_MESSAGING=1']
endif

# Symbol visibility
if cc.has_argument('-fvisibility=hidden')
  va_c_args += ['-DHAVE_GNUC_VISIBILITY_ATTRIBUTE']
endif

if (not WITH_DRM and not WITH_X11 and not WITH_WAYLAND)
  error('Please install at least one backend dev files (DRM, X11, Wayland)')
endif

subdir('va')
subdir('pkgconfig')

doxygen = find_program('doxygen', required: false)

if get_option('enable_docs') and doxygen.found()
  subdir('doc')
endif
