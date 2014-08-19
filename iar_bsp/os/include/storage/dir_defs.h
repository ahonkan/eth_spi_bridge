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
*       dir_defs.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Directory
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains defines and structure definitions for directory services.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       DSTAT                       Generic directory structure used for 
*                                   directory and file searching routines.
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.    
*                                                                       
*************************************************************************/
#include "storage/file_cfg.h"

#ifndef DIR_DEFS_H
#define DIR_DEFS_H


#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Defines the maximum path length allowed */
#define EMAXPATH                255
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 0)
    /* Maximum length */
#define MAX_LFN            EMAXPATH
#define MAX_SFN            8
#define MAX_EXT            3

#else

#define MAX_LFN            EMAXPATH*4
#define MAX_SFN            8*4
#define MAX_EXT            3*4

#endif /* CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1 */

typedef struct dstat_struct
{

    CHAR       sfname[MAX_SFN + 1];    /* Null terminated file and extension */
    CHAR       fext[MAX_EXT + 1];
    CHAR       lfname[MAX_LFN + 1];    /* Null terminated long file name */
    UINT8       fattribute;                 /* File attributes */
    UINT8       fcrcmsec;                   /* File create centesimal mili second */
    UINT16      fcrtime;                    /* File create time */
    UINT16      fcrdate;                    /* File create date */
    UINT16      faccdate;                   /* Access date */
    UINT16      fclusterhigh;               /* High cluster for data file */
    UINT16      fuptime;                    /* File update time */
    UINT16      fupdate;                    /* File update */
    UINT16      fclusterlow;                /* Low cluster for data file */
    UINT32      fsize;                      /* File size */
    
    struct mte_struct  *fs_mte;             /* MTE for subsequent calls */
    VOID               *fs_private;         /* For file system specific data */
    UINT32              drive_id;           /* Unique ID for determining validity */
    UINT16              dh;                 /* Disk handle for this object */
} DSTAT;




#ifdef          __cplusplus
}

#endif /* _cplusplus */

#endif /* DIR_DEFS_H */
