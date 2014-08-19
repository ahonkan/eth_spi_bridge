/****************************************************************************
*
*                  Copyright 2002 Mentor Graphics Corporation
*                             All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
****************************************************************************/

/**************************************************************************
*
* FILE NAME
*
*       iso11898_extr.h
*
* COMPONENT
*
*       ISO11898 - Nucleus CAN implementation for ISO11898 core
*
* DESCRIPTION
*
*       This file contains the functions prototypes for the functions
*       implementing core service of ISO 11898-1 protocol.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       can.h                               Main definition and API file
*                                           for Nucleus CAN
*
*************************************************************************/
/* Check to see if the file has already been included. */
#ifndef     ISO11898_EXTR_H
#define     ISO11898_EXTR_H

#include    "can.h"

/* Function prototypes for CAN data message transmission service. */
STATUS  ISO11898_Data_Request    (CAN_PACKET *can_msg);

#if         (NU_CAN_SUPPORTS_RTR)

/* Function prototype for CAN RTR message transmission service. */
STATUS  ISO11898_Remote_Request  (CAN_PACKET *can_msg);

#else

/* Macros to inform that the RTR support is disabled. */
#define     ISO11898_Remote_Request(can_msg)     CAN_SERVICE_NOT_SUPPORTED

#endif      /* NU_CAN_SUPPORTS_RTR */

#if         NU_CAN_AUTOMATIC_RTR_RESPONSE

/* Function prototypes for functions that will assign response for
   responding automatically to incoming message IDs. */
STATUS  ISO11898_Assign_Remote   (CAN_PACKET *can_msg);
STATUS  ISO11898_Clear_Remote    (CAN_PACKET *can_msg);

#else

/* Macros to inform that the services are not supported when automatic
   RTR response is not set. */
#define     ISO11898_Assign_Remote(can_msg)     CAN_SERVICE_NOT_SUPPORTED
#define     ISO11898_Clear_Remote (can_msg)     CAN_SERVICE_NOT_SUPPORTED

#endif      /* NU_CAN_AUTOMATIC_RTR_RESPONSE */

#if         (NU_CAN_DEBUG)

STATUS  ISO11898_Check_Parameters(CAN_PACKET *can_msg);

#else

/* Macro to return NU_SUCCESS in case parameter checking is not enabled. */
#define     ISO11898_Check_Parameters(can_msg)      NU_SUCCESS

#endif      /* NU_CAN_DEBUG */


#endif      /* ISO11898_EXTR_H */

