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
*       1213tcp.h                                                
*
* COMPONENT
*
*       MIB-II
*
* DESCRIPTION
*
*       This file contains the function declarations for operations
*       on TCP objects.
*
* DATA STRUCTURES
*
*       None
*
*  DEPENDENCIES
*
*       None
*
*************************************************************************/
#if (RFC1213_TCP_INCLUDE == NU_TRUE)

#ifndef _1213_TCP_H_
#define _1213_TCP_H_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

UINT16 tcpRtoAlgorithm        (snmp_object_t *, UINT16, VOID *);
UINT16 tcpRtoMin              (snmp_object_t *, UINT16, VOID *);
UINT16 tcpRtoMax              (snmp_object_t *, UINT16, VOID *);
UINT16 tcpMaxConn             (snmp_object_t *, UINT16, VOID *);
UINT16 tcpActiveOpens         (snmp_object_t *, UINT16, VOID *);
UINT16 tcpPassiveOpens        (snmp_object_t *, UINT16, VOID *);
UINT16 tcpAttemptFails        (snmp_object_t *, UINT16, VOID *);
UINT16 tcpEstabResets         (snmp_object_t *, UINT16, VOID *);
UINT16 tcpCurrEstab           (snmp_object_t *, UINT16, VOID *);
UINT16 tcpInSegs              (snmp_object_t *, UINT16, VOID *);
UINT16 tcpOutSegs             (snmp_object_t *, UINT16, VOID *);
UINT16 tcpRetransSegs         (snmp_object_t *, UINT16, VOID *);
UINT16 tcpConnEntry           (snmp_object_t *, UINT16, VOID *);
UINT16 tcpInErrs              (snmp_object_t *, UINT16, VOID *);
UINT16 tcpOutRsts             (snmp_object_t *, UINT16, VOID *);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
#endif /* (RFC1213_TCP_INCLUDE == NU_TRUE) */


