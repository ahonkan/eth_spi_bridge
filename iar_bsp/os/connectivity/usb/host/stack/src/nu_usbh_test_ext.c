/***********************************************************************
*
*           Copyright 2013 Mentor Graphics Corporation
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
*        nu_usbh_test_ext.c
*
* COMPONENT
*
*        Nucleus USB Software.
*
* DESCRIPTION
*
*        This file contains the implementation Test Modes
*        to fulfill USBH 2.0 Compliance Testing.
*
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*       NU_USBH_Test_Init                - Initializes the test mode
*                                         task
*
*       NU_USBH_Test_Execute             - Event Handler for Test Modes
*
* DEPENDENCIES
*
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef _NU_USBH_TEST_EXT_C_
#define _NU_USBH_TEST_EXT_C_

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usb.h"

#if (USB_TEST_MODE_SUPPORT == NU_TRUE)

/* ========  Test Mode Support Internal Variables ====================  */
static NU_EVENT_GROUP   usbh_test_mode_event;
static NU_TASK          usbh_test_mode_task;
static UINT8            usbh_test_mode_task_stack[USB_TEST_MODE_TASK_STACK];

/* ============  Internal Function Prototype =========================  */
static VOID USBH_Test_Mode_Handler(UNSIGNED, VOID*);

/**************************************************************************
* FUNCTION
*
*       NU_USBH_Test_Init
*
* DESCRIPTION
*
*       This function initializes the test mode task.
*
* INPUTS
*
*       cb                                  Control Block of USB Device
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completion of service.
*       NU_USB_INVLD_ARG                    Indicates an invalid argument.
*
**************************************************************************/
STATUS NU_USBH_Test_Init(NU_USB_HW *cb)
{
    STATUS status;

    /* Create Event Group to be used for function testing */
    status = NU_Create_Event_Group(&usbh_test_mode_event, "USB_TESTH");

    if(status == NU_SUCCESS)
    {
        /* Create Task to be used for function testing */
        status = NU_Create_Task(&usbh_test_mode_task,
                                "USB_TESTH",
                                USBH_Test_Mode_Handler,
                                0,
                                cb,
                                usbh_test_mode_task_stack,
                                USB_TEST_MODE_TASK_STACK,
                                USB_TEST_MODE_TASK_PRIORITY,
                                USB_TEST_MODE_TASK_TIME_SLICE,
                                NU_PREEMPT,
                                NU_START);
    }

    return status;

}

/**************************************************************************
* FUNCTION
*
*       NU_USBH_Test_Execute
*
* DESCRIPTION
*
*       This function handles the execution of tests of USB Host Stack
*
* INPUTS
*
*       cb                                  Control Block of USB Device
*       test_mode                           Test mode to set in dm8148
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completion of service.
*       NU_USB_INVLD_ARG                    Indicates an invalid argument.
*
**************************************************************************/
STATUS NU_USBH_Test_Execute(NU_USB_HW *cb, UINT8 test_mode)
{
    STATUS status;

    switch(test_mode)
    {
        case USB_TEST_J:
        case USB_TEST_K:
        case USB_TEST_SE0_NAK:
        case USB_TEST_PACKET:
        case USB_TEST_FORCE_ENABLE:
        case USB_TEST_CTRL_DELAY:
            status = NU_Set_Events(&usbh_test_mode_event, (1 << test_mode), NU_OR);
            break;
        default:
            status = NU_USB_INVLD_ARG;
            break;
    }

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       USBH_Test_Mode_Handler
*
* DESCRIPTION
*
*       This function sets the corresponding test mode in hardware as
*       soon as it receives the request.
*
* INPUTS
*       cb                                  Control Block of USB Device
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful
*                                           completion of service.
*       NU_USB_INVLD_ARG                    Indicates an invalid argument.
*
**************************************************************************/
static VOID USBH_Test_Mode_Handler(UNSIGNED argc, VOID *argv)
{
    NU_USB_HW   *cb;
    UNSIGNED    event;
    STATUS      status;

    cb = (NU_USB_HW*) argv;

    while(1)
    {
        status = NU_Retrieve_Events(&usbh_test_mode_event,
                                    (USB_EVT_TEST_J             |
                                     USB_EVT_TEST_K             |
                                     USB_EVT_TEST_SE0_NAK       |
                                     USB_EVT_TEST_PACKET        |
                                     USB_EVT_TEST_FORCE_ENABLE  |
                                     USB_EVT_TEST_CTRL_DELAY),
                                     NU_OR_CONSUME,
                                     &event,
                                     NU_SUSPEND);
        if(status == NU_SUCCESS)
        {
            if(event & USB_EVT_TEST_J)
                status = NU_USB_HW_Test_Mode(cb, USB_TEST_J);
            else if(event & USB_EVT_TEST_K)
                status = NU_USB_HW_Test_Mode(cb, USB_TEST_K);
            else if(event & USB_EVT_TEST_SE0_NAK)
                status = NU_USB_HW_Test_Mode(cb, USB_TEST_SE0_NAK);
            else if(event & USB_EVT_TEST_PACKET)
                status = NU_USB_HW_Test_Mode(cb, USB_TEST_PACKET);
            else if(event & USB_EVT_TEST_FORCE_ENABLE)
                status = NU_USB_HW_Test_Mode(cb, USB_TEST_FORCE_ENABLE);
            else if(event & USB_EVT_TEST_CTRL_DELAY)
                status = NU_USB_HW_Test_Mode(cb, USB_TEST_CTRL_DELAY);
        }
    }
}

#endif /* USB_TEST_MODE_SUPPORT */

#endif /* _NU_USBH_TEST_EXT_C_ */
/* =======================  End Of File  ============================== */
