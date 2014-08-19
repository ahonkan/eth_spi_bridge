/**************************************************************************
*                                                                         *
*                Copyright 2008 Mentor Graphics Corporation               *
*                          All Rights Reserved.                           *
*                                                                         *
*  THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS   *
*  THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS    *
*  SUBJECT TO LICENSE TERMS.                                              *
*                                                                         *
***************************************************************************

***************************************************************************
* FILE NAME                                    
*     mutex_nucleus.h
*
* COMPONENT
*     Nucleus SQLite.
*
* DESCRIPTION
*     This file contains declarations of recursive binary mutexes.
*
* DATA STRUCTURES
*     SQLITE_BINARY_MUTEX_STRUCT
*	  SQLITE_RECURSIVE_MUTEX_STRUCT
*
* FUNCTIONS
*     SQLITE_Create_Binary_Mutex
*     SQLITE_Binary_Mutex_Lock
*     SQLITE_Binary_Mutex_UnLock
*     SQLITE_Binary_Mutex_Delete
*     SQLITE_Create_Recursive_Mutex
*     SQLITE_Recursive_Mutex_Lock
*     SQLITE_Recursive_Mutex_UnLock
*     SQLITE_Recursive_Mutex_InLock
*
* DEPENDENCIES
*     nu_storage.h
*
**************************************************************************/

#ifndef SQLITE_MUTEX_H_
#define SQLITE_MUTEX_H_

#include "storage/nu_storage.h"

#define SQLITE_BIN_MUTEX_TEST 0
#define SQLITE_BIN_MUTEX_NAME_MAX_SIZE 8
#define SQLITE_BIN_MUTEX_NAME_LAST_INDEX (SQLITE_BIN_MUTEX_NAME_MAX_SIZE - 1)
#define SQLITE_BIN_MUTEX_COUNT 1
#define SQLITE_BIN_MUTEX_CREATED_TRUE 1L
#define SQLITE_BIN_MUTEX_CREATED_FALSE 0L
#define SQLITE_BIN_MUTEX_OWNER_VALID_TRUE 1L
#define SQLITE_BIN_MUTEX_OWNER_VALID_FALSE 0L

#ifdef __cplusplus

/* C declarations in C++     */
extern "C" {

#endif


typedef struct SQLITE_BINARY_MUTEX_STRUCT
{
    NU_SEMAPHORE semaphore;
    char name[SQLITE_BIN_MUTEX_NAME_MAX_SIZE];
    INT32 is_created;

#if  SQLITE_BIN_MUTEX_TEST
    int bin_lock_count;
    int bin_unlock_count;
#endif

}sqlite_bin_mutex;

typedef struct SQLITE_RECURSIVE_MUTEX_STRUCT
{
    struct SQLITE_BINARY_MUTEX_STRUCT binary_mutex_main;
    struct SQLITE_BINARY_MUTEX_STRUCT binary_mutex_aux;
    char name[SQLITE_BIN_MUTEX_NAME_MAX_SIZE];
    INT32 mutexOwnerValid;       /* True if mutexOwner is valid */
    INT32 inMutex;
    void* owner_task_p;           /* Mutex owner Task Pointer.  */
}sqlite_rec_mutex;


STATUS SQLITE_Create_Binary_Mutex(struct SQLITE_BINARY_MUTEX_STRUCT * mutex_ptr,const char *name);

STATUS SQLITE_Binary_Mutex_Lock(struct SQLITE_BINARY_MUTEX_STRUCT * mutex_ptr);

STATUS SQLITE_Binary_Mutex_UnLock(struct SQLITE_BINARY_MUTEX_STRUCT * mutex_ptr);

STATUS SQLITE_Binary_Mutex_Delete(struct SQLITE_BINARY_MUTEX_STRUCT * mutex_ptr);


STATUS SQLITE_Create_Recursive_Mutex(struct SQLITE_RECURSIVE_MUTEX_STRUCT * r_mutex_ptr,const char *name);

void SQLITE_Recursive_Mutex_Lock(struct SQLITE_RECURSIVE_MUTEX_STRUCT * r_mutex_ptr);

void SQLITE_Recursive_Mutex_UnLock(struct SQLITE_RECURSIVE_MUTEX_STRUCT * r_mutex_ptr);

INT32 SQLITE_Recursive_Mutex_InLock(struct SQLITE_RECURSIVE_MUTEX_STRUCT * r_mutex_ptr, INT32 thisThrd);

#ifdef __cplusplus

/* End of C declarations */
}

#endif  /* __cplusplus */

#endif  /* SQLITE_MUTEX_H_ */
