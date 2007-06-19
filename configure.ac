#  @COPYRIGHT@ Intel Confidential - Unreleased Software

AC_PREREQ(2.57)
AC_INIT([libva], 0.1, [waldo.bastian@intel.com], libva)
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([dist-bzip2])

AM_CONFIG_HEADER([src/config.h])

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC

AC_HEADER_STDC
AC_SYS_LARGEFILE

PKG_CHECK_MODULES([libva_la], [libdrm])

pkgconfigdir=${libdir}/pkgconfig
AC_SUBST(pkgconfigdir)

AC_OUTPUT([Makefile src/Makefile dummy_drv_video/Makefile test/Makefile libva.pc])
