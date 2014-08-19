/**************************************************************************
*
*               Copyright 2005  Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       nu_usbh_com_acm_ext.c
*
*
* COMPONENT
*
*       Nucleus USB host software. communication class driver.
*
* DESCRIPTION
*
*       This file contains the implementation for API and internal
*       functions provided by Nucleus USB host Communication class driver's
*        ACM component.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBH_COM_Set_Line_Coding       Class specific request
*                                         implementation.
*       NU_USBH_COM_Get_Line_Coding       Class specific request
*                                         implementation.
*       NU_USBH_COM_Set_Ctrl_LS           Class specific request
*                                         implementation.
*       NU_USBH_COM_Send_Break            Class specific request
*                                         implementation.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

#include "connectivity/nu_usb.h"

#ifdef INC_ACM_MDL

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Set_Line_Coding
*
* DESCRIPTION
*     This function is responsible for the execution of class specific
*     request which sets serial line coding scheme.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block.
*     p_data_buf         Pointer to line coding structure.
*     data_length        Size of line coding structure.
*
* OUTPUTS
*     NU_SUCCESS              Indicates successful completion.
*     NU_USBH_COM_XFER_ERR    Indicates command didn't complete
*                             successfully.
*     NU_USBH_COM_XFER_FAILED Indicates command failed by Communication
*                             device.
*
**************************************************************************/

STATUS NU_USBH_COM_Set_Line_Coding(
       NU_USBH_COM_DEVICE*   pcb_curr_device,
       VOID*                 p_data_buf)
{
    STATUS   status, irp_status;
    UINT8              intf_num;
    UNSIGNED         ret_events;
    NU_USBH_CTRL_IRP *cb_ctrl_irp;
    UINT8*  p8_ar_temp_buffer = NU_NULL;
    UINT8*  p8_temp = (UINT8*) p_data_buf;

    UINT32 temp1 = 1;
    UINT8 *temp2 = (UINT8*) &temp1;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* To lock device's control pipe. */
    status = NU_Obtain_Semaphore (&pcb_curr_device->sm_ctrl_trans,
                                  NU_SUSPEND);
    if(status != NU_SUCCESS)
    {
        /* Revert to user mode. */
        NU_USER_MODE();
        return status;
    }

    /* Allocate uncached memory buffer for data IO. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 7,
                                 (VOID **) &p8_ar_temp_buffer);
    if(status == NU_SUCCESS)
    {
        memcpy(p8_ar_temp_buffer,p8_temp,0x07);

        if (!*temp2)
        {
            p8_ar_temp_buffer[3] = p8_temp[0];
            p8_ar_temp_buffer[2] = p8_temp[1];
            p8_ar_temp_buffer[1] = p8_temp[2];
            p8_ar_temp_buffer[0] = p8_temp[3];
        }

        /* Getting the interface number. */
        NU_USB_INTF_Get_Intf_Num (pcb_curr_device->pcb_com_intf,
                                  &intf_num);

        /* Get and clear contents of current device control IRP block. */
        cb_ctrl_irp = pcb_curr_device->ctrl_irp;
        memset(cb_ctrl_irp, 0, sizeof(NU_USBH_CTRL_IRP));

        /* Form the control request. */
        NU_USBH_CTRL_IRP_Create (cb_ctrl_irp,
                                 p8_ar_temp_buffer,
                                 NU_USBH_COM_Ctrl_IRP_Complete,
                                 pcb_curr_device,
                                 0x21,
                                 UH_SET_LINE_CODING,
                                 0x00,
                                 HOST_2_LE16 (intf_num),
                                 HOST_2_LE16 (0x0007));

        /* Submits the IRP. */
        NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                                (NU_USB_IRP *)cb_ctrl_irp);

        /* Wait for the the IRP to be completed. */
        NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                            UHC_CTRL_SENT,
                            NU_AND_CONSUME,
                            &ret_events,
                            NU_SUSPEND);

        /* ...and returns status. */
        NU_USB_IRP_Get_Status ((NU_USB_IRP *) cb_ctrl_irp,
                               &irp_status);

        /* Deallocate temporary buffer now. */
        USB_Deallocate_Memory(p8_ar_temp_buffer);
    }

    /* Unlock driver function */
    NU_Release_Semaphore (&pcb_curr_device->sm_ctrl_trans);

    /* Revert to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? irp_status : status);

} /* NU_USBH_COM_Set_Line_Coding */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Get_Line_Coding
*
* DESCRIPTION
*     This function is responsible for the execution of class specific
*     request which gets serial line coding scheme.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block.
*     p_data_buf         Pointer to hold line coding structure.
*     data_length        Size of line coding structure.
*
* OUTPUTS
*     NU_SUCCESS                  Indicates successful completion.
*     NU_USBH_COM_XFER_ERR        Indicates command didn't complete
*                                 successfully.
*     NU_USBH_COM_XFER_FAILED     Indicates command failed by Communication
*                                  device.
*
**************************************************************************/

STATUS NU_USBH_COM_Get_Line_Coding(
       NU_USBH_COM_DEVICE* pcb_curr_device,
       VOID*               p_data_buf)
{
    UINT8*           temp_buffer = NU_NULL;
    STATUS           status,irp_status;
    UINT8            intf_num;
    UNSIGNED         ret_events;
    NU_USBH_CTRL_IRP *cb_ctrl_irp;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* To lock device's control pipe. */
    status = NU_Obtain_Semaphore (&pcb_curr_device->sm_ctrl_trans,
                                  NU_SUSPEND);
    if(status != NU_SUCCESS)
    {
        /* Revert to user mode. */
        NU_USER_MODE();
        return status;
    }

    /* Allocate uncached memory buffer for data IO. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 7,
                                 (VOID **) &temp_buffer);
    if(status == NU_SUCCESS)
    {
        /* Getting the interface number. */
        NU_USB_INTF_Get_Intf_Num (pcb_curr_device->pcb_com_intf,
                                  &intf_num);

        /* Get and clear contents of current device control IRP block. */
        cb_ctrl_irp = pcb_curr_device->ctrl_irp;
        memset(cb_ctrl_irp, 0, sizeof(NU_USBH_CTRL_IRP));

        /* Form the control request. */
        NU_USBH_CTRL_IRP_Create (cb_ctrl_irp,
                                 temp_buffer,
                                 NU_USBH_COM_Ctrl_IRP_Complete,
                                 pcb_curr_device,
                                 0xA1,
                                 UH_GET_LINE_CODING,
                                 0x00,
                                 HOST_2_LE16 (intf_num),
                                 HOST_2_LE16 (0x0007));

        /* Submits the IRP. */
        NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                                (NU_USB_IRP *)cb_ctrl_irp);

        /* Wait for the the IRP to be completed. */
        NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                            UHC_CTRL_SENT,
                            NU_AND_CONSUME,
                            &ret_events,
                            NU_SUSPEND);

        /* ...and returns status. */
        NU_USB_IRP_Get_Status ((NU_USB_IRP *) cb_ctrl_irp,
                               &irp_status);

        /* Copy contents to input memory buffer. */
        memcpy(p_data_buf, temp_buffer, 7);

        /* Deallocate temporary buffer now. */
        USB_Deallocate_Memory(temp_buffer);
    }

    /* Release semaphore which was acquired at the start of execution. */
    NU_Release_Semaphore (&pcb_curr_device->sm_ctrl_trans);

    /* Revert to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? irp_status : status);

} /* NU_USBH_COM_Get_Line_Coding */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Set_Ctrl_LS
*
* DESCRIPTION
*     This function is responsible for the execution of class specific
*     request which sets RS232 type control states.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block.
*     LS_bmp             Line status bitmap.
*
* OUTPUTS
*     NU_SUCCESS                  Indicates successful completion.
*     NU_USBH_COM_XFER_ERR        Indicates command didn't complete
*                                 successfully.
*     NU_USBH_COM_XFER_FAILED     Indicates command failed by Communication
*                                 device.
*
**************************************************************************/

STATUS NU_USBH_COM_Set_Ctrl_LS(
       NU_USBH_COM_DEVICE* pcb_curr_device,
       UINT16              LS_bmp)
{
    STATUS           status, irp_status;
    UINT8            intf_num;
    UNSIGNED         ret_events;

    NU_USBH_CTRL_IRP *cb_ctrl_irp;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* To lock device's control pipe. */
    status = NU_Obtain_Semaphore (&pcb_curr_device->sm_ctrl_trans,
                                  NU_SUSPEND);
    if(status != NU_SUCCESS)
    {
        /* Revert to user mode. */
        NU_USER_MODE();
        return status;
    }

    /* Getting the interface number. */
    NU_USB_INTF_Get_Intf_Num (pcb_curr_device->pcb_com_intf,
                              &intf_num);

    /* Get and clear contents of current device control IRP block. */
    cb_ctrl_irp = pcb_curr_device->ctrl_irp;
    memset(cb_ctrl_irp, 0, sizeof(NU_USBH_CTRL_IRP));

    /* Form the control request. */
    NU_USBH_CTRL_IRP_Create (cb_ctrl_irp,
                             NU_NULL,
                             NU_USBH_COM_Ctrl_IRP_Complete,
                             pcb_curr_device,
                             0x21,
                             UH_SET_CONTROL_LINE_STATE,
                             HOST_2_LE16(LS_bmp),
                             HOST_2_LE16 (intf_num),
                             HOST_2_LE16 (0x00));

    /* Submits the IRP. */
    NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                            (NU_USB_IRP *)cb_ctrl_irp);

    /* Wait for the the IRP to be completed. */
    NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                        UHC_CTRL_SENT,
                        NU_AND_CONSUME,
                        &ret_events,
                        NU_SUSPEND);

    /* ...and returns status. */
    NU_USB_IRP_Get_Status ((NU_USB_IRP *) cb_ctrl_irp,
                           &irp_status);

    NU_Release_Semaphore (&pcb_curr_device->sm_ctrl_trans);

    /* Revert to user mode. */
    NU_USER_MODE();

    return (irp_status);

} /* NU_USBH_COM_Set_Ctrl_LS */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Send_Break
*
* DESCRIPTION
*     This function is responsible for the execution of class specific
*     request which sets RS232 style break timing.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block.
*     break_time         Time to break in milliseconds.
*
* OUTPUTS
*     NU_SUCCESS                  Indicates successful completion.
*     NU_USBH_COM_XFER_ERR        Indicates command didn't complete
*                                 successfully.
*     NU_USBH_COM_XFER_FAILED     Indicates command failed by Communication
*                                 device.
*
**************************************************************************/

STATUS NU_USBH_COM_Send_Break(
       NU_USBH_COM_DEVICE* pcb_curr_device,
       UINT16              break_time)
{
    STATUS           status, irp_status;
    UINT8            intf_num;
    UNSIGNED         ret_events;
    NU_USBH_CTRL_IRP *cb_ctrl_irp;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* To lock device's control pipe. */
    status = NU_Obtain_Semaphore (&pcb_curr_device->sm_ctrl_trans,
                                  NU_SUSPEND);
    if(status != NU_SUCCESS)
    {
        /* Revert to user mode. */
        NU_USER_MODE();
        return status;
    }

    /* Getting the interface number. */
    NU_USB_INTF_Get_Intf_Num (pcb_curr_device->pcb_com_intf,
                              &intf_num);

      /* Get and clear contents of current device control IRP block. */
    cb_ctrl_irp = pcb_curr_device->ctrl_irp;
    memset(cb_ctrl_irp, 0, sizeof(NU_USBH_CTRL_IRP));

    /* Form the control request. */
    NU_USBH_CTRL_IRP_Create (cb_ctrl_irp,
                             NU_NULL,
                             NU_USBH_COM_Ctrl_IRP_Complete,
                             pcb_curr_device,
                             0x21,
                             UH_SEND_BREAK,
                             HOST_2_LE16(break_time),
                             HOST_2_LE16 (intf_num),
                             HOST_2_LE16 (0x00));

    /* Submits the IRP. */
    NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                            (NU_USB_IRP *)cb_ctrl_irp);

    /* Wait for the the IRP to be completed. */
    NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                        UHC_CTRL_SENT,
                        NU_AND_CONSUME,
                        &ret_events,
                        NU_SUSPEND);

    /* ...and returns status. */
    NU_USB_IRP_Get_Status ((NU_USB_IRP *) cb_ctrl_irp,
                           &irp_status);

    NU_Release_Semaphore (&pcb_curr_device->sm_ctrl_trans);

    /* Revert to user mode. */
    NU_USER_MODE();

    return (irp_status);

} /*  NU_USBH_COM_Send_Break */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Check_ACM_Func_Desc
*
* DESCRIPTION
*     This function is responsible for the parsing of functional
*     descriptors for ACM device model.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block.
*     UINT8*             Pointer to class specific descriptor information.
*     UINT8              Total length for all class specific descriptors.
*
* OUTPUTS
*     NU_SUCCESS                  Indicates successful completion.
*     NU_USBH_COM_XFER_ERR        Indicates command didn't complete.
*                                 successfully.
*     NU_USBH_COM_XFER_FAILED     Indicates command failed by Communication
*                                 device.
*
**************************************************************************/

VOID NU_USBH_COM_Check_ACM_Func_Desc(
     NU_USBH_COM_DEVICE* pcb_curr_device,
     UINT8*              class_desc,
     UINT32              temp_length)
{
    NU_USBH_COM_ACM_INFORM *pcb_inform;
    UINT8*                  func_desc;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Allocating a structure to hold ACM specific information. */

    USB_Allocate_Object(sizeof(NU_USBH_COM_ACM_INFORM),
                        (VOID**)&(pcb_curr_device->model_spc_inform));

    if(pcb_curr_device->model_spc_inform == NU_NULL)
    {
        /* Revert to user mode. */
        NU_USER_MODE();
        return;
    }

    memset(pcb_curr_device->model_spc_inform,
           0x00,
           sizeof(NU_USBH_COM_ACM_INFORM));

    pcb_inform =
    (NU_USBH_COM_ACM_INFORM*)(pcb_curr_device->model_spc_inform);

    /* Determining the string index from class specific descriptors. */

    func_desc = NU_USBH_COM_Parse_Strings(class_desc,
                                  temp_length,
                                  UH_CALL_MGMT_FD);
    if(func_desc)
    {
        pcb_inform->call_cap= *(func_desc);

        func_desc = NU_USBH_COM_Parse_Strings(class_desc,
                                              temp_length,
                                              UH_ACM_MGMT_FD);
        if(func_desc)
        {
            pcb_inform->acm_cap= *(func_desc);
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

}/* NU_USBH_COM_Check_ACM_Func_Desc */

#endif /* INC_ACM_MDL */
