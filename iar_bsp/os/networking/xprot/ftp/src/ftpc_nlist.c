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
*       ftpc_nlist.c                                   
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
*       FTPC_Client_Nlist
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

STATIC STATUS ftpc_do_nlist(FTP_CLIENT *client, CHAR *buffer, INT newsock,
                            INT bufsize);

/******************************************************************************
*
*   FUNCTION
*
*       FTPC_Client_Nlist
*
*   DESCRIPTION
*
*       This function retrieves a directory name listing from the remote host.
*       The listing will contain all the files that match the specification
*       defined by filespec, and will be stored in buffer.  If the listing
*       exceeds the space in buffer, as defined by buffsize, it is
*       truncated.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       buffer                  pointer to a buffer
*       buffsize                size of the buffer
*       filespec                specification of files to be listed
*
*   OUTPUTS
*
*       The last error generated.
*       FTP_INVALID_PARM        An required input parameter is NULL.
*
******************************************************************************/
INT FTPC_Client_Nlist(FTP_CLIENT *client, CHAR *buffer, INT buff_size,
                      CHAR *filespec)
{
    INT     status, socketd, newsock;
    struct  addr_struct servaddr, client_addr;
    FD_SET  readfs;

    NU_SUPERV_USER_VARIABLES

    if ( (client == NU_NULL) || (client->local_data_addr == NU_NULL) ||
         (buffer == NU_NULL) || (buff_size <= 0) ||
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

    if (FCP_Client_Verify_Caller(client) == NU_SUCCESS)
    {
        socketd = NU_Socket(client->ftpc_family, NU_TYPE_STREAM, 0);

        if (socketd >= 0)
        {
            UTL_Zero(&servaddr, sizeof(struct addr_struct));

            /* fill in a structure with the server address */
            servaddr.family = client->ftpc_family;

            if (client->mode == FTP_PASSIVE_MODE_ON)
            {
                /* Send a PASV command to the other side to get the server
                 * details
                 */
                status = NU_FCP_Client_PASV(client, &servaddr);
                if (status == NU_SUCCESS)
                {
                    /* In Passive mode, simply connect to the server. */
                    newsock = NU_Connect(socketd, &servaddr, 0);

                    if (socketd < 0)
                    {
                        client->last_error = FTP_STACK_ERROR;
                        client->stack_error = newsock;
                    }
                    else
                    {
                        if (FCP_Client_NLST(client, filespec) == NU_SUCCESS)
                        {
                            /* Start receiving the file listing if the remote
                             * side successfully sends "150" response
                             */
                            status = ftpc_do_nlist(client, buffer, newsock,
                                                   buff_size);
                            if (status < 0)
                            {
                                client->last_error = FTP_STACK_ERROR;
                                client->stack_error = status;
                            }

                            NU_Close_Socket(newsock);
                            FCP_Client_Tran_Ack(client, 0);
                        }
                    }
                }
            }
            else
            {
#ifdef NET_5_1
                servaddr.port = PRT_Get_Unique_Port_Number(NU_PROTO_TCP,
                                                           client->ftpc_family);
    
                memcpy(servaddr.id.is_ip_addrs, client->local_data_addr,
                       MAX_ADDRESS_SIZE);
#else
                servaddr.port = UTL_Get_Unique_Port_Number();
    
                memcpy(servaddr.id.is_ip_addrs, client->local_data_addr,
                       IP_ADDR_LEN);
#endif
    
                /* make an NU_Bind() call to bind the server's address */
                status = NU_Bind(socketd, &servaddr, 0);
    
                if (status >= 0)
                {
                    /* be ready to accept connection requests */
                    status = NU_Listen(socketd, 1);
    
                    if (status == NU_SUCCESS)
                    {
                        status = FCP_Client_EPRT(client, &servaddr);
    
                        if ( (status == FTP_CMD_UNRECOGNIZED) &&
                             (servaddr.family == NU_FAMILY_IP) )
                            status = FCP_Client_PORT(client, &servaddr);
    
                        if (status == NU_SUCCESS)
                        {
                            if (FCP_Client_NLST(client, filespec) == NU_SUCCESS)
                            {
                                NU_FD_Init(&readfs);
                                NU_FD_Set(socketd,&readfs);
    
                                status = NU_Select(socketd + 1, &readfs, NU_NULL,
                                                   NU_NULL, FTPC_INACT_TIMEOUT);
    
                                if ( (status == NU_SUCCESS) &&
                                     (NU_FD_Check(socketd, &readfs) == NU_TRUE) )
                                {
                                    newsock = NU_Accept(socketd, &client_addr, 0);

                                    if (newsock >= 0)
                                    {
                                        /* Start receiving the file listing if
                                         * everything is successful until now
                                         */
                                        status = ftpc_do_nlist(client, buffer, newsock,
                                                               buff_size);
                                        if (status < 0)
                                        {
                                            client->last_error = FTP_STACK_ERROR;
                                            client->stack_error = status;
                                        }
                                    }
    
                                    NU_Close_Socket(newsock);
                                    FCP_Client_Tran_Ack(client, 0);
                                }
                                else
                                {
                                    if (status == NU_NO_DATA)
                                        client->last_error = FTP_TIMEOUT;
                                    else
                                        client->last_error = FTP_STACK_ERROR;
                                }
                            }
                        }
                    }
                    else /* Call to NU_Listen() failed. */
                    {
                        client->last_error = FTP_STACK_ERROR;
                        client->stack_error = status;
                    }
                }
                else /* Call to NU_Bind() failed. */
                {
                    client->last_error = FTP_STACK_ERROR;
                    client->stack_error = status;
                }
    
                NU_Close_Socket(socketd);
            }
        }
        else /* Call to NU_Socket() failed. */
        {
            client->last_error = FTP_STACK_ERROR;
            client->stack_error = socketd;
        }
    }

    NU_USER_MODE();    /* return to user mode */

    return (client->last_error);

} /* FTPC_Client_Nlist */

/******************************************************************************
*
*   FUNCTION
*
*       ftpc_do_nlist
*
*   DESCRIPTION
*
*       This function actually does the work of receiving data containing the
*       file listing.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure.
*       buffer                  pointer to a buffer.
*       newsock                 pointer to character string containing path
*                               to local file.
*
*   OUTPUTS
*
*       The last error generated.
*       FTP_INVALID_PARM        An required input parameter is NULL.
*
******************************************************************************/
STATIC STATUS ftpc_do_nlist(FTP_CLIENT *client, CHAR *buffer, INT newsock,
                            INT bufsize)
{
    INT         out = 0;
    INT         index = 0;
    STATUS      status;
    INT         bytes_received;
    FD_SET      readfs;

    /* The data connection is now open. Start receiving
       the directory listing. */
    NU_FD_Init(&readfs);
    NU_FD_Set(newsock,&readfs);

    while (!out)
    {
        status = NU_Select(newsock + 1, &readfs, NU_NULL,
                           NU_NULL, FTPC_DATACT_TIMEOUT);

        if (status != NU_SUCCESS)
        {
            out++;

            if (status == NU_NO_DATA)
                client->last_error = FTP_TIMEOUT;
            else
                client->last_error = FTP_STACK_ERROR;
        }
        else
        {
            bytes_received = (INT)NU_Recv(newsock, (buffer + index),
                                     (UINT16)bufsize, 0);

            if (bytes_received < 0)
            {
                /* We assume that an error code is the result
                   of a close of the connection, signifying the
                   completion of the data transfer. */
                out++;
                *(buffer + index) = 0;
            }
            else
            {
                if (bytes_received >= bufsize)
                {
                    out++; /* We have hit our limit so truncate. */
                    *(buffer + index) = 0; /* null terminate */
                }
                else
                {
                    bufsize -= bytes_received;
                    index += bytes_received;
                }
            }
        } /* ifelse */
    } /* while */

    return (client->last_error);
} /* ftpc_do_nlist */
