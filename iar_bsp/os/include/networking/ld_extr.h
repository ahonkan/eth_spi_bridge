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

/****************************************************************************
*
*   FILE NAME
*
*       ld_extr.h
*
*   COMPONENT
*
*       LD - Loopback Device
*
*   DESCRIPTION
*
*       This file contains function prototypes for the loopback module.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
****************************************************************************/

#ifndef LD_EXTR_H
#define LD_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

STATUS LDC_Init (DV_DEVICE_ENTRY *);
STATUS LDC_TX_Packet (DV_DEVICE_ENTRY *, NET_BUFFER *);
STATUS LDC_Ioctl(DV_DEVICE_ENTRY *, INT, DV_REQ *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* LD_EXTR_H */
