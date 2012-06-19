/*
 * Copyright (c) 2007-2011 Intel Corporation. All Rights Reserved.
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
 * \file va_enc.h
 * \brief The Core encoding API
 *
 * This file contains the \ref api_enc_core "Core encoding API".
 */

#ifndef VA_ENC_H
#define VA_ENC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <va/va.h>

/**
 * \defgroup api_enc_core Core encoding API
 *
 * @{
 */

/** \brief Abstract representation of a bitstream writer. */
typedef struct _VAEncBitstream VAEncBitstream;

/**
 * @name Picture flags
 *
 * Those flags flags are meant to signal when a picture marks the end
 * of a sequence, a stream, or even both at once.
 *
 * @{
 */
/**
 * \brief Marks the last picture in the sequence.
 *
 */
#define VA_ENC_LAST_PICTURE_EOSEQ       0x01
/**
 * \brief Marks the last picture in the stream.
 *
 */
#define VA_ENC_LAST_PICTURE_EOSTREAM    0x02
/**@}*/


/** @name The set of all possible error codes */
/**@{*/
/** \brief An invalid bitstream writer handle was supplied. */
#define VA_ENC_STATUS_ERROR_INVALID_BITSTREAM_WRITER    (-1)
/** \brief An invalid/unsupported parameter value was supplied. */
#define VA_ENC_STATUS_ERROR_INVALID_VALUE               (-2)
/** \brief A buffer overflow has occurred. */
#define VA_ENC_STATUS_ERROR_BUFFER_OVERFLOW             (-3)
/**@}*/

typedef int (*VAEncBitstreamFlushFunc)(
    VAEncBitstream *bs,
    unsigned char  *buffer,
    unsigned int    buffer_size
);

/** \brief Bitstream writer attribute types. */
typedef enum {
    /**
     * \brief User-provided buffer to hold output bitstream (pointer).
     *
     * If this attribute is provided, then \c VAencBitstreamAttribBufferSize
     * shall also be supplied or va_enc_bitstream_new() will ignore that
     * attribute and allocate its own buffer.
     */
    VAEncBitstreamAttribBuffer          = 1,
    /** \brief Size of the user-provided buffer (integer). */
    VAEncBitstreamAttribBufferSize      = 2,
    /** \brief User-provided \c flush() callback (pointer-to-function). */
    VAEncBitstreamAttribFlushFunc       = 3,
    /** \brief Placeholder for codec-specific attributes. */
    VAEncBitstreamAttribMiscMask        = 0x80000000
} VAEncBitstreamAttribType;

/** \brief Bitstream writer attribute value. */
typedef struct {
    /** \brief Attribute type (#VAEncBitstreamAttribType). */
    VAEncBitstreamAttribType    type;
    /** \brief Attribute value (#VAGenericValue). */
    VAGenericValue              value;
} VAEncBitstreamAttrib;

/**
 * \brief Allocates a new bitstream writer.
 *
 * Allocates a new bitstream writer. By default, libva allocates and
 * maintains its own buffer. However, the user can pass down his own
 * buffer with the \c VAEncBitstreamAttribBuffer attribute, along with
 * the size of that buffer with the \c VAEncBitstreamAttribBufferSize
 * attribute.
 *
 * @param[in] attribs       the optional attributes, or NULL
 * @param[in] num_attribs   the number of attributes available in \c attribs
 * @return a new #VAEncBitstream, or NULL if an error occurred
 */
VAEncBitstream *
va_enc_bitstream_new(VAEncBitstreamAttrib *attribs, unsigned int num_attribs);

/**
 * \brief Destroys a bitstream writer.
 *
 * @param[in] bs            the bitstream writer to destroy
 */
void
va_enc_bitstream_destroy(VAEncBitstream *bs);

/**
 * \brief Writes an unsigned integer.
 *
 * Writes an unsigned int value of the specified length in bits. The
 * value is implicitly zero-extended to the number of specified bits.
 *
 * @param[in] bs            the bitstream writer
 * @param[in] value         the unsigned int value to write
 * @param[in] length        the length (in bits) of the value
 * @return the number of bits written, or a negative value to indicate an error
 */
int
va_enc_bitstream_write_ui(VAEncBitstream *bs, unsigned int value, int length);

/**
 * \brief Writes a signed integer.
 *
 * Writes a signed int value of the specified length in bits. The
 * value is implicitly sign-extended to the number of specified bits.
 *
 * @param[in] bs            the bitstream writer
 * @param[in] value         the signed int value to write
 * @param[in] length        the length (in bits) of the value
 * @return the number of bits written, or a negative value to indicate an error
 */
int
va_enc_bitstream_write_si(VAEncBitstream *bs, int value, int length);

#if 0
/* XXX: expose such API? */
int
va_enc_bitstream_skip(VAEncBitstream *bs, unsigned int length);
#endif

/**
 * \brief Byte aligns the bitstream.
 *
 * Align the bitstream to next byte boundary, while filling in bits
 * with the specified value (0 or 1).
 *
 * @param[in] bs            the bitstream writer
 * @param[in] value         the bit filler value (0 or 1)
 * @return the number of bits written, or a negative value to indicate an error
 */
int
va_enc_bitstream_align(VAEncBitstream *bs, unsigned int value);

/**
 * \brief Flushes the bitstream.
 *
 * Flushes the bitstream, while padding with zeroe's up to the next
 * byte boundary. This functions resets the bitstream writer to its
 * initial state. If the user provided a flush function through the
 * \c VAEncBitstreamFlushFunc attribute, then his callback will be
 * called.
 *
 * @param[in] bs            the bitstream writer
 * @return the number of bytes written, or a negative value to indicate an error
 */
int
va_enc_bitstream_flush(VAEncBitstream *bs);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* VA_ENC_H */
