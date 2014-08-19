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
*       nu_usbh_com_ext.c
*
*
* COMPONENT
*
*       Nucleus USB host software. Communication class driver.
*
* DESCRIPTION
*
*       This file contains the implementation for API functions provided by
*       Nucleus USB host communication class driver.
*
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBH_COM_Create                  Creates communication class
*                                           driver.
*       NU_USBH_COM_Delete                  Deletes a communication class
*                                           driver.
*       _NU_USBH_COM_Initialize_Intf        Connect callback for
*                                           Communication interface.
*       _NU_USBH_COM_Data_Initialize_Intf   Connect callback for data
*                                           interface.
*       NU_USBH_COM_Data_Transfer           Transfers the data packets to
*                                           or from a Communication device.
*       NU_USBH_COM_Send_Encap_Cmd          Send encapsulated command.
*       NU_USBH_COM_Get_Encap_Resp          Receive encapsulated command's
*                                           response.
*       NU_USBH_COM_Set_Comm_Feature        Set a feature on communication
*                                           device.
*       NU_USBH_COM_Get_Comm_Feature        Get details for a specific
*                                           feature from a communication
*                                           device.
*       NU_USBH_COM_Clear_Comm_Feature      Clear details for a specific
*                                           feature on a communication
*                                           device.
*       _NU_USBH_COM_Disconnect             Communication interface
*                                           disconnect callback function.
*       _NU_USBH_COM_Data_Disconnect        Data interface disconnect
*                                           callback function.
*       NU_USBH_COMM_Init                   This function initializes the
*                                           Communication Class Component.
*       NU_USBH_COMM_Init_GetHandle         This function is called to
*                                           retrieve the Host Communication
*                                           class driver's address.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* USB Include Files */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "services/runlevel_init.h"

/* ==========================  Functions ============================== */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Create
*
* DESCRIPTION
*     Communication driver initialization routine. Two class drivers and a
*     semaphore created for the purpose.
*
* INPUTS
*     pcb_com_drvr          Pointer to Communication driver control block.
*     p_name                Name of Communication class host driver.
*     p_memory_pool         Pointer to memory pool available to the driver.
*     usb_stack             Pointer to USB stack to whom driver is to
*                           register.
*
* OUTPUTS
*     NU_SUCCESS            Successful Initialization.
*     NU_USB_INVLD_ARG      Indicates that the driver passed is NU_NULL.
*     NU_INVALID_SEMAPHORE  Indicates that the internally created semaphore
*                           pointer is invalid.
*     NU_SEMAPHORE_DELETED  Indicates that the internally created semaphore
*                           was deleted while the task was suspended.
*     NU_UNAVAILABLE        Indicates that the internally created semaphore
*                           is unavailable.
*     NU_INVALID_SUSPEND    Indicates that this API is called from a non
*                           task thread.
*
**************************************************************************/
STATUS NU_USBH_COM_Create (
       NU_USBH_COM*     pcb_com_drvr,
       CHAR*            p_name,
       NU_MEMORY_POOL*  p_memory_pool,
       NU_USB_STACK*    usb_stack)
{
    STATUS status;
    UINT8  roll_back = 0;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_MEMPOOLCHK_RETURN(p_memory_pool);

    pcb_com_drvr->p_memory_pool = p_memory_pool;

    /* Creating the Semaphore. This semaphore is created for general usage
     * in the Communication driver.
     */
    status = NU_Create_Semaphore (&(pcb_com_drvr->sm_comdrvr_lock),
                                  "COMDRVSM",
                                  1,
                                  NU_FIFO);

    if(status == NU_SUCCESS)
    {
        /* Creating the base class component for Communication interface.*/
        status = _NU_USBH_DRVR_Create ((NU_USB_DRVR *) pcb_com_drvr,
                                       p_name,
                                       USB_MATCH_CLASS,
                                       0,
                                       0,
                                       0,
                                       0,
                                       UH_COM_CLASS_CODE,
                                       0,
                                       0,
                                       &usbh_com_dispatch);
    }
    else
    {
       roll_back = 0;
    }

    if(status == NU_SUCCESS)
    {
        /* Registering the Communication driver with the host stack. */
        status = NU_USB_STACK_Register_Drvr (usb_stack,
                                             (NU_USB_DRVR*)pcb_com_drvr);

    }
    else
    {
       roll_back = 1;
    }

    /* Creating the base class component for data interface. */
    if(status == NU_SUCCESS)
    {
        status = _NU_USBH_DRVR_Create (
                      (NU_USB_DRVR*)(&(pcb_com_drvr->cb_data_drvr)),
                                       "DATA_DRVR",
                                       USB_MATCH_CLASS,
                                       0,
                                       0,
                                       0,
                                       0,
                                       UH_COM_DATA_CLASS_CODE,
                                       0,
                                       0,
                                       &usbh_com_data_dispatch);
    }
    else
    {
       roll_back = 2;
    }

    /* Registering the Communication driver's data interface with the host
     * stack.
     */
    if(status == NU_SUCCESS)
    {
        pcb_com_drvr->cb_data_drvr.pcb_com_drvr = (VOID*)pcb_com_drvr;

        status = NU_USB_STACK_Register_Drvr ((NU_USB_STACK*)usb_stack,
                 (NU_USB_DRVR*)(&(pcb_com_drvr->cb_data_drvr)));
    }
    else
    {
       roll_back = 3;
    }

    switch(roll_back)
    {
        case 3:
        {
            _NU_USBH_DRVR_Delete((NU_USB_DRVR*)
                                 &(pcb_com_drvr->cb_data_drvr));
        }
        case 2:
        {
            _NU_USBH_DRVR_Delete((NU_USB_DRVR*) pcb_com_drvr);
        }
        case 1:
        {
            NU_Delete_Semaphore (&(pcb_com_drvr->sm_comdrvr_lock));
        }
        case 0:
        {
            break;
        }

        default:{break;}
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}/* NU_USBH_COM_Create */

/**************************************************************************
* FUNCTION
*    _NU_USBH_COM_Delete
*
* DESCRIPTION
*     This function deletes the communication driver and all interfaces
*     claimed by this driver are given disconnect callback and the
*     interfaces are released. Driver is also deregistered from the stack
*     it was registered before deletion. Note that this function does not
*     free the memory associated with the Communication driver control
*     block.
*
* INPUTS
*     cb             Pointer to the communication class host driver.
*
* OUTPUTS
*     NU_SUCCESS     Indicates successful completion always.
*
**************************************************************************/

STATUS _NU_USBH_COM_Delete (VOID* cb)
{
    STATUS             status;
    NU_USBH_COM        *pcb_com_drvr = (NU_USBH_COM*) cb;
    NU_USBH_COM_DEVICE *p_next_device,
                       *p_curr_device = pcb_com_drvr->pcb_first_device;

    /* For each connected Communication device. */
    while (p_curr_device)
    {
        p_next_device =(NU_USBH_COM_DEVICE*) p_curr_device->node.cs_next;

        /* Sends disconnect event, for each Communication interface. */
        _NU_USBH_COM_Disconnect ((NU_USB_DRVR *) pcb_com_drvr,
                                 p_curr_device->pcb_stack,
                                 p_curr_device->pcb_device);

        /* Sends disconnect event, for each data interface. */
        _NU_USBH_COM_Data_Disconnect (
                          (NU_USB_DRVR*)(&(pcb_com_drvr->cb_data_drvr)),
                          p_curr_device->pcb_stack,
                          p_curr_device->pcb_device);

        /* All devices delisted from the stack */
        if(p_curr_device == p_next_device)
        {
            break;
        }
        p_curr_device = p_next_device;
    }

    /* Deletes the instance of semaphore inside control block. */
    NU_Delete_Semaphore (&(pcb_com_drvr->sm_comdrvr_lock));

    /* Calls base behavior. */
    _NU_USBH_DRVR_Delete ((VOID*)(&(pcb_com_drvr->cb_data_drvr)));

    status = _NU_USBH_DRVR_Delete (cb);

    return (status);

} /* _NU_USBH_COM_Delete */

/**************************************************************************
* FUNCTION
*     _NU_USBH_COM_Initialize_Intf
*
* DESCRIPTION
*     Connect callback function invoked by stack when an interface with
*     Communication class is found on a device.
*
* INPUTS
*     pcb_drvr     Pointer to class driver control block.
*     pcb_stack    Pointer to stack control block of the calling stack.
*     pcb_device   Pointer to device control block of the device found.
*     pcb_intf     Pointer to interface control block to be served by this
*                  class driver.
*
* OUTPUTS
*     NU_SUCCESS              Indicates successful completion of the
*                             service.
*     NU_NOT_PRESENT          Indicates no alternate setting with
*                             supported protocols is found.
*                             Indicates no user associated with the
*                             subclass is found.
*                             Indicates that the endpoints required by the
*                             protocol are not found.
*     NU_USB_INVLD_ARG        Indicates some control block(s) is(are)
*                             deleted before completion.
*
**************************************************************************/
STATUS _NU_USBH_COM_Initialize_Intf (
       NU_USB_DRVR*   pcb_drvr,
       NU_USB_STACK*  pcb_stack,
       NU_USB_DEVICE* pcb_device,
       NU_USB_INTF*   pcb_intf)
{
    STATUS status          = NU_SUCCESS;
    UINT8  alt_setting_num = 0,
           sub_class = 0,
           ep_count,
           roll_back = 0;

    NU_USB_ALT_SETTG   *pcb_alt_setting;
    NU_USBH_COM        *pcb_com_drvr  = (NU_USBH_COM*) pcb_drvr;
    NU_USBH_COM_DEVICE *pcb_curr_device = NU_NULL;
    BOOLEAN subclass_found = NU_FALSE;

    /* Search for suitable alternate setting of current interface. */
    status = NU_USB_INTF_Find_Alt_Setting(pcb_intf,
                                          USB_MATCH_CLASS,
                                          alt_setting_num,
                                          UH_COM_CLASS_CODE,
                                          0,
                                          0,
                                          &pcb_alt_setting);
    if (status == NU_SUCCESS)
    {
        /* Setting the current interface setting. */
        status = NU_USB_ALT_SETTG_Set_Active (pcb_alt_setting);

        status = USB_Allocate_Object(sizeof (NU_USBH_COM_DEVICE),
                                     (VOID **) &pcb_curr_device);
        if (status == NU_SUCCESS)
        {
            memset (pcb_curr_device,
                    0,
                    sizeof (NU_USBH_COM_DEVICE));

            NU_Place_On_List (
             (CS_NODE **) & pcb_com_drvr->pcb_first_device,
             (CS_NODE *)  pcb_curr_device);
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Allocate memory for Endpoint 0 IRP. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof(NU_USBH_CTRL_IRP),
                                     (VOID **)&(pcb_curr_device->ctrl_irp));
        if(status != NU_SUCCESS)
        {
            roll_back = 1;
        }
        else
        {
            memset (pcb_curr_device->ctrl_irp, 0, sizeof(NU_USBH_CTRL_IRP));
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Finds a NU_USB_USER suitable for the device. We match
         * bInterfaceSubClass for it.
         */
        NU_USB_ALT_SETTG_Get_SubClass (pcb_alt_setting,
                                       &sub_class);

        pcb_curr_device->pcb_user_drvr = (NU_USB_USER*)
                                          NU_USBH_COM_Find_User
                                          (pcb_com_drvr, sub_class);
        subclass_found = NU_TRUE;

        if (pcb_curr_device->pcb_user_drvr == NU_NULL)
        {
            status =  NU_NOT_PRESENT;
            roll_back = 2;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Finds the default pipe. */
        NU_USB_ALT_SETTG_Find_Pipe (pcb_alt_setting,
                                    USB_MATCH_EP_ADDRESS,
                                    0,
                                    0,
                                    0,
                                    &pcb_curr_device->pcb_control_pipe);

        NU_USB_ALT_SETTG_Get_Num_Endps (pcb_alt_setting,
                                        &ep_count);

        if(ep_count)
        {
            /* Finds the optional interrupt endpoint pipe. */
            NU_USB_ALT_SETTG_Find_Pipe (pcb_alt_setting,
                                        USB_MATCH_EP_TYPE |
                                        USB_MATCH_EP_DIRECTION,
                                        0,
                                        0x80,
                                        3,
                                        &pcb_curr_device->pcb_intr_in_pipe);
        }
    }

    if (status == NU_SUCCESS)
    {
        pcb_curr_device->pcb_device   = pcb_device;
        pcb_curr_device->pcb_com_intf = pcb_intf;
        pcb_curr_device->pcb_com_drvr = pcb_drvr;
        pcb_curr_device->pcb_stack    = pcb_stack;
        pcb_curr_device->pcb_com_alt_settg = pcb_alt_setting;

        /* Both interfaces are reported. So its time to evaluate model
         * specific things and reporting to user driver.
         */
        status = NU_USBH_COM_Check_Union(pcb_device,
                                         pcb_intf,
                                         pcb_curr_device);
        if(status != NU_SUCCESS)
        {
            roll_back = 2;
        }
        else
        {
            pcb_curr_device->pcb_data_drvr = (NU_USB_DRVR*)
                                              &pcb_com_drvr->cb_data_drvr;
        }
    }

    if((status == NU_SUCCESS) && (subclass_found == NU_TRUE))
    {
        status = NU_USBH_COM_Init_Device((NU_USBH_COM*)pcb_drvr,
                                         pcb_curr_device,
                                         pcb_intf,
                                         sub_class);
        if(status == NU_SUCCESS )
        {
            /* Interface claiming by class driver. */
            status = NU_USB_INTF_Claim (pcb_curr_device->pcb_com_intf,
                                        pcb_drvr);

            /* Reports user driver for device connection */
            (((NU_USBH_COM_USER_DISPATCH *) (((NU_USB *)
            pcb_curr_device->pcb_user_drvr)->usb_dispatch))->
            Connect_Handler (pcb_curr_device->pcb_user_drvr,
                             pcb_curr_device->pcb_com_drvr,
                             pcb_curr_device,
                             pcb_curr_device->model_spc_inform));
        }
        else
        {
            roll_back = 0x03;
        }
    }

    switch(roll_back)
    {
        case (0x03):{}

        case (0x02):
        {
            USB_Deallocate_Memory (pcb_curr_device->ctrl_irp);
        }

        case (0x01):
        {
            NU_Remove_From_List(
                               (CS_NODE **)&pcb_com_drvr->pcb_first_device,
                               (CS_NODE *) pcb_curr_device);

            USB_Deallocate_Memory (pcb_curr_device);
        }

        default:{;}
    }

    return status;
} /* _NU_USBH_COM_Initialize_Intf */

/**************************************************************************
* FUNCTION
*     _NU_USBH_COM_Data_Initialize_Intf
*
* DESCRIPTION
*     Connect callback function invoked by stack when an interface with
*     Communication class's data interface is found on a device.
*
* INPUTS
*     pcb_drvr     Pointer to class driver control block.
*     pcb_stack    Pointer to Stack control block of the calling stack.
*     pcb_device   Pointer to device control block of the device found.
*     pcb_intf     Pointer to interface control block to be served by this
*                  class driver.
*
* OUTPUTS
*     NU_SUCCESS              Indicates successful completion of the
*                             service.
*     NU_NOT_PRESENT          Indicates no alternate setting with
*                             supported protocols is found.
*                             Indicates no user associated with the
*                             subclass is found.
*                             Indicates, Endpoints required by the
*                             protocol are not found.
*     NU_USB_INVLD_ARG        Indicates some control block(s) is(are)
*                             deleted before completion.
*
**************************************************************************/
STATUS _NU_USBH_COM_Data_Initialize_Intf (
       NU_USB_DRVR*   pcb_drvr,
       NU_USB_STACK*  pcb_stack,
       NU_USB_DEVICE* pcb_device,
       NU_USB_INTF*   pcb_intf)
{
    STATUS status          = NU_SUCCESS;
    status = NU_USB_INTF_Claim (pcb_intf, pcb_drvr);
    return status;
}/* _NU_USBH_COM_Data_Initialize_Intf */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Transfer
*
* DESCRIPTION
*     This function is responsible for sending or receiving communication
*     data to the attached device.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block.
*     pcb_com_xblock     Pointer to control block for Communication
*                        transfers.
*
* OUTPUTS
*     NU_SUCCESS                   Indicates successful completion.
*     NU_USBH_COM_XFER_ERR         Indicates command didn't complete
*                                  successfully.
*     NU_USBH_COM_XFER_FAILED      Indicates command failed by
*                                  Communication Class Driver.
*     NU_USBH_COM_NOT_SUPPORTED    Indicates that the feature is not
*                                  supported by the Communication Class
*                                  Driver.
*
**************************************************************************/

STATUS NU_USBH_COM_Transfer (
       NU_USBH_COM_DEVICE*  pcb_curr_device,
       NU_USBH_COM_XBLOCK*  pcb_com_xblock)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    if(pcb_com_xblock->direction == NU_USBH_COM_DATA_OUT)
    {
        status = NU_USBH_COM_Transfer_Out(pcb_curr_device,
                                        pcb_com_xblock);
    }
    else if(pcb_com_xblock->direction == NU_USBH_COM_DATA_IN)
    {
        status = (NU_USBH_COM_Transfer_In(pcb_curr_device,
                                       pcb_com_xblock));

    }
    else
    {
        status = NU_USBH_COM_NOT_SUPPORTED;
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;

}/* NU_USBH_COM_Transfer */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Send_Encap_Command
*
* DESCRIPTION
*     This function is responsible for execution of class specific cmd.
*
* INPUTS
*     pcb_curr_device   Pointer to Communication device control block
*     p_data_buf        Pointer to buffer to send Communication command.
*     data_length       Byte length of buffer to send Communication command.
*
* OUTPUTS
*     NU_SUCCESS                   Indicates successful completion.
*     NU_USBH_COM_XFER_ERR         Indicates command didn't complete
*                                  successfully.
*     NU_USBH_COM_XFER_FAILED      Indicates command failed by
*                                  Communication Class Driver.
*
**************************************************************************/

STATUS NU_USBH_COM_Send_Encap_Cmd (
       NU_USBH_COM_DEVICE* pcb_curr_device,
       VOID*               p_data_buf,
       UINT32              data_length)
{
    UINT8*           temp_buffer;
    STATUS           status, irp_status;
    UINT8            intf_num;
    UNSIGNED         ret_events;

    NU_USBH_CTRL_IRP *cb_ctrl_irp;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

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
                                 data_length,
                                 (VOID **) &temp_buffer);
    if(status == NU_SUCCESS)
    {
        /* Copy contents into buffer for data IO. */
        memcpy(temp_buffer, p_data_buf, data_length);

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
                                 0x21,
                                 UH_SEND_ENCAPSULATED_COMMAND,
                                 0,
                                 HOST_2_LE16 (intf_num),
                                 HOST_2_LE16 (data_length));

        /* Submits the IRP. */
        irp_status = NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                                            (NU_USB_IRP *)cb_ctrl_irp);

        if(irp_status == NU_SUCCESS)
        {
            /* Wait for the the IRP to be completed. */
            NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                                UHC_CTRL_SENT,
                                NU_AND_CONSUME,
                                &ret_events,
                                NU_SUSPEND);

            /* ...and returns status. */
            NU_USB_IRP_Get_Status ((NU_USB_IRP *) cb_ctrl_irp,
                                   &irp_status);
        }

        /* Deallocate temporary buffer now. */
        USB_Deallocate_Memory(temp_buffer);
    }

    /* Unlock class driver functionality */
    NU_Release_Semaphore (&(pcb_curr_device->sm_ctrl_trans));

    /* Revert to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? irp_status : status);

}/* NU_USBH_COM_Send_Encap_Cmd */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Get_Encap_Command
*
* DESCRIPTION
*     This function is responsible for execution of class specific cmd.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block
*     p_data_buf         Pointer to buffer to hold Communication command.
*     data_length        Byte length of buffer to hold Communication command.
*
* OUTPUTS
*     NU_SUCCESS                   Indicates successful completion.
*     NU_USBH_COM_XFER_ERR         Indicates command didn't complete
*                                  successfully.
*     NU_USBH_COM_XFER_FAILED      Indicates command failed by
*                                  Communication Class Driver.
*
**************************************************************************/

STATUS NU_USBH_COM_Get_Encap_Resp (
       NU_USBH_COM_DEVICE*  pcb_curr_device,
       VOID*                p_data_buf,
       UINT32               data_length)
{
    UINT8*           temp_buffer;
    STATUS           status, irp_status;
    UINT8    intf_num;
    UNSIGNED ret_events;

    NU_USBH_CTRL_IRP *cb_ctrl_irp;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

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
                                 data_length,
                                 (VOID **) &temp_buffer);
    if(status == NU_SUCCESS)
    {
        /* Clear temp buffer contents. */
        memset(temp_buffer, 0, data_length);

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
                                 UH_GET_ENCAPSULATED_RESPONSE,
                                 0,
                                 HOST_2_LE16 (intf_num),
                                 HOST_2_LE16 (data_length));

        /* Submits the IRP. */
        irp_status = NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                                            (NU_USB_IRP *)&cb_ctrl_irp);

        if(irp_status == NU_SUCCESS)
        {
            /* Wait for the the IRP to be completed. */
            NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                                UHC_CTRL_SENT,
                                NU_AND_CONSUME,
                                &ret_events,
                                NU_SUSPEND);

            /* ...and returns status. */
            NU_USB_IRP_Get_Status ((NU_USB_IRP *) cb_ctrl_irp,
                                   &irp_status);

            /* Copy contents back to input buffer. */
            memcpy(p_data_buf, temp_buffer, data_length);
        }

        /* Deallocate temporary buffer now. */
        USB_Deallocate_Memory(temp_buffer);
    }

    /* Unlock class driver functionality */
    NU_Release_Semaphore (&(pcb_curr_device->sm_ctrl_trans));

    /* Revert to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? irp_status : status);

} /* NU_USBH_COM_Get_Encap_Resp */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Set_Comm_Feature
*
* DESCRIPTION
*     This function is responsible for execution of class specific cmd.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block
*     p_data_buf         Pointer to buffer to send feature structure.
*     data_length        Byte length of buffer to send feature structure.
*     feature_sel        Feature number to set.
*
* OUTPUTS
*     NU_SUCCESS                   Indicates successful completion.
*     NU_USBH_COM_XFER_ERR         Indicates command didn't completed
*                                  successfully.
*     NU_USBH_COM_XFER_FAILED      Indicates command failed by
*                                   Communication Class Driver.
*
**************************************************************************/

STATUS NU_USBH_COM_Set_Comm_Feature (
       NU_USBH_COM_DEVICE*  pcb_curr_device,
       VOID*                p_data_buf,
       UINT32               data_length,
       UINT16               feature_sel)
{
    UINT8*   temp_buffer;
    STATUS   status, irp_status;
    UINT8    intf_num;
    UNSIGNED ret_events;
    NU_USBH_CTRL_IRP *cb_ctrl_irp;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

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
                                 data_length,
                                 (VOID **) &temp_buffer);
    if(status == NU_SUCCESS)
    {
        /* Copy contents into buffer for data IO. */
        memcpy(temp_buffer, p_data_buf, data_length);

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
                                 0x21,
                                 UH_SET_COMM_FEATURE,
                                 feature_sel,
                                 HOST_2_LE16 (intf_num),
                                 HOST_2_LE16 (data_length));

        /* Submits the IRP. */
        irp_status = NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                                            (NU_USB_IRP *)&cb_ctrl_irp);

        if(irp_status == NU_SUCCESS)
        {
            /* Wait for the the IRP to be completed. */
            NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                                UHC_CTRL_SENT,
                                NU_AND_CONSUME,
                                &ret_events,
                                NU_SUSPEND);

            /* ...and returns status. */
            NU_USB_IRP_Get_Status ((NU_USB_IRP *) cb_ctrl_irp,
                                   &irp_status);
        }

        /* Deallocate temporary buffer now. */
        USB_Deallocate_Memory(temp_buffer);
    }

    /* Unlock class driver functionality */
    NU_Release_Semaphore (&(pcb_curr_device->sm_ctrl_trans));

    /* Revert to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? irp_status : status);

} /* NU_USBH_COM_Set_Comm_Feature */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Set_Comm_Feature
*
* DESCRIPTION
*     This function is responsible for execution of class specific cmd.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block
*     p_data_buf         Pointer to buffer to hold feature structure.
*     data_length        Byte length of buffer to hold feature structure.
*     feature_sel        Feature number to get.
*
* OUTPUTS
*     NU_SUCCESS                   Indicates successful completion.
*     NU_USBH_COM_XFER_ERR         Indicates command didn't completed
*                                  successfully.
*     NU_USBH_COM_XFER_FAILED      Indicates command failed by
*                                   Communication Class Driver.
*
**************************************************************************/

STATUS NU_USBH_COM_Get_Comm_Feature (
       NU_USBH_COM_DEVICE*  pcb_curr_device,
       VOID*                p_data_buf,
       UINT32               data_length,
       UINT16               feature_sel)
{
    UINT8*           temp_buffer;
    STATUS           status, irp_status;
    UINT8     intf_num;
    UNSIGNED  ret_events;
    NU_USBH_CTRL_IRP *cb_ctrl_irp;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

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
                                 data_length,
                                 (VOID **) &temp_buffer);
    if(status == NU_SUCCESS)
    {
        /* Clear temp buffer contents. */
        memset(temp_buffer, 0, data_length);

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
                                 UH_GET_COMM_FEATURE,
                                 0,
                                 HOST_2_LE16 (intf_num),
                                 HOST_2_LE16 (data_length));

        /* Submits the IRP. */
        irp_status = NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                                            (NU_USB_IRP *)cb_ctrl_irp);

        if(irp_status == NU_SUCCESS)
        {
            /* Wait for the the IRP to be completed. */
            NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                                UHC_CTRL_SENT,
                                NU_AND_CONSUME,
                                &ret_events,
                                NU_SUSPEND);

            /* ...and returns status. */
            NU_USB_IRP_Get_Status ((NU_USB_IRP *) cb_ctrl_irp,
                                   &irp_status);

            /* Copy contents back to input buffer. */
            memcpy(p_data_buf, temp_buffer, data_length);
        }

        /* Deallocate temporary buffer now. */
        USB_Deallocate_Memory(temp_buffer);
    }

    /* Unlock class driver functionality */
    NU_Release_Semaphore (&(pcb_curr_device->sm_ctrl_trans));

    /* Revert to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? irp_status : status);

} /* NU_USBH_COM_Get_Comm_Feature */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Set_Comm_Feature
*
* DESCRIPTION
*     This function is responsible for execution of class specific cmd.
*
* INPUTS
*     pcb_curr_device    Pointer to Communication device control block
*     feature_sel        Feature number to be clear.
*
* OUTPUTS
*     NU_SUCCESS                   Indicates successful completion.
*     NU_USBH_COM_XFER_ERR         Indicates command didn't complete
*                                  successfully.
*     NU_USBH_COM_XFER_FAILED      Indicates command failed by
*                                   Communication Class Driver.
*
**************************************************************************/

STATUS NU_USBH_COM_Clear_Comm_Feature (
       NU_USBH_COM_DEVICE* pcb_curr_device,
       UINT16              feature_sel)
{
    STATUS   status, irp_status;
    UINT8    intf_num;
    UNSIGNED ret_events;
    NU_USBH_CTRL_IRP *cb_ctrl_irp;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_Obtain_Semaphore (&pcb_curr_device->sm_ctrl_trans,
                                  NU_SUSPEND);
    if(status==NU_SUCCESS)
   {
   

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
                                UH_CLEAR_COMM_FEATURE,
                                0,
                                HOST_2_LE16 (intf_num),
                                HOST_2_LE16 (0x00)
                            );

        /* Submits the IRP. */
        irp_status = NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                                        (NU_USB_IRP *)cb_ctrl_irp);

        if(irp_status == NU_SUCCESS)
       {
            /* Wait for the the IRP to be completed. */
            NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                            UHC_CTRL_SENT,
                            NU_AND_CONSUME,
                            &ret_events,
                            NU_SUSPEND);

            /* ...and returns status. */
            NU_USB_IRP_Get_Status ((NU_USB_IRP *) cb_ctrl_irp,
                               &irp_status);

        }
   
        NU_Release_Semaphore (&(pcb_curr_device->sm_ctrl_trans));
   }
   
    /* Revert to user mode. */
    NU_USER_MODE();
    return (status == NU_SUCCESS) ? irp_status : status;
} /* NU_USBH_COM_Clear_Comm_Feature */

/**************************************************************************
* FUNCTION
*     _NU_USBH_COM_Disconnect
*
* DESCRIPTION
*     Disconnect callback function, invoked by stack when an interface
*     with Communication class is removed from the BUS.
*
* INPUTS
*     pcb_drvr      Pointer to class driver control block claimed this
*                   interface.
*     pcb_stack     Pointer to stack control block.
*     pcb_device    Pointer to NU_USB_DEVICE control block disconnected.
*
* OUTPUTS
*     NU_SUCCESS    Indicates successful completion of the service always.
*
**************************************************************************/

STATUS _NU_USBH_COM_Disconnect (
       NU_USB_DRVR*   pcb_drvr,
       NU_USB_STACK*  pcb_stack,
       NU_USB_DEVICE* pcb_device)
{
    NU_USBH_COM        *pcb_com_drvr = (NU_USBH_COM*) pcb_drvr;
    NU_USBH_COM_DEVICE *pcb_next_device,
                       *pcb_curr_device = pcb_com_drvr->pcb_first_device;

    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Scan the list of devices and cleanup all associated ones. */
    while (pcb_curr_device)
    {
        pcb_next_device=
        (NU_USBH_COM_DEVICE *) pcb_curr_device->node.cs_next;

        /* If this interface is associated with the device disconnected. */
        if (pcb_curr_device->pcb_device == pcb_device)
        {
            /* Sends disconnection interrupt to user driver. */

            NU_USB_USER_Disconnect (pcb_curr_device->pcb_user_drvr,
                                    pcb_curr_device->pcb_com_drvr,
                                    pcb_curr_device);

            /* Remove the Communication Structure from the list. */
            if(pcb_curr_device->poll_task != NU_NULL)
            {
                NU_Terminate_Task(pcb_curr_device->poll_task);
                NU_Delete_Task(pcb_curr_device->poll_task);
                USB_Deallocate_Memory (pcb_curr_device->poll_task);
                USB_Deallocate_Memory (pcb_curr_device->poll_stack);
            }

            NU_Delete_Event_Group (&(pcb_curr_device->trans_events));

            /* Delete semaphore to schedule control transfers. */
            NU_Delete_Semaphore (&(pcb_curr_device->sm_ctrl_trans));

            /* Delete semaphore to schedule IN transfers. */
            NU_Delete_Semaphore (&(pcb_curr_device->sm_in_trans));

            /* Delete semaphore to schedule OUT transfers. */
            NU_Delete_Semaphore (&(pcb_curr_device->sm_out_trans));

            NU_Remove_From_List(
                (CS_NODE **) & pcb_com_drvr->pcb_first_device,
                (CS_NODE *) pcb_curr_device);

            /* ...and Deallocate Communication structure. */
            USB_Deallocate_Memory(
                  pcb_curr_device->cb_xfer_block.p_data_buf);
            USB_Deallocate_Memory(pcb_curr_device->model_spc_inform);
            USB_Deallocate_Memory(pcb_curr_device->ctrl_irp);
            USB_Deallocate_Memory(pcb_curr_device);
            status = NU_SUCCESS;
            break;
        }
        else{
            if(pcb_curr_device == (NU_USBH_COM_DEVICE*)
                                 pcb_curr_device->node.cs_next)
            {
                status = NU_USB_INVLD_ARG;
                break;
            }
            pcb_next_device =
            (NU_USBH_COM_DEVICE *) pcb_curr_device->node.cs_next;
            pcb_curr_device = pcb_next_device;
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
} /* _NU_USBH_COM_Disconnect */

/**************************************************************************
* FUNCTION
*     _NU_USBH_COM_Data_Disconnect
*
* DESCRIPTION
*     Disconnect callback function, invoked by stack when an interface
*     with Communication's data class is removed from the BUS.
*
* INPUTS
*     pcb_drvr      Pointer to class driver control block claimed this
*                   Interface.
*     pcb_stack     Pointer to stack control block.
*     pcb_device    Pointer to NU_USB_DEVICE control block disconnected.
*
* OUTPUTS
*     NU_SUCCESS    Indicates successful completion of the service.
*
**************************************************************************/
STATUS _NU_USBH_COM_Data_Disconnect (
       NU_USB_DRVR*   pcb_drvr,
       NU_USB_STACK*  pcb_stack,
       NU_USB_DEVICE* pcb_device)

{
    STATUS status = NU_SUCCESS;

    /* Nothing to do over here. All tasks are handled in communication
     *  interface disconnect.
     */
    return status;
} /* _NU_USBH_COM_Data_Disconnect */

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_COMM_Init
*
*   DESCRIPTION
*
*       This function initializes the communication Class Component.
*
*   INPUTS
*
*       path                                Registry path of component.
*       compctrl                           Flag to find if component is
*                                           being enabled or disabled.
*
*   OUTPUTS
*
*       status             Success or failure of the creation of the
*                          underlying initialization routines.
*
*************************************************************************/
STATUS nu_os_conn_usb_host_comm_class_init(CHAR *path, INT compctrl)
{
    VOID   *stack_handle = NU_NULL;
    UINT8   rollback = 0;
    STATUS  status = NU_SUCCESS, internal_sts = NU_SUCCESS;

    if(compctrl == RUNLEVEL_START)
    {
        /* Allocate memory for USB Host Communication */
        status = USB_Allocate_Object(sizeof(NU_USBH_COM),
                                     (VOID **)&NU_USBH_COMM_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 1;
        }

        if (!rollback)
        {
            /* Get the Host stack handle. */
            status = NU_USBH_Init_GetHandle (&stack_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        if (!rollback)
        {
            /* Zero out allocated block. */
            memset(NU_USBH_COMM_Cb_Pt, 0, sizeof(NU_USBH_COM));

            /* Creating Communication class driver.
             * In following API call, passing memory pool ptr parameter
             * NU_NULL because in ReadyStart memory in USB system is
             * allocated through USB specific memory APIs, not directly
             * with any given memory pool pointer. This parameter remains
             * only for backwards code compatibility. */
            status = NU_USBH_COM_Create (NU_USBH_COMM_Cb_Pt,
                                         "USBH-COMM",
                                         NU_NULL,
                                         (NU_USB_STACK* )stack_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        /* Clean up in case error occurs. */
        switch (rollback)
        {
            case 2:
                if (NU_USBH_COMM_Cb_Pt)
                {
                    
                    internal_sts = USB_Deallocate_Memory((VOID *)NU_USBH_COMM_Cb_Pt);
                    NU_USBH_COMM_Cb_Pt = NU_NULL;
                }

            case 1:
            case 0:
            /* internal_sts is not used after this. So to remove
             * KW and PC-Lint warning set it as unused parameter.
             */
            NU_UNUSED_PARAM(internal_sts);
        }
    }
    else if(compctrl== RUNLEVEL_STOP)
    {
        status = USB_Deallocate_Memory(NU_USBH_COMM_Cb_Pt);
        status =  _NU_USBH_COM_Delete (NU_USBH_COMM_Cb_Pt);
        status = NU_SUCCESS;
    }

    return (status);
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_COMM_Init_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the Host Communication class
*       driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the class
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicate there exists a host Communication
*                           class driver.
*       NU_NOT_PRESENT      Indicate there is no instance created for
*                           Communication class driver.
*
*************************************************************************/
STATUS NU_USBH_COMM_Init_GetHandle(VOID  **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBH_COMM_Cb_Pt;
    if (NU_USBH_COMM_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}
