# Copyright (c) 2007 Intel Corporation. All Rights Reserved.
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

noinst_PROGRAMS = test_01 test_02 test_03 test_04 test_05 test_06 \
		test_07 test_08 test_09 test_10 test_11

AM_CFLAGS = -I$(top_srcdir)/va -I$(top_srcdir)/src/x11

TEST_LIBS = $(top_builddir)/va/$(libvabackendlib) $(top_builddir)/va/$(libvacorelib) -lpthread -lX11

test_01_LDADD = $(TEST_LIBS)
test_01_SOURCES = test_01.c

test_02_LDADD = $(TEST_LIBS)
test_02_SOURCES = test_02.c

test_03_LDADD = $(TEST_LIBS)
test_03_SOURCES = test_03.c

test_04_LDADD = $(TEST_LIBS)
test_04_SOURCES = test_04.c

test_05_LDADD = $(TEST_LIBS)
test_05_SOURCES = test_05.c

test_06_LDADD = $(TEST_LIBS)
test_06_SOURCES = test_06.c

test_07_LDADD = $(TEST_LIBS)
test_07_SOURCES = test_07.c

test_08_LDADD = $(TEST_LIBS)
test_08_SOURCES = test_08.c

test_09_LDADD = $(TEST_LIBS)
test_09_SOURCES = test_09.c

test_10_LDADD = $(TEST_LIBS)
test_10_SOURCES = test_10.c

test_11_LDADD = $(TEST_LIBS)
test_11_SOURCES = test_11.c

EXTRA_DIST = test_common.c test_x11.c

valgrind:	$(noinst_PROGRAMS)
	for a in $(noinst_PROGRAMS); do \
		valgrind --leak-check=full --show-reachable=yes .libs/$$a; \
	done
