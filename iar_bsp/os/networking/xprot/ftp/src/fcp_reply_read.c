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
*   FILE                                             VERSION
*
*       fcp_reply_read.c                             1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client
*
*   DESCRIPTION
*
*       This file contains support for Client Read requests.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Reply_Read
*
*   DEPENDENCIES
*
*       nucleus.h
*       externs.h
*       fc_defs.h
*       ftpc_defs.h
*       fcp_extr.h
*       fc_extr.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/externs.h"
#include "networking/fc_defs.h"
#include "networking/ftpc_def.h"
#include "networking/fcp_extr.h"
#include "networking/fc_extr.h"

/******************************************************************************
*
*   FUNCTION
*
*       FCP_Reply_Read
*
*   DESCRIPTION
*
*       This function is one of the three general purpose functions included
*       with the primitives. When called it returns a single FTP reply, or
*       an error message if the stack returns an error or if the 'next' reply
*       in the buffer is not valid.
*
*   INPUTS
*
*       client                   pointer to valid FTP client structure
*       buffer                   pointer to destination buffer
*       buffsize                 size of destination buffer
*       timeout                  timeout value for read
*
*   OUTPUTS
*
*       FTP_TIMEOUT              The client timed out waiting for a
*                                server response.
*       FTP_STACK_ERROR          A send or receive command returned an error
*                                while communicating with the FTP Server.
*       FTP_BAD_MSG_FORMAT       The reply received was invalid.
*       FTP_REPLY_BUFFER_OVERRUN The reply data exceeded buffer size.
*       FTP_INVALID_PARM         A required parameter is null.
*
******************************************************************************/
INT FCP_Reply_Read(FTP_CLIENT *client, UINT8 *buffer, INT buffsize,
                   UINT32 timeout)
{
    INT     bytes_received;
    INT     out         = 0;
    INT     state       = RR_START;
    INT     hyphen_flag = 0;
    INT     str_index   = 0;
    FD_SET  readfs;
    STATUS  status;

    NU_SUPERV_USER_VARIABLES

    if ( (client == NU_NULL) || (buffer == NU_NULL) || (buffsize <= 0) )
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    NU_FD_Init(&readfs);

    while (!out)
    {
        /* Buffer needs to be refreshed */
        if (client->reply_idx == client->reply_tail)
        {
            NU_FD_Set(client->socketd,&readfs);

            status = NU_Select(client->socketd+1, &readfs, NULL, NULL, timeout);

            if (status != NU_SUCCESS)
            {
                out++;

                if (status == NU_NO_DATA)
                    str_index = client->last_error = FTP_TIMEOUT;
                else
                    str_index = client->last_error = FTP_STACK_ERROR;

                client->reply_idx = 0;
                client->reply_tail = 0;
            }

            else
            {
                /* Turn off blocking. We do not want to suspend on this read. */
                NU_Fcntl(client->socketd, NU_SETFLAG, NU_FALSE);

                /*  Go get the server's response.  */
                bytes_received = (INT)NU_Recv(client->socketd, client->reply_buff,
                                         FTPC_REPLY_BUFFER_SIZE, 0);

                /* Turn blocking back on. */
                NU_Fcntl(client->socketd, NU_SETFLAG, NU_BLOCK);

                if (bytes_received < 0)
                {
                    /* Error in call to NU_Recv(). */
                    str_index = client->last_error = FTP_STACK_ERROR;
                    client->stack_error = bytes_received;
                    out++;
                    client->reply_tail = 0;
                }
                else
                {
                    client->reply_tail = bytes_received;
                    client->reply_buff[bytes_received] = 0;
                }

                client->reply_idx = 0;
            }
        }

        if (client->reply_tail) /* Is something there to read? */
        {
            switch (state)
            {
                case RR_START: /* Initial state. Look for first digit */

                    if ( (client->reply_buff[client->reply_idx] >= '1') &&
                         (client->reply_buff[client->reply_idx] <= '5'))
                    {
                        buffer[str_index] = (UINT8)client->reply_buff[client->reply_idx];
                        state = RR_DIGIT2;
                        str_index++;
                    }

                    else
                    {
                        state = RR_BAD_REP;
                    }

                    break;

                case RR_DIGIT2: /* Looking for second digit of reply code. */

                    if ( (client->reply_buff[client->reply_idx] >= '0') &&
                         (client->reply_buff[client->reply_idx] <= '9'))
                    {
                        buffer[str_index] = (UINT8)client->reply_buff[client->reply_idx];
                        state = RR_DIGIT3;
                        str_index++;
                    }

                    else
                    {
                        state = RR_BAD_REP;
                    }

                    break;

                case RR_DIGIT3: /* Looking for third digit of reply code. */

                    if ( (client->reply_buff[client->reply_idx] >= '0') &&
                         (client->reply_buff[client->reply_idx] <= '9'))
                    {
                        buffer[str_index] = (UINT8)client->reply_buff[client->reply_idx];
                        state = RR_SPACE;
                        str_index++;
                    }

                    else
                    {
                        state = RR_BAD_REP;
                    }

                    break;

                case RR_SPACE: /* Looking for space or hyphen */

                    if (client->reply_buff[client->reply_idx] == ' ')
                    {
                        buffer[str_index] = (UINT8)client->reply_buff[client->reply_idx];
                        state = RR_AFTER_SPACE;
                        str_index++;
                    }

                    else if (client->reply_buff[client->reply_idx] == '-')
                    {
                        buffer[str_index] = (UINT8)client->reply_buff[client->reply_idx];
                        state = RR_AFTER_SPACE;
                        str_index++;
                        hyphen_flag = 1;
                    }

                    else
                    {
                        hyphen_flag = 0;
                        state = RR_BAD_REP;
                    }

                    break;

                case RR_AFTER_SPACE: /* looking for CR */

                    if (client->reply_buff[client->reply_idx] == '\r')
                    {
                        state = RR_FIND_LF;
                    }

                    buffer[str_index] = (UINT8)client->reply_buff[client->reply_idx];
                    str_index++;
                    break;

                case RR_FIND_LF: /* Looking for LF to follow CR */

                    if (client->reply_buff[client->reply_idx] == '\n')
                    {
                        if (!hyphen_flag)
                        {
                            out++;
                        }
                        else
                        {
                            state = RR_HYPHEN_ALT;
                            hyphen_flag = 0;
                        }
                    }
                    else
                    {
                        state = RR_AFTER_SPACE;
                    }

                    buffer[str_index] = (UINT8)client->reply_buff[client->reply_idx];
                    str_index++;
                    break;

                case RR_HYPHEN_ALT: /* Special case hyphen parsing */

                    if (client->reply_buff[client->reply_idx] == buffer[0])
                    {
                        state = RR_HYPHEN2;
                        buffer[str_index] = (UINT8)client->reply_buff[client->reply_idx];
                        str_index++;
                    }

                    else if (client->reply_idx != client->reply_tail)
                    {
                        state = RR_AFTER_SPACE;
                        hyphen_flag = 1;
                    }

                    else
                    {
                        state = RR_BAD_REP;
                    }

                    break;

                case RR_HYPHEN2: /* Special case hyphen parsing */

                    if (client->reply_buff[client->reply_idx] == buffer[1])
                    {
                        state = RR_HYPHEN3;
                        buffer[str_index] = (UINT8)client->reply_buff[client->reply_idx];
                        str_index++;
                    }

                    else
                    {
                        state = RR_BAD_REP;
                    }

                    break;

                case RR_HYPHEN3: /* Special case hyphen parsing */

                    if (client->reply_buff[client->reply_idx] == buffer[2])
                    {
                        state = RR_SPACE;
                        buffer[str_index] = (UINT8)client->reply_buff[client->reply_idx];
                        str_index++;
                    }

                    else
                    {
                        state = RR_BAD_REP;
                    }

                    break;

                case RR_BAD_REP: /* Bad reply. Flush to next CR/LF pair */

                    if (client->reply_buff[client->reply_idx] == '\r')
                    {
                        state = RR_BAD_FIND_LF;
                    }

                    str_index = client->last_error = FTP_BAD_MSG_FORMAT;
                    break;

                case RR_BAD_FIND_LF:

                    if (client->reply_buff[client->reply_idx] == '\n')
                    {
                        out++;
                    }

                    else
                    {
                        state = RR_BAD_REP;
                    }

                    break;

                default:

                    break;
            } /* End of switch */

            client->reply_idx++;

            if ( (str_index == buffsize) && (!out) )
            {   /* Not enough buffer space */
                str_index = client->last_error = FTP_REPLY_BUFFER_OVERRUN;
                out++;
            }
        }
    }

    if ( (str_index != client->reply_tail) &&
         (str_index != FTP_REPLY_BUFFER_OVERRUN) )
        buffer[str_index] = 0;

    NU_USER_MODE();    /* return to user mode */

    return (str_index);

} /* FCP_Reply_Read */
