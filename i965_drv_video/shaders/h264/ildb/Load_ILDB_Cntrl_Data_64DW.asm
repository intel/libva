/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: load_ILDB_Cntrl_Data_64DW.asm
//
// This module loads AVC ILDB 64DW control data for one MB for CLN.  
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

// On CLN, MB control data in memory occupy 64 DWs.

#if defined(_MBAFF) 
	// We need to get control data offset for the bottom MB in mbaff mode.
	// That is, set f0.1=1 if MbaffFlag==1 && BotFieldFlag==1
	and (1)	acc0.0:uw 		BitFields:uw  	MbaffFlag+BotFieldFlag:uw	// Mute all other bits
	cmp.e.f0.1 (1) NULLREGW 	acc0.0:uw  	MbaffFlag+BotFieldFlag:uw	// Check mbaff and bot flags
#endif		// CTemp1_W

    mov (2)	MSGSRC.0<1>:ud	ORIX_CUR<2;2,1>:uw			{ NoDDClr }				// Block origin X,Y
    mov (1)	MSGSRC.2<1>:ud	0x000F000F:ud				{ NoDDChk }				// Block width and height (16x16=256 bytes)

#if defined(_MBAFF) 
	(f0.1) add (1) MSGSRC.1:ud	MSGSRC.1:ud		16:w	// +16 to the bottom MB control data (bot MB)
#endif

    send (8) CNTRL_DATA_D(0)<1>	MSGHDRY	MSGSRC<8;8,1>:ud	DAPREAD	RESP_LEN(8)+DWBRMSGDSC_SC+BI_CNTRL_DATA	// Receive 8 GRFs
	
// End of load_ILDB_Cntrl_Data_64DW.asm
