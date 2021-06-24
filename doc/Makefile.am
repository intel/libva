# Copyright (c) 2007-2011 Intel Corporation. All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

all: html
install-data-local: install-html

EXTRA_DIST = \
	Doxyfile.in			\
	meson.build			\
	$(NULL)

VA_HEADER_DIR   = $(top_srcdir)/va
VA_HEADER_FILES = \
	$(VA_HEADER_DIR)/va.h		\
	$(VA_HEADER_DIR)/va_enc_h264.h	\
	$(VA_HEADER_DIR)/va_enc_mpeg2.h	\
	$(VA_HEADER_DIR)/va_enc_jpeg.h	\
	$(VA_HEADER_DIR)/va_enc_hevc.h	\
	$(VA_HEADER_DIR)/va_enc_vp8.h	\
	$(VA_HEADER_DIR)/va_enc_vp9.h	\
	$(VA_HEADER_DIR)/va_fei.h	\
	$(VA_HEADER_DIR)/va_fei_h264.h	\
	$(VA_HEADER_DIR)/va_fei_hevc.h	\
	$(VA_HEADER_DIR)/va_dec_hevc.h	\
	$(VA_HEADER_DIR)/va_dec_jpeg.h	\
	$(VA_HEADER_DIR)/va_dec_vp8.h	\
	$(VA_HEADER_DIR)/va_dec_vp9.h	\
	$(VA_HEADER_DIR)/va_dec_av1.h	\
	$(VA_HEADER_DIR)/va_prot.h	\
	$(VA_HEADER_DIR)/va_vpp.h	\
	$(NULL)

VA_HTML_FOOTER		= $(top_srcdir)/doc/va_footer.html

Doxyfile: Doxyfile.in $(VA_HEADER_FILES) $(VA_HTML_FOOTER)
	$(AM_V_GEN) $(SED) 					\
           -e "s:\@PACKAGE_VERSION\@:$(PACKAGE_VERSION):"	\
           -e "s:\@VA_HEADER_DIR\@:$(VA_HEADER_DIR):" 		\
           -e "s:\@VA_HEADER_FILES\@:$(VA_HEADER_FILES):"	\
           -e "s:\@VA_HTML_FOOTER\@:$(VA_HTML_FOOTER):"	\
           -e "s:\@OUTDIR\@::" 				\
           $< > $@

html-out/index.html: Doxyfile
	$(AM_V_GEN) $(DOXYGEN)

html: html-out/index.html

install-html-local:
	install -d $(DESTDIR)$(docdir)/html
	for file in `ls html-out/` ; do \
	    if test -f html-out/$$file ; then \
		install -m 0644 html-out/$$file $(DESTDIR)$(docdir)/html ; \
	    else \
		install -d $(DESTDIR)$(docdir)/html/$$file ; \
		install -m 0644 html-out/$$file/* $(DESTDIR)$(docdir)/html/$$file; \
	    fi ; \
	done

uninstall-local:
	$(RM) -rf $(DESTDIR)$(docdir)/html

clean-local:
	$(RM) -rf html-out latex

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in Doxyfile
