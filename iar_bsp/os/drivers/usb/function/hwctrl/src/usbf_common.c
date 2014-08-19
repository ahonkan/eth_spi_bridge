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
*       usbf_common.c
*
*   COMPONENT
*
*       USBF                                - USBF Library
*
*   DESCRIPTION
*
*       This file contains the generic USBF library functions.
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

/*************************************************************************
*
*   FUNCTION
*
*       USBF_Get_Target_Info
*
*   DESCRIPTION
*
*       This function gets target info from Registry
*
*   INPUTS
*
*       key                                 - Registry path
*       inst_info                           - pointer to touchpanel instance info structure
*                                             (populated by this function)
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS for success
*                                           - USBF_REGISTRY_ERROR for error
*
*************************************************************************/
STATUS USBF_Get_Target_Info(const CHAR * key, USBF_INSTANCE_HANDLE *inst_handle)
{
    STATUS     reg_status = NU_SUCCESS;
    STATUS     status;

    /* Get values from the registry */
    reg_status = REG_Get_String_Value (key, "/tgt_settings/dev_name", inst_handle->name, sizeof(inst_handle->name));
    if (reg_status == NU_SUCCESS)
    {
    	reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/base", (UINT32*)(&inst_handle->base_address));
    }
    if (reg_status == NU_SUCCESS)
    {
    	reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/intr_vector", (UINT32*)(&inst_handle->irq));
    }
    if (reg_status == NU_SUCCESS)
    {
    	reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/priority", (UINT32*)(&inst_handle->irq_priority));
    }
    if (reg_status == NU_SUCCESS)
    {
    	reg_status = REG_Get_UINT32_Value (key, "/tgt_settings/trigger_type", (UINT32*)(&inst_handle->irq_type));
    }
    if (reg_status == NU_SUCCESS)
    {
    	reg_status = REG_Get_String_Value (key, "/tgt_settings/ref_clock", &(inst_handle->ref_clock[0]), NU_DRVR_REF_CLOCK_LEN);
    }
    if (reg_status == NU_SUCCESS)
    {
    	reg_status = REG_Get_UINT32_Value ( key, "/tgt_settings/capability", (UINT32*)(&inst_handle->capability));
    }
    if (reg_status == NU_SUCCESS)
    {
    	reg_status = REG_Get_UINT8_Value ( key, "/tgt_settings/speed", (UINT8*)(&inst_handle->speed));
    }
    if (reg_status == NU_SUCCESS)
    {
    	reg_status = REG_Get_UINT8_Value ( key, "/tgt_settings/num_irq", (UINT8*)(&inst_handle->num_irq));
    }
    if(reg_status == NU_SUCCESS)
    {
        status = NU_SUCCESS;
    }
    else
    {
        status = USBF_REGISTRY_ERROR;
    }

    return status;
}

/************************************************************************
* FUNCTION
*
*       USBF_Init
*
* DESCRIPTION
*
*       This function prepares the USBF control block structure by
*       extracting the content from the specific info structure and
*       calls the device specific setup function if it exists.
*
* INPUTS
*
*       USBF_INSTANCE_HANDLE          *inst_handle          - Instance handle
*       CHAR                          *key                  - Key
*
* OUTPUTS
*
*       STATUS        status               - NU_SUCCESS or error code
*
*************************************************************************/
STATUS USBF_Init(USBF_INSTANCE_HANDLE *inst_handle, const CHAR *key)
{
    STATUS         status = NU_SUCCESS;
    CHAR           reg_path[REG_MAX_KEY_LENGTH];
    STATUS         (*setup_fn)(VOID);
    VOID           (*cleanup_fn)(VOID);

    /******************************/
    /* CALL DEVICE SETUP FUNCTION */
    /******************************/

    /* Setup USBF */
    /* If there is a setup function, call it */
    strcpy(reg_path, key);
    strcat(reg_path, "/setup");
    if (REG_Has_Key(reg_path))
    {
        status = REG_Get_UINT32_Value(key, "/setup", (UINT32 *)&setup_fn);
        if (status == NU_SUCCESS && setup_fn != NU_NULL)
        {
            inst_handle->setup_func = (VOID *)setup_fn;
            setup_fn();
        }
        else
        {
            status = NU_NOT_REGISTERED;
        }
    }

    /* If there is a cleanup function, save it */
    strcpy(reg_path, key);
    strcat(reg_path, "/cleanup");
    if (REG_Has_Key(reg_path))
    {
    	status = REG_Get_UINT32(reg_path, (UINT32 *)&cleanup_fn);
        if (status == NU_SUCCESS && cleanup_fn != NU_NULL )
        {
            inst_handle->cleanup_func = (VOID *)cleanup_fn;
        }
    }

    return (status);
}
