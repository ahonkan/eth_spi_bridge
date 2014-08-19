/*************************************************************************
*
*               Copyright 2011 Mentor Graphics Corporation
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
*       serial_tgt_power.c
*
*   COMPONENT
*
*       STM32_USART                           - STM32_USART controller driver
*
*   DESCRIPTION
*
*       This file contains the Serial Driver specific functions.
*
*   FUNCTIONS
*
*       Serial_Tgt_Pwr_Default_State
*       Serial_Tgt_Pwr_Set_State
*       Serial_Tgt_Pwr_Min_OP_Pt_Calc
*       Serial_Tgt_Pwr_Notify_Park1
*       Serial_Tgt_Pwr_Notify_Park2
*       Serial_Tgt_Pwr_Notify_Resume1
*       Serial_Tgt_Pwr_Notify_Resume2
*       Serial_Tgt_Pwr_Notify_Resume3
*
*   DEPENDENCIES
*
*       string.h
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include <string.h>
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "bsp/drivers/serial/stm32_usart/serial_tgt.h"
#include "bsp/drivers/serial/stm32_usart/serial_tgt_power.h"

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Pwr_Default_State
*
*   DESCRIPTION
*
*       This function places STM32_USART in known, low-power state
*
*   INPUTS
*
*       SERIAL_INSTANCE_HANDLE  *inst_handle - Device instance handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Serial_Tgt_Pwr_Default_State (SERIAL_INSTANCE_HANDLE *inst_handle)
{

}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Pwr_Set_State
*
*   DESCRIPTION
*
*       This is the IOCTL function that sets the state of the STM32_USART
*
*   INPUT
*
*       SERIAL_INSTANCE_HANDLE *inst_handle - Instance handle
*       PM_STATE_ID           *state        - Power state
*
*   OUTPUT
*
*       STATUS      pm_status               - NU_SUCCESS
*                                           - NU_PM_Request_Min_OP
*
*************************************************************************/
STATUS    Serial_Tgt_Pwr_Set_State(VOID *instance_handle, PM_STATE_ID *state)
{
    INT       int_level;
    STATUS    pm_status = NU_SUCCESS;
    STATUS    status = NU_SUCCESS;
    SERIAL_INSTANCE_HANDLE* inst_handle = (SERIAL_INSTANCE_HANDLE*)instance_handle;
    UINT32          base_addr = SERIAL_BASE_FROM_I_HANDLE(inst_handle);
    PMI_DEV_HANDLE  pmi_dev = inst_handle->pmi_dev;
    UINT32          reg_val;

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

    UINT8     op_pt;

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


    /* Enable STM32_USART only if already OFF */
    if (((*state == SERIAL_ON) || (*state == POWER_ON_STATE)) && (PMI_STATE_GET(pmi_dev) == SERIAL_OFF))
    {
        if ((inst_handle->device_in_use == NU_TRUE) && (PMI_IS_PARKED(pmi_dev) == NU_FALSE))
        {

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

            /* Determine the min op pt for the device's baud rate */
            status = Serial_Tgt_Pwr_Min_OP_Pt_Calc(inst_handle, (inst_handle->attrs.baud_rate), &op_pt);

            if(status == NU_SUCCESS)
            {
                /* First ensure that DVFS is at min op before STM32_USART is enabled */
                pm_status = PMI_REQUEST_MIN_OP(op_pt, pmi_dev);
            }

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

                if ((pm_status == NU_SUCCESS) && (status == NU_SUCCESS))
                {
                    /* Disable interrupts for critical section */
                    int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

                    /* Configure and enable baud rate */
                    Serial_Tgt_Setup (inst_handle, &(inst_handle->attrs));

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

                    /* Update MPL for STM32_USART */
                    (VOID)PMI_DVFS_Update_MPL_Value(pmi_dev, PM_NOTIFY_ON);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */

                    /* Enable Rx and TX */
                    Serial_Tgt_Enable(inst_handle);

                    if(inst_handle->attrs.rx_mode == USE_IRQ)
                    {
                        /* Enable RX interrupt on controller. */
                        Serial_Tgt_Rx_Int_Enable(inst_handle);
                    }

                    /* Restore interrupts to original level. */
                    (VOID)NU_Local_Control_Interrupts(int_level);
            }
        }

        /* Update the state of the device */
        PMI_STATE_SET(pmi_dev, SERIAL_ON);

        /* Set ON flag in event group */
        PMI_CHANGE_STATE_WAIT_CYCLE(pmi_dev, PMI_ON_EVT, status);
    }

    /* Disable STM32_USART only if already ON */
    if ((*state == SERIAL_OFF) && (PMI_STATE_GET(pmi_dev) == SERIAL_ON))
    {
        /* Disable interrupts */
        reg_val = ESAL_GE_MEM_READ32 (base_addr + STM32_USART_CR1);
        reg_val &= ~STM32_USART_ALL_INTERRUPT_MASK;
        ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_CR1), reg_val);

        /* Update the state of the device in the State Information structure */
        PMI_STATE_SET(pmi_dev, SERIAL_OFF);

        /* Clear if ON event was set */
        PMI_CHANGE_STATE_WAIT_CYCLE(pmi_dev, PMI_OFF_EVT, status);

        /* Wait if transmitter is enabled and transmitter is busy */
        while ((inst_handle->tx_en_shadow == 1) && Serial_Tgt_Tx_Busy(inst_handle));

        /* Disable the STM32_USART */
        Serial_Tgt_Disable(inst_handle);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

        /* Update MPL for DVFS */
        (VOID)PMI_DVFS_Update_MPL_Value(pmi_dev, PM_NOTIFY_OFF);

        /* Release min request for DVFS OP */
        (VOID)PMI_RELEASE_MIN_OP(pmi_dev);

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */
    }

    return (pm_status);
}

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE))

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Pwr_Min_OP_Pt_Calc
*
*   DESCRIPTION
*
*       Computes the minimum DVFS operating point required for
*       proper operation at the given baud-rate
*
*   INPUTS
*
*       SERIAL_INSTANCE_HANDLE *inst_handle  - USART instance handle pointer
*       UINT32  baud_rate                   - Baud rate of uart device
*       UINT8*  min_op_pt                   - Min Op Pt returned here
*
*   OUTPUTS
*
*       STATUS  status                      - NU_SUCCESS if min op pt found
*                                           - USART_ERROR if min op pt not found
*
*************************************************************************/
STATUS Serial_Tgt_Pwr_Min_OP_Pt_Calc (SERIAL_INSTANCE_HANDLE *inst_handle, UINT32 baud_rate, UINT8* min_op_pt)
{
    STATUS    pm_status = NU_SUCCESS;
    STATUS    status = SERIAL_ERROR;
    UINT8     max_op_pt_cnt, highest_op_pt;
    INT       curr_op_pt;
    UINT32    ref_freq;
    UINT32    usartdiv_man;
    UINT32    usartdiv_fra;
    UINT32    temp1;
    UINT32    temp2;
    UINT32    temp3;


    /* Get the maximum number of operating points */
    pm_status = NU_PM_Get_OP_Count (&max_op_pt_cnt);

    if(pm_status == NU_SUCCESS)
    {
        /* The highest op pt is one less than the maximum number of op pts */
        highest_op_pt = max_op_pt_cnt - 1;

        /* Loop from highest op pt to lowest */
        for(curr_op_pt=highest_op_pt; curr_op_pt >= 0; curr_op_pt--)
        {
            /* Get reference clock */
            pm_status = NU_PM_Get_OP_Specific_Info(curr_op_pt, inst_handle->serial_ref_clock,
                                                   &ref_freq, sizeof(ref_freq));

            if (pm_status == NU_SUCCESS)
            {
                /* Calculate Integer part. */
                temp1 = ((25 * ref_freq) / (4 * baud_rate));
                usartdiv_man = ((temp1 / 100) << 4);

                /* Calculate Fractional part. */
                temp2 = temp1 - ((usartdiv_man >> 4) * 100);
                usartdiv_fra = ((((temp2 * 16) + 50) / 100) & 0xF);

                /* Calculate baudrate divisor. */
                temp3 = (usartdiv_man | usartdiv_fra);

                /* If usartdiv valid. */
                if ((temp3 >> 16) == 0)
                {
                    *min_op_pt = curr_op_pt;
                    status = NU_SUCCESS;
                }
            }
        }
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Pwr_Notify_Park1
*
*   DESCRIPTION
*
*       This function will notify the STM32_USART when to park or to resume
*
*   INPUT
*
*       VOID                *inst_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS           status             - SUCCESS
*
*************************************************************************/
STATUS Serial_Tgt_Pwr_Notify_Park1(VOID *instance_handle)
{
    UINT32          base_addr = SERIAL_BASE_FROM_I_HANDLE(instance_handle);
    UINT32          reg_val;


    /* Disable interrupts */
    reg_val = ESAL_GE_MEM_READ32 (base_addr + STM32_USART_CR1);
    reg_val &= ~STM32_USART_ALL_INTERRUPT_MASK;
    ESAL_GE_MEM_WRITE32 ((base_addr + STM32_USART_CR1), reg_val);

    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Pwr_Notify_Park2
*
*   DESCRIPTION
*
*       This function will notify the STM32_USART when to park or to resume
*
*   INPUT
*
*       VOID                *inst_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS           status             - SUCCESS
*
*************************************************************************/
STATUS Serial_Tgt_Pwr_Notify_Park2(VOID *instance_handle)
{
    SERIAL_INSTANCE_HANDLE *inst_ptr = (SERIAL_INSTANCE_HANDLE*)instance_handle;


    /* Wait if transmitter is enabled and transmitter is busy */
    while ((inst_ptr->tx_en_shadow == 1) && Serial_Tgt_Tx_Busy(inst_ptr));

    /* Disable Tx and Rx */
    Serial_Tgt_Disable(inst_ptr);

    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Pwr_Notify_Resume1
*
*   DESCRIPTION
*
*       This function will notify the STM32_USART when to park or to resume
*
*   INPUT
*
*       VOID                *inst_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS           status             - SUCCESS
*
*************************************************************************/
STATUS Serial_Tgt_Pwr_Notify_Resume1(VOID *instance_handle)
{
    SERIAL_INSTANCE_HANDLE *inst_ptr = (SERIAL_INSTANCE_HANDLE*)instance_handle;
    STATUS status;


    /* Set baud rate */
    status = Serial_Tgt_Baud_Rate_Set(inst_ptr, inst_ptr->attrs.baud_rate);

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Pwr_Notify_Resume2
*
*   DESCRIPTION
*
*       This function will notify the STM32_USART when to park or to resume
*
*   INPUT
*
*       VOID                *inst_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS           status             - SUCCESS
*
*************************************************************************/
STATUS Serial_Tgt_Pwr_Notify_Resume2(VOID *instance_handle)
{
    /* Enable Rx and TX */
    Serial_Tgt_Enable((SERIAL_INSTANCE_HANDLE*)instance_handle);

    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       Serial_Tgt_Pwr_Notify_Resume3
*
*   DESCRIPTION
*
*       This function will notify the STM32_USART when to park or to resume
*
*   INPUT
*
*       VOID                *inst_handle    - Instance handle
*
*   OUTPUT
*
*       STATUS           status             - SUCCESS
*
*************************************************************************/
STATUS Serial_Tgt_Pwr_Notify_Resume3(VOID *instance_handle)
{
    SERIAL_INSTANCE_HANDLE *inst_ptr = (SERIAL_INSTANCE_HANDLE*)instance_handle;

    if(inst_ptr->attrs.rx_mode == USE_IRQ)
    {
        /* Enable RX interrupt on controller. */
        Serial_Tgt_Rx_Int_Enable(inst_ptr);
    }

    /* Enable Tx interrupt if it was enabled when we parked */
    if(inst_ptr->tx_intr_en_shadow == NU_TRUE)
    {
        Serial_Tgt_Tx_Int_Enable(inst_ptr);
    }

    return (NU_SUCCESS);
}

#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)) */

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
