/*
 * All Video Processing kernels 
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */

// Module name: readSampler16x1.asm
//
// Read one row of pix through sampler
//



//#define SAMPLER_MSG_DSC		0x166A0000	// ILK Sampler Message Descriptor



// Send Message [DevILK]                                Message Descriptor
//  MBZ MsgL=5 MsgR=8                            H MBZ   SIMD     MsgType   SmplrIndx BindTab
//  000 0 101 0 1000                             1  0     10     0000         0000    00000000
//    0     A    8                                     A             0             0     0     0

//     MsgL=1+2*2(u,v)=5 MsgR=8
 
#define SAMPLER_MSG_DSC		0x0A8A0000	// ILK Sampler Message Descriptor





                                                                                

	// Assume MSGSRC is set already in the caller
        //mov (8)		rMSGSRC.0<1>:ud			0:ud	// Unused fileds



	// Read 16 sampled pixels and stored them in float32 in 8 GRFs
	// 422 data is expanded to 444, return 8 GRF in the order of RGB- (UYV-).
	// 420 data has three surfaces, return 8 GRF. Valid is always in the 1st GRF when in R8.  Make sure no overwrite the following 3 GRFs.
	// alpha data is expanded to 4444, return 8 GRF in the order of RGBA (UYVA).

    mov(16)     mMSGHDR<1>:uw   rMSGSRC<16;16,1>:uw
    send (16)	DATABUF(0)<1>	mMSGHDR		udDUMMY_NULL	0x2 SAMPLER_MSG_DSC+SAMPLER_IDX+BINDING_IDX:ud




    


