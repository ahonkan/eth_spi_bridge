/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/*************************************************************************
* FILE NAME
*
*       file_cfg.h
*
* COMPONENT
*
*       Configuration
*
* DESCRIPTION
*
*       Contains generic configuration settings
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       None.
*
*************************************************************************/
#ifndef FILE_CFG_H
#define FILE_CFG_H

#include "nucleus.h"
#include "storage/uni_defs.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */


/* These definitions control which version of the user API should be
   compatible with. This should allow new versions of API to be shipped but
   remain compatible with applications for previous versions. */
#define FILE_2_5         1       /* FILE 2.5 */
#define FILE_3_1         2       /* FILE 3.1 */

/* The version for which compatibility is desired. */
#ifndef FILE_VERSION_COMP
    #define FILE_VERSION_COMP    FILE_3_1
#endif

/* If Unicode support isn't wanted then set character encoding scheme to ASCII.*/
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 0)
    #define ENABLE_ASCII                NU_TRUE
    #define DEFAULT_CODEPAGE            CP_ASCII
   /* Set to indicate the max number of codepages that will be used, if not using 
      codepage support leave this set to 1.*/ 
    #define CP_MAX_TABLE_SIZE   1
#else
    /* Set to indicate the max number of codepages that will be used.*/ 
    #define CP_MAX_TABLE_SIZE   5
    #define DEFAULT_CODEPAGE            CP_JAPANESE_SHIFT_JIS
#endif

/* Set this to include device layer block caching support */
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE_REORDER == 1)
    #if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_BLK_CACHE == 0)
        #error "Block cache reordering requires block cache support be enabled."
    #endif
#endif

#ifndef FILE_MAX_DEVICE_NAME
    /* Max number of bytes for a device name */
    #define FILE_MAX_DEVICE_NAME        6
#endif

#ifndef FILE_MAX_FILE_SYSTEM_NAME
    /* Max number of bytes for a file system name */
    #define FILE_MAX_FILE_SYSTEM_NAME   8
#endif

/* These handle byte alignment and endianess */
#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
#define SWAP16(X,Y) NUF_Fswap16(X,Y)
#define SWAP32(X,Y) NUF_Fswap32(X,Y)
#else
#define SWAP16(X,Y) *((UINT8*)(X))=*((UINT8*)(Y)); \
                    *((UINT8*)(X)+1)=*((UINT8*)(Y)+1)

#define SWAP32(X,Y) *((UINT8*)(X))=*((UINT8*)(Y)); \
                    *((UINT8*)(X)+1)=*((UINT8*)(Y)+1); \
                    *((UINT8*)(X)+2)=*((UINT8*)(Y)+2); \
                    *((UINT8*)(X)+3)=*((UINT8*)(Y)+3)
#endif

/* NUF_Release_String contains a string describing
 * this release of Nucleus VFS.
 */
#define NUF_Release_String "Nucleus VFS"

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* FILE_CFG_H */
