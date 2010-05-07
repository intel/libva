/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__AVC_ILDB_HEADER__)	// Make sure this file is only included once
#define __AVC_ILDB_HEADER__

// Module name: AVC_ILDB.inc

#undef ORIX
#undef ORIY

//========== Root thread input parameters ==================================================
#define RootParam				r1		// :w
#define	MBsCntX					r1.0	// :w, MB count per row
#define	MBsCntY					r1.1	// :w, MB count per col
//#define PicType					r1.2	// :w, Picture type
#define	MaxThreads				r1.3	// :w, Max Thread limit
#define EntrySignature			r1.4	// :w, Debug flag
#define BitFields				r1.5	// :uw
#define 	MbaffFlag			BIT0	// :w, mbaff flag, bit 0 in BitFields
#define 	BotFieldFlag		BIT1	// :w, bottom field flag, bit 1 in BitFields
#define 	CntlDataExpFlag		BIT2	// :w, Control Data Expansion Flag, bit 2 in BitFields
#define RampConst				r1.12	// 8 :ub, Ramp constant, r1.12 - r1.19:ub
#define StepToNextMB			r1.20	// :b, 2 bytes
#define Minus2Minus1			r1.22	// :b, 2 bytes
// next one starts at r1.11:w

#define 	TopFieldFlag		0xFFFD	// :w, top field flag, used to set bit1 to 0.


//========== Root Locals =============================================================

// Variables in root kernel for launching child therad
#define ChildParam				r2.0	// :w
//Not used  #define	URBOffset				r2.3	// :w, Each row occupies 4 URB entries.  All children in the same row use the same set of URB entries
#define	CurCol					r2.10	// :w, current col
#define	CurColB					r2.20	// :b, current col
#define	CurRow					r2.11	// :w, current row
#define	CurRowB					r2.22	// :b, current row
#define	LastCol					r2.12	// :w, last col
#define	LastRow					r2.13	// :w, last row

// Root local constants during spawning process
#define	Col_Boundary			r3.0	// :w, 
#define	Row_Boundary			r3.1	// :w, 
//#define	TotalBlocks				r3.2	// :w, Total blocks in the frame 
#define	URB_EntriesPerMB_2		r3.3	// :w, = URB entries per MB, but in differnt form
#define	URBOffsetUVBase			r3.4	// :w, UV Base offset in URB

#define	Temp1_D					r3.6	// :d:
#define	Temp1_W					r3.12	// :w, Temp1
#define	Temp1_B					r3.24	// :b, = Temp1_W
#define	Temp2_W					r3.13	// :w, Temp2
#define	Temp2_B					r3.26	// :b, = Temp2_W

// Root local variables
#define JumpTable				r4		// :d, jump table
#define JUMPTABLE_BASE			4*32
#define JumpAddr				a0.7

#define TopRowForScan			r5.0 	// :w, track the top row for scan. All rows above this row is deblocked already. 


// Child Thread R0 Header Field
#define MRF0					m0		
#define CT_R0Hdr				m1

/*
.declare GatewayAperture	Base=r50.0 ElementSize=4 SrcRegion=REGION(8,1) Type=ud	
#define GatewayApertureB	1600	// r50 byte offset from r0.0

// Chroma root thread updates luma root's ThreadLimit at r10.0:w via gateway
#define ThreadLimit				r62.0	// :w, thread limit //r56.0
#define THREAD_LIMIT_OFFSET		0x01800000	// Offset from r50 to r56 = 12*32 = 384 = 0x0180.  0x180 << 16 = 0x01800000
	//#define THREAD_LIMIT_OFFSET		0x00C00000	// Offset from r50 to r56 = 6*32 = 192 = 0x00C0.  0xC0 << 16 = 0x00C00000
*/

// Gateway size is 16 GRF.  68 rows of MBs takes 9 GRFs (r6 - r14)
// For CTG: Expended to support 1280 rows of pixel (80 rows of MBs).  It requires 10 GRFs (r6 - r15)
.declare GatewayAperture	Base=r6.0 ElementSize=4 SrcRegion=REGION(8,1) Type=ud	
#define GatewayApertureB	192	// r0.0 byte offset from r0.0

// Chroma root thread updates luma root's ThreadLimit at r10.0:w via gateway
#define ThreadLimit				r18.0	// :w, thread limit 
#define THREAD_LIMIT_OFFSET		0x01800000	// Offset from r50 to r56 = 12*32 = 384 = 0x0180.  0x180 << 16 = 0x01800000
#define	TotalBlocks				r18.1	// :w, Total blocks in the frame 

// Root local variables
#define	ChildThreadsID			r19.0	// :w, Child thread ID, unique to each child
#define	OutstandingThreads		r20.0	// :w, Outstanding threads 
#define ProcessedMBs			r20.1	// :w, # of MBs processed 

#define	URBOffset				r21.0	// :w, Each row occupies 4 URB entries.  All children in the same row use the same set of URB entries

//=================================================================================

#define ScoreBd_Size			128 //96 // size of Status[] or ProcCol[]

#define ScoreBd_Idx				2		
//#define Saved_Col				0

#define StatusAddr				a0.4	// :w, point to r50
//=================================================================================


// Gateway payload
#define GatewayPayload			r48.0	// :ud
#define GatewayPayloadKey		r48.8	// :uw
#define DispatchID				r48.20	// :ub
#define RegBase_GatewaySize 	r48.5	// :ud, used in open a gateway
#define Offset_Length			r48.5	// :ud, used in forwardmsg back to root
#define EUID_TID				r48.9	// :uw, used in forwardmsg back to root

// Gateway response
#define GatewayResponse 		r49.0	// :ud, one GRF

#define URBWriteMsgDesc			a0.0	// Used in URB write, :ud
#define URBWriteMsgDescLow		a0.0	// Used in URB write, :uw
#define URBWriteMsgDescHigh		a0.1	// Used in URB write, :uw

.declare WritebackResponse		Base=r50 ElementSize=4 SrcRegion=REGION(8,1) Type=ud	// 1 GRF for write backs


/////////////////////////////////////////////////////////////////////////////////////////////
// IDesc Order					Offset
//
// 0) luma root					0 from luma root
// 1) luma child				16 from luma root
// 2) chroma root				32 from luma root
// 3) chroma child				16 from chroma root

// 4) luma field root			0 from luma field root
// 5) luma field child			16 from luma field root
// 6) chroma field root			32 from luma field root
// 7) chroma field child		16 from chroma field root

// 8) luma Mbaff root			0 from luma Mbaff root
// 9) luma Mbaff child 			16 from luma Mbaff root
// 10) chroma Mbaff root		32 from luma Mbaff root
// 11) chroma Mbaff child		16 from chroma Mbaff root

// IDesc offset within non-mbaff or mbaff mode
#define CHROMA_ROOT_OFFSET 		32		// Offset from luma root to chroma root
#define CHILD_OFFSET	  		16		// Offset from luma root to luma child, 
										// and from chroma root to chroma child
/////////////////////////////////////////////////////////////////////////////////////////////


//========== End of Root Variables ======================================================


//========== Child thread input parameters ==============================================
//#define	MBsCntX					r1.0	// :w, MB count per row (same as root)
//#define	MBsCntY					r1.1	// :w, MB count per col (same as root)
//#define PicTypeC				r1.2	// :w, Picture type		same as root thread (same as root)
#define	URBOffsetC				r1.3	// :w, 
#define EntrySignatureC			r1.4	// :w, Debug field (same as root)
//#define BitFields				r1.5	// :w (same as root)
//#define 	MbaffFlag			BIT0	// :w, mbaff flag, bit 0 in BitFields
//#define 	BotFieldFlag		BIT1	// :w, bottom field flag, bit 1 in BitFields
//#define 	CntlDataExpFlag		BIT2	// :w, Control Data Expansion Flag, bit 2 in BitFields
#define RampConstC				r1.12	// 8 :ub, Ramp constant, r1.12 - r1.19:ub.
#define	ORIX					r1.10	// :w, carry over from root r1 in MB count
#define	ORIY					r1.11	// :w, carry over from root r1 in MB count
#define	LastColC				r1.12	// :w, last col
#define	LastRowC				r1.13	// :w, last row

.declare GatewayApertureC		Base=r1.0 ElementSize=4 SrcRegion=REGION(8,1) Type=ud	
#define GatewayApertureCBase	32		// r1 byte offset from r0.0


//========== Child Variables ============================================================

// Mbaff Alpha, Beta, Tc0 vectors for an edge
.declare Mbaff_ALPHA 	Base=r14.0  ElementSize=2 SrcRegion=REGION(8,1) Type=uw		// r14
.declare Mbaff_BETA 	Base=r15.0  ElementSize=2 SrcRegion=REGION(8,1) Type=uw		// r15
.declare Mbaff_TC0 		Base=r16.0  ElementSize=2 SrcRegion=REGION(8,1) Type=uw		// r16
.declare RRampW	   		Base=r17.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w		// r17

.declare Mbaff_ALPHA2	Base=r45.0  ElementSize=2 SrcRegion=REGION(8,1) Type=uw		// alpha2 = (alpha >> 2) + 2


#define	ORIX_CUR				r46.0	// :w, current block origin X in bytes
#define	ORIY_CUR				r46.1	// :w, current block origin Y in bytes
#define	ORIX_LEFT				r46.2	// :w, left block origin X in bytes
#define	ORIY_LEFT				r46.3	// :w, left block origin Y in bytes
#define	ORIX_TOP				r46.4	// :w, top block origin X in bytes
#define	ORIY_TOP				r46.5	// :w, top block origin Y in bytes
//#define FilterSampleFlag		r46.6	// :uw,
#define	CTemp0_W				r46.7	// :w, child Temp0

#define alpha					r46.8	// :w, Scaler version for non Mbaff
#define beta					r46.9	// :w, Scaler version for non Mbaff
#define tc0						r46.20	// 4 :ub, r46.20 ~ r46.23, Scaler version for non Mbaff
#define MaskA					r46.12	// :uw
#define MaskB					r46.13	// :uw

// Child control flags
#define DualFieldMode			r47.0	// Cur MB is frame based, above MB is field based in mbaff mode
										// :uw, 0 = not in dual field mode,  1 = in dual field mode, filter both top and bot fields
#define	GateWayOffsetC			r47.1	// :w, Gateway offset for child writing into root space
#define CntrlDataOffsetY		r47.1	// :ud, MB control data data offset
#define alpha2					r47.4	// :uw, 	alpha2 = (alpha >> 2) + 2

#define VertEdgePattern			r47.5	// :uw, 

#define	CTemp1_W				r47.6	// :w, child Temp1
#define	CTemp1_B				r47.12	// :b, = child Temp1_W
#define	CTemp2_W				r47.7	// :w, child Temp2
#define	CTemp2_B				r47.14	// :b, = child Temp2_W

// Used in child
#define ECM_AddrReg				a0.4	// Edge Control Map register
#define P_AddrReg				a0.6	// point to P samples in left or top MB
#define Q_AddrReg				a0.7	// point to Q samples in cur MB


.declare 	RTempD	   Base=r26.0 ElementSize=4 SrcRegion=REGION(8,1) Type=d	// r26-27
.declare 	RTempB	   Base=r26.0 ElementSize=1 SrcRegion=REGION(8,4) Type=ub	// r26-27
.declare 	RTempW	   Base=r26.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w	// r26-27
#define 	LEFT_TEMP_D		RTempD
#define 	LEFT_TEMP_B		RTempB
#define 	LEFT_TEMP_W		RTempW

.declare 	TempRow0   Base=r26.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w
.declare 	TempRow0B  Base=r26.0 ElementSize=1 SrcRegion=REGION(8,2) Type=ub
.declare 	TempRow1   Base=r27.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w
.declare 	TempRow1B  Base=r27.0 ElementSize=1 SrcRegion=REGION(8,2) Type=ub

.declare 	CUR_TEMP_D	Base=r28.0 ElementSize=4 SrcRegion=REGION(8,1) Type=d	// 8 GRFs
.declare 	CUR_TEMP_B	Base=r28.0 ElementSize=1 SrcRegion=REGION(8,4) Type=ub
.declare 	CUR_TEMP_W	Base=r28.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w

#define FilterSampleFlag		r28.0	// :uw,

.declare 	A   		Base=r28.0 ElementSize=2 SrcRegion=REGION(16,1) Type=w
.declare 	BB   		Base=r29.0 ElementSize=2 SrcRegion=REGION(16,1) Type=w

.declare 	TempRow3   Base=r30.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w
.declare 	TempRow3B  Base=r30.0 ElementSize=1 SrcRegion=REGION(8,2) Type=ub

.declare 	tc0_exp		Base=r30.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w
.declare 	tc8			Base=r30.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w

.declare 	tc_exp		Base=r31.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w
.declare	tx_exp_8	Base=r31.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w

.declare	q0_p0		Base=r32.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w
.declare	ABS_q0_p0	Base=r33.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w

.declare	ap			Base=r34.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w
.declare	aq			Base=r35.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w

// These buffers have the src data for each edge to be beblocked.
// They have modified pixels from previous edges.
//
//	Y:
//	+----+----+----+----+----+----+----+----+
//	| p3 | p2 | P1 | p0 | q0 | q1 | q2 | q3 |
//	+----+----+----+----+----+----+----+----+
//
//	p3 = r[P_AddrReg, 0]<16;16,1>  
//	p2 = r[P_AddrReg, 16]<16;16,1> 
//	p1 = r[P_AddrReg, 32]<16;16,1> 
//	p0 = r[P_AddrReg, 48]<16;16,1> 
// 	q0 = r[Q_AddrReg, 0]<16;16,1>  
//	q1 = r[Q_AddrReg, 16]<16;16,1> 
//	q2 = r[Q_AddrReg, 32]<16;16,1> 
//	q3 = r[Q_AddrReg, 48]<16;16,1> 

.declare	p0123_W		Base=r36.0  ElementSize=2 SrcRegion=REGION(16,1) Type=uw		// r36, r37
.declare	q0123_W		Base=r38.0  ElementSize=2 SrcRegion=REGION(16,1) Type=uw		// r38, r39
.declare	p3			Base=r36.0  ElementSize=1 SrcRegion=REGION(8,1) Type=ub
.declare	p2			Base=r36.16 ElementSize=1 SrcRegion=REGION(8,1) Type=ub
.declare	p1			Base=r37.0  ElementSize=1 SrcRegion=REGION(8,1) Type=ub
.declare	p0			Base=r37.16 ElementSize=1 SrcRegion=REGION(8,1) Type=ub
.declare	q0			Base=r38.0  ElementSize=1 SrcRegion=REGION(8,1) Type=ub
.declare	q1			Base=r38.16 ElementSize=1 SrcRegion=REGION(8,1) Type=ub
.declare	q2			Base=r39.0  ElementSize=1 SrcRegion=REGION(8,1) Type=ub
.declare	q3			Base=r39.16 ElementSize=1 SrcRegion=REGION(8,1) Type=ub

.declare 	TempRow2   Base=r38.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w

// Temp space for mbaff dual field mode
#define		ABOVE_CUR_MB_BASE	40*GRFWIB											// Byte offset to r40
.declare    ABOVE_CUR_MB_YW  Base=r40 	ElementSize=2 SrcRegion=REGION(8,1) Type=uw
.declare    ABOVE_CUR_MB_UW  Base=r40 	ElementSize=2 SrcRegion=REGION(8,1) Type=uw

.declare 	P0_plus_P1   Base=r41.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w
.declare 	Q0_plus_Q1   Base=r42.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w

.declare 	P2_plus_P3   Base=r43.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w
.declare 	Q2_plus_Q3   Base=r44.0 ElementSize=2 SrcRegion=REGION(8,1) Type=w


//////////////////////////////////////////////////////////////////////////////////////////
// MB control data reference

// Expanded control data is in r18 - r25
.declare    CNTRL_DATA_D Base=r18 	ElementSize=4 SrcRegion=REGION(8,1) Type=ud		// For read, 8 GRFs
#define		CNTRL_DATA_BASE	18*GRFWIB												// Base offset to r18

//  Bit mask for extracting bits
#define	MbaffFrameFlag					0x01
#define	FieldModeCurrentMbFlag			0x02
#define FieldModeLeftMbFlag				0x04
#define FieldModeAboveMbFlag			0x08
#define FilterInternal8x8EdgesFlag		0x10
#define FilterInternal4x4EdgesFlag		0x20
#define FilterLeftMbEdgeFlag			0x40
#define FilterTopMbEdgeFlag				0x80

#define	DISABLE_ILDB_FLAG				0x01

//  Exact bit pattern for left and cur MB coding mode (frame vs. field)
#define LEFT_FRAME_CUR_FRAME			0x00
#define LEFT_FRAME_CUR_FIELD			0x02
#define LEFT_FIELD_CUR_FRAME			0x04
#define LEFT_FIELD_CUR_FIELD			0x06

//  Exact bit pattern for above and cur MB coding mode (frame vs. field)
#define ABOVE_FRAME_CUR_FRAME			0x00
#define ABOVE_FRAME_CUR_FIELD			0x02
#define ABOVE_FIELD_CUR_FRAME			0x08
#define ABOVE_FIELD_CUR_FIELD			0x0A



//========== MB control data field offset in byte ==========

#if !defined(_APPLE)

// GRF0 - GRF1 holds original control data

// GRF0
#define HorizOrigin						0
#define VertOrigin						1
#define BitFlags						2		// Bit flags

#define bbSinternalLeftVert				4		// Internal left vertical bS, 2 bits per bS for 4 Y pixels and 2 U/V pixels 	
#define bbSinternalMidVert				5		// Internal mid vertical bS
#define bbSinternalRightVert			6		// Internal right vertical bS
#define bbSinternalTopHorz				7		// Internal top horizontal bS

#define	bbSinternalMidHorz				8		// Internal mid horizontal bS
#define	bbSinternalBotHorz				9		// Internal bottom horizontal bS
#define	wbSLeft0						10		// External left vertical bS (0), 4 bits per bS for 4 Y pixels and 2 U/V pixels, and byte 11

#define	wbSLeft1						12		// External left vertical bS (1), and byte 13
#define	wbSTop0							14		// External top horizontal bS (0), and byte 15

#define	wbSTop1							16		// Externaltop horizontal bS (1), and byte 17
#define	bIndexAinternal_Y				18		// Internal index A for Y
#define	bIndexBinternal_Y				19		// Internal index B for Y

#define	bIndexAleft0_Y					20		// Left index A for Y (0)
#define	bIndexBleft0_Y					21		// Left index B for Y (0)
#define	bIndexAleft1_Y					22		// Left index A for Y (1)
#define	bIndexBleft1_Y					23		// Left index B for Y (1)

#define	bIndexAtop0_Y					24		// Top index A for Y (0)
#define	bIndexBtop0_Y					25		// Top index B for Y (0)
#define	bIndexAtop1_Y					26		// Top index A for Y (1)
#define	bIndexBtop1_Y					27		// Top index B for Y (1)

#define	bIndexAinternal_Cb				28		// Internal index A for Cb
#define	bIndexBinternal_Cb				29		// Internal index B for Cb
#define	bIndexAleft0_Cb					30		// Left index A for Cb (0)
#define	bIndexBleft0_Cb					31		// Left index B for Cb (0)

// GRF1
#define	bIndexAleft1_Cb					32		// Left index A for Cb (1)
#define	bIndexBleft1_Cb					33		// Left index B for Cb (1)
#define	bIndexAtop0_Cb					34		// Top index A for Cb (0)
#define	bIndexBtop0_Cb					35		// Top index B for Cb (0)

#define	bIndexAtop1_Cb					36		// Top index A for Cb (1)
#define	bIndexBtop1_Cb					37		// Top index B for Cb (1)
#define	bIndexAinternal_Cr				38		// Internal index A for Cr
#define	bIndexBinternal_Cr				39		// Internal index B for Cr

#define	bIndexAleft0_Cr					40		// Left index A for Cr (0)
#define bIndexBleft0_Cr					41		// Left index B for Cr (0)
#define	bIndexAleft1_Cr					42		// Left index A for Cr (1)
#define	bIndexBleft1_Cr					43		// Left index B for Cr (1)

#define	bIndexAtop0_Cr					44		// Top index A for Cr (0)
#define	bIndexBtop0_Cr					45		// Top index B for Cr (0)
#define	bIndexAtop1_Cr					46		// Top index A for Cr (1)
#define	bIndexBtop1_Cr					47		// Top index B for Cr (1)

#define	ExtBitFlags						48		// Extended bit flags, such as disable ILDB bits

// Offset 49 - 63 not used


//===== GRF2 - GRF7 hold expanded control data =====

// GRF2
#define	wEdgeCntlMap_IntLeftVert		64		// Derived from bbSinternalLeftVert, 1 bit per pixel 
#define	wEdgeCntlMap_IntMidVert			66		// Derived from bbSinternalLeftVert
		
#define	wEdgeCntlMap_IntRightVert		68		// Derived from bbSinternalRightVert
#define	wEdgeCntlMap_IntTopHorz			70		// Derived from bbSinternalTopHorz, 1bit per pixel 
		
#define	wEdgeCntlMap_IntMidHorz			72		// Derived from bbSinternalMidHorz
#define	wEdgeCntlMap_IntBotHorz			74		// Derived from bbSinternalBotHorz

// Offset 76 - 79 not used

#define	wEdgeCntlMapA_ExtLeftVert0		80		// Derived from wbSLeft0, 1bit per pixel
#define	wEdgeCntlMapB_ExtLeftVert0		82		// Derived from wbSLeft0

#define	wEdgeCntlMapA_ExtTopHorz0		84		// Derived from wbSTop0, 1bit per pixel
#define	wEdgeCntlMapB_ExtTopHorz0		86		// Derived from wbSTop0

#define	wEdgeCntlMapA_ExtLeftVert1		88		// Derived from wbSLeft1, 1bit per pixel
#define	wEdgeCntlMapB_ExtLeftVert1		90		// Derived from wbSLeft1

#define	wEdgeCntlMapA_ExtTopHorz1		92		// Derived from wbSTop1, 1bit per pixel
#define	wEdgeCntlMapB_ExtTopHorz1		94		// Derived from wbSTop1


// GRF3
#define	bTc0_v00_0_Y					96		// Derived from bSv00_0 and bIndexAleft0_Y, 4 pixels per tc0
#define	bTc0_v10_0_Y					97		// Derived from bSv10_0 and bIndexAleft0_Y
#define	bTc0_v20_0_Y					98		// Derived from bSv20_0 and bIndexAleft0_Y
#define	bTc0_v30_0_Y					99		// Derived from bSv30_0 and bIndexAleft0_Y

#define	bTc0_v01_Y						100		// Derived from bSv01 and bIndexAinternal_Y
#define	bTc0_v11_Y						101		// Derived from bSv11 and bIndexAinternal_Y
#define	bTc0_v21_Y						102		// Derived from bSv21 and bIndexAinternal_Y
#define	bTc0_v31_Y						103		// Derived from bSv31 and bIndexAinternal_Y

#define	bTc0_v02_Y						104		// Derived from bSv02 and bIndexAinternal_Y
#define	bTc0_v12_Y						105		// Derived from bSv12 and bIndexAinternal_Y
#define	bTc0_v22_Y						106		// Derived from bSv22 and bIndexAinternal_Y
#define	bTc0_v32_Y						107		// Derived from bSv32 and bIndexAinternal_Y

#define	bTc0_v03_Y						108		// Derived from bSv03 and bIndexAinternal_Y
#define	bTc0_v13_Y						109		// Derived from bSv13 and bIndexAinternal_Y
#define	bTc0_v23_Y						110		// Derived from bSv23 and bIndexAinternal_Y
#define	bTc0_v33_Y						111		// Derived from bSv33 and bIndexAinternal_Y

#define	bTc0_h00_0_Y					112		// Derived from bSh00_0 and bIndexAleft0_Y
#define	bTc0_h01_0_Y					113		// Derived from bSh01_0 and bIndexAleft0_Y
#define	bTc0_h02_0_Y					114		// Derived from bSh02_0 and bIndexAleft0_Y
#define	bTc0_h03_0_Y					115		// Derived from bSh03_0 and bIndexAleft0_Y

#define	bTc0_h10_Y						116		// Derived from bSh10 and bIndexAinternal_Y
#define	bTc0_h11_Y						117		// Derived from bSh11 and bIndexAinternal_Y
#define	bTc0_h12_Y						118		// Derived from bSh12 and bIndexAinternal_Y
#define	bTc0_h13_Y						119		// Derived from bSh13 and bIndexAinternal_Y
		
#define	bTc0_h20_Y						120		// Derived from bSh20 and bIndexAinternal_Y
#define	bTc0_h21_Y						121		// Derived from bSh21 and bIndexAinternal_Y
#define	bTc0_h22_Y						122		// Derived from bSh22 and bIndexAinternal_Y
#define	bTc0_h23_Y						123		// Derived from bSh23 and bIndexAinternal_Y

#define	bTc0_h30_Y						124		// Derived from bSh30 and bIndexAinternal_Y
#define	bTc0_h31_Y						125		// Derived from bSh31 and bIndexAinternal_Y
#define	bTc0_h32_Y						126		// Derived from bSh32 and bIndexAinternal_Y
#define	bTc0_h33_Y						127		// Derived from bSh33 and bIndexAinternal_Y

// GRF4
#define	bAlphaLeft0_Y					128		// Derived from bIndexAleft0_Y	
#define	bBetaLeft0_Y					129		// Derived from bIndexBleft0_Y	
#define	bAlphaTop0_Y					130		// Derived from bIndexAtop0_Y	
#define	bBetaTop0_Y						131		// Derived from bIndexBtop0_Y	

#define	bAlphaInternal_Y				132		// Derived from bIndexAinternal_Y	
#define	bBetaInternal_Y					133		// Derived from bIndexBinternal_Y	
// Offset 134 - 135 not used

// Offset 136 - 143 not used
#define	bAlphaLeft1_Y					144		// Derived from bIndexAleft1_Y	Used in Mbaff mode only
#define	bBetaLeft1_Y					145		// Derived from bIndexBleft1_Y	Used in Mbaff mode only
#define	bAlphaTop1_Y					146		// Derived from bIndexAtop1_Y	Used in Mbaff mode only
#define	bBetaTop1_Y						147		// Derived from bIndexBtop1_Y	Used in Mbaff mode only

// Offset 148 - 151 not used
#define	bTc0_v00_1_Y					152		// Derived from bSv00_1 and bIndexAleft1_Y	Used in Mbaff mode only
#define	bTc0_v10_1_Y					153		// Derived from bSv10_1 and bIndexAleft1_Y	Used in Mbaff mode only
#define	bTc0_v20_1_Y					154		// Derived from bSv20_1 and bIndexAleft1_Y	Used in Mbaff mode only
#define	bTc0_v30_1_Y					155		// Derived from bSv30_1 and bIndexAleft1_Y	Used in Mbaff mode only

#define	bTc0_h00_1_Y					156		// Derived from bSh00_1 and bIndexAleft1_Y	Used in Mbaff mode only
#define	bTc0_h01_1_Y					157		// Derived from bSh01_1 and bIndexAleft1_Y	Used in Mbaff mode only
#define	bTc0_h02_1_Y					158		// Derived from bSh02_1 and bIndexAleft1_Y	Used in Mbaff mode only
#define	bTc0_h03_1_Y					159		// Derived from bSh03_1 and bIndexAleft1_Y	Used in Mbaff mode only


// GRF5
#define	bTc0_v00_0_Cb					160		// Derived from bSv00_0 and bIndexAleft0_Cb, 2 pixels per tc0	Left0
#define	bTc0_v10_0_Cb					161		// Derived from bSv10_0 and bIndexAleft0_Cb	
#define	bTc0_v20_0_Cb					162		// Derived from bSv20_0 and bIndexAleft0_Cb	
#define	bTc0_v30_0_Cb					163		// Derived from bSv30_0 and bIndexAleft0_Cb	

#define	bTc0_v02_Cb						164		// Derived from bSv02 and bIndexAinternal_Cb	MidVert
#define	bTc0_v12_Cb						165		// Derived from bSv12 and bIndexAinternal_Cb	
#define	bTc0_v22_Cb						166		// Derived from bSv22 and bIndexAinternal_Cb	
#define	bTc0_v32_Cb						167		// Derived from bSv32 and bIndexAinternal_Cb	

#define	bTc0_h00_0_Cb					168		// Derived from bSh00_0 and bIndexAleft0_Cb	Top0
#define	bTc0_h01_0_Cb					169		// Derived from bSh01_0 and bIndexAleft0_Cb	
#define	bTc0_h02_0_Cb					170		// Derived from bSh02_0 and bIndexAleft0_Cb	
#define	bTc0_h03_0_Cb					171		// Derived from bSh03_0 and bIndexAleft0_Cb	

#define	bTc0_h20_Cb						172		// Derived from bSh20 and bIndexAinternal_Cb	MidHorz
#define	bTc0_h21_Cb						173		// Derived from bSh21 and bIndexAinternal_Cb	
#define	bTc0_h22_Cb						174		// Derived from bSh22 and bIndexAinternal_Cb	
#define	bTc0_h23_Cb						175		// Derived from bSh23 and bIndexAinternal_Cb	

#define	bTc0_v00_0_Cr					176		// Derived from bSv00_0 and bIndexAleft0_Cr, 2 pixels per tc0	Left0
#define	bTc0_v10_0_Cr					177		// Derived from bSv10_0 and bIndexAleft0_Cr	
#define	bTc0_v20_0_Cr					178		// Derived from bSv20_0 and bIndexAleft0_Cr	
#define	bTc0_v30_0_Cr					179		// Derived from bSv30_0 and bIndexAleft0_Cr	

#define	bTc0_v02_Cr						180		// Derived from bSv02 and bIndexAinternal_Cr	Mid Vert
#define	bTc0_v12_Cr						181		// Derived from bSv12 and bIndexAinternal_Cr	
#define	bTc0_v22_Cr						182		// Derived from bSv22 and bIndexAinternal_Cr	
#define	bTc0_v32_Cr						183		// Derived from bSv32 and bIndexAinternal_Cr	

#define	bTc0_h00_0_Cr					184		// Derived from bSh00_0 and bIndexAleft0_Cr, 2 pixels per tc0	Top0
#define	bTc0_h01_0_Cr					185		// Derived from bSh01_0 and bIndexAleft0_Cr	
#define	bTc0_h02_0_Cr					186		// Derived from bSh02_0 and bIndexAleft0_Cr	
#define	bTc0_h03_0_Cr					187		// Derived from bSh03_0 and bIndexAleft0_Cr	

#define	bTc0_h20_Cr						188		// Derived from bSh20 and bIndexAinternal_Cr	Mid  Horz
#define	bTc0_h21_Cr						189		// Derived from bSh21 and bIndexAinternal_Cr	
#define	bTc0_h22_Cr						190		// Derived from bSh22 and bIndexAinternal_Cr	
#define	bTc0_h23_Cr						191		// Derived from bSh23 and bIndexAinternal_Cr	

// GRF6
#define	bAlphaLeft0_Cb					192		// Derived from bIndexAleft0_Cb
#define	bBetaLeft0_Cb					193		// Derived from bIndexBleft0_Cb
#define	bAlphaTop0_Cb					194		// Derived from bIndexAtop0_Cb
#define	bBetaTop0_Cb					195		// Derived from bIndexBtop0_Cb

#define	bAlphaInternal_Cb				196		// Derived from bIndexAinternal_Cb
#define	bBetaInternal_Cb				197		// Derived from bIndexBinternal_Cb
// Offset 198 - 199 not used		

#define	bAlphaLeft0_Cr					200		// Derived from bIndexAleft0_Cr
#define	bBetaLeft0_Cr					201		// Derived from bIndexBleft0_Cr
#define	bAlphaTop0_Cr					202		// Derived from bIndexAtop0_Cr
#define	bBetaTop0_Cr					203		// Derived from bIndexBtop0_Cr

#define	bAlphaInternal_Cr				204		// Derived from bIndexAinternal_Cr
#define	bBetaInternal_Cr				205		// Derived from bIndexBinternal_Cr
// Offset 206 - 223 not used		

// GRF7
#define	bAlphaLeft1_Cb					224		// Derived from bIndexAleft1_Cb	Used in Mbaff mode only
#define	bBetaLeft1_Cb					225		// Derived from bIndexBleft1_Cb	Used in Mbaff mode only
#define	bAlphaTop1_Cb					226		// Derived from bIndexAtop1_Cb	Used in Mbaff mode only
#define	bBetaTop1_Cb					227		// Derived from bIndexBtop1_Cb	Used in Mbaff mode only

// Offset 228 - 231 not used

#define	bTc0_v00_1_Cb					232		// Derived from bSv00_1 and bIndexAleft1_Cb	Used in Mbaff mode only
#define	bTc0_v10_1_Cb					233		// Derived from bSv10_1 and bIndexAleft1_Cb	Used in Mbaff mode only
#define	bTc0_v20_1_Cb					234		// Derived from bSv20_1 and bIndexAleft1_Cb	Used in Mbaff mode only
#define	bTc0_v30_1_Cb					235		// Derived from bSv30_1 and bIndexAleft1_Cb	Used in Mbaff mode only

#define	bTc0_h00_1_Cb					236		// Derived from bSh00_1 and bIndexAleft1_Cb	Used in Mbaff mode only
#define	bTc0_h01_1_Cb					237		// 	Derived from bSh01_1 and bIndexAleft1_Cb	Used in Mbaff mode only
#define	bTc0_h02_1_Cb					238		// Derived from bSh02_1 and bIndexAleft1_Cb	Used in Mbaff mode only
#define	bTc0_h03_1_Cb					239		// Derived from bSh03_1 and bIndexAleft1_Cb	Used in Mbaff mode only

#define	bAlphaLeft1_Cr					240		// Derived from bIndexAleft1_Cr	Used in Mbaff mode only
#define	bBetaLeft1_Cr					241		// Derived from bIndexBleft1_Cr	Used in Mbaff mode only
#define	bAlphaTop1_Cr					242		// Derived from bIndexAtop1_Cr	Used in Mbaff mode only
#define	bBetaTop1_Cr					243		// Derived from bIndexBtop1_Cr	Used in Mbaff mode only

// Offset 244 - 247 not used		

#define	bTc0_v00_1_Cr					248		// Derived from bSv00_1 and bIndexAleft1_Cr	Used in Mbaff mode only
#define	bTc0_v10_1_Cr					249		// Derived from bSv10_1 and bIndexAleft1_Cr	Used in Mbaff mode only
#define	bTc0_v20_1_Cr					250		// Derived from bSv20_1 and bIndexAleft1_Cr	Used in Mbaff mode only
#define	bTc0_v30_1_Cr					251		// Derived from bSv30_1 and bIndexAleft1_Cr	Used in Mbaff mode only

#define	bTc0_h00_1_Cr					252		// Derived from bSh00_1 and bIndexAleft1_Cr	Used in Mbaff mode only
#define	bTc0_h01_1_Cr					253		// Derived from bSh01_1 and bIndexAleft1_Cr	Used in Mbaff mode only
#define	bTc0_h02_1_Cr					254		// Derived from bSh02_1 and bIndexAleft1_Cr	Used in Mbaff mode only
#define	bTc0_h03_1_Cr					255		// Derived from bSh03_1 and bIndexAleft1_Cr	Used in Mbaff mode only


#else		// _APPLE is defined

//******** Crestline for Apple, progressive only, 88 bytes **********

// GRF0
#define HorizOrigin						0
#define VertOrigin						1
#define BitFlags						2		// Bit flags

#define	wEdgeCntlMap_IntLeftVert		4		// Derived from bbSinternalLeftVert, 1 bit per pixel 
#define	wEdgeCntlMap_IntMidVert			6		// Derived from bbSinternalLeftVert
#define	wEdgeCntlMap_IntRightVert		8		// Derived from bbSinternalRightVert
#define	wEdgeCntlMap_IntTopHorz			10		// Derived from bbSinternalTopHorz, 1bit per pixel 
#define	wEdgeCntlMap_IntMidHorz			12		// Derived from bbSinternalMidHorz
#define	wEdgeCntlMap_IntBotHorz			14		// Derived from bbSinternalBotHorz
#define	wEdgeCntlMapA_ExtLeftVert0		16		// Derived from wbSLeft0, 1bit per pixel
#define	wEdgeCntlMapB_ExtLeftVert0		18		// Derived from wbSLeft0
#define	wEdgeCntlMapA_ExtTopHorz0		20		// Derived from wbSTop0, 1bit per pixel
#define	wEdgeCntlMapB_ExtTopHorz0		22		// Derived from wbSTop0

#define	bAlphaLeft0_Y					24		// Derived from bIndexAleft0_Y	
#define	bBetaLeft0_Y					25		// Derived from bIndexBleft0_Y	
#define	bAlphaTop0_Y					26		// Derived from bIndexAtop0_Y	
#define	bBetaTop0_Y						27		// Derived from bIndexBtop0_Y	
#define	bAlphaInternal_Y				28		// Derived from bIndexAinternal_Y	
#define	bBetaInternal_Y					29		// Derived from bIndexBinternal_Y	

// GRF1
#define	bTc0_v00_0_Y					32		// Derived from bSv00_0 and bIndexAleft0_Y, 4 pixels per tc0
#define	bTc0_v10_0_Y					33		// Derived from bSv10_0 and bIndexAleft0_Y
#define	bTc0_v20_0_Y					34		// Derived from bSv20_0 and bIndexAleft0_Y
#define	bTc0_v30_0_Y					35		// Derived from bSv30_0 and bIndexAleft0_Y
#define	bTc0_v01_Y						36		// Derived from bSv01 and bIndexAinternal_Y
#define	bTc0_v11_Y						37		// Derived from bSv11 and bIndexAinternal_Y
#define	bTc0_v21_Y						38		// Derived from bSv21 and bIndexAinternal_Y
#define	bTc0_v31_Y						39		// Derived from bSv31 and bIndexAinternal_Y
#define	bTc0_v02_Y						40		// Derived from bSv02 and bIndexAinternal_Y
#define	bTc0_v12_Y						41		// Derived from bSv12 and bIndexAinternal_Y
#define	bTc0_v22_Y						42		// Derived from bSv22 and bIndexAinternal_Y
#define	bTc0_v32_Y						43		// Derived from bSv32 and bIndexAinternal_Y
#define	bTc0_v03_Y						44		// Derived from bSv03 and bIndexAinternal_Y
#define	bTc0_v13_Y						45		// Derived from bSv13 and bIndexAinternal_Y
#define	bTc0_v23_Y						46		// Derived from bSv23 and bIndexAinternal_Y
#define	bTc0_v33_Y						47		// Derived from bSv33 and bIndexAinternal_Y

#define	bTc0_h00_0_Y					48		// Derived from bSh00_0 and bIndexAleft0_Y
#define	bTc0_h01_0_Y					49		// Derived from bSh01_0 and bIndexAleft0_Y
#define	bTc0_h02_0_Y					50		// Derived from bSh02_0 and bIndexAleft0_Y
#define	bTc0_h03_0_Y					51		// Derived from bSh03_0 and bIndexAleft0_Y
#define	bTc0_h10_Y						52		// Derived from bSh10 and bIndexAinternal_Y
#define	bTc0_h11_Y						53		// Derived from bSh11 and bIndexAinternal_Y
#define	bTc0_h12_Y						54		// Derived from bSh12 and bIndexAinternal_Y
#define	bTc0_h13_Y						55		// Derived from bSh13 and bIndexAinternal_Y
#define	bTc0_h20_Y						56		// Derived from bSh20 and bIndexAinternal_Y
#define	bTc0_h21_Y						57		// Derived from bSh21 and bIndexAinternal_Y
#define	bTc0_h22_Y						58		// Derived from bSh22 and bIndexAinternal_Y
#define	bTc0_h23_Y						59		// Derived from bSh23 and bIndexAinternal_Y
#define	bTc0_h30_Y						60		// Derived from bSh30 and bIndexAinternal_Y
#define	bTc0_h31_Y						61		// Derived from bSh31 and bIndexAinternal_Y
#define	bTc0_h32_Y						62		// Derived from bSh32 and bIndexAinternal_Y
#define	bTc0_h33_Y						63		// Derived from bSh33 and bIndexAinternal_Y

// GRF2, 
#define	bTc0_v00_0_Cb					64		// Derived from bSv00_0 and bIndexAleft0_Cb, 2 pixels per tc0	Left0
#define	bTc0_v10_0_Cb					65		// Derived from bSv10_0 and bIndexAleft0_Cb	
#define	bTc0_v20_0_Cb					66		// Derived from bSv20_0 and bIndexAleft0_Cb	
#define	bTc0_v30_0_Cb					67		// Derived from bSv30_0 and bIndexAleft0_Cb	
#define	bTc0_v02_Cb						68		// Derived from bSv02 and bIndexAinternal_Cb	MidVert
#define	bTc0_v12_Cb						69		// Derived from bSv12 and bIndexAinternal_Cb	
#define	bTc0_v22_Cb						70		// Derived from bSv22 and bIndexAinternal_Cb	
#define	bTc0_v32_Cb						71		// Derived from bSv32 and bIndexAinternal_Cb	
#define	bTc0_h00_0_Cb					72		// Derived from bSh00_0 and bIndexAleft0_Cb	Top0
#define	bTc0_h01_0_Cb					73		// Derived from bSh01_0 and bIndexAleft0_Cb	
#define	bTc0_h02_0_Cb					74		// Derived from bSh02_0 and bIndexAleft0_Cb	
#define	bTc0_h03_0_Cb					75		// Derived from bSh03_0 and bIndexAleft0_Cb	
#define	bTc0_h20_Cb						76		// Derived from bSh20 and bIndexAinternal_Cb	MidHorz
#define	bTc0_h21_Cb						77		// Derived from bSh21 and bIndexAinternal_Cb	
#define	bTc0_h22_Cb						78		// Derived from bSh22 and bIndexAinternal_Cb	
#define	bTc0_h23_Cb						79		// Derived from bSh23 and bIndexAinternal_Cb	

#define	bAlphaLeft0_Cb					80		// Derived from bIndexAleft0_Cb
#define	bBetaLeft0_Cb					81		// Derived from bIndexBleft0_Cb
#define	bAlphaTop0_Cb					82		// Derived from bIndexAtop0_Cb
#define	bBetaTop0_Cb					83		// Derived from bIndexBtop0_Cb
#define	bAlphaInternal_Cb				84		// Derived from bIndexAinternal_Cb
#define	bBetaInternal_Cb				85		// Derived from bIndexBinternal_Cb

#define	ExtBitFlags						86		// Extended bit flags, such as disable ILDB bits

// Shared between Cb and Cr
#define	bTc0_v00_0_Cr			bTc0_v00_0_Cb	
#define	bTc0_v10_0_Cr		    bTc0_v10_0_Cb	
#define	bTc0_v20_0_Cr		    bTc0_v20_0_Cb	
#define	bTc0_v30_0_Cr		    bTc0_v30_0_Cb	
#define	bTc0_v02_Cr			    bTc0_v02_Cb		
#define	bTc0_v12_Cr			    bTc0_v12_Cb		
#define	bTc0_v22_Cr			    bTc0_v22_Cb		
#define	bTc0_v32_Cr			    bTc0_v32_Cb		
#define	bTc0_h00_0_Cr		    bTc0_h00_0_Cb	
#define	bTc0_h01_0_Cr		    bTc0_h01_0_Cb	
#define	bTc0_h02_0_Cr		    bTc0_h02_0_Cb	
#define	bTc0_h03_0_Cr		    bTc0_h03_0_Cb	
#define	bTc0_h20_Cr			    bTc0_h20_Cb		
#define	bTc0_h21_Cr			    bTc0_h21_Cb		
#define	bTc0_h22_Cr			    bTc0_h22_Cb		
#define	bTc0_h23_Cr			    bTc0_h23_Cb		
                                
#define	bAlphaLeft0_Cr		    bAlphaLeft0_Cb		   
#define	bBetaLeft0_Cr		    bBetaLeft0_Cb		   
#define	bAlphaTop0_Cr		    bAlphaTop0_Cb		   
#define	bBetaTop0_Cr		    bBetaTop0_Cb		   
#define	bAlphaInternal_Cr	    bAlphaInternal_Cb	   
#define	bBetaInternal_Cr	    bBetaInternal_Cb	   


#endif


//========== End of Child Variables ===============================================================

#if !defined(COMBINED_KERNEL)
#define ILDB_LABEL(x)	x		// No symbol extension for standalone kernels
#endif

#endif	// !defined(__AVC_ILDB_HEADER__)
