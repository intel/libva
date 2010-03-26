#ifndef _INTEL_BATCHBUFFER_DUMP_H_
#define _INTEL_BATCHBUFFER_DUMP_H_

#define MASK_CMD_TYPE           0xE0000000

#define SHIFT_CMD_TYPE          29

#define CMD_TYPE_GFXPIPE        3
#define CMD_TYPE_BLT            2
#define CMD_TYPE_MI             0


/* GFXPIPE */
#define MASK_GFXPIPE_SUBTYPE    0x18000000
#define MASK_GFXPIPE_OPCODE     0x07000000
#define MASK_GFXPIPE_SUBOPCODE  0x00FF0000
#define MASK_GFXPIPE_LENGTH     0x0000FFFF

#define SHIFT_GFXPIPE_SUBTYPE           27
#define SHIFT_GFXPIPE_OPCODE            24
#define SHIFT_GFXPIPE_SUBOPCODE         16
#define SHIFT_GFXPIPE_LENGTH            0

/* 3D */
#define GFXPIPE_3D              3

/* BSD */
#define GFXPIPE_BSD             2

#define OPCODE_BSD_AVC          4

#define SUBOPCODE_BSD_IMG       0
#define SUBOPCODE_BSD_QM        1
#define SUBOPCODE_BSD_SLICE     2
#define SUBOPCODE_BSD_BUF_BASE  3
#define SUBOPCODE_BSD_IND_OBJ   4
#define SUBOPCODE_BSD_OBJECT    8

/* MI */
#define MASK_MI_OPCODE          0x1F800000

#define SHIFT_MI_OPCODE         23

#define OPCODE_MI_FLUSH                 0x04
#define OPCODE_MI_BATCH_BUFFER_END      0x0A

int intel_batchbuffer_dump(unsigned int *data, unsigned int offset, int count, unsigned int device);

#endif /* _INTEL_BATCHBUFFER_DUMP_H_ */
