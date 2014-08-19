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

/* Portions of this program were written by: */
/*************************************************************************
*
*      part of:
*      TCP/IP kernel for NCSA Telnet
*      by Tim Krauskopf
*
*      National Center for Supercomputing Applications
*      152 Computing Applications Building
*      605 E. Springfield Ave.
*      Champaign, IL  61820
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME                                              
*
*       windat.h                                       
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus Telnet
*
*   DESCRIPTION
*
*       The date structure of the parameters for a telnet session
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

#ifndef WINDAT_H
#define WINDAT_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/*
*  the struct TN_SESSION_STRUCT contains all the parameter of a telnet session
*/

#define UNKNOWNTYPE -1

#define VTEKTYPE     1
#define DUMBTYPE     2
#define VTTYPE       3
#define TEKTYPE      4
#define RASTYPE      5

#define VT100TYPE    6
#define VT100STR     "VT100"
#define VT220TYPE    7
#define VT52TYPE     8

#define ANSITYPE     9
#define ANSISTR      "ANSI"

struct TN_SESSION_STRUCT {
    unsigned short i_am_client;     /* 1 for client, 0 for server          */
    int pnum,                       /* port number associated              */
        socket,                     /* the socket index associated with it */
        width,                      /* width of the window                 */
        height,                     /* number of rows in the window        */
        telstate,                   /* telnet state for this connection    */
        substate,                   /* telnet subnegotiation state         */
        termtype,                   /* terminal type for this connection   */
        CR_followed_by_LF,          /* what follows a CR? LF or NULL?      */
        nego_NAWS,                  /* the state of NAWS negotiation       */
        nego_TERMTYPE,              /* the state of TERMTYPE negotiation   */
        nego_SGA,                   /* the state of SGA negotiation        */
        nego_Echo,                  /* the state of ECHO negotiation       */
        nego_Binary,                /* the state of ECHO negotiation       */
        bksp,                       /* what keycode for backspace ?        */
        del;                        /* for delete?                         */
    struct TN_SESSION_STRUCT *next,*prev;
};

/* this structure should be used when applications needs to use parameters
   of the current telnet session.  Accessing internal TN_SESSION_STRUCT
   by customer's applications is not a good idea.  NU_TN_PARAMETERS
   should be used instead.
   See NU_Telnet_Get_Session_Parameters() which copies fields from internal
   TN_SESSION_STRUCT to publicly-available NU_TN_PARAMETERS. */
typedef struct {
    unsigned short nu_i_am_client;  /* 1 for client, 0 for server        */
    int nu_width,                   /* width of the window               */
        nu_height,                  /* number of rows in the window      */
        nu_termtype,                /* terminal type for this connection */
        nu_nego_naws,               /* the state of NAWS negotiation     */
        nu_nego_termtype,           /* the state of TERMTYPE negotiation */
        nu_nego_sga,                /* the state of SGA negotiation      */
        nu_nego_echo,               /* the state of ECHO negotiation     */
        nu_nego_binary;             /* the state of BINARY negotiation   */
} NU_TN_PARAMETERS;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif
