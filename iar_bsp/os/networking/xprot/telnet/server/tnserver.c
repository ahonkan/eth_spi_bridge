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
*     part of:
*     TCP/IP kernel for NCSA Telnet
*     by Quincey Koziol
*
*     National Center for Supercomputing Applications
*     152 Computing Applications Building
*     605 E. Springfield Ave.
*     Champaign, IL  61820
*
*************************************************************************/
/*************************************************************************
*
*   FILENAME                                               
*
*       tnserver.c                                     
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus Telnet Server
*
*   DESCRIPTION
*
*       Telnet negotiation utilities - Server side functions
*
*       In this file, there are two points. The users may need modify them to
*       match their specific situation:
*       1.  Negotiation - here we only follow the NCSA template
*       2.  Check and explanation of special terminal-dependent keys
*           '[' and 'O' are the second byte of vt100 key code, however,
*           different terminals must have different mapping and conventions.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NEG_Srv_Handle_IAC_WILL_NAWS()
*       NEG_Srv_Handle_IAC_WONT_NAWS()
*       NEG_Request_Client_Send_TermType()
*       NEG_Srv_Handle_IAC_WILL_TERMTYPE()
*       NEG_Srv_Handle_IAC_WONT_TERMTYPE()
*
*   DEPENDENCIES
*
*       nucleus.h
*       externs.h
*       telopts.h
*       windat.h
*       nvt.h
*       tel_extr.h
*       negotiat.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/externs.h"
#include "networking/telopts.h"

#include "networking/windat.h"

#include "networking/nvt.h"
#include "networking/tel_extr.h"
#include "networking/negotiat.h"

/* defined in TL_UTIL.C */
extern struct TN_SESSION_STRUCT *TN_Session[NSOCKETS];
extern NU_SEMAPHORE TN_Session_Semaphore;
extern STATUS TL_Telnet_Init_Parameters(INT socket);

/*
 the negotiation tables are designed as a basic condition;  if they do not
 match the requirements of the user, the user needs to change them.
 However, the user, who changes the table entries, must follow the way the
 tables are organized now, the rules are:
    1. the array must have even number of commands and options, and
       must always end with END_OF_NEGO_TABLE (which makes the total
       number of elements in the array odd),
    2. the first element of each pair is the MACRO of the index of telnet
       negotiation command, the second is the telnet negotiation option;
    3. usually, the users may only change the command index MACRO, not the
       option,
       BE CAREFUL, the command index MACROs defined in this file are not
       commands, different from the telnet commands defined in telopts.h,
       these index MACROs are used to help NEG_Set_One_Option() (negotiat.c)
       to set correct telnet command for each option;
    4. if the user does not need an option, just put NOTHING before the option;
    5. it is not recommended to use WILL, DONT_WILL, or DO_WILL TO_ECHO
       in the client_nego_table.  Any WILL TO_ECHO command issued by the client
       may result in situation where both hosts enter the mode of echoing
       characters transmitted by the other host.  In this situation, any
       character transmitted in either direction will be "echoed" back and
       forth indefinitely (see http://www.faqs.org/rfcs/rfc857.html).
*/

CHAR server_nego_table[] = {
/*  the command index,  the option  */
    DONT_WONT,          TO_BINARY,
    DONT_WILL,          TO_ECHO,      /* this side support ECHO */
    DO_WILL,            TO_SGA,       /* this side suppress Go Ahead */
    DO,                 TO_TERMTYPE,  /* require other side to tell terminal type */
    DO,                 TO_NAWS,      /* require other side to tell window size */
    END_OF_NEGO_TABLE                 /* NOTE: nego_table should always end with NULL,
                                         or NEG_Size_Of_Nego_Table() will no work right. */
};

/************************************************************************
*
*   FUNCTION
*
*       NEG_Srv_Handle_IAC_WILL_NAWS
*
*   DESCRIPTION
*
*       Internal server-side function called by NU_Telnet_Parse() to
*       process the "IAC WILL NAWS" command received from the telnet client.
*       nego_NAWS bits set by this function are checked by
*       NEG_Telnet_NAWS_Negotiate() to see if NAWS is over.
*
*       This function is a part of the Telnet Window Size Option
*       implementation (see http://www.faqs.org/rfcs/rfc1073.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         -1         -- the function was called by telnet client.
*         NU_SUCCESS -- otherwise.
*
************************************************************************/
STATUS NEG_Srv_Handle_IAC_WILL_NAWS(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    /* this function can only be called by server.  Make sure we are. */
    if (tw->i_am_client == NU_TRUE)
    {
        return (-1);
    }

    if (tw->nego_NAWS & NEGO_I_DO)
    {
        /* NEGO_I_DO is on, it means that we (server) sent the
           "IAC DO NAWS" before and the client replied "IAC WILL NAWS".
           So, the client agrees to NAWS.  Don't do anything -- wait when
           client sends the subnegotiation parameters (window size). */
        ;
    }
    else
    {
        /* The client sent "IAC WILL NAWS".  But NEGO_I_DO flag
           on the server side is off, it means server is not interested
           in knowing the client's window size ... Send "IAC DONT NAWS" if
           we didn't send it before. */

        tw->nego_NAWS |= NEGO_HE_WILL;

        if (!(tw->nego_NAWS & NEGO_I_DONT))
        {
            temp_buf[0] = (CHAR) TO_IAC;
            temp_buf[1] = (CHAR) TO_DONTTEL;
            temp_buf[2] = (CHAR) TO_NAWS;

            NU_Send(socket, temp_buf, 3, 0);

            tw->nego_NAWS |= NEGO_I_DONT;
        }
    }

    return NU_SUCCESS;
} /* NEG_Srv_Handle_IAC_WILL_NAWS */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Srv_Handle_IAC_WONT_NAWS
*
*   DESCRIPTION
*
*       Internal server-side function called by NU_Telnet_Parse() to
*       process the "IAC WONT NAWS" command received from the telnet client.
*       nego_NAWS bits set by this function are checked by
*       NEG_Telnet_NAWS_Negotiate() to see if NAWS is over.
*
*       This function is a part of the Telnet Window Size Option
*       implementation (see http://www.faqs.org/rfcs/rfc1073.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         -1         -- the function was called by telnet client.
*         NU_SUCCESS -- otherwise.
*
************************************************************************/
STATUS NEG_Srv_Handle_IAC_WONT_NAWS(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    /* this function can only be called by server.  Make sure we are. */
    if (tw->i_am_client == NU_TRUE)
    {
        return (-1);
    }

    if (tw->nego_NAWS & NEGO_I_DO)
    {
        /* NEGO_I_DO is on, it means that we (server) sent the
           "IAC DO NAWS" before and the client replied "IAC WONT NAWS".
           So, the client does not agree to NAWS.  Set NEGO_HE_WONT flag.
           Don't send anything back to the client. */

        tw->nego_NAWS |= NEGO_HE_WONT;
    }
    else if (tw->nego_NAWS & NEGO_I_DONT)
    {
        /* we (server) sent the "IAC DONT NAWS" before and the client
           replied "IAC WONT NAWS".  So, the client agrees not to NAWS.
           Set NEGO_HE_WONT flag.  Don't send anything back to the client. */

        tw->nego_NAWS |= NEGO_HE_WONT;
    }
    else if ( !(tw->nego_NAWS & NEGO_I_DO) && !(tw->nego_NAWS & NEGO_I_DONT) )
    {
        /* We (server) sent neither "IAC DO NAWS", nor "IAC DONT NAWS" before,
           but received "IAC WONT NAWS" from the client.  This means the client
           does not want to NAWS.  Send "IAC DONT NAWS". */

        temp_buf[0] = (CHAR) TO_IAC;
        temp_buf[1] = (CHAR) TO_DONTTEL;
        temp_buf[2] = (CHAR) TO_NAWS;

        NU_Send(socket, temp_buf, 3, 0);

        tw->nego_NAWS |= NEGO_HE_WONT;
        tw->nego_NAWS |= NEGO_I_DONT;
    }

    return NU_SUCCESS;
} /* NEG_Srv_Handle_IAC_WONT_NAWS */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Request_Client_Send_TermType
*
*   DESCRIPTION
*
*       Internal server-side function called by
*       NEG_Srv_Handle_IAC_WILL_TERMTYPE() after the server sent
*       "IAC DO TERM-TYPE" and received "IAC WILL TERM-TYPE".
*       This function requests that the telnet client send the info
*       about the terminal type the client supports.
*
*       Note, that the server might request additional terminal types
*       that the client supports by calling this function more than
*       once (i.e., by sending the "IAC SB TERMINAL-TYPE SEND IAC SE"
*       command more than once).  When the server receives the same response
*       from the client two consecutive times, this indicates the end of the
*       list of available types.
*       The transmission of terminal type information by the telnet client
*       in response to a query from the telnet server implies that the client
*       simultaneously changes emulations mode, unless the terminal type
*       sent is a synonym of the preceding terminal type.
*
*       This function is a part of the Telnet Terminal-Type Option
*       implementation (see http://www.faqs.org/rfcs/rfc1091.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         -1         -- the function was called by telnet client.
*         NU_SUCCESS -- otherwise.
*
************************************************************************/
STATUS NEG_Request_Client_Send_TermType(INT socket)
{
    struct TN_SESSION_STRUCT *tw;
    CHAR temp_buf[6];

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->i_am_client == NU_TRUE)
    {
        /* this function can only be called by telnet server ... */
        return (-1);
    }

    temp_buf[0] = (CHAR) TO_IAC;
    temp_buf[1] = (CHAR) TO_SB;
    temp_buf[2] = (CHAR) TO_TERMTYPE;
    temp_buf[3] = (CHAR) TO_SEND;
    temp_buf[4] = (CHAR) TO_IAC;
    temp_buf[5] = (CHAR) TO_SE;

    NU_Send(socket, temp_buf, 6, 0);

    return NU_SUCCESS;
} /* NEG_Request_Client_Send_TermType */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Srv_Handle_IAC_WILL_TERMTYPE
*
*   DESCRIPTION
*
*       Internal server-side function called by NU_Telnet_Parse() to
*       process the "IAC WILL TERMINAL-TYPE" command received from
*       the telnet client.  nego_TERMTYPE bits set by this function are
*       checked by NEG_Telnet_TermType_Negotiate() to see if
*       negotiation about the terminal type is over.
*
*       This function is a part of the Telnet Terminal-Type Option
*       implementation (see http://www.faqs.org/rfcs/rfc1091.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         -1         -- the function was called by telnet client.
*         NU_SUCCESS -- otherwise.
*
************************************************************************/
STATUS NEG_Srv_Handle_IAC_WILL_TERMTYPE(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    /* this function can only be called by server.  Make sure we are. */
    if (tw->i_am_client == NU_TRUE)
    {
        return (-1);
    }

    if (tw->nego_TERMTYPE & NEGO_I_DO)
    {
        if ( (!(tw->nego_TERMTYPE & NEGO_HE_WILL)) &&
             (!(tw->nego_TERMTYPE & NEGO_I_REQUIRED_SUBNEG)) )
        {

            /* NEGO_I_DO is on, it means that we (server) sent the
               "IAC DO TERMINAL-TYPE" before and the client replied
               "IAC WILL TERMINAL-TYPE".  So, the client agrees to
               negotiate about TERMINAL-TYPE.  Ask the client
               to send the terminal type it supports.  Set NEGO_HE_WILL flag to
               make sure we ask the client about his terminal type only once.*/

            tw->nego_TERMTYPE |= NEGO_HE_WILL;
            tw->nego_TERMTYPE |= NEGO_I_REQUIRED_SUBNEG;
            return NEG_Request_Client_Send_TermType(socket);
        }
    }
    else
    {
        /* The client sent "IAC WILL TERMINAL-TYPE".  But NEGO_I_DO flag
           on the server side is off, it means server is not interested
           in knowing the client's terminal type ...
           Send "IAC DONT TERMINAL-TYPE" if we (server) didn't send it before. */

        tw->nego_TERMTYPE |= NEGO_HE_WILL;

        if (!(tw->nego_TERMTYPE & NEGO_I_DONT))
        {
            temp_buf[0] = (CHAR) TO_IAC;
            temp_buf[1] = (CHAR) TO_DONTTEL;
            temp_buf[2] = (CHAR) TO_TERMTYPE;

            NU_Send(socket, temp_buf, 3, 0);

            tw->nego_TERMTYPE |= NEGO_I_DONT;
        }
    }

    return NU_SUCCESS;
} /* NEG_Srv_Handle_IAC_WILL_TERMTYPE */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Srv_Handle_IAC_WONT_TERMTYPE
*
*   DESCRIPTION
*
*       Internal server-side function called by NU_Telnet_Parse() to
*       process the "IAC WONT TERMINAL-TYPE" command received from
*       the telnet client.  nego_TERMTYPE bits set by this function are
*       checked by NEG_Telnet_TermType_Negotiate() to see if
*       negotiation about the terminal type is over.
*
*       This function is a part of the Telnet Terminal-Type Option
*       implementation (see http://www.faqs.org/rfcs/rfc1091.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         -1         -- the function was called by telnet client.
*         NU_SUCCESS -- otherwise.
*
************************************************************************/
STATUS NEG_Srv_Handle_IAC_WONT_TERMTYPE(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    /* this function can only be called by server.  Make sure we are. */
    if (tw->i_am_client == NU_TRUE)
    {
        return (-1);
    }

    if (tw->nego_TERMTYPE & NEGO_I_DO)
    {
        /* NEGO_I_DO is on, it means that we (server) sent the
           "IAC DO TERMINAL-TYPE" before and the client replied
           "IAC WONT TERMINAL-TYPE".  So, the client does not
           agree to negotiate about termtype.  Set NEGO_HE_WONT flag.
           Don't send anything back to the client. */

        tw->nego_TERMTYPE |= NEGO_HE_WONT;
    }
    else if (tw->nego_TERMTYPE & NEGO_I_DONT)
    {
        /* we (server) sent the "IAC DONT TERMINAL-TYPE" before
           and the client replied "IAC WONT TERMINAL-TYPE".  So,
           the client agrees not to negotiate about termtype.
           Set NEGO_HE_WONT flag.
           Don't send anything back to the client. */

        tw->nego_TERMTYPE |= NEGO_HE_WONT;
    }
    else if ( !(tw->nego_TERMTYPE & NEGO_I_DO) &&
              !(tw->nego_TERMTYPE & NEGO_I_DONT) )
    {
        /* We (server) sent neither "IAC DO TERMINAL-TYPE", nor
           "IAC DONT TERMINAL-TYPE" before, but received
           "IAC WONT TERMINAL-TYPE" from the client.  This means
           the client does not want to negotiate about termtype.
           Send "IAC DONT TERMINAL-TYPE". */

        temp_buf[0] = (CHAR) TO_IAC;
        temp_buf[1] = (CHAR) TO_DONTTEL;
        temp_buf[2] = (CHAR) TO_TERMTYPE;

        NU_Send(socket, temp_buf, 3, 0);

        tw->nego_TERMTYPE |= NEGO_HE_WONT;
        tw->nego_TERMTYPE |= NEGO_I_DONT;
    }

    return NU_SUCCESS;
} /* NEG_Srv_Handle_IAC_WONT_TERMTYPE */


/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Server_Init_Parameters
*
*   DESCRIPTION
*
*       Initialize parameters of a server telnet session.
*
*   INPUTS
*
*       server_socket  The index of server socket of telnet connection.
*
*   RETURNS
*
*       NU_SUCCESS,
*
*       or one of the following NU_Allocate_Memory() failure codes:
*
*       NU_INVALID_POOL
*       NU_INVALID_POINTER
*       NU_INVALID_SIZE
*       NU_INVALID_SUSPEND
*       NU_NO_MEMORY
*       NU_TIMEOUT
*       NU_POOL_DELETED
*
************************************************************************/
STATUS NU_Telnet_Server_Init_Parameters(INT server_socket)
{
    struct TN_SESSION_STRUCT *server_session;
    STATUS status;
    NU_SUPERV_USER_VARIABLES

    /* Check for invalid input. */
    if ( (server_socket < 0) || (server_socket >= NSOCKETS) )
        return (NU_INVALID_SOCKET);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    status = TL_Telnet_Init_Parameters(server_socket);

    if (status == NU_SUCCESS)
    {
        NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);
        server_session = TN_Session[server_socket];
        server_session->i_am_client = (unsigned short)NU_FALSE;
        NU_Release_Semaphore(&TN_Session_Semaphore);
    }

    NU_USER_MODE();    /* return to user mode */

    return status;
} /* NU_Telnet_Server_Init_Parameters */


/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Server_Accept
*
*   DESCRIPTION
*
*       Waiting for a telnet connection from client.
*
*   INPUTS
*
*       socket      The first template socket for this server
*
*   RETURNS
*
*       Socket descriptor for the connection if successful, otherwise -1.
*
************************************************************************/
INT NU_Telnet_Server_Accept(INT socket)
{
    INT                 newsock;    /* the new socket descriptor */
    struct addr_struct  client_addr;
    NU_SUPERV_USER_VARIABLES

    /* Check for invalid socket. */
    if ( (socket < 0) || (socket > NSOCKETS) )
        return (-1);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* block in NU_Accept until a client attempts connection */
    newsock = (NU_Accept(socket, &client_addr, 0));
    if (newsock >= 0)
    {
        NU_USER_MODE();    /* return to user mode */
        return(newsock);
    } /* end successful NU_Accept */

    NU_USER_MODE();    /* return to user mode */
    return(-1);
} /* NU_Telnet_Server_Accept */
