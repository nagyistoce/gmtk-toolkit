/*
 * GMTK_Sw_HidDiscRV.h
 *
 *  Observed Discrete Random Variables with switching parent functionality.
 *
 * Written by Jeff Bilmes <bilmes@ee.washington.edu>
 *  $Header$
 *
 * Copyright (C) 2001 Jeff Bilmes
 * Licensed under the Open Software License version 3.0
 * See COPYING or http://opensource.org/licenses/OSL-3.0
 *
 *
 *
 * The discrete random variable type.
 *
 *
 *
 */

#ifndef GMTK_SW_HID_DISC_RV_H
#define GMTK_SW_HID_DISC_RV_H

#include <vector>

#include "GMTK_HidDiscRV.h"
#include "GMTK_SwDiscRV.h"

class Sw_HidDiscRV : public HidDiscRV, public SwDiscRV {
  friend class FileParser;
  friend class CPT;
  friend class MDCPT;
  friend class MSCPT;
  friend class MTCPT;


  virtual void setParents(vector<RV *> &sparents,vector<vector<RV *> > &cpl) {
    setSwitchingConditionalParents(sparents,cpl,this,allParents);
  }
  virtual void setDTMapper(RngDecisionTree *dt) {
    dtMapper = dt;
  }
  virtual vector< RV* >& condParentsVec(unsigned j) {
    assert ( j < conditionalParentsList.size() );
    return conditionalParentsList[j];
  }
  virtual vector< RV* >& switchingParentsVec() {
    return switchingParents;
  }
  virtual void setCpts(vector<CPT*> &cpts) {
    conditionalCPTs = cpts;
  }

protected:

public:

  /////////////////////////////////////////////////////////////////////////
  // constructor: Initialize with the variable type.  The default
  // timeIndex value of ~0x0 indicates a static network.  Discrete
  // nodes must be specified with their cardinalities.
  Sw_HidDiscRV(RVInfo& _rv_info,
	       unsigned _timeFrame = ~0x0,
	       unsigned _cardinality = 0)
    : HidDiscRV(_rv_info,_timeFrame,_cardinality)
  {
  }

  void printSelf(FILE*f,bool nl=true);
  void printSelfVerbose(FILE*f);

  inline virtual void probGivenParents(logpr& p) {
    setCurrentConditionalParents(this);
    curCPT = conditionalCPTs[cachedSwitchingState];
    p = curCPT->probGivenParents(*curConditionalParents,this);
  }

  logpr maxValue() {
    // printf("Sw_HidDiscRV::maxvalue() called\n");
    return SwDiscRV::maxValue();
  }

  inline virtual void begin(logpr& p) {
    setCurrentConditionalParents(this);
    curCPT = conditionalCPTs[cachedSwitchingState];
    curCPT->becomeAwareOfParentValuesAndIterBegin(*curConditionalParents,it,this,p);
    return;
  }

  virtual bool next(logpr& p) { 
    return curCPT->next(it,p);
  }

  // This function assumes that::
  //   1) deterministic() is true
  //   2) the curCPT is a deterministic CPT
  // If these conditions are not true, calling this function will yield a run-time error.
  // TODO: have some form of currentlyDeterministic() function, which based on
  // the current switching parents values, checks if the current CPT is deterministic.
  void assignDeterministicChild() { 
    setCurrentConditionalParents(this);
    curCPT = conditionalCPTs[cachedSwitchingState];
    curCPT->assignDeterministicChild(*curConditionalParents,this); 
  }

  void emIncrement(logpr posterior) {
    setCurrentConditionalParents(this);
    curCPT = conditionalCPTs[cachedSwitchingState];
    curCPT->emIncrement(posterior,*curConditionalParents,this);
  }


  unsigned averageCardinality() { return SwDiscRV::averageCardinality(rv_info); }
  unsigned maxCardinality() { return SwDiscRV::maxCardinality(rv_info); }


  virtual Sw_HidDiscRV* cloneRVShell();
  virtual Sw_HidDiscRV* create() {
    Sw_HidDiscRV*rv = new Sw_HidDiscRV(rv_info,frame(),cardinality);
    return rv;
  }

  virtual void randomSample() {
    setCurrentConditionalParents(this);
    curCPT->becomeAwareOfParentValues( *curConditionalParents, this );
    curCPT->randomSample(this); 
  }


  bool iterable() const {
    return iterableSw();
  }

  ////////////////////////////////////////////////////////////////////////
  // Increment the statistics with probabilty 'posterior' for the
  // current random variable's parameters, for the case where
  // the random variable and its parents are set to their current
  // values (i.e., the increment corresponds to the currently set
  // parent/child values). 


};


#endif
