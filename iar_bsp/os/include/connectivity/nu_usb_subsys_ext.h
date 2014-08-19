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
 *      nu_usb_subsys_ext.h
 *
 * COMPONENT
 *      Nucleus USB Software
 *
 * DESCRIPTION
 *
 *        This file contains the exported function names and data structures
 *        for the sub system.
 *
 * DATA STRUCTURES
 *      None
 *
 * FUNCTIONS
 *      None
 *
 * DEPENDENCIES
 *      nu_usb_subsys_imp.h         Subsystem internal definitions.
 *
 *************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_SUBSYS_EXT_H_
#define _NU_USB_SUBSYS_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================== Include Files ================================ */
#include "connectivity/nu_usb_subsys_imp.h"

#if (!NU_USB_OPTIMIZE_FOR_SIZE)
/* ====================  Function Prototypes ========================== */

/* NU_USB_Sub_System API. */

STATUS NU_USB_SUBSYS_Delete (NU_USB_SUBSYS * cb);

/* NU_USB_Sub_System extended API. */
STATUS _NU_USB_SUBSYS_Create (NU_USB_SUBSYS * cb,
                              const VOID *dispatch);

STATUS NU_USB_SUBSYS_Add_Node (NU_USB_SUBSYS * cb,
                               VOID *node);

STATUS NU_USB_SUBSYS_Remove_Node (NU_USB_SUBSYS * cb,
                                  VOID *node);

/* NU_USB_Sub_System Facts API */
UNSIGNED NU_USB_SUBSYS_Established_Nodes (NU_USB_SUBSYS * cb);
UNSIGNED NU_USB_SUBSYS_Node_Pointers (NU_USB_SUBSYS * cb,
                                      VOID **pointer_list,
                                      UNSIGNED maximum_pointers);

/* Id management APIs */
STATUS NU_USB_SUBSYS_Allocate_Id (NU_USB_SUBSYS * cb,
                                  UINT32 *new_object_id);
STATUS NU_USB_SUBSYS_Release_Id (NU_USB_SUBSYS * cb,
                                 UINT32 object_id);
STATUS NU_USB_SUBSYS_Is_Valid_Object (NU_USB_SUBSYS * cb,
                                      NU_USB * object,
                                      DATA_ELEMENT * reply);
STATUS NU_USB_SUBSYS_Set_Signature (NU_USB_SUBSYS * cb,
                                    UINT16 signature);

/* Virtual Functions for Locking and UnLocking. Will be overridden in the
 * event of locking requirement
 * */

STATUS NU_USB_SUBSYS_Lock (NU_USB_SUBSYS * cb);
STATUS NU_USB_SUBSYS_Unlock (NU_USB_SUBSYS * cb);

#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

/* ==================================================================== */

#endif /* _NU_USB_SUBSYS_EXT_H_ */

/* ======================  End Of File  =============================== */

