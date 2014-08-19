/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/************************************************************************
*
*   FILE NAME
*
*       tcpdefs.h
*
*   COMPONENT
*
*       TCP - Transmission Control Protocol
*
*   DESCRIPTION
*
*       Holds the defines related to the TCP protocol.
*
*   DATA STRUCTURES
*
*       Global component data structures defined in this file
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*
*************************************************************************/

              /* the definitions for Nucleus - TCP/IP program */

#ifndef TCPDEFS_H
#define TCPDEFS_H

#include    "nucleus.h"
#include    "kernel/nu_kernel.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */


/************ RESOURCES ************/
extern NU_SEMAPHORE             TCP_Resource;

/************* EVENTS **************/
extern NU_QUEUE        eQueue;
extern NU_EVENT_GROUP  Buffers_Available;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* TCPDEFS.H */
