/*-
 * GMTK_RealMatrix.cc
 *     General matrix class (for means, etc.)
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

#include "general.h"
#include "error.h"

#include "GMTK_WeightMatrix.h"


#if HAVE_CONFIG_H
#include <config.h>
#endif
#if HAVE_HG_H
#include "hgstamp.h"
#endif
VCID(HGID)



////////////////////////////////////////////////////////////////////
//        General create, read, destroy routines 
////////////////////////////////////////////////////////////////////


/*-
 *-----------------------------------------------------------------------
 * WeightMatrix::WeightMatrix()
 *      Constructor
 *
 * Results:
 *      Constructs the object.
 *
 * Side Effects:
 *      None so far.
 *
 *-----------------------------------------------------------------------
 */
WeightMatrix::WeightMatrix() 
{
}


/*-
 *-----------------------------------------------------------------------
 * WeightMatrix::read(is)
 *      read in the array from file 'is'. 
 *      The data probs are stored as doubles, but when they are read in
 *      they are converted to the log domain.
 * 
 * Results:
 *      No results.
 *
 * Side Effects:
 *      Changes the pmf member function in the object.
 *
 *-----------------------------------------------------------------------
 */
void
WeightMatrix::read(iDataStreamFile& is)
{
  NamedObject::read(is);
  is.read(_rows,"WeightMatrix::read, distribution rows");
  if (_rows <= 0)
    error("WeightMatrix: read rows (%d) < 0 in input",_rows);

  is.read(_cols,"WeightMatrix::read, distribution cols");
  if (_cols <= 0)
    error("WeightMatrix: read cols (%d) < 0 in input",_cols);

  weights.resize(_rows*_cols);

  float* ptr = weights.ptr;
  for (int i=0;i<_rows*_cols;i++) {
    float val;
    is.read(val,"WeightMatrix::read, reading value");
    *ptr++ = val;
  }
  setBasicAllocatedBit();
}




/*-
 *-----------------------------------------------------------------------
 * WeightMatrix::write(os)
 *      write out data to file 'os'. 
 * 
 * Results:
 *      No results.
 *
 * Side Effects:
 *      No effects other than  moving the file pointer of os.
 *
 *-----------------------------------------------------------------------
 */
void
WeightMatrix::write(oDataStreamFile& os)
{
  assert ( basicAllocatedBitIsSet() );

  NamedObject::write(os);
  os.write(_rows,"WeightMatrix::write, distribution rows");
  os.write(_cols,"WeightMatrix::write, distribution cols");
  for (int i=0;i<_rows*_cols;i++) {
    if (i % _cols == 0)
      os.nl();
    os.write(weights[i],"WeightMatrix::write, writing value");
  }
  os.nl();
}



////////////////////////////////////////////////////////////////////
//        Misc Support
////////////////////////////////////////////////////////////////////
