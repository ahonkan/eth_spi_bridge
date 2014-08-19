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
*   FILENAME                                               
*
*       tl_util.c                                      
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus Telnet Server
*
*   DESCRIPTION
*
*       Nucleus Telnet utilities
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Close_And_Check_Retval
*       NU_Receive_NVT_Key
*       NU_Send_NVT_Key
*       NU_Received_Exit
*       TL_Telnet_Init_Parameters
*       NU_Telnet_Client_Init_Parameters
*       NU_Telnet_Server_Init_Parameters
*       NU_Telnet_Get_Session_Parameters
*       NU_Telnet_Free_Parameters
*       NU_Telnet_Check_Connection
*       NU_Telnet_Send
*       NU_Telnet_Client_Connect
*       NU_Telnet_Client_Connect2
*       NU_Telnet_Socket
*       NU_Telnet_Server_Accept
*       TN_Print
*       NU_Wait_For_Pattern
*       NU_Telnet_Echo_Get
*       NU_Telnet_Echo_Set
*
*   DEPENDENCIES
*
*       stdarg.h
*       nucleus.h
*       target.h
*       externs.h
*       nerrs.h
*       telopts.h
*       windat.h
*       vtkeys.h
*       tel_extr.h
*       telnet_cfg.h
*
*************************************************************************/

#include <stdarg.h>

#include "nucleus.h"
#include "networking/target.h"
#include "networking/externs.h"
#include "networking/nerrs.h"
#include "networking/telopts.h"
#include "networking/windat.h"
#include "networking/vtkeys.h"
#include "networking/tel_extr.h"
#include "networking/telnet_cfg.h"

struct TN_SESSION_STRUCT *TN_Session[NSOCKETS];

INT TN_SEMAPHORE_INITIALIZED = NU_FALSE;

NU_SEMAPHORE TN_Session_Semaphore;

/* this variable is used in TL_UNUSED_PARAMETER macro
   defined in tel_extr.h.  The macro is used to remove
   compilation warnings */
UINT32 TL_Unused_Parameter;


/************************************************************************
*
*   FUNCTION
*
*       NU_Receive_NVT_Key
*
*   DESCRIPTION
*
*       Translate a special terminal emulation key to a local key value
*       and explain it. The users can modify this function to match their
*       own typical environment.
*
*   INPUTS
*
*       socket      The index of socket of the telnet connection
*       string      Pointer to the key string to be converted
*       execute     Indicate whether to execute the function associated
*                   with the function key or not (just do mapping)
*
*   OUTPUTS
*
*       The length of the key string.
*       -1 if any input parameter is invalid.
*
************************************************************************/
INT NU_Receive_NVT_Key(INT socket, CHAR *string, CHAR execute)
{
    CHAR len, i=0;
    struct TN_SESSION_STRUCT *temp_twin;
    NU_SUPERV_USER_VARIABLES

    /* Check for bad input. */
    if ( (string == NU_NULL) || (socket < 0) || (socket >= NSOCKETS) )
        return (-1);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);

    /* get the pointer of the parameter data struct of the current session */
    temp_twin = TN_Session[socket];

    if (temp_twin->termtype!=VT100TYPE)
    {
        NU_Release_Semaphore(&TN_Session_Semaphore);
        NU_USER_MODE();    /* return to user mode */
        return(0);
    }

    len = (CHAR)(strlen(vt100keys[(INT)i].key_string));
    while (len)
    {
        if (memcmp(string, vt100keys[(INT)i].key_string, (UINT16)len)==0)
        {
            /* the users, who define certain functions for these keys, need to
              add code here to call the functions associated with the function
              key. The 'execute' flag allows the user to switch whether to execute
              the function or not (just do mapping), before entering this routine.
              */

            if (execute)
            {
            }
            else
            {
            }
            NU_Release_Semaphore(&TN_Session_Semaphore);
            NU_USER_MODE();    /* return to user mode */
            return(len);
        }
        else
            len = (CHAR)(strlen(vt100keys[(INT)++i].key_string));
    }

    NU_Release_Semaphore(&TN_Session_Semaphore);
    NU_USER_MODE();    /* return to user mode */
    return(0);
} /* NU_Receive_NVT_Key */

/************************************************************************
*
*   FUNCTION
*
*       NU_Send_NVT_Key
*
*   DESCRIPTION
*
*       Takes a key value and checks whether it is a mapped key, then
*       checks whether it is a 'special' key (i.e. vt100 keys, and maybe
*       other kermit verbs), passes the characters on to the TCP port in
*       pnum.
*
*   INPUTS
*
*       socket      The index of socket of the telnet connection
*       key_val     The keycode to check, convert and send
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID NU_Send_NVT_Key(INT socket, UINT32 key_val)
{
    INT         i = 0;
    struct TN_SESSION_STRUCT *temp_twin;
    INT    binary_mode_on;
    NU_SUPERV_USER_VARIABLES

    /* Check for bad input. */
    if ( (socket < 0) || (socket >= NSOCKETS) )
        return;

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);

    /* get the pointer of the parameter data struct of the current session */
    temp_twin = TN_Session[socket];

    /* check if the direction of "this side-to-other side" connection
       is in binary mode */
    binary_mode_on = (temp_twin->nego_Binary & NEGO_I_WILL) &&
                     (temp_twin->nego_Binary & NEGO_HE_DO);

    /* if the key is not mapped, just send it. And, make certain it is
        an ascii char, or we are transmitting binary data */
    if ( (key_val<128) || (key_val<=255 && binary_mode_on))
    {
        if ( (key_val==(UINT32)TO_IAC) && (binary_mode_on) )
        /* if we are in binary mode and it happens that byte value is 0xFF,
        in order to avoid to confuse with IAC, the starter of telnet commands,
        send IAC one more time, NU_Telnet_Parse() knows that this IAC
        is a binary data */
            NU_Send(socket, (CHAR *)&key_val, 1, 0);

        NU_Send(socket, (CHAR *)&key_val, 1, 0);
        NU_Release_Semaphore(&TN_Session_Semaphore);
        NU_USER_MODE();    /* return to user mode */
        return;
    }

    /* if terminal emulation is negotiated and VT100 is supported, we do it.
        the users need to modify this code to match specific application. */
    if (temp_twin->termtype==VT100TYPE)
    {
        while (vt100keys[i].key_value)
        {
            if (key_val==(UINT32)vt100keys[i].key_value)
            {
                NU_Send(socket, vt100keys[i].key_string,
                    (UINT16)(strlen(vt100keys[i].key_string)), 0);
                NU_Release_Semaphore(&TN_Session_Semaphore);
                NU_USER_MODE();    /* return to user mode */
                return;
            }
            else
                i++;
        }
    }
    NU_Release_Semaphore(&TN_Session_Semaphore);
    NU_USER_MODE();    /* return to user mode */
}   /* end NU_Send_NVT_Key */

/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Send
*
*   DESCRIPTION
*
*       Send the data to the network.
*
*   INPUTS
*
*       socket      The index of socket of the telnet connection
*       fmt         Pointer to the data to print
*
*   RETURNS
*
*       The number of bytes sent, or < 0 if an error
*
************************************************************************/
INT NU_Telnet_Send(INT socket, CHAR *fmt)
{
#if (TN_VERSION_COMP < TN_1_2)
    CHAR    temp_str[256];     /* do not input a string longer than 255 !!! */
    va_list arg_ptr;
    INT     str_len;
    INT     ret_value = -3;     /* return value; by default -- error */
    NU_SUPERV_USER_VARIABLES

    /* Check for bad input. */
    if ( (fmt == NU_NULL) || (socket < 0) || (socket > NSOCKETS) )
        return (NU_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    va_start(arg_ptr, fmt);
    str_len = vsprintf(temp_str, fmt, arg_ptr);

    if (str_len>255)
    {
        NERRS_Log_Error (NERR_FATAL, __FILE__, __LINE__);
        NU_USER_MODE();    /* return to user mode */
        return ret_value;
    }

    va_end(arg_ptr);
    if (str_len>0)      /* good string to transmit */
    {
        ret_value = (INT)NU_Send(socket, temp_str, (UINT16)strlen(temp_str), 0);
    }

    NU_USER_MODE();    /* return to user mode */
    return ret_value;
#else
    INT ret_value;
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    ret_value = (INT)NU_Send(socket, fmt, (UINT16)strlen(fmt), 0);

    NU_USER_MODE();    /* return to user mode */
    return ret_value;
#endif
}   /* end NU_Telnet_Send() */

/************************************************************************

*   FUNCTION
*
*       NU_Received_Exit
*
*   DESCRIPTION
*
*       Checks "exit" command from client.
*
*   INPUTS
*
*       data    Pointer to the input string to be parsed
*       cnt     The length of the string
*
*   RETURNS
*
*       0       No exit command found
*       1       Found exit command
*
************************************************************************/
INT NU_Received_Exit(CHAR *data, INT cnt)
{
    static char tl_exit=0, e[5]="exit";
    INT    ret_value = 0;
    NU_SUPERV_USER_VARIABLES

    /* Check for bad input. */
    if ( (data == NU_NULL) || (cnt < 1) )
        return 0;

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (cnt==4)
    {
        if (memcmp(data, e, 4)==0)
        {
            ret_value = 1;
        }
    }
    else if (cnt==1)
    {
        if (e[(INT)tl_exit]==data[0])
        {
            if (++tl_exit==4)
            {
                ret_value = 1;
            }
        }
        else
        {
            tl_exit = 0;
        }
    }

    NU_USER_MODE();    /* return to user mode */
    return ret_value;
} /* NU_Received_Exit */

/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Free_Parameters
*
*   DESCRIPTION
*
*       Free the data struct pointer of the parameters of a telnet
*       session.
*
*   INPUTS
*
*       socket      The index of socket of the telnet connection
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID NU_Telnet_Free_Parameters(INT socket)
{
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);

    if ( (socket >= 0) && (socket < NSOCKETS)
                       && (TN_Session[socket] != NULL) )
    {
        /*  clear this telnet session parameter list entry  */
        NU_Deallocate_Memory((VOID *) TN_Session[socket]);
        TN_Session[socket] = NU_NULL;
    }

    NU_Release_Semaphore(&TN_Session_Semaphore);

    NU_USER_MODE();    /* return to user mode */
}

/************************************************************************
*
*   FUNCTION
*
*       TL_Telnet_Init_Parameters
*
*   DESCRIPTION
*
*       Initialize all parameters of a telnet session.
*
*   INPUTS
*
*       socket      The index of socket of the telnet connection
*
*   RETURNS
*
*       NU_SUCCESS,
*       NU_INVALID_SOCKET
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
STATUS TL_Telnet_Init_Parameters(INT socket)
{
    struct TN_SESSION_STRUCT *p;
    STATUS          status;
    NU_MEMORY_POOL  *mem_pool;

    /* Check for bad input. */
    if ( (socket < 0) || (socket >= NSOCKETS) )
        return (NU_INVALID_SOCKET);

    /* get the memory for the parameter data struct of the current session */
#ifdef NET_5_1
    mem_pool = MEM_Cached;
#else
    mem_pool = &System_Memory;
#endif

    status = NU_Allocate_Memory(mem_pool,
                                (VOID **) &TN_Session[socket],
                                (UNSIGNED)(sizeof(struct TN_SESSION_STRUCT)),
                                (UNSIGNED)NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NERRS_Log_Error (NERR_FATAL, __FILE__, __LINE__);
        return status;
    }

    p = TN_Session[socket];

    /* Clear the structure. */
    UTL_Zero(p, sizeof(*p));

    p->pnum              =     -1;
    p->socket            = socket;
    p->termtype          = VTTYPE; /*basetype;*/
    p->bksp              =    127;
    p->del               =      8;
    p->height            =     24;
    p->width             =     80;
    p->nego_NAWS         =      0;
    p->nego_TERMTYPE     =      0;
    p->nego_SGA          =      0;
    p->nego_Echo         =      0;
    p->nego_Binary       =      0;
    p->CR_followed_by_LF =      1;

    return status;
}   /* end TL_Telnet_Init_Parameters() */


/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Get_Session_Parameters
*
*   DESCRIPTION
*
*       This function is used to get the telnet session parameters from
*       the internal TN_Session structure.  Because it is a bad idea for
*       an application to access the internal TN_Session structure directly,
*       NU_Telnet_Get_Session_Parameters() should be used to copy the
*       telnet session parameters from the internal TN_SESSION to the
*       publicly available NU_TN_PARAMETERS.
*
*   INPUTS
*
*       socket          The index of socket of telnet connection
*       session_params  Pointer to the NU_TN_PARAMETERS where
*                       telnet session parameters will be stored
*
*   RETURNS
*
*       NU_SUCCESS in case of success, negative value otherwise
*
*   EXAMPLE OF USAGE
*
*          // declaration
*          NU_TN_PARAMETERS session_params;
*
*          // initialize the server socket
*          NU_Telnet_Server_Init_Parameters(server_socket);
*
*          // negotiate with the client
*          NU_Telnet_Do_Negotiation(server_socket, server_nego_table);
*
*          // get the results of negotiation
*          NU_Telnet_Get_Session_Parameters(server_socket, &session_params);
*
*          // check if client sent his windows size
*          if ((session_params.nego_NAWS & NEGO_I_DO) &&
*              (session_params.nego_NAWS & NEGO_HE_WILL))
*          {
*              client_window_width  = session_params.nu_width;
*              client_window_height = session_params.nu_height;
*          }
*
************************************************************************/
STATUS NU_Telnet_Get_Session_Parameters(INT socket,
                                        NU_TN_PARAMETERS *session_params)
{
    struct TN_SESSION_STRUCT *tw;
    STATUS status;
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* validate the socket */
    if ((socket < 0) || (socket >= NSOCKETS) || (session_params == NU_NULL))
    {
        NU_USER_MODE();    /* switch back to user mode. */
        return NU_INVALID_SOCKET;
    }

    status = NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        NU_USER_MODE();    /* switch back to user mode. */
        return status;
    }

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw != NULL)
    {
        session_params->nu_i_am_client   = tw->i_am_client;
        session_params->nu_width         = tw->width;
        session_params->nu_height        = tw->height;
        session_params->nu_termtype      = tw->termtype;
        session_params->nu_nego_naws     = tw->nego_NAWS;
        session_params->nu_nego_termtype = tw->nego_TERMTYPE;
        session_params->nu_nego_sga      = tw->nego_SGA;
        session_params->nu_nego_echo     = tw->nego_Echo;
        session_params->nu_nego_binary   = tw->nego_Binary;
    }
    else    /* tw is NULL */
    {
        status = -1;    /* return error */
    }

    NU_Release_Semaphore(&TN_Session_Semaphore);
    NU_USER_MODE();    /* return to user mode */

    return status;
} /* NU_Telnet_Get_Session_Parameters */

/************************************************************************
*
*   FUNCTION
*
*       NU_Close_And_Check_Retval
*
*   DESCRIPTION
*
*       Check the return value of NU_Close_Socket().
*
*   INPUTS
*
*       socket      The index of socket of the telnet connection
*
*   RETURNS
*
*       Status of NU_Close_Socket
*
************************************************************************/
INT NU_Close_And_Check_Retval(INT socket)
{
    INT retval;
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if ((retval = NU_Close_Socket(socket)) != NU_SUCCESS)
        NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);

    NU_USER_MODE();    /* return to user mode */

    return(retval);
}    /* NU_Close_And_Check_Retval() */


/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Socket
*
*   DESCRIPTION
*
*       Create the first socket for TELNET Server.
*
*   INPUTS
*
*       server_ip       Pointer to the telnet server ip
*       server_name     Pointer to the telnet server name
*
*   RETURNS
*
*       Socket descriptor if successful, otherwise -1.
*
************************************************************************/
INT NU_Telnet_Socket(CHAR *server_ip, CHAR *server_name)
{
    INT                 socket;             /* the socket descriptor */
    struct addr_struct  *servaddr;          /* holds the server address structure */
    VOID                *pointer;
    INT                 status;
    INT16               family;
    NU_MEMORY_POOL      *mem_pool;

    NU_SUPERV_USER_VARIABLES

    /* Check for invalid input. */
    if ( (server_ip == NU_NULL) || (server_name == NU_NULL) )
        return (-1);
		
    /* If an IP address was specified, the Telnet Server should be bound
     * to that address and only process IPv4 requests; otherwise, bind
     * to the WILDCARD address and process IPv4 and IPv6 requests.
     */
#if ( (defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE) )
    if (IP_ADDR((UINT8*)server_ip) == 0)
        family = NU_FAMILY_IP6;
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        family = NU_FAMILY_IP;
#endif

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* open a connection via the socket interface */
    socket = NU_Socket(family, NU_TYPE_STREAM, 0);

    if (socket < 0)
    {
        NU_USER_MODE();    /* return to user mode */
        return(-1);
    }

#if ( (defined(NET_5_1)) && (INCLUDE_TCP_KEEPALIVE == NU_TRUE) )
    if (NU_Setsockopt_SO_KEEPALIVE(socket, 1) != NU_SUCCESS)
    {
        NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
    }
#endif

#ifdef NET_5_1
    mem_pool = MEM_Cached;
#else
    mem_pool = &System_Memory;
#endif

    status = NU_Allocate_Memory(mem_pool,
                                &pointer,
                                sizeof(struct addr_struct),
                                NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Close the socket. */
        NU_Close_Socket(socket);

        NU_USER_MODE();    /* return to user mode */
        return (-1);
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
            /* Close the socket. */
            NU_Close_Socket(socket);

            /* Deallocate memory reserved for server address. */
            NU_Deallocate_Memory(pointer);

            NERRS_Log_Error( NERR_FATAL, __FILE__, __LINE__);
            NU_USER_MODE();    /* return to user mode */
            return status;
        }

        TN_SEMAPHORE_INITIALIZED = NU_TRUE;
    }

    servaddr = (struct addr_struct *)pointer;

     /* fill in a structure with the server address */
    servaddr->family             = family;
    servaddr->port               = CFG_NU_OS_NET_PROT_TELNET_SERVER_TCP_PORT;

#if ( (defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE) )
    if (family == NU_FAMILY_IP6)
        servaddr->id = IP6_ADDR_ANY;
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        servaddr->id.is_ip_addrs[0]  = (UINT8)server_ip[0];
        servaddr->id.is_ip_addrs[1]  = (UINT8)server_ip[1];
        servaddr->id.is_ip_addrs[2]  = (UINT8)server_ip[2];
        servaddr->id.is_ip_addrs[3]  = (UINT8)server_ip[3];
    }
#endif

    servaddr->name = server_name;

    /* make an NU_Bind() call to bind the server's address */
    if ((NU_Bind(socket, servaddr, 0)) >= 0)
    {
        /* be ready to accept connection requests */
        status = NU_Listen(socket, 10);

        if (status == NU_SUCCESS)
        {
            NU_USER_MODE();    /* return to user mode */
            return (socket);
        }
    }

    if (status != NU_SUCCESS)
    {
        /* Delete TN session semaphore. */
        NU_Delete_Semaphore(&TN_Session_Semaphore);

        /* Close the socket. */
        NU_Close_Socket(socket);

        /* Deallocate memory associated with server address. */
        NU_Deallocate_Memory(servaddr);
    }

    NU_USER_MODE();    /* return to user mode */
    return (-1);

} /* NU_Telnet_Socket */


/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Check_Connection
*
*   DESCRIPTION
*
*       Check if the connection on the socket is closed or not. If it is
*       open, and the parameter close_it = 1, close the connection.
*
*   INPUTS
*
*       socket      The index of socket of the telnet connection
*       close_it    Flag indicator to close the connection
*                   (NU_TRUE - close, NU_FALSE - check the status.)
*
*   RETURNS
*
*      NU_TRUE     1) when close_it is non-zero (trying to close
*                     the connection) and the socket is
*                     closed successfully or not connected, or
*                  2) when close_it is 0 (checking the status) and
*                     the connection on the socket is closed;
*
*      NU_FALSE    when close_it is 0 (checking the status) and
*                  the connection on the socket is opened;
*
*      NU_INVALID_SOCKET
************************************************************************/
INT NU_Telnet_Check_Connection(INT socket, INT close_it)
{
    INT cnt;
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (close_it)
    {
        cnt = NU_Close_Socket(socket);

        switch (cnt)
        {
            case NU_SUCCESS:
            case NU_NOT_CONNECTED:
                cnt = NU_TRUE;
                break;

            case NU_INVALID_SOCKET:
                break;

            default:
                NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
                break;
        }

        NU_USER_MODE();    /* return to user mode */
        return (cnt);
    }

    /* check the connection status */
    cnt = NU_Is_Connected(socket);

    switch (cnt)
    {
        case NU_TRUE:
            cnt = NU_FALSE;    /* connected */
            break;

        case NU_FALSE:
            cnt = NU_TRUE;     /* not connected */
            break;

        case NU_INVALID_SOCKET:
            break;

        default:
            NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
            break;
    }

    NU_USER_MODE();    /* return to user mode */
    return (cnt);
}  /* NU_Telnet_Check_Connection */

/************************************************************************
*
*   FUNCTION
*
*       TN_Print
*
*   DESCRIPTION
*
*       User implemented print function.
*
*   INPUTS
*
*       fmt     String to be printed.
*
*   RETURNS
*
*       None
*
************************************************************************/
VOID TN_Print(CHAR *fmt) { TL_UNUSED_PARAMETER(fmt); }

/************************************************************************
*
*   FUNCTION
*
*       NU_Wait_For_Pattern
*
*   DESCRIPTION
*
*       This function reads from the socketd one char at a time
*       and places the chars to *buf until it finds the pattern,
*       or runs out of space in *buf.
*       Once the pattern is found, the function returns NU_TRUE.
*       The caller can check the buf_chars_read to see how many
*       bytes/chars were read before the pattern was found.
*       The first char of the pattern in this case will be
*       placed in buf[buf_chars_read].
*       If the pattern was not found, the function returns
*       NU_FALSE.  The buf_chars_read will indicate, how
*       many characters were read into the buf.
*
*       NOTE, that pattern_chars_read is used internally by
*       NU_Wait_For_Pattern().  It shows how many chars in the end
*       of *buf match the pattern.  It is essential that the caller
*       set pattern_chars_read to 0 before calling NU_Wait_For_Pattern()
*       for the very first time, and NOT change it
*       unless the pattern was found, or search for a new pattern
*       has started.
*       Every time before calling NU_Wait_For_Pattern() to start
*       looking for a new pattern, pattern_chars_read should be
*       set to 0.  After the pattern has been found,
*       pattern_chars_read == pattern_len.
*
*   INPUTS              IN/OUT
*
*   socketd             in     Socket descriptor.
*   *buf                out    Buffer to store chars read from socketd.
*   buf_size            in     Size of buf.
*   *buf_chars_read     out    Number of chars read from socketd and
*                              placed to buf.
*   *pattern            in     String we are waiting/looking for.
*                              Might be any sequence of chars including
*                              Nulls.  For example, the search pattern
*                              pattern[] = {0,0,'a','b','c',0,0};
*                              ("abc" preceded and followed by two Nulls)
*                              is a perfectly legal pattern.
*   pattern_len         in     Length of the pattern.  If we try to match
*                              one or more Nulls, strlen(pattern) will not
*                              work.  For example, in order to correctly match
*                              pattern[] = {0,0,'a','b','c',0,0};,
*                              pattern_len should be set to 7.
*   *pattern_chars_read in/out Shows how many chars in the end of buf
*                              match beginning of pattern.
*                              Used internally by NU_Wait_For_Pattern(), but
*                              should always be set to 0 by caller before
*                              starting to match for a new pattern.
*   timeout             in     Timeout value in ticks.  Two other possible
*                              values are NU_SUSPEND and NU_NO_SUSPEND.
*
*   RETURNS
*
*       NU_TRUE      - pattern found.
*       NU_FALSE     - pattern not found.
*                      In both cases, buf_chars_read indicates, how many
*                      chars were read into the buf.
*       -1           - wrong arguments.
*       NU_NO_DATA   - timed out.  Returned only when timeout argument is
*                      not NU_SUSPEND or NU_NO_SUSPEND.  When NU_NO_DATA
*                      is returned, check buf_chars_read to see if any data
*                      was read into buf.
*     Failure codes:
*
*       NU_NOT_CONNECTED
*       NU_NO_SOCKETS
*       NU_INVALID_SOCKET
*       NU_NO_PORT_NUMBER
*       NU_INVALID_SOCKET
*       NU_NO_ROUTE_TO_HOST
*       NU_CONNECTION_REFUSED
*
************************************************************************/
STATUS NU_Wait_For_Pattern(INT        socketd,
                           CHAR       *buf,
                           UINT16     buf_size,
                           UINT16     *buf_chars_read,
                           const CHAR *pattern,
                           UINT16     pattern_len,
                           UINT16     *pattern_chars_read,
                           UNSIGNED   timeout)
{
    STATUS   pattern_found = NU_FALSE;
    UINT16   buf_index     = 0;  /* current element of buf           */
    UINT16   pattern_index = 0;  /* element of pattern being matched */
    CHAR     c;                  /* char read from the socketd       */
    STATUS   status;
    FD_SET   readfs;
    UNSIGNED time_elapsed;
    UNSIGNED timeout_set;
    UNSIGNED clock_value;
    NU_SUPERV_USER_VARIABLES

    /* Check for invalid input. */
    if ( (socketd < 0) || (socketd > NSOCKETS) || (buf == NU_NULL) ||
         (buf_chars_read == NU_NULL) || (pattern == NU_NULL) ||
         (pattern_chars_read == NU_NULL) )
        return (-1);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if ((buf_size == 0) || (pattern_len == 0) || (pattern_len > buf_size))
    {
        NU_USER_MODE();    /* return to user mode */
        return -1;
    }

    if (NU_Is_Connected(socketd) != NU_TRUE)
    {
        NU_USER_MODE();    /* return to user mode */
        return NU_NOT_CONNECTED;
    }

    /* any chars read into the end of buf during previous
       call of NU_Wait_For_Pattern() match the pattern? */
    if (*pattern_chars_read)
    {
        /* copy those chars to the beginning of buf */
        strncpy (buf, pattern, *pattern_chars_read);

        buf_index = pattern_index = *pattern_chars_read;
    }

    NU_FD_Init(&readfs);
    NU_FD_Set(socketd, &readfs);

    clock_value = NU_Retrieve_Clock();  /* initialize clock */

    timeout_set = ((timeout != NU_SUSPEND) &&
                   (timeout != NU_NO_SUSPEND)) ? NU_TRUE : NU_FALSE;

    while (buf_index < buf_size)
    {
        if (timeout_set)
        {
            clock_value = NU_Retrieve_Clock();
        }

        status = NU_Telnet_Get_Filtered_Char(socketd, timeout, &c);
        if (status != NU_SUCCESS)
        {
            /* return the status error code to the caller */
            pattern_found = status;
            break;
        }

        buf[buf_index++] = c;

        /* check if the char read is part of the pattern */
        if (c == pattern[pattern_index])
        {
            /* c is element pattern_index of the pattern */
            *pattern_chars_read = ++pattern_index;

            if (pattern_index == pattern_len)
            {
                pattern_found = NU_TRUE;
                break;
            }
        }
        else if (c == pattern[0])
        {
            /* this branch handles the situation when a pattern is
               immediately preceded by an incomplete pattern.
               For example, it will correctly start matching
               pattern[]="abcd" in the sequence of chars "ababcd". */
            pattern_index = *pattern_chars_read = 1;
        }
        else
        {
            /* c is a regular char, neither beginning of pattern,
               nor its continuation. */
            pattern_index = *pattern_chars_read = 0;
        }

        if (timeout_set)
        {
            time_elapsed = NU_Retrieve_Clock() - clock_value;

            if (timeout <= time_elapsed)
            {
                pattern_found = NU_NO_DATA;
                break;
            }
            else
            {
                timeout = timeout - time_elapsed;
            }
        }
    }  /* end of   while (buf_index < buf_size) */

    *buf_chars_read = (pattern_found == NU_TRUE) ?
                      (buf_index - pattern_len) :
                      (buf_index - *pattern_chars_read);

    NU_USER_MODE();    /* return to user mode */

    return pattern_found;
} /* NU_Wait_For_Pattern */

/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Echo_Get
*
*   DESCRIPTION
*
*       This function is used to retrieve the current echo settings for
*       a given socket.
*
*   INPUTS
*
*       socket          The index of socket of telnet connection
*
*   RETURNS
*
*       Old echo params in case of success, negative value otherwise
*
************************************************************************/
STATUS NU_Telnet_Echo_Get(INT socket)
{
    struct TN_SESSION_STRUCT *tw;
    STATUS status;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* validate the socket */
    if ((socket >= 0) && (socket < NSOCKETS))
    {
        /* Obtain Session Semaphore */
        status = NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* get pointer of parameter data struct of current session */
            tw = TN_Session[socket];

            if (tw != NULL)
            {
                /* Set status to current echo value */
                status = tw->nego_Echo;
            }
            else    /* tw is NULL */
            {
                status = -1;    /* return error */
            }
        }
        else; /* Failed to obtain semaphore */
    }
    else /* Failed to validate socket */
    {
        status = NU_INVALID_SOCKET;
    }

    NU_Release_Semaphore(&TN_Session_Semaphore);
    NU_USER_MODE();    /* return to user mode */

    return status;
} /* NU_Telnet_Echo_Get */

/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Echo_Set
*
*   DESCRIPTION
*
*       This function is used to set the echo settings for a given socket.
*       A new_nego_echo value of NEGO_I_WONT|NEGO_HE_DONT will suppress
*       echo. To restore echo settings, the old echo value saved as a
*       result of call to NU_Telnet_Echo_Get() should be passed in to
*       this routine.
*
*   INPUTS
*
*       socket          The index of socket of telnet connection
*       new_nego_echo   New integer value for echo
*                         Set to NEGO_I_WONT|NEGO_HE_DONT (suppress echo).
*
*   RETURNS
*
*       NU_SUCCESS in case of success, negative value otherwise
*
************************************************************************/
STATUS NU_Telnet_Echo_Set(INT socket, INT new_nego_echo)
{
    struct TN_SESSION_STRUCT *tw;
    STATUS status;
    NU_SUPERV_USER_VARIABLES

        NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Validate new_nego_echo */
    if ( (((new_nego_echo & NEGO_I_DO) && (new_nego_echo & NEGO_HE_WILL))||
        ((new_nego_echo & NEGO_I_WILL)&& (new_nego_echo & NEGO_HE_DO))  ||
        (new_nego_echo & NEGO_I_WONT) || (new_nego_echo & NEGO_I_DONT)  ||
        (new_nego_echo & NEGO_HE_WONT)|| (new_nego_echo & NEGO_HE_DONT)) )
    {
        /* validate the socket */
        if ((socket >= 0) && (socket < NSOCKETS))
        {
            status = NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);
            if (status == NU_SUCCESS)
            {
                /* get pointer of parameter data struct of current session */
                tw = TN_Session[socket];

                if (tw != NULL)
                {
                    if (tw->nego_Echo != new_nego_echo)
                    {
                        /* Set new echo value */
                        tw->nego_Echo = new_nego_echo;
                    }
                    else; /* Echo params are same as input */
                }
                else    /* tw is NULL */
                {
                    status = -1;    /* return error */
                }
            }
            else; /* Failed to obtain semaphore */
        }
        else /* Failed to validate socket */
        {
            status = NU_INVALID_SOCKET;
        }
    }
    else /* Failed to validate new_nego_echo */
        status = -1;

    NU_Release_Semaphore(&TN_Session_Semaphore);
    NU_USER_MODE();    /* return to user mode */

    return status;
} /* NU_Telnet_Echo_Set */
