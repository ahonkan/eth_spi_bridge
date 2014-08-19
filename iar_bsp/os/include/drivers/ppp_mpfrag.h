/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       ppp_mpfrag.h
*
*   COMPONENT
*
*       MPFRAG - PPP Multilink Protocol Fragmentation Services
*
*   DESCRIPTION
*
*       This component is responsible for providing fragmentation
*       services.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef PPP_MPFRAG_H
#define PPP_MPFRAG_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

/* Defines for sequence number i.e. short sequence and long
   sequence numbers. */
#define     MP_SHORT_MAX_SEQ      0x0fff
#define     MP_LONG_MAX_SEQ       0x00ffffff

STATUS      PPP_MP_Fragment_Send(NET_BUFFER *, DV_DEVICE_ENTRY *);
NET_BUFFER *PPP_MP_New_Fragment (VOID);
VOID        PPP_MP_Make_Packet (NET_BUFFER **, UINT8 );
VOID        PPP_MP_Delete_Fragments (PPP_MP_BUNDLE *, NET_BUFFER *);

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_MPFRAG_H */
