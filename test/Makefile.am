#  @COPYRIGHT@ Intel Confidential - Unreleased Software

check_PROGRAMS = test
testdir = $(bindir)

# This gives better backtraces when running valgrind:
# test_LDADD = ../src/libva.la ../../psb-video/src/psb_drv_video.la
test_LDADD = ../src/libva.la

AM_CFLAGS = -I$(top_srcdir)/../../include/external/ -I$(top_srcdir)/src
test_SOURCES = test.c

TESTS = $(check_PROGRAMS)