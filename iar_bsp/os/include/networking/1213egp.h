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
*       1213egp.h                                                
*
*   COMPONENT
*
*       MIB-II
*
*   DESCRIPTION
*
*       This file contains the function declarations for operations
*       on EGP objects.
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
#if (RFC1213_EGP_INCLUDE == NU_TRUE)

#ifndef _1213_EGP_H_
#define _1213_EGP_H_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

UINT16 egpInMsgs                (snmp_object_t *, UINT16, VOID *);
UINT16 egpInErrors              (snmp_object_t *, UINT16, VOID *);
UINT16 egpOutMsgs               (snmp_object_t *, UINT16, VOID *);
UINT16 egpOutErrors             (snmp_object_t *, UINT16, VOID *);
UINT16 egpNeighEntry            (snmp_object_t *, UINT16, VOID *);
UINT16 egpAs                    (snmp_object_t *, UINT16, VOID *);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif

#endif /* (RFC1213_EGP_INCLUDE == NU_TRUE) */



