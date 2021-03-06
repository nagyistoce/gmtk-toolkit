/*
**
**
** Copyright (C) 2010 Jeff Bilmes
** Licensed under the Open Software License version 3.0
** See COPYING or http://opensource.org/licenses/OSL-3.0
**
** This code will set up the FPU on x86 archiectures to trap with
** SIGFPEs when an inf or nan occurs (or some other fp exception depending
** on what is uncommented below).
**
** The code is very machine specific to Linux (and probably to a particular
** version of Linux).
**
** Jeff Bilmes
** <bilmes@ee.washington.edu> 
**
**
*/

#ifndef IEEESETUP_H
#define IEEESETUP_H

extern void ieeeFPsetup();

#endif

