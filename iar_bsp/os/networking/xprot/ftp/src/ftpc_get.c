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
*       ftpc_get.c                                     
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
*       FTPC_Client_Get
*
*   DEPENDENCIES
*
*       nucleus.h
*       target.h
*       externs.h
*       pcdisk.h
*       fc_defs.h
*       ftpc_defs.h
*       ftpc_extr.h
*       fcp_extr.h
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

STATIC STATUS ftpc_do_get(FTP_CLIENT *client, INT file_desc, INT newsock);

/******************************************************************************
*
*   FUNCTION
*
*       FTPC_Client_Get
*
*   DESCRIPTION
*
*       This function retrieves the file from the server defined in rpath and
*       stores it to the location defined by lpath.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       rpath                   pointer to character string containing path
*                               of remote file.
*       lpath                   pointer to character string containing path
*                               to local file.
*
*   OUTPUTS
*
*       The last error generated.
*       FTP_INVALID_PARM        An required input parameter is NULL.
*
******************************************************************************/
INT FTPC_Client_Get(FTP_CLIENT *client, CHAR *rpath, CHAR *lpath)
{
    INT         status, socketd, newsock;
    INT         not_good = 0;
    struct      addr_struct servaddr, client_addr;
    INT         file_desc;
    FD_SET      readfs;

    NU_SUPERV_USER_VARIABLES

    if ( (client == NU_NULL) || (client->local_data_addr == NU_NULL) ||
         (rpath == NU_NULL) || (lpath == NU_NULL) ||
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
        /* At this point, let's start building the data connection. */

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

                        /* Check if REST has been set. */
                        if (client->restart)
                        {
                            /* Send REST to server. */
                            status = FCP_Client_REST(client, client->restart);

                            if (status != NU_SUCCESS)
                            {
                                client->last_error = status;
                                client->stack_error = status;
                            }
                        }

                        if ( (status == NU_SUCCESS) &&
                             (FCP_Client_RETR(client, rpath) == NU_SUCCESS) )
                        {
                            /* Set the correct file params for REST. */
                            if ((client->restart > 0) && (status == NU_SUCCESS))
                            {
                                file_desc = NU_Open(lpath, PO_RDWR|PO_BINARY,
                                                     PS_IWRITE);
                                if (file_desc >= 0)
                                {
                                    /* Set the file descriptor to the correct part of file */
                                    status = NU_Seek(file_desc, (client->restart), PSEEK_SET);

                                    /* If seek was successful, status = restart point. */
                                    if (status != (client->restart))
                                    {
                                        if (status > 0)
                                            status = FTP_FILE_ERROR;
                                        client->last_error = FTP_STACK_ERROR;
                                        client->stack_error = status;
                                    }
                                }
                            }
                            else
                                file_desc = NU_Open(lpath,
                                                     PO_RDWR|PO_CREAT|PO_TRUNC|PO_BINARY,
                                                     PS_IWRITE);

                            if ( (status >= 0) && (file_desc >= 0) )
                            {
#ifdef NET_5_1
                                /* Enable Zero Copy mode on the socket */
                                NU_Fcntl(newsock, NU_SET_ZC_MODE, NU_ZC_ENABLE);
#endif
                                /* Start RETRieving data from the remote side if
                                 * we successfully received a "150" response
                                 */
                                status = ftpc_do_get(client, file_desc, newsock);
                                if (status < 0)
                                {
                                    client->last_error = FTP_STACK_ERROR;
                                    client->stack_error = status;
                                    not_good++;
                                }

                                NU_Close_Socket(newsock);
                                FCP_Client_Tran_Ack(client, not_good);
                            }

                            NU_Close(file_desc);
                        }
                        else
                        {
                            client->last_error = FTP_FILE_ERROR;
                        }
                    }
                }
            }
            else
            {

#ifdef NET_5_1
                servaddr.port = PRT_Get_Unique_Port_Number(NU_PROTO_TCP,
                                                           client->ftpc_family);

                memcpy(servaddr.id.is_ip_addrs,
                       client->local_data_addr->ip_num, MAX_ADDRESS_SIZE);
#else
                servaddr.port = UTL_Get_Unique_Port_Number();

                memcpy(servaddr.id.is_ip_addrs,
                       client->local_data_addr->ip_num, IP_ADDR_LEN);
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
                            /* Check if REST has been set. */
                            if (client->restart)
                            {
                                /* Send REST to server. */
                                status = FCP_Client_REST(client, client->restart);

                                if (status != NU_SUCCESS)
                                {
                                    client->last_error = status;
                                    client->stack_error = status;
                                }
                            }

                            if ( (status == NU_SUCCESS) &&
                                 (FCP_Client_RETR(client, rpath) == NU_SUCCESS) )
                            {
                                /* Set the correct file params for REST. */
                                if ((client->restart > 0) && (status == NU_SUCCESS))
                                {
                                    file_desc = NU_Open(lpath, PO_RDWR|PO_BINARY,
                                                         PS_IWRITE);
                                    if (file_desc >= 0)
                                    {
                                        /* Set the file descriptor to the correct part of file */
                                        status = NU_Seek(file_desc, (client->restart), PSEEK_SET);

                                        /* If seek was successful, status = restart point. */
                                        if (status != (client->restart))
                                        {
                                            if (status > 0)
                                                status = FTP_FILE_ERROR;
                                            client->last_error = FTP_STACK_ERROR;
                                            client->stack_error = status;
                                        }
                                    }
                                }
                                else
                                    file_desc = NU_Open(lpath,
                                                         PO_RDWR|PO_CREAT|PO_TRUNC|PO_BINARY,
                                                         PS_IWRITE);

                                if ( (status >= 0) && (file_desc >= 0) )
                                {
                                    /* Make sure we get data on this socket. */
                                    NU_FD_Init(&readfs);
                                    NU_FD_Set(socketd,&readfs);

                                    status = NU_Select(socketd + 1, &readfs,
                                                       NU_NULL, NU_NULL,
                                                       FTPC_INACT_TIMEOUT);

                                    if ( (status == NU_SUCCESS) &&
                                         (NU_FD_Check(socketd, &readfs) == NU_TRUE) )
                                    {
                                        newsock = NU_Accept(socketd, &client_addr, 0);

                                        if (newsock >= 0)
                                        {
                                            /* Start RETRieving data from the remote side
                                             *  if everything is successful until now.
                                             */
                                            status = ftpc_do_get(client, file_desc, newsock);
                                            if (status < 0)
                                            {
                                                client->last_error = FTP_STACK_ERROR;
                                                client->stack_error = status;
                                                not_good++;
                                            }
                                        }
                                        else
                                        {
                                            not_good++;
                                            client->last_error = FTP_STACK_ERROR;
                                            client->stack_error = status;
                                        }

                                        NU_Close_Socket(newsock);
                                        FCP_Client_Tran_Ack(client, not_good);
                                    }
                                    else
                                    {
                                        if (status == NU_NO_DATA)
                                            client->last_error = FTP_TIMEOUT;
                                        else
                                            client->last_error = FTP_STACK_ERROR;
                                    }

                                    NU_Close(file_desc);
                                }
                                else
                                {
                                    client->last_error = FTP_FILE_ERROR;
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
            }

            NU_Close_Socket(socketd);
        }
        else /* Call to NU_Socket() failed. */
        {
            client->last_error = FTP_STACK_ERROR;
            client->stack_error = socketd;
        }
    }

#ifndef NET_5_1
    if (NU_Deallocate_Memory(buffer) != NU_SUCCESS)
        client->last_error = FTP_STACK_ERROR;
#endif

    /* If RESTart has been set, unset it. */
    client->restart = 0;

    NU_USER_MODE();    /* return to user mode */

    return (client->last_error);

} /* FTPC_Client_Get */

/******************************************************************************
*
*   FUNCTION
*
*       ftpc_do_get
*
*   DESCRIPTION
*
*       This function actually does the work of RETRieving data from a remote
*       file and writes it to the local file.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       file_desc               file descriptor handle to the local file
*       newsock                 socket descriptor
*
*   OUTPUTS
*
*       The last error generated.
*       FTP_INVALID_PARM        An required input parameter is NULL.
*
******************************************************************************/
STATIC STATUS ftpc_do_get(FTP_CLIENT *client, INT file_desc, INT newsock)
{
    INT         out = 0;
    STATUS      status;
    INT32       bytes_received;
#ifdef NET_5_1
    NET_BUFFER  *buffer;
#else
    CHAR        *buffer;
#endif
    FD_SET      readfs;

#ifndef NET_5_1

    /* Allocate the receive buffer. */
    status = NU_Allocate_Memory(&System_Memory, (VOID**)&buffer,
                                FTPC_MAX_RECV_PACKET + 1, NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        client->last_error = FTP_MEMORY;
        client->stack_error = status;
        return (client->last_error);
    }

    /* Initialize the buffer */
    UTL_Zero(buffer, FTPC_MAX_RECV_PACKET + 1);

#endif


#ifdef NET_5_1
    /* Enable Zero Copy mode on the socket */
    NU_Fcntl(newsock, NU_SET_ZC_MODE, NU_ZC_ENABLE);
#endif

    NU_FD_Init(&readfs);
    NU_FD_Set(newsock, &readfs);

    while (!out)
    {
        status = NU_Select(newsock + 1, &readfs, NU_NULL, NU_NULL,
                           FTPC_DATACT_TIMEOUT);

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
#ifdef NET_5_1

            bytes_received = NU_ZC_Recv(newsock, &buffer, FTPC_MAX_RECV_PACKET, 0);
#else
            bytes_received = NU_Recv(newsock, buffer, FTPC_MAX_RECV_PACKET, 0);
#endif

            if (bytes_received < 0)
            {
                /* We assume that an error code is the result
                of a close of the connection, signifying the
                completion of the data transfer. */
                out++;
            }
            else
            {
#ifdef NET_5_1
                if (FTP_ZC_Write_Data(buffer, file_desc) <= 0)
#else
                if (NU_Write(file_desc, (CHAR*)buffer, bytes_received) == NU_SUCCESS)
#endif
                {
                    client->last_error = FTP_FILE_ERROR;
                    out++;
                }
#ifdef NET_5_1
                /* Return the buffer to the stack */
                NU_ZC_Deallocate_Buffer(buffer);
#endif
            }
        } /* Select() */
    } /* while */

#ifndef NET_5_1
    if (NU_Deallocate_Memory(buffer) != NU_SUCCESS)
        client->last_error = FTP_STACK_ERROR;
#endif

    return (client->last_error);
} /* ftpc_do_get */
