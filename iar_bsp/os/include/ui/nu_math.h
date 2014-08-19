/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME                                             
*
*  nu_math.h                                                    
*
* DESCRIPTION
*
*  Fixed-point math include file.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
* DEPENDENCIES
*
*  None.
*
***************************************************************************/
#ifndef _NU_MATH_H_
#define _NU_MATH_H_

/* To use floating point math functions uncomment this define */
/* #define CPU_FLOAT */

#ifdef CPU_FLOAT
#include <math.h>

double MATH_sin(double x, double y, INT32 iy);
double MATH_cos(double x, double y);
double MATH_sqr(double b,double a,double v);
double MATH_sqrt(double v);

#else

#include "nucleus.h"
#define FIXPOINT

/* Local Functions */
SIGNED Fix_mul( SIGNED first, SIGNED second);
SIGNED Fix_sqrt( UNSIGNED root);
SIGNED Fix_div( SIGNED a, SIGNED b);
SIGNED iCos( SIGNED Angle);
SIGNED iSin( SIGNED Angle);
SIGNED Nu_asin( SIGNED radian);


/* 64-bit fixed point */
typedef struct _dblFix{
    SIGNED   fUpper;
    UNSIGNED fLower;
} dblFix;

INT32 dFix_add( dblFix *first, dblFix *second, dblFix *result);
INT32 dFix_sub( dblFix *first, dblFix *second, dblFix *result);
INT32 dFix_mul( dblFix *first, dblFix *second, dblFix *result);
INT32 dFix_div( dblFix *first, dblFix *second, dblFix *result);

#endif


#endif /* _NU_MATH_H_ */




