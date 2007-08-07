#  @COPYRIGHT@ Intel Confidential - Unreleased Software

libva_la_LTLIBRARIES = libva.la
libva_ladir = $(libdir)
libva_la_LDFLAGS = -version-number 0:1:0 -no-undefined
libva_la_LIBADD = -ldl -lX11 -lXext

libva_la_SOURCES = va_dri.c va.c va_dristr.h

libvaincludedir = ${includedir}
libvainclude_HEADERS = va.h va_x11.h va_dri.h va_backend.h

EXTRA_DIST = ChangeLog TODO
