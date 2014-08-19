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
 *      nu_usb_subsys_imp.h
 *
 * COMPONENT
 *      Nucleus USB Software
 *
 * DESCRIPTION
 *
 *        This file contains the function names and data structures
 *        for the common sub system component.
 *
 * DATA STRUCTURES
 *      nu_usb_subsys           sub system control block description.
 *      usb_subsys_dispatch     sub system dispatch table.
 *
 * FUNCTIONS
 *      None
 *
 * DEPENDENCIES
 *      None
 *
 *************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_SUBSYS_IMP_H_
#define _NU_USB_SUBSYS_IMP_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ========================== #defines  =============================== */
#define NU_USB_MAX_IDS 255
#define USB_NO_ID      0xff

/* ====================  Data Structures ============================== */

typedef struct nu_usb_subsys_dispatch
{
    STATUS (*Delete) (NU_USB_SUBSYS * cb);
    STATUS (*Lock) (NU_USB_SUBSYS * cb);
    STATUS (*Unlock) (NU_USB_SUBSYS * cb);
}
NU_USB_SUB_SYSTEM_DISPATCH;

struct nu_usb_subsys
{
#if (!NU_USB_OPTIMIZE_FOR_SIZE)
    CS_NODE *list_head;
    NU_USB_SUB_SYSTEM_DISPATCH *dispatch;
    UINT32 signature;
    UINT8 id_map[NU_USB_MAX_IDS / 8 + 1];
    UINT8 last_allocated_id;
#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

};

/* ====================  Function Prototypes ========================== */
#if (!NU_USB_OPTIMIZE_FOR_SIZE)
/* Specific  API. */

STATUS _NU_USB_SUBSYS_Delete (NU_USB_SUBSYS * cb);
STATUS _NU_USB_SUBSYS_Lock (NU_USB_SUBSYS * cb);
STATUS _NU_USB_SUBSYS_Unlock (NU_USB_SUBSYS * cb);

#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

/* Utility functions */
UINT8 usb_get_id (UINT8 *bits,
                  UINT8 size,
                  UINT8 last_id);
VOID usb_release_id (UINT8 *bits,
                     UINT8 size,
                     UINT8 id);
UINT8 usb_is_id_set (UINT8 *bits,
                     UINT8 size,
                     UINT8 id);

/* ==================================================================== */

#endif /* _NU_USB_SUBSYS_IMP_H_ */

/* ======================  End Of File  =============================== */

