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
*       xhci_common.c
*
* COMPONENT
*
*       USB XHCI Controller Driver : Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains the wrapper functions for using XHCI driver
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
*       xhci_common.h
*       pci_extr.h
*
**************************************************************************/
/*********************************/
/* INCLUDE FILES                 */
/*********************************/
#include "drivers/xhci_common.h"

#ifdef CFG_NU_OS_CONN_PCI_COMMON_ENABLE
#include "connectivity/pci_extr.h"
#endif

extern USBH_XHCI_SESSION_HANDLE   *XHCI_Session_Handle;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
static STATUS USBH_XHCI_Enable (VOID*);
static STATUS USBH_XHCI_Disable (VOID *);
static STATUS USBH_XHCI_Sleep (VOID*);
static VOID USBH_XHCI_Enable_VBUS (VOID*);
static VOID USBH_XHCI_Disable_VBUS (VOID*);
static STATUS USBH_XHCI_Port_Resume (VOID *);
#endif
#ifdef CFG_NU_OS_CONN_PCI_COMMON_ENABLE
static STATUS Get_XHCI_Info_From_PCI(USBH_XHCI_TGT_INFO *);
#endif


/**************************************************************************
* FUNCTION
*
*       XHCI_Handle_Initialize
*
* DESCRIPTION
*
*       This function handles initialize XHCI IOCTL.
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
STATUS XHCI_Handle_Initialize(USBH_XHCI_SESSION_HANDLE *xhci_handle,
                              VOID *ioctl_data,
                              int ioctl_data_len)
{
    STATUS status;

    NU_USB_PTRCHK(xhci_handle);
    NU_USB_PTRCHK(ioctl_data);

    /* Disable the global interrupt. */
    ESAL_GE_INT_Disable(xhci_handle->xhci_inst_handle->tgt_info->irq);

    /* Initialize USB function controller driver. */
    status =  NU_USBH_XHCI_Initialize((NU_USB_HW*) &(xhci_handle->xhci_cb),
                                      (NU_USB_STACK *)ioctl_data);

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_IO_Request
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_IO_REQUEST.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_IO_Request(USBH_XHCI_SESSION_HANDLE   *xhci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    USB_IRP_INFO      *irp_info;
    STATUS          status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_IRP_INFO) )

    /* Extract pointer to IRP info from IOCTL data. */
    irp_info = (USB_IRP_INFO *) ioctl_data;

    /* Submit the IRP finally. */
    status = NU_USBH_XHCI_Submit_IRP((NU_USB_HW*) &(xhci_handle->xhci_cb),
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
*       XHCI_Handle_Open_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_OPEN_PIPE.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_Open_Pipe(USBH_XHCI_SESSION_HANDLE    *xhci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    USB_EP_INFO *ep_info;
    STATUS      status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call open pipe function of XHCI driver. */
    status = NU_USBH_XHCI_Open_Pipe((NU_USB_HW*) &(xhci_handle->xhci_cb),
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
*       XHCI_Handle_Close_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_CLOSE_PIPE.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_Close_Pipe(USBH_XHCI_SESSION_HANDLE   *xhci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    USB_EP_INFO *ep_info;
    STATUS      status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call close pipe function of XHCI driver. */
    status = NU_USBH_XHCI_Close_Pipe ((NU_USB_HW*) &(xhci_handle->xhci_cb),
                                      ep_info->function_addr,
                                      ep_info->endp_addr);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Modify_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_MODIFY_PIPE.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_Modify_Pipe(USBH_XHCI_SESSION_HANDLE  *xhci_handle,
                               VOID                      *ioctl_data,
                               INT                        ioctl_data_len)
{
    USB_EP_INFO *ep_info;
    STATUS      status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call modify pipe function of XHCI driver. */
    status = NU_USBH_XHCI_Modify_Pipe((NU_USB_HW*) &(xhci_handle->xhci_cb),
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
*       XHCI_Handle_Flush_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_FLUSH_PIPE.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_Flush_Pipe(USBH_XHCI_SESSION_HANDLE   *xhci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    USB_EP_INFO *ep_info;
    STATUS      status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call flush pipe function of XHCI driver. */
    status = NU_USBH_XHCI_Flush_Pipe ((NU_USB_HW*) &(xhci_handle->xhci_cb),
                                      ep_info->function_addr,
                                      ep_info->endp_addr);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Enable_Int
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_ENABLE_INT.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_Enable_Int(USBH_XHCI_SESSION_HANDLE   *xhci_handle,
                              VOID                  *ioctl_data,
                              INT                   ioctl_data_len)
{
    STATUS              status;

    ESAL_GE_INT_Enable(xhci_handle->xhci_inst_handle->tgt_info->irq,
                       xhci_handle->xhci_inst_handle->tgt_info->irq_type,
                       xhci_handle->xhci_inst_handle->tgt_info->irq_priority);

    /* Call flush pipe function of XHCI driver. */
    status = NU_USBH_XHCI_Enable_Interrupts((NU_USB_HW*) &(xhci_handle->xhci_cb));

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Disable_Int
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_DISABLE_INT.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_Disable_Int(USBH_XHCI_SESSION_HANDLE  *xhci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    STATUS              status;

    ESAL_GE_INT_Disable(xhci_handle->xhci_inst_handle->tgt_info->irq);

    /* Call flush pipe function of XHCI driver. */
    status = NU_USBH_XHCI_Disable_Interrupts((NU_USB_HW*) &(xhci_handle->xhci_cb));

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Execute_ISR
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_EXECUTE_ISR.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_Execute_ISR(USBH_XHCI_SESSION_HANDLE  *xhci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    STATUS              status;

    NU_USB_PTRCHK(ioctl_data == NU_NULL);
    NU_USB_ASSERT( ioctl_data_len == 0 );

    /* Call flush pipe function of XHCI driver. */
    status = NU_USBH_XHCI_ISR ((NU_USB_HW*) &(xhci_handle->xhci_cb) );

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Open_SS_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_EXECUTE_ISR.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_Open_SS_Pipe(USBH_XHCI_SESSION_HANDLE  *xhci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    STATUS       status;
    USB_EP_INFO *ep_info;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call flush pipe function of XHCI driver. */
    status = NU_USBH_XHCI_Open_SS_Pipe((NU_USB_HW*) &(xhci_handle->xhci_cb),
                                       ep_info->function_addr,
                                       ep_info->endp_addr,
                                       ep_info->endp_attrib,
                                       ep_info->speed,
                                       ep_info->max_packet_size,
                                       ep_info->interval,
                                       ep_info->load,
                                       ep_info->max_burst,
                                       ep_info->ep_comp_attrib,
                                       ep_info->bytes_per_interval);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Update_device
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_EXECUTE_ISR.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_Update_device(USBH_XHCI_SESSION_HANDLE  *xhci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    STATUS status;
    USB_DEV_SS_PARAMS *device_params;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_DEV_SS_PARAMS) );

    device_params = (USB_DEV_SS_PARAMS *) ioctl_data;

    /* Call flush pipe function of XHCI driver. */
    status = NU_USBH_XHCI_Update_device((NU_USBH_HW*) &(xhci_handle->xhci_cb),
                                        device_params->device,
                                        device_params->packet_size,
                                        device_params->sel,
                                        device_params->is_hub,
                                        device_params->tt_time,
                                        device_params->num_ports);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Initialize_Device
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_EXECUTE_ISR.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_Initialize_Device(USBH_XHCI_SESSION_HANDLE  *xhci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    STATUS status;

    NU_USB_PTRCHK(ioctl_data == NU_NULL);
    NU_USB_ASSERT(ioctl_data_len == 0);

    /* Call flush pipe function of XHCI driver. */
    status = NU_USBH_XHCI_Initialize_Device((NU_USBH_HW*) &(xhci_handle->xhci_cb) );

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Uninitialize_Device
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_EXECUTE_ISR.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_Uninitialize_Device(USBH_XHCI_SESSION_HANDLE  *xhci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    STATUS   status;
    UINT8   *address;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT(ioctl_data_len == sizeof(UINT8));

    address = (UINT8*) ioctl_data;

    /* Call flush pipe function of XHCI driver. */
    status = NU_USBH_XHCI_Uninitialize_Device((NU_USBH_HW*) &(xhci_handle->xhci_cb),
                                              *address);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Unstall_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_EXECUTE_ISR.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_Unstall_Pipe(USBH_XHCI_SESSION_HANDLE  *xhci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    STATUS       status;
    USB_EP_INFO *ep_info;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT(ioctl_data_len == sizeof(USB_EP_INFO));

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call flush pipe function of XHCI driver. */
    status = NU_USBH_XHCI_Unstall_Pipe((NU_USBH_HW*)&(xhci_handle->xhci_cb),
                                        ep_info->endp_addr,ep_info->function_addr);
    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Disable_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_EXECUTE_ISR.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_Disable_Pipe(USBH_XHCI_SESSION_HANDLE  *xhci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    STATUS       status;
    USB_EP_INFO *ep_info;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT(ioctl_data_len == sizeof(USB_EP_INFO));

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call flush pipe function of XHCI driver. */
    status = NU_USBH_XHCI_Disable_Pipe ((NU_USBH_HW*) &(xhci_handle->xhci_cb),
                                        ep_info->endp_addr,
                                        ep_info->function_addr);

    /* Return status of execution. */
    return ( status );
}
/**************************************************************************
*
* FUNCTION
*
*       XHCI_Handle_Reset_Bandwidth
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_EXECUTE_ISR.
*
* INPUTS
*
*       xhci_handle                         Pointer to session handle.
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
STATUS XHCI_Handle_Reset_Bandwidth(USBH_XHCI_SESSION_HANDLE  *xhci_handle,
                               VOID                  *ioctl_data,
                               INT                   ioctl_data_len)
{
    STATUS   status;
    UINT8   *address;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT(ioctl_data_len == sizeof(UINT8));

    address = (UINT8*) ioctl_data;

    /* Call flush pipe function of XHCI driver. */
    status = NU_USBH_XHCI_Reset_Bandwidth((NU_USBH_HW*) &(xhci_handle->xhci_cb),
                                          *address);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
* FUNCTION
*
*       XHCI_Handle_Get_CB
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
*                                           completion of service.
*       NU_USB_INVLD_ARG                    Indicates an invalid argument.
*
**************************************************************************/
STATUS XHCI_Handle_Get_CB(USBH_XHCI_SESSION_HANDLE *xhci_handle,
                          VOID **ioctl_data,
                          int ioctl_data_len)
{
    NU_USB_PTRCHK(xhci_handle);
    NU_USB_PTRCHK(ioctl_data);

    /* Get xhci control block. */
    *ioctl_data = &(xhci_handle->xhci_cb);

    /* Return status of progress. */
    return( NU_SUCCESS );
}

/**************************************************************************
* FUNCTION
*
*       XHCI_Handle_Get_Speed
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
STATUS XHCI_Handle_Get_Speed(USBH_XHCI_SESSION_HANDLE *xhci_handle,
                             VOID *ioctl_data,
                             INT ioctl_data_len)
{
    NU_USB_PTRCHK(xhci_handle);
    NU_USB_PTRCHK(ioctl_data);

    if ( ioctl_data_len == sizeof(UINT8) )
    {
        *(UINT8 *)ioctl_data = ((NU_USB_HW *) &(xhci_handle->xhci_cb))->speed;
    }

    return ( NU_SUCCESS );
}
/**************************************************************************
* FUNCTION
*
*       XHCI_Handle_Is_Current_Available
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
STATUS XHCI_Handle_Is_Current_Available(USBH_XHCI_SESSION_HANDLE *xhci_handle,
                                        VOID *ioctl_data,
                                        INT   ioctl_data_len)
{
    USB_DEV_CURRENT_INFO    *dev_current_info;
    NU_USB_DEVICE           *device;
    NU_USB_DEVICE           *parent_device;
    NU_USBH_XHCI            *xhci;
    STATUS      status = NU_USB_INVLD_ARG;
    BOOLEAN     is_self_powered = NU_FALSE;
    BOOLEAN     power_from_root_hub = NU_TRUE;
    UINT32      current_required;
    UINT8       cfg_num;

    NU_USB_PTRCHK(xhci_handle);
    NU_USB_PTRCHK(ioctl_data);

    /* Get XHCI control block. */
    xhci = &xhci_handle->xhci_cb;
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
                            ((current_required * 2) <= (xhci->available_current)))
                        {
                            /* If required power is available then update XHCI and device
                               control blocks. */
                            xhci->available_current -= (current_required * 2);
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
*       XHCI_Handle_Release_Power
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
STATUS XHCI_Handle_Release_Power(USBH_XHCI_SESSION_HANDLE *xhci_handle,
                                 VOID *ioctl_data,
                                 INT   ioctl_data_len)
{
    USB_DEV_CURRENT_INFO    *dev_current_info;
    NU_USB_DEVICE           *device;
    NU_USBH_XHCI            *xhci;
    STATUS      status = NU_USB_INVLD_ARG;

    NU_USB_PTRCHK(xhci_handle);
    NU_USB_PTRCHK(ioctl_data);

    xhci = &xhci_handle->xhci_cb;
    dev_current_info = (USB_DEV_CURRENT_INFO *)ioctl_data;
    device = dev_current_info->device;

    if ( ioctl_data_len == sizeof(USB_DEV_CURRENT_INFO) )
    {
        xhci->available_current += device->current_drawn;
        device->current_drawn = 0;
        status = NU_SUCCESS;
    }

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       XHCI_Handle_Request_Power_Down
*
* DESCRIPTION
*
*       This function handles Request Power Down Mode IOCTL.
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
STATUS XHCI_Handle_Request_Power_Down(USBH_XHCI_SESSION_HANDLE *xhci_handle,
                                      VOID *ioctl_data,
                                      INT ioctl_data_len)
{
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    USBH_XHCI_INSTANCE_HANDLE   *inst_handle;
    VOID                        (*cleanup_func)(VOID);
    inst_handle = xhci_handle->xhci_inst_handle;

    if ( PMI_STATE_GET(inst_handle->pmi_dev) == XHCI_OFF )
    {
        if ( inst_handle->cleanup_func != NU_NULL )
        {
            /* Disable clock for XHCI controller. */
            cleanup_func = inst_handle->cleanup_func;
            (void)cleanup_func();
        }
    }

#endif

    return ( NU_SUCCESS );
}
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
/*************************************************************************
*
*   FUNCTION
*
*       USBH_XHCI_Set_State
*
*   DESCRIPTION
*
*       This is the IOCTL function that sets the state of the USBH_XHCI.
*
*   INPUT
*
*       VOID        *sess_handle            - Session handle
*       PM_STATE_ID *state                  - Power state
*
*   OUTPUT
*
*       PM_STATUS   pm_status               - NU_SUCCESS
*
*************************************************************************/
STATUS USBH_XHCI_Set_State(VOID *inst_handle, PM_STATE_ID *state)
{
    USBH_XHCI_INSTANCE_HANDLE *instance_handle;
    STATUS                   pm_status;

    instance_handle = (USBH_XHCI_INSTANCE_HANDLE*)inst_handle;

    switch(*state)
    {
        case XHCI_ON:
        case POWER_ON_STATE:
        {
            /* Validate current state. */
            switch(PMI_STATE_GET(instance_handle->pmi_dev))
            {
                case XHCI_OFF:
                case XHCI_SLEEP:
                case POWER_ON_STATE:
                    pm_status = USBH_XHCI_Enable(XHCI_Session_Handle);
                    break;
                default:
                    pm_status = PM_INVALID_STATE;
                    break;
            }
            break;
        }
        case XHCI_OFF:
        {
            /* Validate current state. */
            switch(PMI_STATE_GET(instance_handle->pmi_dev))
            {
                case XHCI_ON:
                case POWER_ON_STATE:
                case XHCI_SLEEP:
                    pm_status = USBH_XHCI_Port_Resume(XHCI_Session_Handle);
                    pm_status |= USBH_XHCI_Disable(XHCI_Session_Handle);
                    break;
                default:
                    pm_status = PM_INVALID_STATE;
                    break;
            }
            break;
        }
        case XHCI_SLEEP:
        {
            /* Validate current state. */
            switch(PMI_STATE_GET(instance_handle->pmi_dev))
            {
                case XHCI_ON:
                case POWER_ON_STATE:
                    pm_status = USBH_XHCI_Sleep(XHCI_Session_Handle);
                    break;
                default:
                    pm_status = PM_INVALID_STATE;
                    break;
            }
            break;
        }
        default:
        {
            pm_status = PM_INVALID_STATE;
            break;
        }
    }

    if ( pm_status == NU_SUCCESS )
    {
        PMI_STATE_SET(instance_handle->pmi_dev,*state);
    }

    return (pm_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       USBH_XHCI_Enable
*
*   DESCRIPTION
*
*       This function enables the USBH_XHCI hardware.
*
*   INPUTS
*
*       VOID            *sess_handle         - Device session handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static STATUS USBH_XHCI_Enable (VOID *sess_handle)
{
    USBH_XHCI_INSTANCE_HANDLE   *inst_handle;
    USBH_XHCI_SESSION_HANDLE    *pwr_session_handle;
    VOID                        (*setup_func)(VOID);

    pwr_session_handle  = (USBH_XHCI_SESSION_HANDLE*)sess_handle;
    inst_handle         = pwr_session_handle->xhci_inst_handle;

    if ( inst_handle->setup_func != NU_NULL )
    {
        /* Enable clock for XHCI controller. */
        setup_func = inst_handle->setup_func;
        (VOID)setup_func();

        /* Enable VBUS supply on all ports. */
        (VOID)USBH_XHCI_Enable_VBUS(sess_handle);
    }

    (VOID)USBH_XHCI_Port_Resume(sess_handle);

    return ( NU_SUCCESS );
}

/*************************************************************************
*
*   FUNCTION
*
*       USBH_XHCI_Port_Resume
*
*   DESCRIPTION
*
*       This function wakes up the USBH_XHCI hardware from sleep mode.
*
*   INPUTS
*
*       VOID           *sess_handle         - Device session handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static STATUS USBH_XHCI_Port_Resume(VOID *sess_handle)
{
    USBH_XHCI_INSTANCE_HANDLE   *inst_handle;
    USBH_XHCI_SESSION_HANDLE    *pwr_session_handle;
    NU_USB_DEVICE               *usb_device_list, *tmp_usb_device;
    STATUS                       status;

    pwr_session_handle  = (USBH_XHCI_SESSION_HANDLE*)sess_handle;
    inst_handle         = pwr_session_handle->xhci_inst_handle;

    if ( (XHCI_Session_Handle != NU_NULL) && (inst_handle->device_in_use == NU_TRUE) )
    {
        tmp_usb_device = usb_device_list = NU_USBH_STACK_Get_Devices(
                            (NU_USBH_HW*)(&XHCI_Session_Handle->xhci_cb));

        /* Keep traversing until we reach to end of list or root hub. */
        while((tmp_usb_device != NU_NULL) &&
              (!USBH_IS_ROOT_HUB (tmp_usb_device)))
        {
            if ( (tmp_usb_device->state & USB_STATE_SUSPENDED) &&
                 (tmp_usb_device->parent->function_address == USB_ROOT_HUB ) )
            {
                status = NU_USBH_STACK_Resume_Device((NU_USBH_STACK*)(tmp_usb_device->stack),
                                                     tmp_usb_device);
                NU_USB_ASSERT ( status == NU_SUCCESS );
            }

            tmp_usb_device = (NU_USB_DEVICE*)tmp_usb_device->node.cs_next;
            if ( tmp_usb_device == usb_device_list )
            {
                tmp_usb_device = NU_NULL;
            }
        }
    }

    return ( NU_SUCCESS );
}

/*************************************************************************
*
*   FUNCTION
*
*       USBH_XHCI_Disable
*
*   DESCRIPTION
*
*       This function disables the USBH_XHCI hardware.
*
*   INPUTS
*
*       VOID            *sess_handle        - Device Session handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static STATUS USBH_XHCI_Disable (VOID *sess_handle)
{
    USBH_XHCI_INSTANCE_HANDLE   *inst_handle;
    USBH_XHCI_SESSION_HANDLE    *pwr_session_handle;
    NU_USB_DEVICE               *tmp_usb_device;
    VOID                        (*cleanup_func)(VOID);

    pwr_session_handle = (USBH_XHCI_SESSION_HANDLE*)sess_handle;
    inst_handle        = pwr_session_handle->xhci_inst_handle;

    /* Disable VBUS supply on all ports. Once all the devices will be
     * disconnected USB host stack will notify the driver to turn off
     * the clocks if desired.
     */
    (VOID)USBH_XHCI_Disable_VBUS(sess_handle);

    if ( XHCI_Session_Handle != NU_NULL )
    {
        /* If there is no device connected then go shut down clocks immediately. */
        tmp_usb_device = NU_USBH_STACK_Get_Devices((NU_USBH_HW*)(&XHCI_Session_Handle->xhci_cb));
        if(tmp_usb_device != NU_NULL)
        {
            if ( tmp_usb_device->function_address == USB_ROOT_HUB )
            {
                if ( inst_handle->cleanup_func != NU_NULL )
                {
                    /* Disable clock for XHCI controller. */
                    cleanup_func = inst_handle->cleanup_func;
                    (VOID)cleanup_func();
                }
            }
        }
    }

    return ( NU_SUCCESS );
}

/*************************************************************************
*
*   FUNCTION
*
*       USBH_XHCI_Sleep
*
*   DESCRIPTION
*
*       This function put XHCI controller in sleep mode.
*
*   INPUTS
*
*       VOID           *sess_handle         - Device Session handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static STATUS USBH_XHCI_Sleep (VOID *sess_handle)
{
    USBH_XHCI_INSTANCE_HANDLE   *inst_handle;
    USBH_XHCI_SESSION_HANDLE    *pwr_session_handle;
    NU_USB_DEVICE               *usb_device_list, *tmp_usb_device;
    STATUS                      status;

    pwr_session_handle  = (USBH_XHCI_SESSION_HANDLE*)sess_handle;
    inst_handle         = pwr_session_handle->xhci_inst_handle;

    if ( (XHCI_Session_Handle != NU_NULL) && (inst_handle->device_in_use == NU_TRUE) )
    {
        tmp_usb_device = usb_device_list = NU_USBH_STACK_Get_Devices(
                            (NU_USBH_HW*)(&(XHCI_Session_Handle->xhci_cb)));

       /* Keep traversing until we reach to end of list or root hub. */
       while((tmp_usb_device != NU_NULL) &&
             (!USBH_IS_ROOT_HUB (tmp_usb_device)))
       {
            if ( tmp_usb_device->parent->function_address == USB_ROOT_HUB )
            {
                /* Place the entire bus in the suspend state (Global Suspend). */
                status = NU_USBH_STACK_Suspend_Device((NU_USBH_STACK*)(tmp_usb_device->stack),
                                                      tmp_usb_device);
                NU_USB_ASSERT( status == NU_SUCCESS );
            }

            tmp_usb_device = (NU_USB_DEVICE*)tmp_usb_device->node.cs_next;
            if ( tmp_usb_device == usb_device_list )
            {
                tmp_usb_device = NU_NULL;
            }
        }
    }

    return ( NU_SUCCESS );
}

/*************************************************************************
*
*   FUNCTION
*
*       USBH_XHCI_Enable_VBUS
*
*   DESCRIPTION
*
*       This function enables VBUS.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID USBH_XHCI_Enable_VBUS(VOID *sess_handle)
{
    USBH_XHCI_INSTANCE_HANDLE   *inst_handle;
    USBH_XHCI_SESSION_HANDLE    *pwr_session_handle;

    pwr_session_handle  = (USBH_XHCI_SESSION_HANDLE*) sess_handle;
    inst_handle         = pwr_session_handle->xhci_inst_handle;

    if ( inst_handle->xhci_pwr_handle != DV_INVALID_HANDLE )
    {
        DVC_Dev_Ioctl(inst_handle->xhci_pwr_handle,
              inst_handle->xhci_pwr_ioctl_base_addr + NU_USBH_PWR_IOCTL_ENABLE_VBUS,
              NU_NULL,
              0);
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       USBH_XHCI_Disable_VBUS
*
*   DESCRIPTION
*
*       This function disables VBUS.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
void USBH_XHCI_Disable_VBUS(VOID *sess_handle)
{
    USBH_XHCI_INSTANCE_HANDLE   *inst_handle;
    USBH_XHCI_SESSION_HANDLE    *pwr_session_handle;

    pwr_session_handle  = (USBH_XHCI_SESSION_HANDLE*) sess_handle;
    inst_handle         = pwr_session_handle->xhci_inst_handle;

    if ( inst_handle->xhci_pwr_handle != DV_INVALID_HANDLE )
    {
        DVC_Dev_Ioctl(inst_handle->xhci_pwr_handle,
                      inst_handle->xhci_pwr_ioctl_base_addr + NU_USBH_PWR_IOCTL_DISABLE_VBUS,
                      NU_NULL,
                      0);
    }
}



/*************************************************************************
*
*   FUNCTION
*
*       USBH_XHCI_PWR_Drvr_Register_CB
*
*   DESCRIPTION
*
*       This function is called by device manager upon finding a new 
*       port powr driver for xHCI.
*
*   INPUT
*
*       device_id
*       context
*
*   OUTPUT
*
*       None
*
*************************************************************************/
STATUS USBH_XHCI_PWR_Drvr_Register_CB(DV_DEV_ID device_id, VOID *context)
{
    STATUS         status;
    DV_DEV_LABEL   usbh_pwr_label = {USBH_PWR_LABEL};
    DV_DEV_HANDLE  usbh_pwr_sess_hd;
    DV_IOCTL0_STRUCT     dev_ioctl0;
    USBH_XHCI_INSTANCE_HANDLE   *inst_handle;

    inst_handle = (USBH_XHCI_INSTANCE_HANDLE*) context;

    /* Open USBH Port Power controller. */
    status = DVC_Dev_ID_Open(device_id, &usbh_pwr_label, 1, &usbh_pwr_sess_hd);
    if ( status == NU_SUCCESS )
    {
        /* Get IOCTL base address */
        dev_ioctl0.label = usbh_pwr_label;
        status = DVC_Dev_Ioctl(usbh_pwr_sess_hd, DV_IOCTL0, &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));
        if ( status == NU_SUCCESS )
        {
            inst_handle->xhci_pwr_handle            = usbh_pwr_sess_hd;
            inst_handle->xhci_pwr_ioctl_base_addr   = dev_ioctl0.base;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
             if ( (PMI_STATE_GET(inst_handle->pmi_dev) == XHCI_ON) ||
                                             (PMI_STATE_GET(inst_handle->pmi_dev) == POWER_ON_STATE) )
             {
                 DVC_Dev_Ioctl(inst_handle->xhci_pwr_handle,
                        inst_handle->xhci_pwr_ioctl_base_addr + NU_USBH_PWR_IOCTL_ENABLE_VBUS,
                        NU_NULL,
                        0);
            }
#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */
        }
    }
    
    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       USBH_XHCI_PWR_Drvr_Unregister_CB
*
*   DESCRIPTION
*
*       This function is called by device manager when a port power 
*       driver is unreigstered Ideally a port power driver can never 
*       un-register, hence implementation of this function is not 
*       required.
*
*   INPUT
*
*       device_id
*       context
*
*   OUTPUT
*
*       None
*
*************************************************************************/
STATUS USBH_XHCI_PWR_Drvr_Unregister_CB(DV_DEV_ID device_id, VOID *context)
{
    return ( NU_SUCCESS );
}

#endif
/*************************************************************************
*
*   FUNCTION
*
*  
*    XHCI_Get_Target_Info
*
*   DESCRIPTION
*
*       This function retrieves XHCI HW information.
*
*   INPUT
*
*       USBH_XHCI_TGT_INFO                  XHCI target info control block.
*       key                                 Registry path
*
*   OUTPUT
*
*       NU_SUCCESS                          Operation completed successfully.
*
*************************************************************************/
STATUS XHCI_Get_Target_Info(const CHAR * key, USBH_XHCI_TGT_INFO *usbh_xhci_tgt)
{
    STATUS     status = NU_SUCCESS;
    CHAR       reg_path[REG_MAX_KEY_LENGTH];

    /* Get values from the registry */
    strncpy(reg_path, key, sizeof(reg_path));
    REG_Get_String (strcat(reg_path, "/tgt_settings/dev_name"), usbh_xhci_tgt->name, sizeof(usbh_xhci_tgt->name));

    strncpy(reg_path, key, sizeof(reg_path));
    strcat(reg_path, "/tgt_settings/hw_location");

    /* Check if the controller is ON CHIP or connected via external bus interface.Only
     * PCI and PCIe are supported.
     */

#ifdef CFG_NU_OS_CONN_PCI_COMMON_ENABLE
    if ( REG_Has_Key(reg_path) )
    {
         REG_Get_UINT32(reg_path,&usbh_xhci_tgt->hw_location);

         if( usbh_xhci_tgt->hw_location == ON_PCI )
         {
            status = Get_XHCI_Info_From_PCI(usbh_xhci_tgt);
         }
    }
    else
#endif
    {
        strncpy(reg_path, key, sizeof(reg_path));
        REG_Get_UINT32 (strcat(reg_path, "/tgt_settings/base"), (UINT32*)&usbh_xhci_tgt->base_address);
        strncpy(reg_path, key, sizeof(reg_path));
        REG_Get_UINT32 (strcat(reg_path, "/tgt_settings/intr_vector"), (UINT32*)&usbh_xhci_tgt->irq);
        strncpy(reg_path, key, sizeof(reg_path));
        REG_Get_UINT32 (strcat(reg_path, "/tgt_settings/priority"), (UINT32*)&usbh_xhci_tgt->irq_priority);
        strncpy(reg_path, key, sizeof(reg_path));
        REG_Get_UINT32 (strcat(reg_path, "/tgt_settings/trigger_type"), (UINT32*)&usbh_xhci_tgt->irq_type);
    }

    strncpy(reg_path, key, sizeof(reg_path));
    strcat(reg_path, "/tgt_settings/total_current");

    if (REG_Has_Key(reg_path))
    {
        REG_Get_UINT32(reg_path, (UINT32*)&usbh_xhci_tgt->total_current);
    }

    return ( status );
}
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE))
/*************************************************************************
*
*   FUNCTION
*
*  
*    XHCI_Pwr_Pre_Park
*
*   DESCRIPTION
*
*       This function will notify the XHCI when to park or to resume.
*
*   INPUT
*
*       inst_handle                  XHCI instance handle.
*
*   OUTPUT
*
*       NU_SUCCESS                          Operation completed successfully.
*
*************************************************************************/
STATUS XHCI_Pwr_Pre_Park( VOID *inst_handle )
{
	return ( NU_SUCCESS );
}
/*************************************************************************
*
*   FUNCTION
*
*  
*    XHCI_Pwr_Post_Park
*
*   DESCRIPTION
*
*       This function will notify the XHCI when to park or to resume.
*
*   INPUT
*
*       inst_handle                  XHCI instance handle.
*
*   OUTPUT
*
*       NU_SUCCESS                          Operation completed successfully.
*
*************************************************************************/
STATUS XHCI_Pwr_Post_Park( VOID *inst_handle )
{
	return ( NU_SUCCESS );
}
/*************************************************************************
*
*   FUNCTION
*
*  
*    XHCI_Pwr_Pre_Resume
*
*   DESCRIPTION
*
*       This function will notify the XHCI when to park or to resume.
*
*   INPUT
*
*       inst_handle                  XHCI instance handle.
*
*   OUTPUT
*
*       NU_SUCCESS                          Operation completed successfully.
*
*************************************************************************/
STATUS XHCI_Pwr_Pre_Resume( VOID *inst_handle )
{
	return ( NU_SUCCESS );
}
/*************************************************************************
*
*   FUNCTION
*
*  
*    XHCI_Pwr_Post_Resume
*
*   DESCRIPTION
*
*       This function will notify the XHCI when to park or to resume.
*
*   INPUT
*
*       inst_handle                         XHCI instance handle.
*
*   OUTPUT
*
*       NU_SUCCESS                          Operation completed successfully.
*
*************************************************************************/
STATUS XHCI_Pwr_Post_Resume( VOID *inst_handle )
{
	return ( NU_SUCCESS );
}
#endif

#ifdef CFG_NU_OS_CONN_PCI_COMMON_ENABLE
/*************************************************************************
*
*   FUNCTION
*
*  
*    Get_XHCI_Info_From_PCI
*
*   DESCRIPTION
*
*       This function retrieves XHCI HW information from PCI driver.
*
*   INPUT
*
*       USBH_XHCI_TGT_INFO                  XHCI target info control block.
*
*   OUTPUT
*
*       NU_SUCCESS                          Operation completed successfully.
*
*************************************************************************/
static STATUS Get_XHCI_Info_From_PCI(USBH_XHCI_TGT_INFO *xhci_tgt)
{
    STATUS  status = -1;

    UINT32  i,j;
    PCB_BUS *pBus;
    PCD_HDR *pDev;
    UINT32   board_class;
    UINT32   board_id;
    UINT16   cmd;
    UINT8    index;
    UINT32   vector_no;
    UINT32 base_addr[6];
    UINT8 hw_type;
    PCS_SYS *pci_sys;

    status = PCI_Get_Handle( &pci_sys );
    if ( status == NU_SUCCESS )
    {
        for(i=0; i<PCL_Size(pci_sys->bus_list); i++)
        {
            pBus = PCL_Data(PCL_Return_At(pci_sys->bus_list,i));

            for(j=0; j<PCL_Size(pBus->DeviceList); j++)
            {
                pDev = PCL_Data(PCL_Return_At(pBus->DeviceList,j));

                NU_PCI_Read32(pci_sys, pDev->bus, pDev->device, pDev->function,
                                            PCI_CFG_REVISION_ID, &board_class);

                NU_PCI_Read32(pci_sys, pDev->bus, pDev->device, pDev->function,
                                                 PCI_CFG_VENDOR_ID, &board_id);

                if((board_class & 0xFFFFFF00) == USB_XHCI_CLASS)
                {
                    NU_PCI_Read8(pci_sys,
                                 pDev->bus,
                                 pDev->device,
                                 pDev->function,
                                 PCI_CFG_PROGRAMMING_INTERFACE,
                                 &hw_type);

                    for( index = 0; index < 6; index++ )
                    {
                        NU_PCI_Get_BAR(pci_sys,
                                       pDev->bus,
                                       pDev->device,
                                       pDev->function,
                                       index,
                                       &base_addr[index]);
                    }

                    NU_PCI_Get_NU_Legacy_Vector(pci_sys,
                                                pDev->bus,
                                                pDev->device,
                                                pDev->function,
                                                &vector_no);

                    NU_PCI_Read16(pci_sys,
                                  pDev->bus,
                                  pDev->device,
                                  pDev->function,
                                  PCI_CFG_COMMAND,
                                  &cmd);

                    cmd |= PCI_CFG_CMD_MASTER_ENABLE;

                    NU_PCI_Write16(pci_sys,
                                   pDev->bus,
                                   pDev->device,
                                   pDev->function,
                                   PCI_CFG_COMMAND,
                                   cmd);

                    xhci_tgt->base_address = base_addr[0]& (~0xF);
                    xhci_tgt->irq = vector_no;
                    xhci_tgt->irq_priority = 1;
                    xhci_tgt->irq_type = ESAL_TRIG_LEVEL_LOW;

                    return ( NU_SUCCESS );
                }
            }
        }
    }

    return ( status );
}
#endif

/* =======================  End Of File  ============================== */