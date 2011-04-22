/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Modual name: IntraFrame.asm
//
// Make intra predition estimation for Intra frame
//

//
//  Now, begin source code....
//

include(`vme_header.inc')

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

define(`REF_REGION_SIZE',               `0x2020:UW')

define(`BI_SUB_MB_PART_MASK',           `0x0c000000')
define(`MAX_NUM_MV',                    `0x00000020')
        
define(`INTRA_PREDICTORE_MODE',         `0x11111111:UD')
        
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
define(`msg_reg0',              `m0')               /* m0 */
define(`msg_reg1',              `m1')               /* m1 */
define(`msg_reg2',              `m2')               /* m2 */
define(`msg_reg3',              `m3')               /* m3 */

/*
 * __START
 */
__INTER_START:
mov  (16) tmp_reg0.0<1>:UD      0x0:UD {align1};
mov  (16) tmp_reg2.0<1>:UD      0x0:UD {align1};

/*
 * Media Read Message -- fetch neighbor edge pixels 
 */
/* ROW */
// mul  (2) tmp_reg0.0<1>:D        orig_xy_ub<2,2,1>:UB 16:UW {align1};    /* (x, y) * 16 */
// add  (1) tmp_reg0.0<1>:D        tmp_reg0.0<0,1,0>:D -8:W {align1};     /* X offset */
// add  (1) tmp_reg0.4<1>:D        tmp_reg0.4<0,1,0>:D -1:W {align1};     /* Y offset */ 
// mov  (1) tmp_reg0.8<1>:UD       BLOCK_32X1 {align1};
// mov  (1) tmp_reg0.20<1>:UB      thread_id_ub {align1};                  /* dispatch id */
// mov  (8) msg_reg0.0<1>:UD       tmp_reg0.0<8,8,1>:UD {align1};        
// send (16) 0 INEP_ROW null read(BIND_IDX_INEP, 0, 0, 4) mlen 1 rlen 1 {align1};

/* COL */
// mul  (2) tmp_reg0.0<1>:D        orig_xy_ub<2,2,1>:UB 16:UW {align1};    /* (x, y) * 16 */
// add  (1) tmp_reg0.0<1>:D        tmp_reg0.0<0,1,0>:D -4:W {align1};     /* X offset */
// mov  (1) tmp_reg0.8<1>:UD       BLOCK_4X16 {align1};
// mov  (1) tmp_reg0.20<1>:UB      thread_id_ub {align1};                  /* dispatch id */
// mov  (8) msg_reg0.0<1>:UD       tmp_reg0.0<8,8,1>:UD {align1};                
// send (16) 0 INEP_COL0 null read(BIND_IDX_INEP, 0, 0, 4) mlen 1 rlen 2 {align1};

/*
 * VME message
 */
/* m0 */        
mul  (2) tmp_reg0.0<1>:UW       orig_xy_ub<2,2,1>:UB 16:UW {align1};    /* (x, y) * 16 */
mov  (1) tmp_reg0.8<1>:UD       tmp_reg0.0<0,1,0>:UD {align1};
mov  (1) tmp_reg0.12<1>:UD      INTER_SAD_HAAR + INTRA_SAD_HAAR + SUB_PEL_MODE_QUARTER:UD {align1};    /* 16x16 Source, 1/4 pixel, harr */
mov  (1) tmp_reg0.20<1>:UB      thread_id_ub {align1};                  /* dispatch id */
mov  (1) tmp_reg0.22<1>:UW      REF_REGION_SIZE {align1};               /* Reference Width&Height, 32x32 */
mov  (8) msg_reg0.0<1>:UD       tmp_reg0.0<8,8,1>:UD {align1};
        
/* m1 */
mov  (1) tmp_reg1.4<1>:UD       BI_SUB_MB_PART_MASK + MAX_NUM_MV:UD {align1};                                   /* Default value MAX 32 MVs */

mov  (1) intra_part_mask_ub<1>:UB LUMA_INTRA_8x8_DISABLE + LUMA_INTRA_4x4_DISABLE {align1};

// cmp.nz.f0.0 (1) null<1>:UW orig_x_ub<0,1,0>:UB 0:UW {align1};                                                   /* X != 0 */
// (f0.0) add (1) mb_intra_struct_ub<1>:UB mb_intra_struct_ub<0,1,0>:UB INTRA_PRED_AVAIL_FLAG_AE {align1};         /* A */

// cmp.nz.f0.0 (1) null<1>:UW orig_y_ub<0,1,0>:UB 0:UW {align1};                                                   /* Y != 0 */
// (f0.0) add (1) mb_intra_struct_ub<1>:UB mb_intra_struct_ub<0,1,0>:UB INTRA_PRED_AVAIL_FLAG_B {align1};          /* B */

// mul.nz.f0.0 (1) null<1>:UW orig_x_ub<0,1,0>:UB orig_y_ub<0,1,0>:UB {align1};                                    /* X * Y != 0 */
// (f0.0) add (1) mb_intra_struct_ub<1>:UB mb_intra_struct_ub<0,1,0>:UB INTRA_PRED_AVAIL_FLAG_D {align1};          /* D */

// add  (1) tmp_x_w<1>:W orig_x_ub<0,1,0>:UB 1:UW {align1};                                                        /* X + 1 */
// add  (1) tmp_x_w<1>:W w_in_mb_uw<0,1,0>:UW -tmp_x_w<0,1,0>:W {align1};                                          /* width - (X + 1) */
// mul.nz.f0.0 (1) null<1>:UD tmp_x_w<0,1,0>:W orig_y_ub<0,1,0>:UB {align1};                                       /* (width - (X + 1)) * Y != 0 */
// (f0.0) add (1) mb_intra_struct_ub<1>:UB mb_intra_struct_ub<0,1,0>:UB INTRA_PRED_AVAIL_FLAG_C {align1};          /* C */

mov  (8) msg_reg1<1>:UD         tmp_reg1.0<8,8,1>:UD {align1};
        
/* m2 */        
mov  (8) msg_reg2<1>:UD         INEP_ROW.0<8,8,1>:UD {align1};

/* m3 */        
mov  (8) msg_reg3<1>:UD         0x0 {align1};
mov (16) msg_reg3.0<1>:UB       INEP_COL0.3<32,8,4>:UB {align1};
mov  (1) msg_reg3.16<1>:UD      INTRA_PREDICTORE_MODE {align1};

send (8) 0 vme_wb null vme(BIND_IDX_VME,0,0,VME_MESSAGE_TYPE_INTER) mlen 4 rlen 4 {align1};

/*
 * Oword Block Write message
 */
mul  (1) tmp_reg3.8<1>:UD       w_in_mb_uw<0,1,0>:UW orig_y_ub<0,1,0>:UB {align1};
add  (1) tmp_reg3.8<1>:UD       tmp_reg3.8<0,1,0>:UD orig_x_ub<0,1,0>:UB {align1};
mul  (1) tmp_reg3.8<1>:UD       tmp_reg3.8<0,1,0>:UD 0x4:UD {align1};
mov  (1) tmp_reg3.20<1>:UB      thread_id_ub {align1};                  /* dispatch id */
mov  (8) msg_reg0.0<1>:UD       tmp_reg3.0<8,8,1>:UD {align1};

mov  (2) tmp_reg3.0<1>:UW       vme_wb1.0<2,2,1>:UB  {align1};
        
mov  (8) msg_reg1.0<1>:UD       tmp_reg3.0<0,1,0>:UD   {align1};

mov  (8) msg_reg2.0<1>:UD       tmp_reg3.0<0,1,0>:UD   {align1};

/* bind index 3, write 4 oword, msg type: 8(OWord Block Write) */
send (16) 0 obw_wb null write(BIND_IDX_OUTPUT, 3, 8, 1) mlen 3 rlen 1 {align1};
        
/*
 * kill thread
 */        
mov  (8) msg_reg0<1>:UD         r0<8,8,1>:UD {align1};
send (16) 0 acc0<1>UW null thread_spawner(0, 0, 1) mlen 1 rlen 0 {align1 EOT};
