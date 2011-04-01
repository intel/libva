/*
 * Copyright © <2010>, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Eclipse Public License (EPL), version 1.0.  The full text of the EPL is at
 * http://www.opensource.org/licenses/eclipse-1.0.php.
 *
 */
// Modual name: IntraFrame.asm
//
// Make intra predition estimation for Intra frame
//

//
//  Now, begin source code....
//

include(`vme_header.inc')

/*inline input data: r5~r11*/
mov(1) r5.20<1>:UB      r0.20<1,1,0>:UB {align1}            ;
mov(8) m0.0<1>:UD 	r5.0<8,8,1>:UD {align1};
mov(8) m1.0<1>:UD  	r6.0<8,8,1>:UD {align1};
mov(8) m2.0<1>:UD  	r7.0<8,8,1>:UD {align1};
mov(8) m3.0<1>:UD  	r8.0<8,8,1>:UD {align1};
send(8) 0 r12 null  vme(0,0,0,2) mlen 4 rlen 1 {align1};

mov(1) r9.20<1>:UB      r0.20<1,1,0>:UB {align1}            ;        
mov(8) m0.0<1>:UD 	r9.0<8,8,1>:UD 		{align1};
mov(1) m1.0<1>:UD	r12.0<0,1,0>:UD		{align1};		/*W0.0*/
mov(1) m1.4<1>:UD	r12.16<0,1,0>:UD	{align1};		/*W0.4*/
mov(1) m1.8<1>:UD	r12.20<0,1,0>:UD	{align1};		/*W0.5*/
mov(1) m1.12<1>:UD	r12.24<0,1,0>:UD	{align1};		/*W0.6*/

/* bind index 3, write 1 oword, msg type: 8(OWord Block Write) */
send (16) 0 r13 null write(3, 0, 8, 1) mlen 2 rlen 1 {align1};

mov (8) m0.0<1>:UD r0<8,8,1>:UD {align1};
send (16) 0 acc0<1>UW null thread_spawner(0, 0, 1) mlen 1 rlen 0 {align1 EOT};

