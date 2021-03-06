/*
**
** PHiPAC Matrix-Matrix Code for the operation:
**    C = A*B + C
**
** Automatically Generated by mm_cgen ($Revision: 1.27 $) using the command:
**    ./mm_cgen -prec double -opA N -opB N -alpha 1 -sp 1 -holdstripe B -l0 1 20 12 -no_fringes -file ./src/mm_double_NN_1_nofringe.c -routine_name mm_double_NN_1_nofringe 
**
** Run './mm_cgen -help' for help.
**
** WARNING: This routine was generated without code for
** fringes in the following dimension(s): M K N 
** That means that the routine will only work for matrices with the
** appropriate size(s). See the 'Routine Usage' below for details.
**
** Generated on: Wednesday July 10 2013, 08:33:28 PDT
** Created by: Jeff Bilmes <bilmes@cs.berkeley.edu>
**             http://www.icsi.berkeley.edu/~bilmes/phipac
**
**
** Routine Usage: General (M,K,N) = (m*1:m>=0, k*20:k>=0, n*12:n>=0) matrix multiply
**    mm_double_NN_1_nofringe(const int M, const int K, const int N, const double *const A, const double *const B, double *const C, const int Astride, const int Bstride, const int Cstride)
** where
**  A is an MxK matrix
**  B is an KxN matrix
**  C is an MxN matrix
**  Astride is the number of entries between the start of each row of A
**  Bstride is the number of entries between the start of each row of B
**  Cstride is the number of entries between the start of each row of C
**
**
** "Copyright (c) 1995 The Regents of the University of California.  All
** rights reserved."  Permission to use, copy, modify, and distribute
** this software and its documentation for any purpose, without fee, and
** without written agreement is hereby granted, provided that the above
** copyright notice and the following two paragraphs appear in all copies
** of this software.
**
** IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
** DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
** OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
** CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
** INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
** AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
** ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
** PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
**
*/

/*
 * General (M,K,N) = (m*1:m>=0, k*20:k>=0, n*12:n>=0) matrix multiply
 */
void
mm_double_NN_1_nofringe(const int M, const int K, const int N, const double *const A, const double *const B, double *const C, const int Astride, const int Bstride, const int Cstride)
{
   const double *a,*b;
   double *c;
   const double *ap_0;
   const double *bp;
   double *cp;
   const int A_sbs_stride = Astride*1;
   const int C_sbs_stride = Cstride*1;
   const int k_marg_el = K % 20;
   const int k_norm = K - k_marg_el;
   const int m_marg_el = M & 0;
   const int m_norm = M - m_marg_el;
   const int n_marg_el = N % 12;
   const int n_norm = N - n_marg_el;
   double *const c_endp = C+m_norm*Cstride;
   register double c0_0,c0_1,c0_2,c0_3,c0_4,c0_5,c0_6,c0_7,c0_8,c0_9,c0_10,c0_11;
   for (c=C,a=A; c!= c_endp; c+=C_sbs_stride,a+=A_sbs_stride) {
      const double* const ap_endp = a + k_norm;
      double* const cp_endp = c + n_norm;
      for (b=B,cp=c; cp!=cp_endp; b+=12,cp+=12) {
         register double _b0,_b1,_b2,_b3,_b4,_b5,_b6,_b7,_b8,_b9,_b10,_b11;
         register double _a0;
         double *_cp;
         ap_0 = a;
         bp=b;
         _cp=cp;c0_0=_cp[0];c0_1=_cp[1];c0_2=_cp[2];c0_3=_cp[3];c0_4=_cp[4];c0_5=_cp[5];c0_6=_cp[6];c0_7=_cp[7];c0_8=_cp[8];c0_9=_cp[9];c0_10=_cp[10];c0_11=_cp[11];
         for (;ap_0!=ap_endp; ap_0+=20) {
            /* Fixed M,K,N = 1,20,12 fully-unrolled matrix matrix multiply. */
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[0];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[1];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[2];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[3];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[4];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[5];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[6];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[7];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[8];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[9];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[10];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[11];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[12];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[13];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[14];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[15];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[16];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[17];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[18];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 
            
            _b0 = bp[0]; _b1 = bp[1]; _b2 = bp[2]; _b3 = bp[3]; _b4 = bp[4]; _b5 = bp[5]; _b6 = bp[6]; _b7 = bp[7]; _b8 = bp[8]; _b9 = bp[9]; _b10 = bp[10]; _b11 = bp[11]; 
            bp += Bstride;
            _a0 = ap_0[19];
            c0_0 += _a0*_b0; c0_1 += _a0*_b1; c0_2 += _a0*_b2; c0_3 += _a0*_b3; c0_4 += _a0*_b4; c0_5 += _a0*_b5; c0_6 += _a0*_b6; c0_7 += _a0*_b7; c0_8 += _a0*_b8; c0_9 += _a0*_b9; c0_10 += _a0*_b10; c0_11 += _a0*_b11; 

         }
         _cp=cp;_cp[0]=c0_0;_cp[1]=c0_1;_cp[2]=c0_2;_cp[3]=c0_3;_cp[4]=c0_4;_cp[5]=c0_5;_cp[6]=c0_6;_cp[7]=c0_7;_cp[8]=c0_8;_cp[9]=c0_9;_cp[10]=c0_10;_cp[11]=c0_11;
      }
   }
}
