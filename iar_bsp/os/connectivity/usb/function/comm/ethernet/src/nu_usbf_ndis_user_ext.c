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

*************************************************************************
*
* FILE NAME
*
*   nu_usbf_ndis_user_ext.c
*
* COMPONENT
*   Nucleus USB Function Software : Remote NDIS User Driver
*
* DESCRIPTION
*   This file contains the external Interfaces exposed by
*   NDIS User Driver.
*
* DATA STRUCTURES
*   None
*
* FUNCTIONS
*
* NU_USBF_NDIS_USER_Create         Creates an instance of NDIS User Driver.
* NU_USBF_NDIS_USER_Bind_Interface
*                                  Binds COMM ndis interface
*                                  descriptor with device
*                                  configuration descriptor.
* NU_USBF_NDIS_USER_Unbind_Interface
*                                  Unbinds COMM ndis interface
*                                  descriptor from device
*                                  configuration descriptor.
* _NU_USBF_NDIS_USER_Delete        Deletes an instance of NDIS User Driver.
* _NU_USBF_NDIS_USER_Connect       Connect Notification function.
* _NU_USBF_NDIS_USER_Disconnect    Disconnect Notification function.
* _NU_USBF_NDIS_USER_New_Command   Processes a new command
* _NU_USBF_NDIS_USER_New_Transfer  Processes a new transfer
* _NU_USBF_NDIS_USER_Tx_Done       Transfer completion notification
* _NU_USBF_NDIS_USER_Notify        USB event notification processing
* _NU_USBF_NDIS_DATA_Disconnect    Disconnect Notification function.
* _NU_USBF_NDIS_DATA_Connect       Connect Notification function.
* NU_USBF_NDIS_Send_Eth_Frame      Transfer Ethernet frame to Host.
* NU_USBF_NDIS_Send_Status         Send status to Host.
* NU_USBF_NDIS_Get_Conn_Status     Current media connect retrieval.
*
* DEPENDENCIES
*
*       nu_usb.h              All USB definitions
*       nucleus.h
*       nu_kernel.h
*       runlevel_init.h
*
************************************************************************/

/* ==============  USB Include Files =================================  */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "services/runlevel_init.h"

UINT8 fs_hs_ndis_intf_desc[] =
{
    /* Interface Descriptor. */
    0x09,                                   /* bLength              */
    USB_DT_INTERFACE,                       /* INTERFACE            */
    0x00,                                   /* bInterfaceNumber     */
    0x00,                                   /* bAlternateSetting    */
    0x01,                                   /* bNumEndpoints        */
    COMMF_CLASS,                            /* bInterfaceClass      */
    NF_SUBCLASS,                            /* bInterfaceSubClass   */
    0xFF,                                   /* bInterfaceProtocol   */
    0x00,                                   /* iInterface           */

    /* USB communication class call management descriptor. */
    0x05,                                   /* bLength              */
    USB_DT_CLASSSPC,                        /* bDescriptorType      */
    0x01,                                   /* bDescriptorSubtype   */
    0x00,                                   /* bmCapabilities       */
    0x01,                                   /* bDataInterface       */

    /* USB communication class abstract control management descriptor. */
    0x04,                                   /* bLength              */
    USB_DT_CLASSSPC,                        /* bDescriptorType      */
    0x02,                                   /* bDescriptorSubtype   */
    0x00,                                   /* bmCapabilities       */

    /* USB communication class union functional descriptor. */
    0x05,                                   /* bLength               */
    USB_DT_CLASSSPC,                        /* bDescriptorType       */
    0x06,                                   /* bDescriptorSubtype    */
    0x00,                                   /* bMasterInterface      */
    0x01,                                   /* bSlaveInterface0      */

    /* Endpoint Descriptors. */
    0x07,                                   /* bLength              */
    USB_DT_ENDPOINT,                        /* ENDPOINT             */
    USB_DIR_IN,                             /* bEndpointAddress     */
    USB_EP_INTR,                            /* bmAttributes         */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* bInterval            */

    /* Interface Descriptor. */
    0x09,                                   /* bLength              */
    USB_DT_INTERFACE,                       /* INTERFACE            */
    0x00,                                   /* bInterfaceNumber     */
    0x00,                                   /* bAlternateSetting    */
    0x02,                                   /* bNumEndpoints        */
    COMMF_DATA_CLASS,                       /* bInterfaceClass      */
    0x00,                                   /* bInterfaceSubClass   */
    0x00,                                   /* bInterfaceProtocol   */
    0x00,                                   /* iInterface           */

    /* Endpoint Descriptors. */
    0x07,                                   /* bLength              */
    USB_DT_ENDPOINT,                        /* ENDPOINT             */
    USB_DIR_OUT,                            /* bEndpointAddress     */
    USB_EP_BULK,                            /* bmAttributes         */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* bInterval            */
    /* Endpoint Descriptors. */
    0x07,                                   /* bLength              */
    USB_DT_ENDPOINT,                        /* ENDPOINT             */
    USB_DIR_IN,                             /* bEndpointAddress     */
    USB_EP_BULK,                            /* bmAttributes         */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* wMaxPacketSize       */
    0x00                                    /* bInterval            */
};

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
UINT8 ss_ndis_intf_desc[] =
{
    /* Interface Descriptor. */
    0x09,                                   /* bLength              */
    USB_DT_INTERFACE,                       /* INTERFACE            */
    0x00,                                   /* bInterfaceNumber     */
    0x00,                                   /* bAlternateSetting    */
    0x01,                                   /* bNumEndpoints        */
    COMMF_CLASS,                            /* bInterfaceClass      */
    NF_SUBCLASS,                            /* bInterfaceSubClass   */
    0xFF,                                   /* bInterfaceProtocol   */
    0x00,                                   /* iInterface           */

    /* USB communication class call management descriptor. */
    0x05,                                   /* bLength              */
    USB_DT_CLASSSPC,                        /* bDescriptorType      */
    0x01,                                   /* bDescriptorSubtype   */
    0x00,                                   /* bmCapabilities       */
    0x01,                                   /* bDataInterface       */

    /* USB communication class abstract control management descriptor. */
    0x04,                                   /* bLength              */
    USB_DT_CLASSSPC,                        /* bDescriptorType      */
    0x02,                                   /* bDescriptorSubtype   */
    0x00,                                   /* bmCapabilities       */

    /* USB communication class union functional descriptor. */
    0x05,                                   /* bLength               */
    USB_DT_CLASSSPC,                        /* bDescriptorType       */
    0x06,                                   /* bDescriptorSubtype    */
    0x00,                                   /* bMasterInterface      */
    0x01,                                   /* bSlaveInterface0      */

    /* Endpoint Descriptors. */
    0x07,                                   /* bLength              */
    USB_DT_ENDPOINT,                        /* ENDPOINT             */
    USB_DIR_IN,                             /* bEndpointAddress     */
    USB_EP_INTR,                            /* bmAttributes         */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* bInterval            */

    /* Endpoint Companion Descriptor. */
    6,
    USB_DT_SSEPCOMPANION,
    0,
    0,
    0,
    0,

    /* Interface Descriptor. */
    0x09,                                   /* bLength              */
    USB_DT_INTERFACE,                       /* INTERFACE            */
    0x00,                                   /* bInterfaceNumber     */
    0x00,                                   /* bAlternateSetting    */
    0x02,                                   /* bNumEndpoints        */
    COMMF_DATA_CLASS,                       /* bInterfaceClass      */
    0x00,                                   /* bInterfaceSubClass   */
    0x00,                                   /* bInterfaceProtocol   */
    0x00,                                   /* iInterface           */

    /* Endpoint Descriptors. */
    0x07,                                   /* bLength              */
    USB_DT_ENDPOINT,                        /* ENDPOINT             */
    USB_DIR_OUT,                            /* bEndpointAddress     */
    USB_EP_BULK,                            /* bmAttributes         */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* bInterval            */

    /* Endpoint Companion Descriptor. */
    6,
    USB_DT_SSEPCOMPANION,
    0,
    0,
    0,
    0,

    /* Endpoint Descriptors. */
    0x07,                                   /* bLength              */
    USB_DT_ENDPOINT,                        /* ENDPOINT             */
    USB_DIR_IN,                             /* bEndpointAddress     */
    USB_EP_BULK,                            /* bmAttributes         */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                    /* bInterval            */

    /* Endpoint Companion Descriptor. */
    6,
    USB_DT_SSEPCOMPANION,
    0,
    0,
    0,
    0
};
#endif

/* ==========================  Functions ============================== */

/*************************************************************************
* FUNCTION
*
*       NU_USBF_NDIS_USER_Init
*
* DESCRIPTION
*
*       NDIS User Driver Initialization Function
*
* INPUTS
*
*       path                                Registry path of component.
*       startstop                           Flag to find if component is
*                                           being enabled or disabled.
*
* OUTPUTS
*
*       status          Status of initialization
*
*************************************************************************/
STATUS nu_os_conn_usb_func_comm_ndis_init(CHAR *path, INT startstop)
{
    VOID   *usbf_comm_handle = NU_NULL;
    UINT8   rollback = 0;
    STATUS  status = NU_SUCCESS, internal_sts = NU_SUCCESS;

    if (startstop == RUNLEVEL_START)
    {
        /* Allocate Memory for Control Block */
        status = USB_Allocate_Object(sizeof(NU_USBF_NDIS_USER),
                                     (VOID**)&NU_USBF_NDIS_USER_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 1;
        }

        if (!rollback)
        {
            /* Zero out the memory allocated for control block */
            memset (NU_USBF_NDIS_USER_Cb_Pt, 0, sizeof(NU_USBF_NDIS_USER));

            /* In following API call, passing memory pool ptr parameter
             * NU_NULL because in ReadyStart memory in USB system is
             * allocated through USB specific memory APIs, not directly
             * with any given memory pool pointer. This parameter remains
             * only for backwards code compatibility. */
            status = NU_USBF_NDIS_USER_Create(NU_USBF_NDIS_USER_Cb_Pt,
                                        "USBF-NDIS",
#if (NF_VERSION_COMP >= NF_2_0)
                                        NU_NULL,
#endif
                                        NU_NULL,
                                        NU_NULL,
                                        NU_NULL,
                                        NU_NULL);
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        if (!rollback)
        {
            status = NU_USBF_COMM_Init_GetHandle(&usbf_comm_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        if (!rollback)
        {
            /* Register the User driver */
            status = NU_USB_DRVR_Register_User ((NU_USB_DRVR *) usbf_comm_handle,
                                                (NU_USB_USER *) NU_USBF_NDIS_USER_Cb_Pt);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

#ifndef CFG_NU_OS_DRVR_USB_FUNC_NET_IF_ENABLE
        if (!rollback)
        {
            status = NU_USB_SYS_Register_Device(NU_USBF_NDIS_USER_Cb_Pt,
                                                NU_USBCOMPF_RNDIS);
            if(status != NU_SUCCESS)
            {
                rollback = 4;
            }
        }
#endif

        /* Clean up in case error occurs. */
        switch (rollback)
        {
#ifndef CFG_NU_OS_DRVR_USB_FUNC_NET_IF_ENABLE
            case 4:
                internal_sts = NU_USB_DRVR_Deregister_User ((NU_USB_DRVR *) usbf_comm_handle,
                                                            (NU_USB_USER *) NU_USBF_NDIS_USER_Cb_Pt);
#endif
            case 3:
                internal_sts = _NU_USBF_NDIS_USER_Delete ((VOID *) NU_USBF_NDIS_USER_Cb_Pt);

            case 2:
                if (NU_USBF_NDIS_USER_Cb_Pt)
                {
                    internal_sts |= USB_Deallocate_Memory((VOID *)NU_USBF_NDIS_USER_Cb_Pt);
                    NU_USBF_NDIS_USER_Cb_Pt = NU_NULL;
                }

            case 1:
            case 0:
            /* internal_sts is not used after this. So to remove
             * KW and PC-Lint warning set it as unused variable.
             */
            NU_UNUSED_PARAM(internal_sts);
        }
    }
    else if (startstop == RUNLEVEL_STOP)
    {
        NU_USBF_COMM_Init_GetHandle(&usbf_comm_handle);

        NU_USB_DRVR_Deregister_User( (NU_USB_DRVR *) usbf_comm_handle,
                                     (NU_USB_USER *) NU_USBF_NDIS_USER_Cb_Pt);

        _NU_USBF_NDIS_USER_Delete(NU_USBF_NDIS_USER_Cb_Pt);
        USB_Deallocate_Memory((VOID *)NU_USBF_NDIS_USER_Cb_Pt);

        status = NU_SUCCESS;
    }

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_NDIS_USER_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the function NDIS
*       user driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the user
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicate there exists a function NDIS
*                           user driver.
*       NU_NOT_PRESENT      Indicate there exists no user driver.
*
*************************************************************************/
STATUS NU_USBF_NDIS_USER_GetHandle  ( VOID  **handle )
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBF_NDIS_USER_Cb_Pt;
    if (NU_USBF_NDIS_USER_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_NDIS_USER_Register_Cb
*
*   DESCRIPTION
*
*       This function is used to register the call backs with the
*       user driver.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the user
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicate there exists a function NDIS
*                           user driver.
*       NU_NOT_PRESENT      Indicate there exists no user driver.
*
*************************************************************************/
STATUS   NU_USBF_NDIS_USER_Register_Cb (NU_USBF_NDIS_USER *cb,
                                COMMF_APP_NOTIFY     conn_handler,
                                COMMF_APP_RX_CALLBACK   rcvd_cb,
                                COMMF_APP_TX_DONE       tx_done,
                                COMMF_APP_IOCTL         ioctl)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    if (cb == NU_NULL      ||
        conn_handler == NU_NULL  ||
        rcvd_cb == NU_NULL ||
        tx_done == NU_NULL ||
        ioctl   == NU_NULL
        )
    {
        status = NU_NOT_PRESENT;
    }
    else
    {
        /* Register application callback function with the user driver. */
        cb->rndis_app_notify = conn_handler;
        cb->rndis_rcvd_cb = rcvd_cb;
        cb->rndis_tx_done = tx_done;
        cb->rndis_ioctl = ioctl;

        status = NU_SUCCESS;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
* FUNCTION
*
*       NU_USBF_NDIS_USER_Create
*
* DESCRIPTION
*
*       NDIS User Driver initialization routine
*
* INPUTS
*       cb          Pointer to Driver control block.
*       name        Name of this USB object.
*       pool        Pointer to the memory pool.
*       app_notify  Application notification callback.
*       rcvd_cb     Application data received callback.
*       tx_done     Application transmission complete callback.
*       ioctl       Application IOCTL callback.
*
* OUTPUTS
*
*       NU_SUCCESS           Successful Initialization
*
*************************************************************************/
STATUS NU_USBF_NDIS_USER_Create (NU_USBF_NDIS_USER *cb, CHAR *name,
#if (NF_VERSION_COMP >= NF_2_0)
                                 NU_MEMORY_POOL    *pool,
#endif
                                 COMMF_APP_NOTIFY     app_notify,
                                 COMMF_APP_RX_CALLBACK   rcvd_cb,
                                 COMMF_APP_TX_DONE       tx_done,
                                 COMMF_APP_IOCTL         ioctl)
{
    STATUS status;

    /*
     * Switching to supervisor mode.
     */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_USB_MEMPOOLCHK_RETURN(pool);

    /* Register application callback function with the user driver. */
    cb->rndis_app_notify = app_notify;
    cb->rndis_rcvd_cb = rcvd_cb;
    cb->rndis_tx_done = tx_done;
    cb->rndis_ioctl = ioctl;
    #if (NF_VERSION_COMP >= NF_2_0)
    cb->pool = pool;
    #else
    cb->pool = NU_NULL;
    #endif

    /* Call NU_USBF_USER_COMM's behavior using _NU_USBF_USER_COMM_Create
     * by passing cb, address to usbf_ndis_user_dispatch as the
     * dispatch table and bInterfaceSubclass & bInterfaceProtocol
     * specific to NDIS user.
     */
    status =  _NU_USBF_USER_COMM_Create ((NU_USBF_USER_COMM * )cb,
                        name,
                        NF_SUBCLASS,
                        0xFF,
                        NU_TRUE,
                        &usbf_ndis_user_dispatch);

#if ((ETHF_DESC_CONF == INC_ETH_AND_RNDIS) || (ETHF_DESC_CONF == INC_RNDIS))
    if ( status == NU_SUCCESS )
    {
        /* Add RNDIS function with USB device configuration layer.
             */
        /* Add descriptor for FULL, HIGH and SUPER speed. */
        status = USBF_DEVCFG_Add_Function(USBF_DEF_CONFIG_INDEX,
                                         fs_hs_ndis_intf_desc,
                                         sizeof(fs_hs_ndis_intf_desc),
                                         fs_hs_ndis_intf_desc,
                                         sizeof(fs_hs_ndis_intf_desc),
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                                         ss_ndis_intf_desc,
                                         sizeof(ss_ndis_intf_desc),
#endif
                                         &cb->ndis_function);
    }
#endif

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_NDIS_USER_Bind_Interface
*
* DESCRIPTION
*
*       This function actually binds COMM ndis function with USB device
*       configuration. It is necessary that ndis function is
*       already registered with USB device configuration glue layer.
*
* INPUTS
*
*       cb                      Pointer to NDIS function driver
*                               control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        Indicates invalid argument.
*       NU_USB_NOT_PRESENT      Indicates that NDIS function
*                               is not registered.
*       NU_SUCCESS              Indicates successful initialization.
*
**************************************************************************/
STATUS NU_USBF_NDIS_USER_Bind_Interface(NU_USBF_NDIS_USER *cb)
{
    /* Local variables. */
    STATUS     status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;
    if ( cb != NU_NULL )
    {
#if ((ETHF_DESC_CONF == INC_ETH_AND_RNDIS) || (ETHF_DESC_CONF == INC_RNDIS))
        status = NU_NOT_PRESENT;
        if ( cb->ndis_function != NU_NULL )
        {
            status = NU_USB_DEVICE_Set_bDeviceClass(cb->ndis_function->device,
                                                    COMMF_CLASS);
            if ( status == NU_SUCCESS )
            {
                /* Enable modem function. */
                status = USBF_DEVCFG_Enable_Function(cb->ndis_function);
                if ( status == NU_SUCCESS )
                {
                    /* Bind modem function with USB device configuration. */
                    status = USBF_DEVCFG_Bind_Function(cb->ndis_function);
                }
            }
        }
#else
        status = NU_NOT_PRESENT;
#endif
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_NDIS_USER_Unbind_Interface
*
* DESCRIPTION
*
*       This function actually unbinds COMM ndis function with USB
*       device configuration.
*       It is necessary that ndis function is already registered
*       with USB device configuration glue layer.
*
* INPUTS
*
*       pcb_ms_drvr             Pointer to NDIS function driver
*                               control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        Indicates invalid argument.
*       NU_USB_NOT_PRESENT      Indicates that NDIS function
*                               is not registered.
*       NU_SUCCESS              Indicates successful initialization.
*
**************************************************************************/
STATUS NU_USBF_NDIS_USER_Unbind_Interface(NU_USBF_NDIS_USER *cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;
    if ( cb != NU_NULL )
    {
#if ((ETHF_DESC_CONF == INC_ETH_AND_RNDIS) || (ETHF_DESC_CONF == INC_RNDIS))
        status = NU_NOT_PRESENT;
        if ( cb->ndis_function != NU_NULL )
        {
            status = USBF_DEVCFG_Unbind_Function(cb->ndis_function);
        }
#else
        status = NU_NOT_PRESENT;
#endif
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBF_NDIS_USER_Delete
*
* DESCRIPTION
*       This function deletes an instance of NDIS User driver .
*
* INPUTS
*       cb              Pointer to the USB Object control block.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*
*************************************************************************/
STATUS _NU_USBF_NDIS_USER_Delete (VOID *cb)
{
    STATUS status;

    /* Remove USB RNDIS function. */
    status = USBF_DEVCFG_Delete_Function(((NU_USBF_NDIS_USER * )cb)->ndis_function);

    /* Call NU_USBF_USER_COMM's Behavior */
    status |= _NU_USBF_USER_COMM_Delete((NU_USBF_USER_COMM * )cb);

    return ( status);
}

/*************************************************************************
* FUNCTION
*       _NU_USBF_NDIS_USER_Connect
*
* DESCRIPTION
*     This function is called by class driver's Initialize_intf/Device
*     routine when a new device which can be served by this user is found.
*
* INPUTS
*       cb              Pointer to user control block.
*       class_driver    Pointer to calling class driver's control block.
*       handle          Handle for the device connected.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*************************************************************************/
STATUS _NU_USBF_NDIS_USER_Connect (NU_USB_USER * cb,
                                   NU_USB_DRVR * class_driver,
                                   VOID *handle)
{
    STATUS status = NU_SUCCESS;
    NU_USBF_NDIS_DEVICE *pdevice = NU_NULL;
    NU_USBF_NDIS_USER *ndis = (NU_USBF_NDIS_USER *)cb;
    UINT8 mac_set[NF_MAC_ADDR_LEN] = {0x00, 0x07, 0xe9, 0x22, 0x33, 0x44};

    #if (NF_VERSION_COMP >= NF_2_0)
    /* Allocate and initialize a new instance of NDIS device. */
    status = USB_Allocate_Object(sizeof (NU_USBF_NDIS_DEVICE),
                                 (VOID **)&pdevice);
    #else
    pdevice = &NU_USBF_NDIS_DEV_CB;
    #endif

    if (status == NU_SUCCESS)
    {
        memset(pdevice,0,sizeof(NU_USBF_NDIS_DEVICE));
        pdevice->handle = handle;

        /* Reset the driver data. */
        pdevice->resp_in_progress=0;
        pdevice->status_pending=0;

        pdevice->connect_status = NF_STATE_CONNENCTED;

        /* Reset the ethernet statistics maintained by driver. */
        pdevice->frame_tx_ok = 0;
        pdevice->frame_rx_ok = 0;
        pdevice->frame_tx_err = 0;
        pdevice->frame_rx_err = 0;

        NU_Place_On_List((CS_NODE **)&ndis->ndis_list_head, (CS_NODE *)pdevice);

        /* Call the parent's behavior. */
        status = _NU_USBF_USER_COMM_Connect (cb,
                                       class_driver,
                                       handle);

        if (status != NU_SUCCESS)
        {
            NU_Remove_From_List((CS_NODE **)&ndis->ndis_list_head, (CS_NODE *)pdevice);
            #if (NF_VERSION_COMP >= NF_2_0)
            USB_Deallocate_Memory (pdevice);
            #endif
        }
    }

    /* If everything is fine till here, set default MAC address */
    if (status == NU_SUCCESS)
    {
        status = NU_USBF_NDIS_Set_MAC_Address(ndis,
                                              handle,
                                              mac_set,
                                              sizeof(mac_set));
    }

    return (status);
}
/*************************************************************************
* FUNCTION
*       _NU_USBF_NDIS_USER_Disconnect
*
* DESCRIPTION
*       This function is called by the class driver's disconnect function
*       when a device being served by this user gets disconnected. This
*       cleans up the device specific entries associated with the device.
*
* INPUTS
*       cb                  pointer to user control block.
*       class_driver        pointer to calling class driver's control block.
*       handle              handle of the device disconnected.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_INVALID_POINTER  Driver is unable to find the device pointer
*                           from its list on the basis of handle.
*
*************************************************************************/
STATUS _NU_USBF_NDIS_USER_Disconnect (NU_USB_USER * cb,
                                        NU_USB_DRVR * class_driver,
                                        VOID *handle)
{
    STATUS status;
    NU_USBF_NDIS_USER *user = (NU_USBF_NDIS_USER *)cb;
    NU_USBF_NDIS_DEVICE *pdev = NU_NULL;

    pdev = NDISF_Find_Device(user, handle);

    if (pdev)
    {
        NU_Remove_From_List((CS_NODE**)&user->ndis_list_head, (CS_NODE*)pdev);
        #if (NF_VERSION_COMP >= NF_2_0)
        USB_Deallocate_Memory(pdev);
        #endif

        /* Call the parent's function. */
        status = _NU_USBF_USER_COMM_Disconnect (cb,
                                      class_driver,
                                      handle);
    }
    else
    {
        status = NU_INVALID_POINTER;
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBF_NDIS_USER_New_Command
*
* DESCRIPTION
*
*       Processes a new command from the Host.
*
*     These commands are class/subclass specific. This function processes
*     the command identified by the 'command' parameter. The length of
*     the command block, in bytes is expected in the 'cmd_len' parameter.
*
*     If there is any data transfer in response to a command, then the
*     location corresponding to the data to be transferred, either to
*     / from Host is filled in the 'data_out' parameter. The length of the
*     data to be transferred is filled in the location pointed to by the
*     'data_len_out' parameter.
*
*     If there is no data transfer associated with the command, then, the
*     location pointed to by the 'data' is filled in with NU_NULL.
*
*     For unknown and unsupported command, this function returns
*     appropriate error status.
*
* INPUTS
*
*       cb           User control block for which the command is meant for
*
*       drvr         Class driver invoking this function
*
*       handle       Identifies for the logical function to which the
*                    command is directed to.
*
*       command      Memory location where the command block is stored
*
*       cmd_len      Length of the command block
*
*       data_out     Memory location where the data pointer for the
*                    transfer is to be stored
*
*       data_len_out Memory location where the length of data to be
*                    transferred, in bytes, must be filled.
*
* OUTPUTS
*
*       NU_SUCCESS            Indicates that the command has been
*                             processed successfully.
*
*       NU_USB_NOT_SUPPORTED  Indicates that the command is unsupported
*
*       NU_INVALID_POINTER    Driver is unable to find the device pointer
*                             from its list on the basis of handle.
*
*************************************************************************/
STATUS _NU_USBF_NDIS_USER_New_Command (NU_USBF_USER * cb,
                                 NU_USBF_DRVR * drvr,
                                 VOID *handle,
                                 UINT8 *command,
                                 UINT16 cmd_len,
                                 UINT8 **data_out, UINT32 *data_len_out)
{
    STATUS status;
    USBF_COMM_USER_CMD *user_cmd;
    NU_USBF_NDIS_USER *ndis;
    NU_USBF_NDIS_DEVICE *pdevice = NU_NULL;

    /*
     * Switching to supervisor mode.
     */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);

    user_cmd = (USBF_COMM_USER_CMD *)command;
    ndis = (NU_USBF_NDIS_USER*)cb;

    /* Find pointer to the user device */
    pdevice = NDISF_Find_Device(ndis,handle);

    if (pdevice)
    {
        /* If SEND_ENCAPSULATED_COMMAND is received from Host. */
        if(user_cmd->command == NF_SEND_ENCAPSULATED_COMMAND)
        {
            status = NDISF_Decode_Encap_Command(ndis,
                                                pdevice,
                                                user_cmd->cmd_data,
                                                user_cmd->data_len);
        }
        /* This command is sent by Host when RESPONSE_AVAILABLE notification
         * is sent to Host. 'User must fill the valid response value in the
         * 'response_array' before giving this notification.
         */
        else if(user_cmd->command == NF_GET_ENCAPSULATED_RESPONSE)
        {
            /* If status data is needed to be sent. */
            if(pdevice->status_pending == 2)
            {
                pdevice->status_pending = 0;
                /* Initialize the response buffer and length. */
                *data_out = &(pdevice->status_array[0]);
                *data_len_out = pdevice->status_length;
            }
            else
            {
                /* Initialize the response buffer and length. */
                *data_out = &(pdevice->response_array[0]);
                *data_len_out = pdevice->response_length;
            }
            status = NU_SUCCESS;
        }
        else
        {
            status = NU_USB_NOT_SUPPORTED;
        }
    }
    else
    {
        status = NU_INVALID_POINTER;
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}
/*************************************************************************
* FUNCTION
*
*       NU_USBF_NDIS_USER_New_Transfer
*
* DESCRIPTION
*
*       This function processes a received data transfer from USB Host.
*       This function needs to call the NU_USBF_COMM_DATA_Get_Rcvd to pull
*       received data from the completed queue of class driver.
*
* INPUTS
*
*       cb              User control block
*
*       drvr            Class driver invoking this function
*
*       handle          Identifies for the logical function to which the
*                       transfer is directed to.
*
*       data_out        Memory location where the data pointer for the
*                       transfer is to be stored
*
*       data_len_out    Memory location where the length of data to be
*                       transferred, in bytes, must be filled.
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates that the function has executed
*                               successfully.
*
*       NU_USB_NOT_SUPPORTED    Indicates that the new transfer requests
*                               are not supported by the user.
*       NU_INVALID_POINTER      Driver is unable to find the device pointer
*                               from its list on the basis of handle.
*
*************************************************************************/
STATUS _NU_USBF_NDIS_USER_New_Transfer (NU_USBF_USER * cb,
                                  NU_USBF_DRVR * drvr,
                                  VOID *handle,
                                  UINT8 **data_out, UINT32 *data_len_out)
{
    NU_USBF_NDIS_USER *ndis = (NU_USBF_NDIS_USER *)cb;
    NU_USBF_NDIS_DEVICE *pdevice = NU_NULL;
    STATUS status;
    UINT8 *buffer;
    UINT32 length;
    UINT32 pkt_type;
    UINT32 data_length=0;
    UINT32 info_length=0;
    UINT8 *info_buffer = NU_NULL;

    /*
     * Switching to supervisor mode.
     */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Find device pointer corresponding to the handle. */
    pdevice = NDISF_Find_Device(ndis, handle);

    if (pdevice)
    {
        /* If user wants to process the received data now, it can call the
         * function to get the received data parameters otherwise it can call
         * it on later time.
         */
        status = NU_USBF_COMM_DATA_Get_Rcvd(
                            ((NU_USBF_USER_COMM* )ndis)->data_drvr,
                            &buffer, &length,
                            handle);

        if(status == NU_SUCCESS)
        {
            if(pdevice->connect_status == NF_STATE_CONNENCTED)
            {
                /* Parse the received data. */
                pkt_type =
                    LE32_2_HOST(*(UINT32*)((UINT32)buffer+NF_MSG_TYPE_OFF));

                /* Only NF_PACKET_MSG is expected from Host. */
                if(pkt_type == NF_PACKET_MSG)
                {
                    /* Increment the transmitted frame count. */
                    pdevice->frame_tx_ok++;

                    data_length = LE32_2_HOST(*(UINT32*)((UINT32)buffer+
                                              NF_PKT_MSG_DATA_LENGTH_OFF));

                    /* Packet message may contain per packet information, which
                     * includes TCP/IP checksum etc.
                     */
                    info_length = LE32_2_HOST(*(UINT32*)
                            ((UINT32)buffer+NF_PKT_MSG_PKT_INFO_LEN_OFF));
                    if(info_length != NU_NULL)
                    {
                        info_buffer =
                            (UINT8 *)((UINT32)buffer+NF_PKT_MSG_DATA_OFF
                            +LE32_2_HOST(*(UINT32*)((UINT32)buffer+
                            NF_PKT_MSG_PKT_INFO_LEN_OFF)));
                    }
                }
            }

            /* Pass the received data to application. */
            if(ndis->rndis_rcvd_cb)
            {
                #if (NF_VERSION_COMP >= NF_2_0)
                ndis->rndis_rcvd_cb(buffer,
                                    data_length,
                                    info_buffer,
                                    info_length,(VOID*)pdevice->handle);
                #else
                ndis->rndis_rcvd_cb(buffer,
                                    data_length,
                                    info_buffer,
                                    info_length);
                #endif
            }
        }
    }
    else
        status = NU_INVALID_POINTER;

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}
/*************************************************************************
* FUNCTION
*
*       NU_USBF_NDIS_USER_Tx_Done
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
*       If there is no data transfer associated with the command, then,
*       the location pointed to by the 'data' is filled in with NU_NULL.
*
* INPUTS
*
*       cb              User control block
*
*       drvr            Class driver invoking this function
*
*       handle          Identifies for the logical function to which the
*                       notification is directed to.
*
*       completed_data  Memory location to / from where the data has been
*                       transferred
*
*       completed_data_len
*                       length of data transferred, in bytes.
*
*       data_out        Memory location where the data pointer for the
*                       transfer is to be stored, if pending.
*
*       data_len_out    Memory location where the length of data to be
*                       transferred, in bytes, is to be filled.
*
* OUTPUTS
*
*       NU_SUCCESS          Indicates that the function has executed
*                           successfully.
*
*       NU_USB_INVLD_ARG    Indicates a unexpected transfer completion.
*
*       NU_INVALID_POINTER  Driver is unable to find the device pointer
*                           from its list on the basis of handle.
*
*************************************************************************/
STATUS _NU_USBF_NDIS_USER_Tx_Done (NU_USBF_USER * cb,
                                       NU_USBF_DRVR * drvr,
                                       VOID *handle,
                                       UINT8 *completed_data,
                                       UINT32 completed_data_len,
                                       UINT8 **data_out,
                                       UINT32 *data_len_out)
{
    STATUS status = NU_INVALID_POINTER;

    COMMF_CMPL_CTX * ctx;
    NU_USBF_NDIS_USER * ndis;
    NU_USBF_NDIS_DEVICE *pdevice = NU_NULL;
    UINT8   tx_handle;

    /*
     * Switching to supervisor mode.
     */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);

    ndis = (NU_USBF_NDIS_USER *)cb;
    ctx = (COMMF_CMPL_CTX*)handle;
    tx_handle = ctx->transfer_type;

    #if (NF_VERSION_COMP >= NF_2_0)
    /* Find pointer to the device corresponding to the handle */
    pdevice = NDISF_Find_Device(ndis,ctx->handle);
    #else
    pdevice = &NU_USBF_NDIS_DEV_CB;
    #endif

    if (pdevice)
    {
        /* If transmission of data is complete. */
        if(tx_handle == COMMF_DATA_SENT)
        {
            /* Increment the received frame count. */
            pdevice->frame_rx_ok++;

            /* Give callback to application. */
            if(ndis->rndis_tx_done)
            {
                #if (NF_VERSION_COMP >= NF_2_0)
                ndis->rndis_tx_done(completed_data, completed_data_len,ctx->handle);
                #else
                ndis->rndis_tx_done(completed_data, completed_data_len);
                #endif
            }
            status = NU_SUCCESS;
        }

        /* If last notification is successfully sent to Host. */
        else if(tx_handle == COMMF_NOTIF_SENT)
        {
            status = NU_SUCCESS;
        }
        /* If callback is received for the GET_ENCAPSULATED_RESPONSE. */
        else if(tx_handle == COMMF_GET_CMD_SENT)
        {
            status = NU_SUCCESS;
            pdevice->resp_in_progress = 0;

            /* If a notification is pending for Send_Status transfer. */
            if(pdevice->status_pending == 1)
            {
                pdevice->status_pending = 2;
                NDISF_Send_Notification(ndis, NF_RESPONSE_AVAILABLE,pdevice);
            }
        }
        else
        {
            status = NU_USB_INVLD_ARG;
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}
/*************************************************************************
* FUNCTION
*
*       NU_USBF_NDIS_USER_Notify
*
* DESCRIPTION
*
*       This function processes a USB Event. Examples of such USB events
*       include suspend and Reset.
*
*       This function carries out function specific processing for the USB
*       event.
*
*
* INPUTS
*
*       cb            User control block for which the event is meant for
*
*       drvr          Class driver invoking this function
*
*       handle        Identifies for the logical function to which the
*                     notification is directed to.
*
*       event         The USB event that has occurred.
*
* OUTPUTS
*
*       NU_SUCCESS      Indicates that the event has been processed
*                       successfully
*
*************************************************************************/
STATUS _NU_USBF_NDIS_USER_Notify (NU_USBF_USER * cb,
                            NU_USBF_DRVR * drvr,
                            VOID *handle,
                            UINT32 event)
{
    NU_USBF_NDIS_USER *ndis = (NU_USBF_NDIS_USER *)cb;
    COMMF_APP_NOTIFY   app_notify;
    NU_SUPERV_USER_VARIABLES

    app_notify = ndis->rndis_app_notify;

    /* Pass this event to application. */
    if(app_notify)
    {
        /* Switch to user mode before giving callback to application. */
        NU_USER_MODE();

        app_notify((NU_USBF_USER *)cb, event, handle);

        /* Switch back to supervisor mode. */
        NU_SUPERVISOR_MODE();
    }

    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*       _NU_USBF_NDIS_DATA_Disconnect
*
* DESCRIPTION
*       This function is called by the class driver's disconnect function
*       when data interface of devices is un-initialized. This cleans up
*       the device specific entries associated with the device.
*
* INPUTS
*       cb              Pointer to user control block.
*       class_driver    Pointer to calling class driver's control block.
*       handle          Handle of the device disconnected.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*
*************************************************************************/
STATUS _NU_USBF_NDIS_DATA_Disconnect (NU_USB_USER * cb,
                               NU_USB_DRVR * class_driver,
                               VOID *handle)
{
    STATUS status;
    NU_USBF_NDIS_USER *ndis = (NU_USBF_NDIS_USER *)cb;
    COMMF_APP_NOTIFY   app_notify;
    NU_SUPERV_USER_VARIABLES

    app_notify = ndis->rndis_app_notify;

    /* Call the parent's function. */
    status = _NU_USBF_USER_COMM_DATA_Discon (cb,
                               class_driver,
                               handle);
    if(status == NU_SUCCESS)
    {
        /* Pass this initialized event to application. */
        if(app_notify)
        {
            /* Switch to user mode before giving callback to application. */
            NU_USER_MODE();

            app_notify((NU_USBF_USER *)cb,
                       COMMF_EVENT_USER_DEINIT,
                       handle);

            /* Switch back to supervisor mode. */
            NU_SUPERVISOR_MODE();
        }
    }

    return status;
}

/*************************************************************************
* FUNCTION
*       _NU_USBF_NDIS_DATA_Connect
*
* DESCRIPTION
*       This function is called by class driver's Initialize_intf/Device
*       routine when Data Interface of device is initialized.
*
* INPUTS
*       cb              Pointer to user control block.
*       class_driver    Pointer to calling class driver's control block.
*       handle          Handle for the device connected.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*************************************************************************/
STATUS _NU_USBF_NDIS_DATA_Connect (NU_USB_USER * cb,
                            NU_USB_DRVR * class_driver,
                            VOID *handle)
{
    STATUS status;
    NU_USBF_NDIS_USER *ndis = (NU_USBF_NDIS_USER *)cb;
    COMMF_APP_NOTIFY   app_notify;
    NU_SUPERV_USER_VARIABLES

    app_notify = ndis->rndis_app_notify;

    /* Every time DATA_Connect callback is called from Class driver, that
     * means Host has select the default alternate setting of Data class
     * Interface and selected the working alternate setting again. This
     * has the effect of resetting the device, so it should clear all
     * filters, buffers and statistics.
     */
    status = _NU_USBF_USER_COMM_DATA_Connect (cb,
                            class_driver,
                            handle);

    if(status == NU_SUCCESS)
    {
        /* Pass this initialized event to application. */
        if(app_notify)
        {
            /* Switch to user mode before giving callback to application. */
            NU_USER_MODE();

            app_notify((NU_USBF_USER *)cb,
                       COMMF_EVENT_USER_INIT,
                       handle);

            /* Switch back to supervisor mode. */
            NU_SUPERVISOR_MODE();
        }
    }

    return status;
}

/*************************************************************************
* FUNCTION
*
*       NU_USBF_NDIS_Send_Eth_Frame
*
* DESCRIPTION
*
*      Routine for sending ethernet frame to Host. Data should be starting
*      at 44 bytes offset.
*
* INPUTS
*       ndis           Pointer to user control block.
*       handle         Pointer to the logical communication device
*                      control block.
*       data_buffer    Pointer to data buffer for ethernet frame.
*       data_length    Length of ethernet frame buffer.
*       info_buffer    Pointer to buffer for per packet info.
*       info_length    Length of per packet information.
*       handle         Pointer to the device.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*************************************************************************/
STATUS NU_USBF_NDIS_Send_Eth_Frame(NU_USBF_NDIS_USER * ndis,
#if (NF_VERSION_COMP >= NF_2_0)
VOID *handle,
#endif
                                    UINT8 * data_buffer,
                                    UINT32 data_length,
                                    UINT8 * info_buffer,
                                    UINT32 info_length)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Initialize the NF_PACKET_MSG header. */
    *(UINT32*)((UINT32)data_buffer+NF_MSG_TYPE_OFF) =
                                   HOST_2_LE32(NF_PACKET_MSG);
    *(UINT32*)((UINT32)data_buffer+NF_MSG_LENGTH_OFF) =
            HOST_2_LE32(NF_PKT_MSG_LENGTH+data_length+info_length);
    *(UINT32*)((UINT32)data_buffer+NF_PKT_MSG_DATA_OFF) = 0;
    *(UINT32*)((UINT32)data_buffer+NF_PKT_MSG_DATA_LENGTH_OFF) =
                                                HOST_2_LE32(data_length);
    if(data_length > 0)
    {
        *(UINT32*)((UINT32)data_buffer+NF_PKT_MSG_DATA_OFF) = HOST_2_LE32
                            (NF_PKT_MSG_LENGTH-NF_PKT_MSG_DATA_OFF);
    }

    /* No OOB data is defined for Ethernet. */
    *(UINT32*)((UINT32)data_buffer+NF_PKT_MSG_OOB_DATA_OFF) = 0;
    *(UINT32*)((UINT32)data_buffer+NF_PKT_MSG_OOB_DATA_LENGTH_OFF) = 0;
    *(UINT32*)((UINT32)data_buffer+NF_PKT_MSG_NUM_OOB_ELEMS_OFF) = 0;
    *(UINT32*)((UINT32)data_buffer+NF_PKT_MSG_PER_PKT_INFO_OFF) = 0;
    *(UINT32*)((UINT32)data_buffer+NF_PKT_MSG_PKT_INFO_LEN_OFF) =
                                                 HOST_2_LE32(info_length);

    /* If per packet information is required to be sent with packet,
     * update it in header.
     */
    if(info_length > 0)
    {
        *(UINT32*)((UINT32)data_buffer+NF_PKT_MSG_PER_PKT_INFO_OFF) =
           HOST_2_LE32(NF_PKT_MSG_LENGTH-NF_PKT_MSG_DATA_OFF+data_length);
    }

    /* Last 8 bytes of Packet message header are 0. */
    memset((UINT8 *)((UINT32)data_buffer + NF_PKT_MSG_VCHANDLE_OFF), 0,
                                            NF_PKT_MSG_VCHANDLE_SIZE);

    #if (NF_VERSION_COMP >= NF_2_0)
    /* Call the Data Interface driver's API to send the frame to Host. */
    status = NU_USBF_COMM_DATA_Send(
                        ((NU_USBF_USER_COMM *)ndis)->data_drvr,
                        data_buffer,
                        NF_PKT_MSG_LENGTH+data_length+info_length,
                        handle);
    #else
    /* Call the Data Interface driver's API to send the frame to Host. */
    status = NU_USBF_COMM_DATA_Send(
                        ((NU_USBF_USER_COMM *)ndis)->data_drvr,
                        data_buffer,
                        NF_PKT_MSG_LENGTH+data_length+info_length,
                        NU_USBF_NDIS_DEV_CB.handle);
    #endif

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
* FUNCTION
*
*       NU_USBF_NDIS_Send_Status
*
* DESCRIPTION
*
*       Routine for sending INDICATE_STATUS message to Host.
*
* INPUTS
*     ndis           Pointer to user control block.
*     handle         Pointer to the logical communication device
*                    control block.
*     status_val     Status value to be sent to Host.
*     status_buffer  Pointer to buffer status (depends on the status_val).
*     length         Length of status buffer.
*     handle         Pointer to the device.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_INVALID_POINTER  Driver is unable to find the device pointer
*                           from its list on the basis of handle.
*
*
*************************************************************************/
STATUS NU_USBF_NDIS_Send_Status(NU_USBF_NDIS_USER * ndis,
#if (NF_VERSION_COMP >= NF_2_0)
VOID *handle,
#endif
                    UINT32 status_val,
                    UINT8 * status_buffer,
                    UINT32 length)

{
    STATUS status = NU_SUCCESS;
    UINT8 *buffer;
    INT old_level;
    NU_USBF_NDIS_DEVICE *pdevice = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

#if (NF_VERSION_COMP >= NF_2_0)
    /* Find pointer to the device corresponding to the handle passed. */
    pdevice = NDISF_Find_Device(ndis, handle);
#else
    pdevice = &NU_USBF_NDIS_DEV_CB;
#endif

    if(pdevice)
    {
        /* Lock the interrupts. */
        old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Use status_array for preparing the status packet. */
        buffer = pdevice->status_array;
        pdevice->status_length = NF_IND_STATUS_MSG_LENGTH+length;

        /* Initialize the INDICATE_STATUS message header. */
        *(UINT32*)((UINT32)buffer+NF_MSG_TYPE_OFF) =
                                    HOST_2_LE32(NF_IND_STATUS_MSG);
        *(UINT32*)((UINT32)buffer+NF_MSG_LENGTH_OFF) =
                                 HOST_2_LE32(NF_IND_STATUS_MSG_LENGTH+length);
        *(UINT32*)((UINT32)buffer+NF_IND_STATUS_MSG_STATUS_OFF) =
                                                      HOST_2_LE32(status_val);
        *(UINT32*)((UINT32)buffer+NF_IND_STATUS_MSG_BUF_LEN_OFF) =
                                                          HOST_2_LE32(length);
        *(UINT32*)((UINT32)buffer+NF_IND_STATUS_MSG_BUF_OFF) = 0;
        /* If some data is to be sent with this status. */
        if(length > 0)
        {
            *(UINT32*)((UINT32)buffer+NF_IND_STATUS_MSG_BUF_OFF) =
                                        HOST_2_LE32(NF_IND_STATUS_MSG_LENGTH);
            memcpy((UINT8 *)((UINT32)buffer+NF_IND_STATUS_MSG_LENGTH),
                                                       status_buffer, length);
        }

        pdevice->status_pending = 1;

        /* If no response is currently in progress, send notification for
         * sending it, otherwise wait for the Tx_Done of current transfer.
         */
        if(pdevice->resp_in_progress == 0)
        {
            pdevice->status_pending = 2;
            /* Call the routine of sending notification to Host. */
            status = NDISF_Send_Notification(ndis, NF_RESPONSE_AVAILABLE,pdevice);
        }

        /* Update the driver state if status is sent for media connection
         * change.
         */
        if(status_val == NF_STATUS_MEDIA_CONNECT)
        {
            pdevice->connect_status = NF_STATE_CONNENCTED;
        }

        if(status_val == NF_STATUS_MEDIA_DISCONNECT)
        {
            pdevice->connect_status = NF_STATE_DISCONNENCTED;
        }

        /* Restore the interrupts. */
        NU_Local_Control_Interrupts(old_level);
    }
    else
    {
        status = NU_INVALID_POINTER;
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
* FUNCTION
*
*       NU_USBF_NDIS_Get_Conn_Status
*
* DESCRIPTION
*
*       This function returns current media connection status of driver to
*       application..
*
* INPUTS
*       ndis           Pointer to user control block.
*       handle         Pointer to the device.
*       status_out     Memory location for placing connect status.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_INVALID_POINTER  Driver is unable to find the device pointer
*                           from its list on the basis of handle.
*
*
*************************************************************************/

STATUS NU_USBF_NDIS_Get_Conn_Status(NU_USBF_NDIS_USER * ndis,
#if (NF_VERSION_COMP >= NF_2_0)
                                    VOID *handle,
#endif
                                    UINT32* status_out)
{
    INT old_level;
    NU_USBF_NDIS_DEVICE *pdevice = NU_NULL;
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    #if (NF_VERSION_COMP >= NF_2_0)
    pdevice = NDISF_Find_Device(ndis, handle);
    #else
    pdevice = &NU_USBF_NDIS_DEV_CB;
    #endif

    if (pdevice)
    {
        /* Lock the interrupts. */
        old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Update status value. */
        *status_out = pdevice->connect_status;

        /* Restore the interrupts. */
        NU_Local_Control_Interrupts(old_level);
    }
    else
    {
        status = NU_INVALID_POINTER;
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBF_NDIS_Set_MAC_Address
*
* DESCRIPTION
*
*       This function sets the MAC address of RNDIS device. Application
*       must call this function within connection event callback in order
*       to send this MAC address as a result of OID request correctly.
*
* INPUTS
*
*       ndis                                Pointer to user control block.
*       handle                              Pointer to the device.
*       buffer                              Buffer containing MAC address.
*       buffer_len                          Length of data in 'buffer'.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Driver is unable to find
*                                           the device pointer from its
*                                           list on the basis of handle.
*
*************************************************************************/
STATUS NU_USBF_NDIS_Set_MAC_Address(NU_USBF_NDIS_USER   *ndis,
                                    VOID                *handle,
                                    UINT8               *buffer,
                                    UINT8               buffer_len)
{
    NU_USBF_NDIS_DEVICE     *pdevice;
    STATUS                  status;

    NU_SUPERV_USER_VARIABLES
    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    pdevice = NU_NULL;
    status  = NU_SUCCESS;

#if (NF_VERSION_COMP >= NF_2_0)
    pdevice = NDISF_Find_Device(ndis, handle);
#else
    pdevice = &NU_USBF_NDIS_DEV_CB;
#endif

    if (pdevice)
    {
        if ( buffer_len == NF_MAC_ADDR_LEN )
        {
            memcpy(pdevice->mac_address, buffer, buffer_len);
        }
        else
        {
            /* If buffer size is not equal to expectation */
            status = NU_INVALID_SIZE;
        }
    }
    else
    {
        status = NU_INVALID_POINTER;
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBF_NDIS_Get_MAC_Address
*
* DESCRIPTION
*
*       This function gets the MAC address of RNDIS device.
*
* INPUTS
*
*       ndis                                Pointer to user control block.
*       handle                              Pointer to the device.
*       mac_address                         Pointer to the returned MAC
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Driver is unable to find
*                                           the device pointer from its
*                                           list on the basis of handle.
*
*************************************************************************/
STATUS NU_USBF_NDIS_Get_MAC_Address(NU_USBF_NDIS_USER   *ndis,
                                    VOID                *handle,
                                    UINT8               *mac_address)
{
    NU_USBF_NDIS_DEVICE     *pdevice;
    STATUS                  status;

    NU_SUPERV_USER_VARIABLES
    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    pdevice = NU_NULL;
    status  = NU_SUCCESS;

#if (NF_VERSION_COMP >= NF_2_0)
    pdevice = NDISF_Find_Device(ndis, handle);
#else
    pdevice = &NU_USBF_NDIS_DEV_CB;
#endif

    if (pdevice)
    {
        /* get mac of current device */
        memcpy(mac_address,pdevice->mac_address, NF_MAC_ADDR_LEN);
    }
    else
    {
        status = NU_INVALID_POINTER;
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_RNDIS_DM_Open
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
STATUS    NU_USBF_RNDIS_DM_Open (VOID* dev_handle)
{
    STATUS      status;
    NU_USBF_NDIS_USER *ndis_cb;

    ndis_cb = (NU_USBF_NDIS_USER*) dev_handle;
    status = NU_USBF_NDIS_USER_Bind_Interface(ndis_cb);

    return (status) ;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_RNDIS_DM_Close
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
STATUS    NU_USBF_RNDIS_DM_Close(VOID* dev_handle)
{
    STATUS      status;
    NU_USBF_NDIS_USER *ndis_cb;

    ndis_cb = (NU_USBF_NDIS_USER*) dev_handle;
    status = NU_USBF_NDIS_USER_Unbind_Interface(ndis_cb);

    return (status) ;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_RNDIS_DM_Read
*
* DESCRIPTION
*
*       This function is called by the application when it wants to read
*       data from the device. In RNDIS case this is a dummy function as
*       reading is done through buffer group registeration in NET driver.
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
STATUS    NU_USBF_RNDIS_DM_Read(
                            VOID*    dev_handle,
                            VOID*    buffer,
                            UINT32   numbyte,
                            OFFSET_T byte_offset,
                            UINT32*  bytes_read_ptr)
{
    STATUS status = NU_SUCCESS;
    return(status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_RNDIS_DM_Write
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
STATUS   NU_USBF_RNDIS_DM_Write(VOID*       dev_handle,
                            const VOID* buffer,
                            UINT32         numbyte,
                            OFFSET_T    byte_offset,
                            UINT32*        bytes_written_ptr)
{
    STATUS status = NU_SUCCESS;
    return(status);
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
STATUS   NU_USBF_RNDIS_DM_IOCTL (
                                  VOID*     dev_handle,
                                  INT       ioctl_num,
                                  VOID*     ioctl_data,
                                  INT       ioctl_data_len)
{
    STATUS    status = DV_IOCTL_INVALID_CMD;
    DV_IOCTL0_STRUCT       *ioctl0;
    NU_USBF_NDIS_USER      *ndis_cb;
    NU_USBF_NDIS_MAC_DATA  *mac_data;

    ndis_cb = (NU_USBF_NDIS_USER*) dev_handle;

    switch(ioctl_num)
    {
        case DV_IOCTL0:
        {
            if (ioctl_data_len == sizeof(DV_IOCTL0_STRUCT))
            {
                /* Get the ioctl0 structure from the data passed in */
                ioctl0          = (DV_IOCTL0_STRUCT *) ioctl_data;
                ioctl0->base    = USB_RNDIS_IOCTL_BASE;
                status          = NU_SUCCESS;
            }
            else
            {
                status = DV_IOCTL_INVALID_LENGTH;
            }
            break;
        }

        case(USB_RNDIS_IOCTL_BASE + USBF_NDIS_USER_SET_MAC):
        {
            mac_data = (NU_USBF_NDIS_MAC_DATA *) ioctl_data;

            status = NU_USBF_NDIS_Set_MAC_Address(ndis_cb,
                                                  mac_data->handle,
                                                  mac_data->mac_addr,
                                                  NF_MAC_ADDR_LEN);
            break;
        }

        case(USB_RNDIS_IOCTL_BASE + USBF_NDIS_USER_GET_MAC):
        {
            mac_data = (NU_USBF_NDIS_MAC_DATA *) ioctl_data;

            status =  NU_USBF_NDIS_Get_MAC_Address(ndis_cb,
                                                   mac_data->handle,
                                                   mac_data->mac_addr);
            break;
        }
    }

    return(status);
}

/* ======================  End Of File  =============================== */
