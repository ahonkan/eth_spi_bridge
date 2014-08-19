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
*       i2c_common.c
*
*   COMPONENT
*
*       I2C                                 - I2C Library
*
*   DESCRIPTION
*
*       This file contains the common functions required by I2C drivers.
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
#include "connectivity/nu_connectivity.h"

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Get_Target_Info
*
*   DESCRIPTION
*
*       This function gets target info from Registry
*
*   INPUTS
*
*       key                                 - Registry path
*       i_handle                            - pointer to i2c instance handle structure
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS for success
*                                           - SERIAL_REGISTRY_ERROR for error
*
*************************************************************************/
STATUS I2C_Get_Target_Info(const CHAR * key, I2C_INSTANCE_HANDLE *i_handle)
{
    STATUS     status;
    UINT32     temp32;

     /* Get values from the registry */

    status = REG_Get_UINT32_Value (key, "/tgt_settings/base", &temp32);

    if (status == NU_SUCCESS)
    {
        i_handle->io_addr = temp32;
    }

    if (status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/tgt_settings/vector", &temp32);
        if (status == NU_SUCCESS)
        {
            i_handle->irq = temp32;
        }
    }

    if (status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/tgt_settings/priority", &temp32);
        if (status == NU_SUCCESS)
        {
            i_handle->irq_priority = temp32;
        }
    }

    if (status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/tgt_settings/trigger_type", &temp32);
        if (status == NU_SUCCESS)
        {
            i_handle->irq_type = (ESAL_GE_INT_TRIG_TYPE)temp32;
        }
    }

    if (status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/tgt_settings/number", &temp32);
        if (status == NU_SUCCESS)
        {
            i_handle->number = temp32;
        }
    }

    if (status == NU_SUCCESS)
    {
        CHAR       reg_path[REG_MAX_KEY_LENGTH];

        status = REG_Get_String_Value (key, "/tgt_settings/ref_clock", reg_path, NU_DRVR_REF_CLOCK_LEN);
        if (status == NU_SUCCESS)
        {
            strncpy(i_handle->ref_clock, reg_path, NU_DRVR_REF_CLOCK_LEN);
        }
    }

    return status;
}


/*************************************************************************
*
*   FUNCTION
*
*       I2C_Set_Reg_Path
*
*   DESCRIPTION
*
*       This function copies registry path to device handle.
*
*   INPUTS
*
*       key                                 - Registry path
*       i_handle                            - pointer to i2c instance handle structure
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS for success
*                                           - SERIAL_REGISTRY_ERROR for error
*
*************************************************************************/
STATUS I2C_Set_Reg_Path(const CHAR * key, I2C_INSTANCE_HANDLE *i_handle)
{
    STATUS     status = NU_SUCCESS;

    /********************************/
    /* COPY REG PATH */
    /********************************/
    if((strlen(key)+1+I2C_MAX_OPTION_SIZE) <= I2C_DEV_REG_PATH_LENGTH)
    {
        strcpy(i_handle->reg_path, key);
        status = NU_SUCCESS;
    }
    else
    {
        status = I2C_DEV_REG_PATH_TOO_LONG;
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Call_Setup_Func
*
*   DESCRIPTION
*
*       This function gets setup function from Registry and calls it.
*
*   INPUTS
*
*       key                                 - Registry path
*       i_handle                            - pointer to i2c instance handle structure
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS for success
*                                           - SERIAL_REGISTRY_ERROR for error
*
*************************************************************************/
STATUS I2C_Call_Setup_Func(const CHAR * key, I2C_INSTANCE_HANDLE *i_handle)
{
    STATUS     status = NU_SUCCESS;
    CHAR       reg_path[REG_MAX_KEY_LENGTH];
    STATUS     (*setup_func)(VOID*);

    /* Get setup function if exists */
    strcpy(reg_path, key);
    strcat(reg_path,"/setup");
    if (REG_Has_Key (reg_path))
    {
        status = REG_Get_UINT32 (reg_path, (UINT32*)&setup_func);
        if (status == NU_SUCCESS && setup_func != NU_NULL)
        {
            (VOID)setup_func(i_handle);
        }
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       I2C_Call_Cleanup_Func
*
*   DESCRIPTION
*
*       This function gets clean-up function from Registry and calls it.
*
*   INPUTS
*
*       key                                 - Registry path
*       i_handle                            - pointer to i2c instance handle structure
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS for success
*                                           - SERIAL_REGISTRY_ERROR for error
*
*************************************************************************/
STATUS I2C_Call_Cleanup_Func(const CHAR * key, I2C_INSTANCE_HANDLE *i_handle)
{
    STATUS     status = NU_SUCCESS;
    CHAR       reg_path[REG_MAX_KEY_LENGTH];
    STATUS     (*setup_func)(VOID*);

    /* Get setup function if exists */
    strcpy(reg_path, key);
    strcat(reg_path,"/setup");
    if (REG_Has_Key (reg_path))
    {
        status = REG_Get_UINT32 (reg_path, (UINT32*)&setup_func);
        if (status == NU_SUCCESS && setup_func != NU_NULL)
        {
            (VOID)setup_func(i_handle);
        }
    }

    return status;
}
