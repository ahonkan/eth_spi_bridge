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
*       ehci_common.c
*
* COMPONENT
*
*       USB EHCI Controller Driver : Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains the wrapper functions for using EHCI driver
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
*       ehci_common.h
*
**************************************************************************/

/* ==============  USB Include Files =================================== */
#include "drivers/ehci_common.h"

/* =====================  Global data ================================== */
extern NU_MEMORY_POOL       System_Memory;
extern USBH_EHCI_SESSION_HANDLE    *EHCI_Session_Handle;

/**************************************************************************
* FUNCTION
*
*       EHCI_Handle_Initialize
*
* DESCRIPTION
*
*       This function handles initialize ehci IOCTL.
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
STATUS EHCI_Handle_Initialize(USBH_EHCI_SESSION_HANDLE *ehci_handle,
                              VOID *ioctl_data,
                              int ioctl_data_len)
{
    STATUS status;

    NU_USB_PTRCHK(ehci_handle);
    NU_USB_PTRCHK(ioctl_data);

    /* Initialize USB function controller driver. */
    status =  NU_USBH_EHCI_Initialize((NU_USB_HW*) &(ehci_handle->ehci_cb),
                                      (NU_USB_STACK *)ioctl_data);

    if ( status == NU_SUCCESS )
    {
        ehci_handle->is_device_init = NU_TRUE;
    }

    return ( status );
}

/**************************************************************************
* FUNCTION
*
*       EHCI_Handle_Uninitialize
*
* DESCRIPTION
*
*       This function handles uninitialize ehci IOCTL.
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
STATUS EHCI_Handle_Uninitialize(USBH_EHCI_SESSION_HANDLE *ehci_handle,
                              VOID *ioctl_data,
                              int ioctl_data_len)
{
    STATUS status;

    /* Initialize USB function controller driver. */
    status =  NU_USBH_EHCI_Uninitialize((NU_USB_HW*) &(ehci_handle->ehci_cb));
    if ( status == NU_SUCCESS )
    {
        ehci_handle->is_device_init = NU_FALSE;
    }

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       EHCI_Handle_IO_Request
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOTCL_IO_REQUEST.
*
* INPUTS
*
*       ehci_handle                         Pointer to session handle.
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
STATUS EHCI_Handle_IO_Request(USBH_EHCI_SESSION_HANDLE   *ehci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    STATUS          status;
    USB_IRP_INFO    *irp_info;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_IRP_INFO) )

    /* Extract pointer to IRP info from IOCTL data. */
    irp_info = (USB_IRP_INFO*) ioctl_data;

    /* Submit the IRP finally. */
    status = NU_USBH_EHCI_Submit_IRP((NU_USB_HW*) &(ehci_handle->ehci_cb),
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
*       EHCI_Handle_Open_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_OPEN_PIPE.
*
* INPUTS
*
*       ehci_handle                         Pointer to session handle.
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
STATUS EHCI_Handle_Open_Pipe(USBH_EHCI_SESSION_HANDLE    *ehci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    USB_EP_INFO *ep_info;
    STATUS      status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call open pipe function of EHCI driver. */
    status = NU_USBH_EHCI_Open_Pipe ((NU_USB_HW*) &(ehci_handle->ehci_cb),
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
*       EHCI_Handle_Close_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOTCL_CLOSE_PIPE.
*
* INPUTS
*
*       ehci_handle                         Pointer to session handle.
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
STATUS EHCI_Handle_Close_Pipe(USBH_EHCI_SESSION_HANDLE   *ehci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    USB_EP_INFO *ep_info;
    STATUS      status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call close pipe function of EHCI driver. */
    status = NU_USBH_EHCI_Close_Pipe ((NU_USB_HW*) &(ehci_handle->ehci_cb),
                                      ep_info->function_addr,
                                      ep_info->endp_addr);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       EHCI_Handle_Modify_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_MODIFY_PIPE.
*
* INPUTS
*
*       ehci_handle                         Pointer to session handle.
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
STATUS EHCI_Handle_Modify_Pipe(USBH_EHCI_SESSION_HANDLE  *ehci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    USB_EP_INFO *ep_info;
    STATUS      status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call modify pipe function of EHCI driver. */
    status = NU_USBH_EHCI_Modify_Pipe((NU_USB_HW*) &(ehci_handle->ehci_cb),
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
*       EHCI_Handle_Flush_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_FLUSH_PIPE.
*
* INPUTS
*
*       ehci_handle                         Pointer to session handle.
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
STATUS EHCI_Handle_Flush_Pipe(USBH_EHCI_SESSION_HANDLE   *ehci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    USB_EP_INFO *ep_info;
    STATUS      status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call flush pipe function of EHCI driver. */
    status = NU_USBH_EHCI_Flush_Pipe ((NU_USB_HW*) &(ehci_handle->ehci_cb),
                                      ep_info->function_addr,
                                      ep_info->endp_addr);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       EHCI_Handle_Enable_Int
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_ENABLE_INT.
*
* INPUTS
*
*       ehci_handle                         Pointer to session handle.
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
STATUS EHCI_Handle_Enable_Int(USBH_EHCI_SESSION_HANDLE   *ehci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    STATUS              status;

    ESAL_GE_INT_Enable(ehci_handle->ehci_inst_handle->tgt_info->irq,
                       ehci_handle->ehci_inst_handle->tgt_info->irq_type,
                       ehci_handle->ehci_inst_handle->tgt_info->irq_priority);

    /* Call flush pipe function of EHCI driver. */
    status = NU_USBH_EHCI_Enable_Int((NU_USB_HW*) &(ehci_handle->ehci_cb));

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       EHCI_Handle_Disable_Int
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_DISABLE_INT.
*
* INPUTS
*
*       ehci_handle                         Pointer to session handle.
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
STATUS EHCI_Handle_Disable_Int(USBH_EHCI_SESSION_HANDLE  *ehci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    STATUS              status;

    ESAL_GE_INT_Disable(ehci_handle->ehci_inst_handle->tgt_info->irq);

    /* Call flush pipe function of EHCI driver. */
    status = NU_USBH_EHCI_Disable_Int((NU_USB_HW*) &(ehci_handle->ehci_cb));

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       EHCI_Handle_Execute_ISR
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_EXECUTE_ISR.
*
* INPUTS
*
*       ehci_handle                         Pointer to session handle.
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
STATUS EHCI_Handle_Execute_ISR(USBH_EHCI_SESSION_HANDLE  *ehci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    STATUS              status;

    NU_USB_PTRCHK(ioctl_data == NU_NULL);
    NU_USB_ASSERT( ioctl_data_len == 0 );

    ESAL_GE_INT_Disable(ehci_handle->ehci_inst_handle->tgt_info->irq);

    /* Call flush pipe function of EHCI driver. */
    status = NU_USBH_EHCI_ISR ((NU_USB_HW*) &(ehci_handle->ehci_cb) );

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
* FUNCTION
*
*       EHCI_Handle_Get_CB
*
* DESCRIPTION
*
*       This function handles get control block IOCTL.
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
STATUS EHCI_Handle_Get_CB(USBH_EHCI_SESSION_HANDLE *ehci_handle,
                          VOID **ioctl_data,
                          int ioctl_data_len)
{
    NU_USB_PTRCHK(ehci_handle);
    NU_USB_PTRCHK(ioctl_data);

    /* Get ehci control block. */
    *ioctl_data = &(ehci_handle->ehci_cb);

    /* Return status of progress. */
    return( NU_SUCCESS );
}

/**************************************************************************
* FUNCTION
*
*       EHCI_Handle_Get_Speed
*
* DESCRIPTION
*
*       This function handles get speed IOCTL.
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
*                                           completion of service.
*       NU_USB_INVLD_ARG                    Indicates an invalid argument.
*
**************************************************************************/
STATUS EHCI_Handle_Get_Speed(USBH_EHCI_SESSION_HANDLE *ehci_handle,
                             VOID *ioctl_data,
                             INT ioctl_data_len)
{
    NU_USB_PTRCHK(ehci_handle);
    NU_USB_PTRCHK(ioctl_data);

    if ( ioctl_data_len == sizeof(UINT8) )
    {
        *(UINT8 *)ioctl_data = ((NU_USB_HW *) &(ehci_handle->ehci_cb))->speed;
    }

    return ( NU_SUCCESS );
}

/**************************************************************************
* FUNCTION
*
*       EHCI_Handle_Is_Current_Available
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
*                                           completion of service.
*       NU_USB_INVLD_ARG                    Indicates an invalid argument.
*
**************************************************************************/
STATUS EHCI_Handle_Is_Current_Available(USBH_EHCI_SESSION_HANDLE *ehci_handle,
                                        VOID *ioctl_data,
                                        INT   ioctl_data_len)
{
    USB_DEV_CURRENT_INFO    *dev_current_info;
    NU_USB_DEVICE           *device;
    NU_USB_DEVICE           *parent_device;
    NU_USBH_EHCI            *ehci;
    STATUS      status = NU_USB_INVLD_ARG;
    BOOLEAN     is_self_powered = NU_FALSE;
    BOOLEAN     power_from_root_hub = NU_TRUE;
    UINT32      current_required;
    UINT8       cfg_num;

    NU_USB_PTRCHK(ehci_handle);
    NU_USB_PTRCHK(ioctl_data);

    /* Get EHCI control block. */
    ehci = &ehci_handle->ehci_cb;
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
                           required with 2 to find the exact current requirement for the bus powered
                           device. */
                        if ((status == NU_SUCCESS) &&
                            ((current_required * 2) <= (ehci->available_current)))
                        {
                            /* If required power is available then update EHCI and device
                               control blocks. */
                            ehci->available_current -= (current_required * 2);
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
*       EHCI_Handle_Release_Power
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
*                                           completion of service.
*       NU_USB_INVLD_ARG                    Indicates an invalid argument.
*
**************************************************************************/
STATUS EHCI_Handle_Release_Power(USBH_EHCI_SESSION_HANDLE *ehci_handle,
                                 VOID *ioctl_data,
                                 INT   ioctl_data_len)
{
    USB_DEV_CURRENT_INFO    *dev_current_info;
    NU_USB_DEVICE           *device;
    NU_USBH_EHCI            *ehci;
    STATUS      status = NU_USB_INVLD_ARG;

    NU_USB_PTRCHK(ehci_handle);
    NU_USB_PTRCHK(ioctl_data);

    ehci = &ehci_handle->ehci_cb;
    dev_current_info = (USB_DEV_CURRENT_INFO *)ioctl_data;
    device = dev_current_info->device;

    if ( ioctl_data_len == sizeof(USB_DEV_CURRENT_INFO) )
    {
        ehci->available_current += device->current_drawn;
        device->current_drawn = 0;
        status = NU_SUCCESS;
    }

    return (status);
}


/**************************************************************************
* FUNCTION
*
*       EHCI_Handle_Request_Power_Down
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
STATUS EHCI_Handle_Request_Power_Down(USBH_EHCI_SESSION_HANDLE *ehci_handle,
                             VOID *ioctl_data,
                             INT ioctl_data_len)
{

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    USBH_EHCI_INSTANCE_HANDLE   *inst_handle;
    VOID                        (*cleanup_func)(VOID);

    inst_handle = ehci_handle->ehci_inst_handle;

    if ( PMI_STATE_GET(inst_handle->pmi_dev) == EHCI_OFF )
    {
        if ( inst_handle->cleanup_func != NU_NULL )
        {
            /* Disable clock for EHCI controller. */
            cleanup_func = inst_handle->cleanup_func;
           (void)cleanup_func();
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
*    EHCII_Get_Target_Info
*
*   DESCRIPTION
*
*       This function retrieves EHCI HW information.
*
*   INPUT
*
*       USBH_EHCI_TGT_INFO                  EHCI target info control block.
*       key                                 Registry path
*
*   OUTPUT
*
*       NU_SUCCESS                          Operation completed successfully.
*
*************************************************************************/
STATUS EHCI_Get_Target_Info(const CHAR * key, USBH_EHCI_TGT_INFO *usbh_ehci_tgt)
{
    /* Get values from the registry */
    REG_Get_String_Value (key, "/tgt_settings/dev_name", usbh_ehci_tgt->name, sizeof(usbh_ehci_tgt->name));
    REG_Get_UINT32_Value (key, "/tgt_settings/base", (UINT32*)&usbh_ehci_tgt->base_address);
    REG_Get_UINT32_Value (key, "/tgt_settings/intr_vector", (UINT32*)&usbh_ehci_tgt->irq);
    REG_Get_UINT32_Value (key, "/tgt_settings/priority", (UINT32*)&usbh_ehci_tgt->irq_priority);
    REG_Get_UINT32_Value (key, "/tgt_settings/trigger_type", (UINT32*)&usbh_ehci_tgt->irq_type);
    REG_Get_UINT32_Value (key, "/tgt_settings/total_current", (UINT32*)&usbh_ehci_tgt->total_current);

    return ( NU_SUCCESS );
}

/* ======================== End of File ================================ */
