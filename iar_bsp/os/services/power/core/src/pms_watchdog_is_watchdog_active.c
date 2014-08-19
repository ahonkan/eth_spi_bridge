/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation 
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
*
* FILE NAME
*
*      pms_watchdog_is_watchdog_active.c
*
* COMPONENT
*
*      PMS - Watchdog Services
*
* DESCRIPTION
*
*      This file checks whether a watchdog is active. Any functions 
*      blocked on this call will be released if the element is active.
*
* DATA STRUCTURES
*
*      None
*
* DEPENDENCIES
*
*      power_core.h
*      watchdog.h
*
***********************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/watchdog.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)

extern      NU_MEMORY_POOL System_Memory;

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Is_Watchdog_Active
*
* DESCRIPTION
*

*      This function checks if the watchdog is active. If active,
*      this function un-blocks. Function will be suspended if the
*      watchdog element has expired.
*
* INPUTS
*
*      handle               Watchdog to be checked
*      blocking_flag        If non-zero, the call blocks while
*                           watchdog is expired.
*
* OUTPUTS
*
*      PM_WD_EXPIRED        This indicates watchdog is expired
*      PM_WD_NOT_EXPIRED    This indicates the watchdog is not expired
*      PM_WD_DELETED        This indicates the watchdog has been deleted
*      PM_INVALID_WD_HANDLE This error indicates the provided watchdog
*                           handle is not valid
*      PM_UNEXPECTED_ERROR  This indicates an unexpected error has occurred
*                           (should never happen)
*
*************************************************************************/
STATUS NU_PM_Is_Watchdog_Active(PM_WD_HANDLE handle, UINT8 blocking_flag)
{
    STATUS          pm_status = NU_SUCCESS;
    STATUS          status;
    PM_WD           *temp_handle;
    NU_SEMAPHORE    *semaphore; 
    
    NU_SUPERV_USER_VARIABLES
    
    /* Convert handle into type PM_WD */
    temp_handle = (PM_WD *)handle;
    
    /* Check input parameters */
    if ((temp_handle == NU_NULL) || (temp_handle->wd_id != PM_WATCHDOG_ID))
    {
        pm_status = PM_INVALID_WD_HANDLE;
    }    

    /* First check if the watchdog has been deleted */
    else if (temp_handle->timeout == 0)
    {
        pm_status = PM_WD_DELETED;
    }
    
    /* Now check if the Watchdog is active - Return immediately as it is still active */
    else if (temp_handle->expired_flag == NU_FALSE)
    {
        pm_status = PM_WD_NOT_EXPIRED;
    }
    
    else 
    {
        /* If the call is non-blocking then we don't wish to block until activity */
        if (blocking_flag == 0)
        {
            pm_status = PM_WD_EXPIRED;
        }      
        else
        {
            /* Switch to supervisor mode */
            NU_SUPERVISOR_MODE();
            
            /* Allocate memory for the semaphore control block */
            status = NU_Allocate_Memory(&System_Memory, (VOID**)&semaphore,
                                       (UNSIGNED)(sizeof(NU_SEMAPHORE)), (UNSIGNED)NU_NO_SUSPEND); 

            /* If we successfully allocated memory for the semaphore control block */
            if (status == NU_SUCCESS)
            {
                /* Clear the memory we just allocated */
                (VOID)memset((VOID*)semaphore, 0, sizeof(NU_SEMAPHORE));
    
                /* Create a semaphore that we can use to suspend on */
                status = NU_Create_Semaphore(semaphore, "WDSem", (UNSIGNED)0, (OPTION)NU_FIFO);
                
                /* Ensure the WD has not expired or been deleted in the meantime */
                if (temp_handle->timeout == 0)
                {
                    pm_status = PM_WD_DELETED;
                }  
                
                /* Activity may have occurred in the meantime */
                if (temp_handle->expired_flag == NU_FALSE)
                {
                    pm_status = PM_WD_NOT_EXPIRED;
                }
                  
                if (status == NU_SUCCESS) 
                {
                    /* Save the semaphore pointer in the wd structure */
                    temp_handle->semaphore_ptr = semaphore; 
                  
                    /* Now obtain the semaphore to cause suspension */
                    status = NU_Obtain_Semaphore (semaphore, (UNSIGNED)NU_SUSPEND);
                    
                    /* Check if the WD was deleted or if it expired */
                    if (status == NU_SUCCESS)
                    {
                        if (temp_handle->timeout == 0)
                        {
                            pm_status = PM_WD_DELETED;
                        }
                        
                        if (temp_handle->expired_flag == NU_FALSE)
                        {
                            pm_status = PM_WD_NOT_EXPIRED;
                        }
                    }
                    
                    /* Delete the semaphore */
                    (VOID)NU_Delete_Semaphore(semaphore);
        
                    /* Deallocate the memory */
                    (VOID)NU_Deallocate_Memory(semaphore);
                }
            }
            else
            {
                /* No memory for semaphore */
                pm_status = PM_UNEXPECTED_ERROR;
            }
            
            /* Return to user mode */
            NU_USER_MODE();
        }
    } 
      
    return(pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */


