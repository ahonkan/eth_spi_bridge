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
*       negotiat.c                                     
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus Telnet Server
*
*   DESCRIPTION
*
*       Telnet negotiation utilities - Common functions
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
*       NEG_Check_Recv_Retval()
*       NEG_Parsewrite()
*       NEG_Set_One_Option()
*       NEG_Size_Of_Nego_Table()
*       NU_Install_Negotiate_Options()
*       NU_Telnet_Start_Negotiate()
*       NU_Telnet_Specific_Negotiate()
*       NEG_Telnet_TermType_Negotiate()
*       NEG_Telnet_NAWS_Negotiate()
*       NEG_Telnet_SGA_Negotiate()
*       NEG_Telnet_Echo_Negotiate()
*       NEG_Telnet_Binary_Negotiate()
*       NU_Telnet_Do_Negotiation()
*       NEG_Parse_Subnegotiat()
*       NEG_Handle_IAC_WILL_SGA()
*       NEG_Handle_IAC_WONT_SGA()
*       NEG_Handle_IAC_DO_SGA()
*       NEG_Handle_IAC_DONT_SGA()
*       NEG_Handle_IAC_WILL_ECHO()
*       NEG_Handle_IAC_WONT_ECHO()
*       NEG_Handle_IAC_DO_ECHO()
*       NEG_Handle_IAC_DONT_ECHO()
*       NEG_Handle_IAC_WILL_BINARY()
*       NEG_Handle_IAC_WONT_BINARY()
*       NEG_Handle_IAC_DO_BINARY()
*       NEG_Handle_IAC_DONT_BINARY()
*       NU_Telnet_Parse()
*       NU_Telnet_Pick_Up_Ascii()
*       NU_Telnet_Get_Filtered_Char()
*
*   DEPENDENCIES
*
*       nucleus.h
*       ncl.h
*       externs.h
*       nerrs.h
*       telopts.h
*       windat.h
*       nvt.h
*       tel_extr.h
*       negotiat.h
*       telnet_cfg.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/ncl.h"
#include "networking/externs.h"
#include "networking/nerrs.h"
#include "networking/telopts.h"

#include "networking/windat.h"

#include "networking/nvt.h"
#include "networking/tel_extr.h"
#include "networking/negotiat.h"
#include "networking/telnet_cfg.h"

/* defined in TL_UTIL.C */
extern struct TN_SESSION_STRUCT *TN_Session[NSOCKETS];
extern NU_SEMAPHORE TN_Session_Semaphore;

/*************************************************************************
*
*   FUNCTION
*
*       NEG_Check_Recv_Retval
*
*   DESCRIPTION
*
*       Check the return value of NU_Recv.
*
*   INPUTS
*
*       socket      The index of the socket descriptor for the connection
*       retval      The return value from NU_Recv()
*
*   OUTPUTS
*
*       1           If retval is NU_NOT_CONNECTED and the socket is
*                   successfully closed.
*       0           If retval is not negative and retval equals bytes
*       retval      If retval is negative and no excuse was given
*
*************************************************************************/
INT NEG_Check_Recv_Retval(INT socket, INT retval)
{
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (retval == NU_NOT_CONNECTED)
    {
        NU_Close_Socket(socket);
        NU_USER_MODE();    /* return to user mode */
        return(1);
    }

    /* At each location where NEG_Check_Recv_Retval is called it is expected
       that 1 byte of data has been received. */
    if ( (retval < 0) || (retval != 1) )
    {
        NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
        NU_USER_MODE();    /* return to user mode */
        return(retval);
    }

    NU_USER_MODE();    /* return to user mode */
    return(NU_SUCCESS);
}  /* NEG_Check_Recv_Retval */

/************************************************************************
*
*   FUNCTION
*
*       NEG_Parsewrite
*
*   DESCRIPTION
*
*       Print all message text on the users screen
*
*   INPUTS
*
*       str     Pointer to the data stream to be printed
*       len     The length of the part of the stream to be printed
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID NEG_Parsewrite(CHAR *str, INT len)
{
    CHAR buf;

    /* save the first character after printing length
        before replace it by a string terminate */
    buf = str[len];
    str[len] = 0;

    TN_Print(str);

    /* restore the first character after printing length */
    str[len] = buf;
}

/************************************************************************
*
*   FUNCTION
*
*       NEG_Set_One_Option
*
*   DESCRIPTION
*
*       Install one negotiation option.
*
*   INPUTS
*
*       buffer      The buffer for negotiation option
*       action      The requirement
*       option      The option of negotiation
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID NEG_Set_One_Option(CHAR **buffer, INT action, CHAR option)
{
    /* install one negotiation option */
    **buffer = (CHAR)TO_IAC;
    *buffer += 1;
    **buffer = (UINT8)action;
    *buffer += 1;
    **buffer = (UINT8)option;
    *buffer += 1;

} /* NEG_Set_One_Option */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Size_Of_Nego_Table
*
*   DESCRIPTION
*
*       Counts the number of pairs (where "pair" is "COMMAND_INDEX, OPTION")
*       in the negotiation table
*       (not counting the terminating END_OF_NEGO_TABLE).
*
*   INPUTS
*
*       nego_table   Pointer to a negotiation table.
*
*   RETURNS
*
*       Number of "COMMAND_INDEX, OPTION" pairs in the nego_table.
*
************************************************************************/
STATIC UINT16 NEG_Size_Of_Nego_Table(const CHAR *nego_table)
{
    UINT16 count;

    /* the last element in the nego_table is (nego_table always
       ends with) NULL.  This last element is always odd
       (1-st, 3-rd, 5-th, etc.)  To calculate the size of
       nego_table, we have to find this odd NULL.  That is why we
       increment count by 2.
       Note, that even NULL means option TO_BINARY. */
    for (count = 0; ; count = count + 2)
    {
        if (nego_table[count] == END_OF_NEGO_TABLE)
            break;
    }

    /* there are count elements in the nego_table, or (count/2) pairs */
    return count / 2;
} /* NEG_Size_Of_Nego_Table */


/************************************************************************
*
*   FUNCTION
*
*       NU_Install_Negotiate_Options
*
*   DESCRIPTION
*
*       Install all negotiation options.
*
*   INPUTS
*
*       nego_buf        Pointer to the buffer for negotiation option
*       nego_table      Pointer to the negotiation table
*
*   RETURNS
*
*       buffer - nego_table
*
************************************************************************/
INT NU_Install_Negotiate_Options(CHAR *nego_buf, CHAR *nego_table, INT socket)
{
    UINT8  i;
    CHAR   *buffer = nego_buf;
    struct TN_SESSION_STRUCT *tw;
    UINT16 nego_table_size;
    UINT16 command, option;
    NU_SUPERV_USER_VARIABLES

    /* Check for invalid input data. */
    if ( (nego_buf == NU_NULL) || (nego_table == NU_NULL) ||
         (socket < 0) || (socket >= NSOCKETS) )
        return (-1);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* get number of "COMMAND, OPTION" pairs in the nego_table
       (where COMMAND is WILL, or DO, or WONT, etc., and
               OPTION is TO_ECHO, or TO_BINARY, or TO_SGA, etc.) */
    nego_table_size = NEG_Size_Of_Nego_Table(nego_table);

    NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    for (i=0; i<(UINT8)nego_table_size; i++)
    {
        command = 2 * i;
        option  = command + 1;

        switch (nego_table[command])
        {
            case  DO:
                /* we require this option */
                NEG_Set_One_Option(&buffer, TO_DOTEL, (CHAR)nego_table[option]);

                /* Set the state flags */
                if (nego_table[option] == TO_BINARY)
                    tw->nego_Binary |= NEGO_I_DO;
                else if (nego_table[option] == TO_SGA)
                    tw->nego_SGA |= NEGO_I_DO;
                else if (nego_table[option] == TO_ECHO)
                    tw->nego_Echo |= NEGO_I_DO;
                else if ( (nego_table[option] == TO_NAWS) &&
                          (tw->i_am_client != NU_TRUE) )
                    tw->nego_NAWS |= NEGO_I_DO;
                else if ( (nego_table[option] == TO_TERMTYPE) &&
                          (tw->i_am_client != NU_TRUE) )
                    tw->nego_TERMTYPE |= NEGO_I_DO;

                break;

            case  DONT:
                /* we do not require this option, or stop it */
                NEG_Set_One_Option(&buffer, TO_DONTTEL, (CHAR)nego_table[option]);

                /* Clear the state flags */
                if (nego_table[option] == TO_BINARY)
                    tw->nego_Binary |= NEGO_I_DONT;
                else if (nego_table[option] == TO_SGA)
                    tw->nego_SGA |= NEGO_I_DONT;
                else if (nego_table[option] == TO_ECHO)
                    tw->nego_Echo |= NEGO_I_DONT;
                else if ( (nego_table[option] == TO_NAWS) &&
                          (tw->i_am_client != NU_TRUE) )
                    tw->nego_NAWS |= NEGO_I_DONT;
                else if ( (nego_table[option] == TO_TERMTYPE) &&
                          (tw->i_am_client != NU_TRUE) )
                    tw->nego_TERMTYPE |= NEGO_I_DONT;

                break;

            case  WILL:
                /* we support this option */
                NEG_Set_One_Option(&buffer, TO_WILLTEL, (CHAR)nego_table[option]);

                /* Set the state flags */
                if (nego_table[option] == TO_BINARY)
                    tw->nego_Binary |= NEGO_I_WILL;
                else if (nego_table[option] == TO_SGA)
                    tw->nego_SGA |= NEGO_I_WILL;
                else if (nego_table[option] == TO_ECHO)
                    tw->nego_Echo |= NEGO_I_WILL;
                else if ( (nego_table[option] == TO_NAWS) &&
                          (tw->i_am_client == NU_TRUE) )
                    tw->nego_NAWS |= NEGO_I_WILL;
                else if ( (nego_table[option] == TO_TERMTYPE) &&
                          (tw->i_am_client == NU_TRUE) )
                    tw->nego_TERMTYPE |= NEGO_I_WILL;

                break;

            case  WONT:
                /* we do not support this option */
                NEG_Set_One_Option(&buffer, TO_WONTTEL, (CHAR)nego_table[option]);

                /* Clear the state flags */
                if (nego_table[option] == TO_BINARY)
                    tw->nego_Binary |= NEGO_I_WONT;
                else if (nego_table[option] == TO_SGA)
                    tw->nego_SGA |= NEGO_I_WONT;
                else if (nego_table[option] == TO_ECHO)
                    tw->nego_Echo |= NEGO_I_WONT;
                else if ( (nego_table[option] == TO_NAWS) &&
                          (tw->i_am_client == NU_TRUE) )
                    tw->nego_NAWS |= NEGO_I_WONT;
                else if ( (nego_table[option] == TO_TERMTYPE) &&
                          (tw->i_am_client == NU_TRUE) )
                    tw->nego_TERMTYPE |= NEGO_I_WONT;

                break;

            case  DONT_WILL:
                /* we support this option, but the other side does not */
                NEG_Set_One_Option(&buffer, TO_DONTTEL, (CHAR)nego_table[option]);
                NEG_Set_One_Option(&buffer, TO_WILLTEL, (CHAR)nego_table[option]);

                /* Set the state flags */
                if (nego_table[option] == TO_BINARY)
                {
                    tw->nego_Binary |= NEGO_I_DONT;
                    tw->nego_Binary |= NEGO_I_WILL;
                }
                else if (nego_table[option] == TO_SGA)
                {
                    tw->nego_SGA |= NEGO_I_DONT;
                    tw->nego_SGA |= NEGO_I_WILL;
                }
                else if (nego_table[option] == TO_ECHO)
                {
                    tw->nego_Echo |= NEGO_I_DONT;
                    tw->nego_Echo |= NEGO_I_WILL;
                }
                break;

            case  DONT_WONT:
                /* do not do it and we do not support this option */
                NEG_Set_One_Option(&buffer, TO_DONTTEL, (CHAR)nego_table[option]);
                NEG_Set_One_Option(&buffer, TO_WONTTEL, (CHAR)nego_table[option]);

                /* Set the state flags */
                if (nego_table[option] == TO_BINARY)
                {
                    tw->nego_Binary |= NEGO_I_DONT;
                    tw->nego_Binary |= NEGO_I_WONT;
                }
                else if (nego_table[option] == TO_SGA)
                {
                    tw->nego_SGA |= NEGO_I_DONT;
                    tw->nego_SGA |= NEGO_I_WONT;
                }
                else if (nego_table[option] == TO_ECHO)
                {
                    tw->nego_Echo |= NEGO_I_DONT;
                    tw->nego_Echo |= NEGO_I_WONT;
                }
                break;

            case  DO_WONT:
                /* just list all possibilities here */
                NEG_Set_One_Option(&buffer, TO_DOTEL, (CHAR)nego_table[option]);
                NEG_Set_One_Option(&buffer, TO_WONTTEL, (CHAR)nego_table[option]);

                /* Set the state flags */
                if (nego_table[option] == TO_BINARY)
                {
                    tw->nego_Binary |= NEGO_I_DO;
                    tw->nego_Binary |= NEGO_I_WONT;
                }
                else if (nego_table[option] == TO_SGA)
                {
                    tw->nego_SGA |= NEGO_I_DO;
                    tw->nego_SGA |= NEGO_I_WONT;
                }
                else if (nego_table[option] == TO_ECHO)
                {
                    tw->nego_Echo |= NEGO_I_DO;
                    tw->nego_Echo |= NEGO_I_WONT;
                }
                break;

            case  DO_WILL:
                /* we support this option and do not do it, now */
                NEG_Set_One_Option(&buffer, TO_DOTEL, (CHAR)nego_table[option]);
                NEG_Set_One_Option(&buffer, TO_WILLTEL, (CHAR)nego_table[option]);

                /* Set the state flags */
                if (nego_table[option] == TO_BINARY)
                {
                    tw->nego_Binary |= NEGO_I_DO;
                    tw->nego_Binary |= NEGO_I_WILL;
                }
                else if (nego_table[option] == TO_SGA)
                {
                    tw->nego_SGA |= NEGO_I_DO;
                    tw->nego_SGA |= NEGO_I_WILL;
                }
                else if (nego_table[option] == TO_ECHO)
                {
                    tw->nego_Echo |= NEGO_I_DO;
                    tw->nego_Echo |= NEGO_I_WILL;
                }
                break;

            case  NOTHING:
                break;

            default:
                break;
        }
    }

    NU_Release_Semaphore(&TN_Session_Semaphore);
    NU_USER_MODE();    /* return to user mode */

    return (INT)(buffer - nego_buf);
}    /* NU_Install_Negotiate_Options */

/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Start_Negotiate
*
*   DESCRIPTION
*
*       Send the initial negotiations on the network and print
*       the negotiations to the console screen.
*
*   INPUTS
*
*       socket      The socket index of the current connection
*       nego_table  Pointer to the negotiation options table
*
*   RETURNS
*
*       None
*
************************************************************************/
VOID NU_Telnet_Start_Negotiate(INT socket, CHAR *nego_table)
{
    CHAR buf[TELNET_NEGO_OPTIONS_BUF_SIZE];
    INT  data_size;
    NU_SUPERV_USER_VARIABLES

    /* Check for bad input. */
    if ( (nego_table == NU_NULL) || (socket < 0) || (socket > NSOCKETS) )
        return;

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Send the initial telnet negotiations about the telnet options,
        the options formatting and the definition of each command are
        defined in RFC 854, 855.
        NU_Install_Negotiate_Options() will install all the negotiation
        options into buf according to nego_table[], which is declared at
        the beginning of the file. nego_table[] is designed as a basic case,
        if it does not match the need of the user, the user need to change it.
    */
    data_size = NU_Install_Negotiate_Options(buf, nego_table, socket);

    NU_Send(socket, buf, (UINT16)data_size, 0);

    NU_USER_MODE();

}   /* end NU_Telnet_Start_Negotiate */

/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Specific_Negotiate
*
*   DESCRIPTION
*
*       Ask the window size of other party.
*
*   INPUTS
*
*       socket      The socket index of the current connection.
*       option      The option of the negotiation.
*
*   RETURNS
*
*       NU_SUCCESS               -- success
*       NO_NEGO_OPTION_INSTALLED -- the option parameter is not TO_NAWS
*                                   or no nego_NAWS bits are set
*       NU_NO_DATA               -- on timeout
*       <0                       -- socket error
*
*   NOTE
*
*       This function is outdated and is left only for compatibility
*       with previous releases of NET.  It is not recommended to use it.
*       The following sequences of commands supersede this function and
*       should be used instead:
*
*       1) for telnet client:
*
*          STATUS status;
*          status = NU_Telnet_Client_Init_Parameters(client_socket);
*          if (status != NU_SUCCESS)
*              // error initializing client_socket
*          else
*              status =
*                  NU_Telnet_Do_Negotiation(client_socket, client_nego_table);
*
*       2) for telnet server:
*
*          STATUS status;
*          status = NU_Telnet_Server_Init_Parameters(server_socket);
*          if (status != NU_SUCCESS)
*              // error initializing server_socket
*          else
*              status =
*                  NU_Telnet_Do_Negotiation(server_socket, server_nego_table);
*
************************************************************************/
INT NU_Telnet_Specific_Negotiate(INT socket, INT option)
{
    INT    cnt;     /* the number of incoming data bytes */
    UINT8  data[TELNET_BUF_SIZE];
    struct TN_SESSION_STRUCT *tw;
    INT    timeout_countdown = NEG_TIMEOUT;
    FD_SET readfs;
    INT    status = NU_SUCCESS;
    UNSIGNED  sleep_time = TELNET_NEGO_SLEEP_TIME;
    NU_SUPERV_USER_VARIABLES

    /* Check for bad input. */
    if ( (socket < 0) || (socket >= NSOCKETS) )
        return (-1);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if ((tw->nego_NAWS == 0) || (option != TO_NAWS))
    {
        NU_Release_Semaphore(&TN_Session_Semaphore);
        NU_USER_MODE();    /* return to user mode */
        return NO_NEGO_OPTION_INSTALLED;
    }

    NU_FD_Init(&readfs);
    NU_FD_Set(socket, &readfs);

    for (;;)
    {
        if (tw->i_am_client == NU_TRUE)
        {
            /* we are client */

            if ( ((tw->nego_NAWS & NEGO_HE_DO) &&
                  (tw->nego_NAWS & NEGO_I_WILL)) ||
                 ((tw->nego_NAWS & NEGO_HE_DO) &&
                  (tw->nego_NAWS & NEGO_I_WONT)) )
            {
                break;  /* this state set by NEG_Cln_Handle_IAC_DO_NAWS() */
            }
            else if ( ((tw->nego_NAWS & NEGO_I_WILL) &&
                       (tw->nego_NAWS & NEGO_HE_DONT)) ||
                      ((tw->nego_NAWS & NEGO_I_WONT) &&
                       (tw->nego_NAWS & NEGO_HE_DONT)) )
            {
                break;  /* this state set by NEG_Cln_Handle_IAC_DONT_NAWS() */
            }
        }
        else
        {
            /* we are server */

            if ( (tw->nego_NAWS & NEGO_I_DO) &&
                 (tw->nego_NAWS & NEGO_HE_WILL) )
            {
                break;  /* this state set by NEG_Parse_Subnegotiat() */
            }
            else if ( (tw->nego_NAWS & NEGO_HE_WILL) &&
                      (tw->nego_NAWS & NEGO_I_DONT) )
            {
                break;  /* this state set by NEG_Srv_Handle_IAC_WILL_NAWS() */
            }
            else if ( ((tw->nego_NAWS & NEGO_I_DO)  &&
                       (tw->nego_NAWS & NEGO_HE_WONT)) ||
                      ((tw->nego_NAWS & NEGO_I_DONT) &&
                       (tw->nego_NAWS & NEGO_HE_WONT)) )
            {
                 /* these states are set by NEG_Srv_Handle_IAC_WONT_NAWS() */
                break;
            }
        }

        /* check if we timed out  */
        if (timeout_countdown == 0)
        {
            /* we did timeout, return NU_NO_DATA to the caller */
            status = (INT)NU_NO_DATA;
            break;
        }

        timeout_countdown--;    /* decrement timeout_countdown */

        /* check if there is data available for reading on the socket */
        status = NU_Select(socket+1, &readfs, NULL, NULL, NU_NO_SUSPEND);
        if (status == NU_NO_DATA)
        {
            /* there is no data on the socket, wait and try to read again */
            NU_Sleep(sleep_time);

            sleep_time = sleep_time * TELNET_NEGO_INCREASE_SLEEP_TIME_COEFF;
            continue;
        }
        else if (status != NU_SUCCESS)
        {
            /* NU_Select() returned error code, something is wrong,
               return this error code to the caller */
            break;
        }

        /* NU_Select() returned NU_SUCCESS.  It means there are some data
           available for reading on the socket.  So, read these data. */
        cnt = (INT)NU_Recv(socket, (CHAR *)data, (UINT16)TELNET_BUF_SIZE, 0);
        if (cnt<0)
        {
            NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
            status = cnt;
            break;
        }

        /* NU_Telnet_Parse() will analyze data to see if there are telnet
          commands and options, if there is, just do it. */
        NU_Release_Semaphore(&TN_Session_Semaphore);
        NU_Telnet_Parse(socket, data, cnt, 0);
        NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);
    }

    NU_Release_Semaphore(&TN_Session_Semaphore);
    NU_USER_MODE();    /* return to user mode */

    return status;
}   /* NU_Telnet_Specific_Negotiate */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Telnet_TermType_Negotiate
*
*   DESCRIPTION
*
*       Negotiate about terminal type.  Negotiation is over, when
*       nego_TERMTYPE bits are brought to a certain state, or on
*       timeout.
*
*   CALLED BY
*
*        NU_Telnet_Do_Negotiation
*
*   INPUTS
*
*       socket     The socket index of the current connection.
*       data       Buffer with data received from the other host.
*       data_size  Size of data.
*
*   RETURNS
*
*       NU_SUCCESS               -- success
*       NO_NEGO_OPTION_INSTALLED -- no TO_TERMTYPE options to negotiate
*                                   about (no nego_TERMTYPE bits are set,
*                                   because no TO_TERMTYPE options are
*                                   listed in the negotiation table)
*       NU_NO_DATA               -- on timeout
*       <0                       -- socket error
*
************************************************************************/
STATIC
STATUS NEG_Telnet_TermType_Negotiate(INT socket, UINT8* data, UINT16 data_size)
{
    INT    cnt;
    struct TN_SESSION_STRUCT *tw;
    INT    timeout_countdown = NEG_TIMEOUT;
    FD_SET readfs;
    STATUS status = NU_SUCCESS;
    UNSIGNED sleep_time = TELNET_NEGO_SLEEP_TIME;

    NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_TERMTYPE == 0)
    {
        NU_Release_Semaphore(&TN_Session_Semaphore);

        /* no TERMTYPE options installed, nothing to negotiate about */
        return NO_NEGO_OPTION_INSTALLED;
    }

    NU_FD_Init(&readfs);
    NU_FD_Set(socket, &readfs);

    for (;;)
    {
        if (tw->i_am_client == NU_TRUE)
        {
            /* we are client */

            if ( (tw->nego_TERMTYPE & NEGO_HE_REQUIRED_SUBNEG) &&
                 (tw->nego_TERMTYPE & NEGO_I_SENT_SUBNEG) )
            {
                break;  /* this state set by NEG_Parse_Subnegotiat() */
            }
            else if ( ((tw->nego_TERMTYPE & NEGO_HE_DO) &&
                       (tw->nego_TERMTYPE & NEGO_I_WILL)) ||
                      ((tw->nego_TERMTYPE & NEGO_HE_DO) &&
                       (tw->nego_TERMTYPE & NEGO_I_WONT)) )
            {
                break;  /* set by NEG_Cln_Handle_IAC_DO_TERMTYPE() */
            }
            else if ( ((tw->nego_TERMTYPE & NEGO_I_WILL) &&
                       (tw->nego_TERMTYPE & NEGO_HE_DONT)) ||
                      ((tw->nego_TERMTYPE & NEGO_I_WONT) &&
                       (tw->nego_TERMTYPE & NEGO_HE_DONT)) )
            {
                break;  /*  set by NEG_Cln_Handle_IAC_DONT_TERMTYPE() */
            }
        }
        else
        {
            /* we are server */

            if ( (tw->nego_TERMTYPE & NEGO_I_REQUIRED_SUBNEG) &&
                 (tw->nego_TERMTYPE & NEGO_HE_SENT_SUBNEG) )
            {
                break;  /* this state set by NEG_Parse_Subnegotiat() */
            }
            else if ( (tw->nego_TERMTYPE & NEGO_HE_WILL) &&
                      (tw->nego_TERMTYPE & NEGO_I_DONT) )
            {
                break;  /*  set by NEG_Srv_Handle_IAC_WILL_TERMTYPE() */
            }
            else if ( ((tw->nego_TERMTYPE & NEGO_I_DO)   &&
                       (tw->nego_TERMTYPE & NEGO_HE_WONT)) ||
                      ((tw->nego_TERMTYPE & NEGO_I_DONT) &&
                       (tw->nego_TERMTYPE & NEGO_HE_WONT)) )
            {
                /*  set by NEG_Srv_Handle_IAC_WONT_TERMTYPE() */
                break;
            }

        } /* if (tw->i_am_client == NU_TRUE) */

        /* check if we timed out  */
        if (timeout_countdown == 0)
        {
            /* we did timeout, return NU_NO_DATA to the caller */
            status = NU_NO_DATA;
            break;
        }

        timeout_countdown--;    /* decrement timeout_countdown */

        /* check if there is data available for reading on the socket */
        status = NU_Select(socket+1, &readfs, NULL, NULL, NU_NO_SUSPEND);
        if (status == NU_NO_DATA)
        {
            /* there is no data on the socket, wait and try to read again */
            NU_Sleep(sleep_time);

            sleep_time = sleep_time * TELNET_NEGO_INCREASE_SLEEP_TIME_COEFF;
            continue;
        }
        else if (status != NU_SUCCESS)
        {
            /* NU_Select() returned error code, something is wrong,
               return this error code to the caller */
            break;
        }

        /* NU_Select() returned NU_SUCCESS.  It means there are some data
           available for reading on the socket.  So, read these data. */
        cnt = (INT)NU_Recv(socket, (CHAR *)data, data_size, 0);
        if (cnt<0)
        {
            NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
            status = (STATUS)cnt;
            break;
        }

        /* NU_Telnet_Parse() will analyze data to see if there are telnet
          commands and options, if there is, just do it. */
        NU_Release_Semaphore(&TN_Session_Semaphore);
        NU_Telnet_Parse(socket, data, cnt, 0);
        NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);
    }

    NU_Release_Semaphore(&TN_Session_Semaphore);
    return status;
}   /* NEG_Telnet_TermType_Negotiate */

/************************************************************************
*
*   FUNCTION
*
*       NEG_Telnet_NAWS_Negotiate
*
*   DESCRIPTION
*
*       Negotiate about window size.  Negotiation is over, when
*       nego_NAWS bits are brought to a certain state, or on
*       timeout.
*
*   CALLED BY
*
*        NU_Telnet_Do_Negotiation
*
*   INPUTS
*
*       socket     The socket index of the current connection.
*       data       Buffer with data received from the other host.
*       data_size  Size of data.
*
*   RETURNS
*
*       NU_SUCCESS               -- success
*       NO_NEGO_OPTION_INSTALLED -- no TO_NAWS options to negotiate
*                                   about (no nego_NAWS bits are set,
*                                   because no TO_NAWS options are
*                                   listed in the negotiation table)
*       NU_NO_DATA               -- on timeout
*       <0                       -- socket error
*
************************************************************************/
STATIC
STATUS NEG_Telnet_NAWS_Negotiate(INT socket, UINT8* data, UINT16 data_size)
{
    INT    cnt;
    struct TN_SESSION_STRUCT *tw;
    INT    timeout_countdown = NEG_TIMEOUT;
    FD_SET readfs;
    STATUS status = NU_SUCCESS;
    UNSIGNED sleep_time = TELNET_NEGO_SLEEP_TIME;

    NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_NAWS == 0)
    {
        NU_Release_Semaphore(&TN_Session_Semaphore);

        /* no NAWS options installed, nothing to negotiate about */
        return NO_NEGO_OPTION_INSTALLED;
    }

    NU_FD_Init(&readfs);
    NU_FD_Set(socket, &readfs);

    for (;;)
    {
        if (tw->i_am_client == NU_TRUE)
        {
            /* we are client */

            if ( ((tw->nego_NAWS & NEGO_HE_DO) &&
                  (tw->nego_NAWS & NEGO_I_WILL)) ||
                 ((tw->nego_NAWS & NEGO_HE_DO) &&
                  (tw->nego_NAWS & NEGO_I_WONT)) )
            {
                break;  /* this state set by NEG_Cln_Handle_IAC_DO_NAWS() */
            }
            else if ( ((tw->nego_NAWS & NEGO_I_WILL) &&
                       (tw->nego_NAWS & NEGO_HE_DONT)) ||
                      ((tw->nego_NAWS & NEGO_I_WONT) &&
                       (tw->nego_NAWS & NEGO_HE_DONT)) )
            {
                break;  /* set by NEG_Cln_Handle_IAC_DONT_NAWS() */
            }
        }
        else
        {
            /* we are server */

            if ( (tw->nego_NAWS & NEGO_I_DO) &&
                 (tw->nego_NAWS & NEGO_HE_WILL) )
            {
                break;  /* this state set by NEG_Parse_Subnegotiat() */
            }
            else if ( (tw->nego_NAWS & NEGO_HE_WILL) &&
                      (tw->nego_NAWS & NEGO_I_DONT) )
            {
                break;  /*  set by NEG_Srv_Handle_IAC_WILL_NAWS() */
            }
            else if ( ((tw->nego_NAWS & NEGO_I_DO)   &&
                       (tw->nego_NAWS & NEGO_HE_WONT)) ||
                      ((tw->nego_NAWS & NEGO_I_DONT) &&
                       (tw->nego_NAWS & NEGO_HE_WONT)) )
            {
                /* these states are set by NEG_Srv_Handle_IAC_WONT_NAWS() */
                break;
            }
        }

        /* check if we timed out  */
        if (timeout_countdown == 0)
        {
            /* we did timeout, return NU_NO_DATA to the caller */
            status = NU_NO_DATA;
            break;
        }

        timeout_countdown--;    /* decrement timeout_countdown */

        /* check if there is data available for reading on the socket */
        status = NU_Select(socket+1, &readfs, NULL, NULL, NU_NO_SUSPEND);
        if (status == NU_NO_DATA)
        {
            /* there is no data on the socket, wait and try to read again */
            NU_Sleep(sleep_time);

            sleep_time = sleep_time * TELNET_NEGO_INCREASE_SLEEP_TIME_COEFF;
            continue;
        }
        else if (status != NU_SUCCESS)
        {
            /* NU_Select() returned error code, something is wrong,
               return this error code to the caller */
            break;
        }

        /* NU_Select() returned NU_SUCCESS.  It means there are some data
           available for reading on the socket.  So, read these data. */
        cnt = (INT)NU_Recv(socket, (CHAR *)data, data_size, 0);
        if (cnt<0)
        {
            NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
            status = (STATUS)cnt;
            break;
        }

        /* NU_Telnet_Parse() will analyze data to see if there are telnet
          commands and options, if there is, just do it. */
        NU_Release_Semaphore(&TN_Session_Semaphore);
        NU_Telnet_Parse(socket, data, cnt, 0);
        NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);
    }

    NU_Release_Semaphore(&TN_Session_Semaphore);
    return status;
}   /* NEG_Telnet_NAWS_Negotiate */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Telnet_SGA_Negotiate
*
*   DESCRIPTION
*
*       Negotiate about Suppress Go Ahead (SGA) option.
*       Negotiation is over, when nego_SGA bits are brought to a
*       certain state, or on timeout.
*
*   CALLED BY
*
*        NU_Telnet_Do_Negotiation
*
*   INPUTS
*
*       socket     The socket index of the current connection.
*       data       Buffer with data received from the other host.
*       data_size  Size of data.
*
*   RETURNS
*
*       NU_SUCCESS               -- success
*       NO_NEGO_OPTION_INSTALLED -- no TO_SGA options to negotiate
*                                   about (no nego_SGA bits are set,
*                                   because no TO_SGA options are
*                                   listed in the negotiation table)
*       NU_NO_DATA               -- on timeout
*       <0                       -- socket error
*
************************************************************************/
STATIC
STATUS NEG_Telnet_SGA_Negotiate(INT socket, UINT8* data, UINT16 data_size)
{
    INT    cnt;
    struct TN_SESSION_STRUCT *tw;
    INT    timeout_countdown = NEG_TIMEOUT;
    FD_SET readfs;
    STATUS status = NU_SUCCESS;
    UNSIGNED sleep_time = TELNET_NEGO_SLEEP_TIME;

    NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_SGA == 0)
    {
        NU_Release_Semaphore(&TN_Session_Semaphore);

        /* no SGA options installed, nothing to negotiate about */
        return NO_NEGO_OPTION_INSTALLED;
    }

    NU_FD_Init(&readfs);
    NU_FD_Set(socket, &readfs);

    for (;;)
    {
        if (((tw->nego_SGA & NEGO_I_DO)    && (tw->nego_SGA & NEGO_HE_WILL)) ||
            ((tw->nego_SGA & NEGO_I_WILL)  && (tw->nego_SGA & NEGO_HE_DO))   ||
             (tw->nego_SGA & NEGO_I_WONT)  || (tw->nego_SGA & NEGO_I_DONT)   ||
             (tw->nego_SGA & NEGO_HE_WONT) || (tw->nego_SGA & NEGO_HE_DONT))
        {
            break;
        }

        /* check if we timed out  */
        if (timeout_countdown == 0)
        {
            /* we did timeout, return NU_NO_DATA to the caller */
            status = NU_NO_DATA;
            break;
        }

        timeout_countdown--;    /* decrement timeout_countdown */

        /* check if there is data available for reading on the socket */
        status = NU_Select(socket+1, &readfs, NULL, NULL, NU_NO_SUSPEND);
        if (status == NU_NO_DATA)
        {
            /* there is no data on the socket, wait and try to read again */
            NU_Sleep(sleep_time);

            sleep_time = sleep_time * TELNET_NEGO_INCREASE_SLEEP_TIME_COEFF;
            continue;
        }
        else if (status != NU_SUCCESS)
        {
            /* NU_Select() returned error code, something is wrong,
               return this error code to the caller */
            break;
        }

        /* NU_Select() returned NU_SUCCESS.  It means there are some data
           available for reading on the socket.  So, read these data. */
        cnt = (INT)NU_Recv(socket, (CHAR *)data, data_size, 0);
        if (cnt<0)
        {
            NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
            status = (STATUS)cnt;
            break;
        }

        /* NU_Telnet_Parse() will analyze data to see if there are telnet
          commands and options, if there is, just do it. */
        NU_Release_Semaphore(&TN_Session_Semaphore);
        NU_Telnet_Parse(socket, data, cnt, 0);
        NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);
    }

    NU_Release_Semaphore(&TN_Session_Semaphore);
    return status;
}   /* NEG_Telnet_SGA_Negotiate */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Telnet_Echo_Negotiate
*
*   DESCRIPTION
*
*       Negotiate about Echo option.
*       Negotiation is over, when nego_Echo bits are brought to a
*       certain state, or on timeout.
*
*   CALLED BY
*
*        NU_Telnet_Do_Negotiation
*
*   INPUTS
*
*       socket     The socket index of the current connection
*       data       Buffer with data received from the other host
*       data_size  Size of data
*
*   RETURNS
*
*       NU_SUCCESS               -- success
*       NO_NEGO_OPTION_INSTALLED -- no TO_ECHO options to negotiate
*                                   about (no nego_Echo bits are set,
*                                   because no TO_ECHO options are
*                                   listed in the negotiation table)
*       NU_NO_DATA               -- on timeout
*       <0                       -- socket error
*
************************************************************************/
STATIC
STATUS NEG_Telnet_Echo_Negotiate(INT socket, UINT8* data, UINT16 data_size)
{
    INT    cnt;
    struct TN_SESSION_STRUCT *tw;
    INT    timeout_countdown = NEG_TIMEOUT;
    FD_SET readfs;
    STATUS status = NU_SUCCESS;
    UNSIGNED sleep_time = TELNET_NEGO_SLEEP_TIME;

    NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_Echo == 0)
    {
        NU_Release_Semaphore(&TN_Session_Semaphore);

        /* no ECHO options installed, nothing to negotiate about */
        return NO_NEGO_OPTION_INSTALLED;
    }

    NU_FD_Init(&readfs);
    NU_FD_Set(socket, &readfs);

    for (;;)
    {
        if (((tw->nego_Echo & NEGO_I_DO)   && (tw->nego_Echo & NEGO_HE_WILL))||
            ((tw->nego_Echo & NEGO_I_WILL) && (tw->nego_Echo & NEGO_HE_DO))  ||
             (tw->nego_Echo & NEGO_I_WONT) || (tw->nego_Echo & NEGO_I_DONT)  ||
             (tw->nego_Echo & NEGO_HE_WONT)|| (tw->nego_Echo & NEGO_HE_DONT))
        {
            break;
        }

        /* check if we timed out  */
        if (timeout_countdown == 0)
        {
            /* we did timeout, return NU_NO_DATA to the caller */
            status = NU_NO_DATA;
            break;
        }

        timeout_countdown--;    /* decrement timeout_countdown */

        /* check if there is data available for reading on the socket */
        status = NU_Select(socket+1, &readfs, NULL, NULL, NU_NO_SUSPEND);
        if (status == NU_NO_DATA)
        {
            /* there is no data on the socket, wait and try to read again */
            NU_Sleep(sleep_time);

            sleep_time = sleep_time * TELNET_NEGO_INCREASE_SLEEP_TIME_COEFF;
            continue;
        }
        else if (status != NU_SUCCESS)
        {
            /* NU_Select() returned error code, something is wrong,
               return this error code to the caller */
            break;
        }

        /* NU_Select() returned NU_SUCCESS.  It means there are some data
           available for reading on the socket.  So, read these data. */
        cnt = (INT)NU_Recv(socket, (CHAR *)data, data_size, 0);
        if (cnt<0)
        {
            NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
            status = (STATUS)cnt;
            break;
        }

        /* NU_Telnet_Parse() will analyze data to see if there are telnet
          commands and options, if there is, just do it. */
        NU_Release_Semaphore(&TN_Session_Semaphore);
        NU_Telnet_Parse(socket, data, cnt, 0);
        NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);
    }

    NU_Release_Semaphore(&TN_Session_Semaphore);
    return status;
} /* NEG_Telnet_Echo_Negotiate */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Telnet_Binary_Negotiate
*
*   DESCRIPTION
*
*       Negotiate about Transmit-Binary option.
*       Negotiation is over, when nego_Binary bits are brought to a
*       certain state, or on timeout.
*
*   CALLED BY
*
*        NU_Telnet_Do_Negotiation
*
*   INPUTS
*
*       socket     The socket index of the current connection
*       data       Buffer with data received from the other host
*       data_size  Size of data
*
*   RETURNS
*
*       NU_SUCCESS               -- success
*       NO_NEGO_OPTION_INSTALLED -- no TO_BINARY options to negotiate
*                                   about (no nego_Binary bits are set,
*                                   because no TO_BINARY options are
*                                   listed in the negotiation table)
*       NU_NO_DATA               -- on timeout
*       <0                       -- socket error
*
************************************************************************/
STATIC
STATUS NEG_Telnet_Binary_Negotiate(INT socket, UINT8* data, UINT16 data_size)
{
    INT    cnt;
    struct TN_SESSION_STRUCT *tw;
    INT    timeout_countdown = NEG_TIMEOUT;
    FD_SET readfs;
    STATUS status = NU_SUCCESS;
    UNSIGNED sleep_time = TELNET_NEGO_SLEEP_TIME;

    NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_Binary == 0)
    {
        NU_Release_Semaphore(&TN_Session_Semaphore);

        /* no Binary options installed, nothing to negotiate about */
        return NO_NEGO_OPTION_INSTALLED;
    }

    NU_FD_Init(&readfs);
    NU_FD_Set(socket, &readfs);

    for (;;)
    {
        if (((tw->nego_Binary & NEGO_I_DO) && (tw->nego_Binary & NEGO_HE_WILL))
            ||
            ((tw->nego_Binary & NEGO_I_WILL) && (tw->nego_Binary & NEGO_HE_DO))
            || (tw->nego_Binary & NEGO_I_WONT)
            || (tw->nego_Binary & NEGO_I_DONT)
            || (tw->nego_Binary & NEGO_HE_WONT)
            || (tw->nego_Binary & NEGO_HE_DONT) )
        {
            break;
        }

        /* check if we timed out  */
        if (timeout_countdown == 0)
        {
            /* we did timeout, return NU_NO_DATA to the caller */
            status = NU_NO_DATA;
            break;
        }

        timeout_countdown--;    /* decrement timeout_countdown */

        /* check if there is data available for reading on the socket */
        status = NU_Select(socket+1, &readfs, NULL, NULL, NU_NO_SUSPEND);
        if (status == NU_NO_DATA)
        {
            /* there is no data on the socket, wait and try to read again */
            NU_Sleep(sleep_time);

            sleep_time = sleep_time * TELNET_NEGO_INCREASE_SLEEP_TIME_COEFF;
            continue;
        }
        else if (status != NU_SUCCESS)
        {
            /* NU_Select() returned error code, something is wrong,
               return this error code to the caller */
            break;
        }

        /* NU_Select() returned NU_SUCCESS.  It means there are some data
           available for reading on the socket.  So, read these data. */
        cnt = (INT)NU_Recv(socket, (CHAR *)data, data_size, 0);
        if (cnt<0)
        {
            NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
            status = (STATUS)cnt;
            break;
        }

        /* NU_Telnet_Parse() will analyze data to see if there are telnet
          commands and options, if there is, just do it. */
        NU_Release_Semaphore(&TN_Session_Semaphore);
        NU_Telnet_Parse(socket, data, cnt, 0);
        NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);
    }

    NU_Release_Semaphore(&TN_Session_Semaphore);
    return status;
} /* NEG_Telnet_Binary_Negotiate */


/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Do_Negotiation
*
*   DESCRIPTION
*
*       Telnet options negotiation driver.  Installs negotiation
*       options, sends them to the other side, and calls functions
*       responsible for negotiation of telnet options.
*
*   INPUTS
*
*       socket      The socket index of the current connection
*       nego_table  client_nego_table or server_nego_table
*
*   RETURNS
*
*        NU_SUCCESS  -- finished without errors.
*        <0          -- error reading from socket.
*
*   EXAMPLES OF USAGE
*
*        1) for telnet client:
*
*           STATUS status;
*           status = NU_Telnet_Client_Init_Parameters(client_socket);
*           if (status != NU_SUCCESS)
*               // error initializing client_socket
*           else
*               status =
*                   NU_Telnet_Do_Negotiation(client_socket, client_nego_table);
*
*        2) for telnet server:
*
*           STATUS status;
*           status = NU_Telnet_Server_Init_Parameters(server_socket);
*           if (status != NU_SUCCESS)
*               // error initializing server_socket
*           else
*               status =
*                   NU_Telnet_Do_Negotiation(server_socket, server_nego_table);
*
************************************************************************/
STATUS NU_Telnet_Do_Negotiation(INT socket, CHAR *nego_table)
{
    UINT8  buf[TELNET_BUF_SIZE];
    STATUS status;

    CHAR  telnet_nego_options[TELNET_NEGO_OPTIONS_BUF_SIZE];
    INT   data_size;
    NU_SUPERV_USER_VARIABLES

    /* Check for bad input. */
    if ( (nego_table == NU_NULL) || (socket < 0) || (socket >= NSOCKETS) )
        return (-1);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* prepare options in the nego_table to be sent to the other side of telnet
       connection.   NU_Install_Negotiate_Options() precedes every option with
       IAC escape code, and copies IAC code + option code to telnet_options. */
    data_size =
         NU_Install_Negotiate_Options(telnet_nego_options, nego_table, socket);

    /* send this side's telnet negotiation options to the other side */
    NU_Send(socket, telnet_nego_options, (UINT16)data_size, 0);

    /* negotiate about Terminal type (TERMTYPE) */
    status = NEG_Telnet_TermType_Negotiate(socket, buf,
                                           (UINT16)TELNET_BUF_SIZE);
    if ( (status != NU_SUCCESS) && (status != NU_NO_DATA)
                                && (status != NO_NEGO_OPTION_INSTALLED) )
    {
        NU_USER_MODE();    /* return to user mode */
        return status;
    }

    /* Negotiate About Window Size (NAWS) */
    status = NEG_Telnet_NAWS_Negotiate(socket, buf, (UINT16)TELNET_BUF_SIZE);
    if ( (status != NU_SUCCESS) && (status != NU_NO_DATA)
                                && (status != NO_NEGO_OPTION_INSTALLED) )
    {
        NU_USER_MODE();    /* return to user mode */
        return status;
    }

    /* negotiate about SGA (Suppress Go Ahead) */
    status = NEG_Telnet_SGA_Negotiate(socket, buf, (UINT16)TELNET_BUF_SIZE);
    if ( (status != NU_SUCCESS) && (status != NU_NO_DATA)
                                && (status != NO_NEGO_OPTION_INSTALLED) )
    {
        NU_USER_MODE();    /* return to user mode */
        return status;
    }

    /* negotiate about echo */
    status = NEG_Telnet_Echo_Negotiate(socket, buf, (UINT16)TELNET_BUF_SIZE);
    if ( (status != NU_SUCCESS) && (status != NU_NO_DATA)
                                && (status != NO_NEGO_OPTION_INSTALLED) )
    {
        NU_USER_MODE();    /* return to user mode */
        return status;
    }

    /* negotiate about Transmit-Binary */
    status = NEG_Telnet_Binary_Negotiate(socket, buf, (UINT16)TELNET_BUF_SIZE);
    if ( (status != NU_SUCCESS) && (status != NU_NO_DATA)
                                && (status != NO_NEGO_OPTION_INSTALLED) )
    {
        NU_USER_MODE();    /* return to user mode */
        return status;
    }

    NU_USER_MODE();    /* return to user mode */

    return NU_SUCCESS;
}   /* NU_Telnet_Do_Negotiation */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Parse_Subnegotiat
*
*   DESCRIPTION
*
*       Parse the telnet sub-negotiations read into the parsedat array.
*
*   INPUTS
*
*       socket      The socket index of the current connection
*       parsedat    Pointer to the sub-stream of sub-negotiation
*
*   RETURNS
*
*       None
*
************************************************************************/
VOID NEG_Parse_Subnegotiat(INT socket, UINT8 *parsedat)
{
    struct TN_SESSION_STRUCT *tw;
    CHAR   temp_buf[11];

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    /* the sub negotiation is about terminal type */
    if (parsedat[0]==TO_TERMTYPE)
    {
        if ( (tw->i_am_client == NU_TRUE) && (parsedat[1] == TO_SEND) &&
             (tw->nego_TERMTYPE & NEGO_HE_DO) &&
             (tw->nego_TERMTYPE & NEGO_I_WILL) )
        {
            /* The term.type information may not be sent spontaneously, so
               only send it if we are client, if we support this option
               (NEGO_I_WILL is on), and the server required us to send it
               (NEGO_HE_DO is on).  The server may send more than one
               "IAC SB TERMTYPE SEND IAC SE" request to get a list of
               term.types supported by the client
               (see http://www.faqs.org/rfcs/rfc1091.html).
               We only support one.  So make sure that we reply to all
               "TERMTYPE SEND" requests sent by the server.
               (The server should stop querying the client about the term.types
               once the server receives the same response 2 consecutive times.)
            */

            /* tell other part what terminal type we will emulate, it is VT100;
               the users can change it to match their requirements */

            temp_buf[0]  = (CHAR) TO_IAC;
            temp_buf[1]  = (CHAR) TO_SB;
            temp_buf[2]  = (CHAR) TO_TERMTYPE;
            temp_buf[3]  = (CHAR) TO_IS;
            strcpy(&temp_buf[4], VT100STR);
            temp_buf[9]  = (CHAR) TO_IAC;
            temp_buf[10] = (CHAR) TO_SE;

            NU_Send(socket, temp_buf, 11, 0);

            /* after told him mine, set ours to be VT100 as default */
            tw->termtype = VT100TYPE;

            /* set nego_TERMTYPE bits */
            tw->nego_TERMTYPE |= NEGO_HE_REQUIRED_SUBNEG;
            tw->nego_TERMTYPE |= NEGO_I_SENT_SUBNEG;
        }
        else if ( (tw->i_am_client != NU_TRUE) && (parsedat[1]==TO_IS) &&
                  (tw->nego_TERMTYPE & NEGO_I_DO) &&
                  (tw->nego_TERMTYPE & NEGO_HE_WILL) &&
                  (tw->nego_TERMTYPE & NEGO_I_REQUIRED_SUBNEG) )
        {

            /* if parsedat[1]==TO_IS the client informs us (server) about
               his terminal emulation type. The users can change it to
               match his requirement, here we only handle vt100,
               as an example */

            if (NU_STRICMP((char *)&parsedat[2], VT100STR)==0)
            {
                tw->termtype = VT100TYPE;
            }
            else if (NU_STRICMP((char *)&parsedat[2], ANSISTR)==0)
            {
                tw->termtype = ANSITYPE;
            }
            else
            {
                tw->termtype = UNKNOWNTYPE;
            }

            tw->nego_TERMTYPE |= NEGO_HE_SENT_SUBNEG;

        } /* end else if (parsedat[1]==0) */

    } /* if (parsedat[0]==TO_TERMTYPE)*/

    /* received Negotiate About Window Size from other side (client),
        save the size into the global data structure containing
        the telnet session parameters  */
    if ( (parsedat[0]==TO_NAWS) && (tw->i_am_client != NU_TRUE) )
    {
        tw->width  = parsedat[2];
        tw->height = parsedat[4];
        tw->nego_NAWS |= NEGO_HE_WILL;
    }

    return;
}   /* end NEG_Parse_Subnegotiat */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Handle_IAC_WILL_SGA
*
*   DESCRIPTION
*
*       Internal function called by NU_Telnet_Parse() to process the
*       "IAC WILL SGA" command received from the telnet client or
*       server.  This function can be called by both the server and
*       the client.  nego_SGA bits set by this function are checked by
*       NEG_Telnet_SGA_Negotiate() to see if negotiation about the SGA is
*       over.
*
*       This function is a part of the Telnet Suppress Go Ahead Option
*       implementation (see http://www.faqs.org/rfcs/rfc858.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         NU_SUCCESS
*
************************************************************************/
STATIC STATUS NEG_Handle_IAC_WILL_SGA(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_SGA & NEGO_I_DO)
    {
        /* NEGO_I_DO is on, it means that we sent "IAC DO SGA" before
           and the other side replied "IAC WILL SGA".
           Set NEGO_HE_WILL flag.  Don't send anything back. */

        tw->nego_SGA |= NEGO_HE_WILL;
    }
    else
    {
        /* The other side sent "IAC WILL SGA".  But NEGO_I_DO flag
           on this side is off. Send "IAC DONT SGA" if we didn't send
           it before, and set NEGO_I_DONT and NEGO_HE_WILL flags. */

        tw->nego_SGA |= NEGO_HE_WILL;

        if (!(tw->nego_SGA & NEGO_I_DONT))
        {
            temp_buf[0] = (CHAR) TO_IAC;
            temp_buf[1] = (CHAR) TO_DONTTEL;
            temp_buf[2] = (CHAR) TO_SGA;

            NU_Send(socket, temp_buf, 3, 0);

            tw->nego_SGA |= NEGO_I_DONT;
        }
    }

    return NU_SUCCESS;
} /* NEG_Handle_IAC_WILL_SGA */

/************************************************************************
*
*   FUNCTION
*
*       NEG_Handle_IAC_WONT_SGA
*
*   DESCRIPTION
*
*       Internal function called by NU_Telnet_Parse() to process the
*       "IAC WONT SGA" command received from the telnet client or
*       server.  This function can be called by both the server and
*       the client.  nego_SGA bits set by this function are checked by
*       NEG_Telnet_SGA_Negotiate() to see if negotiation about the SGA is
*       over.
*
*       This function is a part of the Telnet Suppress Go Ahead Option
*       implementation (see http://www.faqs.org/rfcs/rfc858.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         NU_SUCCESS
*
************************************************************************/
STATIC STATUS NEG_Handle_IAC_WONT_SGA(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_SGA & NEGO_I_DO)
    {
        /* NEGO_I_DO is on, it means that we sent "IAC DO SGA" before
           and the other side replied "IAC WONT SGA".
           Set NEGO_HE_WONT flag.  Don't send anything back. */

        tw->nego_SGA |= NEGO_HE_WONT;
    }
    else if (tw->nego_SGA & NEGO_I_DONT)
    {
        /* We sent "IAC DONT SGA" and the other side agreed not to SGA by
           replying "IAC WONT SGA".  Set NEGO_HE_WONT flag.
           Don't send anything back. */

        tw->nego_SGA |= NEGO_HE_WONT;
    }
    else if ( !(tw->nego_SGA & NEGO_I_DO) && !(tw->nego_SGA & NEGO_I_DONT) )
    {
        /* We sent neither "IAC DO SGA", nor "IAC DONT SGA" before, but received
           "IAC WONT SGA" from the other side.  This means the other side
           does not want to negotiate about SGA.
           Send "IAC DONT SGA". */

        temp_buf[0] = (CHAR) TO_IAC;
        temp_buf[1] = (CHAR) TO_DONTTEL;
        temp_buf[2] = (CHAR) TO_SGA;

        NU_Send(socket, temp_buf, 3, 0);

        tw->nego_SGA |= NEGO_HE_WONT;
        tw->nego_SGA |= NEGO_I_DONT;
    }

    return NU_SUCCESS;
} /* NEG_Handle_IAC_WONT_SGA */

/************************************************************************
*
*   FUNCTION
*
*       NEG_Handle_IAC_DO_SGA
*
*   DESCRIPTION
*
*       Internal function called by NU_Telnet_Parse() to process the
*       "IAC DO SGA" command received from the telnet client or
*       server.  This function can be called by both the server and
*       the client.  nego_SGA bits set by this function are checked by
*       NEG_Telnet_SGA_Negotiate() to see if negotiation about the SGA is
*       over.
*
*       This function is a part of the Telnet Suppress Go Ahead Option
*       implementation (see http://www.faqs.org/rfcs/rfc858.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         NU_SUCCESS
*
************************************************************************/
STATIC STATUS NEG_Handle_IAC_DO_SGA(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_SGA & NEGO_I_WILL)
    {
        /* NEGO_I_WILL is on, it means that we sent "IAC WILL SGA" before
           and the other side replied "IAC DO SGA".
           Set NEGO_HE_DO flag.  Don't send anything back. */

        tw->nego_SGA |= NEGO_HE_DO;
    }
    else
    {
        /* The other side sent "IAC DO SGA".  But NEGO_I_WILL flag
           on this side is off. Send "IAC WONT SGA" if we didn't send
           it before, and set NEGO_I_WONT and NEGO_HE_DO flags. */

        tw->nego_SGA |= NEGO_HE_DO;

        if (!(tw->nego_SGA & NEGO_I_WONT))
        {
            temp_buf[0] = (CHAR) TO_IAC;
            temp_buf[1] = (CHAR) TO_WONTTEL;
            temp_buf[2] = (CHAR) TO_SGA;

            NU_Send(socket, temp_buf, 3, 0);

            tw->nego_SGA |= NEGO_I_WONT;
        }
    }

    return NU_SUCCESS;
} /* NEG_Handle_IAC_DO_SGA */

/************************************************************************
*
*   FUNCTION
*
*       NEG_Handle_IAC_DONT_SGA
*
*   DESCRIPTION
*
*       Internal function called by NU_Telnet_Parse() to process the
*       "IAC DONT SGA" command received from the telnet client or
*       server.  This function can be called by both the server and
*       the client.  nego_SGA bits set by this function are checked by
*       NEG_Telnet_SGA_Negotiate() to see if negotiation about the SGA is
*       over.
*
*       This function is a part of the Telnet Suppress Go Ahead Option
*       implementation (see http://www.faqs.org/rfcs/rfc858.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         NU_SUCCESS
*
************************************************************************/
STATIC STATUS NEG_Handle_IAC_DONT_SGA(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_SGA & NEGO_I_WILL)
    {
        /* NEGO_I_WILL is on, it means that we sent "IAC WILL SGA" before
           and the other side replied "IAC DONT SGA".
           Set NEGO_HE_DONT flag.  Don't send anything back. */

        tw->nego_SGA |= NEGO_HE_DONT;
    }
    else if (tw->nego_SGA & NEGO_I_WONT)
    {
        /* We sent "IAC WONT SGA" and the other side agreed not to SGA by
           replying "IAC DONT SGA".  Set NEGO_HE_DONT flag.
           Don't send anything back. */

        tw->nego_SGA |= NEGO_HE_DONT;
    }
    else if ( !(tw->nego_SGA & NEGO_I_WILL) && !(tw->nego_SGA & NEGO_I_WONT) )
    {
        /* We sent neither "IAC WILL SGA", nor "IAC WONT SGA" before, but received
           "IAC DONT SGA" from the other side.  This means the other side
           does not want to negotiate about SGA.
           Send "IAC WONT SGA". */

        temp_buf[0] = (CHAR) TO_IAC;
        temp_buf[1] = (CHAR) TO_WONTTEL;
        temp_buf[2] = (CHAR) TO_SGA;

        NU_Send(socket, temp_buf, 3, 0);

        tw->nego_SGA |= NEGO_HE_DONT;
        tw->nego_SGA |= NEGO_I_WONT;
    }

    return NU_SUCCESS;
} /* NEG_Handle_IAC_DONT_SGA */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Handle_IAC_WILL_ECHO
*
*   DESCRIPTION
*
*       Internal function called by NU_Telnet_Parse() to process the
*       "IAC WILL ECHO" command received from the telnet client or
*       server.  This function can be called by both the server and
*       the client.  nego_Echo bits set by this function are checked by
*       NEG_Telnet_Echo_Negotiate() to see if negotiation about the
*       echo option is over.
*
*       This function is a part of the Telnet Echo Option
*       implementation (see http://www.faqs.org/rfcs/rfc857.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         NU_SUCCESS
*
************************************************************************/
STATIC STATUS NEG_Handle_IAC_WILL_ECHO(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_Echo & NEGO_I_DO)
    {
        /* NEGO_I_DO is on, it means that we sent "IAC DO ECHO" before
           and the other side replied "IAC WILL ECHO".
           Set NEGO_HE_WILL flag.  Don't send anything back. */

        tw->nego_Echo |= NEGO_HE_WILL;
    }
    else
    {
        /* The other side sent "IAC WILL ECHO".  But NEGO_I_DO flag
           on this side is off. Send "IAC DONT ECHO" if we didn't send
           it before, and set NEGO_I_DONT and NEGO_HE_WILL flags. */

        temp_buf[0] = (CHAR) TO_IAC;
        temp_buf[1] = (CHAR) TO_DONTTEL;
        temp_buf[2] = (CHAR) TO_ECHO;

        NU_Send(socket, temp_buf, 3, 0);

        tw->nego_Echo |= NEGO_I_DONT;
        tw->nego_Echo |= NEGO_HE_WILL;
    }

    return NU_SUCCESS;
} /* NEG_Handle_IAC_WILL_ECHO */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Handle_IAC_WONT_ECHO
*
*   DESCRIPTION
*
*       Internal function called by NU_Telnet_Parse() to process the
*       "IAC WONT ECHO" command received from the telnet client or
*       server.  This function can be called by both the server and
*       the client.  nego_Echo bits set by this function are checked by
*       NEG_Telnet_Echo_Negotiate() to see if negotiation about the
*       echo option is over.
*
*       This function is a part of the Telnet Echo Option
*       implementation (see http://www.faqs.org/rfcs/rfc857.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         NU_SUCCESS
*
************************************************************************/
STATIC STATUS NEG_Handle_IAC_WONT_ECHO(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_Echo & NEGO_I_DO)
    {
        /* NEGO_I_DO is on, it means that we sent "IAC DO ECHO" before
           and the other side replied "IAC WONT ECHO".
           Set NEGO_HE_WONT flag.  Don't send anything back. */

        tw->nego_Echo |= NEGO_HE_WONT;
    }
    else if (tw->nego_Echo & NEGO_I_DONT)
    {
        /* We sent "IAC DONT ECHO" and the other side agreed not to ECHO by
           replying "IAC WONT ECHO".  Set NEGO_HE_WONT flag.
           Don't send anything back. */

        tw->nego_Echo |= NEGO_HE_WONT;
    }
    else if ( !(tw->nego_Echo & NEGO_I_DO) && !(tw->nego_Echo & NEGO_I_DONT) )
    {
        /* We sent neither "IAC DO ECHO", nor "IAC DONT ECHO" before, but received
           "IAC WONT ECHO" from the other side.  This means the other side
           does not want to negotiate about ECHO.
           Send "IAC DONT ECHO". */

        temp_buf[0] = (CHAR) TO_IAC;
        temp_buf[1] = (CHAR) TO_DONTTEL;
        temp_buf[2] = (CHAR) TO_ECHO;

        NU_Send(socket, temp_buf, 3, 0);

        tw->nego_Echo |= NEGO_HE_WONT;
        tw->nego_Echo |= NEGO_I_DONT;
    }

    return NU_SUCCESS;
} /* NEG_Handle_IAC_WONT_ECHO */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Handle_IAC_DO_ECHO
*
*   DESCRIPTION
*
*       Internal function called by NU_Telnet_Parse() to process the
*       "IAC DO ECHO" command received from the telnet client or
*       server.  This function can be called by both the server and
*       the client.  nego_Echo bits set by this function are checked by
*       NEG_Telnet_Echo_Negotiate() to see if negotiation about the
*       echo option is over.
*
*       This function is a part of the Telnet Echo Option
*       implementation (see http://www.faqs.org/rfcs/rfc857.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         NU_SUCCESS
*
************************************************************************/
STATIC STATUS NEG_Handle_IAC_DO_ECHO(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_Echo & NEGO_I_WILL)
    {
        /* NEGO_I_WILL is on, it means that we sent "IAC WILL ECHO" before
           and the other side replied "IAC DO ECHO".
           Set NEGO_HE_DO flag.  */

        tw->nego_Echo |= NEGO_HE_DO;
    }
    else
    {
        /* The other side sent "IAC DO ECHO".  But NEGO_I_WILL flag
           on this side is off. Send "IAC WONT ECHO" even if we did send
           it before (the other side is waiting for if, so send it
           to avoid deadlock.) */

        temp_buf[0] = (CHAR) TO_IAC;
        temp_buf[1] = (CHAR) TO_WONTTEL;
        temp_buf[2] = (CHAR) TO_ECHO;

        NU_Send(socket, temp_buf, 3, 0);

        tw->nego_Echo |= NEGO_HE_DO;
        tw->nego_Echo |= NEGO_I_WONT;
    }

    return NU_SUCCESS;
} /* NEG_Handle_IAC_DO_ECHO */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Handle_IAC_DONT_ECHO
*
*   DESCRIPTION
*
*       Internal function called by NU_Telnet_Parse() to process the
*       "IAC DONT ECHO" command received from the telnet client or
*       server.  This function can be called by both the server and
*       the client.  nego_Echo bits set by this function are checked by
*       NEG_Telnet_Echo_Negotiate() to see if negotiation about the
*       echo option is over.
*
*       This function is a part of the Telnet Echo Option
*       implementation (see http://www.faqs.org/rfcs/rfc857.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         NU_SUCCESS
*
************************************************************************/
STATIC STATUS NEG_Handle_IAC_DONT_ECHO(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_Echo & NEGO_I_WILL)
    {
        /* NEGO_I_WILL is on, it means that we sent "IAC WILL ECHO" before
           and the other side replied "IAC DONT ECHO".
           Set NEGO_HE_DONT flag.  Don't send anything back. */

        tw->nego_Echo |= NEGO_HE_DONT;
    }
    else if (tw->nego_Echo & NEGO_I_WONT)
    {
        /* We sent "IAC WONT ECHO" and the other side agreed not to ECHO by
           replying "IAC DONT ECHO".  Set NEGO_HE_DONT flag.
           Don't send anything back. */

        tw->nego_Echo |= NEGO_HE_DONT;
    }
    else if ( !(tw->nego_Echo & NEGO_I_WILL) && !(tw->nego_Echo & NEGO_I_WONT) )
    {
        /* We sent neither "IAC WILL ECHO", nor "IAC WONT ECHO" before, but
           received "IAC DONT ECHO" from the other side.  This means the
           other side does not want to negotiate about ECHO.
           Send "IAC WONT ECHO". */

        temp_buf[0] = (CHAR) TO_IAC;
        temp_buf[1] = (CHAR) TO_WONTTEL;
        temp_buf[2] = (CHAR) TO_ECHO;

        NU_Send(socket, temp_buf, 3, 0);

        tw->nego_Echo |= NEGO_HE_DONT;
        tw->nego_Echo |= NEGO_I_WONT;
    }

    return NU_SUCCESS;
} /* NEG_Handle_IAC_DONT_ECHO */

/************************************************************************
*
*   FUNCTION
*
*       NEG_Handle_IAC_WILL_BINARY
*
*   DESCRIPTION
*
*       Internal function called by NU_Telnet_Parse() to process the
*       "IAC WILL BINARY" command received from the telnet client or
*       server.  This function can be called by both the server and
*       the client.  nego_Binary bits set by this function are checked by
*       NEG_Telnet_Binary_Negotiate() to see if negotiation about the
*       binary option is over.
*
*       This function is a part of the Telnet Binary Option
*       implementation (see http://www.faqs.org/rfcs/rfc856.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         NU_SUCCESS
*
************************************************************************/
STATIC STATUS NEG_Handle_IAC_WILL_BINARY(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_Binary & NEGO_I_DO)
    {
        /* NEGO_I_DO is on, it means that we sent "IAC DO BINARY" before
           and the other side replied "IAC WILL BINARY".
           Set NEGO_HE_WILL flag.  Don't send anything back. */

        tw->nego_Binary |= NEGO_HE_WILL;
    }
    else
    {
        /* The other side sent "IAC WILL BINARY".  But NEGO_I_DO flag
           on this side is off. Send "IAC DONT BINARY" if we didn't send
           it before, and set NEGO_I_DONT and NEGO_HE_WILL flags. */

        tw->nego_Binary |= NEGO_HE_WILL;

        if (!(tw->nego_Binary & NEGO_I_DONT))
        {
            temp_buf[0] = (CHAR) TO_IAC;
            temp_buf[1] = (CHAR) TO_DONTTEL;
            temp_buf[2] = (CHAR) TO_BINARY;

            NU_Send(socket, temp_buf, 3, 0);

            tw->nego_Binary |= NEGO_I_DONT;
        }
    }

    return NU_SUCCESS;
} /* NEG_Handle_IAC_WILL_BINARY */

/************************************************************************
*
*   FUNCTION
*
*       NEG_Handle_IAC_WONT_BINARY
*
*   DESCRIPTION
*
*       Internal function called by NU_Telnet_Parse() to process the
*       "IAC WONT BINARY" command received from the telnet client or
*       server.  This function can be called by both the server and
*       the client.  nego_Binary bits set by this function are checked by
*       NEG_Telnet_Binary_Negotiate() to see if negotiation about the
*       binary option is over.
*
*       This function is a part of the Telnet Binary Option
*       implementation (see http://www.faqs.org/rfcs/rfc856.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         NU_SUCCESS
*
************************************************************************/
STATIC STATUS NEG_Handle_IAC_WONT_BINARY(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_Binary & NEGO_I_DO)
    {
        /* NEGO_I_DO is on, it means that we sent "IAC DO BINARY" before
           and the other side replied "IAC WONT BINARY".
           Set NEGO_HE_WONT flag.  Don't send anything back. */

        tw->nego_Binary |= NEGO_HE_WONT;
    }
    else if (tw->nego_Binary & NEGO_I_DONT)
    {
        /* We sent "IAC DONT BINARY" and the other side agreed not to BINARY by
           replying "IAC WONT BINARY".  Set NEGO_HE_WONT flag.
           Don't send anything back. */

        tw->nego_Binary |= NEGO_HE_WONT;
    }
    else if ( !(tw->nego_Binary & NEGO_I_DO) && !(tw->nego_Binary & NEGO_I_DONT) )
    {
        /* We sent neither "IAC DO BINARY", nor "IAC DONT BINARY" before, but received
           "IAC WONT BINARY" from the other side.  This means the other side
           does not want to negotiate about BINARY.
           Send "IAC DONT BINARY". */

        temp_buf[0] = (CHAR) TO_IAC;
        temp_buf[1] = (CHAR) TO_DONTTEL;
        temp_buf[2] = (CHAR) TO_BINARY;

        NU_Send(socket, temp_buf, 3, 0);

        tw->nego_Binary |= NEGO_HE_WONT;
        tw->nego_Binary |= NEGO_I_DONT;
    }

    return NU_SUCCESS;
} /* NEG_Handle_IAC_WONT_BINARY */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Handle_IAC_DO_BINARY
*
*   DESCRIPTION
*
*       Internal function called by NU_Telnet_Parse() to process the
*       "IAC DO BINARY" command received from the telnet client or
*       server.  This function can be called by both the server and
*       the client.  nego_Binary bits set by this function are checked by
*       NEG_Telnet_Binary_Negotiate() to see if negotiation about the
*       binary option is over.
*
*       This function is a part of the Telnet Binary Option
*       implementation (see http://www.faqs.org/rfcs/rfc856.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         NU_SUCCESS
*
************************************************************************/
STATIC STATUS NEG_Handle_IAC_DO_BINARY(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_Binary & NEGO_I_WILL)
    {
        /* NEGO_I_WILL is on, it means that we sent the
           "IAC WILL BINARY" before and the other side replied
           "IAC DO BINARY".  Set NEGO_HE_DO flag.  Don't send
           anything back. */

        tw->nego_Binary |= NEGO_HE_DO;
    }
    else
    {
        /* The other side sent "IAC DO BINARY".  But NEGO_I_WILL flag
           is off, it means we do not want to negotiate about this option.
           Set NEGO_HE_DO flag.  Send "IAC WONT BINARY" and set NEGO_I_WONT
           flag if we didn't do it before. */

        tw->nego_Binary |= NEGO_HE_DO;

        if (!(tw->nego_Binary & NEGO_I_WONT))
        {
            temp_buf[0] = (CHAR) TO_IAC;
            temp_buf[1] = (CHAR) TO_WONTTEL;
            temp_buf[2] = (CHAR) TO_BINARY;

            NU_Send(socket, temp_buf, 3, 0);

            tw->nego_Binary |= NEGO_I_WONT;
        }
    }

    return NU_SUCCESS;
} /* NEG_Handle_IAC_DO_BINARY */


/************************************************************************
*
*   FUNCTION
*
*       NEG_Handle_IAC_DONT_BINARY
*
*   DESCRIPTION
*
*       Internal function called by NU_Telnet_Parse() to process the
*       "IAC DONT BINARY" command received from the telnet client or
*       server.  This function can be called by both the server and
*       the client.  nego_Binary bits set by this function are checked by
*       NEG_Telnet_Binary_Negotiate() to see if negotiation about the
*       binary option is over.
*
*       This function is a part of the Telnet Binary Option
*       implementation (see http://www.faqs.org/rfcs/rfc856.html for
*       more information).
*
*   INPUTS
*
*       socket  the socket index of the current connection
*
*   RETURNS
*
*         NU_SUCCESS
*
************************************************************************/
STATIC STATUS NEG_Handle_IAC_DONT_BINARY(INT socket)
{
    CHAR temp_buf[3];
    struct TN_SESSION_STRUCT *tw;

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw->nego_Binary & NEGO_I_WILL)
    {
        /* NEGO_I_WILL is on, it means that we sent "IAC WILL BINARY" before
           and the other side replied "IAC DONT BINARY".
           Set NEGO_HE_DONT flag.  Don't send anything back. */

        tw->nego_Binary |= NEGO_HE_DONT;
    }
    else if (tw->nego_Binary & NEGO_I_WONT)
    {
        /* We sent "IAC WONT BINARY" and the other side agreed not to BINARY by
           replying "IAC DONT BINARY".  Set NEGO_HE_DONT flag.
           Don't send anything back. */

        tw->nego_Binary |= NEGO_HE_DONT;
    }
    else if ( !(tw->nego_Binary & NEGO_I_WILL) && !(tw->nego_Binary & NEGO_I_WONT) )
    {
        /* We sent neither "IAC WILL BINARY", nor "IAC WONT BINARY" before,
           but received "IAC DONT BINARY" from the other side.
           This means the other side does not want to negotiate about BINARY.
           Send "IAC WONT BINARY". */

        temp_buf[0] = (CHAR) TO_IAC;
        temp_buf[1] = (CHAR) TO_WONTTEL;
        temp_buf[2] = (CHAR) TO_BINARY;

        NU_Send(socket, temp_buf, 3, 0);

        tw->nego_Binary |= NEGO_HE_DONT;
        tw->nego_Binary |= NEGO_I_WONT;
    }

    return NU_SUCCESS;
} /* NEG_Handle_IAC_DONT_BINARY */


/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Parse
*
*   DESCRIPTION
*
*       Check the string coming from the network for special sequences
*       for negotiation, or display it on the users screen.
*
*   INPUTS
*
*       socket      the socket index of the current connection
*       st          pointer to the data stream coming
*       cnt         the count of the data stream
*       terminal    the flag to indicate whether the other data of non
*                   telnet commands and options will be dump on terminal
*                   screen or not
*
*   RETURNS
*
*       0           Command has exhausted data stream.
*       -1          An input parameter is invalid.
*
************************************************************************/
INT NU_Telnet_Parse(INT socket, UINT8 *st, INT cnt, INT terminal)
{
    INT    len;                      /* local counting variable */
    INT    sub_pos = 0;   /* the position we are in the subnegotiation parsing */
    UINT8  *mark, *orig;
    UINT8  parsedat[128];      /* buffer for sub-negotiation */
    struct TN_SESSION_STRUCT *tw;
    CHAR   temp_buf[3];
    NU_SUPERV_USER_VARIABLES

    /* Check for bad input. */
    if ( (st == NU_NULL) || (socket < 0) || (socket >= NSOCKETS) || (cnt < 0))
        return (-1);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    orig = st;              /* remember beginning point */
    mark = st+cnt;          /* set to end of input string */
    NU_Push(socket);

    /* Traverse the stream of data passed in, looking for telnet commands and
        options keystroke string of the emulation of certain terminal type.
        As to the telnet commands and its convention, please refer to RFC 854.
        About emulation of terminal, basically it is defined as VT100,
        the emulation table is declared in vt100key.h. However, the key values
        of the local keystrokes, if the user's system has a keyboard, will
        determined by the user's application, so user has to set this
        table to match his device. */

    while (st<mark)
    {
        while ( (tw->telstate!=STNORM) && (st<mark) )
        {
            /* the following switch branch control structure is FSM (state
            machine), run the machine until the state is set to STNORM, or the
            data stream is exhausted */
            switch (tw->telstate)
            {
                case ESCFOUND:
                    NEG_Parsewrite("\033", 1);        /* send the missing ESC */
                    tw->telstate=STNORM;
                    break;

                case IACFOUND:
                    /* this state means that we get a telnet command , all
                    the telnet command are started with IAC, which is unsigned
                    char of value 255.
                    however, there is only one exception, if the next value is
                    still 255, it means the second IAC is data.
                    So, change state back to normal */
                    if (*st==TO_IAC)
                    {
                        st++;
                        tw->telstate=STNORM;
                        break;
                    }

                    /* the value of the next byte should be a telnet command;
                       Take this value as the next state. */
                    if (is_telnet_command(*st))
                    {
                        tw->telstate = *st++;
                        break;
                    }

                    orig = ++st;
                    tw->telstate = STNORM;
                    break;

                /* each of the states below is one telnet command */
                case TO_DOTEL:
                    /* received a telnet DO negotiation, the following switch
                    control structure is responsible for handling the
                    negotiation */
                    switch (*st)
                    {
                        case TO_BINARY:
                            /* received "IAC DO BINARY" command.  It can be sent by both
                               client and server.  Process it. */
                            NEG_Handle_IAC_DO_BINARY(socket);
                            break;

                        case TO_SGA:
                            /* received "IAC DO SGA" command.  It can be sent by both
                               client and server.  Process it. */
                            NEG_Handle_IAC_DO_SGA(socket);
                            break;

                        case TO_TERMTYPE:
                            /* received "IAC DO TERMINAL-TYPE" command which should only
                               be sent by telnet server.  Only respond if we are client. */
#ifdef CFG_NU_OS_NET_PROT_TELNET_CLIENT_ENABLE
                            if (tw->i_am_client == NU_TRUE)
                                NEG_Cln_Handle_IAC_DO_TERMTYPE(socket);
#endif
                            break;

                        case TO_NAWS:
                            /* received "IAC DO NAWS" command which should only be sent
                               by telnet server.  Only respond if we are client. */
#ifdef CFG_NU_OS_NET_PROT_TELNET_CLIENT_ENABLE
                            if (tw->i_am_client == NU_TRUE)
                                NEG_Cln_Handle_IAC_DO_NAWS(socket);
#endif
                            break;

                        case TO_ECHO:
                            /* received "IAC DO ECHO" command.  It can be sent by both
                               client and server.  Process it. */
                            NEG_Handle_IAC_DO_ECHO(socket);
                            break;

                        default:
                            /* we reject other options */
                            temp_buf[0] = (CHAR) TO_IAC;
                            temp_buf[1] = (CHAR) TO_WONTTEL;
                            temp_buf[2] = (CHAR) *st;

                            NU_Send(socket, temp_buf, 3, 0);
                            break;
                    } /* end switch TO_DOTEL */

                    /* set state back to normal to check rest byte */
                    tw->telstate = STNORM;
                    /* remember the current position */
                    orig = ++st;
                    break;

                case TO_DONTTEL:
                    /* Received a telnet DONT option */
                    switch (*st)
                    {
                        case TO_BINARY:
                            /* received "IAC DONT BINARY" command.  It can be sent by both
                               client and server.  Process it. */
                            NEG_Handle_IAC_DONT_BINARY(socket);
                            break;

                        case TO_SGA:
                            /* received "IAC DONT SGA" command.  It can be sent by both
                               client and server.  Process it. */
                            NEG_Handle_IAC_DONT_SGA(socket);
                            break;

                        case TO_TERMTYPE:
                            /* received "IAC DONT TERMINAL-TYPE" command which
                               should only be sent by telnet server.  Only respond
                               if we are client. */
#ifdef CFG_NU_OS_NET_PROT_TELNET_CLIENT_ENABLE
                            if (tw->i_am_client == NU_TRUE)
                                NEG_Cln_Handle_IAC_DONT_TERMTYPE(socket);
#endif
                            break;

                        case TO_NAWS:
                            /* received "IAC DONT NAWS" command which
                               should only be sent by telnet server.  Only respond
                               if we are client. */
#ifdef CFG_NU_OS_NET_PROT_TELNET_CLIENT_ENABLE
                            if (tw->i_am_client == NU_TRUE)
                                NEG_Cln_Handle_IAC_DONT_NAWS(socket);
#endif
                            break;

                        case TO_ECHO:
                            /* received "IAC DONT ECHO" command which
                               can be sent by both client and server.  Process it. */
                            NEG_Handle_IAC_DONT_ECHO(socket);
                            break;

                        default:
                            break;
                    } /* end switch TO_DONTTEL */

                    /* set state back to normal to check rest byte */
                    tw->telstate = STNORM;
                    /* remember the current position */
                    orig = ++st;
                    break;

                case TO_WILLTEL:
                    /* received a telnet WILL option */
                    switch (*st)
                    {
                        case TO_BINARY:
                            /* received "IAC WILL BINARY" command.  It can be sent by both
                               client and server.  Process it. */
                            NEG_Handle_IAC_WILL_BINARY(socket);
                            break;

                        case TO_SGA:
                            /* received "IAC WILL SGA" command.  It can be sent by both
                               client and server.  Process it. */
                            NEG_Handle_IAC_WILL_SGA(socket);
                            break;

                        case TO_ECHO:
                            /* received "IAC WILL ECHO" command.  It can be sent by both
                               client and server.  Process it. */
                            NEG_Handle_IAC_WILL_ECHO(socket);
                            break;

                        case TO_NAWS:
                            /* received "IAC WILL NAWS" command which should only be sent
                               by telnet client.  Only respond if we are server. */
#ifdef CFG_NU_OS_NET_PROT_TELNET_SERVER_ENABLE
                            if (tw->i_am_client != NU_TRUE)
                                NEG_Srv_Handle_IAC_WILL_NAWS(socket);
#endif
                            break;

                        case TO_TERMTYPE:
                            /* received "IAC WILL TERMINAL-TYPE" command which should only
                               be sent by telnet client.  Only respond if we are server. */
#ifdef CFG_NU_OS_NET_PROT_TELNET_SERVER_ENABLE
                            if (tw->i_am_client != NU_TRUE)
                                NEG_Srv_Handle_IAC_WILL_TERMTYPE(socket);
#endif
                            break;

                        default:
                            /* we do not support other options */
                            temp_buf[0] = (CHAR) TO_IAC;
                            temp_buf[1] = (CHAR) TO_DONTTEL;
                            temp_buf[2] = (CHAR) *st;

                            NU_Send(socket, temp_buf, 3, 0);
                            break;
                    } /* end switch TO_WILLTEL */

                    /* set state back to normal to check rest byte */
                    tw->telstate = STNORM;
                    /* remember the current position */
                    orig = ++st;
                    break;

                case TO_WONTTEL:
                    /* Received a telnet WONT option */
                    switch (*st)
                    {
                        case TO_BINARY:
                            /* received "IAC WONT BINARY" command.  It can be sent by both
                               client and server.  Process it. */
                            NEG_Handle_IAC_WONT_BINARY(socket);
                            break;

                        case TO_SGA:
                            /* received "IAC WONT SGA" command.  It can be sent by both
                               client and server.  Process it. */
                            NEG_Handle_IAC_WONT_SGA(socket);
                            break;

                        case TO_ECHO:
                            /* received "IAC WONT ECHO" command.  It can be sent by both
                               client and server.  Process it. */
                            NEG_Handle_IAC_WONT_ECHO(socket);
                            break;

                        case TO_NAWS:
                            /* received "IAC WONT NAWS" command which can only be sent
                               by telnet client. Only respond if we are server. */
#ifdef CFG_NU_OS_NET_PROT_TELNET_SERVER_ENABLE
                            if (tw->i_am_client != NU_TRUE)
                                NEG_Srv_Handle_IAC_WONT_NAWS(socket);
#endif
                            break;

                        case TO_TERMTYPE:
                            /* received "IAC WONT TERMINAL-TYPE" command which should only
                               be sent by telnet client.  Only respond if we are server. */
#ifdef CFG_NU_OS_NET_PROT_TELNET_SERVER_ENABLE
                            if (tw->i_am_client != NU_TRUE)
                                NEG_Srv_Handle_IAC_WONT_TERMTYPE(socket);
#endif
                            break;

                        default:
                            break;
                    } /* end switch TO_WONTTEL */

                    /* set state back to normal to check rest byte */
                    tw->telstate = STNORM;
                    /* remember the current position */
                    orig = ++st;
                    break;

                case TO_SB:
                    /* we get the start point of telnet sub-options
                    negotiation, change state to collect all sub-negotiate
                    bytes */
                    tw->telstate = NEGOTIATE;
                    /* initialize these variables to receive sub-negotiation */
                    orig = st;
                    sub_pos = tw->substate = 0;

                    /*lint -fallthrough*/
                case NEGOTIATE:
                    /* collect all the sub-negotiate bytes until we get its
                    end: IAC,SE (255,240) */
                    if (tw->substate==0)
                    {
                        if (*st==TO_IAC)
                        {   /* check if we find an IAC before SE */
                            if (*(st+1)==TO_IAC)
                            {  /* skip over double IAC's */
                                st++;
                                parsedat[sub_pos++] = *st++;
                            } /* end if */
                            else
                            {
                                /* put the IAC byte into the sub-state, in order
                                to leave the collection state on the next time
                                of NEGOTIATE state, the main state is intact. */
                                tw->substate = *st++;
                            } /* end else */
                        } /* end if */
                        else
                            /* otherwise, just keep collecting the sub-negotiate
                            bytes in sub-negotiate buffer. */
                            parsedat[sub_pos++] = *st++;
                    } /* (tw->substate==0) */
                    else
                    {
                        /* the next byte should be SE */
                        tw->substate = *st++;

                        /* check if we've really ended the sub-negotiations */
                        if (tw->substate==TO_SE)
                        {
                            parsedat[sub_pos] = 0;  /* NUL-terminate it */
                            NEG_Parse_Subnegotiat(socket, parsedat);
                            sub_pos = 0;
                        }
                        orig = st;
                        tw->telstate = STNORM;
                    } /* end else */
                    break;

                case TO_AYT:       /* received a telnet Are-You-There command */
                    NU_Send(socket, "YES", 3, 0);
                    tw->telstate = STNORM;
                    /* NJT Mod was ++st, caused loss of 1 character */
                    orig = st;
                    break;

                case TO_SGA:
                    /* received "IAC GA" (Go Ahead) command */
                    tw->telstate = STNORM;

                    /* check if we suppress the GAs */
                    if ((tw->nego_SGA & NEGO_I_WILL) || !(tw->nego_SGA & NEGO_I_WONT))
                    {
                        break;  /* do not process the GA, go to the next command ... */
                     }


                    /* we don't suppress GAs.  Process it. */
                    if (tw->i_am_client == NU_TRUE)
                    {
                        /* client received the GA command.  This might mean that the
                           server requires that the client pass control to the user of
                           the terminal (see http://www.faqs.org/rfcs/rfc854.html).
                           The user's application should handle it here as needed.  */
                        ;
                    }
                    else
                    {
                        /* server received the GA command.  The user's application should
                           handle it here as needed.  Note that in the user-to-server
                           direction of communication, GAs need not ever be sent.
                           GAs need not be sent in either direction if telnet is
                           used for process-to-process communication.  GAs maybe
                           required in neither, one, or both directions for
                           terminal-to-terminal communication
                           (see http://www.faqs.org/rfcs/rfc854.html). */
                        ;
                    }
                    break;

                default:
                    tw->telstate = STNORM;
                    break;
            } /* end switch */
        } /* end while */

        /* quick scan of the remaining string, skip chars while they are
            uninteresting, or they are keystroke string */
        if ( (tw->telstate==STNORM) && (st<mark) )
        {
            while ( (st<mark && *st!=ESC) && (*st!=TO_IAC) )
                st++;

            len = (INT)(st - orig);

            /* debug purpose, normally it should be exhausted. */
            if ( (len) && (st!=mark) )
                NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);

            if ( (terminal) && (len) )
            /* if this application dumps the incoming date on its screen
             or printer, dumps it */
                NEG_Parsewrite((CHAR *)orig, len);

            /* forget what we have sent already */
            orig = st;
            if (st<mark)
                switch (*st)
                {
                    case TO_IAC:
                    /* check telnet command, if find IAC, the next state is to
                    parse the telnet command */
                        tw->telstate = IACFOUND;
                        st++;
                        break;

                    case ESC:            /* ESCape code */
                    /* if this byte is Escape, there are four possibilities:
                     1. it is the last byte of the stream,
                     2. it is followed by 12, this is a printer command,
                       12 - Form Feed Moves the printer to the top of the
                       next page, keeping the same horizontal position.
                     3. it is followed by '^', its value is 94, I don't know.
                     4. this is ESC string of vt100 key, it must be followed by
                       other two characters.
                      '[' and 'O' are the second byte of vt100 key code.
                       the users may need modify it to match their specific situation.
                    */
                        if (*(st+1)=='[' || *(st+1)=='O')
                        {
                            /* check this emulation string in key mapping table
                                and explain it */
                            len = NU_Receive_NVT_Key(socket, (CHAR *)st, 1);
                            /* if it maps a key, skip these bytes
                                and go on parsing */
                            if (len)
                                st += len;
                            else
                                st += 3;
                            orig = st;
                            break;
                        }
                        else if (st==mark-1 || *(st+1)==12 || *(st+1)=='^')
                            tw->telstate = ESCFOUND;
                        st++;           /* strip or accept ESC char */
                        break;

                    default:
                        st++;
                        break;
                }   /* end switch */
        }   /* end if */
    } /* end while */

    NU_Release_Semaphore(&TN_Session_Semaphore);
    NU_USER_MODE();    /* return to user mode */

    /* indicate that NU_Telnet_Parse() exhausts this data stream */
    return(0);
}   /* end NU_Telnet_Parse() */

/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Pick_Up_Ascii
*
*   DESCRIPTION
*
*       Check the string coming from the network for special sequences.
*       Only pick up the ascii characters to display, or move to where
*       the user wants by redeclare telnet_print()
*
*   INPUTS
*
*       st      Pointer to the data stream coming
*       cnt     The count of the data stream
*
*   RETURNS
*
*       0 if unsuccessful, otherwise cnt.
*
************************************************************************/
INT NU_Telnet_Pick_Up_Ascii(UINT8 *st, INT cnt)
{
    INT find=0, count=0, i;
    UINT8 *orig;
    NU_SUPERV_USER_VARIABLES

    /* Check for bad input. */
    if ( (st == NU_NULL) || (cnt < 1) )
        return (0);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    while (cnt)
    {
        if ( (*st!=ESC) && (*st!=TO_IAC) )
        {
            st++;
            cnt--;
            find |= NEG_ASCII_CODE;
            continue;
        }

        orig = st;              /* remember beginning point of IAC string */
        if (*st==ESC)
        {
            /* '[' and 'O' are the second byte of vt100 key code.
                the users may need modify it to match their own emulation */
            if (*(st+1)=='[' || *(st+1)=='O')
            {
                st += 3;
                count += 3;
                find |= NEG_ESC_CODE;
            }
            else
                st++;
        }
        else if (*st==TO_IAC)
        {
            find |= NEG_IAC_CODE;
            /* IAC must be followed with, at least, one more byte */
            st++;
            count++;

            /* telnet commands and options string is, at least, of 2 bytes */
            if (*st<TO_SE)
            {
                NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
                orig[0] = 0;
                NU_USER_MODE();    /* return to user mode */
                return(0);
            }

            if ( (TO_WILLTEL<=*st) && (*st<=TO_DONTTEL) )
            {
                /* after DOTEL, DONTTEL, WILLTEL and WONTTEL, ONLY one byte */
                st++;
                count++;
            }
            else if (*st==TO_SB)
            {
                find |= NEG_SUBNEG_CODE;
                /* find all the bytes before end of sub-negotiation. */
                while (*st!=TO_SE)
                {
                    st++;
                    count++;
                }
            }

            /* telnet commands and options string is, at least, of 2 bytes */
            st++;
            count++;
            /* if sub-negotiation, it is, at least, of 5 bytes */
            if ( (find & NEG_SUBNEG_CODE)  && (count < 6) )
            {
                /*\nit's a bad sub negotiation!");*/
                NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
                orig[0] = 0;
                NU_USER_MODE();    /* return to user mode */
                return(0);
            }
        }
        /* calculate the totally retained bytes */
        cnt -= count;
        /* in case some bad situation, cnt<0 will cause crash */
        if (cnt<0)
        {
            NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
            orig[0] = 0;
            NU_USER_MODE();    /* return to user mode */
            return(0);
        }
        for (i=0; i<cnt; i++)
            orig[i] = st[i];
        orig[i] = 0;
        st = orig;
        count = 0;
    }

    NU_USER_MODE();    /* return to user mode */

    /* indicate that which type of data are found in this data stream */
    return(find);
}   /* end NU_Telnet_Pick_Up_Ascii() */

/************************************************************************
*
*   FUNCTION
*
*       NU_Telnet_Get_Filtered_Char
*
*   DESCRIPTION
*
*       Get a character from telnet socket and check if there are telnet
*       commands and options. If there are, read all of them and execute
*       them until we get an ascii character.
*
*   INPUTS
*
*       socket   the socket index of the current connection
*       timeout  timeout value in ticks
*       retchar      points where to put character read from the socket.
*                If NU_Telnet_Get_Filtered_Char() returned NU_SUCCESS,
*                retchar contains the char read. Otherwise, it contains NULL.
*
*   RETURNS
*
*       NU_SUCCESS,
*
*   or one of the following NU_Select() or NU_Recv() error codes:
*
*       -1
*       NU_NO_SOCKETS
*       NU_INVALID_SOCKET
*       NU_NO_DATA
*       NU_NO_PORT_NUMBER
*       NU_NOT_CONNECTED
*       NU_WOULD_BLOCK
*       NU_NO_ROUTE_TO_HOST
*       NU_CONNECTION_REFUSED
*       NU_MSG_TOO_LONG
*
************************************************************************/
STATUS NU_Telnet_Get_Filtered_Char(INT socket, UNSIGNED timeout, CHAR *retchar)
{
    UINT8  chr = 0, i, telnet_command[MAX_TN_COMMAND];
    INT    retval;
    struct TN_SESSION_STRUCT *tw;
    FD_SET readfs;
    NU_SUPERV_USER_VARIABLES

    /* Check for bad input. */
    if ( (retchar == NU_NULL) || (socket < 0) || (socket > NSOCKETS) )
        return (-1);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    NU_FD_Init(&readfs);
    NU_FD_Set(socket, &readfs);

    *retchar = (CHAR)((INT) NULL);

    /* check if there are any data on the socket available for reading */
    retval = (INT)NU_Select(socket+1, &readfs, NULL, NULL, timeout);
    if (retval != NU_SUCCESS)
    {
        NU_USER_MODE();    /* return to user mode */
        return (STATUS)retval;
    }

    /* get one character from the socket */
    retval = (INT)NU_Recv(socket, (CHAR *)&chr, 1, 0);

    /* check if we got exactly one character;  all other values
       returned by NU_Recv() are interpreted as errors */
    if (retval != 1)
    {
        NU_USER_MODE();    /* return to user mode */
        return (STATUS)retval;
    }

    /* check if there are any telnet commands and options, if there are,
        read through and execute them */
    while (chr==ESC || chr==TO_IAC)
    {
        i = 0;
        if (chr==TO_IAC)
        {
            telnet_command[i++] = TO_IAC;

            /* since TO_IAC is always followed by a telnet command or option
               code, we don't need to check with NU_Select() if there are any
               data available for reading.  We know there are. */
            chr = 0;

            /* get one character from the socket */
            retval = (INT)NU_Recv(socket, (CHAR *)&chr, 1, 0);

            /* check if we got exactly one character;  all other values
               returned by NU_Recv() are interpreted as errors */
            if (retval != 1)
            {
                NU_USER_MODE();    /* return to user mode */
                return (STATUS)retval;
            }

            /* TO_IAC should be followed by a telnet command or option whose
               code is no lower than TO_IAC (telnet commands and options
               have codes 240...254, or TO_SE...TO_DONT).
               Otherwise, it is an error. */
            if (chr<TO_SE)
            {
                NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
                NU_USER_MODE();    /* return to user mode */
                return -1;
            }

            telnet_command[i++] = chr;
            if (chr==TO_WILLTEL || chr==TO_WONTTEL ||
                chr==TO_DOTEL   || chr==TO_DONTTEL)
            {
                /* according to the syntax of a telnet command,
                   there must be one more byte; get this one byte  */
                retval = (INT)NU_Recv(socket, (CHAR *)&chr, 1, 0);

                /* check if we got exactly one character;  all other values
                   returned by NU_Recv() are interpreted as errors */
                if (retval != 1)
                {
                    NU_USER_MODE();    /* return to user mode */
                    return (STATUS)retval;
                }

                telnet_command[i++] = chr;
            }
            else if (chr==TO_SB)
            {
                /* sub-negotiation; read through until the end of it */
                while (chr!=TO_SE)
                {
                    /* because SB is always followed by more bytes,
                       there should be more bytes available for reading */
                    retval = (INT)NU_Recv(socket, (CHAR *)&chr, 1, 0);

                    /* check if we got exactly one character;  all other values
                       returned by NU_Recv() are interpreted as errors */
                    if (retval != 1)
                    {
                        NU_USER_MODE();    /* return to user mode */
                        return (STATUS)retval;
                    }

                    telnet_command[i++] = chr;
                    if (i>=MAX_TN_COMMAND)
                    {
                        NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
                        NU_USER_MODE();    /* return to user mode */
                        return -1;
                    }
                }
            }

            telnet_command[i] = 0;
            if (i>=MAX_TN_COMMAND)
            {
                NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
                NU_USER_MODE();    /* return to user mode */
                return -1;
            }
            else if (i==1)
            {
                NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
                NU_USER_MODE();    /* return to user mode */
                return -1;
            }
            else if (i>=1)
            /* there are legal telnet commands and options,
                parse and execute them */
                NU_Telnet_Parse(socket, telnet_command, (INT)i, 0);
        }
        else if (chr==ESC)
        {
            /* read ESC string into buffer */
            telnet_command[i++] = chr;

            /*  check if there are any data on the socket available for reading */
            retval = (INT)NU_Select(socket+1, &readfs, NULL, NULL, timeout);
            if (retval != NU_SUCCESS)
            {
                NU_USER_MODE();    /* return to user mode */
                return (STATUS)retval;
            }

            chr = 0;

            /* get one character from the socket */
            retval = (INT)NU_Recv(socket, (CHAR *)&chr, 1, 0);

            /* check if we got exactly one character;  all other values
               returned by NU_Recv() are interpreted as errors */
            if (retval != 1)
            {
                NU_USER_MODE();    /* return to user mode */
                return (STATUS)retval;
            }

            /* '[' and 'O' are the second byte of vt100 key code.
            the users may need modify it to match their specific emulation */
            if (chr=='O' || chr=='[')
            {
                telnet_command[i++] = chr;

                /* there must be another ESC string character */

                /*  check if there are any data on the socket available for reading */
                retval = (INT)NU_Select(socket+1, &readfs, NULL, NULL, timeout);
                if (retval != NU_SUCCESS)
                {
                    NU_USER_MODE();    /* return to user mode */
                    return (STATUS)retval;
                }

                chr = 0;

                /* get one character from the socket */
                retval = (INT)NU_Recv(socket, (CHAR *)&chr, 1, 0);

                /* check if we got exactly one character;  all other values
                   returned by NU_Recv() are interpreted as errors */
                if (retval != 1)
                {
                    NU_USER_MODE();    /* return to user mode */
                    return (STATUS)retval;
                }

                telnet_command[i++] = chr;
                telnet_command[i] = 0;
                /* check this emulation string in key mapping table and explain it */
                NU_Receive_NVT_Key(socket, (CHAR *)telnet_command, 1);
            }
            else
            {
                *retchar = (CHAR)chr;
                NU_USER_MODE();    /* return to user mode */
                return NU_SUCCESS;
            }
        } /* if (chr==\033) */

        /* There may be more telnet commands, options, or ESC strings;
           not be, so it is necessary to turn on the "block" flag again
           to wait for and get a character from telnet. */

        retval = (INT)NU_Select(socket+1, &readfs, NULL, NULL, timeout);
        if (retval != NU_SUCCESS)
        {
            NU_USER_MODE();    /* return to user mode */
            return (STATUS)retval;
        }

        chr = 0;

        /* get one character from the socket */
        retval = (INT)NU_Recv(socket, (CHAR *)&chr, 1, 0);

        /* check if we got exactly one character;  all other values
           returned by NU_Recv() are interpreted as errors */
        if (retval != 1)
        {
            NU_USER_MODE();    /* return to user mode */
            return (STATUS)retval;
        }
    }

    NU_Obtain_Semaphore(&TN_Session_Semaphore, NU_SUSPEND);

    /* get the pointer of the parameter data struct of the current session */
    tw = TN_Session[socket];

    if (tw != NULL)
    {
        if ( (tw->nego_Echo & NEGO_I_WILL) && (tw->nego_Echo & NEGO_HE_DO) )
        {
            /* echo the char back to sender */
            NU_Send(socket, (CHAR *)&chr, 1, 0);
        }
    }

    NU_Release_Semaphore(&TN_Session_Semaphore);

    *retchar = (CHAR) chr;
    NU_USER_MODE();    /* return to user mode */

    return NU_SUCCESS;
} /* NU_Telnet_Get_Filtered_Char */
