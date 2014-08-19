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
*       nu_usbf_scsi_media_ext.c
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the exported functions for the Mass Storage
*       media driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBF_SCSI_MEDIA_Connect          Connected to Host.
*       NU_USBF_SCSI_MEDIA_Disconnect       Host Disconnection.
*       NU_USBF_SCSI_MEDIA_Format           FORMAT Command processing.
*       NU_USBF_SCSI_MEDIA_Inquiry          INQUIRY Command processing.
*       NU_USBF_SCSI_MEDIA_Mode_Sel6        MODE SELECT 6 Command
*                                           processing.
*       NU_USBF_SCSI_MEDIA_Mode_Sel_10      MODE SELECT 10 Command
*                                           processing.
*       NU_USBF_SCSI_MEDIA_Mode_Sense_6     MODE SENSE 6 Command
*                                           processing.
*       NU_USBF_SCSI_MEDIA_Mode_Sense_10    MODE SENSE 10 Command
*                                           processing.
*       NU_USBF_SCSI_MEDIA_New_Transfer     New Host initiated transfer.
*       NU_USBF_SCSI_MEDIA_Read             Reading data from media.
*       NU_USBF_SCSI_MEDIA_Read_10          READ 10 command processing.
*       NU_USBF_SCSI_MEDIA_Read_12          READ 12 command processing.
*       NU_USBF_SCSI_MEDIA_Read_6           READ 6 command processing.
*       NU_USBF_SCSI_MEDIA_Capacity         READ CAPACITY command
*                                           processing.
*       NU_USBF_SCSI_MEDIA_Release_Unit     RELEASE UNIT command
*                                           processing.
*       NU_USBF_SCSI_MEDIA_Sense            REQUEST SENSE command
*                                           processing.
*       NU_USBF_SCSI_MEDIA_Reserve_Unit     RESERVE UNIT command
*                                           processing.
*       NU_USBF_SCSI_MEDIA_Reset_Device     Reset the SCSI media.
*       NU_USBF_SCSI_MEDIA_Snd_Diag         SEND DIAGNOSTIC command
*                                           processing.
*       NU_USBF_SCSI_MEDIA_Ready            TEST UNIT READY command
*                                           processing.
*       NU_USBF_SCSI_MEDIA_Tx_Done          Transfer completed.
*       NU_USBF_SCSI_MEDIA_Unknown_Cmd      Unknown SCSI command
*                                           processing.
*       NU_USBF_SCSI_MEDIA_Verify           VERIFY command processing.
*       NU_USBF_SCSI_MEDIA_Write            WRITE command processing.
*       NU_USBF_SCSI_MEDIA_Write_10         WRITE 10 command processing.
*       NU_USBF_SCSI_MEDIA_Write_12         WRITE 12 command processing.
*       NU_USBF_SCSI_MEDIA_Write_6          WRITE 6 command processing.
*       NU_USBF_SCSI_MEDIA_Start_Stop       This function processes the
*                                           SCSI Start-Stop Command.
*       NU_USBF_SCSI_MEDIA_Prevent_Allow    This function processes the
*                                           SCSI PREVENT ALLOW Command.
*       _NU_USBF_SCSI_MEDIA_Create          Media initialization.
*       _NU_USBF_SCSI_MEDIA_Delete          Media Un-initialization.
*       _NU_USBF_SCSI_MEDIA_Insert          Storage Media Inserted.
*       _NU_USBF_SCSI_MEDIA_Remove          Storage Media Removed.
*       _NU_USBF_SCSI_MEDIA_Disconnect      Host Disconnection.
*       _NU_USBF_SCSI_MEDIA_Format          FORMAT Command processing.
*       _NU_USBF_SCSI_MEDIA_Inquiry         INQUIRY Command processing.
*       _NU_USBF_SCSI_MEDIA_Mode_Sel6       MODE SELECT 6 Command
*                                           processing.
*       _NU_USBF_SCSI_MEDIA_Mode_Sense6     MODE SENSE 6 Command
*                                           processing.
*       _NU_USBF_SCSI_MEDIA_Transfer        New Host initiated transfer.
*       _NU_USBF_SCSI_MEDIA_RW10            READ/WRITE 10 command
*                                           processing.
*       _NU_USBF_SCSI_MEDIA_Read_Write6     READ/WRITE 6 command
*                                           processing.
*       _NU_USBF_SCSI_MEDIA_Capacity        READ CAPACITY command
*                                           processing.
*       _NU_USBF_SCSI_MEDIA_ReleaseUnit     RELEASE UNIT command
*                                           processing.
*       _NU_USBF_SCSI_MEDIA_Sense           REQUEST SENSE command
*                                           processing.
*       _NU_USBF_SCSI_MEDIA_ReserveUnit     RESERVE UNIT command
*                                           processing.
*       _NU_USBF_SCSI_MEDIA_Snd_Diag        SEND DIAGNOSTIC command
*                                           processing.
*       _NU_USBF_SCSI_MEDIA_Ready           TEST UNIT READY command
*                                           processing.
*       _NU_USBF_SCSI_MEDIA_Tx_Done         Transfer completed.
*       _NU_USBF_SCSI_MEDIA_Command_23      READ FORMAT CAPACITIES command.
*       _NU_USBF_SCSI_MEDIA_Verify          VERIFY command processing.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/
/* USB Include Files. */
#include "connectivity/nu_usb.h"

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Create
*
* DESCRIPTION
*
*        Initializes the media from SCSI perspective.
*
* INPUTS
*
*        pcb_scsi_media     Pointer to media control block to be
*                           initialized.
*        p_name             Name of the media.
*        p_inquiry          Data bytes to be sent in response to an INQUIRY
*                           command.
*        p_capacity         Data bytes to be sent in response to an READ
*                           CAPACITY command.
*        is_formatted       If TRUE indicates that the media is already
*                           formatted.
*        p_dispatch         Pointer to the dispatch table.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates that the initialization has been
*                           successful.
*        NU_USB_NOT_PRESENT Indicates that the maximum number of control
*                           blocks that could be created in the sub system
*                           has exceeded.
*        NU_USB_INVLD_ARG   Indicates an invalid parameter.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Create (NU_USBF_SCSI_MEDIA *pcb_scsi_media,
                                   CHAR               *p_name,
                                   UINT8              *p_inquiry,
                                   UINT8              *p_capacity,
                                   BOOLEAN             is_formatted,
                                   const VOID         *p_dispatch)
{
    /* Local variables. */
    STATUS  status;
    VOID   *memret_ptr;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;

    if((!pcb_scsi_media) || (!p_name) || (!p_dispatch))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        /* Reset contents of SCSI media driver control block. */
        memset(pcb_scsi_media, 0x00, sizeof(NU_USBF_SCSI_MEDIA));

        if(status == NU_SUCCESS)
        {
            /* Invoke parents create functionality. */
            status = _NU_USB_Create ((NU_USB *) pcb_scsi_media,
                                    (NU_USB_SUBSYS *) &(nu_usbf->user_subsys),
                                    p_name,
                                    p_dispatch);
        }
        if (status == NU_SUCCESS)
        {
            /* Initialize SCSI Media control block. */
            pcb_scsi_media->inquiry_data       = p_inquiry;
            pcb_scsi_media->read_capacity_data = p_capacity;
            pcb_scsi_media->is_formatted       = is_formatted;
            pcb_scsi_media->media_present      = NU_FALSE;
            pcb_scsi_media->scsi_user          = NU_NULL;
            pcb_scsi_media->sense_key          = SCSI_SENSE_KEY_NO_SENSE;
            pcb_scsi_media->asc                = 0;
            pcb_scsi_media->asc_qual           = 0;

            /* Allocate memory for temporary data array. */
            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                        300,
                                        (VOID**)&(pcb_scsi_media->temp_data));
            if ( status == NU_SUCCESS )
            {
                /* Reset the temporary data array. */
                memret_ptr = memset (pcb_scsi_media->temp_data,
                                     0,
                                     300);

                /* If array is properly set with all zeros. */
                if (memret_ptr != pcb_scsi_media->temp_data)
                {
                    status = NU_USB_INVLD_ARG;
                }

                if (status == NU_SUCCESS)
                {
                    /* Allocate memory for mode data buffer. */
                    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                                USB_SPEC_MSF_MODE_DATA_LEN,
                                                (VOID**)&(pcb_scsi_media->mode_data));
                    if ( status == NU_SUCCESS )
                    {
                        /* Reset the mode sense data array. */
                        memret_ptr = memset (pcb_scsi_media->mode_data,
                                             0,
                                             USB_SPEC_MSF_MODE_DATA_LEN);

                        if (memret_ptr != pcb_scsi_media->mode_data)
                        {
                            status = NU_USB_INVLD_ARG;
                        }

                        if (status == NU_SUCCESS)
                        {
                            /* Allocate memory for read/write buffer. */
                            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                                        USBF_SCSI_BUFFER_SIZE,
                                                        (VOID**)&(pcb_scsi_media->data_buffer));
                            if ( status == NU_SUCCESS )
                            {
                                /* Initialize media data buffer. This buffer is used in
                                 * reading from and writing to the media.
                                 */
                                memret_ptr = memset (pcb_scsi_media->data_buffer,
                                                     0,
                                                     USBF_SCSI_BUFFER_SIZE);

                                if (memret_ptr != pcb_scsi_media->data_buffer)
                                {
                                    status = NU_USB_INVLD_ARG;
                                }

                                if (status == NU_SUCCESS)
                                {
                                    /* Initialize opcode of the previously executed
                                     * command.
                                     */
                                    pcb_scsi_media->last_command     = 0;

                                    /* Initialize remaining length with zero. */
                                    pcb_scsi_media->remaining_length = 0;

                                    /* Create read/write semaphore. */
                                    status = NU_Create_Semaphore(&pcb_scsi_media->rw_lock,
                                                                "MSFRW",
                                                                0,
                                                                NU_FIFO);
                                    if ( status == NU_SUCCESS )
                                    {
                                        status = NU_USB_SYS_Register_Device(pcb_scsi_media,
                                                                            NU_USBCOMPF_STORAGE);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Insert
*
* DESCRIPTION
*
*        Called by the application when a new media is inserted in empty
*        removable storage device.
*
* INPUTS
*
*        pcb_scsi_media     Pointer to media control block to be
*                           initialized.
* OUTPUTS
*
*        NU_SUCCESS         Indicates that the operation has been
*                           successful.

**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Insert (NU_USBF_SCSI_MEDIA *pcb_scsi_media)
{
     pcb_scsi_media->media_present      = NU_TRUE;
     pcb_scsi_media->media_state        = MEDIA_INSERTED;

     return(NU_SUCCESS);
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Remove
*
* DESCRIPTION
*
*        Called by the application when a media is removed from
*        removable storage device.
*
* INPUTS
*
*        pcb_scsi_media     Pointer to media control block to be
*                           initialized.
* OUTPUTS
*
*        NU_SUCCESS         Indicates that the operation has been
*                           successful.

**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Remove (NU_USBF_SCSI_MEDIA *pcb_scsi_media)
{
     pcb_scsi_media->media_present     = NU_FALSE;
     pcb_scsi_media->media_state       = MEDIA_ABSENT;

     return(NU_SUCCESS);
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Connect
*
* DESCRIPTION
*
*        Processes the connection event. Any hardware specific
*        initializations that are required are done here. This event is
*        used to prepare the hardware for further transactions with the
*        Host. The implementation is media specific.
*
* INPUTS
*
*        pcb_scsi_media         Media to be prepared for transactions.
*        pcb_user_scsi          The media container.
*
* OUTPUTS
*
*        NU_SUCCESS             Indicates successful processing of this
*                               event.
*        NU_USB_INVLD_ARG       Indicates an invalid parameter.
*        NU_USB_NOT_SUPPORTED   Indicates that the media doesn't support
*                               this event.
*
**************************************************************************/
STATUS NU_USBF_SCSI_MEDIA_Connect (
                                  const NU_USBF_SCSI_MEDIA *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi)
{
    /* Initialize the status as not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if ((pcb_scsi_media != NU_NULL) && (pcb_user_scsi != NU_NULL))
    {
        /* If media dispatch contains connect functionality? */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
            (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
            Connect != NU_NULL)
        {
            /* Call SCSI Media dispatch connect behavior. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Connect (pcb_scsi_media, pcb_user_scsi);
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Disconnect
*
* DESCRIPTION
*
*        Processes the disconnection event. Any hardware specific
*        Un-initializations that are required are done here.
*        The implementation is media specific.
*
* INPUTS
*
*        pcb_scsi_media         Pointer to the SCSI media control block.
*        pcb_user_scsi          Pointer to SCSI user control block.
*
* OUTPUTS
*
*        NU_SUCCESS             Indicates successful processing of this
*                               event.
*        NU_USB_INVLD_ARG       Indicates an invalid parameter.
*        NU_USB_NOT_SUPPORTED   Indicates that the media doesn't support
*                               this event.
*
**************************************************************************/
STATUS NU_USBF_SCSI_MEDIA_Disconnect (
                                  const NU_USBF_SCSI_MEDIA *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi)
{
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if ((pcb_scsi_media != NU_NULL) && (pcb_user_scsi != NULL))
    {
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Disconnect != NU_NULL)
        {

            /* Call SCSI Media dispatch disconnect behavior. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                      (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                      Disconnect (pcb_scsi_media, pcb_user_scsi);
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_New_Transfer
*
* DESCRIPTION
*
*        Processes a new transfer request from the Host. If any of the data
*        is to be transferred to/from the Host, the the corresponding data
*        pointer and the length will be set in the parameters. If no data
*        transfer is required then the memory location pointed to by the
*        'buf_out' parameter must be filled with NU_NULL.
*
* INPUTS
*
*        pcb_scsi_media         Media to which the Host requested a
*                               transfer.
*        pcb_user_scsi          The media container.
*        pp_buf_out             Memory location pointed by this variable
*                               must contain the data location, from where
*                               the data must be transferred.
*        p_data_length_out      Location where the number of bytes of data
*                               to be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS             Indicates successful processing of this
*                               event.
*        NU_USB_NOT_SUPPORTED   Indicates that the media doesn't support
*                               this event.
*        NU_USB_INVLD_ARG       Indicates an invalid parameter.
*
**************************************************************************/
STATUS NU_USBF_SCSI_MEDIA_New_Transfer (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if ((pcb_scsi_media) && (pcb_user_scsi) &&
         (pp_buf_out) && (p_data_len_out))
    {
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
            (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
            New_Transfer != NU_NULL)
        {

            /* Call SCSI media dispatch New Transfer behavior. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     New_Transfer (pcb_scsi_media,
                                   pcb_user_scsi,
                                   pp_buf_out,
                                   p_data_len_out);
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Tx_Done
*
* DESCRIPTION
*
*        This function processes a transfer completion notification. It
*        gets, the data pointer and the length associated with the previous
*        transfer, as parameters.
*        Processes a new transfer request from the Host. If any of the data
*        is to be transferred to/from the Host, the the corresponding data
*        pointer and the length will be set in the parameters. If no data
*        transfer is required then the memory location pointed to by the
*        'pp_buf_out' parameter must be filled with NU_NULL.
*
* INPUTS
*
*        pcb_scsi_media          Media to which the Host requested a
*                                transfer.
*        pcb_user_scsi           The media container.
*        completed_data          Data pointer associated with the previous
*                                transfer.
*        completed_data_length   Number of bytes transferred in previous
*                                transfer.
*        pp_buf_out              Memory location pointed by this variable
*                                must contain the data location, from where
*                                the data must be transferred.
*        p_data_len_out          Location where the number of bytes of data
*                                to be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Tx_Done (
                           NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                           const NU_USBF_USER_SCSI  *pcb_user_scsi,
                           const UINT8              *p_completed_data,
                           const UINT32              completed_data_length,
                           UINT8                   **pp_buf_out,
                           UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && pcb_user_scsi != NU_NULL
        && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        /* Check if the SCSI media dispatch is specialized for Tx Done
         * or not.
         */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Tx_Done != NU_NULL)
        {
            /* Call media dispatch behavior for Tx Done. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Tx_Done (pcb_scsi_media,
                              pcb_user_scsi,
                              p_completed_data,
                              completed_data_length,
                              pp_buf_out,
                              p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Ready
*
* DESCRIPTION
*
*        This function processes the SCSI TEST UNIT READY Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Ready (NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                 const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                 const UINT8              *p_cmd,
                                 const UINT16              cmd_len,
                                 UINT8                   **pp_buf_out,
                                 UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        /* Check to see if the ready behavior exist in SCSI media dispatch
         * table.
         */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
            (((NU_USB *) pcb_scsi_media)->usb_dispatch))->Ready != NU_NULL)
        {
            /* Call SCSI Media dispatch Ready behavior. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Ready (pcb_scsi_media,
                            pcb_user_scsi,
                            p_cmd,
                            cmd_len,
                            pp_buf_out,
                            p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Sense
*
* DESCRIPTION
*
*        This function processes the SCSI REQUEST SENSE Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Sense (NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                 const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                 const UINT8              *p_cmd,
                                 const UINT16              cmd_len,
                                 UINT8                   **pp_buf_out,
                                 UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        /* Check to see if the mode sense behavior exist in SCSI media
         * dispatch table.
         */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->Sense != NU_NULL)
        {
            /* Call SCSI Media dispatch Mode Sense behavior. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Sense (pcb_scsi_media,
                            pcb_user_scsi,
                            p_cmd,
                            cmd_len,
                            pp_buf_out,
                            p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Inquiry
*
* DESCRIPTION
*
*        This function processes the SCSI INQUIRY Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Inquiry (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Make sure SCSI media control block pointer is valid. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        /* Check to see if the inquiry behavior exist in SCSI media
         * dispatch table.
         */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Inquiry != NU_NULL)
        {
            /* Call SCSI Media dispatch Inquiry behavior. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Inquiry (pcb_scsi_media,
                              pcb_user_scsi,
                              p_cmd,
                              cmd_len,
                              pp_buf_out,
                              p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Mode_Sense_6
*
* DESCRIPTION
*
*        This function processes the SCSI MODE SENSE (6) Command.
*
* INPUTS
*
*        pcb_scsi_media      Media to which the Host sent the command.
*        pcb_user_scsi       The media container.
*        p_cmd               Byte buffer containing the command block.
*        cmd_len             Length, in bytes, of the command block.
*        pp_buf_out          Memory location pointed by this variable must
*                            contain the data location, from where the data
*                            must be transferred.
*        p_data_len_out      Location where the number of bytes of data to
*                            be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Mode_Sense_6 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* If the SCSI Media pointer is valid one? */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        /* If mode sense6 functionality exist in SCSI media dispatch table.
         */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Mode_Sense_6 != NU_NULL)
        {
            /* Call SCSI Media dispatch Mode Sense6 functionality. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Mode_Sense_6 (pcb_scsi_media,
                                   pcb_user_scsi,
                                   p_cmd,
                                   cmd_len,
                                   pp_buf_out,
                                   p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Mode_Sense_10
*
* DESCRIPTION
*
*        This function processes the SCSI MODE SENSE (10) Command.
*
* INPUTS
*
*        pcb_scsi_media      Media to which the Host sent the command.
*        pcb_user_scsi       The media container.
*        p_cmd               Byte buffer containing the command block.
*        cmd_len             Length, in bytes, of the command block.
*        pp_buf_out          Memory location pointed by this variable must
*                            contain the data location, from where the data
*                            must be transferred.
*        p_data_len_out      Location where the number of bytes of data to
*                            be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Mode_Sense_10 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* If the SCSI Media pointer is valid one? */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        /* If mode sense6 functionality exist in SCSI media dispatch table.
         */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Mode_Sense_10 != NU_NULL)
        {
            /* Call SCSI Media dispatch Mode Sense10 functionality. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Mode_Sense_10 (pcb_scsi_media,
                                   pcb_user_scsi,
                                   p_cmd,
                                   cmd_len,
                                   pp_buf_out,
                                   p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Mode_Sel6
*
* DESCRIPTION
*
*        This function processes the SCSI MODE SELECT (6) Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS              Indicates successful processing of this
*                                event.
*        NU_USB_NOT_SUPPORTED    Indicates that the media doesn't support
*                                this event.
*        NU_USB_INVLD_ARG        Indicates an invalid parameter.
*
*
**************************************************************************/
STATUS NU_USBF_SCSI_MEDIA_Mode_Sel6 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        /* Check to see if the mode select behavior exist in SCSI media
         * dispatch table.
         */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Mode_Sel6 != NU_NULL)
        {

            /* Call SCSI Media dispatch Mode Select6 functionality. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                      Mode_Sel6 (pcb_scsi_media,
                                 pcb_user_scsi,
                                 p_cmd,
                                 cmd_len,
                                 pp_buf_out,
                                 p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Mode_Sel_10
*
* DESCRIPTION
*
*        This function processes the SCSI MODE SELECT (10) Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS              Indicates successful processing of this
*                                event.
*        NU_USB_NOT_SUPPORTED    Indicates that the media doesn't support
*                                this event.
*        NU_USB_INVLD_ARG        Indicates an invalid parameter.
*
*
**************************************************************************/
STATUS NU_USBF_SCSI_MEDIA_Mode_Sel_10 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        /* Check to see if the mode select behavior exist in SCSI media
         * dispatch table.
         */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Mode_Sel_10 != NU_NULL)
        {

            /* Call SCSI Media dispatch Mode Select10 functionality. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                      Mode_Sel_10 (pcb_scsi_media,
                                 pcb_user_scsi,
                                 p_cmd,
                                 cmd_len,
                                 pp_buf_out,
                                 p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Snd_Diag
*
* DESCRIPTION
*
*        This function processes the SCSI SEND DIAGNOSTIC Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Snd_Diag (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {

        /* If media dispatch entry for send diagnostic function is valid?*/
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Snd_Diag != NU_NULL)
        {

            /* Call SCSI Media dispatch functionality for send diagnostic
             * command.
             */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Snd_Diag (pcb_scsi_media,
                     pcb_user_scsi,
                     p_cmd, cmd_len,
                     pp_buf_out,
                     p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Format
*
* DESCRIPTION
*
*        This function processes the SCSI FORMAT Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Format (NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL )
    {

        /* If media dispatch entry for format function is valid? */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Format != NU_NULL)
        {

            /* Call SCSI Media dispatch functionality for SCSI FORMAT
             * command.
             */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Format (pcb_scsi_media,
                             pcb_user_scsi,
                             p_cmd,
                             cmd_len,
                             pp_buf_out,
                             p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Reserve_Unit
*
* DESCRIPTION
*
*        This function processes the SCSI RESERVE UNIT Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Reserve_Unit (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {

        /* If media dispatch entry for reserve unit function is valid? */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Reserve_Unit != NU_NULL)
        {

            /* Call SCSI Media dispatch Reserve Unit functionality. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Reserve_Unit (pcb_scsi_media,
                                   pcb_user_scsi,
                                   p_cmd,
                                   cmd_len,
                                   pp_buf_out,
                                   p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Release_Unit
*
* DESCRIPTION
*
*        This function processes the SCSI RELEASE UNIT Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Release_Unit (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {

        /* If media dispatch entry for release unit function is valid? */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Release_Unit != NU_NULL)
        {

            /* Call SCSI Media dispatch Release Unit functionality. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Release_Unit (pcb_scsi_media,
                                   pcb_user_scsi,
                                   p_cmd,
                                   cmd_len,
                                   pp_buf_out,
                                   p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Capacity
*
* DESCRIPTION
*
*        This function processes the SCSI READ CAPACITY Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Capacity (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {

        /* If media dispatch entry for capacity function is valid? */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Capacity != NU_NULL)
        {

            /* Call SCSI Media dispatch Capacity functionality. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Capacity (pcb_scsi_media,
                               pcb_user_scsi,
                               p_cmd, cmd_len,
                               pp_buf_out,
                               p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Verify
*
* DESCRIPTION
*
*        This function processes the SCSI VERIFY Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Verify (NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {

        /* If media dispatch entry for verify function is valid? */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Verify != NU_NULL)
        {

            /* Call SCSI Media dispatch Verify functionality. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Verify (pcb_scsi_media,
                             pcb_user_scsi,
                             p_cmd,
                             cmd_len,
                             pp_buf_out,
                             p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Prevent_Allow
*
* DESCRIPTION
*
*        This function processes the SCSI PREVENT ALLOW Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS              Indicates successful processing of this
*                                event.
*        NU_USB_NOT_SUPPORTED    Indicates that the media doesn't support
*                                this event.
*
**************************************************************************/
STATUS NU_USBF_SCSI_MEDIA_Prevent_Allow (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {

        /* If media dispatch entry for Prevent Allow function is valid? */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Prevent_Allow != NU_NULL)
        {

            /* Call SCSI Media dispatch Prevent Allow functionality. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Prevent_Allow (pcb_scsi_media,
                             pcb_user_scsi,
                             p_cmd,
                             cmd_len,
                             pp_buf_out,
                             p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Start_Stop
*
* DESCRIPTION
*
*        This function processes the SCSI Start-Stop Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS              Indicates successful processing of this
*                                event.
*        NU_USB_NOT_SUPPORTED    Indicates that the media doesn't support
*                                this event.
*
**************************************************************************/
STATUS NU_USBF_SCSI_MEDIA_Start_Stop (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {

        /* If media dispatch entry for Start Stop function is valid? */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Start_Stop != NU_NULL)
        {

            /* Call SCSI Media dispatch Start Stop functionality. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Start_Stop (pcb_scsi_media,
                             pcb_user_scsi,
                             p_cmd,
                             cmd_len,
                             pp_buf_out,
                             p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Read_6
*
* DESCRIPTION
*
*        This function processes the SCSI READ (6) Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Read_6 (
                              NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                              const NU_USBF_USER_SCSI  *pcb_user_scsi,
                              const UINT8              *p_cmd,
                              const UINT16              cmd_len,
                              UINT8                   **pp_buf_out,
                              UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        /* Call Read Write6 functionality. */
        status = _NU_USBF_SCSI_MEDIA_Read_Write6 (pcb_scsi_media,
                                                   pcb_user_scsi,
                                                   p_cmd,
                                                   cmd_len,
                                                   pp_buf_out,
                                                   p_data_len_out);
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Read_10
*
* DESCRIPTION
*
*        This function processes the SCSI READ (10) Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Read_10 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const  UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        /* Invoke Read/Write 10 functionality. */
        status = _NU_USBF_SCSI_MEDIA_RW10 (pcb_scsi_media,
                                           pcb_user_scsi,
                                           p_cmd, cmd_len,
                                           pp_buf_out,
                                           p_data_len_out);
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Read_12
*
* DESCRIPTION
*
*        This function processes the SCSI READ (12) Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Read_12 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const  UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        /* Invoke Read/Write 12 functionality. */
        status = _NU_USBF_SCSI_MEDIA_RW12 (pcb_scsi_media,
                                           pcb_user_scsi,
                                           p_cmd, cmd_len,
                                           pp_buf_out,
                                           p_data_len_out);
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Write_6
*
* DESCRIPTION
*
*        This function processes the SCSI WRITE (6) Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Write_6 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        /* Invoke Read/Write (6) functionality. */
        status = _NU_USBF_SCSI_MEDIA_Read_Write6 (pcb_scsi_media,
                                                   pcb_user_scsi,
                                                   p_cmd,
                                                   cmd_len,
                                                   pp_buf_out,
                                                   p_data_len_out);
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Write_10
*
* DESCRIPTION
*
*        This function processes the SCSI WRITE (10) Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Write_10 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        /* Invoke Read/Write 10 functionality. */
        status = _NU_USBF_SCSI_MEDIA_RW10 (pcb_scsi_media,
                                           pcb_user_scsi,
                                           p_cmd,
                                           cmd_len,
                                           pp_buf_out,
                                           p_data_len_out);
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Write_12
*
* DESCRIPTION
*
*        This function processes the SCSI WRITE (12) Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Write_12 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        /* Invoke Read/Write 12 functionality. */
        status = _NU_USBF_SCSI_MEDIA_RW12 (pcb_scsi_media,
                                           pcb_user_scsi,
                                           p_cmd,
                                           cmd_len,
                                           pp_buf_out,
                                           p_data_len_out);
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Unknown_Cmd
*
* DESCRIPTION
*
*        This function processes the any of the SCSI Command that is not
*        mandatory as per the SCSI specification.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
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
STATUS NU_USBF_SCSI_MEDIA_Unknown_Cmd (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL && p_cmd != NU_NULL
        && cmd_len > 0 && pp_buf_out != NU_NULL && p_data_len_out != NU_NULL)
    {
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
            Unknown_SCSI_Command != NU_NULL)
        {

            /* Call SCSI Media dispatch Unknown SCSI Command
             * functionality.
             */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                      Unknown_SCSI_Command (pcb_scsi_media,
                                            pcb_user_scsi,
                                            p_cmd, cmd_len,
                                            pp_buf_out,
                                            p_data_len_out);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Reset_Device
*
* DESCRIPTION
*
*        This function resets the media and brings the media to a stable
*        state.
*
* INPUTS
*
*        pcb_scsi_media        Media to which the Host sent the command.
*        pcb_user_scsi         The media container.
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
STATUS NU_USBF_SCSI_MEDIA_Reset_Device (NU_USBF_SCSI_MEDIA *pcb_scsi_media,
                                        NU_USBF_USER_SCSI  *pcb_user_scsi)
{
    /* Initialize status as command not supported. */
    STATUS status = NU_USB_NOT_SUPPORTED;

    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL && pcb_user_scsi != NU_NULL)
    {
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
            Reset_Device != NU_NULL)
        {

            /* Call SCSI Media dispatch Reset device functionality. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
                     (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
                     Reset_Device (pcb_scsi_media, pcb_user_scsi);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Read
*
* DESCRIPTION
*
*        This function reads 'length' number of bytes from the media into
*        the 'p_buffer'.
*
* INPUTS
*
*        pcb_scsi_media        The media to be read from.
*        block                 Block number to start from.
*        p_buffer              Buffer to place the read data.
*        length                Number of bytes to read.
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
STATUS NU_USBF_SCSI_MEDIA_Read (NU_USBF_SCSI_MEDIA *pcb_scsi_media,
                                UINT32              block,
                                UINT8              *p_buffer,
                                UINT32              length)
{
    STATUS status = NU_USB_INVLD_ARG;
    if (pcb_scsi_media != NU_NULL)
    {
        /* To see if we have got Read functionality available in SCSI
         * media dispatch.
         */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Read != NU_NULL)
        {

            /* Invoke SCSI media dispatch read functionality. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
             (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
             Read (pcb_scsi_media, block, p_buffer, length);
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_SCSI_MEDIA_Write
*
* DESCRIPTION
*
*        This function reads 'length' number of bytes from the media into
*        the 'p_buffer'.
*
* INPUTS
*
*        pcb_scsi_media      The media to be read from.
*        block               Block number to start from.
*        p_buffer            Buffer to place the read data.
*        length              Number of bytes to read.
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
STATUS NU_USBF_SCSI_MEDIA_Write (NU_USBF_SCSI_MEDIA *pcb_scsi_media,
                                 UINT32              block,
                                 UINT8              *p_buffer,
                                 UINT32              length)
{
    STATUS status = NU_USB_INVLD_ARG;
    /* Validate SCSI media pointer. */
    if (pcb_scsi_media != NU_NULL)
    {
        /* To see if we have got write functionality available in SCSI
         * media dispatch.
         */
        if (((NU_USBF_SCSI_MEDIA_DISPATCH *)
           (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
           Write != NU_NULL)
        {

            /* Call SCSI Media dispatch write behavior. */
            status = ((NU_USBF_SCSI_MEDIA_DISPATCH *)
            (((NU_USB *) pcb_scsi_media)->usb_dispatch))->
             Write (pcb_scsi_media, block, p_buffer, length);
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Connect
*
* DESCRIPTION
*
*        Processes the connection event. Any hardware specific
*        initialization that are required are done here. This event is used
*        to prepare the hardware for further transactions with the host.
*        The implementation is media specific.
*
* INPUTS
*
*        pcb_scsi_media          Media to be prepared for transactions.
*        pcb_user_scsi           The media container.
*
* OUTPUTS
*
*        NU_SUCCESS              Indicates successful processing of this
*                                event.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Connect (
                                  const NU_USBF_SCSI_MEDIA *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi)
{
    /* Always return success. */
    return( NU_SUCCESS);
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Disconnect
*
* DESCRIPTION
*
*        Processes the disconnection event.  Any hardware specific
*        de-initializations that are required are done here. The
*        implementation is media specific.
*
* INPUTS
*
*        pcb_scsi_media         Pointer to the SCSI media control block.
*        pcb_user_scsi          Pointer to the SCSI user control block.
*
* OUTPUTS
*
*        NU_SUCCESS             Indicates successful processing of this
*                               event.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Disconnect (
                                  const NU_USBF_SCSI_MEDIA *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi)
{
    /* Always return success. */
    return NU_SUCCESS;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Reset_Device
*
* DESCRIPTION
*
*        This function resets the media and brings the media to a stable
*        state.
*
* INPUTS
*
*        pcb_scsi_media        Media to which the Host sent the command.
*        pcb_user_scsi         The media container.
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
STATUS _NU_USBF_SCSI_MEDIA_Reset_Device (
                                    NU_USBF_SCSI_MEDIA *pcb_scsi_media,
                                    NU_USBF_USER_SCSI  *pcb_user_scsi)
{
     return NU_SUCCESS;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Transfer
*
* DESCRIPTION
*
*        This function is never expected to be invoked.
*
* INPUTS
*
*        pcb_scsi_media        Media to which the Host requested a transfer
*        pcb_user_scsi         The media container.
*        pp_buf_out            Memory location pointed by this variable
*                              must contain the data location, from where
*                              the data must be transferred.
*        p_data_len_out        Location where the number of bytes of data
*                              to be transferred must be initialized.
*
* OUTPUTS
*
*        NU_USB_INVLD_ARG      Indicates an invalid parameter.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Transfer (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    *pp_buf_out     = NU_NULL;               /* Reset out buffer. */
    *p_data_len_out = 0;                     /* Reset data length. */

    /* Since it is considered as error condition here, we are returning
     * invalid arguments.
     */
    return NU_USB_INVLD_ARG;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Tx_Done
*
* DESCRIPTION
*
*        This function processes a transfer completion notification. It
*        gets, the data pointer and the length associated with the previous
*        transfer, as parameters.
*        Processes a new transfer request from the Host. If any of the data
*        is to be transferred to/from the Host, the the corresponding data
*        pointer and the length will be set in the parameters. If no data
*        transfer is required then the memory location pointed to by the
*        'pp_buf_out' parameter must be filled with NU_NULL.
*
* INPUTS
*
*        pcb_scsi_media          Media to which the Host requested a
*                                transfer.
*        pcb_user_scsi           The media container.
*        p_completed_data        Data pointer associated with the previous
*                                transfer.
*        completed_data_length   Number of bytes transferred in previous
*                                transfer.
*        pp_buf_out              Memory location pointed by this variable
*                                must contain the data location, from where
*                                the data must be transferred.
*        p_data_len_out          Location where the number of bytes of data
*                                to be transferred must be initialized.
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
STATUS _NU_USBF_SCSI_MEDIA_Tx_Done (
                           NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                           const NU_USBF_USER_SCSI  *pcb_user_scsi,
                           const UINT8              *p_completed_data,
                           const UINT32              completed_data_length,
                           UINT8                   **pp_buf_out,
                           UINT32                   *p_data_len_out)
{
    /* Local variables. */
    UINT8  cmd    = pcb_scsi_media->last_command;
    STATUS status = NU_SUCCESS;

    /* A transfer has just been completed. If the command is neither read
     * nor write, then we are not bothered about subsequent data, because
     * no data is there beyond first transfer.
     */
    if ((cmd != USB_SPEC_MSF_WRITE_6)  &&
        (cmd != USB_SPEC_MSF_WRITE_10) &&
        (cmd != USB_SPEC_MSF_READ_6)   &&
        (cmd != USB_SPEC_MSF_READ_10) &&
        (cmd != USB_SPEC_MSF_READ_12) &&
        (cmd != USB_SPEC_MSF_WRITE_12))
    {
        *pp_buf_out     = NU_NULL;
        *p_data_len_out = 0;
    }

    /* Now that we may have more data to be transferred, lets see what
     * needs to be done. For write operations, flush the data from cache
     * buffer to media.
     */
    else
    {
        if ((cmd == USB_SPEC_MSF_WRITE_6) ||
            (cmd == USB_SPEC_MSF_WRITE_10)||
            (cmd == USB_SPEC_MSF_WRITE_12)||
            (cmd == USB_SPEC_MSF_READ_6)  ||
            (cmd == USB_SPEC_MSF_READ_10) ||
            (cmd == USB_SPEC_MSF_READ_12))
        {
            pcb_scsi_media->remaining_length    -= (completed_data_length / 512);
            pcb_scsi_media->block_num           += (completed_data_length / 512);
            pcb_scsi_media->last_length         = completed_data_length;

            if ( pcb_scsi_media->remaining_length > 0 )
                status = NU_USBF_MS_IO_PENDING;
            else
                status = NU_SUCCESS;

            *pp_buf_out     = NU_NULL;
            *p_data_len_out = 0;

            NU_Release_Semaphore(&pcb_scsi_media->rw_lock);
        }
    }

    return ( status );
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Ready
*
* DESCRIPTION
*
*        This function processes the SCSI TEST UNIT READY Command. Is the
*        medium Ready for Data transfers.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful processing of the command.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Ready (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Save this command in last command. */
    pcb_scsi_media->last_command = *(p_cmd);
    *pp_buf_out                  = NU_NULL;
    *p_data_len_out              = 0;

    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    /* Extend this to realize a Test Unit Ready command. This command
     * determines if this LUN will allow access to the medium. In the event
     * thing, return error here. Otherwise success. It is observed that on
     * some Hosts, responding with error for this command will cause the
     * host to behave inconsistently. Try returning success always.
     */
    if(pcb_scsi_media->media_present != NU_TRUE)
    {
        /* Save the fields which are required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NOT_READY;

        return NU_USBF_MS_SEND_STALL;
    }
    else
    {
        /* If we are here first time after media insertion then send error
         * so that host can query media capacity again.
         */
        if(pcb_scsi_media->media_state == MEDIA_INSERTED)
        {
            /* Update the media state to inserted instead of recently inserted. */
            pcb_scsi_media->media_state = MEDIA_PRESENT;

            /* Save the fields which are required to fill request sense command data. */
            pcb_scsi_media->sense_key = SCSI_SENSE_KEY_UNIT_ATTENTION;
            pcb_scsi_media->asc = 0x28;

            return NU_USBF_MS_SEND_STALL;
        }

        /* Save the fields which are required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NO_SENSE;

        return NU_USBF_MS_SEND_CSW;
    }

}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Sense
*
* DESCRIPTION
*
*        This function process the SCSI REQUEST SENSE Command. Any
*        information about the error reported.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful processing of the command.
*        NU_USB_INVLD_ARG   Indicates incorrect command block.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Sense (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Local variables. */
    VOID   *memret_ptr;
    STATUS  status = NU_SUCCESS;

    pcb_scsi_media->last_command = *(p_cmd); /* Save as last command. */
    *pp_buf_out                  = &pcb_scsi_media->temp_data[0];

    /* This command is issued by the Host to retrieve information about
     * any error that was reported. Extend this to fill the buffer with
     * data relevant to the error.
     */
    memret_ptr = memset (*pp_buf_out,
                         0,
                         (UNSIGNED_INT)USB_SPEC_MSF_SENSE_DATA_LEN);
    /* was memset successful? */
    if (memret_ptr != *pp_buf_out)
    {
        status = NU_USBF_MS_SEND_STALL;
    }
    if (status == NU_SUCCESS)
    {
        *p_data_len_out = (p_cmd[USB_SPEC_MSF_DATA_LEN_OFFSET] >
                           USB_SPEC_MSF_SENSE_DATA_LEN) ?
                           USB_SPEC_MSF_SENSE_DATA_LEN :
                           p_cmd[USB_SPEC_MSF_DATA_LEN_OFFSET];

        /* Sense command reply contains sense error code. */
        (*pp_buf_out)[0] = USB_SPEC_MSF_SENSE_EROR_CODE;

        /* Fill the command data buffer. */
        if(pcb_scsi_media->media_present != NU_TRUE)
        {
            (*pp_buf_out)[2] = SCSI_SENSE_KEY_NOT_READY;
            (*pp_buf_out)[7] = 0x0A;
            (*pp_buf_out)[12] = 0x3A;
        }
        else
        {
            (*pp_buf_out)[2] = (pcb_scsi_media->sense_key & 0x0F);
            (*pp_buf_out)[7] = 0x0A;
            (*pp_buf_out)[12] = pcb_scsi_media->asc;
            (*pp_buf_out)[13] = pcb_scsi_media->asc_qual;
        }
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Inquiry
*
* DESCRIPTION
*
*        This function process the SCSI INQUIRY Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful processing of the command.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Inquiry (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    STATUS status = NU_SUCCESS;

    /* Save command opcode in last command. */
    pcb_scsi_media->last_command = *(p_cmd);

    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    /* Check for media. */
    if(pcb_scsi_media->media_present != NU_TRUE)
    {
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;

        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NOT_READY;

        return NU_USBF_MS_SEND_STALL;
    }
    else
    {
        *p_data_len_out = (p_cmd[USB_SPEC_MSF_DATA_LEN_OFFSET] >
                           USB_SPEC_MSF_INQUIRY_DATA_LEN) ?
                           USB_SPEC_MSF_INQUIRY_DATA_LEN :
                           p_cmd[USB_SPEC_MSF_DATA_LEN_OFFSET];

        /* Return data length and buffer pointer for the inquiry command. */
         *pp_buf_out     = &pcb_scsi_media->inquiry_data[0];

        /* Save the fields which are required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NO_SENSE;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Mode_Sense6
*
* DESCRIPTION
*
*        This function processes the SCSI MODE SENSE (6) Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful processing of the command.
*        NU_USB_INVLD_ARG   Indicates incorrect command block.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Mode_Sense6 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Local variables. */
    VOID   *memret_ptr;
    STATUS  status = NU_SUCCESS;

    /* Save command opcode in last command. */
    pcb_scsi_media->last_command = *(p_cmd);

    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    if(pcb_scsi_media->media_present != NU_TRUE)
    {
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;

        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NOT_READY;

        return  NU_USBF_MS_SEND_STALL;
    }
    else
    {
        *pp_buf_out                  = &pcb_scsi_media->mode_data[0];

        /* Reset the buffer to be sent for mode sense data. */
        memret_ptr = memset (*pp_buf_out,
                             0,
                             (UNSIGNED_INT)USB_SPEC_MSF_MODE_DATA_LEN);

        /* Was memset successful? */
        if (memret_ptr != *pp_buf_out)
        {
            status = NU_USB_INVLD_ARG;
        }

        if (status == NU_SUCCESS)
        {
            if (pcb_scsi_media->mode_data[0] == 0)
            {
                pcb_scsi_media->mode_data[0] = USB_SPEC_MSF_MOD_PAGE_CTRL;
            }

            *p_data_len_out = (p_cmd[USB_SPEC_MSF_DATA_LEN_OFFSET] >
                               USB_SPEC_MSF_MODE_DATA_LEN) ?
                               USB_SPEC_MSF_MODE_DATA_LEN :
                               p_cmd[USB_SPEC_MSF_DATA_LEN_OFFSET];
        }
    }

    /* Save the fields depending upon the status which are required to fill request sense command data. */
    if(status == NU_SUCCESS)
    {
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NO_SENSE;
    }
    else
    {
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_ABBORTED_COMMAND;
        status = NU_USBF_MS_SEND_STALL;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Mode_Sense_10
*
* DESCRIPTION
*
*        This function processes the SCSI MODE SENSE (6) Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful processing of the command.
*        NU_USB_INVLD_ARG   Indicates incorrect command block.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Mode_Sense_10 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Local variables. */
    VOID   *memret_ptr;
    STATUS  status = NU_SUCCESS;
    UINT16  mode_data_length;

    /* Save command opcode in last command. */
    pcb_scsi_media->last_command = *(p_cmd);

    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    if(pcb_scsi_media->media_present != NU_TRUE)
    {
        pcb_scsi_media->last_command = *(p_cmd);
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;

        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NOT_READY;

        return  NU_USBF_MS_SEND_STALL;
    }
    else
    {
        mode_data_length = ((UINT16) p_cmd[8] |
                            ((UINT16)p_cmd[7] << USBF_SCSI_SHIFT_ONE_BYTE));

        *pp_buf_out = &pcb_scsi_media->mode_data[0];

        /* Reset the buffer to be sent for mode sense data. */
        memret_ptr = memset (*pp_buf_out,
                             0,
                             (UNSIGNED_INT)USB_SPEC_MSF_MODE_DATA_LEN);

        /* Was memset successful? */
        if ((memret_ptr != *pp_buf_out))
        {
            status = NU_USB_INVLD_ARG;
        }

        if (status == NU_SUCCESS)
        {

            if (pcb_scsi_media->mode_data[0] == 0)
            {
                pcb_scsi_media->mode_data[0] = USB_SPEC_MSF_MOD_PAGE_CTRL;
            }

            *p_data_len_out = (mode_data_length >
                               USB_SPEC_MSF_MODE_DATA_LEN) ?
                               USB_SPEC_MSF_MODE_DATA_LEN :
                               mode_data_length;
        }
    }

    /* Save the fields depending upon the status which are required to fill request sense command data. */
    if(status == NU_SUCCESS)
    {
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NO_SENSE;
    }
    else
    {
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_ABBORTED_COMMAND;
        status = NU_USBF_MS_SEND_STALL;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Mode_Sel6
*
* DESCRIPTION
*
*        This function processes the SCSI MODE SELECT (6) Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful processing of the command.
*        NU_USB_INVLD_ARG   Indicates incorrect command block.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Mode_Sel6 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Local variables. */
    VOID   *memret_ptr;
    STATUS  status = NU_SUCCESS;

    /* Set the buffer with mode select data. */
    *pp_buf_out         = &pcb_scsi_media->mode_data[0];

    /* Save command opcode as the last command. */
    pcb_scsi_media->last_command = *(p_cmd);

    memret_ptr = memset (*pp_buf_out,
                         0,
                         (UNSIGNED_INT)USB_SPEC_MSF_MODE_DATA_LEN);

    /* Was memset successful? */
    if (memret_ptr != *pp_buf_out)
    {
        status = NU_USB_INVLD_ARG;
    }

    if (status == NU_SUCCESS)
    {
        /* return mode sense data length. */
        *p_data_len_out = (p_cmd[USB_SPEC_MSF_DATA_LEN_OFFSET] >
                           USB_SPEC_MSF_MODE_DATA_LEN) ?
                           USB_SPEC_MSF_MODE_DATA_LEN :
                           p_cmd[USB_SPEC_MSF_DATA_LEN_OFFSET];
    }

    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    /* Save the fields depending upon the status which are required to fill request sense command data. */
    if(status == NU_SUCCESS)
    {
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NO_SENSE;
    }
    else
    {
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_ABBORTED_COMMAND;
        status = NU_USBF_MS_SEND_STALL;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Mode_Sel_10
*
* DESCRIPTION
*
*        This function processes the SCSI MODE SELECT (10) Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful processing of the command.
*        NU_USB_INVLD_ARG   Indicates incorrect command block.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Mode_Sel_10 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Local variables. */
    VOID   *memret_ptr;
    STATUS  status = NU_SUCCESS;
    UINT16  mode_data_length;

    mode_data_length = ((UINT16) p_cmd[8] |
                        ((UINT16)p_cmd[7] << USBF_SCSI_SHIFT_ONE_BYTE));

    /* Save command opcode as the last command. */
    pcb_scsi_media->last_command = *(p_cmd);

    /* Set the buffer with mode select data. */
    *pp_buf_out = &pcb_scsi_media->mode_data[0];

    memret_ptr = memset (*pp_buf_out,
                         0,
                         (UNSIGNED_INT)USB_SPEC_MSF_MODE_DATA_LEN);

    /* Was memset successful? */
    if ((memret_ptr != *pp_buf_out))
    {
        status = NU_USB_INVLD_ARG;
    }

    if (status == NU_SUCCESS)
    {
        *p_data_len_out = (mode_data_length >
                           USB_SPEC_MSF_MODE_DATA_LEN) ?
                           USB_SPEC_MSF_MODE_DATA_LEN :
                           mode_data_length;
    }

    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    /* Save the fields depending upon the status which are required to fill request sense command data. */
    if(status == NU_SUCCESS)
    {
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NO_SENSE;
    }
    else
    {
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_ABBORTED_COMMAND;
        status = NU_USBF_MS_SEND_STALL;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Snd_Diag
*
* DESCRIPTION
*
*        This function processes the SCSI SEND DIAGNOSTIC Command. This
*        command is issued by the Host to have the LUN perform some
*        diagnostics on itself.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful processing of the command.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Snd_Diag (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Extend this retrieve the diagnostic pages and perform those. As on
     * now, we return success here.
     */
    pcb_scsi_media->last_command = *(p_cmd);
    *pp_buf_out                  = NU_NULL;
    *p_data_len_out              = 0;

    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    if(pcb_scsi_media->media_present != NU_TRUE)
    {
        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NOT_READY;
        return NU_USBF_MS_SEND_STALL;
    }
    else
    {
        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NO_SENSE;
        return NU_SUCCESS;
    }
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Format
*
* DESCRIPTION
*
*        This function processes the SCSI FORMAT Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful processing of the command.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Format (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    /* This command is issued by the Host to format the media. Extend this
     * to support data associated with the command. Data will be associated
     * with this command in the event of the initiator preferring control
     * over the defect management done.
     */
    if(pcb_scsi_media->media_present != NU_TRUE)
    {
        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NOT_READY;

        return NU_USBF_MS_SEND_STALL;
    }
    else
    {
        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NO_SENSE;

        pcb_scsi_media->last_command = *(p_cmd);
        pcb_scsi_media->is_formatted = NU_TRUE;
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;

        return NU_SUCCESS;
    }
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_ReserveUnit
*
* DESCRIPTION
*
*        This function processes the SCSI RESERVE UNIT Command.
*        This command is issued by the Host to format the media.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful processing of the command.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_ReserveUnit (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    /* Extend this to support data associated with the command. Data will
     * be associated with this command in the event of the initiator
     * preferring control  over the defect management done.
     */
    if(pcb_scsi_media->media_present != NU_TRUE)
    {
        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NOT_READY;

        return NU_USBF_MS_SEND_STALL;
    }
    else
    {
        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key    = SCSI_SENSE_KEY_NO_SENSE;

        pcb_scsi_media->last_command = *(p_cmd);
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;
        pcb_scsi_media->is_formatted = NU_TRUE;

        return NU_SUCCESS;
    }
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_ReleaseUnit
*
* DESCRIPTION
*
*        This function processes the SCSI RELEASE UNIT Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful processing of the command.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_ReleaseUnit (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    /* This command is issued by the Host to format the media. Extend this
     * to support data associated with the command. Data will be associated
     * with this command in the event of the initiator preferring control
     * over the defect management done.
     */
    if(pcb_scsi_media->media_present != NU_TRUE)
    {
        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NOT_READY;

        return NU_USBF_MS_SEND_STALL;
    }
    else
    {
        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key    = SCSI_SENSE_KEY_NO_SENSE;

        pcb_scsi_media->last_command = *(p_cmd);
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;
        pcb_scsi_media->is_formatted = NU_TRUE;

        return NU_SUCCESS;
    }
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Capacity
*
* DESCRIPTION
*
*        This function processes the SCSI READ CAPACITY Command.
*
* INPUTS
*
*        pcb_scsi_media     Media to which the Host sent the command.
*        pcb_user_scsi      The media container.
*        p_cmd              Byte buffer containing the command block.
*        cmd_len            Length, in bytes, of the command block.
*        pp_buf_out         Memory location pointed by this variable must
*                           contain the data location, from where the data
*                           must be transferred.
*        p_data_len_out     Location where the number of bytes of data to
*                           be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS         Indicates successful processing of the command.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Capacity (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Save Read Capacity opcode as the last command. */
    pcb_scsi_media->last_command = *(p_cmd);

    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    if(pcb_scsi_media->media_present != NU_TRUE)
    {
        pcb_scsi_media->last_command = *(p_cmd);
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;

        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key    = SCSI_SENSE_KEY_NOT_READY;

        return NU_USBF_MS_SEND_STALL;
    }
    else
    {
        /* Save the fields depending required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NO_SENSE;

        /* Set buffer to return to the read capacity data. */
        *pp_buf_out                  = &pcb_scsi_media->read_capacity_data[0];

        /* Set data length to the read capacity data length described in mass
         * storage specification.
         */
        *p_data_len_out              = USB_SPEC_MSF_CAPACITY_DATA_LEN;

        return NU_SUCCESS;
    }
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Verify
*
* DESCRIPTION
*
*        This function processes the SCSI VERIFY Command.
*
* INPUTS
*
*        pcb_scsi_media        Media to which the Host sent the command.
*        pcb_user_scsi         The media container.
*        p_cmd                 Byte buffer containing the command block.
*        cmd_len               Length, in bytes, of the command block.
*        pp_buf_out            Memory location pointed by this variable
*                              must contain the data location, from where
*                              the data must be transferred.
*        p_data_len_out        Location where the number of bytes of data
*                              to be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS            Indicates successful processing of the
*                              command.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Verify (
                                 NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                 const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                 const UINT8              *p_cmd,
                                 const UINT16              cmd_len,
                                 UINT8                   **pp_buf_out,
                                 UINT32                    *p_data_len_out)
{
    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    /* This command is issued by the Host to have the LUN perform some
     * diagnostics on itself. Extend this to retrieve the diagnostic pages
     * and perform those. We return success here.
     */
    if(pcb_scsi_media->media_present != NU_TRUE)
    {
         pcb_scsi_media->last_command = *(p_cmd);
        *pp_buf_out                   = NU_NULL;
        *p_data_len_out               = 0;

        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key     = SCSI_SENSE_KEY_NOT_READY;

        return NU_USBF_MS_SEND_STALL;
    }
    else
    {
        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NO_SENSE;

        pcb_scsi_media->last_command = *(p_cmd);
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;

        return NU_USBF_MS_SEND_CSW;
     }
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Prevent_Allow
*
* DESCRIPTION
*
*        This function processes the SCSI Prevent Allow Command.
*
* INPUTS
*
*        pcb_scsi_media        Media to which the Host sent the command.
*        pcb_user_scsi         The media container.
*        p_cmd                 Byte buffer containing the command block.
*        cmd_len               Length, in bytes, of the command block.
*        pp_buf_out            Memory location pointed by this variable
*                              must contain the data location, from where
*                              the data must be transferred.
*        p_data_len_out        Location where the number of bytes of data
*                              to be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS            Indicates successful processing of the
*                              command.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Prevent_Allow (
                                 NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                 const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                 const UINT8              *p_cmd,
                                 const UINT16              cmd_len,
                                 UINT8                   **pp_buf_out,
                                 UINT32                    *p_data_len_out)
{
    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc_qual = 0;
    pcb_scsi_media->asc = 0;

    /* This command is issued by the Host to have the LUN perform some
     * diagnostics on itself. Extend this to retrieve the diagnostic pages
     * and perform those. We return success here.
     */
    if(pcb_scsi_media->media_present != NU_TRUE)
    {
        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NOT_READY;

         pcb_scsi_media->last_command = *(p_cmd);
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;

        return NU_USBF_MS_SEND_STALL;
    }
    else
    {
        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_ILLEGAL_REQUEST;
        pcb_scsi_media->asc = 0x26;

        pcb_scsi_media->last_command = *(p_cmd);
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;

        /* Return error as media can be removed without safe ejecting from the host. */
        return NU_USBF_MS_SEND_STALL;
     }
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Start_Stop
*
* DESCRIPTION
*
*        This function processes the SCSI Start Stop Command.
*
* INPUTS
*
*        pcb_scsi_media        Media to which the Host sent the command.
*        pcb_user_scsi         The media container.
*        p_cmd                 Byte buffer containing the command block.
*        cmd_len               Length, in bytes, of the command block.
*        pp_buf_out            Memory location pointed by this variable
*                              must contain the data location, from where
*                              the data must be transferred.
*        p_data_len_out        Location where the number of bytes of data
*                              to be transferred must be initialized.
*
* OUTPUTS
*
*        NU_SUCCESS            Indicates successful processing of the
*                              command.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Start_Stop (
                                 NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                 const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                 const UINT8              *p_cmd,
                                 const UINT16              cmd_len,
                                 UINT8                   **pp_buf_out,
                                 UINT32                    *p_data_len_out)
{
    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    /* This command is issued by the Host to have the LUN perform some
     * diagnostics on itself. Extend this to retrieve the diagnostic pages
     * and perform those. We return success here.
     */
    if(pcb_scsi_media->media_present != NU_TRUE)
    {
        pcb_scsi_media->last_command = *(p_cmd);
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;

        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NOT_READY;

        return NU_USBF_MS_SEND_STALL;
    }
    else
    {
        /* Save the sense key which is required to fill request sense command data. */
         pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NO_SENSE;

        pcb_scsi_media->last_command = *(p_cmd);
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;

        return NU_USBF_MS_SEND_CSW;
     }
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Command_23
*
* DESCRIPTION
*
*        UFI - READ FORMAT CAPACITIES Command Processing function.
*
* INPUTS
*
*        pcb_scsi_media        Media control block.
*        pcb_user_scsi         SCSI control block.
*        p_cmd                 Pointer to the command block.
*        data_out              Data transfer to be submitted, if any has
*                              to be filled here.
* OUTPUTS
*
*        NU_SUCCESS            Successful completion.
*        NU_USB_INVLD_ARG      Indicates incorrect command block.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Command_23 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Local variables. */
    VOID   *memret_ptr;
    STATUS  status = NU_SUCCESS;
    UINT32  temp   = 0;
    UINT16  format_data_length;

    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    /* Validate command. */
    if (*(p_cmd) != USB_SPEC_MSF_READ_FMT_CAPACITY)
    {
        status = NU_USB_INVLD_ARG;
    }
    else if(pcb_scsi_media->media_present != NU_TRUE)
    {
        pcb_scsi_media->last_command = *(p_cmd);
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;

        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NOT_READY;

        return  NU_USBF_MS_SEND_STALL;
    }

    if (status == NU_SUCCESS)
    {
        /* Save opcode as the last command. */
        pcb_scsi_media->last_command = *(p_cmd);

        *pp_buf_out = &pcb_scsi_media->temp_data[0];

        memret_ptr = memset (
                         *pp_buf_out,
                         0,
                         (UNSIGNED_INT)USB_SPEC_MSF_FMT_CPTY_DATA_LEN);

        format_data_length = ((UINT16) p_cmd[8] |
                              ((UINT16)p_cmd[7] << USBF_SCSI_SHIFT_ONE_BYTE));

        if (memret_ptr != *pp_buf_out || format_data_length > 255)
        {
            status = NU_USB_INVLD_ARG;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Get Read capacity data. */
        temp  = (UINT32) pcb_scsi_media->read_capacity_data[3];
        temp |= (UINT32)(pcb_scsi_media->read_capacity_data[2] <<
                                               USBF_SCSI_SHIFT_ONE_BYTE);
        temp |= (UINT32)(pcb_scsi_media->read_capacity_data[1] <<
                                               USBF_SCSI_SHIFT_TWO_BYTES);
        temp |= (UINT32)(pcb_scsi_media->read_capacity_data[0] <<
                                               USBF_SCSI_SHIFT_THREE_BYTES);
        temp++;

        /* Capacity list length. */
        pcb_scsi_media->temp_data[3] = USB_SPEC_MSF_CAPACITY_DATA_LEN;

        /* Current/Maximum Capacity Descriptor. First four bytes are
         * number of blocks.
         */
        pcb_scsi_media->temp_data[7] = temp & USBF_SCSI_MASK;
        temp >>= USBF_SCSI_SHIFT_ONE_BYTE;

        pcb_scsi_media->temp_data[6] = temp & USBF_SCSI_MASK;
        temp >>= USBF_SCSI_SHIFT_ONE_BYTE;

        pcb_scsi_media->temp_data[5] = temp & USBF_SCSI_MASK;
        temp >>= USBF_SCSI_SHIFT_ONE_BYTE;

        pcb_scsi_media->temp_data[4] = temp & USBF_SCSI_MASK;

        /* Byte # 5 for Descriptor code. */
        pcb_scsi_media->temp_data[8] = USB_SPEC_MSF_MAX_FORMAT_CPACITY;

        /* Next three bytes corresponds to Block length. */
        pcb_scsi_media->temp_data[9]  =
                                     pcb_scsi_media->read_capacity_data[4];
        pcb_scsi_media->temp_data[10] =
                                     pcb_scsi_media->read_capacity_data[5];
        pcb_scsi_media->temp_data[11] =
                                     pcb_scsi_media->read_capacity_data[6];

        /* Return format capacities data length. */
        *p_data_len_out = (format_data_length >
                           USB_SPEC_MSF_CPACITY_LIST_LEN) ?
                           USB_SPEC_MSF_CPACITY_LIST_LEN :
                           format_data_length;

        /* Length here is deliberately set to USB_SPEC_MSF_CPACITY_LIST_LEN instead of
         * USB_SPEC_MSF_FMT_CPTY_DATA_LEN. Both are allowed by MS specs case 4 and 5 but
         * we adopted the one for putting a workaround for some controller drivers which have
         * problem stalling the device and resetting the data toggle after STALL.
         */
    }

    /* Save the fields depending upon the status which are required to fill request sense command data. */
    if(status == NU_SUCCESS)
    {
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NO_SENSE;
    }
    else
    {
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_ABBORTED_COMMAND;
        status = NU_USBF_MS_SEND_STALL;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Read_Write6
*
* DESCRIPTION
*
*        This function processes the SCSI READ/WRITE (6) Command.
*
* INPUTS
*
*        pcb_scsi_media    Media to which the Host sent the command.
*        pcb_user_scsi     The media container.
*        p_cmd             Byte buffer containing the command block.
*        cmd_len           Length, in bytes, of the command block.
*        pp_buf_out        Memory location pointed by this variable must
*                          contain the data location, from/to where the
*                          data must be transferred.
*        p_data_len_out    Location where the number of bytes of data to
*                          be transferred must be initialized.
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
STATUS _NU_USBF_SCSI_MEDIA_Read_Write6 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Local variables. */
    STATUS      status    = NU_SUCCESS;
    UINT32      block_num = 0;
    UINT32      length;
    UINT8       byte1, byte2, byte3;

    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    if(pcb_scsi_media->media_present != NU_TRUE)
    {
        pcb_scsi_media->last_command = *(p_cmd);
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;

        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NOT_READY;

        return  NU_USBF_MS_SEND_STALL;
    }

    if(status == NU_SUCCESS)
    {
        /* Read the Logical block address. */
        byte1 = *(p_cmd + 3);
        byte2 = *(p_cmd + 2);
        byte3 = *(p_cmd + 1);

        byte3 &= USBF_SCSI_LUN_MASK;             /* Masking off LUN. */
        block_num = ((UINT32) byte1 |
                     ((UINT32) byte2 << USBF_SCSI_SHIFT_ONE_BYTE) |
                     ((UINT32) byte3 << USBF_SCSI_SHIFT_TWO_BYTES));

        /* Read the length. */
        length = *(p_cmd + 4);

        /* This is the number of blocks. */
        pcb_scsi_media->remaining_length = length;
        pcb_scsi_media->last_command     = *p_cmd;
        pcb_scsi_media->block_num        = block_num;

        if (*p_cmd == USB_SPEC_MSF_READ_6)
        {
            /* Call Read functionality. */
            status = NU_USBF_SCSI_MEDIA_Read (pcb_scsi_media,
                                              pcb_scsi_media->block_num,
                                              pcb_scsi_media->data_buffer,
                                              pcb_scsi_media->remaining_length);
        }
        else if (*p_cmd == USB_SPEC_MSF_WRITE_6)
        {
            /* Call Write functionality. */
            status = NU_USBF_SCSI_MEDIA_Write (pcb_scsi_media,
                                              pcb_scsi_media->block_num,
                                              pcb_scsi_media->data_buffer,
                                              pcb_scsi_media->remaining_length);
        }
    }

    /* Save the fields depending upon the status which are required to fill request sense command data. */
    if ((status == NU_SUCCESS) || (status == NU_USBF_MS_IO_PENDING))
    {
        pcb_scsi_media->sense_key       = SCSI_SENSE_KEY_NO_SENSE;
        status                          = NU_USBF_MS_IO_PENDING;
        *pp_buf_out                     = NU_NULL;
        *p_data_len_out                 = 0;
        pcb_scsi_media->last_length     = 0;
    }
    else
    {
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_ABBORTED_COMMAND;
        status = NU_USBF_MS_SEND_STALL;
    }

    return ( status );
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_RW10
*
* DESCRIPTION
*
*        This function processes the SCSI READ/WRITE (10) Command.
*
* INPUTS
*
*        pcb_scsi_media      Media to which the Host sent the command.
*        pcb_user_scsi       The media container.
*        p_cmd               Byte buffer containing the command block.
*        cmd_len             Length, in bytes, of the command block.
*        pp_buf_out          Memory location pointed by this variable must
*                            contain the data location, from/to where the
*                            data must be transferred.
*        p_data_len_out      Location where the number of bytes of data to
*                            be transferred must be initialized.
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
STATUS _NU_USBF_SCSI_MEDIA_RW10 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Local variables. */
    STATUS status    = NU_SUCCESS;
    UINT32 block_num = 0;
    UINT32 length;
    UINT8  byte1, byte2, byte3, byte4;
    NU_USBF_MS_CALLBACKS    *rw_callbacks;
    NU_USBF_SCSI_MEDIA      *scsi_media;

    NU_SUPERV_USER_VARIABLES

    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    if(pcb_scsi_media->media_present != NU_TRUE)
    {
        pcb_scsi_media->last_command = *(p_cmd);
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;

        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NOT_READY;

        return  NU_USBF_MS_SEND_STALL;
    }

    if(status == NU_SUCCESS)
    {
        /* Read first four bytes from read/write command. */
        byte1 = *(p_cmd + 5);
        byte2 = *(p_cmd + 4);
        byte3 = *(p_cmd + 3);
        byte4 = *(p_cmd + 2);

        /* Logical block at which write/read operation will begin. */
        block_num = ((UINT32) byte1 |
                     ((UINT32) byte2 << USBF_SCSI_SHIFT_ONE_BYTE) |
                     ((UINT32) byte3 << USBF_SCSI_SHIFT_TWO_BYTES)|
                     ((UINT32) byte4 << USBF_SCSI_SHIFT_THREE_BYTES));

        /* Get byte no 7 and 8 from read/write command for transfer length. */
        byte1 = *(p_cmd + 8);
        byte2 = *(p_cmd + 7);

        /* Contiguous logical blocks of data that shall be transferred. */
        length = ((UINT32) byte1 |
                 ((UINT32) byte2 << USBF_SCSI_SHIFT_ONE_BYTE));

        /* Save length as remaining length that would actually be transferred.
         * in this turn.
         */
        pcb_scsi_media->remaining_length = length;

        /* Save command opcode for read/write command as last command. */
        pcb_scsi_media->last_command = *p_cmd;

        /* Save logical block number. */
        pcb_scsi_media->block_num = block_num;

        rw_callbacks = pcb_scsi_media->rw_callbacks;
        scsi_media = pcb_scsi_media;

        /* Switch to user mode before giving callback to application. */
        NU_USER_MODE();

        if (*p_cmd == USB_SPEC_MSF_READ_10)
        {
            status = rw_callbacks->Read(scsi_media,
                                        scsi_media->block_num,
                                        scsi_media->data_buffer,
                                        scsi_media->remaining_length);
        }
        else if (*p_cmd == USB_SPEC_MSF_WRITE_10)
        {
            status = rw_callbacks->Write(scsi_media,
                                         scsi_media->block_num,
                                         scsi_media->data_buffer,
                                         scsi_media->remaining_length);
        }

        /* Switch back to supervisor mode. */
        NU_SUPERVISOR_MODE();

    }

    /* Save the fields depending upon the status which are required to fill request sense command data. */
    if ((status == NU_SUCCESS) || (status == NU_USBF_MS_IO_PENDING))
    {
        pcb_scsi_media->sense_key       = SCSI_SENSE_KEY_NO_SENSE;
        status                          = NU_USBF_MS_IO_PENDING;
        *pp_buf_out                     = NU_NULL;
        *p_data_len_out                 = (pcb_scsi_media->remaining_length * USBF_SCSI_BLOCK_SIZE);
        pcb_scsi_media->last_length     = 0;
    }
    else
    {
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_ABBORTED_COMMAND;
        status = NU_USBF_MS_SEND_STALL;
    }

    return ( status );
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_RW12
*
* DESCRIPTION
*
*        This function processes the SCSI READ/WRITE (12) Command.
*
* INPUTS
*
*        pcb_scsi_media      Media to which the Host sent the command.
*        pcb_user_scsi       The media container.
*        p_cmd               Byte buffer containing the command block.
*        cmd_len             Length, in bytes, of the command block.
*        pp_buf_out          Memory location pointed by this variable must
*                            contain the data location, from/to where the
*                            data must be transferred.
*        p_data_len_out      Location where the number of bytes of data to
*                            be transferred must be initialized.
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
STATUS _NU_USBF_SCSI_MEDIA_RW12 (
                                  NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
                                  const NU_USBF_USER_SCSI  *pcb_user_scsi,
                                  const UINT8              *p_cmd,
                                  const UINT16              cmd_len,
                                  UINT8                   **pp_buf_out,
                                  UINT32                   *p_data_len_out)
{
    /* Local variables. */
    STATUS status    = NU_SUCCESS;
    UINT32 block_num = 0;
    UINT32 length;
    UINT8  byte1, byte2, byte3, byte4;

    /* Save additional sense code/qualifier. */
    pcb_scsi_media->asc = 0;
    pcb_scsi_media->asc_qual = 0;

    if(pcb_scsi_media->media_present != NU_TRUE)
    {
        pcb_scsi_media->last_command = *(p_cmd);
        *pp_buf_out                  = NU_NULL;
        *p_data_len_out              = 0;

        /* Save the sense key which is required to fill request sense command data. */
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_NOT_READY;

        return  NU_USBF_MS_SEND_STALL;
    }

    if(status == NU_SUCCESS)
    {
        /* Read first four bytes from read/write command. */
        byte1 = *(p_cmd + 5);
        byte2 = *(p_cmd + 4);
        byte3 = *(p_cmd + 3);
        byte4 = *(p_cmd + 2);

        /* Logical block at which write/read operation will begin. */
        block_num = ((UINT32)  byte1 |
                     ((UINT32) byte2 << USBF_SCSI_SHIFT_ONE_BYTE) |
                     ((UINT32) byte3 << USBF_SCSI_SHIFT_TWO_BYTES)|
                     ((UINT32) byte4 << USBF_SCSI_SHIFT_THREE_BYTES));

        /* Get byte no 7 and 8 from read/write command for transfer length. */
        byte1 = *(p_cmd + 9);
        byte2 = *(p_cmd + 8);
        byte3 = *(p_cmd + 7);
        byte4 = *(p_cmd + 6);

        /* Contiguous logical blocks of data that shall be transferred. */
        length = ((UINT32) byte1 |
                 ((UINT32) byte2 << USBF_SCSI_SHIFT_ONE_BYTE)|
                 ((UINT32) byte3 << USBF_SCSI_SHIFT_TWO_BYTES)|
                 ((UINT32) byte4 << USBF_SCSI_SHIFT_THREE_BYTES));

        /* Save length as remaining length that would actually be transferred.
         * in this turn.
         */
        pcb_scsi_media->remaining_length = length;

        /* Save command opcode for read/write command as last command. */
        pcb_scsi_media->last_command = *p_cmd;

        /* Save logical block number. */
        pcb_scsi_media->block_num = block_num;

        if (*p_cmd == USB_SPEC_MSF_READ_12)
        {
            status = NU_USBF_SCSI_MEDIA_Read (pcb_scsi_media,
                                              pcb_scsi_media->block_num,
                                              pcb_scsi_media->data_buffer,
                                              pcb_scsi_media->remaining_length);
        }
        else if (*p_cmd == USB_SPEC_MSF_WRITE_12)
        {
            status = NU_USBF_SCSI_MEDIA_Write (pcb_scsi_media,
                                              pcb_scsi_media->block_num,
                                              pcb_scsi_media->data_buffer,
                                              pcb_scsi_media->remaining_length);
        }
    }

    /* Save the fields depending upon the status which are required to fill request sense command data. */
    if ((status == NU_SUCCESS) || (status == NU_USBF_MS_IO_PENDING))
    {
        pcb_scsi_media->sense_key       = SCSI_SENSE_KEY_NO_SENSE;
        status                          = NU_USBF_MS_IO_PENDING;
        *pp_buf_out                     = NU_NULL;
        *p_data_len_out                 = 0;
        pcb_scsi_media->last_length     = 0;
    }
    else
    {
        pcb_scsi_media->sense_key = SCSI_SENSE_KEY_ABBORTED_COMMAND;
        status = NU_USBF_MS_SEND_STALL;
    }

    return status;
}

/**************************************************************************
* FUNCTION
*
*        _NU_USBF_SCSI_MEDIA_Delete
*
* DESCRIPTION
*
*        Media un-initialization function.
*
* INPUTS
*
*        pcb_media       Media to be un-initialization.
*
* OUTPUTS
*
*        NU_SUCCESS       Un-initialization successful.
*        NU_USB_INVLD_ARG Invalid argument.
*
**************************************************************************/
STATUS _NU_USBF_SCSI_MEDIA_Delete (VOID *pcb_media)
{
    STATUS status = NU_USB_INVLD_ARG;
    NU_USBF_SCSI_MEDIA *media = (NU_USBF_SCSI_MEDIA *)pcb_media;

    if(pcb_media && media->inquiry_data != NU_NULL)
    {
        if(media->scsi_user != NU_NULL)
        {
            status = NU_USBF_USER_SCSI_Dereg_Media(media->scsi_user, pcb_media);
        }

        /* Deallocate memory for temporary buffer. */
        if ( media->temp_data )
        {
            USB_Deallocate_Memory(media->temp_data);
            media->temp_data = NU_NULL;
        }

        /* Deallocate memory from mode sense data buffer. */
        if ( media->mode_data )
        {
            USB_Deallocate_Memory(media->mode_data);
            media->mode_data = NU_NULL;
        }

        /* Deallocate memory from data I/O buffer. */
        if ( media->data_buffer )
        {
            USB_Deallocate_Memory(media->data_buffer);
            media->data_buffer = NU_NULL;
        }

        status = _NU_USB_Delete (pcb_media);

        memset(pcb_media, 0, sizeof(NU_USBF_SCSI_MEDIA));
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_MS_DM_Open
*
* DESCRIPTION
*
*       This function is called by the application when it opens a device
*       for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the SCSI media cb passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS    NU_USBF_MS_DM_Open (VOID* dev_handle)
{
    STATUS status;
    NU_USBF_MS  *ms_cb;

    status = NU_USBF_MS_Init_GetHandle((VOID**)&ms_cb);
    if ( status == NU_SUCCESS )
    {
        status = NU_USBF_MS_Bind_Interface(ms_cb);
    }
    return (status) ;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_MS_DM_Close
*
* DESCRIPTION
*
*       This function is called by the application when it wants to close a device
*       which it has already been opened for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the SCSI media cb passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS    NU_USBF_MS_DM_Close(VOID* dev_handle)
{
    STATUS      status;
    NU_USBF_MS  *ms_cb;

    status = NU_USBF_MS_Init_GetHandle((VOID**)&ms_cb);
    if ( status == NU_SUCCESS )
    {
        status = NU_USBF_MS_Unbind_Interface(ms_cb);
    }
    return (status) ;
}
/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_MS_DM_Read
*
* DESCRIPTION
*
*       This function is called by the application when it wants to read
*       data from the device.
*
* INPUTS
*
*       dev_handle         Pointer to the SCSI media cb passed as context.
*       buffer             Pointer to memory location where to put the read data.
*       numbyte            Number of bytes to be read.
*       byte_offset        In case read data is to be placed at certain offset in the buffer.
*       bytes_read_ptr     OUTPUT: Number of bytes actually read.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS    NU_USBF_MS_DM_Read(
                            VOID*    dev_handle,
                            VOID*    buffer,
                            UINT32   numbyte,
                            OFFSET_T byte_offset,
                            UINT32   *bytes_read_ptr)
{
    STATUS              status;
    NU_USBF_SCSI_MEDIA  *pdevice;
    NU_USBF_USER_SCSI   *scsi_user;

    pdevice         = (NU_USBF_SCSI_MEDIA*) dev_handle;
    scsi_user       = (pdevice->scsi_user);
    *bytes_read_ptr = 0;

    status = NU_USBF_MS_RW(scsi_user->handle,
                            buffer,
                            numbyte,
                            byte_offset);
    if ( status == NU_SUCCESS )
    {
        status = NU_Obtain_Semaphore(&pdevice->rw_lock, (NU_PLUS_Ticks_Per_Second * 5));
        if ( status == NU_SUCCESS )
        {
            *bytes_read_ptr = pdevice->last_length;
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_MS_DM_Write
*
* DESCRIPTION
*
*       This function is called by the application when it wants to write
*       data from the device.
*
* INPUTS
*
*       dev_handle         Pointer to the Ethernet driver passed as context.
*       buffer             Pointer to memory location where data to be written is available.
*       numbyte            Number of bytes to be written.
*       byte_offset        In case data is to be read at certain offset in the buffer.
*       bytes_written_ptr  OUTPUT: Number of bytes actually written.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS   NU_USBF_MS_DM_Write(VOID*       dev_handle,
                            const VOID*  buffer,
                            UINT32       numbyte,
                            OFFSET_T     byte_offset,
                            UINT32       *bytes_written_ptr)
{
    STATUS              status;
    NU_USBF_SCSI_MEDIA  *pdevice;
    NU_USBF_USER_SCSI   *scsi_user;

    pdevice             = (NU_USBF_SCSI_MEDIA*) dev_handle;
    scsi_user           = (pdevice->scsi_user);
    *bytes_written_ptr  = 0;

    status = NU_USBF_MS_RW(scsi_user->handle,
                            (VOID*)buffer,
                            numbyte,
                            byte_offset);
    if ( status == NU_SUCCESS )
    {
        status = NU_Obtain_Semaphore(&pdevice->rw_lock, (NU_PLUS_Ticks_Per_Second * 5));
        if ( status == NU_SUCCESS )
        {
            *bytes_written_ptr = pdevice->last_length;
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_RNDIS_DM_IOCTL
*
* DESCRIPTION
*
*       This function is called by the application when it wants to perform a control
*       operation on the device.
*
* INPUTS
*
*       dev_handle         Pointer to the SCSI media cb passed as context.
*       cmd                IOCTL number.
*       data               IOCTL data pointer of variable type.
*       length             IOCTL data length in bytes.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS   NU_USBF_MS_DM_IOCTL (
                              VOID*     dev_handle,
                              INT       ioctl_num,
                              VOID*     ioctl_data,
                              INT       ioctl_data_len)
{
    STATUS                  status = DV_IOCTL_INVALID_CMD;
    DV_IOCTL0_STRUCT        *ioctl0;
    NU_USBF_MS_CALLBACKS    *callbacks;
    NU_USBF_SCSI_MEDIA      *pdevice;
    UINT32                  temp;

    pdevice = (NU_USBF_SCSI_MEDIA*) dev_handle;

    /* Process command */
    switch (ioctl_num)
    {
        case DV_IOCTL0:
        {
            if (ioctl_data_len == sizeof(DV_IOCTL0_STRUCT))
            {
                /* Get the ioctl0 structure from the data passed in */
                ioctl0          = (DV_IOCTL0_STRUCT *) ioctl_data;
                ioctl0->base    = USB_STORE_IOCTL_BASE;
                status          = NU_SUCCESS;
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }
        case (USB_STORE_IOCTL_BASE + USBF_SCSI_MEDIA_SET_CALLBACKS):
        {
            /* Get the dev info structure from the data passed in */
            callbacks = (NU_USBF_MS_CALLBACKS*)ioctl_data;
            pdevice->rw_callbacks = callbacks;
            break;
        }
        case (USB_STORE_IOCTL_BASE + USBF_SCSI_MEDIA_SET_LUN_SIZE):
        {
            /* Get the dev info structure from the data passed in */
            temp = *((UINT32*)ioctl_data);
            pdevice->read_capacity_data[0] = (UINT8)((temp >> 24) & 0x000000FF);
            pdevice->read_capacity_data[1] = (UINT8)((temp >> 16) & 0x000000FF);
            pdevice->read_capacity_data[2] = (UINT8)((temp >> 8) & 0x000000FF);
            pdevice->read_capacity_data[3] = (UINT8)(temp & 0x000000FF);
            break;
        }
        case (USB_STORE_IOCTL_BASE + USBF_SCSI_MEDIA_SET_BLOCK_SIZE):
        {
            /* Get the dev info structure from the data passed in */
            temp = *((UINT32*)ioctl_data);
            pdevice->read_capacity_data[4] = (UINT8)((temp >> 24) & 0x000000FF);
            pdevice->read_capacity_data[5] = (UINT8)((temp >> 16) & 0x000000FF);
            pdevice->read_capacity_data[6] = (UINT8)((temp >> 8) & 0x000000FF);
            pdevice->read_capacity_data[7] = (UINT8)(temp & 0x000000FF);
            break;
        }
        case (USB_STORE_IOCTL_BASE + USBF_SCSI_SET_INQUIRY_DATA):
        {
            pdevice->inquiry_data = (UINT8*) ioctl_data;
            break;
        }
        case (USB_STORE_IOCTL_BASE + USBF_SCSI_SET_CAPACITY_DATA):
        {
            pdevice->read_capacity_data = (UINT8*) ioctl_data;
            break;
        }
        case (USB_STORE_IOCTL_BASE + USBF_SCSI_INSERT_MEDIA):
        {
            _NU_USBF_SCSI_MEDIA_Insert(pdevice);
            break;
        }
        case (USB_STORE_IOCTL_BASE + USBF_SCSI_REMOVE_MEDIA):
        {
            _NU_USBF_SCSI_MEDIA_Remove(pdevice);
            break;
        }


        default:
        {
            status = DV_IOCTL_INVALID_CMD;
            break;
        }
    }

    return (status);
}

/************************* End Of File ***********************************/

