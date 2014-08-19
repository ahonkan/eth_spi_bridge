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
*       nu_usbh_com_imp.c
*
*
* COMPONENT
*
*       Nucleus USB host software. Communication class host driver.
*
* DESCRIPTION
*
*       This file contains core routines for Nucleus USB host stack's
*       communication class driver component.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBH_COM_Find_User               Returns pointer to user driver
*                                           if it exists.
*       NU_USBH_COM_Transfer_Out            Sends data to a Communication
*                                           device.
*       NU_USBH_COM_Transfer_In             Fetch data from a Communication
*                                           device.
*       NU_USBH_COM_Ctrl_IRP_Complete       Control IRP completion
*                                           callback.
*       NU_USBH_COM_Intr_IRP_Complete       Interrupt IRP completion
*                                           callback.
*       NU_USBH_COM_TX_IRP_Complete         Send IRP completion callback.
*       NU_USBH_COM_RX_IRP_Complete         Receive IRP completion
*                                           callback.
*       NU_USBH_COM_Check_Intf_Pair         Check if the data or
*                                           Communication interface yet
*                                           reported or not.
*       NU_USBH_COM_Intr_Poll               Interrupt pipe polling
*                                           function.
*       NU_USBH_Com_Get_String              Get String descriptor from
*                                           Communication device.
*       NU_USBH_COM_Parse_Strings           Get specified string out of
*                                           strings.
*       NU_USBH_COM_Init_Device             Initializes Communication
*                                           device's structure.
*       NU_USBH_COM_Check_Union             This function is called during
*                                           interface initialization to
*                                           check for class specific union
*                                           functional descriptor and find
*                                           the data interface from
*                                           extracted information.
*       NU_USBH_COM_Init_Data_Intf          This function is called during
*                                           interface initialization to
*                                           find the altsetting control
*                                           block and pipes for data
*                                           interface.
*
* DEPENDENCIES
*
*       nu_usb.h                            USB Definitions.
*
**************************************************************************/

/* USB Include Files. */
#include "connectivity/nu_usb.h"

/* Functions. */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Find_User
*
* DESCRIPTION
*     Finds the user with matching subclass from all the registered users
*     to this Communication class driver.
*
* INPUTS
*     pcb_com_drvr    Pointer to the communication driver control block.
*     subclass        Subclass to be matched.
*
* OUTPUTS
*     NU_USB_USER*    Pointer to the matched USER.
*     NU_NULL         No Match exists.
*
**************************************************************************/

NU_USB_USER* NU_USBH_COM_Find_User (
             NU_USBH_COM* pcb_com_drvr,
             UINT8        subclass)
{
    UINT8       i;
    UINT8       subclasscode;
    STATUS      status;
    UNSIGNED    num_users;

    NU_USB_USER *apcb_user_drvr[NU_USB_MAX_USERS];
    NU_USB_USER *rvalue = NU_NULL;

    /* Find the number of users registered with this class driver. */
    num_users = NU_USB_DRVR_Get_Users_Count ((NU_USB_DRVR*) pcb_com_drvr);

    /* Check against maximum possible number of users. */
    if (num_users > NU_USB_MAX_USERS)
    {
        num_users = NU_USB_MAX_USERS;
    }

    /* Get the list of users from parent NU_USB_DRVR control block. */
    num_users = NU_USB_DRVR_Get_Users ((NU_USB_DRVR *) pcb_com_drvr,
                                       apcb_user_drvr,
                                       num_users);

    /* Search for the suitable user from the list. */
    for (i = 0; ((i < num_users)&&(i < NU_USB_MAX_USERS)); i++)
    {
        /* Get the subclass */
        status = NU_USB_USER_Get_Subclass (apcb_user_drvr[i],
                                           &subclasscode);
        if (status != NU_SUCCESS)
            continue;

        /* ...and match it with the subclass, required. */
        if (subclass == subclasscode || subclasscode == 0x00)
        {
            /* Return the user pointer if matched or if user is registered
             * with a don't care subclass.
             */
            rvalue = apcb_user_drvr[i];
            break;
        }
        else
        {
            /* No Match exists. */
            rvalue = NU_NULL;
        }
    }
    return rvalue;
}/* NU_USBH_COM_Find_User */

/*************************************************************************
*
* FUNCTION
*     NU_USBH_COM_Transfer_Out
*
* DESCRIPTION
*     This function is responsible for sending communication data to the
*     attached device.
*
* INPUTS
*     pcb_curr_device    Pointer to communication device control block.
*     pcb_com_xblock     Pointer to control block for communication
*                        transfers.
*
* OUTPUTS
*     NU_SUCCESS                   Indicates successful completion.
*     NU_USBH_COM_XFER_ERR         Indicates command didn't complete
*                                  successfully.
*     NU_USBH_COM_XFER_FAILED      Indicates command failed by
*                                  Communication class driver.
*
**************************************************************************/
STATUS NU_USBH_COM_Transfer_Out (
       NU_USBH_COM_DEVICE*  pcb_curr_device,
       NU_USBH_COM_XBLOCK*  pcb_com_xblock)
{

    STATUS             status;
    UNSIGNED           ret_events;
    UINT8              sub_class;
    BOOLEAN            Delineate;

    NU_USB_IRP         cb_bulk_out_irp;
    NU_USB_IRP         cb_iso_out_irp;
    NU_USBH_COM_XBLOCK cb_xfer_block;

    cb_xfer_block = *(pcb_com_xblock);

    /* Schedule out transaction. */
    NU_Obtain_Semaphore (&pcb_curr_device->sm_out_trans,
                         NU_SUSPEND);

    /* If device is of ECM then a zero-length packet based delineation
     * should be exercised to mark the end of frame segment.
     */

    NU_USB_ALT_SETTG_Get_SubClass (pcb_curr_device->pcb_com_alt_settg,
                                   &sub_class);
    Delineate = ((sub_class == UH_ABS_CTRL_MDL)||
                (sub_class == UH_ETH_CTRL_MDL))? NU_TRUE: NU_FALSE;

    /* If to select bulk or ISO channel. */
    if ( pcb_curr_device->pcb_bulk_out_pipe)
    {
        /* Creating Bulk In IRP. */
        NU_USB_IRP_Create (&(cb_bulk_out_irp),
                           cb_xfer_block.data_length,
                           (UINT8 *) cb_xfer_block.p_data_buf,
                           1,
                           Delineate,
                           NU_USBH_COM_TX_IRP_Complete,
                           pcb_curr_device,
                           0);

        /* Submits the Bulk in IRP. */
        if (NU_USB_PIPE_Submit_IRP(pcb_curr_device->pcb_bulk_out_pipe,
                                  (NU_USB_IRP *) &cb_bulk_out_irp)
                                  != NU_SUCCESS)
        {
            /* Submission failure, Error case. */
            status =  NU_USBH_COM_XFER_FAILED;
        }
        else
        {
            /* Wait for the the IRP to be completed. */

            status = NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                                         UHC_OUT_SENT,
                                         NU_AND_CONSUME,
                                         &ret_events,
                                         NU_SUSPEND);
            ret_events = 0x00;
            /* If we stall, we need to clear it before we go on. */
            NU_USB_IRP_Get_Status (&cb_bulk_out_irp,
                                   &status);

            if (status == NU_USB_STALL_ERR)
            {
                NU_USB_PIPE_Unstall(pcb_curr_device->pcb_bulk_out_pipe);
            }
            else if (status != NU_SUCCESS)
            {
                /* Unknown error -- we've got a problem. */
                status =  NU_USBH_COM_XFER_FAILED;
            }
            else
            {
                 /* Get actually transferred bytes */
                 NU_USB_IRP_Get_Actual_Length (
                            &cb_bulk_out_irp,
                            &(pcb_com_xblock->transfer_length));
            }
        }
    }
    else if ( pcb_curr_device->pcb_iso_out_pipe)
    {
        NU_USB_IRP_Create (&(cb_iso_out_irp),
                           cb_xfer_block.data_length,
                           cb_xfer_block.p_data_buf,
                           1,
                           Delineate,
                           NU_USBH_COM_TX_IRP_Complete,
                           pcb_curr_device,
                           0);

        if (NU_USB_PIPE_Submit_IRP(pcb_curr_device->pcb_iso_out_pipe,
                                  (NU_USB_IRP *) &cb_iso_out_irp)
                                  != NU_SUCCESS)
        {
            /* Submission failure, Error case. */
            status =  NU_USBH_COM_XFER_FAILED;
        }
        else
        {
            status = NU_Retrieve_Events(&(pcb_curr_device->trans_events),
                                        UHC_OUT_SENT,
                                        NU_AND_CONSUME,
                                        &ret_events,
                                        NU_SUSPEND);
            ret_events = 0x00;
            /* If we stall, we need to clear it before we go on. */
            NU_USB_IRP_Get_Status (&cb_iso_out_irp,
                                   &status);

            if (status == NU_USB_STALL_ERR)
            {
                NU_USB_PIPE_Unstall(pcb_curr_device->pcb_iso_out_pipe);
            }
            else if (status != NU_SUCCESS)
            {
                /* Unknown error -- we've got a problem. */
                status =  NU_USBH_COM_XFER_FAILED;
            }
            else
            {
                 NU_USB_IRP_Get_Actual_Length (
                 &cb_iso_out_irp,
                 &(pcb_com_xblock->transfer_length));

            }
        }
    }
    /* Invalid transfer. */
    else
    {
        status = NU_USBH_COM_XFER_FAILED;
    }
    NU_Release_Semaphore (&(pcb_curr_device->sm_out_trans));
    return status;
} /* NU_USBH_COM_Transfer_Out */

/*************************************************************************
*
* FUNCTION
*     NU_USBH_COM_Transfer_In
*
* DESCRIPTION
*     This function is responsible for receiving Communication data from
*     the attached device.
*
* INPUTS
*     pcb_curr_device    Pointer to communication device control block.
*     pcb_com_xblock     Pointer to control block for comm transfers.
*
* OUTPUTS
*     NU_SUCCESS                   Indicates successful completion.
*     NU_USBH_COM_XFER_ERR         Indicates command didn't complete
*                                  successfully.
*     NU_USBH_COM_XFER_FAILED      Indicates command failed by
*                                  Communication class driver.
*
**************************************************************************/
STATUS NU_USBH_COM_Transfer_In (
       NU_USBH_COM_DEVICE* pcb_curr_device,
       NU_USBH_COM_XBLOCK* pcb_com_xblock)
{

    STATUS             status;
    UNSIGNED           ret_events;
    UINT8              sub_class;
    BOOLEAN            Delineate;

    NU_USB_IRP         cb_bulk_in_irp;
    NU_USB_IRP         cb_iso_in_irp;
    NU_USBH_COM_XBLOCK cb_xfer_block;

    cb_xfer_block = *(pcb_com_xblock);

    NU_Obtain_Semaphore (&pcb_curr_device->sm_in_trans,
                          NU_SUSPEND);

    /* If device is of ECM then a zero-length packet based delineation
     * should be exercised to mark the end of frame segment.
     */
    NU_USB_ALT_SETTG_Get_SubClass (pcb_curr_device->pcb_com_alt_settg,
                                   &sub_class);
    Delineate = ((sub_class == UH_ABS_CTRL_MDL)||
                (sub_class == UH_ETH_CTRL_MDL))? NU_TRUE: NU_FALSE;

    /* Deciding the pipe bulk or ISO. */
    if(pcb_curr_device->pcb_bulk_in_pipe)
    {
        /* Creating Bulk In IRP. */

        NU_USB_IRP_Create (&(cb_bulk_in_irp),
                           cb_xfer_block.data_length,
                           (UINT8 *) cb_xfer_block.p_data_buf,
                           1,
                           Delineate,
                           NU_USBH_COM_RX_IRP_Complete,
                           pcb_curr_device,
                           0);

        /* Submits the Bulk in IRP. */
        if (NU_USB_PIPE_Submit_IRP(pcb_curr_device->pcb_bulk_in_pipe,
                                  (NU_USB_IRP *) &cb_bulk_in_irp)
                                  != NU_SUCCESS)
        {
            /* Submission Failure , Error Case. */
            status =  NU_USBH_COM_XFER_FAILED;
        }
        else
        {
            status = NU_Retrieve_Events(&pcb_curr_device->trans_events,
                                        UHC_IN_SENT,
                                        NU_AND_CONSUME,
                                        &ret_events,
                                        NU_SUSPEND);
            ret_events = 0x00;
            /* If we stall, we need to clear it before we go on. */
            NU_USB_IRP_Get_Status (&cb_bulk_in_irp,
                                   &status);

            if (status == NU_USB_STALL_ERR)
            {
                NU_USB_PIPE_Unstall(pcb_curr_device->pcb_bulk_in_pipe);
            }
            else if (status != NU_SUCCESS)
            {
                /* Unknown error -- we've got a problem. */
                ;/*status =  NU_USBH_COM_XFER_FAILED;*/
            }
            else
            {
                 NU_USB_IRP_Get_Actual_Length (&cb_bulk_in_irp,
                 &(pcb_com_xblock->transfer_length));
            }
        }
    }
    else if(pcb_curr_device->pcb_iso_in_pipe)
    {
        NU_USB_IRP_Create (&(cb_iso_in_irp),
                           cb_xfer_block.data_length,
                           (UINT8 *) cb_xfer_block.p_data_buf,
                           1,
                           Delineate,
                           NU_USBH_COM_RX_IRP_Complete,
                           pcb_curr_device,
                           0);

        /* Submits the Bulk in IRP. */
        if (NU_USB_PIPE_Submit_IRP(pcb_curr_device->pcb_iso_in_pipe,
                                  (NU_USB_IRP *) &cb_iso_in_irp)
                                  != NU_SUCCESS)
        {

            status =  NU_USBH_COM_XFER_FAILED;
        }
        else
        {
            status = NU_Retrieve_Events(&(pcb_curr_device->trans_events),
                                        UHC_IN_SENT,
                                        NU_AND_CONSUME,
                                        &ret_events,
                                        NU_SUSPEND);
            ret_events = 0x00;
            /* If we stall, we need to clear it before we go on. */
            NU_USB_IRP_Get_Status (&cb_iso_in_irp,
                                   &status);

            if (status == NU_USB_STALL_ERR)
            {
                NU_USB_PIPE_Unstall (pcb_curr_device->pcb_iso_in_pipe);
            }
            else if (status != NU_SUCCESS)
            {
                /* Unknown error -- we've got a problem. */
                status =  NU_USBH_COM_XFER_FAILED;
            }
            else
            {
                 NU_USB_IRP_Get_Actual_Length (
                 &cb_iso_in_irp,
                 &(pcb_com_xblock->transfer_length));

                 NU_USB_PIPE_Flush(pcb_curr_device->pcb_iso_in_pipe);
            }
        }
    }

    /* Invalid transfer. */
    else
    {
        status = NU_USBH_COM_XFER_FAILED;
    }
    NU_Release_Semaphore (&(pcb_curr_device->sm_in_trans));
    return status;
} /* NU_USBH_COM_Transfer_In */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Intr_IRP_Complete
*
* DESCRIPTION
*     This function is called when an IRP on interrupt in pipe of a Communication
*     device gets completed. It takes the context information from the IRP
*     and wakes up the task waiting on submission of IRP.
*
* INPUTS
*     pcb_pipe       Pointer to the Pipe control block.
*     pcb_irp        Pointer to IRP control block.
*
* OUTPUTS
*     None
*
**************************************************************************/
VOID NU_USBH_COM_Intr_IRP_Complete (
     NU_USB_PIPE*  pcb_pipe,
     NU_USB_IRP*   pcb_irp)
{
    VOID *context;
    NU_USBH_COM_XBLOCK cb_xfer_block;
    NU_USBH_COM_DEVICE* pcb_curr_device;
    STATUS status;

    /* Gets the cookie from IRP. */
    NU_USB_IRP_Get_Context (pcb_irp,
                            &context);

    pcb_curr_device = ((NU_USBH_COM_DEVICE* )context);

    cb_xfer_block = pcb_curr_device->cb_xfer_block;

    NU_USB_IRP_Get_Status (pcb_irp,
                           &status);

    if(status == NU_SUCCESS)
    {
        NU_USB_IRP_Get_Actual_Length(
                           pcb_irp,
                           &(cb_xfer_block.transfer_length));

        /* Notify user driver of valid interrupt response */
       (((NU_USBH_COM_USER_DISPATCH *) (((NU_USB *)
        pcb_curr_device->pcb_user_drvr)->usb_dispatch))->
        Intr_Response_Handler (pcb_curr_device,
                               &cb_xfer_block));

        NU_Set_Events (&(pcb_curr_device->trans_events),
                        UHC_INTR_SENT,
                        NU_OR);
    }

} /* NU_USBH_COM_Intr_IRP_Complete */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_TX_IRP_Complete
*
* DESCRIPTION
*     This function is called when an IRP on ISO or bulk out pipe of a
*     Communication device gets completed. It takes the context information
*     from the IRP and wakes up the task waiting on submission of IRP.
*
* INPUTS
*     pcb_pipe       Pointer to the Pipe control block.
*     pcb_irp        Pointer to IRP control block.
*
* OUTPUTS
*     None
*
**************************************************************************/
VOID NU_USBH_COM_TX_IRP_Complete (
     NU_USB_PIPE* pcb_pipe,
     NU_USB_IRP*  pcb_irp)
{
    VOID *context;

    NU_USBH_COM_DEVICE* pcb_curr_device;

    /* Gets the cookie from IRP. */
    NU_USB_IRP_Get_Context (pcb_irp,
                            &context);
    pcb_curr_device = ((NU_USBH_COM_DEVICE* )context);
    NU_Set_Events (&(pcb_curr_device->trans_events),
                   UHC_OUT_SENT,
                   NU_OR);
} /* NU_USBH_COM_TX_IRP_Complete*/

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Ctrl_IRP_Complete
*
* DESCRIPTION
*     This function is called when an IRP on control pipe of a Communication
*     device gets completed. It takes the context information from the IRP
*     and wakes up the task waiting on submission of IRP.
*
* INPUTS
*     pcb_pipe       Pointer to the Pipe control block.
*     pcb_irp        Pointer to IRP control block.
*
* OUTPUTS
*     None
*
**************************************************************************/
VOID NU_USBH_COM_Ctrl_IRP_Complete (
     NU_USB_PIPE* pcb_pipe,
     NU_USB_IRP*  pcb_irp)
{
    VOID *context;

    NU_USBH_COM_DEVICE* pcb_curr_device;

    /* Gets the cookie from IRP. */
    NU_USB_IRP_Get_Context (pcb_irp,
                            &context);
    pcb_curr_device = ((NU_USBH_COM_DEVICE* )context);

    NU_Set_Events (&(pcb_curr_device->trans_events),
                   UHC_CTRL_SENT,
                   NU_OR);
} /* NU_USBH_COM_Ctrl_IRP_Complete */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_RX_IRP_Complete
*
* DESCRIPTION
*     This function is called when an IRP on ISO or bulk in pipe of a
*     Communication device gets completed. It takes the context information
*      from the IRP and wakes up the task waiting on submission of IRP.
*
* INPUTS
*     pcb_pipe       Pointer to the Pipe control block.
*     pcb_irp        Pointer to IRP control block.
*
* OUTPUTS
*     None
*
**************************************************************************/
VOID NU_USBH_COM_RX_IRP_Complete (
     NU_USB_PIPE* pcb_pipe,
     NU_USB_IRP*  pcb_irp)
{
    VOID *context;

    NU_USBH_COM_DEVICE* pcb_curr_device;

    /* Gets the cookie from IRP. */
    NU_USB_IRP_Get_Context (pcb_irp,
                            &context);
    pcb_curr_device = ((NU_USBH_COM_DEVICE* )context);

    NU_Set_Events (&(pcb_curr_device->trans_events),
                   UHC_IN_SENT,
                   NU_OR);
} /* NU_USBH_COM_RX_IRP_Complete */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Check_Intf_Pair
*
* DESCRIPTION
*     This function is called to check if both interfaces of a Communication
*     device have been reported or not. If reported then it returns the
*     pointer to to device otherwise a NULL pointer.
*
* INPUTS
*     pcb_drvr       Pointer to the driver control block.
*     pcb_device     Pointer to USB device control block.
*
* OUTPUTS
*     NU_USBH_COM_DEVICE*   Pointer to Communication device.
*
**************************************************************************/
NU_USBH_COM_DEVICE* NU_USBH_COM_Check_Intf_Pair(
                    NU_USBH_COM*   pcb_drvr,
                    NU_USB_DEVICE* pcb_device,
                    NU_USB_INTF*   pcb_intf )
{

     NU_USBH_COM_DEVICE* pcb_curr_device = pcb_drvr->pcb_first_device;

     while(pcb_curr_device != NU_NULL)
     {
         if(pcb_curr_device->pcb_device == pcb_device )
         {
             if( (pcb_curr_device->master_intf == pcb_intf->intf_num) ||
                 (pcb_curr_device->slave_intf  == pcb_intf->intf_num))
             {
                 break;
             }
         }
         if((NU_USBH_COM_DEVICE*)(pcb_curr_device->node.cs_next) ==
             pcb_drvr->pcb_first_device)
         {
            pcb_curr_device = NU_NULL;
            break;
         }
         else
         {
             pcb_curr_device = (NU_USBH_COM_DEVICE*)
                                pcb_curr_device->node.cs_next;
         }
     }
     return pcb_curr_device;
} /* NU_USBH_COM_Check_Intf_Pair */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Intr_Poll
*
* DESCRIPTION
*     This function is the entry point of interrupt endpoint polling task.
*     if a response is present on the interrupt endpoint it notify
*     its user driver.
*
* INPUTS
*     pcb_com_drvr   Pointer to the Communication driver control block.
*     pcb_com_device Pointer to Communication device.
*
* OUTPUTS
*     None
*
**************************************************************************/

VOID NU_USBH_COM_Intr_Poll(
     UINT32 pcb_drvr,
     VOID*  pcb_com_device)

{
    UINT32             ret_events;

    NU_USB_IRP          cb_intr_in_irp;
    NU_USBH_COM_DEVICE* pcb_curr_device =
                       (NU_USBH_COM_DEVICE*)pcb_com_device;
    NU_USBH_COM_XBLOCK  cb_xfer_block   = pcb_curr_device->cb_xfer_block;

    NU_USB_IRP_Create (&(cb_intr_in_irp),
                       cb_xfer_block.data_length,
                       (UINT8 *) cb_xfer_block.p_data_buf,
                       1,
                       0,
                       NU_USBH_COM_Intr_IRP_Complete,
                       pcb_curr_device,
                       0);

    while(1)
    {
        /* Since it is an independent polling so no need to sync. */
        /* Submits the Interrupt in IRP. */

        NU_USB_PIPE_Submit_IRP(pcb_curr_device->pcb_intr_in_pipe,
                               (NU_USB_IRP *) &cb_intr_in_irp);

        NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                            UHC_INTR_SENT,
                            NU_AND_CONSUME,
                            &ret_events,
                            NU_SUSPEND);

    }
}/* NU_USBH_COM_Intr_Poll */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Get_String
*
* DESCRIPTION
*     This routine fetches the requested string descriptor into the
*     specified buffer
*
* INPUTS
*     pcb_curr_device       Pointer to the target device.
*     buffer_ptr            Pointer to buffer to hold string descriptor.
*     size                  Size for above buffer
*     string_num            Sting descriptor index number
*
* OUTPUTS
*     NU_SUCCESS                   Indicates successful completion.
*     NU_USBH_COM_XFER_ERR         Indicates command didn't complete
*                                  successfully.
*     NU_USBH_COM_XFER_FAILED      Indicates command failed by
*                                  Communication class driver.
*     NU_USBH_COM_NOT_SUPPORTED    Indicates that the feature is not
*                                  supported by the Communication
*
**************************************************************************/

STATUS NU_USBH_COM_Get_String(
       NU_USBH_COM_DEVICE* pcb_curr_device,
       UINT8*              buffer_ptr,
       UINT8               size,
       UINT8               string_num)
{
    UINT8* temp_buffer;
    STATUS status;
    UINT8  intf_num;
    UINT32 ret_events;

    NU_USBH_CTRL_IRP *cb_ctrl_irp;

    /* Lock device's functionality for control pipe. */
    status = NU_Obtain_Semaphore (&pcb_curr_device->sm_ctrl_trans,
                                  NU_SUSPEND);
    if(status != NU_SUCCESS)
    {
        return status;
    }

    /* Allocate uncached memory buffer for data IO. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 size,
                                 (VOID **) &temp_buffer);
    if(status == NU_SUCCESS)
    {
        /* Clear temp buffer contents. */
        memset(temp_buffer, 0, size);

        /* Getting the interface number. */
        NU_USB_INTF_Get_Intf_Num (pcb_curr_device->pcb_com_intf,
                                  &intf_num);

        /* Get and clear contents of current device control IRP block. */
        cb_ctrl_irp = pcb_curr_device->ctrl_irp;
        memset(cb_ctrl_irp, 0, sizeof(NU_USBH_CTRL_IRP));

        /* Forms the control request, as SETUP packet of 8 bytes. */
        NU_USBH_CTRL_IRP_Create (cb_ctrl_irp,
                                 temp_buffer,
                                 NU_USBH_COM_Ctrl_IRP_Complete,
                                 pcb_curr_device,
                                 0x80,
                                 06,
                                 HOST_2_LE16((0x0300 | string_num)),
                                 HOST_2_LE16 (0x0409),
                                 HOST_2_LE16 (size));

        /* Submits the IRP. */
        NU_USB_PIPE_Submit_IRP (pcb_curr_device->pcb_control_pipe,
                                (NU_USB_IRP *)cb_ctrl_irp);

        /* Wait for the the IRP to be completed. */
        status = NU_Retrieve_Events (&(pcb_curr_device->trans_events),
                                     UHC_CTRL_SENT,
                                     NU_AND_CONSUME,
                                     &ret_events,
                                     NU_SUSPEND);

        /* ...and returns status. */
        NU_USB_IRP_Get_Status ((NU_USB_IRP *)cb_ctrl_irp,
                               &status);

        /* Copy contents back to input buffer. */
        memcpy(buffer_ptr, temp_buffer, size);

        /* Deallocate temporary buffer now. */
        USB_Deallocate_Memory(temp_buffer);
    }

    /* Unlock class driver functionality */
    NU_Release_Semaphore (&pcb_curr_device->sm_ctrl_trans);

    return(status);
}/* NU_USBH_COM_Get_String */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Parse_Strings
*
* DESCRIPTION
*     All class specific descriptors are returned in a single buffer.
*     For Communication devices different functional descriptors exist.
*     This function returns the pointer of requested functional descriptor.
* INPUTS
*     ptr       Pointer to the fetched strings.
*     size_in   Its size in bytes.
*     index     Required index.
*
* OUTPUTS
*     buffer_ptr  pointer to the start of requested string.
*
**************************************************************************/

UINT8* NU_USBH_COM_Parse_Strings(
       UINT8* ptr,
       UINT32 size_in,
       UINT8  index)
{
    UINT8* buffer_ptr = ptr;
    UINT32 size = size_in;
    while(size)
    {
        if(*(buffer_ptr + 2) == index)
        {
            break;
        }
        else{
            size -= *(buffer_ptr);
            buffer_ptr += *(buffer_ptr);
        }
    }
    if(size == 0x00)
    {
        buffer_ptr = NU_NULL;
    }
    else
    {
        buffer_ptr += 0x03;
    }
    return(buffer_ptr);
}/* NU_USBH_COM_Parse_Strings */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Init_Device
*
* DESCRIPTION
*     This function is called during interface initialization to setup
*     device structure
*
* INPUTS
*     pcb_com_drvr       Pointer to the Pipe control block.
*     pcb_curr_device    Pointer to IRP control block.
*     pcb_intf           Pointer to control block of interface.
*     sub_class          SubClass code of Communication interface to
*                        identify the device model.
* OUTPUTS
*     None
*
**************************************************************************/

STATUS NU_USBH_COM_Init_Device(
       NU_USBH_COM*        pcb_com_drvr,
       NU_USBH_COM_DEVICE* pcb_curr_device,
       NU_USB_INTF*        pcb_intf,
       UINT8               sub_class)
{
    STATUS      status;
    UINT8*      class_desc;
    UINT32      temp_length;
    NU_USB_ENDP* pcb_ep;
    UINT16       maxp;
    NU_USB_CFG* pcb_cfg;

    /* Get active configuration. */
    status = NU_USB_INTF_Get_Cfg (pcb_intf,
                                  &pcb_cfg);

    /* Get all class specific descriptors. */

    status = NU_USB_CFG_Get_Class_Desc (pcb_cfg,
                                        &class_desc,
                                        &temp_length);

    if(class_desc == NU_NULL)
    {
        status  = NU_USB_ALT_SETTG_Get_Class_Desc (
                                   pcb_curr_device->pcb_com_alt_settg,
                                   &class_desc,
                                   &temp_length);
    }

    /* Create event group responsible for USB transfers. */
    status = NU_Create_Event_Group (&(pcb_curr_device->trans_events),
                                    "TrnsDone");

    /* Create semaphore to schedule control transfers. */
    status = NU_Create_Semaphore (&(pcb_curr_device->sm_ctrl_trans),
                                  "COMDRVSM",
                                  1,
                                  NU_FIFO);

    /* Create semaphore to schedule IN transfers. */
    status = NU_Create_Semaphore (&(pcb_curr_device->sm_in_trans),
                                  "COMDRVSM",
                                  1,
                                  NU_FIFO);

    /* Create semaphore to schedule OUT transfers. */
    status = NU_Create_Semaphore (&(pcb_curr_device->sm_out_trans),
                                  "COMDRVSM",
                                  1,
                                  NU_FIFO);

    /* Perform model specific operations. */
    /* For each model fetches its required functional descriptors. Then on
     * the basis of retrieved data initializes its information structure.
     */
    switch(sub_class)
    {
        case UH_DTL_CTRL_MDL:
        {
            #ifdef INC_DCM_MDL
            UHC_Check_DCM_Func_Desc(pcb_curr_device,
                                    class_desc,
                                    temp_length);
            #endif
            break;
        }
        case UH_ABS_CTRL_MDL:
        {
            #ifdef INC_ACM_MDL
            NU_USBH_COM_Check_ACM_Func_Desc(pcb_curr_device,
                                    class_desc,
                                    temp_length);
            #endif
            break;
        }
        case UH_TEL_CTRL_MDL:
        {
            #ifdef INC_TCM_MDL
            UHC_Check_TCM_Func_Desc(pcb_curr_device,
                                    class_desc,
                                    temp_length);
            #endif
            break;
        }

        case UH_MUL_CTRL_MDL:
        {
            #ifdef INC_MCM_MDL
            UHC_Check_TCM_Func_Desc(pcb_curr_device,
                                    class_desc,
                                    temp_length);
            #endif
            break;
        }
        case UH_CAP_CTRL_MDL:
        {
            #ifdef INC_CCM_MDL
            UHC_Check_TCM_Func_Desc(pcb_curr_device,
                                    class_desc,
                                    temp_length);
            #endif
            break;
        }
        case UH_ETH_CTRL_MDL:
        {
            #ifdef INC_ECM_MDL
            NU_USBH_COM_Check_ECM_Func_Desc(pcb_curr_device,
                                    class_desc,
                                    temp_length);
            #endif
            break;
        }
        case UH_ATM_CTRL_MDL:
        {
            #ifdef INC_ATM_MDL
            UHC_Check_TCM_Func_Desc(pcb_curr_device,
                                    class_desc,
                                    temp_length);
            #endif
            break;
        }
        default:{break;}
    }

    if(pcb_curr_device->pcb_intr_in_pipe != NU_NULL)
    {

        pcb_ep = pcb_curr_device->pcb_intr_in_pipe->endpoint;
        NU_USB_ENDP_Get_Max_Packet_Size (pcb_ep,
                                         &maxp);

        pcb_curr_device->cb_xfer_block.data_length = maxp;

        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     maxp,
                                     (VOID **)&(pcb_curr_device->cb_xfer_block.p_data_buf));

        if(status == NU_SUCCESS)
        {
            /* If device contains an interrupt polling endpoint then create a
             * polling task for it.
             */
            status = USB_Allocate_Object(sizeof (NU_TASK),
                                         (VOID**)&pcb_curr_device->poll_task);
            if(pcb_curr_device->poll_task != NU_NULL)
            {
                memset (pcb_curr_device->poll_task,
                        0,
                        sizeof (NU_TASK));
            }
        }

        if(status == NU_SUCCESS)
        {
            status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                         UH_COM_POLL_STACK_SIZE,
                                         (VOID **)&(pcb_curr_device->poll_stack));
        }

        if(status == NU_SUCCESS)
        {
            status = NU_Create_Task(pcb_curr_device->poll_task,
                                    "COMDEVTK",
                                    NU_USBH_COM_Intr_Poll,
                                    0x00,
                                    pcb_curr_device,
                                    pcb_curr_device->poll_stack,
                                    UH_COM_POLL_STACK_SIZE,
                                    UH_COM_POLL_TASK_PRIORITY,
                                    0,
                                    NU_PREEMPT,
                                    NU_START);
        }
    }

    return status;
}/* NU_USBH_COM_Init_Device */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Check_Union
*
* DESCRIPTION
*     This function is called during interface initialization to check for
*     class specific union functional descriptor and find the data
*     interface from extracted information.
*
* INPUTS
*     pcb_device         Pointer to control block of USB device.
*     pcb_intf           Pointer to control block of interface.
*     pcb_curr_device    Pointer to control block of Communication device.
*
* OUTPUTS
*     None
*
**************************************************************************/

STATUS NU_USBH_COM_Check_Union(
       NU_USB_DEVICE*      pcb_device,
       NU_USB_INTF*        pcb_intf,
       NU_USBH_COM_DEVICE* pcb_curr_device)
{

    STATUS      status;
    UINT8*      class_desc;
    UINT32      temp_length;
    UINT8*      p_union;
    UINT8       union_length = 0;

    NU_USB_CFG*         pcb_cfg;
    NU_USB_INTF*        pcb_data_intf;
    NU_USB_ALT_SETTG*   pcb_alt_set;
    UINT8       data_inf_num = 0x00;


    status = NU_USB_INTF_Get_Cfg (pcb_intf,
                                  &pcb_cfg);

    /* Get all class specific descriptors. */

    status  = NU_USB_CFG_Get_Class_Desc (pcb_cfg,
                                        &class_desc,
                                        &temp_length);

    if(class_desc == NU_NULL)
    {
        NU_USB_INTF_Get_Active_Alt_Setting (pcb_intf,
                                            &pcb_alt_set);

        status  = NU_USB_ALT_SETTG_Get_Class_Desc (pcb_alt_set,
                                                   &class_desc,
                                                   &temp_length);
    }

    p_union = NU_USBH_COM_Parse_Strings(class_desc,
                                        temp_length,
                                        UH_UNION_FD);

    if(p_union == NU_NULL)
    {
         status = NU_NOT_PRESENT;
    }
    else
    {
        union_length |= *(p_union-3);
        union_length -= 0x03;
        status = NU_NOT_PRESENT;

        while(union_length)
        {

            if(pcb_intf->intf_num == *p_union)
            {
                if(union_length & 0x01)
                {
                    pcb_curr_device->master_intf = *(p_union-1);
                    pcb_curr_device->slave_intf  = *(p_union);
                    data_inf_num = pcb_curr_device->master_intf;
                }
                else
                {
                    pcb_curr_device->master_intf = *p_union;
                    pcb_curr_device->slave_intf  = *(p_union+1);
                    data_inf_num = pcb_curr_device->slave_intf;
                }
                status = NU_SUCCESS;
                break;
            }
            else
            {
                union_length--;
                p_union++;
            }

        }
        if(status == NU_SUCCESS)
        {
            status = NU_USB_CFG_Get_Intf(pcb_cfg,
                                         data_inf_num,
                                         &pcb_data_intf);
        }
        if(status == NU_SUCCESS)
        {
            status = NU_USBH_COM_Init_Data_intf(pcb_device,
                                                pcb_data_intf,
                                                pcb_curr_device);
        }
    }
    return status;
}/* NU_USBH_COM_Check_Union */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_Init_Data_intf
*
* DESCRIPTION
*     This function is called during interface initialization to find the
*     altsetting control block and pipes for data interface.
*
* INPUTS
*     pcb_device         Pointer to control block of USB device.
*     pcb_intf           Pointer to control block of data interface.
*     pcb_curr_device    Pointer to control block of Communication device.
*
* OUTPUTS
*     NU_SUCCESS         Successful completion.
*
**************************************************************************/
STATUS NU_USBH_COM_Init_Data_intf (
       NU_USB_DEVICE* pcb_device,
       NU_USB_INTF*   pcb_intf,
       NU_USBH_COM_DEVICE* pcb_curr_device)
{
    STATUS status          = NU_NOT_PRESENT;

    UINT8  alt_setting_num = 0;
    UINT8  ep_count  = 0;

    NU_USB_ALT_SETTG   *pcb_alt_setting;

    /* Search for suitable alternate setting of current interface. */
    while(!ep_count)
    {
        status = NU_USB_INTF_Find_Alt_Setting(pcb_intf,
                                              USB_MATCH_CLASS,
                                              alt_setting_num,
                                              UH_COM_DATA_CLASS_CODE,
                                              0,
                                              0,
                                              &pcb_alt_setting);
        if (status == NU_SUCCESS)
        {
            NU_USB_ALT_SETTG_Get_Num_Endps (pcb_alt_setting,
                                            &ep_count);
            alt_setting_num++;
        }
        else
        {
            break;
        }
    }

    do
    {
        if (status != NU_SUCCESS)
        {
            break;
        }

        status = NU_USB_ALT_SETTG_Set_Active (pcb_alt_setting);
        /* Finding the Bulk or ISO pipes required for data I/O. */

        status = NU_USB_ALT_SETTG_Find_Pipe (
                                      pcb_alt_setting,
                                      USB_MATCH_EP_TYPE |
                                      USB_MATCH_EP_DIRECTION,
                                      0,
                                      0,
                                      2,
                                      &pcb_curr_device->pcb_bulk_out_pipe);

        if ((status != NU_SUCCESS)&&(status != NU_NOT_PRESENT))
        {
            break;
        }
        status = NU_USB_ALT_SETTG_Find_Pipe (
                                           pcb_alt_setting,
                                           USB_MATCH_EP_TYPE |
                                           USB_MATCH_EP_DIRECTION,
                                           0,
                                           0x80,
                                           2,
                                       &pcb_curr_device->pcb_bulk_in_pipe);

        if ((status != NU_SUCCESS)&&(status != NU_NOT_PRESENT))
        {
            break;
        }

        if(status == NU_NOT_PRESENT)
        {
            status = NU_USB_ALT_SETTG_Find_Pipe (
                                               pcb_alt_setting,
                                               USB_MATCH_EP_TYPE |
                                               USB_MATCH_EP_DIRECTION,
                                               0,
                                               0,
                                               1,
                                       &pcb_curr_device->pcb_iso_out_pipe);
            if ((status != NU_SUCCESS)&&(status != NU_NOT_PRESENT))
            {
                break;
            }
            status = NU_USB_ALT_SETTG_Find_Pipe (
                                                  pcb_alt_setting,
                                                   USB_MATCH_EP_TYPE |
                                                   USB_MATCH_EP_DIRECTION,
                                                   0,
                                                   0x80,
                                                   1,
                                        &pcb_curr_device->pcb_iso_in_pipe);
        }
    }while(0);

    if(status == NU_SUCCESS)
    {
        pcb_curr_device->pcb_data_intf   = pcb_intf;
        pcb_curr_device->pcb_data_alt_settg = pcb_alt_setting;
    }

    return status;
}
