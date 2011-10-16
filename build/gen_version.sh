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
    va_api_major_version=`parse_configure_ac va_api_major_version`
    va_api_minor_version=`parse_configure_ac va_api_minor_version`
    va_api_micro_version=`parse_configure_ac va_api_micro_version`
elif test -f "${libva_topdir}/configure"; then
    va_api_major_version=`parse_configure VA_API_MAJOR_VERSION`
    va_api_minor_version=`parse_configure VA_API_MINOR_VERSION`
    va_api_micro_version=`parse_configure VA_API_MICRO_VERSION`
else
    echo "ERROR: configure or configure.ac file not found in $libva_topdir/"
    exit 1
fi
va_api_version="$va_api_major_version.$va_api_minor_version.$va_api_micro_version"

sed -e "s/@VA_API_MAJOR_VERSION@/${va_api_major_version}/" \
    -e "s/@VA_API_MINOR_VERSION@/${va_api_minor_version}/" \
    -e "s/@VA_API_MICRO_VERSION@/${va_api_micro_version}/" \
    -e "s/@VA_API_VERSION@/${va_api_version}/" \
    $version_h_in
