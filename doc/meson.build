if meson.version().version_compare('>= 0.56')
  headerdir = join_paths(meson.project_source_root(), 'va')
else
  headerdir = join_paths(meson.source_root(), 'va')
endif
footer = join_paths(meson.current_source_dir(), 'va_footer.html')

libva_headers_doc = [
  'va.h',
  'va_enc_h264.h',
  'va_enc_mpeg2.h',
  'va_enc_jpeg.h',
  'va_enc_hevc.h',
  'va_enc_vp8.h',
  'va_enc_vp9.h',
  'va_enc_av1.h',
  'va_fei.h',
  'va_fei_h264.h',
  'va_fei_hevc.h',
  'va_dec_hevc.h',
  'va_dec_jpeg.h',
  'va_dec_vp8.h',
  'va_dec_vp9.h',
  'va_dec_av1.h',
  'va_prot.h',
  'va_vpp.h'
]

libva_doc_files = []
headers = ''
foreach h : libva_headers_doc
  libva_doc_files += join_paths (headerdir, h)
  headers += ' ' + join_paths (headerdir, h)
endforeach

config = configuration_data()
config.set('PACKAGE_VERSION', libva_version)
config.set('VA_HEADER_DIR', headerdir)
config.set('VA_HEADER_FILES', headers)
config.set('VA_HTML_FOOTER', footer)
config.set('OUTDIR', meson.current_build_dir())

doxyfile = configure_file(
  input: 'Doxyfile.in',
  output: 'Doxyfile',
  configuration: config)

doc_install_dir = join_paths(get_option('datadir'), 'doc', meson.project_name())
custom_target(
  'doc',
  command: [ doxygen, doxyfile ],
  depend_files: libva_doc_files,
  input: doxyfile,
  output: 'html-out',
  install: true,
  install_dir: doc_install_dir)
