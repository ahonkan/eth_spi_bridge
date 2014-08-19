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

/***********************************************************************
 *
 * FILE NAME 
 *
 *      nu_usb_rwl_imp.h
 *
 * COMPONENT
 *      OS services : Read/Write Lock
 *
 * DESCRIPTION 
 *      This file contains functions the control block definition of 
 *      Read/Write lock.
 *
 *
 * DATA STRUCTURES
 *      rw_lock_t       Control block of Read/Write lock.
 *
 * FUNCTIONS
 *      None
 *
 * DEPENDENCIES 
 *      None
 *
 ************************************************************************/
/* ==================================================================== */
#ifndef RWL_H
#define RWL_H
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ===================== #defines ===================================== */

#define RW_MAX_READERS  7
#define READ_MODE       1
#define WRITE_MODE      2

/* ===================== Data Structures ============================== */
typedef struct
{
    /* Reader/Writer lock */
    NU_SEMAPHORE access_lock;               /* binary semaphore  */
    NU_SEMAPHORE database_lock;             /* binary semaphore  */
    NU_SEMAPHORE num_clients;               /* Counting Semaphore of count RW_MAX_READERS */
    NU_TASK *tasks[RW_MAX_READERS + 1];     /* Reader/Writer task ptrs */
    /* No. of times the lock is currently held by task. */
    UINT8 recursive_cnt[RW_MAX_READERS + 1];
}
rw_lock_t;

/* ===================== Function Prototypes ========================== */

/* API Prototypes */
STATUS os_create_rw_lock (rw_lock_t * rw);
STATUS os_destroy_rw_lock (rw_lock_t * rw);
STATUS os_grab_rw_lock (rw_lock_t * rw,
                        UINT8 mode);
STATUS os_release_rw_lock (rw_lock_t * rw,
                           UINT8 mode);
STATUS os_upgrade_rw_lock (rw_lock_t * rw);
STATUS os_downgrade_rw_lock (rw_lock_t * rw);

#ifndef NU_ASSERT
#define NU_ASSERT(A) if (!(A)) while (1);
#endif

#define NU_USB_DEBUG_ENABLE NU_TRUE

#if (NU_USB_DEBUG_ENABLE == NU_TRUE)
#define NU_USB_ASSERT(A) if (!(A)) while (1);
#else
#define NU_USB_ASSERT(A) ((VOID)0)
#endif
/* ==================================================================== */
#endif /* RWL_H */

/* ======================  End Of File  =============================== */

