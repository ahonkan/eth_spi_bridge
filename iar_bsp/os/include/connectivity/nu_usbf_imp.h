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
*       nu_usbf_imp.h
*
*
* COMPONENT
*
*       Stack Component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the internal declarations for the
*       singleton  object.
*
*
* DATA STRUCTURES
*
*       NU_USBF                             Singleton object Control block.
*       USBF_IRQ_HASH_ENTRY                 Each entry in the IRQ map.
*
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_dat.h               USB Function data structure
*       nu_usbf_hw_ext.h            USB Function hardware controller
*       nu_usbf_subsys_ext.h        USB Function device subsystem
*
**************************************************************************/

#ifndef     _NU_USBF_IMP_H_
#define     _NU_USBF_IMP_H_

/* ===================================================================== */

#ifndef     _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==============  USB Include Files ==================================  */

#include "connectivity/nu_usbf_hw_ext.h"
#include "connectivity/nu_usbf_subsys_ext.h"

/* =====================  #defines ====================================  */

#define NU_USBF_HW_ID           0xfafa
#define NU_USBF_STACK_ID        0xfbfb
#define NU_USBF_DRVR_ID         0xfcfc
#define NU_USBF_USER_ID         0xfdfd

/* ====================  Data Types ===================================  */

typedef struct _usbf_irq_hash_entry
{
    NU_USBF_HW *fc;
    INT vector;
}
USBF_IRQ_HASH_ENTRY;

typedef struct nu_usbf
{
    NU_USBF_SUBSYS controller_subsys;
    NU_USBF_SUBSYS stack_subsys;
    NU_USBF_SUBSYS class_driver_subsys;
    NU_USBF_SUBSYS user_subsys;
    USBF_IRQ_HASH_ENTRY hash_table[NU_USB_MAX_IRQ * NU_USBF_MAX_HW];
    INT8 num_irq_entries;
}
NU_USBF;

/* ===================================================================== */

#include "connectivity/nu_usbf_dat.h"

#endif      /* _NU_USBF_IMP_H_           */

/* ======================  End Of File  ================================ */

