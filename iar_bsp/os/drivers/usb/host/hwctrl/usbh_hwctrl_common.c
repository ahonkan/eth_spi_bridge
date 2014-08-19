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
*       usbh_hwctrl_common.c
*
* COMPONENT
*
*       Generic USB Host Controller Driver
*
* DESCRIPTION
*
*       This file contains the wrapper functions for using USB Host
*       controller driver API's.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       USBH_HWCTRL_Handle_Initialize
*       USBH_HWCTRL_Handle_Uninitialize
*       USBH_HWCTRL_Handle_IO_Request
*       USBH_HWCTRL_Handle_Open_Pipe
*       USBH_HWCTRL_Handle_Close_Pipe
*       USBH_HWCTRL_Handle_Modify_Pipe
*       USBH_HWCTRL_Handle_Flush_Pipe
*       USBH_HWCTRL_Handle_Enable_Int
*       USBH_HWCTRL_Handle_Disable_Int
*       USBH_HWCTRL_Handle_Execute_ISR
*       USBH_HWCTRL_Handle_Get_CB
*       USBH_HWCTRL_Handle_Get_Speed
*       USBH_HWCTRL_Handle_Is_Current_Available
*       USBH_HWCTRL_Handle_Release_Power
*       USBH_HWCTRL_Handle_Request_Power_Down
*       USBH_HWCTRL_Get_Target_Info
*
* DEPENDENCIES
*
*       usbh_hwctrl_common.h
*
**************************************************************************/

/* ==============  USB Include Files =================================== */
#include "drivers/usbh_hwctrl_common.h"

/* =====================  Global data ================================== */
extern USBH_HWCTRL_SESSION_HANDLE    *USBH_HWCTRL_Session_Handle;

/* ===================  External Function Prototypes  ================== */
extern  STATUS  USBH_HWCTRL_Tgt_Initialize(NU_USB_HW*, NU_USB_STACK*);
extern  STATUS  USBH_HWCTRL_Tgt_Uninitialize(NU_USB_HW*);
extern  STATUS  USBH_HWCTRL_Tgt_Submit_Irp(NU_USB_HW*,NU_USB_IRP*,UINT8,UINT8);
extern  STATUS  USBH_HWCTRL_Tgt_Open_Pipe(NU_USB_HW*,UINT8,UINT8,UINT8,UINT8,UINT16,UINT32,UINT32);
extern  STATUS  USBH_HWCTRL_Tgt_Close_Pipe(NU_USB_HW*,UINT8,UINT8);
extern  STATUS  USBH_HWCTRL_Tgt_Flush_Pipe(NU_USB_HW*,UINT8,UINT8);
extern  STATUS  USBH_HWCTRL_Tgt_Enable_Int(NU_USB_HW*);
extern  STATUS  USBH_HWCTRL_Tgt_Disable_Int(NU_USB_HW*);
#if CFG_NU_OS_CONN_USB_COM_STACK_OTG_ENABLE == 1
extern  STATUS  USBH_HWCTRL_Tgt_Get_Role(NU_USB_HW*, USB_PORT_INFO*);
extern  STATUS  USBH_HWCTRL_Tgt_Start_Session (NU_USB_HW*, UINT8,  UINT16);
extern  STATUS  USBH_HWCTRL_Tgt_End_Session (NU_USB_HW*, UINT8);
extern  STATUS  USBH_HWCTRL_Tgt_Notify_Role_Switch (NU_USB_HW*, NU_USB_HW_ROLESWITCH_CALLBACK );
#endif
extern  STATUS  USBH_HWCTRL_Tgt_ISR(NU_USB_HW*);
extern  STATUS  USBH_HWCTRL_Tgt_Modify_Pipe(NU_USB_HW*,UINT8,UINT8,UINT8,UINT16,UINT32,UINT32);
extern  STATUS  USBH_HWCTRL_Tgt_Is_Current_Available(NU_USB_HW*,USB_DEV_CURRENT_INFO*);
extern  STATUS  USBH_HWCTRL_Tgt_Release_Power(NU_USB_HW*,USB_DEV_CURRENT_INFO*);

/**************************************************************************
* FUNCTION
*
*       USBH_HWCTRL_Handle_Initialize
*
* DESCRIPTION
*
*       This function handles initialize usbh_hwctrl IOCTL.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_Initialize(USBH_HWCTRL_SESSION_HANDLE *usbh_hwctrl_handle,
                                     VOID  *ioctl_data,
                                     INT    ioctl_data_len)
{
    STATUS status;

    NU_USB_PTRCHK(usbh_hwctrl_handle);
    NU_USB_PTRCHK(ioctl_data);

    /* Initialize USB host controller driver. */
    status =  USBH_HWCTRL_Tgt_Initialize((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb),
                                         (NU_USB_STACK *)ioctl_data);

    return ( status );
}

/**************************************************************************
* FUNCTION
*
*       USBH_HWCTRL_Handle_UnInitialize
*
* DESCRIPTION
*
*       This function handles Uninitialize usbh_hwctrl IOCTL.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_Uninitialize(USBH_HWCTRL_SESSION_HANDLE *usbh_hwctrl_handle,
                                     VOID  *ioctl_data,
                                     INT    ioctl_data_len)
{
    STATUS status;

    NU_USB_PTRCHK(usbh_hwctrl_handle);

    /* Initialize USB host controller driver. */
    status =  USBH_HWCTRL_Tgt_Uninitialize((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb));

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_HWCTRL_Handle_IO_Request
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_IO_REQUEST.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_IO_Request(USBH_HWCTRL_SESSION_HANDLE   *usbh_hwctrl_handle,
                                     VOID  *ioctl_data,
                                     INT    ioctl_data_len)
{
    STATUS          status;
    USB_IRP_INFO    *irp_info;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT(ioctl_data_len == sizeof(USB_IRP_INFO))

    /* Extract pointer to IRP from IOCTL data. */
    irp_info = (USB_IRP_INFO*) ioctl_data;

    /* Submit the IRP finally. */
    status = USBH_HWCTRL_Tgt_Submit_Irp((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb),
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
*       USBH_HWCTRL_Handle_Open_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_OPEN_PIPE.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_Open_Pipe(USBH_HWCTRL_SESSION_HANDLE    *usbh_hwctrl_handle,
                                    VOID  *ioctl_data,
                                    INT    ioctl_data_len)
{
    USB_EP_INFO *ep_info;
    STATUS       status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call open pipe function of USBH HWCTRL driver. */
    status = USBH_HWCTRL_Tgt_Open_Pipe ((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb),
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
*       USBH_HWCTRL_Handle_Close_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_CLOSE_PIPE.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_Close_Pipe(USBH_HWCTRL_SESSION_HANDLE   *usbh_hwctrl_handle,
                                     VOID  *ioctl_data,
                                     INT    ioctl_data_len)
{
    USB_EP_INFO    *ep_info;
    STATUS          status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call close pipe function of USBH HWCTRL driver. */
    status = USBH_HWCTRL_Tgt_Close_Pipe ((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb),
                                         ep_info->function_addr,
                                         ep_info->endp_addr);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_HWCTRL_Handle_Modify_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_MODIFY_PIPE.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_Modify_Pipe(USBH_HWCTRL_SESSION_HANDLE  *usbh_hwctrl_handle,
                                      VOID  *ioctl_data,
                                      INT    ioctl_data_len)
{
    USB_EP_INFO     *ep_info;
    STATUS           status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call modify pipe function of USBH HWCTRL driver. */
    status = USBH_HWCTRL_Tgt_Modify_Pipe ((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb),
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
*       USBH_HWCTRL_Handle_Flush_Pipe
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_FLUSH_PIPE.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_Flush_Pipe(USBH_HWCTRL_SESSION_HANDLE   *usbh_hwctrl_handle,
                                     VOID  *ioctl_data,
                                     INT    ioctl_data_len)
{
    USB_EP_INFO     *ep_info;
    STATUS          status;

    NU_USB_PTRCHK(ioctl_data);
    NU_USB_ASSERT( ioctl_data_len == sizeof(USB_EP_INFO) );

    ep_info = (USB_EP_INFO*) ioctl_data;

    /* Call flush pipe function of USBH HWCTRL driver. */
    status = USBH_HWCTRL_Tgt_Flush_Pipe ((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb),
                                         ep_info->function_addr,
                                         ep_info->endp_addr);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_HWCTRL_Handle_Enable_Int
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_ENABLE_INT.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_Enable_Int(USBH_HWCTRL_SESSION_HANDLE   *usbh_hwctrl_handle,
                                     VOID  *ioctl_data,
                                     INT    ioctl_data_len)
{
    STATUS  status;

    ESAL_GE_INT_Enable(usbh_hwctrl_handle->usbh_hwctrl_inst_handle->tgt_info->irq,
                       usbh_hwctrl_handle->usbh_hwctrl_inst_handle->tgt_info->irq_type,
                       usbh_hwctrl_handle->usbh_hwctrl_inst_handle->tgt_info->irq_priority);

    /* Call flush pipe function of USBH HWCTRL driver. */
    status = USBH_HWCTRL_Tgt_Enable_Int ((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb) );

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_HWCTRL_Handle_Disable_Int
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_DISABLE_INT.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_Disable_Int(USBH_HWCTRL_SESSION_HANDLE  *usbh_hwctrl_handle,
                                      VOID  *ioctl_data,
                                      INT    ioctl_data_len)
{
    STATUS  status;

    ESAL_GE_INT_Disable(usbh_hwctrl_handle->usbh_hwctrl_inst_handle->tgt_info->irq);

    /* Call flush pipe function of USBH HWCTRL driver. */
    status = USBH_HWCTRL_Tgt_Disable_Int ((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb) );

    /* Return status of execution. */
    return ( status );
}
#if CFG_NU_OS_CONN_USB_COM_STACK_OTG_ENABLE == 1
/**************************************************************************
*
* FUNCTION
*
*       USBH_HWCTRL_Handle_Get_Role
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_GET_ROLE.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_Get_Role(USBH_HWCTRL_SESSION_HANDLE  *usbh_hwctrl_handle,
                                      VOID  *ioctl_data,
                                      INT    ioctl_data_len)
{
    STATUS  status;

    /* Call get role of USBH HWCTRL driver. */
    status = USBH_HWCTRL_Tgt_Get_Role ((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb), (USB_PORT_INFO *)ioctl_data);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_HWCTRL_Handle_Start_Session
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_START_SESSION.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_Start_Session(USBH_HWCTRL_SESSION_HANDLE  *usbh_hwctrl_handle,
                                      VOID  *ioctl_data,
                                      INT    ioctl_data_len)
{
    STATUS  status;
    USB_PORT_INFO *port_info = (USB_PORT_INFO *)ioctl_data;
    status = USBH_HWCTRL_Tgt_Start_Session ((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb), port_info->port_id, port_info->delay);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_HWCTRL_Handle_End_Session
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_END_SESSION.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_End_Session(USBH_HWCTRL_SESSION_HANDLE  *usbh_hwctrl_handle,
                                      VOID  *ioctl_data,
                                      INT    ioctl_data_len)
{
    STATUS  status;
    USB_PORT_INFO *port_info = (USB_PORT_INFO *)ioctl_data;
    status = USBH_HWCTRL_Tgt_End_Session ((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb), port_info->port_id);

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       USBH_HWCTRL_Handle_Notify_Role_Switch
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_NOTIF_ROLE_SWITCH.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_Notify_Role_Switch(USBH_HWCTRL_SESSION_HANDLE  *usbh_hwctrl_handle,
                                      VOID  *ioctl_data,
                                      INT    ioctl_data_len)
{
    STATUS  status;

    status = USBH_HWCTRL_Tgt_Notify_Role_Switch (usbh_hwctrl_handle->usbh_hwctrl_cb, ioctl_data);

    /* Return status of execution. */
    return ( status );
}
#endif
/**************************************************************************
*
* FUNCTION
*
*       USBH_HWCTRL_Handle_Execute_ISR
*
* DESCRIPTION
*
*       This function handles the IOCTL command NU_USB_IOCTL_EXECUTE_ISR.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_Execute_ISR(USBH_HWCTRL_SESSION_HANDLE  *usbh_hwctrl_handle,
                                      VOID  *ioctl_data,
                                      INT    ioctl_data_len)
{
    STATUS  status;

    NU_USB_PTRCHK(ioctl_data == NU_NULL);
    NU_USB_ASSERT( ioctl_data_len == 0 );

    ESAL_GE_INT_Disable(usbh_hwctrl_handle->usbh_hwctrl_inst_handle->tgt_info->irq);

    /* Call flush pipe function of USBH HWCTRL driver. */
    status = USBH_HWCTRL_Tgt_ISR ((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb) );

    /* Return status of execution. */
    return ( status );
}

/**************************************************************************
* FUNCTION
*
*       USBH_HWCTRL_Handle_Get_CB
*
* DESCRIPTION
*
*       This function gets the control block of hardware controller.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
*       ioctl_data                          Pointer to IOCTL data.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBH_HWCTRL_Handle_Get_CB(USBH_HWCTRL_SESSION_HANDLE *usbh_hwctrl_handle,
                                 VOID  **ioctl_data,
                                 INT     ioctl_data_len)
{
    NU_USB_PTRCHK(usbh_hwctrl_handle);
    NU_USB_PTRCHK(ioctl_data);

    /* Get usbh_hwctrl control block. */
    *ioctl_data = usbh_hwctrl_handle->usbh_hwctrl_cb;

    /* Return status of progress. */
    return( NU_SUCCESS );
}

/**************************************************************************
* FUNCTION
*
*       USBH_HWCTRL_Handle_Get_Speed
*
* DESCRIPTION
*
*       This function handles get speed IOCTL.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
*       ioctl_data                          Pointer to IOCTL data.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBH_HWCTRL_Handle_Get_Speed(USBH_HWCTRL_SESSION_HANDLE *usbh_hwctrl_handle,
                                    VOID  *ioctl_data,
                                    INT    ioctl_data_len)
{
    NU_USB_PTRCHK(usbh_hwctrl_handle);
    NU_USB_PTRCHK(ioctl_data);

    if ( ioctl_data_len == sizeof(UINT8) )
    {
        *(UINT8 *)ioctl_data = ((NU_USB_HW *) (usbh_hwctrl_handle->usbh_hwctrl_cb))->speed;
    }

    return ( NU_SUCCESS );
}

/**************************************************************************
* FUNCTION
*
*       USBH_HWCTRL_Handle_Is_Current_Available
*
* DESCRIPTION
*
*       This function handles the request to check that if enough current
*       is available for the newly connected device.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_Is_Current_Available(USBH_HWCTRL_SESSION_HANDLE *usbh_hwctrl_handle,
                                               VOID  *ioctl_data,
                                               INT    ioctl_data_len)
{
    STATUS      status = NU_USB_INVLD_ARG;

    NU_USB_PTRCHK(usbh_hwctrl_handle);
    NU_USB_PTRCHK(ioctl_data);

    if ( ioctl_data_len == sizeof(USB_DEV_CURRENT_INFO) )
    {
        status = USBH_HWCTRL_Tgt_Is_Current_Available((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb),
                                                      (USB_DEV_CURRENT_INFO *)ioctl_data);
    }

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       USBH_HWCTRL_Handle_Release_Power
*
* DESCRIPTION
*
*       This function releases the power which was acquired by the USB
*       device during set configuration.
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
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
STATUS USBH_HWCTRL_Handle_Release_Power(USBH_HWCTRL_SESSION_HANDLE *usbh_hwctrl_handle,
                                        VOID  *ioctl_data,
                                        INT    ioctl_data_len)
{
    STATUS      status = NU_USB_INVLD_ARG;

    NU_USB_PTRCHK(usbh_hwctrl_handle);
    NU_USB_PTRCHK(ioctl_data);

    if ( ioctl_data_len == sizeof(USB_DEV_CURRENT_INFO) )
    {
        status = USBH_HWCTRL_Tgt_Release_Power((NU_USB_HW*) (usbh_hwctrl_handle->usbh_hwctrl_cb),
                                               (USB_DEV_CURRENT_INFO *)ioctl_data);
    }

    return (status);
}


/**************************************************************************
* FUNCTION
*
*       USBH_HWCTRL_Handle_Request_Power_Down
*
* DESCRIPTION
*
*       This function handles Request Power Down Mode IOCTL
*
* INPUTS
*
*       usbh_hwctrl_handle                  Pointer to session handle.
*       ioctl_data                          Pointer to IOCTL data.
*       ioctl_data_len                      Length of IOCTL data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completion of service.
*
**************************************************************************/
STATUS USBH_HWCTRL_Handle_Request_Power_Down(USBH_HWCTRL_SESSION_HANDLE *usbh_hwctrl_handle,
                                             VOID  *ioctl_data,
                                             INT    ioctl_data_len)
{

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    USBH_HWCTRL_INSTANCE_HANDLE   *inst_handle;
    VOID                         (*cleanup_func)(VOID);

    inst_handle = usbh_hwctrl_handle->usbh_hwctrl_inst_handle;

    if ( PMI_STATE_GET(inst_handle->pmi_dev) == USBH_HWCTRL_OFF )
    {
        if ( inst_handle->cleanup_func != NU_NULL )
        {
            /* Disable clock for USBH hardware controller. */
            cleanup_func = inst_handle->cleanup_func;
            (VOID)cleanup_func();
            if(USBH_HWCTRL_Session_Handle != NU_NULL)
            {
                USBH_HWCTRL_Session_Handle->is_clock_enabled = NU_FALSE;
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
*    USBH_HWCTRL_Get_Target_Info
*
*   DESCRIPTION
*
*       This function retrieves USBH hardware controller information.
*
*   INPUT
*
*       key                                 Registry path
*       usbh_hwctrl_tgt                     Pointer to USBH HWCTRL target
*                                           info control block.
*
*   OUTPUT
*
*       NU_SUCCESS                          Operation completed successfully.
*       REG_BAD_PATH                        Invalid registry path.
*
*************************************************************************/
STATUS USBH_HWCTRL_Get_Target_Info(const CHAR * key, USBH_HWCTRL_TGT_INFO *usbh_hwctrl_tgt)
{
    STATUS status;

    /* Get values from the registry. */
    status = REG_Get_String_Value (key, "/tgt_settings/dev_name", usbh_hwctrl_tgt->name, sizeof(usbh_hwctrl_tgt->name));
    if(status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/tgt_settings/base", (UINT32*)&usbh_hwctrl_tgt->base_address);
    }
    if(status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/tgt_settings/intr_vector", (UINT32*)&usbh_hwctrl_tgt->irq);
    }
    if(status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/tgt_settings/priority", (UINT32*)&usbh_hwctrl_tgt->irq_priority);
    }
    if(status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/tgt_settings/trigger_type", (UINT32*)&usbh_hwctrl_tgt->irq_type);
    }
    if(status == NU_SUCCESS)
    {
        status = REG_Get_UINT32_Value (key, "/tgt_settings/total_current", (UINT32*)&usbh_hwctrl_tgt->total_current);
    }

    return ( status );
}

/* ======================== End of File ================================ */
