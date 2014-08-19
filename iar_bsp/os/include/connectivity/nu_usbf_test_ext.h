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
*        nu_usbf_test_ext.h
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
*        nu_usb_imp.h
*
*************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBF_TEST_EXT_H_
#define _NU_USBF_TEST_EXT_H_
#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif

/* ====================  Event Macros ================================= */

/* ====================  Function Prototypes ========================== */

STATUS NU_USBF_Test_Init(NU_USB_HW *cb);

STATUS NU_USBF_Test_Execute(NU_USB_HW *cb, UINT8 test_mode);

#ifdef          __cplusplus
}                                           /* End of C declarations     */
#endif

#endif /* _NU_USBF_TEST_EXT_H_ */

/* ======================  End Of File  =============================== */
