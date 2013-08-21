/*-
 * GMTK_MissingFeatureScaledDiagGaussian.cc
 *        Code for plain vanilla diagonal Gaussians.
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

#include "GMTK_MissingFeatureScaledDiagGaussian.h"
#include "GMTK_GMParms.h"
#include "GMTK_MixtureCommon.h"
#include "tieSupport.h"



void
MissingFeatureScaledDiagGaussian::read(iDataStreamFile& is)
{
  // The index in the global mean array of this mean.
  int meanIndex; 
  // The index in the global variance array of this variance vector
  int covarIndex;


  // read name
  NamedObject::read(is);

  // read mean vector
  string str;
  is.read(str);

  if (GM_Parms.meansMap.find(str) ==  GM_Parms.meansMap.end()) 
      error("Error: MissingFeatureScaledDiagGaussian '%s' in file '%s' line %d specifies mean name '%s' that does not exist",
	    _name.c_str(),is.fileName(),is.lineNo(),str.c_str());
  meanIndex = GM_Parms.meansMap[str];
  mean = GM_Parms.means[meanIndex];
  mean->numTimesShared++;


  // read covariance vector
  is.read(str);
  if (GM_Parms.covarsMap.find(str) == GM_Parms.covarsMap.end())
    error("Error: MissingFeatureScaledDiagGaussian '%s' in file '%s' line %d specifies covar name '%s' that does not exist",
	  _name.c_str(),is.fileName(),is.lineNo(),str.c_str());
  
  covarIndex = GM_Parms.covarsMap[str];
  covar = GM_Parms.covars[covarIndex];
  covar->numTimesShared++;


  // read exponent vector
  is.read(str);
  if (GM_Parms.realMatsMap.find(str) ==  GM_Parms.realMatsMap.end()) 
      error("Error: MissingFeatureScaledDiagGaussian '%s' in file '%s' line %d specifies name '%s' that does not exist",
	    _name.c_str(),is.fileName(),is.lineNo(),str.c_str());
  // The index in the global array of this scale
  int scaleIndex; 
  scaleIndex = GM_Parms.realMatsMap[str];
  scale = GM_Parms.realMats[scaleIndex];


  // check that lengths match, etc.
  if (covar->dim() != mean->dim()) {
    error("Error: MissingFeatureScaledDiagGaussian '%s' in file '%s' line %d specifices a mean '%s' with dim %d and covariance '%s' with dim '%d'\n",
	  _name.c_str(),is.fileName(),is.lineNo(),
	  mean->name().c_str(),
	  mean->dim(),
	  covar->name().c_str(),
	  covar->dim());
  }
  if ((unsigned)covar->dim() != _dim) {
    error("Error: MissingFeatureScaledDiagGaussian '%s' in file '%s' line %d of dim %d does not match its mean '%s' with dim %d or covariance '%s' with dim '%d'\n",
	  _name.c_str(),is.fileName(),is.lineNo(),
	  _dim,
	  mean->name().c_str(),
	  mean->dim(),
	  covar->name().c_str(),
	  covar->dim());
  }

  if (scale->rows() != 1 || (unsigned)scale->cols() != _dim)
    error("Error: MissingFeatureScaledDiagGaussian '%s' in file '%s' line %d specifices a scale real matrix '%s' with rows,cols = %d, %d but rows must be 1 and columns must be %d.\n",
	  _name.c_str(),is.fileName(),is.lineNo(),
	  scale->name().c_str(),
	  scale->rows(),
	  scale->cols(),
	  _dim);


  setBasicAllocatedBit();
}


void
MissingFeatureScaledDiagGaussian::write(oDataStreamFile& os)
{
  assert ( basicAllocatedBitIsSet() );

  // write the type of self and the name
  os.write((int)Component::MissingFeatureScaledDiagGaussian);
  NamedObject::write(os);
  os.nl();

  // write mean vector
  os.write(mean->name());
  os.write(covar->name());

  // the scale vector is constant, not learned, and thus is not
  // written.

  os.nl();
}


/*-
 *-----------------------------------------------------------------------
 * MissingFeatureScaledDiagGaussian::makeRandom
 *      calls makeRandom for the mean and covar
 * 
 * Preconditions:
 *      Object must be allocated.
 *
 * Postconditions:
 *      Object has random values.
 *
 * Side Effects:
 *      destroys previous values.
 *
 * Results:
 *      none
 *
 *-----------------------------------------------------------------------
 */
void
MissingFeatureScaledDiagGaussian::makeRandom()
{
  assert ( basicAllocatedBitIsSet() );
  if (!emAmTrainingBitIsSet())
    return;
  mean->makeRandom();
  covar->makeRandom();
}


/*-
 *-----------------------------------------------------------------------
 * makeUniform()
 *      calls makeUniform for the mean and covar
 * 
 * Preconditions:
 *      Object must be allocated.
 *
 * Postconditions:
 *      Object has uniform values.
 *
 * Side Effects:
 *      destroys previous values.
 *
 * Results:
 *      none
 *
 *-----------------------------------------------------------------------
 */
void
MissingFeatureScaledDiagGaussian::makeUniform()
{
  assert ( basicAllocatedBitIsSet() );
  if (!emAmTrainingBitIsSet())
    return;

  mean->makeUniform();
  covar->makeUniform();
}


/*-
 *-----------------------------------------------------------------------
 * noisyClone()
 *      Create a copy of self, but with cloned perturbed the mean/variance vectors
 * 
 * Preconditions:
 *      Obj must be read in, and basicAllocatedBitIsSet() must be true.
 *
 * Postconditions:
 *      none.
 *
 * Side Effects:
 *      'this' is not changed at all. Allocates new memory though.
 *
 * Results:
 *      returns the new noisy mean.
 *
 *-----------------------------------------------------------------------
 */
Component*
MissingFeatureScaledDiagGaussian::noisyClone()
{
  assert ( basicAllocatedBitIsSet() );

  map<Component*,Component*>::iterator it = MixtureCommon::mcCloneMap.find(this);
  // first check if self is already cloned, and if so, return that.
  if (it == MixtureCommon::mcCloneMap.end()) {
    MissingFeatureScaledDiagGaussian* clone;
    clone = new MissingFeatureScaledDiagGaussian(dim());

    unsigned cloneNo=0; do {
      char buff[256];
      sprintf(buff,"%d",cloneNo);
      clone->_name = _name + string("_cl") + buff;
      cloneNo++;
    } while (GM_Parms.componentsMap.find(clone->_name) 
	     != GM_Parms.componentsMap.end());

    // sharing during training is not implemented (yet). Make sure the
    // user doesn't change code above and set these values.
    // assert (!cloneShareMeans && !cloneShareCovars);
    // if (cloneShareMeans || cloneShareCovars) {
    //   error("ERROR: MissingFeatureScaledDiagGaussian Component '%s' has shared either its mean '%s' or covariance '%s' during EM training, but shared traning is not (yet) implemented.",name().c_str(),mean->name().c_str(),covar->name().c_str());
    // }

    if (cloneShareMeans && cloneShareCovars) {
      // the issue here is that if the user wants to clone a gaussian that shares
      // both mean and covar, it would be identical and no reason to clone in the first
      // place.
      warning("WARNING: MissingFeatureScaledGaussian component '%s' is cloning, and was asked to share both means and covariances. No sharing is occuring instead.",name().c_str());
      clone->mean = mean->noisyClone();
      clone->covar = covar->noisyClone();
    } else {
      if (cloneShareMeans)
	clone->mean = mean;
      else
	clone->mean = mean->noisyClone();
      if (cloneShareCovars)
	clone->covar = covar;
      else
	clone->covar = covar->noisyClone();
    }

    // make a copy of the scale, note that this
    // copy is ok since the object is deleted in the global parameters
    // delete routine.
    clone->scale = scale;
    
    // need to tell mean, and covar that either
    //    1) if this is a new mean,covar that a
    //       a parent object is using them, or
    //    2) if this is a shared mean,covar that
    //       an additional parent object is using them.
    clone->mean->numTimesShared++;
    clone->covar->numTimesShared++;

    clone->setBasicAllocatedBit();
    MixtureCommon::mcCloneMap[this] = clone;


    // also add self to GMParms object.
    GM_Parms.add(clone);

    return clone;
  } else {
    return (*it).second;
  }
}



/*-
 *-----------------------------------------------------------------------
 * MissingFeatureScaledDiagGaussian::identicalIndependentClone
 *      creates an exact copy of this object that shares nothing with
 *      the original
 *
 * Preconditions:
 *      1) object being copied should be allocated
 *      2) GM_Parms should contain all parameters, so that a unique name
 *         for the new object can be generated
 *
 * Postconditions:
 *      none
 *
 * Side Effects:
 *      the new object is added to GM_Parms
 *
 * Results:
 *      a pointer the new object
 *
 *-----------------------------------------------------------------------
 */
Component* 
MissingFeatureScaledDiagGaussian::identicalIndependentClone()
{

  MissingFeatureScaledDiagGaussian* newDG = new MissingFeatureScaledDiagGaussian(dim());

  newDG->mean = mean->identicalIndependentClone();
  newDG->covar = covar->identicalIndependentClone();

  // make a copy of the scale, note that this
  // copy is ok since the object is deleted in the global parameters
  // delete routine.
  newDG->scale = scale;

  // don't change usage counts here - do it in calling function,
  // because only that knows how the sharing is arranged

  //newDG->mean->numTimesShared++;
  //newDG->covar->numTimesShared++;

  //mean->numTimesShared--;
  //covar->numTimesShared--;

  newDG->_name = new_name(name(),&GM_Parms.componentsMap);
  newDG->setBasicAllocatedBit();
  GM_Parms.add(newDG);

  return newDG;
}

/////////////////
// EM routines //
/////////////////


void
MissingFeatureScaledDiagGaussian::emStartIteration()
{
  assert ( basicAllocatedBitIsSet() );

  if (!emAmTrainingBitIsSet())
    return;

  if (emOnGoingBitIsSet())
    return;

  if (!emEmAllocatedBitIsSet()) {
    // this is presumably the first time
    trMembers.allocateIfNeeded();
    emSetEmAllocatedBit();
  }

  // EM iteration is now going.
  emSetOnGoingBit();
  emSetSwappableBit();

  accumulatedProbability = 0.0;
  mean->emStartIteration(trMembers->nextMeans);
  covar->emStartIteration(trMembers->nextDiagCovars);

  if (covar->emSharedBitIsSet() || mean->emSharedBitIsSet()) {
    // we need to initialize the accumulators within both the mean and
    // covariance, even if only one of the objects is being shared
    // since we fall bace to the code where both are shared if one of
    // them is shared.

    covar->initElementAccumulatedProbability();
    mean->initElementAccumulatedProbability();
 
  //     error("ERROR: MissingFeatureScaledDiagGaussian Component '%s' has shared either its mean '%s' or covariance '%s' during EM training, but shared traning is not (yet) implemented.",name().c_str(),mean->name().c_str(),covar->name().c_str());

  }

  // for this object, we need our own vector of accumulators, one for
  // each element of the Gaussian.
  trMembers->elementAccumulatedProbability.growIfNeeded(_dim);
  for (unsigned i =0;i<_dim;i++) {
    trMembers->elementAccumulatedProbability[i].set_to_zero();
  }

}





/*-
 *-----------------------------------------------------------------------
 * log_p()
 *      Computes the probability of this Gaussian, but in this case any value
 *      of the observation that is a NaN is treated as a missing feature (i.e.,
 *      the Gaussian integrates that feature out. Note that another option
 *      might be to use the mean value of the missing feature rather than
 *      integrating it out. It should still produce valid values, however, to
 *      do integration since any ratios or sums we do will always involve
 *      the same quantities (i.e., if we do \sum_i x p(x| i), then each of the 
 *      p(x|i) will always have the same element of x integrated out).
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
MissingFeatureScaledDiagGaussian::log_p(const float *const x,
		    const Data32* const base,
		    const int stride)
{

#if 0
  // check for normal Gaussian evaluation, 
  // TODO ultimately remove the code for this case.

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


  d *= -0.5;
  return logpr(0,(covar->log_inv_normConst() + d));


#else 

  assert ( basicAllocatedBitIsSet() );

  //////////////////////////////////////////////////////////////////
  // The local accumulator type in this routine.
  // This can be changed from 'float' to 'double' to
  // provide extra range for temporary accumulators. Alternatively,
  // decreasing the program's mixCoeffVanishRatio at the beginning
  // of training should eliminate any component that produces
  // such low scores.
#define DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE double

  // the number of features that are present (i.e., that do
  // not have a NaN value).
  unsigned numPresentFeatures = 0;
  // local computation of the partial determinant of the covariance
  // matrix for those features that are present.
  DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE det = 1.0;

  ////////////////////
  // note: 
  // covariances must have been precomputed for this
  // to work.
  const float *xp = x;
  const float *mean_p = mean->basePtr();
  const float *var_inv_p = covar->baseVarInvPtr();
  const float* scale_p = scale->values.ptr;


  // do the non-unrolled version only.
  const float *const x_endp = x + _dim;
  DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE d=0.0;
  do {
    if (!isnan(*xp)) {    

      const DIAG_GAUSSIAN_TMP_ACCUMULATOR_TYPE tmp
	= (*xp - *mean_p);
      d += (tmp*(*var_inv_p))*tmp*(*scale_p);

      // now we need to udpate the normalization constant specific for this
      // instance of not missing data. 
      numPresentFeatures ++;
      det *= covar->covariances[xp-x];

    } else {
      // now we need to udpate the normalization constant specific for this
      // instance of missing data. The assumption here is that
      // NAN indicates that the value is missing, so we should not
      // accumulate for this component. Also
    }

    xp++;
    mean_p++;
    var_inv_p++;
    scale_p++;
  } while (xp != x_endp);


  // now compute the local normalization score for this case.
  if (det <= DBL_MIN) {
    error("ERROR: determinant of missing feature diagonal covariance matrix '%s' has hit minimum and is not above threshold of %e. Possible causes include: 1) not enough training segments, or 2) data that is inappropriately scaled, or 3) too much pruning, or 4) impossible or infrequent state configurations, or 5) not large enough varFloor & floor on read command line args.",
	  DBL_MIN,
	  name().c_str());
  }
  const double tmp = (::pow(2*M_PI,numPresentFeatures/2.0)*::sqrt(det));
  if (tmp <= DBL_MIN)
    coredump("ERROR: norm const has hit maximum of diagonal covariance matrix '%s'",name().c_str());

  // some debugging printing.
  // if ((float)(-0.5*(numPresentFeatures*::log(2*M_PI) + ::log(det))) != covar->log_inv_normConst() ) {
  //   printf("not equal, log(2*M_PI) = %1.10e\n",::log(2*M_PI));
  //   printf("covar->log_inv_normConst() = %f\n",covar->log_inv_normConst());
  //   printf("-0.5*(numPresentFeatures*::log(2*M_PI) + ::log(det)) = %f\n",-0.5*(numPresentFeatures*::log(2*M_PI) + ::log(det)));
  //   assert( -0.5*(numPresentFeatures*::log(2*M_PI) + ::log(det)) == covar->log_inv_normConst() );
  // }


  d *= -0.5;
  return logpr(0, -0.5*(numPresentFeatures*::log(2*M_PI) + ::log(det)) + d);

#endif
}


void
MissingFeatureScaledDiagGaussian::emIncrement(logpr prob,
			  const float*f,
			  const Data32* const base,
			  const int stride)
{
  assert ( basicAllocatedBitIsSet() );
  assert ( emEmAllocatedBitIsSet() );
  if (!emAmTrainingBitIsSet())
    return;

  if (!emOnGoingBitIsSet())
    emStartIteration();

  if (prob < minIncrementProbabilty) {
    missedIncrementCount++;
    return;
    // don't accumulate anything since this one is so small and
    // if we did an unlog() and converted to a single precision
    // floating point number, it would be a denomral.
  }

  accumulatedProbability += prob;
  // prob.unlog() here so it doesn't need to be done
  // twice by the callees.
  const float fprob = prob.unlog();

  mean->emIncrement(prob,fprob,f,base,stride,trMembers->nextMeans.ptr);
  covar->emIncrement(prob,fprob,f,base,stride,trMembers->nextDiagCovars.ptr);

  // these next calls are presumed to be valid for both EM and FK (fisher kernel accumulation).
  if (covar->emSharedBitIsSet() || mean->emSharedBitIsSet()) {
    // the mean and/or the covar is being shared so we need to use
    // the routines that involve the element specific accumulated probability
    // for each of the mean and covariance.
    
    if (!fisherKernelMode) {
      // do normal EM increment
      // this call is optimized to do the 1st and 2nd moment stats simultaneously
      emIncrementMeanDiagCovar(prob,fprob,f,
			       trMembers->nextMeans.size(),
			       trMembers->nextMeans.ptr,trMembers->nextDiagCovars.ptr,
			       mean->trMembers->elementAccumulatedProbability.ptr,
			       covar->trMembers->elementAccumulatedProbability.ptr);
    } else {
      // do a fisher score increment
      fkIncrementMeanDiagCovar(prob,fprob,f,
			       trMembers->nextMeans.size(),
			       mean->means.ptr,
			       covar->covariances.ptr,
			       trMembers->nextMeans.ptr,
			       trMembers->nextDiagCovars.ptr,
			       mean->trMembers->elementAccumulatedProbability.ptr,
			       covar->trMembers->elementAccumulatedProbability.ptr);
    }
  } else {
    // no sharing going on with the current mean and covariance which
    // means that the mean and covariance do not need to have their
    // own element-specific accumulated probability.

    if (!fisherKernelMode) {
      // do normal EM increment
      // this call is optimized to do the 1st and 2nd moment stats simultaneously
      emIncrementMeanDiagCovar(prob,fprob,f,trMembers->nextMeans.size(),trMembers->nextMeans.ptr,trMembers->nextDiagCovars.ptr);
    } else {
      // do a fisher score increment
      fkIncrementMeanDiagCovar(prob,
			       fprob,f,trMembers->nextMeans.size(),
			       mean->means.ptr,
			       covar->covariances.ptr,
			       trMembers->nextMeans.ptr,
			       trMembers->nextDiagCovars.ptr);
    }
  }


}




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
MissingFeatureScaledDiagGaussian::emIncrementMeanDiagCovar(
				       logpr prob,
				       const float fprob,
				       const float * const f,
				       const unsigned len,
				       float *meanAccumulator,
				       float *diagCovarAccumulator)
{
  assert ( emEmAllocatedBitIsSet() );

  register const float * f_p = f;
  register const float *const f_p_endp = f + len;
  register float *meanAccumulator_p = meanAccumulator;
  register float *diagCovarAccumulator_p = diagCovarAccumulator;
  logpr* acp = trMembers->elementAccumulatedProbability.ptr;

  do {
    // a version of the above code that avoids aliasing of f_p and
    // meanAccumulator_p so the compiler can probably optimize better.
    
    if (!isnan(*f_p)) {
      register float tmp = (*f_p)*fprob;
      register float tmp2 = tmp*(*f_p);

      *meanAccumulator_p += tmp;
      *diagCovarAccumulator_p += tmp2;

      *acp += prob;
    } else {
      // we've got a nan in the observation stream, and we use that as
      // a special flag to indicate that we should not accumulate at
      // all, nor do we increment the denominator 'acp' for this
      // element.
    }

    // update pointers to go to next element.
    acp++;
    meanAccumulator_p++;
    diagCovarAccumulator_p++;
    f_p ++;

  } while (f_p != f_p_endp);
}

void 
MissingFeatureScaledDiagGaussian::emIncrementMeanDiagCovar(
				       logpr prob,
				       const float fprob,
				       const float * const f,
				       const unsigned len,
				       float *meanAccumulator,
				       float *diagCovarAccumulator,
				       logpr* extra_prob_accum_1,
				       logpr* extra_prob_accum_2)
{
  assert ( emEmAllocatedBitIsSet() );

  register const float * f_p = f;
  register const float *const f_p_endp = f + len;
  register float *meanAccumulator_p = meanAccumulator;
  register float *diagCovarAccumulator_p = diagCovarAccumulator;
  logpr* acp = trMembers->elementAccumulatedProbability.ptr;
  logpr* ex1p = extra_prob_accum_1;
  logpr* ex2p = extra_prob_accum_2;  

  do {
    // a version of the above code that avoids aliasing of f_p and
    // meanAccumulator_p so the compiler can probably optimize better.
    
    if (!isnan(*f_p)) {
      register float tmp = (*f_p)*fprob;
      register float tmp2 = tmp*(*f_p);

      *meanAccumulator_p += tmp;
      *diagCovarAccumulator_p += tmp2;

      *ex1p += prob;  *ex2p += prob;
      *acp += prob;
    } else {
      // we've got a nan in the observation stream, and we use that as
      // a special flag to indicate that we should not accumulate at
      // all, nor do we increment the denominator 'acp' for this
      // element.
    }

    // update pointers to go to next element.
    ex1p++; ex2p++;
    acp++;
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
MissingFeatureScaledDiagGaussian::fkIncrementMeanDiagCovar(
				       logpr prob,
				       const float fprob,
				       const float * const f,
				       const unsigned len,
				       float *curMeans,
				       float *curDiagCovars,
				       float *meanAccumulator,
				       float *diagCovarAccumulator)
{
  assert ( emEmAllocatedBitIsSet() );

  register const float * f_p = f;
  register const float *const f_p_endp = f + len;

  register float *mean_p = curMeans;
  register float *diagCovar_p = curDiagCovars;

  register float *meanAccumulator_p = meanAccumulator;
  register float *diagCovarAccumulator_p = diagCovarAccumulator;

  logpr* acp = trMembers->elementAccumulatedProbability.ptr;

  do {
    if (!isnan(*f_p)) {
      register float tmp = (*f_p - *mean_p)/(*diagCovar_p);

      // store the values in temporaries so that the
      // compiler knows that there is no aliasing.
      register float mean_val = fprob*tmp;
      tmp = tmp*tmp;
      register float covar_val = 0.5*fprob*(-1.0/(*diagCovar_p) + tmp);
      
      // increment the actual accumulators
      *meanAccumulator_p += mean_val;
      *diagCovarAccumulator_p += covar_val;

      *acp += prob;      
    }

    // update all pointers
    acp++;
    mean_p++;
    diagCovar_p++;
    meanAccumulator_p++;
    diagCovarAccumulator_p++;
    f_p ++;
  } while (f_p != f_p_endp);

}


void 
MissingFeatureScaledDiagGaussian::fkIncrementMeanDiagCovar(
				       logpr prob,
				       const float fprob,
				       const float * const f,
				       const unsigned len,
				       float *curMeans,
				       float *curDiagCovars,
				       float *meanAccumulator,
				       float *diagCovarAccumulator,
				       logpr* extra_prob_accum_1,
				       logpr* extra_prob_accum_2)
{
  assert ( emEmAllocatedBitIsSet() );

  register const float * f_p = f;
  register const float *const f_p_endp = f + len;

  register float *mean_p = curMeans;
  register float *diagCovar_p = curDiagCovars;

  register float *meanAccumulator_p = meanAccumulator;
  register float *diagCovarAccumulator_p = diagCovarAccumulator;

  logpr* acp = trMembers->elementAccumulatedProbability.ptr;
  logpr* ex1p = extra_prob_accum_1;
  logpr* ex2p = extra_prob_accum_2;  


  do {
    if (!isnan(*f_p)) {
      register float tmp = (*f_p - *mean_p)/(*diagCovar_p);

      // store the values in temporaries so that the
      // compiler knows that there is no aliasing.
      register float mean_val = fprob*tmp;
      tmp = tmp*tmp;
      register float covar_val = 0.5*fprob*(-1.0/(*diagCovar_p) + tmp);
      
      // increment the actual accumulators
      *meanAccumulator_p += mean_val;
      *diagCovarAccumulator_p += covar_val;

      *ex1p += prob;  *ex2p += prob;
      *acp += prob;
    }

    // update all pointers
    ex1p++; ex2p++;
    acp++;
    mean_p++;
    diagCovar_p++;
    meanAccumulator_p++;
    diagCovarAccumulator_p++;
    f_p ++;
  } while (f_p != f_p_endp);

}



void
MissingFeatureScaledDiagGaussian::emEndIteration()
{
  assert ( basicAllocatedBitIsSet() );
  assert ( emEmAllocatedBitIsSet() );

  if (!emAmTrainingBitIsSet())
    return;

  // if EM not ongoing, we just return
  // here since, if this object is shared,
  // we don't want to end the epoch > 1 time
  if (!emOnGoingBitIsSet())
    return; 

  accumulatedProbability.floor();
  if (accumulatedProbability.zero()) {
    // Note: we assume here that the mean and covar object will check for us that 
    // its accumulated probability is above threshold. Here, we just
    // check for zero, and then issue a warning if needed. This might not
    // indicate a problem as the mean and covar of this object might be shared
    // and might have received plenty of count.
    warning("WARNING: Gaussian Component named '%s' did not receive any accumulated probability in EM iteration. Global missed increment count is %d. Also check child mean '%s' and covar '%s'",
	    name().c_str(),
	    missedIncrementCount,
	    mean->name().c_str(),
	    covar->name().c_str());
  }


  // TODO: Note that none of the sharing code should execute right now
  // since it should be being checked at the start of EM
  // iteration. Therefore, we add a bunch of asserts for the
  // conditions that are not yet implemented.

  if (covar->emSharedBitIsSet()) {
    // covariance is shared
    if (mean->emSharedBitIsSet()) {
      // mean and covariance are both shared
      mean->emEndIterationSharedMeansCovarsElementProbabilities
	(accumulatedProbability,trMembers->nextMeans.ptr,covar,
	 trMembers->elementAccumulatedProbability.ptr);
      covar->emEndIterationSharedMeansCovarsElementProbabilities
	(accumulatedProbability,
	 trMembers->nextMeans.ptr,
	 trMembers->nextDiagCovars.ptr,
	 mean,
	 trMembers->elementAccumulatedProbability.ptr);
    } else {
      // Mean is not shared, but covariance is shared
      // can still use normal EM
      mean->emEndIterationNoSharingElementProbabilities(trMembers->nextMeans.ptr,
							trMembers->elementAccumulatedProbability.ptr);
      covar->emEndIterationSharedCovarsElementProbabilities(accumulatedProbability,
							    trMembers->nextMeans.ptr,
							    trMembers->nextDiagCovars.ptr,
							    trMembers->elementAccumulatedProbability.ptr);
    }
  } else {
    // covariance is not shared
    if (mean->emSharedBitIsSet()) {
      // covariance is not shared, mean is shared,
      // but use the "all shared" versions since we don't
      // have the new mean at this point.
      mean->emEndIterationSharedMeansCovarsElementProbabilities
	(accumulatedProbability,trMembers->nextMeans.ptr,covar,
	 trMembers->elementAccumulatedProbability.ptr);
      covar->emEndIterationSharedMeansCovarsElementProbabilities
	(accumulatedProbability,
	 trMembers->nextMeans.ptr,
	 trMembers->nextDiagCovars.ptr,
	 mean,
	 trMembers->elementAccumulatedProbability.ptr);
    } else {
      // nothing is shared, use no-sharing version of end EM
      mean->emEndIterationNoSharingElementProbabilities(trMembers->nextMeans.ptr,
							trMembers->elementAccumulatedProbability.ptr);
      covar->emEndIterationNoSharingElementProbabilities(trMembers->nextMeans.ptr,
							 trMembers->nextDiagCovars.ptr,
							 trMembers->elementAccumulatedProbability.ptr);
    }
  }

  emClearOnGoingBit();
}


void
MissingFeatureScaledDiagGaussian::emSwapCurAndNew()
{
  assert ( basicAllocatedBitIsSet() );
  if (!emAmTrainingBitIsSet())
    return;

  if (!emSwappableBitIsSet())
    return;

  mean->emSwapCurAndNew();
  covar->emSwapCurAndNew();

  emClearSwappableBit();
}


////////////////////////////////////////////////////////////
// Parallel EM support
////////////////////////////////////////////////////////////


void
MissingFeatureScaledDiagGaussian::emStoreObjectsAccumulators(oDataStreamFile& ofile,
					 bool writeLogVals,
					 bool writeZeros)
{
  assert ( emEmAllocatedBitIsSet() );

  // since this is a Gaussian, we ignore the writeLogVals
  // argument since it doesn't make sense to take log of
  // these values since they are continuous, could be negative, etc.
  if (writeZeros) {
    const unsigned totalLen = trMembers->nextMeans.len() + trMembers->nextDiagCovars.len();
    for (unsigned i=0;i<totalLen;i++) {
      ofile.write(0.0,"MissingFeatureScaledDiagGaussian store accums nm + nc.");
    }
    for (int i=0;i<trMembers->elementAccumulatedProbability.len();i++) {
      ofile.write(trMembers->elementAccumulatedProbability[0].valref()*0.0,"El Acc Prob 0.0");
    }
  } else {
    for (int i=0;i<trMembers->nextMeans.len();i++) {
      ofile.write(trMembers->nextMeans[i],"MissingFeatureScaledDiagGaussian store accums nm.");
    }
    for (int i=0;i<trMembers->nextDiagCovars.len();i++) {
      ofile.write(trMembers->nextDiagCovars[i],"MissingFeatureScaledDiagGaussian store accums nc.");
    }
    for (int i=0;i<trMembers->elementAccumulatedProbability.len();i++) {
      ofile.write(trMembers->elementAccumulatedProbability[i].valref(),"El Acc Prob i");
    }
  }
}


void
MissingFeatureScaledDiagGaussian::emLoadObjectsDummyAccumulators(iDataStreamFile& ifile)
{
  assert ( emEmAllocatedBitIsSet() );
  //
  // ASSUME MEANS AND COVARIANCES ARE OF TYPE FLOAT
  // See the MeanVector.h and DiagcovarVector.h for specifics.
  float tmp;
  for (int i=0;i<mean->dim();i++) {
    ifile.read(tmp,"MissingFeatureScaledDiagGaussian load accums nm.");
  }
  for (int i=0;i<covar->dim();i++) {
    ifile.read(tmp,"MissingFeatureScaledDiagGaussian load accums nc.");
  }
  logpr tmp2;
  for (int i=0;i<trMembers->elementAccumulatedProbability.len();i++) {
    ifile.read(tmp2.valref(),"MissingFeatureScaledDiagGaussian load el ac prob.");
  }
}


void
MissingFeatureScaledDiagGaussian::emZeroOutObjectsAccumulators()
{
  assert ( emEmAllocatedBitIsSet() );
  for (int i=0;i<trMembers->nextMeans.len();i++) {
    trMembers->nextMeans[i] = 0.0;
  }
  for (int i=0;i<trMembers->nextDiagCovars.len();i++) {
    trMembers->nextDiagCovars[i] = 0.0;
  }
  for (int i=0;i<trMembers->elementAccumulatedProbability.len();i++) {
    trMembers->elementAccumulatedProbability[i].set_to_zero();
  }
}

void
MissingFeatureScaledDiagGaussian::emLoadObjectsAccumulators(iDataStreamFile& ifile)
{
  assert ( emEmAllocatedBitIsSet() );
  for (int i=0;i<trMembers->nextMeans.len();i++) {
    ifile.read(trMembers->nextMeans[i],"MissingFeatureScaledDiagGaussian load accums nm.");
  }
  for (int i=0;i<trMembers->nextDiagCovars.len();i++) {
    ifile.read(trMembers->nextDiagCovars[i],"MissingFeatureScaledDiagGaussian load accums nc.");
  }
  for (int i=0;i<trMembers->elementAccumulatedProbability.len();i++) {
    ifile.read(trMembers->elementAccumulatedProbability[i].valref(),"MissingFeatureScaledDiagGaussian el ac pr.");
  }

}


void
MissingFeatureScaledDiagGaussian::emAccumulateObjectsAccumulators(iDataStreamFile& ifile)
{
  assert ( emEmAllocatedBitIsSet() );
  //
  // ASSUME MEANS AND COVARIANCES ARE OF TYPE FLOAT
  // See the MeanVector.h and DiagcovarVector.h for specifics.
  for (int i=0;i<trMembers->nextMeans.len();i++) {
    float tmp;
    ifile.read(tmp,"MissingFeatureScaledDiagGaussian accumulate accums nm.");
    trMembers->nextMeans[i] += tmp;
  }
  for (int i=0;i<trMembers->nextDiagCovars.len();i++) {
    float tmp;
    ifile.read(tmp,"MissingFeatureScaledDiagGaussian accumulate accums nc.");
    trMembers->nextDiagCovars[i] += tmp;
  }

  for (int i=0;i<trMembers->elementAccumulatedProbability.len();i++) {
    logpr tmp2;
    ifile.read(tmp2.valref(),"MissingFeatureScaledDiagGaussian load el ac prob.");
    trMembers->elementAccumulatedProbability[i] += tmp2;
  }
}


////////////////////////////////////////////////////////////
// Sample generation
////////////////////////////////////////////////////////////



void MissingFeatureScaledDiagGaussian::sampleGenerate(float *const sample,
				  const Data32* const base,
				  const int stride)
{
  error("not implemented");
}


