pkgconf = configuration_data()

pkgconf.set('prefix', get_option('prefix'))
pkgconf.set('exec_prefix', '${prefix}')
pkgconf.set('libdir', '${prefix}/@0@'.format(get_option('libdir')))
pkgconf.set('includedir', '${prefix}/@0@'.format(get_option('includedir')))
pkgconf.set('LIBVA_VERSION', libva_version)
pkgconf.set('VA_API_VERSION', va_api_version)
pkgconf.set('LIBVA_DRIVERS_PATH', driverdir)

pkg_install_dir = '@0@/pkgconfig'.format(get_option('libdir'))

pkg_files = [ 'libva' ]

if WITH_DRM
  pkg_files += [ 'libva-drm' ]
endif

if WITH_X11
  pkg_files += [ 'libva-x11' ]
endif

if WITH_GLX
  pkg_files += [ 'libva-glx' ]
endif

if WITH_WAYLAND
  pkg_files += [ 'libva-wayland' ]
endif

foreach p : pkg_files
  infile = p + '.pc.in'
  outfile = p + '.pc'
  configure_file(
    input : infile,
    output : outfile,
    configuration : pkgconf,
    install_dir : pkg_install_dir)
endforeach
