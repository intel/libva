#ifndef _I965_MEDIA_H264_H_
#define _I965_MEDIA_H264_H_

#include "i965_avc_bsd.h"
#include "i965_avc_hw_scoreboard.h"
#include "i965_avc_ildb.h"

struct decode_state;
struct i965_media_context;

#define INST_UNIT_GEN4  16
#define INST_UNIT_GEN5  8

#define MB_CMD_IN_BYTES         64
#define MB_CMD_IN_DWS           16
#define MB_CMD_IN_OWS           4

enum {
    H264_AVC_COMBINED = 0,
    H264_AVC_NULL
};

#define NUM_H264_AVC_KERNELS    2

struct i965_h264_context
{
    struct {
        dri_bo *bo;
        unsigned int mbs;
    } avc_it_command_mb_info;

    struct {
        dri_bo *bo;
        long write_offset;
    } avc_it_data;

    struct {
        dri_bo *bo;
    } avc_ildb_data;

    struct {
        unsigned int width_in_mbs;
        unsigned int height_in_mbs;
        int mbaff_frame_flag;
        int i_flag;
    } picture;

    int enable_avc_ildb;
    int use_avc_hw_scoreboard;

    int use_hw_w128;
    unsigned int weight128_luma_l0;
    unsigned int weight128_luma_l1;
    unsigned int weight128_chroma_l0;
    unsigned int weight128_chroma_l1;
    char weight128_offset0_flag;
    short weight128_offset0;

    struct i965_avc_bsd_context i965_avc_bsd_context;
    struct i965_avc_hw_scoreboard_context avc_hw_scoreboard_context;
    struct i965_avc_ildb_context avc_ildb_context;

    struct {
        VASurfaceID surface_id;
        int frame_store_id;
    } fsid_list[16];

    struct i965_kernel avc_kernels[NUM_H264_AVC_KERNELS];
};

void i965_media_h264_decode_init(VADriverContextP ctx, struct decode_state *decode_state, struct i965_media_context *media_context);
void i965_media_h264_dec_context_init(VADriverContextP ctx, struct i965_media_context *media_context);

#endif /* _I965_MEDIA_H264_H_ */
