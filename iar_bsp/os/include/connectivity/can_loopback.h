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
*       can_loopback.h
*
* COMPONENT
*
*       CAN Loopback - Loopback device for Nucleus CAN
*
* DESCRIPTION
*
*       This file contains the constant definitions and structure
*       declarations for implementing the support of a loopback device
*       in Nucleus CAN.
*
* DATA STRUCTURES
*
*       CAN_LOOPBACK_HWMB                   Data structure for simulating
*                                           a message buffer in loopback
*                                           device for Nucleus CAN.
*
* DEPENDENCIES
*
*       can.h                               Main definition and API file
*                                           for Nucleus CAN.
*
*************************************************************************/
/* Check to see if the file has already been included. */
#ifndef     CAN_LOOPBACK_H
#define     CAN_LOOPBACK_H

#include    "can.h"

#if         (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK)

/* Define indicating the sleep/wakeup mode for loopback device. */
#define     CAN_LOOPBACK_IN_SLEEP_MODE  1
#define     CAN_LOOPBACK_IS_AWAKE       0

/* Macro to check the sleep/wakeup mode of the loopback device. */
#define     CAN_LOOPBACK_GET_MODE()     CAN_Sleeping

/* Number of loopback hardware message buffers. */
#define     CAN_LOOPBACK_HWMB_COUNT     5

/* Structure for simulating hardware message buffer in loopback device
   for Nucleus CAN. */
typedef struct CAN_LOOPBACK_HWMB_STRUCT
{
    CAN_PACKET      can_loopback_packet;    /* CAN loopback packet.       */
    UINT16          can_loopback_flags;     /* Message buffer usage flags.*/
    UINT16          can_loopback_ints;      /* Message buffer interrupts. */

} CAN_LOOPBACK_HWMB;

/* Possible values of flags for message buffers. */

#define     CAN_LOOPBACK_HWMB_IN_USE    0x1 /* Message buffer is in use. */
#define     CAN_LOOPBACK_HWMB_RTR       0x2 /* Message buffer is assigned
                                               for RTR response.         */

/* Possible values for operation to perform on a message buffer
   interrupt. */

#define     CAN_LOOPBACK_DATA_TX_OK_INT 0x0001
#define     CAN_LOOPBACK_RTR_TX_OK_INT  0x0002
#define     CAN_LOOPBACK_DATA_RX_INT    0x0004
#define     CAN_LOOPBACK_RTR_RX_INT     0x0008
#define     CAN_LOOPBACK_TX_ERR_INT     0x0010
#define     CAN_LOOPBACK_RX_ERR_INT     0x0020

/* Macros to set/clear/check the interrupt of a message buffer. */

#define     CAN_LOOPBACK_SET_INT(mb_no)     \
            (CAN_Loopback_IF_Reg |= (UINT16)(1U << mb_no))

#define     CAN_LOOPBACK_CLEAR_INT(mb_no)   \
            (CAN_Loopback_IF_Reg &= ((UINT16)~(UINT16)(1U << mb_no)))

#define     CAN_LOOPBACK_CHECK_INT(mb_no)   \
            (CAN_Loopback_IF_Reg & (UINT16)(1U << mb_no))

#endif      /* (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK) */

#endif      /* !CAN_LOOPBACK_H */

