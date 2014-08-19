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
*       ctype.h
*
*   COMPONENT
*
*       RTL - RunTime Library.
*
*   DESCRIPTION
*
*       Contains the character types definitions.
*
*       NOTE: Standard C RTL header.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*
*************************************************************************/

#ifndef NU_PSX_CTYPE_H
#define NU_PSX_CTYPE_H

#include "services/config.h"
#include "services/compiler.h"

/* For Metaware Metrowerks and KMC GNU Tools */
#ifndef _CTYPE_H
#define _CTYPE_H

/* For ADS Tools */
#ifndef __ctype_h
#define __ctype_h

/* For Hitachi Tools and TI Tools  */
#ifndef _CTYPE
#define _CTYPE

/* For Paradigm Tools and Microtec Tools */
#ifndef __CTYPE_H
#define __CTYPE_H

/* For Microsoft Visual C */
#ifndef _INC_CTYPE
#define _INC_CTYPE

#ifndef __CTYPE_H_
#define __CTYPE_H_

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef _CTYPE_H_
#define _CTYPE_H_

/* For DIAB tools */
#ifndef __Ictype
#define __Ictype

#ifdef __cplusplus
extern "C" {
#endif

/* API functions */
int isalnum(int);
int isalpha(int);
int isascii(int);
int isblank(int);
int iscntrl(int);
int isdigit(int);
int isgraph(int);
int islower(int);
int isprint(int);
int ispunct(int);
int isspace(int);
int isupper(int);
int isxdigit(int);
int toascii(int);
int tolower(int);
int toupper(int);

#ifdef __cplusplus
}
#endif

#endif /* __Ictype */
#endif /* _CTYPE_H_ */
#endif /* __CTYPE_H_ */
#endif /* _INC_CTYPE */
#endif /* __CTYPE_H */
#endif /* _CTYPE */
#endif /* __ctype_h */
#endif /* _CTYPE_H */

#endif /* NU_PSX_CTYPE_H */
