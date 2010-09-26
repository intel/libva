/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */


.declare SRC_B		Base=REG(r,10)	ElementSize=2 SrcRegion=REGION(8,1) DstRegion=<1> Type=uw
.declare SRC_G		Base=REG(r,18)	ElementSize=2 SrcRegion=REGION(8,1) DstRegion=<1> Type=uw
.declare SRC_R		Base=REG(r,26)	ElementSize=2 SrcRegion=REGION(8,1) DstRegion=<1> Type=uw
.declare SRC_A		Base=REG(r,34)	ElementSize=2 SrcRegion=REGION(8,1) DstRegion=<1> Type=uw

#define DEST_ARGB		ubBOT_ARGB

#undef 	nSRC_REGION
#define nSRC_REGION		nREGION_2


//Pack directly to mrf as optimization - vK

$for(0, 0; <8; 1, 2) {
//	mov	(16) 	DEST_ARGB(%2,0)<4>		SRC_B(%1) 					{ Compr, NoDDClr }			// 16 B
//	mov	(16) 	DEST_ARGB(%2,1)<4>		SRC_G(%1)					{ Compr, NoDDClr, NoDDChk }	// 16 G
//	mov	(16) 	DEST_ARGB(%2,2)<4>		SRC_R(%1)					{ Compr, NoDDClr, NoDDChk }	// 16 R	//these 2 inst can be merged - vK
//	mov	(16) 	DEST_ARGB(%2,3)<4>		SRC_A(%1)					{ Compr, NoDDChk }			//DEST_RGB_FORMAT<0;1,0>:ub	{ Compr, NoDDChk }			// 16 A

	mov	(8) 	DEST_ARGB(%2,  0)<4>		SRC_B(%1) 					{ NoDDClr }				// 8 B
	mov	(8) 	DEST_ARGB(%2,  1)<4>		SRC_G(%1)					{ NoDDClr, NoDDChk }	// 8 G
	mov	(8) 	DEST_ARGB(%2,  2)<4>		SRC_R(%1)					{ NoDDClr, NoDDChk }	// 8 R
	mov	(8) 	DEST_ARGB(%2,  3)<4>		SRC_A(%1)					{ NoDDChk }				// 8 A

	mov	(8) 	DEST_ARGB(%2+1,0)<4>		SRC_B(%1,8) 				{ NoDDClr }				// 8 B
	mov	(8) 	DEST_ARGB(%2+1,1)<4>		SRC_G(%1,8)					{ NoDDClr, NoDDChk }	// 8 G
	mov	(8) 	DEST_ARGB(%2+1,2)<4>		SRC_R(%1,8)					{ NoDDClr, NoDDChk }	// 8 R
	mov	(8) 	DEST_ARGB(%2+1,3)<4>		SRC_A(%1,8)					{ NoDDChk }				// 8 A
}
