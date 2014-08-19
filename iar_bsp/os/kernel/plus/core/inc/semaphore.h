/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file contains data structure definitions and constants for
*       the Semaphore component.
*
***********************************************************************/

/* Check to see if the file has been included already.  */

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#ifdef          __cplusplus

/* C declarations in C++     */
extern          "C" {

#endif

/* Define constants local to this component.  */

#define         SM_SEMAPHORE_ID         0x53454d41UL

/* Define the semaphore suspension structure.  This structure is allocated
   off of the caller's stack.  */

typedef struct SM_SUSPEND_STRUCT
{
    CS_NODE             sm_suspend_link;       /* Link to suspend blocks */
    SM_SCB             *sm_semaphore;          /* Pointer to semaphore   */
    TC_TCB             *sm_suspended_task;     /* Task suspended         */
    STATUS              sm_return_status;      /* Return status          */
} SM_SUSPEND;

VOID    SMC_Kill_Semaphore_Owner(NU_SEMAPHORE *semaphore_ptr, NU_TASK *owning_task);

#ifdef          __cplusplus

/* End of C declarations */
}

#endif  /* __cplusplus */

#endif
