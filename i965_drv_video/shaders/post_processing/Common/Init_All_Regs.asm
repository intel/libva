/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

#ifdef GT	// to remove error messages of un-initialized GRF
	.declare 	udGRF_space	 	 Base=r0.0 ElementSize=4 SrcRegion=REGION(8,1) Type=ud	

	$for (7; <80; 1) {
		mov (8) udGRF_space(%1)<1>	0:ud
	}
#else
#endif