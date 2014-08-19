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
*       snmp_udp.h                                                    
*
*   DESCRIPTION
*
*       This file contains function declarations, data structures and
*       macros specific to the UDP transmission of SNMP packets.
*
*   DATA STRUCTURES
*
*       udp_stat_t
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef _SNMP_UDP_H_
#define _SNMP_UDP_H_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

typedef struct udp_stat_s   udp_stat_t;

struct udp_stat_s
{
    UINT32    noPorts;
    UINT32    inDatagrams;
    UINT32    inErrors;
    UINT32    outDatagrams;
    UINT32    outErrors;
};

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* _SNMP_UDP_H */

