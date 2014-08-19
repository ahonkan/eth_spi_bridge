/*************************************************************************
*
*              Copyright 2013 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       nussh_unimp.h
*
*   DESCRIPTION
*
*       This file defines unimplemented routines for dropbear SSH.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*
************************************************************************/
#ifndef NUSSH_UNIMP_H_
#define NUSSH_UNIMP_H_
#include "nussh_includes.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG_TRACE
#define UNIMPLEMENTED unimplemented(__FILE__, __LINE__)
#else
#define UNIMPLEMENTED unimplemented()
#endif

/** File System **/
#define chdir(...)    UNIMPLEMENTED
#define chmod(...)    UNIMPLEMENTED
#define chown(...)    UNIMPLEMENTED
#define fcntl(...)    UNIMPLEMENTED
#define fgetc(...)    UNIMPLEMENTED
#define fputs(...)    UNIMPLEMENTED
#define stat(...)     UNIMPLEMENTED
#define unlink(...)   UNIMPLEMENTED


/** System **/
#define dup2(...)            UNIMPLEMENTED
#define daemon(...)          UNIMPLEMENTED
#define execv(...)           UNIMPLEMENTED
#define fileno(...)          UNIMPLEMENTED
#define fork(...)            UNIMPLEMENTED
#define fstat(...)           UNIMPLEMENTED
#define geteuid(...)         UNIMPLEMENTED
#define getgrnam(...)        UNIMPLEMENTED
#define getuid(...)          UNIMPLEMENTED
#define initgroups(...)      UNIMPLEMENTED
#define ioctl(...)           UNIMPLEMENTED
#define kill(...)            UNIMPLEMENTED
#define open(...)            UNIMPLEMENTED
#define pipe(...)            UNIMPLEMENTED
#define putenv(...)          UNIMPLEMENTED
#define raise(...)           UNIMPLEMENTED
#define setgid(...)          UNIMPLEMENTED
#define setrlimit(...)       UNIMPLEMENTED
#define setsid(...)          UNIMPLEMENTED
#define setuid(...)          UNIMPLEMENTED
#define sigaction(...)       UNIMPLEMENTED
#define sigemptyset(...)     UNIMPLEMENTED
#define signal(...)          UNIMPLEMENTED
#define tcsetattr(...)       UNIMPLEMENTED
#define waitpid(...)         UNIMPLEMENTED


/* function declaration */
int unimplemented(
#ifdef DEBUG_TRACE
		CHAR *file, INT line
#else
		VOID
#endif /*DEBUG_TRACE*/
		);

#ifdef __cplusplus
}
#endif

#endif /* NUSSH_UNIMP_H_ */
