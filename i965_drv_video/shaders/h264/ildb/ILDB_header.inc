/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__ILDB_HEADER__)	// Make sure this file is only included once
#define __ILDB_HEADER__

// Module name: ILDB_header.inc
//

.default_execution_size	(16)
.default_register_type	:ub

#undef NULLREG
#undef RETURN_REG
#undef EOTMSGDSC
#undef MSGSRC
#undef END_THREAD
#undef TSMSGDSC

//  ----------- Common constant definitions ------------
//
//  Bit position constants
//
#define BIT0	0x01
#define BIT1	0x02
#define BIT2	0x04
#define BIT3	0x08
#define BIT4	0x10
#define BIT5	0x20
#define BIT6	0x40
#define BIT7	0x80
#define BIT8	0x0100
#define BIT9	0x0200
#define BIT10	0x0400
#define BIT11	0x0800
#define BIT12	0x1000
#define BIT13	0x2000
#define BIT14	0x4000
#define BIT15	0x8000
#define BIT16	0x00010000
#define BIT17	0x00020000
#define BIT18	0x00040000
#define BIT19	0x00080000
#define BIT20	0x00100000
#define BIT21	0x00200000
#define BIT22	0x00400000
#define BIT23	0x00800000
#define BIT24	0x01000000
#define BIT25	0x02000000
#define BIT26	0x04000000
#define BIT27	0x08000000
#define BIT28	0x10000000
#define BIT29	0x20000000
#define BIT30	0x40000000
#define BIT31	0x80000000

// Common constants
//
#define	INST_SIZE	16	// Instruction size in byte

#define	GRFWIB	32		// GRF register width in byte
#define	GRFWIW	16		// GRF register width in word
#define	GRFWID	8		// GRF register width in dword

#define TOP_FIELD	0
#define BOTTOM_FIELD	1

#define PREVIOUS_FRAME	0	// Previous frame
#define CURRENT_FRAME	1	// Current frame
#define NEXT_FRAME	2	// Next frame

#define Y_ROW_WIDTH		16	// in bytes
#define UV_ROW_WIDTH	8

//  Useful macros
//
#define REGION(Width,HStride) <Width*HStride;Width,HStride>	// Region definition when ExecSize = Width

#define NULLREG		null<1>:d
#define NULLREGW	null<1>:w

#define RETURN_REG	r62		// Return pointer for all sub-routine calls (type DWORD)

#define CALL(subFunc, skipInst)	add (1) RETURN_REG<1>:ud   ip:ud	(1+skipInst)*INST_SIZE \n\
				jmpi (1) subFunc

#define	RETURN		mov (1)	ip:ud	RETURN_REG<0;1,0>:ud		// Return to calling module

#define PRED_CALL(flag, subFunc, skipInst)	add (1) RETURN_REG<1>:ud   ip:ud	(1+skipInst)*INST_SIZE \n\
			(flag)	jmpi (1) subFunc


//  Definitions for surface states, GRF regions, and common data fields
//
//  Note: Each kernel needs to define a specific symbol before including this
//  header file to ensure correct definitions.
//



#if defined(AVC_ILDB)	

.reg_count_total    64
.reg_count_payload  4

    // Binding Table Index
	#define	BI_CNTRL_DATA		0			// Control data map  
    #define	BI_SRC_Y			1
    #define	BI_SRC_UV			2
    #define	BI_DEST_Y			3
    #define	BI_DEST_UV			4
	

	//========== Left MB, 4x16 in r2 and r3 ==========
	#define		PREV_MB_Y_BASE	64		//2*GRFWIB												// Byte offset to r2
	.declare    PREV_MB_YD  Base=r2 	ElementSize=4 SrcRegion=REGION(8,1) Type=ud
	.declare    PREV_MB_YW  Base=r2 	ElementSize=2 SrcRegion=REGION(8,1) Type=uw
    .declare    PREV_MB_YB  Base=r2 	ElementSize=1 SrcRegion=REGION(16,1) Type=ub
    
    #define		PREV_MB_U_BASE	64		//2*GRFWIB		// seperate thread from Y				// Byte offset to r2
	.declare	PREV_MB_UD	Base=r2 	ElementSize=4 SrcRegion=REGION(8,1) Type=ud
    .declare    PREV_MB_UW  Base=r2 	ElementSize=2 SrcRegion=REGION(16,1) Type=uw
    .declare    PREV_MB_UB  Base=r2 	ElementSize=1 SrcRegion=REGION(16,1) Type=ub
    
	#define		PREV_MB_V_BASE	65		//2*GRFWIB+1		// NV12									// Byte offset to r2.1
    .declare    PREV_MB_VB	Base=r2.1 	ElementSize=1 SrcRegion=REGION(16,1) Type=ub

	//========== Top MB, 16x4 in r4 and r5 ==========	
	#define		TOP_MB_Y_BASE	128		//4*GRFWIB												// Byte offset to r4
	.declare    TOP_MB_YD	Base=r4		ElementSize=4 SrcRegion=REGION(8,1) Type=ud
	.declare    TOP_MB_YW	Base=r4		ElementSize=2 SrcRegion=REGION(8,1) Type=uw
    .declare    TOP_MB_YB	Base=r4		ElementSize=1 SrcRegion=REGION(16,1) Type=ub
    
    #define		TOP_MB_U_BASE	128		//4*GRFWIB		// seperate thread from Y				// Byte offset to r4
	.declare	TOP_MB_UD	Base=r4 	ElementSize=4 SrcRegion=REGION(8,1) Type=ud
    .declare    TOP_MB_UW	Base=r4		ElementSize=2 SrcRegion=REGION(16,1) Type=uw
    .declare    TOP_MB_UB	Base=r4 	ElementSize=1 SrcRegion=REGION(16,1) Type=ub
    
	#define		TOP_MB_V_BASE	129		//4*GRFWIB+1		// NV12									// Byte offset to r4.1
    .declare    TOP_MB_VB  Base=r4.1 	ElementSize=1 SrcRegion=REGION(16,1) Type=ub

    
    //========== Current MB, 16x16 in r6-r13 ==========
	#define		SRC_MB_Y_BASE	192 	//6*GRFWIB												// Byte offset to r6
    .declare    SRC_YD  	Base=r6 	ElementSize=4 SrcRegion=REGION(8,1) Type=ud		// For read and write, 8 GRFs
    .declare    SRC_YW  	Base=r6 	ElementSize=2 SrcRegion=REGION(8,1) Type=uw
    .declare    SRC_YB  	Base=r6 	ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// 8 GRFs	

	#define		SRC_MB_U_BASE	192		//6*GRFWIB		// seperate thread from Y				// Byte offset to r6
    .declare    SRC_UD  	Base=r6 	ElementSize=4 SrcRegion=REGION(8,1) Type=ud		// For read and write, 2 GRFs
    .declare    SRC_UW  	Base=r6 	ElementSize=2 SrcRegion=REGION(16,1) Type=uw	// For read and write, 4 GRFs
    .declare    SRC_UB  	Base=r6 	ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// 2 GRFs
          
	#define		SRC_MB_V_BASE	193		// 6*GRFWIB+1		// NV12									// Byte offset to r6.1
    .declare    SRC_VD  	Base=r6.1 	ElementSize=4 SrcRegion=REGION(8,1) Type=ud		// For read and write, 2 GRFs
    .declare    SRC_VW  	Base=r6.1 	ElementSize=2 SrcRegion=REGION(16,1) Type=uw	// For read and write, 4 GRFs
    .declare    SRC_VB  	Base=r6.1 	ElementSize=1 SrcRegion=REGION(16,1) Type=ub	// 2 GRFs

#else				// No kernel specified, define nothing.

.reg_count_total    64
.reg_count_payload  2

	#define	SRCAOFF	r1.0:ud		// Offset into alpha data
	#define	SRCOFF	r1.1:ud		// Offset into source YUV data
	#define ORIX	r1.4		// :w, H. origin of the destination block in pel
	#define ORIY	r1.5		// :w, V. origin of the destination block in pel

#endif

//  ----------- Message Payload Header fields------------
//
#define IDP		r0.2:ud		// Interface Descriptor Pointer
#define	BTP		r0.4:ud		// Binding Table Pointer

//  ----------- Common Message Descriptor ------------
//
#ifdef DEV_ILK
#define GW_DCN				// Should be enabled only for ILK-B0 and beyond
#define MSG_GW		0x03		// Message Gateway
#define MSG_GW_EOT	0x23		// Message Gateway plus EOT bit set (For ILK only)
#define DAPREAD		0x04		// Data Port Read Extended Message Descriptor,
#define DAPWRITE	0x05		// Data Port Write Extended Message Descriptor,
#define URBWRITE	0x06		// URB
#define TS			0x07		// Thread Spawner Extended Message Descriptor
#define TS_EOT		0x27		// End of Thread Extended Message Descriptor

#define EOTMSGDSC	0x02000000	// End of Thread Message Descriptor /w URB handle dereferenced (used by root kernel)
#define CHILD_EOTMSGDSC	0x02000012	// End of Child Thread Message Descriptor w/o URB handle dereferenced

// Data Port Message Descriptor
#define DWBRMSGDSC_RC	 0x02086000	// DWORD Block Read Message Descriptor, reading from render cache = 6.
#define DWBRMSGDSC_RC_TF 0x02086600	// DWORD Block Read Message Descriptor, reading from render cache = 6.
#define DWBRMSGDSC_RC_BF 0x02086700	// DWORD Block Read Message Descriptor, reading from render cache = 6.
#define DWBRMSGDSC_SC	 0x0208A000	// DWORD Block Read Message Descriptor, reading from sampler cache = A.
#define DWBRMSGDSC_SC_TF 0x0208E600	// DWORD Block Read Message Descriptor, reading top field from field mode sampler cache.
#define DWBRMSGDSC_SC_BF 0x0208E700	// DWORD Block Read Message Descriptor, reading bottom field from field mode sampler cache.

#define ILDBRMSGDSC		 0x02085800	// AVC ILDB Control Data Read Msg Desc on Bearlake-C

#define DWBWMSGDSC		 0x02082000	// DWORD Block Write Message Descriptor
#define DWBWMSGDSC_WC 	 0x0218A000	// DWORD Block Write Message Descriptor + write commit

// URB Message Descriptor
#define	URBWMSGDSC	0x02080000		// URB Write Message Descriptor

// Thread Spawner Message Descriptor
#define	TSMSGDSC	0x02000001

// Message Gateway Message Descriptors
#define OGWMSGDSC	0x02000000	// OpenGateway Message Descriptor
#define CGWMSGDSC	0x02000001	// CloseGateway Message Descriptor
#define FWDMSGDSC	0x02000002	// ForwardMsg Message Descriptor
#define	NOTIFYMSG	0x00008000	// Send notification with ForwardMsg message

#define	RESP_LEN(len)	0x100000*len
#define MSG_LEN(len)	0x2000000*len

#else	// Pre DEV_ILK

#define MSG_GW
#define MSG_GW_EOT
#define DAPREAD
#define DAPWRITE
#define URBWRITE
#define TS
#define TS_EOT

#define EOTMSGDSC	0x87100000		// End of Thread Message Descriptor /w URB handle dereferenced (used by root kernel) 
#define CHILD_EOTMSGDSC	0x87100012	// End of Child Thread Message Descriptor w/o URB handle dereferenced

// Data Port Message Descriptor
#define DWBRMSGDSC_RC	 0x04106000	// DWORD Block Read Message Descriptor, reading from render cache = 6.
#define DWBRMSGDSC_RC_TF 0x04106600	// DWORD Block Read Message Descriptor, reading from render cache = 6.
#define DWBRMSGDSC_RC_BF 0x04106700	// DWORD Block Read Message Descriptor, reading from render cache = 6.
#define DWBRMSGDSC_SC	 0x0410A000	// DWORD Block Read Message Descriptor, reading from sampler cache = A.
#define DWBRMSGDSC_SC_TF 0x0410A600	// DWORD Block Read Message Descriptor, reading top field from field mode sampler cache.
#define DWBRMSGDSC_SC_BF 0x0410A700	// DWORD Block Read Message Descriptor, reading bottom field from field mode sampler cache.

#define ILDBRMSGDSC		 0x04105800	// AVC ILDB Control Data Read Msg Desc on Bearlake-C

#define DWBWMSGDSC		 0x05102000	// DWORD Block Write Message Descriptor
#define DWBWMSGDSC_WC 	 0x0511A000	// DWORD Block Write Message Descriptor + write commit

// URB Message Descriptor
#define	URBWMSGDSC	0x06100000		// URB Write Message Descriptor

// Thread Spawner Message Descriptor
#define	TSMSGDSC	0x07100001

// Message Gateway Message Descriptors
#define OGWMSGDSC	0x03100000	// OpenGateway Message Descriptor
#define CGWMSGDSC	0x03100001	// CloseGateway Message Descriptor
#define FWDMSGDSC	0x03100002	// ForwardMsg Message Descriptor
#define	NOTIFYMSG	0x00008000	// Send notification with ForwardMsg message

#define	RESP_LEN(len)	0x10000*len
#define MSG_LEN(len)	0x100000*len

//	bits 15 - 0 = 01 011 000 00000000 = 0101 1000 0000 0000 = 5800
// Render cache, AVC loop rd, 				  			
#endif	// DEV_ILK
				
//	Enable frame/field selection in message descriptor
#define ENMSGDSCFM	0x400		// Enable MSGDSC to select frame surface
#define ENMSGDSCTF	0x600		// Enable MSGDSC to select top field surface
#define ENMSGDSCBF	0x700		// Enable MSGDSC to select bottom field surface

#define END_THREAD      send (8) NULLREG MSGHDR r0:ud TS_EOT	EOTMSGDSC
#define END_CHILD_THREAD	send (8) NULLREG MSGHDR r0:ud TS_EOT	CHILD_EOTMSGDSC

//  ----------- Message related register ------------
//
#define MSGHDR		m1		// Message Payload Header
#define MSGHDRY		m1		// Message Payload Header register for Y data
#define MSGHDRU		m2		// Message Payload Header register for U data
#define MSGHDRV		m3		// Message Payload Header register for V data

#define MSGHDRC		m1		// Message Payload Header register for CUR MB
#define MSGHDRL		m2		// Message Payload Header register for LEFT MB
#define MSGHDRT		m3		// Message Payload Header register for TOP MB

#define MSGHDRYA	m4		// Second Message Payload Header register for Y data
#define MSGSRC		r63		// Message source register
#define MSGDSC		a0.0:ud		// Message Descriptor register (type DWORD)

#define MH_ORI		MSGSRC.0	// DWORD block R/W message header block offset
#define MH_ORIX		MSGSRC.0	// DWORD block R/W message header X offset
#define MH_ORIY		MSGSRC.1	// DWORD block R/W message header Y offset
#define MH_SIZE		MSGSRC.2	// DWORD block R/W message header block width & height


//  M2 - M9 for message data payload
.declare    MSGPAYLOADB	Base=m2 ElementSize=1 SrcRegion=REGION(16,1) Type=ub
.declare    MSGPAYLOADW	Base=m2 ElementSize=2 SrcRegion=REGION(16,1) Type=uw
.declare    MSGPAYLOADD	Base=m2 ElementSize=4 SrcRegion=REGION(8,1) Type=ud
.declare    MSGPAYLOADF	Base=m2 ElementSize=4 SrcRegion=REGION(8,1) Type=f

// End of ILDB_header.inc

#endif	// !defined(__ILDB_HEADER__)
