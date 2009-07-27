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
#define CMD_SAMPLER_PALETTE_LOAD                CMD(3, 3, 2)

#define CMD_MEDIA_STATE_POINTERS                CMD(2, 0, 0)
#define CMD_MEDIA_OBJECT                        CMD(2, 1, 0)
#define CMD_MEDIA_OBJECT_EX                     CMD(2, 1, 1)

#define CMD_PIPELINED_POINTERS                  CMD(3, 0, 0)
#define CMD_BINDING_TABLE_POINTERS              CMD(3, 0, 1)
#define CMD_VERTEX_BUFFERS                      CMD(3, 0, 8)
#define CMD_VERTEX_ELEMENTS                     CMD(3, 0, 9)
#define CMD_DRAWING_RECTANGLE                   CMD(3, 1, 0)
#define CMD_CONSTANT_COLOR                      CMD(3, 1, 1)
#define CMD_3DPRIMITIVE                         CMD(3, 3, 0)

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
#define VE0_VALID			(1 << 26)
#define VE0_FORMAT_SHIFT		16
#define VE0_OFFSET_SHIFT		0
#define VE1_VFCOMPONENT_0_SHIFT		28
#define VE1_VFCOMPONENT_1_SHIFT		24
#define VE1_VFCOMPONENT_2_SHIFT		20
#define VE1_VFCOMPONENT_3_SHIFT		16
#define VE1_DESTINATION_ELEMENT_OFFSET_SHIFT	0

#define VB0_BUFFER_INDEX_SHIFT          27
#define VB0_VERTEXDATA                  (0 << 26)
#define VB0_INSTANCEDATA                (1 << 26)
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

#define URB_SIZE(intel)         (IS_IGDNG(intel->device_id) ? 1024 : \
                                 IS_G4X(intel->device_id) ? 384 : 256)
#endif /* _I965_DEFINES_H_ */
