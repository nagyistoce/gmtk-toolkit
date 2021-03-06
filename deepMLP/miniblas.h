#pragma once

/*
 * miniblas.cc - internal replacement functions for a few
 * BLAS 1 kernels
 *
 * Written by Richard Rogers <rprogers@ee.washington.edu>
 *
 * Copyright (C) 2013 Jeff Bilmes
 * Licensed under the Open Software License version 3.0
 * See COPYING or http://opensource.org/licenses/OSL-3.0
 *
 */

#if defined(HAVE_CONFIG_H)
#  include <config.h>
#endif

#if !defined(HAVE_BLAS) && !defined(HAVE_MKL)

// Simple-minded BLAS level 1 replacement kernels

void cblas_dcopy(int n, double const* x, int incx, double *y, int incy);

void cblas_dscal(int n, double alpha, double *x, int incx);

void cblas_daxpy(int n, double alpha, double const *x, int incx, double *y, int incy);

double cblas_ddot(int n, double const *x, int incx, double const *y, int incy);

double cblas_dasum(int n, double const *x, int incx);

double cblas_dnrm2(int n, double const *x, int incx);

#endif
