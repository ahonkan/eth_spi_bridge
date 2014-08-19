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
*       negotiat.h                                     
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus Telnet
*
*   DESCRIPTION
*
*       Contains definitions for functions in negotiat.c.
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

#ifndef NEGOTIAT_H
#define NEGOTIAT_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* last element in nego_tables should always be NULL.  If the table(s)
   is/are empty, it/they should still contain END_OF_NEGO_TABLE. */
#define END_OF_NEGO_TABLE (INT) NULL


/* the codes between 240 and 255 are the defined telnet commands (see rfc854).
   The first defined telnet command has decimal code 240; it is TO_SE.  */
#define FIRST_DEFINED_TELNET_COMMAND 240

/* check if c is a telnet command.
   Telnet commands have decimal codes 240-255. */
#define is_telnet_command(c) \
                  (c>=FIRST_DEFINED_TELNET_COMMAND) ? NU_TRUE : NU_FALSE


#define ESC 27 /* escape */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* #ifndef NEGOTIAT_H */
