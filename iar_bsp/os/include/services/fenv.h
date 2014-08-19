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
*       fenv.h
*
*   COMPONENT
*
*       PX - POSIX
*
*   DESCRIPTION
*
*       Contains the definition of floating-point environment (FPE) for
*       POSIX implementation.  Note that this is required for PSE 52.
*
*       NOTE: The Nucleus POSIX implementation does not support the
*             Floating-Point Environment (FPE).  This file is
*             included for API conformance only.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       "config.h" - Nucleus POSIX configuration settings.
*       "stddef.h" - STDDEF related definitions.
*
*************************************************************************/

#ifndef NU_POSIX_FENV_H
#define NU_POSIX_FENV_H

#include "services/compiler.h"
#include "services/stddef.h"

/* FP Environment */

typedef int fenv_t;

/* FP Exception */

typedef int fexcept_t;

/* FPE Default Environment - This value represents the default floating
   - point environment (that is, the one installed at program startup).
   Note that the POSIX specification says that it must be a reference to
   a object that represents the default value. */

extern  const fenv_t    FENV_FE_DFL_ENV;
#define FE_DFL_ENV      &FENV_FE_DFL_ENV

/* FPE API Functions */

int  feclearexcept(int);
int  fegetexceptflag(fexcept_t *, int);
int  feraiseexcept(int);
int  fesetexceptflag(const fexcept_t *, int);
int  fetestexcept(int);
int  fegetround(void);
int  fesetround(int);
int  fegetenv(fenv_t *);
int  feholdexcept(fenv_t *);
int  fesetenv(const fenv_t *);
int  feupdateenv(const fenv_t *);

#endif /* NU_POSIX_FENV_H */
