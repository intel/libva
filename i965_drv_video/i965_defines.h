#ifndef _I965_DEFINES_H_
#define _I965_DEFINES_H_

#define CMD(pipeline,op,sub_op)		((3 << 29) | \
                                           	((pipeline) << 27) | \
                                           	((op) << 24) | \
                                           	((sub_op) << 16))

#define CMD_URB_FENCE                           CMD(0, 0, 0)
#define CMD_CS_URB_STATE                        CMD(0, 0, 1)
#define CMD_CONSTANT_BUFFER                     CMD(0, 0, 2)
#define CMD_STATE_PREFETCH                      CMD(0, 0, 3)

#define CMD_STATE_BASE_ADDRESS                  CMD(0, 1, 1)
#define CMD_STATE_SIP                           CMD(0, 1, 2)
#define CMD_PIPELINE_SELECT                     CMD(1, 1, 4)
#define CMD_SAMPLER_PALETTE_LOAD                CMD(3, 1, 2)

#define CMD_MEDIA_STATE_POINTERS                CMD(2, 0, 0)
#define CMD_MEDIA_VFE_STATE                     CMD(2, 0, 0)
#define CMD_MEDIA_CURBE_LOAD                    CMD(2, 0, 1)
#define CMD_MEDIA_INTERFACE_LOAD                CMD(2, 0, 2)
#define CMD_MEDIA_OBJECT                        CMD(2, 1, 0)
#define CMD_MEDIA_OBJECT_EX                     CMD(2, 1, 1)

#define CMD_AVC_BSD_IMG_STATE                   CMD(2, 4, 0)
#define CMD_AVC_BSD_QM_STATE                    CMD(2, 4, 1)
#define CMD_AVC_BSD_SLICE_STATE                 CMD(2, 4, 2)
#define CMD_AVC_BSD_BUF_BASE_STATE              CMD(2, 4, 3)
#define CMD_BSD_IND_OBJ_BASE_ADDR               CMD(2, 4, 4)
#define CMD_AVC_BSD_OBJECT                      CMD(2, 4, 8)

#define CMD_MEDIA_VFE_STATE                     CMD(2, 0, 0)
#define CMD_MEDIA_CURBE_LOAD                    CMD(2, 0, 1)
#define CMD_MEDIA_INTERFACE_DESCRIPTOR_LOAD     CMD(2, 0, 2)
#define CMD_MEDIA_GATEWAY_STATE                 CMD(2, 0, 3)
#define CMD_MEDIA_STATE_FLUSH                   CMD(2, 0, 4)
#define CMD_MEDIA_OBJECT_WALKER                 CMD(2, 1, 3)

#define CMD_PIPELINED_POINTERS                  CMD(3, 0, 0)
#define CMD_BINDING_TABLE_POINTERS              CMD(3, 0, 1)
# define GEN6_BINDING_TABLE_MODIFY_PS           (1 << 12)/* for GEN6 */
# define GEN6_BINDING_TABLE_MODIFY_GS           (1 << 9) /* for GEN6 */
# define GEN6_BINDING_TABLE_MODIFY_VS           (1 << 8) /* for GEN6 */

#define CMD_VERTEX_BUFFERS                      CMD(3, 0, 8)
#define CMD_VERTEX_ELEMENTS                     CMD(3, 0, 9)
#define CMD_DRAWING_RECTANGLE                   CMD(3, 1, 0)
#define CMD_CONSTANT_COLOR                      CMD(3, 1, 1)
#define CMD_3DPRIMITIVE                         CMD(3, 3, 0)

#define CMD_DEPTH_BUFFER                        CMD(3, 1, 5)
# define CMD_DEPTH_BUFFER_TYPE_SHIFT            29
# define CMD_DEPTH_BUFFER_FORMAT_SHIFT          18

#define CMD_CLEAR_PARAMS                        CMD(3, 1, 0x10)
/* DW1 */
# define CMD_CLEAR_PARAMS_DEPTH_CLEAR_VALID     (1 << 15)

#define CMD_PIPE_CONTROL                        CMD(3, 2, 0)

/* for GEN6+ */
#define GEN6_3DSTATE_SAMPLER_STATE_POINTERS	CMD(3, 0, 0x02)
# define GEN6_3DSTATE_SAMPLER_STATE_MODIFY_PS	(1 << 12)
# define GEN6_3DSTATE_SAMPLER_STATE_MODIFY_GS	(1 << 9)
# define GEN6_3DSTATE_SAMPLER_STATE_MODIFY_VS	(1 << 8)

#define GEN6_3DSTATE_URB			CMD(3, 0, 0x05)
/* DW1 */
# define GEN6_3DSTATE_URB_VS_SIZE_SHIFT		16
# define GEN6_3DSTATE_URB_VS_ENTRIES_SHIFT	0
/* DW2 */
# define GEN6_3DSTATE_URB_GS_ENTRIES_SHIFT	8
# define GEN6_3DSTATE_URB_GS_SIZE_SHIFT		0

#define GEN6_3DSTATE_VIEWPORT_STATE_POINTERS	CMD(3, 0, 0x0d)
# define GEN6_3DSTATE_VIEWPORT_STATE_MODIFY_CC		(1 << 12)
# define GEN6_3DSTATE_VIEWPORT_STATE_MODIFY_SF		(1 << 11)
# define GEN6_3DSTATE_VIEWPORT_STATE_MODIFY_CLIP	(1 << 10)

#define GEN6_3DSTATE_CC_STATE_POINTERS		CMD(3, 0, 0x0e)

#define GEN6_3DSTATE_VS				CMD(3, 0, 0x10)

#define GEN6_3DSTATE_GS				CMD(3, 0, 0x11)
/* DW4 */
# define GEN6_3DSTATE_GS_DISPATCH_START_GRF_SHIFT	0

#define GEN6_3DSTATE_CLIP			CMD(3, 0, 0x12)

#define GEN6_3DSTATE_SF				CMD(3, 0, 0x13)
/* DW1 */
# define GEN6_3DSTATE_SF_NUM_OUTPUTS_SHIFT		22
# define GEN6_3DSTATE_SF_URB_ENTRY_READ_LENGTH_SHIFT	11
# define GEN6_3DSTATE_SF_URB_ENTRY_READ_OFFSET_SHIFT	4
/* DW2 */
/* DW3 */
# define GEN6_3DSTATE_SF_CULL_BOTH			(0 << 29)
# define GEN6_3DSTATE_SF_CULL_NONE			(1 << 29)
# define GEN6_3DSTATE_SF_CULL_FRONT			(2 << 29)
# define GEN6_3DSTATE_SF_CULL_BACK			(3 << 29)
/* DW4 */
# define GEN6_3DSTATE_SF_TRI_PROVOKE_SHIFT		29
# define GEN6_3DSTATE_SF_LINE_PROVOKE_SHIFT		27
# define GEN6_3DSTATE_SF_TRIFAN_PROVOKE_SHIFT		25


#define GEN6_3DSTATE_WM				CMD(3, 0, 0x14)
/* DW2 */
# define GEN6_3DSTATE_WM_SAMPLER_COUNT_SHITF			27
# define GEN6_3DSTATE_WM_BINDING_TABLE_ENTRY_COUNT_SHIFT	18
/* DW4 */
# define GEN6_3DSTATE_WM_DISPATCH_START_GRF_0_SHIFT		16
/* DW5 */
# define GEN6_3DSTATE_WM_MAX_THREADS_SHIFT			25
# define GEN6_3DSTATE_WM_DISPATCH_ENABLE			(1 << 19)
# define GEN6_3DSTATE_WM_16_DISPATCH_ENABLE			(1 << 1)
# define GEN6_3DSTATE_WM_8_DISPATCH_ENABLE			(1 << 0)
/* DW6 */
# define GEN6_3DSTATE_WM_NUM_SF_OUTPUTS_SHIFT			20
# define GEN6_3DSTATE_WM_NONPERSPECTIVE_SAMPLE_BARYCENTRIC	(1 << 15)
# define GEN6_3DSTATE_WM_NONPERSPECTIVE_CENTROID_BARYCENTRIC	(1 << 14)
# define GEN6_3DSTATE_WM_NONPERSPECTIVE_PIXEL_BARYCENTRIC	(1 << 13)
# define GEN6_3DSTATE_WM_PERSPECTIVE_SAMPLE_BARYCENTRIC		(1 << 12)
# define GEN6_3DSTATE_WM_PERSPECTIVE_CENTROID_BARYCENTRIC	(1 << 11)
# define GEN6_3DSTATE_WM_PERSPECTIVE_PIXEL_BARYCENTRIC		(1 << 10)


#define GEN6_3DSTATE_CONSTANT_VS		CMD(3, 0, 0x15)
#define GEN6_3DSTATE_CONSTANT_GS          	CMD(3, 0, 0x16)
#define GEN6_3DSTATE_CONSTANT_PS          	CMD(3, 0, 0x17)

# define GEN6_3DSTATE_CONSTANT_BUFFER_3_ENABLE  (1 << 15)
# define GEN6_3DSTATE_CONSTANT_BUFFER_2_ENABLE  (1 << 14)
# define GEN6_3DSTATE_CONSTANT_BUFFER_1_ENABLE  (1 << 13)
# define GEN6_3DSTATE_CONSTANT_BUFFER_0_ENABLE  (1 << 12)

#define GEN6_3DSTATE_SAMPLE_MASK		CMD(3, 0, 0x18)

#define GEN6_3DSTATE_MULTISAMPLE		CMD(3, 1, 0x0d)
/* DW1 */
# define GEN6_3DSTATE_MULTISAMPLE_PIXEL_LOCATION_CENTER         (0 << 4)
# define GEN6_3DSTATE_MULTISAMPLE_PIXEL_LOCATION_UPPER_LEFT     (1 << 4)
# define GEN6_3DSTATE_MULTISAMPLE_NUMSAMPLES_1                  (0 << 1)
# define GEN6_3DSTATE_MULTISAMPLE_NUMSAMPLES_4                  (2 << 1)
# define GEN6_3DSTATE_MULTISAMPLE_NUMSAMPLES_8                  (3 << 1)

#define MFX(pipeline, op, sub_opa, sub_opb)     \
    (3 << 29 |                                  \
     (pipeline) << 27 |                         \
     (op) << 24 |                               \
     (sub_opa) << 21 |                          \
     (sub_opb) << 16)

#define MFX_PIPE_MODE_SELECT                    MFX(2, 0, 0, 0)
#define MFX_SURFACE_STATE                       MFX(2, 0, 0, 1)
#define MFX_PIPE_BUF_ADDR_STATE                 MFX(2, 0, 0, 2)
#define MFX_IND_OBJ_BASE_ADDR_STATE             MFX(2, 0, 0, 3)
#define MFX_BSP_BUF_BASE_ADDR_STATE             MFX(2, 0, 0, 4)
#define MFX_AES_STATE                           MFX(2, 0, 0, 5)
#define MFX_STATE_POINTER                       MFX(2, 0, 0, 6)

#define MFX_WAIT                                MFX(1, 0, 0, 0)

#define MFX_AVC_IMG_STATE                       MFX(2, 1, 0, 0)
#define MFX_AVC_QM_STATE                        MFX(2, 1, 0, 1)
#define MFX_AVC_DIRECTMODE_STATE                MFX(2, 1, 0, 2)
#define MFX_AVC_SLICE_STATE                     MFX(2, 1, 0, 3)
#define MFX_AVC_REF_IDX_STATE                   MFX(2, 1, 0, 4)
#define MFX_AVC_WEIGHTOFFSET_STATE              MFX(2, 1, 0, 5)

#define MFD_AVC_BSD_OBJECT                      MFX(2, 1, 1, 8)

#define MFC_AVC_FQM_STATE                       MFX(2, 1, 2, 2)
#define MFC_AVC_INSERT_OBJECT                   MFX(2, 1, 2, 8)
#define MFC_AVC_PAK_OBJECT                      MFX(2, 1, 2, 9)

#define MFX_MPEG2_PIC_STATE                     MFX(2, 3, 0, 0)
#define MFX_MPEG2_QM_STATE                      MFX(2, 3, 0, 1)

#define MFD_MPEG2_BSD_OBJECT                    MFX(2, 3, 1, 8)

#define MFX_VC1_PIC_STATE                       MFX(2, 2, 0, 0)
#define MFX_VC1_PRED_PIPE_STATE                 MFX(2, 2, 0, 1)
#define MFX_VC1_DIRECTMODE_STATE                MFX(2, 2, 0, 2)

#define MFD_VC1_BSD_OBJECT                      MFX(2, 2, 1, 8)

#define I965_DEPTHFORMAT_D32_FLOAT              1

#define BASE_ADDRESS_MODIFY             (1 << 0)

#define PIPELINE_SELECT_3D              0
#define PIPELINE_SELECT_MEDIA           1


#define UF0_CS_REALLOC                  (1 << 13)
#define UF0_VFE_REALLOC                 (1 << 12)
#define UF0_SF_REALLOC                  (1 << 11)
#define UF0_CLIP_REALLOC                (1 << 10)
#define UF0_GS_REALLOC                  (1 << 9)
#define UF0_VS_REALLOC                  (1 << 8)
#define UF1_CLIP_FENCE_SHIFT            20
#define UF1_GS_FENCE_SHIFT              10
#define UF1_VS_FENCE_SHIFT              0
#define UF2_CS_FENCE_SHIFT              20
#define UF2_VFE_FENCE_SHIFT             10
#define UF2_SF_FENCE_SHIFT              0

#define VFE_GENERIC_MODE        0x0
#define VFE_VLD_MODE            0x1
#define VFE_IS_MODE             0x2
#define VFE_AVC_MC_MODE         0x4
#define VFE_AVC_IT_MODE         0x7

#define FLOATING_POINT_IEEE_754        0
#define FLOATING_POINT_NON_IEEE_754    1


#define I965_SURFACE_1D      0
#define I965_SURFACE_2D      1
#define I965_SURFACE_3D      2
#define I965_SURFACE_CUBE    3
#define I965_SURFACE_BUFFER  4
#define I965_SURFACE_NULL    7

#define I965_SURFACEFORMAT_R32G32B32A32_FLOAT             0x000 
#define I965_SURFACEFORMAT_R32G32B32A32_SINT              0x001 
#define I965_SURFACEFORMAT_R32G32B32A32_UINT              0x002 
#define I965_SURFACEFORMAT_R32G32B32A32_UNORM             0x003 
#define I965_SURFACEFORMAT_R32G32B32A32_SNORM             0x004 
#define I965_SURFACEFORMAT_R64G64_FLOAT                   0x005 
#define I965_SURFACEFORMAT_R32G32B32X32_FLOAT             0x006 
#define I965_SURFACEFORMAT_R32G32B32A32_SSCALED           0x007
#define I965_SURFACEFORMAT_R32G32B32A32_USCALED           0x008
#define I965_SURFACEFORMAT_R32G32B32_FLOAT                0x040 
#define I965_SURFACEFORMAT_R32G32B32_SINT                 0x041 
#define I965_SURFACEFORMAT_R32G32B32_UINT                 0x042 
#define I965_SURFACEFORMAT_R32G32B32_UNORM                0x043 
#define I965_SURFACEFORMAT_R32G32B32_SNORM                0x044 
#define I965_SURFACEFORMAT_R32G32B32_SSCALED              0x045 
#define I965_SURFACEFORMAT_R32G32B32_USCALED              0x046 
#define I965_SURFACEFORMAT_R16G16B16A16_UNORM             0x080 
#define I965_SURFACEFORMAT_R16G16B16A16_SNORM             0x081 
#define I965_SURFACEFORMAT_R16G16B16A16_SINT              0x082 
#define I965_SURFACEFORMAT_R16G16B16A16_UINT              0x083 
#define I965_SURFACEFORMAT_R16G16B16A16_FLOAT             0x084 
#define I965_SURFACEFORMAT_R32G32_FLOAT                   0x085 
#define I965_SURFACEFORMAT_R32G32_SINT                    0x086 
#define I965_SURFACEFORMAT_R32G32_UINT                    0x087 
#define I965_SURFACEFORMAT_R32_FLOAT_X8X24_TYPELESS       0x088 
#define I965_SURFACEFORMAT_X32_TYPELESS_G8X24_UINT        0x089 
#define I965_SURFACEFORMAT_L32A32_FLOAT                   0x08A 
#define I965_SURFACEFORMAT_R32G32_UNORM                   0x08B 
#define I965_SURFACEFORMAT_R32G32_SNORM                   0x08C 
#define I965_SURFACEFORMAT_R64_FLOAT                      0x08D 
#define I965_SURFACEFORMAT_R16G16B16X16_UNORM             0x08E 
#define I965_SURFACEFORMAT_R16G16B16X16_FLOAT             0x08F 
#define I965_SURFACEFORMAT_A32X32_FLOAT                   0x090 
#define I965_SURFACEFORMAT_L32X32_FLOAT                   0x091 
#define I965_SURFACEFORMAT_I32X32_FLOAT                   0x092 
#define I965_SURFACEFORMAT_R16G16B16A16_SSCALED           0x093
#define I965_SURFACEFORMAT_R16G16B16A16_USCALED           0x094
#define I965_SURFACEFORMAT_R32G32_SSCALED                 0x095
#define I965_SURFACEFORMAT_R32G32_USCALED                 0x096
#define I965_SURFACEFORMAT_B8G8R8A8_UNORM                 0x0C0 
#define I965_SURFACEFORMAT_B8G8R8A8_UNORM_SRGB            0x0C1 
#define I965_SURFACEFORMAT_R10G10B10A2_UNORM              0x0C2 
#define I965_SURFACEFORMAT_R10G10B10A2_UNORM_SRGB         0x0C3 
#define I965_SURFACEFORMAT_R10G10B10A2_UINT               0x0C4 
#define I965_SURFACEFORMAT_R10G10B10_SNORM_A2_UNORM       0x0C5 
#define I965_SURFACEFORMAT_R8G8B8A8_UNORM                 0x0C7 
#define I965_SURFACEFORMAT_R8G8B8A8_UNORM_SRGB            0x0C8 
#define I965_SURFACEFORMAT_R8G8B8A8_SNORM                 0x0C9 
#define I965_SURFACEFORMAT_R8G8B8A8_SINT                  0x0CA 
#define I965_SURFACEFORMAT_R8G8B8A8_UINT                  0x0CB 
#define I965_SURFACEFORMAT_R16G16_UNORM                   0x0CC 
#define I965_SURFACEFORMAT_R16G16_SNORM                   0x0CD 
#define I965_SURFACEFORMAT_R16G16_SINT                    0x0CE 
#define I965_SURFACEFORMAT_R16G16_UINT                    0x0CF 
#define I965_SURFACEFORMAT_R16G16_FLOAT                   0x0D0 
#define I965_SURFACEFORMAT_B10G10R10A2_UNORM              0x0D1 
#define I965_SURFACEFORMAT_B10G10R10A2_UNORM_SRGB         0x0D2 
#define I965_SURFACEFORMAT_R11G11B10_FLOAT                0x0D3 
#define I965_SURFACEFORMAT_R32_SINT                       0x0D6 
#define I965_SURFACEFORMAT_R32_UINT                       0x0D7 
#define I965_SURFACEFORMAT_R32_FLOAT                      0x0D8 
#define I965_SURFACEFORMAT_R24_UNORM_X8_TYPELESS          0x0D9 
#define I965_SURFACEFORMAT_X24_TYPELESS_G8_UINT           0x0DA 
#define I965_SURFACEFORMAT_L16A16_UNORM                   0x0DF 
#define I965_SURFACEFORMAT_I24X8_UNORM                    0x0E0 
#define I965_SURFACEFORMAT_L24X8_UNORM                    0x0E1 
#define I965_SURFACEFORMAT_A24X8_UNORM                    0x0E2 
#define I965_SURFACEFORMAT_I32_FLOAT                      0x0E3 
#define I965_SURFACEFORMAT_L32_FLOAT                      0x0E4 
#define I965_SURFACEFORMAT_A32_FLOAT                      0x0E5 
#define I965_SURFACEFORMAT_B8G8R8X8_UNORM                 0x0E9 
#define I965_SURFACEFORMAT_B8G8R8X8_UNORM_SRGB            0x0EA 
#define I965_SURFACEFORMAT_R8G8B8X8_UNORM                 0x0EB 
#define I965_SURFACEFORMAT_R8G8B8X8_UNORM_SRGB            0x0EC 
#define I965_SURFACEFORMAT_R9G9B9E5_SHAREDEXP             0x0ED 
#define I965_SURFACEFORMAT_B10G10R10X2_UNORM              0x0EE 
#define I965_SURFACEFORMAT_L16A16_FLOAT                   0x0F0 
#define I965_SURFACEFORMAT_R32_UNORM                      0x0F1 
#define I965_SURFACEFORMAT_R32_SNORM                      0x0F2 
#define I965_SURFACEFORMAT_R10G10B10X2_USCALED            0x0F3
#define I965_SURFACEFORMAT_R8G8B8A8_SSCALED               0x0F4
#define I965_SURFACEFORMAT_R8G8B8A8_USCALED               0x0F5
#define I965_SURFACEFORMAT_R16G16_SSCALED                 0x0F6
#define I965_SURFACEFORMAT_R16G16_USCALED                 0x0F7
#define I965_SURFACEFORMAT_R32_SSCALED                    0x0F8
#define I965_SURFACEFORMAT_R32_USCALED                    0x0F9
#define I965_SURFACEFORMAT_B5G6R5_UNORM                   0x100 
#define I965_SURFACEFORMAT_B5G6R5_UNORM_SRGB              0x101 
#define I965_SURFACEFORMAT_B5G5R5A1_UNORM                 0x102 
#define I965_SURFACEFORMAT_B5G5R5A1_UNORM_SRGB            0x103 
#define I965_SURFACEFORMAT_B4G4R4A4_UNORM                 0x104 
#define I965_SURFACEFORMAT_B4G4R4A4_UNORM_SRGB            0x105 
#define I965_SURFACEFORMAT_R8G8_UNORM                     0x106 
#define I965_SURFACEFORMAT_R8G8_SNORM                     0x107 
#define I965_SURFACEFORMAT_R8G8_SINT                      0x108 
#define I965_SURFACEFORMAT_R8G8_UINT                      0x109 
#define I965_SURFACEFORMAT_R16_UNORM                      0x10A 
#define I965_SURFACEFORMAT_R16_SNORM                      0x10B 
#define I965_SURFACEFORMAT_R16_SINT                       0x10C 
#define I965_SURFACEFORMAT_R16_UINT                       0x10D 
#define I965_SURFACEFORMAT_R16_FLOAT                      0x10E 
#define I965_SURFACEFORMAT_I16_UNORM                      0x111 
#define I965_SURFACEFORMAT_L16_UNORM                      0x112 
#define I965_SURFACEFORMAT_A16_UNORM                      0x113 
#define I965_SURFACEFORMAT_L8A8_UNORM                     0x114 
#define I965_SURFACEFORMAT_I16_FLOAT                      0x115
#define I965_SURFACEFORMAT_L16_FLOAT                      0x116
#define I965_SURFACEFORMAT_A16_FLOAT                      0x117 
#define I965_SURFACEFORMAT_R5G5_SNORM_B6_UNORM            0x119 
#define I965_SURFACEFORMAT_B5G5R5X1_UNORM                 0x11A 
#define I965_SURFACEFORMAT_B5G5R5X1_UNORM_SRGB            0x11B
#define I965_SURFACEFORMAT_R8G8_SSCALED                   0x11C
#define I965_SURFACEFORMAT_R8G8_USCALED                   0x11D
#define I965_SURFACEFORMAT_R16_SSCALED                    0x11E
#define I965_SURFACEFORMAT_R16_USCALED                    0x11F
#define I965_SURFACEFORMAT_R8_UNORM                       0x140 
#define I965_SURFACEFORMAT_R8_SNORM                       0x141 
#define I965_SURFACEFORMAT_R8_SINT                        0x142 
#define I965_SURFACEFORMAT_R8_UINT                        0x143 
#define I965_SURFACEFORMAT_A8_UNORM                       0x144 
#define I965_SURFACEFORMAT_I8_UNORM                       0x145 
#define I965_SURFACEFORMAT_L8_UNORM                       0x146 
#define I965_SURFACEFORMAT_P4A4_UNORM                     0x147 
#define I965_SURFACEFORMAT_A4P4_UNORM                     0x148
#define I965_SURFACEFORMAT_R8_SSCALED                     0x149
#define I965_SURFACEFORMAT_R8_USCALED                     0x14A
#define I965_SURFACEFORMAT_R1_UINT                        0x181 
#define I965_SURFACEFORMAT_YCRCB_NORMAL                   0x182 
#define I965_SURFACEFORMAT_YCRCB_SWAPUVY                  0x183 
#define I965_SURFACEFORMAT_BC1_UNORM                      0x186 
#define I965_SURFACEFORMAT_BC2_UNORM                      0x187 
#define I965_SURFACEFORMAT_BC3_UNORM                      0x188 
#define I965_SURFACEFORMAT_BC4_UNORM                      0x189 
#define I965_SURFACEFORMAT_BC5_UNORM                      0x18A 
#define I965_SURFACEFORMAT_BC1_UNORM_SRGB                 0x18B 
#define I965_SURFACEFORMAT_BC2_UNORM_SRGB                 0x18C 
#define I965_SURFACEFORMAT_BC3_UNORM_SRGB                 0x18D 
#define I965_SURFACEFORMAT_MONO8                          0x18E 
#define I965_SURFACEFORMAT_YCRCB_SWAPUV                   0x18F 
#define I965_SURFACEFORMAT_YCRCB_SWAPY                    0x190 
#define I965_SURFACEFORMAT_DXT1_RGB                       0x191 
#define I965_SURFACEFORMAT_FXT1                           0x192 
#define I965_SURFACEFORMAT_R8G8B8_UNORM                   0x193 
#define I965_SURFACEFORMAT_R8G8B8_SNORM                   0x194 
#define I965_SURFACEFORMAT_R8G8B8_SSCALED                 0x195 
#define I965_SURFACEFORMAT_R8G8B8_USCALED                 0x196 
#define I965_SURFACEFORMAT_R64G64B64A64_FLOAT             0x197 
#define I965_SURFACEFORMAT_R64G64B64_FLOAT                0x198 
#define I965_SURFACEFORMAT_BC4_SNORM                      0x199 
#define I965_SURFACEFORMAT_BC5_SNORM                      0x19A 
#define I965_SURFACEFORMAT_R16G16B16_UNORM                0x19C 
#define I965_SURFACEFORMAT_R16G16B16_SNORM                0x19D 
#define I965_SURFACEFORMAT_R16G16B16_SSCALED              0x19E 
#define I965_SURFACEFORMAT_R16G16B16_USCALED              0x19F

#define I965_CULLMODE_BOTH      0
#define I965_CULLMODE_NONE      1
#define I965_CULLMODE_FRONT     2
#define I965_CULLMODE_BACK      3

#define I965_MAPFILTER_NEAREST        0x0 
#define I965_MAPFILTER_LINEAR         0x1 
#define I965_MAPFILTER_ANISOTROPIC    0x2

#define I965_MIPFILTER_NONE        0   
#define I965_MIPFILTER_NEAREST     1   
#define I965_MIPFILTER_LINEAR      3

#define I965_TEXCOORDMODE_WRAP            0
#define I965_TEXCOORDMODE_MIRROR          1
#define I965_TEXCOORDMODE_CLAMP           2
#define I965_TEXCOORDMODE_CUBE            3
#define I965_TEXCOORDMODE_CLAMP_BORDER    4
#define I965_TEXCOORDMODE_MIRROR_ONCE     5

#define I965_BLENDFACTOR_ONE                 0x1
#define I965_BLENDFACTOR_SRC_COLOR           0x2
#define I965_BLENDFACTOR_SRC_ALPHA           0x3
#define I965_BLENDFACTOR_DST_ALPHA           0x4
#define I965_BLENDFACTOR_DST_COLOR           0x5
#define I965_BLENDFACTOR_SRC_ALPHA_SATURATE  0x6
#define I965_BLENDFACTOR_CONST_COLOR         0x7
#define I965_BLENDFACTOR_CONST_ALPHA         0x8
#define I965_BLENDFACTOR_SRC1_COLOR          0x9
#define I965_BLENDFACTOR_SRC1_ALPHA          0x0A
#define I965_BLENDFACTOR_ZERO                0x11
#define I965_BLENDFACTOR_INV_SRC_COLOR       0x12
#define I965_BLENDFACTOR_INV_SRC_ALPHA       0x13
#define I965_BLENDFACTOR_INV_DST_ALPHA       0x14
#define I965_BLENDFACTOR_INV_DST_COLOR       0x15
#define I965_BLENDFACTOR_INV_CONST_COLOR     0x17
#define I965_BLENDFACTOR_INV_CONST_ALPHA     0x18
#define I965_BLENDFACTOR_INV_SRC1_COLOR      0x19
#define I965_BLENDFACTOR_INV_SRC1_ALPHA      0x1A

#define I965_BLENDFUNCTION_ADD               0
#define I965_BLENDFUNCTION_SUBTRACT          1
#define I965_BLENDFUNCTION_REVERSE_SUBTRACT  2
#define I965_BLENDFUNCTION_MIN               3
#define I965_BLENDFUNCTION_MAX               4

#define I965_SURFACERETURNFORMAT_FLOAT32  0
#define I965_SURFACERETURNFORMAT_S1       1

#define I965_VFCOMPONENT_NOSTORE      0
#define I965_VFCOMPONENT_STORE_SRC    1
#define I965_VFCOMPONENT_STORE_0      2
#define I965_VFCOMPONENT_STORE_1_FLT  3
#define I965_VFCOMPONENT_STORE_1_INT  4
#define I965_VFCOMPONENT_STORE_VID    5
#define I965_VFCOMPONENT_STORE_IID    6
#define I965_VFCOMPONENT_STORE_PID    7

#define VE0_VERTEX_BUFFER_INDEX_SHIFT	27
#define GEN6_VE0_VERTEX_BUFFER_INDEX_SHIFT      26 /* for GEN6 */
#define VE0_VALID			(1 << 26)
#define GEN6_VE0_VALID                  (1 << 25) /* for GEN6 */
#define VE0_FORMAT_SHIFT		16
#define VE0_OFFSET_SHIFT		0
#define VE1_VFCOMPONENT_0_SHIFT		28
#define VE1_VFCOMPONENT_1_SHIFT		24
#define VE1_VFCOMPONENT_2_SHIFT		20
#define VE1_VFCOMPONENT_3_SHIFT		16
#define VE1_DESTINATION_ELEMENT_OFFSET_SHIFT	0

#define VB0_BUFFER_INDEX_SHIFT          27
#define GEN6_VB0_BUFFER_INDEX_SHIFT     26
#define VB0_VERTEXDATA                  (0 << 26)
#define VB0_INSTANCEDATA                (1 << 26)
#define GEN6_VB0_VERTEXDATA             (0 << 20)
#define GEN6_VB0_INSTANCEDATA           (1 << 20)
#define VB0_BUFFER_PITCH_SHIFT          0

#define _3DPRIMITIVE_VERTEX_SEQUENTIAL  (0 << 15)
#define _3DPRIMITIVE_VERTEX_RANDOM      (1 << 15)
#define _3DPRIMITIVE_TOPOLOGY_SHIFT     10

#define _3DPRIM_POINTLIST         0x01
#define _3DPRIM_LINELIST          0x02
#define _3DPRIM_LINESTRIP         0x03
#define _3DPRIM_TRILIST           0x04
#define _3DPRIM_TRISTRIP          0x05
#define _3DPRIM_TRIFAN            0x06
#define _3DPRIM_QUADLIST          0x07
#define _3DPRIM_QUADSTRIP         0x08
#define _3DPRIM_LINELIST_ADJ      0x09
#define _3DPRIM_LINESTRIP_ADJ     0x0A
#define _3DPRIM_TRILIST_ADJ       0x0B
#define _3DPRIM_TRISTRIP_ADJ      0x0C
#define _3DPRIM_TRISTRIP_REVERSE  0x0D
#define _3DPRIM_POLYGON           0x0E
#define _3DPRIM_RECTLIST          0x0F
#define _3DPRIM_LINELOOP          0x10
#define _3DPRIM_POINTLIST_BF      0x11
#define _3DPRIM_LINESTRIP_CONT    0x12
#define _3DPRIM_LINESTRIP_BF      0x13
#define _3DPRIM_LINESTRIP_CONT_BF 0x14
#define _3DPRIM_TRIFAN_NOSTIPPLE  0x15

#define I965_TILEWALK_XMAJOR                 0
#define I965_TILEWALK_YMAJOR                 1

#define SCAN_RASTER_ORDER       0
#define SCAN_SPECIAL_ORDER      1

#define ENTROPY_CAVLD           0
#define ENTROPY_CABAC           1

#define SLICE_TYPE_P            0
#define SLICE_TYPE_B            1
#define SLICE_TYPE_I            2
#define SLICE_TYPE_SP           3
#define SLICE_TYPE_SI           4

#define PRESENT_REF_LIST0               (1 << 0)
#define PRESENT_REF_LIST1               (1 << 1)
#define PRESENT_WEIGHT_OFFSET_L0        (1 << 2)
#define PRESENT_WEIGHT_OFFSET_L1        (1 << 3)

#define RESIDUAL_DATA_OFFSET    48

#define PRESENT_NOMV            0
#define PRESENT_NOWO            1
#define PRESENT_MV_WO           3

#define SCOREBOARD_STALLING     0
#define SCOREBOARD_NON_STALLING 1

#define SURFACE_FORMAT_YCRCB_NORMAL     0
#define SURFACE_FORMAT_YCRCB_SWAPUVY    1
#define SURFACE_FORMAT_YCRCB_SWAPUV     2
#define SURFACE_FORMAT_YCRCB_SWAPY      3
#define SURFACE_FORMAT_PLANAR_420_8     4
#define SURFACE_FORMAT_PLANAR_411_8     5
#define SURFACE_FORMAT_PLANAR_422_8     6
#define SURFACE_FORMAT_STMM_DN_STATISTICS       7
#define SURFACE_FORMAT_R10G10B10A2_UNORM        8
#define SURFACE_FORMAT_R8G8B8A8_UNORM   9
#define SURFACE_FORMAT_R8B8_UNORM       10
#define SURFACE_FORMAT_R8_UNORM         11
#define SURFACE_FORMAT_Y8_UNORM         12

#define AVS_FILTER_ADAPTIVE_8_TAP       0
#define AVS_FILTER_NEAREST              1

#define IEF_FILTER_COMBO                0
#define IEF_FILTER_DETAIL               1

#define IEF_FILTER_SIZE_3X3             0
#define IEF_FILTER_SIZE_5X5             1

#define MFX_FORMAT_MPEG2        0
#define MFX_FORMAT_VC1          1
#define MFX_FORMAT_AVC          2

#define MFX_CODEC_DECODE        0
#define MFX_CODEC_ENCODE        1

#define MFD_MODE_VLD            0
#define MFD_MODE_IT             1

#define MFX_SURFACE_PLANAR_420_8        4
#define MFX_SURFACE_MONOCHROME          12

#define URB_SIZE(intel)         (IS_GEN6(intel->device_id) ? 1024 :     \
                                 IS_IRONLAKE(intel->device_id) ? 1024 : \
                                 IS_G4X(intel->device_id) ? 384 : 256)

#endif /* _I965_DEFINES_H_ */
