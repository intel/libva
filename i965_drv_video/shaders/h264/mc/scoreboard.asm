/*
 * Dependency control scoreboard kernel
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: scoreboard.asm
//
// Dependency control scoreboard kernel
//
//  $Revision: 16 $
//  $Date: 10/18/06 4:10p $
//

// ----------------------------------------------------
//  Main: scoreboard
// ----------------------------------------------------
// ----------------------------------------------------
//  Scoreboard structure
// ----------------------------------------------------
//
//	1 DWORD per thread
//
//	Bit 31:	"Checking" thread, i.e. an intra MB that sends "check dependency" message
//	Bit 30: "Completed" thread. This bit set by an "update" message from intra/inter MB.
//	Bits 29:28:	Must set to 0
//	Bits 27:24:	EUID
//	Bits 23:18: Reserved
//	Bits 17:16: TID
//	Bits 15:8:	X offset of current MB
//	Bits 15:5:	Reserved
//	Bits 4:0: 5 bits of available neighbor MB flags

.kernel scoreboard
SCOREBOARD:

#ifdef _DEBUG
// WA for FULSIM so we'll know which kernel is being debugged
mov (1) acc0:ud 0xf0aa55a5:ud
#endif

#include "header.inc"
#include "scoreboard_header.inc"

//
//  Now, begin source code....
//

.code

#ifdef	AS_ENABLED
	and.z.f0.1	(1)	NULLREG	r0.2<0;1,0>:ud	TH_RES	// Is this a restarted thread previously interrupted?
	(f0.1) jmpi	(1)	Scoreboard_Init

	#include "scoreboard_restore_AS.asm"

	jmpi (1)	Scoreboard_OpenGW
Scoreboard_Init:
#endif	// End AS_ENABLED

// Scoreboard must be initialized to 0xc000ffff, meaning all "completed"
// And it also avoids message mis-handling for the first MB
    $for(0; <32; 2) {
	mov (16)	CMD_SB(%1)<1>	0xc000ffff:ud {Compr}
	}
#ifdef	DOUBLE_SB					// Scoreboard size needs to be doubled
    $for(32; <64; 2) {
	mov (16)	CMD_SB(%1)<1>	0xc000ffff:ud {Compr}
	}
#endif	// DOUBLE_SB

//----------------------------------------------------------
//	Open message gateway for the scoreboard thread
//
//	RegBase = r4 (0x04)
//	Gateway Size = 64 GRF registers (0x6)
//	Dispatch ID = r0.20:ub
//	Scoreboard Thread Key = 0
//----------------------------------------------------------
Scoreboard_OpenGW:
    mov (8)	MSGHDRY0<1>:ud	0x00000000:ud			// Initialize message header payload with 0

	// Send a message with register base RegBase=0x04(r4) and Gateway size = 0x6 = 64 GRF reg and Key = 0
	// 000 00000100 00000 00000 110 00000000 ==> 0000 0000 1000 0000 0000 0110 0000 0000
#ifdef	AS_ENABLED
	add (1) MSGHDRY0.5<1>:ud r0.20:ub	0x00800700:ud	// Allocate 128 GRFs for message gateway - for SIP to send notification MSG
#else
  #ifdef	DOUBLE_SB
	add (1) MSGHDRY0.5<1>:ud r0.20:ub	0x00800600:ud	// 64 GRF's for CTG-B
  #else
	add (1) MSGHDRY0.5<1>:ud r0.20:ub	0x00800500:ud	// 32 GRF's for CTG-A
  #endif	// DOUBLE_SB
#endif
	send (8)	NULLREG  MSGHDRY0	null:ud    MSG_GW	OGWMSGDSC

//------------------------------------------------------------------------
//	Send Thread Spawning Message to start dispatching macroblock threads
//
//------------------------------------------------------------------------
#ifdef	AS_ENABLED
	mov (8)	acc0<1>:ud	CMD_SB(31)<8;8,1>			// Ensure scoreboard data have been completely restored
#endif	// End AS_ENABLED
    mov (8)	MSGHDRY1<1>:ud		r0<8;8,1>:ud		// Initialize message header payload with R0
    mov (1)	MSGHDRY1.4<1>:ud	0x00000400:ud		// Dispatch URB length = 1

	send (8)	NULLREG  MSGHDRY1	null:ud    TS	TSMSGDSC

    mov (8)	MSGHDRY0<1>:ud		0x00000000:ud		// Initialize message header payload with 0

//------------------------------------------------------------------------
//	Scoreboard control data initialization
//------------------------------------------------------------------------
#ifdef	AS_ENABLED
	or	(1)	cr0.1:ud	cr0.1:ud	AS_INT_EN		// Enable interrupt
	(f0.1) jmpi	(1)	Scoreboard_State_Init	// Jump if not restarted thread

	// Restore scoreboard kernel control data to r1 - r3
    mov (1)	m4.1:ud	64:ud				// Starting r1
    mov (1)	m4.2:ud	0x0002001f:ud		// for 3 registers
    send (8)	r1.0<1>:ud	m4	null:ud	DWBRMSGDSC_SC+0x00030000+AS_SAVE	// Restore r1 - r3
	mov	(8)	a0.0<1>:uw	AR_SAVE<8;8,1>:uw				// Restore all address registers

// Check whether all MBs have been decoded
	cmp.e.f0.0 (1)	NULLREG	TotalMB<0;1,0>:w	0:w	// Set "Last MB" flag
	(-f0.0) jmpi (1)	Before_First_MB
    END_THREAD

// Check whether it is before the first MB
Before_First_MB:
	cmp.e.f0.0 (1)	NULLREG	AVAILFLAGD<1>:ud	0x08020401:ud	// in ACBD order
	(f0.0) jmpi (1)	Wavefront_Walk

Scoreboard_State_Init:
#endif	// End AS_ENABLED
	mov (2) WFLen_B<2>:w		HEIGHTINMB_1<0;1,0>:w
	mov (1)	AVAILFLAGD<1>:ud	0x08020401:ud	// in ACBD order
	mov	(1) CASE00PTR<1>:ud	Notify_MSG_IP-No_Message_IP:ud		// Inter kernel starts
	mov	(1) CASE10PTR<1>:ud	Dependency_Check_IP-No_Message_IP:ud	// Intra kernel starts
#ifdef	AS_ENABLED
	mov	(1) CASE11PTR<1>:ud	0:ud		// No message
#else
	mov	(1) CASE11PTR<1>:ud	MB_Loop_IP-No_Message_IP:ud		// No message
#endif	// End AS_ENABLED
	mov	(1) StartXD<1>:ud	0:ud
	mov	(1) NewWFOffsetD<1>:ud	0x01ffff00:ud

	mov (4) WFStart(0)<1>	0xffff:w
	mov (1) WFStart(0)<1>	0:w

	mov	(8)	a0.0<1>:uw	0x0:uw						// Initialize all pointers to 0

//------------------------------------------------------------------------
//	Scoreboard message handling loop
//------------------------------------------------------------------------
//
Scoreboard_Loop:
	// Calculate current wavefront length
	add.ge.f0.1 (16)	acc0<1>:w	StartX<0;1,0>:w	0:w		// Used for x>2*y check
	mac.g.f0.0 (16)	NULLREGW	WFLenY<0;1,0>:w	-2:w		// X - 2*Y > 0 ??
	(f0.0) mov (1)	WFLen<1>:w	WFLenY<0;1,0>:w				// Use smaller vertical wavefront length
	(-f0.0) asr.sat (1)	WFLen<1>:uw	StartX<0;1,0>:w	1:w		// Horizontal wavefront length is smaller

	// Initialize 5-MB group
#ifdef ONE_MB_WA
	mov (2) MBINDEX(0)<1>		WFStart(0)<2;2,1>
	(f0.1) add (4) MBINDEX(0,2)<1>		WFStart(0,1)<4;4,1>	-1:w
	(-f0.1) add (4) MBINDEX(0,2)<1>		WFStart(0,0)<4;4,1>	-1:w
	(-f0.1) mov (1) StartX<1>:w		0:w					// WA for 1-MB wide pictures
#else
	mov (2) MBINDEX(0)<1>		WFStart(0)<2;2,1>			{NoDDClr}
	add (4) MBINDEX(0,2)<1>		WFStart(0,1)<4;4,1>	-1:w	{NoDDChk}
#endif

	// Update WFStart
	mov (8)	acc0<1>:w	WFStart(0)<0;1,0>					// Move WFStart(0) to acc0 to remove dependency later
	mov (4)	WFStart(0,1)<1>	WFStart(0)<4;4,1>	{NoDDClr}	// Shift WFStart(0:2) to WFStart(1:3)
	add (1)	WFStart(0)<1>	acc0.0<0;1,0>:w		WFLen<0;1,0>:w	{NoDDChk}	// WFStart(0) = WFStart(0) + WFLen

	mul (8)	MBINDEX(0)<1>	MBINDEX(0)<8;8,1>	4:w	// Adjust MB order # to be DWORD aligned
	and (1)	DEPPTR<1>:uw	acc0<0;1,0>:w	SB_MASK*4:uw {NoDDClr}	// Wrap around scoreboard entries for current MB
	and (4)	DEPPTRL<1>:uw	acc0.1<4;4,1>:w	SB_MASK*4:uw {NoDDChk}	// Wrap around scoreboard entries for neighbor MBs

Wavefront_Walk:
	wait	n0:ud
//	Check for combined "checking" or "completed" threads in forwarded message
//	2 MSB of scoreboard message indicate:
//	0b00 = "inter start" message
//	0b10 = "intra start" message
//	0b11 = "No Message" or "inter complete" message
//	0b01 = Reserved (should never occur)
//
MB_Loop:
	shr	(1)	PMSGSEL<1>:uw	r[CMDPTR,CMD_SB_REG_OFF*GRFWIB+2]<0;1,0>:uw	12:w					// DWORD aligned pointer to message handler
	and.nz.f0.1 (4) NULLREG	r[CMDPTR,CMD_SB_REG_OFF*GRFWIB]<0;1,0>:ub	AVAILFLAG<4;4,1>:ub		// f0.1 4 LSB will have the available flags in ACBD order
	mov (1) MSGHDRY0.4<1>:ud	r[CMDPTR,CMD_SB_REG_OFF*GRFWIB]<0;1,0>:ud		// Copy MB thread info from scoreboard
	jmpi (1)	r[PMSGSEL, INLINE_REG_OFF*GRFWIB+16]<0;1,0>:d

//	Now determine whether this is "inter done" or "no message"
//	through checking debug_counter
//
No_Message:
#ifdef	AS_ENABLED
	cmp.z.f0.1 (1)	NULLREG	n0:ud	0	// Are all messages handled?
	and.z.f0.0 (1)	NULLREG	cr0.1:ud	AS_INT	// Poll interrupt bit
	(-f0.1) jmpi (1)	MB_Loop			// Continue polling the remaining message from current thread

// All messages have been handled
	(f0.0) jmpi (1) Wavefront_Walk		// No interrupt occurs. Wait for next one

// Interrupt has been detected
// Save all contents and terminate the scoreboard
//
	#include "scoreboard_save_AS.asm"

	// Save scoreboard control data as well
	//
	mov (8) AR_SAVE<1>:uw	a0.0<8;8,1>:uw		// All address registers needs to be saved
    mov (1)	MSGHDR.1:ud		64:ud
    mov (1)	MSGHDR.2:ud		0x0002001f:ud	// for 3 registers
	$for(0; <3; 1) {
	mov (8)	MSGPAYLOADD(%1)<1>	CMD_SB(%1-3)REGION(8,1)
	}
    send (8)	NULLREG	MSGHDR	null:ud	DWBWMSGDSC+0x00300000+AS_SAVE	// Save r1 - r3

	send (8) NULLREG MSGHDR r0:ud EOTMSGDSC+TH_INT	// Terminate with "Thread Interrupted" bit set
#endif	// End AS_ENABLED

Dependency_Check:
//	Current thread is "checking" but not "completed" (0b10 case).
//	Check for dependency clear using all availability bits
//
	(f0.1) and.z.f0.1 (4)	NULLREG	r[DEPPTRL,CMD_SB_REG_OFF*GRFWIB+3]<1,0>:ub	DONEFLAG:uw	// f0.1 4 LSB contains dependency clear
	(f0.1.any4h) jmpi (1)	Dependency_Check		// Dependency not clear, keep polling..

//	"Checking" thread and dependency cleared, send a message to let the thread go
//
Notify_MSG:
	send (8)	NULLREG  MSGHDRY0	null:ud    MSG_GW	FWDMSGDSC+NOTIFYMSG

//	Current macroblock has been serviced. Update to next macroblock in special zig-zag order
//
Update_CurMB:
#if 0
	add.ge.f0.0 (1)	WFLen<1>:w	WFLen<0;1,0>:w	-1:w 			// Set "End of wavefront" flag
	add (1)	TotalMB<1>:w	TotalMB<0;1,0>:w	-1:w 			// Decrement "TotalMB"
#else
	add.ge.f0.0 (2)	TotalMB<2>:w	TotalMB<4;2,2>:w	-1:w 	// Set "End of wavefront" flag and decrement "TotalMB"
#endif
	add (8)	MBINDEX(0)<1>	MBINDEX(0)<8;8,1>	4:w				// Increment MB indices
	and (1)	DEPPTR<1>:uw	acc0<0;1,0>:w	SB_MASK*4:uw {NoDDClr}	// Wrap around 256 scoreboard entries for current MB
	and (4)	DEPPTRL<1>:uw	acc0.1<4;4,1>:w	SB_MASK*4:uw {NoDDChk}	// Wrap around 256 scoreboard entries for neighbor MBs
	cmp.e.f0.1 (16)	NULLREGW  StartX<0;1,0>:uw	WIDTHINMB_1<0;1,0>:uw	// Set "on picture right boundary" flag
#if 0
	(f0.0) jmpi (1) Wavefront_Walk			// Continue wavefront walking
#else
	(f0.0.all2h) jmpi (1) Wavefront_Walk	// Continue wavefront walking
#endif

//	Start new wavefront
//
	cmp.e.f0.0 (1)	NULLREG	TotalMB<0;1,0>:w		0:w	// Set "Last MB" flag
	(f0.1) add (4)	WFLen<1>:w	WFLen<4;4,1>:w		NewWFOffset<4;4,1>:b
	(f0.1) add (4)	WFStart(0)<1>	WFStart(0)<4;4,1>	1:w
	(-f0.1) add (1)	StartX<1>:w		StartX<0;1,0>:w	1:w		// Move to right MB
	(-f0.1) add (1)	WFStart(0)<1>	WFStart(0)<0;1,0>	1:w

	(-f0.0) jmpi (1)	Scoreboard_Loop				// Not last MB, start new wavefront walking

// All MBs have decoded. Terminate the thread now
//
    END_THREAD

#if !defined(COMBINED_KERNEL)		// For standalone kernel only
.end_code

.end_kernel
#endif

// End of scoreboard
