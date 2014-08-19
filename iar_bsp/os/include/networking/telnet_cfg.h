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
*       telnet_cfg.h                                   
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus Telnet
*
*   DESCRIPTION
*
*       This file contains all configurable parameters for Nucleus
*       Telnet.
*
*   DATA STRUCTURES
*
*       None
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

#ifndef TELNET_CFG_H
#define TELNET_CFG_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* size of telnet negotiation options buffer;
   used in NU_Telnet_Do_Negotiation and NU_Telnet_Start_Negotiate() */
#define TELNET_NEGO_OPTIONS_BUF_SIZE 30

/* size of buffer to read to from socket;
   used in NU_Telnet_Do_Negotiation and NU_Telnet_Start_Negotiate() */
#define TELNET_BUF_SIZE 201

/* how long we should sleep if there is no data on the socket,
   before trying to read again; used by NEG_Telnet_*_Negotiate functions */
#define TELNET_NEGO_SLEEP_TIME 10

/* coefficient by which to increase the sleep time;
   used by NEG_Telnet_*_Negotiate functions */
#define TELNET_NEGO_INCREASE_SLEEP_TIME_COEFF 2

/* used by NEG_Telnet_*_Negotiate functions, means that
   no bits of the current negotiation option are set up */
#define NO_NEGO_OPTION_INSTALLED 1

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* #ifndef TELNET_CFG_H */
