/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Module name: load_ILDB_Cntrl_Data_22DW.asm
//
// ********** Apple only module **********
//
// This module loads AVC ILDB 22DW control data for one MB for CLN.
// The reduced control data set is for progressive picture ONLY.
//
// Control data memory layout for each MB is 8x11 = 88 bytes.  
// It ocuppies 3 GRFs after reading in.
//
//----------------------------------------------------------------
//  Symbols need to be defined before including this module
//
//	Source region in :ud
//	CNTRL_DATA_D:	CNTRL_DATA_D Base=rxx ElementSize=4 SrcRegion=REGION(8,1) Type=ud			// 3 GRFs
//
//	Binding table index: 
//	BI_CNTRL_DATA:	Binding table index of control data surface
//
//----------------------------------------------------------------

    mul (1)	MSGSRC.0<1>:ud	ORIX:uw			8:uw		{ NoDDClr }				// Block origin X
    mul (1)	MSGSRC.1<1>:ud	ORIY:uw			11:uw		{ NoDDClr, NoDDChk }	// Block origin Y
    mov (1)	MSGSRC.2<1>:ud	0x000A0007:ud				{ NoDDChk }				// Block width and height (8x11=88 bytes)

    send (8) CNTRL_DATA_D(0)<1>	MSGHDRY	MSGSRC<8;8,1>:ud	DAPREAD	RESP_LEN(3)+DWBRMSGDSC_SC+BI_CNTRL_DATA	// Receive 3 GRFs
	
// End of load_ILDB_Cntrl_Data_22DW.asm
