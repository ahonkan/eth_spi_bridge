/**************************************************************************
*                                                                         *
*                Copyright 2012 Mentor Graphics Corporation               *
*                          All Rights Reserved.                           *
*                                                                         *
*  THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS   *
*  THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS    *
*  SUBJECT TO LICENSE TERMS.                                              *
*                                                                         *
***************************************************************************

***************************************************************************
* FILE NAME                                    
*     mutex_nucleus.c
*
* COMPONENT
*     Nucleus SQLite.
*
* DESCRIPTION
*     This file contains definitions of recursive & binary mutexes.
*
* DATA STRUCTURES
*     None.
*
* FUNCTIONS
*     SQLITE_Init_Binary_Mutex
*     SQLITE_Create_Binary_Mutex
*     SQLITE_Binary_Mutex_Lock
*     SQLITE_Binary_Mutex_UnLock
*     SQLITE_Binary_Mutex_Delete
*     SQLITE_Create_Recursive_Mutex
*     SQLITE_Recursive_Mutex_Lock
*     SQLITE_Recursive_Mutex_UnLock
*     SQLITE_Recursive_Mutex_InLock
*
* --> The below mentioned set of functions are only visible internally.
* --> But these all functions are required by SQLite and it is using
* --> them via function pointers.
*     nuMutexHeld
*     nuMutexNotheld
*     nuMutexInit
*     nuMutexEnd
*     nuMutexAlloc
*     nuMutexFree
*     nuMutexEnter
*     nuMutexTry
*     nuMutexLeave
*
* DEPENDENCIES
*     sqliteInt.h
*     mutex_nucleus.h
*     nu_storage.h
*     assert.h
*     string.h
*
**************************************************************************/
#include "sqliteInt.h"

#ifndef SQLITE_MUTEX_OMIT
#ifdef SQLITE_MUTEX_NUCLEUS

#include "storage/nu_storage.h"
#include "mutex_nucleus.h"
#include <assert.h>
#include <string.h>

/* Use already declared memory pool */
static NU_MEMORY_POOL *sys_pool_ptr;

#if SQLITE_BIN_MUTEX_TEST
int dummy = 0;
#endif

/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Create_Binary_Mutex
*
*   DESCRIPTION
*
*       creates binary mutex
*
*   CALLED BY
*
*       Create_File_Mutex
*       SQLITE_Create_Recursive_Mutex
*       nuMutexAlloc
*       sqlite3_os_init
*
*   CALLS
*
*       strncpy
*       memset
*       NU_Create_Semaphore
*
*   INPUTS
*
*       mutex_ptr
*        mutex name
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_SEMAPHORE
*       NU_INVALID_SUSPEND
*
*
***********************************************************************/
STATUS SQLITE_Create_Binary_Mutex(struct SQLITE_BINARY_MUTEX_STRUCT * mutex_ptr,
                                  const char *name)
{
    STATUS sem_status;
    strncpy(mutex_ptr->name, name, SQLITE_BIN_MUTEX_NAME_LAST_INDEX);
    mutex_ptr->name[SQLITE_BIN_MUTEX_NAME_LAST_INDEX] = '\0';
    memset(&mutex_ptr->semaphore, 0, sizeof(mutex_ptr->semaphore));
    sem_status = NU_Create_Semaphore(&(mutex_ptr->semaphore),
                                     mutex_ptr->name,
                                     SQLITE_BIN_MUTEX_COUNT,
                                     NU_PRIORITY_INHERIT);
    mutex_ptr->is_created = SQLITE_BIN_MUTEX_CREATED_TRUE;
    
#if  SQLITE_BIN_MUTEX_TEST
    /*for testing only*/
    mutex_ptr->bin_lock_count = 0;
    mutex_ptr->bin_unlock_count = 0;
#endif

    return (sem_status);
}

/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Binary_Mutex_Lock
*
*   DESCRIPTION
*
*       locks binary mutex
*
*   CALLED BY
*       nucleusCreateLock
*       nucleusDestroyLock
*       nucleusLockFile
*       nucleusUnlockFile
*       nuMutexEnter
*       SQLITE_Init_SQL_File
*       SQLITE_Get_File_Mutex
*       SQLITE_Close_File_Mutex
*       SQLITE_Close_File_Mutex_Indexed
*       SQLITE_Get_File_Shared_Memory
*       SQLITE_Close_File_Shared_Memory
*       SQLITE_Close_File_Shared_Memory_Indexed
*
*   CALLS
*
*       NU_Obtain_Semaphore
*
*   INPUTS
*
*       mutex_ptr
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_SEMAPHORE
*       NU_INVALID_SUSPEND
*       NU_SEMAPHORE_DELETED
*       NU_SEMAPHORE_RESET
*
*
***********************************************************************/
STATUS SQLITE_Binary_Mutex_Lock(struct SQLITE_BINARY_MUTEX_STRUCT * mutex_ptr)
{
    STATUS sem_status;
    NU_SEMAPHORE *nu_semaphore;

#if  SQLITE_BIN_MUTEX_TEST
    printf("\r\n Binary Mutex (%x) Lock Called = %d",mutex_ptr,mutex_ptr->bin_lock_count);
    mutex_ptr->bin_lock_count++;
    if(mutex_ptr->bin_lock_count == 20)
    {
        dummy++;
    }
#endif

    nu_semaphore = (NU_SEMAPHORE *) &(mutex_ptr->semaphore);
    /* If the mutex is already locked, the calling thread shall
    block until the mutex becomes available. That's why we have
    used NU_SUSPEND below.  */
    sem_status = NU_Obtain_Semaphore(nu_semaphore,NU_SUSPEND);
    return (sem_status);
}

/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Binary_Mutex_UnLock
*
*   DESCRIPTION
*
*       unlock a binary mutex
*
*   CALLED BY
*
*       nucleusCreateLock
*       nucleusDestroyLock
*       nucleusLockFile
*       nucleusUnlockFile
*       nuMutexLeave
*       SQLITE_Init_SQL_File
*       SQLITE_Get_File_Mutex
*       SQLITE_Close_File_Mutex
*       SQLITE_Close_File_Mutex_Indexed
*       SQLITE_Get_File_Shared_Memory
*       SQLITE_Close_File_Shared_Memory
*       SQLITE_Close_File_Shared_Memory_Indexed
*       SQLITE_Recursive_Mutex_Lock
*       SQLITE_Recursive_Mutex_UnLock
*       SQLITE_Recursive_Mutex_InLock
*
*   CALLS
*
*       NU_Release_Semaphore
*
*   INPUTS
*
*       mutex_ptr
*
*   OUTPUTS
*
*       NU_SUCCESS
*        NU_INVALID_SEMAPHORE
*        NU_SEMAPHORE_COUNT_ROLLOVER
*
***********************************************************************/
STATUS SQLITE_Binary_Mutex_UnLock(struct SQLITE_BINARY_MUTEX_STRUCT * mutex_ptr)
{
    STATUS sem_status;
    NU_SEMAPHORE *nu_semaphore;

#if  SQLITE_BIN_MUTEX_TEST
    printf("\r\n Binary Mutex (%x) UnLock Called = %d",mutex_ptr,mutex_ptr->bin_unlock_count);
    mutex_ptr->bin_unlock_count++;
#endif

    nu_semaphore = (NU_SEMAPHORE *) &(mutex_ptr->semaphore);
    sem_status = NU_Release_Semaphore(nu_semaphore);
    return (sem_status);
}

/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Binary_Mutex_Delete
*
*   DESCRIPTION
*
*       Deletes binary mutex
*
*   CALLED BY
*
*       sqlite3_os_end
*       nuMutexEnd
*       nuMutexFree
*       SQLITE_Close_File_Mutex
*       SQLITE_Close_File_Mutex_Indexed
*
*   CALLS
*
*       NU_Delete_Semaphore
*
*   INPUTS
*
*        mutex_ptr
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_SEMAPHORE
*
*
***********************************************************************/
STATUS SQLITE_Binary_Mutex_Delete(struct SQLITE_BINARY_MUTEX_STRUCT * mutex_ptr)
{
    STATUS sem_status;
    NU_SEMAPHORE *nu_semaphore;
    nu_semaphore = (NU_SEMAPHORE *) &(mutex_ptr->semaphore);
    if(mutex_ptr->is_created == SQLITE_BIN_MUTEX_CREATED_TRUE)
    {
        sem_status = NU_Delete_Semaphore(nu_semaphore);
    }
    return (sem_status);
}

/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Create_Recursive_Mutex
*
*   DESCRIPTION
*
*       creates recursive mutex
*
*   CALLED BY
*
*       nuMutexAlloc
*
*   CALLS
*
*       strncpy
*       strcpy
*       strcat
*       SQLITE_Create_Binary_Mutex
*
*   INPUTS
*
*       r_mutex_ptr
*       name
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_SEMAPHORE
*       NU_INVALID_SUSPEND
*
*
***********************************************************************/
STATUS SQLITE_Create_Recursive_Mutex(struct SQLITE_RECURSIVE_MUTEX_STRUCT * r_mutex_ptr,const char *name)
{
    STATUS status;
    CHAR name_buffer[SQLITE_BIN_MUTEX_NAME_MAX_SIZE];

    strncpy(r_mutex_ptr->name,name,6);
    r_mutex_ptr->name[6] = '\0';

    strcpy(name_buffer,r_mutex_ptr->name);
    strcat(name_buffer, "M");
    status = SQLITE_Create_Binary_Mutex(&(r_mutex_ptr->binary_mutex_main), name_buffer);

    if(status == NU_SUCCESS)
    {
        strcpy(name_buffer,r_mutex_ptr->name);
        strcat(name_buffer, "A");
        status = SQLITE_Create_Binary_Mutex(&(r_mutex_ptr->binary_mutex_aux), name_buffer);

        if(status == NU_SUCCESS)
        {
            r_mutex_ptr->inMutex = 0L;
            r_mutex_ptr->owner_task_p = NU_NULL;
            r_mutex_ptr->mutexOwnerValid = 0L;
        }
        else
        {
            /* If failed to create auxiliary mutex, delete the main aswell. */
            SQLITE_Binary_Mutex_Delete(&(r_mutex_ptr->binary_mutex_aux));
        }

    }

    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Recursive_Mutex_Lock
*
*   DESCRIPTION
*
*       locks recursive mutex
*
*   CALLED BY
*
*       nuMutexEnter
*
*   CALLS
*
*       SQLITE_Binary_Mutex_Lock
*       NU_Current_Task_Pointer
*       SQLITE_Binary_Mutex_UnLock
*       assert
*
*   INPUTS
*
*       r_mutex_ptr
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void SQLITE_Recursive_Mutex_Lock(struct SQLITE_RECURSIVE_MUTEX_STRUCT * r_mutex_ptr)
{

  /* pthread_mutex_lock(&mutexAux); */
  SQLITE_Binary_Mutex_Lock(&(r_mutex_ptr->binary_mutex_aux));

  /* if( !mutexOwnerValid || !pthread_equal(mutexOwner, pthread_self()) ){ */
  if( (!(r_mutex_ptr->mutexOwnerValid)) ||
      !(r_mutex_ptr->owner_task_p == NU_Current_Task_Pointer()) )
  {
      /* pthread_mutex_unlock(&mutexAux); */
      SQLITE_Binary_Mutex_UnLock(&(r_mutex_ptr->binary_mutex_aux));

      /* pthread_mutex_lock(&mutexMain); */
      SQLITE_Binary_Mutex_Lock(&(r_mutex_ptr->binary_mutex_main));

      /* assert( inMutex==0 ); */
      assert( (r_mutex_ptr->inMutex)==0 );

      /* assert( !mutexOwnerValid ); */
      assert( !(r_mutex_ptr->mutexOwnerValid) );
    
      /* pthread_mutex_lock(&mutexAux); */
      SQLITE_Binary_Mutex_Lock(&(r_mutex_ptr->binary_mutex_aux));

      /* mutexOwner = pthread_self(); */
      r_mutex_ptr->owner_task_p = NU_Current_Task_Pointer();

      /* mutexOwnerValid = 1; */
      r_mutex_ptr->mutexOwnerValid = SQLITE_BIN_MUTEX_OWNER_VALID_TRUE;
  }
  /* inMutex++; */
  r_mutex_ptr->inMutex = r_mutex_ptr->inMutex + 1;

  /* pthread_mutex_unlock(&mutexAux); */
  SQLITE_Binary_Mutex_UnLock(&(r_mutex_ptr->binary_mutex_aux));
}

/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Recursive_Mutex_UnLock
*
*   DESCRIPTION
*
*       unlocks a recursive mutex
*
*   CALLED BY
*
*       nuMutexLeave
*
*   CALLS
*
*       assert
*       SQLITE_Binary_Mutex_Lock
*       SQLITE_Binary_Mutex_UnLock
*
*   INPUTS
*
*       r_mutex_ptr
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void SQLITE_Recursive_Mutex_UnLock(struct SQLITE_RECURSIVE_MUTEX_STRUCT * r_mutex_ptr){
    
    /*assert( inMutex>0 ); */
    assert( (r_mutex_ptr->inMutex)>0 );
    
    /*pthread_mutex_lock(&mutexAux); */
    SQLITE_Binary_Mutex_Lock(&(r_mutex_ptr->binary_mutex_aux));
    
    /* inMutex--; */
    r_mutex_ptr->inMutex = r_mutex_ptr->inMutex - 1;
    
    /* assert( pthread_equal(mutexOwner, pthread_self()) ); */
    assert( (r_mutex_ptr->owner_task_p == NU_Current_Task_Pointer()) );
    
    /* if( inMutex==0 ){ */
    if( (r_mutex_ptr->inMutex)==0 )
    {
        /* assert( mutexOwnerValid ); */
        assert( (r_mutex_ptr->mutexOwnerValid) );
        
        /* mutexOwnerValid = 0; */
        r_mutex_ptr->mutexOwnerValid = SQLITE_BIN_MUTEX_OWNER_VALID_FALSE;
        
        /* pthread_mutex_unlock(&mutexMain); */
        SQLITE_Binary_Mutex_UnLock(&(r_mutex_ptr->binary_mutex_main));
    }
    
    /* pthread_mutex_unlock(&mutexAux); */
    SQLITE_Binary_Mutex_UnLock(&(r_mutex_ptr->binary_mutex_aux));
}

/***********************************************************************
*
*   FUNCTION
*
*       SQLITE_Recursive_Mutex_InLock
*
*   DESCRIPTION
*
*       checks whether current thread has the lock
*
*   CALLED BY
*
*       nuMutexHeld
*
*   CALLS
*
*       SQLITE_Binary_Mutex_Lock
*       NU_Current_Task_Pointer
*       SQLITE_Binary_Mutex_UnLock
*
*   INPUTS
*
*       r_mutex_ptr
*       thisThrd
*
*   OUTPUTS
*
*       1        if the current thread has lock
*       0        otherwise
*
***********************************************************************/
INT32 SQLITE_Recursive_Mutex_InLock(struct SQLITE_RECURSIVE_MUTEX_STRUCT * r_mutex_ptr, INT32 thisThrd)
{
    INT32 rc;

    /* pthread_mutex_lock(&mutexAux); */
    SQLITE_Binary_Mutex_Lock(&(r_mutex_ptr->binary_mutex_aux));

    rc = ((r_mutex_ptr->inMutex)>0) && (thisThrd==0 || 
        ( (r_mutex_ptr->owner_task_p)== NU_Current_Task_Pointer() ) ) ;

    SQLITE_Binary_Mutex_UnLock(&(r_mutex_ptr->binary_mutex_aux));
    return (rc);
}

/*
** The mutex object
*/
typedef struct sqlite3_mutex {
  void *mutex_ptr;     /* Pointer to mutex. */
  int   id;    /* The mutex type */
} sqlite3_mutex;

/* The STATIC Mutexes. */
static sqlite3_mutex aStatic[] = { {NU_NULL, SQLITE_MUTEX_STATIC_MASTER},
                                   {NU_NULL, SQLITE_MUTEX_STATIC_MEM},
                                   {NU_NULL, SQLITE_MUTEX_STATIC_MEM2},
                                   {NU_NULL, SQLITE_MUTEX_STATIC_OPEN},
                                   {NU_NULL, SQLITE_MUTEX_STATIC_PRNG},
                                   {NU_NULL, SQLITE_MUTEX_STATIC_LRU},
                                   {NU_NULL, SQLITE_MUTEX_STATIC_PMEM} };

#if SQLITE_DEBUG && !defined(NDEBUG)
/*
** The sqlite3_mutex_held() and sqlite3_mutex_notheld() routine are
** intended for use inside assert() statements.
*/
static int nuMutexHeld(sqlite3_mutex *pX){

    NU_TASK *sem_owner;
    int status = NU_TRUE;
    if (pX == NU_NULL) return status;

    switch(pX->id) {
    case SQLITE_MUTEX_RECURSIVE: {
        status = SQLITE_Recursive_Mutex_InLock((sqlite_rec_mutex*)pX->mutex_ptr, NU_TRUE);
      break;
        }
    default: {
        sqlite_bin_mutex *tmp = (sqlite_bin_mutex*)pX->mutex_ptr;

        status = NU_Get_Semaphore_Owner(&(tmp->semaphore), &sem_owner);

        if(status == NU_INVALID_SEMAPHORE || (status == NU_SUCCESS &&
                                NU_Current_Task_Pointer() == sem_owner))
            status = NU_TRUE;
        else
            status = NU_FALSE;
      break;
        }
    }

  return status;
}

static int nuMutexNotheld(sqlite3_mutex *pX){

    return !nuMutexHeld(pX);
}
#endif

/*
** Initialize and deinitialize the mutex subsystem.
*/
static int nuMutexInit(void){
    int status;

    status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);
    return status == NU_SUCCESS ? SQLITE_OK : status;

}
static int nuMutexEnd(void){

    unsigned int i = 0;

    /*Remove all the static mutexes.*/
    for(i=0; i < (sizeof(aStatic)/sizeof(aStatic[0])); i++)
    {
        if(aStatic[i].mutex_ptr != NU_NULL)
        {
            SQLITE_Binary_Mutex_Delete(aStatic[i].mutex_ptr);
            aStatic[i].mutex_ptr = NU_NULL;
        }
    }
    return SQLITE_OK;
}

/*
** The sqlite3_mutex_alloc() routine allocates a new
** mutex and returns a pointer to it.  If it returns NULL
** that means that a mutex could not be allocated.
*/
static sqlite3_mutex *nuMutexAlloc(int id){
    sqlite3_mutex *pNew;
    STATUS status = NU_SUCCESS;

    switch( id ){
        case SQLITE_MUTEX_FAST: {
            if(NU_Allocate_Aligned_Memory(sys_pool_ptr, (VOID **)&pNew,
                    sizeof(*pNew)+sizeof(sqlite_bin_mutex), 4, NU_NO_SUSPEND) == NU_SUCCESS)
            {
                pNew->id = id;
                pNew->mutex_ptr = (char *)pNew + sizeof(*pNew);
                status = SQLITE_Create_Binary_Mutex(pNew->mutex_ptr, "bMutex");
            }
            else
                pNew = NU_NULL;

            break;
        }
        case SQLITE_MUTEX_RECURSIVE: {
            if(NU_Allocate_Aligned_Memory(sys_pool_ptr, (VOID **)&pNew,
                    sizeof(*pNew)+sizeof(sqlite_rec_mutex), 4, NU_NO_SUSPEND) == NU_SUCCESS)
            {
                pNew->id = id;
                pNew->mutex_ptr = (char*)pNew + sizeof(*pNew);
                status = SQLITE_Create_Recursive_Mutex(pNew->mutex_ptr, "rMutex");
            }
            else
                pNew = NU_NULL;
            break;
        }

        default: {/*static mutexes*/

            assert( id-2 >= 0 );
            assert( id-2 < (int)(sizeof(aStatic)/sizeof(aStatic[0])) );
            pNew = &aStatic[id-2];

            /* The static mutex will be allocated once only, on first call. */
            if(pNew->mutex_ptr == NU_NULL) {
                pNew->id = id;
                if(NU_Allocate_Aligned_Memory(sys_pool_ptr, (VOID **)&(pNew->mutex_ptr),
                        sizeof(sqlite_bin_mutex), 4, NU_NO_SUSPEND) == NU_SUCCESS)
                    status = SQLITE_Create_Binary_Mutex(pNew->mutex_ptr, "bMutex");
                else
                    pNew = NU_NULL;
            }
            break;
        }
    }

    if(pNew && status != NU_SUCCESS){
        if(id == SQLITE_MUTEX_FAST || id == SQLITE_MUTEX_RECURSIVE)
            NU_Deallocate_Memory(pNew);
        pNew = NU_NULL;
    }

    return (sqlite3_mutex*)pNew;
}

/*
** This routine deallocates a previously allocated mutex.
*/
static void nuMutexFree(sqlite3_mutex *pX){

    assert(nuMutexNotheld(pX));

    if(pX->id == SQLITE_MUTEX_FAST)
    {
        SQLITE_Binary_Mutex_Delete((sqlite_bin_mutex*)pX->mutex_ptr);
        NU_Deallocate_Memory(pX);
    }
    else if(pX->id == SQLITE_MUTEX_RECURSIVE)
    {
        sqlite_rec_mutex* tmp = pX->mutex_ptr;
        pX->mutex_ptr = NU_NULL;
        SQLITE_Binary_Mutex_Delete(&tmp->binary_mutex_aux);
        SQLITE_Binary_Mutex_Delete(&tmp->binary_mutex_main);
        NU_Deallocate_Memory(pX);
    }
}

/*
** The sqlite3_mutex_enter() and sqlite3_mutex_try() routines attempt
** to enter a mutex.  If another thread is already within the mutex,
** sqlite3_mutex_enter() will block and sqlite3_mutex_try() will return
** SQLITE_BUSY.  The sqlite3_mutex_try() interface returns SQLITE_OK
** upon successful entry.  Mutexes created using SQLITE_MUTEX_RECURSIVE can
** be entered multiple times by the same thread.  In such cases the,
** mutex must be exited an equal number of times before another thread
** can enter.  If the same thread tries to enter any other kind of mutex
** more than once, the behavior is undefined.
*/
static void nuMutexEnter(sqlite3_mutex *pX){

      if( pX->id == SQLITE_MUTEX_RECURSIVE)
      {
            SQLITE_Recursive_Mutex_Lock(pX->mutex_ptr);
      }
      else
      {
            /* Make sure the mutex is not held before acquiring. */
            assert(nuMutexNotheld(pX));
            SQLITE_Binary_Mutex_Lock(pX->mutex_ptr);
      }
}

static int nuMutexTry(sqlite3_mutex *pX){

    /* Some systems (for example, Windows 95) do not support the operation
     * implemented by sqlite3_mutex_try(). On those systems, sqlite3_mutex_try()
     * will always return SQLITE_BUSY. The SQLite core only ever uses
     * sqlite3_mutex_try() as an optimization so this is acceptable behavior.*/
    return SQLITE_BUSY;
}

/*
** The sqlite3_mutex_leave() routine exits a mutex that was
** previously entered by the same thread.  The behavior
** is undefined if the mutex is not currently entered or
** is not currently allocated.  SQLite will never do either.
*/
static void nuMutexLeave(sqlite3_mutex *pX){

        if( pX->id == SQLITE_MUTEX_RECURSIVE)
        {
            SQLITE_Recursive_Mutex_UnLock(pX->mutex_ptr);
        }
        else
        {
            SQLITE_Binary_Mutex_UnLock(pX->mutex_ptr);
        }
}

sqlite3_mutex_methods const *sqlite3DefaultMutex(void){
    static const sqlite3_mutex_methods sMutex = {
        nuMutexInit,
        nuMutexEnd,
        nuMutexAlloc,
        nuMutexFree,
        nuMutexEnter,
        nuMutexTry,
        nuMutexLeave,
#if SQLITE_DEBUG && !defined(NDEBUG)
        nuMutexHeld,
        nuMutexNotheld
#else
        0,
        0
#endif
    };

    return &sMutex;
}

#endif /* defined(SQLITE_MUTEX_NUCLEUS) */
#endif /* !defined(SQLITE_MUTEX_OMIT) */

/************** End of mutex_nucleus.c ******************************************/
