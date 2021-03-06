/*
 *
 * Written by Katrin Kirchhoff <katrin@ee.washington.edu>
 *
 * Modified by Karim Filali <karim@cs.washington.edu> to add the
 * option to pipe the list of file names through CPP.  Made a few
 * other minor bug fixes.
 *
 * Also, added a sentence range.
 *
 * $Header$
 *
 * Copyright (C) 2001 Jeff Bilmes
 * Licensed under the Open Software License version 3.0
 * See COPYING or http://opensource.org/licenses/OSL-3.0
 *
 * */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include "GMTK_Stream.h"

#include "error.h"
#include "general.h"

#if 0
// karim - 29aug2003
#include "fileParser.h"
#endif

#ifdef PIPE_ASCII_FILES_THROUGH_CPP
#ifndef DECLARE_POPEN_FUNCTIONS_EXTERN_C
extern "C" {
#ifdef __CYGWIN__
     FILE     *popen(const char *, const char *) __THROW;
     int pclose(FILE *stream) __THROW;
#endif
}
#endif
#endif

#define CPP_DIRECTIVE_CHAR '#'


#ifdef DEBUG
#define DBGFPRINTF(_x_) fprintf _x_
#else
#define DBGFPRINTF(_x_)
#endif



HTKFileInfo::HTKFileInfo(int samp_size, int n_samples, int startOfData,
						 bool isCompressed, float* scale, float* offset):
	samp_size(samp_size), n_samples(n_samples), startOfData(startOfData),
	isCompressed(isCompressed), scale(scale), offset(offset){
	
}

HTKFileInfo::~HTKFileInfo(){
	if(scale)
		delete [] scale;
	if(offset)
		delete [] offset;
}


StreamInfo::StreamInfo(const char *name, const char *crng_str,
		       const char *drng_str,
		       unsigned *nfloats, unsigned *nints, 
		       unsigned *format, bool swap, unsigned num,bool cppIfAscii,char* cppCommandOptions, const char* sr_range_str) 
  : cppIfAscii(cppIfAscii),cppCommandOptions(cppCommandOptions),numFileNames(0),srRng(NULL), pfile_istr(NULL),curDataFile(NULL),curHTKFileInfo(NULL), dataNames(NULL), cont_rng(NULL),disc_rng(NULL)
{

  if (name == NULL) 	
    error("StreamInfo: File name is NULL for stream %i\n",num);	
  
  // local copy of file name
  fofName = new char[strlen(name)+1];
  strcpy(fofName,name);
  
  if (format == NULL)
    error("StreamInfo: Data format unspecified for stream %i\n",num);
   
   dataFormat = *format;
   
   if (nfloats == NULL)
     error("StreamInfo: Number of floats unspecified for stream %i\n",num);

   nFloats = *nfloats;

   if (nints == NULL) 
      error("StreamInfo: Number of ints unspecified for stream %i\n",num);

   nInts = *nints;

   // Update: this is already taken car of when {c|d}rng_str eq "nil" or "none"
   ////// Special cases {c|d}rng_str eq "-1"
   //if(crng_str != NULL && strcmp(crng_str,"-1")==0) {
     // empty range
   //cont_rng = new Range(NULL,0,0);
   //}
   //else {
   cont_rng = new Range(crng_str,0,nFloats);
   //}
   //if(drng_str != NULL && strcmp(drng_str,"-1")==0) {
   //disc_rng = new Range(NULL,0,0);  // empty range
   //}
   //else {
   disc_rng = new Range(drng_str,0,nInts);
   //}



   // If in the future we want to have the option to append deltas and
   // double deltas, it should be taken into account below.

   nFloatsUsed = cont_rng->length();
   nIntsUsed = disc_rng->length();

   if( (nFloatsUsed + nIntsUsed) == 0) {
       warning("WARNING: No features were selected for stream %i\n",num);
   }

   bswap = swap;

   if (dataFormat == PFILE) {

     if ((curDataFile = fopen(fofName,"rb")) == NULL)
       error("StreamInfo: Can't open '%s' for input\n", fofName);

     pfile_istr = new InFtrLabStream_PFile(0,fofName,curDataFile,1,bswap);
     
     fullFofSize = pfile_istr->num_segs();
     
     if (pfile_istr->num_ftrs() != nFloats) 
       error("StreamInfo: File %s has %i floats, expected %i",
	     fofName,
	     pfile_istr->num_ftrs(),
	     nFloats);
     
     // in a pfile, only the labs can be ints
     
     if (pfile_istr->num_labs() != nInts)
       error("StreamInfo: File %s has %i ints, expected %i",
	     fofName,
	     pfile_istr->num_labs(),
	     nInts);
   }
   else {
     pfile_istr = NULL;
#ifdef PIPE_ASCII_FILES_THROUGH_CPP     
     if(cppIfAscii) { 
       string cppCommand = CPP_Command();
       if (cppCommandOptions != NULL) {
	 cppCommand = cppCommand + string(" ") + string(cppCommandOptions);
       }
       // make sure the file  exists first.
       if ((fofFile = ::fopen(fofName,"r")) == NULL) {
	 error("ERROR: unable to open file (%s) for reading",fofName);
       }
       fclose(fofFile);
       cppCommand = cppCommand + string(" ") + string(fofName);
       fofFile = ::popen(cppCommand.c_str(),"r");    
       if (fofFile == NULL)
	 error("ERROR, can't open file stream from (%s)",fofName);
     }
     else {
       if ((fofFile = fopen(fofName,"r")) == NULL)
	 error("StreamInfo: Can't open '%s' for input\n",fofName);
     }
#else
      if ((fofFile = fopen(fofName,"r")) == NULL)
	 error("StreamInfo: Can't open '%s' for input\n",fofName);
#endif

     fullFofSize = readFof(fofFile);

#ifdef PIPE_ASCII_FILES_THROUGH_CPP     
     if(cppIfAscii) {
       // first, scan until end of file since sometimes it appears
       // that cosing a pipe when not at the end causes an error (e.g., mac osx).
       freadUntilEOF(fofFile);
       if (pclose(fofFile) != 0) {
	 // we don' give a warning here since sometimes 'cpp' might return
	 // with an error that we really don't care about. TODO: the proper
	 // thing to do here is no to use 'cpp' as a pre-processor and use
	 // some other macro preprocessor (such as m4).
	 // warning("WARNING: Can't close pipe '%s %s'.",CPP_Command());
       }
     }
     else
       fclose(fofFile);
#else
     fclose(fofFile);
#endif
   }
   
   srRng= new Range(sr_range_str,0,fullFofSize);
   assert(srRng != NULL);
   if(!fullFofSize || (unsigned) srRng->last() >= (unsigned) fullFofSize)
     error("ERROR: Specified per-stream sentence range (%s) exceeds total number of sentences (%d).",srRng->GetDefStr(),fullFofSize);
   
   fofSize = srRng->length();
}


StreamInfo::~StreamInfo() {

  if (fofName != NULL)
    delete [] fofName;

  if (dataFormat != PFILE) {
    if (dataNames != NULL) {
      for (unsigned i = 0; i < fofSize; i++)
	delete [] dataNames[i];
      delete [] dataNames;
    }
  }
  else 
    delete pfile_istr;

  if(curDataFile){
      fclose(curDataFile);
      curDataFile = NULL;
  }

  if(curHTKFileInfo){
   	  delete curHTKFileInfo;
   	  curHTKFileInfo= NULL;
  }
  
  if (cont_rng != NULL)
    delete cont_rng;   
  if (disc_rng != NULL)
    delete disc_rng;

  if (srRng != NULL)
    delete srRng;

}



/**
 *  calcNumFileNames -- calculate the number of file names in the file pointed to by the file handle f
 *
 *  pre-conditions: the file name, fofName, must be initialized 
 *
 *  side effects: if f is a CPP pipe, the file is closed and re-opened
 *  in order to achieve the same effect as a rewind
 * */
size_t StreamInfo::calcNumFileNames(FILE* &f) {
  char line[MAXSTRLEN];

  numFileNames = 0;
#ifdef PIPE_ASCII_FILES_THROUGH_CPP     
  if(cppIfAscii) {
    while (fgets(line,sizeof(line),f) != NULL) {
      int l = strlen(line);
      if(l==0 || (l==1 && line[0]=='\n')) continue;
      if(line[0]==CPP_DIRECTIVE_CHAR) continue;  // lines that start with # are CPP directives 
      numFileNames++;
    }
    // since it's a pipe we need to close it and reopen it
    pclose(f);
    string cppCommand = CPP_Command();
    if (cppCommandOptions != NULL) {
      cppCommand = cppCommand + string(" ") + string(cppCommandOptions);
    }
    cppCommand = cppCommand + string(" ") + string(fofName);
    f = ::popen(cppCommand.c_str(),"r");    
    if (f == NULL)
      error("ERROR, can't open file stream from (%s)",fofName);
  } 
  else {
    while (fgets(line,sizeof(line),f) != NULL) {
      int l = strlen(line);
      if(l==0 || (l==1 && line[0]=='\n') || line[0]==CPP_DIRECTIVE_CHAR) continue;
      numFileNames++;
    }
    rewind(f);
  }
#else
   while (fgets(line,sizeof(line),f) != NULL) {
      int l = strlen(line);
      if(l==0 || (l==1 && line[0]=='\n') ) continue;
      numFileNames++;
    }
    rewind(f);
#endif

  return numFileNames;

}	

size_t StreamInfo::readFof(FILE * &f) {

  size_t n_lines = 0;

  numFileNames = 0;
  char line[MAXSTRLEN];

  numFileNames=calcNumFileNames(f);
  dataNames = new char*[numFileNames];

  n_lines = 0;
  while (fgets(line,sizeof(line),f) != NULL) {
    int l = strlen(line);
    if(l==0|| (l==1 && line[0]=='\n')) continue;
    if(line[0]==CPP_DIRECTIVE_CHAR) continue;
    if (line[l-1] != '\n') {
      if (n_lines < numFileNames-1) 
	error("StreamInfo::readFof: line %i too long in file '%s' - increase MAXSTRLEN (currently set to %d) or decrease your line lengths.\n",
	      n_lines+1,fofName,MAXSTRLEN);
    }
    else
      line[l-1] = '\0';
    
    dataNames[n_lines] = new char[l+1];
    strcpy(dataNames[n_lines],line);
    n_lines++;
  }
  assert(numFileNames==n_lines);
  return n_lines;
}

