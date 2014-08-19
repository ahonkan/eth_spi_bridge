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
*       nu_usbf_init_ext.c
*
*
* COMPONENT
*
*       Nucleus USB Function Software : Function Stack Initialization
*
* DESCRIPTION
*
*       This file contains the Host stack initialization sequence.
*
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBF_Init            Initializes the function stack.
*       NU_USBF_Init_GetHandle  Gets the function stack handle.
*       NU_USBF_DeInit          Un-initializes the function stack.
*
* DEPENDENCIES
*
*       nu_usb.h              All USB definitions
*
**************************************************************************/
#ifndef USBF_INIT_EXT_C
#define USBF_INIT_EXT_C
/* ==============  USB Include Files =================================== */
#include "connectivity/nu_usb.h"
#include "services/reg_api.h"
#include "services/runlevel_init.h"

/* Global variable used to open USB Function controller. */
UINT16      USBF_PID;
UINT16      USBF_VID;
CHAR        USBF_Manufacturar_String[NU_USB_MAX_STRING_LEN];
CHAR        USBF_Product_String[NU_USB_MAX_STRING_LEN];
CHAR        USBF_Serial_Number[NU_USB_MAX_STRING_LEN];

#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
    /* Event group for USBF device.*/
    static    NU_EVENT_GROUP          USBF_DEV_Event;
    #define   EVENT_USBF_INITIALIZED  0x00000001
#endif /* DV_DEV_DISCOVERY_TASK_ENB */

/**************************************************************************
*   FUNCTION
*
*       NU_USBF_System_Init
*
*   DESCRIPTION
*
*       This function first initializes the USB function stack and
*       class drivers based on their registry values. At the end it
*       opens USB function hardware controllers.
*
*   INPUTS
*
*       path                                Registry path of component.
*       startstop                           Flag to find if component is
*                                           being enabled or disabled.
*
*   OUTPUTS
*
*       status              Success or failure of the creation of the
*                           underlying initialization routines.
*
***************************************************************************/
STATUS nu_os_conn_usb_func_stack_init(CHAR *path, INT startstop)
{
    NU_USB_DEVICE       *device;
    DV_LISTENER_HANDLE  listener_handle;
    DV_DEV_LABEL        usbf_ctrl_label = {USBFHW_LABEL};
    STATUS              status  = NU_SUCCESS, reg_status;
    CHAR                func_stack_path[80];

    if (startstop == RUNLEVEL_START)
    {
        func_stack_path[0] = '\0';
        strcat(func_stack_path, path);

        /* Save registry settings of USB Function Stack. */
        func_stack_path[0] = '\0';
        strcat(func_stack_path, path);
        strcat(func_stack_path, "/PID");
        reg_status = REG_Get_UINT16(func_stack_path, &USBF_PID);
        if(reg_status == NU_SUCCESS)
        {
            func_stack_path[0] = '\0';
            strcat(func_stack_path, path);
            strcat(func_stack_path, "/VID");
            reg_status = REG_Get_UINT16(func_stack_path, &USBF_VID);
            if(reg_status == NU_SUCCESS)
            {
                func_stack_path[0] = '\0';
                strcat(func_stack_path, path);
                strcat(func_stack_path, "/manuf_string");
                reg_status = REG_Get_String(func_stack_path, USBF_Manufacturar_String, NU_USB_MAX_STRING_LEN);
                if(reg_status == NU_SUCCESS)
                {
                    func_stack_path[0] = '\0';
                    strcat(func_stack_path, path);
                    strcat(func_stack_path, "/product_string");
                    reg_status = REG_Get_String(func_stack_path, USBF_Product_String, NU_USB_MAX_STRING_LEN);
                    if(reg_status == NU_SUCCESS)
                    {
                        func_stack_path[0] = '\0';
                        strcat(func_stack_path, path);
                        strcat(func_stack_path, "/serial_number");
                        reg_status = REG_Get_String(func_stack_path, USBF_Serial_Number, NU_USB_MAX_STRING_LEN);
                    }
                }
            }
        }

        /* First create USB Memory Pools. */
        status = nu_os_conn_usb_com_stack_init(startstop);
        if(status == NU_SUCCESS)
        {
            /* First initialize USB Function stack.
             * In following API call, passing memory pool ptr parameters
             * NU_NULL because in ReadyStart memory in USB system is
             * allocated through USB specific memory APIs, not directly
             * with any given memory pool pointer. These parameter remain
             * only for backwards code compatibility. */

            status = NU_USBF_Init(NU_NULL, NU_NULL);
            if (status == NU_SUCCESS)
            {
                status = USBF_DEVCFG_Get_Device_Handle(&device);
                if ( (status == NU_SUCCESS) && (device != NU_NULL) )
                {
                    /* Read Vendor ID, Product ID, Manufacturer String, Product String and Serial Num String here. */
                    status = USBF_DEVCFG_Device_Initialize(device, USBF_VID, USBF_PID, USBF_Manufacturar_String, USBF_Product_String, USBF_Serial_Number);
                    if ( status == NU_SUCCESS )
                    {
                        /* Register a notification for device registeration and un-registeration. */
                        status = DVC_Reg_Change_Notify(&usbf_ctrl_label,
                                                    DV_GET_LABEL_COUNT(usbf_ctrl_label),
                                                    NU_USBF_Dev_Register,
                                                    NU_USBF_Dev_Unregister,
                                                    NU_NULL,
                                                    &listener_handle);
                    }
                }
            }
        }
    }
    else if (startstop == RUNLEVEL_STOP)
    {
        status = nu_os_conn_usb_com_stack_init(startstop);
    }

    return (status);
}

/**************************************************************************
*   FUNCTION
*
*       NU_USBF_Init
*
*   DESCRIPTION
*
*       This function creates the function stack subsystem and then
*       the stack itself.
*
*   INPUTS
*
*       name                string name of the initialize process.
*       context             specific context data.
*       init_data           containing system memories for used by
*                          components.
*       event_id            event id for this initialization process.
*
*   OUTPUTS
*
*       status              Success or failure of the creation of the
*                           underlying initialization routines.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
***************************************************************************/
STATUS NU_USBF_Init(NU_MEMORY_POOL *USB_Cached_Pool,
                    NU_MEMORY_POOL *USB_Uncached_Pool)
{
    STATUS  status;
    STATUS  internal_sts = 0;
    UINT8    rollback = 0;
    NU_USBF *usbf_singleton = NU_NULL;

    /* Error checking.   */
    NU_USB_MEMPOOLCHK(USB_Cached_Pool);
    NU_USB_MEMPOOLCHK(USB_Uncached_Pool);

    /* Allocate memory for USBF Singleton */
    status = USB_Allocate_Object(sizeof(NU_USBF),
                                (VOID **)&usbf_singleton);
    if(status != NU_SUCCESS)
    {
        rollback = 1;
    }

    if(!rollback)
    {
        /* Allocate memory for USBF Stack */
        status = USB_Allocate_Object(sizeof(NU_USBF_STACK),
                                    (VOID **)&NU_USBF_Stack_CB_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 2;
        }
    }

    /* Create the device subsystem */
    if(!rollback)
    {
        status = NU_USBF_Create (usbf_singleton);
        if(status != NU_SUCCESS)
        {
            rollback = 3;
        }
    }

    /* Create the stack */
    if(!rollback)
    {
        status = NU_USBF_STACK_Create (NU_USBF_Stack_CB_Pt, "USBF Stk");
        if(status != NU_SUCCESS)
        {
            rollback = 4;
        }
    }
#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
    if (!rollback)
    {
        /* Create event group that will post USBF device */
        /* events e.g. device initialized. */
        status = NU_Create_Event_Group (&USBF_DEV_Event, "USBF_EG");
        if (status != NU_SUCCESS)
        {
            rollback = 5;
        }
    }
#endif /* DV_DEV_DISCOVERY_TASK_ENB */

    /* Clean up in case error occurs. */
    switch (rollback)
    {
#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
        case 5:
            internal_sts = _NU_USBF_STACK_Delete(NU_USBF_Stack_CB_Pt);
#endif /* DV_DEV_DISCOVERY_TASK_ENB */
        case 4:
            internal_sts |= NU_USBF_Delete();

        case 3:
            internal_sts |= USB_Deallocate_Memory(NU_USBF_Stack_CB_Pt);
            NU_USBF_Stack_CB_Pt = NU_NULL;

        case 2:
            internal_sts |= USB_Deallocate_Memory(usbf_singleton);
            nu_usbf = NU_NULL;

        case 1:
        case 0:
        /* internal_sts is not used after this. So to remove
         * KW and PC-Lint warning set it as unused parameter.
         */
        NU_UNUSED_PARAM(internal_sts);
    }

    return (status);
}

/**************************************************************************
*   FUNCTION
*
*       NU_USBF_Init_GetHandle
*
*   DESCRIPTION
*
*       This function gets the function stack handle.
*
*   INPUTS
*
*       handle       double pointer used to retrieve the function
*                   stack handle.
*
*   OUTPUTS
*
*       NU_SUCCESS          indicate there exists a function stack.
*       NU_NOT_PRESENT      indicate there exists no function stack.
*
***************************************************************************/
STATUS NU_USBF_Init_GetHandle(VOID  **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    if (handle == NU_NULL || NU_USBF_Stack_CB_Pt == NU_NULL)
    {
        /* Function stack is not present.*/
        status = NU_NOT_PRESENT;
    }
    else
    {
        *handle = NU_USBF_Stack_CB_Pt;
        status  = NU_SUCCESS;
    }

   /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/************************************************************************
*
*   FUNCTION
*
*       NU_USBF_Dev_Register
*
*   DESCRIPTION
*
*       This function is called by device manager as soon as a new USB
*       function controller hardware is registered with device manager.
*       This funciton, opens the specified device, and register it with
*       nucleus USB function stack.
*
*************************************************************************/
STATUS NU_USBF_Dev_Register(DV_DEV_ID device_id, VOID *context)
{
    NU_USBF_HW          *hwctrl_ptr;
    NU_USB_DEVICE       *device;
    NU_USBF_STACK       *usbf_stack;
    STATUS              status;
    DV_DEV_LABEL        usbf_ctrl_label = {USBFHW_LABEL};
    DV_DEV_HANDLE       usbf_ctrl_sess_hd;
    UINT8               config_idx;
    DV_IOCTL0_STRUCT    dev_ioctl0;
    UINT8               speed;

    /* Hardware initialization. Open USB Function controller.
     * As there is support for only one USB function controller
     * so pass first id only. */
    status = DVC_Dev_ID_Open(device_id, &usbf_ctrl_label, 1, &usbf_ctrl_sess_hd);

    /* If device was opened successfully then find its handle and register it with the stack. */
    if(status == NU_SUCCESS)
    {
        /* Get IOCTL base address */
        dev_ioctl0.label = usbf_ctrl_label;
        status = DVC_Dev_Ioctl(usbf_ctrl_sess_hd, DV_IOCTL0, &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));

        if(status == NU_SUCCESS)
        {
            /* Get the device handle */
            status  = DVC_Dev_Ioctl(usbf_ctrl_sess_hd,
                                    dev_ioctl0.base + NU_USB_IOCTL_GET_HW_CB,
                                    (VOID*)&hwctrl_ptr,
                                    sizeof(NU_USBF_HW *));
            if(status == NU_SUCCESS)
            {
                /* Save IOCTL base address and session handle. */
                ((NU_USB_HW*)hwctrl_ptr)->dv_handle = usbf_ctrl_sess_hd;
                ((NU_USB_HW*)hwctrl_ptr)->ioctl_base_addr = dev_ioctl0.base;

                status = NU_USBF_Init_GetHandle ((VOID **)&usbf_stack);
                if(status == NU_SUCCESS)
                {
                    /* Add the controller to the stack. */
                    status = NU_USB_STACK_Add_Hw((NU_USB_STACK *)usbf_stack,
                                                 (NU_USB_HW *)hwctrl_ptr);
                    if(status == NU_SUCCESS)
                    {
                        status = USBF_DEVCFG_Get_Device_Handle(&device);
                        if ( status == NU_SUCCESS )
                        {
                            status = NU_USB_DEVICE_Set_Hw(device,(NU_USB_HW *)hwctrl_ptr );
                            if(status == NU_SUCCESS)
                            {
                                /* Get maximum supported speed of USB device. */
                                status = NU_USB_HW_Get_Speed((NU_USB_HW *)hwctrl_ptr, &speed);
                                if ( status == NU_SUCCESS )
                                {
                                    switch(speed)
                                    {
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                                    case USB_SPEED_SUPER:
                                        status = USBF_Init_Super_Speed_Device(device, hwctrl_ptr);
                                        break;
#endif /* #if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
                                    case USB_SPEED_HIGH:
                                        status = USBF_Init_High_Speed_Device(device, hwctrl_ptr);
                                        break;
                                    case USB_SPEED_FULL:
                                        status = USBF_Init_Full_Speed_Device(device, hwctrl_ptr);
                                        break;
                                    default:
                                        status = NU_USB_INVLD_SPEED;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        /* If everything is fine then create the default configuration for device. */
        if ( status == NU_SUCCESS )
        {
            /* Create default configuration. */
            status = USBF_DEVCFG_Create_Config(device, &config_idx);
#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
            if (status == NU_SUCCESS)
            {
                /* USBF device initialized, post event. */
                status = NU_Set_Events (&USBF_DEV_Event, EVENT_USBF_INITIALIZED, NU_OR);
            }
#endif /* (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE) */
        }
    }

    return ( status );
}

/************************************************************************
*
*   FUNCTION
*
*      NU_USBF_Dev_Unregister
*
*   DESCRIPTION
*
*   This function is called by device manager as soon as a USB function
*   controller hardware is unregistered from device manager.
*   This function is left empty because its implementation is not required.
*
*************************************************************************/
STATUS NU_USBF_Dev_Unregister(DV_DEV_ID device_id, VOID *context)
{
    return ( NU_SUCCESS );
}

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
STATUS USBF_Init_Super_Speed_Device(NU_USB_DEVICE *device, NU_USBF_HW *hwctrl_ptr)
{
    UINT8   ep0_ss_maxp;
    STATUS  status;

    /* Get maximum packet size of device for super Speed. */
    status = NU_USBF_HW_Get_EP0_Maxp(hwctrl_ptr, USB_SPEED_SUPER, &ep0_ss_maxp);
    if ( status == NU_SUCCESS )
    {
        /* Prepare BOS descriptor for SS device. */
        status = USBF_DEVCFG_Create_BOS(device);
        if ( status == NU_SUCCESS )
        {
            /* Add device capability descriptors to BOS. */
            status = USBF_DEVCFG_Add_BOS_DevCap_USB2Ext(device);
            if ( status == NU_SUCCESS )
            {
                status = USBF_DEVCFG_Add_BOS_DevCap_SS(device);
                if ( status == NU_SUCCESS )
                {
                    /* Set BCDUSB to USB 3.0 and max packet size to 9 (2^9 = 512). */
                    status = NU_USB_DEVICE_Set_bcdUSB(device, BCD_USB_VERSION_30);
                    if ( status == NU_SUCCESS )
                    {
                        status = NU_USB_DEVICE_Set_bMaxPacketSize0(device, ep0_ss_maxp);
                    }
                }
            }
        }
    }

    return ( status );
}
#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

STATUS USBF_Init_High_Speed_Device(NU_USB_DEVICE *device, NU_USBF_HW *hwctrl_ptr)
{
    STATUS  status;
    UINT8   ep0_fs_maxp, ep0_hs_maxp;
    NU_USB_DEV_QUAL_DESC    device_qual;

    /* Get maximum packet size of device for Full Speed. */
    status = NU_USBF_HW_Get_EP0_Maxp(hwctrl_ptr, USB_SPEED_FULL, &ep0_fs_maxp);
    if ( status == NU_SUCCESS )
    {
        /* Get maximum packet size of device for High Speed. */
        status = NU_USBF_HW_Get_EP0_Maxp(hwctrl_ptr, USB_SPEED_HIGH, &ep0_hs_maxp);
        if ( status == NU_SUCCESS )
        {
            /* Set BCDUSB to USB 2.0 and max packet size to 64. */
            status = NU_USB_DEVICE_Set_bcdUSB(device, BCD_USB_VERSION_20);
            if ( status == NU_SUCCESS )
            {
                status = NU_USB_DEVICE_Set_bMaxPacketSize0(device, ep0_hs_maxp);
                if ( status == NU_SUCCESS )
                {
                    device_qual.bLength             = 10;
                    device_qual.bDescriptorType     = USB_DT_DEVICE_QUALIFIER;
                    device_qual.bcdUSB              = BCD_USB_VERSION_20;
                    device_qual.bDeviceClass        = 0;
                    device_qual.bDeviceSubClass     = 0;
                    device_qual.bDeviceProtocol     = 0;
                    device_qual.bMaxPacketSize0     = ep0_fs_maxp;
                    device_qual.bNumConfigurations  = 0;
                    device_qual.bReserved           = 0;

                    status = NU_USB_DEVICE_Set_Device_Qualifier(device, &device_qual);

#if (USB_TEST_MODE_SUPPORT == NU_TRUE)
                    if(status == NU_SUCCESS)
                    {
                    	status = NU_USBF_Test_Init((NU_USB_HW*)hwctrl_ptr);
                    }
#endif
                }
            }
        }
    }

    return ( status );
}

STATUS USBF_Init_Full_Speed_Device(NU_USB_DEVICE *device, NU_USBF_HW *hwctrl_ptr)
{
    STATUS  status;
    UINT8   ep0_fs_maxp;

    /* Get maximum packet size of device for Full Speed. */
    status = NU_USBF_HW_Get_EP0_Maxp(hwctrl_ptr, USB_SPEED_FULL, &ep0_fs_maxp);
    if ( status == NU_SUCCESS )
    {
        /* Get maximum packet size in device descriptor. */
        status = NU_USB_DEVICE_Set_bMaxPacketSize0(device, ep0_fs_maxp);
        if ( status == NU_SUCCESS )
        {
            /* Set BCDUSB to USB 2.0. */
            status = NU_USB_DEVICE_Set_bcdUSB(device, BCD_USB_VERSION_20);

#if (USB_TEST_MODE_SUPPORT == NU_TRUE)
            if(status == NU_SUCCESS)
            {
                status = NU_USBF_Test_Init((NU_USB_HW*)hwctrl_ptr);
            }
#endif
        }
    }

    return ( status );
}

/**************************************************************************
*   FUNCTION
*
*       NU_USBF_DeInit
*
*   DESCRIPTION
*
*       This function deletes the function stack and function stack
*       subsystem.
*
*   INPUTS
*
*       context             specific context data.
*       event_id            event id for this initialization process.
*
*   OUTPUTS
*
*       status              Success or failure of the deletion of the
*                           underlying initialization routines.
**************************************************************************/
STATUS NU_USBF_DeInit( VOID *context,
                       UINT32 event_id)
{
    STATUS  status = NU_INVALID_POINTER;
    NU_USBF *usbf_singleton = NU_NULL;

    NU_UNUSED_PARAM(context);
    NU_UNUSED_PARAM(event_id);

    /* Delete the function stack. */

    if (NU_USBF_Stack_CB_Pt)
    {
        /* Any generic deletions to be done by the parent */
        status = _NU_USBF_STACK_Delete (NU_USBF_Stack_CB_Pt);
        status |= USB_Deallocate_Memory(NU_USBF_Stack_CB_Pt);
        NU_USBF_Stack_CB_Pt = NU_NULL;
    }

    /* Delete the function stack subsystem. */
    if (nu_usbf)
    {
        usbf_singleton = nu_usbf;
        status |= NU_USBF_Delete();
        status |= USB_Deallocate_Memory(usbf_singleton);
        usbf_singleton = NU_NULL;
    }

#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
    /* Delete device initialization event group. */
    status |= NU_Delete_Event_Group (&USBF_DEV_Event);
#endif /* DV_DEV_DISCOVERY_TASK_ENB */

    return (status);
}

#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
/**************************************************************************
*   FUNCTION
*
*       NU_USBF_Wait_For_Init
*
*   DESCRIPTION
*
*       This function retrives USB Function initialzed event.
*       It can wait for event indefinately or return immediately
*       based on input argument.
*
*   INPUTS
*
*       suspend       Event wait suspend option
*                     (NU_SUSPEND/NU_NO_SUSPEND)
*
*   OUTPUTS
*
*       status        NU_SUCCESS
*                     NU_NOT_PRESENT
*                     ...
*
**************************************************************************/
STATUS NU_USBF_Wait_For_Init(UNSIGNED suspend)
{
    STATUS   status;
    UNSIGNED ret_event;

    /* Wait for USBF initialized event. */
    status = NU_Retrieve_Events(&USBF_DEV_Event, NU_AND_CONSUME,
                                EVENT_USBF_INITIALIZED, &ret_event, suspend);
    if (status == NU_SUCCESS)
    {
        if ( EVENT_USBF_INITIALIZED != (ret_event & EVENT_USBF_INITIALIZED) )
        {
            /* Event does not match with what we are looking for. */
            status = NU_NOT_PRESENT;
        }
    }

    return status;
}
#endif /* (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE) */

/*************************************************************************/
#endif /* USBF_INIT_EXT_C*/
/* ======================  End Of File  ================================ */
