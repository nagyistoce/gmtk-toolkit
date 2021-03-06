/*-
 * GMTK_VECPT.h
 *
 *   A "virtual evidence" CPT class. This CPT gives GMTK the ability
 *   to do virtual evidence, where the probabilties come from an
 *   observed file.  Also, this gives GMTK the ability to do Hybrid
 *   ANN/HMM speech recognition (ANN = artificial neural network), aka
 *   Morgan/Bourlard. I.e., with this CPT, GMTK can act as a hybrid
 *   system decoder. This CPT also allows GMTK to support hybrid
 *   ANN/DBN and/or hybird SVM/DBN systems. See the GMTK documentation
 *   for more details.
 *
 *   Specifically, this CPT can be used for a binary variable C that
 *   has exactly one parent A.  The CPT takes probabilities from a
 *   file which correspond to P(C=1|A=a). The file (a standard
 *   observation file, so it can be a pfile, htk file, ascii, etc.)
 *   contains either all floats (so each row must be the same as
 *   card(A)) or contains floats and ints, where the ints give the
 *   values of A for which probability is given (everything else not
 *   given in this case it is assumed that P(C=1|A=a) = 0). Note that
 *   C need not be observed, although it typically is to provide
 *   virtual online evidence to variable A.  In this way, the actual
 *   floats in the file need not even be probabilties, they can be
 *   arbitrary scores. If C is hidden, however, then the values of
 *   P(C=0|A=a) are taken to be 1-P(C=0|A=a), so in such a case (when
 *   C is hidden) it is more sensible for the scores for each a to be
 *   actual values between 0 and 1.
 *
 *  Written by Jeff Bilmes <bilmes@ee.washington.edu>
 * 
 *  $Header$
 * 
 * Copyright (C) 2004 Jeff Bilmes
 * Licensed under the Open Software License version 3.0
 * See COPYING or http://opensource.org/licenses/OSL-3.0
 *
 *
 */


#ifndef GMTK_VECPT_H
#define GMTK_VECPT_H

#include "fileParser.h"
#include "logp.h"

#include "sArray.h"

#include "GMTK_CPT.h"
#include "GMTK_EMable.h"
#include "GMTK_RV.h"
#include "GMTK_NamedObject.h"
#if 0
#  include "GMTK_ObservationMatrix.h"
#else
#  include "GMTK_ObservationSource.h"
#  include "GMTK_FileSource.h"
#endif
// we need to interface to the external global observation
// matrix object to get some parametes (such as start skip, end skip,
// and the length of each utterance to ensure lengths are the same).
#if 0
extern ObservationMatrix globalObservationMatrix;
#else
extern ObservationSource *globalObservationMatrix;
#endif
class VECPT : public CPT {

  // the mode. Dense means that we have a score in the obs file for
  // all parent values. Sparse means that the obs file consists of
  // floats and same number of ints which give DiscRV value for each
  // corresponding float.
  enum VeMode { VE_Dense, VE_Sparse };
  VeMode veMode;

  // The observation file where the probabilities are taken from
  // (constructor creates an empty object). Note that these
  // observation temporal lengths here must exactly match that of the
  // observation file.
#if 0
  ObservationMatrix *obs;
#else
  ObservationSource *obs;
#endif

  // TODO: redo all this so that it works well
  // with obs.openFile(). Either change to char* and make
  // proper destructor, or something else.

  // The observation file name.
  string obsFileName;
  // number of floats expeced in the file
  unsigned nfs;
  // number of ints expeced in the file
  unsigned nis;
  // float range string in file
  string frs;
  // int range string in file  
  string irs;
  // per-segment range string
  char* pr_rs;
  // format string "pfile", "htk", "ascii", or "binary"
  string fmt;
  // Endian swap condition for observation files.
  bool iswp;
  // Frame padding
  unsigned leftPad, rightPad;

  // Observation Matrix transforms
  char* preTransforms;
  char* postTransforms;
  
  char* sentRange;

  // these variables are used in case that we
  // get the VECPT information from the global
  // observation matrix directly, rather than
  // from a local observation file here. Otherwise,
  // this variable is set to zero. We have
  // two offsets, one for floats and one for ints.
  unsigned obs_file_foffset;
  unsigned obs_file_ioffset;

  ////////////////
  // current parent value
  unsigned curParentValue;

  ////////////////
  // the value
  int _val;

public:

  ///////////////////////////////////////////////////////////  
  // General constructor, 
  // VECPTs always have one parent, and a binary child.
  VECPT() : CPT(di_VECPT)
  { _numParents = 1; _card = 2; cardinalities.resize(_numParents); }
  ~VECPT() { }

  // a VECPT is considered iterable since its implementation can
  // change not only from segment to segment, but even within a
  // segment.
  bool iterable() { return true; }

  ///////////////////////////////////////////////////////////    
  // Semi-constructors: useful for debugging.
  // See parent class for further documention.
  void setNumParents(const int _nParents) {
    assert ( _nParents == 1 );
    CPT::setNumParents(_nParents);
    bitmask &= ~bm_basicAllocated;
  }
  void setNumCardinality(const int var, const int card) {
    if (var == 1) {
      // setting child card.
      assert ( card == 2);
      CPT::setNumCardinality(var,card);
    } else if (var == 0) {
      // setting parent cardinality, which can be anything, so no check.
      CPT::setNumCardinality(var,card);
    } else {
      // we're never allowed to have more than 1 parent in this case.
      assert (0);
    }
  }
  void allocateBasicInternalStructures() {
    error("VECPT::allocateBasicInternalStructures() not implemented");
  }

  ///////////////////////////////////////////////////////////  
  // Probability evaluation, compute Pr( child | parents ), and
  // iterator support. See GMTK_CPT.h for documentation.
  void becomeAwareOfParentValues( vector <RV *>& parents,
				  const RV* rv );
  void begin(iterator& it,DiscRV* drv, logpr& p);
  void becomeAwareOfParentValuesAndIterBegin(vector < RV *>& parents, 
					     iterator &it, 
					     DiscRV* drv,
					     logpr& p);
  logpr probGivenParents(vector <RV *>& parents,
			 DiscRV * drv);
  bool next(iterator &it,logpr& p);

  ///////////////////////////////////
  // random sample given current parent value.
  int randomSample(DiscRV* drv);

  ////////////////////////////////////////////////////////////////////
  // How many parameters does this consume? We return 0 since the
  // virtual evidence does not constitute parameters in the normal
  // sense.
  unsigned totalNumberParameters() { return 0; }

  ///////////////////////////////////////////////////////////  
  // These routines are no-ops in this case since all
  // distributions come from files.
  void normalize() {}
  // set all values to random values.
  void makeRandom() {}
  // set all values to uniform values.
  void makeUniform() {}

  //////////////////////////////////////////////
  // read/write basic parameters
  void read(iDataStreamFile& is);
  void write(oDataStreamFile& os);

  //////////////////////////////////
  // Public interface support for EM
  //////////////////////////////////
  void emStartIteration() {}
  void emIncrement(logpr p,vector <RV*>& parents,RV*rv) {}
  void emEndIteration() {}
  void emSwapCurAndNew() {}

  // parallel training (VECPTs are never trained)
  void emStoreObjectsAccumulators(oDataStreamFile& ofile,
				  bool writeLogVals = true,
				  bool writeZeros = false) {}
  void emLoadObjectsDummyAccumulators(iDataStreamFile& ifile) {}
  void emZeroOutObjectsAccumulators() {}
  void emLoadObjectsAccumulators(iDataStreamFile& ifile) {}
  void emAccumulateObjectsAccumulators(iDataStreamFile& ifile) {}
  const string typeName() { return "VirtualEvidenceCPT"; }
  //////////////////////////////////

  // support to change the segment number
  void setSegment(const unsigned segNo) {
#if 0
    obs->loadSegment(segNo);
#else
    obs->openSegment(segNo);
#endif
  }
  unsigned numFrames() {
    // return the number of frames in the current segment.
    return obs->numFrames();
  }

};



#endif // defined MDCPT
