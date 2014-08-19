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

*************************************************************************
*
* FILE NAME
*
*        nu_usbh_init_ext.c
*
* COMPONENT
*
*        Nucleus USB Host Software : Host Stack Initialization
*
* DESCRIPTION
*
*        This file contains the Host stack initialization sequence.
*
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       NU_USBH_Init                         Initializes the host stack.
*       NU_USBH_Init_GetHandle               Gets the host stack handle.
*       NU_USBH_DeInit                       Un-Initializes the host stack.
*
* DEPENDENCIES
*
*       nu_usb.h                             All USB definitions
*
************************************************************************/
#ifndef USBH_INIT_EXT_C
#define USBH_INIT_EXT_C

/* ==============  Standard Include Files ============================  */
#include "connectivity/nu_usb.h"
#include "services/runlevel_init.h"

/**************************************************************************
*   FUNCTION
*
*       NU_USBH_System_Init
*
*   DESCRIPTION
*
*       This function first initializes the USB host stack and
*       class drivers based on their registry values. At the end it
*       opens USB host hardware controllers.
*
*   INPUTS
*
*       path                                Registry path of component.
*       compctrl                           Flag to find if component is
*                                           being enabled or disabled.
*
*   OUTPUTS
*
*       status              Success or failure of the creation of the
*                           underlying initialization routines.
*
***************************************************************************/
STATUS nu_os_conn_usb_host_stack_init(CHAR *path, INT compctrl)
{
    STATUS              status;
    DV_LISTENER_HANDLE  listener_handle;
    DV_DEV_LABEL        usbh_ctrl_label = {USBHHW_LABEL};

    if(compctrl == RUNLEVEL_START)
    {
        /* First create USB Memory Pools. */
        status = nu_os_conn_usb_com_stack_init(compctrl);
        
        if(status == NU_SUCCESS)
        {
            /* First initialize USB Host stack.
             * In following API call, passing memory pool ptr parameters
             * NU_NULL because in ReadyStart memory in USB system is
             * allocated through USB specific memory APIs, not directly
             * with any given memory pool pointer. These parameter remain
             * only for backwards code compatibility. */

            status = NU_USBH_Init(NU_NULL, NU_NULL);
            if (status == NU_SUCCESS)
            {
                /* Register a notification for device registeration and un-registeration. */
                status = DVC_Reg_Change_Notify(&usbh_ctrl_label,
                                            DV_GET_LABEL_COUNT(usbh_ctrl_label),
                                            NU_USBH_Dev_Register,
                                            NU_USBH_Dev_Unregister,
                                            NU_NULL,
                                            &listener_handle);
            }
        }
    }
    else
    {
        status = NU_SUCCESS;
    }

    return (status);
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_Init
*
*   DESCRIPTION
*
*       This function creates the host stack subsystem and then
*       the stack itself.
*
*   INPUTS
*
*       name                string name of the initialize process.
*       context             specific context data.
*       init_data           containing system memories for used by
*                           components.
*       event_id            event id for this initialization process.
*
*   OUTPUTS
*
*       status              Success or failure of the creation of the
*                           underlying initialization routines.
*       NU_USB_INVLD_ARG    Invalid arguments
*************************************************************************/
STATUS  NU_USBH_Init(NU_MEMORY_POOL *USB_Cached_Pool,
                     NU_MEMORY_POOL *USB_Uncached_Pool)
{
    STATUS  status, internal_sts = NU_SUCCESS;
    VOID    *host_stack = NU_NULL;
    VOID    *hub_stack = NU_NULL;
    VOID    *hisr_stack = NU_NULL;
    NU_USBH *singleton_cb = NU_NULL;
    UINT8    rollback = 0;

    /* Error checking.   */
    NU_USB_MEMPOOLCHK(USB_Cached_Pool);
    NU_USB_MEMPOOLCHK(USB_Uncached_Pool);

    /* Allocate memory for USBH Singleton */
    status = USB_Allocate_Object(sizeof(NU_USBH),
                                 (VOID **)&singleton_cb);
    /* Create the device subsystem */
    if (status != NU_SUCCESS)
    {
        rollback = 1;
    }

    if(!rollback)
    {
        /* Sets up the Host environment */
        status = NU_USBH_Create (singleton_cb);
        if(status != NU_SUCCESS)
        {
            rollback = 2;
        }
    }

    if(!rollback)
    {
        /* Allocate memory for host stack */
        status = USB_Allocate_Object(sizeof(NU_USBH_STACK),
                                     (VOID **)&NU_USBH_Stack_CB_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 3;
        }
    }

    if(!rollback)
    {
        /* Allocate the stack for the host task stack */
        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                     NU_USBH_TASK_STACK_SIZE,
                                     &host_stack);
        if(status != NU_SUCCESS)
        {
            rollback = 4;
        }
    }

    if(!rollback)
    {
        /* Allocate the stack for the hub task stack */
        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                     NU_USBH_HUB_STACK_SIZE,
                                     &hub_stack);
        if(status != NU_SUCCESS)
        {
            rollback = 5;
        }
    }

    if(!rollback)
    {
        /* Allocate the stack for the HISR stack */
        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                     NU_USBH_HISR_STACK_SIZE,
                                     &hisr_stack);
        if(status != NU_SUCCESS)
        {
            rollback = 6;
        }
    }

    if(!rollback)
    {
        /* Create the Host stack */
        status = NU_USBH_STACK_Create (NU_USBH_Stack_CB_Pt, "USBH Stk",
                                        USB_Cached_Pool,
                                        host_stack,
                                        NU_USBH_TASK_STACK_SIZE,
                                        NU_USBH_STACK_PRIORITY,
                                        hub_stack,
                                        NU_USBH_HUB_STACK_SIZE,
                                        NU_USBH_HUB_PRIORITY,
                                        hisr_stack,
                                        NU_USBH_HISR_STACK_SIZE,
                                        NU_USBH_HISR_PRIORITY);
        if(status != NU_SUCCESS)
        {
            rollback = 7;
        }
    }

    /* Clean up in case of error. */
    switch (rollback)
    {
        case 7:
        internal_sts |= USB_Deallocate_Memory(hisr_stack);

        case 6:
        internal_sts |= USB_Deallocate_Memory(hub_stack);

        case 5:
        internal_sts |= USB_Deallocate_Memory(host_stack);

        case 4:
        internal_sts |= USB_Deallocate_Memory(NU_USBH_Stack_CB_Pt);
        NU_USBH_Stack_CB_Pt = NU_NULL;

        case 3:
        internal_sts |= NU_USBH_Delete();

        case 2:
        internal_sts |= USB_Deallocate_Memory(singleton_cb);

        case 1:
        case 0:
        /* internal_sts is not used after this. So to remove
         * KW and PC-Lint warning set it as unused parameter.
         */
        NU_UNUSED_PARAM(internal_sts);
    }

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_Init_GetHandle
*
*   DESCRIPTION
*
*       This function gets the host stack handle.
*
*   INPUTS
*
*       handle              double pointer used to retrieve the host
*                           stack handle.
*
*   OUTPUTS
*
*       NU_SUCCESS          indicate there is a host stack.
*       NU_NOT_PRESENT      indicate there is no host stack.
*
*************************************************************************/
STATUS NU_USBH_Init_GetHandle(VOID  **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBH_Stack_CB_Pt;
    if (NU_USBH_Stack_CB_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/************************************************************************
*
*   FUNCTION
*
*      NU_USBH_Dev_Register
*
*   DESCRIPTION
*
*   This function is called by device manager as soon as it finds a new USB
*   host controller hardware is registered with device manager.
*   This funciton, opens the specified device, and register it with nucleus USB host
*   stack.
*
*************************************************************************/
STATUS NU_USBH_Dev_Register(DV_DEV_ID device_id, VOID *context)
{
    NU_USBH_HW          *hwctrl_ptr;
    NU_USBH_STACK       *usbh_stack;
    STATUS              status;
    DV_DEV_HANDLE       usbh_ctrl_sess_hd;
    DV_IOCTL0_STRUCT    dev_ioctl0;
    DV_DEV_LABEL        usbh_ctrl_label = {USBHHW_LABEL};

    /* Hardware initialization. Open USB Host controller.
     * As there is support for only one USB host controller
     * so pass first id only.
     */
    status = DVC_Dev_ID_Open(device_id, &usbh_ctrl_label, 1, &usbh_ctrl_sess_hd);
    if(status == NU_SUCCESS)
    {
        /* Get IOCTL base address */
        dev_ioctl0.label = usbh_ctrl_label;
        status = DVC_Dev_Ioctl(usbh_ctrl_sess_hd, DV_IOCTL0, &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));

        /* If device was opened successfully then find its handle and register it with the stack. */
        if(status == NU_SUCCESS)
        {
            /* Get the device handle */
            status  = DVC_Dev_Ioctl(usbh_ctrl_sess_hd,
                                    dev_ioctl0.base + NU_USB_IOCTL_GET_HW_CB,
                                    (VOID*)&hwctrl_ptr,
                                    sizeof(NU_USBH_HW *));
            if(status == NU_SUCCESS)
            {
                /* Save IOCTL base address and session handle. */
                ((NU_USB_HW*)hwctrl_ptr)->dv_handle = usbh_ctrl_sess_hd;
                ((NU_USB_HW*)hwctrl_ptr)->ioctl_base_addr = dev_ioctl0.base;

                status = NU_USBH_Init_GetHandle ((VOID **)&usbh_stack);
                if(status == NU_SUCCESS)
                {
                    /* Add the controller to the stack. */
                    status = NU_USB_STACK_Add_Hw((NU_USB_STACK *)usbh_stack,
                                                 (NU_USB_HW *)hwctrl_ptr);
                }

#if (USB_TEST_MODE_SUPPORT == NU_TRUE)
                    if(status == NU_SUCCESS)
                    {
                         status = NU_USBH_Test_Init((NU_USB_HW*)hwctrl_ptr);
                    }
#endif
            }
        }
    }

    return ( status );
}

/************************************************************************
*
*   FUNCTION
*
*      NU_USBH_Dev_Unregister
*
*   DESCRIPTION
*
*   This function is called by device manager as soon as a USB host controller
*   hardware is unregistered from device manager.
*   This function is left empty because its implementation is not required.
*
*************************************************************************/
STATUS NU_USBH_Dev_Unregister(DV_DEV_ID devicd_id, VOID *context)
{
    return ( NU_SUCCESS );
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_DeInit
*
*   DESCRIPTION
*
*       This function deletes the host stack subsystem and then
*       the stack itself.
*
*   INPUTS
*
*       context             specific context data.
*       event_id            event id for this initialization process.
*
*   OUTPUTS
*
*       status              Success or failure of the creation of the
*                           underlying un-initialization routines.
*       NU_USB_INVLD_ARG    Invalid arguments
*************************************************************************/
STATUS NU_USBH_DeInit( VOID *context,
                       UINT32 event_id)
{
    STATUS  status = NU_INVALID_POINTER;
    NU_USBH *singleton_cb = NU_NULL;

    NU_UNUSED_PARAM(context);
    NU_UNUSED_PARAM(event_id);

    if(NU_USBH_Stack_CB_Pt)
    {
        status = _NU_USBH_STACK_Delete(NU_USBH_Stack_CB_Pt);

        status |= USB_Deallocate_Memory
                 ((&(NU_USBH_Stack_CB_Pt->usbh_stack_task))->tc_stack_start);

        status |= USB_Deallocate_Memory
                  ((&(&(NU_USBH_Stack_CB_Pt->usbh_hisr))->hisr)->tc_stack_start);

        status |= USB_Deallocate_Memory
                  ((&(&(NU_USBH_Stack_CB_Pt->hub_driver))->hubTask)->tc_stack_start);

        status |= USB_Deallocate_Memory(NU_USBH_Stack_CB_Pt);

        NU_USBH_Stack_CB_Pt = NU_NULL;
    }

    if(nu_usbh)
    {
        singleton_cb = nu_usbh;
        status |= NU_USBH_Delete();
        status |= USB_Deallocate_Memory(singleton_cb);
        singleton_cb = NU_NULL;
    }

    return status;
}

/************************************************************************/

#endif /* USBH_INIT_EXT_C */
/* ======================  End Of File  =============================== */
