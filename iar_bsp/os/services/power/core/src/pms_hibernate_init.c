/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
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
*       pms_hibernate_init.c
*
*   COMPONENT
*
*       Hibernate
*
*   DESCRIPTION
*
*       Contains all functionality for hibernate initialization (performed
*       at OS initialization).
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PMS_Hibernate_Resume
*       PMS_Hibernate_Initialize
*       PMS_Hibernate_Check
*
*   DEPENDENCIES
*
*       <stdio.h>
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*       hibernate.h
*       nvm.h
*       error_management.h
*
*************************************************************************/

#include  <stdio.h>
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "os/services/power/core/inc/hibernate.h"
#include "os/services/power/core/inc/nvm.h"
#include "os/kernel/plus/supplement/inc/error_management.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

extern UINT32                   PMS_Hibernate_Self_Refresh;

/* Hibernate Resume CB - The hibernate resume control
   block variable used during hibernate initialization.
   (initialiazed during check for resume). */
static HB_RESUME_CB             pms_hb_resume_cb;

/* Hibernate Resume Stack - Stack used for the hibernate resume process
   to ensure that there are no issues when memory is restored. */
static UINT8                    pms_resume_stack[HB_RESUME_STACK_SIZE];

/*************************************************************************
*
*   FUNCTION
*
*       PMS_Hibernate_Resume
*
*   DESCRIPTION
*
*       This function performs a hibernate resume operation and then
*       passes execution to the hibernate system.
*
*       NOTE: This function does not return!
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       None
*
*************************************************************************/
static VOID PMS_Hibernate_Resume(VOID)
{
    STATUS          status;
    HB_RESUME_CB    hb_resume_cb;
    UINT            item_size = 0;
    UINT            i;
    
    /* Avoid warnings when NVM not available */
    NU_UNUSED_PARAM(item_size);

    /* Check for hibernate level that utilizes self refresh mode */
    if (PMS_Hibernate_Self_Refresh == PM_SELF_REFRESH_ON)
    {
        /* No restoration needed, simply exit standby mode */
        PMS_Hibernate_Exit();
    }
    else
    {
        /* Initialize the local (stack) variable to the values previously
           saved during hibernate check to avoid stomping during memory
           restoration */
        for (i = 0; i < sizeof(HB_RESUME_CB); i++)
        {
            ((UINT8 *)&hb_resume_cb)[i] = ((UINT8 *)&pms_hb_resume_cb)[i];
        }
    
        /* Flush all cached values to memory at this time to ensure that
           local (stack) variables are updated before memory restore and
           following invalidation of cache. */
        ESAL_GE_MEM_DCACHE_ALL_FLUSH_INVAL();

        /* Restore all remaining memory items from NVM. */
        i = 0;
        while ((NU_SUCCESS == status) &&
               (i < HB_MAX_REGION_COUNT))
        {
            /* Check if there is an item to restore... */
            if (hb_resume_cb.restore_addr[i] != NU_NULL)
            {
                /* Restore item from NVM. */
                status = PM_NVM_READ(hb_resume_cb.restore_addr[i],
                                     &item_size, (i + 1));
            }
    
            /* Move to next item. */
            i++;
        }
    
        if (NU_SUCCESS == status)
        {
            /* Close NVM */
            status = PM_NVM_CLOSE();
        }
    
        /* Invalidate cache to make sure that newly restored memory values
           are used by the CPU going forward. */
        ESAL_GE_MEM_DCACHE_ALL_INVALIDATE();

        if (NU_SUCCESS == status)
        {
            /* NOTE: Execution goes to the hibernate exit function and does
               not return here! */
    
            /* Call hibernate exit function. */
            hb_resume_cb.hb_exit_func();
        }
        else
        {
            /* Call error-handling routine. */
            ERC_System_Error(status);
        }
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_Hibernate_Initialize
*
*   DESCRIPTION
*
*       This function performs restoration of memory from Non-Volatile 
*       Memory (NVM).  The system is set to a new stack that will not
*       be overwritten by the previously saved memory as it is restored
*       from NVM.
*
*       NOTE: This function never returns.
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID PMS_Hibernate_Initialize(VOID)
{
    /* Switch to hibernate resume stack. */
    ESAL_TS_RTE_SP_WRITE((VOID *)ESAL_GE_STK_ALIGN(&pms_resume_stack[HB_RESUME_STACK_SIZE]));

    /* Start execution on new stack. */
    PMS_Hibernate_Resume();
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_Hibernate_Check
*
*   DESCRIPTION
*
*       This function checks if a hibernate resume is needed.
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       NU_TRUE             System needs to resume from hibernate state
*       NU_FALSE            System needs to boot normally
*
*************************************************************************/
BOOLEAN PMS_Hibernate_Check(VOID)
{
    STATUS              status;
    UINT                item_size = 0;
    UINT                i;
    BOOLEAN             hib_resume = NU_FALSE;
    
    /* Avoid warnings when NVM not available */
    NU_UNUSED_PARAM(item_size);

    /* Check for self refresh hibernation */
    if (PMS_Hibernate_Self_Refresh == PM_SELF_REFRESH_ON)
    {
        /* There is a pending resume. */
        hib_resume = NU_TRUE;
    }
    else
    {
        /* Now check for dormant level */
        
        /* Open NVM storage. */
        status = PM_NVM_OPEN();
    
        /* NOTE: If the NVM could not be opened then there may not be NVM
                 available on the target.  In this case, execution will
                 fall through and this function will return. */
    
        if (NU_SUCCESS == status)
        {
            /* Get hibernate resume control block (first item in NVM).
               
               NOTE: This is saved globally since the resume control
               block will be reset */
            status = PM_NVM_READ(&pms_hb_resume_cb, &item_size, 0);
        }
    
        if (NU_SUCCESS == status)
        {
            /* Verify NVM item is hibernate resume control block. */
            if (item_size != sizeof(HB_RESUME_CB))
            {
                /* Invalid hibernate resume control block found.  This may
                   indicate that the NVM is needs to be initialized for
                   hibernate operation. */
    
                /* Reset NVM write process. */
                status = PM_NVM_RESET();
    
                if (NU_SUCCESS == status)
                {
                    /* A full reset of the resume control block
                       is done here in case any errors are found.
                       This will allow a fresh boot of the system
                       should the sytem be reset */

                    /* Reset Hibernate Resume control block. */
                    pms_hb_resume_cb.resume_pending = NU_FALSE;
                    pms_hb_resume_cb.hb_exit_func = NU_NULL;
                    for (i = 0; i < HB_MAX_REGION_COUNT; i++)
                    {
                        pms_hb_resume_cb.restore_addr[i] = NU_NULL;
                    }
    
                    /* Store hibernate resume control block as first item. */
                    status = PM_NVM_WRITE(&pms_hb_resume_cb, sizeof(HB_RESUME_CB),
                                          &i, NU_FALSE);
                }
    
                if (NU_SUCCESS == status)
                {
                    /* Close NVM */
                    status = PM_NVM_CLOSE();
                }
            }
            else
            {
                /* Valid hibernate resume control block found. */
    
                /* Determine if there is a pending hibernate resume... */
                if (pms_hb_resume_cb.resume_pending == NU_TRUE)
                {
                    /* There is a pending resume. */
                    hib_resume = NU_TRUE;
                }
            }
        }
    }

    return (hib_resume);
}

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE) */


