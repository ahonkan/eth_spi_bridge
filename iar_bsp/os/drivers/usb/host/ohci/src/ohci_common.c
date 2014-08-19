/*************************************************************************
*
*              Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       ohci_common.c
*
* COMPONENT
*
*       USB OHCI Controller Driver : Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains the wrapper functions for using OHCI driver
*       API's and Power management related logic.
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
*       ohci_common.h
*
**************************************************************************/

/* ==============  USB Include Files =================================== */
#include "drivers/ohci_common.h"

/* =====================  Global data ================================== */
extern NU_MEMORY_POOL       System_Memory;
extern USBH_OHCI_SESSION_HANDLE    *OHCI_Session_Handle;


/**************************************************************************
* FUNCTION
*
*       OHCI_Handle_Initialize
*
* DESCRIPTION
*
*       This function handles initialize ohci IOCTL.
*
* INPUTS
*
*       handle                              Pointer to session handle.
*       ioctl_data                          Pointer to IOCTL data.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completeion of service.
*       NU_USB_INVLD_ARG                    Indicates an invalid argument.
*
**************************************************************************/
STATUS OHCI_Handle_Initialize(USBH_OHCI_SESSION_HANDLE *ohci_handle,
                              VOID *ioctl_data,
                              int ioctl_data_len)
{
    STATUS status;

    NU_USB_PTRCHK(ohci_handle);
    NU_USB_PTRCHK(ioctl_data);

    /* Initialize USB host controller driver. */
    status =  NU_USBH_OHCI_Initialize((NU_USB_HW*) &(ohci_handle->ohci_cb),
                                      (NU_USB_STACK *)ioctl_data);
    if(status == NU_SUCCESS)
    {
        ohci_handle->is_device_init = NU_TRUE;
    }

    return ( status );
}

/**************************************************************************
* FUNCTION
*
*       OHCI_Handle_Uninitialize
*
* DESCRIPTION
*
*       This function handles uninitialize ohci IOCTL.
*
* INPUTS
*
*       handle                              Pointer to session handle.
*       ioctl_data                          Pointer to IOCTL data.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completeion of service.
*       NU_USB_INVLD_ARG                    Indicates an invalid argument.
*
**************************************************************************/
STATUS OHCI_Handle_Uninitialize(USBH_OHCI_SESSION_HANDLE *ohci_handle,
                              VOID *ioctl_data,
                              int ioctl_data_len)
{
    STATUS status;

    /* Initialize USB host controller driver. */
    status =  NU_USBH_OHCI_Uninitialize((NU_USB_HW*) &(ohci_handle->ohci_cb));
    if(status == NU_SUCCESS)
    {
        ohci_handle->is_device_init = NU_FALSE;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       OHCI_Handle_IO_Request
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOTCL_IO_REQUEST.
*
* INPUTS
*
*       ohci_handle                         Pointer to session handle.
*       ioctl_data                          Pointer to data associate
*                                           with IOCTL command.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*
**************************************************************************/
STATUS OHCI_Handle_IO_Request(USBH_OHCI_SESSION_HANDLE   *ohci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    STATUS          status;
    USB_IRP_INFO    *irp_info;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_IRP_INFO) )

    /* Extract pointer to IRP from IOCTL data. */
    irp_info = (USB_IRP_INFO*) ioctl_data;

    /* Submit the IRP finally. */
    status = NU_USBH_OHCI_Submit_IRP( (NU_USB_HW*) &(ohci_handle->ohci_cb),
                                        irp_info->irp,
                                        irp_info->func_addr,
                                        irp_info->endp_addr);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       OHCI_Handle_Open_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOTCL_OPEN_PIPE.
*
* INPUTS
*
*       ohci_handle                         Pointer to session handle.
*       ioctl_data                          Pointer to data associate
*                                           with IOCTL command.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*
**************************************************************************/
STATUS OHCI_Handle_Open_Pipe(USBH_OHCI_SESSION_HANDLE    *ohci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    USB_EP_INFO *ep_info;
    STATUS      status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call open pipe function of OHCI driver. */
    status = NU_USBH_OHCI_Open_Pipe ((NU_USB_HW*) &(ohci_handle->ohci_cb),
                                    ep_info->function_addr,
                                    ep_info->endp_addr,
                                    ep_info->endp_attrib,
                                    ep_info->speed,
                                    ep_info->max_packet_size,
                                    ep_info->interval,
                                    ep_info->load);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       OHCI_Handle_Close_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOTCL_CLOSE_PIPE.
*
* INPUTS
*
*       ohci_handle                         Pointer to session handle.
*       ioctl_data                          Pointer to data associate
*                                           with IOCTL command.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*
**************************************************************************/
STATUS OHCI_Handle_Close_Pipe(USBH_OHCI_SESSION_HANDLE   *ohci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    USB_EP_INFO    *ep_info;
    STATUS          status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call close pipe function of OHCI driver. */
    status = NU_USBH_OHCI_Close_Pipe ((NU_USB_HW*) &(ohci_handle->ohci_cb),
                                    ep_info->function_addr,
                                    ep_info->endp_addr);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       OHCI_Handle_Modify_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOTCL_MODIFY_PIPE.
*
* INPUTS
*
*       ohci_handle                         Pointer to session handle.
*       ioctl_data                          Pointer to data associate
*                                           with IOCTL command.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*
**************************************************************************/
STATUS OHCI_Handle_Modify_Pipe(USBH_OHCI_SESSION_HANDLE  *ohci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    USB_EP_INFO     *ep_info;
    STATUS          status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call modify pipe function of OHCI driver. */
    status = NU_USBH_OHCI_Modify_Pipe ((NU_USB_HW*) &(ohci_handle->ohci_cb),
                                    ep_info->function_addr,
                                    ep_info->endp_addr,
                                    ep_info->endp_attrib,
                                    ep_info->max_packet_size,
                                    ep_info->interval,
                                    ep_info->load);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       OHCI_Handle_Flush_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOTCL_FLUSH_PIPE.
*
* INPUTS
*
*       ohci_handle                         Pointer to session handle.
*       ioctl_data                          Pointer to data associate
*                                           with IOCTL command.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*
**************************************************************************/
STATUS OHCI_Handle_Flush_Pipe(USBH_OHCI_SESSION_HANDLE   *ohci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    USB_EP_INFO     *ep_info;
    STATUS          status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call flush pipe function of OHCI driver. */
    status = NU_USBH_OHCI_Flush_Pipe ((NU_USB_HW*) &(ohci_handle->ohci_cb),
                                    ep_info->function_addr,
                                    ep_info->endp_addr);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       OHCI_Handle_Enable_Int
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOTCL_ENABLE_INT.
*
* INPUTS
*
*       ohci_handle                         Pointer to session handle.
*       ioctl_data                          Pointer to data associate
*                                           with IOCTL command.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*
**************************************************************************/
STATUS OHCI_Handle_Enable_Int(USBH_OHCI_SESSION_HANDLE   *ohci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    STATUS              status;

    ESAL_GE_INT_Enable(ohci_handle->ohci_inst_handle->tgt_info->irq,
                       ohci_handle->ohci_inst_handle->tgt_info->irq_type,
                       ohci_handle->ohci_inst_handle->tgt_info->irq_priority);

    /* Call flush pipe function of OHCI driver. */
    status = NU_USBH_OHCI_Enable_Int ((NU_USB_HW*) &(ohci_handle->ohci_cb) );

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       OHCI_Handle_Disable_Int
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOTCL_DISABLE_INT.
*
* INPUTS
*
*       ohci_handle                         Pointer to session handle.
*       ioctl_data                          Pointer to data associate
*                                           with IOCTL command.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*
**************************************************************************/
STATUS OHCI_Handle_Disable_Int(USBH_OHCI_SESSION_HANDLE  *ohci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    STATUS              status;

    ESAL_GE_INT_Disable(ohci_handle->ohci_inst_handle->tgt_info->irq);

    /* Call flush pipe function of OHCI driver. */
    status = NU_USBH_OHCI_Disable_Int ((NU_USB_HW*) &(ohci_handle->ohci_cb) );

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       OHCI_Handle_Execute_ISR
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOTCL_EXECUTE_ISR.
*
* INPUTS
*
*       ohci_handle                         Pointer to session handle.
*       ioctl_data                          Pointer to data associate
*                                           with IOCTL command.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*
**************************************************************************/
STATUS OHCI_Handle_Execute_ISR(USBH_OHCI_SESSION_HANDLE  *ohci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    STATUS              status;

    NU_USB_PTRCHK(ioctl_data == NU_NULL);
    NU_USB_ASSERT( ioctl_data_len == 0 );

    ESAL_GE_INT_Disable(ohci_handle->ohci_inst_handle->tgt_info->irq);

    /* Call flush pipe function of OHCI driver. */
    status = NU_USBH_OHCI_ISR ((NU_USB_HW*) &(ohci_handle->ohci_cb) );

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
* FUNCTION
*
*       OHCI_Handle_Get_CB
*
* DESCRIPTION
*
*       This function handles get contril block IOCTL.
*
* INPUTS
*
*       handle                              Pointer to session handle.
*       ioctl_data                          Pointer to IOCTL data.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completeion of service.
*       NU_USB_INVLD_ARG                    Indicates an invalid argument.
*
**************************************************************************/
STATUS OHCI_Handle_Get_CB(USBH_OHCI_SESSION_HANDLE *ohci_handle,
                          VOID **ioctl_data,
                          INT ioctl_data_len)
{
    NU_USB_PTRCHK(ohci_handle);
    NU_USB_PTRCHK(ioctl_data);

    /* Get ohci control block. */
    *ioctl_data = &(ohci_handle->ohci_cb);

    /* Return status of progress. */
    return( NU_SUCCESS );
}

/**************************************************************************
* FUNCTION
*
*       OHCI_Handle_Get_Speed
*
* DESCRIPTION
*
*       This function handles get speed IOCTL
*
* INPUTS
*
*       handle                              Pointer to session handle.
*       ioctl_data                          Pointer to IOCTL data.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completeion of service.
*       NU_USB_INVLD_ARG                    Indicates an invalid argument.
*
**************************************************************************/
STATUS OHCI_Handle_Get_Speed(USBH_OHCI_SESSION_HANDLE *ohci_handle,
                             VOID *ioctl_data,
                             INT ioctl_data_len)
{
    NU_USB_PTRCHK(ohci_handle);
    NU_USB_PTRCHK(ioctl_data);

    if ( ioctl_data_len == sizeof(UINT8) )
    {
        *(UINT8 *)ioctl_data = ((NU_USB_HW *) &(ohci_handle->ohci_cb))->speed;
    }

    return ( NU_SUCCESS );
}

/**************************************************************************
* FUNCTION
*
*       OHCI_Handle_Is_Current_Available
*
* DESCRIPTION
*
*       This function handles the request to check that if enough current
*       is available for the newly connected device.
*
* INPUTS
*
*       handle                              Pointer to session handle.
*       ioctl_data                          Pointer to IOCTL data.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completeion of service.
*       NU_USB_INVLD_ARG                    Indicates an invalid argument.
*
**************************************************************************/
STATUS OHCI_Handle_Is_Current_Available(USBH_OHCI_SESSION_HANDLE *ohci_handle,
                                        VOID *ioctl_data,
                                        INT   ioctl_data_len)
{
    USB_DEV_CURRENT_INFO    *dev_current_info;
    NU_USB_DEVICE           *device;
    NU_USB_DEVICE           *parent_device;
    NU_USBH_OHCI            *ohci;
    STATUS      status = NU_USB_INVLD_ARG;
    BOOLEAN     is_self_powered = NU_FALSE;
    BOOLEAN     power_from_root_hub = NU_TRUE;
    UINT32      current_required;
    UINT8       cfg_num;

    NU_USB_PTRCHK(ohci_handle);
    NU_USB_PTRCHK(ioctl_data);

    /* Get OHCI control block. */
    ohci = &ohci_handle->ohci_cb;
    dev_current_info = (USB_DEV_CURRENT_INFO *)ioctl_data;
    device = dev_current_info->device;
    cfg_num = dev_current_info->cfg;
    dev_current_info->is_current_available = NU_FALSE;
    device->current_drawn = 0;

    if ( ioctl_data_len == sizeof(USB_DEV_CURRENT_INFO) )
    {
        /* If device is root hub then no need to check power requirement. */
        if(USBH_IS_ROOT_HUB (device))
        {
            dev_current_info->is_current_available = NU_TRUE;
            status = NU_SUCCESS;
        }
        else
        {
            /* Check if newly connected device is a self powered or bus powered. */
            status = USBH_Is_Device_Self_Powered((NU_USBH_STACK *)device->stack,
                                                 device, &is_self_powered);
            if(status == NU_SUCCESS)
            {
                dev_current_info->is_current_available = NU_TRUE;

                /* If device is not self powered then check its parent. */
                if(!is_self_powered)
                {
                    parent_device = device->parent;

                    /* Check all parent device until you reach to root hub or find a self powered
                      device (hub) in between. */
                    while(!USBH_IS_ROOT_HUB (parent_device))
                    {
                        status = USBH_Is_Device_Self_Powered((NU_USBH_STACK *)parent_device->stack,
                                                             parent_device, &is_self_powered);

                        /* If parent is self powered device (hub) but not root hub then device
                           doesn't need power from controller. */
                        if((status == NU_SUCCESS) && (is_self_powered))
                        {
                            power_from_root_hub = NU_FALSE;
                            break;
                        }
                        parent_device = parent_device->parent;
                    }

                    /* If power is required from root hub then get it. */
                    if(power_from_root_hub == NU_TRUE)
                    {
                        /* Retrieve the exact power requirement from configuration descriptor. */
                        status = NU_USB_DEVICE_Get_Current_Requirement(device, cfg_num,
                                                                   &current_required);

                        /* Check if controller can supply the required current.  According to
                           USB 2.0 Bus specification section 9.6.3, table 9.10 multiply current
                           required with 2 to find the exact current requirment for the bus powered
                           device. */
                        if ((status == NU_SUCCESS) &&
                            ((current_required * 2) <= (ohci->available_current)))
                        {
                            /* If required power is available then update OHCI and device
                               control blocks. */
                            ohci->available_current -= (current_required * 2);
                            device->current_drawn = current_required * 2;
                        }
                        else
                        {
                            dev_current_info->is_current_available = NU_FALSE;
                        }
                    }
                }
            }
        }
    }

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       OHCI_Handle_Release_Power
*
* DESCRIPTION
*
*       This function releases the power which was acquired by the USB
*       device during set configuration.
*
* INPUTS
*
*       handle                              Pointer to session handle.
*       ioctl_data                          Pointer to IOCTL data.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completeion of service.
*       NU_USB_INVLD_ARG                    Indicates an invalid argument.
*
**************************************************************************/
STATUS OHCI_Handle_Release_Power(USBH_OHCI_SESSION_HANDLE *ohci_handle,
                                 VOID *ioctl_data,
                                 INT   ioctl_data_len)
{
    USB_DEV_CURRENT_INFO    *dev_current_info;
    NU_USB_DEVICE           *device;
    NU_USBH_OHCI            *ohci;
    STATUS      status = NU_USB_INVLD_ARG;

    NU_USB_PTRCHK(ohci_handle);
    NU_USB_PTRCHK(ioctl_data);

    ohci = &ohci_handle->ohci_cb;
    dev_current_info = (USB_DEV_CURRENT_INFO *)ioctl_data;
    device = dev_current_info->device;

    if ( ioctl_data_len == sizeof(USB_DEV_CURRENT_INFO) )
    {
        ohci->available_current += device->current_drawn;
        device->current_drawn = 0;
        status = NU_SUCCESS;
    }

    return (status);
}


/**************************************************************************
* FUNCTION
*
*       OHCI_Handle_Request_Power_Down
*
* DESCRIPTION
*
*       This function handles Request Power Down Mode IOCTL
*
* INPUTS
*
*       handle                              Pointer to session handle.
*       ioctl_data                          Pointer to IOCTL data.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completeion of service.
*       NU_USB_INVLD_ARG                    Indicates an invalid argument.
*
**************************************************************************/
STATUS OHCI_Handle_Request_Power_Down(USBH_OHCI_SESSION_HANDLE *ohci_handle,
                             VOID *ioctl_data,
                             INT ioctl_data_len)
{

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    USBH_OHCI_INSTANCE_HANDLE   *inst_handle;
    VOID                        (*cleanup_func)(VOID);

    inst_handle = ohci_handle->ohci_inst_handle;

    if ( PMI_STATE_GET(inst_handle->pmi_dev) == OHCI_OFF )
    {
        if ( inst_handle->cleanup_func != NU_NULL )
        {
            /* Disable clock for OHCI controller. */
            cleanup_func = inst_handle->cleanup_func;
           (VOID)cleanup_func();
           if(OHCI_Session_Handle != NU_NULL)
            {
                OHCI_Session_Handle->is_clock_enabled = NU_FALSE;
            }
        }
    }
#endif

    return ( NU_SUCCESS );
}

/*************************************************************************
*
*   FUNCTION
*
*
*    OHCII_Get_Target_Info
*
*   DESCRIPTION
*
*       This function retrieves OHCI HW information.
*
*   INPUT
*
*       USBH_OHCI_TGT_INFO                  OHCI target info control block.
*       key                                 Registry path
*
*   OUTPUT
*
*       NU_SUCCESS                          Operation completed successfully.
*
*************************************************************************/
STATUS OHCI_Get_Target_Info(const CHAR * key, USBH_OHCI_TGT_INFO *usbh_ohci_tgt)
{
    /* Get values from the registry */
    REG_Get_String_Value (key, "/tgt_settings/dev_name", usbh_ohci_tgt->name, sizeof(usbh_ohci_tgt->name));
    REG_Get_UINT32_Value (key, "/tgt_settings/base", (UINT32*)&usbh_ohci_tgt->base_address);
    REG_Get_UINT32_Value (key, "/tgt_settings/intr_vector", (UINT32*)&usbh_ohci_tgt->irq);
    REG_Get_UINT32_Value (key, "/tgt_settings/priority", (UINT32*)&usbh_ohci_tgt->irq_priority);
    REG_Get_UINT32_Value (key, "/tgt_settings/trigger_type", (UINT32*)&usbh_ohci_tgt->irq_type);
    REG_Get_UINT32_Value (key, "/tgt_settings/total_current", (UINT32*)&usbh_ohci_tgt->total_current);

    return ( NU_SUCCESS );
}

/* ======================== End of File ================================ */
