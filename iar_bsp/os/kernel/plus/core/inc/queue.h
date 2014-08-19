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
*       the message Queue component.
*
***********************************************************************/

/* Check to see if the file has been included already.  */

#ifndef QUEUE_H
#define QUEUE_H

#ifdef          __cplusplus

/* C declarations in C++     */
extern          "C" {

#endif

/* Define constants local to this component.  */

#define         QU_QUEUE_ID             0x51554555UL
#define         QU_COPY_SIZE(size)      ((size) * sizeof(UNSIGNED))

/* Define the queue suspension structure.  This structure is allocated off of
   the caller's stack.  */

typedef struct QU_SUSPEND_STRUCT
{
    CS_NODE             qu_suspend_link;       /* Link to suspend blocks */
    QU_QCB             *qu_queue;              /* Pointer to the queue   */
    TC_TCB             *qu_suspended_task;     /* Task suspended         */
    UNSIGNED_PTR        qu_message_area;       /* Pointer to message area*/
    UNSIGNED            qu_message_size;       /* Message size requested */
    UNSIGNED            qu_actual_size;        /* Actual size of message */
    STATUS              qu_return_status;      /* Return status          */
} QU_SUSPEND;
                                                
#ifdef          __cplusplus

/* End of C declarations */
}

#endif  /* __cplusplus */

#endif
