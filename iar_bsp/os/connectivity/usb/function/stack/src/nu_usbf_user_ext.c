/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************

***************************************************************************
*
* FILE NAME
*
*       nu_usbf_user_ext.c
*
*
* COMPONENT
*
*       Stack component/Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the exported function names and data structures
*       for the device (base) user component.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       _NU_USBF_USER_Create                Initializes a User component.
*       NU_USBF_USER_New_Command            Processes a new command.
*       NU_USBF_USER_New_Transfer           Processes a new transfer.
*       NU_USBF_USER_Notify                 USB event notification
*                                           processing.
*       NU_USBF_USER_Tx_Done                Transfer completion
*                                           notification.
*       _NU_USBF_USER_Delete                Disables a User component.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/
#ifndef USBF_USER_EXT_C
#define USBF_USER_EXT_C
/* ==============  USB Include Files ==================================  */

#include    "connectivity/nu_usb.h"

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_USER_Create
*
* DESCRIPTION
*
*       Initializes the User component.
*
*       The parameters describe the user. The subclass and protocol codes
*       are defined by the USB. These indicate the capabilities of the
*       device, in a family of devices. An example would be a SCSI device
*       in a family of Mass storage devices. If the protocol code is not
*       applicable for a specific user, it can be initialized to zero (0).
*       Subclass code, however can not be zero.
*
* INPUTS
*
*       cb                                  User control block.
*       name                                name of the User.
*       subclass                            USB Defined Subclass code,
*                                           supported by the user.
*       protocol                            USB Defined protocol code,
*                                           supported by the user.
*       dispatch                            User dispatch table.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the
*                                           Initialization has been
*                                           successful.
*       NU_NOT_PRESENT                      Indicates a configuration
*                                           problem because of which no
*                                           more usb objects could be
*                                           created.
*       NU_USB_INVLD_ARG                    Indicates an incorrect
*                                           parameter to this
*                                           function.
*
**************************************************************************/
STATUS  _NU_USBF_USER_Create (NU_USBF_USER *cb,
                              CHAR         *name,
                              UINT8         subclass,
                              UINT8         protocol,
                              const VOID   *dispatch)
{
    STATUS  status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(dispatch);
    NU_USB_PTRCHK_RETURN(name);

    status = _NU_USB_USER_Create ((NU_USB_USER *) cb,
                               (NU_USB_SUBSYS *) &(nu_usbf->user_subsys),
                               name, subclass, protocol, dispatch);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_USER_New_Command
*
* DESCRIPTION
*
*       Processes a new command from the Host.
*
*       These commands are class/subclass specific. This function processes
*       the command identified by the 'command' parameter. The length of
*       the command block, in bytes is expected in the 'cmd_len' parameter.
*
*       If there is any data transfer in response to a command, then the
*       location corresponding to the data to be transferred, either to
*       / from Host is filled in the 'data_out' parameter. The length of
*       the data to be transferred is filled in the location pointed to by
*       the 'data_len_out' parameter.
*
*       If there is no data transfer associated with the command, then, the
*       location pointed to by the 'data' is filled in with NU_NULL.
*
*       For unknown and unsupported command, this function returns
*       appropriate error status.
*
* INPUTS
*
*       cb                                  User control block for which
*                                           the command is meant for.
*       drvr                                Class driver invoking this
*                                           function.
*       handle                              Identifies for the logical
*                                           function to which the command
*                                           is directed to.
*       command                             Memory location where the
*                                           command block is stored.
*       cmd_len                             Length of the command block.
*       data_out                            Memory location where the data
*                                           pointer for the transfer is to
*                                           be stored.
*       data_len_out                        Memory location where the
*                                           length of data to be
*                                           transferred, in bytes, must be
*                                           filled.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the command has
*                                           been processed successfully.
*       NU_USB_NOT_SUPPORTED                Indicates that the command is
*                                           unsupported.
*       NU_USB_INVLD_ARG                    Indicates a malformed command
*                                           block.
*
**************************************************************************/
STATUS  NU_USBF_USER_New_Command (NU_USBF_USER  *cb,
                                  NU_USBF_DRVR  *drvr,
                                  VOID          *handle,
                                  UINT8         *command,
                                  UINT16         cmd_len,
                                  UINT8        **data_out,
                                  UINT32        *data_len_out)
{
    STATUS  status = NU_USB_NOT_SUPPORTED;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(drvr);
    NU_USB_PTRCHK_RETURN(command);

    if (((NU_USBF_USER_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
        New_Command != NU_NULL)
    {
        status = ((NU_USBF_USER_DISPATCH *)
            (((NU_USB *) cb)->usb_dispatch))->
            New_Command (cb, drvr, handle, command, cmd_len,
                         data_out, data_len_out);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_USER_New_Transfer
*
* DESCRIPTION
*
*       This function processes a transfer request from the host for a
*       previously sent command.
*
*       If there is any data transfer in response to a previously received
*       command, then the  location corresponding to the data to be
*       transferred, either to / from Host is filled in the 'data_out'
*       parameter. The length of the data to be transferred is filled
*       in the location pointed to by the 'data_len_out' parameter.
*
*       If there is no data transfer required, then this function returns
*       appropriate error status.
*
* INPUTS
*
*       cb                                  User control block.
*       drvr                                Class driver invoking this
*                                           function.
*       handle                              Identifies for the logical
*                                           function to which the transfer
*                                           is directed to.
*       data_out                            Memory location where the data
*                                           pointer for the transfer is to
*                                           be stored.
*       data_len_out                        Memory location where the
*                                           length of data to be
*                                           transferred, in bytes, must be
*                                           filled.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the function has
*                                           executed successfully.
*       NU_USB_INVLD_ARG                    Indicates a unexpected transfer
*                                           request from the Host.
*       NU_USB_NOT_SUPPORTED                Indicates that the new transfer
*                                           requests are not supported by
*                                           the user.
*
**************************************************************************/
STATUS  NU_USBF_USER_New_Transfer (NU_USBF_USER  *cb,
                                   NU_USBF_DRVR  *drvr,
                                   VOID          *handle,
                                   UINT8        **data_out,
                                   UINT32        *data_len_out)
{
    STATUS  status = NU_USB_NOT_SUPPORTED;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(drvr);
    NU_USB_PTRCHK_RETURN(handle);

    if (((NU_USBF_USER_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
        New_Transfer != NU_NULL)
    {
        status = ((NU_USBF_USER_DISPATCH *)
            (((NU_USB *) cb)->usb_dispatch))->
            New_Transfer (cb, drvr, handle, data_out, data_len_out);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_USER_Tx_Done
*
* DESCRIPTION
*
*       This function processes a transfer completion notification for a
*       previously submitted data transfer.
*
*       The completed data transfer is described in the 'completed_data'
*       and the 'completed_data_out' parameters. The 'completed_data'
*       contains the pointer to the memory location to / from which
*       'completed_data_out' bytes have been transferred from/to host.
*
*       If there is still data to be transferred to the previous
*       command, then the  location corresponding to the data to be
*       transferred, either to / from Host is filled in the 'data_out'
*       parameter. The length of the data to be transferred is filled
*       in the location pointed to by the 'data_len_out' parameter.
*
*       If there is no data transfer associated with the command, then, the
*       location pointed to by the 'data' is filled in with NU_NULL.
*
* INPUTS
*
*       cb                                  User control block.
*       drvr                                Class driver invoking this
*                                           function.
*       handle                              Identifies for the logical
*                                           function to which the
*                                           notification is directed to.
*       completed_data                      Memory location to / from where
*                                           the data has been transferred.
*       completed_data_len                  Length of data transferred,
*                                           in bytes.
*       data_out                            Memory location where the data
*                                           pointer for the transfer is to
*                                           be stored, if pending.
*       data_len_out                        Memory location where the
*                                           length of data to be
*                                           transferred, in bytes, is to be
*                                           filled.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the function has
*                                           executed successfully.
*       NU_USB_INVLD_ARG                    Indicates a unexpected transfer
*                                           completion.
*       NU_USB_NOT_SUPPORTED                Indicates that the transfer
*                                           completion notifications are
*                                           not supported by the user.
*
**************************************************************************/
STATUS  NU_USBF_USER_Tx_Done (NU_USBF_USER  *cb,
                              NU_USBF_DRVR  *drvr,
                              VOID          *handle,
                              UINT8         *completed_data,
                              UINT32         completed_data_len,
                              UINT8        **data_out,
                              UINT32        *data_len_out)
{
    STATUS  status = NU_USB_NOT_SUPPORTED;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(drvr);
    NU_USB_PTRCHK_RETURN(handle);

    if (((NU_USBF_USER_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
        Transfer_Complete != NU_NULL)
    {
        status = ((NU_USBF_USER_DISPATCH *)
            (((NU_USB *) cb)->usb_dispatch))->
            Transfer_Complete (cb, drvr, handle, completed_data,
                               completed_data_len, data_out, data_len_out);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_USER_Notify
*
* DESCRIPTION
*
*       This function processes a USB Event. Examples of such USB events
*       include suspend and Reset.
*
*       This function carries out function specific processing for the usb
*       event.
*
* INPUTS
*
*       cb                                  User control block for which
*                                           the event is meant for.
*       drvr                                Class driver invoking this
*                                           function.
*       handle                              Identifies for the logical
*                                           function to which the
*                                           notification is directed to.
*       event                               The USB event that has occurred
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the event has
*                                           been processed successfully.
*       NU_USB_INVLD_ARG                    Indicates a unexpected event.
*       NU_USB_NOT_SUPPORTED                Indicates that the event
*                                           notifications are not supported
*                                           by the user.
*
**************************************************************************/
STATUS  NU_USBF_USER_Notify (NU_USBF_USER *cb,
                             NU_USBF_DRVR *drvr,
                             VOID         *handle,
                             UINT32        event)
{
    STATUS  status = NU_USB_NOT_SUPPORTED;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(drvr);

    if (((NU_USBF_USER_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
        Notify != NU_NULL)
    {
        status = ((NU_USBF_USER_DISPATCH *)
            (((NU_USB *) cb)->usb_dispatch))->
            Notify (cb, drvr, handle, event);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_USER_Delete
*
* DESCRIPTION
*
*       This function disables the User.
*
* INPUTS
*
*       user                                User to be uninitialized.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the user has
*                                           been uninitialized successfully
*
**************************************************************************/
STATUS  _NU_USBF_USER_Delete (VOID *user)
{
    STATUS  status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(user);

    status = _NU_USB_USER_Delete (user);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************/
#endif /* USBF_USER_EXT_C */
/* ======================  End Of File  ================================ */
