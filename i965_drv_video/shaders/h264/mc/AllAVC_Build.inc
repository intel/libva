/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
$table {
AllAVC_END_IP/INSTFACTOR    // Total instruction count
#if (defined(SW_SCOREBOARD) || defined(HW_SCOREBOARD)) && defined(ENABLE_ILDB)
//    23    // Total kernel count
#elif defined(SW_SCOREBOARD) || defined(HW_SCOREBOARD)
//    11    // Total kernel count
#elif defined(ENABLE_ILDB)
//    21    // Total kernel count
#else
//    11    // Total kernel count
#endif
INTRA_16x16_ENTRY/INSTFACTOR    // Instruction offset to 'Intra_16x16'
INTRA_8x8_ENTRY/INSTFACTOR    // Instruction offset to 'Intra_8x8'
INTRA_4x4_ENTRY/INSTFACTOR    // Instruction offset to 'Intra_4x4'
INTRA_PCM_ENTRY/INSTFACTOR    // Instruction offset to 'Intra_PCM'
FRAME_MB_ENTRY/INSTFACTOR    // Instruction offset to 'FrameMB_Motion'
FIELD_MB_ENTRY/INSTFACTOR    // Instruction offset to 'FieldMB_Motion'
MBAFF_MB_ENTRY/INSTFACTOR    // Instruction offset to 'MBAff_Motion'
#ifdef SW_SCOREBOARD    
SCOREBOARD_ENTRY/INSTFACTOR			// Instruction offset to 'scoreboard'
SCOREBOARD_MBAFF_ENTRY/INSTFACTOR	// Instruction offset to 'scoreboard_MBAFF'
#elif defined(HW_SCOREBOARD)
SETHWSCOREBOARD_ENTRY/INSTFACTOR		// Instruction offset to 'AVC_SetIntraDepend'
SETHWSCOREBOARD_MBAFF_ENTRY/INSTFACTOR	// Instruction offset to 'AVC_SetIntraDependMBAFF'
#endif	// SW_SCOREBOARD
#ifdef ENABLE_ILDB
AVC_ILDB_ROOT_Y_ILDB_FRAME_ENTRY/INSTFACTOR    // Instruction offset to 'AVC_ILDB_Root_Y'
AVC_ILDB_CHILD_Y_ILDB_FRAME_ENTRY/INSTFACTOR    // Instruction offset to 'AVC_ILDB_Child_Y'
AVC_ILDB_ROOT_UV_ILDB_FRAME_ENTRY/INSTFACTOR    // Instruction offset to 'AVC_ILDB_Root_UV'
AVC_ILDB_CHILD_UV_ILDB_FRAME_ENTRY/INSTFACTOR    // Instruction offset to 'AVC_ILDB_Child_UV'
AVC_ILDB_ROOT_Y_ILDB_FIELD_ENTRY/INSTFACTOR    // Instruction offset to 'AVC_ILDB_Root_Field_Y'
AVC_ILDB_CHILD_Y_ILDB_FIELD_ENTRY/INSTFACTOR    // Instruction offset to 'AVC_ILDB_Child_Field_Y'
AVC_ILDB_ROOT_UV_ILDB_FIELD_ENTRY/INSTFACTOR    // Instruction offset to 'AVC_ILDB_Root_Field_UV'
AVC_ILDB_CHILD_UV_ILDB_FIELD_ENTRY/INSTFACTOR    // Instruction offset to 'AVC_ILDB_Child_Field_UV'
AVC_ILDB_ROOT_Y_ILDB_MBAFF_ENTRY/INSTFACTOR    // Instruction offset to 'AVC_ILDB_Root_Mbaff_Y'
AVC_ILDB_CHILD_Y_ILDB_MBAFF_ENTRY/INSTFACTOR    // Instruction offset to 'AVC_ILDB_Child_Mbaff_Y'
AVC_ILDB_ROOT_UV_ILDB_MBAFF_ENTRY/INSTFACTOR    // Instruction offset to 'AVC_ILDB_Root_Mbaff_UV'
AVC_ILDB_CHILD_UV_ILDB_MBAFF_ENTRY/INSTFACTOR    // Instruction offset to 'AVC_ILDB_Child_Mbaff_UV'
#endif	// ENABLE_ILDB
BSDRESET_ENTRY/INSTFACTOR    // Instruction offset to 'BSDReset'
DCRESETDUMMY_ENTRY/INSTFACTOR    // Instruction offset to 'DCResetDummy'

//    0    // Instruction offset to Intra_4x4_luma_prediction_mode_0
INTRA_4X4_HORIZONTAL_IP-INTRA_4X4_VERTICAL_IP    // Instruction offset to Intra_4x4_luma_prediction_mode_1
INTRA_4X4_DC_IP-INTRA_4X4_VERTICAL_IP    // Instruction offset to Intra_4x4_luma_prediction_mode_2
INTRA_4X4_DIAG_DOWN_LEFT_IP-INTRA_4X4_VERTICAL_IP    // Instruction offset to Intra_4x4_luma_prediction_mode_3
INTRA_4X4_DIAG_DOWN_RIGHT_IP-INTRA_4X4_VERTICAL_IP    // Instruction offset to Intra_4x4_luma_prediction_mode_4
INTRA_4X4_VERT_RIGHT_IP-INTRA_4X4_VERTICAL_IP    // Instruction offset to Intra_4x4_luma_prediction_mode_5
INTRA_4X4_HOR_DOWN_IP-INTRA_4X4_VERTICAL_IP    // Instruction offset to Intra_4x4_luma_prediction_mode_6
INTRA_4X4_VERT_LEFT_IP-INTRA_4X4_VERTICAL_IP    // Instruction offset to Intra_4x4_luma_prediction_mode_7
INTRA_4X4_HOR_UP_IP-INTRA_4X4_VERTICAL_IP    // Instruction offset to Intra_4x4_luma_prediction_mode_8

//    0    // Instruction offset to Intra_8x8_luma_prediction_mode_0
INTRA_8X8_HORIZONTAL_IP-INTRA_8X8_VERTICAL_IP    // Instruction offset to Intra_8x8_luma_prediction_mode_1
INTRA_8X8_DC_IP-INTRA_8X8_VERTICAL_IP    // Instruction offset to Intra_8x8_luma_prediction_mode_2
INTRA_8X8_DIAG_DOWN_LEFT_IP-INTRA_8X8_VERTICAL_IP    // Instruction offset to Intra_8x8_luma_prediction_mode_3
INTRA_8X8_DIAG_DOWN_RIGHT_IP-INTRA_8X8_VERTICAL_IP    // Instruction offset to Intra_8x8_luma_prediction_mode_4
INTRA_8X8_VERT_RIGHT_IP-INTRA_8X8_VERTICAL_IP    // Instruction offset to Intra_8x8_luma_prediction_mode_5
INTRA_8X8_HOR_DOWN_IP-INTRA_8X8_VERTICAL_IP    // Instruction offset to Intra_8x8_luma_prediction_mode_6
INTRA_8X8_VERT_LEFT_IP-INTRA_8X8_VERTICAL_IP    // Instruction offset to Intra_8x8_luma_prediction_mode_7
INTRA_8X8_HOR_UP_IP-INTRA_8X8_VERTICAL_IP    // Instruction offset to Intra_8x8_luma_prediction_mode_8

//    0    // Instruction offset to Intra_16x16_luma_prediction_mode_0
INTRA_16x16_HORIZONTAL_IP-INTRA_16x16_VERTICAL_IP    // Instruction offset to Intra_16x16_luma_prediction_mode_1
INTRA_16x16_DC_IP-INTRA_16x16_VERTICAL_IP    // Instruction offset to Intra_16x16_luma_prediction_mode_2
INTRA_16x16_PLANE_IP-INTRA_16x16_VERTICAL_IP    // Instruction offset to Intra_16x16_luma_prediction_mode_3

//    0    // Instruction offset to intra_chroma_prediction_mode_0
INTRA_CHROMA_HORIZONTAL_IP-INTRA_CHROMA_DC_IP    // Instruction offset to intra_chroma_prediction_mode_1
INTRA_CHROMA_VERTICAL_IP-INTRA_CHROMA_DC_IP    // Instruction offset to intra_chroma_prediction_mode_2
INTRA_Chroma_PLANE_IP-INTRA_CHROMA_DC_IP    // Instruction offset to intra_chroma_prediction_mode_3

intra_Pred_4x4_Y_IP-ADD_ERROR_SB3_IP*0x100+intra_Pred_4x4_Y_IP-ADD_ERROR_SB2_IP*0x100+intra_Pred_4x4_Y_IP-ADD_ERROR_SB1_IP*0x100+intra_Pred_4x4_Y_IP-ADD_ERROR_SB0_IP	// Instruction offset to intra_4x4_pred_module
}
