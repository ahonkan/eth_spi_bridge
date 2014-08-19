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
 *      nu_usb_mutex_imp.h
 * 
 * COMPONENT 
 *      OS services : Mutex 
 * 
 * DESCRIPTION 
 *      Mutex related data structure and functions are deprecated. 
 *
 *      This file contains data structures and function declaration for
 *      Mutex component.
 * 
 * DATA STRUCTURES 
 *      OS_Mutex    The mutex structure. 
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      None
 *
 *************************************************************************/

/* ==================================================================== */
#ifndef MUTEX_H
#define MUTEX_H
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ===================== Data Structures ============================== */
/* This data structure is deprecated */
typedef struct
{
    NU_SEMAPHORE sem1;                      /* To attempt to own this mutex */
    NU_SEMAPHORE sem2;                      /* The 'mutex' lock  */
    NU_TASK *task;                          /* Task that currently holding the mutex */
    UINT8 recursive_cnt;                    /* # of times the lock is grabbed by the task */
}
OS_Mutex;

/* ====================  Function Prototypes ========================== */
/* These functions are deprecated */
/* API prototypes */
STATUS usb_create_mutex (OS_Mutex * mutex, CHAR * name);
STATUS usb_obtain_mutex (OS_Mutex * mutex);
STATUS usb_release_mutex (OS_Mutex * mutex);
STATUS usb_destroy_mutex (OS_Mutex * mutex);

/* ==================================================================== */

#endif /* MUTEX_H */

/* ======================  End Of File  =============================== */

