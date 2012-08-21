/*
 * Copyright (c) 2007-2009 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL INTEL AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SYSDEPS_H
#define SYSDEPS_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#ifdef ANDROID
# define Bool  int
# define True  1
# define False 0

/* Macros generated from configure */
# define LIBVA_VERSION_S "1.1.0"

/* Android logging utilities */
# include <utils/Log.h>

# ifdef ANDROID_ALOG
#  define va_log_error(buffer)  do { ALOGE("%s", buffer); } while (0)
#  define va_log_info(buffer)   do { ALOGI("%s", buffer); } while (0)
# elif ANDROID_LOG
#  define va_log_error(buffer)  do { LOGE("%s", buffer); } while (0)
#  define va_log_info(buffer)   do { LOGI("%s", buffer); } while (0)
# endif
#endif

#ifndef va_log_error
#define va_log_error(buffer) do {                       \
        fprintf(stderr, "libva error: %s", buffer);     \
    } while (0)
#endif

#ifndef va_log_info
#define va_log_info(buffer) do {                        \
        fprintf(stderr, "libva info: %s", buffer);      \
    } while (0)
#endif

#if defined __GNUC__ && defined HAVE_GNUC_VISIBILITY_ATTRIBUTE
# define DLL_HIDDEN __attribute__((visibility("hidden")))
# define DLL_EXPORT __attribute__((visibility("default")))
#else
# define DLL_HIDDEN
# define DLL_EXPORT
#endif

#endif /* SYSDEPS_H */
