/*-
 * GMTK_GMParms.cc
 *        All parameters associated with a GM.
 *
 *
 * Written by Jeff Bilmes <bilmes@ee.washington.edu>
 *
 * Copyright (c) 2001, < fill in later >
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any non-commercial purpose
 * and without fee is hereby granted, provided that the above copyright
 * notice appears in all copies.  The University of Washington,
 * Seattle, and Jeff Bilmes make no representations about
 * the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 */



#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ieeefp.h>
#include <float.h>
#include <assert.h>

#include "general.h"
#include "error.h"

#include "GMTK_GMParms.h"

#include "GMTK_Dense1DPMF.h"
#include "GMTK_Sparse1DPMF.h"
#include "GMTK_MeanVector.h"
#include "GMTK_DiagCovarVector.h"
#include "GMTK_RealMatrix.h"
#include "GMTK_PackedSparseRealMatrix.h"
#include "GMTK_DlinkMatrix.h"
#include "GMTK_Dlinks.h"
#include "GMTK_WeightMatrix.h"
#include "GMTK_MDCPT.h"
#include "GMTK_MSCPT.h"
#include "GMTK_MTCPT.h"

#include "GMTK_GaussianComponent.h"
#include "GMTK_DiagGaussian.h"

#include "GMTK_MixGaussiansCommon.h"
#include "GMTK_MixGaussians.h"


VCID("$Header$");

/////////////////////////////////
// an integer that specifies the maximum number of objects (such
// as means, covariances, DTs, etc.) that may be specified at
// one time in a file. This can be safely increased (to the
// extend that memory on the machine exists), but is here
// many for checking obviously invalid values.
const unsigned GMPARMS_MAX_NUM = 100000;


////////////////////////////////
// Magic String definitions
#define MAGIC_DT_FILE "GMTK_DT_FILE"
#define MAGIC_PRM_FILE "GMTK_PRM_FILE"


////////////////////////////////////////////////////////////////////
//        General create, read, destroy routines 
////////////////////////////////////////////////////////////////////

GMParms::GMParms()
{
  emTrainBitmask = emDefaultState;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//        READING Routines
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


/*-
 *-----------------------------------------------------------------------
 * readDPmfs
 *      Read in the dense PMF functions from a file.
 * 
 * Preconditions:
 *      No conditions.
 *
 * Postconditions:
 *      All dense PMFs from file are now contained in internal arrays.
 *
 * Side Effects:
 *      Modifies internal arrays of object.
 *
 * Results:
 *      none
 *
 *-----------------------------------------------------------------------
 */
void 
GMParms::readDPmfs(iDataStreamFile& is, bool reset)
{
  unsigned num;
  unsigned cnt;
  unsigned start = 0;

  is.read(num,"num dPMFs");
  if (num > GMPARMS_MAX_NUM) error("ERROR: number of dense PMFs (%d) in file '%s' exceeds maximum",num,is.fileName());
  if (reset) {
    start = 0;
    dPmfs.resize(num);
  } else {
    start = dPmfs.size();
    dPmfs.resize(start+num);
  }
  for (unsigned i=0;i<num;i++) {
    // first read the count
    Dense1DPMF* ob;

    is.read(cnt,"DPMF cnt");
    if (cnt != i) 
      error("ERROR: dense PMF count (%d), out of order in file '%s', expecting %d",
	    cnt,is.fileName(),i);

    ob = new Dense1DPMF;
    ob->read(is);
    if (dPmfsMap.find(ob->name()) != dPmfsMap.end())
      error("ERROR: dense PMF named '%s' specified more than once in file '%s'",ob->name().c_str(),is.fileName());
    dPmfs[i+start] = ob;
    dPmfsMap[ob->name()] = i+start;
  }
}


/*-
 *-----------------------------------------------------------------------
 * readSPmfs
 *      Read in the sparse PMF functions from a file.
 * 
 * Preconditions:
 *      No conditions.
 *
 * Postconditions:
 *      All sparse PMFs from file are now contained in internal arrays.
 *
 * Side Effects:
 *      Modifies internal arrays of object.
 *
 * Results:
 *      none
 *
 *-----------------------------------------------------------------------
 */
void 
GMParms::readSPmfs(iDataStreamFile& is, bool reset)
{
  unsigned num;
  unsigned cnt;
  unsigned start = 0;

  is.read(num,"num sPMFs");
  if (num > GMPARMS_MAX_NUM) error("ERROR: number of sparse PMFs (%d) in file '%s' exceeds maximum",num,is.fileName());
  if (reset) {
    start = 0;
    sPmfs.resize(num);
  } else {
    start = sPmfs.size();
    sPmfs.resize(start+num);
  }
  for (unsigned i=0;i<num;i++) {
    // first read the count
    Sparse1DPMF* ob;

    is.read(cnt,"SPMF cnt");
    if (cnt != i) 
      error("ERROR: dense PMF count (%d), out of order in file '%s', expecting %d",
	    cnt,is.fileName(),i);

    ob = new Sparse1DPMF;
    ob->read(is);
    if (sPmfsMap.find(ob->name()) != sPmfsMap.end())
      error("ERROR: sparse PMF named '%s' specified more than once in file '%s'",ob->name().c_str(),is.fileName());
    sPmfs[i+start] = ob;
    sPmfsMap[ob->name()] = i+start;
  }
}


/*-
 *-----------------------------------------------------------------------
 * readMeans
 *      Read in the mean from a file.
 * 
 * Preconditions:
 *      No conditions.
 *
 * Postconditions:
 *      All means from file are now contained in internal arrays.
 *
 * Side Effects:
 *      Modifies internal arrays of object.
 *
 * Results:
 *      none
 *
 *-----------------------------------------------------------------------
 */
void 
GMParms::readMeans(iDataStreamFile& is, bool reset)
{
  unsigned num;
  unsigned cnt;
  unsigned start = 0;

  is.read(num,"num means");
  if (num > GMPARMS_MAX_NUM) error("ERROR: number of means (%d) in file '%s' exceeds maximum",num,is.fileName());
  if (reset) {
    start = 0;
    means.resize(num);
  } else {
    start = means.size();
    means.resize(start+num);
  }
  for (unsigned i=0;i<num;i++) {
    // first read the count
    MeanVector* ob;

    is.read(cnt,"mean cnt");
    if (cnt != i) 
      error("ERROR: mean count (%d), out of order in file '%s', expecting %d",
	    cnt,is.fileName(),i);

    ob = new MeanVector;
    ob->read(is);
    if (meansMap.find(ob->name()) != meansMap.end())
      error("ERROR: mean named '%s' specified more than once in file '%s'",ob->name().c_str(),is.fileName());
    means[i+start] = ob;
    meansMap[ob->name()] = i+start;
  }
}


void 
GMParms::readCovars(iDataStreamFile& is, bool reset)
{
  unsigned num;
  unsigned cnt;
  unsigned start = 0;

  is.read(num,"num covars");
  if (num > GMPARMS_MAX_NUM) error("ERROR: number of covars (%d) in file '%s' exceeds maximum",num,is.fileName());
  if (reset) {
    start = 0;
    covars.resize(num);
  } else {
    start = covars.size();
    covars.resize(start+num);
  }
  for (unsigned i=0;i<num;i++) {
    // first read the count
    DiagCovarVector* ob;

    is.read(cnt,"cova cnt");
    if (cnt != i) 
      error("ERROR: covar count (%d), out of order in file '%s', expecting %d",
	    cnt,is.fileName(),i);

    ob = new DiagCovarVector;
    ob->read(is);
    if (covarsMap.find(ob->name()) != covarsMap.end())
      error("ERROR: covar named '%s' specified more than once in file '%s'",ob->name().c_str(),is.fileName());
    covars[i+start] = ob;
    covarsMap[ob->name()] = i+start;
  }
}


void 
GMParms::readDLinkMats(iDataStreamFile& is, bool reset)
{
  unsigned num;
  unsigned cnt;
  unsigned start = 0;

  is.read(num,"num dlinkmats");
  if (num > GMPARMS_MAX_NUM) error("ERROR: number of dlink matrices (%d) in file '%s' exceeds maximum",num,is.fileName());
  if (reset) {
    start = 0;
    dLinkMats.resize(num);
  } else {
    start = dLinkMats.size();
    dLinkMats.resize(start+num);
  }
  for (unsigned i=0;i<num;i++) {
    // first read the count
    DlinkMatrix* ob;

    is.read(cnt,"dlink cnt");
    if (cnt != i) 
      error("ERROR: dlink matrix count (%d), out of order in file '%s', expecting %d",
	    cnt,is.fileName(),i);

    ob = new DlinkMatrix;
    ob->read(is);
    if (dLinkMatsMap.find(ob->name()) != dLinkMatsMap.end())
      error("ERROR: dlink matrix named '%s' specified more than once in file '%s'",ob->name().c_str(),is.fileName());
    dLinkMats[i+start] = ob;
    dLinkMatsMap[ob->name()] = i+start;
  }
}


void 
GMParms::readDLinks(iDataStreamFile& is, bool reset)
{
  unsigned num;
  unsigned cnt;
  unsigned start = 0;

  is.read(num,"num dlinks");
  if (num > GMPARMS_MAX_NUM) error("ERROR: number of dlinks (%d) in file '%s' exceeds maximum",num,is.fileName());
  if (reset) {
    start = 0;
    dLinks.resize(num);
  } else {
    start = dLinks.size();
    dLinks.resize(start+num);
  }
  for (unsigned i=0;i<num;i++) {
    // first read the count
    Dlinks* ob;

    is.read(cnt,"dlinks cnt");
    if (cnt != i) 
      error("ERROR: dlink count (%d), out of order in file '%s', expecting %d",
	    cnt,is.fileName(),i);

    ob = new Dlinks;
    ob->read(is);
    if (dLinksMap.find(ob->name()) != dLinksMap.end())
      error("ERROR: dlink structure named '%s' specified more than once in file '%s'",ob->name().c_str(),is.fileName());
    dLinks[i+start] = ob;
    dLinksMap[ob->name()] = i+start;
  }
}


void 
GMParms::readWeightMats(iDataStreamFile& is, bool reset)
{
  unsigned num;
  unsigned cnt;
  unsigned start = 0;

  is.read(num,"num weight matrices");
  if (num > GMPARMS_MAX_NUM) error("ERROR: number of weight matrices (%d) in file '%s' exceeds maximum",num,is.fileName());
  if (reset) {
    start = 0;
    weightMats.resize(num);
  } else {
    start = weightMats.size();
    weightMats.resize(start+num);
  }
  for (unsigned i=0;i<num;i++) {
    // first read the count
    WeightMatrix* ob;

    is.read(cnt,"weight mat cnt");
    if (cnt != i) 
      error("ERROR: weight matrix count (%d), out of order in file '%s', expecting %d",
	    cnt,is.fileName(),i);

    ob = new WeightMatrix;
    ob->read(is);
    if (weightMatsMap.find(ob->name()) != weightMatsMap.end())
      error("ERROR: weight matrix named '%s' specified more than once in file '%s'",ob->name().c_str(),is.fileName());
    weightMats[i+start] = ob;
    weightMatsMap[ob->name()] = i+start;
  }
}


void 
GMParms::readMdCpts(iDataStreamFile& is, bool reset)
{
  unsigned num;
  unsigned cnt;
  unsigned start = 0;

  is.read(num,"num MDCPTs");
  if (num > GMPARMS_MAX_NUM) error("ERROR: number of MDCPTs (%d) in file '%s' exceeds maximum",num,is.fileName());
  if (reset) {
    start = 0;
    mdCpts.resize(num);
  } else {
    start = mdCpts.size();
    mdCpts.resize(start+num);
  }
  for (unsigned i=0;i<num;i++) {
    // first read the count
    MDCPT* ob;

    is.read(cnt,"MDCPT cnt");
    if (cnt != i) 
      error("ERROR: MDCPT count (%d), out of order in file '%s', expecting %d",
	    cnt,is.fileName(),i);

    ob = new MDCPT;
    ob->read(is);
    if (mdCptsMap.find(ob->name()) != mdCptsMap.end())
      error("ERROR: MDCPT named '%s' specified more than once in file '%s'",ob->name().c_str(),is.fileName());
    mdCpts[i+start] = ob;
    mdCptsMap[ob->name()] = i+start;
  }
}


void 
GMParms::readMsCpts(iDataStreamFile& is, bool reset)
{
  unsigned num;
  unsigned cnt;
  unsigned start = 0;

  is.read(num,"num MSCPTs");
  if (num > GMPARMS_MAX_NUM) error("ERROR: number of MSCPTs (%d) in file '%s' exceeds maximum",num,is.fileName());
  if (reset) {
    start = 0;
    msCpts.resize(num);
  } else {
    start = msCpts.size();
    msCpts.resize(start+num);
  }
  for (unsigned i=0;i<num;i++) {
    // first read the count
    MSCPT* ob;

    is.read(cnt,"MSCPT cnt");
    if (cnt != i) 
      error("ERROR: MSCPT count (%d), out of order in file '%s', expecting %d",
	    cnt,is.fileName(),i);

    ob = new MSCPT;
    ob->read(is);
    if (msCptsMap.find(ob->name()) != msCptsMap.end())
      error("ERROR: MSCPT named '%s' specified more than once in file '%s'",ob->name().c_str(),is.fileName());
    msCpts[i+start] = ob;
    msCptsMap[ob->name()] = i+start;
  }
}



void 
GMParms::readMtCpts(iDataStreamFile& is, bool reset)
{
  unsigned num;
  unsigned cnt;
  unsigned start = 0;

  is.read(num,"num MTCPTs");
  if (num > GMPARMS_MAX_NUM) error("ERROR: number of MTCPTs (%d) exceeds maximum",num);
  if (reset) {
    start = 0;
    mtCpts.resize(num);
  } else {
    start = mtCpts.size();
    mtCpts.resize(start+num);
  }
  for (unsigned i=0;i<num;i++) {
    // first read the count
    MTCPT* ob;

    is.read(cnt,"MTCPT cnt");
    if (cnt != i) 
      error("ERROR: MTCPT count (%d), out of order in file '%s', expecting %d",
	    cnt,is.fileName(),i);

    ob = new MTCPT;
    ob->read(is);
    if (mtCptsMap.find(ob->name()) != mtCptsMap.end())
      error("ERROR: MTCPT named '%s' specified more than once in file '%s'",ob->name().c_str(),is.fileName());
    mtCpts[i+start] = ob;
    mtCptsMap[ob->name()] = i+start;
  }
}



void 
GMParms::readDTs(iDataStreamFile& is, bool reset)
{
  unsigned num;
  unsigned cnt;
  unsigned start = 0;

  is.read(num,"num DTs");
  if (num > GMPARMS_MAX_NUM) error("ERROR: number of DTs (%d) in file '%s' exceeds maximum",num,is.fileName());
  if (reset) {
    start = 0;
    dts.resize(num);
  } else {
    start = dts.size();
    dts.resize(start+num);
  }
  for (unsigned i=0;i<num;i++) {
    // first read the count
    RngDecisionTree<unsigned>* ob;

    is.read(cnt,"DT cnt");
    if (cnt != i) 
      error("ERROR: DT count (%d), out of order in file '%s', expecting %d",
	    cnt,is.fileName(),i);

    ob = new RngDecisionTree<unsigned>;
    ob->read(is);
    if (dtsMap.find(ob->name()) != dtsMap.end())
      error("ERROR: DT named '%s' specified more than once in file '%s'",ob->name().c_str(),is.fileName());
    dts[i+start] = ob;
    dtsMap[ob->name()] = i+start;
  }
}


void 
GMParms::readGaussianComponents(iDataStreamFile& is, bool reset)
{
  unsigned num;
  unsigned cnt;

  unsigned start = 0;
  is.read(num,"num Gaussian components");
  if (num > GMPARMS_MAX_NUM) error("ERROR: number of Gaussian components (%d) in file '%s' exceeds maximum",num,is.fileName());
  if (reset) {
    start = 0;
    gaussianComponents.resize(num);
  } else {
    start = gaussianComponents.size();
    gaussianComponents.resize(start+num);
  }
  for (unsigned i=0;i<num;i++) {
    // first read the count
    GaussianComponent* gc;

    is.read(cnt,"Gaussian comp cnt");
    if (cnt != i) 
      error("ERROR: Gaussian component count (%d), out of order in file '%s', expecting %d",cnt,is.fileName(),i);

    // next read the dimension of this Gaussian
    int dim;
    is.read(dim,"Gaussian comp dim");

    // read the Gaussian type
    int t;
    is.read(t,"Gaussian comp type");
    if (t == GaussianComponent::Diag) {
      gc = new DiagGaussian(dim);
    } else if (t == GaussianComponent::LinMeanCondDiag) {
      error("LinMeanCondDiag not implemented");
      // gc = new LinMeanCondDiagGaussian(dim);
    } else if (t == GaussianComponent::NLinMeanCondDiag) {
      error("NLinMeanCondDiag not implemented");
      // gc = new NLinMeanCondDiagGaussian(dim);
    } else {
      error("Error: unknown gaussian component type in file");
    }
    gc->read(is);
    if (gaussianComponentsMap.find(gc->name()) != gaussianComponentsMap.end())
      error("ERROR: Gaussian component named '%s' specified more than once in file '%s'",gc->name().c_str(),is.fileName());
    gaussianComponents[i+start] = gc;
    gaussianComponentsMap[gc->name()] = i+start;
  }
}


void 
GMParms::readMixGaussians(iDataStreamFile& is, bool reset)
{
  unsigned num;
  unsigned cnt;
  unsigned start = 0;

  is.read(num,"num MGs");
  if (num > GMPARMS_MAX_NUM) error("ERROR: number of mixtures of Gaussians (%d) in file '%s' exceeds maximum",num,is.fileName());
  if (reset) {
    start = 0;
    mixGaussians.resize(num);
  } else {
    start = mixGaussians.size();
    mixGaussians.resize(start+num);
  }
  for (unsigned i=0;i<num;i++) {
    // first read the count
    MixGaussians* gm;

    is.read(cnt,"MG cnt");
    if (cnt != i) 
      error("ERROR: mix Gaussian count (%d), out of order in file '%s', expecting %d",cnt,is.fileName(),i);


    // next read the dimension of this Gaussian
    int dim;
    is.read(dim,"MG dim");

    gm = new MixGaussians(dim);
    gm->read(is);
    if (mixGaussiansMap.find(gm->name()) != mixGaussiansMap.end()) {
      error("ERROR: mixture of Gaussian named '%s' specified more than once in file '%s'",gm->name().c_str(),is.fileName());
    }
    mixGaussians[i+start] = gm;
    mixGaussiansMap[gm->name()] = i+start;
  }
}

void 
GMParms::readGausSwitchMixGaussians(iDataStreamFile& is, bool reset)
{
  error("not implemented");
}

void 
GMParms::readLogitSwitchMixGaussians(iDataStreamFile& is, bool reset)
{
  error("not implemented");
}

void 
GMParms::readMlpSwitchMixGaussians(iDataStreamFile& is, bool reset)
{
  error("not implemented");
}


void 
GMParms::readBasic(iDataStreamFile& is)
{

  unsigned num;
  unsigned cnt;
  string str;

  is.read(str,"GMTK_GMParms::readBasic, magic");
  if (str != MAGIC_PRM_FILE)
    error("GMTK_GMParms::readBasic. Expecting basic param file, got (%s) in file (%s)",str.c_str(),is.fileName());


  is.read(num,"GMTK_GMParms::readBasic, dpmfs");
  if (num < 0) error("GMTK_GMParms::readBasic num dpmfs = %d",num);
  dPmfs.resize(num);
  for (unsigned i=0;i<num;i++) {
    is.read(cnt,"GMTK_GMParms::readBasic, cnt dpmfs");
    if (cnt != i) 
      error("GMTK_GMParms::readBasic,dpmfs, out of order count",cnt);
    dPmfs[i] = new Dense1DPMF;
    dPmfs[i]->read(is);
    if (dPmfsMap.find(dPmfs[i]->name()) != dPmfsMap.end())
      error("GMTK_GMParms::readBasic,dpmfs, multiple use of name '%s'",
	    dPmfs[i]->name().c_str());
    dPmfsMap[dPmfs[i]->name()] = i;
  }


  is.read(num,"GMTK_GMParms::readBasic, spmfs");
  if (num < 0) error("GMTK_GMParms::readBasic num spmfs = %d",num);
  sPmfs.resize(num);
  for (unsigned i=0;i<num;i++) {
    is.read(cnt,"GMTK_GMParms::readBasic, cnt spmfs");
    if (cnt != i) 
      error("GMTK_GMParms::readBasic,spmfs, out of order count",cnt);
    sPmfs[i] = new Sparse1DPMF;
    sPmfs[i]->read(is);
    sPmfsMap[sPmfs[i]->name()] = i;
  }

  is.read(num,"GMTK_GMParms::readBasic, means");
  if (num < 0) error("GMTK_GMParms::readBasic num means = %d",num);
  means.resize(num);
  for (unsigned i=0;i<num;i++) {
    is.read(cnt,"GMTK_GMParms::readBasic, cnt means");
    if (cnt != i) 
      error("GMTK_GMParms::readBasic,means,  out of order count",cnt);
    means[i] = new MeanVector;
    means[i]->read(is);
    meansMap[means[i]->name()] = i;
  }

  is.read(num,"GMTK_GMParms::readBasic, covars");
  if (num < 0) error("GMTK_GMParms::readBasic num covars = %d",num);
  covars.resize(num);
  for (unsigned i=0;i<num;i++) {
    is.read(cnt,"GMTK_GMParms::readBasic, cnt covars");
    if (cnt != i) 
      error("GMTK_GMParms::readBasic,covars, out of order count",cnt);
    covars[i] = new DiagCovarVector;
    covars[i]->read(is);
    covarsMap[covars[i]->name()] = i;
  }

  is.read(num,"GMTK_GMParms::readBasic, DlinkMatrix");
  if (num < 0) error("GMTK_GMParms::readBasic num DlinkMatrix = %d",num);
  dLinkMats.resize(num);
  for (unsigned i=0;i<num;i++) {
    is.read(cnt,"GMTK_GMParms::readBasic, cnt dlinks");
    if (cnt != i) 
      error("GMTK_GMParms::readBasic,dlinks, out of order count",cnt);
    dLinkMats[i] = new DlinkMatrix;
    dLinkMats[i]->read(is);
    dLinkMatsMap[dLinkMats[i]->name()] = i;
  }

  is.read(num,"GMTK_GMParms::readBasic, WeightMatrix");
  if (num < 0) error("GMTK_GMParms::readBasic num WeightMatrix = %d",num);
  weightMats.resize(num);
  for (unsigned i=0;i<num;i++) {
    is.read(cnt,"GMTK_GMParms::readBasic, cnt weights");
    if (cnt != i) 
      error("GMTK_GMParms::readBasic, weights, out of order count",cnt);
    weightMats[i] = new WeightMatrix;
    weightMats[i]->read(is);
    weightMatsMap[weightMats[i]->name()] = i;
  }


  is.read(num,"GMTK_GMParms::readBasic, MDCPT");
  if (num < 0) error("GMTK_GMParms::readBasic num MDCPT = %d",num);
  mdCpts.resize(num);
  for (unsigned i=0;i<num;i++) {
    is.read(cnt,"GMTK_GMParms::readBasic, cnt MDCPTs");
    if (cnt != i) 
      error("GMTK_GMParms::readBasic, MDCPTs, out of order count",cnt);
    mdCpts[i] = new MDCPT;
    mdCpts[i]->read(is);
    mdCptsMap[mdCpts[i]->name()] = i;
  }

  is.read(num,"GMTK_GMParms::readBasic, MSCPT");
  if (num < 0) error("GMTK_GMParms::readBasic num MSCPT = %d",num);
  msCpts.resize(num);
  for (unsigned i=0;i<num;i++) {
    is.read(cnt,"GMTK_GMParms::readBasic, cnt MSCPTS");
    if (cnt != i) 
      error("GMTK_GMParms::readBasic, MSCPTs, out of order count",cnt);
    msCpts[i] = new MSCPT;
    msCpts[i]->read(is);
    msCptsMap[msCpts[i]->name()] = i;
  }


  is.read(num,"GMTK_GMParms::readBasic, MTCPT");
  if (num < 0) error("GMTK_GMParms::readBasic num MTCPT = %d",num);
  mtCpts.resize(num);
  for (unsigned i=0;i<num;i++) {
    is.read(cnt,"GMTK_GMParms::readBasic, cnt MTCPTS");
    if (cnt != i) 
      error("GMTK_GMParms::readBasic, MTCPTs, out of order count",cnt);
    mtCpts[i] = new MTCPT;
    mtCpts[i]->read(is);
    mtCptsMap[mtCpts[i]->name()] = i;
  }

}

void 
GMParms::read(iDataStreamFile& is,bool dataFilesAreBinary)
{
  // read a file consisting of a list of keyword,filename
  // pairs. the keyword says which structure to read in,
  // and the filename says where to get it.

  string keyword;
  string fileName;

  while (is.readString(keyword)) {
    if (!is.readString(fileName)) {
      error("ERROR: while reading file '%s', got keyword '%s' without a filename",is.fileName(),keyword.c_str());
    }

    iDataStreamFile isf(fileName.c_str(),dataFilesAreBinary);
    if (keyword == "DPMF_FILE") {
      readDPmfs(isf,false);
    } else if (keyword == "SPMF_FILE") {
      readSPmfs(isf,false);
    } else if (keyword == "MEAN_FILE") {
      readMeans(isf,false);
    } else if (keyword == "COVAR_FILE") {
      readCovars(isf,false);
    } else if (keyword == "DLINK_MAT_FILE") {
      readDLinkMats(isf,false);
    } else if (keyword == "DLINK_FILE") {
      readDLinks(isf,false);
    } else if (keyword == "WEIGHT_MAT_FILE") {
      readWeightMats(isf,false);
    } else if (keyword == "MDCPT_FILE") {
      readMdCpts(isf,false);
    } else if (keyword == "MSCPT_FILE") {
      readMsCpts(isf,false);
    } else if (keyword == "MTCPT_FILE") {
      readMtCpts(isf,false);
    } else if (keyword == "DT_FILE") {
      readDTs(isf,false);
    } else if (keyword == "GC_FILE") {
      readGaussianComponents(isf,false);
    } else if (keyword == "MG_FILE") {
      readMixGaussians(isf,false);
    } else if (keyword == "GSMG_FILE") {
      error("GSMG_FILE in file '%s', not implemented",
	    is.fileName());
    } else if (keyword == "LSMG_FILE") {
      error("LSMG_FILE in file '%s', not implemented",
	    is.fileName());
    } else if (keyword == "MSMG_FILE") {
      error("MSMG_FILE in file '%s', not implemented",
	    is.fileName());
    } else {
      error("ERROR: encountered unknown file type '%s' in file '%s'",
	    keyword.c_str(),is.fileName());
    }
  }

}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//        WRITING Routines
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


/*-
 *-----------------------------------------------------------------------
 * write<OBJECTS>
 *      Write the <OBJECT> to a file specified by os
 * 
 * Preconditions:
 *      None, as if the arrays are zero sized, nothing will be written
 *      other than the number 0
 *
 * Postconditions:
 *      All objects have been written.
 *      
 * Side Effects:
 *      None.
 *
 * Results:
 *      none
 *
 *-----------------------------------------------------------------------
 */


void 
GMParms::writeDPmfs(oDataStreamFile& os)
{
  os.write(dPmfs.size(),"num dPMFs");
  os.nl();
  for (unsigned i=0;i<dPmfs.size();i++) {
    // first write the count
    os.write(i,"dDPMF cnt");
    os.nl();
    dPmfs[i]->write(os);
  }
  os.nl();
}



void 
GMParms::writeSPmfs(oDataStreamFile& os)
{
  os.write(sPmfs.size(),"num sPMFs");
  os.nl();
  for (unsigned i=0;i<sPmfs.size();i++) {
    // first write the count
    os.write(i,"sPMFs cnt");
    os.nl();
    sPmfs[i]->write(os);
  }
  os.nl();
}


void 
GMParms::writeMeans(oDataStreamFile& os)
{
  os.write(means.size(),"num Means");
  os.nl();
  for (unsigned i=0;i<means.size();i++) {
    // first write the count
    os.write(i,"means cnt");
    os.nl();
    means[i]->write(os);
  }
  os.nl();
}


void 
GMParms::writeCovars(oDataStreamFile& os)
{
  os.write(covars.size(),"num covars");
  os.nl();
  for (unsigned i=0;i<covars.size();i++) {
    // first write the count
    os.write(i,"covar cnt");
    os.nl();
    covars[i]->write(os);
  }
  os.nl();
}


void 
GMParms::writeDLinkMats(oDataStreamFile& os)
{
  os.write(dLinkMats.size(),"num dlink mats");
  os.nl();
  for (unsigned i=0;i<dLinkMats.size();i++) {
    // first write the count
    os.write(i,"dlink mat cnt");
    os.nl();
    dLinkMats[i]->write(os);
  }
  os.nl();
}


void 
GMParms::writeDLinks(oDataStreamFile& os)
{
  os.write(dLinks.size(),"num dlinks");
  os.nl();
  for (unsigned i=0;i<dLinks.size();i++) {
    // first write the count
    os.write(i,"dlink cnt");
    os.nl();
    dLinks[i]->write(os);
  }
  os.nl();
}


void 
GMParms::writeWeightMats(oDataStreamFile& os)
{
  os.write(weightMats.size(),"num weight mats");
  os.nl();
  for (unsigned i=0;i<weightMats.size();i++) {
    // first write the count
    os.write(i,"weight mat cnt");
    os.nl();
    weightMats[i]->write(os);
  }
  os.nl();
}


void 
GMParms::writeMdCpts(oDataStreamFile& os)
{
  os.write(mdCpts.size(),"num MDCPTs");
  os.nl();
  for (unsigned i=0;i<mdCpts.size();i++) {
    // first write the count
    os.write(i,"MDCPT cnt");
    os.nl();
    mdCpts[i]->write(os);
  }
  os.nl();
}


void 
GMParms::writeMsCpts(oDataStreamFile& os)
{
  os.write(msCpts.size(),"num MSCPTs");
  os.nl();
  for (unsigned i=0;i<msCpts.size();i++) {
    // first write the count
    os.write(i,"MSCPT cnt");
    os.nl();
    msCpts[i]->write(os);
  }
  os.nl();
}


void 
GMParms::writeMtCpts(oDataStreamFile& os)
{
  os.write(mtCpts.size(),"num MTCPTs");
  os.nl();
  for (unsigned i=0;i<mtCpts.size();i++) {
    // first write the count
    os.write(i,"MTCPT cnt");
    os.nl();
    mtCpts[i]->write(os);
  }
  os.nl();
}


void 
GMParms::writeDTs(oDataStreamFile& os)
{
  os.write(dts.size(),"num DTS");
  os.nl();
  for (unsigned i=0;i<dts.size();i++) {
    // first write the count
    os.write(i,"DTS cnt");
    os.nl();
    dts[i]->write(os);
  }
  os.nl();
}


void 
GMParms::writeGaussianComponents(oDataStreamFile& os)
{
  os.write(gaussianComponents.size(),"num GCs");
  os.nl();
  for (unsigned i=0;i<dts.size();i++) {
    // first write the count
    os.write(i,"GC cnt");
    os.nl();

    // next write the dimension of this Gaussian
    os.write(gaussianComponents[i]->dim(),"GC dim");
    os.nl();

    ////////////////////////////////////////////////////////////
    // Assume that the GC's write routine will 
    // itself write the Gaussian type
    gaussianComponents[i]->write(os);
  }
  os.nl();
}


void 
GMParms::writeMixGaussians(oDataStreamFile& os)
{
  os.write(mixGaussians.size(),"num MIXGAUSSIANS");
  os.nl();
  for (unsigned i=0;i<mixGaussians.size();i++) {
    // first write the count
    os.write(i,"MIXGAUSSIANS cnt");
    os.nl();

    // next write the dimension of this Gaussian
    os.write(mixGaussians[i]->dim(),"MG dim");
    os.nl();

    mixGaussians[i]->write(os);
  }
  os.nl();
}

void 
GMParms::writeGausSwitchMixGaussians(oDataStreamFile& os)
{
  error("not implemented");
}

void 
GMParms::writeLogitSwitchMixGaussians(oDataStreamFile& os)
{
  error("not implemented");
}

void 
GMParms::writeMlpSwitchMixGaussians(oDataStreamFile& os)
{
  error("not implemented");
}



void 
GMParms::writeBasic(oDataStreamFile& os)
{

  os.write(MAGIC_PRM_FILE,"GMTK_GMParms::writeBasic, magic");
  os.nl();

  os.nl(); os.writeComment("dense PMFs");os.nl();
  os.write(dPmfs.size(),"GMTK_GMParms::writeBasic, dpmfs");
  os.nl();
  for (unsigned i=0;i<dPmfs.size();i++) {
    os.write(i);
    dPmfs[i]->write(os);
  }
  os.nl();

  os.nl(); os.writeComment("sparse PMFs");os.nl();
  os.write(sPmfs.size(),"GMTK_GMParms::writeBasic, spmfs");
  os.nl();
  for (unsigned i=0;i<sPmfs.size();i++) {
    os.write(i);
    sPmfs[i]->write(os);
  }
  os.nl();


  os.nl(); os.write(means.size(),"GMTK_GMParms::writeBasic, means");
  os.nl();
  for (unsigned i=0;i<means.size();i++) {
    os.write(i);
    means[i]->write(os);
  }
  os.nl();

  os.nl(); os.writeComment("covars");os.nl();
  os.write(covars.size(),"GMTK_GMParms::writeBasic, covars");
  os.nl();
  for (unsigned i=0;i<covars.size();i++) {
    os.write(i);
    covars[i]->write(os);
  }
  os.nl();

  os.nl(); os.writeComment("dlink matrices");os.nl();
  os.write(dLinkMats.size(),"GMTK_GMParms::writeBasic, DlinkMatrix");
  os.nl();
  for (unsigned i=0;i<dLinkMats.size();i++) {
    os.write(i); os.nl();
    dLinkMats[i]->write(os);
  }
  os.nl();

  os.nl(); os.writeComment("weight matrices");os.nl();
  os.write(weightMats.size(),"GMTK_GMParms::writeBasic, WeightMatrix");
  os.nl();
  for (unsigned i=0;i<weightMats.size();i++) {
    os.write(i); os.nl();
    weightMats[i]->write(os);
  }
  os.nl();

  os.nl(); os.writeComment("MDCPTs");os.nl();
  os.write(mdCpts.size(),"GMTK_GMParms::writeBasic, MDCPT");
  os.nl();
  for (unsigned i=0;i<mdCpts.size();i++) {
    os.write(i); os.nl();
    mdCpts[i]->write(os);
  }
  os.nl();

  os.nl();  os.writeComment("MSCPTs");os.nl();
  os.write(msCpts.size(),"GMTK_GMParms::writeBasic, MSCPT");
  os.nl();
  for (unsigned i=0;i<msCpts.size();i++) {
    os.write(i); os.nl();
    msCpts[i]->write(os);
  }
  os.nl();


  os.nl(); os.writeComment("MTCPTs");os.nl();
  os.write(mtCpts.size(),"GMTK_GMParms::writeBasic, MTCPT");
  os.nl();
  for (unsigned i=0;i<mtCpts.size();i++) {
    os.write(i); os.nl();
    mtCpts[i]->write(os);
  }
  os.nl();

}



////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//        EM Routines
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


void
GMParms::emEndIteration()
{
  // go through all EMable objects possibly
  // used by any RV and make the call


  // first do the basic objects
  for (unsigned i=0;i<dPmfs.size();i++)
    dPmfs[i]->emEndIteration();
  for (unsigned i=0;i<sPmfs.size();i++)
    sPmfs[i]->emEndIteration();
  for (unsigned i=0;i<means.size();i++)
    means[i]->emEndIteration();
  for (unsigned i=0;i<covars.size();i++)
    covars[i]->emEndIteration();
  for (unsigned i=0;i<dLinkMats.size();i++)
    dLinkMats[i]->emEndIteration();
  for (unsigned i=0;i<weightMats.size();i++)
    weightMats[i]->emEndIteration();

  // gaussian components
  for (unsigned i=0;i<gaussianComponents.size();i++)
    gaussianComponents[i]->emEndIteration();

  // for discrete RVs
  for (unsigned i=0;i<mdCpts.size();i++)
    mdCpts[i]->emEndIteration();
  for (unsigned i=0;i<msCpts.size();i++)
    msCpts[i]->emEndIteration();
  for (unsigned i=0;i<mtCpts.size();i++)
    mtCpts[i]->emEndIteration();

  // for continuous RVs
  for (unsigned i=0;i<mixGaussians.size();i++)
    mixGaussians[i]->emEndIteration();
#if 0
  for (unsigned i=0;i<gausSwitchingMixGaussians.size();i++)
    gausSwitchingMixGaussians[i]->emEndIteration();
  for (unsigned i=0;i<logitSwitchingMixGaussians.size();i++)
    logitSwitchingMixGaussians[i]->emEndIteration();
  for (unsigned i=0;i<mlpSwitchingMixGaussians.size();i++)
    mlpSwitchingMixGaussians[i]->emEndIteration();
#endif

}


void
GMParms::emSwapCurAndNew()
{
  // go through all EMable objects possibly
  // used by any RV and make the call

  // first do the basic objects
  for (unsigned i=0;i<dPmfs.size();i++)
    dPmfs[i]->emSwapCurAndNew();
  for (unsigned i=0;i<sPmfs.size();i++)
    sPmfs[i]->emSwapCurAndNew();
  for (unsigned i=0;i<means.size();i++)
    means[i]->emSwapCurAndNew();
  for (unsigned i=0;i<covars.size();i++)
    covars[i]->emSwapCurAndNew();
  for (unsigned i=0;i<dLinkMats.size();i++)
    dLinkMats[i]->emSwapCurAndNew();
  for (unsigned i=0;i<weightMats.size();i++)
    weightMats[i]->emSwapCurAndNew();

  // gaussian components
  for (unsigned i=0;i<gaussianComponents.size();i++)
    gaussianComponents[i]->emSwapCurAndNew();

  // for discrete RVs
  for (unsigned i=0;i<mdCpts.size();i++)
    mdCpts[i]->emSwapCurAndNew();
  for (unsigned i=0;i<msCpts.size();i++)
    msCpts[i]->emSwapCurAndNew();
  for (unsigned i=0;i<mtCpts.size();i++)
    mtCpts[i]->emSwapCurAndNew();

  // for continuous RVs
  for (unsigned i=0;i<mixGaussians.size();i++)
    mixGaussians[i]->emSwapCurAndNew();
#if 0
  for (unsigned i=0;i<gausSwitchingMixGaussians.size();i++)
    gausSwitchingMixGaussians[i]->emSwapCurAndNew();
  for (unsigned i=0;i<logitSwitchingMixGaussians.size();i++)
    logitSwitchingMixGaussians[i]->emSwapCurAndNew();
  for (unsigned i=0;i<mlpSwitchingMixGaussians.size();i++)
    mlpSwitchingMixGaussians[i]->emSwapCurAndNew();
#endif

}


void
GMParms::makeRandom()
{
  // go through all EMable objects possibly
  // used by any RV and make the call

  // first do the basic objects
  // which also does everything for continuous RVs
  for (unsigned i=0;i<dPmfs.size();i++)
    dPmfs[i]->makeRandom();
  for (unsigned i=0;i<sPmfs.size();i++)
    sPmfs[i]->makeRandom();
  for (unsigned i=0;i<means.size();i++)
    means[i]->makeRandom();
  for (unsigned i=0;i<covars.size();i++)
    covars[i]->makeRandom();
  for (unsigned i=0;i<dLinkMats.size();i++)
    dLinkMats[i]->makeRandom();
  for (unsigned i=0;i<weightMats.size();i++)
    weightMats[i]->makeRandom();

  // gaussian components
  for (unsigned i=0;i<gaussianComponents.size();i++)
    gaussianComponents[i]->makeRandom();

  // for discrete RVs
  for (unsigned i=0;i<mdCpts.size();i++)
    mdCpts[i]->makeRandom();
  for (unsigned i=0;i<msCpts.size();i++)
    msCpts[i]->makeRandom();
  for (unsigned i=0;i<mtCpts.size();i++)
    mtCpts[i]->makeRandom();

}



void
GMParms::makeUniform()
{
  // go through all EMable objects possibly
  // used by any RV and make the call

  // first do the basic objects
  // which also does everything for continuous RVs
  for (unsigned i=0;i<dPmfs.size();i++)
    dPmfs[i]->makeUniform();
  for (unsigned i=0;i<sPmfs.size();i++)
    sPmfs[i]->makeUniform();
  for (unsigned i=0;i<means.size();i++)
    means[i]->makeUniform();
  for (unsigned i=0;i<covars.size();i++)
    covars[i]->makeUniform();
  for (unsigned i=0;i<dLinkMats.size();i++)
    dLinkMats[i]->makeUniform();
  for (unsigned i=0;i<weightMats.size();i++)
    weightMats[i]->makeUniform();

  // gaussian components
  for (unsigned i=0;i<gaussianComponents.size();i++)
    gaussianComponents[i]->makeUniform();

  // for discrete RVs
  for (unsigned i=0;i<mdCpts.size();i++)
    mdCpts[i]->makeUniform();
  for (unsigned i=0;i<msCpts.size();i++)
    msCpts[i]->makeUniform();
  for (unsigned i=0;i<mtCpts.size();i++)
    mtCpts[i]->makeUniform();

}








#ifdef MAIN

GMParms GM_Parms;

int
main()
{

  // read in a file of decision trees
  iDataStreamFile isdt("dataFiles/foo.dt",false);
  GM_Parms.readDTs(isdt);

  // read in basic structures
  iDataStreamFile is("dataFiles/test1.gmb",false);
  GM_Parms.readBasic(is);

  // write both out again to stdout (i.e., "-")
  oDataStreamFile os("-");
  GM_Parms.writeDTs(os);
  GM_Parms.writeBasic(os);

}


#endif
