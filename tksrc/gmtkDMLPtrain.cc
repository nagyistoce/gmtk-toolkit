/*
 * gmtkDMLPtrain.cc
 *
 * Written by Richard Rogers <rprogers@ee.washington.edu>
 *
 * Copyright (C) 2013 Jeff Bilmes
 * Licensed under the Open Software License version 3.0
 * See COPYING or http://opensource.org/licenses/OSL-3.0
 *
 */


#if HAVE_CONFIG_H
#include <config.h>
#endif
#if HAVE_HG_H
#include "hgstamp.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <float.h>
#include <assert.h>
#include <signal.h>

#include "DBN.h"
#include "MMapMatrix.h"
#include "BatchSource.h"
#include "AsynchronousBatchSource.h"

#include "general.h"
#include "error.h"
#include "rand.h"
#include "arguments.h"
#include "ieeeFPsetup.h"
#include "debug.h"
#include "version.h"


#include "GMTK_FileParser.h"
#include "GMTK_RV.h"
#include "GMTK_DiscRV.h"
#include "GMTK_ContRV.h"
#include "GMTK_GMParms.h"
#include "GMTK_GMTemplate.h"
#include "GMTK_Partition.h"

#include "GMTK_ObservationSource.h"
#include "GMTK_FileSource.h"
#include "GMTK_CreateFileSource.h"
#include "GMTK_ASCIIFile.h"
#include "GMTK_FlatASCIIFile.h"
#include "GMTK_PFileFile.h"
#include "GMTK_HTKFile.h"
#include "GMTK_HDF5File.h"
#include "GMTK_BinaryFile.h"
#include "GMTK_Filter.h"
#include "GMTK_Stream.h"
#include "GMTK_RandomSampleSchedule.h"
#include "GMTK_LinearSchedule.h"
#include "GMTK_ShuffleSchedule.h"
#include "GMTK_PermutationSchedule.h"

#include "GMTK_MixtureCommon.h"
#include "GMTK_GaussianComponent.h"
#include "GMTK_LinMeanCondDiagGaussian.h"
#include "GMTK_MeanVector.h"
#include "GMTK_DiagCovarVector.h"
#include "GMTK_DlinkMatrix.h"

#include "GMTK_WordOrganization.h"

#include "GMTK_DeepVECPT.h"

VCID(HGID)

#define GMTK_ARG_OBS_FILES
/****************************      FILE RANGE OPTIONS             ***********************************************/
#define GMTK_ARG_FILE_RANGE_OPTIONS
#define GMTK_ARG_TRRNG
#define GMTK_ARG_START_END_SKIP
/************************  OBSERVATION MATRIX TRANSFORMATION OPTIONS   ******************************************/
#define GMTK_ARG_OBS_MATRIX_OPTIONS
#define GMTK_ARG_OBS_MATRIX_XFORMATION


/*************************   INPUT TRAINABLE PARAMETER FILE HANDLING  *******************************************/
#define GMTK_ARG_INPUT_TRAINABLE_FILE_HANDLING
#define GMTK_ARG_INPUT_MASTER_FILE_OPT_ARG

#define GMTK_ARG_OUTPUT_MASTER_FILE
#define GMTK_ARG_OUTPUT_TRAINABLE_PARAMS
#define GMTK_ARG_INPUT_TRAINABLE_PARAMS
#define GMTK_ARG_CPP_CMD_OPTS
#define GMTK_ARG_ALLOC_DENSE_CPTS
#define GMTK_ARG_CPT_NORM_THRES


/*************************   CONTINUOUS RANDOM VARIABLE OPTIONS       *******************************************/
#define GMTK_ARG_CONTINUOUS_RANDOM_VAR_OPTIONS
#define GMTK_ARG_VAR_FLOOR
#define GMTK_ARG_VAR_FLOOR_ON_READ


/*************************   DEEP MLP TRAINING OPTIONS                *******************************************/

#define GMTK_ARG_DMLP_TRAINING_OPTIONS
#define GMTK_ARG_DMLP_TRAINING_PARAMS


/*************************   GENERAL OPTIONS                          *******************************************/

#define GMTK_ARG_GENERAL_OPTIONS
#define GMTK_ARG_SEED
#define GMTK_ARG_VERB
#define GMTK_ARG_VERSION
#define GMTK_ARG_HELP


#define GMTK_ARGUMENTS_DEFINITION
#include "GMTK_Arguments.h"
#undef GMTK_ARGUMENTS_DEFINITION


Arg Arg::Args[] = {


#define GMTK_ARGUMENTS_DOCUMENTATION
#include "GMTK_Arguments.h"
#undef GMTK_ARGUMENTS_DOCUMENTATION



  // final one to signal the end of the list
  Arg()

};



void
usr1_handler(int arg) {
  DBN::checkSignal = true;
  signal(SIGUSR1, usr1_handler);
}

/*
 * definition of needed global arguments
 */
RAND rnd(seedme);
GMParms GM_Parms;
#if 0
ObservationMatrix globalObservationMatrix;
#endif

FileSource *gomFS;
ObservationSource *globalObservationMatrix;

#include <iostream>

int
main(int argc,char*argv[])
{
  ////////////////////////////////////////////
  // set things up so that if an FP exception
  // occurs such as an "invalid" (NaN), overflow
  // or divide by zero, we actually get a FPE
  ieeeFPsetup();

  CODE_TO_COMPUTE_ENDIAN;

  ////////////////////////////////////////////
  // parse arguments
  bool parse_was_ok = Arg::parse(argc,(char**)argv,
    "\nThis program trains deep neural networks for use with DeepVECPTs.\n");
  if(!parse_was_ok) {
    Arg::usage(); exit(-1);
  }


#define GMTK_ARGUMENTS_CHECK_ARGS
#include "GMTK_Arguments.h"
#undef GMTK_ARGUMENTS_CHECK_ARGS


  /////////////////////////////////////////////


  // Setup to read observation files
  infoMsg(IM::Max,"Opening Files ...\n");
  gomFS = instantiateFileSource();
  globalObservationMatrix = gomFS;
  infoMsg(IM::Max,"Finished opening files.\n");

  // Read in all GMTK object definitions (matrices, neural networks, etc.)
  if (inputMasterFile != NULL) {
    iDataStreamFile pf(inputMasterFile,false,true,cppCommandOptions);
    GM_Parms.read(pf);
  }
  if (inputTrainableParameters != NULL) {
    // flat, where everything is contained in one file
    iDataStreamFile pf(inputTrainableParameters,binInputTrainableParameters,true,cppCommandOptions);
    GM_Parms.readTrainable(pf);
  }
  GM_Parms.finalizeParameters();  
  
  printf("Finished reading in all parameters and structures\n");

  gomFS->openSegment(0);

  // Look up the Deep VE CPT to train 
  string DMLPNameStr(DMLPName);
  if (GM_Parms.deepNNsMap.find(DMLPNameStr) == GM_Parms.deepNNsMap.end()) {
    error("Error: No Deep NN named '%s' found\n", DMLPName);
  }
  DeepNN *dnn = GM_Parms.deepNNs[ GM_Parms.deepNNsMap[DMLPNameStr] ];

  printf("Total number of trainable parameters in input files = %u\n",
	 dnn->totalNumberParameters());

  struct rusage rus; /* starting time */
  struct rusage rue; /* ending time */
  getrusage(RUSAGE_SELF,&rus);

  // Construct Galen's DBN data structure based on DeepNN defined in master file(s)
  int inputSize =  (int)dnn->numInputs();

  if (inputSize != (2 * radius + 1) * numFeatures) {
    error("ERROR: total inputs to '%s' (%d) must equal (2r+1)n, where r is -radius and n is -numFeatures\n", 
	  inputSize, radius, numFeatures);
  }
  int numLayers =  (int)dnn->numLayers();
  int outputSize = (int)dnn->numOutputs();
  
  vector<int> hiddenSize(numLayers);
  for (unsigned i=0; i < numLayers; i+=1)
    hiddenSize[i] = (int)dnn->layerOutputs(i);

  bool warned = false;
  vector<Layer::ActFunc> hActFunc(numLayers);
  for (unsigned i=0; i < numLayers; i+=1) {
    switch (dnn->getSquashFn(i)) {
    case DeepNN::SOFTMAX: 
      if (i != numLayers - 1) {
	error("ERROR: gmtkDMLPtrain only supports softmax for the output layer\n");
      }
      hActFunc[i] = Layer::ActFunc(Layer::ActFunc::LINEAR);
      break;
    case DeepNN::LOGISTIC: 
      if (i == numLayers - 1) {
	error("ERROR: gmtkDMLPtrain only supports linear or softmax for the output layer\n");
      }
      hActFunc[i] = Layer::ActFunc(Layer::ActFunc::LOG_SIG); 
      break;
    case DeepNN::TANH: 
      if (i == numLayers - 1) {
	error("ERROR: gmtkDMLPtrain only supports linear or softmax for the output layer\n");
      }
      hActFunc[i] = Layer::ActFunc(Layer::ActFunc::TANH); 
      break;
    case DeepNN::ODDROOT: 
      if (i == numLayers - 1) {
	error("ERROR: gmtkDMLPtrain only supports linear or softmax for the output layer\n");
      }
      hActFunc[i] = Layer::ActFunc(Layer::ActFunc::CUBIC); 
      break;
    case DeepNN::LINEAR:
      hActFunc[i] = Layer::ActFunc(Layer::ActFunc::LINEAR);
      break;
    case DeepNN::RECTLIN:
      if (i == numLayers - 1) {
	error("ERROR: gmtkDMLPtrain only supports linear or softmax for the output layer\n");
      }
      if (pretrainMode == DBN::CD) {
	error("ERROR: gmtkDMLPtrain does not support rectified linear activation functions with -pretrainType CD\n");
      }
      if (DBN::sparseInitLayer && !warned) {
	warning("WARNING: Deep NN '%s' uses rectified linear, which may perform poorly without -sparseInitLayer F\n",
		dnn->name().c_str());
	warned = true;
      }
      hActFunc[i] = Layer::ActFunc(Layer::ActFunc::RECT_LIN);
      break;
    default: 
      error("Error: unknown activation function\n");
    }
  }

  vector<AllocatingMatrix> W(numLayers); // copy existing weights into W & B in case we're resuming training
  vector<AllocatingVector> B(numLayers);
  for (unsigned j=0; j < numLayers; j+=1) {
    double *params;
    int rows, cols;
    dnn->getParams(j, rows, cols, params);
    Matrix P(params, cols, rows, cols, false);
    W[j].CopyFrom( P.SubMatrix(0, cols-1, 0, rows) ); // -1 for bias column
    B[j].CopyFrom( P.GetRow(cols - 1) );
  }
  DBN dbn(numLayers, inputSize, hiddenSize, outputSize, iActFunc, hActFunc, W, B);

  // Setup TrainingSchedule to create training instances from observation files in desired order
  gomFS->setMinPastFrames( radius );
  gomFS->setMinFutureFrames( radius );

  if (oneHot) {
    if (labelOffset < gomFS->numContinuous()) {
      error("ERROR: labelOffset (%u) must refer to a discrete feature (the first %u are continuous)\n", 
	    labelOffset, gomFS->numContinuous());
    }
    if (labelOffset >= gomFS->numFeatures()) {
      error("ERROR: labelOffset (%u) is too large for the number of available features (%u)\n",
	    labelOffset, gomFS->numFeatures());
    }
  } else {
    if (labelOffset >= gomFS->numContinuous()) {
      error("ERROR: labelOffset (%u) is too large for the number of continuous features (%u)\n",
	    labelOffset, gomFS->numContinuous());
    }
    if (labelOffset + outputSize > gomFS->numContinuous()) {
      error("ERROR: labelOffset (%u) + number of outputs (%u) is too large for the number of continuous features (%u)\n", 
	    labelOffset, outputSize, gomFS->numContinuous());
    }
  }

  if (obsOffset >= gomFS->numContinuous()) {
    error("ERROR: featureOffset (%u) is too large for the number of continuous features (%u)\n",
	  obsOffset, gomFS->numContinuous());
  }
  if (obsOffset + numFeatures > gomFS->numContinuous()) {
    error("ERROR: featureOffset (%u) + numFeatures (%u) is too large for the number of continuous features (%u)\n", 
	  obsOffset, numFeatures, gomFS->numContinuous());
  }

  TrainingSchedule *trainSched;
  if (strcasecmp(trainingSchedule, "linear") == 0) {
    trainSched = new LinearSchedule(obsOffset, numFeatures, labelOffset, outputSize, oneHot, radius, 1, gomFS, trrng_str);
  } else if (strcasecmp(trainingSchedule, "random") == 0) {
    trainSched = new RandomSampleSchedule(obsOffset, numFeatures, labelOffset, outputSize, oneHot, radius, 1, gomFS, trrng_str);
  } else if (strcasecmp(trainingSchedule, "permute") == 0) {
    trainSched = new PermutationSchedule(obsOffset, numFeatures, labelOffset, outputSize, oneHot, radius, 1, gomFS, trrng_str);
  } else if (strcasecmp(trainingSchedule, "shuffle") == 0) {
    trainSched = new ShuffleSchedule(obsOffset, numFeatures, labelOffset, outputSize, oneHot, radius, 1, gomFS, trrng_str);
  } else {
    error("ERROR: unknown training schedule '%s', must be one of linear, random, shuffle, or permute\n", trainingSchedule);
  }

  BatchSource *batchSrc = new ScheduleBatchSource(trainSched);
#if 1
  BatchSource *asynchBatchSrc = new AsynchronousBatchSource(batchSrc, batchQueueSize);
#endif

  // Set hyperparameters (mostly from command line arguments)
  vector<DBN::HyperParams> pretrainHyperParams(numLayers);
  for (int j = 0; j < numLayers; j+=1) {
    pretrainHyperParams[j].initStepSize     = ptInitStepSize;
    pretrainHyperParams[j].minMomentum      = ptMinMomentum;
    pretrainHyperParams[j].maxMomentum      = ptMaxMomentum;
    pretrainHyperParams[j].maxUpdate        = ptMaxUpdate;
    pretrainHyperParams[j].l2               = ptL2;
    pretrainHyperParams[j].numUpdates       = (int)(0.5 + (ptNumEpochs * trainSched->numInstances()) / ptMiniBatchSize);
    pretrainHyperParams[j].numAnnealUpdates = (int)(0.5 + (ptNumAnnealEpochs * trainSched->numInstances()) / ptMiniBatchSize);
    pretrainHyperParams[j].miniBatchSize    = ptMiniBatchSize;
    pretrainHyperParams[j].checkInterval    = ptCheckInterval;
    pretrainHyperParams[j].iDropP           = 0; // no dropout in pretraining
    pretrainHyperParams[j].hDropP           = 0;
    pretrainHyperParams[j].pretrainType     = pretrainMode;
  }

  DBN::HyperParams bpHyperParams;
  bpHyperParams.initStepSize     = bpInitStepSize;
  bpHyperParams.minMomentum      = bpMinMomentum;
  bpHyperParams.maxMomentum      = bpMaxMomentum;
  bpHyperParams.maxUpdate        = bpMaxUpdate;
  bpHyperParams.l2               = bpL2;
  bpHyperParams.numUpdates       = (int)(0.5 + (bpNumEpochs * trainSched->numInstances()) / bpMiniBatchSize);
  bpHyperParams.numAnnealUpdates = (int)(0.5 + (bpNumAnnealEpochs * trainSched->numInstances()) / bpMiniBatchSize);
  bpHyperParams.miniBatchSize    = bpMiniBatchSize;
  bpHyperParams.checkInterval    = bpCheckInterval;
  bpHyperParams.iDropP           = bpIdropP;
  bpHyperParams.hDropP           = bpHdropP;

#if 0
  // Pack training instances into a Matrix for Galen's training code
  unsigned numUnits = trainSched->numTrainingUnitsPerEpoch();

  unsigned features_per_instance, instances_per_unit, dataSize, labelSize, labelStride;
  trainSched->describeFeatures(features_per_instance, instances_per_unit);
  dataSize = features_per_instance * instances_per_unit;
  double *ddata = new double[dataSize];

  trainSched->describeLabels(features_per_instance, instances_per_unit, labelStride);
  labelSize = instances_per_unit * features_per_instance;
  double *dlabel = new double[labelSize];

  MMapMatrix   trainData(inputSize,  numUnits, inputSize);
  MMapMatrix trainLabels(outputSize, numUnits, outputSize);

  unsigned segment, frame, destCol = 0;
  for (unsigned b=0; b < numUnits; b+=1) {
    unsigned length;
    trainSched->nextTrainingUnit(segment, frame);
    float *data = trainSched->getFeatures(segment, frame, length);
    for (unsigned i=0; i < dataSize; i+=1) ddata[i] = (double) data[i];
    trainData.PutCols(ddata, 1, inputSize, inputSize, destCol);

    data = trainSched->getLabels(segment, frame, length);
    for (unsigned i=0; i < labelSize; i+=1) dlabel[i] = (double) data[i];
    trainLabels.PutCols(dlabel, 1, outputSize, labelStride, destCol);

    destCol += 1;
  }
  delete[] ddata;
  delete[] dlabel;
#endif

  // Sending the process a usr1 signal will cause it to print out
  // status as if a check interval expired. If check intervals take
  // a long time to finish and you're worried the process has gone
  // awry, you can signal it to restore your faith :)
  signal(SIGUSR1, usr1_handler);

  // Do the actual training
  DBN::ObjectiveType objType = 
    ( dnn->getSquashFn(numLayers-1) == DeepNN::SOFTMAX ) ? DBN::SOFT_MAX : DBN::SQ_ERR;

#if 1
  // We want to minimize the size of the temporary files used to hold
  // the mapped inputs to the next layer for pre-training. If either
  // ptNumEpochs or ptNumAnnealEpochs are > 1, the code will just make
  // the temp files hold 1 epoch and cycle through them to get the desired
  // number of minibatches. If both are < 1, we need the larger of them to
  // supply enough minibatches.
  float ptFraction = max(ptNumEpochs, ptNumAnnealEpochs);
  dbn.Train(asynchBatchSrc, objType, pretrainHyperParams, bpHyperParams, ptFraction,
	    bpEpochFraction, bpAnnealEpochFraction, loadTrainingFile, saveTrainingFile);
#else
  dbn.Train(batchSrc, objType, pretrainHyperParams, bpHyperParams, 
	    bpEpochFraction, bpAnnealEpochFraction, loadTrainingFile, saveTrainingFile);
#endif

  // Now write training results back to master file(s)
  vector<DoubleMatrix *> layerMatrix = dnn->getMatrices();
  assert(layerMatrix.size() == numLayers);
  for (unsigned layer=0; layer < numLayers; layer+=1) {
    Matrix const W = dbn.getWeights(layer);
    dnn->setParams(layer, W.Start(), W.Ld(), dbn.getBias(layer).Start());
  }
  
  if (outputTrainableParameters != NULL) {
    char buff[2048];
    copyStringWithTag(buff,outputTrainableParameters,
		      CSWT_EMPTY_TAG,2048);
    oDataStreamFile of(buff,binOutputTrainableParameters);
    GM_Parms.writeTrainable(of);
  }

  // also write according to output master
  GM_Parms.write(outputMasterFile,cppCommandOptions);  

  getrusage(RUSAGE_SELF,&rue);
  if (IM::messageGlb(IM::Default)) { 
    infoMsg(IM::Default,"### Final time (seconds) just for DMLP training stage: ");
    double userTime,sysTime;
    reportTiming(rus,rue,userTime,sysTime,stdout);
  }
  MMapMatrix::GarbageCollect(); // delete left-over temp files
  exit_program_with_status(0);
}

