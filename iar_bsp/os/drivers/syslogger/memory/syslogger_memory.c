/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
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
*       syslogger_memory.c
*
*   DESCRIPTION
*
*       This file contains function that allow a simple but system level
*       logger driver that logs to a memory buffer.
*
*       NOTE:  This component does little to no error checking - it
*              expects the system logger driver to only call the
*              provided services with correct data and at appropriate
*              times (ie error checking is done by system logger driver)
*
*   FUNCTIONS
*
*       SysLogger_Open_Medium
*       SysLogger_Close_Medium
*       SysLogger_Write_Medium
*       SysLogger_Read_Medium
*
***********************************************************************/

/* Include required header files */
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/syslog_extern.h"
#include    "drivers/syslogger_defs.h"

#if (CFG_NU_OS_DRVR_SYSLOGGER_MEDIUM == 1)

/* Local variables */
static CHAR *       SysLogger_Memory_Read_Ptr;
static CHAR *       SysLogger_Memory_Write_Ptr;
static CHAR *       SysLogger_Memory_End_Ptr;
static CHAR *       SysLogger_Memory_Start_Ptr;
static NU_SEMAPHORE SysLogger_Memory_Semaphore;


/*************************************************************************
*
*   FUNCTION
*
*       SysLogger_Open_Medium
*
*   DESCRIPTION
*
*       This function allocates memory for logging
*
*   INPUTS
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS                          Logging medium successfully opened
*       Other                               Logging medium unsuccessfully opened
*
*************************************************************************/
STATUS  SysLogger_Open_Medium(VOID)
{
    STATUS              status;
    NU_MEMORY_POOL *    pool_ptr;
    
    
    /* Get pointer to system memory pool */
    status = NU_System_Memory_Get(&pool_ptr, NU_NULL);

    /* Ensure memory pool pointer retrieved */
    if (status == NU_SUCCESS)
    {
        /* Allocate memory for logging */
        status = NU_Allocate_Memory(pool_ptr, (VOID **)&SysLogger_Memory_Start_Ptr,
                                    CFG_NU_OS_DRVR_SYSLOGGER_MEM_SIZE, NU_NO_SUSPEND);

        /* Set read, write and end pointers */
        SysLogger_Memory_Read_Ptr = SysLogger_Memory_Start_Ptr;
        SysLogger_Memory_Write_Ptr = SysLogger_Memory_Start_Ptr;
        SysLogger_Memory_End_Ptr = SysLogger_Memory_Start_Ptr + CFG_NU_OS_DRVR_SYSLOGGER_MEM_SIZE;
    }

    /* Ensure memory allocated */
    if (status == NU_SUCCESS)
    {
        /* Zeroize allocated memory */
        memset((VOID *)SysLogger_Memory_Start_Ptr, 0, CFG_NU_OS_DRVR_SYSLOGGER_MEM_SIZE);

        /* Create a semaphore to protect read/write operations */
        status = NU_Create_Semaphore(&SysLogger_Memory_Semaphore, "log_mem", 1, NU_FIFO);
    }

    /* Return status to caller */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       SysLogger_Close_Medium
*
*   DESCRIPTION
*
*       This function deallocates the memory used for logging
*
*   INPUTS
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS                          Logging medium successfully closed
*       Other                               Logging medium unsuccessfully closed
*
*************************************************************************/
STATUS  SysLogger_Close_Medium(VOID)
{
    STATUS  status;
    

    /* Deallocate memory */
    status = NU_Deallocate_Memory(SysLogger_Memory_Start_Ptr);

    /* Ensure status was successful */
    if (status == NU_SUCCESS)
    {
        /* Delete the write protection semaphore */
        status = NU_Delete_Semaphore(&SysLogger_Memory_Semaphore);
    }

    /* Return status of close */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       SysLogger_Write_Medium
*
*   DESCRIPTION
*
*       This function logs data to the memory buffer
*
*   INPUTS
*       buf_ptr                             Pointer to buffer to log
*       size                                Size of buffer to log
*
*   OUTPUTS
*
*       NU_SUCCESS                          Logging medium successfully written
*       Other                               Logging medium unsuccessfully written
*
*************************************************************************/
STATUS  SysLogger_Write_Medium(const CHAR * buf_ptr, UINT32 size)
{
    STATUS  status;


    /* Obtain the read/write protection semaphore */
    status = NU_Obtain_Semaphore(&SysLogger_Memory_Semaphore, NU_SUSPEND);

    /* Ensure semaphore obtained */
    if (status == NU_SUCCESS)
    {
        /* Loop through entire buffer */
        while (size)
        {
            /* Write a byte */
            *SysLogger_Memory_Write_Ptr = *buf_ptr;

            /* Move to next byte in buffer */
            buf_ptr++;

            /* Decrement remaining size */
            size--;

            /* Move to next byte in memory */
            SysLogger_Memory_Write_Ptr++;

            /* Check to see if write pointer needs to wrap */
            if (SysLogger_Memory_Write_Ptr == SysLogger_Memory_End_Ptr)
            {
                /* Wrap write pointer */
                SysLogger_Memory_Write_Ptr = SysLogger_Memory_Start_Ptr;
            }
        }

        /* Release the read/write semaphore */
        status = NU_Release_Semaphore(&SysLogger_Memory_Semaphore);

    }
    
    /* Return status of write */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       SysLogger_Read_Medium
*
*   DESCRIPTION
*
*       This function reads data from memory
*
*       NOTE:  Currently not used by system logging service - implemented
*              for testing
*
*   INPUTS
*       buf_ptr                             Pointer to buffer to put read data
*       size                                Size of buffer to read
*       size_read                           Amount read from medium
*
*   OUTPUTS
*
*       NU_SUCCESS                          Logging medium successfully written
*       Other                               Logging medium unsuccessfully written
*
*************************************************************************/
STATUS  SysLogger_Read_Medium(CHAR * buf_ptr, UINT32 size, UINT32 * size_read)
{
    STATUS  status;


    /* Obtain the read/write protection semaphore */
    status = NU_Obtain_Semaphore(&SysLogger_Memory_Semaphore, NU_SUSPEND);

    /* Initialize size read */
    *size_read = 0;

    /* Ensure semaphore obtained */
    if (status == NU_SUCCESS)
    {
        /* Loop through entire buffer or until caught up with memory write pointer */
        while ((size) && (SysLogger_Memory_Read_Ptr != SysLogger_Memory_Write_Ptr))
        {
            /* Read a byte */
            *buf_ptr = *SysLogger_Memory_Read_Ptr;

            /* Move to next byte in buffer */
            buf_ptr++;

            /* Decrement remaining size */
            size--;

            /* Move to next byte in memory */
            SysLogger_Memory_Read_Ptr++;

            /* Increment size read */
            (*size_read)++;

            /* Check to see if read pointer needs to wrap */
            if (SysLogger_Memory_Read_Ptr == SysLogger_Memory_End_Ptr)
            {
                /* Wrap read pointer */
                SysLogger_Memory_Read_Ptr = SysLogger_Memory_Start_Ptr;
            }
        }

        /* Release the read/write semaphore */
        status = NU_Release_Semaphore(&SysLogger_Memory_Semaphore);
    }
    
    /* Return status of write */
    return (status);
}
#endif
