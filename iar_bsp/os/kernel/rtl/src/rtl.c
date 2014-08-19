/*************************************************************************
*
*             Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       rtl.c
*
*   DESCRIPTION
*
*       This file contains the Nucleus run-time library functionality
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTL_malloc
*       RTL_calloc
*       RTL_realloc
*       RTL_free
*
*   DEPENDENCIES
*
*       string.h
*       nucleus.h
*       nu_kernel.h
*       rtl_extr.h
*       rtl.h
*
*************************************************************************/

#include            <string.h>
#include            <stdlib.h>
#include            "nucleus.h"
#include            "kernel/nu_kernel.h"
#include            "kernel/rtl_extr.h"
#include            "kernel/rtl.h"

/* Nucleus memory pool to use for Nucleus malloc support. */
extern NU_MEMORY_POOL           RTL_MALLOC_POOL;

/*************************************************************************
*
*   FUNCTION
*
*       RTL_malloc
*
*   DESCRIPTION
*
*       Allocates memory (using Nucleus RTL).
*
*   INPUTS
*
*       size - Indicates the size (in bytes) of the requested memory.
*
*   OUTPUTS
*
*      <pointer> - Pointer to allocated memory.
*
*      NU_NULL - Indicates internal error or requested memory not
*                available.
*
*************************************************************************/
void * RTL_malloc (size_t size)
{
    STATUS              status = NU_SUCCESS;
    void *              mem_ptr;
    NU_MEMORY_POOL *    pool_ptr = &RTL_MALLOC_POOL;

    /* Ensure valid allocation size. */
    if (size != 0)
    {

#ifdef CFG_NU_OS_KERN_PROCESS_CORE_ENABLE

        /* Get appropriate memory pool for allocation. */
        status = PROC_Get_Memory_Pool(&pool_ptr);

#endif /* CFG_NU_OS_KERN_PROCESS_CORE_ENABLE */

        if (status == NU_SUCCESS)
        {
            /* Allocate requested memory from Nucleus memory system. */
            if (NU_SUCCESS != NU_Allocate_Memory(pool_ptr,
                                                 &mem_ptr,
                                                 (UNSIGNED)size,
                                                 NU_NO_SUSPEND))
            {
                /* ERROR: Update return pointer. */
                mem_ptr = NULL;
            }

        }
        else
        {
            /* Update return pointer. */
            mem_ptr = NULL;
        }

    }
    else
    {
        /* Update return pointer. */
        mem_ptr = NULL;
    }

    return (mem_ptr);
}

/*************************************************************************
*
*   FUNCTION
*
*       RTL_calloc
*
*   DESCRIPTION
*
*       Allocates zero-initialized memory (using Nucleus RTL).
*
*   INPUTS
*
*       nmemb - Number of objects to allocate.
*
*       size - Indicates the size (in bytes) of an object.
*
*   OUTPUTS
*
*      <pointer> - Pointer to allocated memory.
*
*      NU_NULL - Indicates internal error or requested memory not
*                available.
*
*************************************************************************/
void * RTL_calloc (size_t nmemb, size_t size)
{
    STATUS              status = NU_SUCCESS;
    void *              mem_ptr;
    NU_MEMORY_POOL *    pool_ptr = &RTL_MALLOC_POOL;

    /* Ensure valid allocation size. */
    if ((nmemb * size) != 0)
    {

#ifdef CFG_NU_OS_KERN_PROCESS_CORE_ENABLE

        /* Get appropriate memory pool for allocation. */
        status = PROC_Get_Memory_Pool(&pool_ptr);

#endif /* CFG_NU_OS_KERN_PROCESS_CORE_ENABLE */

        if (status == NU_SUCCESS)
        {
            /* Allocate requested memory from Nucleus memory system. */
            if (NU_SUCCESS == NU_Allocate_Memory(pool_ptr,
                                                 &mem_ptr,
                                                 (UNSIGNED)(nmemb * size),
                                                 NU_NO_SUSPEND))
            {
                /* Zero-initialize memory. */
                (VOID)memset(mem_ptr,
                             0x00,
                             (nmemb * size));
            }
            else
            {
                /* ERROR: Update return pointer. */
                mem_ptr = NULL;
            }

        }
        else
        {
            /* Update return pointer. */
            mem_ptr = NULL;

        }

    }
    else
    {
        /* Update return pointer. */
        mem_ptr = NULL;
    }

    return (mem_ptr);
}

/*************************************************************************
*
*   FUNCTION
*
*       RTL_realloc
*
*   DESCRIPTION
*
*       Re-allocates memory (using Nucleus RTL).
*
*   INPUTS
*
*       ptr - Pointer to the memory to be re-allocated.
*
*       size - Indicates the new size (in bytes) of the requested memory.
*
*   OUTPUTS
*
*      <pointer> - Pointer to allocated memory.
*
*      NU_NULL - Indicates internal error or requested memory not
*                available.
*
*************************************************************************/
void * RTL_realloc (void * ptr, size_t size)
{
    STATUS              status = NU_SUCCESS;
    void *              mem_ptr = ptr;
    NU_MEMORY_POOL *    pool_ptr = &RTL_MALLOC_POOL;

#ifdef CFG_NU_OS_KERN_PROCESS_CORE_ENABLE

    /* Get appropriate memory pool for allocation. */
    status = PROC_Get_Memory_Pool(&pool_ptr);

#endif /* CFG_NU_OS_KERN_PROCESS_CORE_ENABLE */

    if (status == NU_SUCCESS)
    {
        /* Reallocate requested memory from Nucleus memory system. */
        if (NU_SUCCESS != NU_Reallocate_Memory(pool_ptr,
                                               &mem_ptr,
                                               (UNSIGNED)size,
                                               NU_NO_SUSPEND))
        {
            /* ERROR: Update return pointer. */
            mem_ptr = NULL;
        }

    }
    else
    {
        /* Update return pointer. */
        mem_ptr = NULL;
    }

    return (mem_ptr);
}

/*************************************************************************
*
*   FUNCTION
*
*       RTL_free
*
*   DESCRIPTION
*
*       Frees allocated memory (using Nucleus RTL).
*
*   INPUTS
*
*       ptr - Pointer to memory to be deallocated.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
void RTL_free (void * ptr)
{
    /* Call Nucleus API to deallocate memory for valid pointer. */
    if (ptr != NULL)
    {
        (VOID)NU_Deallocate_Memory(ptr);
    }

    return;
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_RTL_Rand_Seed
*
*   DESCRIPTION
*
*       Seeds the RTL rand() number generator
*
*       NOTE:  This function uses methods that are not guaranteed to
*              generate a "good" seed.  For this reason, this function
*              is declared as "weak" so it can be over-ridden by the
*              application or BSP with a "good" seed generation function
*              (ie using device serial # or similar)
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
ESAL_TS_WEAK_DEF(VOID NU_RTL_Rand_Seed(VOID))
{
    static BOOLEAN      seeding_complete = NU_FALSE;
    NU_MEMORY_POOL *    sys_pool;
    UINT32 *            mem_ptr;
    UINT                seed;
    INT                 i;
    ESAL_AR_INT_CONTROL_VARS


    /* Disable interrupts during critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* See if seeding has not yet completed */
    if (seeding_complete == NU_FALSE)
    {
        /* Number of UINT32s to allocate from memory pool */
        #define NUM_UINT32S     64
        
        /* Remove compiler warnings - we want seed to be uninitialized */
        NU_UNUSED_PARAM(seed);

        /* Get pointer to system memory pool */
        NU_System_Memory_Get(&sys_pool, NU_NULL);

        /* Try to allocate a small piece of memory to help
           generate a more unique seed */
        if (NU_Allocate_Memory(sys_pool, (VOID **)&mem_ptr, 
                               sizeof(UINT32) * NUM_UINT32S, 
                               NU_NO_SUSPEND) == NU_SUCCESS)
        {
            /* Loop through all allocated UINT32s */
            for (i=0;i<NUM_UINT32S;i++)
            {
                /* Add values in allocated memory to seed */
                seed += (UINT)(*(mem_ptr + i));
            }
            
            /* Deallocate memory */
            NU_Deallocate_Memory(mem_ptr);
        }
    
        /* XOR current seed with number derived from hardware timer count */
        seed ^= ((ESAL_GE_TMR_OS_COUNT_READ() << 16) | ESAL_GE_TMR_OS_COUNT_READ());

        /* Seed random number generator */
        srand(seed);

        /* Set flag showing seeding complete */
        seeding_complete = NU_TRUE;
        
        /* Undefine number of UINT32s allocated from memory pool */
        #undef NUM_UINT32S
    }

    /* Restore interrupts to original state */
    ESAL_GE_INT_ALL_RESTORE();
}

/*************************************************************************
*
*   FUNCTION
*
*       RTL_Calc_Time_Since_Epoch
*
*   DESCRIPTION
*
*       This function calculates the seconds elapsed since epoch.
*
*   INPUTS
*
*       *time_ptr        - Pointer to time structure
*
*   OUTPUTS
*
*       Number of seconds elapsed since epoch.
*
*************************************************************************/
UINT64 RTL_Calc_Time_Since_Epoch(struct tm *time_ptr)
{
    UINT64 year_secs;
    UINT32 days_secs;
    UINT32 today_secs;
    UINT32 leap_secs;

    /* The following calculations evaluate the number of seconds elapsed since January 1st 1970 00:00:00 (epoch). */

    /* First we calculate the number of seconds elapsed in years since epoch */
    year_secs = (time_ptr->tm_year - RTL_EPOCH_YEAR) * RTL_SEC_PER_YEAR;

    /* Now calculate the total seconds elapsed since January 1st this year (not including today). */
    days_secs = time_ptr->tm_yday * RTL_SEC_PER_DAY;

    /* Now calculate the number of seconds elapsed today */
    today_secs = ((time_ptr->tm_hour * RTL_SEC_PER_HOUR) + (time_ptr->tm_min * RTL_SEC_PER_MIN) + (time_ptr->tm_sec));

    /* Now calculate the seconds elapsed in leap days */
    leap_secs = ((UINT32)(time_ptr->tm_year - RTL_EPOCH_YEAR)/4) * RTL_SEC_PER_DAY;

    /* The total seconds elapsed since epoch is the sum of all the seconds */
    return (year_secs + days_secs + today_secs + leap_secs);
}

