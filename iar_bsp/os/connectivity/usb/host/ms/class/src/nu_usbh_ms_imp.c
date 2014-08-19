/**************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*
*       nu_usbh_ms_imp.c
*
*
* COMPONENT
*
*       Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains core routines for Nucleus USB Host Stack's
*       Mass Storage class driver component.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       UHMS_Find_User                       Returns pointer to a suitable
*                                           NU_USB_USER which matches the
*                                           given subclass.
*       UHMS_Bulk_Reset_Recovery             Performs reset recovery.
*                                           (BOT only)
*       UHMS_Bulk_Get_Max_LUN                Issues Get Max LUN command.
*                                           (BOT only)
*       UHMS_CBI_Send_ADSC                   Issues ADSC Command.
*                                           (CB/CBI Only)
*       UHMS_CBI_Command_Block_Reset         Issues Command Block Reset
*                                           Command. (CB/CBI Only)
*       UHMS_Validate_CSW                    Validates a receives CSW for
*                                           correctness, as defined by Mass
*                                           Storage Bulk Only Transport
*                                           Spec.
*       UHMS_Bulk_Transport                  Transfers a Mass Storage
*                                           command according to USB Mass
*                                           Storage Class Bulk Only.
*                                           Transport Spec.
*       UHMS_CBI_Transport                   Transfers a Mass Storage
*                                           Command across USB. (CBI Only)
*       UHMS_CB_Transport                    Transfers a Mass Storage
*                                           Command across USB. (CB Only)
*       UHMS_IRP_Complete                    IRP completion callback.
*       UHMS_Intr_Callback                   Interrupt IRP completion
*                                           callback. (CBI Only)
*       UHMS_Validate_Drive              Checks if a mass storage drive
*                                           is valid and attached
*       USB_MSC_Event_Reporters             Entry function for MSC driver's
*                                           connection/disconnection events,
*                                           invoked when a device connection
*                                           or disconnection is reported.
*
* DEPENDENCIES
*
*       nu_usb.h                            USB Definitions.
*
**************************************************************************/

/* =====================  USB Include Files  =========================== */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"

/* =====================  Functions  =================================== */

/**************************************************************************
*
* FUNCTION
*
*       UHMS_Find_User
*
* DESCRIPTION
*
*       Finds the User with matching subclass from all the registered Users
*       to this mass storage class driver.
*
* INPUTS
*       cb                                  Pointer to the Mass Storage
*                                           Driver control block.
*       subclass                            Subclass to be matched.
*
* OUTPUTS
*
*       NU_USB_USER*                        Pointer to the matched USER.
*       NU_NULL                             No Matches.
*
**************************************************************************/
NU_USB_USER *UHMS_Find_User (NU_USBH_MS * cb,
                            UINT8 subclass)
{
    UINT8        i;
    UINT8        subclasscode;
    STATUS       status;
    UNSIGNED     num_users;
    NU_USB_USER *users[NU_USB_MAX_USERS];
    NU_USB_USER *ptr = NU_NULL;

    /* Find the number of users registered with this class driver. */
    num_users = NU_USB_DRVR_Get_Users_Count ((NU_USB_DRVR *) cb);
    
    /* Check against maximum possible number of users. */
    if (num_users > NU_USB_MAX_USERS)
        num_users = NU_USB_MAX_USERS;

    /* Get the list of users from parent NU_USB_DRVR control block. */
    num_users = NU_USB_DRVR_Get_Users ((NU_USB_DRVR *) cb, users,
                                        num_users);

    /* Search for the suitable user from the list. */
    for (i = 0; ((i < num_users)&&(i < NU_USB_MAX_USERS)); i++)
    {
        /* Get the subclass. */
        status = NU_USB_USER_Get_Subclass (users[i], &subclasscode);
        if (status != NU_SUCCESS)
        {
            continue;
        }

        /* ...and match it with the subclass required. */
        if (subclass == (subclass&subclasscode))
        {
            /* Return the user pointer if matched
             * or if user is registered with a don't care
             * subclass.
             */
            ptr = users[i];
            break;
        }

    }

    return (ptr);
}

/**************************************************************************
*
* FUNCTION
*
*       UHMS_Bulk_Reset_Recovery
*
* DESCRIPTION
*
*       Performs Bulk Reset Recovery. This is done by sending a Bulk Only
*       mass storage reset, mass storage class specific command, to the
*       device. Followed by clearing bulk in and bulk out pipes for Stalls.
*
* INPUTS
*
*       cb                                  Pointer to the Mass Storage
*                                           Driver control block.
*       stack                               Pointer to Stack Control Block.
*       currDrive                           Pointer to the DRIVE structure
*                                           for this Interface.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*                                           of the service.
*       NU_INVALID_SUSPEND                  Indicates that this API is
*                                           called from a non-task thread.
*       NU_USB_INVLD_ARG                    Indicates that one or more
*                                           arguments passed to this
*                                           function are invalid.
*
**************************************************************************/
STATUS UHMS_Bulk_Reset_Recovery (NU_USBH_MS       *cb,
                                NU_USB_STACK     *stack,
                                NU_USBH_MS_DRIVE *currDrive)
{
    /* #1- Perform a Bulk-Only Mass Storage Reset. */
    NU_USBH_CTRL_IRP *irp;
    STATUS           status   =   NU_SUCCESS;
    STATUS           irp_status =   NU_SUCCESS;
    UINT8            intf_num   =   NU_NULL;
    UINT8            i          =   NU_NULL;


    /* Remove unused parameter warning. */
    NU_UNUSED_PARAM(cb);
    NU_UNUSED_PARAM(stack);
    for(i = 0; i < 3; i++ )
    {
        status = NU_USB_INTF_Get_Intf_Num (currDrive->intf, &intf_num);
        if ( status == NU_SUCCESS )
        {
            /* Get and clear contents of current device control IRP block. */
            irp = currDrive->ctrl_irp;
            memset(irp, 0, sizeof(NU_USBH_CTRL_IRP));

            /* Populate Control IRP. */
            status = NU_USBH_CTRL_IRP_Create (irp,
                           NU_NULL,                  /* Data.               */
                           UHMS_IRP_Complete,         /* Callback function.  */
                           (VOID *)currDrive,        /* Context.            */
                           0x21,                     /* bmRequestType.      */
                           MS_BULK_RESET_REQUEST,    /* bRequest.           */
                           0,                        /* wValue.             */
                           HOST_2_LE16 (intf_num),   /* wIndex.             */
                           0);                       /* wLength.            */
        }

        if ( status == NU_SUCCESS )
        {
            /* Submit the IRP. */
            status = UHMS_Submit_IRP (currDrive,
                                      currDrive->control_pipe,
                                      (NU_USB_IRP *) irp);
        }

        if (status == NU_SUCCESS)
        {
            /* Wait for the the IRP to complete. */
            status = NU_Obtain_Semaphore (&currDrive->tx_request,
                                          NU_USBH_MS_TX_LOCK_TIMEOUT);
        }

        /* And read status. */
        if (status == NU_SUCCESS)
        {
            NU_USB_IRP_Get_Status ((NU_USB_IRP *) irp, &irp_status);
        }

        if (irp_status == NU_SUCCESS)
        {
            /* #2- Clear Halt Feature on bulk-in pipe. */
            status = NU_USB_PIPE_Unstall (currDrive->bulk_in_pipe);
        }

        if (status == NU_SUCCESS)
        {
            /* #3- Clear Halt Feature on bulk-out pipe. */
            status = NU_USB_PIPE_Unstall (currDrive->bulk_out_pipe);
        }
        if( status == NU_SUCCESS )
        {
            break;
        }
    }
    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       UHMS_Bulk_Get_Max_LUN
*
* DESCRIPTION
*
*       This function gets the Number of Logical Units on a mass storage
*       device by sending a Bulk Only, Mass storage class specific command
*       over control pipe.
*
* INPUTS
*
*       cb                                  Pointer to the Mass Storage
*                                           Driver control block.
*       stack                               Pointer to Stack Control Block.
*       currDrive                           Pointer to the DRIVE structure
*                                           for this interface.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*                                           of the service.
*       NU_USBH_MS_DISCONNECTED             Indicates the device is
*                                           disconnected.
*       NU_USB_INVLD_ARG                    Indicates one or more arguments
*                                           are invalid.
*       NU_UNAVAILABLE                      If an instance of the
*                                           semaphore is not available
*       NU_TIMEOUT                          If timeout on service
*       NU_SEMAPHORE_DELETED                If semaphore deleted during
*                                           suspension
*       NU_SEMAPHORE_RESET                  If semaphore reset during
*                                           suspension
**************************************************************************/
STATUS UHMS_Bulk_Get_Max_LUN (NU_USBH_MS * cb,
                             NU_USB_STACK * stack,
                             NU_USBH_MS_DRIVE * currDrive)
{
    NU_USBH_CTRL_IRP *irp;
    STATUS           irp_status, status;
    UINT8            intf_num;
    UINT8            *ptr_max_lun = NU_NULL;

    /* Allocate max LUN on uncached memory. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 sizeof(UINT8),
                                 (VOID **)&ptr_max_lun);

    /* Remove unused parameter warning. */
    NU_UNUSED_PARAM(cb);
    NU_UNUSED_PARAM(stack);

    if (currDrive->protocol == MS_PR_BULK
        && (status == NU_SUCCESS) )
    {
        /* Clear the buffer. */
        memset (&currDrive->max_lun, 0, sizeof(currDrive->max_lun) );

        status = NU_USB_INTF_Get_Intf_Num (currDrive->intf, &intf_num);
        if ( status == NU_SUCCESS )
        {
            /* Get and clear contents of current device control IRP block. */
            irp = currDrive->ctrl_irp;
            memset(irp, 0, sizeof(NU_USBH_CTRL_IRP));

            /* Form the control request. */
            status = NU_USBH_CTRL_IRP_Create (irp,
                       ptr_max_lun,             /* Data.               */
                       UHMS_IRP_Complete,       /* Callback function.  */
                       (VOID *)currDrive,       /* Context.            */
                       0xA1,                    /* bmRequestType.      */
                       MS_BULK_GET_MAX_LUN,     /* bRequest.           */
                       0,                       /* wValue.             */
                       HOST_2_LE16 (intf_num),  /* wIndex.             */
                       HOST_2_LE16 (1) );       /* wLength.            */
        }

        if ( status == NU_SUCCESS )
        {
            /* Submit the IRP. */
            status = UHMS_Submit_IRP (currDrive,
                                      currDrive->control_pipe,
                                      (NU_USB_IRP *) irp);
        }

        /* Wait for the the IRP to complete. */
        if (status == NU_SUCCESS)
        {
            status = NU_Obtain_Semaphore (&currDrive->tx_request,
                                              NU_USBH_MS_TX_LOCK_TIMEOUT);
        }

        /* Return status. */
        if (status == NU_SUCCESS)
        {
            status = NU_USB_IRP_Get_Status ((NU_USB_IRP *) irp, &irp_status);
            if ( status == NU_SUCCESS )
            {
                if(irp_status == NU_USB_STALL_ERR)
                {
                    /* Stall is allowed for get max lun if the device
                    * supports only one LUN.
                    */
                    currDrive->max_lun = 0;
                }
                else
                {
                    status = irp_status;
                    /* Copy Max LUN value. */
                    currDrive->max_lun = *ptr_max_lun;
                    currDrive->max_lun = currDrive->max_lun & 0x0F;
                }
            }
        }
    }
    else
    {
        /* Protocol other than Bulk Only Transport doesn't support GetMaxLun command. */
        status = NU_SUCCESS;
        currDrive->max_lun = 0;
    }

    /* Deallocate memory. */
    if (ptr_max_lun)
    {
        USB_Deallocate_Memory(ptr_max_lun);
    }

    return (status);

}

/**************************************************************************
*
* FUNCTION
*
*       UHMS_CBI_Send_ADSC
*
* DESCRIPTION
*
*       This function sends ADSC, CBI/CB only, mass storage class specific
*       command to the device. This command is send over control pipe and
*       marks the beginning of a command to be transmitted to a mass
*       storage device.
*
* INPUTS
*
*       cb                                  Pointer to the Mass Storage
*                                           Driver control block.
*       stack                               Pointer to Stack Control Block.
*       currDrive                           Pointer to the DRIVE structure
*                                           for this Interface.
*       cmd_blk                             Command Block Pointer
*                                           containing ADSC Command.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*                                           of the service.
*       NU_USB_CRC_ERR                      Indicates CRC error in transfer
*       NU_USB_BITSTUFF_ERR                 Indicates bit stuffing error.
*       NU_USB_TOGGLE_ERR                   Indicates data toggle mismatch.
*       NU_USB_STALL_ERR                    Indicates endpoint stall.
*       NU_USB_NO_RESPONSE                  Indicates device not responding
*       NU_USB_INVLD_PID                    Indicates check bits on PID
*                                           failed.
*       NU_USB_UNEXPECTED_PID               Indicates unexpected PID.
*       NU_USB_DATA_OVERRUN                 Indicates packet exceeded Max
*                                           packet Size.
*       NU_USB_DATA_UNDERRUN                Indicates packet less than Max
*                                           packet size.
*       NU_USB_BFR_OVERRUN                  Indicates host bus couldn't
*                                           keep up.
*       NU_USB_BFR_UNDERRUN                 Indicates host bus couldn't
*                                           keep up.
*       NU_USB_NOT_ACCESSED                 Indicates IRP not accessed.
*       NU_USB_EP_HALTED                    Indicates Endpoint halted, may
*                                           be due to failure of handshake.
*       NU_USB_IRP_CANCELLED                Indicates TD is terminated, may
*                                           be due to closure of the pipe.
*       NU_USB_UNKNOWN_ERR                  Indicates Unknown Error in IRP
*                                           transmission.
*
**************************************************************************/
STATUS UHMS_CBI_Send_ADSC (NU_USBH_MS * cb,
                          NU_USB_STACK * stack,
                          NU_USBH_MS_DRIVE * currDrive,
                          USBH_MS_CB * cmd_blk)
{
    NU_USBH_CTRL_IRP *irp;
    STATUS irp_status, status;
    UINT8 intf_num;

    /* Remove unused parameter warning. */
    NU_UNUSED_PARAM(cb);
    NU_UNUSED_PARAM(stack);

    status = NU_USB_INTF_Get_Intf_Num (currDrive->intf, &intf_num);
    if ( status == NU_SUCCESS )
    {
        /* Get and clear contents of current device control IRP block. */
        irp = currDrive->ctrl_irp;
        memset(irp, 0, sizeof(NU_USBH_CTRL_IRP));

        /* Populate control IRP. */
        status = NU_USBH_CTRL_IRP_Create (irp,
                             (UINT8 *) cmd_blk->command,            /* Data.               */
                             UHMS_IRP_Complete,                     /* Callback function.  */
                             (VOID *)currDrive,                     /* Context.            */
                             0x21,                                  /* bmRequestType.      */
                             MS_CBI_ADSC,                           /* bRequest.           */
                             NU_NULL,                               /* wValue.             */
                             HOST_2_LE16 (intf_num),                /* wIndex.             */
                             HOST_2_LE16 (cmd_blk->cmd_length) );   /* wLength.            */
    }

    if ( status == NU_SUCCESS )
    {
        /* Submit the IRP. */
        status = UHMS_Submit_IRP (  currDrive,
                                    currDrive->control_pipe,
                                    (NU_USB_IRP *) irp);
    }

    /* Wait for the the IRP to complete. */
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore (&currDrive->tx_request,
                                      NU_USBH_MS_TX_LOCK_TIMEOUT);
    }

    /* Return status. */
    if (status == NU_SUCCESS)
    {
        status = NU_USB_IRP_Get_Status ((NU_USB_IRP *) irp, &irp_status);
        if ( status == NU_SUCCESS )
        {
            status = irp_status;
        }
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       UHMS_CBI_Command_Block_Reset
*
* DESCRIPTION
*
*       This function Sends a Command Block Reset, CBI only Mass Storage
*       class specific command to the device.
*
* INPUTS
*
*       cb                                  Pointer to the Mass Storage
*                                           Driver control block.
*       stack                               Pointer to Stack Control Block.
*       currDrive                           Pointer to the DRIVE structure
*                                           for this Interface.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*                                           of the service.
*       NU_INVALID_SUSPEND                  Indicates that this API is
*                                           called from a non-task thread.
*       NU_USB_INVLD_ARG                    Indicates that one or more
*                                           arguments passed to this
*                                           function are invalid.
**************************************************************************/
STATUS UHMS_CBI_Command_Block_Reset (NU_USBH_MS * cb,
                                    NU_USB_STACK * stack,
                                    NU_USBH_MS_DRIVE * currDrive)
{
    UINT8 cmd_blk[NU_USBH_MS_CB_SIZE];

    NU_USBH_CTRL_IRP *irp;
    STATUS irp_status, status;
    UINT8 intf_num;

    /* Remove unused parameter warning. */
    NU_UNUSED_PARAM(cb);
    NU_UNUSED_PARAM(stack);

    status = NU_USB_INTF_Get_Intf_Num (currDrive->intf, &intf_num);
    if ( status == NU_SUCCESS )
    {
        /* Fill in command block. */
        memset (cmd_blk, 0xFF, NU_USBH_MS_CB_SIZE);
        cmd_blk[0] = 0x1D;
        cmd_blk[1] = 0x04;

        /* Get and clear contents of current device control IRP block. */
        irp = currDrive->ctrl_irp;
        memset(irp, 0, sizeof(NU_USBH_CTRL_IRP));

        /* Fill setup packet for ADSC. */
        status = NU_USBH_CTRL_IRP_Create (irp,
                       (UINT8 *) cmd_blk,                  /* Data.                     */
                       UHMS_IRP_Complete,                  /* Callback function.        */
                       (VOID *)currDrive,                  /* Context.                  */
                       0x21,                               /* Value of bmRequestType.   */
                       MS_CBI_ADSC,                        /* bRequest.                 */
                       0,                                  /* wValue .                  */
                       HOST_2_LE16 (intf_num),             /* wIndex .                  */
                       HOST_2_LE16 (sizeof (cmd_blk)) );   /* wLength.                  */
    }

    if ( status == NU_SUCCESS )
    {
        /* Submit the IRP. */
        status = UHMS_Submit_IRP (  currDrive,
                                    currDrive->control_pipe,
                                    (NU_USB_IRP *) irp);
    }

    /* Wait for the the IRP to complete. */
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore (&currDrive->tx_request,
                                          NU_USBH_MS_TX_LOCK_TIMEOUT);
    }

    /* Get and return the status. */
    if (status == NU_SUCCESS)
    {
        status = NU_USB_IRP_Get_Status ((NU_USB_IRP *) irp, &irp_status);
        if ( status == NU_SUCCESS )
        {
            status = irp_status;
        }
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       UHMS_Validate_CSW
*
* DESCRIPTION
*
*       This function verifies the validity of a Command Status wrapper
*       received from a device. The necessary and sufficient conditions for
*       a CSW to be valid are described in USB Mass Storage(Bulk only)
*       Class Specification.
*
* INPUTS
*
*       csw                                 CSW to be checked for validity.
*       cb                                  Pointer to the command block
*                                           this csw is associated with.
*
* OUTPUTS
*
*       MS_CSW_BAD                          Indicates CSW is not valid.
*       MS_CSW_OK                           Indicates CSW is valid.
*
**************************************************************************/
UINT8 UHMS_Validate_CSW (NU_USBH_MS_DRIVE * currDrive, MS_BOT_CSW csw, USBH_MS_CB * cmd_blk)
{
    UINT8 csw_state = MS_CSW_OK;

    /* Check signature. */
    if (LE32_2_HOST (csw.signature) != MS_CSW_SIGNATURE)
    {
        csw_state = MS_CSW_BAD;
    }

    /* Check CSW tag to be same as CBW tag. */
    else
    {
        if (LE32_2_HOST (csw.tag) != currDrive->ms_cbw_tag)
        {
            csw_state = MS_CSW_BAD;
        }

        /* Check against residue length. */
        else
        {
            if ((csw.status == 0x00) || (csw.status == 0x01))
            {
                if (LE32_2_HOST (csw.data_residue) > cmd_blk->buf_length)
                {
                    csw_state = MS_CSW_BAD;
                }
            }
        }
    }
    return (csw_state);
}

/**************************************************************************
*
* FUNCTION
*
*       UHMS_Bulk_Transport
*
* DESCRIPTION
*
*       This function Transfers a Mass Storage Media specific command on to
*       a mass storage device, by using Bulk Only protocol specified in USB
*       Mass Storage class specification. This is done by sending a Command
*       Block Wrapper to the specified LUN on device followed by data
*       transfer and finally receiving a Command Status Wrapper from the
*       device.
*
* INPUTS
*
*       cb                                  Pointer to the Mass Storage
*                                           Driver control block.
*       stack                               Pointer to Stack Control Block.
*       currDrive                           Pointer to the DRIVE structure
*                                           for this Interface.
*       lun                                 LUN to which command is to be
*                                           transported.
*       cmd_blk                             Pointer to command block to be
*                                           transported.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USBH_MS_TRANSPORT_ERROR          Indicates Command doesn't
*                                           completed successfully.
*       NU_USBH_MS_TRANSPORT_FAILED         Indicates command failed by the
*                                           media.
*
**************************************************************************/
STATUS UHMS_Bulk_Transport (NU_USBH_MS * cb,
                           NU_USB_STACK * stack,
                           NU_USBH_MS_DRIVE * currDrive,
                           UINT8 lun,
                           USBH_MS_CB * cmd_blk)

{
    UINT8 ii;
    STATUS irp_status, status;
    MS_BOT_CBW *cbw;
    MS_BOT_CSW *csw;
    NU_USB_IRP *bulk_in_irp;
    NU_USB_IRP *bulk_out_irp;

    status          = NU_SUCCESS;
    bulk_in_irp     = &(currDrive->bulk_in_irp);
    bulk_out_irp    = &(currDrive->bulk_out_irp);
    cbw             = NU_NULL;
    csw             = NU_NULL;

    do
    {
        cbw = (MS_BOT_CBW *)currDrive->cbw_ptr;
        csw = (MS_BOT_CSW *)currDrive->csw_ptr;
        currDrive->ms_cbw_tag ++;
        cbw->signature = HOST_2_LE32 (MS_CBW_SIGNATURE);
        cbw->tag = HOST_2_LE32 (currDrive->ms_cbw_tag);
        cbw->data_transfer_length = HOST_2_LE32 (cmd_blk->buf_length);
        cbw->flags = (cmd_blk->direction == USB_DIR_IN) ?
                     MS_CBW_FLAG_IN : MS_CBW_FLAG_OUT;

        /* Fill with proper lun number. */
        cbw->lun = lun & 0x0F;

        /* Length of the SCSI command. */
        cbw->cb_length = cmd_blk->cmd_length;

        memset (cbw->cb, 0, NU_USBH_MS_CBW_CB_SIZE);

        /* Copy the Subclass Specific Command on CBW. */
        for (ii = 0; ii < cmd_blk->cmd_length; ii++)
        {
            cbw->cb[ii] = *((UINT8 *) cmd_blk->command + ii);
        }

        /* Intialize csw */
        memset((VOID*)csw,0,sizeof(MS_BOT_CSW));

        /* Fill in the IRP structure and submit the irp. */
        status = NU_USB_IRP_Create(bulk_out_irp,
                                    0x1F,
                                    (UINT8*)cbw,
                                    NU_TRUE,
                                    NU_FALSE,
                                    UHMS_IRP_Complete,
                                    currDrive,
                                    0x00);
        if ( status != NU_SUCCESS )
        {
            break;
        }


        /* Submit the IRP. */
        status = UHMS_Submit_IRP(   currDrive,
                                      currDrive->bulk_out_pipe,
                                    (NU_USB_IRP *) bulk_out_irp);
        if ( status != NU_SUCCESS )
        {
            status = NU_USBH_MS_TRANSPORT_ERROR;
            break;
        }

        /* Wait for the the IRP to complete. */
        status = NU_Obtain_Semaphore (&currDrive->tx_request,
                                      NU_USBH_MS_TX_LOCK_TIMEOUT);
        if(status != NU_SUCCESS)
        {
            status = NU_USBH_MS_TRANSPORT_ERROR;
            break;
        }

        /* If we stall, we need to clear it before we go on. */
        status = NU_USB_IRP_Get_Status (bulk_out_irp, &irp_status);
        if ( status != NU_SUCCESS )
        {
            break;
        }

        if (irp_status == NU_USB_STALL_ERR)
        {
            /* Ref. "Bulk-Only Transport" Sec.5.3.1 */
            status = UHMS_Bulk_Reset_Recovery (cb, stack, currDrive);
            if ( status == NU_SUCCESS )
            {
                status = NU_USBH_MS_TRANSPORT_ERROR;
            }
            break;
        }
        else if (irp_status != NU_SUCCESS)
        {
            /* Unknown error -- we've got a problem. */
            status = NU_USBH_MS_TRANSPORT_ERROR;
            break;
        }

        /* If the command transferred well, then we go to the data
         * stage.
         */

        /* Send/receive data payload, if there is any. */
        if (cbw->data_transfer_length)
        {
            if (cmd_blk->direction == USB_DIR_IN)
            {
                status = NU_USB_IRP_Create(bulk_in_irp,
                                          cmd_blk->buf_length,
                                          (UINT8*)cmd_blk->data_buf,
                                          NU_TRUE,
                                          NU_FALSE,
                                          UHMS_IRP_Complete,
                                          currDrive,
                                          0x00);

                if ( status != NU_SUCCESS )
                {
                    break;
                }

                status = NU_USB_IRP_Set_Buffer_Type_Cachable(bulk_in_irp,
                                                    currDrive->is_data_buff_cachable);

                if ( status != NU_SUCCESS )
                {
                    break;
                }

                status = UHMS_Submit_IRP (  currDrive,
                                            currDrive->bulk_in_pipe,
                                            bulk_in_irp);
                if ( status != NU_SUCCESS )
                {
                    status = NU_USBH_MS_TRANSPORT_ERROR;
                    break;
                }

                /*  Wait for the the IRP to complete. */
                status = NU_Obtain_Semaphore (&currDrive->tx_request,
                                              NU_USBH_MS_TX_LOCK_TIMEOUT);

                if (status != NU_SUCCESS)
                {
                    /* Unknown error -- we've got a problem. */
                    status = NU_USBH_MS_TRANSPORT_ERROR;
                    break;
                }

                /* Get transfer status. */
                status = NU_USB_IRP_Get_Status (bulk_in_irp, &irp_status);
                if ( status != NU_SUCCESS )
                {
                    break;
                }

                if (irp_status == NU_USB_STALL_ERR)
                {
                    /* Clear the HALT state of the pipe. */
                    status = NU_USB_PIPE_Unstall (currDrive->bulk_in_pipe);
                    if ( status != NU_SUCCESS )
                    {
                        break;
                    }
                }
                else if(irp_status != NU_SUCCESS)
                {
                    if (irp_status == NU_USB_DATA_OVERRUN)
                    {
                       /* Device sent more data than host asked for. */
                       /* In this case, do not skip status(CSW) stage. */
                    }
                    else
                    {
                        /* Some error occured, skip status(CSW) stage. */
                        status = NU_USBH_MS_TRANSPORT_ERROR;
                        break;
                    }
                }
            }
            else /* (cmd_blk->direction == USB_DIR_OUT) */
            {
                status = NU_USB_IRP_Create(bulk_out_irp,
                                          cmd_blk->buf_length,
                                          (UINT8*)cmd_blk->data_buf,
                                          NU_TRUE,
                                          NU_FALSE,
                                          UHMS_IRP_Complete,
                                          currDrive,
                                          0x00);

                if ( status != NU_SUCCESS )
                {
                    break;
                }

                status = NU_USB_IRP_Set_Buffer_Type_Cachable(bulk_out_irp,
                                                    currDrive->is_data_buff_cachable);

                if ( status != NU_SUCCESS )
                {
                    break;
                }

                status = UHMS_Submit_IRP ( currDrive,
                                          currDrive->bulk_out_pipe,
                                          bulk_out_irp);
                if ( status != NU_SUCCESS )
                {
                    status = NU_USBH_MS_TRANSPORT_ERROR;
                    break;
                }

                /* Wait for the the IRP to complete. */
                status = NU_Obtain_Semaphore (&currDrive->tx_request,
                                            NU_USBH_MS_TX_LOCK_TIMEOUT);

                if (status != NU_SUCCESS)
                {
                    /* Unknown error -- we've got a problem. */
                    status = NU_USBH_MS_TRANSPORT_ERROR;
                    break;
                }

                /* Get transfer status. */
                status = NU_USB_IRP_Get_Status (bulk_out_irp, &irp_status);
                if ( status != NU_SUCCESS )
                {
                    break;
                }

                if (irp_status == NU_USB_STALL_ERR)
                {
                    /* Clear the HALT state of the pipe. */
                    status = NU_USB_PIPE_Unstall (currDrive->bulk_out_pipe);
                    if ( status != NU_SUCCESS )
                    {
                        break;
                    }
                }
                else if(irp_status != NU_SUCCESS)
                {
                    status = NU_USBH_MS_TRANSPORT_ERROR;
                    break;
                }
            }
        }

        /* See flow chart on page# 15 of the Bulk Only Transport spec. */

        /* Get CSW for device status. */
        status = NU_USB_IRP_Create(bulk_in_irp,
                                13,
                                (UINT8 *) csw,
                                NU_TRUE,
                                NU_FALSE,
                                UHMS_IRP_Complete,
                                currDrive,
                                0x00);
        if ( status != NU_SUCCESS )
        {
            break;
        }

        status = UHMS_Submit_IRP ( currDrive,
                                    currDrive->bulk_in_pipe,
                                    bulk_in_irp);
        if ( status != NU_SUCCESS )
        {
          status = NU_USBH_MS_TRANSPORT_ERROR;
          break;
        }

        /* Wait for the the IRP to complete. */
        status = NU_Obtain_Semaphore (&currDrive->tx_request,
                                      NU_USBH_MS_TX_LOCK_TIMEOUT);

        if (status != NU_SUCCESS)
        {
            /* Unknown error -- we've got a problem. */
            status = NU_USBH_MS_TRANSPORT_ERROR;
            break;
        }

        status = NU_USB_IRP_Get_Status (bulk_in_irp, &irp_status);
        if ( status != NU_SUCCESS )
        {
            break;
        }

        if (irp_status == NU_USB_STALL_ERR)
        {
            status = NU_USB_PIPE_Unstall (currDrive->bulk_in_pipe);
            if ( status != NU_SUCCESS )
            {
                break;
            }

            /* Get the status again. */
            status = UHMS_Submit_IRP ( currDrive,
                                      currDrive->bulk_in_pipe,
                                      bulk_in_irp);
            if ( status != NU_SUCCESS )
            {
                status = NU_USBH_MS_TRANSPORT_ERROR;
                break;
            }

            /* Wait for the the IRP to complete. */
            status = NU_Obtain_Semaphore (&currDrive->tx_request, NU_USBH_MS_TX_LOCK_TIMEOUT);
            if (status != NU_SUCCESS)
            {
                /* Unknown error -- we've got a problem. */
                status = NU_USBH_MS_TRANSPORT_ERROR;
                break;
            }

            /* If it fails again, we need a reset and
             * return an error.
             */
            status = NU_USB_IRP_Get_Status (bulk_in_irp, &irp_status);
            if ( status != NU_SUCCESS )
            {
                break;
            }

            if (irp_status == NU_USB_STALL_ERR)
            {
                status = UHMS_Bulk_Reset_Recovery (cb, stack, currDrive);
                if ( status == NU_SUCCESS )
                {
                    status = NU_USBH_MS_TRANSPORT_ERROR;
                }
                break;
            }
            else if (irp_status != NU_SUCCESS)
            {
                status = NU_USBH_MS_TRANSPORT_ERROR;
                break;

            }
        }        /* if (irp_status == NU_USB_STALL_ERR) */

        /* If we still have a failure at this point,
         * we're in trouble.
         */
        if (irp_status != NU_SUCCESS)
        {
            status = NU_USBH_MS_TRANSPORT_ERROR;
            break;
        }

        /* Check bulk status. */
        if (!UHMS_Validate_CSW (currDrive, *csw, cmd_blk))
        {
            status = NU_USBH_MS_TRANSPORT_ERROR;
            break;
        }

        /* Based on the status code,  we report
             * good or bad.
             */
        switch (csw->status)
        {
            /* Command good -- note that data could be short. */
            case MS_CSW_GOOD:
                status = NU_SUCCESS;
                break;

            /* Command failure. */
            case MS_CSW_FAIL:
                status = NU_USBH_MS_TRANSPORT_FAILED;
                break;

                  /* Case (7), (8), (10), (13). */
            case MS_CSW_PHASE_ERROR:
                if (cmd_blk->direction == USB_DIR_IN)
                {
                    /* Reset Recovery. */
                    status = UHMS_Bulk_Reset_Recovery (cb, stack, currDrive);
                    if ( status != NU_SUCCESS )
                    {
                        break;
                    }
                }
                else  /* (cmd_blk->direction == USB_DIR_OUT) */
                {
                    if (cmd_blk->buf_length < currDrive->bulk_out_irp.actual_length)
                    {
                        /* Reset Recovery. */
                        status = UHMS_Bulk_Reset_Recovery (cb, stack, currDrive);
                        if ( status != NU_SUCCESS )
                        {
                            break;
                        }
                    }
                }
                status = NU_USBH_MS_TRANSPORT_ERROR;
                break;

            default:

                /* We should never get here, but if
                 * we do, we're in trouble.
                 */
                status = NU_USBH_MS_TRANSPORT_ERROR;
                break;
        }

    } while(0);

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       UHMS_CBI_Transport
*
* DESCRIPTION
*
*       This function Transfers a media specific command block to mass
*       storage device using CBI protocol as defined in USB Mass Storage
*       Class Specification. This is done by sending a ADSC command on
*       control pipe. Reading/Writing data on bulk pipe and Completion
*       event reported on an interrupt pipe.
*
* INPUTS
*
*       cb                                  Pointer to the Mass Storage
*                                           Driver control block.
*       stack                               Pointer to Stack Control Block.
*       currDrive                           Pointer to the DRIVE structure
*                                           for this Interface.
*       cmd_blk                             Pointer to command block to be
*                                           transmitted to device.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USBH_MS_TRANSPORT_ERROR          Indicates Command doesn't
*                                           completed successfully.
*       NU_USBH_MS_TRANSPORT_FAILED         Indicates command failed by the
*                                           media.
*
**************************************************************************/
STATUS UHMS_CBI_Transport (NU_USBH_MS * cb,
                          NU_USB_STACK * stack,
                          NU_USBH_MS_DRIVE * currDrive,
                          USBH_MS_CB * cmd_blk)
{
    STATUS     result;
    STATUS     irp_status, status;
    NU_USB_IRP *bulk_in_irp;
    NU_USB_IRP *bulk_out_irp;
    NU_USB_IRP *interrupt_irp;

    status              = NU_SUCCESS;
    bulk_in_irp         = &(currDrive->bulk_in_irp);
    bulk_out_irp        = &(currDrive->bulk_out_irp);
    interrupt_irp       = &(currDrive->interrupt_irp);
    currDrive->irq_data = NU_NULL;

    /* Populate Bulk In IRP. */
    status = NU_USB_IRP_Create (bulk_in_irp,
                       0,                   /* Length  of data.   */
                       NU_NULL,             /* Data.              */
                       1, 0,
                       UHMS_IRP_Complete,   /* Callback function. */
                       NU_NULL,             /* Context.           */
                       0);
    if ( status == NU_SUCCESS )
    {
        /* Populate Bulk Out IRP. */
        status = NU_USB_IRP_Create (bulk_out_irp,
                           0,                   /* Length  of data.   */
                           NU_NULL,             /* Data.              */
                           1, 0,
                           UHMS_IRP_Complete,    /* Callback function. */
                           NU_NULL,             /* Context.           */
                           0);
    }

    if ( status == NU_SUCCESS )
    {
        /* Allocate memory for irq data. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     NU_USBH_MS_IRQ_DATA_SIZE,
                                     (VOID **)&currDrive->irq_data);
        if (status == NU_SUCCESS)
        {
            /* Populate Interrupt IRP. */
            status = NU_USB_IRP_Create (interrupt_irp,
                                        0,           /* Length  of data.   */
                                        currDrive->irq_data, /* Data.              */
                                        1, 0,
                                        UHMS_Intr_Callback, /* Callback function. */
                                        NU_NULL,       /* Context.           */
                                        0);
       }
    }

    do
    {
        /* COMMAND STAGE. */

        if ( status != NU_SUCCESS )
        {
            break;
        }

        /* Let's send the command via the control pipe. */
        result = UHMS_CBI_Send_ADSC (cb, stack, currDrive, cmd_blk);

        /* Check the return code for the command. */
        if (result != NU_SUCCESS)
        {
            /* If the command was aborted, indicate that. */
            if (result == NU_USBH_MS_TRANSPORT_ABORTED)
            {
                status = NU_USBH_MS_TRANSPORT_ABORTED;
            }
            else
            {
                 /* ... serious problem here. */
                 status = NU_USBH_MS_TRANSPORT_ERROR;
            }

            break;
        }

        /* DATA STAGE. */

        /* Transfer the data payload for this command, if one exists. */
        if (cmd_blk->buf_length)
        {
            /* Fill in the IRP structure and re-submit the irp.
             * Most of the fields are filled only once at the connect time.
             */
            if (cmd_blk->direction == USB_DIR_IN)
            {
                status = NU_USB_IRP_Create (bulk_in_irp,
                                   cmd_blk->buf_length,           /* Length  of data.   */
                                   cmd_blk->data_buf,             /* Data.              */
                                   1, 0,
                                   UHMS_IRP_Complete,             /* Callback function. */
                                   (VOID *)currDrive,             /* Context.           */
                                   0);
                
                if ( status != NU_SUCCESS )
                {
                    status = NU_USB_UNKNOWN_ERR;
                    break;
                }
                                   

                status = UHMS_Submit_IRP (  currDrive,
                                            currDrive->bulk_in_pipe,
                                            bulk_in_irp);
      

                if ( status != NU_SUCCESS )
                {
                    status = NU_USBH_MS_TRANSPORT_ERROR;
                    break;
                }

                /* Wait for the the IRP to complete. */
                status = NU_Obtain_Semaphore (&currDrive->tx_request,
                                            NU_USBH_MS_TX_LOCK_TIMEOUT);
                if (status != NU_SUCCESS)
                {
                    /* Unknown error -- we've got a problem. */
                    status = NU_USBH_MS_TRANSPORT_ERROR;
                    break;
                }

                /* If it was aborted, we need to indicate that. */
                status = NU_USB_IRP_Get_Status (bulk_in_irp, &irp_status);
                if ( status != NU_SUCCESS )
                {
                    break;
                }

                if (irp_status == NU_USBH_MS_TRANSPORT_ABORTED)
                {
                    status = NU_USBH_MS_TRANSPORT_ABORTED;
                    break;
                }
                else if ( irp_status == NU_USB_STALL_ERR )
                {
                    /* Clear the HALT state of the pipes. */
                    status = NU_USB_PIPE_Unstall (currDrive->bulk_in_pipe);
                    if ( status == NU_SUCCESS )
                    {
                        status = NU_USB_PIPE_Unstall (currDrive->bulk_out_pipe);
                    }
                    if ( status == NU_SUCCESS )
                    {
                        status = NU_USBH_MS_TRANSPORT_FAILED;
                    }
                    break;
                }
            }
            else  /* (cmd_blk->direction == USB_DIR_OUT) */
            {
                status = NU_USB_IRP_Create (bulk_out_irp,
                                   cmd_blk->buf_length,           /* Length  of data.   */
                                   cmd_blk->data_buf,             /* Data.              */
                                   1, 0,
                                   UHMS_IRP_Complete,             /* Callback function. */
                                   (VOID *)currDrive,             /* Context.           */
                                   0);

                status = UHMS_Submit_IRP (  currDrive,
                                            currDrive->bulk_out_pipe,
                                            bulk_out_irp);
                if ( status != NU_SUCCESS)
                {
                    status = NU_USBH_MS_TRANSPORT_ERROR;
                    break;
                }

                /* Wait for the the IRP to complete. */
                status = NU_Obtain_Semaphore (&currDrive->tx_request,
                                     NU_USBH_MS_TX_LOCK_TIMEOUT);
                if (status != NU_SUCCESS)
                {
                    /* Unknown error -- we've got a problem. */
                    status = NU_USBH_MS_TRANSPORT_ERROR;
                    break;
                }

                /* If it was aborted, we need to indicate that. */
                status = NU_USB_IRP_Get_Status (bulk_out_irp, &irp_status);
                if ( status != NU_SUCCESS )
                {
                    break;
                }

                if (irp_status == NU_USBH_MS_TRANSPORT_ABORTED)
                {
                    status = NU_USBH_MS_TRANSPORT_ABORTED;
                    break;
                }
                else if (irp_status == NU_USB_STALL_ERR)
                {
                    /* Clear the HALT state of the pipes. */
                    status = NU_USB_PIPE_Unstall (currDrive->bulk_out_pipe);
                    if ( status == NU_SUCCESS )
                    {
                        status = NU_USB_PIPE_Unstall (currDrive->bulk_in_pipe);
                    }
                    if ( status == NU_SUCCESS )
                    {
                        status = NU_USBH_MS_TRANSPORT_FAILED;
                    }
                    break;
                }
            }
        }   /* if (cmd_blk->buf_length) */

        /* STATUS STAGE. */
        status = NU_USB_IRP_Create (interrupt_irp,
                           2,           /* Length  of data.   */
                           currDrive->irq_data,             /* Data.              */
                           1, 0,
                           UHMS_Intr_Callback,             /* Callback function. */
                           (VOID *)currDrive,             /* Context.           */
                           0);

        status = UHMS_Submit_IRP (  currDrive,
                                    currDrive->interrupt_pipe,
                                    interrupt_irp);
        if ( status != NU_SUCCESS )
        {
            status = NU_USBH_MS_TRANSPORT_ERROR;
            break;
        }

        /* Go to sleep until we get this interrupt. */
        status = NU_Obtain_Semaphore (&currDrive->tx_request,
                                 NU_USBH_MS_TX_LOCK_TIMEOUT);
        if (status != NU_SUCCESS)
        {
            /* Unknown error -- we've got a problem. */
            status = NU_USBH_MS_TRANSPORT_ERROR;
            break;
        }

        /* If we were waken up by an abort instead of the actual
         * interrupt.
         */
        status = NU_USB_IRP_Get_Status (interrupt_irp, &irp_status);
        if ( status != NU_SUCCESS )
        {
            break;
        }

        if (irp_status == NU_USB_STALL_ERR)
        {
        /* Clear the HALT state of the interrupt pipe. */
            status = NU_USB_PIPE_Unstall (currDrive->interrupt_pipe);
            if ( status == NU_SUCCESS )
            {
                status = NU_USBH_MS_TRANSPORT_FAILED;
            }
            break;
        }
        else if (irp_status != NU_SUCCESS)
        {
            status = NU_USBH_MS_TRANSPORT_ABORTED;
            break;
        }

        /* UFI gives us ASC and ASCQ, like a request sense
         * REQUEST_SENSE and INQUIRY don't affect the sense
         * data on UFI devices, so we need to ignore the
         * information for those commands.This will be done
         * when UFI will be implemented.
         */

        /* If not UFI, we interpret the data as a result code
         * The first byte should always be a 0x0
         * The second byte & 0x0F should be 0x0 for good,
             * otherwise error.
         */        
        if (currDrive->irq_data)
        {
            if (currDrive->irq_data[0])
            {
                status = NU_USBH_MS_TRANSPORT_ERROR;
            }
            else
            {
                switch (currDrive->irq_data[1] & 0x0F)
                {
                    case 0x00:
                        status = NU_SUCCESS;
                        break;
    
                    case 0x01:
                        status = NU_USBH_MS_TRANSPORT_FAILED;
                        break;
    
                    default:
                        status = NU_USBH_MS_TRANSPORT_ERROR;
                        break;
                }
            }
        }
    } while(0);
    
    /* De-allocate irq data memory. */
    if (currDrive->irq_data)
    {
        USB_Deallocate_Memory(currDrive->irq_data);
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       UHMS_CB_Transport
*
* DESCRIPTION
*
*       This function Transfers a media specific command block to mass
*       storage device using CB protocol as defined in USB Mass Storage
*       Class Specification. This is done by sending a ADSC command on
*       control pipe. Reading/Writing data on bulk pipe with no status
*       reporting.
*
* INPUTS
*
*       cb                                  Pointer to the Mass Storage
*                                           Driver control block.
*       stack                               Pointer to Stack Control Block.
*       currDrive                           Pointer to the DRIVE structure
*                                           for this Interface.
*       cmd_blk                             Pointer to the command block to
*                                           be transmitted.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USBH_MS_TRANSPORT_ERROR          Indicates Command doesn't
*                                           completed successfully.
*       NU_USBH_MS_TRANSPORT_FAILED         Indicates command failed by the
*                                           media.
*
**************************************************************************/
STATUS UHMS_CB_Transport (NU_USBH_MS * cb,
                         NU_USB_STACK * stack,
                         NU_USBH_MS_DRIVE * currDrive,
                         USBH_MS_CB * cmd_blk)
{
    STATUS result;
    STATUS irp_status;
    STATUS status;
    NU_USB_IRP *bulk_in_irp;
    NU_USB_IRP *bulk_out_irp;

    status          = NU_SUCCESS;
    bulk_in_irp     = &(currDrive->bulk_in_irp);
    bulk_out_irp    = &(currDrive->bulk_out_irp);

    /* Populate Bulk In IRP. */
    status = NU_USB_IRP_Create (bulk_in_irp,
                       0,             /* Length  of data.   */
                       NU_NULL,             /* Data.                */
                       1, 0,
                       UHMS_IRP_Complete,  /* Callback function.   */
                       NU_NULL,             /* Context.             */
                       0);
    if ( status == NU_SUCCESS )
    {
        /* Populate Bulk Out IRP. */
        status = NU_USB_IRP_Create (bulk_out_irp,
                       0,             /* Length  of data.     */
                       NU_NULL,             /* Data.                */
                       1, 0,
                       UHMS_IRP_Complete,  /* Callback function.   */
                       NU_NULL,             /* Context.             */
                       0);
    }

    do
    {
        /* COMMAND STAGE. */

        if ( status != NU_SUCCESS )
        {
              break;
        }

        /* Let's send the command via the control pipe. */
        result = UHMS_CBI_Send_ADSC (cb, stack, currDrive, cmd_blk);

        /* Check the return code for the command. */
        if (result != NU_SUCCESS)
        {
            /* If the command was aborted, indicate that. */
            if (result == NU_USBH_MS_TRANSPORT_ABORTED)
            {
                status = NU_USBH_MS_TRANSPORT_ABORTED;
            }
            else
            {
                /* Serious problem here. */
                status = NU_USBH_MS_TRANSPORT_ERROR;
            }

            break;
        }

        /* DATA STAGE. */

        /* Transfer the data payload for this command, if one exists. */
        if (cmd_blk->buf_length)
        {
            /* Fill in the IRP structure and re-submit the IR. */
            /* Most of the fields are filled only once at the connect
             * time.
             */
            if (cmd_blk->direction == USB_DIR_IN)
            {
                status = NU_USB_IRP_Create (bulk_in_irp,
                                   cmd_blk->buf_length,             /* Length  of data.   */
                                   cmd_blk->data_buf,               /* Data.                */
                                   1, 0,
                                   UHMS_IRP_Complete,               /* Callback function.   */
                                   (VOID *)currDrive,               /* Context.             */
                                   0);
                                   
                if ( status != NU_SUCCESS )
                {
                    status = NU_USB_UNKNOWN_ERR;
                    break;
                }
               
                status = UHMS_Submit_IRP (  currDrive,
                                            currDrive->bulk_in_pipe,
                                            bulk_in_irp);

                
                if ( status != NU_SUCCESS )
                {
                    status = NU_USBH_MS_TRANSPORT_ERROR;
                    break;
                }

                /* Wait for the the IRP to complete. */
                status = NU_Obtain_Semaphore (&currDrive->tx_request,
                                     NU_USBH_MS_TX_LOCK_TIMEOUT);
                if (status != NU_SUCCESS)
                {
                    /* Unknown error -- we've got a problem. */
                    status = NU_USBH_MS_TRANSPORT_ERROR;
                    break;
                }

                /* If it was aborted, we need to indicate that. */
                status = NU_USB_IRP_Get_Status (bulk_in_irp, &irp_status);
                if ( status != NU_SUCCESS )
                {
                    break;
                }

                if (irp_status == NU_USBH_MS_TRANSPORT_ABORTED)
                {
                    status = NU_USBH_MS_TRANSPORT_ABORTED;
                    break;
                }
                else if (irp_status == NU_USB_STALL_ERR)
                {
                  /* Clear the HALT state of the pipe. */
                    status = NU_USB_PIPE_Unstall (currDrive->bulk_in_pipe);
                    if ( status == NU_SUCCESS )
                    {
                        status = NU_USBH_MS_TRANSPORT_FAILED;
                    }
                    break;
                }
            }
            else  /* (cmd_blk->direction == USB_DIR_OUT) */
            {
                status = NU_USB_IRP_Create (bulk_out_irp,
                                   cmd_blk->buf_length,             /* Length  of data.   */
                                   cmd_blk->data_buf,               /* Data.                */
                                   1, 0,
                                   UHMS_IRP_Complete,               /* Callback function.   */
                                   (VOID *)currDrive,               /* Context.             */
                                   0);

                if ( status != NU_SUCCESS )
                {
                    status = NU_USB_UNKNOWN_ERR;
                    break;
                }
                
                status = UHMS_Submit_IRP (  currDrive,
                                            currDrive->bulk_out_pipe,
                                            bulk_out_irp);

                if ( status != NU_SUCCESS )
                {
                    status = NU_USBH_MS_TRANSPORT_ERROR;
                    break;
                }

                /* Wait for the IRP to complete. */
                status = NU_Obtain_Semaphore (&currDrive->tx_request,
                                     NU_USBH_MS_TX_LOCK_TIMEOUT);
                if (status != NU_SUCCESS)
                {
                    /* Unknown error -- we've got a problem. */
                    status = NU_USBH_MS_TRANSPORT_ERROR;
                    break;
                }

        /* If it was aborted, we need to indicate that. */
                status = NU_USB_IRP_Get_Status (bulk_out_irp, &irp_status);
                if ( status != NU_SUCCESS )
                {
                    break;
                }

                if (irp_status == NU_USBH_MS_TRANSPORT_ABORTED)
                {
                    status = NU_USBH_MS_TRANSPORT_ABORTED;
                    break;
                }
                else
                if (irp_status == NU_USB_STALL_ERR)
                {
                    status = NU_USB_PIPE_Unstall (currDrive->bulk_out_pipe);
                    if ( status == NU_SUCCESS )
                    {
                        status = NU_USBH_MS_TRANSPORT_FAILED;
                    }
                    break;
                }
            }
        }

        /* STATUS STAGE. */
        /* CB does not have a status stage. */

    } while(0);

    return (status);
}
/**************************************************************************
*
* FUNCTION
*
*       UHMS_IRP_Complete
*
* DESCRIPTION
*
*       This function is called when an IRP on control or bulk pipe of a
*       mass storage device gets completed. It takes the context
*       information from the IRP and wakes up the task waiting on
*       submission of IRP.
*
* INPUTS
*
*       pipe                                Pointer to the Pipe control
*                                           block.
*       irp                                 Pointer to IRP control block.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
VOID UHMS_IRP_Complete (NU_USB_PIPE * pipe,NU_USB_IRP * irp)
{
    VOID *context;
    STATUS status;

    /* Remove unused parameter warnings. */
    NU_UNUSED_PARAM(pipe);


    /* Gets the cookie from IRP. */
    status = NU_USB_IRP_Get_Context (irp, &context);
    NU_USB_ASSERT(status == NU_SUCCESS);
    if(context != NU_NULL )
    {
        /* Release the semaphore and resume the task. */
        status = NU_Release_Semaphore (&((NU_USBH_MS_DRIVE *) context)->tx_request);
        NU_USB_ASSERT(status == NU_SUCCESS);
    }
}

/**************************************************************************
*
* FUNCTION
*
*       UHMS_Intr_Callback
*
* DESCRIPTION
*
*       This function is called when an IRP on the Interrupt pipe gets
*       completed. It checks for the status and actual length of the
*       transmitted IRP.
*
* INPUTS
*
*       pipe                                Pointer to the Pipe control
*                                           block.
*       irp                                 Pointer to IRP control block.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
VOID UHMS_Intr_Callback (NU_USB_PIPE * pipe,
                        NU_USB_IRP * irp)
{
    STATUS  status = NU_SUCCESS;
    VOID  *context;
    UINT32 actual_len;
    /* Remove unused parameter warnings. */
    NU_UNUSED_PARAM(pipe);
    NU_UNUSED_PARAM(status);

    status = NU_USB_IRP_Get_Context (irp, &context);
    NU_USB_ASSERT( status == NU_SUCCESS );
    NU_USB_IRP_Get_Actual_Length(irp,&actual_len);
    NU_USB_ASSERT(actual_len != 2)
    /* Wake up the command thread. */
    status = NU_Release_Semaphore (&((NU_USBH_MS_DRIVE *)context)->tx_request);
    NU_USB_ASSERT( status == NU_SUCCESS );
}

/**************************************************************************
*
* FUNCTION
*
*      UHMS_Validate_Drive
*
* DESCRIPTION
*
*      This is an internal routine called in transfer API service to check
*      if a storage drive is valid and attached.
*
* INPUTS
*
*      cb                  Pointer to MSC driver.
*      session             Pointer to disk partition.
*
* OUTPUTS
*
*      None.
*
* RETURNS
*
*      NU_USBH_MS_DRIVE    Pointer to mass storage disk from internal
*                          link list.
*
**************************************************************************/
NU_USBH_MS_DRIVE*  UHMS_Validate_Drive(NU_USBH_MS    * cb,
                                          VOID          * session)
{
    STATUS status;
    UINT8 i = 0x00;
    UINT8 drive_found = 0x00;
    NU_USBH_MS_DRIVE *ms_curr_drive = NU_NULL;
    NU_USBH_MS_DRIVE *ms_next_drive = NU_NULL;


    status = NU_Obtain_Semaphore (&cb->driver_lock, NU_SUSPEND);
    if ( status == NU_SUCCESS )
    {
        ms_curr_drive = cb->session_list_head;

        while(ms_curr_drive)
        {
            ms_next_drive = (NU_USBH_MS_DRIVE *) ms_curr_drive->node.cs_next;

            for(i = 0x00; i <= (ms_curr_drive->max_lun); i++)
            {
                if ( ((&ms_curr_drive->lun_info[i]) == session) &&
                    (ms_curr_drive->device_state == NU_USBH_MS_CONNECTED) )
                {
                    drive_found++;
                    break;
                }
            }

            if (drive_found)
            {
                break;
            }

            if (ms_next_drive == cb->session_list_head) /* no more drives */
            {
                ms_curr_drive = NU_NULL;
            }
            else
            {
                ms_curr_drive = ms_next_drive;
            }
        }

        status = NU_Release_Semaphore (&cb->driver_lock);
        NU_USB_ASSERT( status == NU_SUCCESS );
    }

    return (ms_curr_drive);
}

/**************************************************************************
*
* FUNCTION
*
*      UHMS_Validate_Device
*
* DESCRIPTION
*
*      This is an internal routine called in event reporter function. This
*       checks for a device if it present in device list or not.
*
* INPUTS
*
*      cb                  Pointer to MSC driver.
*      drive               Pointer to mass storage device.
*
* OUTPUTS
*
*      None.
*
* RETURNS
*
*      NU_TRUE             If device is present in device list.
*      NU_FALSE            If device is not present in device list.
*
**************************************************************************/
BOOLEAN UHMS_Validate_Device(NU_USBH_MS          * cb,
                                NU_USBH_MS_DRIVE    * drive)
{
    NU_USBH_MS_DRIVE *next, *current;
    BOOLEAN found;
    STATUS status;
    found = NU_FALSE;

    status = NU_Obtain_Semaphore (&cb->driver_lock, NU_SUSPEND);
    if ( status == NU_SUCCESS )
    {
        current = cb->session_list_head;

        while (current)
        {
            next = (NU_USBH_MS_DRIVE *) current->node.cs_next;

            if ( current == drive )
            {
                found = NU_TRUE;
                break;
            }

            if ((next                   == cb->session_list_head) ||
                (cb->session_list_head  == NU_NULL))
            {
               current = NU_NULL;
               break;
            }
            else
            {
                current = next;
            }
        }

        status = NU_Release_Semaphore (&cb->driver_lock);
        NU_USB_ASSERT( status == NU_SUCCESS );
    }

    return ( found );
}

/**************************************************************************
*
* FUNCTION
*
*       USB_MSC_Event_Reporters
*
* DESCRIPTION
*
*       This is an entry function for MSC driver's conn/discon events. This
*       is invoked only when a device conn/disconn is reported.
*
* INPUTS
*
*       argc                Unused parameter.
*       cb                  Pointer to MSC driver.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
VOID UHMS_Event_Reporter (UNSIGNED argc, VOID *cb)
{
    STATUS status;
    UINT8 i = 0x00;
    UINT8 user_count = 0x00;
    NU_USBH_MS *mscb = NU_NULL;
    NU_USBH_MS_DRIVE *ms_curr_drive = NU_NULL;
    UNSIGNED rcv_message[NU_USBH_MS_MSG_SIZE];
    UNSIGNED received_size = 0x00;

    mscb = (NU_USBH_MS *) cb;

    for (;;)
    {
        status = NU_Receive_From_Queue (&(mscb->ms_queue),
                                        rcv_message,
                                        NU_USBH_MS_MSG_SIZE,
                                        &received_size,
                                        NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            if ( UHMS_Validate_Device(mscb,
                               (NU_USBH_MS_DRIVE*)rcv_message[1]) == NU_FALSE )
            {
                continue;
            }

            ms_curr_drive = (NU_USBH_MS_DRIVE *) rcv_message[1];

            /* Decode received message */
            if (rcv_message[0] == MSC_DISCONNECT_MESSAGE) /* DisConnect */
            {
                if(ms_curr_drive->device_state == NU_USBH_MS_DISCONNECTED)
                {
                    status = NU_Delete_Semaphore(&(ms_curr_drive->tx_request));
                    NU_USB_ASSERT( status == NU_SUCCESS );

                    status = NU_Delete_Semaphore(&(ms_curr_drive->drive_lock));
                    NU_USB_ASSERT( status == NU_SUCCESS );

                    for(i = 0x00; i <= (ms_curr_drive->max_lun) ; i++)
                    {
                        if(ms_curr_drive->lun_info[i].user_pointer != NU_NULL)
                        {
                            status = NU_USB_USER_Disconnect (ms_curr_drive->user,
                                        ms_curr_drive->drvr,
                                        &ms_curr_drive->lun_info[i]);
                            NU_USB_ASSERT( status == NU_SUCCESS );
                            if (status == NU_SUCCESS)
                            {
                                user_count++;
                            }
                        }
                    }

                    status = NU_Obtain_Semaphore (&mscb->driver_lock, NU_SUSPEND);
                    NU_USB_ASSERT( status == NU_SUCCESS );

                    /* Remove the Drive Structure from the List... */
                    NU_Remove_From_List ( (CS_NODE **) &mscb->session_list_head,
                                           (CS_NODE *) ms_curr_drive);

                    mscb->queue_av++;
                    mscb->connect_count--;

                    status = NU_Release_Semaphore (&mscb->driver_lock);
                    NU_USB_ASSERT( status == NU_SUCCESS );

                    /* Deallocate control IRP memory. */
                    status = USB_Deallocate_Memory (ms_curr_drive->ctrl_irp);
                    NU_USB_ASSERT( status == NU_SUCCESS );
                    
                    /* Deallocate cbw and csw memory. */
                    if (ms_curr_drive->cbw_ptr)
                    {
                        status = USB_Deallocate_Memory(ms_curr_drive->cbw_ptr);
                        NU_USB_ASSERT( status == NU_SUCCESS );
                    }
                    if (ms_curr_drive->csw_ptr)
                    {
                        status = USB_Deallocate_Memory(ms_curr_drive->csw_ptr);
                        NU_USB_ASSERT( status == NU_SUCCESS );
                    }

                    /* ...and Deallocate DRIVE structure. */
                    status = USB_Deallocate_Memory (ms_curr_drive);
                    NU_USB_ASSERT( status == NU_SUCCESS );
                }
            }
            else if (rcv_message[0] == MSC_CONNECT_MESSAGE) /* Connect */
            {
                user_count = 0x00;

                for (i = 0x00;
                     (i <= ms_curr_drive->max_lun) && (i < NU_USBH_MS_MAX_LUNS); i++)
                {
                    /* Store information for each LUN */
                    ms_curr_drive->lun_info[i].lun = i;
                    ms_curr_drive->lun_info[i].drive = ms_curr_drive;
                    ms_curr_drive->lun_info[i].user_pointer = ms_curr_drive->user;
                    /* Give Connect callback to USER with LUN_INFO as handle. */
                    status = NU_USB_USER_Connect (ms_curr_drive->user,
                              ms_curr_drive->drvr,
                              &ms_curr_drive->lun_info[i]);

                    if (status == NU_SUCCESS)
                    {
                        user_count++;
                    }
                }

                if (user_count == 0x00)
                {
                    status = NU_Delete_Semaphore(&ms_curr_drive->tx_request);
                    NU_USB_ASSERT( status == NU_SUCCESS );

                    status = NU_Delete_Semaphore(&ms_curr_drive->drive_lock);
                    NU_USB_ASSERT( status == NU_SUCCESS );

                    status = NU_Obtain_Semaphore (&mscb->driver_lock, NU_SUSPEND);
                    NU_USB_ASSERT( status == NU_SUCCESS );

                    /* Remove the Drive Structure from the List... */
                    NU_Remove_From_List ( (CS_NODE **) &mscb->session_list_head,
                                             (CS_NODE *) ms_curr_drive);
                    mscb->queue_av += 2;
                    status = NU_Release_Semaphore (&mscb->driver_lock);
                    NU_USB_ASSERT( status == NU_SUCCESS );

                    /* ...and Deallocate DRIVE structure. */
                    status = USB_Deallocate_Memory (ms_curr_drive);
                    NU_USB_ASSERT( status == NU_SUCCESS );
                }
                else
                {
                    mscb->queue_av++;
                    mscb->connect_count++;
                }
            }
        }
    }
}

/**************************************************************************
*
* FUNCTION
*
*       UHMS_Submit_IRP
*
* DESCRIPTION
*
*       This routine submit an IRP to host stack to request data IO after
*       disconnection check.
*       If a disconnection for the device has been reported then requested
*       PIPE should be no longer valid which may result in an exception.
*
* INPUTS
*
*       currDrive           Pointer to mass storage device.
*       p_pipe              Pointer to communicaton PIPE.
*       p_irp               Pointer to IRP containing IO information.
*
* OUTPUTS
*
*       None.
*
**************************************************************************/
STATUS UHMS_Submit_IRP (NU_USBH_MS_DRIVE*   currDrive,
                        NU_USB_PIPE*        p_pipe,
                        NU_USB_IRP*         p_irp)
{

    STATUS status;


    /* Submit IRP only incase where device is connected and exists in
     * host stack database.
     */
    if(currDrive->device_state == NU_USBH_MS_CONNECTED)
    {
        status = NU_USB_PIPE_Submit_IRP(p_pipe, p_irp);
    }
    else
    {
        status = NU_USBH_MS_DISCONNECTED;
    }
    return(status);
}
/* =====================  End Of File  ================================= */
