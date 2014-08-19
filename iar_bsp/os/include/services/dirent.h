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
*       dirent.h
*
*   COMPONENT
*
*       Nucleus POSIX - file system
*
*   DESCRIPTION
*
*       This file contains the format of directory entries.
*
*   DATA STRUCTURES
*
*       DIR                                 Holds directory related
*                                           information.
*
*   DEPENDENCIES
*
*       limits.h
*       types.h
*
*************************************************************************/

#ifndef NU_PSX_DIRENT_H
#define NU_PSX_DIRENT_H

#include "services/config.h"
#include "services/limits.h"
#include "services/sys/types.h"

/* For DIAB tools */
#ifndef __Idirent
#define __Idirent

/* For MinGNU or other GNU tools (includes Sourcery CodeBench CSLibc and
   Newlib C library) */
#ifndef _DIRENT_H_
#define _DIRENT_H_

/* The following Directory entry structure is based on POSIX specs.  */
struct dirent
{
    ino_t   d_ino;          /* File serial number. */
    /* The character array d_name is of unspecified size, but the number
       of bytes preceding the terminating null byte shall not exceed
       {NAME_MAX}.  */
    char    d_name[NAME_MAX+1];
};

/* Directory structure */
typedef struct DIR_STRUCT
{
    struct dirent*  dir_ent;          /* Directory entries */
    struct dirent   static_dir_ent;   /* Non-reentrant version */
    unsigned short  id;               /* Open directory id */
    unsigned short  pad;              /* padding */
}DIR;

#ifdef __cplusplus
extern "C" {
#endif

/* Ends directory read operation.  */
int closedir(DIR *);

/* Opens a directory.  */
DIR *opendir(const char *);

/* Reads a directory.  */
struct dirent *readdir(DIR *);

/* Thread-safe version of readdir().  */
int readdir_r(DIR *, struct dirent *, struct dirent **);

/* Resets the readdir() pointer.  */
void rewinddir(DIR *);

/* Sets the position of the directory stream.  */
void seekdir(DIR *, long );

/* Current location of the named directory stream.  */
long telldir(DIR *);

#ifdef __cplusplus
}
#endif

#endif /* _DIRENT_H_ */
#endif /* __Idirent */

#endif /* NU_PSX_DIRENT_H */
