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
*       ftpc_put.c                                     
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
*       FTPC_Client_Put
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
#include "networking/fc_extr.h"
#include "networking/ftpc_def.h"
#include "networking/ftpc_ext.h"
#include "networking/fcp_extr.h"

#ifdef NET_5_1
#include "networking/ftp_zc_extr.h"
#endif

STATIC STATUS ftpc_do_put(FTP_CLIENT *client, INT file_desc, INT newsock);

/******************************************************************************
*
*   FUNCTION
*
*       FTPC_Client_Put
*
*   DESCRIPTION
*
*       This function sends the local file defined by lpath to the server,
*       storing it at the location defined by rpath.
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
INT FTPC_Client_Put(FTP_CLIENT *client, CHAR *rpath, CHAR *lpath)
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

    NU_FD_Init(&readfs);

    if (FCP_Client_Verify_Caller(client) == NU_SUCCESS)
    {
        file_desc = NU_Open(lpath, PO_RDONLY|PO_BINARY, PS_IREAD);

        if (file_desc >= 0)
        {
            /* At this point, let's start building the data connection. */
            socketd = NU_Socket(client->ftpc_family, NU_TYPE_STREAM, 0);

            if (socketd >=0 )
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
                            /* If REST was set, send REST command. */
                            if (client->restart > 0)
                            {
                                /* Send REST to server. */
                                status = FCP_Client_REST(client, client->restart);

                                if (status == NU_SUCCESS)
                                {
                                    /* Set the file descriptor to the correct part of file */
                                    status = NU_Seek(file_desc, (client->restart),PSEEK_SET);

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
                            if ( (status >= NU_SUCCESS) &&
                                (FCP_Client_STOR(client, rpath) == NU_SUCCESS) )
                            {
                                /* Start sending data to the remote side if it
                                 * successfully sends "150" response
                                 */
                                status = ftpc_do_put(client, file_desc, newsock);
                                if (status < 0)
                                {
                                    client->last_error = FTP_STACK_ERROR;
                                    client->stack_error = status;
                                    not_good++;
                                }

                                NU_Close_Socket(newsock);

                                FCP_Client_Tran_Ack(client, not_good);
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
                                /* If REST was set, send REST command. */
                                if (client->restart > 0)
                                {
                                    /* Send REST to server. */
                                    status = FCP_Client_REST(client, client->restart);

                                    if (status == NU_SUCCESS)
                                    {
                                        /* Set the file descriptor to the correct part of file */
                                        status = NU_Seek(file_desc, (client->restart),PSEEK_SET);

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
                                if ( (status >= NU_SUCCESS) &&
                                    (FCP_Client_STOR(client, rpath) == NU_SUCCESS) )
                                {
                                    /* Make sure we get data on this socket. */
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
                                            status = NU_Fcntl(newsock, NU_SETFLAG, NU_BLOCK);

                                            if (status == NU_SUCCESS)
                                            {
                                                /* Start STORing data to the remote file if everything
                                                 * is successful until now.
                                                 */
                                                status = ftpc_do_put(client, file_desc, newsock);
                                                if (status < 0)
                                                {
                                                    client->last_error = FTP_STACK_ERROR;
                                                    client->stack_error = status;
                                                    not_good++;
                                                }
                                            } /* if (status == NU_SUCCESS) */
                                            else
                                            {
                                                client->last_error = FTP_STACK_ERROR;
                                                client->stack_error = status;
                                                not_good++;
                                            }

                                            NU_Close_Socket(newsock);

                                        } /* if (newsock >= 0) */
                                        else
                                        {
                                            not_good++;
                                            client->last_error = FTP_STACK_ERROR;
                                            client->stack_error = status;
                                        }

                                        FCP_Client_Tran_Ack(client, not_good);
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

            NU_Close(file_desc);
        }
        else
        {
            client->last_error = FTP_FILE_ERROR;
        }
    }

    /* If a restart point was set, unset it. */
    client->restart = 0;

    NU_USER_MODE();    /* return to user mode */

    return (client->last_error);

} /* FTPC_Client_Put */

/******************************************************************************
*
*   FUNCTION
*
*       ftpc_do_put
*
*   DESCRIPTION
*
*       This function actually does the work of STORing data to a remote file
*       by reading the local file and sending it across to the remote side.
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
STATIC STATUS ftpc_do_put(FTP_CLIENT *client, INT file_desc, INT newsock)
{
    INT         out = 0;
    STATUS      status;
    INT         bytes_read;
#ifdef NET_5_1
    NET_BUFFER  *buffer;
    UINT32      file_size;
#else
    CHAR        *buffer;
#endif

#ifndef NET_5_1

    /* Allocate memory for the send buffer */
    status = NU_Allocate_Memory(&System_Memory, (VOID *)&buffer,
                                FTPC_TRANSFER_BUFF_SIZE, NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        client->last_error = FTP_MEMORY;
        client->stack_error = status;
        return (client->last_error);
    }
#endif

#ifdef NET_5_1
    /* Enable Zero Copy mode on the socket */
    NU_Fcntl(newsock, NU_SET_ZC_MODE, NU_ZC_ENABLE);

    /* Get the length of the file to be transmitted */
    file_size = FTP_Handle_File_Length(file_desc) - (client->restart);
#endif

    while (!out)
    {
#ifdef NET_5_1
        /* Copy the data from the file into Zero Copy buffers */
        bytes_read = FTP_ZC_Read_Data(&buffer, file_size, newsock, file_desc);
#else
        bytes_read = NU_Read(file_desc, buffer, FTPC_TRANSFER_BUFF_SIZE);
#endif
        if (bytes_read == FTP_EOF)
        {
            out++;
            client->last_error = NU_SUCCESS;
        }
        else
        {
            if (bytes_read > 0)
            {
#ifdef NET_5_1
                /* Send the datagram. */
                status = (INT)NU_ZC_Send(newsock, (CHAR*)buffer, (UINT16)bytes_read, 0);
#else
                status = NU_Send(newsock, buffer, (UINT16)bytes_read,0);
#endif
                if (status < 0)
                {
                    client->last_error = FTP_STACK_ERROR;
                    client->stack_error = status;
                    out++;

#ifdef NET_5_1
                    /* If the data was not sent, the application is
                     * responsible for freeing the buffer.
                     */
                    NU_ZC_Deallocate_Buffer(buffer);
#endif
                }

#ifdef NET_5_1
                else
                {
                    /* Decrement the size of the file to be transmitted */
                    file_size -= (UINT32)bytes_read;

                    if (file_size == 0)
                    {
                        out++;
                        client->last_error = NU_SUCCESS;
                    }
                }
#endif
            }
            else
            {
                client->last_error = FTP_FILE_ERROR;
                out++;
            }
        }
    }
#ifndef NET_5_1
    if (NU_Deallocate_Memory(buffer) != NU_SUCCESS)
        client->last_error = FTP_STACK_ERROR;
#endif

    return (client->last_error);
} /* ftpc_do_put */
