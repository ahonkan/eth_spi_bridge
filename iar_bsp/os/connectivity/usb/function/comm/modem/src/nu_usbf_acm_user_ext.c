/**************************************************************************
*
*           Copyright 2005 Mentor Graphics Corporation
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
*       nu_usbf_acm_user_ext.c
*
*
* COMPONENT
*
*       Nucleus USB Function Software : ACM User Driver.
*
* DESCRIPTION
*
*       This file contains the external Interfaces exposed by
*       ACM User Driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBF_ACM_USER_Init               Initialize Function ACM User
*                                           driver
*       NU_USBF_USER_ACM_GetHandle          Get handle to the Function ACM
*                                           User driver
*       NU_USBF_USER_ACM_Register_Cb        Register Callbacks with ACM User
*                                           driver
*       NU_USBF_ACM_USER_Create             Creates an instance of
*                                           ACM User Driver.
*       NU_USBF_ACM_USER_Bind_Interface     Binds COMM modem interface
*                                           descriptor with device
*                                           configuration descriptor.
*       NU_USBF_ACM_USER_Unbind_Interface   Unbinds COMM modem interface
*                                           descriptor from device
*                                           configuration descriptor.
*       _NU_USBF_ACM_USER_Delete            Deletes an instance of
*                                           ACM User Driver.
*       _NU_USBF_ACM_USER_Disconnect        Disconnect Notification
*                                           function.
*       _NU_USBF_ACM_USER_Connect           Connect Notification
*                                           function.
*       _NU_USBF_ACM_USER_New_Command       Processes a new command
*       _NU_USBF_ACM_USER_New_Transfer      Processes a new transfer
*       _NU_USBF_ACM_USER_Notify            USB acmf_event
*                                           notification processing
*       _NU_USBF_ACM_DATA_Connect           Connect notification for the data
*                                           interface.
*       _NU_USBF_ACM_DATA_Disconnect        Disconnect notification for the
*                                           data interface.
*       _NU_USBF_ACM_USER_Tx_Done           Transfer completion
*                                           notification
*       NU_USBF_ACM_Update_State            Updates state of modem
*       NU_USBF_ACM_Rem_Mdm_Send_Data       Sends Remote modem to
*                                           Host.
*       NU_USBF_ACM_Send_Resp               Sends AT Command response
*                                           to Host.
*       NU_USBF_ACM_Update_State            Update modem state
*       NU_USBF_ACM_Ring_Notif              Sends Ring Notification to
*                                           Host.
*       NU_USBF_ACM_Par_Err_Notif           Sends Parity Error
*                                           Notification to Host.
*       NU_USBF_ACM_Frm_Err_Notif           Sends Framing Error
*                                           Notification to Host.
*       NU_USBF_ACM_Overrun_Notif           Sends Overrun
*                                           Notification to Host.
*       NU_USBF_ACM_Brk_Detect_Notif        Sends Break Detect
*                                           Notification to Host.
*       NU_USBF_ACM_DSR_Notif               Sends Data Set Ready
*                                           Notification to Host.
*       NU_USBF_ACM_DCD_Notif               Sends Data Carrier Detect
*                                           Notification to Host.
*
*   DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
************************************************************************/

/* ==============  USB Include Files =================================  */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "services/reg_api.h"

char USBF_MDM_Configuration_String[NU_USB_MAX_STRING_LEN];
char USBF_MDM_Interface_String[NU_USB_MAX_STRING_LEN];

UINT8 fs_hs_modem_intf_desc[] =
{
    /* Interface Descriptor. */
    0x09,                                   /* bLength              */
    USB_DT_INTERFACE,                       /* INTERFACE            */
    0x00,                                   /* bInterfaceNumber     */
    0x00,                                   /* bAlternateSetting    */
    0x01,                                   /* bNumEndpoints        */
    COMMF_CLASS,                            /* bInterfaceClass      */
    ACMF_SUBCLASS,                          /* bInterfaceSubClass   */
    0x01,                                   /* bInterfaceProtocol   */
    0x00,                                   /* iInterface           */

    /* Header functional descriptor. */
    0x05,                                   /* bfunctionLength     */
    USB_DT_CLASSSPC,                        /* CS_Interface        */
    0x00,                                   /* bDescriptorSubtype  */
    0x00,                                   /* bcdCDC              */
    0x01,

    /* Abstract control model descriptor. */
    0x04,                                   /* bFunctionLength     */
    USB_DT_CLASSSPC,                        /* bDescriptorType     */
    0x02,                                   /* bDescriptorSubType  */
    0x07,                                   /* bmCapabilities      */

    /* Union descriptor Functional descriptor. */
    0x05,                                   /* bFunctionLength     */
    USB_DT_CLASSSPC,                        /* bDescriptorType     */
    0x06,                                   /* bDescriptorSubtype  */
    0x00,                                   /* bMasterInterface    */
    0x01,                                   /* bSlaveInterface     */

    /* Call management functional descriptor. */
    0x05,                                   /* bFunctionLength     */
    USB_DT_CLASSSPC,                        /* bDescriptorType     */
    0x01,                                   /* bDescriptorSubType  */
    0x03,                                   /* bmCapabilities      */
    0x01,                                   /* bDataInterface      */

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
    USB_DIR_IN,                             /* bEndpointAddress     */
    USB_EP_BULK,                            /* bmAttributes         */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* bInterval            */

    /* Endpoint Descriptors. */

    0x07,                                   /* bLength              */
    USB_DT_ENDPOINT,                        /* ENDPOINT             */
    USB_DIR_OUT,                            /* bEndpointAddress     */
    USB_EP_BULK,                            /* bmAttributes         */
    0x00,                                   /* wMaxPacketSize       */
    0x00,                                   /* wMaxPacketSize       */
    0x00                                    /* bInterval            */
};

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
UINT8 ss_modem_intf_desc[] =
{
    /* Interface Descriptor. */
    0x09,                                   /* bLength              */
    USB_DT_INTERFACE,                       /* INTERFACE            */
    0x00,                                   /* bInterfaceNumber     */
    0x00,                                   /* bAlternateSetting    */
    0x01,                                   /* bNumEndpoints        */
    COMMF_CLASS,                            /* bInterfaceClass      */
    ACMF_SUBCLASS,                          /* bInterfaceSubClass   */
    0x01,                                   /* bInterfaceProtocol   */
    0x00,                                   /* iInterface           */

    /* Header functional descriptor. */
    0x05,                                   /* bfunctionLength     */
    USB_DT_CLASSSPC,                        /* CS_Interface        */
    0x00,                                   /* bDescriptorSubtype  */
    0x00,                                   /* bcdCDC              */
    0x01,

    /* Abstract control model descriptor. */
    0x04,                                   /* bFunctionLength     */
    USB_DT_CLASSSPC,                        /* bDescriptorType     */
    0x02,                                   /* bDescriptorSubType  */
    0x07,                                   /* bmCapabilities      */

    /* Union descriptor Functional descriptor. */
    0x05,                                   /* bFunctionLength     */
    USB_DT_CLASSSPC,                        /* bDescriptorType     */
    0x06,                                   /* bDescriptorSubtype  */
    0x00,                                   /* bMasterInterface    */
    0x01,                                   /* bSlaveInterface     */

    /* Call management functional descriptor. */
    0x05,                                   /* bFunctionLength     */
    USB_DT_CLASSSPC,                        /* bDescriptorType     */
    0x01,                                   /* bDescriptorSubType  */
    0x03,                                   /* bmCapabilities      */
    0x01,                                   /* bDataInterface      */

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
    USB_DIR_IN,                             /* bEndpointAddress     */
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
    0
};
#endif

/* ==========================  Functions ============================== */

/*************************************************************************
* FUNCTION
*
*       NU_USBF_ACM_USER_Init
*
* DESCRIPTION
*
*       ACM User Driver Initialization Function
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
STATUS nu_os_conn_usb_func_comm_mdm_init(CHAR *path, INT startstop)
{
    VOID   *usbf_comm_handle = NU_NULL;
    UINT8   rollback = 0;
    STATUS  status, internal_sts = NU_SUCCESS, reg_status;
    CHAR    usb_func_comm_mdm_path[80];

    usb_func_comm_mdm_path[0] = '\0';
    strcat(usb_func_comm_mdm_path, path);

    /* Save registry settings of USB Function Modem. */
    strcat(usb_func_comm_mdm_path, "/configstring");
    reg_status = REG_Get_String(usb_func_comm_mdm_path, USBF_MDM_Configuration_String, NU_USB_MAX_STRING_LEN);
    if(reg_status == NU_SUCCESS)
    {
        usb_func_comm_mdm_path[0] = '\0';
        strcat(usb_func_comm_mdm_path, path);
        strcat(usb_func_comm_mdm_path, "/interfacestring");
        reg_status = REG_Get_String(usb_func_comm_mdm_path, USBF_MDM_Interface_String, NU_USB_MAX_STRING_LEN);
    }

    if(startstop)
    {
        /* Allocate memory for Modem user driver control block. */
        status = USB_Allocate_Object(sizeof(NU_USBF_ACM_USER),
                                     (VOID**)&NU_USBF_USER_ACM_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 1;
        }

        if (!rollback)
        {
            /* Zero out allocated memory for the block */
            memset(NU_USBF_USER_ACM_Cb_Pt, 0, sizeof(NU_USBF_ACM_USER));

            /* In following API call, passing memory pool ptr parameter
             * NU_NULL because in ReadyStart memory in USB system is
             * allocated through USB specific memory APIs, not directly
             * with any given memory pool pointer. This parameter remains
             * only for backwards code compatibility. */
            status = NU_USBF_ACM_USER_Create(NU_USBF_USER_ACM_Cb_Pt,
                                            "USBF_MDM",
                                            #if (ACMF_VERSION_COMP >= ACMF_2_0)
                                            NU_NULL,
                                            #endif
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
            status = NU_USB_DRVR_Register_User ( (NU_USB_DRVR *) usbf_comm_handle,
                                                 (NU_USB_USER *) NU_USBF_USER_ACM_Cb_Pt);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        if(!rollback)
        {
            /*Register USB Function Modem device with DM. */
            status = NU_USB_SYS_Register_Device((VOID *)NU_USBF_USER_ACM_Cb_Pt,
                                                NU_USBCOMPF_MODEM);
            if(status != NU_SUCCESS)
            {
                rollback = 4;
            }
        }

        /* Clean up in case error occurs. */
        switch (rollback)
        {
            case 4:
            case 3:
                internal_sts = NU_USB_DRVR_Deregister_User ((NU_USB_DRVR *) usbf_comm_handle,
                                                            (NU_USB_USER *) NU_USBF_USER_ACM_Cb_Pt);

            case 2:
                
                if (NU_USBF_USER_ACM_Cb_Pt)
                {
                    internal_sts = _NU_USBF_ACM_USER_Delete ((VOID *) NU_USBF_USER_ACM_Cb_Pt);
                    internal_sts |= USB_Deallocate_Memory((VOID *)NU_USBF_USER_ACM_Cb_Pt);
                    NU_USBF_USER_ACM_Cb_Pt = NU_NULL;
                }

            case 1:
            case 0:
            /* internal_sts is not used after this. So to remove
             * KW and PC-Lint warning set it as unused variable.
             */
            NU_UNUSED_PARAM(internal_sts);
        }
    }
    else
    {
        status = NU_SUCCESS;
    }

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_USER_ACM_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the function Modem
*       user driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the user
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicate there exists a function ACM
*                           user driver.
*       NU_NOT_PRESENT      Indicate there exists no user driver.
*
*************************************************************************/
STATUS NU_USBF_USER_ACM_GetHandle  ( VOID  **handle )
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBF_USER_ACM_Cb_Pt;
    if (NU_USBF_USER_ACM_Cb_Pt == NU_NULL)
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
*       NU_USBF_USER_ACM_Register_Cb
*
*   DESCRIPTION
*
*       This function is used to register call back functions with the
*       user driver.
*
*   INPUTS
*
*       cb                                   Pointer to Driver control
*                                            block.
*       encap_comm_rcv_callback              Pointer to function which
*                                            decodes AT Command
*       new_transfer_callback                Pointer to function which
*                                            completes remaining transfer.
*       app_notify                           Pointer to function which
*                                            notifies events to
*                                            the application.
*
*   OUTPUTS
*
*       NU_SUCCESS                           Indicate Callbacks registered
*                                            successfully
*       NU_USB_INVLD_ARG                     Invalid input argument.
*
*
*************************************************************************/

STATUS   NU_USBF_USER_ACM_Register_Cb (
       NU_USBF_ACM_USER                *cb,
       ACMF_ENCAP_COMM_RCV_CALLBACK    encap_comm_rcv_callback,
       ACMF_NEW_TRANSFER_CALLBACK      new_transfer_callback,
       ACMF_APP_NOTIFY                 app_notify)
{
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    if (cb == NU_NULL ||
        encap_comm_rcv_callback == NU_NULL ||
        new_transfer_callback == NU_NULL ||
        app_notify == NU_NULL)
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        cb->encap_comm_rcv_callback = encap_comm_rcv_callback;
        cb->new_transfer_callback = new_transfer_callback;
        cb->app_notify = app_notify;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
* FUNCTION
*
*       NU_USBF_ACM_USER_Create
*
* DESCRIPTION
*
*       ACM User Driver initialization routine
*
* INPUTS
*
*       cb                                   Pointer to Driver control
*                                            block.
*       name                                 Name of this USB object.
*       pool                                 Memory pool pointer.
*       encap_comm_rcv_callback              Pointer to function which
*                                            decodes AT Command
*       new_transfer_callback                Pointer to function which
*                                            completes remaining transfer.
*       app_notify                           Pointer to function which
*                                            passes notification
* OUTPUTS
*
*       NU_SUCCESS                           Successful Initialization
*       NU_USB_INVLD_ARG                     Invalid input argument.
*
*************************************************************************/

STATUS NU_USBF_ACM_USER_Create(
       NU_USBF_ACM_USER*               cb,
       CHAR*                           name,
       #if (ACMF_VERSION_COMP >= ACMF_2_0)
       NU_MEMORY_POOL                  *pool,
       #endif
       ACMF_ENCAP_COMM_RCV_CALLBACK    encap_comm_rcv_callback,
       ACMF_NEW_TRANSFER_CALLBACK      new_transfer_callback,
       ACMF_APP_NOTIFY                 app_notify)
{
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_USB_MEMPOOLCHK_RETURN(pool);

#if (ACMF_VERSION_COMP >= ACMF_2_0) /* For ACM version 2.0 and greater */
    if (cb == NU_NULL)
         status = NU_USB_INVLD_ARG;
#else
    if (cb == NU_NULL)
         status = NU_USB_INVLD_ARG;
#endif

    if (status == NU_SUCCESS)
    {
        /* For ACM version 2.0 and greater */
        #if (ACMF_VERSION_COMP >= ACMF_2_0)
        /* Assigning memory pool pointer to the passed memory pool. */
        cb->pool = pool;
        #else
        cb->pool = NU_NULL;
        #endif

        cb->encap_comm_rcv_callback = encap_comm_rcv_callback;
        cb->new_transfer_callback = new_transfer_callback;
        cb->app_notify = app_notify;
        cb->acm_list_head = NU_NULL;

        /* Create the acmf_event group here. */
        status = _NU_USBF_USER_COMM_Create ((NU_USBF_USER_COMM * )cb,
                                name,
                                ACMF_SUBCLASS,
                                1,
                                NU_TRUE,
                                &usbf_acm_user_dispatch);

        if(status == NU_SUCCESS)
        {

            /* Create timer for send break signal */
            status = NU_Create_Timer(&cb->timer_send_break,"timer",
            NU_USBF_ACM_NEG_SEND_BRK,(UNSIGNED )&cb->timer_send_break,1,
            0,NU_DISABLE_TIMER);

            if ( status == NU_SUCCESS )
            {
                /* Add modem function with USB device configuration layer.
                     */
                /* Add descriptor for FULL, HIGH and SUPER speed. */
                status = USBF_DEVCFG_Add_Function(USBF_DEF_CONFIG_INDEX,
                                                 fs_hs_modem_intf_desc,
                                                 sizeof(fs_hs_modem_intf_desc),
                                                 fs_hs_modem_intf_desc,
                                                 sizeof(fs_hs_modem_intf_desc),
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                                                 ss_modem_intf_desc,
                                                 sizeof(ss_modem_intf_desc),
#endif
                                                 &cb->acm_function);
            }
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_ACM_USER_Bind_Interface
*
* DESCRIPTION
*
*       This function actually binds COMM modem function with USB device
*       configuration. It is necessary that modem function is
*       already registered with USB device configuration glue layer.
*
* INPUTS
*
*       cb                      Pointer to modem function driver
*                               control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        Indicates invalid argument.
*       NU_USB_NOT_PRESENT      Indicates that modem function
*                               is not registered.
*       NU_SUCCESS              Indicates successful initialization.
*
**************************************************************************/
STATUS NU_USBF_ACM_USER_Bind_Interface(NU_USBF_ACM_USER *cb)
{
    /* Local variables. */
    STATUS     status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;
    if ( cb != NU_NULL )
    {
        status = NU_NOT_PRESENT;
        if ( cb->acm_function != NU_NULL )
        {
            status = NU_USB_DEVICE_Set_bDeviceClass(cb->acm_function->device,
                                                    COMMF_CLASS);
            if ( status == NU_SUCCESS )
            {
                /* Enable modem function. */
                status = USBF_DEVCFG_Enable_Function(cb->acm_function);
                if ( status == NU_SUCCESS )
                {
                    /* Bind modem function with USB device configuration. */
                    status = USBF_DEVCFG_Bind_Function(cb->acm_function);
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
*        NU_USBF_ACM_USER_Unbind_Interface
*
* DESCRIPTION
*
*       This function actually unbinds COMM modem function with USB
*       device configuration.
*       It is necessary that modem function is already registered
*       with USB device configuration glue layer.
*
* INPUTS
*
*       cb                      Pointer to modem function driver
*                               control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        Indicates invalid argument.
*       NU_USB_NOT_PRESENT      Indicates that modem function
*                               is not registered.
*       NU_SUCCESS              Indicates successful initialization.
*
**************************************************************************/
STATUS NU_USBF_ACM_USER_Unbind_Interface(NU_USBF_ACM_USER *cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;
    if ( cb != NU_NULL )
    {
        status = NU_NOT_PRESENT;
        if ( cb->acm_function != NU_NULL )
        {
            status = USBF_DEVCFG_Unbind_Function(cb->acm_function);
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBF_ACM_USER_Delete
*
* DESCRIPTION
*
*       This function deletes an instance of ACM User driver .
*
* INPUTS
*
*       cb              Pointer to the USB Object control block.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*
*************************************************************************/
STATUS _NU_USBF_ACM_USER_Delete(VOID *cb)
{
    STATUS status = NU_SUCCESS;

    /* Remove USB Modem function. */
    status = USBF_DEVCFG_Delete_Function(((NU_USBF_ACM_USER *)cb)->acm_function);

    /* Delete timer. */
    status |= NU_Delete_Timer(&((NU_USBF_ACM_USER*)cb)->timer_send_break);

    /* Call NU_USBF_USER_COMM's Behavior. */
    status |= _NU_USBF_USER_COMM_Delete ((NU_USBF_USER_COMM * )cb);

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBF_ACM_USER_Connect
*
* DESCRIPTION
*
*       This function is called by class driver's Initialize_intf/Device
*       routine when a new device which can be served by this user is
*       found.
*
* INPUTS
*
*       cb                                   Pointer to user control block.
*       class_driver                         Pointer to calling class
*                                            driver's control block.
*       handle                               Handle for the device
*                                            connected.
*
* OUTPUTS
*
*       NU_SUCCESS                           Indicates successful
*                                            completion.
*       NU_USB_MAX_EXCEEDED                  Indicates user is already
*                                            serving maximum devices it
*                                            can support.
*
*************************************************************************/
STATUS _NU_USBF_ACM_USER_Connect(
       NU_USB_USER*    cb,
       NU_USB_DRVR*    class_driver,
       VOID*           handle)
{
    STATUS status = NU_SUCCESS;
    NU_USBF_ACM_USER *user = (NU_USBF_ACM_USER *)cb;
    NU_USBF_ACM_DEV *pdevice = NU_NULL;

    #if (ACMF_VERSION_COMP >= ACMF_2_0)
    /* Allocate and initialize a new instance of ACM device. */
    status = USB_Allocate_Object(sizeof (NU_USBF_ACM_DEV),
                                 (VOID **)&pdevice);
    #else
    pdevice = &NU_USBF_ACM_DEV_CB;
    #endif

    if (status == NU_SUCCESS)
    {
        memset(pdevice, 0, sizeof(NU_USBF_ACM_DEV));
        pdevice->handle = handle;

        /* Assigning default values to acmf_speed, acmf_parity, data bit
         * and stop bits. */
        /* Speed can range from 300 bps to 115,200 bps. */
        pdevice->acmf_speed = 0;
        /* Stop bits can be 1 or 2. */
        pdevice->acmf_stop_bits = 0;
        pdevice->acmf_parity = 0;
        /* Data length can be 7 or 8. */
        pdevice->acmf_data_bits = 8;
        /* On power up, modem is in command state. */
        pdevice->acmf_modem_state = ACMF_MODEM_COMMAND;

        /* Create data_send_event for synchronization purpose */
        status = NU_Create_Event_Group(&(pdevice->data_send_event),
                                       "data_send_event");
        if(status == NU_SUCCESS)
        {
            status = NU_Set_Events(&(pdevice->data_send_event),
                                   ACMF_DATA_SENT,NU_OR);

            NU_Place_On_List((CS_NODE **)&user->acm_list_head, (CS_NODE *)pdevice);

            /* Call the parent's behavior. */
            status = _NU_USBF_USER_COMM_Connect (cb, class_driver,(VOID*)handle);

            if (status != NU_SUCCESS)
            {
                NU_Delete_Event_Group (&(pdevice->data_send_event));
                NU_Remove_From_List((CS_NODE **)&user->acm_list_head, (CS_NODE *)pdevice);
                #if (ACMF_VERSION_COMP >= ACMF_2_0)
                status = USB_Deallocate_Memory (pdevice);
                #endif
            }
        }
        else
        {
            #if (ACMF_VERSION_COMP >= ACMF_2_0)
            status = USB_Deallocate_Memory (pdevice);
            #endif
        }
    }

    return status;
}
/*************************************************************************
* FUNCTION
*
*       _NU_USBF_ACM_USER_Disconnect
*
* DESCRIPTION
*
*       This function is called by the class driver's disconnect function
*       when a device being served by this user gets disconnected. This
*       cleans up the device specific entries associated with the device.
*
* INPUTS
*       cb                                   pointer to user control block.
*       class_driver                         pointer to calling class
*                                            driver's control block.
*       handle                               handle of the device
*                                            disconnected.
*
* OUTPUTS
*       NU_SUCCESS                           Indicates successful
*                                            completion.
*       NU_NOT_PRESENT                       If control block is deleted
*                                            before call completion.
*       NU_INVALID_POINTER                   Driver is unable to find pdev
*                                            pointer on the basis of input
*                                            handle passed as argument from
*                                            the user device list.
*
*
*************************************************************************/
STATUS _NU_USBF_ACM_USER_Disconnect (
       NU_USB_USER*    cb,
       NU_USB_DRVR*    class_driver,
       VOID*           handle)
{
    STATUS status;
    NU_USBF_ACM_DEV *pdev = NU_NULL;
    NU_USBF_ACM_USER *acm = ((NU_USBF_ACM_USER*)cb);

    pdev = ACM_Find_Device((NU_USBF_ACM_USER*)cb, handle);

    /* Call the parent's behavior. */
    status = _NU_USBF_USER_COMM_Disconnect (cb, class_driver, handle);

    if (status == NU_SUCCESS)
    {
        if(pdev)
        {
            NU_Remove_From_List ((CS_NODE **)&acm->acm_list_head,
                                   (CS_NODE*)pdev);

            status = NU_Delete_Event_Group (&(pdev->data_send_event));

            #if (ACMF_VERSION_COMP >= ACMF_2_0)
            status = USB_Deallocate_Memory(pdev);
            #endif
        }
        else
        {
            status = NU_INVALID_POINTER;
        }
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBF_ACM_USER_New_Command
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
*       or from Host is filled in the 'data_out' parameter. The length of
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
*       cb                                   User control block for which
*                                            the command is meant for
*       drvr                                 Class driver invoking this
*                                            function
*       handle                               Identifies for the logical
*                                            function to which the command
*                                            is directed to.
*       command                              Memory location where the
*                                            command block is stored
*       cmd_len                              Length of the command block
*       data_out                             Memory location where the
*                                            data pointer for the transfer
*                                            is to be stored
*       data_len_out                         Memory location where the
*                                            length of data to be
*                                            transferred, in bytes, must
*                                            be filled.
* OUTPUTS
*
*       NU_SUCCESS                           Indicates that the command has
*                                            been processed successfully
*       NU_USB_NOT_SUPPORTED                 Indicates that the command is
*                                            unsupported
*       NU_USB_INVLD_ARG                     Indicates a malformed command
*                                            Block
*
*************************************************************************/
STATUS _NU_USBF_ACM_USER_New_Command(
       NU_USBF_USER*        cb,
       NU_USBF_DRVR*        drvr,
       VOID*                handle,
       UINT8*               command,
       UINT16               cmd_len,
       UINT8**              data_out,
       UINT32*              data_len_out)
{
    STATUS status;
    USBF_COMM_USER_CMD *user_cmd;
    NU_USBF_ACM_USER *acm;
    NU_USBF_ACM_DEV *pdevice = NU_NULL;
    UINT8 *config_buff_ptr;
    ACMF_ENCAP_COMM_RCV_CALLBACK    encap_comm_rcv_callback;
    NU_SUPERV_USER_VARIABLES

#ifdef TIMER
    UINT32 htick1, htick2,interval,i,ticks;
#endif

    user_cmd = (USBF_COMM_USER_CMD *)command;
    NU_ASSERT(cb);
    acm = (NU_USBF_ACM_USER*)cb;
    encap_comm_rcv_callback = acm->encap_comm_rcv_callback;
    status = NU_SUCCESS;

    /* Find pointer to the user device corresponding to
     * this handle.
     */
    pdevice = ACM_Find_Device(acm, handle);

    if(pdevice)
    {
        /* If SEND_ENCAPSULATED_COMMAND is received from Host. */
        if(user_cmd->command == ACMF_SEND_ENCAPSULATED_COMMAND)
        {
            /* Update the data flag */
            pdevice->acmf_data_flag = ACMF_COMM_FLAG;

            /* Process the Class specific request */
            if(encap_comm_rcv_callback)
            {
                /* Switch to user mode before giving callback to application. */
                NU_USER_MODE();

                #if (ACMF_VERSION_COMP >= ACMF_2_0)
                status  = encap_comm_rcv_callback(user_cmd->cmd_data,
                          user_cmd->data_len, (VOID *)handle);
                #else
                status  = encap_comm_rcv_callback(user_cmd->cmd_data,
                          user_cmd->data_len);
                #endif

                /* Switch back to supervisor mode. */
                NU_SUPERVISOR_MODE();
            }
        }
        /* This command is sent by Host when RESPONSE_AVAILABLE
         *  notification is sent to Host. 'User must fill the valid response
         *  value in the 'response_array' before giving this notification.
         */
        else if(user_cmd->command == ACMF_GET_ENCAPSULATED_RESPONSE)
        {
            /* Initialize the response buffer and length. */
            *data_out = pdevice->acmf_resp_buff_ptr;
            *data_len_out = pdevice->acmf_resp_length;
            status = NU_SUCCESS;
        }

        /* Get acmf_speed, acmf_parity, stop bits, data bits from device */
        else if(user_cmd->command == ACMF_GET_LINE_CODING)
        {
            /* Speed is 4 bytes in length */
            pdevice->acmf_config_buff[0] = pdevice->acmf_speed;
            pdevice->acmf_config_buff[1] = (pdevice->acmf_speed) >> 8;
            pdevice->acmf_config_buff[2] = (pdevice->acmf_speed) >> 16;
            pdevice->acmf_config_buff[3] = (pdevice->acmf_speed) >> 24;
            pdevice->acmf_config_buff[4] = pdevice->acmf_parity;
            pdevice->acmf_config_buff[5] = pdevice->acmf_stop_bits;
            pdevice->acmf_config_buff[6] = pdevice->acmf_data_bits;

            *data_out  = &(pdevice->acmf_config_buff[0]);
            *data_len_out = 7;
            status = NU_SUCCESS;
        }

        /* Set acmf_speed, acmf_parity, stop bits, data bits of device */
        else if(user_cmd->command == ACMF_SET_LINE_CODING)
        {
            /* Initialize the Configuration buffer pointer in Data */
            config_buff_ptr=user_cmd->cmd_data;
            pdevice->acmf_speed = config_buff_ptr[3];
            pdevice->acmf_speed = (pdevice->acmf_speed << 8) | config_buff_ptr[2];
            pdevice->acmf_speed = (pdevice->acmf_speed << 8) | config_buff_ptr[1];
            pdevice->acmf_speed = (pdevice->acmf_speed << 8) | config_buff_ptr[0];
            pdevice->acmf_parity = config_buff_ptr[4];
            pdevice->acmf_stop_bits = config_buff_ptr[5];
            pdevice->acmf_data_bits = config_buff_ptr[6];
            status = NU_SUCCESS;
        }

        /* Send serial state notification to host */
        else if (user_cmd->command == ACMF_SET_CONTROL_LINE_STATE)
        {
            /* wValue contains bitmap*/
            /* Check bit 1 from 2nd byte*/
            if(user_cmd->cmd_value & 0x02)
            {
                pdevice->commf_acm_carrier_activate = NU_SUCCESS;
            }
            else
            {
                pdevice->commf_acm_carrier_activate = NU_FALSE;
            }

            /* Check bit 0 from 2nd byte */
            if(user_cmd->cmd_value & 0x01)
            {
                pdevice->acmf_dte_present = NU_SUCCESS;
            }
            else
            {
                pdevice->acmf_dte_present = NU_FALSE;
            }

            status = NU_SUCCESS;

            /*Send notification when wValue is 0.*/
            if(user_cmd->cmd_value == 0x00)
            {
                pdevice->acmf_notif_buff[0] = 2;
                pdevice->acmf_notif_buff[1] = 0;
                status = ACM_Send_Notification(acm, pdevice,ACM_SERIAL_STATE);
            }

            /*Send notification when wValue is 1*/
            if(user_cmd->cmd_value == 0x01)
            {
                pdevice->acmf_notif_buff[0] = 3;
                pdevice->acmf_notif_buff[1] = 0;
                status = ACM_Send_Notification(acm, pdevice,ACM_SERIAL_STATE);
            }

        }
        else if(user_cmd->command == ACMF_CLEAR_COMM_FEATURE)
        {
            status = NU_SUCCESS;
        }
        else if(user_cmd->command == ACMF_SET_COMM_FEATURE)
        {
            if(user_cmd->cmd_value == 0x00)
            {
                /* Reserved Feature Selector*, do nothing */
                status = NU_SUCCESS;
            }
            else if(user_cmd->cmd_value == 0x01)
            {
                 /* Bit D1 represent enables multiplexing of call
                  * management commands over data interface. */
                 if( user_cmd->cmd_data[1] & 0x2)
                 {
                    pdevice->acmf_data_flag = ACMF_DATA_FLAG;
                 }
                 else
                 {
                     pdevice->acmf_data_flag = ACMF_COMM_FLAG;
                 }

                 status = NU_SUCCESS;
            }
            else if(user_cmd->cmd_value == 0x02)
            {
                /* Country specific descriptor not supported by ACM */
                status = NU_USB_INVLD_ARG;
            }
            else
            {
                /* Invalid Feature Selector */
                status = NU_USB_INVLD_ARG;
            }

        }
        else if(user_cmd->command == ACMF_SEND_BREAK)
        {
#ifdef TIMER
            /* This section of code is platform
            * dependent */
            interval = 0;
            ticks=  3687 * user_cmd->cmd_value;
            do
            {
                NU_Retrieve_Hardware_Clock(htick1);
                /* Waste some time in a delay loop*/
                for( i= 0;i < 500;i++);
                    NU_Retrieve_Hardware_Clock(htick2);
                interval = interval + (htick1) - (htick2);
                /* Send the break signal here */

            }
            /* This number is dependent on system clock frequency*/
            while(interval <= ticks);
#endif

#ifndef TIMER
            /* Assert the send break signal here */
            NU_Control_Timer(&acm->timer_send_break,NU_ENABLE_TIMER);

#endif

            /* Stop sending the break signal here */
            status = NU_SUCCESS;
        }
        else if(user_cmd->command == ACMF_GET_COMM_FEATURE)
        {
            if(user_cmd->cmd_value == 0x00)
            {
                /* Reserved Feature Selector, do nothing */
                status = NU_SUCCESS;
            }
            else if(user_cmd->cmd_value == 0x01)
            {
                /* Bit D1 represent enables multiplexing of call
                 * management commands over data interface */
                if(pdevice->acmf_data_flag == ACMF_DATA_FLAG)
                {
                    user_cmd->cmd_data[1] = 1;
                }
                else
                {
                    user_cmd->cmd_data[1] = 0;
                }
                status = NU_SUCCESS;
            }
            else if(user_cmd->cmd_value == 0x02)
            {
                /* Country specific descriptor not supported by ACM */
                status = NU_USB_NOT_SUPPORTED;
            }
            else
            {
                /* Invalid Feature Selector */
                status = NU_USB_NOT_SUPPORTED;
            }
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

   return (status);
}
/*************************************************************************
* FUNCTION
*
*       NU_USBF_ACM_USER_New_Transfer
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
*       cb                                  User control block
*
*       drvr                                Class driver invoking this
*                                           function
*       handle                              Identifies for the logical
*                                           function to which the transfer
*                                           is directed to.
*       data_out                            Memory location where the data
*                                           pointer for the transfer is
*                                           to be stored.
*       data_len_out                        Memory location where the
*                                           length of data to be
*                                           transferred, in bytes,
*                                           must be filled.

* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the function
*                                           has executed successfully
*       NU_USB_INVLD_ARG                    Indicates a unexpected
*                                           transfer request from
*                                           the Host.
*       NU_USB_NOT_SUPPORTED                Indicates that the new
*                                           transfer requests are not
*                                           supported by the user.
*
*************************************************************************/
STATUS _NU_USBF_ACM_USER_New_Transfer(
       NU_USBF_USER*    cb,
       NU_USBF_DRVR*    drvr,
       VOID*            handle,
       UINT8**          data_out,
       UINT32*          data_len_out)
{
    NU_USBF_ACM_USER *acm = (NU_USBF_ACM_USER *)cb;
    NU_USBF_ACM_DEV  *pdevice = NU_NULL;
    STATUS status;
    UINT8 *buffer;
    UINT32 length;
    ACMF_ENCAP_COMM_RCV_CALLBACK    encap_comm_rcv_callback;
    ACMF_NEW_TRANSFER_CALLBACK      new_transfer_callback;
    NU_SUPERV_USER_VARIABLES

    encap_comm_rcv_callback = acm->encap_comm_rcv_callback;
    new_transfer_callback = acm->new_transfer_callback;
    pdevice = ACM_Find_Device(acm, handle);

    if (pdevice != NU_NULL)
    {
        /* If user wants to process the received data now, it can call the
         * function to get the received data parameters otherwise it can
         *  call it on later time.
         */

        /* On power up, the modem is in command mode */
        status = NU_USBF_COMM_DATA_Get_Rcvd(
                                      ((NU_USBF_USER_COMM* )acm)->data_drvr,
                                       &buffer, &length,
                                       handle);

        if(pdevice->acmf_modem_state == ACMF_MODEM_COMMAND)
        {
            /* command is received from the data mode */
            pdevice->acmf_data_flag = ACMF_DATA_FLAG;

            /* Process the AT Command */
            if(encap_comm_rcv_callback != NU_NULL)
            {
                /* Switch to user mode before giving callback to application. */
                NU_USER_MODE();

                #if (ACMF_VERSION_COMP >= ACMF_2_0)
                status = encap_comm_rcv_callback(buffer,length,(VOID*)handle);
                #else
                status = encap_comm_rcv_callback(buffer,length);
                #endif

                /* Switch back to supervisor mode. */
                NU_SUPERVISOR_MODE();
            }
        }

        /* Data is received , Modem is in online mode */
        else
        {
            /* Process the Data  */
            if(new_transfer_callback != NU_NULL)
            {
                /* Switch to user mode before giving callback to application. */
                NU_USER_MODE();

                #if (ACMF_VERSION_COMP >= ACMF_2_0)
                status = new_transfer_callback( (CHAR *)buffer, length,(VOID*)handle);
                #else
                status = new_transfer_callback( (CHAR *)buffer, length);
                #endif

                /* Switch back to supervisor mode. */
                NU_SUPERVISOR_MODE();
            }
        }
    }
    else
        status = NU_USB_INVLD_ARG;

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBF_ACM_USER_Tx_Done
*
* DESCRIPTION
*
*      This function processes a transfer completion notification for a
*      previously submitted data transfer.
*
*      The completed data transfer is described in the 'completed_data'
*      and the 'completed_data_out' parameters. The 'completed_data'
*      contains the pointer to the memory location to / from which
*      'completed_data_out' bytes have been transferred from/to host.
*
*      If there is still data to be transferred to the previous
*      command, then the  location corresponding to the data to be
*      transferred, either to / from Host is filled in the 'data_out'
*      parameter. The length of the data to be transferred is filled
*      in the location pointed to by the 'data_len_out' parameter.
*
*      If there is no data transfer associated with the command, then, the
*      location pointed to by the 'data' is filled in with NU_NULL.
*
* INPUTS
*
*       cb                                   User control block
*
*       drvr                                 Class driver invoking this
*                                            function
*
*       handle                               Identifies for the logical
*                                            function to which the
*                                            notification is directed to.
*       completed_data                       Memory location to / from
*                                            where the data has been
*                                            transferred
*       completed_data_len                   length of data transferred,
*                                            in bytes.
*
*       data_out                             Memory location where the
*                                            data pointer for the
*                                            transfer is to be stored,
*                                            if pending.
*
*       data_len_out                         Memory location where the
*                                            length of data to be
*                                            transferred, in bytes, is to
*                                            be filled.
*
* OUTPUTS
*
*       NU_SUCCESS                           Indicates that the function
*                                            has executed successfully.
*       NU_USB_INVLD_ARG                     Indicates a unexpected
*                                            transfer completion.
*       NU_USB_NOT_SUPPORTED                 Indicates that the transfer
*                                            completion notifications are
*                                            not supported by the user.
*
*
*************************************************************************/
STATUS _NU_USBF_ACM_USER_Tx_Done (
       NU_USBF_USER*      cb,
       NU_USBF_DRVR*      drvr,
       VOID*              handle,
       UINT8*             completed_data,
       UINT32             completed_data_len,
       UINT8**            data_out,
       UINT32*            data_len_out)
{
    STATUS status = NU_USB_NOT_SUPPORTED;
    NU_USBF_ACM_DEV *pdev = NU_NULL;

    COMMF_CMPL_CTX * ctx;

    NU_USBF_ACM_USER *acm = (NU_USBF_ACM_USER *)cb;
    ACMF_APP_NOTIFY   app_notify;
    UINT8   tx_handle;
    NU_SUPERV_USER_VARIABLES

    NU_ASSERT(cb);

    ctx = (COMMF_CMPL_CTX*)handle;
    app_notify = acm->app_notify;

    /* Find user device pointer corresponding to the device
     * handle.
     */

    #if (ACMF_VERSION_COMP >= ACMF_2_0)
    pdev = ACM_Find_Device(acm, ctx->handle);
    #else
    pdev = &NU_USBF_ACM_DEV_CB;
    #endif

    if (pdev == NU_NULL)
    {
        status = NU_USB_INVLD_ARG;
        return (status);
    }

    tx_handle = ctx->transfer_type;

    /* If transmission of data is complete. */
    if(tx_handle == COMMF_DATA_SENT )
    {
        if(pdev)
        {
            if(pdev->acmf_modem_state == ACMF_MODEM_ONLINE)
            {

                status = NU_SUCCESS;

                /* Switch to user mode before giving callback to application. */
                NU_USER_MODE();

                app_notify((NU_USBF_USER*)acm,
                           ACMF_MODEM_TX_DONE,
                           completed_data);

                /* Switch back to supervisor mode. */
                NU_SUPERVISOR_MODE();

                status = NU_Set_Events(&(pdev->data_send_event),ACMF_DATA_SENT,NU_OR);
            }
        }
    }

    /* If last notification is successfully sent to Host. */
    else if(tx_handle == COMMF_NOTIF_SENT)
    {
        if(pdev)
        {
            /* current notification is no more valid. */
            pdev->acmf_curr_notif = COMMF_NO_NOTIF;
            status = NU_SUCCESS;
        }
    }

    /* If callback is received for the GET_ENCAPSULATED_RESPONSE. */
    else if(tx_handle == COMMF_GET_CMD_SENT)
    {
        /* If callback is received for the GET_ENCAPSULATED_RESPONSE.*/
        status = NU_SUCCESS;
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return status;
}
/*************************************************************************
* FUNCTION
*
*       _NU_USBF_ACM_USER_Notify
*
* DESCRIPTION
*
*       This function processes a USB ACMF_EVENT. Examples of such
*       USB acmf_events include suspend and Reset.
*
*       This function carries out function specific processing for the USB
*       acmf_event.
*
*
* INPUTS
*
*       cb                                   User control block for which
*                                            the acmf_event is meant for
*       drvr                                 Class driver invoking this
*                                            function
*       handle                               Identifies for the logical
*                                            function to which the
*                                            notification is directed to.
*       acmf_event                           The USB acmf_event that has
*                                            occurred.
*
* OUTPUTS
*
*       NU_SUCCESS                           Indicates that the acmf_event
*                                            has been processed
*                                            successfully
*       NU_USB_INVLD_ARG                     Indicates a unexpected
*                                            acmf_event.
*       NU_USB_NOT_SUPPORTED                 Indicates that the acmf_event
*                                            notifications are not
*                                            supported by the user.
*
*************************************************************************/
STATUS _NU_USBF_ACM_USER_Notify(
       NU_USBF_USER*          cb,
       NU_USBF_DRVR*          drvr,
       VOID*                  handle,
       UINT32                 acmf_event)
{
    STATUS status = NU_SUCCESS;
    NU_USBF_ACM_USER *acm = (NU_USBF_ACM_USER *)cb;
    NU_USBF_ACM_DEV  *pdev = NU_NULL;
    ACMF_APP_NOTIFY   app_notify;
    NU_SUPERV_USER_VARIABLES

    app_notify = acm->app_notify;

    /* Find user device pointer corresponding to the device
     * handle.
     */

    pdev = ACM_Find_Device(acm, handle);

    if(pdev)
    {
        if((acmf_event == USBF_EVENT_RESET))
        {
            pdev->acmf_modem_state = ACMF_MODEM_COMMAND;
        }

        if(app_notify != NU_NULL)
        {
            /* Switch to user mode before giving callback to application. */
            NU_USER_MODE();

            app_notify((NU_USBF_USER *)cb, acmf_event, (VOID*)handle);

            /* Switch back to supervisor mode. */
            NU_SUPERVISOR_MODE();
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*               _NU_USBF_ACM_DATA_Disconnect
*
* DESCRIPTION
*       This function is called by the class driver's disconnect function
*       when a device being served by this user gets disconnected. This
*       cleans up the device specific entries
*       associated with the device.
*
* INPUTS
*       cb                                  Pointer to user control block.
*       class_driver                        Pointer to calling class
*                                           driver's control block.
*       handle                              Handle of the device
*                                           disconnected.
*
* OUTPUTS
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_NOT_PRESENT                      If control block is deleted
*                                           before call completion.
*
*************************************************************************/
STATUS _NU_USBF_ACM_DATA_Disconnect(
       NU_USB_USER*     cb,
       NU_USB_DRVR*     class_driver,
       VOID*            handle)
{
    STATUS status;
    NU_USBF_ACM_USER * user = (NU_USBF_ACM_USER *) cb;
    ACMF_APP_NOTIFY   app_notify;
    NU_SUPERV_USER_VARIABLES

    app_notify = user->app_notify;

    /* Call the parent's behavior. */
    status = _NU_USBF_USER_COMM_DATA_Discon(cb,class_driver,handle);

    if( (status == NU_SUCCESS) && (app_notify))
    {
        /* Switch to user mode before giving callback to application. */
        NU_USER_MODE();

        app_notify((NU_USBF_USER *)cb,
                   COMMF_EVENT_USER_DEINIT,(VOID*)handle);

        /* Switch back to supervisor mode. */
        NU_SUPERVISOR_MODE();
    }

    return status;
}

/*************************************************************************
* FUNCTION
*               _NU_USBF_ACM_DATA_Connect
*
* DESCRIPTION
*       This function is called by class driver's Initialize_intf/Device
*       routine when a
*       new device which can be served by this user is found.
*
* INPUTS
*       cb                                  Pointer to user control block.
*       class_driver                        Pointer to calling class
*                                           driver's control block.
*       handle                              Handle for the device
*                                           connected.
*
* OUTPUTS
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_USB_MAX_EXCEEDED                 Indicates user is already
*                                           serving maximum devices it can
*                                           support.
*       NU_INVALID_POINTER                  Driver is unable to find pdev
*                                           pointer on the basis of input
*                                           handle passed as argument from
*                                           the user device list.
*
*
*************************************************************************/
STATUS _NU_USBF_ACM_DATA_Connect(
       NU_USB_USER*       cb,
       NU_USB_DRVR*       class_driver,
       VOID*              handle)
{
    NU_USBF_ACM_USER * user = (NU_USBF_ACM_USER *) cb;
    ACMF_APP_NOTIFY   app_notify;
    STATUS status;
    NU_SUPERV_USER_VARIABLES

    app_notify = user->app_notify;

    NU_ASSERT(cb);

    /* Every time DATA_Connect callback is called from Class driver, that
     * means Host has select the default alternate setting of Data class
     * Interface and selected the working alternate setting again. This
     * has the effect of resetting the device, so it should clear all
     * filters, buffers and statistics.
     */

    status = _NU_USBF_USER_COMM_DATA_Connect (cb,class_driver,handle);

    if( (status == NU_SUCCESS) && (app_notify))
    {
        /* Switch to user mode before giving callback to application. */
        NU_USER_MODE();

        app_notify((NU_USBF_USER *)cb,
                   COMMF_EVENT_USER_INIT,(VOID*)handle);

        /* Switch back to supervisor mode. */
        NU_SUPERVISOR_MODE();
    }

    return (status);
}
/*************************************************************************
* FUNCTION
*
*       NU_USBF_ACM_Rem_Mdm_Send_Data
*
* DESCRIPTION
*
*       This function is called when remote modem
*       data is needed to be sent to the host.
*
*   INPUTS
*
*       acm                                 Pointer to user control block.
*       buffer                              Pointer to hold Remote modem
*                                           data.
*       handle                              Identifier for the logical
*                                           function to which the transfer
*                                           is directed to.
*       len                                 Remote modem data length
*
* OUTPUTS
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Driver is unable to find pdev
*                                           pointer on the basis of input
*                                           handle passed as argument from
*                                           the user device list.
*
*
*************************************************************************/

STATUS NU_USBF_ACM_Rem_Mdm_Send_Data(
       NU_USBF_ACM_USER*        acm,
#if (ACMF_VERSION_COMP >= ACMF_2_0)
       VOID*                    handle,
#endif
       CHAR*                    info_buffer,
       UINT32                   info_length)
{
    STATUS status = NU_SUCCESS;
    UNSIGNED retrieved_events;
    NU_USBF_ACM_DEV *pdev = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Find user device pointer corresponding to the device
     * handle.
     */
    #if (ACMF_VERSION_COMP >= ACMF_2_0)
    pdev = ACM_Find_Device(acm, handle);
    #else
    pdev = &NU_USBF_ACM_DEV_CB;
    #endif

    if(pdev)
    {
        status = NU_Retrieve_Events(&(pdev->data_send_event),ACMF_DATA_SENT,
                   NU_OR_CONSUME,&retrieved_events,NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            status = NU_USBF_COMM_DATA_Send(
                    ((NU_USBF_USER_COMM *)acm)->data_drvr,
                    (UINT8 *)info_buffer, info_length,
                    pdev->handle);
        }
    }
    else
        status = NU_INVALID_POINTER;

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
* FUNCTION
*
*       NU_USBF_ACM_Send_Resp
*
* DESCRIPTION
*
*       This function is called when response need
*       to sent to an AT Command from the host.
*
*   INPUTS
*
*       acm                 Pointer to user control block.
*       handle              Identifier for the logical
*                           function to which the transfer
*                           is directed to.
*       buffer              Pointer to hold AT Command Response.
*       len                 AT Command Response length
*
* OUTPUTS
*
*       NU_SUCCESS          Indicates successful completion.
*       NU_INVALID_POINTER  Driver is unable to find pdev
*                           pointer on the basis of input
*                           handle passed as argument from
*                           the user device list.
*
*
*************************************************************************/
STATUS NU_USBF_ACM_Send_Resp(
       NU_USBF_ACM_USER*   acm,
#if (ACMF_VERSION_COMP >= ACMF_2_0)
       VOID*               handle,
#endif
       UINT8*              buffer,
       UINT32              len)
{
    /* Save flag internally for COMM or data. and check here to select
     * desired method. */
    STATUS status_flag = NU_SUCCESS;
    NU_USBF_ACM_DEV *pdev = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Find user device pointer corresponding to the device
     * handle.
     */
    #if (ACMF_VERSION_COMP >= ACMF_2_0)
    pdev = ACM_Find_Device(acm, handle);
    #else
    pdev = &NU_USBF_ACM_DEV_CB;
    #endif

    if(pdev)
    {
        if(pdev->acmf_data_flag == ACMF_DATA_FLAG)
        {
              status_flag = NU_USBF_COMM_DATA_Send(
                             ((NU_USBF_USER_COMM *)acm)->data_drvr,buffer,
                             len,
                             pdev->handle);
        }
        else
        {
              pdev->acmf_resp_buff_ptr = buffer;
              pdev->acmf_resp_length = len;

              /*Call Notification */
              status_flag = ACM_Send_Notification(acm,pdev,
                                                  ACM_RESPONSE_AVAILABLE);
        }
    }
    else
        status_flag = NU_INVALID_POINTER;

    /* Revert to user mode. */
    NU_USER_MODE();

    return status_flag;
}
/*************************************************************************
* FUNCTION
*
*       NU_USBF_ACM_Update_State
*
* DESCRIPTION
*
*       This function is called when modem state is changed
*       from command mode to online mode and vice-versa
*
* INPUTS
*
*       acm                                 Pointer to user control block.
*       handle                              Identifier for the logical
*                                           function to which the transfer
*                                           is directed to.
*       state                               UINT8 to hold either 1 or 2.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Driver is unable to find pdev
*                                           pointer on the basis of input
*                                           handle passed as argument from
*                                           the user device list.
*
*************************************************************************/
STATUS NU_USBF_ACM_Update_State(
       NU_USBF_ACM_USER*  acm,
       #if (ACMF_VERSION_COMP >= ACMF_2_0)
       VOID* handle,
       #endif
       UINT8              state)
{
    NU_USBF_ACM_DEV *pdev = NU_NULL;
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Find user device pointer corresponding to the device
     * handle.
     */

    #if (ACMF_VERSION_COMP >= ACMF_2_0)
    pdev = ACM_Find_Device(acm, handle);
    #else
    pdev = &NU_USBF_ACM_DEV_CB;
    #endif

    if (pdev)
    {
        if (state == ACMF_MODEM_COMMAND)
        {
            pdev->acmf_modem_state = ACMF_MODEM_COMMAND;
        }
        else
        {
            pdev->acmf_modem_state = ACMF_MODEM_ONLINE;
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
*       NU_USBF_ACM_Ring_Notif
*
* DESCRIPTION
*
*       This function is called when ring signal is detected
*
* INPUTS
*
*       acm                                 Pointer to user control block.
*       handle                              Identifier for the logical
*                                           function to which the transfer
*                                           is directed to.
*       bit                                 Boolean to hold either 0 or 1.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Driver is unable to find pdev
*                                           pointer on the basis of input
*                                           handle passed as argument from
*                                           the user device list.
*
*************************************************************************/
STATUS NU_USBF_ACM_Ring_Notif(
       NU_USBF_ACM_USER*        acm,
       #if (ACMF_VERSION_COMP >= ACMF_2_0)
       VOID* handle,
       #endif
       BOOLEAN                  bit)
{
    STATUS status = NU_SUCCESS;
    NU_USBF_ACM_DEV *pdev = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Find user device pointer corresponding to the device
     * handle.
     */
    #if (ACMF_VERSION_COMP >= ACMF_2_0)
    pdev = ACM_Find_Device(acm, handle);
    #else
    pdev = &NU_USBF_ACM_DEV_CB;
    #endif

    if(pdev)
    {
        /* Send Ring indication to host*/
        pdev->acmf_notif_buff[0] = (pdev->acmf_notif_buff[0]) |(bit<<3);
        pdev->acmf_notif_buff[1] = 0;

        status = ACM_Send_Notification(acm,pdev,ACM_SERIAL_STATE);
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
*       NU_USBF_ACM_Par_Err_Notif
*
* DESCRIPTION
*
*       This function is called when Parity error is detected
*
* INPUTS
*
*       acm                                 Pointer to user control block.
*       handle                              Identifier for the logical
*                                           function to which the transfer
*                                           is directed to.
*       bit                                 Boolean to hold either 0 or 1.
*
* OUTPUTS
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Driver is unable to find pdev
*                                           pointer on the basis of input
*                                           handle passed as argument from
*                                           the user device list.
*
*************************************************************************/
STATUS NU_USBF_ACM_Par_Err_Notif(
       NU_USBF_ACM_USER*     acm,
#if (ACMF_VERSION_COMP >= ACMF_2_0)
       VOID* handle,
#endif
       BOOLEAN               bit)
{
    NU_USBF_ACM_DEV *pdev = NU_NULL;
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Find user device pointer corresponding to the device
     * handle.
     */
    #if (ACMF_VERSION_COMP >= ACMF_2_0)
    pdev = ACM_Find_Device(acm, handle);
    #else
    pdev = &NU_USBF_ACM_DEV_CB;
    #endif

    if (pdev)
    {
        /* Send Parity error notification to host*/
        pdev->acmf_notif_buff[0] = (pdev->acmf_notif_buff[0]) |(bit << 5);
        pdev->acmf_notif_buff[1] = 0;

        status = ACM_Send_Notification(acm,pdev,ACM_SERIAL_STATE);
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
*       NU_USBF_ACM_Frm_Err_Notif
*
* DESCRIPTION
*
*       This function is called when Framing error is detected
*
* INPUTS
*
*       acm                                 Pointer to user control block.
*       handle                              Identifier for the logical
*                                           function to which the transfer
*                                           is directed to.
*       bit                                 Boolean to hold either 0 or 1.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Driver is unable to find pdev
*                                           pointer on the basis of input
*                                           handle passed as argument from
*                                           the user device list.
*
*************************************************************************/
STATUS NU_USBF_ACM_Frm_Err_Notif(
       NU_USBF_ACM_USER*      acm,
       #if (ACMF_VERSION_COMP >= ACMF_2_0)
       VOID* handle,
       #endif
       BOOLEAN                bit)
{
    NU_USBF_ACM_DEV *pdev = NU_NULL;
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Find user device pointer corresponding to the device
     * handle.
     */
    #if (ACMF_VERSION_COMP >= ACMF_2_0)
    pdev = ACM_Find_Device(acm, handle);
    #else
    pdev = &NU_USBF_ACM_DEV_CB;
    #endif

    if (pdev)
    {
        /* Send Framing error notification to host*/
        pdev->acmf_notif_buff[0] =(pdev->acmf_notif_buff[0]) |(bit<<4);
        pdev->acmf_notif_buff[1] = 0;

        status = ACM_Send_Notification(acm,pdev,ACM_SERIAL_STATE);
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
*       NU_USBF_ACM_Overrun_Notif
*
* DESCRIPTION
*
*       This function is called when Overrun
*       is detected
*
* INPUTS
*
*       acm                                 Pointer to user control block.
*       handle                              Identifier for the logical
*                                           function to which the transfer
*                                           is directed to.
*       bit                                 Boolean to hold either 0 or 1.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Driver is unable to find pdev
*                                           pointer on the basis of input
*                                           handle passed as argument from
*                                           the user device list.
*
*************************************************************************/
STATUS NU_USBF_ACM_Overrun_Notif(
       NU_USBF_ACM_USER*      acm,
       #if (ACMF_VERSION_COMP >= ACMF_2_0)
       VOID* handle,
       #endif
       BOOLEAN                bit)
{
    NU_USBF_ACM_DEV *pdev = NU_NULL;
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Find user device pointer corresponding to the device
     * handle.
     */
    #if (ACMF_VERSION_COMP >= ACMF_2_0)
    pdev = ACM_Find_Device(acm, handle);
    #else
    pdev = &NU_USBF_ACM_DEV_CB;
    #endif

    if (pdev)
    {
        /* Send Overrun notification to host*/
        pdev->acmf_notif_buff[0] = (pdev->acmf_notif_buff[0]) |(bit<<6);
        pdev->acmf_notif_buff[1] = 0;
        status = ACM_Send_Notification(acm, pdev,ACM_SERIAL_STATE);
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
*       NU_USBF_ACM_Brk_Detect_Notif
*
* DESCRIPTION
*
*       This function is called when Break
*       is detected
*
* INPUTS
*
*       acm                                 Pointer to user control block.
*       handle                              Identifier for the logical
*                                           function to which the transfer
*                                           is directed to.
*       bit                                 Boolean to hold either 0 or 1.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Driver is unable to find pdev
*                                           pointer on the basis of input
*                                           handle passed as argument from
*                                           the user device list.
*
*************************************************************************/
STATUS NU_USBF_ACM_Brk_Detect_Notif(
       NU_USBF_ACM_USER*          acm,
       #if (ACMF_VERSION_COMP >= ACMF_2_0)
       VOID* handle,
       #endif
       BOOLEAN                    bit)
{
    NU_USBF_ACM_DEV *pdev = NU_NULL;
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Find user device pointer corresponding to the device
     * handle.
     */
    #if (ACMF_VERSION_COMP >= ACMF_2_0)
    pdev = ACM_Find_Device(acm, handle);
    #else
    pdev = &NU_USBF_ACM_DEV_CB;
    #endif

    if (pdev)
    {
        /* Send Break detect notification to host*/
        pdev->acmf_notif_buff[0] = (pdev->acmf_notif_buff[0]) |(bit<<2);
        pdev->acmf_notif_buff[1] = 0;
        status = ACM_Send_Notification(acm, pdev,ACM_SERIAL_STATE);
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
*       NU_USBF_ACM_DSR_Notif
*
* DESCRIPTION
*
*       This function is called when status of
*       Data Set Ready (DSR) changes.
*
*
* INPUTS
*
*       acm                                 Pointer to user control block.
*       handle                              Identifier for the logical
*                                           function to which the transfer
*                                           is directed to.
*       bit                                 Boolean to hold either 0 or 1.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Driver is unable to find pdev
*                                           pointer on the basis of input
*                                           handle passed as argument from
*                                           the user device list.
*
*************************************************************************/
STATUS NU_USBF_ACM_DSR_Notif(
       NU_USBF_ACM_USER*      acm,
       #if (ACMF_VERSION_COMP >= ACMF_2_0)
       VOID* handle,
       #endif
       BOOLEAN                bit)
{
    NU_USBF_ACM_DEV *pdev = NU_NULL;
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Find user device pointer corresponding to the device
     * handle.
     */
    #if (ACMF_VERSION_COMP >= ACMF_2_0)
    pdev = ACM_Find_Device(acm, handle);
    #else
    pdev = &NU_USBF_ACM_DEV_CB;
    #endif

    if (pdev)
    {
        /* Send Data Set Ready (DSR) notification to host*/
        pdev->acmf_notif_buff[0] = (pdev->acmf_notif_buff[0]) | (bit << 1);
        pdev->acmf_notif_buff[1] = 0;

        status = ACM_Send_Notification(acm, pdev,ACM_SERIAL_STATE);
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
*       NU_USBF_ACM_DCD_Notif
*
* DESCRIPTION
*
*       This function is called when status of Received signal line
*       detector hangs.
*
*
* INPUTS
*
*       acm                                 Pointer to user control block.
*       handle                              Identifier for the logical
*                                           function to which the transfer
*                                           is directed to.
*       bit                                 Boolean to hold either 0 or 1.
*
* OUTPUTS
*       NU_SUCCESS                          Indicates successful
*                                           completion.
*       NU_INVALID_POINTER                  Driver is unable to find pdev
*                                           pointer on the basis of input
*                                           handle passed as argument from
*                                           the user device list.
*
*************************************************************************/
STATUS NU_USBF_ACM_DCD_Notif(
       NU_USBF_ACM_USER*      acm,
       #if (ACMF_VERSION_COMP >= ACMF_2_0)
       VOID* handle,
       #endif
       BOOLEAN                bit)
{
    NU_USBF_ACM_DEV *pdev = NU_NULL;
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Find user device pointer corresponding to the device
     * handle.
     */
    #if (ACMF_VERSION_COMP >= ACMF_2_0)
    pdev = ACM_Find_Device(acm, handle);
    #else
    pdev = &NU_USBF_ACM_DEV_CB;
    #endif
    if (pdev)
    {
        /* Send Carrier Detect Notification to host*/
        if(bit) /* DTR ASSERTED */
        {
            pdev->acmf_notif_buff[0] = (pdev->acmf_notif_buff[0]) | 0x01;
        }
        else /* DTR DE-ASSERTED */
        {
            pdev->acmf_notif_buff[0] = (pdev->acmf_notif_buff[0]) & 0xFE;;
        }
        pdev->acmf_notif_buff[1] = 0;

        status = ACM_Send_Notification(acm,pdev, ACM_SERIAL_STATE);
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
*       NU_USBF_ACM_NEG_SEND_BRK
*
* DESCRIPTION
*
*       This function is called when status of Received signal line
*       detector hangs.
*
*
* INPUTS
*
*       data                                Pointer to user control block.
*
* OUTPUTS
*
*
*************************************************************************/
VOID NU_USBF_ACM_NEG_SEND_BRK (UNSIGNED data)
{

    NU_TIMER *timer = (NU_TIMER *)data;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* De-assert the break signal in this routine */

    /* Disable the timer block here */
    NU_Control_Timer(timer,NU_DISABLE_TIMER);

    /* Revert to user mode. */
    NU_USER_MODE();
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_ACM_DM_Open
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
STATUS  NU_USBF_ACM_DM_Open (VOID* dev_handle)
{
    STATUS      status;
    NU_USBF_ACM_USER *acm_cb;

    acm_cb = (NU_USBF_ACM_USER*) dev_handle;
    status = NU_USBF_ACM_USER_Bind_Interface(acm_cb);

    return (status) ;
}
/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_ACM_DM_Close
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
STATUS  NU_USBF_ACM_DM_Close(VOID* dev_handle)
{
    STATUS      status;
    NU_USBF_ACM_USER *acm_cb;

    acm_cb = (NU_USBF_ACM_USER*) dev_handle;
    status = NU_USBF_ACM_USER_Unbind_Interface(acm_cb);

    return (status) ;
}
/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_ACM_DM_Read
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
STATUS  NU_USBF_ACM_DM_Read(VOID*    dev_handle,
                            VOID*    buffer,
                            UINT32   numbyte,
                            OFFSET_T byte_offset,
                            UINT32*  bytes_read_ptr)
{
    STATUS status = NU_SUCCESS;
    return (status);
}
/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_ACM_DM_Write
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
STATUS  NU_USBF_ACM_DM_Write(VOID*       dev_handle,
                             const VOID* buffer,
                             UINT32      numbyte,
                             OFFSET_T    byte_offset,
                             UINT32*     bytes_written_ptr)
{
    STATUS status = NU_SUCCESS;
    return (status);
}
/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_ACM_DM_IOCTL
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
STATUS  NU_USBF_ACM_DM_IOCTL(VOID*     dev_handle,
                             INT       ioctl_num,
                             VOID*     ioctl_data,
                             INT       ioctl_data_len)
{
    STATUS status;
    NU_USBF_ACM_USER     *acm_cb = (NU_USBF_ACM_USER * )dev_handle;
    USBF_COMM_DATA_PARAM *usbf_comm_data_param;
    USBF_ACM_CALLBACKS   *usbf_acm_callbacks;
    USBF_ACM_DATA        *acm_data;

    switch(ioctl_num)
    {
        /******************************** *
         * USB Function Modem User IOCTLS *
         ******************************** */
        case (NU_USBF_ACM_IOCTL_BASE + NU_USBF_ACM_IOCTL_REGISTER_CB):
            usbf_acm_callbacks = (USBF_ACM_CALLBACKS *)ioctl_data;
            status = NU_USBF_USER_ACM_Register_Cb(acm_cb,
                                                  usbf_acm_callbacks->encap_comm_rcv_callback,
                                                  usbf_acm_callbacks->new_transfer_callback,
                                                  usbf_acm_callbacks->app_notify);
            break;

        case (NU_USBF_ACM_IOCTL_BASE + NU_USBF_ACM_IOCTL_UPDATE_STATE):
            acm_data = (USBF_ACM_DATA *)ioctl_data;
            status = NU_USBF_ACM_Update_State(acm_cb,
                                              acm_data->dev_handle,
                                              acm_data->state);
            break;

        case (NU_USBF_ACM_IOCTL_BASE + NU_USBF_ACM_IOCTL_SEND_RESP):
            acm_data = (USBF_ACM_DATA *)ioctl_data;
            status = NU_USBF_ACM_Send_Resp(acm_cb,
                                           acm_data->dev_handle,
                                           acm_data->buffer,
                                           acm_data->length);
            break;

        case (NU_USBF_ACM_IOCTL_BASE + NU_USBF_ACM_IOCTL_REM_MDM_SEND_DATA):
            acm_data = (USBF_ACM_DATA *)ioctl_data;
            status = NU_USBF_ACM_Rem_Mdm_Send_Data(acm_cb,
                                                   acm_data->dev_handle,
                                                   (CHAR *)acm_data->buffer,
                                                   acm_data->length);
            break;

        /********************************
         * USB Function COMM Data IOCTLS *
         ********************************/
        case (NU_USBF_ACM_IOCTL_BASE + NU_USBF_ACM_IOCTL_GET_COMM_DATA):
            status = NU_USBF_COMM_DATA_GetHandle((VOID **)ioctl_data);
            break;

        case (NU_USBF_ACM_IOCTL_BASE + NU_USBF_ACM_IOCTL_REG_RX_BUF):
            usbf_comm_data_param = (USBF_COMM_DATA_PARAM *)ioctl_data;
            status = NU_USBF_COMM_DATA_Reg_Rx_Buffer(
                                      (NU_USBF_COMM_DATA *)usbf_comm_data_param->comm_data_drvr_ptr,
                                      usbf_comm_data_param->dev_handle,
                                      usbf_comm_data_param->param_ptr);
            break;

        case (NU_USBF_ACM_IOCTL_BASE + NU_USBF_ACM_IOCTL_CONFIG_XFERS):
            usbf_comm_data_param = (USBF_COMM_DATA_PARAM *)ioctl_data;
            status = NU_USBF_COMM_DATA_Config_Xfers(
                                      (NU_USBF_COMM_DATA *)usbf_comm_data_param->comm_data_drvr_ptr,
                                      usbf_comm_data_param->dev_handle,
                                      usbf_comm_data_param->param_ptr);
            break;

        case (NU_USBF_ACM_IOCTL_BASE + NU_USBF_ACM_IOCTL_DIS_RECEPTION):
            usbf_comm_data_param = (USBF_COMM_DATA_PARAM *)ioctl_data;
            status = NU_USBF_COMM_DATA_Dis_Reception(
                                      (NU_USBF_COMM_DATA *)usbf_comm_data_param->comm_data_drvr_ptr,
                                      usbf_comm_data_param->dev_handle);
            break;

        default:
        {
            status = NU_USB_NOT_SUPPORTED;
            break;
        }
    }

    return (status);
}
/* ======================  End Of File  =============================== */

