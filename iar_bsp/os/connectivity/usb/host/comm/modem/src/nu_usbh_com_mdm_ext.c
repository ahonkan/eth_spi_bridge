/**************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
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
*     nu_usbh_com_mdm_ext.c
*
* COMPONENT
*     Nucleus USB software : Communication user driver.
*
* DESCRIPTION
*     This file contains the external Interfaces exposed by modem user
*     driver.
*
* DATA STRUCTURES
*     None
*
* FUNCTIONS
*
*     NU_USBH_COM_MDM_Create              Initializes the control block of
*                                         Communication Modem user driver.
*     _NU_USBH_COM_MDM_Delete             Deletes an instance of
*                                         Communication Modem user driver.
*     _NU_USBH_COM_MDM_Connect_Handler    Notifies a new device connection
*                                         which can be served by
*                                         Communication modem user driver.
*     _NU_USBH_COM_MDM_Disconnect_Handler Notifies a modem disconnection.
*     _NU_USBH_COM_MDM_Intr_Handler       Asynchronous response handler
*                                         called when interrupt data is
*                                         received for modem.
*     NU_USBH_COM_MODEM_Init              Initialization routine for auto
*                                         initialization.
*     NU_USBH_COM_MDM_GetHandle           Service to retrieve internal
*                                         modem driver pointer.
*     NU_USBH_MDM_Reg_Event_Handler       Service to register event report
*                                         callback.
*
* DEPENDENCIES
*     nu_usb.h              All USB definitions.
*
**************************************************************************/

/* =====================  USB Include Files ===========================  */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "services/runlevel_init.h"

/* ==========================  Functions =============================== */

/*************************************************************************
*   FUNCTION
*
*       nu_os_conn_usb_host_comm_mdm_init
*
*   DESCRIPTION
*
*       This function initializes the Modem User driver component.
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
STATUS nu_os_conn_usb_host_comm_mdm_init(CHAR *path, INT compctrl)
{
    VOID   *usbh_comm_handle = NU_NULL;
    UINT8   rollback = 0;
    STATUS  status = NU_SUCCESS, internal_sts = NU_SUCCESS;

    if(compctrl == RUNLEVEL_START)
    {
        /* Allocate memory for Modem user driver control block. */
        status = USB_Allocate_Object(sizeof(NU_USBH_COM_MDM),
                                     (VOID **)&NU_USBH_COM_MDM_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 1;
        }

        /* Create the device subsystem. */
        if ((!rollback))
        {
            /* Zero out allocated block. */
            memset(NU_USBH_COM_MDM_Cb_Pt, 0, sizeof(NU_USBH_COM_MDM));

            /* In following API call, passing memory pool ptr parameter
             * NU_NULL because in ReadyStart memory in USB system is
             * allocated through USB specific memory APIs, not directly
             * with any given memory pool pointer. This parameter remains
             * only for backwards code compatibility. */
            status = NU_USBH_COM_MDM_Create(NU_USBH_COM_MDM_Cb_Pt,
                                            "MDMDRVR",
                                            NU_NULL,
                                            NU_NULL);
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        if (!rollback)
        {
            /* Get the host comm class driver handle. */
            status = NU_USBH_COMM_Init_GetHandle(&usbh_comm_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        if (!rollback)
        {
            /* Register the users to the class driver */
            status = NU_USBH_COM_USER_Register ((NU_USBH_COM_MDM*) NU_USBH_COM_MDM_Cb_Pt,
                                                (NU_USBH_COM*) usbh_comm_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        /* Clean up in case error occurs. */
        switch (rollback)
        {
            case 3:
                internal_sts = _NU_USBH_COM_MDM_Delete ((VOID *) NU_USBH_COM_MDM_Cb_Pt);

            case 2:
                if (NU_USBH_COM_MDM_Cb_Pt)
                {
                    internal_sts |= USB_Deallocate_Memory((VOID *)NU_USBH_COM_MDM_Cb_Pt);
                    NU_USBH_COM_MDM_Cb_Pt = NU_NULL;
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
        USB_Deallocate_Memory(NU_USBH_COM_MDM_Cb_Pt);
        _NU_USBH_COM_MDM_Delete (NU_USBH_COM_MDM_Cb_Pt);
        status = NU_SUCCESS;
    }

    return (status);
}

/**************************************************************************
* FUNCTION
*
*     NU_USBH_COM_MDM_Create
*
* DESCRIPTION
*
*     Communication user driver initialization routine
*
* INPUTS
*
*     pcb_user_drvr    pointer to driver control block.
*     p_name           Name of this USB object.
*     p_memory_pool    Memory pool to be used by user driver.
*
* OUTPUTS
*
*     NU_SUCCESS            Indicates successful completion.
*     NU_INVALID_SEMAPHORE  Indicates control block is invalid.
*     NU_INVALID_GROUP      Indicates control block is invalid.
*
**************************************************************************/
STATUS NU_USBH_COM_MDM_Create (
       NU_USBH_COM_MDM*       pcb_user_drvr,
       CHAR*                  p_name,
       NU_MEMORY_POOL*        p_memory_pool,
       NU_USBH_COM_MDM_HANDL* p_handlers)
{
    STATUS  status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_MEMPOOLCHK_RETURN(p_memory_pool);

    /* Create base component. */
    status = _NU_USBH_COM_USER_Create(pcb_user_drvr,
                                    p_name,
                                    p_memory_pool,
                                    0x02,
                                    p_handlers,
                       (NU_USBH_COM_USER_DISPATCH*)&usbh_com_mdm_dispatch);

    /* Revert to user mode. */
    NU_USER_MODE();

    return(status);
}

/**************************************************************************
* FUNCTION
*
*     _NU_USBH_COM_MDM_Delete
*
* DESCRIPTION
*
*     This function deletes an instance of Communication user driver .
*
* INPUTS
*
*     pcb_user_drvr       Pointer to the USB Object control block.
*
* OUTPUTS
*
*     NU_SUCCESS          Indicates successful completion.
*     NU_INVALID_POINTER  Indicates control block is invalid.
*
**************************************************************************/
STATUS _NU_USBH_COM_MDM_Delete (VOID* pcb_user_drvr)
{
    STATUS  status;
    NU_USBH_COM_MDM* pcb_mdm_drvr = (NU_USBH_COM_MDM*)pcb_user_drvr;

    NU_USBH_COM_MDM_DEVICE* pcb_next_device;
    NU_USBH_COM_MDM_DEVICE* pcb_curr_device;

    pcb_curr_device = (NU_USBH_COM_MDM_DEVICE*)
                      (pcb_mdm_drvr->pcb_first_device);

    while (pcb_curr_device)
    {
        pcb_next_device =(NU_USBH_COM_MDM_DEVICE*)
        (pcb_curr_device->user_device.node.cs_next);

        /* Sends disconnect event, for each MOD device. */
        _NU_USBH_COM_MDM_Disconnect_Handler(
                                 (NU_USB_USER*) pcb_mdm_drvr,
                                 (NU_USB_DRVR*)
                                 pcb_curr_device->user_device.class_drvr,
                                 pcb_curr_device);

        /* All devices delisted from the stack */
        if(pcb_curr_device == pcb_next_device)
        {
            break;
        }
        pcb_curr_device = pcb_next_device;
    }

    /* Delete all OS acquired resources. */
    /* Deleting the base component. */
    status = _NU_USBH_COM_USER_Delete (pcb_user_drvr);

    return status;
}

/**************************************************************************
* FUNCTION
*
*     _NU_USBH_COM_MDM_Connect_Handler
*
* DESCRIPTION
*
*     This function is called by the class driver whenever there is a MOD
*     communication device connected to USB host.
*
* INPUTS
*
*     pcb_user            Pointer to control block of user driver.
*     pcb_drvr            Pointer to control block of class driver.
*     pcb_curr_device     Pointer to connected device.
*     information         Pointer to control block of device information
*
* OUTPUTS
*
*     NU_SUCCESS            Indicates successful completion.
*     NU_USB_INVLD_ARG      Some pointer became stale before call
*                           completion.
*
**************************************************************************/
STATUS _NU_USBH_COM_MDM_Connect_Handler(
       NU_USB_USER*  pcb_user,
       NU_USB_DRVR*  pcb_drvr,
       VOID*         pcb_curr_device,
       VOID*         information)
{
    STATUS  status;
    NU_USBH_COM_MDM* pcb_mdm_drvr = (NU_USBH_COM_MDM*)pcb_user;
    NU_USBH_COM_MDM_DEVICE*  pcb_mdm_dev;
    UINT8 rollback = 0x00;

    /* Sending connection notification to application. */

    status = USB_Allocate_Object (sizeof (NU_USBH_COM_MDM_DEVICE),
                                 (VOID **) &pcb_mdm_dev);
    if(status == NU_SUCCESS)
    {
        memset (pcb_mdm_dev,
                0,
                sizeof (NU_USBH_COM_MDM_DEVICE));

        pcb_mdm_dev->inform_str = *((NU_USBH_COM_ACM_INFORM*)information);

        pcb_mdm_dev->user_device.usb_device = pcb_curr_device;
        pcb_mdm_dev->user_device.user_drvr  = pcb_user;
        pcb_mdm_dev->user_device.class_drvr = pcb_drvr;

        /* Since the function is over written by Communication user driver,
         * therefore calling the base behavior for its internal
         * requirements. */

        status = _NU_USBH_USER_Connect(pcb_user,pcb_drvr,pcb_mdm_dev);

        if(status == NU_SUCCESS)
        {
            NU_Place_On_List ((CS_NODE **)&pcb_mdm_drvr->pcb_first_device,
                               (CS_NODE *) pcb_mdm_dev);

            if(pcb_mdm_drvr->p_hndl_table)
            {
                if(pcb_mdm_drvr->p_hndl_table->Connect_Handler)
                {
                    pcb_mdm_drvr->p_hndl_table->Connect_Handler(pcb_user,
                                                                pcb_mdm_dev,
                                                                information);
                }
                /* This will be called when callback functions are already
                 * registered with host modem driver (after first connection). */
                status = NU_USB_SYS_Register_Device(pcb_mdm_dev,
                                                    NU_USBCOMPH_MODEM);
                if(status != NU_SUCCESS)
                {
                    rollback = 0x02;
                }
            }
            else
            {
                /* This will be called when callback functions are NOT already
                 * registered with host modem driver (first time connection). */
                status = NU_USB_SYS_Register_Device(pcb_mdm_dev,
                                                    NU_USBCOMPH_MODEM);
                if(status != NU_SUCCESS)
                {
                    rollback = 0x02;
                }
            }
        }
        else
        {
            rollback = 0x01;
        }
    }

    switch(rollback)
    {
        case 0x02: NU_Remove_From_List((CS_NODE **) &pcb_mdm_drvr->pcb_first_device,
                                       (CS_NODE *) pcb_mdm_dev);

        case 0x01: USB_Deallocate_Memory(pcb_mdm_dev);

        default: break;
    }

    return (status);

}
/**************************************************************************
* FUNCTION
*
*     _NU_USBH_COM_MDM_Disconnect_Handler
*
* DESCRIPTION
*
*     This function is called by the class driver whenever there is a modem
*     communication device connected to USB host.
*
* INPUTS
*
*     pcb_user            Pointer to control block of user driver.
*     pcb_drvr            Pointer to control block of class driver.
*     pcb_curr_device     Pointer to connected device.
*     information         Pointer to control block of device information
*
* OUTPUTS
*
*     NU_SUCCESS            Indicates successful completion.
*     NU_USB_INVLD_ARG      Some pointer became stale before call
*                           completion.
*
**************************************************************************/
STATUS _NU_USBH_COM_MDM_Disconnect_Handler(
       NU_USB_USER*  pcb_user_drvr,
       NU_USB_DRVR*  pcb_com_drvr,
       VOID*         pcb_device)
{
    NU_USBH_COM_MDM        *pcb_mdm_drvr = (NU_USBH_COM_MDM*)
                                            pcb_user_drvr;
    NU_USBH_COM_MDM_DEVICE *pcb_curr_device = pcb_mdm_drvr->
                                              pcb_first_device;

    STATUS status = NU_USB_INVLD_ARG;
    /* Scan the list of devices and cleanup all associated ones. */
    while (pcb_curr_device)
    {
        /* If this Drive is Associated with the device disconnected... */
        if (pcb_curr_device->user_device.usb_device == pcb_device)
        {

            /* If connect callback is present then use directly disconnect
             * callback to send disconnection to application.
             * Otherwise use deregister function of DM so that it can
             * close device. */
            if(pcb_mdm_drvr->p_hndl_table)
            {
                if(pcb_mdm_drvr->p_hndl_table->Disconnect_Handler)
                {
                    pcb_mdm_drvr->p_hndl_table->Disconnect_Handler(pcb_user_drvr,
                                                                  pcb_curr_device);
                }
                else
                {
                    NU_USB_SYS_DeRegister_Device (pcb_curr_device,
                                                  NU_USBCOMPH_MODEM);
                }
            }

            /* Sends disconnection interrupt to user driver. */
            _NU_USBH_USER_Disconnect ((NU_USB_USER*)pcb_user_drvr,
                                      (NU_USB_DRVR*)pcb_com_drvr,
                                      pcb_curr_device);
            /* Remove the device structure from the List... */

            NU_Remove_From_List(
             (CS_NODE **) & pcb_mdm_drvr->pcb_first_device,
             (CS_NODE *) pcb_curr_device);

            /* ...and deallocate device structure. */
            USB_Deallocate_Memory (pcb_curr_device);
            status = NU_SUCCESS;
            break;
        }
        else
        {
            if(pcb_curr_device == (NU_USBH_COM_MDM_DEVICE*)
                                 pcb_curr_device->user_device.node.cs_next)
            {
                status = NU_USB_INVLD_ARG;
                break;
            }
            pcb_curr_device = (NU_USBH_COM_MDM_DEVICE*)
                               pcb_curr_device->user_device.node.cs_next;
        }
    }

    return (status);
}

/**************************************************************************
* FUNCTION
*
*     _NU_USBH_COM_MDM_Intr_Handler
*
* DESCRIPTION
*
*     This function is called by the class driver whenever there is a valid
*     response present at communication device.
*
* INPUTS
*
*     pcb_curr_device       Pointer to Communication device with response.
*     pcb_xblock            Pointer to transfer block to hold data size and
*                           pointer.
*
* OUTPUTS
*
*     NU_SUCCESS            Indicates successful completion.
*     NU_NOT_PRESENT        Modem cookie corrupted.
*
**************************************************************************/
STATUS _NU_USBH_COM_MDM_Intr_Handler(
       VOID*               pcb_curr_device,
       NU_USBH_COM_XBLOCK* pcb_xblock)
{
    STATUS status = NU_SUCCESS;
    NU_USBH_COM_MDM* pcb_mdm_drvr;
    NU_USBH_COM_MDM_DEVICE* modem;

    pcb_mdm_drvr = (NU_USBH_COM_MDM*)((NU_USBH_COM_DEVICE*)pcb_curr_device)
                                  ->pcb_user_drvr;
    modem = (NU_USBH_COM_MDM_DEVICE*)pcb_mdm_drvr->pcb_first_device;

    if(modem == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    if(status == NU_SUCCESS)
    {
        while(modem->user_device.usb_device != pcb_curr_device)
        {
            if(modem == (NU_USBH_COM_MDM_DEVICE*)
                         modem->user_device.node.cs_next)
            {
                status = NU_NOT_PRESENT;
                break;
            }
            else
            {
                modem = (NU_USBH_COM_MDM_DEVICE*)
                         modem->user_device.node.cs_next;
            }
        }
    }

    if(status == NU_SUCCESS)
    {
        if(pcb_mdm_drvr->p_hndl_table)
        {
            if(pcb_mdm_drvr->p_hndl_table->Event_Handler)
            {
                pcb_mdm_drvr->p_hndl_table->Event_Handler(modem,
                                                          pcb_xblock);
            }
        }
    }

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_COM_MDM_Init_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the host modem user driver's
*       address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the user
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates there exists a host modem
*                           user driver.
*       NU_NOT_PRESENT      Indicates there exists no user driver.
*
*************************************************************************/
STATUS NU_USBH_COM_MDM_GetHandle(VOID  **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBH_COM_MDM_Cb_Pt;
    if (NU_USBH_COM_MDM_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_MDM_Reg_Event_Handler
*
* DESCRIPTION
*
*       Registers the pointer to application's structure for handlers.
*
*
* INPUTS
*
*       cb          Pointer to modem driver control block.
*       func        Pointer to the event handler.
*
* OUTPUTS
*
*       NU_SUCCESS  Indicates successful completion.
*
*************************************************************************/
STATUS NU_USBH_MDM_Reg_Event_Handler (
                                NU_USBH_COM_MDM *   cb,
                                NU_USBH_COM_MDM_HANDL* p_handlers)
{
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    cb->p_hndl_table = p_handlers;

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_MDM_DM_Open
*
* DESCRIPTION
*
*       This function is called by the application when it opens a device
*       for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the modem driver passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBH_MDM_DM_Open (VOID* dev_handle)
{
    STATUS status = NU_SUCCESS;
    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_MDM_DM_Close
*
* DESCRIPTION
*
*       This function is called by the application when it wants to close a device
*       which it has opend already for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the modem driver passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBH_MDM_DM_Close(VOID* dev_handle)
{
    STATUS status = NU_SUCCESS;
    NU_USBH_COM_MDM_DEVICE*  pcb_mdm_dev = (NU_USBH_COM_MDM_DEVICE*)dev_handle;
    NU_USBH_COM_MDM* pcb_mdm_drvr = (NU_USBH_COM_MDM *) pcb_mdm_dev->user_device.user_drvr;

    /* Update application about disconnection. */
    if(pcb_mdm_drvr->p_hndl_table)
    {
        if(pcb_mdm_drvr->p_hndl_table->Disconnect_Handler)
        {
            pcb_mdm_drvr->p_hndl_table->Disconnect_Handler((VOID *)pcb_mdm_drvr,
                                                           dev_handle);
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_MDM_DM_Read
*
* DESCRIPTION
*
*       This function is called by the application when it wants to read
*       data from the device.
*
* INPUTS
*
*       dev_handle         Pointer to the modem driver passed as context.
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
STATUS  NU_USBH_MDM_DM_Read(VOID*     dev_handle,
                            VOID*     buffer,
                            UINT32    numbyte,
                            OFFSET_T  byte_offset,
                            UINT32*   bytes_read_ptr)
{
    STATUS status = NU_SUCCESS;
    NU_USBH_COM_MDM_DEVICE*  pcb_mdm_dev = NU_NULL;

    pcb_mdm_dev = (NU_USBH_COM_MDM_DEVICE*)dev_handle;

    status = NU_USBH_COM_USER_Get_Data (
                        &pcb_mdm_dev->user_device,
                        (UINT8*) ((UINT32)buffer),
                        numbyte,
                        bytes_read_ptr);

    return (status);
}
/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_MDM_DM_Write
*
* DESCRIPTION
*
*       This function is called by the application when it wants to write
*       data from the device.
*
* INPUTS
*
*       dev_handle         Pointer to the modem driver passed as context.
*       buffer             Pointer to memory location where data to be written is avaiable.
*       numbyte            Number of bytes to be written.
*       byte_offset        In case data is to be read at certain offset in the buffer.
*       bytes_written_ptr  OUTPUT: Number of bytes actually written.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBH_MDM_DM_Write (VOID*     dev_handle,
                              const VOID*     buffer,
                              UINT32    numbyte,
                              OFFSET_T  byte_offset,
                              UINT32*   bytes_written_ptr)
{
    STATUS status = NU_SUCCESS;
    NU_USBH_COM_MDM_DEVICE*  pcb_mdm_dev = NU_NULL;

    pcb_mdm_dev = (NU_USBH_COM_MDM_DEVICE*)dev_handle;

    status = NU_USBH_COM_USER_Send_Data (
                        &pcb_mdm_dev->user_device,
                        (UINT8*) ((UINT32)buffer),
                        numbyte);

    *bytes_written_ptr = numbyte;

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_MDM_DM_IOCTL
*
* DESCRIPTION
*
*       This function is called by the application when it wants to perform a control
*       operation on the device.
*
* INPUTS
*
*       dev_handle         Pointer to the modem driver passed as context.
*       cmd                IOCTL number.
*       data               IOCTL data pointer of variable type.
*       length             IOCTL data length in bytes.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBH_MDM_DM_IOCTL(VOID*     dev_handle,
                             INT       ioctl_num,
                             VOID*     ioctl_data,
                             INT       ioctl_data_len)
{
    STATUS status = NU_SUCCESS;
    NU_USBH_COM_MDM_DEVICE*  pcb_mdm_dev = (NU_USBH_COM_MDM_DEVICE*)dev_handle;
    NU_USBH_COM_MDM* pcb_mdm_drvr = (NU_USBH_COM_MDM *) pcb_mdm_dev->user_device.user_drvr;

    switch(ioctl_num)
    {
        /********************************
         * USB Host Modem User IOCTLS   *
         ********************************/
        case (NU_USBH_MDM_IOCTL_BASE + NU_USBH_MDM_IOCTL_REG_EVT_HDL):
            status = NU_USBH_MDM_Reg_Event_Handler(pcb_mdm_drvr,
                                                  (NU_USBH_COM_MDM_HANDL *)ioctl_data);
            break;

        case (NU_USBH_MDM_IOCTL_BASE + NU_USBH_MDM_IOCTL_GET_DEV_CB):
            *(void **)ioctl_data = (VOID *)pcb_mdm_dev;
            status = NU_SUCCESS;
            break;

        case (NU_USBH_MDM_IOCTL_BASE + NU_USBH_MDM_IOCTL_SET_RX_DATA_BUF):
            status = NU_USBH_MDM_Set_Rx_Data_Buffer(pcb_mdm_dev, (VOID*) ioctl_data);
            break;

        case (NU_USBH_MDM_IOCTL_BASE + NU_USBH_MDM_IOCTL_SET_RX_DATA_LEN):
            status = NU_USBH_MDM_Set_Rx_Data_Length(pcb_mdm_dev, *(UINT32 *) ioctl_data);
            break;

        case (NU_USBH_MDM_IOCTL_BASE + NU_USBH_MDM_IOCTL_SET_RX_DATA_TRFR_LEN):
            status = NU_USBH_MDM_Set_Rx_Data_Transfer_Length(pcb_mdm_dev, *(UINT32 *) ioctl_data);
            break;

        case (NU_USBH_MDM_IOCTL_BASE + NU_USBH_MDM_IOCTL_GET_RX_DATA_TRFR_LEN):
            status = NU_USBH_MDM_Get_Rx_Data_Transfer_Length(pcb_mdm_dev, (UINT32 *) ioctl_data);
            break;

        /********************************
         * USB Host COMM User IOCTLS    *
         ********************************/
        case (NU_USBH_MDM_IOCTL_BASE + NU_USBH_MDM_IOCTL_CREATE_POLL):
            status = NU_USBH_COM_USER_Create_Polling((NU_USBH_COM_USER *) pcb_mdm_drvr,
                                                     (NU_USBH_COM_USR_DEVICE *) pcb_mdm_dev);
            break;

        case (NU_USBH_MDM_IOCTL_BASE + NU_USBH_MDM_IOCTL_START_POLL):
            status = NU_USBH_COM_USER_Start_Polling((NU_USBH_COM_USR_DEVICE *) pcb_mdm_dev);
            break;

        case (NU_USBH_MDM_IOCTL_BASE + NU_USBH_MDM_IOCTL_STOP_POLL):
            status = NU_USBH_COM_USER_Stop_Polling((NU_USBH_COM_USR_DEVICE *) pcb_mdm_dev);
            break;

        case (NU_USBH_MDM_IOCTL_BASE + NU_USBH_MDM_IOCTL_DELETE_POLL):
            status = NU_USBH_COM_USER_Delete_Polling((NU_USBH_COM_USR_DEVICE *) pcb_mdm_dev);
            break;

        /********************************
         * USB Host COMM IOCTLS         *
         ********************************/
         case (NU_USBH_MDM_IOCTL_BASE + NU_USBH_MDM_IOCTL_GET_LINE_CODE):
            status = NU_USBH_COM_Get_Line_Coding(
                                              ((NU_USBH_COM_USR_DEVICE *) pcb_mdm_dev)->usb_device,
                                              ioctl_data);
            break;

        case (NU_USBH_MDM_IOCTL_BASE + NU_USBH_MDM_IOCTL_SET_LINE_CODE):
            status = NU_USBH_COM_Set_Line_Coding(
                                               ((NU_USBH_COM_USR_DEVICE *) pcb_mdm_dev)->usb_device,
                                               ioctl_data);
            break;

        default:
        {
            status = NU_USB_NOT_SUPPORTED;
            break;
        }
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_MDM_Set_Rx_Data_Buffer
*
* DESCRIPTION
*
*       Sets the pointer for receive data buffer.
*
*
* INPUTS
*
*       cb          Pointer to modem device control block.
*       data_buf    Pointer to rx data buffer.
*
* OUTPUTS
*
*       NU_SUCCESS  Indicates successful completion.
*
*************************************************************************/
STATUS NU_USBH_MDM_Set_Rx_Data_Buffer(NU_USBH_COM_MDM_DEVICE *   cb,
                                      VOID* data_buffer)
{
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    cb->user_device.rx_xblock.p_data_buf = data_buffer;

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_MDM_Set_Rx_Data_Length
*
* DESCRIPTION
*
*       Sets the totoal length for receive data buffer.
*
*
* INPUTS
*
*       cb          Pointer to modem device control block.
*       data_length Length of rx data buffer.
*
* OUTPUTS
*
*       NU_SUCCESS  Indicates successful completion.
*
*************************************************************************/
STATUS NU_USBH_MDM_Set_Rx_Data_Length(NU_USBH_COM_MDM_DEVICE *   cb,
                                      UINT32 data_length)
{
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    cb->user_device.rx_xblock.data_length = data_length;

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_MDM_Set_Rx_Data_Transfer_Length
*
* DESCRIPTION
*
*       Gets the totoal length for receive data buffer.
*
*
* INPUTS
*
*       cb          Pointer to modem device control block.
*       data_length Length of rx data buffer.
*
* OUTPUTS
*
*       NU_SUCCESS  Indicates successful completion.
*
*************************************************************************/
STATUS NU_USBH_MDM_Set_Rx_Data_Transfer_Length(NU_USBH_COM_MDM_DEVICE *   cb,
                                               UINT32 transfer_length)
{
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    cb->user_device.rx_xblock.transfer_length = transfer_length;

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_MDM_Get_Rx_Data_Transfer_Length
*
* DESCRIPTION
*
*       Gets the totoal length for receive data buffer.
*
*
* INPUTS
*
*       cb          Pointer to modem device control block.
*       data_length Length of rx data buffer.
*
* OUTPUTS
*
*       NU_SUCCESS  Indicates successful completion.
*
*************************************************************************/
STATUS NU_USBH_MDM_Get_Rx_Data_Transfer_Length(NU_USBH_COM_MDM_DEVICE *   cb,
                                               UINT32* transfer_length)
{
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    *transfer_length = cb->user_device.rx_xblock.transfer_length;

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}

/* ======================  End Of File  ================================ */
