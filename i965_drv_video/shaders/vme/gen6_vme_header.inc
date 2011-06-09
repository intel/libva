/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Modual name: ME_header.inc
//
// Global symbols define
//

/*
 * Constant
 */
define(`VME_MESSAGE_TYPE_INTER',        `1')
define(`VME_MESSAGE_TYPE_INTRA',        `2')
define(`VME_MESSAGE_TYPE_MIXED',        `3')
        
define(`BLOCK_32X1',                    `0x0000001F')
define(`BLOCK_4X16',                    `0x000F0003')
        
define(`LUMA_INTRA_16x16_DISABLE',      `0x1')
define(`LUMA_INTRA_8x8_DISABLE',        `0x2')
define(`LUMA_INTRA_4x4_DISABLE',        `0x4')

define(`INTRA_PRED_AVAIL_FLAG_AE',      `0x60')
define(`INTRA_PRED_AVAIL_FLAG_B',       `0x10')
define(`INTRA_PRED_AVAIL_FLAG_C',       `0x8')
define(`INTRA_PRED_AVAIL_FLAG_D',       `0x4')

define(`BIND_IDX_VME',                  `0')
define(`BIND_IDX_VME_REF0',             `1')
define(`BIND_IDX_VME_REF1',             `2')
define(`BIND_IDX_OUTPUT',               `3')
define(`BIND_IDX_INEP',                 `4')

define(`SUB_PEL_MODE_INTEGER',          `0x00000000')
define(`SUB_PEL_MODE_HALF',             `0x00001000')
define(`SUB_PEL_MODE_QUARTER',          `0x00003000')

define(`INTER_SAD_NONE',                `0x00000000')
define(`INTER_SAD_HAAR',                `0x00200000')

define(`INTRA_SAD_NONE',                `0x00000000')
define(`INTRA_SAD_HAAR',                `0x00800000')

define(`INTER_PART_MASK',               `0x7E000000')

define(`REF_REGION_SIZE',               `0x2020:UW')

define(`BI_SUB_MB_PART_MASK',           `0x0c000000')
define(`MAX_NUM_MV',                    `0x00000020')
define(`SEARCH_PATH_LEN',               `0x00003F3F')
 
define(`INTRA_PREDICTORE_MODE',         `0x11111111:UD')

define(`OBW_CACHE_TYPE',                `5')

define(`OBW_MESSAGE_TYPE',              `8')

define(`OBW_BIND_IDX',                  `BIND_IDX_OUTPUT')

define(`OBW_CONTROL_0',                 `0')    /* 1 OWord, low 128 bits */
define(`OBW_CONTROL_1',                 `1')    /* 1 OWord, high 128 bits */
define(`OBW_CONTROL_2',                 `2')    /* 2 OWords */
define(`OBW_CONTROL_3',                 `3')    /* 4 OWords */

define(`OBW_WRITE_COMMIT_CATEGORY',     `1')    /* write commit on Sandybrige */

define(`OBW_HEADER_PRESENT',            `1')

/* GRF registers
 * r0 header
 * r1~r4 constant buffer (reserved)
 * r5 inline data
 * r6~r11 reserved        
 * r12 write back of VME message
 * r13 write back of Oword Block Write        
 */
/*
 * GRF 0 -- header       
 */        
define(`thread_id_ub',          `r0.20<0,1,0>:UB')  /* thread id in payload */

/*
 * GRF 1~4 -- Constant Buffer (reserved)
 */
        
/*
 * GRF 5 -- inline data
 */        
define(`inline_reg0',           `r5')
define(`w_in_mb_uw',            `inline_reg0.2')
define(`orig_xy_ub',            `inline_reg0.0')
define(`orig_x_ub',             `inline_reg0.0')    /* in macroblock */    
define(`orig_y_ub',             `inline_reg0.1')

/*
 * GRF 6~11 -- reserved
 */

/*
 * GRF 12~15 -- write back for VME message 
 */
define(`vme_wb',                `r12')
define(`vme_wb0',               `r12')
define(`vme_wb1',               `r13')
define(`vme_wb2',               `r14')
define(`vme_wb3',               `r15')
        
/*
 * GRF 16 -- write back for Oword Block Write message with write commit bit
 */        
define(`obw_wb',                `r16')
define(`obw_wb_length',         `1')

/*
 * GRF 18~21 -- Intra Neighbor Edge Pixels
 */
define(`INEP_ROW',              `r18')
define(`INEP_COL0',             `r20')
define(`INEP_COL1',             `r21')
        
/*
 * temporary registers
 */
define(`tmp_reg0',              `r32')
define(`tmp_reg1',              `r33')
define(`intra_part_mask_ub',    `tmp_reg1.28')        
define(`mb_intra_struct_ub',    `tmp_reg1.29')
define(`tmp_reg2',              `r34')
define(`tmp_x_w',               `tmp_reg2.0')
define(`tmp_reg3',              `r35')

/*
 * MRF registers
 */        
define(`msg_ind',               `0')
define(`msg_reg0',              `m0')               /* m0 */
define(`msg_reg1',              `m1')               /* m1 */
define(`msg_reg2',              `m2')               /* m2 */
define(`msg_reg3',              `m3')               /* m3 */
define(`msg_reg4',              `m4')               /* m4 */

/*
 * VME message payload
 */
define(`vme_msg_length',        `4')
define(`vme_intra_wb_length',   `1')
define(`vme_inter_wb_length',   `4')
define(`vme_msg_ind',           `msg_ind')
define(`vme_msg_0',             `msg_reg0')
define(`vme_msg_1',             `msg_reg1')
define(`vme_msg_2',             `msg_reg2')
define(`vme_msg_3',             `vme_msg_2')
define(`vme_msg_4',             `msg_reg3')


