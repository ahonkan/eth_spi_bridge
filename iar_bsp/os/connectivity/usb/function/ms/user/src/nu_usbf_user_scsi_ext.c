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
* FILE NAME
*
*       nu_usbf_user_scsi_ext.c
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the exported function implementations for the
*       Mass Storage SCSI media container.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBF_USER_SCSI_Create            Initializes the container.
*       _NU_USBF_USER_SCSI_Delete           Un-initializes the container.
*       NU_USBF_USER_SCSI_Dereg_Media       DeRegisters media with
*                                           container.
*       NU_USBF_USER_SCSI_Reg_Media         Registers media with
*                                           container.
*       _NU_USBF_USER_SCSI_Create           Initializes the container.
*       _NU_USBF_USER_SCSI_Command          New SCSI Command handling.
*       _NU_USBF_USER_SCSI_Connect          Host connected to Media.
*       _NU_USBF_USER_SCSI_Dereg_Media      Media deregistration with the
*                                           container.
*       _NU_USBF_USER_SCSI_Connect          Host connected to media.
*       _NU_USBF_USER_SCSI_Disconnect       Host disconnected from media.
*       _NU_USBF_USER_SCSI_Notify           USB Event processing.
*       _NU_USBF_USER_SCSI_Reg_Media        Media registration with the
*                                           container.
*       _NU_USBF_USER_SCSI_Reset            Media Reset.
*       _NU_USBF_USER_SCSI_Transfer         Host initiated transfer
*                                           processing.
*       _NU_USBF_USER_SCSI_Tx_Done          Transfer completion
*                                           processing.
*       NU_USBF_USER_SCSI_Init              This function is used to
*                                           initialize the SCSI user
*                                           driver component.
*       NU_USBF_USER_SCSI_GetHandle         This function is used to
*                                           get the address of the SCSI
*                                           user driver.
*       NU_USBF_USER_SCSI_Get_Class_Handl   This function returns device
*                                           control block.
*       _NU_USBF_USER_SCSI_Get_Max_Lun      Process gets maximum logical
*                                           unit command.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/
/* USB Include Files. */
#include "connectivity/nu_usb.h"
#include "services/reg_api.h"

UINT8  USBF_MS_Max_LUN;
NU_USBF_SCSI_MEDIA *USBF_MS_LUN_Cb_Ptrs[8];

/* External functions we need to call them to report connection and
 * disconnection functions to USB FILE driver layer.
 */

#if NU_USBF_MS_FILE
   extern STATUS _NU_USBF_NUF_DRVR_Disconnect(
                                           NU_USBF_SCSI_MEDIA *scsi_media);

   extern STATUS _NU_USBF_NUF_DRVR_Connect(NU_USBF_SCSI_MEDIA *scsi_media);
#endif

/**************************************************************************
* FUNCTION
*
*        NU_USBF_USER_SCSI_Create
*
* DESCRIPTION
*
*        Initializes the SCSI Media container.
*
* INPUTS
*
*        pcb_user_scsi      SCSI Media container control block.
*        p_name             The container name.
*        num_scsi_disks     This is unused parameter.
*                           NU_USBF_USER_SCSI_NUM_DISKS define is now used.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful initialization.
*        NU_USB_INVLD_ARG   Indicates invalid parameters.
*        NU_NOT_PRESENT     Indicates a configuration problem because
*                           of which no more USB objects could be created.
*
**************************************************************************/
STATUS NU_USBF_USER_SCSI_Create (NU_USBF_USER_SCSI *pcb_user_scsi,
                                 CHAR              *p_name,
                                 UINT8              num_scsi_disks)
{

    STATUS status;
    UINT8  num_cmds;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;

    if((!pcb_user_scsi) || (!p_name))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        if(status == NU_SUCCESS)
        {
            /* Get number of commands. */
            num_cmds = sizeof (nu_usbf_user_scsi_command_table) /
                       sizeof (NU_USBF_USER_SCSI_CMD);

            /* Call SCSI user create functionality. */
            status  = _NU_USBF_USER_SCSI_Create (
                                             pcb_user_scsi,
                                             p_name,
                                             num_scsi_disks,
                                             nu_usbf_user_scsi_command_table,
                                             num_cmds,
                                             (VOID *)&usbf_user_scsi_dispatch);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_USER_SCSI_Create
*
* DESCRIPTION
*
*        Initializes the SCSI Media container.
*
* INPUTS
*
*        pcb_user_scsi      SCSI Media container control block.
*        p_name             The container name.
*        num_scsi_disks     This is unused parameter.
*                           NU_USBF_USER_SCSI_NUM_DISKS define is now used.
*        pcb_cmd_list       Command list supported by the container.
*        num_cmds           Number of commands supported.
*        p_dispatch         Pointer to the dispatch table.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful initialization.
*        NU_USB_INVLD_ARG   Indicates invalid parameters.
*        NU_NOT_PRESENT     Indicates a configuration problem because of
*                           which no more USB objects could be created.
*
**************************************************************************/

STATUS _NU_USBF_USER_SCSI_Create (NU_USBF_USER_SCSI     *pcb_user_scsi,
                                  CHAR                  *p_name,
                                  UINT8                  num_scsi_disks,
                                  NU_USBF_USER_SCSI_CMD *pcb_cmd_list,
                                  UINT8                  num_cmds,
                                  const VOID            *p_dispatch)
{
    /* Local variables. */
    STATUS status;
    INT    disk_index;

    status = NU_SUCCESS;

    if((!pcb_user_scsi) || (!p_name) || (!pcb_cmd_list) || (!p_dispatch))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        if(status == NU_SUCCESS)
        {
            status = _NU_USBF_USER_MS_Create (&pcb_user_scsi->parent,
                                             p_name,
                                             (UINT8)USB_SPEC_MSF_SCSI_SUBCLASS,
                                             num_scsi_disks,
                                             p_dispatch);
            if (status == NU_SUCCESS)
            {
               /* Initialize SCSI user control block variables. */
                pcb_user_scsi->num_disks         = 0;
                pcb_user_scsi->scsi_command_list = pcb_cmd_list;
                pcb_user_scsi->num_cmds          = num_cmds;
                pcb_user_scsi->handle            = NU_NULL;

                for (disk_index = 0;
                     disk_index < NU_USBF_USER_SCSI_NUM_DISKS; disk_index++)
                {
                    /* Reset media and handles. */
                    pcb_user_scsi->disks[disk_index].media  = NU_NULL;
                    pcb_user_scsi->disks[disk_index].handle = NU_NULL;
                }
            }
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_USER_SCSI_Delete
*
* DESCRIPTION
*
*        Un-initializes the SCSI media container.
*
* INPUTS
*
*        pcb             Pointer to the control block to be uninitialized.
*
* OUTPUTS
*
*        NU_SUCCESS       Indicates successful initialization.
*        NU_USB_INVLD_ARG Invalid arguments.
*
**************************************************************************/
STATUS _NU_USBF_USER_SCSI_Delete (VOID *pcb)
{
    STATUS status = NU_USB_INVLD_ARG;

    if(pcb)
    {
        /* Call parent Mass Storage user delete function. */
        status = _NU_USBF_USER_MS_Delete (pcb);
        memset(pcb, 0, sizeof(NU_USBF_USER_SCSI));
    }
    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_USER_SCSI_Delete
*
* DESCRIPTION
*
*        Un-initializes the SCSI media container.
*
* INPUTS
*
*        pcb             Pointer to the control block to be uninitialized.
*
* OUTPUTS
*
*        NU_SUCCESS       Indicates successful initialization.
*        NU_USB_INVLD_ARG Invalid arguments.
*
**************************************************************************/
STATUS NU_USBF_USER_SCSI_Delete (VOID *pcb)
{
    STATUS status = NU_USB_INVLD_ARG;
    UINT8  disk_index;
    NU_USBF_USER_SCSI *scsi_ptr = (NU_USBF_USER_SCSI *)pcb;

    if(pcb)
    {
        /* If there is any media registered with the stack, de-register
         * them first.
         */
        for( disk_index = 0; disk_index < scsi_ptr->num_disks; disk_index++)
        {
            status = NU_USBF_USER_SCSI_Dereg_Media (
                                       scsi_ptr,
                                       scsi_ptr->disks[disk_index].media);
        }

        /* Call parent Mass Storage user delete function. */
        status = _NU_USBF_USER_MS_Delete (pcb);
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_USER_SCSI_Reg_Media
*
* DESCRIPTION
*
*        Registers SCSI media with this container. The media exports
*        functions that process individual commands.
*
* INPUTS
*
*        pcb_user_scsi   SCSI Container control block.
*        p_media         Media to be registered with the container.
*
* OUTPUTS
*
*        NU_SUCCESS              Indicates successful processing of this
*                                event.
*        NU_USB_NOT_SUPPORTED    Indicates that the media doesn't support
*                                this event.
*        NU_USB_INVLD_ARG        Indicates an invalid parameter.
*
**************************************************************************/
STATUS NU_USBF_USER_SCSI_Reg_Media (NU_USBF_USER_SCSI  *pcb_user_scsi,
                                    NU_USBF_SCSI_MEDIA *p_media)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;

    /* Validate SCSI user control block. */
    if ((pcb_user_scsi != NU_NULL) && (p_media != NU_NULL)
         && (pcb_user_scsi->num_cmds != 0x00))
    {
        /* Does the function for deregister media exist in user SCSI
         * dispatch table?
         */
        if (((NU_USBF_USER_SCSI_DISPATCH *)
           (((NU_USB *) pcb_user_scsi)->usb_dispatch))->
           Register_Media != NU_NULL)
        {

            /* Call user SCSI dispatch Deregister media routine. */
            status = ((NU_USBF_USER_SCSI_DISPATCH *)
                     (((NU_USB *) pcb_user_scsi)->usb_dispatch))->
                     Register_Media (pcb_user_scsi, p_media);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_USER_SCSI_Dereg_Media
*
* DESCRIPTION
*
*        Deregisters SCSI media with this container.
*
* INPUTS
*
*        pcb_user_scsi   SCSI Container control block.
*        p_media         Media to be deregistered with the container.
*
* OUTPUTS
*
*        NU_SUCCESS              Indicates successful processing of this
*                                event.
*        NU_USB_NOT_SUPPORTED    Indicates that the media doesn't support
*                                this event.
*        NU_USB_INVLD_ARG        Indicates an invalid parameter.
*
**************************************************************************/
STATUS NU_USBF_USER_SCSI_Dereg_Media (
                                         NU_USBF_USER_SCSI  *pcb_user_scsi,
                                         NU_USBF_SCSI_MEDIA *p_media)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;

    /* Validate SCSI user control block. */
    if ((pcb_user_scsi != NU_NULL) && (p_media != NU_NULL))
    {
        /* Does the function for deregister media exists in user SCSI
         * dispatch table?
         */
        if (((NU_USBF_USER_SCSI_DISPATCH *)
           (((NU_USB *) pcb_user_scsi)->usb_dispatch))->
           Deregister_Media != NU_NULL)
        {
            /* Call user SCSI dispatch Deregister media routine. */
            status = ((NU_USBF_USER_SCSI_DISPATCH *)
                     (((NU_USB *) pcb_user_scsi)->usb_dispatch))->
                     Deregister_Media (pcb_user_scsi, p_media);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_USER_SCSI_Connect
*
* DESCRIPTION
*
*        This function processes the Host connect events for the media
*        device. It locates an empty handle for the media and invokes a
*        connect callback to the corresponding media driver.
*
* INPUTS
*
*        pcb_user             Scsi media container control block.
*        pcb_drvr             Mass storage class driver control block.
*        p_handle             Unique identification for the storage device.
*
* OUTPUTS
*
*        NU_SUCCESS           Indicates successful processing of the event.
*        NU_USB_INVLD_ARG     Indicates invalid parameter.
*        NU_USB_NOT_SUPPORTED Indicates that the media doesn't support
*                             this event.
*
**************************************************************************/
STATUS _NU_USBF_USER_SCSI_Connect (NU_USB_USER *pcb_user,
                                   NU_USB_DRVR *pcb_drvr,
                                   VOID        *p_handle)
{

    /* Local variables. */
    INT    disk_index;
    STATUS status = NU_USB_INVLD_ARG;
    VOID *temp_ptr;
    /* Connect to a SCSI device. Start a handle / disk. */
    NU_USBF_USER_SCSI *scsi_ptr = (NU_USBF_USER_SCSI *) pcb_user;
    temp_ptr = scsi_ptr->handle; 
    scsi_ptr->handle = p_handle;

    /* Look out for an empty handle. */
    for (disk_index = 0; disk_index < scsi_ptr->num_disks; disk_index++)
    {
        if (scsi_ptr->disks[disk_index].handle == NU_NULL ||
            scsi_ptr->disks[disk_index].handle == p_handle)
        {
            /* Empty handle found. */
            scsi_ptr->disks[disk_index].handle = p_handle;
            scsi_ptr->disks[disk_index].drvr   = pcb_drvr;

            /* Give a connect callback to media driver. */
            status = NU_USBF_SCSI_MEDIA_Connect (
                                        scsi_ptr->disks[disk_index].media,
                                        scsi_ptr);

            if (status != NU_SUCCESS)
            {
                /* On failure reset handle. */
                scsi_ptr->disks[disk_index].handle = NU_NULL;
                scsi_ptr->handle = NU_NULL;
            }
        }
    }

    if (status != NU_SUCCESS)
    {
        scsi_ptr->handle = temp_ptr;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_USER_SCSI_Disconnect
*
* DESCRIPTION
*
*        This function processes the Host disconnect events for the media
*        device. It locates the handle for the media and invokes the
*        disconnect callback to the corresponding media driver.
*
* INPUTS
*
*        pcb_user             Scsi media container control block.
*        pcb_drvr             Mass storage class driver control block.
*        p_handle             Unique identification for the storage device.
*
* OUTPUTS
*
*        NU_SUCCESS           Indicates successful processing of the event.
*        NU_USB_INVLD_ARG     Indicates invalid parameter.
*        NU_USB_NOT_SUPPORTED Indicates that the media doesn't support
*                             this event.
*
**************************************************************************/
STATUS _NU_USBF_USER_SCSI_Disconnect (NU_USB_USER *pcb_user,
                                      NU_USB_DRVR *pcb_drvr,
                                      VOID        *p_handle)
{

    /* Local variables. */
    STATUS status = NU_USB_INVLD_ARG;

    /* Disconnect to a SCSI device. Close a handle / disk. */
    NU_USBF_USER_SCSI *scsi_ptr   = (NU_USBF_USER_SCSI *) pcb_user;
    INT                disk_index;

    if(scsi_ptr->handle == p_handle)
    {
        /* Look out for the handle. */
        for (disk_index = 0; disk_index < scsi_ptr->num_disks;
                             disk_index++)
        {
            scsi_ptr->disks[disk_index].handle = NU_NULL;
            scsi_ptr->disks[disk_index].drvr   = NU_NULL;

            /* Give a disconnect callback to media. */
            status = NU_USBF_SCSI_MEDIA_Disconnect (
                                         scsi_ptr->disks[disk_index].media,
                                         scsi_ptr);
        }

        scsi_ptr->handle = NU_NULL;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_USER_SCSI_Reset
*
* DESCRIPTION
*
*        This function resets the media device. It locates the handle for
*        the media and invokes the reset callback to the corresponding
*        media driver.
*
* INPUTS
*
*        pcb_user            Scsi media container control block.
*        pcb_drvr            Mass storage class driver control block.
*        p_handle            Unique identification for the storage device.
*
* OUTPUTS
*
*        NU_SUCCESS          Indicates successful processing of the event.
*        NU_USB_INVLD_ARG    Indicates invalid parameter.
*
**************************************************************************/
STATUS _NU_USBF_USER_SCSI_Reset (NU_USB_USER *pcb_user,
                                 const NU_USB_DRVR *pcb_drvr,
                                 const VOID        *p_handle)
{

    /* Reset to a SCSI device. Reset a handle / disk. */
    NU_USBF_USER_SCSI *scsi_ptr   = (NU_USBF_USER_SCSI *) pcb_user;
    STATUS             status = NU_USB_INVLD_ARG;
    INT                disk_index;

    if(scsi_ptr->handle == p_handle)
    {
        /* Look out for the handle. */
        for (disk_index = 0; disk_index < scsi_ptr->num_disks;
                             disk_index++)
        {

            /* Give a reset callback to media. */
            status = NU_USBF_SCSI_MEDIA_Reset_Device (
                                         scsi_ptr->disks[disk_index].media,
                                         scsi_ptr);
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_USER_SCSI_Get_Max_Lun
*
* DESCRIPTION
*
*        This function processes get maximum logical unit command.
*
* INPUTS
*
*        pcb_user            Scsi media container control block.
*        pcb_drvr            Mass storage class driver control block.
*        p_handle            Unique identification for the storage device.
*        lun_out             Retrieve logical unit number.
*
* OUTPUTS
*
*        NU_SUCCESS          Indicates successful processing of the event.
*        NU_USB_INVLD_ARG    Indicates invalid parameter.
*
**************************************************************************/
STATUS _NU_USBF_USER_SCSI_Get_Max_Lun (
                                 const NU_USB_USER *pcb_user,
                                 const NU_USB_DRVR *pcb_drvr,
                                 const VOID        *p_handle,
                                 UINT8             *p_lun_out)
{
    STATUS status = NU_USB_INVLD_ARG;

    /* Get SCSI user driver pointer */
    NU_USBF_USER_SCSI *scsi_ptr = (NU_USBF_USER_SCSI *) pcb_user;

    /* Check to see if media is registered against every logical unit.  */
    if(scsi_ptr->num_disks > 0)
    {
         /* '0' is considered as first LUN. */
        *p_lun_out = scsi_ptr->num_disks - 1;
        status = NU_SUCCESS;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_USER_SCSI_Command
*
* DESCRIPTION
*
*        This function processes the new SCSI command, sent by the Host. It
*        locates the media corresponding the command, deciphers the command
*        block and invokes the appropriate worker function for processing
*        the SCSI command.
*
* INPUTS
*
*        pcb_user             Scsi media container control block.
*        pcb_drvr             Mass storage class driver control block.
*        p_handle             Unique identification for the storage device.
*        p_cmd                Scsi command block byte buffer.
*        cmd_len              Length of the command block.
*        pp_buf_out           Memory location to keep the data pointer, in
*                             the event of a  transfer associated with the
*                             command. If no data transfer is associated,
*                             then, a, NU_NULL must be placed in this
*                             location.
*        data_length          length of the data to be transferred.
*
* OUTPUTS
*
*        NU_SUCCESS           Indicates successful processing of the event.
*        NU_USB_INVLD_ARG     Indicates invalid parameter.
*
**************************************************************************/
STATUS _NU_USBF_USER_SCSI_Command (NU_USB_USER  *pcb_user,
                                   NU_USB_DRVR  *pcb_drvr,
                                   VOID         *p_handle,
                                   UINT8        *p_cmd,
                                   UINT16        cmd_len,
                                   UINT8       **pp_buf_out,
                                   UINT32       *data_length)
{
    /* Local variables. */
    STATUS                status = NU_SUCCESS;
    UINT8                 opcode;
    USBF_SCSI_CMD_WORKER  handler;
    NU_USBF_SCSI_MEDIA   *p_media;
    UINT8                 disk_index, cmd_index;
    NU_USBF_USER_SCSI_CMD *cmd_list;

    /* New SCSI command to a SCSI device */
    NU_USBF_USER_SCSI *scsi_ptr = (NU_USBF_USER_SCSI *) pcb_user;

    disk_index = 0xFF;

    /* Look out for the handle */
    if (scsi_ptr->handle != p_handle)
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        if (status == NU_SUCCESS)
        {
            /* Get LUN. */
            disk_index = scsi_ptr->curr_comand_lun =
                                          (p_cmd[USB_SPEC_MSF_CMD_LUN_BYTE] >>
                                           NU_USBF_MS_SHIFT_FIVE_BITS);

            /* Check validity of LUN. */
            if(disk_index >= NU_USBF_USER_SCSI_NUM_DISKS)
            {
                status = NU_USB_INVLD_ARG;
            }
        }

        if (status == NU_SUCCESS)
        {
            /* Bring media and opcode in local variables. */
            p_media  = scsi_ptr->disks[disk_index].media;
            opcode   = (UINT16)*p_cmd;

            /* Bring supported command list in local variable. */
            cmd_list = scsi_ptr->scsi_command_list;

            /* In all supported SCSI command list, look-out for a matching
             * command. To start with, we have a default handler.
             */
            handler = NU_USBF_SCSI_MEDIA_Unknown_Cmd;

            for (cmd_index = 0; cmd_index < scsi_ptr->num_cmds; cmd_index++)
            {
                if (cmd_list[cmd_index].opcode == opcode)
                {
                    /* command found, get the handler. */
                    handler = cmd_list[cmd_index].handler;

                    /* Set command direction as data IN/OUT. */
                    status = NU_USBF_MS_Set_Command_Direction(p_handle,
                              (UINT8)(cmd_list[cmd_index].direction));
                    break;
                }
            }

            /* Invoke the command handler. */
            status = handler(p_media, scsi_ptr, p_cmd,
                              cmd_len, pp_buf_out,
                              data_length);
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_USER_SCSI_Transfer
*
* DESCRIPTION
*
*        Processes a new transfer request from the Host to a media. If any
*        of the data is to be transferred to/from the Host, then the
*        corresponding data pointer and the length will be set in the
*        parameters. If no data transfer is required then the memory
*        location pointed to by the 'pp_buf_out' parameter must be filled
*        with NU_NULL.
*
* INPUTS
*
*        pcb_user          Scsi media container control block.
*        pcb_drvr          Mass storage class driver control block.
*        p_handle          Unique identification for the storage device.
*        pp_buf_out        Memory location to keep the data pointer, in the
*                          event of a  transfer associated with the command
*                          If no data transfer is associated, then, a
*                          NU_NULL must be placed in this location.
*        data_length       Length of the data to be transferred.
*
* OUTPUTS
*
*        NU_SUCCESS              Indicates successful processing of this
*                                event.
*        NU_USB_NOT_SUPPORTED    Indicates that the media doesn't support
*                                this event.
*        NU_USB_INVLD_ARG        Indicates an invalid parameter.
*
**************************************************************************/

STATUS _NU_USBF_USER_SCSI_Transfer (NU_USB_USER  *pcb_user,
                                    NU_USB_DRVR  *pcb_drvr,
                                    VOID         *p_handle,
                                    UINT8       **pp_buf_out,
                                    UINT32       *data_length)
{

    /* Reset to a SCSI device. Reset a handle / disk. */
    NU_USBF_USER_SCSI *scsi_ptr = (NU_USBF_USER_SCSI *) pcb_user;
    STATUS             status   = NU_USB_INVLD_ARG;
    UINT8              disk_index;

    /* Look out for the handle. */
    if (scsi_ptr->handle == p_handle)
    {
        disk_index = scsi_ptr->curr_comand_lun;

        /* Give a reset callback to media. */
        status = NU_USBF_SCSI_MEDIA_New_Transfer (
                                         scsi_ptr->disks[disk_index].media,
                                         scsi_ptr,
                                         pp_buf_out,
                                         data_length);
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_USER_SCSI_Tx_Done
*
* DESCRIPTION
*
*        This function processes a transfer completion notification. It
*        gets, the data pointer and the length associated with the previous
*        transfer, as parameters. If any of the data is to be transferred
*        to/from the Host, then the corresponding data pointer and the
*        length will be set in the parameters. If no data transfer is
*        required then the memory location pointed to by the 'pp_buf_out'
*        parameter must be filled with NU_NULL.
*
* INPUTS
*
*        pcb_user           Scsi media container control block.
*        pcb_drvr           Mass storage class driver control block.
*        p_handle           unique identification for the storage device.
*        p_buf              Data pointer associated with previous transfer.
*        buf_length         Length of data transferred.
*        pp_buf_out         Memory location to keep the data pointer, in
*                           the event of a  transfer associated with the
*                           command. If no data transfer is associated,
*                           then, a NU_NULL must be placed in this
*                           location.
*        data_length        Length of the data to be transferred.
*
* OUTPUTS
*
*        NU_SUCCESS              Indicates successful processing of this
*                                event.
*        NU_USB_NOT_SUPPORTED    Indicates that the media doesn't support
*                                this event.
*        NU_USB_INVLD_ARG        Indicates an invalid parameter.
*
**************************************************************************/
STATUS _NU_USBF_USER_SCSI_Tx_Done (NU_USB_USER  *pcb_user,
                                   NU_USB_DRVR  *pcb_drvr,
                                   VOID         *p_handle,
                                   UINT8        *p_buf,
                                   UINT32        buf_length,
                                   UINT8       **pp_buf_out,
                                   UINT32       *data_length)
{

    /* Local variables. */
    NU_USBF_USER_SCSI *scsi_ptr = (NU_USBF_USER_SCSI *) pcb_user;
    STATUS             status   = NU_USB_INVLD_ARG;
    UINT8              disk_index;

    /* Look out for the handle. */
    if (scsi_ptr->handle == p_handle)
    {
         disk_index = scsi_ptr->curr_comand_lun;

         /* Give a transfer complete callback to media. */
         status = NU_USBF_SCSI_MEDIA_Tx_Done (
                                        scsi_ptr->disks[disk_index].media,
                                        scsi_ptr,
                                        p_buf,
                                        buf_length,
                                        pp_buf_out,
                                        data_length);
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_USER_SCSI_Notify
*
* DESCRIPTION
*
*        USB event notification processing.
*
* INPUTS
*
*        pcb_user            Scsi media container control block.
*        pcb_drvr            Mass storage class driver control block.
*        p_handle            unique identification for the storage device.
*        event               The USB event occurred.
*
* OUTPUTS
*
*        NU_SUCCESS          Indicates successful processing of the event.
*
**************************************************************************/
STATUS _NU_USBF_USER_SCSI_Notify (NU_USB_USER *pcb_user,
                                  NU_USB_DRVR *pcb_drvr,
                                  VOID        *p_handle,
                                  UINT32       event)
{

    /* Initialize status with success. */
    STATUS status = NU_SUCCESS;

/* If we are using Mass Storage class Driver with Nucleus FILE, we need to
 * report connection and disconnection events to USB FILE drivers layer.
 */
#if NU_USBF_MS_FILE

#ifdef  NU_USBF_MS_TASK_MODE

    NU_USBF_MS_DEVICE *ms_device_ptr = (NU_USBF_MS_DEVICE *)p_handle;

#endif

    /* Which event occurred?*/
    switch(event)
    {
        /* Mass Storage Device gets disconnected. */
        case  USBF_EVENT_DISCONNECT:

          /* Call Nucleus USB FILE Driver disconnect behavior. */
          _NU_USBF_NUF_DRVR_Disconnect((NU_USBF_SCSI_MEDIA*)pcb_user);
          break;

        /* Mass Storage Device gets connected. */
        case  USBF_EVENT_CONNECT:

#ifdef  NU_USBF_MS_TASK_MODE
           /* Submit IRP for CBW processing. */
           status = NU_USBF_MS_Start_Cmd_Processing (ms_device_ptr);
#endif

          if(status == NU_SUCCESS)
          {
              /* Call Nucleus USB FILE Driver connect behavior. */
              _NU_USBF_NUF_DRVR_Connect((NU_USBF_SCSI_MEDIA*)pcb_user);
              break;
          }

        /* Do nothing. */
        default:
          break;
    }

#endif

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_USER_SCSI_Reg_Media
*
* DESCRIPTION
*
*        Registers SCSI media with this container. The media exports
*        functions that process individual commands.
*
* INPUTS
*
*        pcb_user                SCSI Container control block.
*        pcb_scsi_media          Media to be registered with the container.
*
* OUTPUTS
*
*        NU_SUCCESS              Indicates that the registration is
*                                successful.
*        NU_USB_INVLD_ARG        Indicates invalid parameter.
*
**************************************************************************/
STATUS _NU_USBF_USER_SCSI_Reg_Media (
                                        NU_USBF_USER_SCSI  *pcb_user_scsi,
                                        NU_USBF_SCSI_MEDIA *pcb_scsi_media)
{

    /* Local variables. */
    STATUS             status   = NU_SUCCESS;
    NU_USBF_USER_SCSI *scsi_ptr = (NU_USBF_USER_SCSI *)pcb_user_scsi;
    UINT8              disk_index;

    if (scsi_ptr->num_disks == NU_USBF_USER_SCSI_NUM_DISKS ||
        scsi_ptr->scsi_command_list == NU_NULL)
    {
        /* Array size is limited up to the number of registered medias. */
        /* Command list must be non null for valid user scsi drivers. */
        status = NU_USB_INVLD_ARG;
    }

    if(status == NU_SUCCESS)
    {

        for( disk_index = 0; disk_index < scsi_ptr->num_disks;
                                disk_index++)
        {
            if(scsi_ptr->disks[disk_index].media == pcb_scsi_media)
            {
                /* Multiple registration of the same media is not allowed. */
                status = NU_USB_INVLD_ARG;
            }
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Save media control block. */
        scsi_ptr->disks[scsi_ptr->num_disks].media  = pcb_scsi_media;

        /* Rest of the entries will be filled on device connection. */
        scsi_ptr->disks[scsi_ptr->num_disks].handle = NU_NULL;
        scsi_ptr->disks[scsi_ptr->num_disks++].drvr = NU_NULL;
        pcb_scsi_media->scsi_user = scsi_ptr;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_USER_SCSI_Dereg_Media
*
* DESCRIPTION
*
*        Deregisters SCSI media with this container.
*
* INPUTS
*
*        pcb_user_scsi         SCSI Container control block.
*        pcb_scsi_media        Media to be deregistered with the contain.
*
* OUTPUTS
*
*        NU_SUCCESS            Indicates that the deregistration is
*                              successful.
*        NU_USB_INVLD_ARG      Indicates invalid parameter.
*
**************************************************************************/
STATUS _NU_USBF_USER_SCSI_Dereg_Media (
                                  NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const NU_USBF_SCSI_MEDIA *pcb_scsi_media)
{
    /* Local variables. */
    STATUS             status   = NU_USB_INVLD_ARG;
    NU_USBF_USER_SCSI *scsi_ptr = (NU_USBF_USER_SCSI *)pcb_user_scsi;
    INT                disk_index;

    for (disk_index = 0; disk_index < scsi_ptr->num_disks; disk_index++)
    {
        if (scsi_ptr->disks[disk_index].media == pcb_scsi_media)
        {
            /* Set status as success. */
            status                      = NU_SUCCESS;

            /* Deregister the media. */
            scsi_ptr->disks[disk_index] =
                                    scsi_ptr->disks[--scsi_ptr->num_disks];

            break;
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_USER_SCSI_Get_Class_Handl
*
* DESCRIPTION
*
*        This function returns pointer to the device control block
*        structure.
*
* INPUTS
*
*        pcb_user_scsi          Pointer SCSI user control block.
*        pcb_scsi_media         Pointer to the SCSI media control block.
*        pcb_ms_device          Device return pointer.
*
* OUTPUTS
*
*        NU_SUCCESS             Indicates successful completion.
*        NU_NOT_PRESENT         Device handle not found.
*        NU_USB_INVLD_ARG       Indicates invalid parameter.
*
**************************************************************************/
STATUS NU_USBF_USER_SCSI_Get_Class_Handl(
                                 const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                 const NU_USBF_SCSI_MEDIA *pcb_scsi_media,
                                 VOID                    **pcb_ms_device)
{

    UINT8  disk_index;

    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;

    if((pcb_user_scsi) && (pcb_scsi_media) && (pcb_ms_device))
    {
        status = NU_NOT_PRESENT;

        /* Search for the class driver handle for media. */
        for (disk_index = 0; disk_index < pcb_user_scsi->num_disks;
                                        disk_index++)
        {
            /* Validate media pointer. */
            if (pcb_user_scsi->disks[disk_index].media == pcb_scsi_media)
            {
                /* Media is the registered one. Get device control block. */
                (*pcb_ms_device) = pcb_user_scsi->handle;
                status = NU_SUCCESS;
                break;
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_USER_SCSI_Init
*
*   DESCRIPTION
*
*       This function initializes the SCSI User driver component.
*
*   INPUTS
*
*       path                                Registry path of component.
*       startstop                           Flag to find if component is
*                                           being enabled or disabled.
*
*   OUTPUTS
*
*       status             Success or failure of the creation of the
*                          underlying initialization routines.
*
*************************************************************************/
STATUS nu_os_conn_usb_func_ms_user_init(CHAR *path, INT startstop)
{
    VOID   *usbf_ms_handle = NU_NULL;
    UINT8   rollback = 0, index;
    STATUS  status, internal_sts = NU_SUCCESS;
    CHAR    usb_func_ms_user_path[80];

    usb_func_ms_user_path[0] = '\0';
    strcat(usb_func_ms_user_path, path);
    /* Find max LUN from registry value. */
    strcat(usb_func_ms_user_path, "/max_lun");

    REG_Get_UINT8(usb_func_ms_user_path, &USBF_MS_Max_LUN);

    if(startstop)
    {
        /* Allocate memory for SCSI user driver control block. */
        status = USB_Allocate_Object(sizeof(NU_USBF_USER_SCSI),
        		                     (VOID **)&NU_USBF_USER_SCSI_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 1;
        }

        /* Create the device subsystem. */
        if (!rollback)
        {
            /* Zero out allocated block. */
            memset(NU_USBF_USER_SCSI_Cb_Pt, 0, sizeof(NU_USBF_USER_SCSI));
            status = NU_USBF_USER_SCSI_Create (NU_USBF_USER_SCSI_Cb_Pt,
                                               "scsi",0);
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        /*  Get the function mass storage class driver handle. */
        if (!rollback)
        {
            status = NU_USBF_MS_Init_GetHandle(&usbf_ms_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        /* Register the user driver to the class driver. */
        if (!rollback)
        {
            status = NU_USB_DRVR_Register_User ((NU_USB_DRVR *) usbf_ms_handle,
                                                (NU_USB_USER *)
                                                NU_USBF_USER_SCSI_Cb_Pt);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        if(!rollback)
        {
            for(index = 0; index < USBF_MS_Max_LUN; index++)
            {
                /* Allocate memory for LUN control block. */
                status = USB_Allocate_Object(sizeof(NU_USBF_SCSI_MEDIA),
                							 (VOID **)&USBF_MS_LUN_Cb_Ptrs[index]);

                if(status == NU_SUCCESS)
                {
                    status = _NU_USBF_SCSI_MEDIA_Create (USBF_MS_LUN_Cb_Ptrs[index],
                                                         "DISK",
                                                         NU_NULL,
                                                         NU_NULL,
                                                         NU_FALSE,
                                                         &Scsi_Ram_Dispatch);
                    if(status == NU_SUCCESS)
                    {
                        /* Register media with the user. */
                        status = NU_USBF_USER_SCSI_Reg_Media (NU_USBF_USER_SCSI_Cb_Pt,
                                             (NU_USBF_SCSI_MEDIA *) USBF_MS_LUN_Cb_Ptrs[index]);
                    }
                }
            }
        }

        /* Clean up in case error occurs. */
        switch (rollback)
        {
            case 3:
                internal_sts = NU_USBF_USER_SCSI_Delete ((VOID *) NU_USBF_USER_SCSI_Cb_Pt);

            case 2:
                if (NU_USBF_USER_SCSI_Cb_Pt)
                {
                    internal_sts |= USB_Deallocate_Memory((VOID *)NU_USBF_USER_SCSI_Cb_Pt);
                    NU_USBF_USER_SCSI_Cb_Pt = NU_NULL;
                }

            case 1:
            case 0:
            /* internal_sts is not used after this. So to remove 
             * KW and PC-Lint warning set it as unused parameter. 
             */
            NU_UNUSED_PARAM(internal_sts);
        }
    }
    else
    {
        if (NU_USBF_USER_SCSI_Cb_Pt)
        {
            for(index = 0; index < USBF_MS_Max_LUN; index++)
            {
                NU_USBF_USER_SCSI_Dereg_Media (
                                             NU_USBF_USER_SCSI_Cb_Pt,
                                             (NU_USBF_SCSI_MEDIA *) USBF_MS_LUN_Cb_Ptrs[index]);

                _NU_USBF_SCSI_MEDIA_Delete (USBF_MS_LUN_Cb_Ptrs[index]);

                if (USBF_MS_LUN_Cb_Ptrs[index])
                {
                    USB_Deallocate_Memory(USBF_MS_LUN_Cb_Ptrs[index]);
                }
            }

            NU_USBF_MS_Init_GetHandle(&usbf_ms_handle);         
            NU_USB_DRVR_Deregister_User (usbf_ms_handle,(NU_USB_USER * )NU_USBF_USER_SCSI_Cb_Pt);
            NU_USBF_USER_SCSI_Delete (NU_USBF_USER_SCSI_Cb_Pt);   
            USB_Deallocate_Memory(NU_USBF_USER_SCSI_Cb_Pt);
        }

        status = NU_SUCCESS;
    }

    return (status);
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_USER_SCSI_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the function SCSI
*       user driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the user
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates there exists a function SCSI
*                           user driver.
*       NU_NOT_PRESENT      Indicates there exists no user driver.
*
*************************************************************************/
STATUS NU_USBF_USER_SCSI_GetHandle(VOID  **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;

    if(handle)
    {
        status = NU_SUCCESS;
        *handle = NU_USBF_USER_SCSI_Cb_Pt;

        if (NU_USBF_USER_SCSI_Cb_Pt == NU_NULL)
        {
            status = NU_NOT_PRESENT;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}
/************************* End Of File ***********************************/
