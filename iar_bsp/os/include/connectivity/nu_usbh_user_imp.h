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
 *      nu_usbh_user_imp.h
 * 
 * COMPONENT 
 *      Nucleus USB Host Software 
 * 
 * DESCRIPTION 
 *
 *       This file contains the Control Block and other internal data 
 *       structures and definitions for USB Host User layer.
 *
 * 
 * DATA STRUCTURES 
 * 
 *      usbh_user_dispatch  User's dispatch table.
 *      usbh_user_session   This structure maintains information about each
 *                          device served by this instance of user.
 *      nu_usbh_user        USB Host User Control Block.
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      nu_usbh_ext.h      USB Host definitions. 
 * 
 *************************************************************************/

/* ===================================================================  */
#ifndef _NU_USBH_USER_IMP_H_
#define _NU_USBH_USER_IMP_H_
/* ===================================================================  */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usbh_ext.h"

/* =================== Data Structures ===============================  */
#define NU_USBH_USER_MAX_DEVICES    32

/* Each connection served by this user is defined in the following structure. */
typedef struct usbh_user_session
{
    CS_NODE     node;
    VOID        *handle;
    NU_USB_DRVR *drvr;
    UINT8 event_flag_index;  /* The event flag index assigned to this handle */

    /* These are used for synchronization */
    BOOLEAN trying_to_remove;  /* This is a state variable. True when some
                                * thread has called Remove_Device().
                                * subsequent open_device() request would then
                                * fail.
                                */
    INT8 reference_count;      /* Number of applications using this device */
    UINT32 num_tasks;          /* Number of entries in tasks list */
    NU_SEMAPHORE exclusive_lock;   /* Exclusive lock to use the device
                                    * this is used by safe removal process
                                    */
    NU_TASK *tasks_list[NU_USBH_USER_MAX_TASKS];
}
USBH_USER_SESSION;

/* The Control Block */
struct nu_usbh_user
{
    NU_USB_USER cb;
    NU_MEMORY_POOL *memory_pool;

    /* List of USBH_USER_SESSION's */
    USBH_USER_SESSION *session_list_head;

    /* above mentioned list's access lock */
    NU_SEMAPHORE list_lock; 

    /* Number of Session Entries */
    UINT8 num_sessions;
    UINT32 waiting_tasks;
    NU_EVENT_GROUP device_ready_event;
};

/* ================== Function Prototypes ============================  */
USBH_USER_SESSION *USBH_USER_Find_Session (NU_USBH_USER * cb,
                                           VOID *handle);
STATUS USBH_USER_Set_Device_Event (NU_USBH_USER * cb,
                                   UINT8 device_number);
STATUS USBH_Clear_Device_Event (NU_USBH_USER * cb,
                                UINT8 device_number);
UINT8 USBH_Assign_Index_User (NU_USBH_USER * cb);

/* ===================================================================  */

#endif /* _NU_USBH_USER_IMP_H_ */
/* ====================== end of file ================================  */

