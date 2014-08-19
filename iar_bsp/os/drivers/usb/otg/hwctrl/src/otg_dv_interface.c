/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
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
*       otg_dv_interface.c
*
*   COMPONENT
*
*       Generic USB OTG Controller Driver                               
*
*   DESCRIPTION
*
*       This file contains the generic OTG DV interface
*       library functions.
*
*   FUNCTIONS
*
*       OTG_Dv_Register
*       OTG_Dv_Unregister
*       OTG_Dv_Open
*       OTG_Dv_Close
*       OTG_Dv_Read
*       OTG_Dv_Write
*       OTG_Dv_Ioctl
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*       nu_connectivity.h
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "connectivity/nu_connectivity.h"

extern  NU_MEMORY_POOL      System_Memory;
extern USBH_HWCTRL_SESSION_HANDLE *USBH_HWCTRL_Session_Handle;
OTG_HWCTRL_SESSION_HANDLE   *OTG_HWCTRL_Session_Handle;

static 	STATUS OTG_Dv_Open(VOID *instance_handle, 
                           DV_DEV_LABEL labels_list[], 
                           INT labels_cnt,
                           VOID ** session_handle);
static  STATUS OTG_Dv_Close(VOID *session_handle);

static  STATUS OTG_Dv_Write(VOID *session_handle, const VOID *buffer, 
                            UINT32 numbyte, OFFSET_T byte_offset,
                            UINT32 *bytes_written_ptr);
static	STATUS OTG_Dv_Read(VOID *session_handle, VOID *buffer,
			   UINT32 numbyte, OFFSET_T byte_offset,
			   UINT32 *bytes_read_ptr);
static	STATUS OTG_Dv_Ioctl(VOID *session_handle, INT cmd,
			    VOID *data, INT length);
			    
/* USBF HWCTRL driver external function prototypes */
extern STATUS  USBF_HWCTRL_Tgt_Handle_Get_Role(USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);
                                    
extern STATUS  USBF_HWCTRL_Tgt_Handle_Start_Session(USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);
                                    
extern STATUS  USBF_HWCTRL_Tgt_Handle_End_Session(USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);
                                    
extern STATUS  USBF_HWCTRL_Tgt_Handle_Notify_Role_Switch(USBF_SESSION_HANDLE *handle,
                                    VOID *ioctl_data,
                                    INT ioctl_data_len);
 
/* USBH HWCTRL driver external function prototypes */                                    
extern STATUS USBH_HWCTRL_Handle_Get_Role(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT );

extern STATUS USBH_HWCTRL_Handle_Start_Session(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT );

extern STATUS USBH_HWCTRL_Handle_End_Session(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT );

extern STATUS USBH_HWCTRL_Handle_Notify_Role_Switch(USBH_HWCTRL_SESSION_HANDLE*, VOID*, INT );

/******************************************************************************
*
* FUNCTION
*
*       OTG_Dv_Register
*
* DESCRIPTION
*
*       This function registers the OTG controller driver
*       with device manager.OTG controller driver must be registered
*       before USBH and USBF generic controller drivers.
*
* INPUTS
*
*       key                   Key
*       usbh_cb               USB host hardware controller base control block
*       usbf_cb               USB function hardware controller base control block
*
* OUTPUTS
*
*       status                             
*
**************************************************************************/
STATUS OTG_Dv_Register (const CHAR * key)
{
    STATUS                      status;
    DV_DRV_FUNCTIONS            otg_drv_funcs;
    DV_DEV_LABEL                otg_label;
    static DV_DEV_ID            otg_dev_id = DV_INVALID_DEV;
    
    status = NU_Allocate_Memory (&System_Memory, (VOID *)&OTG_HWCTRL_Session_Handle,
                                  sizeof (OTG_HWCTRL_SESSION_HANDLE), NU_NO_SUSPEND);
    if(status == NU_SUCCESS)
    {
        /* Zero out allocated space. */
        (VOID)memset (OTG_HWCTRL_Session_Handle, 0, sizeof (OTG_HWCTRL_SESSION_HANDLE));
        
        status = REG_Get_Bytes_Value (key, "/labels/otg", (UINT8*)&otg_label, sizeof(DV_DEV_LABEL));
    
        if(status == NU_SUCCESS)
        {
            otg_drv_funcs.drv_open_ptr  = OTG_Dv_Open;
            otg_drv_funcs.drv_close_ptr = OTG_Dv_Close;
            otg_drv_funcs.drv_read_ptr  = OTG_Dv_Read;
            otg_drv_funcs.drv_write_ptr = OTG_Dv_Write;
            otg_drv_funcs.drv_ioctl_ptr = OTG_Dv_Ioctl;

            /* Register this device with the Device Manager */
            status = DVC_Dev_Register(NU_NULL, &otg_label,
                                      1, &otg_drv_funcs,
                                      &otg_dev_id);
        }
    }
    
    return status;
}

/**************************************************************************
*
* FUNCTION
*
*       OTG_Dv_Unregister
*
* DESCRIPTION
*
*       This function is called when device manager wants to stop OTG
*       controller.
*
* INPUTS
*
*       key                                 Registry path
*       startstop                           Options specifying start or
*                                           stop.
*       dev_id                              ID of the device to be
*                                           unregistered.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           stopped successfully.
*       Error Code                          Some error has occured while
*                                           unregistering the driver.
*
**************************************************************************/
STATUS OTG_Dv_Unregister (const CHAR *key,
                               INT startstop,
                               DV_DEV_ID dev_id)
{
    STATUS status;

    /* Unregister the device. */
    status = DVC_Dev_Unregister (dev_id, NU_NULL);
    if(status == NU_SUCCESS )
    {
        status = NU_Deallocate_Memory(OTG_HWCTRL_Session_Handle);
    }
    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       OTG_Dv_Open
*
* DESCRIPTION
*
*       This is the actual 'open' handler of OTG controller driver.
*
* INPUTS
*
*       instance_handle                     Pointer to an instance handle
*                                           created during initialization.
*       label_list                          List of labels associated with
*                                           this driver.
*       label_cnt                           Number of labels
*       session_handle                      Driver creates a session handle
*                                           and return it in output argument.
*
* OUTPUTS
*
*       status                          
*
**************************************************************************/
static STATUS OTG_Dv_Open(VOID *instance_handle, 
                          DV_DEV_LABEL labels_list[], 
                          INT labels_cnt,
                          VOID ** session_handle)
{
    OTG_HWCTRL_Session_Handle->usbh_handle = USBH_HWCTRL_Session_Handle;
    *session_handle = (VOID*)OTG_HWCTRL_Session_Handle;
    return (NU_SUCCESS);
}


/**************************************************************************
*
* FUNCTION
*
*       OTG_Dv_Close
*
* DESCRIPTION
*
*       This is the actual 'close' handler of otg controller driver.
*
* INPUTS
*
*       session_handle                      Pointer to session handle 
*                                           returned during device open.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver 
*                                           closed successfully.
*
**************************************************************************/
static STATUS OTG_Dv_Close(VOID *session_handle)
{
    return (NU_SUCCESS);
}

/***************************************************************************
*
* FUNCTION
*
*       OTG_Dv_Write
*
* DESCRIPTION
*
*       This is the actual 'write' handler of OTG controller driver.
*       This function is not used for OTG controller, hence it should 
*       remain empty.
*
* INPUTS
*
*       session_handle                      Pointer to session handle
*                                           returned during device open.
*       buffer                              Pointer to buffer containing
*                                           data to be written.
*       numbytes                            Length of data present in
*                                           'buffer'.
*       byte_offset
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*
**************************************************************************/
static STATUS OTG_Dv_Write(VOID *session_handle, const VOID *buffer,
	       		   UINT32 numbyte, OFFSET_T byte_offset,
                           UINT32 *bytes_written_ptr)
{
    return (NU_SUCCESS);
}

/**************************************************************************
*
* FUNCTION
*
*       OTG_Dv_Read
*
* DESCRIPTION
*
*       This is the actual 'read' handler of OTG controller driver.
*       This function is not used for OTG, hence it should remain empty.
*
* INPUTS
*
*       session_handle                      Pointer to session handle
*                                           returned during device open.
*       buffer                              Pointer to buffer where data
*                                           is to be saved.
*       numbytes                            Length of data present in
*                                           'buffer'.
*       byte_offset
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver
*                                           closed successfully.
*
**************************************************************************/
static STATUS OTG_Dv_Read(VOID *session_handle, VOID *buffer,
			  UINT32 numbyte, OFFSET_T byte_offset,
			  UINT32 *bytes_read_ptr)						  
{
    return (NU_SUCCESS);
}
/**************************************************************************
* FUNCTION
*
*       OTG_Dv_Ioctl
*
* DESCRIPTION
*
*       This function is Ioctl routine for OTG controller driver,
*       which is registered with device manager. This function calls 
*       a specific Ioctl based on ioctl_num.
*
* INPUTS
*
*       dev_handle                          This parameter is the device
*                                           handle of the device to control.
*       ioctl_num                           This parameter is the ioctl
*                                           command.
*       ioctl_data                          This parameter is the optional
*                                           ioctl data.
*       ioctl_data_len                      This parameter is the size, in
*                                           bytes, of the ioctl data.
*                                           
* OUTPUTS                                   
*                                           
*       NU_SUCCESS                          Indicates successful
*                                           initialization of controller.
*       DV_IOCTL_INVALID_LENGTH             Specified data length is invalid
*       DV_INVALID_INPUT_PARAMS             Specified IOCTL code is unknown
*
**************************************************************************/
static STATUS OTG_Dv_Ioctl(VOID *session_handle, INT cmd,
		            VOID *data, INT length)
{
    DV_IOCTL0_STRUCT              *ioctl0;
    STATUS		          status = DV_IOCTL_INVALID_MODE;
    OTG_HWCTRL_SESSION_HANDLE     *otg_handle = (OTG_HWCTRL_SESSION_HANDLE  *)session_handle;
    USBH_HWCTRL_SESSION_HANDLE    *usbh_hwctrl_handle = otg_handle->usbh_handle;
    USBF_SESSION_HANDLE           *usbf_hwctrl_handle = otg_handle->usbf_handle;
    NU_USBOTG_NOTIFY_CALLBACKS    *callbacks;
	
    switch(cmd)
    {
        case DV_IOCTL0:
        {
            if (length == sizeof(DV_IOCTL0_STRUCT))
            {
                /* Get the ioctl0 structure from the data passed in */
                ioctl0 = (DV_IOCTL0_STRUCT *)data;
                ioctl0->base = NU_USB_OTG_IOCTL_BASE;
                status = NU_SUCCESS;
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }
        /********************************
        * USB OTG  IOCTLS *
        ********************************/
        case (NU_USB_OTG_IOCTL_BASE + NU_USB_OTG_IOCTL_GET_ROLE):
            status = USBH_HWCTRL_Handle_Get_Role(usbh_hwctrl_handle, data, length);
            break;
            
        case (NU_USB_OTG_IOCTL_BASE + NU_USB_OTG_IOCTL_START_HOST_SESSION):
            status = USBH_HWCTRL_Handle_Start_Session(usbh_hwctrl_handle, data, length);
            break;
            
        case (NU_USB_OTG_IOCTL_BASE + NU_USB_OTG_IOCTL_END_HOST_SESSION):
            status = USBH_HWCTRL_Handle_End_Session(usbh_hwctrl_handle, data, length);
            break;
            
        case (NU_USB_OTG_IOCTL_BASE + NU_USB_OTG_IOCTL_START_FUNC_SESSION):
            status = USBF_HWCTRL_Tgt_Handle_Start_Session(usbf_hwctrl_handle, data, length);
            break;
            
        case (NU_USB_OTG_IOCTL_BASE + NU_USB_OTG_IOCTL_END_FUNC_SESSION):
            status = USBF_HWCTRL_Tgt_Handle_End_Session(usbf_hwctrl_handle, data, length);
            break;
            
        case (NU_USB_OTG_IOCTL_BASE + NU_USB_OTG_IOCTL_NOTIFY_ROLE_SWITCH):
            callbacks = (NU_USBOTG_NOTIFY_CALLBACKS *)data;
            status = USBH_HWCTRL_Handle_Notify_Role_Switch(usbh_hwctrl_handle, callbacks->usbh_role_switch_cb,
                                                           sizeof(NU_USB_HW_ROLESWITCH_CALLBACK));
            if(status == NU_SUCCESS)
            {
                status = USBF_HWCTRL_Tgt_Handle_Notify_Role_Switch(usbf_hwctrl_handle, callbacks->usbf_role_switch_cb, 
                                                                   sizeof(NU_USB_HW_ROLESWITCH_CALLBACK));
            }
            break;
            
        default:
            status = DV_INVALID_INPUT_PARAMS;
    }
	
    return (status);
}
/* ======================== End of File ================================ */