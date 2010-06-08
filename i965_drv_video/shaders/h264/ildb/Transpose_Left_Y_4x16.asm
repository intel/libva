/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
//	Transpose left MB 4x16 to 16x4
//  Assume source is LEFT_TEMP_B, and detination is PREV_MB_YB


//	Input received from dport:
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|73 72 71 70 63 62 61 60 53 52 51 50 43 42 41 40 33 32 31 30 23 22 21 20 13 12 11 10 03 02 01 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|f3 f2 f1 f0 e3 e2 e1 e0 d3 d2 d1 d0 c3 c2 c1 c0 b3 b2 b1 b0 a3 a2 a1 a0 93 92 91 90 83 82 81 80|
//	+-----------------------+-----------------------+-----------------------+-----------------------+

//	Output of transpose:		<1>	<= <32;8,4>
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|f1 e1 d1 c1 b1 a1 91 81 71 61 51 41 31 21 11 01 f0 e0 d0 c0 b0 a0 90 80 70 60 50 40 30 20 10 00|
//	+-----------------------+-----------------------+-----------------------+-----------------------+
//	|f3 e3 d3 c3 b3 a3 93 83 73 63 53 43 33 23 13 03 f2 e2 d2 c2 b2 a2 92 82 72 62 52 42 32 22 12 02|
//	+-----------------------+-----------------------+-----------------------+-----------------------+

	// Transpose the data, also occupy 2 GRFs
	mov (16)	PREV_MB_YB(0)<1>			LEFT_TEMP_B(0, 0)<32;8,4>		{ NoDDClr }
	mov (16)	PREV_MB_YB(0, 16)<1>		LEFT_TEMP_B(0, 1)<32;8,4>		{ NoDDChk }
	mov (16)	PREV_MB_YB(1)<1>			LEFT_TEMP_B(0, 2)<32;8,4>		{ NoDDClr }
	mov (16)	PREV_MB_YB(1, 16)<1>		LEFT_TEMP_B(0, 3)<32;8,4>		{ NoDDChk }
