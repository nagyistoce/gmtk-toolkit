/*
 * GMTK_RV.cc
 *
 * Written by Jeff Bilmes <bilmes@ee.washington.edu>
 *  $Header$
 *
 * Copyright (c) 2001, < fill in later >
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any non-commercial purpose
 * and without fee is hereby granted, provided that the above copyright
 * notice appears in all copies.  The University of Washington,
 * Seattle make no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 *
 * The top level GMTK random variable object for the RV class hierarchy.
 *
 *
 *
 */


#include "general.h"
VCID("$Header$");

#include <iostream>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <string.h>


#include "GMTK_RV.h"


/*-
 *-----------------------------------------------------------------------
 * printParentInfo()
 *      print information about all parents of this RV.
 *
 * Preconditions:
 *      all parents and all children member variables must already
 *      be initialized with a valid graph
 *
 * Postconditions:
 *      parent info printed.
 *
 * Side Effects:
 *      none
 *
 * Results:
 *      none
 *
 *-----------------------------------------------------------------------
 */

void RV::printParentInfo(FILE*f, bool nl)
{
  for (unsigned i=0;i<allParents.size();i++) {
    allParents[i]->printNameFrameValue(f,false);
    if (i < (allParents.size()-1))
      fprintf(f,",");
  }
  pnl(f,nl);
}



/*-
 *-----------------------------------------------------------------------
 * printChildrenInfo()
 *      print information about all children of this RV.
 *
 * Preconditions:
 *      all parents and all children member variables must already
 *      be initialized with a valid graph
 *
 * Postconditions:
 *      parent info printed.
 *
 * Side Effects:
 *      none
 *
 * Results:
 *      none
 *
 *-----------------------------------------------------------------------
 */

void RV::printChildrenInfo(FILE*f, bool nl)
{
  for (unsigned i=0;i<allChildren.size();i++) {
    allChildren[i]->printNameFrameValue(f,false);
    if (i < (allChildren.size()-1))
      fprintf(f,",");
  }
  pnl(f,nl);
}



/*-
 *-----------------------------------------------------------------------
 * createNeighborsFromParentsChildren()
 *      initializes the neighbors member set with
 *      entries from all parents and all children.
 *
 * Preconditions:
 *      all parents and all children member variables must already
 *      be initialized with a valid graph
 *
 * Postconditions:
 *      graph now has undirected representation, where neighbors
 *      represents the undirected edges.
 *
 * Side Effects:
 *      none
 *
 * Results:
 *      neighbors is changed.
 *
 *-----------------------------------------------------------------------
 */

void RV::createNeighborsFromParentsChildren()
{
  // make sure it is clear first
  neighbors.clear();
  // a neighbor is a parent or a child at this point.
  for (unsigned i=0;i<allParents.size();i++) {
    neighbors.insert(allParents[i]);
  }
  for (unsigned i=0;i<allChildren.size();i++) {
    neighbors.insert(allChildren[i]);
  }
  assert ( neighbors.find(this) == neighbors.end() );
}




/*-
 *-----------------------------------------------------------------------
 * connectNeighbors()
 *      Makes it such that all neighbors of self are
 *      connected (but does not touch the varibles in the
 *      'exclude' argument. This function can therefore be used
 *      as part of a node 'elimination' process.
 *
 * Preconditions:
 *      createNeighborsFromParentsChildren() must have been
 *      called at some point before.
 *
 * Postconditions:
 *      node is now such that self and all neighbors form
 *      a complete set.
 *
 * Side Effects:
 *      will change the neighbors member variables of other variables.
 *
 * Results:
 *      none.
 *
 *-----------------------------------------------------------------------
 */

void RV::connectNeighbors(set<RV*> exclude)
{
  set<RV*> nodes;

  set_difference(neighbors.begin(),neighbors.end(),
		 exclude.begin(),exclude.end(),
		 inserter(nodes,nodes.end()));
  for (set<RV*>::iterator n = nodes.begin();
       n != nodes.end();
       n++) {
    // just union together each nodes's neighbor variable
    // with self nodes
    set<RV*> tmp;
    set_union((*n)->neighbors.begin(),(*n)->neighbors.end(),
	      nodes.begin(),nodes.end(),
	      inserter(tmp,tmp.end()));
    // make sure self is not its own neighbor
    tmp.erase((*n));
    (*n)->neighbors = tmp;
  }

}



/*-
 *-----------------------------------------------------------------------
 * moralize()
 *      moralize the node, i.e., make sure that all parents of
 *      this node are neighbors of each other.
 *
 * Preconditions:
 *      createNeighborsFromParentsChildren() must have
 *      been run for *all* parents of this node.
 *
 * Postconditions:
 *      all parents are now neighbors.
 *
 * Side Effects:
 *      none
 *
 * Results:
 *      parent's neighbors member variable is changed.
 *
 *-----------------------------------------------------------------------
 */

void RV::moralize()
{
  for (unsigned i=0;i<allParents.size();i++) {
    for (unsigned j=i+1;j<allParents.size();j++) {
      if (allParents[i]->neighbors.find(allParents[j])
	  == allParents[i]->neighbors.end()) {
	allParents[i]->neighbors.insert(allParents[j]);
	allParents[j]->neighbors.insert(allParents[i]);
      }
    }
  }
}



/*-
 *-----------------------------------------------------------------------
 * allParentsContainedInSet()
 *      Returns true if all parents are contained within the given set.
 *
 * Preconditions:
 *      allParents member must be created.
 *
 * Postconditions:
 *      none
 *
 * Side Effects:
 *      none
 *
 * Results:
 *      bool
 *
 *-----------------------------------------------------------------------
 */
bool RV::allParentsContainedInSet(const set <RV*> givenSet)
{
  set <RV*> res;
  set_intersection(givenSet.begin(),givenSet.end(),
		   allParents.begin(),allParents.end(),
		   inserter(res,res.end()));
  return (res.size() == allParents.size());
}




/*-
 *-----------------------------------------------------------------------
 * setParents()
 *      Set the parents to the given values. Works with variables
 *      that do not have switching. Also sets children.
 *
 * Preconditions:
 *      'this' Variable must not have switching.
 *
 * Postconditions:
 *      parent are re-set.
 *
 * Side Effects:
 *      parent are re-set.
 *
 * Results:
 *      none
 *
 *-----------------------------------------------------------------------
 */

void RV::setParents(vector<RV *> &sparents,vector<vector<RV *> > &cpl)
{
  // since this is for a non-switching RV, we include the following
  // assertions.
  assert ( sparents.size() == 0 );
  assert ( cpl.size() == 1 );
  
  allParents = cpl[0];

  // now set this as a child of all parents, making sure to avoid
  // duplicates by creating a temporary set.
  set<RV *> parentSet;
  for (unsigned i=0;i<allParents.size();i++) {
    parentSet.insert(allParents[i]);
  }
  set<RV *>::iterator si;
  for (si = parentSet.begin(); si != parentSet.end(); si++) {
    RV* rv = (*si);
    rv->allChildren.push_back(this);
  }



}




/*-
 *-----------------------------------------------------------------------
 * printRVSet{,AndValues}()
 *      Prints out the set of random variables and perhaps their values as well.
 *
 * Preconditions:
 *      f must be open, locset a set of RVs.
 *
 * Postconditions:
 *      none
 *
 * Side Effects:
 *      none
 *
 * Results:
 *      void
 *
 *-----------------------------------------------------------------------
 */
void printRVSetAndValues(FILE*f,vector<RV*>& locset,const bool nl) 
{
  bool first = true;
  for (unsigned i=0;i<locset.size();i++) {
    RV* rv = locset[i];
    if (!first)
      fprintf(f,",");
    rv->printNameFrameValue(f,false);
    first = false;
  }
  if (nl) fprintf(f,"\n");
}
void printRVSetAndValues(FILE*f,sArray<RV*>& locset,const bool nl) 
{
  bool first = true;
  for (unsigned i=0;i<locset.size();i++) {
    RV* rv = locset[i];
    if (!first)
      fprintf(f,",");
    rv->printNameFrameValue(f,false);
    first = false;
  }
  if (nl) fprintf(f,"\n");
}
void printRVSetAndValues(FILE*f,set<RV*>& locset,bool nl)
{
  bool first = true;
  set<RV*>::iterator it;
  for (it = locset.begin();
       it != locset.end();it++) {
    RV* rv = (*it);
    if (!first)
      fprintf(f,",");
    rv->printNameFrameValue(f,false);
    first = false;
  }
  if (nl) fprintf(f,"\n");
}


void printRVSet(FILE*f,vector<RV*>& locset,const bool nl) 
{
  bool first = true;
  for (unsigned i=0;i<locset.size();i++) {
    RV* rv = locset[i];
    if (!first)
      fprintf(f,",");
    rv->printNameFrame(f,false);
    first = false;
  }
  if (nl) fprintf(f,"\n");
}
void printRVSet(FILE*f,sArray<RV*>& locset,const bool nl) 
{
  bool first = true;
  for (unsigned i=0;i<locset.size();i++) {
    RV* rv = locset[i];
    if (!first)
      fprintf(f,",");
    rv->printNameFrame(f,false);
    first = false;
  }
  if (nl) fprintf(f,"\n");
}
void printRVSet(FILE*f,set<RV*>& locset,bool nl)
{
  bool first = true;
  set<RV*>::iterator it;
  for (it = locset.begin();
       it != locset.end();it++) {
    RV* rv = (*it);
    if (!first)
      fprintf(f,",");
    rv->printNameFrame(f,false);
    first = false;
  }
  if (nl) fprintf(f,"\n");
}



void
printRVSetPtr(FILE*f,set<RV*>& locset,bool nl)
{
  bool first = true;
  set<RV*>::iterator it;
  for (it = locset.begin();
       it != locset.end();it++) {
    RV* rv = (*it);
    if (!first)
      fprintf(f,",");
    fprintf(f,"%s(%d)=0x%X",rv->name().c_str(),rv->frame(),(unsigned)rv);
    first = false;
  }
  if (nl) fprintf(f,"\n");
}



