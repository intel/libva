/*
 * Video Decode Acceleration API Specification
 *
 * Rev. 0.20
 * <jonathan.bian@intel.com>
 *
 * Revision History:
 * rev 0.10 (12/10/06 Jonathan Bian) - Initial draft
 * rev 0.11 (12/15/06 Jonathan Bian) - Fixed some errors
 * rev 0.12 (02/05/07 Jonathan Bian) - Added VC-1 data structures for slice level decode
 * rev 0.13 (02/28/07 Jonathan Bian) - Added GetDisplay()
 * rev 0.14 (04/13/07 Jonathan Bian) - Fixed MPEG-2 PictureParameter structure, cleaned up a few funcs.
 * rev 0.15 (04/20/07 Jonathan Bian) - Overhauled buffer management  
 * rev 0.16 (05/02/07 Jonathan Bian) - Added error codes and fixed some issues with configuration 
 * rev 0.17 (05/07/07 Jonathan Bian) - Added H.264/AVC data structures for slice level decode.
 * rev 0.18 (05/14/07 Jonathan Bian) - Added data structures for MPEG-4 slice level decode 
 *                                     and MPEG-2 motion compensation.
 * rev 0.19 (08/06/07 Jonathan Bian) - Removed extra type for bitplane data (VAPictureBitPlaneBufferType)
 * rev 0.20 (08/07/07 Jonathan Bian) - Added missing fields to VC-1 PictureParameter structure.
 *
 * Acknowledgements:
 *  Thanks to Waldo Bastian for many valuable feedbacks.
 */

#ifndef _VA_H_
#define _VA_H_

#ifdef __cplusplus
extern "C" {
#endif

/* 
Overview 

This is a decode only interface currently.  The basic steps are:

- Negotiate a mutually acceptable configuration with the server to lock
  down profile, entrypoints, and other attributes that will not change on 
  a frame-by-frame basis.
- Create a decode context which represents a "virtualized" hardware decode 
  device
- Get and fill decode buffers with picture level, slice level and macroblock 
  level data (depending on entrypoints)
- Pass the decode buffers to the server to decode the current frame

Initialization & Configuration Management 

- Find out supported profiles
- Find out entrypoints for a given profile
- Find out configuration attributes for a given profile/entrypoint pair
- Create a configuration for use by the decoder

*/

typedef void* VADisplay;	/* window system dependent */

typedef int VAStatus;	/* Return status type from functions */
/* Values for the return status */
#define VA_STATUS_SUCCESS			0x00000000
#define VA_STATUS_ERROR_ALLOCATION_FAILED	0x00000001
#define VA_STATUS_ERROR_INVALID_CONFIG		0x00000002
#define VA_STATUS_ERROR_INVALID_CONTEXT		0x00000003
#define VA_STATUS_ERROR_INVALID_SURFACE		0x00000004
#define VA_STATUS_ERROR_INVALID_BUFFER		0x00000005
#define VA_STATUS_ERROR_ATTR_NOT_SUPPORTED	0x00000006 /* Todo: Remove */
#define VA_STATUS_ERROR_MAX_NUM_EXCEEDED	0x00000007
#define VA_STATUS_ERROR_UNSUPPORTED_PROFILE		0x00000008
#define VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT	0x00000009
#define VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT	0x0000000a
#define VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE	0x0000000b
#define VA_STATUS_ERROR_UNKNOWN			0xFFFFFFFF

/*
 * Returns a short english description of error_status
 */
const char *vaErrorStr(VAStatus error_status);

/*
 * Initialization:
 * A display must be obtained by calling vaGetDisplay() before calling
 * vaInitialize() and other functions. This connects the API to the 
 * native window system.
 * For X Windows, native_dpy would be from XOpenDisplay()
 */
typedef void* NativeDisplay;	/* window system dependent */

VADisplay vaGetDisplay (
    NativeDisplay native_dpy	/* implementation specific */
);

VAStatus vaInitialize (
    VADisplay dpy,
    int *major_version,	 /* out */
    int *minor_version 	 /* out */
);

/*
 * After this call, all library internal resources will be cleaned up
 */ 
VAStatus vaTerminate (
    VADisplay dpy
);

/* Currently defined profiles */
typedef enum
{
    VAProfileMPEG2Simple		= 0,
    VAProfileMPEG2Main			= 1,
    VAProfileMPEG4Simple		= 2,
    VAProfileMPEG4AdvancedSimple	= 3,
    VAProfileMPEG4Main			= 4,
    VAProfileH264Baseline		= 5,
    VAProfileH264Main			= 6,
    VAProfileH264High			= 7,
    VAProfileVC1Simple			= 8,
    VAProfileVC1Main			= 9,
    VAProfileVC1Advanced		= 10,
} VAProfile;

/* Currently defined entrypoints */
typedef enum
{
    VAEntrypointVLD		= 1,
    VAEntrypointIZZ		= 2,
    VAEntrypointIDCT		= 3,
    VAEntrypointMoComp		= 4,
    VAEntrypointDeblocking	= 5,
} VAEntrypoint;

/* Currently defined configuration attribute types */
typedef enum
{
    VAConfigAttribRTFormat		= 0,
    VAConfigAttribSpatialResidual	= 1,
    VAConfigAttribSpatialClipping	= 2,
    VAConfigAttribIntraResidual		= 3,
    VAConfigAttribEncryption		= 4,
} VAConfigAttribType;

/*
 * Configuration attributes
 * If there is more than one value for an attribute, a default
 * value will be assigned to the attribute if the client does not
 * specify the attribute when creating a configuration
 */
typedef struct _VAConfigAttrib {
    VAConfigAttribType type;
    unsigned int value; /* OR'd flags (bits) for this attribute */
} VAConfigAttrib;

/* attribute value for VAConfigAttribRTFormat */
#define VA_RT_FORMAT_YUV420	0x00000001	
#define VA_RT_FORMAT_YUV422	0x00000002
#define VA_RT_FORMAT_YUV444	0x00000004

/*
 * if an attribute is not applicable for a given
 * profile/entrypoint pair, then set the value to the following 
 */
#define VA_ATTRIB_NOT_SUPPORTED 0x80000000

/* Get maximum number of profiles supported by the implementation */
int vaMaxNumProfiles (
    VADisplay dpy
);

/* Get maximum number of entrypoints supported by the implementation */
int vaMaxNumEntrypoints (
    VADisplay dpy
);

/* Get maximum number of attributs supported by the implementation */
int vaMaxNumConfigAttributes (
    VADisplay dpy
);

/* 
 * Query supported profiles 
 * The caller must provide a "profile_list" array that can hold at
 * least vaMaxNumProfile() entries. The actual number of profiles
 * returned in "profile_list" is returned in "num_profile".
 */
VAStatus vaQueryConfigProfiles (
    VADisplay dpy,
    VAProfile *profile_list,	/* out */
    int *num_profiles		/* out */
);

/* 
 * Query supported entrypoints for a given profile 
 * The caller must provide an "entrypoint_list" array that can hold at
 * least vaMaxNumEntrypoints() entries. The actual number of entrypoints 
 * returned in "entrypoint_list" is returned in "num_entrypoints".
 */
VAStatus vaQueryConfigEntrypoints (
    VADisplay dpy,
    VAProfile profile,
    VAEntrypoint *entrypoint_list,	/* out */
    int *num_entrypoints		/* out */
);

/* 
 * Query attributes for a given profile/entrypoint pair 
 * The caller must provide an “attrib_list” with all attributes to be 
 * queried.  Upon return, the attributes in “attrib_list” have been 
 * updated with their value.  Unknown attributes or attributes that are 
 * not supported for the given profile/entrypoint pair will have their 
 * value set to VA_ATTRIB_NOT_SUPPORTED
 */
VAStatus vaQueryConfigAttributes (
    VADisplay dpy,
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list, /* in/out */
    int num_attribs
);

typedef int VAConfigID;

/* 
 * Create a configuration for the decode pipeline 
 * it passes in the attribute list that specifies the attributes it cares 
 * about, with the rest taking default values.  
 */
VAStatus vaCreateConfig (
    VADisplay dpy,
    VAProfile profile, 
    VAEntrypoint entrypoint, 
    VAConfigAttrib *attrib_list,
    int num_attribs,
    VAConfigID *config_id /* out */
);

/* 
 * Get all attributes for a given configuration 
 * The profile of the configuration is returned in “profile”
 * The entrypoint of the configuration is returned in “entrypoint”
 * The caller must provide an “attrib_list” array that can hold at least 
 * vaMaxNumConfigAttributes() entries. The actual number of attributes 
 * returned in “attrib_list” is returned in “num_attribs”
 */
VAStatus vaGetConfigAttributes (
    VADisplay dpy,
    VAConfigID config_id, 
    VAProfile *profile, 	/* out */
    VAEntrypoint *entrypoint, 	/* out */
    VAConfigAttrib *attrib_list,/* out */
    int *num_attribs 		/* out */
);


/*
 * Context 
 *
 * Context represents a "virtual" video decode pipeline
 */

/* generic context ID type, can be re-typed for specific implementation */
typedef int VAContextID;

/* generic surface ID type, can be re-typed for specific implementation */
typedef int VASurfaceID;

#define VA_INVALID_SURFACE	-1

typedef struct _VAContext
{
    VAContextID		context_id; /* to identify this context */
    VAConfigID		config_id;
    unsigned short	picture_width;
    unsigned short	picture_height;
    VASurfaceID		*render_targets;
    int			num_render_targets;	
    int 		flags;
    void		*privData;	 
} VAContext;

/*
    flags - Any combination of the following:
      VA_PROGRESSIVE (only progressive frame pictures in the sequence when set)
*/
#define VA_PROGRESSIVE	0x1

/*

Surface Management 

Surfaces are render targets for a given context. The data in the surfaces 
are not accessible to the client and the internal data format of
the surface is implementatin specific. 

Question: Is there a need to know the data format (fourcc) or just 
differentiate between 420/422/444 is sufficient?

*/

typedef struct _VASurface
{
    VASurfaceID		surface_id; /* uniquely identify this surface */
    VAContextID		context_id; /* which context does this surface belong */
    unsigned short	width;
    unsigned short	height;
    int			format; /* 420/422/444 */
    void		*privData; /* private to the library */
} VASurface;

/* 
 * Surfaces will be bound to a context when the context is created. Once
 * a surface is bound to a given context, it can not be used to create
 * another context. The association is removed when the context is destroyed
 */

/* Surface Functions */
VAStatus vaCreateSurfaces (
    VADisplay dpy,
    int width,
    int height,
    int format,
    int num_surfaces,
    VASurface *surfaces	/* out */
);

/*
 * surfaces can only be destroyed after the context associated has been 
 * destroyed
 */
VAStatus vaDestroySurface (
    VADisplay dpy,
    VASurface *surface_list,
    int num_surfaces
);

VAStatus vaCreateContext (
    VADisplay dpy,
    VAConfigID config_id,
    int picture_width,
    int picture_height,
    int flag,
    VASurface *render_targets,
    int num_render_targets,
    VAContext *context		/* out */
);

VAStatus vaDestroyContext (
    VADisplay dpy,
    VAContext *context
);

/*
 *
 *	Buffers 
 *	Buffers are used to pass various types of data from the
 *	client to the server. The server maintains a data store
 *	for each buffer created, and the client idenfies a buffer
 *	through a unique buffer id assigned by the server.
 *
 */

typedef int VABufferID;

typedef enum
{
    VAPictureParameterBufferType	= 0,
    VAIQMatrixBufferType		= 1,
    VABitPlaneBufferType		= 2,
    VASliceGroupMapBufferType		= 3,
    VASliceParameterBufferType		= 4,
    VASliceDataBufferType		= 5,
    VAMacroblockParameterBufferType	= 6,
    VAResidualDataBufferType		= 7,
    VADeblockingParameterBufferType	= 8
} VABufferType;

/****************************
 * MPEG-2 data structures
 ****************************/
 
/* MPEG-2 Picture Parameter Buffer */
/* 
 * For each frame or field, and before any slice data, a single
 * picture parameter buffer must be send.
 */
typedef struct _VAPictureParameterBufferMPEG2
{
    unsigned short horizontal_size;
    unsigned short vertical_size;
    VASurfaceID forward_reference_picture;
    VASurfaceID backward_reference_picture;
    /* meanings of the following fields are the same as in the standard */
    int picture_coding_type;
    int f_code; /* pack all four fcode into this */
    union {
        struct {
            unsigned char intra_dc_precision		: 2; 
            unsigned char picture_structure		: 2; 
            unsigned char top_field_first		: 1; 
            unsigned char frame_pred_frame_dct		: 1; 
            unsigned char concealment_motion_vectors	: 1;
            unsigned char q_scale_type			: 1;
            unsigned char intra_vlc_format		: 1;
            unsigned char alternate_scan		: 1;
            unsigned char repeat_first_field		: 1;
            unsigned char progressive_frame		: 1;
        };
        unsigned int picture_coding_extension;
    };
} VAPictureParameterBufferMPEG2;

/* MPEG-2 Inverse Quantization Matrix Buffer */
typedef struct _VAIQMatrixBufferMPEG2
{
    int load_intra_quantiser_matrix;
    int load_non_intra_quantiser_matrix;
    int load_chroma_intra_quantiser_matrix;
    int load_chroma_non_intra_quantiser_matrix;
    unsigned char intra_quantiser_matrix[64];
    unsigned char non_intra_quantiser_matrix[64];
    unsigned char chroma_intra_quantiser_matrix[64];
    unsigned char chroma_non_intra_quantiser_matrix[64];
} VAIQMatrixBufferMPEG2;

/* 
 * There will be cases where the bitstream buffer will not have enough room to hold
 * the data for the entire slice, and the following flags will be used in the slice
 * parameter to signal to the server for the possible cases.
 * If a slice parameter buffer and slice data buffer pair is sent to the server with 
 * the slice data partially in the slice data buffer (BEGIN and MIDDLE cases below), 
 * then a slice parameter and data buffer needs to be sent again to complete this slice. 
 */
#define VA_SLICE_DATA_FLAG_ALL		0x00	/* whole slice is in the buffer */
#define VA_SLICE_DATA_FLAG_BEGIN	0x01	/* The beginning of the slice is in the buffer but the end if not */
#define VA_SLICE_DATA_FLAG_MIDDLE	0x02	/* Neither beginning nor end of the slice is in the buffer */
#define VA_SLICE_DATA_FLAG_END		0x04	/* end of the slice is in the buffer */

/* MPEG-2 Slice Parameter Buffer */
typedef struct _VASliceParameterBufferMPEG2
{
    unsigned int slice_data_size;/* number of bytes in the slice data buffer for this slice */
    unsigned int slice_data_offset;/* the offset to the first byte of slice data */
    unsigned int slice_data_flag; /* see VA_SLICE_DATA_FLAG_XXX defintions */
    unsigned int macroblock_offset;/* the offset to the first bit of MB from the first byte of slice data */
    unsigned int slice_vertical_position;
    int quantiser_scale_code;
    int intra_slice_flag;
} VASliceParameterBufferMPEG2;

/* MPEG-2 Macroblock Parameter Buffer */
typedef struct _VAMacroblockParameterBufferMPEG2
{
    unsigned short macroblock_address;
    /* 
     * macroblock_address (in raster scan order)
     * top-left: 0
     * bottom-right: picture-height-in-mb*picture-width-in-mb - 1
     */
    unsigned char macroblock_type;  /* see definition below */
    union {
        struct {
            unsigned char frame_motion_type		: 2; 
            unsigned char field_motion_type		: 2; 
            unsigned char dct_type			: 1; 
        };
        unsigned char macroblock_modes;
    };
    unsigned char motion_vertical_field_select; 
    /* 
     * motion_vertical_field_select:
     * see section 6.3.17.2 in the spec
     * only the lower 4 bits are used
     * bit 0: first vector forward
     * bit 1: first vector backward
     * bit 2: second vector forward
     * bit 3: second vector backward
     */
    short PMV[2][2][2]; /* see Table 7-7 in the spec */
    unsigned short coded_block_pattern;
    /* 
     * The bitplanes for coded_block_pattern are described 
     * in Figure 6.10-12 in the spec
     */
} VAMacroblockParameterBufferMPEG2;

/* 
 * OR'd flags for macroblock_type (section 6.3.17.1 in the spec)
 */
#define VA_MB_TYPE_MOTION_FORWARD	0x02
#define VA_MB_TYPE_MOTION_BACKWARD	0x04
#define VA_MB_TYPE_MOTION_PATTERN	0x08
#define VA_MB_TYPE_MOTION_INTRA		0x10

/* 
 * MPEG-2 Residual Data Buffer 
 * For each macroblock, there wil be 64 shorts (16-bit) in the 
 * residual data buffer
 */

/****************************
 * MPEG-4 Part 2 data structures
 ****************************/
 
/* MPEG-4 Picture Parameter Buffer */
/* 
 * For each frame or field, and before any slice data, a single
 * picture parameter buffer must be send.
 */
typedef struct _VAPictureParameterBufferMPEG4
{
    unsigned short vop_width;
    unsigned short vop_height;
    VASurfaceID forward_reference_picture;
    VASurfaceID backward_reference_picture;
    union {
        struct {
            unsigned char short_video_header		: 1; 
            unsigned char chroma_format			: 2; 
            unsigned char interlaced			: 1; 
            unsigned char obmc_disable			: 1; 
            unsigned char sprite_enable			: 2; 
            unsigned char sprite_warping_accuracy	: 2; 
            unsigned char quant_type			: 1; 
            unsigned char quarter_sample		: 1; 
            unsigned char data_partitioned		: 1; 
            unsigned char reversible_vlc		: 1; 
        };
        unsigned short vol_fields;
    };
    unsigned char no_of_sprite_warping_points;
    short sprite_trajectory_du[3];
    short sprite_trajectory_dv[3];
    unsigned char quant_precision;
    union {
        struct {
            unsigned char vop_coding_type		: 2; 
            unsigned char backward_reference_vop_coding_type	: 2; 
            unsigned char vop_rounding_type		: 1; 
            unsigned char intra_dc_vlc_thr		: 3; 
            unsigned char top_field_first		: 1; 
            unsigned char alternate_vertical_scan_flag	: 1; 
        };
        unsigned short vop_fields;
    };
    unsigned char vop_fcode_forward;
    unsigned char vop_fcode_backward;
    /* short header related */
    unsigned char num_gobs_in_vop;
    unsigned char num_macroblocks_in_gob;
    /* for direct mode prediction */
    short TRB;
    short TRD;
} VAPictureParameterBufferMPEG4;

/* MPEG-4 Inverse Quantization Matrix Buffer */
typedef struct _VAIQMatrixBufferMPEG4
{
    int load_intra_quant_mat;
    int load_non_intra_quant_mat;
    unsigned char intra_quant_mat[64];
    unsigned char non_intra_quant_mat[64];
} VAIQMatrixBufferMPEG4;

/* MPEG-4 Slice Parameter Buffer */
typedef struct _VASliceParameterBufferMPEG4
{
    unsigned int slice_data_size;/* number of bytes in the slice data buffer for this slice */
    unsigned int slice_data_offset;/* the offset to the first byte of slice data */
    unsigned int slice_data_flag; /* see VA_SLICE_DATA_FLAG_XXX defintions */
    unsigned int macroblock_offset;/* the offset to the first bit of MB from the first byte of slice data */
    unsigned int macroblock_number;
    int quant_scale;
} VASliceParameterBufferMPEG4;

/*
 VC-1 data structures
*/
 
/* VC-1 Picture Parameter Buffer */
/* 
 * For each picture, and before any slice data, a picture parameter
 * buffer must be send. Multiple picture parameter buffers may be
 * sent for a single picture. In that case picture parameters will
 * apply to all slice data that follow it until a new picture
 * parameter buffer is sent.
 */
typedef struct _VAPictureParameterBufferVC1
{
    VASurfaceID forward_reference_picture;
    VASurfaceID backward_reference_picture;
    /* if out-of-loop post-processing is done on the render
       target, then we need to keep the in-loop decoded 
       picture as a reference picture */
    VASurfaceID inloop_decoded_picture;

    /* sequence layer for AP or meta data for SP and MP */
    union {
        struct {
            unsigned char interlace	: 1; /* SEQUENCE_LAYER::INTERLACE */
            unsigned char syncmarker	: 1;/* METADATA::SYNCMARKER */
            unsigned char overlap	: 1;/* METADATA::OVERLAP */
        };
        unsigned char sequence_fields;
    };

    unsigned short coded_width;		/* ENTRY_POINT_LAYER::CODED_WIDTH */
    unsigned short coded_height;	/* ENTRY_POINT_LAYER::CODED_HEIGHT */
    unsigned char closed_entry;		/* ENTRY_POINT_LAYER::CLOSED_ENTRY */
    unsigned char broken_link;		/* ENTRY_POINT_LAYER::BROKEN_LINK */
    unsigned char conditional_overlap_flag; /* ENTRY_POINT_LAYER::CONDOVER */
    unsigned char fast_uvmc_flag;	/* ENTRY_POINT_LAYER::FASTUVMC */
    union {
        struct {
            unsigned char range_mapping_luma_flag: 	1; /* ENTRY_POINT_LAYER::RANGE_MAPY_FLAG */
            unsigned char range_mapping_luma: 		3; /* ENTRY_POINT_LAYER::RANGE_MAPY */
            unsigned char range_mapping_choma_flag:	1; /* ENTRY_POINT_LAYER::RANGE_MAPUV_FLAG */
            unsigned char range_mapping_choma:		3; /* ENTRY_POINT_LAYER::RANGE_MAPUV */
        };
        unsigned char range_mapping_fields;
    };

    unsigned char b_picture_fraction;	/* PICTURE_LAYER::BFRACTION */
    unsigned char cbp_table;		/* PICTURE_LAYER::CBPTAB/ICBPTAB */
    unsigned char mb_mode_table;	/* PICTURE_LAYER::MBMODETAB */
    unsigned char range_reduction_frame;/* PICTURE_LAYER::RANGEREDFRM */
    unsigned char rounding_control;	/* PICTURE_LAYER::RNDCTRL */
    unsigned char post_processing;	/* PICTURE_LAYER::POSTPROC */
    unsigned char picture_resolution_index;	/* PICTURE_LAYER::RESPIC */
    unsigned char luma_scale;		/* PICTURE_LAYER::LUMSCALE */
    unsigned char luma_shift;		/* PICTURE_LAYER::LUMSHIFT */
    union {
        struct {
            unsigned char picture_type	: 2; 	/* PICTURE_LAYER::PTYPE */
            unsigned char frame_coding_mode	: 3;/* PICTURE_LAYER::FCM */
            unsigned char top_field_first	: 1;/* PICTURE_LAYER::TFF */
            unsigned char is_first_field	: 1; /* set to 1 if it is first field */
            unsigned char intensity_compensation: 1;/* PICTURE_LAYER::INTCOMP */
        };
        unsigned char picture_fields;
    };
    union {
       struct {
            unsigned char mv_type_mb	: 1; 	/* PICTURE::MVTYPEMB */
            unsigned char direct_mb	: 1; 	/* PICTURE::DIRECTMB */
            unsigned char skip_mb	: 1; 	/* PICTURE::SKIPMB */
            unsigned char field_tx	: 1; 	/* PICTURE::FIELDTX */
            unsigned char foward_mb	: 1;	/* PICTURE::FORWARDMB */
            unsigned char ac_pred	: 1;	/* PICTURE::ACPRED */
            unsigned char overflags	: 1;	/* PICTURE::OVERFLAGS */
        };
        unsigned char raw_coding_flag;
    };
    union {
        struct {
            unsigned char reference_distance_flag : 1;/* PICTURE_LAYER::REFDIST_FLAG */
            unsigned char reference_distance	: 1;/* PICTURE_LAYER::REFDIST */
            unsigned char num_reference_pictures: 1;/* PICTURE_LAYER::NUMREF */
            unsigned char reference_field_pic_indicator	: 1;/* PICTURE_LAYER::REFFIELD */
        };
        unsigned short reference_fields;
    };
    union {
        struct {
            unsigned char mv_mode	: 2; 	/* PICTURE_LAYER::MVMODE */
            unsigned char mv_mode2	: 2; 	/* PICTURE_LAYER::MVMODE2 */
            unsigned char mv_table	: 3;/* PICTURE_LAYER::MVTAB/IMVTAB */
            unsigned char two_mv_block_pattern_table: 2;/* PICTURE_LAYER::2MVBPTAB */
            unsigned char four_mv_switch: 1; 	/* PICTURE_LAYER::4MVSWITCH */
            unsigned char four_mv_block_pattern_table : 2;/* PICTURE_LAYER::4MVBPTAB */
            unsigned char extended_mv_flag: 1;/* ENTRY_POINT_LAYER::EXTENDED_MV */
            unsigned char extended_mv_range : 2;/* PICTURE_LAYER::MVRANGE */
            unsigned char extended_dmv_flag : 1;/* ENTRY_POINT_LAYER::EXTENDED_DMV */
            unsigned char extended_dmv_range : 2;/* PICTURE_LAYER::DMVRANGE */
        };
        unsigned int mv_fields;
    };
    union {
        struct {
            unsigned char dquant	: 2; 	/* ENTRY_POINT_LAYER::DQUANT */
            unsigned char half_qp	: 1; 	/* PICTURE_LAYER::HALFQP */
            unsigned char pic_quantizer_scale : 1;/* PICTURE_LAYER::PQUANT */
            unsigned char pic_quantizer_type : 1;/* PICTURE_LAYER::PQUANTIZER */
            unsigned char dq_frame	: 1; 	/* VOPDQUANT::DQUANTFRM */
            unsigned char dq_profile	: 2; 	/* VOPDQUANT::DQPROFILE */
            unsigned char dq_sb_edge	: 2; 	/* VOPDQUANT::DQSBEDGE */
            unsigned char dq_db_edge 	: 2; 	/* VOPDQUANT::DQDBEDGE */
            unsigned char dq_binary_level : 1; 	/* VOPDQUANT::DQBILEVEL */
            unsigned char alt_pic_quantizer : 5;/* VOPDQUANT::ALTPQUANT */
        };
        unsigned short pic_quantizer_fields;
    };
    union {
        struct {
            unsigned char variable_sized_transform_flag	: 1;/* ENTRY_POINT_LAYER::VSTRANSFORM */
            unsigned char mb_level_transform_type_flag	: 1;/* PICTURE_LAYER::TTMBF */
            unsigned char frame_level_transform_type	: 2;/* PICTURE_LAYER::TTFRM */
            unsigned char transform_ac_codingset_idx1	: 2;/* PICTURE_LAYER::TRANSACFRM */
            unsigned char transform_ac_codingset_idx2	: 2;/* PICTURE_LAYER::TRANSACFRM2 */
            unsigned char intra_transform_dc_table	: 1;/* PICTURE_LAYER::TRANSDCTAB */
        };
        unsigned short transform_fields;
    };
} VAPictureParameterBufferVC1;

/* VC-1 Bitplane Buffer 
There will be at most three bitplanes coded in any picture header. To send 
the bitplane data more efficiently, each byte is divided in two nibbles, with
each nibble carrying three bitplanes for one macroblock.  The following table
shows the bitplane data arrangement within each nibble based on the picture
type.

Picture Type	Bit3		Bit2		Bit1		Bit0
I or BI				OVERFLAGS	ACPRED		FIELDTX
P				MYTYPEMB	SKIPMB		DIRECTMB
B				FORWARDMB	SKIPMB		DIRECTMB

Within each byte, the lower nibble is for the first MB and the upper nibble is 
for the second MB.  E.g. the lower nibble of the first byte in the bitplane
buffer is for Macroblock #1 and the upper nibble of the first byte is for 
Macroblock #2 in the first row.
*/

/* VC-1 Slice Parameter Buffer */
typedef struct _VASliceParameterBufferVC1
{
    unsigned int slice_data_size;/* number of bytes in the slice data buffer for this slice */
    unsigned int slice_data_offset;/* the offset to the first byte of slice data */
    unsigned int slice_data_flag; /* see VA_SLICE_DATA_FLAG_XXX defintions */
    unsigned int macroblock_offset;/* the offset to the first bit of MB from the first byte of slice data */
    unsigned int slice_vertical_position;
} VASliceParameterBufferVC1;

/* VC-1 Slice Data Buffer */
/* 
This is simplely a buffer containing raw bit-stream bytes 
*/

/****************************
 * H.264/AVC data structures
 ****************************/

typedef struct _VAPictureH264
{
    VASurfaceID picture_id;
    unsigned int flags;
    unsigned int TopFieldOrderCnt;
    unsigned int BottomFieldOrderCnt;
} VAPictureH264;
/* flags in VAPictureH264 could be OR of the following */
#define VA_PICTURE_H264_INVALID			0x00000001
#define VA_PICTURE_H264_TOP_FIELD		0x00000002
#define VA_PICTURE_H264_BOTTOM_FIELD		0x00000004
#define VA_PICTURE_H264_SHORT_TERM_REFERENCE	0x00000008
#define VA_PICTURE_H264_LONG_TERM_REFERENCE	0x00000010
#define VA_PICTURE_H264_USED_AS_REFERENCE	0x00000020

/* H.264 Picture Parameter Buffer */
/* 
 * For each picture, and before any slice data, a single
 * picture parameter buffer must be send.
 */
typedef struct _VAPictureParameterBufferH264
{
    VAPictureH264 CurrPic;
    VAPictureH264 ReferenceFrames[16];	/* in DPB */
    unsigned short picture_width_in_mbs_minus1;
    unsigned short picture_height_in_mbs_minus1;
    unsigned char bit_depth_luma_minus8;
    unsigned char bit_depth_chroma_minus8;
    unsigned char num_ref_frames;
    union {
        struct {
            unsigned char chroma_format_idc			: 2; 
            unsigned char residual_colour_transform_flag	: 1; 
            unsigned char frame_mbs_only_flag			: 1; 
            unsigned char mb_adaptive_frame_field_flag		: 1; 
            unsigned char direct_8x8_inference_flag		: 1; 
        };
        unsigned char seq_fields;
    };
    unsigned char num_slice_groups_minus1;
    unsigned char slice_group_map_type;
    unsigned char pic_init_qp_minus26;
    unsigned char chroma_qp_index_offset;
    unsigned char second_chroma_qp_index_offset;
    union {
        struct {
            unsigned char entropy_coding_mode_flag	: 1; 
            unsigned char weighted_pred_flag		: 1; 
            unsigned char weighted_bipred_idc		: 1; 
            unsigned char transform_8x8_mode_flag	: 1; 
            unsigned char field_pic_flag		: 1;
        };
        unsigned char pic_fields;
    };
    unsigned short frame_num;
} VAPictureParameterBufferH264;

/* H.264 Inverse Quantization Matrix Buffer */
typedef struct _VAIQMatrixBufferH264
{
    unsigned char ScalingList4x4[6][16];
    unsigned char ScalingList8x8[2][64];
} VAIQMatrixBufferH264;

/* 
 * H.264 Slice Group Map Buffer 
 * When VAPictureParameterBufferH264::num_slice_group_minus1 is not equal to 0,
 * A slice group map buffer should be sent for each picture if required. The buffer
 * is sent only when there is a change in the mapping values.
 * The slice group map buffer map "map units" to slice groups as specified in 
 * section 8.2.2 of the H.264 spec. The buffer will contain one byte for each macroblock 
 * in raster scan order
 */ 

/* H.264 Slice Parameter Buffer */
typedef struct _VASliceParameterBufferH264
{
    unsigned int slice_data_size;/* number of bytes in the slice data buffer for this slice */
    unsigned int slice_data_offset;/* the offset to first byte of slice data */
    unsigned int slice_data_flag; /* see VA_SLICE_DATA_FLAG_XXX defintions */
    unsigned short slice_data_bit_offset; /* bit offset in the first byte of valid data */
    unsigned short first_mb_in_slice;
    unsigned char slice_type;
    unsigned char direct_spatial_mv_pred_flag;
    unsigned char num_ref_idx_10_active_minus1;
    unsigned char num_ref_idx_11_active_minus1;
    unsigned char cabac_init_idc;
    char slice_qp_delta;
    unsigned char disable_deblocking_filter_idc;
    char slice_alpha_c0_offset_div2;
    char slice_beta_offset_div2;
    VAPictureH264 RefPicList0[32];	/* See 8.2.4.2 */
    VAPictureH264 RefPicList1[32];	/* See 8.2.4.2 */
    unsigned char luma_log2_weight_denom;
    unsigned char chroma_log2_weight_denom;
} VASliceParameterBufferH264;

/* Buffer functions */

/*
 * Creates a buffer for storing a certain type of data, no data store allocated
 */
VAStatus vaCreateBuffer (
    VADisplay dpy,
    VABufferType type,	/* in */
    VABufferID *buf_id	/* out */
);

/*
 * Create data store for the buffer and initalize with "data".
 * if "data" is null, then the contents of the buffer data store
 * are undefined.
 * Basically there are two ways to get buffer data to the server side. One is 
 * to call vaBufferData() with a non-null "data", which results the data being
 * copied to the data store on the server side.  A different method that 
 * eliminates this copy is to pass null as "data" when calling vaBufferData(),
 * and then use vaMapBuffer() to map the data store from the server side to the
 * client address space for access.
 */
VAStatus vaBufferData (
    VADisplay dpy,
    VABufferID buf_id,	/* in */
    unsigned int size,	/* in */
    unsigned int num_elements, /* in */
    void *data		/* in */
);

/*
 * Convey to the server how many valid elements are in the buffer. 
 * e.g. if multiple slice parameters are being held in a single buffer,
 * this will communicate to the server the number of slice parameters
 * that are valid in the buffer.
 */
VAStatus vaBufferSetNumElements (
    VADisplay dpy,
    VABufferID buf_id,	/* in */
    unsigned int num_elements /* in */
);

/*
 * Map data store of the buffer into the client's address space
 * vaBufferData() needs to be called with "data" set to NULL before
 * calling vaMapBuffer()
 */
VAStatus vaMapBuffer (
    VADisplay dpy,
    VABufferID buf_id,	/* in */
    void **pbuf 	/* out */
);

/*
 * After client making changes to a mapped data store, it needs to
 * "Unmap" it to let the server know that the data is ready to be
 * consumed by the server
 */
VAStatus vaUnmapBuffer (
    VADisplay dpy,
    VABufferID buf_id	/* in */
);

/*
 * After this call, the buffer is deleted and this buffer_id is no longer valid
 */
VAStatus vaDestroyBuffer (
    VADisplay dpy,
    VABufferID buffer_id
);

/*
Render (Decode) Pictures

A picture represents either a frame or a field.

The Begin/Render/End sequence sends the decode buffers to the server
*/

/*
 * Get ready to decode a picture to a target surface
 */
VAStatus vaBeginPicture (
    VADisplay dpy,
    VAContext *context,
    VASurface *render_target
);

/* 
 * Send decode buffers to the server.
 */
VAStatus vaRenderPicture (
    VADisplay dpy,
    VAContext *context,
    VABufferID *buffers,
    int num_buffers
);

/* 
 * Make the end of rendering for a picture. 
 * The server should start processing all pending operations for this 
 * surface. This call is non-blocking. The client can start another 
 * Begin/Render/End sequence on a different render target.
 */
VAStatus vaEndPicture (
    VADisplay dpy,
    VAContext *context
);

/*

Synchronization 

*/

/* 
 * This function blocks until all pending operations on the render target
 * have been completed.  Upon return it is safe to use the render target for a 
 * different picture. 
 */
VAStatus vaSyncSurface (
    VADisplay dpy,
    VAContext *context,
    VASurface *render_target
);

typedef enum
{
    VASurfaceRendering	= 0,
    VASurfaceReady	= 1,
} VASurfaceStatus;

/*
 * Find out any pending ops on the render target 
 */
VAStatus vaQuerySurfaceStatus (
    VADisplay dpy,
    VAContext *context,
    VASurface *render_target,
    VASurfaceStatus *status	/* out */
);

/*
 * Copies the surface to a buffer
 * The stride of the surface will be stored in *stride
 * Caller should free the returned buffer with free() when done. 
 */
VAStatus vaDbgCopySurfaceToBuffer(VADisplay dpy,
    VASurface *surface,
    void **buffer, /* out */
    unsigned int *stride /* out */
);

#ifdef __cplusplus
}
#endif

#endif /* _VA_H_ */

#if 0
/*****************************************************************************/ 

Sample Program (w/ pseudo code)

Mostly to demonstrate program flow with no error handling ...

/*****************************************************************************/

	/* MPEG-2 VLD decode for a 720x480 frame */

	int major_ver, minor_ver;
	vaInitialize(dpy, &major_ver, &minor_ver);

	int max_num_profiles, max_num_entrypoints, max_num_attribs;
	max_num_profiles = vaMaxNumProfiles(dpy);
	max_num_entrypoints = vaMaxNumProfiles(dpy);
	max_num_attribs = vaMaxNumProfiles(dpy);

	/* find out whether MPEG2 MP is supported */
	VAProfile *profiles = malloc(sizeof(VAProfile)*max_num_profiles);
	int num_profiles;
	vaQueryConfigProfiles(dpy, profiles, &profiles);
	/*
	 * traverse "profiles" to locate the one that matches VAProfileMPEG2Main
         */ 

	/* now get the available entrypoints for MPEG2 MP */
	VAEntrypoint *entrypoints = malloc(sizeof(VAEntrypoint)*max_num_entrypoints);
	int num_entrypoints;
	vaQueryConfigEntrypoints(dpy, VAProfileMPEG2Main, entrypoints, &num_entrypoints);

	/* traverse "entrypoints" to see whether VLD is there */

	/* Assuming finding VLD, find out the format for the render target */
	VAConfigAttrib attrib;
	attrib.type = VAConfigAttribRTFormat;
	vaQueryConfigAttributes(dpy, VAProfileMPEG2Main, VAEntrypointVLD,
                                &attrib, 1);

	if (attrib.value & VA_RT_FORMAT_YUV420)
		/* Found desired RT format, keep going */ 

	VAConfigID config_id;
	vaCreateConfig(dpy, VAProfileMPEG2Main, VAEntrypointVLD, &attrib, 1,
                       &config_id);

	/* 
         * create surfaces for the current target as well as reference frames
	 * we can get by with 4 surfaces for MPEG-2
	 */
	VASurface surfaces[4];
	vaCreateSurfaces(dpy, 720, 480, VA_RT_FORMAT_YUV420, 4, surfaces);

	/* 
         * Create a context for this decode pipe
	 */
	VAContext context;
	vaCreateContext(dpy, config_id, 720, 480, VA_PROGRESSIVE, surfaces,
                        4, &context);

	/* Create a picture parameter buffer for this frame */
	VABufferID picture_buf;
	VAPictureParameterBufferMPEG2 *picture_param;
	vaCreateBuffer(dpy, VAPictureParameterBufferType, &picture_buf);
	vaBufferData(dpy, picture_buf, sizeof(VAPictureParameterBufferMPEG2), NULL);
	vaMapBuffer(dpy, picture_buf, &picture_param);
	picture_param->horizontal_size = 720;
	picture_param->vertical_size = 480;
	picture_param->picture_coding_type = 1; /* I-frame */	
	/* fill in picture_coding_extension fields here */
	vaUnmapBuffer(dpy, picture_buf);


	/* Create an IQ matrix buffer for this frame */
	VABufferID iq_buf;
	VAIQMatrixBufferMPEG2 *iq_matrix;
	vaCreateBuffer(dpy, VAIQMatrixBufferType, &iq_buf);
	vaBufferData(dpy, iq_buf, sizeof(VAIQMatrixBufferMPEG2), NULL);
	vaMapBuffer(dpy, iq_buf, &iq_matrix);
	/* fill values for IQ_matrix here */
	vaUnmapBuffer(dpy, iq_buf);

	/* send the picture and IQ matrix buffers to the server */
	vaBeginPicture(dpy, context, &surfaces[0]);

	vaRenderPicture(dpy, context, &picture_buf, 1);
	vaRenderPicture(dpy, context, &iq_buf, 1);

	/* 
         * Send slices in this frame to the server.
         * For MPEG-2, each slice is one row of macroblocks, and
         * we have 30 slices for a 720x480 frame 
         */
	for (int i = 1; i <= 30; i++) {

		/* Create a slice parameter buffer */
		VABufferID slice_param_buf;
		VASliceParameterBufferMPEG2 *slice_param;
		vaCreateBuffer(dpy, VASliceParameterBufferType, &slice_param_buf);
		vaBufferData(dpy, slice_param_buf, sizeof(VASliceParameterBufferMPEG2), NULL);
		vaMapBuffer(dpy, slice_param_buf, &slice_param);
		slice_param->slice_data_offset = 0;
		/* Let's say all slices in this bit-stream has 64-bit header */
		slice_param->macroblock_offset = 64; 
		slice_param->vertical_position = i;
		/* set up the rest based on what is in the slice header ... */
		vaUnmapBuffer(dpy, slice_param_buf);

		/* send the slice parameter buffer */
		vaRenderPicture(dpy, context, &slice_param_buf, 1);

		/* Create a slice data buffer */
		unsigned char *slice_data;
		VABufferID slice_data_buf;
		vaCreateBuffer(dpy, VASliceDataBufferType, slice_data_buf);
		vaBufferData(dpy, slice_data_buf, x /* decoder figure out how big */, NULL);
		vaMapBuffer(dpy, slice_data_buf, &slice_data);
		/* decoder fill in slice_data */
		vaUnmapBuffer(dpy, slice_data_buf);

		/* send the slice data buffer */
		vaRenderPicture(dpy, context, &slice_data_buf, 1);
	}

	/* all slices have been sent, mark the end for this frame */
	vaEndPicture(dpy, context);
#endif
