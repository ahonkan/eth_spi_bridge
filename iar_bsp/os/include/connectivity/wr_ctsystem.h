/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/

/************************************************************************
*                                                                            
* FILE NAME                                                                   
*                                                                                    
*       wr_ctsystem.h                                  
*                                                                                    
* DESCRIPTION                                                                
*        
*       Nucleus wrapper code for SDIO stack                                                                    
*                                                                            *                                                                            
* DEPENDENCIES                                                               
*            
*       nucleus.h       
*		nu_kernel.h          
*                                                                            
*************************************************************************/


#ifndef WR_CTSYSTEM_H
#define WR_CTSYSTEM_H

#include	<string.h>
#include 	<stdarg.h>
 
/* #define DEBUG 1         */ /* Enable trace/debug messages on console */
/* #define DBG_TIMESTAMP 1 */

/* Nucleus support */
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"


/* Mode and attribute type defines */
#define SDIO_HCD_LABEL {0x45,0x94,0xA3,0x9E,0x05,0x1F,0x4b,0xff,0xAD,0x47,0x39,0x9C,0xFE,0x45,0x81,0xFF}

/* HCD Commands */
#define SDIO_GET_MW_CONFIG_PATH 1

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)
#define HCD_PWR_HIB_RESTORE     2
#define SDIO_CMD_DELIMITER      (SDIO_GET_MW_CONFIG_PATH + 2)
#else
#define SDIO_CMD_DELIMITER      (SDIO_GET_MW_CONFIG_PATH + 1)
#endif /* #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE) */


/* generic types */
typedef    UNSIGNED_CHAR    UCHAR;
typedef    UNSIGNED_CHAR *  PUCHAR;
typedef    CHAR             TEXT;
typedef    CHAR *           PTEXT;

typedef    UINT16           USHORT;
typedef    UINT16 *         PUSHORT;


typedef    UINT *           PUINT;
typedef    INT*             PINT;
typedef    UINT             ULONG;
typedef    UINT *           PULONG;
typedef    UINT8 *          PUINT8;
typedef    UINT16 *         PUINT16;
typedef    UINT32 *         PUINT32;
typedef    UNSIGNED_CHAR *  ULONG_PTR;
typedef    VOID *           PVOID;
typedef    BOOLEAN          BOOL;
typedef    BOOLEAN *        PBOOL;
typedef    int              SYSTEM_STATUS;
typedef    unsigned int     EVENT_TYPE;
typedef    unsigned int     EVENT_ARG;
typedef    unsigned int *   PEVENT_TYPE;

#define    OS_DEFAULT_SIGNAL    1       /* default bit to be used in the 
                                        signal event group */
                                        
#define SDDMA_DESCRIPTION_FLAG_DMA   0x1  /* DMA enabled */
#define SDDMA_DESCRIPTION_FLAG_SGDMA 0x2  /* Scatter-Gather DMA enabled */

typedef struct _SDDMA_DESCRIPTION {
    UINT16      Flags;                  /* SDDMA_DESCRIPTION_FLAG_xxx */
    UINT16      MaxDescriptors;         /* number of supported scatter gather entries */
    UINT32      MaxBytesPerDescriptor;  /* maximum bytes in a DMA descriptor entry */
    UINT32      Mask;                   /* dma address mask */
    UINT32      AddressAlignment;       /* dma address alignment mask, least significant bits indicate illegal address bits */
    UINT32      LengthAlignment;        /* dma buffer length alignment mask, least significant bits indicate illegal length bits  */
}SDDMA_DESCRIPTION;
#define PSDDMA_DESCRIPTION SDDMA_DESCRIPTION*

typedef struct _SDDMA_DESCRIPTOR {
    UINT32      Address;
    UINT32      Length;
} SDDMA_DESCRIPTOR;
#define PSDDMA_DESCRIPTOR SDDMA_DESCRIPTOR*

struct _OSKERNEL_HELPER;

typedef INT (*PHELPER_FUNCTION)(struct _OSKERNEL_HELPER *);

typedef struct _OSKERNEL_HELPER {
    NU_TASK                 Task;
    BOOL                    ShutDown;
    NU_EVENT_GROUP          WakeSignal;
    NU_EVENT_GROUP		    Completion; 
    PVOID                   pContext;
    PHELPER_FUNCTION        pHelperFunc;
	PVOID					stack;
}OSKERNEL_HELPER, *POSKERNEL_HELPER;

#ifdef __CC_ARM
	#define	packed					__packed
#endif
#ifdef __GNUC__     /* GNU ARM */
#define __ATTRIB_PACK           __attribute__ ((packed))
#define INLINE inline
#define POSTPACK                __ATTRIB_PACK
#define PREPACK
#else
#define __ATTRIB_PACK           packed
#define INLINE                  __inline
#define inline                  __inline
#define POSTPACK
#define PREPACK                 __ATTRIB_PACK
#endif

#ifdef _TARGET_ARM
#define INLINE inline
#endif

#define CT_PACK_STRUCT

/* Debug printing */

//#include "serial/nu_sd.h"
#include <stdio.h>

/* debug print macros */
#ifdef DEBUG

extern VOID NU_Printf(char * buffer,...);

/*************************************************************************
*
*   MACRO
*
*       DBG_ASSERT
*
*   DESCRIPTION
*
*       Evaluation the expression and throw an assertion if false.
*
*   INPUTS
*
*       test   - boolean expression
*
*   NOTES
*
*       This function can be conditionally compiled using the c-define DEBUG.
*   
*************************************************************************/
#define DBG_ASSERT(test)    \
{                           \
    if (!(test)) {          \
        DBG_PRINT(SDDBG_ERROR, ("Debug Assert Caught, File %s, Line: %d, Test:%s \n",__FILE__, __LINE__,#test)); \
    }                       \
}
#define DBG_ASSERT_WITH_MSG(test,s) \
{                                   \
    if (!(test)) {                  \
        DBG_PRINT(SDDBG_ERROR, ("Assert:%s File %s, Line: %d \n",(s),__FILE__, __LINE__)); \
    }                       \
}

#ifdef DBG_DECLARE

int debuglevel = DBG_DECLARE;

#ifdef DBG_TIMESTAMP
    cycles_t g_lasttimestamp = 0;
#endif /* DBG_TIMESTAMP */

#else  /* DBG_DECLARE */

extern int debuglevel;

#ifdef DBG_TIMESTAMP
extern cycles_t g_lasttimestamp;
#endif /* DBG_TIMESTAMP */

#endif /* DBG_DECLARE */


/*************************************************************************
*
*   MACRO
*
*       DBG_PRINT
*
*   DESCRIPTION
*
*       Print a string to the debugger or console
*       If Level is less than the current debug level, the print will be
*       issued.  This function can be conditionally compiled using the c-define DEBUG.
* 
*       Example: 
*           DBG_PRINT(MY_DBG_LEVEL, ("Return Status: %d\r\n",status));
*
*
*   INPUTS
*
*       lvl   - debug level for the print
*       args  - formatting string
*
*   OUTPUTS
*
*       None
*
*   
*************************************************************************/

#ifdef DBG_TIMESTAMP

#define DBG_PRINT(lvl, args)\
    {if (lvl <= DBG_GET_DEBUG_LEVEL()) {\
        ulong _delta_timestamp;\
        cycles_t _last_timestamp = g_lasttimestamp;\
        g_lasttimestamp = get_cycles();\
        /* avoid 64-bit divides, to microseconds*/\
        _delta_timestamp =((ulong)(g_lasttimestamp - _last_timestamp)/(ulong)(cpu_khz/(ulong)1000));\
        NU_Printf("(%lld:%ldus) ", g_lasttimestamp, _delta_timestamp);\
        NU_Printf(_DBG_PRINTX_ARG args);\
        }\
    }

#else /* DBG_TIMESTAMP */

extern VOID NU_Debug_Printf (char *format, ...);
#define DBG_PRINT(lvl, args)\
    {if (lvl <= DBG_GET_DEBUG_LEVEL())\
        NU_Debug_Printf(_DBG_PRINTX_ARG args);\
    }
#endif /* DBG_TIMESTAMP */


#else
/* DEBUG is not defined */

#ifdef DBG_DECLARE

int debuglevel = DBG_DECLARE;

#else  /* DBG_DECLARE */

extern int debuglevel;

#endif /* DBG_DECLARE */

#define DBG_PRINT(lvl, str)
#define DBG_ASSERT(test)
#define DBG_ASSERT_WITH_MSG(test,s)


#endif /* DEBUG */

#define _DBG_PRINTX_ARG(...)  __VA_ARGS__

#define DBG_GET_DEBUG_LEVEL() (debuglevel)

#define DBG_SET_DEBUG_LEVEL(v) debuglevel = (v)

/*************************************************************************
*
*   MACRO
*
*       REL_PRINT
*
*   DESCRIPTION
*
*       Print a string to the debugger or console
*       If Level is less than the current debug level, the print will be
*       issued.  This function can not be conditionally compiled using the c-define DEBUG.
* 
*       Example: 
*           REL_PRINT(MY_DBG_LEVEL, ("Return Status: %d\r\n",status));
*
*   INPUTS
*
*       lvl   - debug level for the print
*       args  - formatting string
*
*   OUTPUTS
*
*       None
*
*   
*************************************************************************/
#define REL_PRINT(lvl, args)\
    {if ((lvl <= DBG_GET_DEBUG_LEVEL())) \
        NU_Printf(args);\
    }

/* debug output levels, this must be order low number to higher */
#define SDDBG_ERROR 3  
#define SDDBG_WARN  4  
#define SDDBG_DEBUG 6  
#define SDDBG_TRACE 7  

#define NonSchedulable() false

static inline STATUS CriticalSectionInit(NU_SEMAPHORE * pCrit);
static inline STATUS CriticalSectionAcquire(NU_SEMAPHORE * pCrit);
static inline STATUS CriticalSectionRelease(NU_SEMAPHORE * pCrit);
static inline void CriticalSectionDelete(NU_SEMAPHORE * pCrit);

static inline STATUS SemaphoreInitialize(NU_SEMAPHORE * pSem, UINT value);
static inline void SemaphoreDelete(NU_SEMAPHORE * pSem);
static inline STATUS SemaphorePend(NU_SEMAPHORE * pSem);
static inline STATUS SemaphorePendInterruptable(NU_SEMAPHORE * pSem);
static inline STATUS SemaphorePost(NU_SEMAPHORE * pSem);

static inline STATUS SignalInitialize(NU_EVENT_GROUP * pSignal);
static inline void SignalDelete(NU_EVENT_GROUP * pSignal);
static inline STATUS SignalWaitInterruptible(NU_EVENT_GROUP * pSignal);
static inline STATUS SignalWait(NU_EVENT_GROUP * pSignal);
static inline STATUS SignalSet(NU_EVENT_GROUP * pSignal);

static inline PVOID KernelAlloc(UINT size);
static inline PVOID KernelAlloc_Suspend(UINT size);
static inline void KernelFree(PVOID ptr);
static inline PVOID KernelAllocIrqSafe(UINT size);
static inline void KernelFreeIrqSafe(PVOID ptr);
extern NU_MEMORY_POOL *sdio_mem_pool;

static inline STATUS OSErrorToSDIOError(STATUS status);
static inline STATUS SDIOErrorToOSError(STATUS status);
static inline STATUS OSSleep(INT sleepInterval);

static inline UINT32 AtomicTest_Set(volatile UINT32 *pValue, UINT bitNo);
static inline UINT32 AtomicTest_Clear(volatile UINT32 *pValue, UINT BitNo) ;


/*************************************************************************
*
*   INLINE FUNCTION
*
*       CriticalSectionInit
*
*   DESCRIPTION
*
*       Initialize a critical section, using semaphore
*
*   INPUTS
*
*       pCrit - pointer to critical section to initialize
*
*   OUTPUTS
*
*       NU_SUCCESS on success.
*
*   NOTES
*
*       CriticalSectionDelete() must be called to cleanup any resources
*           associated with the critical section.
*     
*       See also: CriticalSectionDelete, CriticalSectionAcquire,
*                 CriticalSectionRelease 
*
*       Example: To initialize a critical section:        
*           status = CriticalSectionInit(&pDevice->ListLock);
*           if (!SDIO_SUCCESS(status)) {
*              .. failed
*              return status;
*           }
*
*	
*   
*************************************************************************/
static inline STATUS CriticalSectionInit(NU_SEMAPHORE * pCrit) 
{
    STATUS status;
    
    (void)memset ((pCrit), 0, sizeof(*(pCrit)));
    
    status = NU_Create_Semaphore(pCrit, "SD_CRIT", (UNSIGNED)1, (OPTION)NU_PRIORITY);
    
    return (STATUS)OSErrorToSDIOError(status);
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       CriticalSectionAcquire
*
*   DESCRIPTION
*
*       Acquire a critical section lock. 
*
*   INPUTS
*
*       pCrit - pointer to critical section that was initialized
*
*   OUTPUTS
*
*       NU_SUCCESS on success.
*
*   NOTES
*
*	The critical section lock is acquired when this function returns 
*            NU_SUCCESS.  Use CriticalSectionRelease() to release
*            the critical section lock.
*
*       See also: CriticalSectionRelease            
*
*	Example: To acquire a critical section lock:        
*           status = CriticalSectionAcquire(&pDevice->ListLock);
*           if (!SDIO_SUCCESS(status)) {
*               .. failed
*               return status;
*           }

*           .. access protected data
*           .. unlock 
*           status = CriticalSectionRelease(&pDevice->ListLock);
*           if (!SDIO_SUCCESS(status)) {
*               .. failed
*               return status;
*           }
*   
*************************************************************************/
static inline STATUS CriticalSectionAcquire(NU_SEMAPHORE * pCrit) 
{
    STATUS status = NU_Obtain_Semaphore(pCrit, (UNSIGNED)NU_SUSPEND);

    return (STATUS)OSErrorToSDIOError(status);
}

#define CT_DECLARE_IRQ_SYNC_CONTEXT() 

#define CriticalSectionAcquireSyncIrq(pCrit) CriticalSectionAcquire(pCrit)

#define CriticalSectionReleaseSyncIrq(pCrit) CriticalSectionRelease(pCrit)

/*************************************************************************
*
*   INLINE FUNCTION
*
*       CriticalSectionRelease
*
*   DESCRIPTION
*
*       
*
*   INPUTS
*
*       pCrit - pointer to critical section that was initialized
*
*
*   OUTPUTS
*
*       NU_SUCCESS on success.
*
*   NOTES
*
*       The critical section lock is released when this function returns 
*           NU_SUCCESS. 
*
*       See also: CriticalSectionAcquire       
*  
*       Example: see CriticalSectionAcquire
*   
*************************************************************************/
static inline STATUS CriticalSectionRelease(NU_SEMAPHORE * pCrit) 
{
    STATUS status;

    status = NU_Release_Semaphore(pCrit);

    return (STATUS)OSErrorToSDIOError(status);
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       CriticalSectionDelete
*
*   DESCRIPTION
*
*       Cleanup a critical section
*
*   INPUTS
*
*       pCrit - an initialized critical section object
*
*    OUTPUTS
*
*       NU_SUCCESS on success.
*
*   NOTES
*
*	See also: CriticalSectionInit, CriticalSectionAcquire,
*                 CriticalSectionRelease            
*
*   
*************************************************************************/
static inline void CriticalSectionDelete(NU_SEMAPHORE * pCrit) 
{
    (void)NU_Delete_Semaphore(pCrit);
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       SemaphoreInitialize
*
*   DESCRIPTION
*
*       Initialize a semaphore object.
*
*   INPUTS
*
*       value - initial value of the semaphore
*       pSem  - pointer to a semaphore object to initialize
*
*   OUTPUTS
*
*       NU_SUCCESS on success.
*
*   NOTES
*       SemaphoreDelete() must be called to cleanup any resources
*       associated with the semaphore
*
*       See also: SemaphoreDelete, SemaphorePend, SemaphorePendInterruptable
*  
*       Example: To initialize a semaphore:        
*           status = SemaphoreInitialize(&pDevice->ResourceSem,1);
*           if (!SDIO_SUCCESS(status)) {
*               .. failed
*               return status;
*           }
*	    
*   
*************************************************************************/
static inline STATUS SemaphoreInitialize (NU_SEMAPHORE * pSem, UINT value) 
{
    STATUS status;

    (void)memset((pSem),0,sizeof(*(pSem)));

    status = NU_Create_Semaphore (pSem, "SD_CRIT", (UNSIGNED)value, (OPTION)NU_PRIORITY);

    return OSErrorToSDIOError(status);
 
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       SemaphoreDelete
*
*   DESCRIPTION
*
*       Cleanup a semaphore object.
*
*   INPUTS
*
*       pSem - pointer to a semaphore object to cleanup
*
*   OUTPUTS
*
*       None		
*
*   NOTES
*
*	    
*   
*************************************************************************/
static inline void SemaphoreDelete(NU_SEMAPHORE * pSem) 
{
    (void)NU_Delete_Semaphore(pSem);
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       SemaphorePend
*
*   DESCRIPTION
*
*       Acquire the semaphore or pend if the resource is not available
*
*   INPUTS
*
*       pSem - pointer to an initialized semaphore object
*
*   OUTPUTS
*
*       NU_SUCCESS on success.
*
*   NOTES
*
*	If the semaphore count is zero this function blocks until the count
*       becomes non-zero, otherwise the count is decremented and  execution 
*       continues. While waiting, the task/thread cannot be interrupted. 
*       If the task or thread should be interruptible, use SemaphorePendInterruptible.
*       On some OSes SemaphorePend and SemaphorePendInterruptible behave the same.
*
*       See also: SemaphorePendInterruptable, SemaphorePost          
*
*       Example: To wait for a resource using a semaphore:        
*           status = SemaphorePend(&pDevice->ResourceSem);
*           if (!SDIO_SUCCESS(status)) {
*               .. failed
*               return status;
*           }     
*           ... resource acquired
*           SemaphorePost(&pDevice->ResourceSem);
*   
*************************************************************************/
static inline STATUS SemaphorePend(NU_SEMAPHORE * pSem) 
{
    STATUS status;

    status = NU_Obtain_Semaphore(pSem, (UNSIGNED)NU_SUSPEND);

    return (STATUS)OSErrorToSDIOError(status);
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       SemaphorePendInterruptable
*
*   DESCRIPTION
*
*       Acquire the semaphore or pend if the resource is not available
*
*   INPUTS
*
*       pSem - pointer to an initialized semaphore object
*
*   OUTPUTS
*
*       NU_SUCCESS on success.
*
*   NOTES
*
*       If the semaphore count is zero this function blocks until the count
*       becomes non-zero, otherwise the count is decremented and execution 
*       continues. While waiting, the task/thread can be interrupted. 
*       If the task or thread should not be interruptible, use SemaphorePend.
*
*       See also: SemaphorePend, SemaphorePost          
*
*       Example: To wait for a resource using a semaphore:        
*           status = SemaphorePendInterruptable(&pDevice->ResourceSem);
*           if (!SDIO_SUCCESS(status)) {
*                .. failed, could have been interrupted
*               return status;
*           }  
*           ... resource acquired
*           SemaphorePost(&pDevice->ResourceSem);
*   
*************************************************************************/
static inline STATUS SemaphorePendInterruptable(NU_SEMAPHORE * pSem) 
{
    STATUS status;

    status = NU_Obtain_Semaphore(pSem, (UNSIGNED)NU_SUSPEND);

    return (STATUS)OSErrorToSDIOError(status);
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       SemaphorePost
*
*   DESCRIPTION
*
*       Post a semaphore.
*
*   INPUTS
*
*       pSem - pointer to an initialized semaphore object
*
*   OUTPUTS
*
*       NU_SUCCESS on success.
*
*   NOTES
*
*	This function increments the semaphore count.
*
*       See also: SemaphorePend, SemaphorePendInterruptable.          
*
*       Example: Posting a semaphore:        
*           status = SemaphorePendInterruptable(&pDevice->ResourceSem);
*           if (!SDIO_SUCCESS(status)) {
*               .. failed, could have been interrupted
*               return status;
*           }  
*           ... resource acquired
*           .. Post the semaphore 
*           SemaphorePost(&pDevice->ResourceSem);
*   
*************************************************************************/
static inline STATUS SemaphorePost(NU_SEMAPHORE * pSem) 
{
    return (STATUS)OSErrorToSDIOError((STATUS)NU_Release_Semaphore(pSem));
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       SignalInitialize
*
*   DESCRIPTION
*
*       Initialize a signal, which is implemented as a semaphore
*
*   INPUTS
*
*       pSignal - pointer to a semaphore
*
*   OUTPUTS
*
*       NU_SUCCESS on success.
*
*   NOTES
*
*	    
*   
*************************************************************************/
static inline STATUS SignalInitialize(NU_EVENT_GROUP * pSignal) 
{
    STATUS status;
    
    status = NU_Create_Event_Group(pSignal, "SDIOSIG");
	
    return (STATUS)OSErrorToSDIOError(status);
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       SignalDelete
*
*   DESCRIPTION
*
*       Removed a signal, which is implemented as a semaphore
*
*   INPUTS
*
*       pSignal - pointer to a signal
*
*   OUTPUTS
*
*       NU_SUCCESS on success.
*		
*   NOTES
*
*   
*************************************************************************/
static inline void SignalDelete(NU_EVENT_GROUP * pSignal) 
{
     (VOID)NU_Delete_Event_Group(pSignal);
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       SignalWaitInterruptible
*
*   DESCRIPTION
*
*       Wait for a signal, pend if the resource is not set
*
*   INPUTS
*
*       pSignal - pointer to a signal
*
*   OUTPUTS
*
*       NU_SUCCESS on success.
*		
*   NOTES
*
*   
*************************************************************************/
static inline STATUS SignalWaitInterruptible(NU_EVENT_GROUP * pSignal) 
{
    UNSIGNED      retrieved_signal;
    STATUS status;

    status = NU_Retrieve_Events(pSignal, OS_DEFAULT_SIGNAL, 
                NU_OR_CONSUME, &retrieved_signal, NU_SUSPEND);
                
    return (STATUS)OSErrorToSDIOError(status);
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       SignalWait
*
*   DESCRIPTION
*
*       Wait for a signal, pend if the resource is not set
*
*   INPUTS
*
*       pSignal - pointer to a signal
*
*   OUTPUTS
*
*       NU_SUCCESS on success.
*
*   NOTES
*	    
*   
*************************************************************************/
static inline STATUS SignalWait(NU_EVENT_GROUP * pSignal) 
{
    return SignalWaitInterruptible(pSignal);

}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       SignalSet
*
*   DESCRIPTION
*
*       Set the signal
*
*   INPUTS
*
*       pSignal - pointer to a signal
*
*   OUTPUTS
*
*       NU_SUCCESS on success.
*
*   NOTES
*	    
*   
*************************************************************************/
static inline STATUS SignalSet(NU_EVENT_GROUP * pSignal) 
{
    STATUS status;
    
    status = NU_Set_Events (pSignal, OS_DEFAULT_SIGNAL, NU_OR);
    
    return (STATUS)OSErrorToSDIOError(status);;
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       KernelAlloc
*
*   DESCRIPTION
*
*       
*
*   INPUTS
*
*       size - size of memory block to allocate
*
*   OUTPUTS
*
*
*       ptr  - pointer to the allocated memory, NULL if allocation failed
*
*   NOTES
*
*       For operating systems that use paging, the allocated memory is always
*       non-paged memory.  Caller should only use KernelFree() to release the
*       block of memory.  This call can potentially block and should only be called
*       from a schedulable context.  Use KernelAllocIrqSafe() if the allocation
*       must be made from a non-schedulable context.
*
*       See also: KernelFree, KernelAllocIrqSafe         
*
*       Example: allocating memory:        
*           pBlock = KernelAlloc(1024);
*           if (pBlock == NULL) {
*               . failed, no memory
*               return SDIO_STATUS_INSUFFICIENT_RESOURCES;
*           }   	    
*   
*************************************************************************/
static inline PVOID KernelAlloc(UINT size) 
{
    PVOID         ptr;
    STATUS status;

    status = NU_Allocate_Memory (sdio_mem_pool, &ptr, (UNSIGNED)size, (UNSIGNED)NU_NO_SUSPEND);

    if (status != NU_SUCCESS) 
    {
        ptr = NU_NULL;
    }
    else
    {
        (void)memset(ptr, 0, (int)size);
    }

    return ptr;
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       KernelAlloc_Suspend
*
*   DESCRIPTION
*
*       
*
*   INPUTS
*
*       size - size of memory block to allocate
*
*   OUTPUTS
*
*       ptr  - pointer to the allocated memory, NULL if allocation failed
*
*   NOTES
*
*       For operating systems that use paging, the allocated memory is always
*       non-paged memory.  Caller should only use KernelFree() to release the
*       block of memory.  This call can potentially block and should only be called
*       from a schedulable context.  Use KernelAllocIrqSafe() if the allocation
*       must be made from a non-schedulable context.
*
*       See also: KernelFree, KernelAllocIrqSafe         
*
*       Example: allocating memory:        
*           pBlock = KernelAlloc_Suspend(1024);
*           if (pBlock == NULL) {
*               .. failed, no memory
*               return SDIO_STATUS_INSUFFICIENT_RESOURCES;
*           }   	    
*   
*************************************************************************/
static inline PVOID KernelAlloc_Suspend(UINT size) 
{
    PVOID         ptr;
    STATUS status;

    status = NU_Allocate_Memory (sdio_mem_pool, &ptr, (UNSIGNED)size, (UNSIGNED)NU_SUSPEND);
    if (status != NU_SUCCESS) 
    {
        ptr = NU_NULL;
    }
    else
    {
        (void)memset(ptr, 0, (int)size);
    }

    return ptr;
}
/*************************************************************************
*
*   INLINE FUNCTION
*
*       KernelFree
*
*   DESCRIPTION
*
*       Free a block of kernel accessible memory.
*
*   INPUTS
*
*       ptr - pointer to memory allocated with KernelAlloc()
*
*   OUTPUTS
*
*       None		
*
*   NOTES
*
*       Caller should only use KernelFree() to release memory that was *allocated
*       with KernelAlloc().
*
*       See also: KernelAlloc        

*       Example: KernelFree(pBlock);
*        
*   
*************************************************************************/
static inline void KernelFree(PVOID ptr)
{
    (void)NU_Deallocate_Memory(ptr);
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       KernelAllocIrqSafe
*
*   DESCRIPTION
*
*       Allocate a block of kernel accessible memory in an IRQ-safe manner
*
*   INPUTS
*
*       size - size of memory block to allocate
*
*   OUTPUTS
*
*       ptr  - pointer to the allocated memory, NULL if allocation failed
*
*   NOTES
*
*	This variant of KernelAlloc allows the allocation of small blocks of
*       memory from an ISR or from a context where scheduling has been disabled.
*       The allocations should be small as the memory is typically allocated
*       from a critical heap. The caller should only use KernelFreeIrqSafe() 
*       to release the block of memory.
*
*       See also: KernelAlloc, KernelFreeIrqSafe     
*
*       Example: allocating memory:        
*           pBlock = KernelAllocIrqSafe(16);
*           if (pBlock == NULL) {
*               .. failed, no memory
*               return SDIO_STATUS_INSUFFICIENT_RESOURCES;
*           }   
*   
*************************************************************************/
static inline PVOID KernelAllocIrqSafe(UINT size) 
{
    PVOID         ptr;
    STATUS status;

    status = NU_Allocate_Memory (sdio_mem_pool, &ptr, (UNSIGNED)size, (UNSIGNED)NU_NO_SUSPEND);
    if (status != NU_SUCCESS) 
    {
        ptr = NU_NULL;
    }
    return ptr;
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       KernelFreeIrqSafe
*
*   DESCRIPTION
*
*       Free a block of kernel accessible memory.
*
*   INPUTS
*
*       ptr - pointer to memory allocated with KernelAllocIrqSafe()
*
*   OUTPUTS
*
*       NU_SUCCESS on success.
*
*   NOTES
*
*       Caller should only use KernelFreeIrqSafe() to release memory that *was allocated
*       with KernelAllocIrqSafe().
*
*       See also: KernelAllocIrqSafe
*
*       Example: KernelFreeIrqSafe(pBlock);
*        
*   
*************************************************************************/
static inline void KernelFreeIrqSafe(PVOID ptr) 
{
    KernelFree(ptr);
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       SDIOErrorToOSError
*
*   DESCRIPTION
*
*       Error and status code conversion
*
*   INPUTS
*
*       status        - in STATUS code
*
*   OUTPUTS
*
*       STATUS - in os-dependent STATUS
*
*   NOTES
*
*   
*************************************************************************/
static inline STATUS SDIOErrorToOSError (STATUS status) 
{
    STATUS system_status;

    switch (status) 
    {
        case NU_SUCCESS: 
            system_status = NU_SUCCESS;
            break;
        case SDIO_STATUS_INVALID_PARAMETER:
            system_status = NU_INVALID_OPERATION;
            break;
        case SDIO_STATUS_PENDING:
            system_status = NU_SUCCESS; /* try again */
            break;
        case SDIO_STATUS_DEVICE_NOT_FOUND:
            system_status = NU_INVALID_DRIVER;
            break;
        case SDIO_STATUS_DEVICE_ERROR:
            system_status = NU_UNAVAILABLE;
            break;
        case SDIO_STATUS_INTERRUPTED:
            system_status = NU_TIMEOUT;
            break;
        case SDIO_STATUS_NO_RESOURCES:
            system_status = NU_NO_MEMORY;
            break;
        case SDIO_STATUS_ERROR:    
        default:
            system_status = NU_UNAVAILABLE;
            break;
    }
    return system_status;
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       OSErrorToSDIOError
*
*   DESCRIPTION
*
*       Error and status code conversion
*
*   INPUTS
*
*       status       - in os-dependent STATUS code
*
*   OUTPUTS
*
*       STATUS 		
*
*   NOTES
*
*   
*************************************************************************/
static inline STATUS OSErrorToSDIOError (STATUS status) 
{
    STATUS sdio_status;
    
    if (status != NU_SUCCESS) 
    {
        DBG_PRINT(SDDBG_ERROR, ("SDIO OSErrorToSDIOError - Error:%d \n",
                                    status));
    }
    
    switch (status) 
    {
        case NU_SUCCESS:
            sdio_status = NU_SUCCESS;
            break;
        case NU_INVALID_DELETE:
        case NU_INVALID_DRIVER:
        case NU_INVALID_ENABLE:
        case NU_INVALID_ENTRY:
        case NU_INVALID_FUNCTION:
        case NU_INVALID_GROUP:
        case NU_INVALID_HISR:
        case NU_INVALID_MAILBOX:
        case NU_INVALID_MEMORY:
        case NU_INVALID_MESSAGE:
        case NU_INVALID_OPERATION:
        case NU_INVALID_PIPE:
        case NU_INVALID_POINTER:
        case NU_INVALID_POOL:
        case NU_INVALID_PREEMPT:
        case NU_INVALID_PRIORITY:
        case NU_INVALID_QUEUE:
        case NU_INVALID_RESUME:
        case NU_INVALID_SEMAPHORE:
        case NU_INVALID_SIZE:
        case NU_INVALID_START:
        case NU_INVALID_SUSPEND:
        case NU_INVALID_TASK:
        case NU_INVALID_TIMER:
        case NU_INVALID_VECTOR:
            sdio_status =  SDIO_STATUS_INVALID_PARAMETER;
            break;
        case NU_INVALID_REGION:
        case NU_MEMORY_CORRUPT:
        case NU_EMPTY_DEBUG_ALLOCATION_LIST:
            sdio_status = SDIO_STATUS_NO_RESOURCES;
            break;
        case NU_SEMAPHORE_RESET:
            sdio_status = SDIO_STATUS_ERROR;
            break;
        default:
            sdio_status = SDIO_STATUS_ERROR;
            break;
    }

    return sdio_status;
}

/*************************************************************************
*
*   INLINE FUNCTION
*
*       OSSleep
*
*   DESCRIPTION
*
*       Sleep or delay the execution context for a number of milliseconds.
*
*   INPUTS
*
*       sleepInterval - time in milliseconds to put the execution context to sleep
*
*   OUTPUTS
*
*       NU_SUCCESS on success.
*
*   NOTES
*
*       Caller should be in a context that allows it to sleep or block.  The 
*       minimum duration of sleep may be greater than 1 MS on some platforms and OSes.   
*
*       See also: OSSleep         
*
*       Example: Using sleep to delay
*           EnableSlotPower(pSlot);
*           // Wait for power to settle
*           status = OSSleep(100);
*           if (!SDIO_SUCCESS(status)){
*               // failed..
*           }
*   
*************************************************************************/
static inline STATUS OSSleep (INT sleepInterval) 
{
    UNSIGNED tim = (sleepInterval * NU_PLUS_Ticks_Per_Second) / 1000;

    NU_Sleep(tim);

    return (STATUS)NU_SUCCESS;
} 

/*************************************************************************
*
*   MACRO
*
*       SD_GET_OS_DEVICE
*
*   DESCRIPTION
*
*       Get the OS-dependent device object
*
*   INPUTS
*
*       pDevice - the device on the HCD
*
*   OUTPUTS
*
*       pointer to the OSs device
*
*   NOTES
*
*       Example: obtain low level device
*           pFunctionContext->GpsDevice.Port.dev = SD_GET_OS_DEVICE(pDevice); 
*
*   
*************************************************************************/

#define SD_GET_OS_DEVICE(pDevice)  (NULL)

/*************************************************************************
*
*   MACRO
*
*       SD_GET_HCD_OS_DEVICE
*
*   DESCRIPTION
*
*       Get the HCD OS device object
*
*   INPUTS
*
*       pDevice - the device on the HCD
*
*   OUTPUTS
*
*       pointer to the HCD's OS device
*
*   NOTES
*
*
*************************************************************************/

#define SD_GET_HCD_OS_DEVICE(p)  ((p)->pHcd->pDevice)

/*************************************************************************
*
*   INLINE FUNCTION
*
*       AtomicTest_Set
*
*   DESCRIPTION
*
*       Test and set a bit atomically
*
*   INPUTS
*
*       pValue - pointer to a UINT32
*       bitNo - which bit to be tested
*
*   OUTPUTS
*
*       The value of the bit before the operation
*
*   NOTES
*
*	    
*   
*************************************************************************/
static inline UINT32 AtomicTest_Set(volatile UINT32 *pValue, 
                                            UINT bitNo) 
{
    UINT32 	oldvalue = *pValue;
    INT     		int_level;
    UINT32     		bit = (UINT)1<<bitNo;

    /* Disable interrupts */
    int_level = (INT)NU_Local_Control_Interrupts((INT)NU_DISABLE_INTERRUPTS);

    /* Check if flag is not set */
    if ((*pValue & bit) == (UNSIGNED)0) {
        *pValue |= bit;
    }

    /* Re-enable interrupts */
    (void)NU_Local_Control_Interrupts(int_level);

    return (UINT32)(oldvalue & bit);
	
}   
/*************************************************************************
*
*   INLINE FUNCTION
*
*       AtomicTest_Clear
*
*   DESCRIPTION
*
*       Test and clear a bit atomically
*
*   INPUTS
*
*       pValue - pointer to a UINT32
*       bitNo  - which bit to be tested
*
*   OUTPUTS
*
*       The value of the bit before the operation
*
*   NOTES
*
*	    
*   
*************************************************************************/
static inline UINT32 AtomicTest_Clear(volatile UINT32 *pValue, UINT BitNo) 
{
    INT int_level;
    UINT32 oldvalue = *pValue;
    UINT32 bit = (UINT)1 << BitNo;

    /* Disable interrupts */
    int_level = NU_Local_Control_Interrupts((INT)NU_DISABLE_INTERRUPTS);
    *pValue &= ~bit;
	
    /* Re-enable interrupts */
    (void)NU_Local_Control_Interrupts(int_level);

    return (UINT32)(oldvalue & bit);
}  


/*************************************************************************
*
*   MACRO
*
*       SD_WAKE_OS_HELPER
*
*   DESCRIPTION
*
*       Wake the helper thread
*
*   INPUTS
*
*       pOSHelper - the OS helper object
*
*   OUTPUTS
*
*       ???
*		
*   NOTES
*
*	Example: Waking up a helper thread
*           status = SD_WAKE_OS_HELPER(&pInstance->OSHelper); 
*
*   
*************************************************************************/

#define SD_WAKE_OS_HELPER(p)        SignalSet(&(p)->WakeSignal)

/*************************************************************************
*
*   MACRO
*
*       SD_GET_OS_HELPER_CONTEXT
*
*   DESCRIPTION
*
*       Obtains the context for the helper function
*
*   INPUTS
*
*       pOSHelper - the OS helper object
*
*   OUTPUTS
*
*       helper specific context  
*
*   NOTES
*
*       This macro should only be called by the function associated with
*       the helper object.
*          
*       See also: SDLIB_OSCreateHelper
*         
*       Example: Getting the helper specific context
*           PMYCONTEXT pContext = *(PMYCONTEXT)SD_GET_OS_HELPER_CONTEXT(pHelper); 
*
*   
*************************************************************************/

#define SD_GET_OS_HELPER_CONTEXT(p)     (p)->pContext

/*************************************************************************
*
*   MACRO
*
*       SD_IS_HELPER_SHUTTING_DOWN
*
*   DESCRIPTION
*
*       Check helper function shut down flag.
*
*   INPUTS
*
*       ???       
*
*   OUTPUTS
*
*       TRUE if shutting down, else FALSE 
*
*   NOTES
*
*       This macro should only be called by the function associated with
*       the helper object.  The function should call this macro when it
*       unblocks from the call to SD_WAIT_FOR_WAKEUP().  If this *function 
*       returns TRUE, the function should clean up and exit. 
*          
*       See also: SDLIB_OSCreateHelper , SD_WAIT_FOR_WAKEUP
*         
*       Example: Checking for shutdown
*           while(1) {
*               status = SD_WAIT_FOR_WAKEUP(pHelper);
*               if (!SDIO_SUCCESS(status)) {
*                   break;
*               }
*               if (SD_IS_HELPER_SHUTTING_DOWN(pHelper)) {
*                  ... shutting down
*                   break;
*               }
*           }
*        
*   
*************************************************************************/

#define SD_IS_HELPER_SHUTTING_DOWN(p)   (p)->ShutDown

/*************************************************************************
*
*   MACRO
*
*       SD_WAIT_FOR_WAKEUP
*
*   DESCRIPTION
*
*       Suspend and wait for wakeup signal
*
*   INPUTS
*
*      ??? 
*
*   OUTPUTS
*
*      NU_SUCCESS on success.
*
*   NOTES
*
*      This macro should only be called by the function associated with
*      the helper object.  The function should call this function to suspend (block)
*      itself and wait for a wake up signal. The function should always check 
*      whether the function should exit by calling SD_IS_HELPER_SHUTTING_DOWN.
*          
*      See also: SDLIB_OSCreateHelper , SD_IS_HELPER_SHUTTING_DOWN
*         
*      Example: block on the wake signal
*           while(1) {
*               status = SD_WAIT_FOR_WAKEUP(pHelper);
*               if (!SDIO_SUCCESS(status)) {
*                   break;
*               }
*               if (SD_IS_HELPER_SHUTTING_DOWN(pHelper)) {
*                    .. shutting down
*                   break;
*               }
*           }
*   
*************************************************************************/

#define SD_WAIT_FOR_WAKEUP(p)   SignalWait(&(p)->WakeSignal) 



#define __le16_to_cpu(x) (((x) & 0x00FF << 8) | ((x) & 0xFF00 >> 8))
#define CT_LE16_TO_CPU_ENDIAN(x) __le16_to_cpu(x)
#define CT_LE32_TO_CPU_ENDIAN(x) __le32_to_cpu(x)
#define CT_CPU_ENDIAN_TO_LE16(x) __cpu_to_le16(x)
#define CT_CPU_ENDIAN_TO_LE32(x) __cpu_to_le32(x)

#define CT_CPU_ENDIAN_TO_BE16(x) __cpu_to_be16(x)
#define CT_CPU_ENDIAN_TO_BE32(x) __cpu_to_be32(x)
#define CT_BE16_TO_CPU_ENDIAN(x) __be16_to_cpu(x)
#define CT_BE32_TO_CPU_ENDIAN(x) __be32_to_cpu(x)

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif /* max*/

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif /* min*/
#define UNUSED_PARAM (void)


STATUS sdio_busdriver_init(NU_MEMORY_POOL* mem_pool, int DebugLevel);

void sdio_busdriver_cleanup(void);



#endif /* __CPSYSTEM_LINUX_H___ */

