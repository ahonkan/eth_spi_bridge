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
*       string.h
*
*   COMPONENT
*
*       RTL - RunTime Library.
*
*   DESCRIPTION
*
*       This file contains the string operations.
*
*       NOTE: Standard C RTL header.
*
*   DATA STRUCTURES
*
*       size_t          An unsigned integer data type.
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*       unistd.h            POSIX unistd.h definitions.
*
*************************************************************************/

#ifndef NU_PSX_STRING_H
#define NU_PSX_STRING_H

#include "services/config.h"
#include "services/compiler.h"
#include "services/stddef.h"

/* For Metaware Metrowerks and KMC GNU Tools */
#ifndef _STRING_H
#define _STRING_H

/* For ADS Tools */
#ifndef __string_h
#define __string_h

/* For Hitachi Tools and TI Tools  */
#ifndef _STRING
#define _STRING

/* For Paradigm Tools and Microtec Tools */
#ifndef __STRING_H
#define __STRING_H

/* For Microsoft Visual C */
#ifndef _INC_STRING
#define _INC_STRING

#ifndef __STRING_H_
#define __STRING_H_

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef _STRING_H_
#define _STRING_H_

/* For DIAB tools */
#ifndef __Istring
#define __Istring

#ifdef __cplusplus
extern "C" {
#endif

/* Null pointer constant. */
#ifndef NULL
#define NULL            0
#endif /* NULL */

void *  memchr(const void *, int, size_t);
int     memcmp(const void *, const void *, size_t);
void *  memcpy(void *, const void *, size_t);
void *  memmove(void *, const void *, size_t);
void *  memset(void *, int, size_t);
char *  strcat(char *, const char *);
char *  strchr(const char *, int);
int     strcmp(const char *, const char *);
int     strcoll(const char *, const char *);
char *  strcpy(char *, const char *);
size_t  strcspn(const char *, const char *);
char *  strdup(const char *);
char *  strerror(int);
size_t  strlen(const char *);
char *  strncat(char *, const char *, size_t);
int     strncmp(const char *, const char *, size_t);
char *  strncpy(char *, const char *, size_t);
char *  strpbrk(const char *, const char *);
char *  strrchr(const char *, int);
size_t  strspn(const char *, const char *);
char *  strstr(const char *, const char *);
char *  strtok(char *, const char *);
size_t  strxfrm(char *, const char *, size_t);

#if (_POSIX_THREAD_SAFE_FUNCTIONS != -1)

int     strerror_r(int, char *, size_t);
char *  strtok_r(char *, const char *, char **);

#endif  /* _POSIX_THREAD_SAFE_FUNCTIONS */

#ifdef __cplusplus
}
#endif

#endif /* __Istring */
#endif /* _STRING_H_ */
#endif /* __STRING_H_ */
#endif /* _INC_STRING */
#endif /* __STRING_H */
#endif /* _STRING */
#endif /* __string_h */
#endif /* _STRING_H */

#endif /* NU_PSX_STRING_H */
