/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//	Transpose left MB 2x8 to 8x2
//  Assume source is LEFT_TEMP_W, and detination is PREV_MB_UW

//	Input from dport for transpose:	
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|71 71 70 70 61 61 60 60 51 51 50 50 41 41 40 40 31 31 30 30 21 21 20 20 11 11 10 10 01 01 00 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//
//	Output of transpose:	<1>	<=== <16;8,2>:w
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|71 71 61 61 51 51 41 41 31 31 21 21 11 11 01 01 70 70 60 60 50 50 40 40 30 30 20 20 10 10 00 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+

//	mov (8)	PREV_MB_UW(0,0)<1>		BUF_W(0,0)<16;8,2>		{ NoDDClr }
//	mov (8)	PREV_MB_UW(0,8)<1>		BUF_W(0,1)<16;8,2>		{ NoDDChk }
	
//	mov (8)	PREV_MB_UW(0,0)<1>		LEFT_TEMP_W(0,0)<16;8,2>		{ NoDDClr }
//	mov (8)	PREV_MB_UW(0,8)<1>		LEFT_TEMP_W(0,1)<16;8,2>		{ NoDDChk }

	mov (16)	PREV_MB_UW(0,0)<1>		LEFT_TEMP_W(0,0)<1;8,2>
