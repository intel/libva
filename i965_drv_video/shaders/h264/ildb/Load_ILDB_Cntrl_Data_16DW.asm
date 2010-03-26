/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: load_ILDB_Cntrl_Data_16DW.asm
//
// This module loads AVC ILDB 64DW control data for one MB CTG. 
// Dataport expands from 16DW to 64DW.
//
//----------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Source region in :ud
//	CNTRL_DATA_D:	CNTRL_DATA_D Base=rxx ElementSize=4 SrcRegion=REGION(8,1) Type=ud			// 8 GRFs
//
//	Binding table index: 
//	BI_CNTRL_DATA:	Binding table index of control data surface
//
//----------------------------------------------------------------

// On CTG, MB control data in memory occupy 16 DWs. Data port returns 8 GRFs with expanded control data.

#if defined(_MBAFF) 
	// We need to get control data offset for the bottom MB in mbaff mode.
	// That is, get f0.1=1 if MbaffFlag==1 && BotFieldFlag==1
//	and (1)	CTemp1_W:uw 		BitFields:uw  	MbaffFlag+BotFieldFlag:uw	// Mute all other bits
//	cmp.e.f0.1 (1) NULLREGW 	CTemp1_W:uw  	MbaffFlag+BotFieldFlag:uw	// Check mbaff and bot flags
		
	and.ne.f0.1 (1)	NULLREGW 	BitFields:uw  	BotFieldFlag:uw

	// Global offset
	mov (1)	MSGSRC.2:ud		CntrlDataOffsetY:ud
		
	(f0.1) add (1) MSGSRC.2:ud		MSGSRC.2:ud		64:w	// +64 to the next MB control data (bot MB)
#endif

    send (8) CNTRL_DATA_D(0)<1>	MSGHDRY	MSGSRC<8;8,1>:ud	DAPREAD	RESP_LEN(8)+ILDBRMSGDSC+BI_CNTRL_DATA	// Receive 8 GRFs

// End of load_ILDB_Cntrl_Data_16DW.asm



// AVC ILDB control data message header format

//DWord	Bit	Description
//M0.7	31:0	Debug 
//M0.6	31:0	Debug
//M0.5	31:8	Ignored
//		7:0		Dispatch ID. // This ID is assigned by the fixed function unit and is a unique identifier for the thread.  It is used to free up resources used by the thread upon thread completion.
//M0.4	31:0	Ignored
//M0.3	31:0	Ignored
//M0.2	31:0	Global Offset. Specifies the global byte offset into the buffer.
				//	This offset must be OWord aligned (bits 3:0 MBZ) Format = U32 Range = [0,FFFFFFF0h]
//M0.1	31:0	Ignored
//M0.0	31:0	Ignored



