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
*       nu_usbh_com_eth_ext.c
*
*
* COMPONENT
*       Nucleus USB software : Communication user driver.
*
* DESCRIPTION
*       This file contains the external Interfaces exposed by
*       ethernet user driver.
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*
*        NU_USBH_COM_ETH_Create             Initializes the control block
*                                           of Communication ethernet user
*                                           driver.
*       _NU_USBH_COM_ETH_Delete             Deletes an instance of
*                                           Communication ethernet user
*                                           driver.
*       _NU_USBH_COM_ETH_Connect_Handler    Notifies a new device
*                                           connection which can be served
*                                           by Communication ethernet user
*                                           driver.
*       _NU_USBH_COM_ETH_Disconnect_Handler Notifies a device disconnection
*                                           which was being served by
*                                           Communication ethernet user
*                                           driver.
*       _NU_USBH_COM_ETH_Intr_Handler       Asynchronous response handler.
*       NU_USBH_COMM_ETH_Init               Initialization routine for auto
*                                           initialization.
*       NU_USBH_COM_ETH_GetHandle           Service to retrieve internal
*                                           ethernet driver pointer.
*       NU_USBH_COM_ETH_Reg_Hndlr           Service to register event
*                                           report callback.
*
* DEPENDENCIES
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* =====================  USB Include Files ===========================  */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "services/runlevel_init.h"

/* ==========================  Functions =============================== */

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_ETH_Create
*
* DESCRIPTION
*     Communication user driver initialization routine
*
* INPUTS
*     pcb_user_drvr    pointer to driver control block.
*     p_name           Name of this USB object.
*     p_memory_pool    Memory pool to be used by user driver.
*     p_handlers       Application event handler.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*     NU_INVALID_SEMAPHORE  Indicates control block is invalid.
*     NU_INVALID_GROUP      Indicates control block is invalid.
*
**************************************************************************/
STATUS NU_USBH_COM_ETH_Create (
       NU_USBH_COM_ETH*       pcb_user_drvr,
       CHAR*                  p_name,
       NU_MEMORY_POOL*        p_memory_pool,
       NU_USBH_COM_ETH_HANDL* p_handlers)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_MEMPOOLCHK_RETURN(p_memory_pool);

    /* Create base component. */
    status = _NU_USBH_COM_USER_Create(
             pcb_user_drvr,
             p_name,
             p_memory_pool,
             0x06,
             p_handlers,
             (NU_USBH_COM_USER_DISPATCH*)&usbh_com_eth_dispatch);

    /* Revert to user mode. */
    NU_USER_MODE();

    return(status);
}

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_ETH_Delete
*
* DESCRIPTION
*     This function deletes an instance of Communication user driver.
*
* INPUTS
*     pcb_user_drvr       Pointer to the USB Object control block.
*
* OUTPUTS
*     NU_SUCCESS          Indicates successful completion.
*     NU_INVALID_POINTER  Indicates control block is invalid.
*
**************************************************************************/
STATUS _NU_USBH_COM_ETH_Delete (VOID* pcb_user_drvr)
{
    STATUS status;

    NU_USBH_COM_ETH* pcb_eth_drvr = (NU_USBH_COM_ETH*)pcb_user_drvr;

    NU_USBH_COM_ETH_DEVICE* pcb_next_device;
    NU_USBH_COM_ETH_DEVICE* pcb_curr_device;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    pcb_curr_device = (NU_USBH_COM_ETH_DEVICE*)
                      (pcb_eth_drvr->pcb_first_device);

    while (pcb_curr_device)
    {
        pcb_next_device = (NU_USBH_COM_ETH_DEVICE*)(pcb_curr_device->
                          user_device.node.cs_next);

        /* Sends disconnect event, for each ethernet device. */
        _NU_USBH_COM_ETH_Disconnect_Handler(
                                 (NU_USB_USER*) pcb_eth_drvr,
                                 pcb_curr_device->user_device.class_drvr,
                                 pcb_curr_device);

        /* All devices are delisted from the stack */
        if(pcb_curr_device == pcb_next_device)
        {
            break;
        }
        pcb_curr_device = pcb_next_device;
    }

    /* Delete all OS acquired resources. */
    /* Deleting the base component. */
    status = _NU_USBH_COM_USER_Delete (pcb_user_drvr);

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*     _NU_USBH_COM_ETH_Connect_Handler
*
* DESCRIPTION
*     This function is called by the class driver whenever there is an
*     ethernet communication device connected to USB host.
*
* INPUTS
*     pcb_user            Pointer to control block of user driver.
*     pcb_drvr            Pointer to control block of class driver.
*     pcb_curr_device     Pointer to connected device.
*     information         Pointer to control block of device information
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*     NU_USB_INVLD_ARG      Some pointer became stale before call
*                           completion.
*
**************************************************************************/

STATUS _NU_USBH_COM_ETH_Connect_Handler(
       NU_USB_USER*  pcb_user,
       NU_USB_DRVR*  pcb_drvr,
       VOID*         pcb_curr_device,
       VOID*         information)
{
    STATUS status;
    NU_USBH_COM_ETH* pcb_eth_drvr = (NU_USBH_COM_ETH*)pcb_user;
    NU_USBH_COM_ETH_DEVICE*  pcb_eth_dev = NU_NULL;
    UINT8 rollback = 0x00;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Sending connection notification to application. */

    status = USB_Allocate_Object (sizeof (NU_USBH_COM_ETH_DEVICE),
                                  (VOID **) &pcb_eth_dev);
    if(status == NU_SUCCESS)
    {
        memset (pcb_eth_dev,
                0,
                sizeof (NU_USBH_COM_ETH_DEVICE));

        pcb_eth_dev->inform_str = *((NU_USBH_COM_ECM_INFORM*)information);
        pcb_eth_dev->user_device.usb_device = pcb_curr_device;
        pcb_eth_dev->user_device.user_drvr  = pcb_user;
        pcb_eth_dev->user_device.class_drvr = pcb_drvr;

        /* Since the function is over written by Communication user driver,
         * therefore calling the base behavior for its internal
         * requirements.
         */
        status = _NU_USBH_USER_Connect(pcb_user,pcb_drvr,pcb_eth_dev);

        if(status == NU_SUCCESS)
        {
            NU_Place_On_List ((CS_NODE **) &pcb_eth_drvr->pcb_first_device,
                               (CS_NODE *) pcb_eth_dev);

            if(pcb_eth_drvr->p_hndl_table)
            {
                if(pcb_eth_drvr->p_hndl_table->Connect_Handler)
                {

                     pcb_eth_drvr->p_hndl_table->Connect_Handler(pcb_user,
                                             pcb_eth_dev,
                                             information);
                }
            }
            else
            {
                status = NU_USB_SYS_Register_Device(pcb_eth_dev,
                                                    NU_USBCOMPH_CDC);

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
        case 0x02: NU_Remove_From_List((CS_NODE **) &pcb_eth_drvr->pcb_first_device,
                                       (CS_NODE *) pcb_eth_dev);
        case 0x01: USB_Deallocate_Memory(pcb_eth_dev);
        default: break;
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}
/*************************************************************************
* FUNCTION
*     _NU_USBH_COM_ETH_Disconnect_Handler
*
* DESCRIPTION
*     This function is called by the class driver whenever there is an
*     ethernet communication device connected to USB host.
*
* INPUTS
*     pcb_user            Pointer to control block of user driver.
*     pcb_drvr            Pointer to control block of class driver.
*     pcb_curr_device     Pointer to connected device.
*     information         Pointer to control block of device information
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*     NU_USB_INVLD_ARG      Some pointer became stale before call
*                           completion.
*
**************************************************************************/

STATUS _NU_USBH_COM_ETH_Disconnect_Handler(
       NU_USB_USER*  pcb_user_drvr,
       NU_USB_DRVR*  pcb_com_drvr,
       VOID*         pcb_device)
{
    NU_USBH_COM_ETH*        pcb_eth_drvr = (NU_USBH_COM_ETH*) pcb_user_drvr;
    NU_USBH_COM_ETH_DEVICE* pcb_next_device,
                           *pcb_curr_device = pcb_eth_drvr->pcb_first_device;

    STATUS status = NU_USB_INVLD_ARG;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Scan the list of devices and Cleanup all associated ones. */
    while (pcb_curr_device)
    {
        /* If this device is associated with the device disconnected. */
        if (pcb_curr_device->user_device.usb_device == pcb_device)
        {

            /* Sends disconnection interrupt to user driver. */
            if(pcb_eth_drvr->p_hndl_table)
            {
                if(pcb_eth_drvr->p_hndl_table->Disconnect_Handler)
                {
                    pcb_eth_drvr->p_hndl_table->Disconnect_Handler(pcb_user_drvr,
                                                              pcb_curr_device);
                }
            }
            else
            {
                NU_USB_SYS_DeRegister_Device (pcb_curr_device,
                                              NU_USBCOMPH_CDC);
            }

            _NU_USBH_USER_Disconnect ((NU_USB_USER*)pcb_user_drvr,
                                      (NU_USB_DRVR*)pcb_com_drvr,
                                      pcb_curr_device);

            /* Remove the device structure from the list... */
            NU_Remove_From_List(
             (CS_NODE **) & pcb_eth_drvr->pcb_first_device,
             (CS_NODE *) pcb_curr_device);

            /* ...and deallocate the device structure. */
            USB_Deallocate_Memory (pcb_curr_device);
            status = NU_SUCCESS;
            break;
        }
        else{
            pcb_next_device = (NU_USBH_COM_ETH_DEVICE *)
                              pcb_curr_device->user_device.node.cs_next;
            if(pcb_next_device == pcb_curr_device)
            {
               status = NU_INVALID_POINTER;
               break;
            }
            pcb_curr_device = pcb_next_device;

        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
* FUNCTION
*     _NU_USBH_COM_ETH_Intr_Handler
*
* DESCRIPTION
*     This function is called by the class driver whenever there is a valid
*     response present at communication device.
*
* INPUTS
*     pcb_curr_device       Pointer to Communication device with response.
*     pcb_xblock            Pointer to transfer block to hold data size and
*                           pointer.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*     NU_USB_INVLD_ARG      Some pointer became stale before call
*                           completion.
*
**************************************************************************/
STATUS _NU_USBH_COM_ETH_Intr_Handler(
       VOID*                pcb_curr_device,
       NU_USBH_COM_XBLOCK*  pcb_xblock)
{
    STATUS status = NU_SUCCESS;
    NU_USBH_COM_ETH* pcb_eth_drvr;
    NU_USBH_COM_ETH_DEVICE* pcb_eth_device;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    pcb_eth_drvr = (NU_USBH_COM_ETH*)((NU_USBH_COM_DEVICE*)pcb_curr_device)
                                  ->pcb_user_drvr;
    pcb_eth_device = (NU_USBH_COM_ETH_DEVICE*)
                      pcb_eth_drvr->pcb_first_device;

    while(pcb_eth_device->user_device.usb_device != pcb_curr_device)
    {
        if(pcb_eth_device == (NU_USBH_COM_ETH_DEVICE*)
                              pcb_eth_device->user_device.node.cs_next)
        {
            status = NU_NOT_PRESENT;
            break;
        }
        else
        {
            pcb_eth_device = (NU_USBH_COM_ETH_DEVICE*)
                              pcb_eth_device->user_device.node.cs_next;
        }
    }

    if(status == NU_SUCCESS)
    {
        pcb_eth_drvr->p_hndl_table->Event_Handler(pcb_eth_device,
                                                  pcb_xblock);
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}
/*************************************************************************
*   FUNCTION
*
*       NU_USBH_COM_ETH_Reg_Hndlr
*
*   DESCRIPTION
*
*       This function initializes the ethernet user driver component.
*
*   INPUTS
*
*       pcb_user_drvr    pointer to driver control block.
*       p_handlers       Application event handler.
*
*
*   OUTPUTS
*
*       status             Success or failure of the creation of the
*                          underlying initialization routines.
*
*************************************************************************/
VOID NU_USBH_COM_ETH_Reg_Hndlr(NU_USBH_COM_ETH*       pcb_user_drvr,
                               NU_USBH_COM_ETH_HANDL* p_handlers)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    ((NU_USBH_COM_USER*)pcb_user_drvr)->p_hndl_table = p_handlers;

    /* Revert to user mode. */
    NU_USER_MODE();
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_COMM_ETH_Init
*
*   DESCRIPTION
*
*       This function initializes the com ethernet driver component.
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
STATUS nu_os_conn_usb_host_comm_eth_init(CHAR *path, INT compctrl)
{
    VOID   *usbh_comm_handle = NU_NULL;
    UINT8   rollback = 0;
    STATUS  status = NU_SUCCESS, internal_sts = NU_SUCCESS;

    if(compctrl == RUNLEVEL_START)
    {
        /* Allocate memory for ethernet user driver control block. */
        status = USB_Allocate_Object(sizeof(NU_USBH_COM_ETH),
                                     (VOID **)&NU_USBH_COM_ETH_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 1;
        }

        /* Create the device subsystem. */
        if (!rollback)
        {
            /* Zero out allocated block. */
            memset(NU_USBH_COM_ETH_Cb_Pt, 0, sizeof(NU_USBH_COM_ETH));

            /* In following API call, passing memory pool ptr parameter
             * NU_NULL because in ReadyStart memory in USB system is
             * allocated through USB specific memory APIs, not directly
             * with any given memory pool pointer. This parameter remains
             * only for backwards code compatibility. */
            status = NU_USBH_COM_ETH_Create (NU_USBH_COM_ETH_Cb_Pt,
                                             "ethernet",
                                             NU_NULL,
                                             NU_NULL);
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        /*  Get the host comm class driver handle. */
        if (!rollback)
        {
            status = NU_USBH_COMM_Init_GetHandle(&usbh_comm_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        /* Register the user driver to the class driver. */
        if (!rollback)
        {
            status = NU_USB_DRVR_Register_User ((NU_USB_DRVR *) usbh_comm_handle,
                                                (NU_USB_USER *) NU_USBH_COM_ETH_Cb_Pt);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
#ifdef  CFG_NU_OS_DRVR_USB_HOST_NET_IF_ENABLE
            else
            {
                NU_USBH_NET_Entry(path);
            }
#endif

        }

        /* Clean up in case error occurs. */
        switch (rollback)
        {
            case 3:
                internal_sts = _NU_USBH_COM_ETH_Delete ((VOID *) NU_USBH_COM_ETH_Cb_Pt);

            case 2:
                if (NU_USBH_COM_ETH_Cb_Pt)
                {
                    internal_sts |= USB_Deallocate_Memory((VOID *)NU_USBH_COM_ETH_Cb_Pt);
                    NU_USBH_COM_ETH_Cb_Pt = NU_NULL;
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
         NU_USBH_COMM_Init_GetHandle(&usbh_comm_handle);

         if (usbh_comm_handle)
         {
            NU_USB_DRVR_Deregister_User((NU_USB_DRVR *) usbh_comm_handle,
                                     (NU_USB_USER *) NU_USBH_COM_ETH_Cb_Pt);
         }

         _NU_USBH_COM_ETH_Delete (NU_USBH_COM_ETH_Cb_Pt);
         USB_Deallocate_Memory(NU_USBH_COM_ETH_Cb_Pt);
         status = NU_SUCCESS;
    }

    return (status);
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_COM_ETH_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the comm ethernet
*       user driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the user
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates there exists a host ethernet
*                           user driver.
*       NU_NOT_PRESENT      Indicates there exists no user driver.
*
*************************************************************************/
STATUS NU_USBH_COM_ETH_GetHandle(VOID  **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBH_COM_ETH_Cb_Pt;
    if (NU_USBH_COM_ETH_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_ETH_DM_Open
*
* DESCRIPTION
*
*       This function is called by the application when it opens a device
*       for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the etherent driver passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS    NU_USBH_ETH_DM_Open (VOID* dev_handle)
{
    STATUS status = NU_SUCCESS;
    return (status);
}
/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_ETH_DM_Close
*
* DESCRIPTION
*
*       This function is called by the application when it wants to close a device
*       which it has opend already for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the etherent driver passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS    NU_USBH_ETH_DM_Close(VOID* dev_handle)
{
    STATUS status = NU_SUCCESS;
    return (status);
}
/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_ETH_DM_Read
*
* DESCRIPTION
*
*       This function is called by the application when it wants to read
*       data from the device.
*
* INPUTS
*
*       dev_handle         Pointer to the etherent driver passed as context.
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
STATUS NU_USBH_ETH_DM_Read( VOID*       dev_handle,
                            VOID*       buffer,
                            UINT32      numbyte,
                            OFFSET_T    byte_offset,
                            UINT32*     bytes_read_ptr)
{
    STATUS status;
    NU_USBH_COM_ETH_DEVICE*  pcb_eth_dev = NU_NULL;

    pcb_eth_dev = (NU_USBH_COM_ETH_DEVICE*)dev_handle;

    status = NU_USBH_COM_USER_Get_Data (
                        &pcb_eth_dev->user_device,
                        (UINT8*) ((UINT32)buffer),
                        numbyte,
                        bytes_read_ptr);

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_ETH_DM_Write
*
* DESCRIPTION
*
*       This function is called by the application when it wants to write
*       data from the device.
*
* INPUTS
*
*       dev_handle         Pointer to the etherent driver passed as context.
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
STATUS NU_USBH_ETH_DM_Write(VOID*       dev_handle,
                            const VOID* buffer,
                            UINT32      numbyte,
                            OFFSET_T    byte_offset,
                            UINT32*     bytes_written_ptr)
{
    STATUS status;
    NU_USBH_COM_ETH_DEVICE*  pcb_eth_dev = NU_NULL;

    pcb_eth_dev = (NU_USBH_COM_ETH_DEVICE*)dev_handle;

    status = NU_USBH_COM_USER_Send_Data (
                        &pcb_eth_dev->user_device,
                        (UINT8*) ((UINT32)buffer),
                        numbyte);

    *bytes_written_ptr = numbyte;

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBH_ETH_DM_IOCTL
*
* DESCRIPTION
*
*       This function is called by the application when it wants to perform a control
*       operation on the device.
*
* INPUTS
*
*       dev_handle         Pointer to the etherent driver passed as context.
*       cmd                IOCTL number.
*       data               IOCTL data pointer of variable type.
*       length             IOCTL data length in bytes.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS    NU_USBH_ETH_DM_IOCTL   (VOID*     dev_handle,
                                  INT       ioctl_num,
                                  VOID*     ioctl_data,
                                  INT       ioctl_data_len)
{
    STATUS status = NU_SUCCESS;
    return (status);
}
