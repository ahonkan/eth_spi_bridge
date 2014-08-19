/*************************************************************************/
/*                                                                       */
/*               Copyright 2010 Mentor Graphics Corporation              */
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
*       safe.c
*
* COMPONENT
*
*       SAFE Disk Driver
*
* DESCRIPTION
*
*       Provides a configurable ram drive capability using pages allocated
*       from a Nucleus memory partition.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       SAFE_Dv_Read                         Read routine (N/A)
*       SAFE_Dv_Write                        Write routine (N/A)
*       SAFE_Init                            SAFE initialization
*       SAFE_Set_State                       Set power mode state
*
*******************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"

/**********************************/
/* EXTERNAL VARIABLE DECLARATIONS */
/**********************************/
extern  NU_MEMORY_POOL  System_Memory;


STATUS    SAFE_Register (const CHAR *key, INT startstop, DV_DEV_ID *dev_id,
                         VOID *tgt_info)
{
    return (NU_SUCCESS);
}

STATUS     SAFE_Unregister (DV_DEV_ID dev_id) 
{
    return (NU_SUCCESS);
}


/************************************************************************
* FUNCTION
*
*       SAFE_Init
*
* DESCRIPTION
*
*       This function prepares the SAFE control block structure by
*       extracting the content from the specific info structure and 
*       calls the device specific setup function if it exists.
*
* INPUTS
*
*       SAFE_INSTANCE_HANDLE          *inst_handle          - Instance handle
*       CHAR                                     *key
*
* OUTPUTS
*
*       STATUS        status               - NU_SUCCESS or error code
*
*************************************************************************/
STATUS SAFE_Init(SAFE_INSTANCE_HANDLE *inst_handle, const CHAR *key)
{
    STATUS         status;
    VOID           *p;
    UINT32         psize;
    WR_SAFE_DEV_CB *safe_dev_cb;
    CHAR           reg_path[REG_MAX_KEY_LENGTH];
    VOID           (*setup_func)(VOID);
    STATUS         reg_stat = NU_NOT_REGISTERED;
    
    /* Save the config path in the instance handle */
    strncpy(inst_handle->config_path, key, sizeof(inst_handle->config_path));

    /* Get the device name and save it in the instance handle */
    strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
    strcat(reg_path, "/tgt_settings/dev_name");
    reg_stat = REG_Get_String (reg_path, inst_handle->dev_name, sizeof(inst_handle->dev_name));

    if (reg_stat == NU_SUCCESS)
    {
        /* Allocate memory for the WR_SAFE_DEV_CB structure */
        status = NU_Allocate_Memory (&System_Memory, (VOID*)&safe_dev_cb, sizeof (WR_SAFE_DEV_CB), NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Zero out allocated space */
            (VOID)memset (safe_dev_cb, 0, sizeof (WR_SAFE_DEV_CB));

            /* Attach the structure */
            inst_handle->safe_dev_cb = safe_dev_cb;
            
            /******************************/
            /* CALL DEVICE SETUP FUNCTION */
            /******************************/

            /* If there is a setup function, call it */
            strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
            strcat(reg_path,"/setup");

            if (REG_Has_Key(reg_path))
            {
                reg_stat = REG_Get_UINT32 (reg_path, (UINT32*)&setup_func);
                if (reg_stat == NU_SUCCESS)
                {
                    setup_func();
                }
            }
       }
    }   

    /* Get local pointer to new control block. */
    safe_dev_cb = inst_handle->safe_dev_cb;

    /* If the physical control block hasn't been initialized. */
    if (!(safe_dev_cb->phy_flag & WR_SAFE_DEV_CB_FL_VALID))
    {
        /* Assign Safe file system specific configuration parameters. */
        safe_dev_cb->getmem_func = inst_handle->getmem_func;
        safe_dev_cb->safe_mnt_params.phyfunc = inst_handle->phy_func;
        safe_dev_cb->safe_mnt_params.mountfunc = inst_handle->mount_func;

        /* Determine the memory requirements for the device.  */
        psize = safe_dev_cb->getmem_func(safe_dev_cb->safe_mnt_params.phyfunc);

        /* Allocate memory for the device. */
        status = NU_Allocate_Memory(&System_Memory, (VOID*)&p, psize, NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            pc_memfill(p, psize, 0x55); /* fill with value */

            /* Assign device memory requirements. */
            safe_dev_cb->safe_mnt_params.buffer = p;
            safe_dev_cb->safe_mnt_params.buffsize = psize;

            /* Set control block as valid. */
            safe_dev_cb->phy_flag |= WR_SAFE_DEV_CB_FL_VALID;
        }
    }
    else
    {
        status = NUF_IN_USE;

    }

    return (status);

}

/*************************************************************************
*
*   FUNCTION
*
*       Safe_Get_Target_Info
*
*   DESCRIPTION
*
*       This function gets target info from Registry
*
*   INPUTS
*
*       key                                 - Registry path
*       inst_info                           - pointer to safe instance info structure
*                                             (populated by this function)
*
*   OUTPUTS
*
*       STATUS    status                    - NU_SUCCESS for success
*                                           - SAFE_REGISTRY_ERROR for error
*
*************************************************************************/
STATUS Safe_Get_Target_Info(const CHAR * key, SAFE_INSTANCE_HANDLE *inst_info)
{
    STATUS      reg_stat;    

    if (key != NU_NULL)
    {
        /* Save the configuration structure */
        reg_stat = REG_Get_UINT32_Value (key, "/tgt_settings/addr_reg", &(inst_info->addr_reg));
        
        if (reg_stat == NU_SUCCESS)
        {
            reg_stat = REG_Get_UINT32_Value (key, "/tgt_settings/cmd_reg", &(inst_info->cmd_reg));
        }
        if (reg_stat == NU_SUCCESS)
        {
            reg_stat = REG_Get_UINT32_Value (key, "/tgt_settings/data_reg", &(inst_info->data_reg));
        }
        if (reg_stat == NU_SUCCESS)
        {
            reg_stat = REG_Get_UINT32_Value (key, "/tgt_settings/CSH_reg", &(inst_info->CSH_reg));
        }
        if (reg_stat == NU_SUCCESS)
        {
            reg_stat = REG_Get_UINT32_Value (key, "/tgt_settings/CSL_reg", &(inst_info->CSL_reg));
        }
        if (reg_stat == NU_SUCCESS)
        {
            reg_stat = REG_Get_UINT32_Value (key, "/tgt_settings/stat_reg", &(inst_info->stat_reg));
        }
        if (reg_stat == NU_SUCCESS)
        {
            reg_stat = REG_Get_UINT32_Value (key, "/tgt_settings/CE_bit", &(inst_info->CE_bit));
        }
        if (reg_stat == NU_SUCCESS)
        {            
            reg_stat = REG_Get_UINT32_Value (key, "/tgt_settings/RB_bit", &(inst_info->RB_bit));
        }
    }
    else
    {
        reg_stat = REG_NOT_WRITABLE;
    }

    return reg_stat;
}



