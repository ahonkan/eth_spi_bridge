/*************************************************************************
*
*             Copyright 1995-2007 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME                                              
*
*       nvt.h                                          
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus Telnet
*
*   DESCRIPTION
*
*       Negotiation options tables for client and server
*
*   DATA STRUCTURES
*
*       client_nego_table   An array of even elements, the first element
*                           of each pair is the index of telnet negotiation
*                           command, the second is the telnet negotiation
*                           option.
*       server_nego_table   An array as the same as that of client.
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef NVT_H
#define NVT_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* the following MACROs are the index of telnet commands: DOTEL, DONTTEL,
  WILLTEL and WONTTEL,
  they are not the telnet commands directly
*/
#define DO          1
#define DONT        2
#define WILL        3
#define WONT        4
#define DONT_WILL   5
#define DONT_WONT   6
#define DO_WONT     7
#define DO_WILL     8
#define NOTHING     9

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NVT_H */
