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

/*************************************************************************
*
* FILE NAME 
*
*        nu_usbh_user_imp.c
*
* COMPONENT
*
*       Nucleus USB Host Stack : User Layer
*
* DESCRIPTION
*
*       This file provides the implementation of USBH_User layer.
* This layer provides a generic implementation for Applications to
* wait for a particular device to be connected and maintain a reference 
* counting of applications using the device. It also provides service
* to safely remove a device which assures no application thread is using
* the device and thus guarantees data consistency.
*
*
* DATA STRUCTURES
*       None 
*
* FUNCTIONS
*
*       USBH_USER_Find_Session              -Finds a session corresponding
*                                           to a device handle.
* 
* DEPENDENCIES 
*
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USBH_USER_IMP_C
#define USBH_USER_IMP_C

/* ==============  Standard Include Files ============================  */
#include "connectivity/nu_usb.h"

/***********************************************************************/

/* Local Functions Declarations */

/*************************************************************************
* FUNCTION 
*        USBH_USER_Find_Session
*
* DESCRIPTION
*       This function finds session corresponding to device handle
* 
* INPUTS 
*       cb          pointer to user control block.
*       handle      handle for which session is to be found.
*
* OUTPUTS
*       USBH_USER_SESSION*      found Session pointer.
*       NU_NULL                 Indicates not found.
*
*************************************************************************/
USBH_USER_SESSION *USBH_USER_Find_Session (NU_USBH_USER * cb,
                                           VOID *handle)
{
    USBH_USER_SESSION *next;

    if(cb == NU_NULL
       ||handle == NU_NULL)
    {
        return NU_NULL;
    }

    next = cb->session_list_head;

    /* Traverse the list of sessions */
    while (next)
    {
        if (next->handle == handle)
            /* Found !!! */
            return next;

        next = (USBH_USER_SESSION *) (((NU_USB *) next)->list_node.cs_next);
        if (next == cb->session_list_head)
            next = NU_NULL;
    }

    return NU_NULL;             /* failure...not found... NU_NULL; */
}

/************************************************************************/

#endif /* USBH_USER_IMP_C */
/* ======================  End Of File  =============================== */
