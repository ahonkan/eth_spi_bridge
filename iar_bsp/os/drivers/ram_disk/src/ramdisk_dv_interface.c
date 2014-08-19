/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       ramdisk_dv_interface.c
*
* COMPONENT
*
*       RAM Disk Driver
*
* DESCRIPTION
*
*       Provides a configurable ram drive capability using pages allocated
*       from a Nucleus memory partition.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       RD_Dv_Register                      Registers device with DM
*       RD_Dv_Unregister                    Unregisters the device with DM
*       RD_Dv_Open                          Init and create RD page pool.
*       RD_Dv_Close                         Uninit and deallocate RD
*       RD_Dv_Ioctl                         IOCTL routine.
*
*******************************************************************/

#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "services/nu_services.h"
#include        "storage/nu_storage.h"
#include        "drivers/nu_drivers.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "services/nu_trace_os_mark.h"

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
extern STATUS RD_Pwr_Set_State  (VOID *inst_handle, PM_STATE_ID *state);
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

/*************************************************************************
*
*   FUNCTION
*
*       RD_Dv_Register
*
*   DESCRIPTION
*
*       This function registers the RD hardware
*
*   INPUTS
*
*       CHAR          *key                  - Key
*       INT           startstop             - Start or stop flag
*       DV_DEV_ID     *dev_id               - Returned Device ID
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS
*                                           - DV_LABEL_LIST_TOO_SMALL
*                                           - RD_NO_INSTANCE_AVAILABLE
*                                           - RD_TGT_SETUP_FAILED
*
*************************************************************************/
STATUS RD_Dv_Register (const CHAR * key, RD_INSTANCE_HANDLE *inst_handle)
{
    STATUS             status;
    DV_DEV_LABEL       rd_labels[3] = {{RD_LABEL}, {STORAGE_LABEL}};
    INT                rd_label_cnt = 2;
    DV_DRV_FUNCTIONS   rd_drv_funcs;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif

    inst_handle->dev_id = DV_INVALID_DEV;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    /******************************************/
    /* PLACE DEVICE IN KNOWN, LOW-POWER STATE */
    /******************************************/

    /********************************/
    /* INITIALIZE AS POWER DEVICE   */
    /********************************/
    status = PMI_Device_Initialize(&(inst_handle->pmi_dev), key, rd_labels,
                                   &rd_label_cnt, NU_NULL);

    if (status == NU_SUCCESS)
    {
        /* Setup the power device */
        PMI_Device_Setup(inst_handle->pmi_dev, &RD_Pwr_Set_State, RD_POWER_BASE,
                         RD_TOTAL_STATE_COUNT, &(inst_handle->dev_id), (VOID*)inst_handle);
    }

#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

    /*********************************/
    /* REGISTER WITH DEVICE MANAGER  */
    /*********************************/

    /* Populate function pointers */
    rd_drv_funcs.drv_open_ptr  = &RD_Dv_Open;
    rd_drv_funcs.drv_close_ptr = &RD_Dv_Close;
    rd_drv_funcs.drv_read_ptr  = &RD_Read;
    rd_drv_funcs.drv_write_ptr = &RD_Write;
    rd_drv_funcs.drv_ioctl_ptr = &RD_Dv_Ioctl;

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))   
    TCCT_Schedule_Lock();
#endif
    
    /* Register this device with the Device Manager */
    status = DVC_Dev_Register(inst_handle, rd_labels,
                              rd_label_cnt, &rd_drv_funcs,
                              &inst_handle->dev_id);
                              
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                                    
    /* Get default power state */
    init_pwr_state = PMI_STATE_GET(inst_handle->pmi_dev); 
                        
    /* Trace log */
    T_DEV_NAME((CHAR*)key, init_pwr_state, RD_TOTAL_STATE_COUNT, inst_handle->dev_id);
 
    TCCT_Schedule_Unlock(); 
#endif    

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       RD_Dv_Unregister
*
*   DESCRIPTION
*
*       This function unregisters the RD hardware
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
STATUS     RD_Dv_Unregister (const CHAR *key, INT startstop, DV_DEV_ID dev_id)
{
    STATUS             status;
    RD_INSTANCE_HANDLE *inst_handle;


    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(startstop);

    /* Unregister the device */
    status = DVC_Dev_Unregister (dev_id, (VOID**)&inst_handle);

    if (status == NU_SUCCESS)
    {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

        status = PMI_Device_Unregister(inst_handle->pmi_dev);

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

        if (status == NU_SUCCESS)
        {
            /* Deallocate the instance handle memory */
            (VOID)NU_Deallocate_Memory((VOID*)inst_handle);
        }
    }

    return (status);
}
 

/*************************************************************************
*
*   FUNCTION
*
*       RD_Dv_Open
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
STATUS RD_Dv_Open (VOID *instance_handle,
                    DV_DEV_LABEL labels_list[], 
                    INT labels_cnt,
                    VOID* *session_handle) 
{
    STATUS             status = ~NU_SUCCESS;
    INT                int_level;
    RD_INSTANCE_HANDLE *inst_handle = (RD_INSTANCE_HANDLE*)instance_handle;
    RD_SESSION_HANDLE  *sess_ptr;
    UINT32             open_mode_requests = 0;
    DV_DEV_LABEL       storage_label = {STORAGE_LABEL};
    DV_DEV_LABEL       rd_label   = {RD_LABEL};
    NU_MEMORY_POOL     *sys_pool_ptr;


    /* Get open mode requests from labels */
    if ((DVS_Label_List_Contains(labels_list, labels_cnt, storage_label) == NU_SUCCESS) ||
        (DVS_Label_List_Contains(labels_list, labels_cnt, rd_label) == NU_SUCCESS) )
    {
        open_mode_requests |= STORAGE_OPEN_MODE;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    /* Call the Power device open function */
    (VOID)PMI_Device_Open (&open_mode_requests, labels_list, labels_cnt);

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

    /* If device is already open AND if the open request contains storage mode, return an error. */
    if (!((inst_handle->device_in_use == NU_TRUE) && (open_mode_requests & STORAGE_OPEN_MODE)) &&
          (inst_handle->rd_opencount == 0))
    {
        /* Get system memory pool pointer */
        status =  NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

        if (status == NU_SUCCESS)
        {
            /* Allocate memory for the session handle structure */
            status = NU_Allocate_Memory (sys_pool_ptr, (VOID**)&sess_ptr,
                                         sizeof (RD_SESSION_HANDLE), NU_NO_SUSPEND);

            /* If we allocated the memory */
            if (status == NU_SUCCESS)
            {
                /* Zero out allocated space */
                (VOID)memset (sess_ptr, 0, sizeof (RD_SESSION_HANDLE));

                /* Disable interrupts while accessing shared variables */
                int_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);
            
                /* Save the instance_handle of the driver in the session ptr */
                sess_ptr->inst_info = instance_handle;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

                /* If open mode request is power */                
                if (open_mode_requests & POWER_OPEN_MODE)
                {
                    sess_ptr->open_modes |= POWER_OPEN_MODE;
                }

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
            
                /* If the open mode request is Storage */
                if (open_mode_requests & STORAGE_OPEN_MODE)
                {
                    sess_ptr->open_modes |= STORAGE_OPEN_MODE;

                     /* Increment the open count */
                    inst_handle->rd_opencount++;

                    /* Set device in use flag to true */
                    inst_handle->device_in_use = NU_TRUE;
                }

                /* Set the return address of the session handle */
                *session_handle = (VOID*)sess_ptr;

                /* Restore interrupts to previous level */
                (VOID)NU_Local_Control_Interrupts (int_level);
            }
        }
    }
        
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       RD_DVM_Close
*
*   DESCRIPTION
*
*       This function deletes the session handle
*
*   INPUTS
*
*       VOID   *session_handle              - Session handle of the device
*
*   OUTPUTS
*
*       STATUS status                       - NU_SUCCESS or error code
*
*************************************************************************/
STATUS RD_Dv_Close(VOID *session_handle)
{
    STATUS             status = NU_SUCCESS;
    INT                int_level;
    RD_SESSION_HANDLE  *sess_handle = (RD_SESSION_HANDLE*)session_handle;
    RD_INSTANCE_HANDLE *inst_handle = (RD_INSTANCE_HANDLE*)(sess_handle->inst_info);


    /* Disable interrupts */
    int_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

    /* If the open mode request was Storage */
    if (sess_handle->open_modes & STORAGE_OPEN_MODE) 
    {
        /* Decrement the open count */     
        inst_handle->rd_opencount--;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            status = PMI_Device_Close((inst_handle->pmi_dev));

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */


        /* Set device in use flag to false */
        inst_handle->device_in_use = NU_FALSE;
    }

    /* Deallocate session handle memory */
    (VOID)NU_Deallocate_Memory(sess_handle);

    /* Restore interrupts to previous level */
    (VOID)NU_Local_Control_Interrupts (int_level);

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       RD_DVM_Ioctl
*
*   DESCRIPTION
*
*       This function controls IO operations of the RD driver.
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
STATUS RD_Dv_Ioctl(VOID *session_handle, INT cmd, VOID *data, INT length)
{
    STATUS                 status = NU_SUCCESS;
    DV_IOCTL0_STRUCT       *ioctl0;
    DV_DEV_LABEL           rd_label      = {RD_LABEL};
    DV_DEV_LABEL           storage_label = {STORAGE_LABEL};
    FPART_DISK_INFO_S      *disk_info;
    STATUS                 *chkdsk_sts;
    RD_SESSION_HANDLE      *sess_handle = (RD_SESSION_HANDLE*)session_handle;
    RD_INSTANCE_HANDLE     *inst_handle = (RD_INSTANCE_HANDLE*)(sess_handle->inst_info);
    CHAR                   reg_path[REG_MAX_KEY_LENGTH];
    STORAGE_MW_CONFIG_PATH *config_path;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    PMI_DEV_HANDLE          pmi_dev = inst_handle->pmi_dev;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */


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

                if (status != NU_SUCCESS)
                {
                    /* If the mode requested is supported and if the session was opened for that mode */
                    if (((DV_COMPARE_LABELS(&ioctl0->label, &storage_label)) ||
                         (DV_COMPARE_LABELS(&ioctl0->label, &rd_label))) &&
                         (sess_handle->open_modes & STORAGE_OPEN_MODE))
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

        case (STORAGE_CMD_BASE + FDEV_GET_DISK_INFO):

            /* Get the dev info structure from the data passed in */
            disk_info = (FPART_DISK_INFO_S *) data;
            
            /* Fill in the disk_info structure */
            strncpy(disk_info->fpart_name, inst_handle->dev_name, sizeof(disk_info->fpart_name));
            disk_info->fpart_bytes_p_sec = inst_handle->rd_sector_size;

            disk_info->fpart_tot_sec = NRAMDISKBLOCKS;
            disk_info->fpart_heads = RAMDISK_PAGE_SIZE*2;

            disk_info->fpart_cyls = (disk_info->fpart_tot_sec / 0x3EC1) + ((disk_info->fpart_tot_sec % 0x3EC1)? 1 : 0);
            disk_info->fpart_secs = NRAMDISKBLOCKS/(disk_info->fpart_cyls * disk_info->fpart_heads);

            /* The FPART_DI_RMVBL_MED is a trick to get
            the partition component to use a fake partition. */
            disk_info->fpart_flags = FPART_DI_LBA_SUP | FPART_DI_RMVBL_MED | FPART_DI_RAMDISK;

            break;
            
        case (STORAGE_CMD_BASE + FDEV_GET_DISK_STATUS):
            
            chkdsk_sts = (STATUS *) data;
            
            /* Check if the disk has been initialized */
            if (inst_handle->rd_pool_init_completed == NU_TRUE)
            {
                *chkdsk_sts = NU_SUCCESS;
            }
            else
            {
                *chkdsk_sts = NUF_NOT_OPENED;
            }
            
            break;              

        case (STORAGE_CMD_BASE + FDEV_FLUSH):
            
            break;              

        case (STORAGE_CMD_BASE + FDEV_TRIM):
            
            break;              

        case (STORAGE_CMD_BASE + FDEV_GET_MW_CONFIG_PATH):
            
            config_path = (STORAGE_MW_CONFIG_PATH *) data;
            
            /* Return the middleware config path */
            strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
            strcat(reg_path, "/mw_settings");
            strncpy(config_path->config_path, reg_path, config_path->max_path_len);

            break;              

        default:

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

            /* Call the PMI IOCTL function for Power */
            status = PMI_Device_Ioctl(pmi_dev, cmd, data, length, inst_handle,
                                      sess_handle->open_modes);

#else

            status = DV_INVALID_INPUT_PARAMS;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

            break;
    }

    return (status);
} 
