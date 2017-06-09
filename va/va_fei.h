/*
 * Copyright (c) 2007-2017 Intel Corporation. All Rights Reserved.
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
 * \file va_fei.h
 * \brief The FEI encoding common API
 */

#ifndef VA_FEI_H
#define VA_FEI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * \brief FEI specific attribute definitions
 */
/** @name Attribute values for VAConfigAttribFEIFunctionType
 *
 * This is only for VAEntrypointFEI
 * The desired type should be passed to driver when creating the configuration.
 * If VA_FEI_FUNCTION_ENC_PAK is set, VA_FEI_FUNCTION_ENC and VA_FEI_FUNCTION_PAK
 * will be ignored if set also. Combination of VA_FEI_FUNCTION_ENC and VA_FEI_FUNCTION_PAK
 * is not valid. If  VA_FEI_FUNCTION_ENC is set, there will be no bitstream output.
 * If VA_FEI_FUNCTION_PAK is set, two extra input buffers for PAK are needed:
 * VAEncFEIMVBufferType and VAEncFEIMBCodeBufferType.
 * VA_FEI_FUNCTION_ENC_PAK is recommended for best performance.
 *
 **/
/**@{*/
/** \brief ENC only is supported */
#define VA_FEI_FUNCTION_ENC                             0x00000001
/** \brief PAK only is supported */
#define VA_FEI_FUNCTION_PAK                             0x00000002
/** \brief ENC_PAK is supported */
#define VA_FEI_FUNCTION_ENC_PAK                         0x00000004

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* VA_FEI_H */
