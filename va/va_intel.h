/*
 * Copyright (c) 2022 Intel Corporation. All Rights Reserved.
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

/**
 * \file va_intel.h
 * \brief The video acceleration Intel specific API
 *
 * This file contains the \ref "Video Acceleration Intel Specific API".
 */

#ifndef VA_INTEL_H
#define VA_INTEL_H

#include <va/va.h>


#ifdef __cplusplus
extern "C" {
#endif

#define VA_INTEL_MEDIA_ENGINE_VDBOX    VA_EXEC_MODE_CUSTOM+1
#define VA_INTEL_MEDIA_ENGINE_VEBOX    VA_EXEC_MODE_CUSTOM+2
#define VA_INTEL_MEDIA_ENGINE_SFC      VA_EXEC_MODE_CUSTOM+3

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* VA_INTEL_H */
