DEBIANFILES = \
	README.Debian				\
	changelog.in				\
	compat					\
	control.in				\
	copyright				\
	libva-dev.dirs				\
	libva-dev.install			\
	libva1.dirs				\
	libva1.install				\
	rules					\
	vainfo.install				\
	$(NULL)

DEBIANGENFILES =				\
	changelog				\
	control					\
	$(NULL)

EXTRA_DIST = $(DEBIANFILES)

dist_noinst_DATA = $(DEBIANGENFILES)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in $(DEBIANGENFILES)

$(DEBIANGENFILES): %: %.in Makefile
	-sed \
	  -e 's|\@DATE\@|'"`LC_ALL=C date +'%a, %d %b %Y %X %z'`"'|' \
	  -e 's|\@LIBDRM_VERSION\@|$(LIBDRM_VERSION)|' \
	  -e 's|\@LIBVA_PACKAGE_VERSION\@|$(LIBVA_PACKAGE_VERSION)|' \
	  -e 's|\@PACKAGE_VERSION\@|$(PACKAGE_VERSION)|' \
	  $< > $@
