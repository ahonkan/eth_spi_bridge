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
*       tnclient.c                                     
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus Telnet Client
*
*   DESCRIPTION
*
*       Telnet negotiation utilities - Client side functions
*
*       In this file, there are two points. The users may need modify them to
*       match their specific situation:
*       1.  Negotiation - here we only follow the NCSA template
*       2.  Check and explanation of special terminal-dependent keys
*           '[' and 'O' are the second byte of vt100 key code, however, different
*           terminals must have different mapping and conventions.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NEG_Cln_Handle_IAC_DO_NAWS()
*       NEG_Cln_Handle_IAC_DONT_NAWS()
*       NEG_Cln_Handle_IAC_DO_TERMTYPE()
*       NEG_Cln_Handle_IAC_DONT_TERMTYPE()
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
extern INT TN_SEMAPHORE_INITIALIZED;
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

CHAR client_nego_table[] = {
/*  the command index,  the option  */
    DONT_WONT,          TO_BINARY,    /* not binary */
    DO_WONT,            TO_ECHO,      /* Avoid using WILL/DO_WILL/DONT_WILL TO_ECHO */
    DO_WILL,            TO_SGA,       /* require other side suppress Go Ahead */
    WILL,               TO_NAWS,      /* this side is willing to tell window size */
    WILL,               TO_TERMTYPE,  /* this side is willing to tell terminal type */
    END_OF_NEGO_TABLE                 /* NOTE: nego_table should always end with NULL,
                                         or NEG_Size_Of_Nego_Table() will no work right. */
};


/************************************************************************
*
*   FUNCTION
*
*       NEG_Cln_Handle_IAC_DO_NAWS
*
*   DESCRIPTION
*
*       Internal client-side function called by NU_Telnet_Parse() to
*       process the "IAC DO NAWS" command received from the telnet server.
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
*         -1         -- the function was called by telnet server.
*         NU_SUCCESS -- otherwise.
*
************************************************************************/
STATUS NEG_Cln_Handle_IAC_DO_NAWS(INT socket)
{
    CHAR temp_buf[9];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    /* this function can only be called by client.  Make sure we are. */
    if (tw->i_am_client != NU_TRUE)
    {
        return (-1);
    }

    if (tw->nego_NAWS & NEGO_I_WILL)
    {
        /* NEGO_I_WILL is on, it means that we (client) sent the
           "IAC WILL NAWS" before and the server replied "IAC DO NAWS".
           So, the server agrees to NAWS.  Set NEGO_HE_DO flag.
           Send back to the server subnegotiation parameters (window size)
           if we didn't send it before. */

        if (!(tw->nego_NAWS & NEGO_HE_DO))
        {
            temp_buf[0] = (CHAR) TO_IAC;
            temp_buf[1] = (CHAR) TO_SB;
            temp_buf[2] = (CHAR) TO_NAWS;
            temp_buf[3] = (UINT8) 0;
            temp_buf[4] = (UINT8) tw->width;
            temp_buf[5] = (UINT8) 0;
            temp_buf[6] = (UINT8) tw->height;
            temp_buf[7] = (CHAR) TO_IAC;
            temp_buf[8] = (CHAR) TO_SE;

            NU_Send(socket, temp_buf, 9, 0);

            tw->nego_NAWS |= NEGO_HE_DO;
        }
    }
    else
    {
        /* The server sent "IAC DO NAWS".  But NEGO_I_WILL flag
           on the client side is off, it means client is not interested
           in telling the server window size ... Send "IAC WONT NAWS" if
           we didn't send it before. */

        tw->nego_NAWS |= NEGO_HE_DO;

        if (!(tw->nego_NAWS & NEGO_I_WONT))
        {
            temp_buf[0] = (CHAR) TO_IAC;
            temp_buf[1] = (CHAR) TO_WONTTEL;
            temp_buf[2] = (CHAR) TO_NAWS;

            NU_Send(socket, temp_buf, 3, 0);

            tw->nego_NAWS |= NEGO_I_WONT;
        }
    }

    return NU_SUCCESS;
} /* end NEG_Cln_Handle_IAC_DO_NAWS */

/************************************************************************
*
*   FUNCTION
*
*       NEG_Cln_Handle_IAC_DONT_NAWS
*
*   DESCRIPTION
*
*       Internal client-side function called by NU_Telnet_Parse() to
*       process the "IAC DONT NAWS" command received from the telnet server.
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
*         -1         -- the function was called by telnet server.
*         NU_SUCCESS -- otherwise.
*
************************************************************************/
STATUS NEG_Cln_Handle_IAC_DONT_NAWS(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    /* this function can only be called by client.  Make sure we are. */
    if (tw->i_am_client != NU_TRUE)
    {
        return (-1);
    }

    if (tw->nego_NAWS & NEGO_I_WILL)
    {
        /* NEGO_I_WILL is on, it means that we (client) sent the
           "IAC WILL NAWS" before and the server replied "IAC DONT NAWS".
           So, the server does not agree to NAWS.  Set NEGO_HE_DONT flag.
           Don't send anything back to the client. */

        tw->nego_NAWS |= NEGO_HE_DONT;
    }
    else if (tw->nego_NAWS & NEGO_I_WONT)
    {
        /* NEGO_I_WONT is on, it means that we (client) sent the
           "IAC WONT NAWS" before and the server replied "IAC DONT NAWS".
           So, the server agrees not to NAWS.  Set NEGO_HE_DONT flag.
           Don't send anything back to the client. */

        tw->nego_NAWS |= NEGO_HE_DONT;
    }
    else if (!(tw->nego_NAWS & NEGO_I_WILL) && !(tw->nego_NAWS & NEGO_I_WONT))
    {
        /* We (client) sent neither "IAC WILL NAWS", nor "IAC WONT NAWS" before,
           but received "IAC DONT NAWS" from the server.  This means the server
           does not want to NAWS.  Send "IAC WONT NAWS". */

        temp_buf[0] = (CHAR) TO_IAC;
        temp_buf[1] = (CHAR) TO_WONTTEL;
        temp_buf[2] = (CHAR) TO_NAWS;

        NU_Send(socket, temp_buf, 3, 0);

        tw->nego_NAWS |= NEGO_HE_DONT;
        tw->nego_NAWS |= NEGO_I_WONT;
    }

    return NU_SUCCESS;
} /* NEG_Cln_Handle_IAC_DONT_NAWS */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Cln_Handle_IAC_DO_TERMTYPE
*
*   DESCRIPTION
*
*       Internal client-side function called by NU_Telnet_Parse() to
*       process the "IAC DO TERMINAL-TYPE" command received from
*       the telnet server.  nego_TERMTYPE bits set by this function are
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
*         -1         -- the function was called by telnet server.
*         NU_SUCCESS -- otherwise.
*
************************************************************************/
STATUS NEG_Cln_Handle_IAC_DO_TERMTYPE(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    /* this function can only be called by client.  Make sure we are. */
    if (tw->i_am_client != NU_TRUE)
    {
        return (-1);
    }

    if (tw->nego_TERMTYPE & NEGO_I_WILL)
    {
        /* NEGO_I_WILL is on, it means that we (client) sent the
           "IAC WILL TERMINAL-TYPE" before and the server replied
           "IAC DO TERMINAL-TYPE".  So, the server agrees to
           negotiate about TERMINAL-TYPE.
           Next, the server will ask about term.type we (client) support, i.e.
           we should be receiving IAC SB TERMINAL-TYPE SEND IAC SE from the
           server. */

        tw->nego_TERMTYPE |= NEGO_HE_DO;
    }
    else
    {
        /* The server sent "IAC DO TERMINAL-TYPE".  But NEGO_I_WILL flag
           on the client side is off, it means we (client) do not want to tell
           the server what terminal type we support.
           Send "IAC WONT TERMINAL-TYPE" if we (client) didn't send it before. */

        tw->nego_TERMTYPE |= NEGO_HE_DO;

        if (!(tw->nego_TERMTYPE & NEGO_I_WONT))
        {

            temp_buf[0] = (CHAR) TO_IAC;
            temp_buf[1] = (CHAR) TO_WONTTEL;
            temp_buf[2] = (CHAR) TO_TERMTYPE;

            NU_Send(socket, temp_buf, 3, 0);

            tw->nego_TERMTYPE |= NEGO_I_WONT;
        }
    }

    return NU_SUCCESS;
} /* NEG_Cln_Handle_IAC_DO_TERMTYPE */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Cln_Handle_IAC_DONT_TERMTYPE
*
*   DESCRIPTION
*
*       Internal client-side function called by NU_Telnet_Parse() to
*       process the "IAC DONT TERMINAL-TYPE" command received from
*       the telnet server.  nego_TERMTYPE bits set by this function are
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
*         -1         -- the function was called by telnet server.
*         NU_SUCCESS -- otherwise.
*
************************************************************************/
STATUS NEG_Cln_Handle_IAC_DONT_TERMTYPE(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    /* this function can only be called by client.  Make sure we are. */
    if (tw->i_am_client != NU_TRUE)
    {
        return (-1);
    }

    if (tw->nego_TERMTYPE & NEGO_I_WILL)
    {
        /* NEGO_I_WILL is on, it means that we (client) sent the
           "IAC WILL TERMINAL-TYPE" before and the server replied
           "IAC DONT TERMINAL-TYPE".  So, the server does not
           agree to negotiate about termtype.  Set NEGO_HE_DONT flag.
           Don't send anything back to the server. */

        tw->nego_TERMTYPE |= NEGO_HE_DONT;
    }
    else if (tw->nego_TERMTYPE & NEGO_I_WONT)
    {
        /* we (client) sent the "IAC WONT TERMINAL-TYPE" before
           and the server replied "IAC DONT TERMINAL-TYPE".  So,
           the server agrees not to negotiate about termtype.
           Set NEGO_HE_DONT flag.  Don't send anything back to the server. */

        tw->nego_TERMTYPE |= NEGO_HE_DONT;
    }
    else if ( !(tw->nego_TERMTYPE & NEGO_I_WILL) &&
              !(tw->nego_TERMTYPE & NEGO_I_WONT) )
    {
        /* We (client) sent neither "IAC WILL TERMINAL-TYPE", nor
           "IAC WONT TERMINAL-TYPE" before, but received
           "IAC DONT TERMINAL-TYPE" from the server.  This means
           the server does not want to negotiate about termtype.
           Send "IAC WONT TERMINAL-TYPE". */

        temp_buf[0] = (CHAR) TO_IAC;
        temp_buf[1] = (CHAR) TO_WONTTEL;
        temp_buf[2] = (CHAR) TO_TERMTYPE;

        NU_Send(socket, temp_buf, 3, 0);

        tw->nego_TERMTYPE |= NEGO_I_WONT;
        tw->nego_TERMTYPE |= NEGO_HE_DONT;
    }

    return NU_SUCCESS;
} /* NEG_Client_Handle_IAC_DONT_TERMTYPE */


/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Client_Init_Parameters
*
*   DESCRIPTION
*
*       Initialize parameters of a client telnet session.
*
*   INPUTS
*
*       client_socket  The index of client socket of telnet connection.
*
*   RETURNS
*
*       NU_SUCCESS,
*       NU_INVALID_SOCKET,
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
STATUS NU_Telnet_Client_Init_Parameters(INT client_socket)
{
    struct TN_SESSION_STRUCT *client_session;
    STATUS status;
    NU_SUPERV_USER_VARIABLES

    /* Check for invalid input. */
    if ( (client_socket < 0) || (client_socket >= NSOCKETS) )
        return (NU_INVALID_SOCKET);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    status = TL_Telnet_Init_Parameters(client_socket);

    if (status == NU_SUCCESS)
    {
        NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);
        client_session = TN_Session[client_socket];
        client_session->i_am_client = (unsigned short)NU_TRUE;
        NU_Release_Semaphore(&TN_Session_Semaphore);
    }

    NU_USER_MODE();    /* return to user mode */

    return status;
} /* NU_Telnet_Client_Init_Parameters */


/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Client_Connect
*
*   DESCRIPTION
*
*       Open a telnet connection with server.
*
*   INPUTS
*
*       server_ip       Pointer to the telnet server ip
*       server_name     Pointer to the telnet server name
*
*   OUTPUTS
*
*       None
*
*   RETURNS
*
*       On success, a socket descriptor with a value greater than
*                   or equal to 0;
*
*       On failure, one of the following Nucleus status codes:
*           1) NU_Socket() failure codes:
*                               NU_INVALID_PROTOCOL
*                               NU_NO_SOCKET_SPACE
*                               NU_NO_SOCK_MEMORY
*          2) NU_Allocate_Memory() failure codes:
*                               NU_INVALID_POOL
*                               NU_INVALID_POINTER
*                               NU_INVALID_SIZE
*                               NU_INVALID_SUSPEND
*                               NU_NO_MEMORY
*                               NU_TIMEOUT
*                               NU_POOL_DELETED
*          3) NU_Connect() failure codes:
*                               NU_NO_PORT_NUMBER
*                               NU_INVALID_PARM
*                               NU_NOT_CONNECTED
*                               NU_INVALID_SOCKET
*
************************************************************************/
INT NU_Telnet_Client_Connect(CHAR *server_ip, CHAR *server_name)
{
    return (NU_Telnet_Client_Connect2(server_ip, server_name, NU_FAMILY_IP));
}

/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Client_Connect2
*
*   DESCRIPTION
*
*       Open a telnet connection with server.
*
*   INPUTS
*
*       server_ip       Pointer to the telnet server ip
*       server_name     Pointer to the telnet server name
*       family          Family type of the server
*
*   OUTPUTS
*
*       None
*
*   RETURNS
*
*       On success, a socket descriptor with a value greater than
*                   or equal to 0;
*
*       On failure, one of the following Nucleus status codes:
*           1) NU_Socket() failure codes:
*                               NU_INVALID_PROTOCOL
*                               NU_NO_SOCKET_SPACE
*                               NU_NO_SOCK_MEMORY
*          2) NU_Allocate_Memory() failure codes:
*                               NU_INVALID_POOL
*                               NU_INVALID_POINTER
*                               NU_INVALID_SIZE
*                               NU_INVALID_SUSPEND
*                               NU_NO_MEMORY
*                               NU_TIMEOUT
*                               NU_POOL_DELETED
*          3) NU_Connect() failure codes:
*                               NU_NO_PORT_NUMBER
*                               NU_INVALID_PARM
*                               NU_NOT_CONNECTED
*                               NU_INVALID_SOCKET
************************************************************************/
INT NU_Telnet_Client_Connect2(CHAR *server_ip, CHAR *server_name,
                              INT16 family)
{
    INT                 socket;     /* the socket descriptor */
    struct addr_struct  servaddr;  /* holds the server address structure */
    INT                 status;
    NU_SUPERV_USER_VARIABLES

    /* Check for bad input. */
    if ( (server_ip == NU_NULL) || (server_name == NU_NULL) )
        return (NU_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* open a connection via the socket interface */
    socket = NU_Socket(family, NU_TYPE_STREAM, 0);

    if (socket < 0)
    {
        /* Can't get socket for telnet, get out. */
        NERRS_Log_Error (NERR_FATAL, __FILE__, __LINE__);
        NU_USER_MODE();    /* return to user mode */
        return socket;
    }

    if (TN_SEMAPHORE_INITIALIZED == NU_FALSE)
    {
        /* this semaphore will control access to the TN_Session structure;
           it is used by both client and server.  Make sure we create
           the semaphore only once. */
        status = NU_Create_Semaphore(&TN_Session_Semaphore,
                                     "TN_SESSION_SEMAPHORE", 1, NU_FIFO);
        if (status != NU_SUCCESS)
        {
            NERRS_Log_Error( NERR_FATAL, __FILE__, __LINE__);

            /* Close the socket created above */
            NU_Close_Socket(socket);

            NU_USER_MODE();    /* return to user mode */
            return status;
        }

        TN_SEMAPHORE_INITIALIZED = NU_TRUE;
    }

    /* fill in a structure with the telnet server address */
    servaddr.family             = family;
    servaddr.port               = NK_TELNET;

#if ( (defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE) )
    if (family == NU_FAMILY_IP6)
        memcpy(servaddr.id.is_ip_addrs, server_ip, IP6_ADDR_LEN);
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
        memcpy(servaddr.id.is_ip_addrs, server_ip, IP_ADDR_LEN);
#endif

    servaddr.name = server_name;

    status = (INT)NU_Connect(socket, &servaddr, 0);

    /* If the connect failed, close the socket */
    if (status < 0)
        NU_Close_Socket(socket);

    NU_USER_MODE();    /* return to user mode */

    return status;

} /* NU_Telnet_Client_Connect2 */
