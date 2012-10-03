DEBIANFILES = \
	README.Debian					\
	changelog.in					\
	compat						\
	control.in					\
	copyright					\
	libva-dev.install				\
	libva-drm.install.in				\
	libva-egl.install.in				\
	libva-glx.install.in				\
	libva-wayland.install.in			\
	libva-x11.install.in				\
	libva.install.in	 			\
	rules						\
	vainfo.install					\
	$(NULL)

DEBIANGENFILES = \
	changelog					\
	control						\
	libva$(LIBVA_MAJOR_VERSION).install		\
	libva-drm$(LIBVA_MAJOR_VERSION).install		\
	libva-egl$(LIBVA_MAJOR_VERSION).install		\
	libva-glx$(LIBVA_MAJOR_VERSION).install		\
	libva-wayland$(LIBVA_MAJOR_VERSION).install	\
	libva-x11-$(LIBVA_MAJOR_VERSION).install	\
	$(NULL)

EXTRA_DIST = $(DEBIANFILES)

dist_noinst_DATA = $(DEBIANGENFILES)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in $(DEBIANGENFILES)
