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
*       user_defs.h
*
* COMPONENT
*
*       User
*
* DESCRIPTION
*
*       Contains defines and structure definitions for user services.
*
* DATA STRUCTURES
*
*       FILE_SYSTEM_USER        User structure
*
* FUNCTIONS
*
*       None.
*
*************************************************************************/
#ifndef USER_DEFS_H
#define USER_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Number of users requested in metadata increased by one to account for the default user entry */
#define VFS_NUM_USERS_WDU (CFG_NU_OS_STOR_FILE_VFS_NUM_USERS + 1)
    /* This macro is an API call prolog. The User API is being deprecated. */

#if (FILE_VERSION_COMP == FILE_2_5)
    #define CHECK_USER()    ((NU_Check_File_User() == YES) ? NU_SUCCESS :  NUF_BAD_USER)
#elif (FILE_VERSION_COMP > FILE_2_5)
    #define CHECK_USER()    NU_SUCCESS
#endif

/* This is the type of a unique number that is associated with each task. It
   could be a process ID as in unix or a task control block address or
   index in a real time exec.  It is used to validate tasks as registered
   Nucleus FILE users. */
typedef int CONTEXT_HANDLE_TYPE;

/* User structure: Each task that accesses the file system may have one of
   these structures. The pointer fs_user points to the current user. In
   a real time exec you might put this in the process control block or
   have an array of them an change the fs_user pointer at task switch time

   Note: Having one of these structures per task is NOT absolutely necessary.
     If you do not these properties are simply system globals.

*/

typedef struct file_system_user
{
    INT         p_errno;            /* error number */
    INT16       dfltdrv;            /* Default drive if no drive specified */
    /* We place the task's handle here when we register it as a USER.
     * This allows us to validate users when API calls are made. */
    CONTEXT_HANDLE_TYPE context_handle;
} FILE_SYSTEM_USER;

typedef FILE_SYSTEM_USER *PFILE_SYSTEM_USER;

/* NUCLEUS - fs_user is a function call under nucleus. See pc_users.c */
#define fs_user ((PFILE_SYSTEM_USER)(fs_current_user_structure()))

#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* USER_DEFS_H */

