#!/bin/sh
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

PROJECT="libva"

test -n "$srcdir" || srcdir="`dirname \"$0\"`"
test -n "$srcdir" || srcdir=.

if ! test -f "$srcdir/configure.ac"; then
    echo "Failed to find the top-level $PROJECT directory"
    exit 1
fi

olddir="`pwd`"
cd "$srcdir"

mkdir -p m4

AUTORECONF=`which autoreconf`
if test -z $AUTORECONF; then
    echo "*** No autoreconf found ***"
    exit 1
else
    autoreconf -v --install || exit $?
fi

cd "$olddir"

if test -z "$NO_CONFIGURE"; then
    $srcdir/configure "$@" && echo "Now type 'make' to compile $PROJECT."
fi
