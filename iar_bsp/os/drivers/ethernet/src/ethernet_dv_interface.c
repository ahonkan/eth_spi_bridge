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
*     ethernet_dv_interface.c
*
* COMPONENT
*
*     Ethernet Device Driver.
*
* DESCRIPTION
*
*     This file contains the source of the Ethernet driver.
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
#include "os/kernel/plus/core/inc/thread_control.h"
#include "services/nu_trace_os_mark.h"

/* Forward declaration of the CPSW_Xdata struct*/
struct CPSW_XDATA_STRUCT;


/*************************************************************************
*
*   FUNCTION
*
*       Ethernet_Dv_Register
*
*   DESCRIPTION
*
*       This function registers the ETH hardware
*
*   INPUTS
*
*       CHAR          *key                  - Key
*       INT           startstop             - Start or stop flag
*       DV_DEV_ID     *dev_id               - Returned Device ID
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS or error code
*
*************************************************************************/
STATUS    Ethernet_Dv_Register (const CHAR *key, ETHERNET_INSTANCE_HANDLE *inst_handle)
{
    STATUS                    status = NU_SUCCESS;
    DV_DEV_LABEL              eth_labels[4] = {{NETWORKING_LABEL}, {ETHERNET_LABEL}};
    INT                       labels_cnt = 2;
    DV_DRV_FUNCTIONS          eth_drv_funcs;
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE)) 
    UINT32          init_pwr_state;
#endif    

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    /************************/
    /* CONFIGURE THE DEVICE */
    /************************/

    /* Configure the ETH device */
    (inst_handle->tgt_fn.Tgt_Pwr_Default_State)(inst_handle);

    /********************************/
    /* INITIALIZE AS POWER DEVICE   */
    /********************************/
    status = PMI_Device_Initialize(&inst_handle->pmi_dev, key,
                                    eth_labels, &labels_cnt, 0);

    if (status == NU_SUCCESS)
    {
        /* Setup the power device */
        PMI_Device_Setup(inst_handle->pmi_dev, 
                         inst_handle->tgt_fn.Tgt_Pwr_Set_State,
                            ETHERNET_POWER_BASE, ETHERNET_TOTAL_STATE_COUNT,
                            &inst_handle->dev_id, (VOID*)inst_handle);
    }
#endif

    /*********************************/
    /* REGISTER WITH DEVICE MANAGER  */
    /*********************************/

    /* Populate function pointers */
    eth_drv_funcs.drv_open_ptr  = &Ethernet_Dv_Open;
    eth_drv_funcs.drv_close_ptr = &Ethernet_Dv_Close;
    eth_drv_funcs.drv_read_ptr  = inst_handle->tgt_fn.Tgt_Read;
    eth_drv_funcs.drv_write_ptr = inst_handle->tgt_fn.Tgt_Write;
    eth_drv_funcs.drv_ioctl_ptr = &Ethernet_Dv_Ioctl;

    if (status == NU_SUCCESS)
    {
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))       
        TCCT_Schedule_Lock();
#endif        
        /********************************/
        /* REGISTER WITH DM             */
        /********************************/
        /* Register this device with the Device Manager */
        status = DVC_Dev_Register((VOID*)inst_handle, eth_labels,
                                    labels_cnt, &eth_drv_funcs,
                                    &inst_handle->dev_id);
                                    
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && defined(CFG_NU_OS_SVCS_TRACE_CORE_ENABLE))                                 
        /* Get default power state */
        init_pwr_state = PMI_STATE_GET(inst_handle->pmi_dev);
                                   
        /* Trace log */
        T_DEV_NAME((CHAR*)key, init_pwr_state, ETHERNET_TOTAL_STATE_COUNT, inst_handle->dev_id);
        
        TCCT_Schedule_Unlock();
#endif        
    } 
    


    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Ethernet_Dv_Unregister
*
*   DESCRIPTION
*
*       This function unregisters the ETH hardware
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
STATUS    Ethernet_Dv_Unregister (DV_DEV_ID dev_id)
{
    STATUS               status;
    ETHERNET_INSTANCE_HANDLE *inst_handle;

    /*****************************************/
    /* UNREGISTER DEVICE WITH DEVICE MANAGER */
    /*****************************************/

    /* Unregister the device with Device Manager */
    status = DVC_Dev_Unregister(dev_id, (VOID**)&inst_handle);

    if (status == NU_SUCCESS)
    {
        /* Set dev id to a negative value */
        inst_handle->dev_id = DV_INVALID_DEV;

        /******************************************/
        /* PLACE DEVICE IN KNOWN, LOW-POWER STATE */
        /******************************************/

        (inst_handle->tgt_fn.Tgt_Disable)(inst_handle);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
        status = PMI_Device_Unregister(inst_handle->pmi_dev);
#endif

        /* Deallocate the memory of the instance handle */
        (VOID)NU_Deallocate_Memory(inst_handle);

    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Ethernet_Dv_Open
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
*       VOID*         *session_handle       - Session handle of the driver
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS or error code
*
*************************************************************************/
STATUS Ethernet_Dv_Open (VOID *instance_handle, DV_DEV_LABEL labels_list[],
                             INT labels_cnt, VOID* *session_handle)
{
    STATUS                    status = NU_SUCCESS;
    INT                       int_level;
    ETHERNET_SESSION_HANDLE  *ses_ptr;
    ETHERNET_INSTANCE_HANDLE *inst_handle = (ETHERNET_INSTANCE_HANDLE*)instance_handle;
    UINT32                    open_mode_requests = 0;
    DV_DEV_LABEL              ethernet_label = {ETHERNET_LABEL};
    DV_DEV_LABEL              net_label      = {NETWORKING_LABEL};
    NU_MEMORY_POOL           *sys_pool_ptr;

    /* Get open mode requests from labels */
    if ((DVS_Label_List_Contains(labels_list, labels_cnt, ethernet_label) == NU_SUCCESS) ||
            (DVS_Label_List_Contains(labels_list, labels_cnt, net_label) == NU_SUCCESS))
    {
        open_mode_requests |= ETHERNET_OPEN_MODE;
    }

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    status = PMI_Device_Open (&open_mode_requests, labels_list, labels_cnt);
#endif   

    /* If device is already open AND if the open request contains ETHERNET_OPEN_MODE, return a error. */
    if (!((inst_handle->device_in_use == NU_TRUE) && (open_mode_requests & ETHERNET_OPEN_MODE)))
    {
        /* Allocate memory for the ETHERNET_SESSION_HANDLE structure */
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

        if (status == NU_SUCCESS)
        {
            status = NU_Allocate_Memory (sys_pool_ptr, (VOID *)&ses_ptr, sizeof (ETHERNET_SESSION_HANDLE), NU_NO_SUSPEND);
        }

        if (status == NU_SUCCESS)
        {
            /* Zero out allocated space */
            (VOID)memset (ses_ptr, 0, sizeof (ETHERNET_SESSION_HANDLE));

            /* Disable interrupts */
            int_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

            /* Generate the MW structure that contains ETHERNET specific info */
            ses_ptr->eth_mw.dv_name                    = inst_handle->name;
            ses_ptr->eth_mw.dv_hw.ether.dv_irq         = inst_handle->irq;
            ses_ptr->eth_mw.dv_hw.ether.dv_io_addr     = inst_handle->io_addr;
            ses_ptr->eth_mw.dv_init                    = &ETHERNET_Initialize;
            ses_ptr->eth_mw.dv_hw.ether.dv_shared_addr = 0;
            ses_ptr->eth_mw.dv_flags                   = 0;

            /* Place a pointer to instance handle in session handle */
            ses_ptr->inst_info = inst_handle;

            /* Transfer the dev_id to the PHY structure */
            (inst_handle->tgt_fn.Tgt_Set_Phy_Dev_ID)(inst_handle);

            /* Attach the target specific data to the driver options so the
               info can later be extracted.  This is for internal use only. */
            ses_ptr->eth_mw.dv_driver_options          = (UINT32) inst_handle;

            /* If the open mode request is Ethernet */
            if (open_mode_requests & ETHERNET_OPEN_MODE)
            {
                /* Set device in use flag to true */
                inst_handle->device_in_use = NU_TRUE;
            }

            /* Update mode requests in session handle */
            ses_ptr->open_modes |= open_mode_requests;

            /* Set the return address of the session handle */
            *session_handle = (VOID*)ses_ptr;

            /* Restore interrupts to previous level */
            NU_Local_Control_Interrupts (int_level);
        }

    }

    else
    {
        /* No session found */
        status = NU_ETHERNET_SESSION_UNAVAILABLE;
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       Ethernet_Dv_Close
*
*   DESCRIPTION
*
*       This function deletes the session handle
*
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*
*   OUTPUTS
*
*       STATUS       status                 - NU_SUCCESS
*
*************************************************************************/
STATUS Ethernet_Dv_Close(VOID *session_handle)
{
    STATUS                    status;
    INT                       int_level;
    ETHERNET_SESSION_HANDLE  *sess_handle = (ETHERNET_SESSION_HANDLE*)session_handle;
    ETHERNET_INSTANCE_HANDLE *inst_handle = (ETHERNET_INSTANCE_HANDLE*)(sess_handle->inst_info);

    /* Disable interrupts */
    int_level = NU_Local_Control_Interrupts (NU_DISABLE_INTERRUPTS);

    /* If the open mode request was ethernet */
    if (sess_handle->open_modes & ETHERNET_OPEN_MODE)
    {
        /* Set device in use flag to false */
        inst_handle->device_in_use = NU_FALSE;
    }

    /* Disable the ETHERNET */
    (inst_handle->tgt_fn.Tgt_Disable)(inst_handle);

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    status = PMI_Device_Close(inst_handle->pmi_dev);
#endif

    /* Remove the ethernet device */
    status = NU_Remove_Device(sess_handle->eth_mw.dv_name, NU_NULL);

    /* Deallocate data */
    (VOID)NU_Deallocate_Memory(sess_handle);

    /* Restore interrupts to previous level */
    NU_Local_Control_Interrupts(int_level);

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       Ethernet_Dv_Ioctl
*
*   DESCRIPTION
*
*       This function controls IO operations of the ETHERNET.
*
*   INPUTS
*
*       session_handle                      Session handle of the driver
*       cmd                                 Ioctl command
*       data                                Ioctl data pointer
*       length                              Ioctl length
*
*   OUTPUTS
*
*       NU_SUCCESS                          Successful completion
*       DV_IOCTL_INVALID_MODE               Invalid open mode/label
*       DV_IOCTL_INVALID_LENGTH             Specified data length is invalid
*       NU_UNAVAILABLE                      Specified IOCTL code is unknown
*
*************************************************************************/
STATUS  Ethernet_Dv_Ioctl(VOID *session_handle, INT cmd, VOID *data, INT length)
{
    STATUS                    status = NU_SUCCESS;
    DV_IOCTL0_STRUCT          *ioctl0;
    DV_DEV_LABEL              net_label = {NETWORKING_LABEL};
    DV_DEV_LABEL              ethernet_label = {ETHERNET_LABEL};
    DV_DEV_HANDLE             *dev_handle;
    NU_DEVICE                 *eth_mw;
    DV_DEVICE_ENTRY           *device;
    ETHERNET_ISR_INFO         *isr_info;
    ETHERNET_SESSION_HANDLE  *ses_handle = (ETHERNET_SESSION_HANDLE *)session_handle;
    ETHERNET_INSTANCE_HANDLE *inst_handle = ses_handle->inst_info;
    INT                       *link_status;
    ETHERNET_CONFIG_PATH      *config_path;

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
                status = PMI_Device_Ioctl((inst_handle->pmi_dev), cmd, data, length,
                                          inst_handle, ses_handle->open_modes);
#endif
                if (status != NU_SUCCESS)
                {

                    /* If the mode requested is supported and if the session was opened for that mode */
                    if ( (ses_handle->open_modes & ETHERNET_OPEN_MODE) &&
                            (DV_COMPARE_LABELS(&ioctl0->label, &net_label)) )
                    {
                        ioctl0->base = IOCTL_NET_BASE;
                        status = NU_SUCCESS;
                    }
    
                    else if ( (ses_handle->open_modes & ETHERNET_OPEN_MODE) &&
                            (DV_COMPARE_LABELS(&ioctl0->label, &ethernet_label)) )
                    {
                        ioctl0->base = IOCTL_ETHERNET_BASE;
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
            /* Ethernet Ioctls */
            /*******************/
        case (IOCTL_ETHERNET_BASE + ETHERNET_CMD_GET_XDATA):

            /* Get the device structure from the data passed in */
            device = (DV_DEVICE_ENTRY *)data;

            /* Save the device to handle */
            ses_handle->device = device;

            /* Attach the ETHERNET extended data to the device structure */
            status = (inst_handle->tgt_fn.Tgt_Create_Extended_Data)(device);

            break;


        case (IOCTL_ETHERNET_BASE + ETHERNET_CMD_SET_DEV_HANDLE):

            /* Get the dev handle from the data passed in */
            dev_handle = (DV_DEV_HANDLE *) data;

            /* Save the handle to the device structure */
            ses_handle->eth_mw.dev_handle = (DV_DEV_HANDLE) * dev_handle;

            break;

        case (IOCTL_ETHERNET_BASE + ETHERNET_CMD_GET_DEV_STRUCT):

            /* Get the NU_DEVICE structure from the data passed in */
            eth_mw = (NU_DEVICE *) data;

            /* Return the populated structure */
            *eth_mw = ses_handle->eth_mw;

            break;

        case (IOCTL_ETHERNET_BASE + ETHERNET_CMD_TARGET_INIT):

            /* Get the device structure from the data passed in */
            device = (DV_DEVICE_ENTRY *)data;

            /* Initialize the target specific portion */
            (inst_handle->tgt_fn.Tgt_Target_Initialize)(inst_handle, device);

            break;

        case (IOCTL_ETHERNET_BASE + ETHERNET_CMD_PHY_INIT):

            /* Get the device structure from the data passed in */
            device = (DV_DEVICE_ENTRY *)data;

            /* Initialize the target specific portion */
            status = (inst_handle->tgt_fn.Tgt_Phy_Initialize)(inst_handle, device);

            break;

        case (IOCTL_ETHERNET_BASE + ETHERNET_CMD_CTRL_INIT):

            /* Get the device structure from the data passed in */
            device = (DV_DEVICE_ENTRY *)data;

            /* Initialize the controller */
            status = (inst_handle->tgt_fn.Tgt_Controller_Init)(inst_handle, device, device->dev_mac_addr);

            break;

        case (IOCTL_ETHERNET_BASE + ETHERNET_CMD_CTRL_ENABLE):

            /* Enable the device */
            (inst_handle->tgt_fn.Tgt_Enable)(ses_handle->inst_info);

            break;

        case (IOCTL_ETHERNET_BASE + ETHERNET_CMD_GET_ISR_INFO):

            /* Get the ISR structure from the data passed in */
            isr_info = (ETHERNET_ISR_INFO *)data;

            /* Get XDATA ISR info */
            (inst_handle->tgt_fn.Tgt_Get_ISR_Info)(ses_handle, isr_info);

            break;

        case (IOCTL_ETHERNET_BASE + ETHERNET_CMD_GET_CONFIG_PATH):
        {
            CHAR                      reg_path[REG_MAX_KEY_LENGTH];

            /* Get the Config Path structure from the data passed in */
            config_path = (ETHERNET_CONFIG_PATH *) data;

            /* Return the middleware config path */
            strncpy(reg_path, inst_handle->config_path, sizeof(reg_path));
            strcat(reg_path, "/mw_settings");
            strncpy(config_path->config_path, reg_path, config_path->max_path_len);

            break;
        }

        case (IOCTL_ETHERNET_BASE + ETHERNET_CMD_SEND_LINK_STATUS):

            /* Send link status */
            (inst_handle->tgt_fn.Tgt_Notify_Status_Change)(inst_handle);

            break;

        case (IOCTL_ETHERNET_BASE + ETHERNET_CMD_GET_LINK_STATUS):

            link_status = (INT *)data;

            /* Get the link status */
            status = (inst_handle->tgt_fn.Tgt_Get_Link_Status)(inst_handle, link_status);

            break;


            /*******************/
            /* Net Ioctls      */
            /*******************/
        case (IOCTL_NET_BASE + ETHERNET_CMD_DEV_ADDMULTI):
        case (IOCTL_NET_BASE + ETHERNET_CMD_DEV_DELMULTI):

            /* Get the device structure from the data passed in */
            device = (DV_DEVICE_ENTRY *)data;
 
            /* Update multicast */
            (inst_handle->tgt_fn.Tgt_Update_Multicast)(device);

            break;

        case (IOCTL_NET_BASE + ETHERNET_CMD_SET_HW_ADDR):

            /* Set the MAC address */
            status = (inst_handle->tgt_fn.Tgt_Set_Address)(inst_handle,
                                              ((DV_REQ *) data)->dvr_dvru.dvru_data);

            break;

        case (IOCTL_NET_BASE + ETHERNET_CMD_GET_HW_ADDR):

            /*Get the MAC Address and set it directly in the Dev_table entry*/
            status = (inst_handle->tgt_fn.Tgt_Get_Address)(inst_handle,
                                              ((DV_REQ *) data)->dvr_dvru.dvru_data);

            break;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

    #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

        case (IOCTL_ETHERNET_BASE + ETHERNET_CMD_PWR_HIB_RESTORE):

            /* Call hibernate restore for serial session. */
            status = (inst_handle->tgt_fn.Tgt_Pwr_Hibernate_Restore)(ses_handle);

            break;

    #endif /* CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE */

        default:

            /* Call the PMI IOCTL function for Power and UII IOCTLs */
            status = PMI_Device_Ioctl((inst_handle->pmi_dev), cmd, data, length,
                                      inst_handle, ses_handle->open_modes);

            break;

#else

        default:

            status = NU_UNAVAILABLE;

            break;

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
    }

    return (status);
}

