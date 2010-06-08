.export entry_point INTRA_16x16
.export entry_point INTRA_8x8
.export entry_point INTRA_4x4
.export entry_point INTRA_PCM
.export entry_point FRAME_MB
.export entry_point FIELD_MB
.export entry_point MBAFF_MB
#ifdef SW_SCOREBOARD    
.export entry_point SCOREBOARD
.export entry_point SCOREBOARD_MBAFF
#elif defined(HW_SCOREBOARD)
.export entry_point SETHWSCOREBOARD
.export entry_point SETHWSCOREBOARD_MBAFF
#endif	// SW_SCOREBOARD

#ifdef ENABLE_ILDB
.export entry_point AVC_ILDB_ROOT_Y_ILDB_FRAME
.export entry_point AVC_ILDB_CHILD_Y_ILDB_FRAME
.export entry_point AVC_ILDB_ROOT_UV_ILDB_FRAME
.export entry_point AVC_ILDB_CHILD_UV_ILDB_FRAME
.export entry_point AVC_ILDB_ROOT_Y_ILDB_FIELD
.export entry_point AVC_ILDB_CHILD_Y_ILDB_FIELD
.export entry_point AVC_ILDB_ROOT_UV_ILDB_FIELD
.export entry_point AVC_ILDB_CHILD_UV_ILDB_FIELD
.export entry_point AVC_ILDB_ROOT_Y_ILDB_MBAFF
.export entry_point AVC_ILDB_CHILD_Y_ILDB_MBAFF
.export entry_point AVC_ILDB_ROOT_UV_ILDB_MBAFF
.export entry_point AVC_ILDB_CHILD_UV_ILDB_MBAFF
#endif	// ENABLE_ILDB

.export entry_point BSDRESET
.export entry_point DCRESETDUMMY

.export label INTRA_16x16_VERTICAL
.export label INTRA_16x16_HORIZONTAL
.export label INTRA_16x16_DC
.export label INTRA_16x16_PLANE

.export label INTRA_8X8_VERTICAL
.export label INTRA_8X8_HORIZONTAL
.export label INTRA_8X8_DC
.export label INTRA_8X8_DIAG_DOWN_LEFT
.export label INTRA_8X8_DIAG_DOWN_RIGHT
.export label INTRA_8X8_VERT_RIGHT
.export label INTRA_8X8_HOR_DOWN
.export label INTRA_8X8_VERT_LEFT
.export label INTRA_8X8_HOR_UP

.export label INTRA_4X4_VERTICAL
.export label INTRA_4X4_HORIZONTAL
.export label INTRA_4X4_DC
.export label INTRA_4X4_DIAG_DOWN_LEFT
.export label INTRA_4X4_DIAG_DOWN_RIGHT
.export label INTRA_4X4_VERT_RIGHT
.export label INTRA_4X4_HOR_DOWN
.export label INTRA_4X4_VERT_LEFT
.export label INTRA_4X4_HOR_UP

.export label INTRA_CHROMA_DC
.export label INTRA_CHROMA_HORIZONTAL
.export label INTRA_CHROMA_VERTICAL
.export label INTRA_Chroma_PLANE

.export label intra_Pred_4x4_Y
.export label ADD_ERROR_SB0
.export label ADD_ERROR_SB1
.export label ADD_ERROR_SB2
.export label ADD_ERROR_SB3

.export label AllAVC_END

#ifdef SW_SCOREBOARD    
.export label MB_Loop
.export label No_Message
.export label Dependency_Check
.export label Notify_MSG
.export label Update_CurMB
.export label MBAFF_MB_Loop
.export label MBAFF_No_Message
.export label MBAFF_Dependency_Check
.export label MBAFF_Notify_MSG
.export label MBAFF_Update_CurMB

//.export label 

// Definitions for first pass MC kernel building
#ifndef No_Message_IP
#define No_Message_IP	0
#endif

#ifndef Dependency_Check_IP
#define Dependency_Check_IP	0
#endif

#ifndef Notify_MSG_IP
#define Notify_MSG_IP	0
#endif

#ifndef Update_CurMB_IP
#define Update_CurMB_IP	0
#endif

#ifndef MBAFF_No_Message_IP
#define MBAFF_No_Message_IP	0
#endif

#ifndef MBAFF_Dependency_Check_IP
#define MBAFF_Dependency_Check_IP	0
#endif

#ifndef MBAFF_Notify_MSG_IP
#define MBAFF_Notify_MSG_IP	0
#endif

#ifndef	AS_ENABLED
 #ifndef MBAFF_MB_Loop_IP
 #define MBAFF_MB_Loop_IP	0
 #endif

 #ifndef MB_Loop_IP
 #define MB_Loop_IP	0
 #endif
#endif	// End AS_ENABLED

#ifndef MBAFF_Update_CurMB_IP
#define MBAFF_Update_CurMB_IP	0
#endif

#endif	// SW_SCOREBOARD
/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

#ifdef ENABLE_ILDB
.export label	ALL_SPAWNED_UV_ILDB_FRAME
.export label	SLEEP_ENTRY_UV_ILDB_FRAME
.export label	POST_SLEEP_UV_ILDB_FRAME
.export label	ALL_SPAWNED_Y_ILDB_FRAME
.export label	SLEEP_ENTRY_Y_ILDB_FRAME
.export label	POST_SLEEP_Y_ILDB_FRAME

// Definitions for first pass ILDB kernel building
#ifndef	ALL_SPAWNED_UV_ILDB_FRAME_IP
#define	ALL_SPAWNED_UV_ILDB_FRAME_IP	0
#endif

#ifndef	SLEEP_ENTRY_UV_ILDB_FRAME_IP
#define	SLEEP_ENTRY_UV_ILDB_FRAME_IP	0
#endif

#ifndef	POST_SLEEP_UV_ILDB_FRAME_IP
#define	POST_SLEEP_UV_ILDB_FRAME_IP	0
#endif

#ifndef	ALL_SPAWNED_Y_ILDB_FRAME_IP
#define	ALL_SPAWNED_Y_ILDB_FRAME_IP	0
#endif

#ifndef	SLEEP_ENTRY_Y_ILDB_FRAME_IP
#define	SLEEP_ENTRY_Y_ILDB_FRAME_IP	0
#endif

#ifndef	POST_SLEEP_Y_ILDB_FRAME_IP
#define	POST_SLEEP_Y_ILDB_FRAME_IP	0
#endif

#endif	// ENABLE_ILDB
