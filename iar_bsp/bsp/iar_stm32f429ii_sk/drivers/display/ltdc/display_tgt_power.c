/*************************************************************************
*
*            Copyright 2013 Mentor Graphics Corporation
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
*       display_tgt_power.c
*
*   COMPONENT
*
*       LTDC - ST LTDC driver
*
*   DESCRIPTION
*
*       This file contains the target specific routines for ST LTDC
*       controller driver.
*
*   DATA STRUCTURES
*
*   FUNCTIONS
*
*       Display_Tgt_Pwr_Set_State
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*       display_tgt.h
*       display_tgt.h
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "bsp/drivers/display/ltdc/display_tgt.h"
#include "bsp/drivers/display/ltdc/display_tgt_power.h"

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_Pwr_Set_State
*
*   DESCRIPTION
*
*       This is the IOCTL function that sets the state of the LCD.
*
*   INPUT
*
*       instance_handle                     Device instance handle
*       state_ptr                           Requested state
*
*   OUTPUT
*
*       NU_SUCCESS                          Successful completion
*       DV_INVALID_INPUT_PARAMS             Requested power state is invalid
*       DISPLAY_PM_ERROR                    Error occurred during power
*                                           management requests
*
*************************************************************************/
STATUS Display_Tgt_Pwr_Set_State(VOID *instance_handle, PM_STATE_ID *state_ptr)
{
    PM_STATE_ID                 state = *state_ptr;
    DISPLAY_INSTANCE_HANDLE     *lcdc_handle = (DISPLAY_INSTANCE_HANDLE *)instance_handle;
    LTDC_TGT                    *tgt_inf = (LTDC_TGT*)(lcdc_handle->display_tgt_ptr);
    PMI_DEV_HANDLE              pmi_dev = lcdc_handle->pmi_dev;
    VOID                        (*setup_fn)(VOID);
    VOID                        (*cleanup_fn)(VOID);
    STATUS                      pm_status = NU_SUCCESS;
    STATUS                      status = NU_SUCCESS;
    UINT8                       min_op_pt;

    /* Check if device is already in use. */
    if (lcdc_handle->device_in_use)
    {
        if ((state != LCD_OFF) && (PMI_STATE_GET(pmi_dev) == LCD_OFF))
        {
            /* Call the target specific setup function. */
            setup_fn = tgt_inf->setup_func;

            if(setup_fn != NU_NULL)
            {
                setup_fn();
            }
        }

        if (state == LCD_BRIGHT)
        {
            Display_Tgt_Contrast_Set(tgt_inf, LCD_CONTRAST_BRIGHT);
        }
        else if (state == LCD_ON)
        {
            Display_Tgt_Contrast_Set(tgt_inf, LCD_CONTRAST_ON);
        }
        else if (state == LCD_DIM)
        {
            Display_Tgt_Contrast_Set(tgt_inf, LCD_CONTRAST_DIM);
        }
        else if (state == LCD_OFF)
        {
            Display_Tgt_Contrast_Set(tgt_inf, LCD_CONTRAST_OFF);
        }
        else
        {
            status = DV_INVALID_INPUT_PARAMS;
        }

        /* Check if device was OFF prior to this request and the requested
           state is not OFF. */
        if ((state != LCD_OFF) && (PMI_STATE_GET(pmi_dev) == LCD_OFF))
        {
        
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

            /* Determine the Min op for the display driver */
            (VOID) Display_Tgt_Min_OP_Pt_Calc (&min_op_pt);

            /* First ensure that DVFS is at min op before DMA is enabled */
            pm_status = PMI_REQUEST_MIN_OP(min_op_pt, pmi_dev);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
        
            if (pm_status == NU_SUCCESS)
            {
                /* Turn the LCD ON. */
                status = Display_Tgt_Set_State_ON(lcdc_handle);
                
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
                /* Check if LCD turned ON successfully. */
                if (status == NU_SUCCESS)
                {
                    /* Update MPL for DVFS */
                    pm_status = PMI_DVFS_Update_MPL_Value(pmi_dev, PM_NOTIFY_ON);
                }
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
            }
        }

        /* Otherwise check if requested state is OFF. */
        else if (state == LCD_OFF)
        {
            /* Turn the LCD OFF. */
            Display_Tgt_Set_State_OFF(lcdc_handle);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
            if (status == NU_SUCCESS)
            {
            /* Release min request for DVFS OP. */
            pm_status = PMI_RELEASE_MIN_OP(pmi_dev);

                if (pm_status == NU_SUCCESS)
                {
                    /* Update MPL for DVFS. */
                    pm_status = PMI_DVFS_Update_MPL_Value(pmi_dev, PM_NOTIFY_OFF);
                }
            }
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

            /* Call the target specific cleanup_func function. */
            cleanup_fn = tgt_inf->cleanup_func;
            
            if(cleanup_fn != NU_NULL)
            {
                cleanup_fn();
            }

        }
    }

    /* Check if requested state was successfully switched. */
    if (pm_status == NU_SUCCESS)
    {
        status = NU_SUCCESS;
        PMI_STATE_SET(pmi_dev, *state_ptr);
    }

    else
    {
        /* No. Indicate that PM operations failed. */
        status = DISPLAY_PM_ERROR;
    }

    /* Return the completion status. */
    return (status);
}

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
/*************************************************************************
*
*   FUNCTION
*
*       Display_Tgt_Min_OP_Pt_Calc
*
*   DESCRIPTION
*
*       This function returns the Min Op pt the driver can work on.
*
*   INPUT
*
*       UINT8  min_op_pt                   Pointer to Min Op pt.
*
*   OUTPUT
*
*       NU_SUCCESS                          Successful.
*
*************************************************************************/
STATUS  Display_Tgt_Min_OP_Pt_Calc (UINT8* min_op_pt)
{
    *min_op_pt = DISPLAY_MIN_DVFS_OP;
    return (NU_SUCCESS);
}

#endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS */
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
