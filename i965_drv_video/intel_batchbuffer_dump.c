#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>

#include "intel_driver.h"
#include "intel_batchbuffer_dump.h"

#define BUFFER_FAIL(_count, _len, _name) do {			\
    fprintf(gout, "Buffer size too small in %s (%d < %d)\n",	\
	    (_name), (_count), (_len));				\
    (*failures)++;						\
    return count;						\
} while (0)

static FILE *gout;

static void
instr_out(unsigned int *data, unsigned int offset, unsigned int index, char *fmt, ...)
{
    va_list va;

    fprintf(gout, "0x%08x: 0x%08x:%s ", offset + index * 4, data[index],
	    index == 0 ? "" : "  ");
    va_start(va, fmt);
    vfprintf(gout, fmt, va);
    va_end(va);
}


static int
dump_mi(unsigned int *data, unsigned int offset, int count, unsigned int device, int *failures)
{
    unsigned int opcode;
    int length, i;

    struct {
	unsigned int opcode;
	int mask_length;
	int min_len;
	int max_len;
	char *name;
    } mi_commands[] = {
	{ 0x00, 0, 1, 1, "MI_NOOP" },
	{ 0x04, 0, 1, 1, "MI_FLUSH" },
	{ 0x0a, 0, 1, 1, "MI_BATCH_BUFFER_END" },
	{ 0x26, 0x3f, 4, 5, "MI_FLUSH_DW" },
    };

    opcode = ((data[0] & MASK_MI_OPCODE) >> SHIFT_MI_OPCODE);

    for (i = 0; i < sizeof(mi_commands) / sizeof(mi_commands[0]); i++) {
        if (opcode == mi_commands[i].opcode) {
            int index;

            length = 1;
	    instr_out(data, offset, 0, "%s\n", mi_commands[i].name);

	    if (mi_commands[i].max_len > 1) {
                length = (data[0] & mi_commands[i].mask_length) + 2;

                if (length < mi_commands[i].min_len ||
                    length > mi_commands[i].max_len) {
		    fprintf(gout, "Bad length (%d) in %s, [%d, %d]\n",
			    length, mi_commands[i].name,
			    mi_commands[i].min_len,
			    mi_commands[i].max_len);
		}
	    }

            for (index = 1; index < length; index++) {
                if (index >= count)
		    BUFFER_FAIL(count, length, mi_commands[i].name);

		instr_out(data, offset, index, "dword %d\n", index);
	    }

	    return length;
	}
    }

    instr_out(data, offset, 0, "UNKNOWN MI COMMAND\n");
    (*failures)++;
    return 1;
}

static int
dump_gfxpipe_3d(unsigned int *data, unsigned int offset, int count, unsigned int device, int *failures)
{
    instr_out(data, offset, 0, "UNKNOWN 3D COMMAND\n");
    (*failures)++;

    return 1;
}

static void
dump_avc_bsd_img_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    int img_struct = ((data[3] >> 8) & 0x3);

    instr_out(data, offset, 1, "frame size: %d\n", (data[1] & 0xffff));
    instr_out(data, offset, 2, "width: %d, height: %d\n", (data[2] & 0xff), (data[2] >> 16) & 0xff);
    instr_out(data, offset, 3, 
              "second_chroma_qp_offset: %d,"
              "chroma_qp_offset: %d,"
              "QM present flag: %d," 
              "image struct: %s,"
              "img_dec_fs_idc: %d,"
              "\n",
              (data[3] >> 24) & 0x1f,
              (data[3] >> 16) & 0x1f,
              (data[3] >> 10) & 0x1,
              (img_struct == 0) ? "frame" : (img_struct == 2) ? "invalid" : (img_struct == 1) ? "top field" : "bottom field",
              data[3] & 0xff);
    instr_out(data, offset, 4,
              "residual off: 0x%x,"
              "16MV: %d,"
              "chroma fmt: %d,"
              "CABAC: %d,"
              "non-ref: %d,"
              "constrained intra: %d,"
              "direct8x8: %d,"
              "trans8x8: %d,"
              "MB only: %d,"
              "MBAFF: %d,"
              "\n",
              (data[4] >> 24) & 0xff,
              (data[4] >> 12) & 0x1,
              (data[4] >> 10) & 0x3,
              (data[4] >> 7) & 0x1,
              (data[4] >> 6) & 0x1,
              (data[4] >> 5) & 0x1,
              (data[4] >> 4) & 0x1,
              (data[4] >> 3) & 0x1,
              (data[4] >> 2) & 0x1,
              (data[4] >> 1) & 0x1);
    instr_out(data, offset, 5, "AVC-IT Command Header\n");
}

static void
dump_avc_bsd_qm_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    unsigned int length = ((data[0] & MASK_GFXPIPE_LENGTH) >> SHIFT_GFXPIPE_LENGTH) + 2;
    int i;

    instr_out(data, offset, 1, "user default: %02x, QM list present: %02x\n", 
              (data[1] >> 8) & 0xff, data[1] & 0xff);

    for (i = 2; i < length; i++) {
        instr_out(data, offset, i, "dword %d\n", i);
    }
}

static void
dump_avc_bsd_slice_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{

}

static void
dump_avc_bsd_buf_base_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    int i;

    instr_out(data, offset, 1, "BSD row store base address\n");
    instr_out(data, offset, 2, "MPR row store base address\n");
    instr_out(data, offset, 3, "AVC-IT command buffer base address\n");
    instr_out(data, offset, 4, "AVC-IT data buffer: 0x%08x, write offset: 0x%x\n", 
              data[4] & 0xFFFFF000, data[4] & 0xFC0);
    instr_out(data, offset, 5, "ILDB data buffer\n");

    for (i = 6; i < 38; i++) {
        instr_out(data, offset, i, "Direct MV read base address for reference frame %d\n", i - 6);
    }

    instr_out(data, offset, 38, "direct mv wr0 top\n");
    instr_out(data, offset, 39, "direct mv wr0 bottom\n");

    for (i = 40; i < 74; i++) {
        instr_out(data, offset, i, "POC List %d\n", i - 40);
    }
}

static void
dump_bsd_ind_obj_base_addr(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    instr_out(data, offset, 1, "AVC indirect object base address\n");
    instr_out(data, offset, 2, "AVC Indirect Object Access Upper Bound\n");
}

static void 
dump_ironlake_avc_bsd_object(unsigned int *data, unsigned int offset, int *failures)
{
    int slice_type = data[3] & 0xf;
    int i, is_phantom = ((data[1] & 0x3fffff) == 0);

    if (!is_phantom) {
        instr_out(data, offset, 1, "Encrypted: %d, bitsteam length: %d\n", data[1] >> 31, data[1] & 0x3fffff);
        instr_out(data, offset, 2, "Indirect Data Start Address: %d\n", data[2] & 0x1fffffff);
        instr_out(data, offset, 3, "%s Slice\n", slice_type == 0 ? "P" : slice_type == 1 ? "B" : "I");
        instr_out(data, offset, 4, 
                  "Num_Ref_Idx_L1: %d,"
                  "Num_Ref_Idx_L0: %d,"
                  "Log2WeightDenomChroma: %d,"
                  "Log2WeightDenomLuma: %d"
                  "\n",
                  (data[4] >> 24) & 0x3f,
                  (data[4] >> 16) & 0x3f,
                  (data[4] >> 8) & 0x3,
                  (data[4] >> 0) & 0x3);
        instr_out(data, offset, 5,
                  "WeightedPredIdc: %d,"
                  "DirectPredType: %d,"
                  "DisableDeblockingFilter: %d,"
                  "CabacInitIdc: %d,"
                  "SliceQp: %d,"
                  "SliceBetaOffsetDiv2: %d,"
                  "SliceAlphaC0OffsetDiv2: %d"
                  "\n",
                  (data[5] >> 30) & 0x3,
                  (data[5] >> 29) & 0x1,
                  (data[5] >> 27) & 0x3,
                  (data[5] >> 24) & 0x3,
                  (data[5] >> 16) & 0x3f,
                  (data[5] >> 8) & 0xf,
                  (data[5] >> 0) & 0xf);
        instr_out(data, offset, 6,
                  "Slice_MB_Start_Vert_Pos: %d,"
                  "Slice_MB_Start_Hor_Pos: %d,"
                  "Slice_Start_Mb_Num: %d"
                  "\n",
                  (data[6] >> 24) & 0xff,
                  (data[6] >> 16) & 0xff,
                  (data[6] >> 0) & 0x7fff);
        instr_out(data, offset, 7,
                  "Fix_Prev_Mb_Skipped: %d,"
                  "First_MB_Bit_Offset: %d"
                  "\n",
                  (data[7] >> 7) & 0x1,
                  (data[7] >> 0) & 0x7);

        for (i = 8; i < 16; i++)
            instr_out(data, offset, i, "dword %d\n", i);
    } else {
        instr_out(data, offset, 1, "phantom slice\n");

        for (i = 2; i < 6; i++)
            instr_out(data, offset, i, "dword %d\n", i);

        instr_out(data, offset, 6,
                  "Slice_Start_Mb_Num: %d"
                  "\n",
                  (data[6] >> 0) & 0x7fff);

        for (i = 7; i < 16; i++)
            instr_out(data, offset, i, "dword %d\n", i);

    }
}

static void 
dump_g4x_avc_bsd_object(unsigned int *data, unsigned int offset, int *failures)
{

}

static void 
dump_avc_bsd_object(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    if (IS_IRONLAKE(device))
        dump_ironlake_avc_bsd_object(data, offset, failures);
    else
        dump_g4x_avc_bsd_object(data, offset, failures);
}

static int
dump_bsd_avc(unsigned int *data, unsigned int offset, int count, unsigned int device, int *failures)
{
    unsigned int subopcode;
    int length, i;

    struct {
	unsigned int subopcode;
	int min_len;
	int max_len;
	char *name;
        void (*detail)(unsigned int *data, unsigned int offset, unsigned int device, int  *failures);
    } avc_commands[] = {
        { 0x00, 0x06, 0x06, "AVC_BSD_IMG_STATE", dump_avc_bsd_img_state },
        { 0x01, 0x02, 0x3a, "AVC_BSD_QM_STATE", dump_avc_bsd_qm_state },
        { 0x02, 0x02, 0xd2, "AVC_BSD_SLICE_STATE", NULL },
        { 0x03, 0x4a, 0x4a, "AVC_BSD_BUF_BASE_STATE", dump_avc_bsd_buf_base_state },
        { 0x04, 0x03, 0x03, "BSD_IND_OBJ_BASE_ADDR", dump_bsd_ind_obj_base_addr },
        { 0x08, 0x08, 0x10, "AVC_BSD_OBJECT", dump_avc_bsd_object },
    };

    subopcode = ((data[0] & MASK_GFXPIPE_SUBOPCODE) >> SHIFT_GFXPIPE_SUBOPCODE);

    for (i = 0; i < sizeof(avc_commands) / sizeof(avc_commands[0]); i++) {
        if (subopcode == avc_commands[i].subopcode) {
            unsigned int index;

            length = (data[0] & MASK_GFXPIPE_LENGTH) >> SHIFT_GFXPIPE_LENGTH;
            length += 2;
            instr_out(data, offset, 0, "%s\n", avc_commands[i].name);

            if (length < avc_commands[i].min_len || 
                length > avc_commands[i].max_len) {
                fprintf(gout, "Bad length(%d) in %s [%d, %d]\n", 
                        length, avc_commands[i].name,
                        avc_commands[i].min_len,
                        avc_commands[i].max_len);
            }

            if (length - 1 >= count)
                BUFFER_FAIL(count, length, avc_commands[i].name);

            if (avc_commands[i].detail)
                avc_commands[i].detail(data, offset, device, failures);
            else {
                for (index = 1; index < length; index++)
                    instr_out(data, offset, index, "dword %d\n", index);
            }

	    return length;
	}
    }

    instr_out(data, offset, 0, "UNKNOWN AVC COMMAND\n");
    (*failures)++;
    return 1;
}

static int
dump_gfxpipe_bsd(unsigned int *data, unsigned int offset, int count, unsigned int device, int *failures)
{
    int length;

    switch ((data[0] & MASK_GFXPIPE_OPCODE) >> SHIFT_GFXPIPE_OPCODE) {
    case OPCODE_BSD_AVC:
        length = dump_bsd_avc(data, offset, count, device, failures);
        break;

    default:
        length = 1;
        (*failures)++;
        instr_out(data, offset, 0, "UNKNOWN BSD OPCODE\n");
        break;
    }

    return length;
}

static void
dump_mfx_mode_select(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    instr_out(data, offset, 1, 
              "decoder mode: %d(%s),"
              "post deblocking output enable %d,"
              "pre deblocking output enable %d,"
              "codec select: %d(%s),"
              "standard select: %d(%s)"
              "\n",
              (data[1] >> 16) & 0x1, ((data[1] >> 16) & 0x1) ? "IT" : "VLD",
              (data[1] >> 9) & 0x1,
              (data[1] >> 8) & 0x1,
              (data[1] >> 4) & 0x1, ((data[1] >> 4) & 0x1) ? "Encode" : "Decode",
              (data[1] >> 0) & 0x3, ((data[1] >> 0) & 0x3) == 0 ? "MPEG2" :
              ((data[1] >> 0) & 0x3) == 1 ? "VC1" :
              ((data[1] >> 0) & 0x3) == 2 ? "AVC" : "Reserved");
    instr_out(data, offset, 2, "dword 02\n");
    instr_out(data, offset, 3, "dword 03\n");
}

static void
dump_mfx_surface_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    instr_out(data, offset, 1, "dword 01\n");
    instr_out(data, offset, 2, "dword 02\n");
    instr_out(data, offset, 3, "dword 03\n");
    instr_out(data, offset, 4, "dword 04\n");
    instr_out(data, offset, 5, "dword 05\n");
}

static void
dump_mfx_pipe_buf_addr_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    instr_out(data, offset, 1, "dword 01\n");
    instr_out(data, offset, 2, "dword 02\n");
    instr_out(data, offset, 3, "dword 03\n");
    instr_out(data, offset, 4, "dword 04\n");
    instr_out(data, offset, 5, "dword 05\n");
    instr_out(data, offset, 6, "dword 06\n");
    instr_out(data, offset, 7, "dword 07\n");
    instr_out(data, offset, 8, "dword 08\n");
    instr_out(data, offset, 9, "dword 09\n");
    instr_out(data, offset, 10, "dword 10\n");
    instr_out(data, offset, 11, "dword 11\n");
    instr_out(data, offset, 12, "dword 12\n");
    instr_out(data, offset, 13, "dword 13\n");
    instr_out(data, offset, 14, "dword 14\n");
    instr_out(data, offset, 15, "dword 15\n");
    instr_out(data, offset, 16, "dword 16\n");
    instr_out(data, offset, 17, "dword 17\n");
    instr_out(data, offset, 18, "dword 18\n");
    instr_out(data, offset, 19, "dword 19\n");
    instr_out(data, offset, 20, "dword 20\n");
    instr_out(data, offset, 21, "dword 21\n");
    instr_out(data, offset, 22, "dword 22\n");
    instr_out(data, offset, 24, "dword 23\n");
}

static void
dump_mfx_ind_obj_base_addr_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    instr_out(data, offset, 1, "dword 01\n");
    instr_out(data, offset, 2, "dword 02\n");
    instr_out(data, offset, 3, "dword 03\n");
    instr_out(data, offset, 4, "dword 04\n");
    instr_out(data, offset, 5, "dword 05\n");
    instr_out(data, offset, 6, "dword 06\n");
    instr_out(data, offset, 7, "dword 07\n");
    instr_out(data, offset, 8, "dword 08\n");
    instr_out(data, offset, 9, "dword 09\n");
    instr_out(data, offset, 10, "dword 10\n");
}

static void
dump_mfx_bsp_buf_base_addr_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    instr_out(data, offset, 1, "dword 01\n");
    instr_out(data, offset, 2, "dword 02\n");
    instr_out(data, offset, 3, "dword 03\n");
}

static void
dump_mfx_aes_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    instr_out(data, offset, 1, "dword 01\n");
    instr_out(data, offset, 2, "dword 02\n");
    instr_out(data, offset, 3, "dword 03\n");
    instr_out(data, offset, 4, "dword 04\n");
    instr_out(data, offset, 5, "dword 05\n");
    instr_out(data, offset, 6, "dword 06\n");
}

static void
dump_mfx_state_pointer(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    instr_out(data, offset, 1, "dword 01\n");
}

static int
dump_mfx_common(unsigned int *data, unsigned int offset, int count, unsigned int device, int *failures)
{
    unsigned int subopcode;
    int length, i;

    struct {
	unsigned int subopcode;
	int min_len;
	int max_len;
	char *name;
        void (*detail)(unsigned int *data, unsigned int offset, unsigned int device, int  *failures);
    } mfx_common_commands[] = {
        { SUBOPCODE_MFX(0, 0), 0x04, 0x04, "MFX_PIPE_MODE_SELECT", dump_mfx_mode_select },
        { SUBOPCODE_MFX(0, 1), 0x06, 0x06, "MFX_SURFACE_STATE", dump_mfx_surface_state },
        { SUBOPCODE_MFX(0, 2), 0x18, 0x18, "MFX_PIPE_BUF_ADDR_STATE", dump_mfx_pipe_buf_addr_state },
        { SUBOPCODE_MFX(0, 3), 0x0b, 0x0b, "MFX_IND_OBJ_BASE_ADDR_STATE", dump_mfx_ind_obj_base_addr_state },
        { SUBOPCODE_MFX(0, 4), 0x04, 0x04, "MFX_BSP_BUF_BASE_ADDR_STATE", dump_mfx_bsp_buf_base_addr_state },
        { SUBOPCODE_MFX(0, 5), 0x07, 0x07, "MFX_AES_STATE", dump_mfx_aes_state },
        { SUBOPCODE_MFX(0, 6), 0x00, 0x00, "MFX_STATE_POINTER", dump_mfx_state_pointer },
    };

    subopcode = ((data[0] & MASK_GFXPIPE_SUBOPCODE) >> SHIFT_GFXPIPE_SUBOPCODE);

    for (i = 0; i < ARRAY_ELEMS(mfx_common_commands); i++) {
        if (subopcode == mfx_common_commands[i].subopcode) {
            unsigned int index;

            length = (data[0] & MASK_GFXPIPE_LENGTH) >> SHIFT_GFXPIPE_LENGTH;
            length += 2;
            instr_out(data, offset, 0, "%s\n", mfx_common_commands[i].name);

            if (length < mfx_common_commands[i].min_len || 
                length > mfx_common_commands[i].max_len) {
                fprintf(gout, "Bad length(%d) in %s [%d, %d]\n", 
                        length, mfx_common_commands[i].name,
                        mfx_common_commands[i].min_len,
                        mfx_common_commands[i].max_len);
            }

            if (length - 1 >= count)
                BUFFER_FAIL(count, length, mfx_common_commands[i].name);

            if (mfx_common_commands[i].detail)
                mfx_common_commands[i].detail(data, offset, device, failures);
            else {
                for (index = 1; index < length; index++)
                    instr_out(data, offset, index, "dword %d\n", index);
            }

	    return length;
	}
    }

    instr_out(data, offset, 0, "UNKNOWN MFX COMMON COMMAND\n");
    (*failures)++;
    return 1;
}

static void
dump_mfx_avc_img_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    instr_out(data, offset, 1, "dword 01\n");
    instr_out(data, offset, 2, "dword 02\n");
    instr_out(data, offset, 3, "dword 03\n");
    instr_out(data, offset, 4, "dword 04\n");
    instr_out(data, offset, 5, "dword 05\n");
    instr_out(data, offset, 6, "dword 06\n");
    instr_out(data, offset, 7, "dword 07\n");
    instr_out(data, offset, 8, "dword 08\n");
    instr_out(data, offset, 9, "dword 09\n");
    instr_out(data, offset, 10, "dword 10\n");
    instr_out(data, offset, 11, "dword 11\n");
    instr_out(data, offset, 12, "dword 12\n");
}

static void
dump_mfx_avc_qm_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    unsigned int length = ((data[0] & MASK_GFXPIPE_LENGTH) >> SHIFT_GFXPIPE_LENGTH) + 2;
    int i;

    instr_out(data, offset, 1, "user default: %02x, QM list present: %02x\n", 
              (data[1] >> 8) & 0xff, data[1] & 0xff);

    for (i = 2; i < length; i++) {
        instr_out(data, offset, i, "dword %d\n", i);
    }
}

static void
dump_mfx_avc_directmode_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    int i;

    for (i = 1; i < 33; i++) {
        instr_out(data, offset, i, "Direct MV Buffer Base Address for Picture %d\n", i - 1);
    }

    for (i = 33; i < 35; i++) {
        instr_out(data, offset, i, "Direct MV Buffer Base Address for Current Decoding Frame/Field\n");
    }

    for (i = 35; i < 69; i++) {
        instr_out(data, offset, i, "POC List\n");
    }
}

static void
dump_mfx_avc_slice_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    instr_out(data, offset, 1, "dword 01\n");
    instr_out(data, offset, 2, "dword 02\n");
    instr_out(data, offset, 3, "dword 03\n");
    instr_out(data, offset, 4, "dword 04\n");
    instr_out(data, offset, 5, "dword 05\n");
    instr_out(data, offset, 6, "dword 06\n");
    instr_out(data, offset, 7, "dword 07\n");
    instr_out(data, offset, 8, "dword 08\n");
    instr_out(data, offset, 9, "dword 09\n");
}

static void
dump_mfx_avc_ref_idx_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    instr_out(data, offset, 1, "dword 01\n");
    instr_out(data, offset, 2, "dword 02\n");
    instr_out(data, offset, 3, "dword 03\n");
    instr_out(data, offset, 4, "dword 04\n");
    instr_out(data, offset, 5, "dword 05\n");
    instr_out(data, offset, 6, "dword 06\n");
    instr_out(data, offset, 7, "dword 07\n");
    instr_out(data, offset, 8, "dword 08\n");
    instr_out(data, offset, 9, "dword 09\n");
}

static void
dump_mfx_avc_weightoffset_state(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    int i;

    instr_out(data, offset, 1, 
              "Weight and Offset L%d table\n",
              (data[1] >> 0) & 0x1);

    for (i = 2; i < 31; i++) {
        instr_out(data, offset, i, "dword %d\n", i);
    }
}

static void
dump_mfd_bsd_object(unsigned int *data, unsigned int offset, unsigned int device, int *failures)
{
    int is_phantom_slice = ((data[1] & 0x3fffff) == 0);

    if (is_phantom_slice) {
        instr_out(data, offset, 1, "phantom slice\n");
        instr_out(data, offset, 2, "dword 02\n");
        instr_out(data, offset, 3, "dword 03\n");
        instr_out(data, offset, 4, "dword 04\n");
        instr_out(data, offset, 5, "dword 05\n");
    } else {
        instr_out(data, offset, 1, "Indirect BSD Data Length: %d\n", data[1] & 0x3fffff);
        instr_out(data, offset, 2, "Indirect BSD Data Start Address: 0x%08x\n", data[2] & 0x1fffffff);
        instr_out(data, offset, 3, "dword 03\n");
        instr_out(data, offset, 4,
                  "First_MB_Byte_Offset of Slice Data from Slice Header: 0x%08x,"
                  "slice header skip mode: %d"
                  "\n",
                  (data[4] >> 16),
                  (data[4] >> 6) & 0x1);
        instr_out(data, offset, 5, "dword 05\n");
    }
}

static int
dump_mfx_avc(unsigned int *data, unsigned int offset, int count, unsigned int device, int *failures)
{
    unsigned int subopcode;
    int length, i;

    struct {
	unsigned int subopcode;
	int min_len;
	int max_len;
	char *name;
        void (*detail)(unsigned int *data, unsigned int offset, unsigned int device, int  *failures);
    } mfx_avc_commands[] = {
        { SUBOPCODE_MFX(0, 0), 0x0d, 0x0d, "MFX_AVC_IMG_STATE", dump_mfx_avc_img_state },
        { SUBOPCODE_MFX(0, 1), 0x02, 0x3a, "MFX_AVC_QM_STATE", dump_mfx_avc_qm_state },
        { SUBOPCODE_MFX(0, 2), 0x45, 0x45, "MFX_AVC_DIRECTMODE_STATE", dump_mfx_avc_directmode_state },
        { SUBOPCODE_MFX(0, 3), 0x0b, 0x0b, "MFX_AVC_SLICE_STATE", dump_mfx_avc_slice_state },
        { SUBOPCODE_MFX(0, 4), 0x0a, 0x0a, "MFX_AVC_REF_IDX_STATE", dump_mfx_avc_ref_idx_state },
        { SUBOPCODE_MFX(0, 5), 0x32, 0x32, "MFX_AVC_WEIGHTOFFSET_STATE", dump_mfx_avc_weightoffset_state },
        { SUBOPCODE_MFX(1, 8), 0x06, 0x06, "MFD_AVC_BSD_OBJECT", dump_mfd_bsd_object },
    };

    subopcode = ((data[0] & MASK_GFXPIPE_SUBOPCODE) >> SHIFT_GFXPIPE_SUBOPCODE);

    for (i = 0; i < ARRAY_ELEMS(mfx_avc_commands); i++) {
        if (subopcode == mfx_avc_commands[i].subopcode) {
            unsigned int index;

            length = (data[0] & MASK_GFXPIPE_LENGTH) >> SHIFT_GFXPIPE_LENGTH;
            length += 2;
            instr_out(data, offset, 0, "%s\n", mfx_avc_commands[i].name);

            if (length < mfx_avc_commands[i].min_len || 
                length > mfx_avc_commands[i].max_len) {
                fprintf(gout, "Bad length(%d) in %s [%d, %d]\n", 
                        length, mfx_avc_commands[i].name,
                        mfx_avc_commands[i].min_len,
                        mfx_avc_commands[i].max_len);
            }

            if (length - 1 >= count)
                BUFFER_FAIL(count, length, mfx_avc_commands[i].name);

            if (mfx_avc_commands[i].detail)
                mfx_avc_commands[i].detail(data, offset, device, failures);
            else {
                for (index = 1; index < length; index++)
                    instr_out(data, offset, index, "dword %d\n", index);
            }

	    return length;
	}
    }

    instr_out(data, offset, 0, "UNKNOWN MFX AVC COMMAND\n");
    (*failures)++;
    return 1;
}

static int
dump_gfxpipe_mfx(unsigned int *data, unsigned int offset, int count, unsigned int device, int *failures)
{
    int length;

    switch ((data[0] & MASK_GFXPIPE_OPCODE) >> SHIFT_GFXPIPE_OPCODE) {
    case OPCODE_MFX_COMMON:
        length = dump_mfx_common(data, offset, count, device, failures);
        break;

    case OPCODE_MFX_AVC:
        length = dump_mfx_avc(data, offset, count, device, failures);
        break;

    default:
        length = 1;
        (*failures)++;
        instr_out(data, offset, 0, "UNKNOWN MFX OPCODE\n");
        break;
    }

    return length;
}

static int
dump_gfxpipe(unsigned int *data, unsigned int offset, int count, unsigned int device, int *failures)
{
    int length;

    switch ((data[0] & MASK_GFXPIPE_SUBTYPE) >> SHIFT_GFXPIPE_SUBTYPE) {
    case GFXPIPE_3D:
        length = dump_gfxpipe_3d(data, offset, count, device, failures);
        break;

    case GFXPIPE_BSD:
        if (IS_GEN6(device))
            length = dump_gfxpipe_mfx(data, offset, count, device, failures);
        else
            length = dump_gfxpipe_bsd(data, offset, count, device, failures);

        break;

    default:
        length = 1;
        (*failures)++;
        instr_out(data, offset, 0, "UNKNOWN GFXPIPE COMMAND\n");
        break;
    }

    return length;
}

int intel_batchbuffer_dump(unsigned int *data, unsigned int offset, int count, unsigned int device)
{
    int index = 0;
    int failures = 0;

    gout = fopen("/tmp/bsd_command_dump.txt", "w+");

    while (index < count) {
	switch ((data[index] & MASK_CMD_TYPE) >> SHIFT_CMD_TYPE) {
	case CMD_TYPE_MI:
	    index += dump_mi(data + index, offset + index * 4,
                             count - index, device, &failures);
	    break;

	case CMD_TYPE_GFXPIPE:
            index += dump_gfxpipe(data + index, offset + index * 4,
                                  count - index, device, &failures);
	    break;

	default:
	    instr_out(data, offset, index, "UNKNOWN COMMAND\n");
	    failures++;
	    index++;
	    break;
	}

	fflush(gout);
    }

    fclose(gout);

    return failures;
}
