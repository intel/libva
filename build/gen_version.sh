#!/bin/sh

libva_topdir="$1"
version_h_in="$2"

parse_configure_ac() {
    sed -n "/^m4_define.*${1}.*\[\([0-9]*\)\].*/s//\1/p" ${libva_topdir}/configure.ac
}

parse_configure() {
    sed -n "/^${1}=\([0-9]*\)/s//\1/p" ${libva_topdir}/configure
}

if test -f "${libva_topdir}/configure.ac"; then
    libva_major_version=`parse_configure_ac libva_major_version`
    libva_minor_version=`parse_configure_ac libva_minor_version`
    libva_micro_version=`parse_configure_ac libva_micro_version`
elif test -f "${libva_topdir}/configure"; then
    libva_major_version=`parse_configure LIBVA_MAJOR_VERSION`
    libva_minor_version=`parse_configure LIBVA_MINOR_VERSION`
    libva_micro_version=`parse_configure LIBVA_MICRO_VERSION`
else
    echo "ERROR: configure or configure.ac file not found in $libva_topdir/"
    exit 1
fi
libva_version="$libva_major_version.$libva_minor_version.$libva_micro_version"

sed -e "s/@LIBVA_MAJOR_VERSION@/${libva_major_version}/" \
    -e "s/@LIBVA_MINOR_VERSION@/${libva_minor_version}/" \
    -e "s/@LIBVA_MICRO_VERSION@/${libva_micro_version}/" \
    -e "s/@LIBVA_VERSION@/${libva_version}/" \
    $version_h_in
