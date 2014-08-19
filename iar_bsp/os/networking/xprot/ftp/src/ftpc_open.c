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
*       ftpc_open.c                                    
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client-Level API
*       Functions
*
*   DESCRIPTION
*
*       This file contains the client-level API functions.  These
*       functions provide a basic FTP client implementation, also serving
*       as an example for building custom clients.  This function set is
*       built on a single-task model, in that all connections, control and
*       data, are managed by a single task.  This single task usage is
*       strictly enforced by the API.  The API also expects a file system
*       for retrieval and storage of files.  Possible alternatives would be
*       to use memory space to store retrieved files, or to send memory
*       images to store at the server.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FTPC_Client_Open
*       FTPC_Client_Open2
*
*   DEPENDENCIES
*
*       nucleus.h
*       target.h
*       externs.h
*       fc_defs.h
*       ftpc_defs.h
*       ftpc_extr.h
*       fcp_extr.h
*       pcdisk.h
*       ftp_zc_extr.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/target.h"
#include "networking/externs.h"
#include "storage/pcdisk.h"
#include "networking/fc_defs.h"
#include "networking/ftpc_def.h"
#include "networking/ftpc_ext.h"
#include "networking/fcp_extr.h"

#ifdef NET_5_1
#include "networking/ftp_zc_extr.h"
#endif

/******************************************************************************
*
*   FUNCTION
*
*       FTPC_Client_Open
*
*   DESCRIPTION
*
*       This function is for API backward compatibility when making a call
*       to open a connection with an IPv4 server.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       host_ip                 pointer to an ip address of a remote FTP host
*
*   OUTPUTS
*
*       The last error generated.
*       FTP_INVALID_PARM        An required input parameter is NULL.
*
******************************************************************************/
INT FTPC_Client_Open(FTP_CLIENT *client, IP_ADDR *host_ip)
{
    if ( (client == NU_NULL) || (host_ip == NU_NULL) )
        return (FTP_INVALID_PARM);

    client->ftpc_family = NU_FAMILY_IP;

    return (FTPC_Client_Open2(client, host_ip));

} /* FTPC_Client_Open */

/******************************************************************************
*
*   FUNCTION
*
*       FTPC_Client_Open2
*
*   DESCRIPTION
*
*       This function performs an FTP connection to the host defined by
*       the host_ip parameter.  This function also initializes the FTP_CLIENT
*       structure for this connection.  Note that this function does not
*       perform a login sequence.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       host_ip                 pointer to an ip address of a remote FTP host
*
*   OUTPUTS
*
*       The last error generated.
*       FTP_INVALID_PARM        An required input parameter is NULL.
*
******************************************************************************/
INT FTPC_Client_Open2(FTP_CLIENT *client, IP_ADDR *host_ip)
{
    INT     bytes_received;
    struct  addr_struct servaddr;
    UINT8   buffer[FTPC_GENERIC_BUFF_SIZE];

    NU_SUPERV_USER_VARIABLES

    if ( (client == NU_NULL) || (host_ip == NU_NULL) ||
         (
#if (INCLUDE_IPV4 == NU_TRUE)
         (client->ftpc_family != NU_FAMILY_IP)
#endif
#if ( (defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE) )
#if (INCLUDE_IPV4 == NU_TRUE)
         &&
#endif
         (client->ftpc_family != NU_FAMILY_IP6)
#endif
         ))
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    client->valid_pattern = 0;
    client->reply_idx = 0;
    client->reply_tail = 0;

    client->socketd = NU_Socket(client->ftpc_family, NU_TYPE_STREAM, 0);

    if (client->socketd >= 0)
    {
        /* fill in a structure with the server address */
        servaddr.family = client->ftpc_family;

        if (host_ip->port_num)
            servaddr.port = (UINT16)host_ip->port_num;
        else
            servaddr.port = FTP_WELLKNOWN;

#ifdef NET_5_1
        memcpy(servaddr.id.is_ip_addrs, host_ip->ip_num, MAX_ADDRESS_SIZE);
#else
        memcpy(servaddr.id.is_ip_addrs, host_ip->ip_num, IP_ADDR_LEN);
#endif

        servaddr.name = "ftp-host";

        client->stack_error = NU_Connect(client->socketd, &servaddr, 0);

        if (client->stack_error >= 0)
        {
            /* turn on the "block during a read" flag */
            NU_Fcntl(client->socketd, NU_SETFLAG, NU_BLOCK);

            /*  Go get the server's response.  */
            bytes_received = FCP_Reply_Read(client, buffer,
                                            FTPC_GENERIC_BUFF_SIZE,
                                            FTPC_INACT_TIMEOUT);

            if (bytes_received < 0)
            {
                /* Error in call to NU_Recv(). */
                client->last_error = FTP_STACK_ERROR;
                client->stack_error = bytes_received;
                NU_Close_Socket(client->socketd);
            }
            else
            {
                /* Check to see if the recently received data contains the
                   appropriate FTP Server message. */
                if ( (buffer[0]=='2') && (buffer[1]=='2') && (buffer[2]=='0') )
                {
                    client->valid_pattern = FTP_VALID_PATTERN;  /* We have a valid FTP
                                                                   connection at this
                                                                   point */
                    client->task_id =
                        (UNSIGNED)NU_Current_Task_Pointer();    /* Virtualize the interface to
                                                                   the kernel. */
                    client->host_addr = host_ip;
                    client->restart = 0;                        /* Clear the Restart counter. */
                    client->transfer_type = FTP_TYPE_ASCII;
                    client->last_error = NU_SUCCESS;
                }
                else
                {
                    /* The buffer did not contain the correct data.  A good FTP
                       connection has not been established.  Break off. */
                    client->last_error = FTP_BAD_HOST;
                    NU_Close_Socket(client->socketd);
                }
            }
        }
        else
        {
            /* Error in call to NU_Connect(). */
            client->last_error = FTP_STACK_ERROR;
            NU_Close_Socket(client->socketd);
        }
    }
    else
    {
        /* Error in call to NU_Socket(). */
        client->last_error = FTP_STACK_ERROR;
        client->stack_error = client->socketd;
    }

    NU_USER_MODE();    /* return to user mode */

    return (client->last_error);

} /* FTPC_Client_Open2 */
