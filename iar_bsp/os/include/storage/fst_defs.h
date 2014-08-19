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
*       fst_defs.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       FST
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains defines and structure definitions for file system 
*       table services.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       FSTE_S                      File system table entry
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#ifndef FST_DEFS_H
#define FST_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */


/* File system configuration defines */
#define FSC_MAX_TABLE_SIZE      CFG_NU_OS_STOR_FILE_VFS_MAX_FILE_SYSTEMS
#define FSC_MAX_FS_NAME         FILE_MAX_FILE_SYSTEM_NAME

/* File system table entry flag definition */
#define FSTE_FL_VALID   0x1

/* Structure definition of a file system table entry. */
typedef struct fste_struct
{
    CHAR fste_name[FSC_MAX_FS_NAME];    /* Name describing the type of file system */
    UINT32 fste_flags;                    
    UINT32 fste_cnt;                    /* Usage count */
    UINT32 fste_last;                   /* Last element from listing, only used in 
                                           get first/next/done operations */
    struct fs_struct fste_fs;           /* File system operations table */

} FSTE_S;


#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* FST_DEFS_H */
