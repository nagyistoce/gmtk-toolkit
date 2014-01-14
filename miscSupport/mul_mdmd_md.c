/*
**
** PHiPAC Matrix-Matrix Code for the operation:
**    C = A*B
**
** Automatically Generated by mm_lgen ($Revision$) using the command:
**    /n/hop/db/bilmes/phipac/mm_gen/mm_lgen -opA N -opB N -sp 2ma -prec double -l0 2 16 2 -l1 5 5 5 -gen_r mul_mdmd_md_l0g -gen_nf mul_mdmd_md_l0nf -file mul_mdmd_md.c -alpha 1 -beta 0 -rout mul_mdmd_md 
**
** Run '/n/hop/db/bilmes/phipac/mm_gen/mm_lgen -help' for help.
**
** Generated on: Thursday July 03 1997, 01:19:08 PDT
** Created by: Jeff Bilmes <bilmes@cs.berkeley.edu>
**             http://www.icsi.berkeley.edu/~bilmes/phipac
**
**
** Routine Usage: 
**    mul_mdmd_md(const int M, const int K, const int N, const double *const A, const double *const B, double *const C, const int Astride, const int Bstride, const int Cstride)
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

extern void 
mul_mdmd_md_l0g(const int M, const int K, const int N, 
		const double *const A, const double *const B, double *const C,
		const int Astride, const int Bstride, const int Cstride);

extern void 
mul_mdmd_md_l0nf(const int M, const int K, const int N, 
		 const double *const A, const double *const B, double *const C,
		 const int Astride, const int Bstride, const int Cstride);

void
mul_mdmd_md(const int M, const int K, const int N, const double *const A, const double *const B, double *const C, const int Astride, const int Bstride, const int Cstride)
{
   int m,k,n;
   const double *a,*b;
   double *c;
   const double *ap,*bp;
   double *cp;
   const int m_marg = (M % 10) - (M % 2);
   const int k_marg = (K % 81) - ((K%81)-1)%16;
   const int n_marg = (N % 10) - (N % 2);
   {
      double *cprb,*cpre,*cp,*cpe;
      cpre = C + M*Cstride;
      for (cprb = C; cprb != cpre; cprb += Cstride) {
         cpe = cprb + N;
         for (cp = cprb; cp != cpe; cp++) {
            *cp = 0.0;
         }
      }
   }
   for (m=0; (m+10)<=M; m+=10) {
      c = C + m*Cstride;
      a = A + m*Astride;
      for (n=0,b=B,cp=c; (n+10)<=N; n+=10,b+=10,cp+=10) {
         for (k=0,bp=b,ap=a; (k+81)<=K; k+=81,bp+=81*Bstride,ap+=81) {
            mul_mdmd_md_l0nf(10,81,10,ap,bp,cp,Astride,Bstride,Cstride);
         }
         if (k + 17 <= K) {
            mul_mdmd_md_l0nf(10,k_marg,10,ap,bp,cp,Astride,Bstride,Cstride);
            k+=k_marg;bp+=k_marg*Bstride;ap+=k_marg;
         }
         if (k < K) {
            mul_mdmd_md_l0g(10,K-k,10,ap,bp,cp,Astride,Bstride,Cstride);
         }
      }
      if (n + 2 <= N) {
         for (k=0,bp=b,ap=a; (k+81)<=K; k+=81,bp+=81*Bstride,ap+=81) {
            mul_mdmd_md_l0nf(10,81,n_marg,ap,bp,cp,Astride,Bstride,Cstride);
         }
         if (k + 17 <= K) {
            mul_mdmd_md_l0nf(10,k_marg,n_marg,ap,bp,cp,Astride,Bstride,Cstride);
            k+=k_marg;bp+=k_marg*Bstride;ap+=k_marg;
         }
         if (k < K) {
            mul_mdmd_md_l0g(10,K-k,n_marg,ap,bp,cp,Astride,Bstride,Cstride);
         }
         n+=n_marg;b+=n_marg;cp+=n_marg;
      }
      if (n < N) {
         for (k=0,bp=b,ap=a; (k+81)<=K; k+=81,bp+=81*Bstride,ap+=81) {
            mul_mdmd_md_l0g(10,81,N-n,ap,bp,cp,Astride,Bstride,Cstride);
         }
         if (k + 17 <= K) {
            mul_mdmd_md_l0g(10,k_marg,N-n,ap,bp,cp,Astride,Bstride,Cstride);
            k+=k_marg;bp+=k_marg*Bstride;ap+=k_marg;
         }
         if (k < K) {
            mul_mdmd_md_l0g(10,K-k,N-n,ap,bp,cp,Astride,Bstride,Cstride);
         }
      }
   }
   if (m + 2 <= M) {
      c = C + m*Cstride;
      a = A + m*Astride;
      for (n=0,b=B,cp=c; (n+10)<=N; n+=10,b+=10,cp+=10) {
         for (k=0,bp=b,ap=a; (k+81)<=K; k+=81,bp+=81*Bstride,ap+=81) {
            mul_mdmd_md_l0nf(m_marg,81,10,ap,bp,cp,Astride,Bstride,Cstride);
         }
         if (k + 17 <= K) {
            mul_mdmd_md_l0nf(m_marg,k_marg,10,ap,bp,cp,Astride,Bstride,Cstride);
            k+=k_marg;bp+=k_marg*Bstride;ap+=k_marg;
         }
         if (k < K) {
            mul_mdmd_md_l0g(m_marg,K-k,10,ap,bp,cp,Astride,Bstride,Cstride);
         }
      }
      if (n + 2 <= N) {
         for (k=0,bp=b,ap=a; (k+81)<=K; k+=81,bp+=81*Bstride,ap+=81) {
            mul_mdmd_md_l0nf(m_marg,81,n_marg,ap,bp,cp,Astride,Bstride,Cstride);
         }
         if (k + 17 <= K) {
            mul_mdmd_md_l0nf(m_marg,k_marg,n_marg,ap,bp,cp,Astride,Bstride,Cstride);
            k+=k_marg;bp+=k_marg*Bstride;ap+=k_marg;
         }
         if (k < K) {
            mul_mdmd_md_l0g(m_marg,K-k,n_marg,ap,bp,cp,Astride,Bstride,Cstride);
         }
         n+=n_marg;b+=n_marg;cp+=n_marg;
      }
      if (n < N) {
         for (k=0,bp=b,ap=a; (k+81)<=K; k+=81,bp+=81*Bstride,ap+=81) {
            mul_mdmd_md_l0g(m_marg,81,N-n,ap,bp,cp,Astride,Bstride,Cstride);
         }
         if (k + 17 <= K) {
            mul_mdmd_md_l0g(m_marg,k_marg,N-n,ap,bp,cp,Astride,Bstride,Cstride);
            k+=k_marg;bp+=k_marg*Bstride;ap+=k_marg;
         }
         if (k < K) {
            mul_mdmd_md_l0g(m_marg,K-k,N-n,ap,bp,cp,Astride,Bstride,Cstride);
         }
      }
      m+=m_marg;
   }
   if (m < M) {
      c = C + m*Cstride;
      a = A + m*Astride;
      for (n=0,b=B,cp=c; (n+10)<=N; n+=10,b+=10,cp+=10) {
         for (k=0,bp=b,ap=a; (k+81)<=K; k+=81,bp+=81*Bstride,ap+=81) {
            mul_mdmd_md_l0g(M-m,81,10,ap,bp,cp,Astride,Bstride,Cstride);
         }
         if (k + 17 <= K) {
            mul_mdmd_md_l0g(M-m,k_marg,10,ap,bp,cp,Astride,Bstride,Cstride);
            k+=k_marg;bp+=k_marg*Bstride;ap+=k_marg;
         }
         if (k < K) {
            mul_mdmd_md_l0g(M-m,K-k,10,ap,bp,cp,Astride,Bstride,Cstride);
         }
      }
      if (n + 2 <= N) {
         for (k=0,bp=b,ap=a; (k+81)<=K; k+=81,bp+=81*Bstride,ap+=81) {
            mul_mdmd_md_l0g(M-m,81,n_marg,ap,bp,cp,Astride,Bstride,Cstride);
         }
         if (k + 17 <= K) {
            mul_mdmd_md_l0g(M-m,k_marg,n_marg,ap,bp,cp,Astride,Bstride,Cstride);
            k+=k_marg;bp+=k_marg*Bstride;ap+=k_marg;
         }
         if (k < K) {
            mul_mdmd_md_l0g(M-m,K-k,n_marg,ap,bp,cp,Astride,Bstride,Cstride);
         }
         n+=n_marg;b+=n_marg;cp+=n_marg;
      }
      if (n < N) {
         for (k=0,bp=b,ap=a; (k+81)<=K; k+=81,bp+=81*Bstride,ap+=81) {
            mul_mdmd_md_l0g(M-m,81,N-n,ap,bp,cp,Astride,Bstride,Cstride);
         }
         if (k + 17 <= K) {
            mul_mdmd_md_l0g(M-m,k_marg,N-n,ap,bp,cp,Astride,Bstride,Cstride);
            k+=k_marg;bp+=k_marg*Bstride;ap+=k_marg;
         }
         if (k < K) {
            mul_mdmd_md_l0g(M-m,K-k,N-n,ap,bp,cp,Astride,Bstride,Cstride);
         }
      }
   }
}
