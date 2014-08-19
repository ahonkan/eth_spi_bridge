/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       touchpanel_tgt_power.c
*
*   COMPONENT
*
*       STMPE811 - STMPE811 touch panel controller driver
*
*   DESCRIPTION
*
*       This file contains the generic routines of STMPE811 touch panel
*       controller driver.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       Touchpanel_Tgt_Pwr_Set_State
*       Touchpanel_Tgt_Pwr_Notify_Park1
*       Touchpanel_Tgt_Pwr_Notify_Park2
*       Touchpanel_Tgt_Pwr_Notify_Resume1
*       Touchpanel_Tgt_Pwr_Notify_Resume2
*       Touchpanel_Tgt_Pwr_Resume_Complete
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*       nu_connectivity.h
*       touchpanel_tgt.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "bsp/drivers/touchpanel/stmpe811/touchpanel_tgt.h"
#include "connectivity/nu_connectivity.h"

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Pwr_Set_State
*
*   DESCRIPTION
*
*       This is the IOCTL function that sets the state of the touchpanel.
*
*   INPUT
*
*       instance_handle                     Instance handle
        state                               New state to set
*
*   OUTPUT
*
*       PM_UNEXPECTED_ERROR                 Error occurred while accessing
*                                           registry
*       NU_SUCCESS                          Service completed successfully
*
*************************************************************************/
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
STATUS Touchpanel_Tgt_Pwr_Set_State(VOID *instance_handle, PM_STATE_ID *state)
{
    STATUS                        status;
    STATUS                        pm_status = NU_SUCCESS;
    DV_DEV_ID                     i2c_device_id;
    INT                           dev_count = 1;
    TOUCHPANEL_INSTANCE_HANDLE    *inst_handle = (TOUCHPANEL_INSTANCE_HANDLE*)instance_handle;
    TOUCHPANEL_TGT                *tgt_handle = (TOUCHPANEL_TGT *) inst_handle->touchpanel_reserved;
    VOID                          (*setup_fn)(VOID) = NU_NULL;            
    VOID                          (*cleanup_fn)(VOID) = NU_NULL;
    DV_DEV_LABEL                  label = {I2C_LABEL}; 

    /* Enable touchpanel only if already OFF */
    if (((*state == TOUCHPANEL_ON) || (*state == POWER_ON_STATE)) &&
            (PMI_STATE_GET(inst_handle->pmi_dev) == TOUCHPANEL_OFF))
    {
        /* Get the I2C device ID */
        status = DVC_Dev_ID_Get(&label, 1, &i2c_device_id, &dev_count);

        if ((status == NU_SUCCESS) && (dev_count > 0))
        {
            /* Initialize I2C driver */
            if (pm_status == NU_SUCCESS)
            {
                if (tgt_handle->initialized)
                {
                    /* Call setup function if available. */
                    setup_fn = tgt_handle->setup_func;

                    if(setup_fn != NU_NULL)
                    {
                        /* Call the setup function */
                        setup_fn();
                    }

                    /* Check to see if previous operation was successful */
                    /* Enable the touchpanel interrupt */
                    Touchpanel_Tgt_Enable();
                }

                /* Update the state to ON */
                PMI_STATE_SET(inst_handle->pmi_dev, TOUCHPANEL_ON);
            }
        }
        else
        {
            pm_status = PM_UNEXPECTED_ERROR;
        }
    }

    /* Disable touchpanel only if already ON */
    if ((*state ==  TOUCHPANEL_OFF) && (PMI_STATE_GET(inst_handle->pmi_dev) ==  TOUCHPANEL_ON))
    {
        /* Get the I2C device ID */
        status = DVC_Dev_ID_Get(&label, 1, &i2c_device_id, &dev_count);

        if ((status == NU_SUCCESS) && (dev_count > 0))
        {
            /* Disable touchpanel */
            Touchpanel_Tgt_Disable();

            /* Set current state to OFF */
            PMI_STATE_SET(inst_handle->pmi_dev, TOUCHPANEL_OFF);

            /* Call the target specific cleanup_func function. */
            cleanup_fn = tgt_handle->cleanup_func;

            if(cleanup_fn != NU_NULL)
            {
                cleanup_fn();
            }
        }
        else
        {
            pm_status = PM_UNEXPECTED_ERROR;
        }
    }

    return (pm_status);
}

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Pwr_Notify_Park1
*
*   DESCRIPTION
*
*       This function will notify the device when to park or to resume
*
*   INPUT
*
*       VOID            *instance_handle    - Instance handle
*
*   OUTPUT
*
*                                           - NU_SUCCESS always 
*
*************************************************************************/
STATUS Touchpanel_Tgt_Pwr_Notify_Park1(VOID *instance_handle)
{
    /* Suppress warning. */
    NU_UNUSED_PARAM(instance_handle);

    /* Return success */
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Pwr_Notify_Park2
*
*   DESCRIPTION
*
*       This function will notify the device when to park or to resume
*
*   INPUT
*
*       VOID             *instance_handle   - Instance handle
*
*   OUTPUT
*
*                                           - NU_SUCCESS always 
*
*************************************************************************/
STATUS Touchpanel_Tgt_Pwr_Notify_Park2(VOID *instance_handle)
{
    /* Suppress warning. */
    NU_UNUSED_PARAM(instance_handle);

    /* Return success */
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Pwr_Notify_Resume1
*
*   DESCRIPTION
*
*       This function will notify the device when to park or to resume
*
*   INPUT
*
*       VOID             *instance_handle   - Instance handle
*
*   OUTPUT
*
*                                           - NU_SUCCESS always 
*
*************************************************************************/
STATUS Touchpanel_Tgt_Pwr_Notify_Resume1(VOID *instance_handle)
{
    /* Suppress warning. */
    NU_UNUSED_PARAM(instance_handle);

    /* Return success */
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Pwr_Notify_Resume2
*
*   DESCRIPTION
*
*       This function will notify the device when to park or to resume
*
*   INPUT
*
*       VOID             *inst_handle       - Instance handle
*
*   OUTPUT
*
*                                           - NU_SUCCESS always 
*
*************************************************************************/
STATUS Touchpanel_Tgt_Pwr_Notify_Resume2(VOID *inst_handle)
{
    /* Suppress warning. */
    NU_UNUSED_PARAM(inst_handle);

    /* Return success*/
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       Touchpanel_Tgt_Pwr_Resume_Complete
*
*   DESCRIPTION
*
*       This function will notify the device when resume complete
*
*   INPUT
*
*       VOID             *instance_handle       - Instance handle
*
*   OUTPUT
*
*                                               - NU_SUCCESS always 
*
*************************************************************************/
STATUS Touchpanel_Tgt_Pwr_Resume_Complete(VOID *instance_handle)
{
    /* Suppress warning. */
    NU_UNUSED_PARAM(instance_handle);

    /* Return success*/
    return (NU_SUCCESS);
}
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

#endif
