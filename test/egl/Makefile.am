bin_PROGRAMS = va_egl

INCLUDES = -I$(top_srcdir) \
	$(GLESv1_CM_CFLAGS)

TEST_LIBS = $(top_builddir)/va/$(libvabackendlib) $(top_builddir)/va/$(libvacorelib) $(top_builddir)/va/libva-egl.la -lpthread -lX11 -lEGL $(GLESv1_CM_LIBS)

va_egl_LDADD = $(TEST_LIBS)
va_egl_SOURCES = va_egl_x11.c

EXTRA_DIST =

valgrind:	$(bin_PROGRAMS)
	for a in $(bin_PROGRAMS); do \
		valgrind --leak-check=full --show-reachable=yes .libs/$$a; \
	done
