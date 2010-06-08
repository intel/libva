/*
 * Weighted prediction of luminance and chrominance
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Kernel name: WeightedPred.asm
//
// Weighted prediction of luminance and chrominance
//


//#if !defined(__WeightedPred__)		// Make sure this is only included once
//#define __WeightedPred__


	and.z.f0.0 (1) gWEIGHTFLAG:w	gWPREDFLAG:ub					nWBIDIR_MASK:w
	cmp.e.f0.1 (1) null:w			gPREDFLAG:w						2:w
	(-f0.0)	jmpi INTERLABEL(WeightedPred)
	(f0.1) jmpi INTERLABEL(DefaultWeightedPred_BiPred)
	
INTERLABEL(DefaultWeightedPred_UniPred):

	cmp.e.f0.0 (1) null:w			gPREDFLAG:w						0:w
	(f0.0) jmpi INTERLABEL(Return_WeightedPred)

	// luma
	mov (32)	gubYPRED(0)<2>		gubINTPY1(0)	{Compr}
	mov (32)	gubYPRED(2)<2>		gubINTPY1(2)	{Compr}

#ifndef MONO
	// chroma	
	mov (32)	gubCPRED(0)<2>		gubINTPC1(0)	{Compr}
#endif

	jmpi INTERLABEL(Return_WeightedPred)
	
INTERLABEL(DefaultWeightedPred_BiPred):

	// luma
	avg.sat (32) gubYPRED(0)<2>		gubINTPY0(0)					gubINTPY1(0)	{Compr}
	avg.sat (32) gubYPRED(2)<2>		gubINTPY0(2)					gubINTPY1(2)	{Compr}
	
#ifndef MONO
	// chroma
	avg.sat (32) gubCPRED(0)<2>		gubINTPC0(0)					gubINTPC1(0)	{Compr}
#endif
	
	jmpi INTERLABEL(Return_WeightedPred)
	
INTERLABEL(WeightedPred):
	cmp.e.f0.1 (1) null:w			gWEIGHTFLAG:w					0x80:w
	(-f0.1) jmpi INTERLABEL(WeightedPred_Explicit)
	
	cmp.e.f0.0 (1) null:w			gPREDFLAG:w						2:w
	(-f0.0) jmpi INTERLABEL(DefaultWeightedPred_UniPred)

	mov (2)		gYADD<1>:w			32:w								{NoDDClr}	
	mov (2)		gYSHIFT<1>:w		6:w									{NoDDChk}
	mov (4)		gOFFSET<1>:w		0:w
	mov (8)		gWT0<2>:w			r[pWGT,0]<0;2,1>:w
	
	jmpi INTERLABEL(WeightedPred_LOOP)
	
	// Explicit Prediction
INTERLABEL(WeightedPred_Explicit):
	
	// WA for weighted prediction - 2007/09/06	
#ifdef	SW_W_128		// CTG SW WA
	cmp.e.f0.1 (8) null:ud			r[pWGT,0]<8;8,1>:uw				gudW128(0)<0;1,0>
#else					// ILK HW solution
	and.ne.f0.1 (8) null:uw			r[pWGT,12]<0;1,0>:ub			0x88848421:v	// Expand W=128 flag to all components. 2 MSB are don't care
#endif	
	asr.nz.f0.0 (2)	gBIPRED<1>:w	gPREDFLAG<0;1,0>:w				1:w
	asr (1)		gWEIGHTFLAG:w		gWEIGHTFLAG:w					6:w	
	(-f0.0) mov (2)	gPREDFLAG1<1>:w	gPREDFLAG<0;1,0>:w								
	(f0.0) mov (2)	gPREDFLAG0<1>:ud 0x00010001:ud
	(-f0.0) add (2) gPREDFLAG0<1>:w	-gPREDFLAG1<2;2,1>:w			1:w
	
	// WA for weighted prediction - 2007/09/06	
	(f0.1) mov (8)	gWT0<1>:ud		0x00000080:ud
	(-f0.1) mov (8) gWT0<2>:w		r[pWGT,0]<16;8,2>:b
	(-f0.1) mov (8) gO0<2>:w		r[pWGT,1]<16;8,2>:b
	mul (16)		gWT0<1>:w		gWT0<16;16,1>:w					gPREDFLAG0<0;4,1>:w

	// Compute addition
	cmp.e.f0.1 (2) null<1>:w		gYWDENOM<2;2,1>:ub				0:w
	(-f0.1) shl (2) gW0<1>:w		gWEIGHTFLAG<0;1,0>:w			gYWDENOM<2;2,1>:ub
	(f0.1) mov (2) gW0<1>:w			0:w
	(-f0.1) asr (2) gW0<1>:w		gW0<2;2,1>:w					1:w
	shl (2)		gYADD<1>:w			gW0<2;2,1>:w					gBIPRED<0;1,0>:w
	(f0.1) add (2)	gYADD<1>:w		gYADD<2;2,1>:w					gBIPRED<0;1,0>:w
	
	// Compute shift
	add (2)		gYSHIFT<1>:w		gYWDENOM<2;2,1>:ub				gBIPRED<0;1,0>:w
	
	// Compute offset
	add (4)		acc0<1>:w			gO0<16;4,4>:w					gO1<16;4,4>:w
	add (4)		acc0<1>:w			acc0<4;4,1>:w					gBIPRED<0;1,0>:w
	asr (4)		gOFFSET<1>:w		acc0<4;4,1>:w					gBIPRED<0;1,0>:w

INTERLABEL(WeightedPred_LOOP):	
	// luma
	$for(0;<4;2) {	
	mul (16)	acc0<1>:w			gubINTPY0(%1)					gWT0<0;1,0>:w
	mul (16)	acc1<1>:w			gubINTPY0(%1+1)					gWT0<0;1,0>:w
	mac (16)	acc0<1>:w			gubINTPY1(%1)					gWT1<0;1,0>:w
	mac (16)	acc1<1>:w			gubINTPY1(%1+1)					gWT1<0;1,0>:w
	add (16)	acc0<1>:w			acc0<16;16,1>:w					gYADD:w
	add (16)	acc1<1>:w			acc1<16;16,1>:w					gYADD:w
	// Accumulator cannot be used as destination for ASR
	asr (16)	gwINTERIM_BUF3(0)<1> acc0<16;16,1>:w				gYSHIFT:w
	asr (16)	gwINTERIM_BUF3(1)<1> acc1<16;16,1>:w				gYSHIFT:w
	add.sat (16) gubYPRED(%1)<2>	gwINTERIM_BUF3(0)				gOFFSET:w
	add.sat (16) gubYPRED(%1+1)<2>	gwINTERIM_BUF3(1)				gOFFSET:w
	}	

#ifndef MONO
	// chroma
	mul (16)	acc0<1>:w			gubINTPC0(0)					gUW0<0;2,4>:w
	mul (16)	acc1<1>:w			gubINTPC0(1)					gUW0<0;2,4>:w
	mac (16)	acc0<1>:w			gubINTPC1(0)					gUW1<0;2,4>:w
	mac (16)	acc1<1>:w			gubINTPC1(1)					gUW1<0;2,4>:w
	add (16)	acc0<1>:w			acc0<16;16,1>:w					gCADD:w
	add (16)	acc1<1>:w			acc1<16;16,1>:w					gCADD:w
	// Accumulator cannot be used as destination for ASR
	asr (16)	gwINTERIM_BUF3(0)<1> acc0<16;16,1>:w				gCSHIFT:w
	asr (16)	gwINTERIM_BUF3(1)<1> acc1<16;16,1>:w				gCSHIFT:w
	add.sat (16) gubCPRED(0)<2>		gwINTERIM_BUF3(0)				gUOFFSET<0;2,1>:w
	add.sat (16) gubCPRED(1)<2>		gwINTERIM_BUF3(1)				gUOFFSET<0;2,1>:w
#endif


INTERLABEL(Return_WeightedPred):

        
//#endif	// !defined(__WeightedPred__)
