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
*       the message Pipe component.
*
***********************************************************************/

/* Check to see if the file has been included already.  */

#ifndef PIPE_H
#define PIPE_H

#ifdef          __cplusplus

/* C declarations in C++     */
extern          "C" {

#endif

/* Define constants local to this component.  */

#define         PI_PIPE_ID              0x50495045UL

/* Define the Pipe suspension structure.  This structure is allocated off of
   the caller's stack.  */

typedef struct PI_SUSPEND_STRUCT
{
    CS_NODE             pi_suspend_link;       /* Link to suspend blocks */
    PI_PCB             *pi_pipe;               /* Pointer to the pipe    */
    TC_TCB             *pi_suspended_task;     /* Task suspended         */
    BYTE_PTR            pi_message_area;       /* Pointer to message area*/
    UNSIGNED            pi_message_size;       /* Message size requested */
    UNSIGNED            pi_actual_size;        /* Actual size of message */
    STATUS              pi_return_status;      /* Return status          */
} PI_SUSPEND;
                                                
#ifdef          __cplusplus

/* End of C declarations */
}

#endif  /* __cplusplus */

#endif
