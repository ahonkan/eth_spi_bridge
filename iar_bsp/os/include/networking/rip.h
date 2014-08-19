/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
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
*       rip.h                                                    
*
*   DESCRIPTION
*
*       This file contains the declaration of the rip_t data structure
*       used by SNMP to process RMON requests.
*
*   DATA STRUCTURES
*
*       rip_s
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef RIP_H
#define RIP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/*
 * Receive Frame Information template
 * If the ri_stat field is non-zero, there is viable info there;
 */
typedef struct rip_s
{
    UINT16    rip_used;       /* This rip is being used if not zero    */
    UINT16    rip_stat;       /* Status as reported in the descriptor  */
    UINT16    rip_size;       /* Length of the packet                  */
    UINT16    rip_seq;        /* Frame sequence number                 */
    UINT16    rip_unit;       /* the unit this rip came from           */
    UINT16    rip_Status;     /* For RMON Capture group entry          */
    UINT16    rip_Len;        /* For RMON Capture group entry          */

    UINT8     snmp_pad[2];
    
    UINT32    rip_ID;         /* For RMON Capture group entry          */
    UINT32    rip_Time;       /* For RMON Capture group entry          */
    UINT8     *rip_pktp;      /* pointer to this packet for this RIP.  */
    UINT8     rip_buff[2048]; /* storage for the packet                */
} rip_t;

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
