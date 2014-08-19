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
*       nu_usbf_eth_ext.c
*
* COMPONENT
*
*       Nucleus USB Function Software : Ethernet User Driver.
*
* DESCRIPTION
*
*       This file contains the external Interfaces exposed by
*       Ethernet User Driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*       nu_os_conn_usb_func_comm_eth_init   Initializes the component of
*                                           function ETH User Driver.
*       NU_USBF_ETH_GetHandle               Gets handle of function ETH
*                                           User driver.
*       NU_USBF_ETH_Create                  Creates an instance of ETH User
*                                           Driver.
*       NU_USBF_ETH_Bind_Interface          Binds COMM ethernet interface
*                                           descriptor with device
*                                           configuration descriptor.
*       NU_USBF_ETH_Unbind_Interface        Unbinds COMM ethernet interface
*                                           descriptor from device
*                                           configuration descriptor.
*       _NU_USBF_ETH_Delete                 Deletes an instance of ETH User
*                                           Driver.
*       _NU_USBF_ETH_Connect                Connect Notification function.
*       _NU_USBF_ETH_Disconnect             Disconnect Notification
*                                           function.
*       _NU_USBF_ETH_New_Command            Processes a new command.
*       _NU_USBF_ETH_New_Transfer           Processes a new transfer.
*       _NU_USBF_ETH_Tx_Done                Transfer completion
*                                           notification.
*       _NU_USBF_ETH_Notify                 USB event notification
*                                           processing.
*       _NU_USBF_ETH_DATA_Disconnect        Data Disconnect Notification
*                                           function.
*       _NU_USBF_ETH_DATA_Connect           Data Connect Notification
*                                           function.
*       NU_USBF_ETH_Send_Notif              Sends notification to Host.
*       NU_USBF_ETH_Send_Data               Give request to send ethernet
*                                           data.
*       NU_USBF_ETH_Send_Connection         Send connection notification to
*                                           Host.
*       NU_USBF_ETH_Send_Encap_Resp         Send encapsulated response to
*                                           Host.
*       NU_USBF_ETH_Send_Disconnection      Send disconnection notification
*                                           to Host.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*       nucleus.h
*       nu_kernel.h
*       reg_api.h
*       runlevel_init.h
*
**************************************************************************/

/* ==============  USB Include Files =================================  */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "services/reg_api.h"
#include "services/runlevel_init.h"

CHAR USBF_ETH_Configuration_String[NU_USB_MAX_STRING_LEN];
CHAR USBF_ETH_Interface_String[NU_USB_MAX_STRING_LEN];
CHAR USBF_ETH_Mac_Address[18];
UINT16 USBF_ETH_Max_Frame_Size;

/* Internal function used for ETH device initialization. */
STATUS NU_USBF_ETH_DEV_Init(CHAR *path);

#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
    /* If device discovery task has lower priority
     * than device is not yet discovered.
     * We need a new task that waits for device
     * initialized event and initializes
     * ETH device.
     */
    static NU_TASK  *ETH_Dev_Init_Task = NU_NULL;
    /* Path to ETH registry. */
    static CHAR     eth_reg_path[REG_MAX_KEY_LENGTH];
    static VOID    *eth_init_task_stack_ptr = NU_NULL;

    #define USBF_ETH_DEV_INIT_TASK_STACK_SIZE (1024)
    #define USBF_ETH_DEV_INIT_TASK_PRIORITY   17

    /* ETH device init task function. */
    VOID NU_USBF_ETH_DEV_Init_Task(UNSIGNED, VOID *);
#endif /* DV_DEV_DISCOVERY_TASK_ENB */


UINT8 fs_hs_eth_intf_desc[] =
{
    /* Interface Descriptor. */
    0x09,                                       /* bLength              */
    USB_DT_INTERFACE,                           /* INTERFACE            */
    0x00,                                       /* bInterfaceNumber     */
    0x00,                                       /* bAlternateSetting    */
    0x01,                                       /* bNumEndpoints        */
    COMMF_CLASS,                                /* bInterfaceClass      */
    EF_SUBCLASS,                                /* bInterfaceSubClass   */
    0x00,                                       /* bInterfaceProtocol   */
    0x00,                                       /* iInterface           */

    /* Class specific functional descriptor (Header). */
    0x05,                                       /* bFunctionLength      */
    USB_DT_CLASSSPC,                            /* bDescriptorType      */
    0x00,                                       /* bDescriptorSubtype   */
    0x10,                                       /* bcdCDCLo             */
    0x01,                                       /* bcdCDCHi             */

    /* Class specific functional descriptor (Union). */
    0x05,                                       /* bFunctionLength      */
    USB_DT_CLASSSPC,                            /* bDescriptorType      */
    0x06,                                       /* bDescriptorSubtype   */
    0x00,                                       /* bMasterInterface     */
    0x01,                                       /* bSlaveInterface0     */

    /* Class specific functional descriptor (Ethernet Networking). */
    0x0D,                                       /* bFunctionLength      */
    USB_DT_CLASSSPC,                            /* bDescriptorType      */
    0x0F,                                       /* bDescriptorSubtype   */
    0x01,                                       /* iMACAddress          */
    0x00,                                       /* bmEthernetStatistics0*/
    0x00,                                       /* bmEthernetStatistics1*/
    0x00,                                       /* bmEthernetStatistics2*/
    0x00,                                       /* bmEthernetStatistics3*/
    (UINT8)(ETHF_MAX_SEG_SIZE&0xFF),            /* Maximum Segment Size LO  */
    (UINT8)(ETHF_MAX_SEG_SIZE>>8),              /* Maximum Segment Size HI  */
    0x00,                                       /* wNumberMCFiltersLo   */
    0x00,                                       /* wNumberMCFiltersHi   */
    0x00,                                       /* bNumberPowerFilters  */

    /* Endpoint Descriptors. */
    0x07,                                       /* bLength              */
    USB_DT_ENDPOINT,                            /* ENDPOINT             */
    USB_DIR_IN,                                 /* bEndpointAddress     */
    USB_EP_INTR,                                /* bmAttributes         */
    0x00,                                       /* wMaxPacketSize       */
    0x00,                                       /* wMaxPacketSize       */
    0x00,                                       /* bInterval            */

    /* Interface Descriptor. */
    0x09,                                       /* bLength              */
    USB_DT_INTERFACE,                           /* INTERFACE            */
    0x00,                                       /* bInterfaceNumber     */
    0x00,
    0x02,                                       /* bNumEndpoints        */
    COMMF_DATA_CLASS,                           /* bInterfaceClass      */
    0x00,                                       /* bInterfaceSubClass   */
    0x00,                                       /* bInterfaceProtocol   */
    0x00,                                       /* iInterface           */

    /* Endpoint Descriptors. */
    0x07,                                       /* bLength              */
    USB_DT_ENDPOINT,                            /* ENDPOINT             */
    USB_DIR_IN,                                 /* bEndpointAddress     */
    USB_EP_BULK,                                /* bmAttributes         */
    0x00,                                       /* wMaxPacketSize       */
    0x00,                                       /* wMaxPacketSize       */
    0x00,                                       /* bInterval            */

    /* Endpoint Descriptors. */
    0x07,                                       /* bLength              */
    USB_DT_ENDPOINT,                            /* ENDPOINT             */
    USB_DIR_OUT,                                /* bEndpointAddress     */
    USB_EP_BULK,                                /* bmAttributes         */
    0x00,                                       /* wMaxPacketSize       */
    0x00,                                       /* wMaxPacketSize       */
    0x00                                        /* bInterval            */
};

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
UINT8 ss_eth_intf_desc[] =
{
    /* Interface Descriptor. */
    0x09,                                       /* bLength              */
    USB_DT_INTERFACE,                           /* INTERFACE            */
    0x00,                                       /* bInterfaceNumber     */
    0x00,                                       /* bAlternateSetting    */
    0x01,                                       /* bNumEndpoints        */
    COMMF_CLASS,                                /* bInterfaceClass      */
    EF_SUBCLASS,                                /* bInterfaceSubClass   */
    0x00,                                       /* bInterfaceProtocol   */
    0x00,                                       /* iInterface           */

    /* Class specific functional descriptor (Header). */
    0x05,                                       /* bFunctionLength      */
    USB_DT_CLASSSPC,                            /* bDescriptorType      */
    0x00,                                       /* bDescriptorSubtype   */
    0x10,                                       /* bcdCDCLo             */
    0x01,                                       /* bcdCDCHi             */

    /* Class specific functional descriptor (Union). */
    0x05,                                       /* bFunctionLength      */
    USB_DT_CLASSSPC,                            /* bDescriptorType      */
    0x06,                                       /* bDescriptorSubtype   */
    0x00,                                       /* bMasterInterface     */
    0x01,                                       /* bSlaveInterface0     */

    /* Class specific functional descriptor (Ethernet Networking). */
    0x0D,                                       /* bFunctionLength      */
    USB_DT_CLASSSPC,                            /* bDescriptorType      */
    0x0F,                                       /* bDescriptorSubtype   */
    0x01,                                       /* iMACAddress          */
    0x00,                                       /* bmEthernetStatistics0*/
    0x00,                                       /* bmEthernetStatistics1*/
    0x00,                                       /* bmEthernetStatistics2*/
    0x00,                                       /* bmEthernetStatistics3*/
    (UINT8)(ETHF_MAX_SEG_SIZE&0xFF),            /* Maximum Segment Size LO  */
    (UINT8)(ETHF_MAX_SEG_SIZE>>8),              /* Maximum Segment Size HI  */
    0x00,                                       /* wNumberMCFiltersLo   */
    0x00,                                       /* wNumberMCFiltersHi   */
    0x00,                                       /* bNumberPowerFilters  */

    /* Endpoint Descriptors. */
    0x07,                                       /* bLength              */
    USB_DT_ENDPOINT,                            /* ENDPOINT             */
    USB_DIR_IN,                                 /* bEndpointAddress     */
    USB_EP_INTR,                                /* bmAttributes         */
    0x00,                                       /* wMaxPacketSize       */
    0x00,                                       /* wMaxPacketSize       */
    0x00,                                       /* bInterval            */

    /* Endpoint Companion Descriptor. */
    6,
    USB_DT_SSEPCOMPANION,
    0,
    0,
    0,
    0,

    /* Interface Descriptor. */
    0x09,                                       /* bLength              */
    USB_DT_INTERFACE,                           /* INTERFACE            */
    0x00,                                       /* bInterfaceNumber     */
    0x00,
    0x02,                                       /* bNumEndpoints        */
    COMMF_DATA_CLASS,                           /* bInterfaceClass      */
    0x00,                                       /* bInterfaceSubClass   */
    0x00,                                       /* bInterfaceProtocol   */
    0x00,                                       /* iInterface           */

    /* Endpoint Descriptors. */
    0x07,                                       /* bLength              */
    USB_DT_ENDPOINT,                            /* ENDPOINT             */
    USB_DIR_IN,                                 /* bEndpointAddress     */
    USB_EP_BULK,                                /* bmAttributes         */
    0x00,                                       /* wMaxPacketSize       */
    0x00,                                       /* wMaxPacketSize       */
    0x00,                                       /* bInterval            */

    /* Endpoint Companion Descriptor. */
    6,
    USB_DT_SSEPCOMPANION,
    0,
    0,
    0,
    0,

    /* Endpoint Descriptors. */
    0x07,                                       /* bLength              */
    USB_DT_ENDPOINT,                            /* ENDPOINT             */
    USB_DIR_OUT,                                /* bEndpointAddress     */
    USB_EP_BULK,                                /* bmAttributes         */
    0x00,                                       /* wMaxPacketSize       */
    0x00,                                       /* wMaxPacketSize       */
    0x00,                                       /* bInterval            */

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
*       nu_os_conn_usb_func_comm_eth_init
*
* DESCRIPTION
*
*       Ethernet Driver Initialization Function.
*       If USBF device is not yet initilized, it creats a
*       new task that waits for USBF device initialization
*       and afterwards initializez ETH driver.
*
*
* INPUTS
*
*       path        Registry path of component.
*       startstop   Flag to find if component is
*                   being enabled or disabled.
*
* OUTPUTS
*
*       status      Status of initialization
*
*************************************************************************/
STATUS nu_os_conn_usb_func_comm_eth_init(CHAR *path, INT startstop)
{
    VOID   *usbf_comm_handle = NU_NULL;
    UINT8   rollback = 0;
    STATUS  status = NU_SUCCESS, internal_sts = 0, reg_status;
    CHAR    usb_func_comm_eth_path[80];

    usb_func_comm_eth_path[0] = '\0';
    strcat(usb_func_comm_eth_path, path);

    /* Save registry settings of USB Function Ethernet/RNDIS. */
    strcat(usb_func_comm_eth_path, "/configstring");
    reg_status = REG_Get_String(usb_func_comm_eth_path, USBF_ETH_Configuration_String, NU_USB_MAX_STRING_LEN);
    if(reg_status == NU_SUCCESS)
    {
        usb_func_comm_eth_path[0] = '\0';
        strcat(usb_func_comm_eth_path, path);
        strcat(usb_func_comm_eth_path, "/interfacestring");
        reg_status = REG_Get_String(usb_func_comm_eth_path, USBF_ETH_Interface_String, NU_USB_MAX_STRING_LEN);
        if(reg_status == NU_SUCCESS)
        {
            usb_func_comm_eth_path[0] = '\0';
            strcat(usb_func_comm_eth_path, path);
            strcat(usb_func_comm_eth_path, "/macaddress");
            reg_status = REG_Get_String(usb_func_comm_eth_path, USBF_ETH_Mac_Address, 18);
            if(reg_status == NU_SUCCESS)
            {
                usb_func_comm_eth_path[0] = '\0';
                strcat(usb_func_comm_eth_path, path);
                strcat(usb_func_comm_eth_path, "/maxframesize");
                reg_status = REG_Get_UINT16(usb_func_comm_eth_path, &USBF_ETH_Max_Frame_Size);
            }
        }
    }

    /* First initialize NDIS user driver. */
    internal_sts = nu_os_conn_usb_func_comm_ndis_init(path, startstop);

    if (startstop == RUNLEVEL_START)
    {
#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
        /* USBF device initialized? */
        status = NU_USBF_Wait_For_Init(NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* USBF device is initialized. */
            /* Proceed with ETH device initialization. */
            status = NU_USBF_ETH_DEV_Init(path);
        }
        else
        {
            /* USBF device is not initialized. */
            /* Create task that waits for USBF device initialization event. */
            /* And then initializes ETH device. */
            status = USB_Allocate_Object(sizeof(NU_TASK), (VOID **)&ETH_Dev_Init_Task);
            if (status == NU_SUCCESS)
            {
                memset((VOID *) ETH_Dev_Init_Task, 0x00, sizeof(NU_TASK));
                strcpy(eth_reg_path, path);
                status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED, USBF_ETH_DEV_INIT_TASK_STACK_SIZE,
                        &eth_init_task_stack_ptr);
                if (status == NU_SUCCESS)
                {
                    memset((VOID *) eth_init_task_stack_ptr, 0x00, USBF_ETH_DEV_INIT_TASK_STACK_SIZE);
                    status = NU_Create_Task (ETH_Dev_Init_Task, "ETHCFGT",
                                             NU_USBF_ETH_DEV_Init_Task, 1, (VOID *)eth_reg_path,
                                             eth_init_task_stack_ptr, USBF_ETH_DEV_INIT_TASK_STACK_SIZE,
                                             USBF_ETH_DEV_INIT_TASK_PRIORITY, 0x00,
                                             NU_NO_PREEMPT, NU_START);
                    if(status != NU_SUCCESS)
                    {
                        rollback = 0x02;
                    }
                }
                else
                {
                    rollback = 0x01;
                }
                switch (rollback)
                {
                    case 0x02:
                        internal_sts |= USB_Deallocate_Memory((VOID *) eth_init_task_stack_ptr);
                        eth_init_task_stack_ptr = NU_NULL;
                    case 0x01:
                        internal_sts |= USB_Deallocate_Memory((VOID *) ETH_Dev_Init_Task);
                        ETH_Dev_Init_Task = NU_NULL;
                    default:
                        break;
                }
            }
        }
#else
        status = NU_USBF_ETH_DEV_Init(path);
#endif /* DV_DEV_DISCOVERY_TASK_ENB */

    }
    else if (startstop == RUNLEVEL_STOP)
    {
        NU_USBF_COMM_Init_GetHandle(&usbf_comm_handle);

        if (usbf_comm_handle)
        {
            NU_USB_DRVR_Deregister_User (usbf_comm_handle, (NU_USB_USER *)NU_USBF_USER_ETH_Cb_Pt);
            _NU_USBF_ETH_Delete(NU_USBF_USER_ETH_Cb_Pt);
            USB_Deallocate_Memory((VOID *)NU_USBF_USER_ETH_Cb_Pt);
            status = NU_SUCCESS;
        }
        else
        {
            status = NU_INVALID_POINTER;
        }
#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
        /* ETH device initialzation task cleanup. */
        if (ETH_Dev_Init_Task != NU_NULL)
        {
            if (NU_Terminate_Task(ETH_Dev_Init_Task) == NU_SUCCESS)
            {
                status |= NU_Delete_Task(ETH_Dev_Init_Task);
            }
            status |= USB_Deallocate_Memory((VOID *) ETH_Dev_Init_Task);
            ETH_Dev_Init_Task = NU_NULL;
        }
        if (eth_init_task_stack_ptr != NU_NULL)
        {
            status |= USB_Deallocate_Memory((VOID *) eth_init_task_stack_ptr);
            eth_init_task_stack_ptr = NU_NULL;
        }
#endif
    }

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_ETH_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the function Ethernet
*       user driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the user
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicate there exists a function Ethernet
*                           user driver.
*       NU_NOT_PRESENT      Indicate there exists no user driver.
*
*************************************************************************/
STATUS NU_USBF_ETH_GetHandle  ( VOID  **handle )
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBF_USER_ETH_Cb_Pt;
    if (NU_USBF_USER_ETH_Cb_Pt == NU_NULL)
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
*       NU_USBF_ETH_Register_Cb
*
*   DESCRIPTION
*
*       This function is used to register the callback functions with
*       the user driver.
*
*   INPUTS
*
*       cb                  Pointer to the user driver control block.
*       app_notify          Callback for notification of events.
*       rcvd_cb             Callback for data reception.
*       tx_done             Transmission completion callback.
*       ioctl               IOCTL callback.
*
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicate there exists a function Ethernet
*                           user driver.
*       NU_NOT_PRESENT      Indicate there exists no user driver.
*
*************************************************************************/

STATUS   NU_USBF_ETH_Register_Cb (NU_USBF_ETH *cb,
                                COMMF_APP_NOTIFY        app_notify,
                                COMMF_APP_RX_CALLBACK   rcvd_cb,
                                COMMF_APP_TX_DONE       tx_done,
                                COMMF_APP_IOCTL         ioctl)
{
    /* Register application callbacks with the driver. */
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    if (cb == NU_NULL ||
        app_notify == NU_NULL ||
        rcvd_cb == NU_NULL ||
        tx_done == NU_NULL ||
        ioctl   == NU_NULL
        )
    {
        status = NU_NOT_PRESENT;
    }
    else
    {
        cb->eth_app_notify = app_notify;
        cb->eth_rcvd_cb = rcvd_cb;
        cb->eth_tx_done = tx_done;
        cb->eth_ioctl = ioctl;

        status = NU_SUCCESS;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
* FUNCTION
*
*        NU_USBF_ETH_Create
*
* DESCRIPTION
*
*        ETH User Driver initialization routine
*
* INPUTS
*        cb            Pointer to Driver control block.
*        name          Name of this USB object.
*        pool          Memory pool pointer.
*        app_notify    Application notification callback.
*        rcvd_cb       Application data received callback.
*        tx_done       Application transmission complete callback.
*        ioctl         Application IOCTL callback.
*
* OUTPUTS
*        NU_SUCCESS    Successful Initialization.
*
*************************************************************************/
STATUS NU_USBF_ETH_Create ( NU_USBF_ETH *cb,
                            CHAR *name,
                            #if (EF_VERSION_COMP >= EF_2_0)
                            NU_MEMORY_POOL *pool,
                            #endif
                            COMMF_APP_NOTIFY     app_notify,
                            COMMF_APP_RX_CALLBACK   rcvd_cb,
                            COMMF_APP_TX_DONE       tx_done,
                            COMMF_APP_IOCTL         ioctl)
{
    NU_USB_DEVICE   *device;
    UINT8           config_idx;
    UINT8           rollback;
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);
    NU_USB_MEMPOOLCHK_RETURN(pool);

    /* Register application callbacks with the driver. */
    cb->eth_app_notify = app_notify;
    cb->eth_rcvd_cb = rcvd_cb;
    cb->eth_tx_done = tx_done;
    cb->eth_ioctl = ioctl;
    rollback = 0;

    #if (EF_VERSION_COMP >= EF_2_0)
    /* Store memory pool pointer in the driver control block. */
    cb->pool = pool;
    #else
    cb->pool = NU_NULL;
    #endif

    do
    {
        /* Call NU_USBF_USER_COMM's behavior using _NU_USBF_USER_COMM_Create
         * by passing cb, address to usbf_eth_dispatch as the
         * dispatch table.
         */
        status = (_NU_USBF_USER_COMM_Create ((NU_USBF_USER_COMM * )cb,
                                name,
                                EF_SUBCLASS,
                                0,
                                NU_TRUE,
                                &usbf_eth_dispatch));
        if ( status != NU_SUCCESS )
        {
            rollback = 0;
            break;
        }

#if ((ETHF_DESC_CONF == INC_ETH_AND_RNDIS) || (ETHF_DESC_CONF == INC_ETH))
#if (ETHF_DESC_CONF == INC_ETH)
        config_idx = USBF_DEF_CONFIG_INDEX;
#elif (ETHF_DESC_CONF == INC_ETH_AND_RNDIS)
        status = USBF_DEVCFG_Get_Device_Handle(&device);
        if ( status != NU_SUCCESS )
        {
            rollback = 1;
            break;
        }

        status = USBF_DEVCFG_Create_Config(device, &config_idx);
        if ( status != NU_SUCCESS )
        {
            rollback = 1;
            break;
        }
#endif
        /* Add ethernet function with USB device configuration layer.
             */
        /* Add descriptor for FULL, HIGH and SUPER speed. */
        status = USBF_DEVCFG_Add_Function(config_idx,
                                         fs_hs_eth_intf_desc,
                                         sizeof(fs_hs_eth_intf_desc),
                                         fs_hs_eth_intf_desc,
                                         sizeof(fs_hs_eth_intf_desc),
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                                         ss_eth_intf_desc,
                                         sizeof(ss_eth_intf_desc),
#endif
                                         &cb->eth_function);
        if ( status != NU_SUCCESS )
        {
            rollback = 2;
            break;
        }
#endif
    }while(0);

    switch(rollback)
    {
        case 2:
            if ( config_idx != USBF_DEF_CONFIG_INDEX )
            {
                USBF_DEVCFG_Delete_Config(device, config_idx);
            }
        case 1:
            _NU_USBF_USER_COMM_Delete(cb);
        default:
            break;
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
* FUNCTION
*
*        NU_USBF_ETH_Bind_Interface
*
* DESCRIPTION
*
*       This function actually binds COMM ethernet function with USB device
*       configuration. It is necessary that ethernet function is
*       already registered with USB device configuration glue layer.
*
* INPUTS
*
*       cb                      Pointer to ethernet function driver
*                               control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        Indicates invalid argument.
*       NU_USB_NOT_PRESENT      Indicates that ethernet function
*                               is not registered.
*       NU_SUCCESS              Indicates successful initialization.
*
**************************************************************************/
STATUS NU_USBF_ETH_Bind_Interface(NU_USBF_ETH *cb)
{
    /* Local variables. */
    STATUS     status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;
    if ( cb != NU_NULL )
    {
#if ((ETHF_DESC_CONF == INC_ETH_AND_RNDIS) || (ETHF_DESC_CONF == INC_ETH))
        status = NU_NOT_PRESENT;
        if ( cb->eth_function != NU_NULL )
        {
            status = NU_USB_DEVICE_Set_bDeviceClass(cb->eth_function->device,
                                                    COMMF_CLASS);
            if ( status == NU_SUCCESS )
            {
                /* Enable modem function. */
                status = USBF_DEVCFG_Enable_Function(cb->eth_function);
                if ( status == NU_SUCCESS )
                {
                    /* Bind modem function with USB device configuration. */
                    status = USBF_DEVCFG_Bind_Function(cb->eth_function);
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
*        NU_USBF_ETH_Unbind_Interface
*
* DESCRIPTION
*
*       This function actually unbinds COMM ethernet function with USB
*       device configuration.
*       It is necessary that ethernet function is already registered
*       with USB device configuration glue layer.
*
* INPUTS
*
*       cb                      Pointer to ethernet function driver
*                               control block.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG        Indicates invalid argument.
*       NU_USB_NOT_PRESENT      Indicates that ethernet function
*                               is not registered.
*       NU_SUCCESS              Indicates successful initialization.
*
**************************************************************************/
STATUS NU_USBF_ETH_Unbind_Interface(NU_USBF_ETH *cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    status = NU_USB_INVLD_ARG;
    if ( cb != NU_NULL )
    {
#if ((ETHF_DESC_CONF == INC_ETH_AND_RNDIS) || (ETHF_DESC_CONF == INC_ETH))
        status = NU_NOT_PRESENT;
        if ( cb->eth_function != NU_NULL )
        {
            status = USBF_DEVCFG_Unbind_Function(cb->eth_function);
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
*       _NU_USBF_ETH_Delete
*
* DESCRIPTION
*       This function deletes an instance of ETH User driver.
*
* INPUTS
*       cb              Pointer to the USB Object control block.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*
*************************************************************************/
STATUS _NU_USBF_ETH_Delete (VOID *cb)
{
    STATUS status;
    NU_USB_DEVICE   *device = NU_NULL;
    UINT8            config_idx;

    NU_USB_PTRCHK(cb);

    /* Find the configuration index where this interface was added. */
    config_idx = ((NU_USBF_ETH *)cb)->eth_function->config_index;

    /* Remove USB Ethernet function. */
    status = USBF_DEVCFG_Delete_Function(((NU_USBF_ETH *)cb)->eth_function);

    /* Delete device configuration if it is other then default. */
    if (config_idx != USBF_DEF_CONFIG_INDEX)
    {
        /* Get the function device handle. */
        status |= USBF_DEVCFG_Get_Device_Handle(&device);
        if(device != NU_NULL)
        {
            /* Delete this device configuration. */
            status |= USBF_DEVCFG_Delete_Config(device, config_idx);
        }
    }

    /* Call NU_USBF_USER_COMM_Delete Behavior. */
    status |= _NU_USBF_USER_COMM_Delete((NU_USBF_USER_COMM * )cb);

    return (status);
}

/*************************************************************************
* FUNCTION
*       _NU_USBF_ETH_Connect
*
* DESCRIPTION
*       This function is called by class driver's Initialize_intf/Device
*       routine when a new device which can be served by this user is
*       found.
*
* INPUTS
*       cb                Pointer to user control block.
*       class_driver      Pointer to calling class driver's control block.
*       handle            Handle for the device connected.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*************************************************************************/

STATUS _NU_USBF_ETH_Connect (NU_USB_USER * cb, NU_USB_DRVR * class_driver,
                             VOID *handle)
{
    STATUS status = NU_SUCCESS;
    NU_USBF_ETH *eth = (NU_USBF_ETH *)cb;
    NU_USBF_ETH_DEV *pdevice = NU_NULL;
    INT i;

    #if (EF_VERSION_COMP >= EF_2_0)
    /* Allocate and initialize a new instance of ethernet device.*/
    status = USB_Allocate_Object(sizeof (NU_USBF_ETH_DEV),
                                 (VOID **)&pdevice);
    #else
    pdevice = &NU_USBF_ETH_DEV_CB;
    #endif

    if (status == NU_SUCCESS)
    {
        memset(pdevice,0,sizeof(NU_USBF_ETH_DEV));
        pdevice->handle = handle;

        /* Reset the device internal state. */
        pdevice->curr_notif = COMMF_NO_NOTIF;
        pdevice->next_notif = COMMF_NO_NOTIF;
        pdevice->conn_status = EF_DISCONNECTED;
        pdevice->us_bit_rate = 0;
        pdevice->ds_bit_rate = 0;

        pdevice->num_multicast_filters = 0;

        for(i=0; i<EF_MAX_POWER_PATTERN_FILTERS; i++)
        {
            pdevice->power_pattern_filter_array[i].filter_status = EF_FALSE;
        }

        memset(&pdevice->packet_filter, 0, sizeof(ETHF_PKT_FILTER));
        memset(pdevice->ethernet_statistics, 0, sizeof(pdevice->ethernet_statistics));

        NU_Place_On_List((CS_NODE **)&eth->eth_dev_list_head, (CS_NODE *)pdevice);

        /* Call the parent's connect function. */
        status = _NU_USBF_USER_COMM_Connect (cb,
                                         class_driver,
                                         handle);
        if (status != NU_SUCCESS)
        {
            NU_Remove_From_List((CS_NODE **)&eth->eth_dev_list_head, (CS_NODE *)pdevice);

            #if (EF_VERSION_COMP >= EF_2_0)
            USB_Deallocate_Memory (pdevice);
            #endif
        }
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*       _NU_USBF_ETH_Disconnect
*
* DESCRIPTION
*
*       This function is called by the class driver's disconnect function
*       when a device being served by this user gets disconnected. This
*       cleans up the device specific entries
*       associated with the device.
*
* INPUTS
*       cb                  Pointer to user control block.
*       class_driver        Pointer to calling class driver's control block.
*       handle              Handle of the device disconnected.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_INVALID_POINTER  Driver is unable to find the device pointer
*                           from its list on the basis of handle.
*
*************************************************************************/

STATUS _NU_USBF_ETH_Disconnect (NU_USB_USER *cb,
                                NU_USB_DRVR *class_driver,
                                VOID *handle)
{
    STATUS status = NU_SUCCESS;
    NU_USBF_ETH *user = (NU_USBF_ETH*)cb;
    NU_USBF_ETH_DEV *pdev = NU_NULL;

    pdev = EF_Find_Device(user, handle);

    if (pdev)
    {
        NU_Remove_From_List((CS_NODE**)&user->eth_dev_list_head, (CS_NODE*)pdev);
        #if (EF_VERSION_COMP >= EF_2_0)
        USB_Deallocate_Memory(pdev);
        #endif
    }
    else
    {
        status = NU_INVALID_POINTER;
    }

    if (status == NU_SUCCESS)
        status = _NU_USBF_USER_COMM_Disconnect (cb,
                                                class_driver,
                                                handle);
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBF_ETH_New_Command
*
* DESCRIPTION
*
*       Processes a new command from the Host.
*
*       These commands are class/subclass specific. This function
*       processes the command identified by the 'command' parameter. The
*       length of the command block, in bytes is expected in the 'cmd_len'
*       parameter.
*       If there is any data transfer in response to a command, then the
*       location corresponding to the data to be transferred, either to
*       / from Host is filled in the 'data_out' parameter. The length of
*       the data to be transferred is filled in the location pointed to by
*        the 'data_len_out' parameter.
*
*       If there is no data transfer associated with the command, then,
*       the location pointed to by the 'data' is filled in with NU_NULL.
*
*       For unknown and unsupported command, this function returns
*       appropriate error status.
*
* INPUTS
*
*       cb                    User control block for which the command is
*                             meant for.

*       drvr                  Class driver invoking this function
*
*       handle                Identifies for the logical function to which
*                             the command is directed to.
*
*       command               Memory location where the command block is
*                             stored.

*       cmd_len               Length of the command block.
*
*       data_out              Memory location where the data pointer for
*                             the transfer is to be stored
*
*       data_len_out          Memory location where the length of data to
*                             be transferred, in bytes, must be filled.
*
* OUTPUTS
*
*   NU_SUCCESS                Indicates that the command has been processed
*                             successfully
*
*   NU_USB_NOT_SUPPORTED      Indicates that the command is unsupported.
*   NU_INVALID_POINTER        Driver is unable to find the device pointer
*                             from its list on the basis of handle.

*
*
*************************************************************************/
STATUS _NU_USBF_ETH_New_Command (NU_USBF_USER * cb,
                                 NU_USBF_DRVR * drvr,
                                 VOID *handle,
                                 UINT8 *command,
                                 UINT16 cmd_len,
                                 UINT8 **data_out, UINT32 *data_len_out)
{
    NU_USBF_ETH *eth = (NU_USBF_ETH *)cb;
    USBF_COMM_USER_CMD *user_cmd;
    STATUS status;
    INT i;
    ETHF_POWER_MNG_FILTER *filter;
    NU_USBF_ETH_DEV *pdev = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);

    user_cmd = (USBF_COMM_USER_CMD *)command;

    pdev = EF_Find_Device(eth,handle);

    if (pdev)
    {
       /* If EF_SEND_ENCAPSULATED_COMMAND is received from Host. */
       if(user_cmd->command == EF_SEND_ENCAPSULATED_COMMAND)
       {
           /* No command is defined as encapsulated command, so we pass it to
            * application.
            */
           if(eth->eth_ioctl)
           {
               eth->eth_ioctl((NU_USBF_USER *)eth,
                               EF_SEND_ENCAPSULATED_COMMAND,
                               user_cmd->cmd_data,
                               user_cmd->data_len);
           }
           status = NU_SUCCESS;
       }
       /* This command is sent by Host when EF_RESPONSE_AVAILABLE notification
        * is sent to Host. 'User must fill the valid response value in the
        * 'response_array' before giving this notification.
        */
       else if(user_cmd->command == EF_GET_ENCAPSULATED_RESPONSE)
       {
           /* Initialize the response buffer and length. */
           *data_out = (UINT8 *)pdev->response_array;
           *data_len_out = pdev->response_length;

           status =  NU_SUCCESS;
       }
       /* This command is sent by Host to change the Ethernet Multicast
        * filters. Handler for this command initializes the Multicast filter
        * array and calls the demo function.
        */
       else if(user_cmd->command == EF_SET_ETHF_MULTICAST_FILTERS)
       {
           status = EF_Set_Multi_Filters_Rcvd(eth, pdev, user_cmd);
       }
       /* If EF_SET_ETH_PWR_MGT_PTRN_FILTER command is received
        * from Host, target device will validate the filter and if it is
        * supported by it, this filter will be set active.
        */
       else if(user_cmd->command == EF_SET_ETH_PWR_MGT_PTRN_FILTER)
       {
           status = EF_Set_Power_Filters_Rcvd(eth, pdev, user_cmd);
       }
       /* If EF_GET_ETH_PWR_MGT_PTRN_FILTER command is received from Host. */
       else if(user_cmd->command == EF_GET_ETH_PWR_MGT_PTRN_FILTER)
       {
           /* Locate the filter number specified in command. */
           for(i=0; i<EF_MAX_POWER_PATTERN_FILTERS; i++)
           {
               filter = &pdev->power_pattern_filter_array[i];

               /* Find the filter with given filter number. */
               if(filter->filter_num == user_cmd->cmd_value)
               {
                   break;
               }
           }
           /* If given filter is found, return the status of specific filter,
            * otherwise return EF_FALSE.
            */
           if(i < EF_MAX_POWER_PATTERN_FILTERS)
           {
               pdev->power_pattern_status_buffer = HOST_2_LE16
                                           (filter->filter_status);
           }
           else
           {
               pdev->power_pattern_status_buffer = HOST_2_LE16(EF_FALSE);
           }

           *data_out = (UINT8 *)&pdev->power_pattern_status_buffer;
           *data_len_out = EF_PWR_PTRN_STAT_SIZE;
           status =  NU_SUCCESS;

       }
       /* If Host sends a command to set the ethernet packet filter, update
        * the filter structure and call the demo function.
        */
       else if(user_cmd->command == EF_SET_ETHERNET_PACKET_FILTER)
       {
           status = EF_Set_Pkt_Filters_Rcvd(eth, pdev, user_cmd);
       }
       /* If Host asks for the Ethernet statistics, read the 4 byte value from
        * array and return to Host.
        */
       else if(user_cmd->command == EF_GET_ETHERNET_STATISTIC)
       {
           UINT16 stat_index = user_cmd->cmd_value;
           pdev->eth_stat_buffer = HOST_2_LE32(
                       pdev->ethernet_statistics[stat_index]);
           *data_out = (UINT8 *)&pdev->eth_stat_buffer;
           *data_len_out = EF_ETHERNET_STAT_SIZE;
           status =  NU_SUCCESS;
       }
       else
       {
           status =  NU_USB_NOT_SUPPORTED;
       }

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
*       _NU_USBF_ETH_New_Transfer
*
* DESCRIPTION
*
*       This function processes a received data transfer from USB Host.
*       This function needs to call the NU_USBF_COMM_DATA_Get_Rcvd to pull
*       received data from the completed queue of class driver.
*
* INPUTS
*
*       cb                      User control block
*       drvr                    Class driver invoking this function
*       handle                  Identifies for the logical function to
*                               which the transfer is directed to.
*       data_out                Memory location where the data pointer for
*                               the transfer is to be stored.
*       data_len_out            Memory location where the length of data
*                               to be transferred, in bytes, must be filled.
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates that the function has executed
*                               successfully.
*       NU_INVALID_POINTER      Driver is unable to find the device pointer
*                               from its list on the basis of handle.
*
*************************************************************************/
STATUS _NU_USBF_ETH_New_Transfer (NU_USBF_USER * cb,
                                  NU_USBF_DRVR * drvr,
                                  VOID *handle,
                                  UINT8 **data_out, UINT32 *data_len_out)
{
    NU_USBF_ETH *eth = (NU_USBF_ETH *)cb;
    STATUS status;
    UINT8 *buffer;
    UINT32 length;
    NU_USBF_ETH_DEV *pdev = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    pdev = EF_Find_Device(eth,handle);

    if (pdev)
    {
        /* If user wants to process the received data now, it can call the
        * function to get the received data parameters otherwise it can call
        * it on later time.
        */
        status = NU_USBF_COMM_DATA_Get_Rcvd(
                       ((NU_USBF_USER_COMM* )eth)->data_drvr,
                       &buffer, &length,
                       handle);

        if(status == NU_SUCCESS)
        {
            /* Pass the received data to application. */
            if((length != 0) && (eth->eth_rcvd_cb))
            {
                #if (EF_VERSION_COMP >= EF_2_0)
                eth->eth_rcvd_cb(buffer,
                             length,
                             NU_NULL,
                             0,
                             handle);
                #else
                eth->eth_rcvd_cb(buffer,
                             length,
                             NU_NULL,
                             0);

                #endif
            }

            /* Increment the successful transmitted statistics. */
            pdev->ethernet_statistics[EF_XMIT_OK]++;
        }
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
*       _NU_USBF_ETH_Tx_Done
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
*       cb                    User control block
*
*       drvr                  Class driver invoking this function
*
*       handle                Identifies for the logical function to
*                             which the notification is directed to.
*
*       completed_data        Memory location to / from where the data
*                             has been transferred
*
*       completed_data_len
*                             Length of data transferred, in bytes.
*
*       data_out              Memory location where the data pointer for
*                             the transfer is to be stored, if pending.
*
*       data_len_out          Memory location where the length of data
*                             to be transferred, in bytes, is to be filled.
*
* OUTPUTS
*
*       NU_SUCCESS            Indicates that the function has executed
*                             successfully.
*
*       NU_USB_INVLD_ARG      Indicates a unexpected transfer completion.
*       NU_INVALID_POINTER    Driver is unable to find the device pointer
*                             from its list on the basis of handle.
*
*************************************************************************/
STATUS _NU_USBF_ETH_Tx_Done (NU_USBF_USER * cb,
                             NU_USBF_DRVR * drvr,
                             VOID *handle,
                             UINT8 *completed_data,
                             UINT32 completed_data_len,
                             UINT8 **data_out,
                             UINT32 *data_len_out)
{
    NU_USBF_ETH *eth;
    UINT8   tx_handle;
    COMMF_CMPL_CTX * ctx;
    NU_USBF_ETH_DEV *pdev = NU_NULL;
    STATUS status = NU_USB_INVLD_ARG;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);

    eth = (NU_USBF_ETH *)cb;

    ctx = (COMMF_CMPL_CTX*)handle;

    tx_handle = ctx->transfer_type;

    #if (EF_VERSION_COMP >= EF_2_0)
        pdev = EF_Find_Device(eth, ctx->handle);
    #else
        pdev = &NU_USBF_ETH_DEV_CB;
    #endif

    if (pdev)
    {
        /* If transmission of data is complete. */
        if(tx_handle == COMMF_DATA_SENT)
        {
            /* Increment the successful received statistics. */
            pdev->ethernet_statistics[EF_RCV_OK]++;

            /* Give callback to application. */
            if(eth->eth_tx_done)
            {
                #if (EF_VERSION_COMP >= EF_2_0)
                eth->eth_tx_done(completed_data, completed_data_len, ctx->handle);
                #else
                eth->eth_tx_done(completed_data, completed_data_len);
                #endif
            }
            status = NU_SUCCESS;
        }
        /* If last notification is successfully sent to Host. */
        else if(tx_handle == COMMF_NOTIF_SENT)
        {
            /* Mark that current notification is no more valid. */
            pdev->curr_notif = COMMF_NO_NOTIF;

            status = NU_SUCCESS;

            /* If other notification is in queue, send it. */
            if(pdev->next_notif != COMMF_NO_NOTIF)
            {
                #if (EF_VERSION_COMP >= EF_2_0)
                status = (NU_USBF_ETH_Send_Notif(eth, ctx->handle, pdev->next_notif,
                                                 COMMF_NO_NOTIF));
                #else
                status = (NU_USBF_ETH_Send_Notif(eth, pdev->next_notif,
                                                 COMMF_NO_NOTIF));
                #endif
            }
        }
        else if(tx_handle == COMMF_GET_CMD_SENT)
        {
            status = NU_SUCCESS;
        }
        else
        {
            status = NU_USB_INVLD_ARG;
        }
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
*       _NU_USBF_ETH_Notify
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
*       cb                      User control block for which the event is
*                               meant for.
*
*       drvr                    Class driver invoking this function.
*
*       handle                  Identifies for the logical function to
*                               which the notification is directed to.
*
*       event                   The USB event that has occurred.
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates that the event has been
*                               processed successfully.
*
*************************************************************************/
STATUS _NU_USBF_ETH_Notify (NU_USBF_USER * cb,
                            NU_USBF_DRVR * drvr, VOID *handle,
                            UINT32 event)
{
    NU_USBF_ETH *eth = (NU_USBF_ETH *)cb;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Pass this notification to application. */
    if(eth->eth_app_notify)
    {
       eth->eth_app_notify((NU_USBF_USER *)cb, event,
                                handle);
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*       _NU_USBF_ETH_DATA_Disconnect
*
* DESCRIPTION
*       This function is called by the Data Interface class driver's
*       disconnect function when a device being served by this user gets
*       disconnected. This cleans up the device specific entries
*       associated with the device.
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

STATUS _NU_USBF_ETH_DATA_Disconnect (NU_USB_USER * cb,
                                     NU_USB_DRVR * class_driver,
                                     VOID *handle)
{

    STATUS status;
    NU_USBF_ETH *pcb_eth = (NU_USBF_ETH *)cb;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Call the parent's function. */
    status = _NU_USBF_USER_COMM_DATA_Discon (cb,
                               class_driver,
                               handle);
    if(status == NU_SUCCESS)
    {
        /* Pass this disconnection event to application. */
        if(pcb_eth->eth_app_notify)
        {
           pcb_eth->eth_app_notify((NU_USBF_USER *)cb,
                                    COMMF_EVENT_USER_DEINIT,
                                    handle);
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
* FUNCTION
*       _NU_USBF_ETH_DATA_Connect
*
* DESCRIPTION
*       This function is called by Data interface class driver's
*       Initialize_intf/Device routine when a new device which can be
*       served by this user is found.
*
* INPUTS
*       cb                Pointer to user control block.
*       class_driver      Pointer to calling class driver's control block.
*       handle            Handle for the device connected.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*************************************************************************/
STATUS _NU_USBF_ETH_DATA_Connect (NU_USB_USER * cb,
                                  NU_USB_DRVR * class_driver,
                                  VOID *handle)
{
    INT i;
    STATUS status = NU_SUCCESS;
    NU_USBF_ETH_DEV *pdev = NU_NULL;
    NU_USBF_ETH *eth = (NU_USBF_ETH *)cb;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);

    pdev = EF_Find_Device(eth,handle);

    /* Every time DATA_Connect callback is called from Class driver, that
     * means Host has select the default alternate setting of Data class
     * Interface and selected the working alternate setting again. This
     * has the effect of resetting the device, so it should clear all
     * filters, buffers and statistics.
     */
    if (pdev)
    {
        pdev->curr_notif = COMMF_NO_NOTIF;
        pdev->next_notif = COMMF_NO_NOTIF;
        pdev->num_multicast_filters = 0;

        for(i=0; i<EF_MAX_POWER_PATTERN_FILTERS; i++)
            pdev->power_pattern_filter_array[i].filter_status = EF_FALSE;

        memset(&pdev->packet_filter, 0, sizeof(ETHF_PKT_FILTER));
        memset(pdev->ethernet_statistics, 0, sizeof(pdev->ethernet_statistics));

        /* For demonstration purposes, connect status setting to connected. */
        pdev->conn_status = EF_CONNECTED;

        /* Call the parent's function. */
        status = _NU_USBF_USER_COMM_DATA_Connect (cb,
                                              class_driver,
                                              handle);
        if(status == NU_SUCCESS)
        {
            /* Pass the initialization event to Host. */
            if(eth->eth_app_notify)
            {
                eth->eth_app_notify((NU_USBF_USER *)cb, COMMF_EVENT_USER_INIT,
                                    handle);
            }
        }
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
*       NU_USBF_ETH_Send_Notif
*
* DESCRIPTION
*     This function is used to send the notification to the communication
*     class driver by the application. This function can send two
*     notification one after the other which is required sometimes by the
*     ethernet user.
*
* INPUTS
*       cb                          Pointer to user driver.
*       handle                      Handle to the communication device.
*       first_notif                 Code for first notification
*       second_notif                Code for second notification
*
* OUTPUTS
*       NU_SUCCESS                  Indicates successful completion.
*       COMMF_NOTIF_CMPLT_AWAITED   Completion callback is awaited from
*                                   last notification.
*       NU_USB_INVLD_ARG            Invalid value of input parameter(s).
*       NU_INVALID_POINTER          Driver is unable to find the device pointer
*                                   from its list on the basis of handle.
*
*************************************************************************/

STATUS NU_USBF_ETH_Send_Notif(NU_USBF_ETH *cb,
                              #if (EF_VERSION_COMP >= EF_2_0)
                              VOID *handle,
                              #endif
                              UINT8 first_notif,
                              UINT8 second_notif)
{
    STATUS status;
    UINT32 *buffer;
    USBF_COMM_USER_NOTIFICATION *notif;
    NU_USBF_ETH_DEV *pdev = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);

    #if (EF_VERSION_COMP >= EF_2_0)
    pdev = EF_Find_Device(cb,handle);
    #else
    pdev = &NU_USBF_ETH_DEV_CB;
    #endif

    if (pdev)
    {
        /* Check completion callback is awaited for last notification. */
        if(pdev->curr_notif != COMMF_NO_NOTIF)
        {
            status = COMMF_NOTIF_CMPLT_AWAITED;
        }
        else
        {
            notif = &pdev->user_notif;
            pdev->curr_notif = first_notif;
            pdev->next_notif = second_notif;

            /* If network connection notification is being sent. */
            if(first_notif == EF_NETWORK_CONNECTION)
            {
                /* Initialize user structure for this notification. */
                notif->notification = EF_NETWORK_CONNECTION;
                notif->notif_value = pdev->conn_status;
                notif->data = NU_NULL;
                notif->length = 0;

                /* Call class driver function for sending notification. */
                status = NU_USBF_COMM_Send_Notification(((NU_USBF_USER_COMM *)
                                                        cb)->mng_drvr, notif,
                                                        pdev->handle);
            }
            /* If connection speed notification is being sent. */
            else if(first_notif == EF_CONNECTION_SPEED_CHANGE)
            {
                /* Initialize user structure for this notification. */
                buffer = (UINT32 *)pdev->conn_speed_array;
                *buffer++ = HOST_2_LE32(pdev->us_bit_rate);
                *buffer++ = HOST_2_LE32(pdev->ds_bit_rate);
                notif->notification = EF_CONNECTION_SPEED_CHANGE;
                notif->notif_value = 0;
                notif->data = (UINT8 *)pdev->conn_speed_array;
                notif->length = EF_CONN_SPD_NOTIF_SIZE;

                /* Call class driver function for sending notification. */
                status = NU_USBF_COMM_Send_Notification(((NU_USBF_USER_COMM *)
                                                        cb)->mng_drvr, notif,
                                                        pdev->handle);
            }
            /* If connection speed notification is being sent. */
            else if(first_notif == EF_RESPONSE_AVAILABLE)
            {
                /* Initialize user structure for this notification. */
                notif->notification = EF_RESPONSE_AVAILABLE;
                notif->notif_value = 0;
                notif->data = NU_NULL;
                notif->length = 0;

                /* Call class driver function for sending notification. */
                status = NU_USBF_COMM_Send_Notification(((NU_USBF_USER_COMM *)
                                                        cb)->mng_drvr, notif,
                                                        pdev->handle);
            }
            else
            {
                status = NU_USB_INVLD_ARG;
            }
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
*       NU_USBF_ETH_Send_Data
*
* DESCRIPTION
*       This function is used to send data to USB Host.
*
* INPUTS
*       cb          Pointer to user driver control block.
*       handle      Pointer to the communication device control block.
*       buffer      Pointer to data buffer.
*       length      Length of data.
*
* OUTPUTS
*       NU_SUCCESS                  Indicates successful completion.
*       NU_USB_INVLD_ARG            Input argument has invalid value.
*       NU_USB_ETH_NOT_CONNECTED    Ethernet device is not in connected
*                                   state.
*       NU_INVALID_POINTER          Driver is unable to find the device
*                                   pointer from its list on the basis
*                                   of handle.
*
*************************************************************************/
STATUS NU_USBF_ETH_Send_Data(NU_USBF_ETH *cb,
                             #if (EF_VERSION_COMP >= EF_2_0)
                             VOID *handle,
                             #endif
                             UINT8 *buffer,UINT16 length)
{
    STATUS status;
    NU_USBF_ETH_DEV *pdev = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);

    #if (EF_VERSION_COMP >= EF_2_0)
    pdev = EF_Find_Device(cb,handle);
    #else
    pdev = &NU_USBF_ETH_DEV_CB;
    #endif

    if (pdev)
    {
        if(length > ETHF_MAX_SEG_SIZE)
        {
            status = NU_USB_INVLD_ARG;
        }
        else
        {
            if(pdev->conn_status != EF_CONNECTED)
            {
                status = NU_USB_ETH_NOT_CONNECTED;
            }
            else
            {
                /* Call class driver function for sending this data. */
                status = NU_USBF_COMM_DATA_Send(
                        ((NU_USBF_USER_COMM *)cb)->data_drvr,
                        buffer, length,
                        pdev->handle);
            }
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
*       NU_USBF_ETH_Send_Connection
*
* DESCRIPTION
*       This function is used for sending connection.
*
* INPUTS
*       cb          Pointer to user driver control block.
*       handle      Pointer to the device control block.
*       us_bit_rate Upstream port bit rate.
*       ds_bit_rate Downstream port bit rate.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_INVALID_POINTER  Driver is unable to find the device
*                           pointer from its list on the basis
*                           of handle.
*
*************************************************************************/
STATUS NU_USBF_ETH_Send_Connection(NU_USBF_ETH *eth,
                                   #if (EF_VERSION_COMP >= EF_2_0)
                                   VOID *handle,
                                   #endif
                                   UINT32 us_bit_rate, UINT32 ds_bit_rate)
{
    STATUS status;
    NU_USBF_ETH_DEV *pdev = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    #if (EF_VERSION_COMP >= EF_2_0)
    pdev = EF_Find_Device(eth,handle);
    #else
    pdev = &NU_USBF_ETH_DEV_CB;
    #endif

    if(pdev)
    {
        /* Update the driver data. */
        pdev->conn_status = EF_CONNECTED;
        pdev->us_bit_rate = us_bit_rate;
        pdev->ds_bit_rate = ds_bit_rate;

        /* Send this notification. */
        status =  NU_USBF_ETH_Send_Notif(eth,
        #if (EF_VERSION_COMP >= EF_2_0)
        pdev->handle,
        #endif
                    EF_NETWORK_CONNECTION,
                    EF_CONNECTION_SPEED_CHANGE);
    }
    else
        status = NU_INVALID_POINTER;

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
* FUNCTION
*       NU_USBF_ETH_Send_Encap_Resp
*
* DESCRIPTION
*       This function is used for sending response.
*
* INPUTS
*       cb          Pointer to user driver control block.
*       handle      Pointer to the communication device.
*       data        Pointer to data buffer.
*       length      Length of data.
*
* OUTPUTS
*       NU_SUCCESS                  Indicates successful completion.
*       NU_USB_INVLD_ARG            Input argument has invalid value.
*       NU_USB_ETH_NOT_CONNECTED    Ethernet device is not in connected
*                                   state.
*       NU_INVALID_POINTER          Driver is unable to find the device
*                                   pointer from its list on the basis
*                                   of handle.
*
*************************************************************************/
STATUS NU_USBF_ETH_Send_Encap_Resp(NU_USBF_ETH *eth,
                                   #if (EF_VERSION_COMP >= EF_2_0)
                                   VOID  *handle,
                                   #endif
                                   UINT8 *data, UINT16 length)
{
    /* Larger length than EF_MAX_RESPONSE_LENGTH is not allowed. */
    STATUS status;
    NU_USBF_ETH_DEV *pdev = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    #if (EF_VERSION_COMP >= EF_2_0)
    pdev = EF_Find_Device(eth,handle);
    #else
    pdev = &NU_USBF_ETH_DEV_CB;
    #endif

    if (pdev)
    {
        if(length > EF_MAX_RESPONSE_LENGTH)
        {
            status = NU_USB_INVLD_ARG;
        }
        else
        {
            if(pdev->conn_status != EF_CONNECTED)
            {
                status = NU_USB_ETH_NOT_CONNECTED;
            }
            else
            {
                /* Copy the response data in the buffer. */
                memcpy(pdev->response_array, data, length);
                pdev->response_length = length;

                /* Send notification to class driver. */
                status = (NU_USBF_ETH_Send_Notif(eth,
                #if (EF_VERSION_COMP >= EF_2_0)
                pdev->handle,
                #endif
                EF_RESPONSE_AVAILABLE,
                COMMF_NO_NOTIF));
            }
        }
    }
    else
        status = NU_INVALID_POINTER;

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_ETH_Send_Disconnection
*
* DESCRIPTION
*
*       This function sends disconnection to Host.
*
* INPUTS
*
*       eth                Pointer to user driver control block.
*       handle             Pointer to communication device.
*
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*       NU_INVALID_POINTER Driver is unable to find the device
*                          pointer from its list on the basis
*                          of handle.
**************************************************************************/

STATUS NU_USBF_ETH_Send_Disconnection(NU_USBF_ETH *eth
                                      #if (EF_VERSION_COMP >= EF_2_0)
                                      ,VOID *handle
                                      #endif
                                      )
{
    STATUS status;
    NU_USBF_ETH_DEV *pdev = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    #if (EF_VERSION_COMP >= EF_2_0)
    pdev = EF_Find_Device(eth,handle);
    #else
    pdev = &NU_USBF_ETH_DEV_CB;
    #endif

    if (pdev)
    {
        /* Update the driver data. */
        pdev->conn_status = EF_DISCONNECTED;

        /* Send this notification. */
        status = NU_USBF_ETH_Send_Notif(eth,
        #if (EF_VERSION_COMP >= EF_2_0)
        pdev->handle,
        #endif
        EF_NETWORK_CONNECTION,
        COMMF_NO_NOTIF);
    }
    else
        status = NU_INVALID_POINTER;

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_ETH_DM_Open
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
STATUS    NU_USBF_ETH_DM_Open (VOID* dev_handle)
{
    STATUS      status;
    NU_USBF_ETH *eth_cb;

    eth_cb = (NU_USBF_ETH*) dev_handle;
    status = NU_USBF_ETH_Bind_Interface(eth_cb);
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
STATUS    NU_USBF_ETH_DM_Close(VOID* dev_handle)
{
    STATUS      status;
    NU_USBF_ETH *eth_cb;

    eth_cb = (NU_USBF_ETH*) dev_handle;
    status = NU_USBF_ETH_Unbind_Interface(eth_cb);
    return (status) ;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_ETH_DM_Read
*
* DESCRIPTION
*
*       This function is called by the application when it wants to read
*       data from the device. In Ethernet case this is a dummy function as
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
STATUS    NU_USBF_ETH_DM_Read( VOID*    dev_handle,
                            VOID*    buffer,
                            UINT32      numbyte,
                            OFFSET_T byte_offset,
                            UINT32*     bytes_read_ptr)
{
    STATUS status = NU_SUCCESS;
    return (status) ;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_ETH_DM_Write
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

STATUS   NU_USBF_ETH_DM_Write(
                            VOID*         dev_handle,
                            const VOID*   buffer,
                            UINT32        numbyte,
                            OFFSET_T      byte_offset,
                            UINT32*       bytes_written_ptr)
{
    STATUS status = NU_SUCCESS;
    return (status) ;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_ETH_DM_IOCTL
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
STATUS   NU_USBF_ETH_DM_IOCTL   (VOID*     dev_handle,
                                  INT       ioctl_num,
                                  VOID*     ioctl_data,
                                  INT       ioctl_data_len)
{
    STATUS status = NU_SUCCESS;
    return (status) ;
}

/*************************************************************************
* FUNCTION
*
*       NU_USBF_ETH_DEV_Init
*
* DESCRIPTION
*
*       Ethernet Driver Initialization Function
*
* INPUTS
*
*       path           path to registry
*
* OUTPUTS
*
*       status
*
*************************************************************************/
STATUS NU_USBF_ETH_DEV_Init(CHAR *path)
{
    STATUS  status, internal_sts = NU_SUCCESS;
    UINT8   rollback = 0;
    VOID   *usbf_comm_handle = NU_NULL;

    /* Allocate Memory for Control Block */
    status = USB_Allocate_Object(sizeof(NU_USBF_ETH),
                                 (VOID**)&NU_USBF_USER_ETH_Cb_Pt);
    if(status != NU_SUCCESS)
    {
        rollback = 1;
    }

    if (!rollback)
    {
        /* Zero out the memory allocated for control block */
        memset (NU_USBF_USER_ETH_Cb_Pt, 0, sizeof(NU_USBF_ETH));

        /* In following API call, passing memory pool ptr parameter
         * NU_NULL because in ReadyStart memory in USB system is
         * allocated through USB specific memory APIs, not directly
         * with any given memory pool pointer. This parameter remains
         * only for backwards code compatibility. */
        status = NU_USBF_ETH_Create(NU_USBF_USER_ETH_Cb_Pt,
                                    "USBF-ETH",
#if (EF_VERSION_COMP >= EF_2_0)
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
        status = NU_USB_DRVR_Register_User((NU_USB_DRVR *) usbf_comm_handle,
                                           (NU_USB_USER *) NU_USBF_USER_ETH_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 3;
        }
    }

    if (!rollback)
    {
#ifndef CFG_NU_OS_DRVR_USB_FUNC_NET_IF_ENABLE
        /* Register the device with device manager. */
        status = NU_USB_SYS_Register_Device(NU_USBF_USER_ETH_Cb_Pt,
                                            NU_USBCOMPF_CDC);
#endif
#ifdef  CFG_NU_OS_DRVR_USB_FUNC_NET_IF_ENABLE
        /* Initialize USB function net driver. */
        status = NU_USBF_NET_Initialize(path);
#endif
        if(status != NU_SUCCESS)
        {
            rollback = 4;
        }
    }

    /* Clean up in case error occurs. */
    switch (rollback)
    {
    case 4:
        internal_sts |= NU_USB_DRVR_Deregister_User((NU_USB_DRVR *) usbf_comm_handle,
                                                    (NU_USB_USER *) NU_USBF_USER_ETH_Cb_Pt);

    case 3:
        internal_sts |= _NU_USBF_ETH_Delete ((VOID *) NU_USBF_USER_ETH_Cb_Pt);

    case 2:
        if (NU_USBF_USER_ETH_Cb_Pt)
        {
            internal_sts |= USB_Deallocate_Memory((VOID *)NU_USBF_USER_ETH_Cb_Pt);
            NU_USBF_USER_ETH_Cb_Pt = NU_NULL;
        }

    case 1:
    case 0:
        /* internal_sts is not used after this. So to remove
         * KW and PC-Lint warning set it as unused variable.
         */
         NU_UNUSED_PARAM(internal_sts);
         break;
    }
    return (status);
}

#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
/*************************************************************************
* FUNCTION
*
*       NU_USBF_ETH_DEV_Init_Task
*
* DESCRIPTION
*
*       Ethernet Driver Initialization Task Function.
*       This task is created in 'nu_os_conn_usb_func_comm_eth_init'
*       if USBF device is not yet initialized. It waits for initialzation
*       of USBF device and calls ETH initialize function.
*
* INPUTS
*
*       argc           Argument count
*       argv           Argument
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID NU_USBF_ETH_DEV_Init_Task(UNSIGNED argc, VOID * argv)
{
    CHAR *path = (CHAR *)argv;
    STATUS status;

    status = NU_USBF_Wait_For_Init(NU_SUSPEND);
    if (status == NU_SUCCESS)
    {
        status = NU_USBF_ETH_DEV_Init(path);
    }
}
#endif /* DV_DEV_DISCOVERY_TASK_ENB */
/* ======================  End Of File  =============================== */
