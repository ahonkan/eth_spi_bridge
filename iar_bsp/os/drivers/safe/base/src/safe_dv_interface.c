/*************************************************************************/
/*                                                                       */
/*               Copyright 2011 Mentor Graphics Corporation              */
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
*       safe_dv_interface.c
*
* COMPONENT
*
*       SAFE             - Safe library
*
* DESCRIPTION
*
*      This file contains the generic Safe library functions.
*
*
* FUNCTIONS
*
*       SAFE_Dv_Register                         Registers device with DM
*       Safe_Dv_Unregister                          Unregisters the device with DM
*       SAFE_Dv_Open                            Init and open a SAFE device
*       SAFE_Dv_Close                           Uninit and deallocate SAFE device
*       SAFE_Dv_Ioctl                           IOCTL routine.
*
*******************************************************************/
/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "services/nu_trace_os_mark.h"

/**********************************/
/* EXTERNAL VARIABLE DECLARATIONS */
/**********************************/
extern  NU_MEMORY_POOL  System_Memory;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
extern STATUS   SAFE_Set_State(VOID *inst_handle, PM_STATE_ID *state);
#endif

/*************************************************************************
*
*   FUNCTION
*
*       SAFE_Dv_Register
*
*   DESCRIPTION
*
*       This function registers the SAFE hardware
*
*   INPUTS
*
*       CHAR          *key                  - Key
*       INT           startstop             - Start or stop flag
*       DV_DEV_ID     *dev_id               - Returned Device ID
*       SAFE_INSTANCE_HANDLE          *inst_handle             - Target info
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS or error code
*
*************************************************************************/
STATUS    SAFE_Dv_Register (const CHAR *key, INT startstop, DV_DEV_ID *dev_id,
                         SAFE_INSTANCE_HANDLE *inst_handle)
{
    STATUS               status = NU_NOT_REGISTERED;
    DV_DEV_LABEL         safe_labels[3] = {{SAFE_LABEL}, {STORAGE_LABEL}};
    DV_DRV_FUNCTIONS     safe_drv_funcs;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    INT                  all_labels_cnt = 2;

#endif

    /* Populate function pointers */
    safe_drv_funcs.drv_open_ptr  = &SAFE_Dv_Open;
    safe_drv_funcs.drv_close_ptr = &SAFE_Dv_Close;
    safe_drv_funcs.drv_read_ptr  = NU_NULL;
    safe_drv_funcs.drv_write_ptr = NU_NULL;
    safe_drv_funcs.drv_ioctl_ptr = &SAFE_Dv_Ioctl;

    if ((key != NU_NULL) && (inst_handle != NU_NULL))
    {

        /* Poplulate info */
        inst_handle->dev_id = DV_INVALID_DEV;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

        /********************************/
        /* INITIALIZE AS POWER DEVICE   */
        /********************************/

        status = PMI_Device_Initialize(&(inst_handle->pmi_dev), key, safe_labels,
                                                    &all_labels_cnt, NU_NULL);

        if (status == NU_SUCCESS)
        {
            /* Setup the power device */
            PMI_Device_Setup(inst_handle->pmi_dev, &SAFE_Set_State, SAFE_POWER_BASE,
                             SAFE_TOTAL_STATE_COUNT, &(inst_handle->dev_id), (VOID*)inst_handle);
        }

#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

        /*********************************/
        /* REGISTER WITH DEVICE MANAGER  */
        /*********************************/
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))   
        TCCT_Schedule_Lock();
#endif
        
        /* Register this device with the Device Manager */
        status = DVC_Dev_Register(inst_handle,
                                  safe_labels,
                                  DV_GET_LABEL_COUNT(safe_labels),
                                  &safe_drv_funcs,
                                  &inst_handle->dev_id);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                                    
        /* Get default power state */
        init_pwr_state = PMI_STATE_GET(inst_handle->pmi_dev); 
                              
        /* Trace log */
        T_DEV_NAME((CHAR*)key, init_pwr_state, SAFE_TOTAL_STATE_COUNT, inst_handle->dev_id);
  
        TCCT_Schedule_Unlock();       
#endif    
        /* Save dev_id in instance structure */
        *dev_id = inst_handle->dev_id;
        
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       Safe_Dv_Unregister
*
*   DESCRIPTION
*
*       This function unregisters the SAFE hardware
*
*   INPUTS
*
*       DV_DEV_ID     dev_id                - Device ID
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS or error code
*
*************************************************************************/
STATUS     Safe_Dv_Unregister (DV_DEV_ID dev_id)
{
    STATUS               status;
    SAFE_INSTANCE_HANDLE *inst_handle;
    WR_SAFE_DEV_CB       *safe_dev_cb;

    /*****************************************/
    /* UNREGISTER DEVICE WITH DEVICE MANAGER */
    /*****************************************/
    status = DVC_Dev_Unregister (dev_id, (VOID*)&inst_handle);

    if(status == NU_SUCCESS)
    {
        if((inst_handle != NU_NULL))
        {
            safe_dev_cb = (WR_SAFE_DEV_CB *)inst_handle->safe_dev_cb;           

            /* Check if the device is still open */
            if (inst_handle->safe_opencount == 0)
            {
                if (!(safe_dev_cb->log_flag & WR_SAFE_DEV_CB_FL_VALID))
                {
                    /* Free buffer. */
                    (VOID)NU_Deallocate_Memory(safe_dev_cb->safe_mnt_params.buffer);

                    /* Clear all fields */
                    safe_dev_cb->getmem_func = NU_NULL;
                    safe_dev_cb->safe_mnt_params.buffer = NU_NULL;
                    safe_dev_cb->safe_mnt_params.buffsize = 0;
                    safe_dev_cb->safe_mnt_params.mountfunc = NU_NULL;
                    safe_dev_cb->safe_mnt_params.phyfunc = NU_NULL;

                    safe_dev_cb->phy_flag &= !WR_SAFE_DEV_CB_FL_VALID;


                }
                else
                {
                    status = NUF_IN_USE;
                }
            }
            else
            {
                status = NUF_IN_USE;
            }
        }       

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

        if((inst_handle != NU_NULL))
        {
            status = PMI_Device_Unregister(inst_handle->pmi_dev);
        }

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

        /* Deallocate the memory of the instance handle */
        (VOID)NU_Deallocate_Memory(inst_handle);
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       SAFE_Dv_Open
*
*   DESCRIPTION
*
*       This function creates a session handle
*
*   INPUTS
*
*       VOID          *instance_handle      - Instance handle of the driver
*       DV_DEV_LABEL  labels_list[]         - Access mode (label) of open
*       INT           labels_cnt            - Number of labels
*       VOID*         *session_handle       - Session handle
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS or error code
*
*************************************************************************/
STATUS SAFE_Dv_Open (VOID *instance_handle,
                             DV_DEV_LABEL labels_list[],
                             INT labels_cnt,
                             VOID* *session_handle)
{
    STATUS               status = NU_SUCCESS;
    INT                  int_level;
    SAFE_SESSION_HANDLE  *sess_handle;
    SAFE_INSTANCE_HANDLE *safe_dev = (SAFE_INSTANCE_HANDLE*)instance_handle;
    UINT32               open_mode_requests = 0;
    DV_DEV_LABEL         safe_label    = {SAFE_LABEL};
    DV_DEV_LABEL         storage_label = {STORAGE_LABEL};

    /* Get open mode requests from labels */
    if ((DVS_Label_List_Contains(labels_list, labels_cnt, safe_label) == NU_SUCCESS) ||
        (DVS_Label_List_Contains(labels_list, labels_cnt, storage_label) == NU_SUCCESS))
    {
        open_mode_requests |= SAFE_OPEN_MODE;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    /* Call the Power device open function */
    status = PMI_Device_Open (&open_mode_requests, labels_list, labels_cnt);

#endif

    /* If device is already open AND if the open request contains serial mode, return a error. */
    if (!((safe_dev->device_in_use == NU_TRUE) && (open_mode_requests & SAFE_OPEN_MODE)))
    {
        /* Allocate memory for the SAFE_SESSION_HANDLE structure */
        status = NU_Allocate_Memory (&System_Memory, (VOID *)&sess_handle, sizeof (SAFE_SESSION_HANDLE), NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Zero out allocated space */
            (VOID)memset (sess_handle, 0, sizeof (SAFE_SESSION_HANDLE));

            /* Disable interrupts */
            int_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

            /* Place a pointer to instance handle in session handle */
            sess_handle->inst_info = instance_handle;


            /* If the open mode request is SAFE */
            if (open_mode_requests & SAFE_OPEN_MODE)
            {
                /* Set device in use flag to true */
                safe_dev->device_in_use = NU_TRUE;

                /* Increment the open count */
                sess_handle->inst_info->safe_opencount++;
            }

            sess_handle->open_modes |= open_mode_requests;

            /* Set the return address of the session handle */
            *session_handle = (VOID*)sess_handle;

            /* Restore interrupts to previous level */
            (VOID)NU_Local_Control_Interrupts (int_level);

        }
    }
    else
    {
        /* No session found */
        status = NU_SAFE_SESSION_UNAVAILABLE;
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SAFE_Dv_Close
*
*   DESCRIPTION
*
*       This function deletes the session handle
*
*   INPUTS
*
*       VOID          *session_handle       - Session handle of the device
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS or error code
*
*************************************************************************/
STATUS SAFE_Dv_Close(VOID *session_handle)
{
    STATUS               status = NU_SUCCESS;
    INT                  int_level;
    SAFE_SESSION_HANDLE  *sess_handle = (SAFE_SESSION_HANDLE*)session_handle;
    SAFE_INSTANCE_HANDLE *inst_handle = (SAFE_INSTANCE_HANDLE*)(sess_handle->inst_info);

    /* Disable interrupts */
    int_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    status = PMI_Device_Close((inst_handle->pmi_dev));

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* If the open mode request was SAFE */
    if (sess_handle->open_modes & SAFE_OPEN_MODE)
    {
        sess_handle->open_modes &= ~SAFE_OPEN_MODE;

        /* Decrement the open count */
        inst_handle->safe_opencount--;

        /* Set device in use flag to false */
        inst_handle->device_in_use = NU_FALSE;
    }

    /* Deallocate data */
    (VOID)NU_Deallocate_Memory(sess_handle);

    /* Restore interrupts to previous level */
    (VOID)NU_Local_Control_Interrupts (int_level);

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SAFE_Dv_Ioctl
*
*   DESCRIPTION
*
*       This function controls IO operations of the SAFE driver.
*
*   INPUTS
*       VOID          *session_handle       - Session handle of the driver
*       INT           cmd                   - Ioctl command
*       VOID          *data                 - Ioctl data pointer
*       INT           length                - Ioctl length
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS
*
*************************************************************************/
STATUS SAFE_Dv_Ioctl(VOID *session_handle, INT cmd, VOID *data, INT length)
{
    STATUS                 status = NU_SUCCESS;
    DV_IOCTL0_STRUCT       *ioctl0;
    DV_DEV_LABEL           safe_label = {SAFE_LABEL};
    DV_DEV_LABEL           storage_label = {STORAGE_LABEL};
    FPART_DISK_INFO_S      *disk_info;
    STATUS                 *chkdsk_sts;
    SAFE_SESSION_HANDLE    *sess_handle = (SAFE_SESSION_HANDLE*)session_handle;
    SAFE_INSTANCE_HANDLE   *inst_handle = (SAFE_INSTANCE_HANDLE*)(sess_handle->inst_info);
    CHAR                   reg_path[REG_MAX_KEY_LENGTH];
    STORAGE_MW_CONFIG_PATH *config_path;
    WR_SAFE_DEV_CB         *safe_dev_cb;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    PMI_DEV_HANDLE          pmi_dev = inst_handle->pmi_dev;

#endif

    /* Process command */
    switch (cmd)
    {
        case DV_IOCTL0:

            if (length == sizeof(DV_IOCTL0_STRUCT))
            {
                /* Get the ioctl0 structure from the data passed in */
                ioctl0 = (DV_IOCTL0_STRUCT *) data;
                status = DV_IOCTL_INVALID_MODE;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
                status = PMI_Device_Ioctl(pmi_dev, cmd, data, length, inst_handle,
                                          sess_handle->open_modes);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

                if(status != NU_SUCCESS)
                {
                    /* If the mode requested is supported and if the session was opened for that mode */
                    if ( (sess_handle->open_modes & SAFE_OPEN_MODE) &&
                         ((DV_COMPARE_LABELS(&ioctl0->label, &storage_label)) ||
                          (DV_COMPARE_LABELS(&ioctl0->label, &safe_label)) ))
                    {
                        ioctl0->base = STORAGE_CMD_BASE;
                        status = NU_SUCCESS;
                    }
                }                       
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }

            break;

        /*******************/
        /* Storage Ioctls  */
        /*******************/

        case (STORAGE_CMD_BASE + FDEV_GET_DISK_INFO):

            if ((length == sizeof (FPART_DISK_INFO_S)) && (data != NU_NULL))
            {
                /* Get the dev info structure from the data passed in */
                disk_info = (FPART_DISK_INFO_S *) data;
                (VOID)memset (disk_info, 0, sizeof (FPART_DISK_INFO_S));

                /* Setup the device name */
                strcpy(disk_info->fpart_name, sess_handle->inst_info->dev_name);

                /* Populate other SAFE info */
                disk_info->fpart_flags = FPART_DI_SAFE;
                disk_info->fpart_spec = sess_handle->inst_info->safe_dev_cb;
            }

            break;

        case (STORAGE_CMD_BASE + FDEV_GET_DISK_STATUS):

            if ((length == sizeof (STATUS)) && (data != NU_NULL))
            {
                chkdsk_sts = (STATUS *) data;

                /* Get local pointer to new control block. */
                safe_dev_cb = inst_handle->safe_dev_cb;

                /* Check if the disk has been initialized */
                /* If the physical control block hasn't been initialized. */
                if (safe_dev_cb->phy_flag & WR_SAFE_DEV_CB_FL_VALID)
                {
                    *chkdsk_sts = NU_SUCCESS;
                }
                else
                {
                    *chkdsk_sts = NUF_NOT_OPENED;
                }
            }

            break;

        case (STORAGE_CMD_BASE + FDEV_FLUSH):

            break;

        case (STORAGE_CMD_BASE + FDEV_TRIM):

            break;

        case (STORAGE_CMD_BASE + FDEV_GET_MW_CONFIG_PATH):

            if ((length == sizeof (STORAGE_MW_CONFIG_PATH)) && (data != NU_NULL))
            {
                config_path = (STORAGE_MW_CONFIG_PATH *) data;

                /* Return the middleware config path */
                strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
                strcat(reg_path, "/mw_settings");
                strncpy(config_path->config_path, reg_path, config_path->max_path_len);
            }

            break;

        default:
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            status = PMI_Device_Ioctl(pmi_dev, cmd, data, length, inst_handle,
                                      sess_handle->open_modes);

#else

            status = DV_INVALID_INPUT_PARAMS;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */


            status = DV_IOCTL_INVALID_CMD;

            break;
    }

    return (status);
}


