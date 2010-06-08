/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
////////// AVC ILDB Chroma Core Mbaff /////////////////////////////////////////////////////////////////////////////////
//
//	This core performs AVC U or V ILDB filtering on one horizontal edge (8 pixels) of a MB.
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
//	U or V:
//	+----+----+----+----+
//	| P1 | p0 | q0 | q1 |
//	+----+----+----+----+
//
//	p1 = r[P_AddrReg, 0]<16;8,2> 
//	p0 = r[P_AddrReg, 16]<16;8,2> 
// 	q0 = r[Q_AddrReg, 0]<16;8,2>  
//	q1 = r[Q_AddrReg, 16]<16;8,2> 
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The region is both src and dest
// P0-P3 and Q0-Q3 should be only used if they have not been modified to new values  
#undef 	P1
#undef 	P0
#undef 	Q0
#undef 	Q1

#define P1 		r[P_AddrReg,  0]<16;8,2>:ub
#define P0 		r[P_AddrReg, 16]<16;8,2>:ub
#define Q0 		r[Q_AddrReg,  0]<16;8,2>:ub
#define Q1 		r[Q_AddrReg, 16]<16;8,2>:ub

// New region as dest
#undef 	NewP0
#undef 	NewQ0

#define NewP0 	r[P_AddrReg, 16]<2>:ub
#define NewQ0 	r[Q_AddrReg,  0]<2>:ub

// Filter one chroma edge - mbaff
FILTER_UV_MBAFF:

#if defined(_DEBUG) 
	mov		(1)		EntrySignatureC:w			0x1112:w
#endif
	//---------- Derive filterSampleflag in AVC spec, equition (8-469) ----------

	//===== Assume f0.0 contains MaskA when entering this routine
//	mov (1)	f0.0:uw		MaskA:uw

	add (8) q0_p0(0)<1>			Q0		-P0				// q0-p0
	add (8) TempRow0(0)<1>		P1		-P0				// p1-p0
	add (8) TempRow1(0)<1>		Q1		-Q0				// q1-q0

	// Build FilterSampleFlag
	// abs(q0-p0) < alpha
	(f0.0) cmp.l.f0.0 (16) null:w		(abs)q0_p0(0)			Mbaff_ALPHA(0)
	// abs(p1-p0) < Beta
	(f0.0) cmp.l.f0.0 (16) null:w		(abs)TempRow0(0)		Mbaff_BETA(0)
	// abs(q1-q0) < Beta
	(f0.0) cmp.l.f0.0 (16) null:w		(abs)TempRow1(0)		Mbaff_BETA(0)

	//-----------------------------------------------------------------------------------------

	// if 
    (f0.0)	if	(8)		MBAFF_UV_ENDIF1
		// For channels whose edge control map1 = 1 ---> perform de-blocking

//		mov (1)		f0.1:w		MaskB:w		{NoMask}		// Now check for which algorithm to apply

		(f0.1)	if	(8)		MBAFF_UV_ELSE2

			// For channels whose edge control map2 = 1 ---> bS = 4 algorithm 
			// p0' = (2*p1 + P0 + q1 + 2) >> 2
			// q0' = (2*q1 + q0 + p1 + 2) >> 2
			//------------------------------------------------------------------------------------

			// p0' = (2*p1 + p0 + q1 + 2) >> 2
			add (8) acc0<1>:w		Q1				2:w
			mac (8) acc0<1>:w		P1				2:w
			add (8)	acc0<1>:w		acc0<8;8,1>:w	P0
			shr.sat	(8)	TempRow0B(0)<2>		acc0<8;8,1>:w		2:w

			// q0' = (2*q1 + q0 + p1 + 2) >> 2
			add (8) acc0<1>:w		P1				2:w
			mac (8) acc0<1>:w		Q1				2:w
			add (8)	acc0<1>:w		acc0<8;8,1>:w	Q0
			shr.sat	(8)	TempRow1B(0)<2>		acc0<8;8,1>:w		2:w

			mov (8) NewP0		TempRow0B(0)					// p0'
			mov (8) NewQ0		TempRow1B(0)					// q0'
			
MBAFF_UV_ELSE2: 
		else 	(8)		MBAFF_UV_ENDIF2
			// For channels whose edge control map2 = 0 ---> bS < 4 algorithm
			
			// tc_exp = tc0_exp + 1
			add (8) tc_exp(0)<1>	Mbaff_TC0(0)		1:w

			// delta = Clip3(-tc, tc, ((((q0 - p0)<<2) + (p1-q1) + 4) >> 3))
			// 4 * (q0-p0) + p1 - q1 + 4
			add (8)	acc0<1>:w		P1			4:w
			mac (8) acc0<1>:w		q0_p0(0)	4:w	
			add (8) acc0<1>:w		acc0<8;8,1>:w		-Q1
			shr (8) TempRow0(0)<1>	acc0<8;8,1>:w		3:w

			// tc clip
			cmp.g.f0.0	(8) null:w		TempRow0(0)		tc_exp(0)				// Clip if > tc0
			cmp.l.f0.1	(8) null:w		TempRow0(0)		-tc_exp(0)				// Clip if < -tc0
			
			(f0.0) mov (8) TempRow0(0)<1>				tc_exp(0)
			(f0.1) mov (8) TempRow0(0)<1>				-tc_exp(0)
			
			// p0' = Clip1(p0 + delta) = Clip3(0, 0xFF, p0 + delta)
			add.sat (8)	TempRow1B(0)<2>		P0			TempRow0(0)				// p0+delta
		
			// q0' = Clip1(q0 - delta) = Clip3(0, 0xFF, q0 - delta)
			add.sat (8)	TempRow0B(0)<2>		Q0			-TempRow0(0)			// q0-delta

			mov (8) NewP0				TempRow1B(0)			// p0'
			mov (8) NewQ0				TempRow0B(0)			// q0'

		endif
MBAFF_UV_ENDIF2:
MBAFF_UV_ENDIF1:
	endif

RETURN



