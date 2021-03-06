/*-
 * GMTK_DiagGaussianOpt.cc
 *      -  Code for plain vanilla diagonal Gaussians.
 *      -  These routines might benefit from separate optimization (such as separate loop unrolling).
 *     
 *
 * Written by Jeff Bilmes <bilmes@ee.washington.edu>
 *
 * Copyright (C) 2001 Jeff Bilmes
 * Licensed under the Open Software License version 3.0
 * See COPYING or http://opensource.org/licenses/OSL-3.0
 *
 *
 */


#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <float.h>
#include <assert.h>
#include <ctype.h>

#include <string>

#include "general.h"
#if HAVE_CONFIG_H
#include <config.h>
#endif
#if HAVE_HG_H
#include "hgstamp.h"
#endif
VCID(HGID)

#include "error.h"
#include "rand.h"

#include "GMTK_DiagGaussian.h"
#include "GMTK_GMParms.h"
#include "GMTK_MixtureCommon.h"



/*-
 *-----------------------------------------------------------------------
 * log_p()
 *      Computes the probability of this Gaussian.
 * 
 * Preconditions:
 *      preCompute() must have been called on covariance matrix before this.
 *
 * Postconditions:
 *      nil
 *
 * Side Effects:
 *      nil, other than possible FPEs if the values are garbage
 *
 * Results:
 *      Returns the probability.
 *
 *-----------------------------------------------------------------------
 */

logpr
DiagGaussian::log_p(const float *const x,
		    const Data32* const base,
		    const int stride)
{
  assert ( basicAllocatedBitIsSet() );

  //////////////////////////////////////////////////////////////////
  // The local accumulator type in this routine.
  // This can be changed from 'float' to 'double' to
  // provide extra range for temporary accumulators. Alternatively,
  // decreasing the program's mixCoeffVanishRatio at the beginning
  // of training should eliminate any component that produces
  // such low scores.
#define DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE double

  ////////////////////
  // note: 
  // covariances must have been precomputed for this
  // to work.
  const float *xp = x;
  const float *mean_p = mean->basePtr();
  const float *var_inv_p = covar->baseVarInvPtr();

#if 0
  // do the non-unrolled version only.
  const float *const x_endp = x + _dim;
  DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE d=0.0;
  do {
    const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp
      = (*xp - *mean_p);
    d += (tmp*(*var_inv_p))*tmp;

    xp++;
    mean_p++;
    var_inv_p++;
  } while (xp != x_endp);
#else
  // do various unrolled unrolled versions
  DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE d=0.0;
  if (_dim >= 8)  {
    // handle all lengths >= 8.

    // unroll a bit to allow better software pipelining to occur
    const float *const x_endp = x + (_dim & ~0x7);

    DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE d0=0.0,d1=0.0,d2=0.0,d3=0.0;
    DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE d4=0.0,d5=0.0,d6=0.0,d7=0.0;
    do {

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp0
	= (xp[0] - mean_p[0]);
      d0 += (tmp0*(var_inv_p[0]))*tmp0;

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp1
	= (xp[1] - mean_p[1]);
      d1 += (tmp1*(var_inv_p[1]))*tmp1;

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp2
	= (xp[2] - mean_p[2]);
      d2 += (tmp2*(var_inv_p[2]))*tmp2;

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp3
	= (xp[3] - mean_p[3]);
      d3 += (tmp3*(var_inv_p[3]))*tmp3;

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp4
	= (xp[4] - mean_p[4]);
      d4 += (tmp4*(var_inv_p[4]))*tmp4;

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp5
	= (xp[5] - mean_p[5]);
      d5 += (tmp5*(var_inv_p[5]))*tmp5;

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp6
	= (xp[6] - mean_p[6]);
      d6 += (tmp6*(var_inv_p[6]))*tmp6;

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp7
	= (xp[7] - mean_p[7]);
      d7 += (tmp7*(var_inv_p[7]))*tmp7;

      xp +=8;
      mean_p +=8;
      var_inv_p +=8;
    } while (xp != x_endp);

    if (_dim & 0x4) {
      if ((_dim & 0x3) == 0x3) {
	// remainder is 7
	
	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp0
	  = (xp[0] - mean_p[0]);
	d0 += (tmp0*(var_inv_p[0]))*tmp0;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp1
	  = (xp[1] - mean_p[1]);
	d1 += (tmp1*(var_inv_p[1]))*tmp1;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp2
	  = (xp[2] - mean_p[2]);
	d2 += (tmp2*(var_inv_p[2]))*tmp2;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp3
	  = (xp[3] - mean_p[3]);
	d3 += (tmp3*(var_inv_p[3]))*tmp3;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp4
	  = (xp[4] - mean_p[4]);
	d4 += (tmp4*(var_inv_p[4]))*tmp4;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp5
	  = (xp[5] - mean_p[5]);
	d5 += (tmp5*(var_inv_p[5]))*tmp5;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp6
	  = (xp[6] - mean_p[6]);
	d6 += (tmp6*(var_inv_p[6]))*tmp6;

      } else if (_dim & 0x2) {
	// remainder is 6

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp0
	  = (xp[0] - mean_p[0]);
	d0 += (tmp0*(var_inv_p[0]))*tmp0;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp1
	  = (xp[1] - mean_p[1]);
	d1 += (tmp1*(var_inv_p[1]))*tmp1;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp2
	  = (xp[2] - mean_p[2]);
	d2 += (tmp2*(var_inv_p[2]))*tmp2;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp3
	  = (xp[3] - mean_p[3]);
	d3 += (tmp3*(var_inv_p[3]))*tmp3;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp4
	  = (xp[4] - mean_p[4]);
	d4 += (tmp4*(var_inv_p[4]))*tmp4;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp5
	  = (xp[5] - mean_p[5]);
	d5 += (tmp5*(var_inv_p[5]))*tmp5;


      } else if (_dim & 0x1) {
	// remainder is 5
	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp0
	  = (xp[0] - mean_p[0]);
	d0 += (tmp0*(var_inv_p[0]))*tmp0;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp1
	  = (xp[1] - mean_p[1]);
	d1 += (tmp1*(var_inv_p[1]))*tmp1;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp2
	  = (xp[2] - mean_p[2]);
	d2 += (tmp2*(var_inv_p[2]))*tmp2;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp3
	  = (xp[3] - mean_p[3]);
	d3 += (tmp3*(var_inv_p[3]))*tmp3;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp4
	  = (xp[4] - mean_p[4]);
	d4 += (tmp4*(var_inv_p[4]))*tmp4;

      } else {
	// remainder is 4
	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp0
	  = (xp[0] - mean_p[0]);
	d0 += (tmp0*(var_inv_p[0]))*tmp0;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp1
	  = (xp[1] - mean_p[1]);
	d1 += (tmp1*(var_inv_p[1]))*tmp1;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp2
	  = (xp[2] - mean_p[2]);
	d2 += (tmp2*(var_inv_p[2]))*tmp2;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp3
	  = (xp[3] - mean_p[3]);
	d3 += (tmp3*(var_inv_p[3]))*tmp3;

      }
    } else {
      if ((_dim & 0x3) == 0x3) {
	// remainder is 3

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp0
	  = (xp[0] - mean_p[0]);
	d0 += (tmp0*(var_inv_p[0]))*tmp0;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp1
	  = (xp[1] - mean_p[1]);
	d1 += (tmp1*(var_inv_p[1]))*tmp1;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp2
	  = (xp[2] - mean_p[2]);
	d2 += (tmp2*(var_inv_p[2]))*tmp2;
      } else if (_dim & 0x2) {
	// remainder is 2

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp0
	  = (xp[0] - mean_p[0]);
	d0 += (tmp0*(var_inv_p[0]))*tmp0;

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp1
	  = (xp[1] - mean_p[1]);
	d1 += (tmp1*(var_inv_p[1]))*tmp1;
      } else if (_dim & 0x1) {
	// remainder is 1

	const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp0
	  = (xp[0] - mean_p[0]);
	d0 += (tmp0*(var_inv_p[0]))*tmp0;
      } else {
	// remainder is 0, do nothing.
      }
    }

    d = d0+d1+d2+d3+d4+d5+d6+d7;
  } else if (_dim >= 4)  {
    // handle all lengths >= 4
    
    // unroll a bit to allow better software pipelining to occur.
    // Note, we could just use this 4-case and not unroll
    // by units of 8 above. We keep the general unroll-4 case code
    // here in case we want to not do the 8 case above for some
    // architectures/compilers.

    const float *const x_endp = x + (_dim & ~0x3);

    DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE d0=0.0,d1=0.0,d2=0.0,d3=0.0;
    do {

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp0
	= (xp[0] - mean_p[0]);
      d0 += (tmp0*(var_inv_p[0]))*tmp0;

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp1
	= (xp[1] - mean_p[1]);
      d1 += (tmp1*(var_inv_p[1]))*tmp1;

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp2
	= (xp[2] - mean_p[2]);
      d2 += (tmp2*(var_inv_p[2]))*tmp2;

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp3
	= (xp[3] - mean_p[3]);
      d3 += (tmp3*(var_inv_p[3]))*tmp3;

      xp +=4;
      mean_p +=4;
      var_inv_p +=4;
    } while (xp != x_endp);

    if ((_dim & 0x3) == 0x3) {
      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp0
	= (xp[0] - mean_p[0]);
      d0 += (tmp0*(var_inv_p[0]))*tmp0;

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp1
	= (xp[1] - mean_p[1]);
      d1 += (tmp1*(var_inv_p[1]))*tmp1;

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp2
	= (xp[2] - mean_p[2]);
      d2 += (tmp2*(var_inv_p[2]))*tmp2;
    } else if (_dim & 0x2) {
      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp0
	= (xp[0] - mean_p[0]);
      d0 += (tmp0*(var_inv_p[0]))*tmp0;

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp1
	= (xp[1] - mean_p[1]);
      d1 += (tmp1*(var_inv_p[1]))*tmp1;
    } else if (_dim & 0x1) {
      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp0
	= (xp[0] - mean_p[0]);
      d0 += (tmp0*(var_inv_p[0]))*tmp0;
    }

    d = d0+d1+d2+d3;
  } else {
    // do the base case only. This also handles all lengths even
    // though the above 8 and 4 cases handle all lenghts >=8 and >=4
    // respectively. We assume that _dim > 0 though.
    const float *const x_endp = x + _dim;
    do {
      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp
	= (*xp - *mean_p);
      d += (tmp*(*var_inv_p))*tmp;
      xp++;
      mean_p++;
      var_inv_p++;
    } while (xp != x_endp);
  }
#endif

  d *= -0.5;
  return logpr(0,(covar->log_inv_normConst() + d));
}



/////////////////
// EM routines //
/////////////////



/*-
 *-----------------------------------------------------------------------
 * emIncrementMeanDiagCovar
 *      Simultaneously increments a mean and a diagonal covariance vector
 *      with one loop rather than doing each separately with two loops.
 * 
 * Preconditions:
 *      Vectors must be allocated and pointing to appropriately sized
 *      arrays. No other assumptions are made (e.g., such as like prob
 *      is large enough).
 *
 * Postconditions:
 *      Vectors have been accumulated by f.
 *
 * Side Effects:
 *      Changes meanAccumulator and diagCovarAccumulator arrays.
 *
 * Results:
 *      nothing.
 *
 *-----------------------------------------------------------------------
 */
void 
DiagGaussian::emIncrementMeanDiagCovar(const float fprob,
				       const float * const f,
				       const unsigned len,
				       float *meanAccumulator,
				       float *diagCovarAccumulator)
{
  register const float * f_p = f;
  register const float *const f_p_endp = f + len;
  register float *meanAccumulator_p = meanAccumulator;
  register float *diagCovarAccumulator_p = diagCovarAccumulator;
  do {

#if 0
    // this code has aliasing so is commented out in favor of the
    // code below.
    register float tmp = (*f_p)*fprob;
    *meanAccumulator_p += tmp;
    tmp *= (*f_p);
    *diagCovarAccumulator_p += tmp;
    meanAccumulator_p++;
    diagCovarAccumulator_p++;
    f_p ++;
#endif

    // a version of the above code that avoids aliasing of f_p and
    // meanAccumulator_p so the compiler can probably optimize better.

    register float tmp = (*f_p)*fprob;
    register float tmp2 = tmp*(*f_p);

    *meanAccumulator_p += tmp;
    *diagCovarAccumulator_p += tmp2;

    meanAccumulator_p++;
    diagCovarAccumulator_p++;
    f_p ++;

  } while (f_p != f_p_endp);
}




/*-
 *-----------------------------------------------------------------------
 * fkIncrementMeanDiagCovar
 *      Simultaneously increments a mean and a diagonal covariance vector
 *      with one loop rather than doing each separately with two loops.
 *      Here we incement the mean and covariance according to what is needed
 *      to produce the Fisher kernel vector (i.e., what is needed to produce the
 *      Fisher kernel of the DBN). The resulting feature space parameters are
 *      stored in the very same EM accumulators.
 * 
 * Preconditions:
 *      Vectors must be allocated and pointing to appropriately sized
 *      arrays. No other assumptions are made (e.g., such as like prob
 *      is large enough).
 *
 * Postconditions:
 *      Vectors have been accumulated by f.
 *
 * Side Effects:
 *      Changes meanAccumulator and diagCovarAccumulator arrays.
 *
 * Results:
 *      nothing.
 *
 *-----------------------------------------------------------------------
 */
void 
DiagGaussian::fkIncrementMeanDiagCovar(const float fprob,
				       const float * const f,
				       const unsigned len,
				       float *curMeans,
				       float *curDiagCovars,
				       float *meanAccumulator,
				       float *diagCovarAccumulator)
{
  register const float * f_p = f;
  register const float *const f_p_endp = f + len;

  register float *mean_p = curMeans;
  register float *diagCovar_p = curDiagCovars;

  register float *meanAccumulator_p = meanAccumulator;
  register float *diagCovarAccumulator_p = diagCovarAccumulator;

  do {

    register float tmp = (*f_p - *mean_p)/(*diagCovar_p);

    // store the values in temporaries so that the
    // compiler knows that there is no aliasing.
    register float mean_val = fprob*tmp;
    tmp = tmp*tmp;
    register float covar_val = 0.5*fprob*(-1.0/(*diagCovar_p) + tmp);

    // increment the actual accumulators
    *meanAccumulator_p += mean_val;
    *diagCovarAccumulator_p += covar_val;

    // update all pointers

    mean_p++;
    diagCovar_p++;
    meanAccumulator_p++;
    diagCovarAccumulator_p++;
    f_p ++;
  } while (f_p != f_p_endp);

}


