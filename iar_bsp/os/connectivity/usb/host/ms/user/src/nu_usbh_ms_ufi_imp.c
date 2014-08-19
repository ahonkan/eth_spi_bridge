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
*       nu_usbh_ms_ufi_imp.c
*
* COMPONENT
*
*       Nucleus USB Host Mass Storage class User Driver
*       Wrapper for UFI subclass.
*
* DESCRIPTION
*
*       This file contains the definitions for intrfaces
*       exported by UFI wrapper.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBH_MS_UFI_Inquiry              Transfer INQUIRY command.
*       NU_USBH_MS_UFI_Test_Unit_Ready      Transfer TEST UNIT READY
*                                           command.
*       NU_USBH_MS_UFI_Read_Capacity        Transfer READ CAPACITY command.
*       NU_USBH_MS_UFI_Request_Sense        Transfer REQUEST SENSE command.
*       NU_USBH_MS_UFI_Read10               Transfer READ10 command.
*       NU_USBH_MS_UFI_Write10              Transfer WRITE10 command.
*       NU_USBH_MS_UFI_Request              General UFI command transfer
*                                           request API.
*
* DEPENDENCIES
*
*       nu_usb.h                            USB Definitions.
*
**************************************************************************/
#ifndef NU_USBH_MS_UFI_IMP_C
#define NU_USBH_MS_UFI_IMP_C
/* ==============  USB Include Files ==================================  */
#include "connectivity/nu_usb.h"

#if INCLUDE_UFI

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_MS_UFI_Inquiry
*
* DESCRIPTION
*
*       Get a standard INQUIRY data.
*       This API send INQUIRY command with "CmdDt" and "EVPD" bit of
*       zero.
*
* INPUTS
*
*       handle                              Handle.
*       command                             Pointer to a CBWCB.
*       buffer                              Pointer to buffer to save
*                                           INQUIRY data.
*       buf_len                             Length of INQUIRY data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USBH_MS_TRANSPORT_ERROR          Indicates command didn't
*                                           complete successfully.
*       NU_USBH_MS_TRANSPORT_FAILED         Indicates command failed by the
*                                           media.
*       NU_USBH_MS_NOT_INITIALIZED          This driver hasn't been
*                                           initialized.
*
**************************************************************************/
STATUS  NU_USBH_MS_UFI_Inquiry(VOID     *handle,
                                UINT8   command[NU_UFI_CML_MAX],
                                VOID    *buffer,
                                UINT8   buf_len )
{
    STATUS  status;

    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(handle);
    NU_USB_PTRCHK_RETURN(command);

    status = NU_USBH_MS_NOT_INITIALIZED;

    /* Check whether this driver has already been initialized or not. */

    memset(command, 0, NU_UFI_CML_INQUIRY);

    /* Set command code. */
    command[0] = NU_UFI_CMD_INQUIRY;

    /* Set data length. */
    command[4] = buf_len;

    status = NU_USBH_MS_UFI_Request(
                    handle,             /* Handle.                   */
                    command,            /* Pointer to command block. */
                    NU_UFI_CML_INQUIRY, /* Length of CBWCB.          */
                    buffer,             /* Pointer to buffer.        */
                    buf_len,            /* Data length.              */
                    USB_DIR_IN);        /* Direction.                */

    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_MS_UFI_Test_Unit_Ready
*
* DESCRIPTION
*
*       Send TEST UNIT READY command to check if the logical unit is
*       ready.
*
* INPUTS
*
*       handle                              Handle.
*       command                             Pointer to a CBWCB.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USBH_MS_TRANSPORT_ERROR          Indicates Command doesn't
*                                           completed successfully.
*       NU_USBH_MS_TRANSPORT_FAILED         Indicates command failed by the
*                                           media.
*       NU_USBH_MS_NOT_INITIALIZED          This driver hasn't been
*                                           initialized.
*
**************************************************************************/
STATUS  NU_USBH_MS_UFI_Test_Unit_Ready(VOID *handle,
                                       UINT8 command[NU_UFI_CML_MAX])
{
    STATUS  status;

    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(handle);
    NU_USB_PTRCHK_RETURN(command);

    status = NU_USBH_MS_NOT_INITIALIZED;

    /* Check whether this driver has already been initialized or not. */


    memset(command, 0, NU_UFI_CML_TST_UNT_RDY);

    /* Set command code. */
    command[0] = NU_UFI_CMD_TST_UNT_RDY;

    status = NU_USBH_MS_UFI_Request(
                    handle,             /* Handle.                   */
                    command,            /* Pointer to command block. */
                    /* Length of CBWCB. */
                    NU_UFI_CML_TST_UNT_RDY,
                    NU_NULL,                  /* Pointer to buffer.        */
                    0,                  /* Data length.              */
                    USB_DIR_OUT);       /* Direction.                */

    NU_USER_MODE();

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       NU_USBH_MS_UFI_Read_Capacity
*
* DESCRIPTION
*
*       Get READ CAPACITY data which is constructed in 8bytes.
*       This API send READ CAPACITY command with "RELADR" bit of zero
*       and "PMI" bit of zero.
*
* INPUTS
*
*       handle                              Handle.
*       command                             Pointer to a CBWCB.
*       buffer                              Pointer to buffer to save READ
*                                           CAPACITY data. The buffer size
*                                           must be over 8 byte.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USBH_MS_TRANSPORT_ERROR          Indicates Command doesn't
*                                           completed successfully.
*       NU_USBH_MS_TRANSPORT_FAILED         Indicates command failed by the
*                                           media.
*       NU_USBH_MS_NOT_INITIALIZED          This driver hasn't been
*                                           initialized.
*
**************************************************************************/
STATUS  NU_USBH_MS_UFI_Read_Capacity( VOID *handle,
                                      UINT8 command[NU_UFI_CML_MAX],
                                      UINT8 buffer[NU_UFI_CML_RD_CAP] )
{
    STATUS  status;

    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(handle);
    NU_USB_PTRCHK_RETURN(command);

    status = NU_USBH_MS_NOT_INITIALIZED;

    /* Check whether this driver has already been initialized or not. */



    memset(command, 0, NU_UFI_CML_RD_CAP);

    /* Set command code. */
    command[0] = NU_UFI_CMD_RD_CAP;

    status = NU_USBH_MS_UFI_Request(
                    handle,             /* Handle.                   */
                    command,            /* Pointer to command block. */
                    NU_UFI_CML_RD_CAP,  /* Length of CBWCB.          */
                    buffer,             /* Pointer to buffer.        */
                    NU_UFI_DL_RD_CAP,   /* Data length.              */
                    USB_DIR_IN);        /* Direction.                */

    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_MS_UFI_Request_Sense
*
* DESCRIPTION
*
*       Get REQUEST SENSE data.
*
* INPUTS
*
*       handle                              Handle.
*       command                             Pointer to a CBWCB.
*       buffer                              Pointer to buffer to save
*                                           REQUEST SENSE data.
*       buf_len                             Length of REQUEST SENSE data.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USBH_MS_TRANSPORT_ERROR          Indicates Command doesn't
*                                           completed successfully.
*       NU_USBH_MS_TRANSPORT_FAILED         Indicates command failed by the
*                                           media.
*       NU_USBH_MS_NOT_INITIALIZED          This driver hasn't been
*                                           initialized.
*
**************************************************************************/
STATUS  NU_USBH_MS_UFI_Request_Sense( VOID    *handle,
                                      UINT8   command[NU_UFI_CML_MAX],
                                      VOID    *buffer,
                                      UINT8   buf_len )
{
    STATUS  status;

    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(handle);
    NU_USB_PTRCHK_RETURN(command);

    status = NU_USBH_MS_NOT_INITIALIZED;

    /* Check whether this driver has already been initialized or not. */


    memset(command, 0, NU_UFI_CML_REQ_SNS);

    /* Set command code. */
    command[0] = NU_UFI_CMD_REQ_SNS;

    /* Set data length. */
    command[4] = buf_len;               /* ALLOCATION LENGTH.        */

    status = NU_USBH_MS_UFI_Request(
                    handle,             /* Handle.                   */
                    command,            /* Pointer to command block. */
                    NU_UFI_CML_REQ_SNS, /* Length of CBWCB.          */
                    buffer,             /* Pointer to buffer.        */
                    buf_len,            /* Data length.              */
                    USB_DIR_IN);        /* Direction.                */

    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_MS_UFI_Read10
*
* DESCRIPTION
*
*       Read data by SFF-8070i read command.
*
* INPUTS
*
*       handle                              Handle.
*       command                             Pointer to a CBWCB.
*       block_addr                          Header address of Logical block
*                                           to read.
*       block_num                           The number of logical blocks to
*                                           read.
*       buffer                              Pointer to buffer to store read
*                                           data.
*       buf_len                             Data length to receive.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USBH_MS_TRANSPORT_ERROR          Indicates Command doesn't
*                                           complete successfully.
*       NU_USBH_MS_TRANSPORT_FAILED         Indicates command failed by the
*                                           media.
*       NU_USBH_MS_NOT_INITIALIZED          This driver hasn't been
*                                           initialized.
*
**************************************************************************/
STATUS  NU_USBH_MS_UFI_Read10(  VOID    *handle,
                                UINT8   command[NU_UFI_CML_MAX],
                                UINT32  block_addr,
                                UINT32  block_num,
                                VOID    *buffer,
                                UINT32  buf_len)
{
    STATUS  status;

    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(handle);
    NU_USB_PTRCHK_RETURN(command);

    status = NU_USBH_MS_NOT_INITIALIZED;

    /* Check whether this driver has already been initialized or not. */

    memset(command, 0, NU_UFI_CML_READ10);

    /* Set command code. */
    command[0] = NU_UFI_CMD_READ10;

    /* Set logical block address. */
    command[2] = (UINT8)(block_addr >> 24);
    command[3] = (UINT8)(block_addr >> 16);
    command[4] = (UINT8)(block_addr >> 8);
    command[5] = (UINT8)(block_addr >> 0);

    /* Set the number of logical blocks. */
    command[7] = (UINT8)(block_num >> 8);
    command[8] = (UINT8)(block_num >> 0);

    status = NU_USBH_MS_UFI_Request(
                    handle,             /* Handle.                   */
                    command,            /* Pointer to command block. */
                    NU_UFI_CML_READ10,  /* Length of CBWCB.          */
                    buffer,             /* Pointer to buffer.        */
                    buf_len,            /* Data length.              */
                    USB_DIR_IN);        /* Direction.                */

    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBH_MS_UFI_Write10
*
* DESCRIPTION
*       Write data by UFI read command.
*
* INPUTS
*
*       handle                              Handle.
*       command                             Pointer to a CBWCB.
*       block_addr                          Header address of Logical block
*                                           to write.
*       block_num                           The number of logical blocks to
*                                           write.
*       buffer                              Pointer to buffer in which data
*                                           to be send is saved.
*       buf_len                             Data length to send.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_USBH_MS_TRANSPORT_ERROR          Indicates Command doesn't
*                                           completed successfully.
*       NU_USBH_MS_TRANSPORT_FAILED         Indicates command failed by the
*                                           media.
*       NU_USBH_MS_NOT_INITIALIZED          This driver hasn't been
*                                           initialized.
*
**************************************************************************/
STATUS  NU_USBH_MS_UFI_Write10(VOID     *handle,
                                UINT8   command[NU_UFI_CML_MAX],
                                UINT32  block_addr,
                                UINT32  block_num,
                                VOID    *buffer,
                                UINT32  buf_len )
{
    STATUS  status;

    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(handle);
    NU_USB_PTRCHK_RETURN(command);

    status = NU_USBH_MS_NOT_INITIALIZED;

    /* Check whether this driver has already been initialized or not. */


    memset(command, 0, NU_UFI_CML_WRITE10);

    /* Set command code. */
    command[0] = NU_UFI_CMD_WRITE10;

    /* Set logical block address. */
    command[2] = (UINT8)(block_addr >> 24);
    command[3] = (UINT8)(block_addr >> 16);
    command[4] = (UINT8)(block_addr >> 8);
    command[5] = (UINT8)(block_addr >> 0);

    /* Set the number of logical blocks. */
    command[7] = (UINT8)(block_num >> 8);
    command[8] = (UINT8)(block_num >> 0);

    status = NU_USBH_MS_UFI_Request(
                    handle,             /* Handle.                   */
                    command,            /* Pointer to command block. */
                    NU_UFI_CML_WRITE10, /* Length of CBWCB.          */
                    buffer,             /* Pointer to buffer.        */
                    buf_len,            /* Data length.              */
                    USB_DIR_OUT);       /* Direction.                */

    NU_USER_MODE();

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       NU_USBH_MS_UFI_Request
*
* DESCRIPTION
*       Request to transfer a UFI command.
*
* INPUTS
*
*       handle                              Handle.
*       command                             Pointer to a CBWCB.
*       cmd_len                             Length of a CBWCB.
*       buffer                              Pointer to buffer in which data
*                                           to be transferred is saved.
*       buf_len                             Data length to transfer.
*       direct                              Direction of a transfer.
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
STATUS  NU_USBH_MS_UFI_Request (VOID *handle, VOID *command,
                                UINT32 cmd_len, VOID *buffer,
                                UINT32 buf_len, UINT8 direct)
{
    STATUS status;
    UINT8   *tmp_cmd = command;
    NU_USBH_MS* ms_drvr;

    NU_USB_PTRCHK(handle);
    NU_USB_PTRCHK(command);

    /* Set logical unit number. */
    *(tmp_cmd + 1) = (((MS_LUN *)handle)->lun & 0x07) << 4;

    status = NU_USBH_MS_Init_GetHandle((VOID**)&ms_drvr);

    if (status == NU_SUCCESS)
    {
        status = NU_USBH_MS_Transport (ms_drvr,
                                       NU_NULL,
                                      (handle),
                                      (command),
                                      (cmd_len),
                                      (buffer),
                                      (buf_len),
                                      (direct));
    }

    return (status);
}

#endif      /* INCLUDE_UFI */
#endif /*_NU_USBH_MS_UFI_IMP_C*/
/* ======================  End Of File  ================================ */
