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
************************************************************************/

/************************************************************************
*
*    FILE NAME
*
*        nu_usb_test_ext.h
*
*    COMPONENT
*
*        Nucleus USB Software
*
*    DESCRIPTION
*
*        This file contains compliance test mode support function
*        prototypes for the usb 2.0 component.
*
*    DATA STRUCTURES
*
*        None
*
*    FUNCTIONS
*
*        None
*
*    DEPENDENCIES
*
*        nu_usb_imp.h            USB Component's Internal definitions
*
*************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_TEST_EXT_H_
#define _NU_USB_TEST_EXT_H_
#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif

/* ====================  Event Macros ================================= */

#define	USB_EVT_TEST_J              (1 << USB_TEST_J)
#define	USB_EVT_TEST_K              (1 << USB_TEST_K)
#define	USB_EVT_TEST_SE0_NAK        (1 << USB_TEST_SE0_NAK)
#define	USB_EVT_TEST_PACKET         (1 << USB_TEST_PACKET)
#define	USB_EVT_TEST_FORCE_ENABLE   (1 << USB_TEST_FORCE_ENABLE)
#define	USB_EVT_TEST_CTRL_DELAY     (1 << USB_TEST_CTRL_DELAY)

/* ==============  Task Specific Macros ==============================  */

#define	USB_TEST_MODE_TASK_STACK        2048
#define	USB_TEST_MODE_TASK_PRIORITY	    20
#define	USB_TEST_MODE_TASK_TIME_SLICE   10

/* ====================  Function Prototypes ========================== */

#ifdef          __cplusplus
}                                           /* End of C declarations     */
#endif

#endif /* _NU_USB_TEST_EXT_H_ */

/* ======================  End Of File  =============================== */
