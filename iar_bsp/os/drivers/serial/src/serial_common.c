/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
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
*       serial_common.c
*
*   COMPONENT
*
*       SERIAL                              - Serial Library
*
*   DESCRIPTION
*
*       This file contains the generic Serial library functions.
*
*   FUNCTIONS
*
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*
*************************************************************************/

/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
extern VOID    Serial_Tgt_LISR (INT vector);


/*************************************************************************
*
* FUNCTION
*
*       Serial_PR_Int_Enable
*
* DESCRIPTION
*
*       This function initializes processor-level TX interrupt
*
* INPUTS
*
*       VOID           *sess_handle         - Device session handle
*
* OUTPUTS
*
*       STATUS         status               - Status resulting from
*                                             NU_Register_LISR calls.
*
*************************************************************************/
STATUS Serial_PR_Int_Enable(SERIAL_SESSION_HANDLE *session_handle)
{
    SERIAL_INSTANCE_HANDLE *instance_handle = (session_handle->instance_ptr);
    STATUS                status = NU_SUCCESS;
    VOID                  (*old_lisr)(INT);


    /* Check if TX interrupt needs to be registered and enabled */
    if (instance_handle->attrs.tx_mode == USE_IRQ)
    {
        /* Register UART handler for the TX interrupt */
        status = NU_Register_LISR (instance_handle->serial_tx_vector,
                                   &Serial_Tgt_LISR, &old_lisr);

        /* Check if registration successful */
        if (status == NU_SUCCESS)
        {
            /* Register the UART data structure with this vector id */
            ESAL_GE_ISR_VECTOR_DATA_SET (instance_handle->serial_tx_vector, session_handle);

            /* Enable the UART TX interrupt */
            (VOID) ESAL_GE_INT_Enable (instance_handle->serial_tx_vector,
                                       instance_handle->serial_tx_irq_type,
                                       instance_handle->serial_tx_irq_priority);
        }

    }

    /* Check if RX interrupt needs to be registered and enabled */
    if (instance_handle->attrs.rx_mode == USE_IRQ)
    {
        /* Register RX UART vector */
        status = NU_Register_LISR(instance_handle->serial_rx_vector,
                                  &Serial_Tgt_LISR, &old_lisr);

        /* Check if registration successful */
        if (status == NU_SUCCESS)
        {
            /* Register the UART data structure with this vector id */
            ESAL_GE_ISR_VECTOR_DATA_SET (instance_handle->serial_rx_vector, session_handle);

            /* Enable the UART RX interrupt */
            (VOID) ESAL_GE_INT_Enable (instance_handle->serial_rx_vector,
                                       instance_handle->serial_rx_irq_type,
                                       instance_handle->serial_rx_irq_priority);
        }
    }

    return status;
}


/*************************************************************************
*
* FUNCTION
*
*       Serial_PR_Int_Disable
*
* DESCRIPTION
*
*       This function initializes processor-level TX interrupt
*
* INPUTS
*
*       VOID           *sess_handle         - Device session handle
*
* OUTPUTS
*
*       STATUS         status               - Status resulting from
*                                             NU_Register_LISR calls.
*
*************************************************************************/
STATUS Serial_PR_Int_Disable(SERIAL_INSTANCE_HANDLE *instance_handle)
{
    STATUS                status = NU_SUCCESS;
    VOID                  (*old_lisr)(INT);


    /* Disable TX interrupts */
    if (instance_handle->serial_tx_vector != ESAL_DP_INT_VECTOR_ID_DELIMITER)
    {
        /* Disable the UART TX interrupt */
        (VOID) ESAL_GE_INT_Disable (instance_handle->serial_tx_vector);

        /* Register the UART data structure with this vector id */
        ESAL_GE_ISR_VECTOR_DATA_SET (instance_handle->serial_tx_vector, NU_NULL);

        /* Unregister TX UART LISR */
        status = NU_Register_LISR (instance_handle->serial_tx_vector, NU_NULL, &old_lisr);
    }

    /* Disable RX interrupts */
    if (instance_handle->serial_rx_vector != ESAL_DP_INT_VECTOR_ID_DELIMITER)
    {
        /* Disable the UART TX interrupt */
        (VOID) ESAL_GE_INT_Disable (instance_handle->serial_rx_vector);

        /* Register the UART data structure with this vector id */
        ESAL_GE_ISR_VECTOR_DATA_SET (instance_handle->serial_rx_vector, NU_NULL);

        /* Unregister RX UART LISR */
        status = NU_Register_LISR (instance_handle->serial_rx_vector, NU_NULL, &old_lisr);
    }

    return status;
}


/*************************************************************************
*
*   FUNCTION
*
*       Serial_Get_Target_Info
*
*   DESCRIPTION
*
*       This function gets target info from Registry
*
*   INPUTS
*
*       key                                 - Registry path
*       inst_info                           - pointer to serial instance info structure
*                                             (populated by this function)
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS for success
*                                           - SERIAL_REGISTRY_ERROR for error
*
*************************************************************************/
STATUS Serial_Get_Target_Info(const CHAR * key, SERIAL_INSTANCE_HANDLE *inst_info)
{
    STATUS     reg_status = NU_SUCCESS;
    STATUS     status;
    UINT32     temp32;
    CHAR       reg_path[REG_MAX_KEY_LENGTH];


    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/base_addr", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            inst_info->serial_io_addr = temp32;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/clock", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            inst_info->serial_clock = temp32;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/tx_intr_vector", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            inst_info->serial_tx_vector = temp32;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/tx_intr_priority", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            inst_info->serial_tx_irq_priority = temp32;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/tx_intr_trigger_type", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            inst_info->serial_tx_irq_type = (ESAL_GE_INT_TRIG_TYPE)temp32;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/rx_intr_vector", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            inst_info->serial_rx_vector = temp32;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/rx_intr_priority", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            inst_info->serial_rx_irq_priority = temp32;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/tgt_settings/rx_intr_trigger_type", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            inst_info->serial_rx_irq_type = (ESAL_GE_INT_TRIG_TYPE)temp32;
        }
    }

    if (reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_String_Value(key, "/tgt_settings/ref_clock", reg_path, NU_DRVR_REF_CLOCK_LEN);
        if (reg_status == NU_SUCCESS)
        {
            strncpy(inst_info->serial_ref_clock, reg_path, NU_DRVR_REF_CLOCK_LEN);
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        status = NU_SUCCESS;
    }

    else
    {
        status = SERIAL_REGISTRY_ERROR;
    }

    return status;
}


/*************************************************************************
*
*   FUNCTION
*
*       Serial_Get_Default_Cfg
*
*   DESCRIPTION
*
*       This function gets default config from Registry
*
*   INPUTS
*
*       CHAR       *key                     - Registry path
*       SERIAL_ATTR *attrs                   - pointer to device attributes
*                                             (populated by this function)
*
*   OUTPUTS
*
*       STATUS     status                   - NU_SUCCESS for success
*                                           - SERIAL_REGISTRY_ERROR for error
*
*************************************************************************/
STATUS Serial_Get_Default_Cfg(const CHAR *key, SERIAL_ATTR *attrs)
{
    STATUS     reg_status = NU_SUCCESS;
    STATUS     status;
    UINT32     temp32;


    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/dev_settings/baud_rate", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            attrs->baud_rate = temp32;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/dev_settings/data_bits", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            attrs->data_bits = temp32;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/dev_settings/stop_bits", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            attrs->stop_bits = temp32;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/dev_settings/parity", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            attrs->parity = temp32;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/dev_settings/tx_mode", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            attrs->tx_mode = temp32;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/dev_settings/rx_mode", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            attrs->rx_mode = temp32;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        reg_status = REG_Get_UINT32_Value(key, "/dev_settings/flow_ctrl", &temp32);
        if (reg_status == NU_SUCCESS)
        {
            attrs->flow_ctrl = temp32;
        }
    }

    if(reg_status == NU_SUCCESS)
    {
        status = NU_SUCCESS;
    }
    else
    {
        status = SERIAL_REGISTRY_ERROR;
    }

    return status;
}

