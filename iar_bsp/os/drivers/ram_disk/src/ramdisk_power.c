/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       ramdisk_power.c
*
* COMPONENT
*
*       RAM Disk Driver
*
* DESCRIPTION
*
*       Provides a configurable ram drive capability using pages allocated
*       from a Nucleus memory partition.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       RD_Set_State                         Registers device with DM
*
*******************************************************************/

#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "services/nu_services.h"
#include        "storage/nu_storage.h"
#include        "drivers/nu_drivers.h"


#ifdef CFG_NU_OS_SVCS_PWR_ENABLE


STATUS RD_Pwr_Set_State  (VOID *inst_handle, PM_STATE_ID *state);


/*************************************************************************
*
*   FUNCTION
*
*       RD_Pwr_Set_State
*
*   DESCRIPTION
*
*       This is the IOCTL function that sets the state of the RD
*
*   INPUT
*
*       VOID        *inst_handle            - Instance handle
*       PM_STATE_ID *state                  - Power state
*
*   OUTPUT
*
*       STATUS   pm_status               - NU_SUCCESS
*                                           - PM_INVALID_PARAMETER
*
*************************************************************************/
STATUS RD_Pwr_Set_State(VOID *inst_handle_void, PM_STATE_ID *state)
{ 
    STATUS              pm_status = NU_SUCCESS;
    STATUS              status = NU_SUCCESS;
    RD_INSTANCE_HANDLE  *inst_handle = ((RD_INSTANCE_HANDLE*)inst_handle_void);
    PMI_DEV_HANDLE      pmi_dev_ptr;
    
    /* Suppress warnings */
    NU_UNUSED_PARAM(status);

    /* Check input parameters */
    if ((inst_handle == NU_NULL) || (state == NU_NULL))
    {
        pm_status = PM_INVALID_PARAMETER;
    }
    else
    {
        /* Initialize pmi device pointer */
        pmi_dev_ptr = inst_handle->pmi_dev;

        /* Enable RD only if already OFF */
        if (((*state == RD_ON) || (*state == POWER_ON_STATE)) && (PMI_STATE_GET(pmi_dev_ptr) == RD_OFF))
        {          
            if (inst_handle->device_in_use == NU_TRUE)
            {       
                /* Update the state of the device in the State Information structure */
                PMI_STATE_SET(pmi_dev_ptr, RD_ON);

                 /* Now set the write event because the device is enabled */
                PMI_CHANGE_STATE_WAIT_CYCLE(pmi_dev_ptr, RD_POWER_EVENT_MASK, status);
                            
            }
            else
            {
                /* Device is not in use yet, so just change the state of the device */
                PMI_STATE_SET(pmi_dev_ptr, RD_ON);
            }       
        }
        
        /* Disable RD only if already ON */
        if ((*state == RD_OFF) && ((PMI_STATE_GET(pmi_dev_ptr) == RD_ON) || (PMI_STATE_GET(pmi_dev_ptr) == POWER_ON_STATE)))
        {    
            /* Reset the events as previous resumes and ON may have incrmented count */
            PMI_CHANGE_STATE_WAIT_CYCLE(pmi_dev_ptr, ~RD_POWER_EVENT_MASK, status);
                
            /* Update the state of the device in the State Information structure */
            PMI_STATE_SET(pmi_dev_ptr, RD_OFF);
        }
    }

    return (pm_status);
}

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
