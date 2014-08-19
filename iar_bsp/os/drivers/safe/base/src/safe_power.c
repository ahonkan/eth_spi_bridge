/*************************************************************************/
/*                                                                       */
/*               Copyright 2011 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       safe_power.c
*
* COMPONENT
*
*       SAFE Power Driver
*
* DESCRIPTION
*
*       Provides power functionality to the safe driver
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       SAFE_Set_State
*
*******************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

/*************************************************************************
*
*   FUNCTION
*
*       SAFE_Set_State
*
*   DESCRIPTION
*
*       This is the IOCTL function that sets the state of the SAFE driver
*
*   INPUT
*
*       VOID          *inst_handle          - Instance handle
*       PM_STATE_ID   *state                - Power mode state
*
*   OUTPUT
*
*       STATUS     pm_status             - NU_SUCCESS
*
*************************************************************************/
STATUS SAFE_Set_State(VOID *inst_handle, PM_STATE_ID *state)
{
    STATUS               status = NU_SUCCESS;
    STATUS               pm_status;
    SAFE_INSTANCE_HANDLE *instance_handle = (SAFE_INSTANCE_HANDLE *)inst_handle;
    PMI_DEV_HANDLE       pmi_dev_ptr = instance_handle->pmi_dev;

    /* Enable SAFE only if already OFF */
    if (((*state == SAFE_ON) || (*state == POWER_ON_STATE)) &&
        (PMI_STATE_GET(pmi_dev_ptr) == SAFE_OFF))
    {
        if(instance_handle->device_in_use == NU_TRUE)
        {
            /* Update the state of the device in the State Information structure */
            PMI_STATE_SET(pmi_dev_ptr, SAFE_ON);

            /* Set ON flag in event group */
            PMI_CHANGE_STATE_WAIT_CYCLE(pmi_dev_ptr, PMI_ON_EVT, status);
        }
    }

    /* Disable SAFE only if already ON */
    if ((*state == SAFE_OFF) && ((PMI_STATE_GET(pmi_dev_ptr) == SAFE_ON) ||
        (PMI_STATE_GET(pmi_dev_ptr) == POWER_ON_STATE)))
    {

        /* Update the state of the device in the State Information structure */
        PMI_STATE_SET(pmi_dev_ptr, SAFE_OFF);

        /* Clear if ON event was set */
        PMI_CHANGE_STATE_WAIT_CYCLE(pmi_dev_ptr, PMI_OFF_EVT, status);
    }

    if(status == NU_SUCCESS)
    {
        pm_status = NU_SUCCESS;
    }
    else
    {
        pm_status = PM_UNEXPECTED_ERROR;
    }

    return pm_status;
}
#endif

