/*************************************************************************
*
*               Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*     ethernet_tgt_power.c
*
* COMPONENT
*
*     STM32_EMAC Ethernet Device Driver.
*
* DESCRIPTION
*
*     This file contains the source of the STM32_EMAC Ethernet driver.
*
* DATA STRUCTURES
*
*
* FUNCTIONS
*
*     IF_SPI_Tgt_Pwr_Set_State
*
* DEPENDENCIES
*
*     nucleus.h
*     nu_kernel.h
*     externs.h
*     nlog.h
*     mii.h
*     nu_services.h
*     nu_drivers.h
*
*************************************************************************/


/**********************************/
/* INCLUDE FILES                  */
/**********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "networking/externs.h"
#include "networking/nlog.h"
#include "networking/mii.h"
#include "bsp/drivers/ifspi/ifspi_tgt.h"
#include "bsp/drivers/ifspi/ifspi_tgt_power.h"
#include "bsp/drivers/ethernet/stm32_emac/ethernet_tgt_power.h"
#include "bsp/drivers/ethernet/stm32_emac/phy.h"


#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
/*************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Pwr_Default_State
*
*   DESCRIPTION
*
*
*   INPUTS
*
*       VOID         *inst_handle           - Instance handle pointer
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IF_SPI_Tgt_Pwr_Default_State (VOID *inst_handle)
{
    if (inst_handle != NU_NULL)
    {
        /* Call the disable function which will ensure interrupts and
           TX and RX are disabled. */
        IF_SPI_Tgt_Disable( (ETHERNET_INSTANCE_HANDLE*)inst_handle);
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       IF_SPI_Tgt_Pwr_Set_State
*
*   DESCRIPTION
*
*       This is the IOCTL function that sets the state of the STM32_EMAC
*
*   INPUT
*
*       VOID        *inst_handle            - Instance handle
*       PM_STATE_ID *state                  - Power state
*
*   OUTPUT
*
*       STATUS   pm_status               - NU_SUCCESS
*
*************************************************************************/
STATUS IF_SPI_Tgt_Pwr_Set_State(VOID *inst_handle, PM_STATE_ID *state)
{
    STATUS                   pm_status = NU_SUCCESS;
    STATUS                   status = NU_SUCCESS;
    ETHERNET_INSTANCE_HANDLE *lm3s_inst_handle = (ETHERNET_INSTANCE_HANDLE*)inst_handle;
    PMI_DEV_HANDLE           pmi_dev_ptr = (((ETHERNET_INSTANCE_HANDLE*)inst_handle)->pmi_dev);
#if(CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
    UINT8           min_op_id;
#endif

    /* Enable STM32_EMAC only if already OFF */
    if (((*state == ETHERNET_ON) || (*state == POWER_ON_STATE)) &&
        (PMI_STATE_GET(pmi_dev_ptr) == ETHERNET_OFF))
    {
        /* Update the state of the device */
        PMI_STATE_SET(pmi_dev_ptr, ETHERNET_ON);

        if (lm3s_inst_handle->device_in_use == NU_TRUE)
        {
        
#if(CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
            /* Calculate the min op id for this device based on its minimum frequency */
            pm_status = PMI_Min_OP_Pt_Calc(pmi_dev_ptr, &(lm3s_inst_handle->ref_clock[0]), IF_SPI_MIN_FREQUENCY, &min_op_id);

            if (pm_status == NU_SUCCESS)
            {
                if (min_op_id != 0)
                {
                    /* Ensure that DVFS is at min op before the MAC is enabled */
                    pm_status = PMI_REQUEST_MIN_OP(min_op_id, pmi_dev_ptr);
                }
            }            
#endif
        
            if (pm_status == NU_SUCCESS)
            {
                /* Enable RX and TX */
                IF_SPI_Tgt_Enable(lm3s_inst_handle);
            }                          
        }

        /* Now set the write event because the device is enabled */
        PMI_CHANGE_STATE_WAIT_CYCLE(pmi_dev_ptr, PMI_ON_EVT, status);
    }

    /* Disable STM32_EMAC only if already ON */
    if ((*state == ETHERNET_OFF) && (PMI_STATE_GET(pmi_dev_ptr) == ETHERNET_ON))
    {
        /* Reset the events as previous resumes and ON may have incremented count */
        PMI_CHANGE_STATE_WAIT_CYCLE(pmi_dev_ptr, PMI_OFF_EVT, status);

        /* Update the state of the device in the State Information structure */
        PMI_STATE_SET(pmi_dev_ptr, ETHERNET_OFF);

        /* Disable the device */
        IF_SPI_Tgt_Disable(lm3s_inst_handle);

#if(CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
        /* Release min operating point */
        pm_status = PMI_RELEASE_MIN_OP(pmi_dev_ptr);
#endif 

    }

    /* Suppress harmless warnings. */
    NU_UNUSED_PARAM(status);
    
    return (pm_status);
}
#endif
