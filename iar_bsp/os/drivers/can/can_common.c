/*************************************************************************
*
*            Copyright 2011 Mentor Graphics Corporation
*                      All Rights Reserved.
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
*       can_common.c
*
* COMPONENT
*
*       Can Device Manager Interface - Hardware Driver for CAN controller.
*
* DESCRIPTION
*
*       This file contains the routine for integration of AtmelCAN
*       controller with Device Manager.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_connectivity.h
*       nu_drivers.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/nu_services.h"
#include    "connectivity/nu_connectivity.h"
#include    "drivers/nu_drivers.h"


/**************************************************************************
*
* FUNCTION
*
*       CAN_Get_Registry
*
* DESCRIPTION
*
*       This function registers the CAN driver with device manager
*       and associates an instance handle with it.
*
* INPUTS
*
*       key                                 Key.
*       inst_handle                         can instance pointer
*
* OUTPUTS
*
*       status
*
**************************************************************************/
STATUS CAN_Get_Registry(const CHAR *key, CAN_INSTANCE_HANDLE *inst_handle)
{
    STATUS status;


    /* Get values from the registry. */
    status = REG_Get_String_Value (key, "/tgt_settings/dev_name", inst_handle->name, sizeof(inst_handle->name));
    if (status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/tgt_settings/base_address", (UINT32*)&inst_handle->base_address);
    }
    if (status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/tgt_settings/intr_vector", (UINT32*)&inst_handle->irq);
    }
    if (status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/tgt_settings/intr_priority", (UINT32*)&inst_handle->irq_priority);
    }
    if (status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/tgt_settings/intr_trigger_type", (UINT32*)&inst_handle->irq_type);
    }
    if (status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/dev_settings/baud_rate", (UINT32*)&inst_handle->baud_rate);
    }
    
    if (status == NU_SUCCESS)
    {
        status = REG_Get_String_Value(key, "/tgt_settings/ref_clock", &(inst_handle->ref_clock[0]), NU_DRVR_REF_CLOCK_LEN);
    }

    return (status);
}

/* ======================== End of File ================================ */
