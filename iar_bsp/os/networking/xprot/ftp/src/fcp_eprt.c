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
*       fcp_eprt.c                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client commands
*
*   DESCRIPTION
*
*       This file contains the FTP Client implementation of EPRT.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_EPRT
*
*   DEPENDENCIES
*
*       nucleus.h
*       externs.h
*       ncl.h
*       fc_defs.h
*       ftpc_defs.h
*       fcp_extr.h
*       fc_extr.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/externs.h"
#include "networking/ncl.h"
#include "networking/fc_defs.h"
#include "networking/ftpc_def.h"
#include "networking/fcp_extr.h"
#include "networking/fc_extr.h"

/******************************************************************************
*
*   FUNCTION
*
*       FCP_Client_EPRT
*
*   DESCRIPTION
*
*       This function provides a primitive to send an FTP EPRT message to the
*       server. Parameters include a pointer to a valid FTP client structure,
*       and a pointer to a structure containing the IP address and tcp port
*       to be sent to the server.
*
*       The format of the EPRT command follows:
*
*           EPRT<space><d><net-prt><d><n-addr><d><tcp-port><d>/r/n
*
*           Where <d> is a delimiter in the ASCII range 33-126.  Nucleus FTP
*           uses the RFC 2428 recommended character '|' (ASCII 124)
*
*       IPv4 Example:
*
*           EPRT |1|192.200.100.1|1125|
*
*       IPv6 Example:
*
*           EPRT |2|fe80::240:5ff:fe20:709f|1125|
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       servaddr                pointer to structure containing address to
*                               be sent to the server
*
*   OUTPUTS
*
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the FTP Server.
*       FTP_SERVICE_UNAVAILABLE The FTP Server returned a 'Service Unavailable'
*                               message. This may be a reply to any command if
*                               the ftp service knows it must shut down.
*       FTP_CMD_UNRECOGNIZED    The FTP Server did not recognize the command.
*       FTP_BAD_CMD_FORMAT      FTP Server did not recognize command format.
*       FTP_INVALID_USER        The FTP Server did not recognize user account.
*       FTP_BAD_RESPONSE        The FTP Server returned an unknown error code.
*       FTP_INVALID_PARM        A required parameter is null.
*
******************************************************************************/
INT FCP_Client_EPRT(FTP_CLIENT *client, struct addr_struct *servaddr)
{
    INT     status,
            bytes_received;
    UINT8   buffer[FTPC_GENERIC_BUFF_SIZE];
    CHAR    port_buffer[10];
#ifndef NET_5_1
    INT     index;
    INT     port_len;
#endif

    NU_SUPERV_USER_VARIABLES

    if ( (client == NU_NULL) || (servaddr == NU_NULL) )
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    UTL_Zero(buffer, FTPC_GENERIC_BUFF_SIZE);

    /* Put the family type in the message */
#if (defined(NET_5_1))
    if (servaddr->family == NU_FAMILY_IP6)
        memcpy(buffer, "EPRT |2|", 8);
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        memcpy(buffer, "EPRT |1|", 8);
#endif

    /* Convert the port to ASCII */
    NU_ITOA(servaddr->port, port_buffer, 10);

#ifdef NET_5_1
    /* Put the IP address in the message */
    NU_Inet_NTOP(servaddr->family, servaddr->id.is_ip_addrs, (CHAR*)(&buffer[8]),
                 (FTPC_GENERIC_BUFF_SIZE - 8));

    strcat((CHAR*)buffer, "|");

    /* Put the ASCII port string in the buffer */
    strcat((CHAR*)buffer, port_buffer);

    strcat((CHAR*)buffer, "|\r\n");

#else

    index = 8;

    /* Store the address and port in the buffer */
    index += FC_Store_Addr(servaddr, (CHAR*)(&buffer[index]), '.');

    buffer[index] = '|';

    /* Copy the port into the buffer */
    strcpy((CHAR*)(&buffer[index + 1]), port_buffer);

    /* Get the length of the port number just stored */
    port_len = strlen(port_buffer);

    /* Increment past the port number */
    index += (port_len + 1);

    buffer[index] = '|';
    buffer[index + 1] = '\r';
    buffer[index + 2] = '\n';
    buffer[index + 3] = 0;

#endif

    /* send port message */
    status = (INT)NU_Send(client->socketd, (CHAR *)buffer,
                     (UINT16)strlen((CHAR*)buffer), 0);

    if (status < 0)
    {
        /* Error encountered while sending command to host. */
        client->last_error = FTP_STACK_ERROR;
        client->stack_error = status;
    }
    else
    {
        /*  Go get the server's response.  */
        bytes_received = FCP_Reply_Read(client, buffer, FTPC_GENERIC_BUFF_SIZE,
                                        FTPC_INACT_TIMEOUT);

        if (bytes_received >= 0)
        {
            switch (NU_ATOI((CHAR*)buffer))
            {
            case 200:
                client->last_error = NU_SUCCESS;
                break;

            case 421:
                client->last_error = FTP_SERVICE_UNAVAILABLE;
                break;

            case 500:
                client->last_error = FTP_CMD_UNRECOGNIZED;
                break;

            case 501:
                client->last_error = FTP_BAD_CMD_FORMAT;
                break;

            case 522:
                client->last_error = FTP_UNKNOWN_NETWORK_PROTOCOL;
                break;

            case 530:
                client->last_error = FTP_INVALID_USER;
                break;

            default:
                client->last_error = FTP_BAD_RESPONSE;
                break;
            }
        }

        else
        {
            /* Error in call to NU_Recv(). */
            client->last_error = FTP_STACK_ERROR;
            client->stack_error = bytes_received;
            NU_Close_Socket(client->socketd);
        }
    }

    NU_USER_MODE();    /* return to user mode */

    return (client->last_error);

} /* FCP_Client_EPRT */
