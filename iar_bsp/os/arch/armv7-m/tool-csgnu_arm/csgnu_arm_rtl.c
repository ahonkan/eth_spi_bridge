/***********************************************************************
*
*             Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       csgnu_arm_rtl.c
*
*   DESCRIPTION
*
*       This file contains the CSGNU run-time library functionality
*
*   FUNCTIONS
*
*       ESAL_TS_RTE_Initialize
*       ESAL_GE_RTE_Cxx_Region_Objects_Initialize
*       ESAL_TS_RTE_Cxx_System_Objects_Initialize
*       ESAL_TS_RTE_Cxx_Exceptions_Initilize
*
*   DEPENDENCIES
*
*       nucleus.h
*
************************************************************************/

/* Include required header files */
#include            "nucleus.h"
#include            "kernel/nu_kernel.h"
#include            "drivers/nu_drivers.h"

#if (__GNUC__ < 4)
#error ERROR: Unsupported version of CSGNU tools.
#endif /* (__GNUC__ < 4) */

/* External variable declarations */
#if (ESAL_TS_RTL_SUPPORT == NU_TRUE)

#include            <sys/stat.h>
#include            <errno.h>
#include            <stdio.h>
#include            <stdlib.h>
#include            <sys/types.h>
#include            <sys/time.h>
#include            <sys/times.h>

/* C RTL initialization */
extern char _rtl_init_start[];
extern char _rtl_init_end[];

/* Invoke Initializers parameter values */
#define ESAL_TS_RTE_INVOKE_INIT_ALL             NU_NULL

/* Invoke Initializers function prototype */
VOID ESAL_TS_RTE_Invoke_Initializers(VOID *     region_start,
                                     VOID *     region_end,
                                     VOID *     initializers_start,
                                     VOID *     initializers_end);

#include            "nucleus.h"

/* Used for remapping of malloc functions. */
#include            "kernel/rtl_extr.h"

#ifdef CFG_NU_OS_SVCS_CXX_ENABLE

/* C++ Static Object support. */
extern void (*_cxx_ctor_start[])();         /* Symbol taken from linker command file. */
extern void (*_cxx_ctor_end[])();           /* Symbol taken from linker command file. */

VOID *__dso_handle = &__dso_handle;         /* Handle required by tool library functions. */

#endif /* CFG_NU_OS_SVCS_CXX_ENABLE */

#if (CFG_NU_OS_KERN_RTL_FP_OVERRIDE == NU_TRUE)

unsigned char __cslibc$printf_fp_override = 0;
unsigned char __cslibc$scanf_fp_override = 0;

#endif /* CFG_NU_OS_KERN_RTL_FP_OVERRIDE == NU_TRUE */

#endif /* ESAL_TS_RTL_SUPPORT == NU_TRUE */

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_TS_RTE_Initialize
*
*   DESCRIPTION
*
*       This function initializes the run-time environment as required
*       for the given toolset
*
*   CALLED BY
*
*       ESAL_GE_RTE_Initialize
*
*   CALLS
*
*       setvbuf
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    ESAL_TS_RTE_Initialize(VOID)
{
#if (ESAL_TS_RTL_SUPPORT == NU_TRUE)

    /* Determine if C library is initialized (check standard IO state). */
    if (stdout == NULL)
    {
        /* Initialize C library (and standard IO file streams). */
        ESAL_TS_RTE_Invoke_Initializers(ESAL_TS_RTE_INVOKE_INIT_ALL,
                                        ESAL_TS_RTE_INVOKE_INIT_ALL,
                                        _rtl_init_start,
                                        _rtl_init_end);

    }

    /* Configure standard IO file streams to be non-buffered. */
    setvbuf (stdin, NULL, _IONBF, 0);
    setvbuf (stdout, NULL, _IONBF, 0);
    setvbuf (stderr, NULL, _IONBF, 0);

#endif /* ESAL_TS_RTL_SUPPORT == NU_TRUE */

    return;
}

#if (__GNUC__ >= 4)

#ifdef CFG_NU_OS_KERN_RTL_ENABLE
/*************************************************************************
*
*   FUNCTION
*
*       __wrap_malloc
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
void * __wrap_malloc(size_t size)
{
    void *      mem_ptr;

    NU_SUPERV_USER_VARIABLES

    /***** BEGIN SUPERVISOR MODE *****/

    NU_SUPERVISOR_MODE();

    mem_ptr = RTL_malloc(size);

    NU_USER_MODE();

    /****** END SUPERVISOR MODE ******/

    return(mem_ptr);
}

/*************************************************************************
*
*   FUNCTION
*
*       __wrap_calloc
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
void * __wrap_calloc (size_t nmemb, size_t size)
{
    void *      mem_ptr;

    NU_SUPERV_USER_VARIABLES

    /***** BEGIN SUPERVISOR MODE *****/

    NU_SUPERVISOR_MODE();

    mem_ptr = RTL_calloc(nmemb, size);

    NU_USER_MODE();

    /****** END SUPERVISOR MODE ******/

    return(mem_ptr);
}

/*************************************************************************
*
*   FUNCTION
*
*       __wrap_realloc
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
void * __wrap_realloc(void * ptr, size_t size)
{
    void *      mem_ptr;

    NU_SUPERV_USER_VARIABLES

    /***** BEGIN SUPERVISOR MODE *****/

    NU_SUPERVISOR_MODE();

    mem_ptr = RTL_realloc(ptr, size);

    NU_USER_MODE();

    /****** END SUPERVISOR MODE ******/

    return(mem_ptr);
}

/*************************************************************************
*
*   FUNCTION
*
*       __wrap_free
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
void __wrap_free(void * ptr)
{    
    NU_SUPERV_USER_VARIABLES

    /***** BEGIN SUPERVISOR MODE *****/

    NU_SUPERVISOR_MODE();

    RTL_free(ptr);

    NU_USER_MODE();

    /****** END SUPERVISOR MODE ******/

    return;
}

#endif 

/*************************************************************************
*
*   FUNCTION
*
*       ESAL_TS_RTE_Invoke_Initializers
*
*   DESCRIPTION
*
*       Invoke initializer functions in a specific memory region.  If the
*       regions specified is not a region (i.e. start and end are the
*       same) then all initializers will be invoked.
*
*   CALLED BY
*
*       ESAL_TS_RTE_Initialize
*       ESAL_TS_RTE_Cxx_Region_Objects_Initialize
*       ESAL_TS_RTE_Cxx_System_Objects_Initialize
*
*   CALLS
*
*       (Initializer functions)
*
*   INPUTS
*
*       region_start - Starting address of memory region to call
*                      initializers.  If equal to region end address then
*                      all initializers will be invoked.
*
*       region_end - Ending address of memory region to call initializers.
*                    If equal to region start address then all
*                    initializers will be invoked.
*
*       initializers_start - Starting address of the initializer list.
*
*       initializers_end - Ending address of the initializer list.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID ESAL_TS_RTE_Invoke_Initializers(VOID *     region_start,
                                     VOID *     region_end,
                                     VOID *     initializers_start,
                                     VOID *     initializers_end)
{
    INT         cursor;
    INT         quantity;
    UINT *      init_start;

    VOID (*ptr_to_function)();

    /* Get the start address of the initializers list */
    init_start = (UINT *)initializers_start;

    /* Calculate the number of initializers in the list */
    quantity = ((INT)((UINT) initializers_end - (UINT) initializers_start)) / 4;

    for(cursor = 0; cursor < quantity; cursor++)
    {
        /* Get the next initializer function */
        ptr_to_function = (VOID (*)()) (*(init_start + cursor));

        /* Determine if all initializers or only those in a region are
           invoked. */
        if (region_start == region_end)
        {
            /*  Ensure initializer function is not NULL. */
            if(*ptr_to_function != NU_NULL)
            {
                /* The initializer function resides within the region so
                   call it. */
                (*ptr_to_function)();
            }

        }
        else
        {
            /*  Does the address reside in specified memory region and is
                not NULL. */
            if(((UNSIGNED) *ptr_to_function > (UNSIGNED) region_start) &&
               ((UNSIGNED) *ptr_to_function < (UNSIGNED) region_end) &&
               (*ptr_to_function != NU_NULL))
            {
                /* The initializer function resides within the region so
                   call it. */
                (*ptr_to_function)();
            }

        }

    }


    return;
}

    #ifdef CFG_NU_OS_SVCS_CXX_ENABLE

/*************************************************************************
*
*   FUNCTION
*
*       __cxa_pure_virtual
*
*   DESCRIPTION
*
*       Error handler for C++ call of pure virtual function.
*
*   CALLED BY
*
*       (C++ library)
*
*   CALLS
*
*       None
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
void __cxa_pure_virtual(void)
{
    while(1);
}

        #if (CFG_NU_OS_SVCS_CXX_INIT_STATIC_OBJECTS == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       ESAL_TS_RTE_Cxx_Region_Objects_Initialize
*
*   DESCRIPTION
*
*       Initialize C++ Static Objects in a specific memory region.
*
*   CALLED BY
*
*       ESAL_TS_RTE_Cxx_System_Objects_Initialize
*
*   CALLS
*
*       (C++ Static Object Constructors)
*
*   INPUTS
*
*       region_start - Starting address of memory region to call
*                      constructors.
*
*       region_end - Ending address of memory region to call constructors.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID ESAL_TS_RTE_Cxx_Region_Objects_Initialize(VOID *   region_start,
                                               VOID *   region_end)
{
    /* Invoke all initializer functions in the constructors list within
       the specified region of memory. */
    ESAL_TS_RTE_Invoke_Initializers(region_start,
                                    region_end,
                                    _cxx_ctor_start,
                                    _cxx_ctor_end);

    return;
}

/*************************************************************************
*
*   FUNCTION
*
*       ESAL_TS_RTE_Cxx_System_Objects_Initialize
*
*   DESCRIPTION
*
*       Initialize C++ System Objects.
*
*   CALLED BY
*
*       CXX_RTE_Initialize
*
*   CALLS
*
*       ESAL_TS_RTE_Cxx_Region_Objects_Initialize
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
VOID ESAL_TS_RTE_Cxx_System_Objects_Initialize(VOID)
{
    /* Initialize all objects in system memory region (all of memory). */
    ESAL_TS_RTE_Invoke_Initializers(ESAL_TS_RTE_INVOKE_INIT_ALL,
                                    ESAL_TS_RTE_INVOKE_INIT_ALL,
                                    _cxx_ctor_start,
                                    _cxx_ctor_end);

    return;
}

        #endif /* (CFG_NU_OS_SVCS_CXX_INIT_STATIC_OBJECTS == NU_TRUE) */

        #if (CFG_NU_OS_SVCS_CXX_INIT_EXCEPTION_SUPPORT == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       ESAL_TS_RTE_Cxx_Exceptions_Initialize
*
*   DESCRIPTION
*
*       Initialize C++ Exceptions.
*
*   CALLED BY
*
*       CXX_RTE_Initialize
*
*   CALLS
*
*       None
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
VOID ESAL_TS_RTE_Cxx_Exceptions_Initialize(VOID)
{
    /* Nothing to do here. */

    return;
}

        #endif /* (CFG_NU_OS_SVCS_CXX_INIT_EXCEPTION_SUPPORT == NU_TRUE) */

    #endif /* CFG_NU_OS_SVCS_CXX_ENABLE */

#endif /* __GNUC__ */

#if (ESAL_TS_RTL_SUPPORT == NU_TRUE)

/*************************************************************************
*
*  FUNCTION
*
*      abort
*
*  DESCRIPTION
*
*      C RTL abort.  Minimal implementation.
*
*  INPUTS
*
*      None
*
*  OUTPUTS
*
*      None
*
*************************************************************************/
void abort(void)
{
    while(1);
}

/*************************************************************************
*
*   FUNCTION
*
*       _close
*
*   DESCRIPTION
*
*       Close a file.  Minimal implementation
*
*   INPUTS
*
*       file    - Unused
*
*   OUTPUTS
*
*       A constant value of -1.
*
*************************************************************************/
INT _close(INT file)
{
    return(-1);
}

/*************************************************************************
*
*   FUNCTION
*
*       _exit
*
*   DESCRIPTION
*
*       Low level function stub for exit.  Minimal implementation.
*
*   INPUTS
*
*       status  - Unused.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID _exit(INT status)
{
    while(1);
}

/*************************************************************************
*
*   FUNCTION
*
*       _fstat
*
*   DESCRIPTION
*
*       Status of an open file. For consistency with other minimal
*       implementations in these examples, all files are regarded
*       as character special devices.
*
*   INPUTS
*
*       file    - Unused.
*
*       st      - Status structure.
*
*   OUTPUTS
*
*       A constant value of 0.
*
*************************************************************************/
int _fstat(int file, struct stat * st)
{
    st -> st_mode = S_IFCHR;

    return(0);
}

/***********************************************************************
*
* 	FUNCTION
*
*      _system
*
*  	DESCRIPTION
*
*      Nonfunctional implementation.
*
*  	INPUTS
*
*      None
*
*  	OUTPUTS
*
*      0 - Indicates /bin/sh is not available.
*
***********************************************************************/
int _system(char * s)
{
	return(0);
}

/* If Nucleus POSIX is present use functions it provides, otherwise use
   functions defined here. */
#ifndef CFG_NU_OS_SVCS_POSIX_ENABLE

/*************************************************************************
*
*   FUNCTION
*
*       getpid
*
*   DESCRIPTION
*
*       Process-ID; this is sometimes used to generate strings
*       unlikely to conflict with other processes. Minimal
*       implementation, for a system without processes.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       A constant value of 1.
*
*************************************************************************/
INT getpid(VOID)
{
    return(1);
}

/*************************************************************************
*
*   FUNCTION
*
*       kill
*
*   DESCRIPTION
*
*       Send a signal. Minimal implementation
*
*   INPUTS
*
*       pid - Unused
*
*       sig - Unused
*
*   OUTPUTS
*
*       A constant value of -1.
*
*************************************************************************/
INT kill(INT pid, INT sig)
{
    errno = EINVAL;
    return(-1);
}

#endif /* CFG_NU_OS_SVCS_POSIX_ENABLE */

/*************************************************************************
*
*  FUNCTION
*
*      gettimeofday
*
*  DESCRIPTION
*
*      The function returns seconds since the Epoch in the timeval
*      structure.  The function does not support timezone.
*
*  INPUTS
*
*      tp       - Unused
*
*      dummy    - Unused
*
*  OUTPUTS
*
*      A constant value of 0.
*
*************************************************************************/
INT gettimeofday(struct timeval *tp, VOID *tz)
{
    STATUS status = -1;

#ifdef CFG_NU_OS_DRVR_RTC_ENABLE

    struct tm  time_ptr;


    if (tp != NU_NULL)
    {
        /* Clear out time structure. */
        memset(&time_ptr, 0, sizeof(struct tm));

        /* Get the current time from RTC */
        status = NU_Retrieve_RTC_Time(&time_ptr);

        /* Check the completion of last operation */
        if(status == NU_SUCCESS)
        {
            /* Return seconds since epoch (JAN 1st, 1970 00:00:00)*/
            tp->tv_sec = RTL_Calc_Time_Since_Epoch(&time_ptr);

            /* Microseconds not supported */
            tp->tv_usec = 0;
        }
        else
        {
            status = -1;
        }
   	}

#endif /* CFG_NU_OS_DRVR_RTC_ENABLE */

    return(status);
}

/*************************************************************************
*
*  FUNCTION
*
*      _times
*
*  DESCRIPTION
*
*      Returns the number of clock ticks that have elapsed since
*      an arbitrary point in the past.
*
*  INPUTS
*
*      tms       - Pointer to structure.
*
*  OUTPUTS
*
*      A constant value of 0.
*
*************************************************************************/
INT _times(struct tms *tmstruct)
{
    /* The structure tms is defined in sys/time.h as:
        struct tms {
        clock_t tms_utime;       user time
        clock_t tms_stime;       system time
        clock_t tms_cutime;      user time, children
        clock_t tms_cstime;      system time, children
    };
    We return the total ticks as user time. (No distinction between user and system time)   */

    /* CLOCKS_PER_SEC is defined in time.h */
    tmstruct->tms_utime = NU_Retrieve_Clock() * (CLOCKS_PER_SEC/NU_PLUS_TICKS_PER_SEC);
    tmstruct->tms_stime = 0;
    tmstruct->tms_cutime = 0;
    tmstruct->tms_cstime = 0;

    return 0;
}

/*************************************************************************
*
*   FUNCTION
*
*       _isatty
*
*   DESCRIPTION
*
*       Query whether output stream is a terminal. For consistency
*       with the other minimal implementations, which only support
*       output to stdout, this minimal implementation is suggested
*
*   INPUTS
*
*       file    - Unused
*
*   OUTPUTS
*
*       A constant value of 1.
*
*************************************************************************/
int _isatty(int file)
{
    return(1);
}

/*************************************************************************
*
*   FUNCTION
*
*       _lseek
*
*   DESCRIPTION
*
*       Set position in a file. Minimal implementation.
*
*   INPUTS
*
*       file    - Unused
*
*       ptr     - Unused
*
*       dir     - Unused
*
*   OUTPUTS
*
*       A constant value of 0.
*
*************************************************************************/
int _lseek(int file, int ptr, int dir)
{
    return(0);
}

/*************************************************************************
*
*   FUNCTION
*
*       _open
*
*   DESCRIPTION
*
*       Open a file.  Minimal implementation
*
*   INPUTS
*
*       filename    - Unused
*
*       flags       - Unused
*
*       mode        - Unused
*
*   OUTPUTS
*
*       A constant value of 1.
*
*************************************************************************/
INT _open(const char * filename, int flags, int mode)
{
    /* Any number will work. */
    return(1);
}

/*************************************************************************
*
*   FUNCTION
*
*       _read
*
*   DESCRIPTION
*
*       Low level function to redirect IO to serial.
*
*   INPUTS
*
*       fd          - Unused
*
*       buffer      - Buffer where read data will be placed.
*
*       buflen      - Size (in bytes) of buffer.
*
*   OUTPUTS
*
*       A constant value of 1.
*
*************************************************************************/
int _read(int fd, char * buffer, int buflen)
{
    INT i;
    INT c;
    INT status;

    NU_SUPERV_USER_VARIABLES

    /***** BEGIN SUPERVISOR MODE *****/

    NU_SUPERVISOR_MODE();

    /* Check for NULL pointer */
    if (ESAL_GE_RTE_Byte_Read != NU_NULL)
    {
        /* Loop until we receive the required number of characters */
        for (i = 0; i < buflen; i++)
        {
            /* Try and receive one character */
            while ((c = (*ESAL_GE_RTE_Byte_Read)()) == -1)
            {
                /* Loop until we receive a character */
            }

            /* Store the received character in the buffer */
            *buffer++ = c;
        }

        /* Return the number of characters received */
        status = buflen;
    }
    else
    {
        /* Return error */
        status = -1;
    }

    NU_USER_MODE();

    /****** END SUPERVISOR MODE ******/

    return(status);
}


#ifdef CFG_NU_OS_KERN_RTL_ENABLE
/***********************************************************************
*
*   FUNCTION
*
*       _sbrk
*
*   DESCRIPTION
*
*       Low level function to calculate the end of heap
*
*   INPUTS
*
*       int  nbytes                       - Number of bytes needed in heap space
*
*   OUTPUTS
*
*       void *prev_heap_end               - End of heap pointer
*
***********************************************************************/
void * _sbrk (int nbytes)
{
    static void     *heap_end;
    void            *prev_heap_end = (void *)-1; /* Out of heap space */ 
    STATUS          status = NU_SUCCESS;
    NU_MEMORY_POOL  *mem_pool;
    static int      bytes_used;

    NU_SUPERV_USER_VARIABLES

    /***** BEGIN SUPERVISOR MODE *****/

    NU_SUPERVISOR_MODE();

    /* Check if it's first call to _sbrk() */
    if (heap_end == 0)
    {
        /* Get memory pool pointer */
        status = NU_System_Memory_Get(&mem_pool, NU_NULL);

        if (status == NU_SUCCESS)
        {
            /* Allocate memory */
            status = NU_Allocate_Memory(mem_pool, &heap_end, CFG_NU_OS_KERN_RTL_HEAP_SIZE, NU_NO_SUSPEND);
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Validate request */ 
        if (((bytes_used + nbytes) <= CFG_NU_OS_KERN_RTL_HEAP_SIZE) && ((bytes_used + nbytes) >= 0))
        {
            /* Request is in range, grant it */
            prev_heap_end = heap_end;

            /* Increment the heap_end pointer and bytes_used */
            heap_end += nbytes;
            bytes_used += nbytes;
        }
    }

    NU_USER_MODE();

    /****** END SUPERVISOR MODE ******/

    return(prev_heap_end);
}
#endif

/*************************************************************************
*
*   FUNCTION
*
*       _write
*
*   DESCRIPTION
*
*       Low level function to redirect IO to serial.
*
*   INPUTS
*
*       INT file                          - Unused
*       CHAR *ptr                         - String to output
*       INT len                           - Length of the string
*
*   OUTPUTS
*
*       INT len                            - The length of the string
*
*************************************************************************/
int _write (int file, const char * ptr, int len)
{
    int     i;
    int     byte_cnt;

    NU_SUPERV_USER_VARIABLES

    /***** BEGIN SUPERVISOR MODE *****/

    NU_SUPERVISOR_MODE();

    /* Check for NULL pointer */
    if (ESAL_GE_RTE_Byte_Write != NU_NULL)
    {
        /* Loop until we transmit the required number of characters */
        for (i = 0; i < len; i++)
        {
            /* Transmit one character */
            (*ESAL_GE_RTE_Byte_Write)(*ptr++);
        }

        /* Return the number of characters transmitted */
        byte_cnt = len;
    }
    else
    {
        /* Return error */
        byte_cnt = -1;
    }

    NU_USER_MODE();

    /****** END SUPERVISOR MODE ******/
    
    return(byte_cnt);
}

#endif /* ESAL_TS_RTL_SUPPORT == NU_TRUE */

