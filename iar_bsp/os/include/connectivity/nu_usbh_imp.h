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
************************************************************************/

/************************************************************************ 
 * 
 * FILE NAME
 *
 *      nu_usbh_imp.h 
 * 
 * COMPONENT 
 *      Nucleus USB Host Software 
 * 
 * DESCRIPTION 
 *      This file contains the Nucleus USB Host Singleton control block
 *      definitions.
 *
 * 
 * DATA STRUCTURES 
 *      nu_usbh             USB Host singleton control block. 
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      nu_usbh_stack_ext.h         Host stack definitions.
 *      nu_usbh_subsys_imp.h        Host subsystem definitions.
 *      nu_usbh_dat.h               Host Singleton dispatch table.
 * 
 *************************************************************************/

/* ===================================================================  */
#ifndef _NU_USBH_IMP_H_
#define _NU_USBH_IMP_H_
/* ===================================================================  */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usbh_stack_ext.h"
#include "connectivity/nu_usbh_subsys_imp.h"

/* ============================ #defines =============================  */
#define NU_USBH_HW_ID 0xfafa
#define NU_USBH_STACK_ID 0xfbfb
#define NU_USBH_DRVR_ID 0xfcfc
#define NU_USBH_USER_ID 0xfdfd

/* =================== Data Structures ===============================  */

typedef struct usbh_vector_hash
{
    INT vector;
    NU_USBH_HW *controller;
}
USBH_VECTOR_HASH;

typedef struct nu_usbh
{
    NU_USBH_SUBSYS controller_subsys;
    NU_USBH_SUBSYS stack_subsys;
    NU_USBH_SUBSYS class_driver_subsys;
    NU_USBH_SUBSYS user_subsys;
    UINT8 num_irq_entries;
    USBH_VECTOR_HASH hash_table[NU_USBH_MAX_HW];
}
NU_USBH;

/* ================== Function Prototypes ============================  */

/* Specific API */
STATUS _NU_USBH_Delete (void);

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usbh_dat.h"

/* ===================================================================  */
#endif /* _NU_USBH_IMP_H_ */
/* ====================== end of file ================================  */

