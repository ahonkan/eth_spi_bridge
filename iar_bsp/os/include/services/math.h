/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       math.h
*
*   COMPONENT
*
*        RTL -  RunTime Library
*
*   DESCRIPTION
*
*        Contains the various mathematical declarations.
*
*   DATA STRUCTURES
*
*       float_t             A real-floating type at least as wide as
*                           float.
*       double_t            A real-floating type at least as wide as
*                           double.
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*
*************************************************************************/

#ifndef NU_PSX_MATH_H
#define NU_PSX_MATH_H

#include "services/config.h"
#include "services/compiler.h"

/* For Metaware Metrowerks and KMC GNU Tools */
#ifndef _MATH_H
#define _MATH_H

/* For ADS Tools */
#ifndef __math_h
#define __math_h

/* For Hitachi Tools and TI Tools  */
#ifndef _MATH
#define _MATH

/* For Paradigm Tools and Microtec Tools */
#ifndef __MATH_H
#define __MATH_H

/* For Microsoft Visual C */
#ifndef _INC_MATH
#define _INC_MATH

#ifndef __MATH_H_
#define __MATH_H_

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef  _MATH_H_
#define  _MATH_H_

/* For DIAB tools */
#ifndef __Imath
#define __Imath

typedef float   float_t;
typedef double  double_t;

#define     M_E                 2.7182818284590452
#define     M_LOG2E             1.4426950408889634
#define     M_LOG10E            0.4342944819032518
#define     M_LN2               0.6931471805599453
#define     M_LN10              2.3025850929940457
#define     M_PI                3.1415926535897932
#define     M_PI_2              1.5707963267948966
#define     M_PI_4              0.7853981633974483
#define     M_1_PI              0.3183098861837907
#define     M_2_PI              0.6366197723675813
#define     M_2_SQRTPI          1.1283791670955126
#define     M_SQRT2             1.4142135623730950
#define     M_SQRT1_2           0.7071067811865475

extern const double __huge_val_;
#define HUGE_VAL (__huge_val_)

#define     HUGE_VALF

#define     HUGE_VALL

#define     INFINITY

#define     NAN

#define     MATH_ERRNO          1

#define     MATH_ERREXCEPT      2

extern int  math_errhandling;

#ifdef __cplusplus
extern "C" {
#endif

double acos(double);
double asin(double);
double atan(double);
double atan2(double, double);
double ceil(double);
long double ceill(long double);
double cos(double);
double cosh(double);
double exp(double);
double fabs(double);
double floor(double);
double fmod(double,double);
double modf(double, double *);
double frexp(double,int *);

#ifdef __cplusplus

    #if defined(PSX_MTECPPC) || defined(PSX_MTECCF) || \
        defined(PSX_MTEC68K) || defined(PSX_EDGEARM)

    #else

double hypot(double,double);

    #endif /* PSX_MTECPPC, PSX_MTECCF, PSX_MTEC68K, PSX_EDGEARM */

#else

double hypot(double,double);

#endif /* __cplusplus */

double ldexp(double, int);
double log(double);
double log10(double);
long double log1pl(long double);
double pow(double, double);
double sin(double);
double sinh(double);
double sqrt(double);
double tan(double);
double tanh(double);

#ifdef __cplusplus
}
#endif

#endif  /* __Imath */
#endif  /* _MATH_H_ */
#endif  /* __MATH_H_ */
#endif  /* _INC_MATH */
#endif  /* __MATH_H */
#endif  /* _MATH */
#endif  /* __math_h */
#endif  /* _MATH_H */

#endif /* NU_PSX_MATH_H */
