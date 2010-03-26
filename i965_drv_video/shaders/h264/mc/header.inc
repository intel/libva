/*
 * Common header file for all AVC MC kernels
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__HEADER__)	// Make sure this file is only included once
#define __HEADER__

// Module name: header.inc
//
// Common header file for all AVC MC kernels
//

#ifndef	COMBINED_KERNEL
#ifdef DEV_CTG
  #define SW_SCOREBOARD		// SW Scoreboard should be enabled for CTG and earlier
  #undef HW_SCOREBOARD		// HW Scoreboard should be disabled for CTG and earlier
#else
  #define HW_SCOREBOARD		// HW Scoreboard should be enabled for ILK and beyond
  #undef SW_SCOREBOARD		// SW Scoreboard should be disabled for ILK and beyond
#endif	// DEV_CTG
#endif	// COMBINED_KERNEL

//#define MONO				// Build Monochrome kernels

//  Surface state definition
//
#define	DESTY		0
#define	DESTUV		1
#define	REFYFM0		2
#define	REFYFM1		3
#define	REFYFM2		4
#define	REFYFM3		5
#define	REFYFM4		6
#define	REFYFM5		7
#define	REFYFM6		8
#define	REFYFM7		9
#define	REFYFM8		10
#define	REFYFM9		11
#define	REFYFM10	12
#define	REFYFM11	13
#define	REFYFM12	14
#define	REFYFM13	15
#define	REFYFM14	16
#define	REFYFM15	17
#define	REFUVFM0	18
#define	REFUVFM1	19
#define	REFUVFM2	20
#define	REFUVFM3	21
#define	REFUVFM4	22
#define	REFUVFM5	23
#define	REFUVFM6	24
#define	REFUVFM7	25
#define	REFUVFM8	26
#define	REFUVFM9	27
#define	REFUVFM10	28
#define	REFUVFM11	29
#define	REFUVFM12	30
#define	REFUVFM13	31
#define	REFUVFM14	32
#define	REFUVFM15	33

.default_execution_size	(16)
.default_register_type	:ub

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

#define	GRFWIB	32		// GRF register width in byte
#define	GRFWIW	16		// GRF register width in word
#define	GRFWID	8		// GRF register width in dword

#define INST_SIZE   16		// Instruction size = 128b = 16 Bytes

#define REGION(Width,HStride) <Width*HStride;Width,HStride>

#define NULLREG		null<1>:ud
#define NULLREGW	null<1>:w

#define TOP_FIELD		0
#define BOTTOM_FIELD	1

//  M2 - M9 for date writing message payload
.declare    MSGPAYLOAD	Base=m2 ElementSize=1 SrcRegion=REGION(16,1) Type=ub
.declare    MSGPAYLOADB	Base=m2 ElementSize=1 SrcRegion=REGION(16,1) Type=ub
.declare    MSGPAYLOADW	Base=m2 ElementSize=2 SrcRegion=REGION(16,1) Type=uw
.declare    MSGPAYLOADD	Base=m2 ElementSize=4 SrcRegion=REGION(8,1) Type=ud

//  ----------- Common Message Descriptor ------------
//
#ifdef DEV_ILK
#define MSG_GW		0x03		// Message Gateway Extended Message Descriptor,
#define DAPREAD		0x04		// Data Port Read Extended Message Descriptor,
#define DAPWRITE	0x05		// Data Port Write Extended Message Descriptor,
#define TS			0x07		// Thread Spawner Extended Message Descriptor
#define TS_EOT		0x27		// End of Thread Extended Message Descriptor

#define EOTMSGDSC	0x02000010	// End of Thread Message Descriptor, don't deference URB handle

// Data Port Message Descriptor
#define DWBRMSGDSC_RC	 0x02086000	// DWORD Block Read Message Descriptor, reading from render cache = 6.
#define DWBRMSGDSC_RC_TF 0x02086600	// DWORD Block Read Message Descriptor, reading from render cache = 6.
#define DWBRMSGDSC_RC_BF 0x02086700	// DWORD Block Read Message Descriptor, reading from render cache = 6.
#define DWBRMSGDSC_SC	 0x0208A000	// DWORD Block Read Message Descriptor, reading from sampler cache = A.
#define DWBRMSGDSC_SC_TF 0x0208E600	// DWORD Block Read Message Descriptor, reading top field from field mode sampler cache.
#define DWBRMSGDSC_SC_BF 0x0208E700	// DWORD Block Read Message Descriptor, reading bottom field from field mode sampler cache.

#define DWBWMSGDSC		 0x02082000	// DWORD Block Write Message Descriptor
#define DWBWMSGDSC_WC 	 0x0218A000	// DWORD Block Write Message Descriptor + write commit

// Enable Write Commit writeback mesage
#define	ENWRCOM		0x00108000	// Enable "write commit" and set response length = 1

// Thread Spawner Message Descriptor
#define	TSMSGDSC	0x02000011

// Message Gateway Message Descriptors
#define OGWMSGDSC	0x02000000	// OpenGateway Message Descriptor
#define CGWMSGDSC	0x02000001	// CloseGateway Message Descriptor
#define FWDMSGDSC	0x02000002	// ForwardMsg Message Descriptor

#define	NOTIFYMSG	0x00008000	// Send notification with ForwardMsg message

#define	RESP_LEN(len)	0x100000*len
#define MSG_LEN(len)	0x2000000*len

#else	// Pre DEV_ILK

#define MSG_GW
#define DAPREAD
#define DAPWRITE
#define TS
#define TS_EOT

#define EOTMSGDSC	0x87100010	// End of Thread Message Descriptor, don't deference URB handle

// Data Port Message Descriptor
#define DWBRMSGDSC_RC	 0x04106000	// DWORD Block Read Message Descriptor, reading from render cache = 6.
#define DWBRMSGDSC_RC_TF 0x04106600	// DWORD Block Read Message Descriptor, reading from render cache = 6.
#define DWBRMSGDSC_RC_BF 0x04106700	// DWORD Block Read Message Descriptor, reading from render cache = 6.
#define DWBRMSGDSC_SC	 0x0410A000	// DWORD Block Read Message Descriptor, reading from sampler cache = A.
#define DWBRMSGDSC_SC_TF 0x0410A600	// DWORD Block Read Message Descriptor, reading top field from field mode sampler cache.
#define DWBRMSGDSC_SC_BF 0x0410A700	// DWORD Block Read Message Descriptor, reading bottom field from field mode sampler cache.

#define DWBWMSGDSC		 0x05102000	// DWORD Block Write Message Descriptor
#define DWBWMSGDSC_WC 	 0x0511A000	// DWORD Block Write Message Descriptor + write commit

// Enable Write Commit writeback mesage
#define	ENWRCOM		0x00018000	// Enable "write commit" and set response length = 1

// Thread Spawner Message Descriptor
#define	TSMSGDSC	0x07100011

// Message Gateway Message Descriptors
#define OGWMSGDSC	0x03100000	// OpenGateway Message Descriptor
#define CGWMSGDSC	0x03100001	// CloseGateway Message Descriptor
#define FWDMSGDSC	0x03100002	// ForwardMsg Message Descriptor

#define	NOTIFYMSG	0x00008000	// Send notification with ForwardMsg message
#define	ACKREQMSG	0x00014000	// Acknowledgement required so response length should be 1

#define	RESP_LEN(len)	0x10000*len
#define MSG_LEN(len)	0x100000*len

#endif	// DEV_ILK

// Enable frame/field selection in message descriptor
#define ENMSGDSCFM	0x400		// Enable MSGDSC to select frame surface
#define ENMSGDSCTF	0x600		// Enable MSGDSC to select top field surface
#define ENMSGDSCBF	0x700		// Enable MSGDSC to select bottom field surface

//  ----------- Message related register ------------
//
#define MSGHDR		m1		// Message Payload Header
#define MSGHDRY		m1		// Message Payload Header register for Y data
#define MSGHDRY0	m1		// Message Payload Header register for Y data
#define MSGHDRY1	m2		// Message Payload Header register for Y data
#define MSGHDRY2	m3		// Message Payload Header register for Y data
#define MSGHDRY3	m4		// Message Payload Header register for Y data
#define MSGHDRUV	m5		// Message Payload Header register for U/V data
#define MSGSRC		r62		// Message source register, should never be used for other purposes
#define MSGDSC		a0.0:ud	// Message Descriptor register (type DWORD)

#define MH_ORI		MSGSRC.0	// DWORD block R/W message header block offset
#define MH_ORIX		MSGSRC.0	// DWORD block R/W message header X offset
#define MH_ORIY		MSGSRC.1	// DWORD block R/W message header Y offset
#define MH_SIZE		MSGSRC.2	// DWORD block R/W message header block width & height

// Data necessary for kernel operations
//
//  Address registers used as pointers
//
//  Note: Please keep the register order as is since they are used in compressed instructions
//
#define	    PPREDBUF_Y		a0.4	// Pointer to predicted Y picture
#define	    PPREDBUF_Y1		a0.5	// Pointer to predicted Y picture for extended instruction

#define	    PPREDBUF_UV		a0.4	// Pointer to predicted U/V picture
#define	    PPREDBUF_UV1	a0.5	// Pointer to predicted U/V picture for extended instruction

#define	    PDECBUF		a0.4	// Pointer to decoded picture data
#define	    PDECBUF_UD	a0.2	// Pointer to decoded picture data in DWORD unit

//  ----------- R63 is reserved for global variables ------------
//
//  Note: Don't program it with values other than what are defined here. 

#define G_REG		r63

#define RETURN_REG	G_REG.0		// Return pointer for all sub-routine calls (type DWORD)
#define RETURN_REG1	G_REG.1		// Return pointer for second-level calls

#define I_ORIX		G_REG.13	// :uw, H. origin of the macroblock in pixel unit, don't overwrite in-line data
#define I_ORIY		G_REG.14	// :uw, V. origin of the macroblock in pixel unit, don't overwrite in-line data

//  Macros
//
//  Note: For macros that require multiple line expansion, insert "\n" at the end of each line.
//
#define	GRF(reg)	r##reg
#ifdef DEV_ILK
#define END_THREAD			send (8) NULLREG MSGHDR r0:ud TS_EOT	EOTMSGDSC
#else
#define END_THREAD			send (8) NULLREG MSGHDR r0:ud EOTMSGDSC
#endif	// DEV_ILK

#define CALL(subFunc, skipInst)	add (1) RETURN_REG<1>:ud   ip:ud	(1+skipInst)*INST_SIZE \n\
				jmpi (1) subFunc

#define CALL_1(subFunc, skipInst)	add (1) RETURN_REG1<1>:ud   ip:ud	(1+skipInst)*INST_SIZE \n\
				jmpi (1) subFunc

#define	RETURN		mov (1)	ip:ud	RETURN_REG<0;1,0>:ud		// Return to calling module
#define	RETURN_1	mov (1)	ip:ud	RETURN_REG1<0;1,0>:ud		// Return to second-level calling module
																// To support iterative calling
#ifdef SW_SCOREBOARD    

#ifdef DEV_CTG_A
  #define LEADING_THREAD	1		// For CTG A, no SRT is needed. Only PRT is necessary
#else
  #define LEADING_THREAD	0		// For CTG B0 and beyond, PRT doesn't take into debug count
  #define DOUBLE_SB					// Scoreboard size needs to be doubled
#endif

#ifdef	DOUBLE_SB					// Scoreboard size needs to be doubled
  #define SB_MASK		0x1ff		// Scoreboard wrap-around mask (for 512 entries)
#else
  #define SB_MASK		0xff		// Scoreboard wrap-around mask (for 256 entries)
#endif	// defined(DOUBLE_SB)

// Scoreboard related definitions

#define	TEMPX		r50
#define	TEMPY		r51
#define	DELTA		r52

#define M05_STORE	r0.13		// :uw, reuse r0.6:ud upper-word to store M0.5 header information for scoreboard


#endif	// SW_SCOREBOARD

// End of header.inc

#endif	// !defined(__HEADER__)

