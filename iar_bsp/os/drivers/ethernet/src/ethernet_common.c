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
*       ethernet_common.c
*
*   COMPONENT
*
*       ETHERNET                            - Ethernet Library
*
*   DESCRIPTION
*
*       This file contains the generic Ethernet library functions.
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
*       Ethernet_Get_Target_Info
*
*   DESCRIPTION
*
*       This function gets target info from Registry
*
*   INPUTS
*
*       CHAR      *key                      - Registry path
*       SERIAL_TGT *tgt_info                 - pointer to target info structure
*                                             (populated by this function)
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS for success
*                                           - ETHERNET_REGISTRY_ERROR for error
*
*************************************************************************/
STATUS Ethernet_Get_Target_Info(const CHAR * key, ETHERNET_INSTANCE_HANDLE *inst_handle)
{
    STATUS     reg_stat;
    STATUS     status = NU_SUCCESS;
    CHAR       reg_path[255];

    /* Get values from the registry */
    strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
    reg_stat = REG_Get_String (strcat(reg_path, "/tgt_settings/dev_name"), inst_handle->name, sizeof(inst_handle->name));

    if (reg_stat == NU_SUCCESS)
    {
        strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
        reg_stat = REG_Get_UINT32 (strcat(reg_path, "/tgt_settings/base"), (UINT32*)&inst_handle->io_addr);
    }

    if (reg_stat == NU_SUCCESS)
    {
        strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
        reg_stat = REG_Get_UINT32 (strcat(reg_path, "/tgt_settings/vector"), (UINT32*)&inst_handle->irq);
    }

    if (reg_stat == NU_SUCCESS)
    {
        strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
        reg_stat = REG_Get_UINT32 (strcat(reg_path, "/tgt_settings/priority"), (UINT32*)&inst_handle->irq_priority);
    }

    if (reg_stat == NU_SUCCESS)
    {
        strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
        reg_stat = REG_Get_UINT32 (strcat(reg_path, "/tgt_settings/trigger_type"), (UINT32*)&inst_handle->irq_type);
    }

    if (reg_stat == NU_SUCCESS)
    {
        strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
        reg_stat = REG_Get_UINT32 (strcat(reg_path, "/tgt_settings/number"), (UINT32*)&inst_handle->number);
    }

    if (reg_stat == NU_SUCCESS)
    {
        strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
        reg_stat = REG_Get_UINT32 (strcat(reg_path, "/tgt_settings/phy_vector"), (UINT32*)&inst_handle->phy_irq);
    }

    if(reg_stat == NU_SUCCESS)
    {
        strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
        reg_stat = REG_Get_String (strcat(reg_path, "/tgt_settings/ref_clock"), &(inst_handle->ref_clock[0]), NU_DRVR_REF_CLOCK_LEN);
    }

    if (reg_stat != NU_SUCCESS)
    {
        status = reg_stat;
    }

    return(status);
}

