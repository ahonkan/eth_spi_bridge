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
*       lck_defs.h
*
* COMPONENT
*
*       Locking
*
* DESCRIPTION
*
*       Contains defines and structure definitions for locking services
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
#ifndef LCK_DEFS_H
#define LCK_DEFS_H

#include "nucleus.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Define Supervisor and User mode functions */
#ifndef PLUS_2_0
    #if (!defined(NU_SUPERV_USER_MODE)) || (NU_SUPERV_USER_MODE < 1)
        #if (defined(NU_IS_SUPERVISOR_MODE))
            #undef NU_IS_SUPERVISOR_MODE
        #endif
        #define NU_IS_SUPERVISOR_MODE() (NU_TRUE)
        #define NU_SUPERVISOR_MODE() ((void) 0)
        #define NU_USER_MODE() ((void) 0)
        #define NU_SUPERV_USER_VARIABLES
    #endif /* NU_SUPERV_USER_MODE */
#endif


/* LCK_FS_ENTER / LCK_FS_EXIT are the prologue and epilogue to FILE. If
   there is any procedures that need to take place at FILE entry and
   exit, these macros should be edited */
#define LCK_FS_ENTER()                  NU_SUPERV_USER_VARIABLES  \
                                        NU_SUPERVISOR_MODE();

#define LCK_FS_EXIT()                   NU_USER_MODE();

/* Lock mechanism uses an array of semaphores. Each structure that requires
   a lock will receive an entry in the array. Access to the lock is done
   using the define for the lock as a parameter */
#define LCK_MAX_LOCKS   7   /* This is +1 the highest lock number. Used
                               to create the array of locks */
#define LCK_BIGLOCK     0   /* BIGLOCK is temporary */
#define LCK_FS_TABLE    1   /* Controls access to the file system table
                               containing FSTE_S entries */
#define LCK_MT_TABLE    2   /* Controls access to the mount table */
#define LCK_DEV_TABLE   3   /* Control access to the device table */
#define LCK_FD_TABLE    4   /* Controls access to the file descriptor
                               table */
#define LCK_DH_TABLE    5   /* Control access to the disk handle table */

#define LCK_USR_TABLE   6   /* Control access to the user's list */

/* Macros for VFS structure locks */
extern NU_SEMAPHORE     lck_locks[LCK_MAX_LOCKS];
#define LCK_ENTER(x)    NU_Obtain_Semaphore(&lck_locks[x], NU_SUSPEND);
#define LCK_EXIT(x)     NU_Release_Semaphore(&lck_locks[x]);

#ifdef          __cplusplus
}

#endif /* _cplusplus */

#endif /* LCK_DEFS_H */
