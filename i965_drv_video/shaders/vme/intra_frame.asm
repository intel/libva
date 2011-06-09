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

/*
 * __START
 */
__INTRA_START:
mov  (16) tmp_reg0.0<1>:UD      0x0:UD {align1};
mov  (16) tmp_reg2.0<1>:UD      0x0:UD {align1};
       
/*
 * Media Read Message -- fetch neighbor edge pixels 
 */
/* ROW */
mul  (2) tmp_reg0.0<1>:D        orig_xy_ub<2,2,1>:UB 16:UW {align1};    /* (x, y) * 16 */
add  (1) tmp_reg0.0<1>:D        tmp_reg0.0<0,1,0>:D -8:W {align1};     /* X offset */
add  (1) tmp_reg0.4<1>:D        tmp_reg0.4<0,1,0>:D -1:W {align1};     /* Y offset */ 
mov  (1) tmp_reg0.8<1>:UD       BLOCK_32X1 {align1};
mov  (1) tmp_reg0.20<1>:UB      thread_id_ub {align1};                  /* dispatch id */
mov  (8) msg_reg0.0<1>:UD       tmp_reg0.0<8,8,1>:UD {align1};        
send (8) msg_ind INEP_ROW<1>:UB null read(BIND_IDX_INEP, 0, 0, 4) mlen 1 rlen 1 {align1};

/* COL */
mul  (2) tmp_reg0.0<1>:D        orig_xy_ub<2,2,1>:UB 16:UW {align1};    /* (x, y) * 16 */
add  (1) tmp_reg0.0<1>:D        tmp_reg0.0<0,1,0>:D -4:W {align1};     /* X offset */
mov  (1) tmp_reg0.8<1>:UD       BLOCK_4X16 {align1};
mov  (1) tmp_reg0.20<1>:UB      thread_id_ub {align1};                  /* dispatch id */
mov  (8) msg_reg0.0<1>:UD       tmp_reg0.0<8,8,1>:UD {align1};                
send (8) msg_ind INEP_COL0<1>:UB null read(BIND_IDX_INEP, 0, 0, 4) mlen 1 rlen 2 {align1};
        
/*
 * VME message
 */
/* m0 */        
mul  (2) tmp_reg0.8<1>:UW       orig_xy_ub<2,2,1>:UB 16:UW {align1};    /* (x, y) * 16 */
mov  (1) tmp_reg0.20<1>:UB      thread_id_ub {align1};                  /* dispatch id */
mov  (8) vme_msg_0.0<1>:UD      tmp_reg0.0<8,8,1>:UD {align1};

/* m1 */
mov  (1) intra_part_mask_ub<1>:UB LUMA_INTRA_8x8_DISABLE + LUMA_INTRA_4x4_DISABLE {align1};

cmp.nz.f0.0 (1) null<1>:UW orig_x_ub<0,1,0>:UB 0:UW {align1};                                                   /* X != 0 */
(f0.0) add (1) mb_intra_struct_ub<1>:UB mb_intra_struct_ub<0,1,0>:UB INTRA_PRED_AVAIL_FLAG_AE {align1};         /* A */

cmp.nz.f0.0 (1) null<1>:UW orig_y_ub<0,1,0>:UB 0:UW {align1};                                                   /* Y != 0 */
(f0.0) add (1) mb_intra_struct_ub<1>:UB mb_intra_struct_ub<0,1,0>:UB INTRA_PRED_AVAIL_FLAG_B {align1};          /* B */

mul.nz.f0.0 (1) null<1>:UW orig_x_ub<0,1,0>:UB orig_y_ub<0,1,0>:UB {align1};                                    /* X * Y != 0 */
(f0.0) add (1) mb_intra_struct_ub<1>:UB mb_intra_struct_ub<0,1,0>:UB INTRA_PRED_AVAIL_FLAG_D {align1};          /* D */

add  (1) tmp_x_w<1>:W orig_x_ub<0,1,0>:UB 1:UW {align1};                                                        /* X + 1 */
add  (1) tmp_x_w<1>:W w_in_mb_uw<0,1,0>:UW -tmp_x_w<0,1,0>:W {align1};                                          /* width - (X + 1) */
mul.nz.f0.0 (1) null<1>:UD tmp_x_w<0,1,0>:W orig_y_ub<0,1,0>:UB {align1};                                       /* (width - (X + 1)) * Y != 0 */
(f0.0) add (1) mb_intra_struct_ub<1>:UB mb_intra_struct_ub<0,1,0>:UB INTRA_PRED_AVAIL_FLAG_C {align1};          /* C */

mov  (8) vme_msg_1<1>:UD        tmp_reg1.0<8,8,1>:UD {align1};

/* m2 */        
mov (8) vme_msg_2<1>:UD         0x0:UD {align1};
        
/* m3 */        
mov  (8) vme_msg_3<1>:UD         INEP_ROW.0<8,8,1>:UD {align1};

/* m4 */        
mov  (8) vme_msg_4<1>:UD         0x0 {align1};
mov (16) vme_msg_4.0<1>:UB       INEP_COL0.3<32,8,4>:UB {align1};
mov  (1) vme_msg_4.16<1>:UD      INTRA_PREDICTORE_MODE {align1};
        
send (8)
        vme_msg_ind
        vme_wb
        null
        vme(
                BIND_IDX_VME,
                0,
                0,
                VME_MESSAGE_TYPE_INTRA
        )
        mlen vme_msg_length
        rlen vme_intra_wb_length
        {align1};

/*
 * Oword Block Write message
 */
mul  (1) tmp_reg3.8<1>:UD       w_in_mb_uw<0,1,0>:UW orig_y_ub<0,1,0>:UB {align1};
add  (1) tmp_reg3.8<1>:UD       tmp_reg3.8<0,1,0>:UD orig_x_ub<0,1,0>:UB {align1};
mov  (1) tmp_reg3.20<1>:UB      thread_id_ub {align1};                  /* dispatch id */
mov  (8) msg_reg0.0<1>:UD       tmp_reg3<8,8,1>:UD {align1};
        
mov  (1) msg_reg1.0<1>:UD       vme_wb.0<0,1,0>:UD      {align1};
mov  (1) msg_reg1.4<1>:UD       vme_wb.16<0,1,0>:UD     {align1};
mov  (1) msg_reg1.8<1>:UD       vme_wb.20<0,1,0>:UD     {align1};
mov  (1) msg_reg1.12<1>:UD      vme_wb.24<0,1,0>:UD     {align1};
/* bind index 3, write 1 oword, msg type: 8(OWord Block Write) */
send (16)
        msg_ind
        obw_wb
        null
        data_port(
                OBW_CACHE_TYPE,
                OBW_MESSAGE_TYPE,
                OBW_CONTROL_0,
                OBW_BIND_IDX,
                OBW_WRITE_COMMIT_CATEGORY,
                OBW_HEADER_PRESENT
        )
        mlen 2
        rlen obw_wb_length
        {align1};

/*
 * kill thread
 */        
mov  (8) msg_reg0<1>:UD         r0<8,8,1>:UD {align1};
send (16) msg_ind acc0<1>UW null thread_spawner(0, 0, 1) mlen 1 rlen 0 {align1 EOT};
