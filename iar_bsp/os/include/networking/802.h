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

/*************************************************************************
*
*   FILENAME
*
*       802.h
*
*   COMPONENT
*
*       MAC Layer
*
*   DESCRIPTION
*
*       This include file will handle function relating to mac layer.
*
*   DATA STRUCTURES
*
*       None.
*
*   DEPENDENCIES
*
*       None.
*
*************************************************************************/
#ifndef MAC_H
#define MAC_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* _cplusplus */

#define SPAN_CONFIG                 0x0026
#define SPAN_TOP_CHG                0x0007

STATUS EightZeroTwo_Input(DV_DEVICE_ENTRY *device, UINT16 protocol_type,
                          UINT8 *ether_pkt);
STATUS EightZeroTwo_Output(NET_BUFFER *, DV_DEVICE_ENTRY *, VOID *,
                           const VOID *, UINT8 *, UINT16 *type);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* #ifndef MAC_H */

