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

/**************************************************************************
*
* FILENAME
*
*       vlan_defs.h
*
* DESCRIPTION
*
*       This include file will handle VLAN protocol defines.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
****************************************************************************/

#ifndef VLAN_DEFS_H
#define VLAN_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define     VLAN_TAG_SIZE       2
#define     VLAN_TYPE_SIZE      2
#define     VLAN_HEADER_SIZE    (VLAN_TAG_SIZE + VLAN_TYPE_SIZE)
#define     VLAN_TYPE_OFFSET    ETHER_TYPE_OFFSET
#define     VLAN_TAG_OFFSET     (VLAN_TYPE_OFFSET + 2)

#define     VLAN_TYPE           0x8100      /* Ethernet type for VLAN */

/* Canonical Format Indication : controls what bit is treated as the LSB */
/* on MAC Address bytes.  0x00 is the default for network byte order.    */

#define     VLAN_CFI            0x00
#define     VLAN_VID_CHECK      0x0FFF
#define     VLAN_PRIORITY_MASK  0x07

/* This value is only used with HW VLAN OFFLOADING which requires a fixed */
/* VLAN priority to be configured.  The SW VLAN METHOD extracts the user  */
/* priority from either the TOS or DSCP fields in the IP layer.           */

#define     VLAN_USER_PRIORITY  0x03


#define  ADD_VLAN_FILTER 0x01   /* used to add new VLAN filter entry */

#define  REPLACE_VLAN_FILTER 0x02 /* used to replace VLAN filter entry */

typedef struct VLAN_INIT_DATA_STUCT
{
   UINT16   vlan_vid;
   UINT8    padN[2];
   UINT8    *vlan_device;
} VLAN_INIT_DATA;


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* VLAN_DEFS_H */
