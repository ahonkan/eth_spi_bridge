/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2012
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILENAME
*
*       os_nucleus.c
*
*   DESCRIPTION
*
*       This file provides the OS support that is required by SQLite.
*       This file contains code that is specific to nucleus OS only.
*
*   DATA STRUCTURES
*
*       nucleusFile
*
*   FUNCTIONS
*
*   Memory Subsystem:
*   --> The below mentioned set of functions is externally visible.
*       sqlite3MemGetNucleus
*       sqlite3MemSetDefault
*
*   --> The below mentioned set of functions are only visible internally.
*   --> But these all functions are required by SQLite and it is using
*   --> them via function pointers.
*       nuMemMalloc
*       nuMemFree
*       nuMemRealloc
*       nuMemSize
*       nuMemRoundup
*       nuMemInit
*       nuMemShutdown
*
*   VFS Subsystem:
*   --> The below mentioned set of functions is externally visible.
*       sqlite3_os_init
*       sqlite3_os_end
*
*   --> The below mentioned set of functions are only visible internally.
*   --> But these all functions are required by SQLite and it is using
*   --> them via function pointers.
*       nuOpen
*       nuDelete
*       nuAccess
*       nuFullPathname
*       nuRandomness
*       nuSleep
*       nuCurrentTime
*       nuGetLastError
*
*       nucleusClose
*       nucleusRead
*       nucleusWrite
*       nucleusTruncate
*       nucleusSync
*       nucleusFileSize
*       nucleusLock
*       nucleusUnlock
*       nucleusCheckReservedLock
*       nucleusFileControl
*       nucleusSectorSize
*       nucleusDeviceCharacteristics
*
*   DEPENDENCIES
*       sqliteInt.h
*       nucleus.h
*       nu_storage.h
*       mutex_nucleus.h
*       sqlfile.h
*       ctype.h
*
*************************************************************************/

/**
** This file contains code that is specific to Nucleus.
*/
#include  "sqliteInt.h"

#if SQLITE_OS_NUCLEUS           /* This file is used for nucleus only */

#include  "nucleus.h"
#include  "storage/nu_storage.h"
#include  "mutex_nucleus.h"
#include  "../sqlfile.h"
#include  <ctype.h>

#define OSTRACE(args...)
#define OSTRACE2 OSTRACE
#define OSTRACE3 OSTRACE
#define OSTRACE4 OSTRACE
#define OSTRACE5 OSTRACE

/* Define the current date for nuCurrentTime. */
#define SQLITE3_YEAR 2012
#define SQLITE3_MONTH 12
#define SQLITE3_DAY 3

#ifdef SQLITE_NUCLEUS_MALLOC

/*     The memory requested by SQLite is taken from sqlite_mem_pool pool.
 *     One of the SQLite memory APIs return the size of dynamic memory block.
 *     Nucleus does not have any API to support this. So we request 8 extra
 *     bytes, at the time of allocation or reallocation, and record size
 *     in it as INT64.
 */

/* memory pool pointer */
static NU_MEMORY_POOL*  sqlite_mem_pool;

/*
** Allocate nBytes of memory.
*/
void *nuMemMalloc(int nBytes){
    VOID    *return_ptr;
    INT     status;
    const INT ALLOC_ALIGNMENT = 8;

    if(nBytes == 0)
    {
        return(NULL);
    }

    /* Allocate the memory from System Memory */
    /* since SQLite code uses 64-bit int, on some processors,
       memory should always be aligned to double words == 8 bytes */
    status = NU_Allocate_Aligned_Memory(sqlite_mem_pool, &return_ptr, nBytes+sizeof(INT64), ALLOC_ALIGNMENT, NU_NO_SUSPEND);

    if(status == NU_SUCCESS)
    {
        /* Save the size of this allocation at start of memory block. */
        *((INT64 *)return_ptr) =  nBytes;

        /* Move the pointer one INT64 ahead. */
        return_ptr = (INT64 *)return_ptr + 1;

    }
    else
        return_ptr = NU_NULL;

    return return_ptr;
}

/*
** Free memory.
*/
void nuMemFree(void *pPrior){

    assert(pPrior);

    /* Move the pointer one INT64 back. */
    pPrior = ((INT64 *)pPrior) - 1;

    NU_Deallocate_Memory(pPrior);
}

/*
** Change the size of an existing memory allocation
*/
void *nuMemRealloc(void *pPrior, int nBytes){

    const INT ALLOC_ALIGNMENT = 8;
    INT       status = NU_SUCCESS;

    /*If size is 0 and pPrior is not a null pointer, the object pointed to is freed.*/
    if(nBytes == 0 && pPrior != NU_NULL)
    {
        nuMemFree(pPrior);
    }
    /*If pPrior is a null pointer, realloc() shall be equivalent to malloc() for the specified size.*/
    else if( pPrior == NU_NULL )
    {
        pPrior = nuMemMalloc(nBytes);
    }
    else
    {
        pPrior = ((INT64 *)pPrior) - 1;

        /* Allocate the memory from System Memory */
        /* since SQLite code uses 64-bit int, on some processors,
           memory should always be aligned to double words == 8 bytes */
        status = NU_Reallocate_Aligned_Memory(sqlite_mem_pool, &pPrior, nBytes+sizeof(INT64), ALLOC_ALIGNMENT, NU_NO_SUSPEND);
    }

    if(status == NU_SUCCESS)
    {
        *((INT64 *)pPrior) =  nBytes;
        pPrior = ((INT64 *)pPrior) + 1;
    }
    else
        pPrior = NU_NULL;

    return pPrior;
}

/*
** Return the size of an outstanding allocation, in bytes.
*/
int nuMemSize(void *p){

    return (p != NU_NULL) ? *((INT64 *)p-1) : 0;
}

/*
** Round up a request size to the next valid allocation size.
*/
static int nuMemRoundup(int n){
    return n;
}

/*
** Initialize this module.
*/
static int nuMemInit(void *pAppData){
    int status;

    status = NU_System_Memory_Get(&sqlite_mem_pool, NU_NULL);
    return status == NU_SUCCESS ? SQLITE_OK : status;
}

/*
** Deinitialize this module.
*/
static void nuMemShutdown(void *pAppData){

}

/*
** Populate the low-level memory allocation function pointers in
** sqlite3GlobalConfig.m with pointers to the routines in this file. The
** arguments specify the block of memory to manage.
**
** This routine is only called by sqlite3_config(), and therefore
** is not required to be threadsafe (it is not).
*/
const sqlite3_mem_methods *sqlite3MemGetNucleus(void){
    static const sqlite3_mem_methods nuMemMethods = {
    nuMemMalloc,
    nuMemFree,
    nuMemRealloc,
    nuMemSize,
    nuMemRoundup,
    nuMemInit,
    nuMemShutdown,
    NU_NULL
    };
    return &nuMemMethods;
}

void sqlite3MemSetDefault(void){
    sqlite3_config(SQLITE_CONFIG_MALLOC, sqlite3MemGetNucleus());
}
#endif /* SQLITE_NUCLEUS_MALLOC */

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#define  SQLite_DEFAULT_PATH  CFG_NU_OS_STOR_DB_SQLITE_SQL_DB_DIRECTORY

#ifndef TEMP_FILE_PREFIX
#define TEMP_FILE_PREFIX "etilqs_"
#endif

/*
** When testing, keep a count of the number of open files.
*/
#ifdef SQLITE_TEST
int sqlite3_open_file_count = 0;
#define OpenCounter(X)  sqlite3_open_file_count+=(X)
#else
#define OpenCounter(X)
#endif

VOID SQLiteZeroMemory(unsigned char* zMemory, INT32 mem_size);
VOID SQLiteStringConvertCharTo(CHAR* zString, const CHAR from_char, const CHAR to_char);
VOID SQLiteStringToLower(CHAR* zString);
static int nuSleep(sqlite3_vfs *pVfs, int microsec);
static int nuRandomness(sqlite3_vfs *pVfs, int nBuf, char *zBuf);

/*
** nucleus lacks native support for file locking so we have to fake it
** with some code of our own.
*/
typedef struct nucleusFileLock {
    int nReaders;          /* Number of reader locks obtained */
    BOOLEAN bPending;      /* Indicates a pending lock has been obtained */
    BOOLEAN bReserved;     /* Indicates a reserved lock has been obtained */
    BOOLEAN bExclusive;    /* Indicates an exclusive lock has been obtained */
    BOOLEAN bPad;
} nucleusFileLock;


/*
** The nucleusFile structure is subclass of OsFile specific for the nucleus
** protability layer.
*/
typedef struct nucleusFile nucleusFile;
struct nucleusFile {
    sqlite3_io_methods const *pMethod;/* Always the first entry */
    sqlite3_vfs *pVfs;                /* The VFS used to open this file */
    INT h;                            /* Handle for accessing the file */
    int delOnClose;                   /* True if file is to be deleted on close */
    unsigned char locktype;           /* Type of lock currently held on this file */
    short sharedLockByte;             /* Randomly chosen byte used as a shared lock */
    const char *zPath;                /* Full pathname of this file */
    char* zDeleteOnClose;             /* Name of file to delete on close */
    struct SQLITE_BINARY_MUTEX_STRUCT* hMutex;  /* Shared file mutex */
    INT32 hShared;                    /* Index of file name */
    nucleusFileLock local;            /* Locks obtained by this instance of nucleusFile */
    nucleusFileLock *shared;          /* Global shared lock memory for the file  */
    INT lastErrno;                    /* last error */
};


#define HANDLE_TO_NUCLEUSFILE(a) (nucleusFile*)&((char*)a)[-offsetof(nucleusFile,h)]

typedef sqlite3_file OsFile;
#define NU_IMPL_CODE 1

/*
** Create the mutex and shared memory used for locking in the file
** descriptor pFile
*/
static BOOLEAN nucleusCreateLock(const char *zFilename, nucleusFile *pFile){
    char *zName;
    INT32 is_created = 0;
    INT32 hShared = -1;

    BOOLEAN bInit = NU_TRUE;

    if(NU_Allocate_Memory(sqlite_mem_pool, (VOID **)&zName, (strlen(zFilename)+1),
                                                    NU_NO_SUSPEND) != NU_SUCCESS)
      zName = NU_NULL;

    strcpy(zName,zFilename);

    /* Initialize the local lockdata */
    SQLiteZeroMemory((void *)&pFile->local, sizeof(pFile->local));

    /* Replace the backslashes from the filename and lowercase it
    ** to derive a mutex name. */
    SQLiteStringToLower(zName);
    SQLiteStringConvertCharTo(zName,'\\','_');

    /* Create/open the named mutex */
    pFile->hMutex = SQLITE_Get_File_Mutex(zName,&is_created);
    /*CreateMutexW(NULL, FALSE, zName);*/

    if (!pFile->hMutex){
    NU_Deallocate_Memory(zName);
    return NU_FALSE;
    }

    /* Acquire the mutex before continuing */
    SQLITE_Binary_Mutex_Lock(pFile->hMutex);

    pFile->shared = SQLITE_Get_File_Shared_Memory(zName,sizeof(nucleusFileLock),&is_created,&hShared);


    /* Set a flag that indicates we're the first to create the memory so it
    ** must be zero-initialized */
    if (!is_created){
    bInit = NU_FALSE;
    }

    /* If we succeeded in making the file shared memory. */
    if (pFile->shared){
        pFile->hShared = hShared;
    }

    /* If shared memory could not be created, then close the mutex and fail */
    if (pFile->shared == NU_NULL){
        SQLITE_Binary_Mutex_UnLock(pFile->hMutex);
        SQLITE_Close_File_Mutex(zName);
        NU_Deallocate_Memory(zName);
        pFile->hMutex = NULL;
        pFile->hShared = -1;
        return NU_FALSE;
    }

    /* Initialize the shared memory if we're supposed to */
    if (bInit) {
        SQLiteZeroMemory((void *)pFile->shared, sizeof(nucleusFileLock));
    }

    NU_Deallocate_Memory(zName);
    SQLITE_Binary_Mutex_UnLock(pFile->hMutex);
    return NU_TRUE;
}

/*
** Destroy the part of nucleusFile that deals with nucleus locks
*/
static void nucleusDestroyLock(nucleusFile *pFile){
    INT32 is_last = 0;
    if (pFile->hMutex){
        /* Acquire the mutex */
        SQLITE_Binary_Mutex_Lock(pFile->hMutex);

        /* The following blocks should probably assert in debug mode, but they
           are to cleanup in case any locks remained open */
        if (pFile->local.nReaders){
            pFile->shared->nReaders --;
        }
        if (pFile->local.bReserved){
            pFile->shared->bReserved = NU_FALSE;
        }
        if (pFile->local.bPending){
            pFile->shared->bPending = NU_FALSE;
        }
        if (pFile->local.bExclusive){
            pFile->shared->bExclusive = NU_FALSE;
        }

        /* De-reference and close our copy of the shared memory handle */
        SQLITE_Close_File_Shared_Memory_Indexed(pFile->hShared,sizeof(nucleusFileLock),&is_last);

        /* Done with the mutex */
        SQLITE_Binary_Mutex_UnLock(pFile->hMutex);

        if(is_last)
        {
            SQLITE_Close_File_Mutex_Indexed(pFile->hShared);
        }
        pFile->hMutex = NULL;
    }
}


/*
** An implementation of the LockFile() API of windows for wince
*/
static BOOLEAN nucleusLockFile(
    INT *phFile,
    INT32 dwFileOffsetLow,
    INT32 dwFileOffsetHigh,
    INT32 nNumberOfBytesToLockLow,
    INT32 nNumberOfBytesToLockHigh
){
    nucleusFile *pFile = HANDLE_TO_NUCLEUSFILE(phFile);
    BOOLEAN bReturn = NU_FALSE;

    if (!pFile->hMutex) return NU_TRUE;
    SQLITE_Binary_Mutex_Lock(pFile->hMutex);

    /* Wanting an exclusive lock? */
    if (dwFileOffsetLow == SHARED_FIRST
         && nNumberOfBytesToLockLow == SHARED_SIZE){
        if (pFile->shared->nReaders == 0 && pFile->shared->bExclusive == 0){
            pFile->shared->bExclusive = NU_TRUE;
            pFile->local.bExclusive = NU_TRUE;
            bReturn = NU_TRUE;
        }
    }

    /* Want a read-only lock? */
    else if ((dwFileOffsetLow >= SHARED_FIRST &&
              dwFileOffsetLow < SHARED_FIRST + SHARED_SIZE) &&
              nNumberOfBytesToLockLow == 1){
        if (pFile->shared->bExclusive == 0){
            pFile->local.nReaders ++;
            if (pFile->local.nReaders == 1){
                pFile->shared->nReaders ++;
            }
            bReturn = NU_TRUE;
        }
    }

    /* Want a pending lock? */
    else if (dwFileOffsetLow == PENDING_BYTE && nNumberOfBytesToLockLow == 1){
        /* If no pending lock has been acquired, then acquire it */
        if (pFile->shared->bPending == 0) {
            pFile->shared->bPending = NU_TRUE;
            pFile->local.bPending = NU_TRUE;
            bReturn = NU_TRUE;
        }
    }
    /* Want a reserved lock? */
    else if (dwFileOffsetLow == RESERVED_BYTE && nNumberOfBytesToLockLow == 1){
        if (pFile->shared->bReserved == 0) {
            pFile->shared->bReserved = NU_TRUE;
            pFile->local.bReserved = NU_TRUE;
            bReturn = NU_TRUE;
        }
    }

    SQLITE_Binary_Mutex_UnLock(pFile->hMutex);
    return bReturn;
}

/*
** An implementation of the UnlockFile API of windows for wince
*/
static BOOLEAN nucleusUnlockFile(
    INT *phFile,
    INT32 dwFileOffsetLow,
    INT32 dwFileOffsetHigh,
    INT32 nNumberOfBytesToUnlockLow,
    INT32 nNumberOfBytesToUnlockHigh
){
    nucleusFile *pFile = HANDLE_TO_NUCLEUSFILE(phFile);
    BOOLEAN bReturn = NU_FALSE;

    if (!pFile->hMutex) return NU_TRUE;
    SQLITE_Binary_Mutex_Lock(pFile->hMutex);

    /* Releasing a reader lock or an exclusive lock */
    if (dwFileOffsetLow >= SHARED_FIRST &&
         dwFileOffsetLow < SHARED_FIRST + SHARED_SIZE){
        /* Did we have an exclusive lock? */
        if (pFile->local.bExclusive){
            pFile->local.bExclusive = NU_FALSE;
            pFile->shared->bExclusive = NU_FALSE;
            bReturn = NU_TRUE;
        }

        /* Did we just have a reader lock? */
        else if (pFile->local.nReaders){
            pFile->local.nReaders --;
            if (pFile->local.nReaders == 0)
            {
                pFile->shared->nReaders --;
            }
            bReturn = NU_TRUE;
        }
    }

    /* Releasing a pending lock */
    else if (dwFileOffsetLow == PENDING_BYTE && nNumberOfBytesToUnlockLow == 1){
        if (pFile->local.bPending){
            pFile->local.bPending = NU_FALSE;
            pFile->shared->bPending = NU_FALSE;
            bReturn = NU_TRUE;
        }
    }
    /* Releasing a reserved lock */
    else if (dwFileOffsetLow == RESERVED_BYTE && nNumberOfBytesToUnlockLow == 1){
        if (pFile->local.bReserved) {
             pFile->local.bReserved = NU_FALSE;
             pFile->shared->bReserved = NU_FALSE;
             bReturn = NU_TRUE;
        }
    }

    SQLITE_Binary_Mutex_UnLock(pFile->hMutex);
    return bReturn;
}

/*
** Close a file.
**
** It is possible in multitasking environment that an attempt to close
** a handle might sometimes fail.
** If the close fails, we pause for 100 milliseconds and try again.  As
** many as MX_CLOSE_ATTEMPT attempts to close the handle are made before
** giving up and returning an error.
*/
#define MX_CLOSE_ATTEMPT 3
static int nucleusClose(OsFile *pId)
{
    nucleusFile *pFile;
    int rc = 1;
    if( pId && (pFile = (nucleusFile*)pId)!=0 ){
        int rc, cnt = 0;
        OSTRACE2("CLOSE %d\n", pFile->h);
        do{
            rc = NU_Close(pFile->h);
        }while( rc!=NU_SUCCESS && cnt++ < MX_CLOSE_ATTEMPT && (nuSleep(NU_NULL, 100), 1) );
        nucleusDestroyLock(pFile);
        if( pFile->zDeleteOnClose ){
            NU_Delete(pFile->zDeleteOnClose);
            NU_Deallocate_Memory(pFile->zDeleteOnClose);
        }
        OpenCounter(-1);
    }
    return rc==NU_SUCCESS ? SQLITE_OK : SQLITE_IOERR;
}

/*
** Append a byte at the end of file for the desired amount of count.
** This routine will help the nucleus seek operation to seek behind
** the file size just like as it is possible in windows.
** If appended successfully returns SQLITE_OK otherwise SQLITE_FULL.
*/
static int writeAnExtraBytesAtEnd(INT h, UINT8 val, int amt)
{
    char *ptr;
    int result = SQLITE_FULL;

    NU_Seek(h, 0, PSEEK_END);

    if(NU_Allocate_Memory(sqlite_mem_pool, (VOID **)&ptr, amt,
                                     NU_NO_SUSPEND) != NU_SUCCESS)
        ptr = NU_NULL;

    if (ptr != NULL)
    {
        memset(ptr, (int) val, amt);

        if(NU_Write(h, ptr, amt) == amt)
           {
               result = SQLITE_OK;
           }
    }

    NU_Deallocate_Memory(ptr);

    return result;
}

/*
** Move the read/write pointer in a file.
*/
static int nucleusSeek(OsFile *id, i64 offset){
    STATUS status;
    INT32 extraBytes = 0;
    INT32 lowerBits=0;
    lowerBits = (INT32)(offset & 0xffffffff);
    status = NU_Seek(((nucleusFile*)id)->h, lowerBits, PSEEK_SET);
    if(status>0 && status<lowerBits){
        /* This means user want to seek more than the file size.
        ** In order to make it work with nucleus just append the
        ** extra bytes at the end and try to seek again!
        */
        extraBytes = (lowerBits - status) + 1;
        if(SQLITE_OK != writeAnExtraBytesAtEnd(((nucleusFile*)id)->h,0,extraBytes))
        {
            return SQLITE_FULL;
        }
        status = NU_Seek(((nucleusFile*)id)->h, lowerBits, PSEEK_SET);
    }
    if(status == lowerBits)
    {
        return SQLITE_OK;
    }
    else
    {
        ((nucleusFile*)id)->lastErrno = status;
        return SQLITE_FULL;
    }
}

/*
** Read data from a file into a buffer.  Return SQLITE_OK if all
** bytes were read successfully and SQLITE_IOERR if anything goes
** wrong.
*/
static int nucleusRead(OsFile *id, void *pBuf, int amt, sqlite3_int64 offset){
    STATUS status;

    if(nucleusSeek(id, offset) != NU_SUCCESS)
        return SQLITE_IOERR_READ;

    status = NU_Read(((nucleusFile*)id)->h, (char *)pBuf, amt);

    if((int)status < 0)
    {
        return SQLITE_IOERR;
    }

    else
    {
        if(amt == (int)status)
        {
            return SQLITE_OK;
        }
        else
        {
            memset(&((char*)pBuf)[(int)status], 0, amt-((int)status));
            return SQLITE_IOERR_SHORT_READ;
        }
    }
}

/*
** Write data from a buffer into a file.  Return SQLITE_OK on success
** or some other error code on failure.
*/
static int nucleusWrite(OsFile *id, const void *pBuf, int amt, sqlite3_int64 offset){
    STATUS status;
    assert( amt>0 );

    if(nucleusSeek(id, offset) != NU_SUCCESS)
        return SQLITE_IOERR_READ;

    while( amt>0 && (status = NU_Write(((nucleusFile*)id)->h, (char *)pBuf, amt))>0){
        amt -= status;
        pBuf = &((char*)pBuf)[status];
    }
    if(amt>0){
        return SQLITE_FULL;
    }else{
        return SQLITE_OK;
    }
}

/*
** Determine the current size of a file in bytes
*/
static int nucleusFileSize(OsFile *id, i64 *pSize){
    UINT32      data_length = 0;
    INT32       original_location;
    /* Save off the current location of the file pointer - seek 0 bytes
     * from the current position.
     */
    original_location = NU_Seek(((nucleusFile*)id)->h, 0, PSEEK_CUR);
    if (original_location >= 0){
        /* Get the end location of the file pointer - seek to the end of
        * the file.
        */
        data_length = (UINT32)NU_Seek(((nucleusFile*)id)->h, 0, PSEEK_END);

        /* Restore the original position of the file pointer - seek
        * original_location bytes from the beginning of the file.
        */
        NU_Seek(((nucleusFile*)id)->h, original_location, PSEEK_SET);
    }
    *pSize = data_length;
    return SQLITE_OK;
}

/*
** Truncate an open file to a specified size
*/
static int nucleusTruncate(OsFile *id, i64 nByte)
{
    INT32 lowerBits = 0;
    i64 psize;
    int result = SQLITE_IOERR;

    lowerBits = (INT32)(nByte & 0xffffffff);

    if(SQLITE_OK == nucleusFileSize(id, &psize))
    {
        if(nByte < psize)
        {
            if(NU_SUCCESS == NU_Truncate(((nucleusFile*)id)->h, lowerBits))
            {
                result = SQLITE_OK;
            }
        }
        else if(nByte > psize)
        {
            result = nucleusSeek(id, nByte);
        }
        else
        {
            /* file size is already equal to the requested size, do nothing */
            result = SQLITE_OK;
        }
    }

    return result;
}

/*
** Make sure all writes to a particular file are committed to disk.
*/
static int nucleusSync(OsFile *id, int dataOnly){
    if(NU_Flush(((nucleusFile*)id)->h) == NU_SUCCESS){
        return SQLITE_OK;
    }else{
        return SQLITE_IOERR;
    }
}

/*
** Acquire a reader lock.
*/
static int getReadLock(nucleusFile *id){
    int res;
    int lk;
    nuRandomness(NU_NULL, sizeof(lk), (char *)&lk);
    id->sharedLockByte = (lk & 0x7fffffff)%(SHARED_SIZE - 1);
    res = nucleusLockFile(&(id->h), SHARED_FIRST+id->sharedLockByte, 0, 1, 0);
    return res;
}

/*
** Undo a readlock
*/
static int unlockReadLock(nucleusFile *pFile){
    int res;
    res = nucleusUnlockFile(&(pFile->h), SHARED_FIRST + pFile->sharedLockByte, 0, 1, 0);
    return res;
}

/*
** Lock the file with the lock specified by parameter locktype - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  The nucleusUnlock() routine
** erases all locks at once and returns us immediately to locking level 0.
** It is not possible to lower the locking level one step at a time.  You
** must go straight to locking level 0.
*/
static int nucleusLock(OsFile *id, int locktype){
    int rc = SQLITE_OK;    /* Return code from subroutines */
    int res = 1;           /* Result of a windows lock call */
    int newLocktype;       /* Set id->locktype to this value before exiting */
    int gotPendingLock = 0;/* True if we acquired a PENDING lock this time */
    nucleusFile *pFile = (nucleusFile*)id;

#ifndef NU_IMPL_CODE
    return SQLITE_OK;
#endif

    assert( pFile!=0 );
    OSTRACE5("LOCK %s to %d was %d(%d) by %s \n",
            pFile->zPath, locktype, pFile->locktype, pFile->sharedLockByte, ((NU_TASK *)TCD_Current_Thread)->tc_name);

    /* If there is already a lock of this type or more restrictive on the
    ** OsFile, do nothing. Don't use the end_lock: exit path, as
    ** sqlite3OsEnterMutex() hasn't been called yet.
    */
    if( pFile->locktype>=locktype ){
        return SQLITE_OK;
    }

    /* Make sure the locking sequence is correct
    */
    assert( pFile->locktype!=NO_LOCK || locktype==SHARED_LOCK );
    assert( locktype!=PENDING_LOCK );
    assert( locktype!=RESERVED_LOCK || pFile->locktype==SHARED_LOCK );

    /* Lock the PENDING_LOCK byte if we need to acquire a PENDING lock or
    ** a SHARED lock.  If we are acquiring a SHARED lock, the acquisition of
    ** the PENDING_LOCK byte is temporary.
    */
    newLocktype = pFile->locktype;
    if( pFile->locktype==NO_LOCK
     || (locktype==EXCLUSIVE_LOCK && pFile->locktype==RESERVED_LOCK)
    ){
        int cnt = 3;
        while( cnt-->0 && (res = nucleusLockFile(&(pFile->h), PENDING_BYTE, 0, 1, 0))==0 ){
            /* Try 3 times to get the pending lock.  The pending lock might be
            ** held by another reader process who will release it momentarily.
            */

            OSTRACE2("could not get a PENDING lock. cnt=%d\n", cnt);
            nuSleep(NU_NULL, 10);/*Sleep(1);*/
        }

        gotPendingLock = res;
    }

    /* Acquire a shared lock
    */
    if( locktype==SHARED_LOCK && res ){
        assert( pFile->locktype==NO_LOCK );
        res = getReadLock(pFile);
        if( res ){
            newLocktype = SHARED_LOCK;
        }
    }

    /* Acquire a RESERVED lock
    */
    if( locktype==RESERVED_LOCK && res ){
        assert( pFile->locktype==SHARED_LOCK );
        res = nucleusLockFile(&(pFile->h), RESERVED_BYTE, 0, 1, 0);
        if( res ){
            newLocktype = RESERVED_LOCK;
        }
    }

    /* Acquire a PENDING lock
    */
    if( locktype==EXCLUSIVE_LOCK && res ){
        newLocktype = PENDING_LOCK;
        gotPendingLock = 0;
    }

    /* Acquire an EXCLUSIVE lock
    */
    if( locktype==EXCLUSIVE_LOCK && res ){
        assert( pFile->locktype>=SHARED_LOCK );
        res = unlockReadLock(pFile);
        OSTRACE2("unreadlock = %d\n", res);
        res = nucleusLockFile(&(pFile->h), SHARED_FIRST, 0, SHARED_SIZE, 0);
        if( res ){
            newLocktype = EXCLUSIVE_LOCK;
        }else{
            OSTRACE2("error-code = %d\n", res);/*GetLastError());*/
            getReadLock(pFile);
        }
    }

    /* If we are holding a PENDING lock that ought to be released, then
    ** release it now.
    */
    if( gotPendingLock && locktype==SHARED_LOCK ){
        nucleusUnlockFile(&(pFile->h), PENDING_BYTE, 0, 1, 0);
    }

    /* Update the state of the lock has held in the file descriptor then
    ** return the appropriate result code.
    */
    if( res ){
        rc = SQLITE_OK;
    }else{
        OSTRACE4("LOCK FAILED on %s trying for %d but got %d by \n", pFile->zPath,
             locktype, newLocktype, ((NU_TASK *)TCD_Current_Thread)->tc_name);
        rc = SQLITE_BUSY;
    }
    pFile->locktype = newLocktype;
    return rc;
}

/*
** Lower the locking level on file descriptor id to locktype.  locktype
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
**
** It is not possible for this routine to fail if the second argument
** is NO_LOCK.  If the second argument is SHARED_LOCK then this routine
** might return SQLITE_IOERR;
*/
static int nucleusUnlock(OsFile *id, int locktype){
    int type;
    int rc = SQLITE_OK;

#ifndef NU_IMPL_CODE
    return SQLITE_OK;
#endif

    nucleusFile *pFile = (nucleusFile*)id;
    assert( pFile!=0 );
    assert( locktype<=SHARED_LOCK );
    OSTRACE5("UNLOCK %s to %d was %d(%d) by %s \n",
        pFile->zPath, locktype, pFile->locktype, pFile->sharedLockByte, ((NU_TASK *)TCD_Current_Thread)->tc_name);

    type = pFile->locktype;
    if( type>=EXCLUSIVE_LOCK ){
        nucleusUnlockFile(&(pFile->h), SHARED_FIRST, 0, SHARED_SIZE, 0);
        if( locktype==SHARED_LOCK && !getReadLock(pFile) ){
            /* This should never happen.  We should always be able to
            ** reacquire the read lock */
            rc = SQLITE_IOERR_UNLOCK;
        }
    }
    if( type>=RESERVED_LOCK ){
        nucleusUnlockFile(&(pFile->h), RESERVED_BYTE, 0, 1, 0);
    }
    if( locktype==NO_LOCK && type>=SHARED_LOCK ){
        unlockReadLock(pFile);
    }
    if( type>=PENDING_LOCK ){
        nucleusUnlockFile(&(pFile->h), PENDING_BYTE, 0, 1, 0);
    }
    pFile->locktype = locktype;
    return rc;
}

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, return
** non-zero, otherwise zero.
*/
static int nucleusCheckReservedLock(OsFile *id, int *pResOut){
    int rc;

#ifndef NU_IMPL_CODE
    return SQLITE_OK;
#endif

    nucleusFile *pFile = (nucleusFile*)id;
    assert( pFile!=0 );
    if( pFile->locktype>=RESERVED_LOCK ){
        rc = 1;
        OSTRACE3("TEST WR-LOCK %d %d (local)\n", pFile->h, rc);
    }else{
        rc = nucleusLockFile(&(pFile->h), RESERVED_BYTE, 0, 1, 0);
        if( rc ){
            nucleusUnlockFile(&(pFile->h), RESERVED_BYTE, 0, 1, 0);
        }
        rc = !rc;
        OSTRACE3("TEST WR-LOCK %d %d (remote)\n", pFile->h, rc);
    }
    *pResOut = rc;
    return SQLITE_OK;
}

/*
** Control and query of the open file handle.
*/
static int nucleusFileControl(sqlite3_file *id, int op, void *pArg){

    switch (op){
    case SQLITE_FCNTL_LOCKSTATE:
        *((int *)pArg) = ((nucleusFile*)id)->locktype;
        break;
    default:
        return SQLITE_NOTFOUND;
    }

    return SQLITE_OK;
}

/*
** Return the sector size in bytes of the underlying block device for
** the specified file. This is almost always 512 bytes, but may be
** larger for some devices.
**
** SQLite code assumes this function cannot fail. It also assumes that
** if two files are created in the same file-system directory (i.e.
** a database and it's journal file) that the sector size will be the
** same for both.
*/
static int nucleusSectorSize(OsFile *id){
    return SQLITE_DEFAULT_SECTOR_SIZE;
}

/*
** Return a vector of device characteristics.
*/
static int nucleusDeviceCharacteristics(sqlite3_file *id){
    return 0;
}

/*
** This vector defines all the methods that can operate on an OsFile
** for nucleus.
*/
static const sqlite3_io_methods sqlite3NucleusIoMethod = {
    2,                                /* iVersion */
    nucleusClose,                     /* xClose */
    nucleusRead,                      /* xRead */
    nucleusWrite,                     /* xWrite */
    nucleusTruncate,                  /* xTruncate */
    nucleusSync,                      /* xSync */
    nucleusFileSize,                  /* xFileSize */
    nucleusLock,                      /* xLock */
    nucleusUnlock,                    /* xUnlock */
    nucleusCheckReservedLock,         /* xCheckReservedLock */
    nucleusFileControl,               /* xFileControl */
    nucleusSectorSize,                /* xSectorSize */
    nucleusDeviceCharacteristics,     /* xDeviceCapabilities */
    NU_NULL,                          /* xShmMap */
    NU_NULL,                          /* xShmLock */
    NU_NULL,                          /* xShmBarrier */
    NU_NULL                           /* xShmUnmap */
};

/*
** Delete the named file.
**
** Note that nucleus does not allow a file to be deleted if some
** other process has it open. While this other process is holding the
** file open, we will be unable to delete it.  To work around this
** problem, we delay 100 milliseconds and try to delete again.  Up
** to MX_DELETION_ATTEMPTs deletion attempts are run before giving
** up and returning an error.
*/

#define MX_DELETION_ATTEMPTS 3

/*
** If the following global variable points to a string which is the
** name of a directory, then that directory will be used to store
** temporary files.
*/
extern char *sqlite3_temp_directory;

extern INT16 fsl_pc_parsedrive(CHAR  *path, UINT8 use_default);

/*
** Check the existance and status of a file.
*/
static int nuAccess(
    sqlite3_vfs *pVfs,         /* Not used on win32 */
    const char *zFilename,     /* Name of file to check */
    int flags,                 /* Type of test to make on this file */
    int *pResOut               /* OUT: Result */
){
      UINT8 file_attr = 0;
      STATUS status;
      *pResOut = 0;

      status = NU_Get_Attributes(&file_attr, (char*)zFilename);

      if(status != NU_SUCCESS && status != NUF_NOFILE)
          return SQLITE_IOERR_ACCESS;
      else
      {

          switch( flags ){
            case SQLITE_ACCESS_READ:
            /* There is no specific attribute which verifies if
             * read is allowed. So we only check if file exist
             * it should be readable. */
            case SQLITE_ACCESS_EXISTS:
            *pResOut = (status != NUF_NOFILE);
              break;
            case SQLITE_ACCESS_READWRITE:
            *pResOut = (status != NUF_NOFILE) && !(file_attr&ARDONLY);
              break;
            default:
              assert(!"Invalid flags argument");
          }
      }
      return SQLITE_OK;
}

/*
** Create a temporary file name in zBuf.  zBuf must be big enough to
** hold at least SQLITE_TEMPNAME_SIZE characters.
*/
int nucleusTempFileName(char *zBuf){
    static char zChars[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
    int i, j;
    int attrib;
    char zTempPath[MAX_PATH];
    if( sqlite3_temp_directory ){
        strncpy(zTempPath, sqlite3_temp_directory, MAX_PATH-30);
        zTempPath[MAX_PATH-30] = 0;
    }else {
        i = 0;
        zTempPath[i] = 0;
    }

    for(;;){
        sprintf(zBuf, "%s"TEMP_FILE_PREFIX, zTempPath);
        j = strlen(zBuf);
        assert(j<MAX_PATH);
        nuRandomness(NULL, 15,&zBuf[j]);
        for(i=0; i<15; i++, j++){
            zBuf[j] = (char)zChars[ ((unsigned char)zBuf[j])%(sizeof(zChars)-1) ];
        }
        zBuf[j] = 0;
        nuAccess(NU_NULL, zBuf, SQLITE_ACCESS_EXISTS, &attrib);
        if(!(attrib & SQLITE_ACCESS_EXISTS)) break;
    }
    return SQLITE_OK;
}

/*
** Open a file.
*/
static int nuOpen(
    sqlite3_vfs *pVfs,        /* Not used */
    const char *zName,        /* Name of the file (UTF-8) */
    sqlite3_file *id,         /* Write the SQLite file handle here */
    int flags,                /* Open mode flags */
    int *pOutFlags            /* Status return flags */
){
    UINT16 dwDesiredAccess = 0;
    UINT16 dwDesiredMode = 0;
    INT fd;

    nucleusFile *pFile = (nucleusFile*)id;
    CHAR* zOSFileName;              /* Filename in OS encoding */
    UINT8 file_attrib;

    /* If argument zPath is a NULL pointer, this function is required to open
    ** a temporary file. Use this buffer to store the file name in.
    */
    char zTmpname[MAX_PATH+2];     /* Buffer used to create temp filename */

    int rc = SQLITE_OK;            /* Function Return Code */

#ifndef NDEBUG
    int eType = flags&0xFFFFFF00;  /* Type of file to open */
#endif

    int isExclusive  = (flags & SQLITE_OPEN_EXCLUSIVE);
    int isDelete     = (flags & SQLITE_OPEN_DELETEONCLOSE);
    int isCreate     = (flags & SQLITE_OPEN_CREATE);
#ifndef NDEBUG
    int isReadonly   = (flags & SQLITE_OPEN_READONLY);
#endif
    int isReadWrite  = (flags & SQLITE_OPEN_READWRITE);

#ifndef NDEBUG
    int isOpenJournal = (isCreate && (
          eType==SQLITE_OPEN_MASTER_JOURNAL
       || eType==SQLITE_OPEN_MAIN_JOURNAL
       || eType==SQLITE_OPEN_WAL
    ));
#endif

    /* Check the following statements are true:
    **
    **   (a) Exactly one of the READWRITE and READONLY flags must be set, and
    **   (b) if CREATE is set, then READWRITE must also be set, and
    **   (c) if EXCLUSIVE is set, then CREATE must also be set.
    **   (d) if DELETEONCLOSE is set, then CREATE must also be set.
    */
    assert((isReadonly==0 || isReadWrite==0) && (isReadWrite || isReadonly));
    assert(isCreate==0 || isReadWrite);
    assert(isExclusive==0 || isCreate);
    assert(isDelete==0 || isCreate);

    /* The main DB, main journal, WAL file and master journal are never
    ** automatically deleted. Nor are they ever temporary files.  */
    assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MAIN_DB );
    assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MAIN_JOURNAL );
    assert( (!isDelete && zName) || eType!=SQLITE_OPEN_MASTER_JOURNAL );
    assert( (!isDelete && zName) || eType!=SQLITE_OPEN_WAL );

    /* Assert that the upper layer has set one of the "file-type" flags. */
    assert( eType==SQLITE_OPEN_MAIN_DB      || eType==SQLITE_OPEN_TEMP_DB
         || eType==SQLITE_OPEN_MAIN_JOURNAL || eType==SQLITE_OPEN_TEMP_JOURNAL
         || eType==SQLITE_OPEN_SUBJOURNAL   || eType==SQLITE_OPEN_MASTER_JOURNAL
         || eType==SQLITE_OPEN_TRANSIENT_DB || eType==SQLITE_OPEN_WAL
    );

    assert( id!=0 );
    UNUSED_PARAMETER(pVfs);

    memset(pFile, 0, sizeof(*pFile));

    pFile->h = NUF_INVPARM;

    pFile->pVfs = pVfs;

    /* If the second argument to this function is NULL, generate a
    ** temporary file name to use
    */
    if( !zName ){
        assert(isDelete && !isOpenJournal);
        rc = nucleusTempFileName(zTmpname);
        if( rc!=SQLITE_OK ){
            return rc;
        }
        zOSFileName = zTmpname;
    }
    else
        zOSFileName = (CHAR *)zName;

    /* Save the name of file passed in to be open. */
    pFile->zPath = zName;

    /* If the name passed in is a directory return error. */
    rc = NU_Get_Attributes(&file_attrib, (CHAR *)zOSFileName);
    if((rc == NU_SUCCESS) && (file_attrib&ADIRENT))
    {
        return SQLITE_CANTOPEN_ISDIR;
    }

    if(isCreate)
    {
        dwDesiredAccess = PO_CREAT;
        if( isReadWrite ){
            dwDesiredMode =  PS_IREAD | PS_IWRITE;
        }else{
            dwDesiredMode =  PS_IREAD;
        }
    }

    if( isReadWrite ){
        dwDesiredAccess |= (PO_RDWR | PO_BINARY);
    }else{
        dwDesiredAccess |= (PO_RDONLY | PO_BINARY);
    }

    /* SQLITE_OPEN_EXCLUSIVE is used to make sure that a new file is
    ** created. SQLite doesn't use it to indicate "exclusive access"
    ** as it is usually understood.
    */
    if( isExclusive ){
        /* Creates a new file, only if it does not already exist. */
        /* If the file exists, it fails. */
        dwDesiredAccess |= PO_EXCL;
    }

    fd = NU_Open(zOSFileName, dwDesiredAccess, dwDesiredMode);

    /* If the open request is readwrite and it failed, try to open it
     *  in readonly mode. */
    if(fd < 0 && !isExclusive && isReadWrite){
        dwDesiredAccess &= (~PO_RDWR);
        dwDesiredAccess |= PO_RDONLY;

        fd = NU_Open(zOSFileName, dwDesiredAccess, dwDesiredMode);
    }

    /* File opened successfully. */
    if(fd >= 0)
    {
        if(pOutFlags != NULL){
            /* Mark the out flags with access allowed. */
            *pOutFlags = (dwDesiredAccess & PO_RDWR)? SQLITE_OPEN_READWRITE : SQLITE_OPEN_READONLY;
        }

        pFile->h = fd;

        pFile->delOnClose = isDelete? 1 : 0;

        if(isDelete){
            if(NU_Allocate_Memory(sqlite_mem_pool, (VOID **)&(pFile->zDeleteOnClose),
                            (strlen(zOSFileName)+1), NU_NO_SUSPEND) != NU_SUCCESS)
            pFile->zDeleteOnClose = NU_NULL;

            if(!(pFile->zDeleteOnClose)){
                pFile->delOnClose = 0;
                return SQLITE_CANTOPEN;
            }
            strcpy(pFile->zDeleteOnClose,zOSFileName);
        }
        else
            pFile->zDeleteOnClose = NULL;

        /* Create a lock for file, if it may be shared for writing. */
        if(!isExclusive && isReadWrite)
        {
            if(!nucleusCreateLock(zOSFileName,pFile))
            {
                NU_Close(fd);
                NU_Deallocate_Memory(pFile->zDeleteOnClose);
                return SQLITE_CANTOPEN;
            }
        }
        else
        {
            pFile->hShared = -1;
            pFile->hMutex = NU_NULL;
            pFile->shared = NU_NULL;
        }

        pFile->pMethod = &sqlite3NucleusIoMethod;
        pFile->locktype = NO_LOCK;
        pFile->sharedLockByte = 0;

        OpenCounter(+1);
        return SQLITE_OK;
    }
    else
    {
        pFile->pMethod = NU_NULL;
        /* Store the nucleus error code in file handle. */
        pFile->lastErrno = fd;
        return SQLITE_CANTOPEN;
    }
}

/*
** Delete the named file.
**
** Note that Windows does not allow a file to be deleted if some other
** process has it open.  Sometimes a virus scanner or indexing program
** will open a journal file shortly after it is created in order to do
** whatever it does.  While this other process is holding the
** file open, we will be unable to delete it.  To work around this
** problem, we delay 100 milliseconds and try to delete again.  Up
** to MX_DELETION_ATTEMPTs deletion attempts are run before giving
** up and returning an error.
*/
static int nuDelete(
    sqlite3_vfs *pVfs,          /* Not used on win32 */
    const char *zFilename,      /* Name of file to delete */
    int syncDir                 /* Not used on win32 */
)
{
    int cnt = 0;
    STATUS rc;
    UINT8 file_attr = 0;
    STATUS del_rc;
    do{
        del_rc = NU_Delete((char *)zFilename);
    }while(((rc = NU_Get_Attributes(&file_attr, (char *)zFilename)) != NUF_NOFILE)
        && (cnt++ < MX_DELETION_ATTEMPTS) && (nuSleep(NULL, 100), 1) );
    if(del_rc == NU_SUCCESS || rc == NUF_NOFILE){
        return SQLITE_OK;
    }
    else{
        return SQLITE_IOERR;
    }
}

/*
** Turn a relative pathname into a full pathname.  Write the full
** pathname into zOut[].  zOut[] will be at least pVfs->mxPathname
** bytes in size.
*/
static int nuFullPathname(
    sqlite3_vfs *pVfs,            /* Pointer to vfs object */
    const char *zRelative,        /* Possibly relative input path */
    int nFull,                    /* Size of output buffer in bytes */
    char *zFull                   /* Output buffer */
){
    INT16 dd = NU_Get_Default_Drive();
    UINT8 nu_drive[] = "_:";
    UINT8 cont_drv_ltr = (fsl_pc_parsedrive((char *)zRelative, NU_FALSE) < 0) ?
                                                        NU_FALSE : NU_TRUE;
    UINT8 CWD_set = NU_FALSE;
    CHAR path[EMAXPATH] = {0};

    /* If default drive is set, check if CWD is also set. */
    if((dd >= 0) && (dd <= 26))
    {
        nu_drive[0] = dd + 'A';
        if(NU_Current_Dir(nu_drive, path) == NU_SUCCESS)
            CWD_set = (path[0] != '\0')? NU_TRUE:NU_FALSE;
    }

    /* If zRelative does contains complete path or current working
     * directory is set, simply copy the path passed in. */
    if(cont_drv_ltr || CWD_set)
    {
        if(zFull){
            strcpy(zFull,zRelative);
            return SQLITE_OK;
        }
    }
    else
    {
        /* if can not complete path append the SQLite_DEFAULT_PATH before
         * zRelative to create a complete path.*/
        if((strlen(SQLite_DEFAULT_PATH) + strlen(zRelative)) > (nFull-1))
            return SQLITE_CANTOPEN;

        strcpy(zFull, SQLite_DEFAULT_PATH);
        strcat(zFull, zRelative);

    }
    return SQLITE_OK;

}

/* Macros for Julian Day Calculation. */
#define A (14-SQLITE3_MONTH)/12
#define Y (SQLITE3_YEAR + 4800 - A)
#define M (SQLITE3_MONTH + 12 * A - 3)
#define CURRENT_JDN (SQLITE3_DAY + (153*M + 2)/5 + (365*Y) + Y/4 - 32083)

/*
** Find the current time (in Universal Coordinated Time).  Write the
** current time and date as a Julian Day number into *prNow and
** return 0.  Return 1 if the time and date cannot be found.
*/
static int nuCurrentTime(sqlite3_vfs *pVfs, double *prNow){

    UINT32 sqlite3_current_time = NU_Retrieve_Clock()/NU_PLUS_Ticks_Per_Second;

   *prNow = sqlite3_current_time/86400.0 + CURRENT_JDN;

    return SQLITE_OK;
}

/*
** Write up to nBuf bytes of randomness into zBuf.
*/
static int nuRandomness(sqlite3_vfs *pVfs, int nBuf, char *zBuf){
    double time = 0;

    nuCurrentTime(NULL, &time);
    memcpy(zBuf, &time, nBuf);

    return nBuf;

}

/*
** Sleep for a little while.  Return the amount of time slept.
*/
static int nuSleep(sqlite3_vfs *pVfs, int microsec){

    /*The actuall formula is NU_TICKS_PER_SECOND*(microsec/1000000).
     * (microsec/1000000) will convert time from micro sec to seconds.
     *  Modified to microsec/(1000000/NU_TICKS_PER_SECOND), so .
     */
    unsigned int ticks = microsec/(1000000/NU_PLUS_TICKS_PER_SEC);

    /* Still if ticks is 0, sleep for minimal granulate possible. */
    NU_Sleep(ticks?ticks:1);

    return SQLITE_OK;
}

static int nuGetLastError(sqlite3_vfs *pVfs, int nBuf, char *zBuf){

    assert(0);
    return -1;
}

static sqlite3_vfs nucleusVfs = {
    3,                  /* iVersion */
    sizeof(nucleusFile),/* szOsFile */
    MAX_PATH,           /* mxPathname */
    0,                  /* pNext */
    "nucleusVFS",       /* zName */
    0,                  /* pAppData */
    nuOpen,             /* xOpen */
    nuDelete,           /* xDelete */
    nuAccess,           /* xAccess */
    nuFullPathname,     /* xFullPathname */
    NU_NULL,            /* xDlOpen */
    NU_NULL,            /* xDlError */
    NU_NULL,            /* xDlSym */
    NU_NULL,            /* xDlClose */
    nuRandomness,       /* xRandomness */
    nuSleep,            /* xSleep */
    nuCurrentTime,      /* xCurrentTime */
    nuGetLastError,     /* xGetLastError */
    NU_NULL,            /* xCurrentTimeInt64 */
    NU_NULL,            /* xSetSystemCall */
    NU_NULL,            /* xGetSystemCall */
    NU_NULL,            /* xNextSystemCall */
};

extern struct SQLITE_BINARY_MUTEX_STRUCT Sql_File_Mutex;

/*
** Initialize and deinitialize the operating system interface.
*/
int sqlite3_os_init(void){
    STATUS rc;

    /* Register the nucleus virtual file system. */
    sqlite3_vfs_register(&nucleusVfs, 1);

    /* Set the temporary directory path. */
    sqlite3_temp_directory = CFG_NU_OS_STOR_DB_SQLITE_SQL_TEMP_DIRECTORY;

    /* Nucleus file system does not have inherent prperty to lock files.
     * We have created a file manger to provide this property using mutexes.
     * but we need to explicitly initialize it. For details see sqlfile.c*/
    rc = SQLITE_Create_Binary_Mutex(&Sql_File_Mutex,"SQLfile");

    if(rc == SQLITE_OK)
  {
    rc = SQLITE_Init_SQL_File();
    }

    return rc;
}

/*
** Deinitialize the operating system interface.
*/
int sqlite3_os_end(void){

    /* Unregister the nucleus virtual file system. */
    sqlite3_vfs_unregister(&nucleusVfs);

    /* Delete the mutex created for filesystem with
     * locking capability. */
    SQLITE_Binary_Mutex_Delete(&Sql_File_Mutex);

    return SQLITE_OK;

}

/***********************************************************************
*
*   FUNCTION
*
*       SQLiteStringToLower
*
*   DESCRIPTION
*
*       converts a string to lower
*
*   CALLED BY
*
*       nucleusCreateLock
*
*   CALLS
*
*       None.
*
*   INPUTS
*
*       zString        lock name
*
*   OUTPUTS
*
*       None.
*
*
***********************************************************************/
VOID SQLiteStringToLower(CHAR* zString)
{
    INT32 i, length;

    length = strlen(zString);
    for(i = 0; i < length; i++)
    {
        if(isupper((int)zString[i]))
        {
            zString[i] = (CHAR) tolower((int)zString[i]);
        }
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       SQLiteStringConvertCharTo
*
*   DESCRIPTION
*
*       converts characters to others
*
*   CALLED BY
*
*       nucleusCreateLock
*
*   CALLS
*
*       None.
*
*   INPUTS
*
*       zString        input string
*       from_char   character to be converted
*        to_cahr        character to be converted to
*
*   OUTPUTS
*
*       None.
*
*
***********************************************************************/
VOID SQLiteStringConvertCharTo(CHAR* zString, const CHAR from_char, const CHAR to_char)
{
    INT32 i, length;

    length = strlen(zString);
    for(i = 0; i < length; i++)
    {
        if(zString[i] == from_char)
        {
            zString[i] = to_char;
        }
    }

}

/***********************************************************************
*
*   FUNCTION
*
*       SQLiteZeroMemory
*
*   DESCRIPTION
*
*       initializes a memory location with 0.
*
*   CALLED BY
*
*       nucleusCreateLock
*
*   CALLS
*
*       None.
*
*   INPUTS
*
*       zMemory     memory address
*       mem_size    memory size
*
*   OUTPUTS
*
*       None.
*
*
***********************************************************************/
VOID SQLiteZeroMemory(unsigned char* zMemory, INT32 mem_size)
{
    INT32 i;
    for(i=0; i < mem_size; i++)
    {
        zMemory[i] = 0;
    }
}
#endif /* SQLITE_OS_NUCLEUS */
