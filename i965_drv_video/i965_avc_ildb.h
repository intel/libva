/*
 * Copyright © 2010 Intel Corporation
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
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Xiang Haihao <haihao.xiang@intel.com>
 *
 */

#ifndef __I965_AVC_ILDB_H__
#define __I965_AVC_ILDB_H__

#define SURFACE_EDGE_CONTROL_DATA       0
#define SURFACE_SRC_Y                   1
#define SURFACE_SRC_UV                  2
#define SURFACE_DEST_Y                  3
#define SURFACE_DEST_UV                 4

#define NUM_AVC_ILDB_SURFACES           5

#define EDGE_CONTROL_DATA_IN_DWS        16
#define EDGE_CONTROL_DATA_IN_BTYES      64

struct i965_avc_ildb_context
{
    struct {
        dri_bo *bo;
    } curbe;

    struct {
        dri_bo *ss_bo;
        dri_bo *s_bo;
        unsigned long offset;
        int surface_type;
        int width;
        int height;
        int depth;
        int pitch;
        int format;
        int vert_line_stride;
        int vert_line_stride_ofs;
        int is_target;
    } surface[NUM_AVC_ILDB_SURFACES];

    struct {
        dri_bo *bo;
    } binding_table;

    struct {
        dri_bo *bo;
    } idrt;

    struct {
        dri_bo *bo;
    } vfe_state;

    struct {
        unsigned int vfe_start;
        unsigned int cs_start;

        unsigned int num_vfe_entries;
        unsigned int num_cs_entries;

        unsigned int size_vfe_entry;
        unsigned int size_cs_entry;
    } urb;

    int picture_type;
    int mbs_per_picture;
};

void i965_avc_ildb(VADriverContextP, struct decode_state *, void *h264_context);
void i965_avc_ildb_decode_init(VADriverContextP, void *h264_context);
Bool i965_avc_ildb_ternimate(struct i965_avc_ildb_context *);

#endif /* __I965_AVC_ILDB_H__ */

