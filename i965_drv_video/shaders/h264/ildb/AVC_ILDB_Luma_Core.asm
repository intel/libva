/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
#if !defined(__AVC_ILDB_LUMA_CORE__)	// Make sure this file is only included once
#define __AVC_ILDB_LUMA_CORE__

////////// AVC ILDB Luma Core /////////////////////////////////////////////////////////////////////////////////
//
//	This core performs AVC LUMA ILDB filtering on one horizontal edge (16 pixels) of a MB.  
//	If data is transposed, it can also de-block a vertical edge.
//
//	Bafore calling this subroutine, caller needs to set the following parameters.
//
//	- EdgeCntlMap1				//	Edge control map A
//	- EdgeCntlMap2				//	Edge control map B
//	- P_AddrReg					//	Src and dest address register for P pixels
//	- Q_AddrReg					//	Src and dest address register for Q pixels 	
//	- alpha						//  alpha corresponding to the edge to be filtered
//	- beta						//  beta corresponding to the edge to be filtered
//	- tc0						// 	tc0  corresponding to the edge to be filtered
//
//
//	+----+----+----+----+----+----+----+----+
//	| p3 | p2 | P1 | p0 | q0 | q1 | q2 | q3 |
//	+----+----+----+----+----+----+----+----+
//
//	p3 = r[P_AddrReg, 0]<16;16,1>  
//	p2 = r[P_AddrReg, 16]<16;16,1> 
//	p1 = r[P_AddrReg, 32]<16;16,1> 
//	p0 = r[P_AddrReg, 48]<16;16,1> 
// 	q0 = r[Q_AddrReg, 0]<16;16,1>  
//	q1 = r[Q_AddrReg, 16]<16;16,1> 
//	q2 = r[Q_AddrReg, 32]<16;16,1> 
//	q3 = r[Q_AddrReg, 48]<16;16,1> 
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The region is both src and dest
// P0-P3 and Q0-Q3 should be only used if they have not been modified to new values
#undef 	P3
#undef 	P2
#undef 	P1
#undef 	P0
#undef 	Q0
#undef 	Q1
#undef 	Q2
#undef 	Q3
  
#define P3 		r[P_AddrReg,  0]<16;16,1>:ub
#define P2 		r[P_AddrReg, 16]<16;16,1>:ub
#define P1 		r[P_AddrReg, 32]<16;16,1>:ub
#define P0 		r[P_AddrReg, 48]<16;16,1>:ub
#define Q0 		r[Q_AddrReg,  0]<16;16,1>:ub
#define Q1 		r[Q_AddrReg, 16]<16;16,1>:ub
#define Q2 		r[Q_AddrReg, 32]<16;16,1>:ub
#define Q3 		r[Q_AddrReg, 48]<16;16,1>:ub

// New region as dest
#undef 	NewP2
#undef 	NewP1
#undef 	NewP0
#undef 	NewQ0
#undef 	NewQ1
#undef 	NewQ2

#define NewP2 	r[P_AddrReg, 16]<1>:ub
#define NewP1 	r[P_AddrReg, 32]<1>:ub
#define NewP0 	r[P_AddrReg, 48]<1>:ub
#define NewQ0 	r[Q_AddrReg,  0]<1>:ub
#define NewQ1 	r[Q_AddrReg, 16]<1>:ub
#define NewQ2 	r[Q_AddrReg, 32]<1>:ub

// Filter one luma edge
FILTER_Y:

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0x1111:w
#endif
	//---------- Derive filterSampleflag in AVC spec, equition (8-469) ----------
	// bS is in MaskA

	// Src copy of the p3, p2, p1, p0, q0, q1, q2, q3
//	mov (16) p0123_W(0)<1>		r[P_AddrReg]<16;16,1>:uw
//	mov (16) p0123_W(1)<1>		r[P_AddrReg, 32]<16;16,1>:uw
//	mov (16) q0123_W(0)<1>		r[Q_AddrReg]<16;16,1>:uw
//	mov (16) q0123_W(1)<1>		r[Q_AddrReg, 32]<16;16,1>:uw

	mov (2)	f0.0<1>:uw		MaskA<2;2,1>:uw

	add (16) q0_p0(0)<1>		Q0		-P0				// q0-p0
	add (16) TempRow0(0)<1>		P1		-P0				// p1-p0
	add (16) TempRow1(0)<1>		Q1		-Q0				// q1-q0

	// Build FilterSampleFlag
	// abs(q0-p0) < alpha
	(f0.0) cmp.l.f0.0 (16) null:w		(abs)q0_p0(0)			alpha:w
	// abs(p1-p0) < Beta
	(f0.0) cmp.l.f0.0 (16) null:w		(abs)TempRow0(0)		beta:w
	// abs(q1-q0) < Beta
	(f0.0) cmp.l.f0.0 (16) null:w		(abs)TempRow1(0)		beta:w

	//-----------------------------------------------------------------------------------------

    (f0.0)	if	(16)		Y_ENDIF1
		// For channels whose edge control map1 = 1 ---> perform de-blocking

//		mov (1)		f0.1:uw		MaskB:uw	{NoMask}		// Now check for which algorithm to apply

		// (abs)ap = |p2-p0|
		add (16) ap(0)<1>		P2		-P0		// ap = p2-p0
		// (abs)aq = |q2-q0|
		add (16) aq(0)<1>		Q2		-Q0		// aq = q2-q0

		// Make a copy of unmodified p0 and p1 for use in q0'and q1' calculation
		mov (16) p0123_W(1)<1>		r[P_AddrReg, 32]<16;16,1>:uw		{NoMask}

		(f0.1)	if	(16)		Y_ELSE2

			// For channels whose edge control map2 = 1 ---> bS = 4 algorithm

			// Compute q0', q1' and q2'
			//-----------------------------------------------------------------------------
			// bS = 4 Algorithm :			
			//
			// gama = |p0-q0| < ((alpha >> 2) + 2) 
			// deltap = (ap<beta) && gama;  		// deep filter flag
			//	if (deltap) {
			//		p0' = (        p2 +2*p1 +2*p0 +2*q0 + q1 + 4) >> 3; 
			// 		p1' = (        p2 +  p1 +  p0 +  q0      + 2) >> 2;
			// 		p2' = (2*p3 +3*p2 +  p1 +  p0 +  q0      + 4) >> 3;
			//	} else {  
			//		p0' = (            2*p1 +  p0 +  q1      + 2) >> 2;
			//	}
			//-----------------------------------------------------------------------------

			// gama = |p0-q0| < ((alpha >> 2) + 2) = |p0-q0| < alpha2  
			cmp.l.f0.1 (16) null:w	(abs)q0_p0(0)	alpha2:w

			// Common P01 = p0 + p1
			add (16)	P0_plus_P1(0)<1>	P0			P1	

			// Common Q01 = q0 + q1
			add (16)	Q0_plus_Q1(0)<1>	Q0			Q1

//			mov (1)	CTemp1_W:w		f0.1:uw						{NoMask}
			mov (1)	f0.0:uw			f0.1:uw						{NoMask}
	
			// deltap = ((abs)ap < beta) && gama
			(f0.1) cmp.l.f0.1 (16) null:w	(abs)ap(0)		beta<0;1,0>:w							// (abs)ap < beta ?

			// deltaq = ((abs)aq < beta) && gama
			(f0.0) cmp.l.f0.0 (16) null:w	(abs)aq(0)		beta<0;1,0>:w							// (abs)aq < beta ?


//			mov (1)	CTemp1_W:w		f0.0:uw						{NoMask}					// gama = |p0-q0| < ((alpha >> 2) + 2) for each channel	
//			and (1)		f0.1:w		f0.1:uw		CTemp1_W:w		{NoMask}					// deltap = (ap<beta) && gama


			(f0.1)	if	(16)		Y_ELSE3			// for channels its deltap = true

			add (16)	P2_plus_P3(0)<1>	P2		P3
			
			// A =  (p1 + p0) + q0 = P01 + q0
			add (16)	A(0)<1>			P0_plus_P1(0)		Q0							// A =  P01 + q0

			// Now acc0 = A

			// B =  p2 + (p1 + p0 + q0) + 4 = p2 + A + 4
//			add (16)	acc0.0<1>:w		P2				4:w								// p2 + 4 
//			add (16)	BB(0)<1>			acc0.0<16;16,1>:w		A(0)					// B = p2 + A + 4
			add (16)	acc0.0<1>:w		acc0.0<16;16,1>:w		4:w								// p2 + 4 
			add (16)	BB(0)<1>			acc0.0<16;16,1>:w		P2					// B = p2 + A + 4
			
			// Now acc0 = B

			// p2' = (2*p3 +3*p2 + A + 4) >> 3 = (2*(p3+p2) + B) >> 3
//			mov	(16)	acc0.0<1>:w		BB(0)
			mac (16)	acc0.0<1>:w		P2_plus_P3(0)		2:w		
			shr.sat (16) TempRow3B(0)<2>	acc0.0<16;16,1>:w		3:w
			
			// p1' = (p2 + A + 2) >> 2 = (B - 2) >> 2
			add (16)	acc0.0<1>:w		BB(0)			-2:w
			shr.sat (16) TempRow1B(0)<2>	acc0.0<16;16,1>:w		2:w
	
			// p0' = (p2 +2*A + q1 + 4) >> 3 = (B + A + q1) >> 3
			add (16)	acc0.0<1>:w		Q1				A(0)							// B + A
			add (16)	acc0.0<1>:w		acc0.0<16;16,1>:w		BB(0)							// B + A + q1
			shr.sat (16) TempRow0B(0)<2>	acc0.0<16;16,1>:w		3:w								// (B + A + q1) >> 3

			// p2' = (2*p3 +3*p2 + A + 4) >> 3 = (2*(p3+p2) + B) >> 3
//			mov	(16)	acc0.0<1>:w		BB(0)
//			mac (16)	acc0.0<1>:w		P2_plus_P3(0)		2:w		
//			shr.sat (16) TempRow3B(0)<2>	acc0.0<16;16,1>:w		3:w

			mov (16) 	NewP2		TempRow3B(0)						// p2'
			mov (16) 	NewP1		TempRow1B(0)						// p1'			
			mov (16) 	NewP0		TempRow0B(0)						// p0'

Y_ELSE3:
			else (16)		Y_ENDIF3		// for channels its deltap = false

			// p0' = (2*p1 + p0 + q1 + 2) >> 2 =  (p1 + P01 + q1 + 2) >> 2
			add (16)	acc0.0<1>:w		P1			P0_plus_P1(0)			// p1 + P01 (TempRow1(0) = P01)
			add (16)	acc0.0<1>:w		acc0.0<16;16,1>:w	Q1				
			add (16)	acc0.0<1>:w		acc0.0<16;16,1>:w	2:w			// p1 + P01 + q1 + 2

			shr.sat (16) TempRow0B(0)<2>	acc0.0<16;16,1>:w		2:w	// >> 2
			mov (16) 	NewP0		TempRow0B(0)						// p0'

			endif
Y_ENDIF3:
			// Compute q0', q1' and q2'
			//-----------------------------------------------------------------------------
			// bS = 4 Algorithm (cont):			
			//
			//	deltaq = (aq<beta) && gama;  		// deep filter flag
			//	if (deltaq) {
			//		q0' = (        q2 +2*q1 +2*q0 +2*p0 + p1 + 4) >> 3; 
			//		q1' = (        q2 +  q1 +  q0 +  p0      + 2) >> 2;
			//		q2' = (2*q3 +3*q2 +  q1 +  q0 +  p0      + 4) >> 3;
			//	} else {
			//		q0' = (            2*q1 +  q0 +  p1      + 2) >> 2;
			//	}
			
			// deltaq = ((abs)aq < beta) && gama
//			cmp.l.f0.1 (16) null:w	(abs)aq(0)		beta<0;1,0>:w							// (abs)aq < beta ?

			// Common Q01 = q0 + q1
//			add (16)	Q0_plus_Q1(0)<1>	Q0			Q1
			
//			and (1)		f0.1:w		f0.1:uw		CTemp1_W:w		{NoMask}				// deltaq = ((abs)ap < beta) && gama

			(f0.0)	if	(16)		Y_ELSE4			// for channels its deltaq = true
			
			add (16)	Q2_plus_Q3(0)<1>	Q2			Q3

			// A =  (q1 + q0) + p0 = Q01 + p0
			add (16)	A(0)<1>			Q0_plus_Q1(0)		p0(0)							// A =  q1+q0 + p0

			// Acc0 = A

			// B =  q2 + q1 + q0 + p0 + 4 = q2 + A + 4
			add (16)	acc0.0<1>:w		acc0.0<16;16,1>:w		4:w							// q2 + 4 
			add (16)	BB(0)<1>			acc0.0<16;16,1>:w		Q2								// B = q2 + A + 4

			// Acc0 = B
			
			// q2' = (2*q3 +3*q2 + A + 4) >> 3 = (2*(q3+q2) + B) >> 3
//			mov (16)	acc0.0<1>:w		BB(0)	
			mac (16)	acc0.0<1>:w		Q2_plus_Q3(0)	2:w
			shr.sat (16) TempRow3B(0)<2>	acc0.0<16;16,1>:w		3:w

			// q1' = (q2 + A + 2) >> 2 = (B - 2) >> 2
			add (16)	acc0.0<1>:w		BB(0)			-2:w
			shr.sat (16) TempRow1B(0)<2>	acc0.0<16;16,1>:w	2:w
			
			// q0' = (q2 +2*A + p1 + 4) >> 3 = (B + A + p1) >> 3
			add (16)	acc0.0<1>:w		p1(0)					A(0)
			add (16)	acc0.0<1>:w		acc0.0<16;16,1>:w		BB(0)
			shr.sat (16) TempRow0B(0)<2>	acc0.0<16;16,1>:w	3:w
			
			mov (16) 	NewQ2		TempRow3B(0)						// q2'
			mov (16) 	NewQ1		TempRow1B(0)						// q1'
			mov (16) 	NewQ0		TempRow0B(0)						// q0'

Y_ELSE4:
			else (16)		Y_ENDIF4		// for channels its deltaq = false

			// q0' = (2*q1 + q0 + p1 + 2) >> 2 =  (q1 + Q01 + p1 + 2) >> 2
			// Use original p1 values in p1(0)
			add (16)	acc0.0<1>:w		p1(0)			Q0_plus_Q1(0)			// p1 + P01 (TempRow1(0) = P01)
			add (16)	acc0.0<1>:w		acc0.0<16;16,1>:w	Q1				
			add (16)	acc0.0<1>:w		acc0.0<16;16,1>:w	2:w			// p1 + P01 + q1 + 2

			shr.sat (16)	TempRow0B(0)<2>		acc0.0<16;16,1>:w		2:w								// >> 2
			mov (16) 	NewQ0		TempRow0B(0)						// q0'

			endif
Y_ENDIF4:

			
			// Done with bS = 4 algorithm
			
Y_ELSE2: 
		else 	(16)		Y_ENDIF2
			// For channels whose edge control map2 = 0 ---> bS < 4 algorithm

			//-----------------------------------------------------------------------------
			// bS < 4 Algorithm :
			// tc = tc0 + (|p2-p0|<Beta ? 1 : 0) + (|q2-q0|<Beta ? 1 : 0)
			// delta = Clip3(-tc, tc, ((((q0-p0)<<2) + (p1-q1) + 4) >> 3))
			// p0' = Clip1(p0 + delta) = Clip3(0, 0xFF, p0 + delta)
			// q0' = Clip1(q0 - delta) = Clip3(0, 0xFF, q0 - delta)
			// if (|p2-p0|<Beta)
			// 		p1' = p1 + Clip3(-tc0, tc0, (p2 + ((p0+q0+1)>>1) - (p1<<1)) >> 1 )
			// if (|q2-q0|<Beta)
			// 		q1' = q1 + Clip3(-tc0, tc0, (q2 + ((p0+q0+1)>>1) - (q1<<1)) >> 1 )
			//-----------------------------------------------------------------------------
			
			// Expand tc0
			mov (16)	tc_exp(0)<1>	tc0<1;4,0>:ub	{NoMask}
			mov (16)	tc0_exp(0)<1>	tc0<1;4,0>:ub	{NoMask}					// tc0_exp = tc0, each tc0 is duplicated 4 times for 4 adjcent 4 pixels	
						
			// tc_exp = tc0_exp + (|p2-p0|<Beta ? 1 : 0) + (|q2-q0|<Beta ? 1 : 0)			
//			mov (16)	tc_exp(0)<1>		tc0_exp(0)									// tc = tc0_exp first
			

			cmp.l.f0.0 (16)	null:w		(abs)ap(0)			beta:w						// |p2-p0|< Beta ? ---> (abs)ap < Beta ?
			cmp.l.f0.1 (16)	null:w		(abs)aq(0)			beta:w						// |q2-q0|< Beta ? ---> (abs)aq < Beta ?
			
			//--- Use free cycles here ---
			// delta = Clip3(-tc, tc, ((((q0-p0)<<2) + (p1-q1) + 4) >> 3))
			// 4 * (q0-p0) + p1 - q1 + 4
			add (16) acc0<1>:w		P1			4:w							// p1 + 4
			mac (16) acc0<1>:w		q0_p0(0)	4:w							// 4 * (q0-p0) + p1 + 4
			add (16) acc0<1>:w		acc0<16;16,1>:w		-Q1					// 4 * (q0-p0) + p1 - q1 + 4
			shr (16) TempRow0(0)<1> acc0<16;16,1>:w		3:w
						
			// Continue on getting tc_exp
			(f0.0) add (16)	tc_exp(0)<1>	tc_exp(0)	1:w							// tc0_exp + (|p2-p0|<Beta ? 1 : 0)
			mov (2)	CTemp1_W<1>:w		f0.0<2;2,1>:w			{NoMask}					// Save	|p2-p0|<Beta flag			
			(f0.1) add (16)	tc_exp(0)<1>	tc_exp(0)	1:w							// tc_exp = tc0_exp + (|p2-p0|<Beta ? 1 : 0) + (|q2-q0|<Beta ? 1 : 0)
			

			// Continue on cliping tc to get delta
			cmp.g.f0.0	(16) null:w		TempRow0(0)		tc_exp(0)					// Clip if delta' > tc
			cmp.l.f0.1	(16) null:w		TempRow0(0)		-tc_exp(0)					// Clip if delta' < -tc

			//--- Use free cycles here ---
			// common = (p0+q0+1) >> 1 	  --->  TempRow2(0)
			// Same as avg of p0 and q0
			avg (16) TempRow2(0)<1>		P0			Q0

			// Continue on cliping tc to get delta
			(f0.0) mov (16) TempRow0(0)<1>				tc_exp(0)
			(f0.1) mov (16) TempRow0(0)<1>				-tc_exp(0)

			//--- Use free cycles here ---
			mov (2)	f0.0<1>:w		CTemp1_W<2;2,1>:w	{NoMask}			// CTemp1_W = (|p2-p0|<Beta)
																			// CTemp2_W = (|q2-q0|<Beta)		
			//-----------------------------------------------------------------------

			// p0' = Clip1(p0 + delta) = Clip3(0, 0xFF, p0 + delta)
			// q0' = Clip1(q0 - delta) = Clip3(0, 0xFF, q0 - delta)
			add.sat (16) TempRow1B(0)<2>		P0			TempRow0(0)					// p0+delta
			add.sat (16) TempRow0B(0)<2>		Q0			-TempRow0(0) 				// q0-delta
			mov (16) NewP0		TempRow1B(0)					// p0'
			mov (16) NewQ0		TempRow0B(0)					// q0'
			//-----------------------------------------------------------------------

			// Now compute p1' and q1'

			// if (|p2-p0|<Beta)
//			mov (1)	f0.0:w		CTemp1_W:w				{NoMask}			// CTemp1_W = (|p2-p0|<Beta)
			(f0.0)	if	(16)		Y_ENDIF6
		
			// p1' = p1 + Clip3(-tc0, tc0, adj)
			// adj = (p2 + common - (p1<<1)) >> 1 = (p2 + common - (p1*2)) >> 1
			add (16) acc0<1>:w	P2		TempRow2(0)							// TempRow2(0) = common = (p0+q0+1) >> 1
			mac (16) acc0<1>:w	P1		-2:w
			shr (16) TempRow1(0)<1>		acc0<16;16,1>:w		1:w

			// tc clip to get tc_adj
			cmp.g.f0.0	(16) null:w		TempRow1(0)		tc0_exp(0)					// Clip if delta' > tc
			cmp.l.f0.1	(16) null:w		TempRow1(0)		-tc0_exp(0)					// Clip if delta' < -tc
			
			(f0.0) mov (16) TempRow1(0)<1>				tc0_exp(0)
			(f0.1) mov (16) TempRow1(0)<1>				-tc0_exp(0)

			//--- Use free cycles here ---
			mov (1)	f0.1:w		CTemp2_W:w				{NoMask}			// CTemp2_W = (|q2-q0|<Beta)

			// p1' = p1 + tc_adj
			add.sat (16) TempRow1B(0)<2>		P1			TempRow1(0)					// p1+tc_adj
			mov (16) NewP1			TempRow1B(0)				// p1'

			//------------------------------------------------------------------------
Y_ENDIF6:
			endif
			
			// if (|q2-q0|<Beta)
//			mov (1)	f0.1:w		CTemp2_W:w				{NoMask}			// CTemp2_W = (|q2-q0|<Beta)
			(f0.1)	if	(16)		Y_ENDIF7
					
			// q1' = q1 + Clip3(-tc0, tc0, adj)
			// adj = (q2 + common - (q1<<1)) >> 1 
			// same as q2 + common - (q1 * 2)
			add (16) acc0<1>:w	Q2		TempRow2(0)
			mac (16) acc0<1>:w	Q1		-2:w
			shr (16) TempRow1(0)<1>		acc0<16;16,1>:w		1:w	

			// tc clip to get tc_adj
			cmp.g.f0.0	(16) null:w		TempRow1(0)		tc0_exp(0)					// Clip if delta' > tc
			cmp.l.f0.1	(16) null:w		TempRow1(0)		-tc0_exp(0)					// Clip if delta' < -tc

			(f0.0) mov (16) TempRow1(0)<1>				tc0_exp(0)
			(f0.1) mov (16) TempRow1(0)<1>				-tc0_exp(0)

			// q1' = q1 + tc_adj
			add.sat (16) TempRow1B(0)<2>		Q1			TempRow1(0)					// q1+tc_adj
			mov (16) NewQ1			TempRow1B(0)				// q1'

			//------------------------------------------------------------------------			
Y_ENDIF7:
			endif

		endif
Y_ENDIF2:
Y_ENDIF1:
	endif

RETURN

#endif	// !defined(__AVC_ILDB_LUMA_CORE__)
