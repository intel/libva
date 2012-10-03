DEBIANFILES = \
	README.Debian					\
	changelog.in					\
	compat						\
	control.in					\
	copyright					\
	libva-dev.install				\
	libva.install.in	 			\
	rules						\
	vainfo.install					\
	$(NULL)

DEBIANGENFILES = \
	changelog					\
	control						\
	libva$(LIBVA_MAJOR_VERSION).install		\
	$(NULL)

EXTRA_DIST = $(DEBIANFILES)

dist_noinst_DATA = $(DEBIANGENFILES)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in $(DEBIANGENFILES)
