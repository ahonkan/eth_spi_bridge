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
*       ftps.c                                         
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Server-Level API
*       Functions
*
*   DESCRIPTION
*
*      This file contains the server-level functions.  These functions
*      provide a basic FTP server implementation, also serving as an
*      example for building custom servers.  This function set is built on
*      a multiple-task model, in that all connections, control and data, are
*      managed by separate tasks.  The API also expects a file system for
*      retrieval and storage of files.  Possible alternatives would be to
*      use memory space to store retrieved files, or to send memory images
*      to store at the server.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FTPS_Server_Session_Init
*       FTPS_Server_Structure_Init
*       FTPS_Create_Data_Task
*       FTPS_Server_Get
*       FTPS_Server_Put
*       FTPS_Server_ChDir
*       FTPS_Server_Dir
*       FTPS_Server_MkDir
*       FTPS_Server_RmDir
*       FTPS_Server_PrintDir
*       FTPS_Server_Rename_File
*       FTPS_Server_Delete_File
*       FTPS_Get_Rename
*       Str_Equal
*       FTPS_Server_File_Error
*       FTPS_File_Time
*       FTPS_UserInit
*       FTPS_Parsedrive
*
*   DEPENDENCIES
*
*       nucleus.h
*       target.h
*       ncl.h
*       externs.h
*       fc_defs.h
*       fc_extr.h
*       ftps_defs.h
*       ftps_ext.h
*       fst_extr.h
*       fsp_extr.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/target.h"
#include "networking/ncl.h"
#include "networking/externs.h"
#ifdef NET_5_1
#include "networking/um_defs.h"
#include "networking/zc_defs.h"
#include "networking/ftp_zc_extr.h"
#endif
#include "networking/fc_defs.h"
#include "networking/fc_extr.h"
#include "networking/ftps_def.h"
#include "networking/ftps_ext.h"
#include "networking/fst_extr.h"
#include "networking/fsp_extr.h"

CHAR    FTPS_Svrcontrol[] = "FTPSctrl";
CHAR    FTPS_Clicontrol[] = "FTPCctrl";
CHAR    FTPS_Srvdata[] = "FTPSdata";
CHAR    FTPS_Clidata[] = "FTPCdata";

CHAR FTP_Month[12][5] = {"Jan ", "Feb ", "Mar ", "Apr ", "May ", "Jun ", "Jul ",
                         "Aug ", "Sep ", "Oct ", "Nov ", "Dec "};

extern NU_MEMORY_POOL   *FTPS_Memory;
extern NU_PROTECT       FST_Active_List_Protect;
extern FST_ACTIVE_LIST  FST_Active_List[FTP_SERVER_MAX_CONNECTIONS];

#ifndef NET_5_1
extern FTPSACCT         FTP_Password_List[];
#endif

STATIC INT16 FTPS_Parsedrive(UINT8  *path);
STATIC CHAR* FTPS_Format_Pathname(CHAR *buffer, CHAR *pathname);

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Server_Session_Init
*
*   DESCRIPTION
*
*      This function checks the first FTP command from the client.
*      If the first command is not USER then an error message is returned.
*      Parameters are a pointer to an FTP server structure and the socket
*      ID number.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*       socketID                socket descriptor
*       hardDrive               default hard drive
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the host.
*
******************************************************************************/
INT FTPS_Server_Session_Init(FTP_SERVER *server, UNSIGNED socketID, INT hardDrive,
                             INT16 family)
{
    INT32   status;
    INT32   bytesReceived;
    INT     bytesSent;

    status = NU_Create_Event_Group(&server->FTP_Events, "FTP_Evts");

    if (status == NU_SUCCESS)
    {
        status = FTPS_Server_Structure_Init(server, socketID, hardDrive, family);

        if (status != NU_SUCCESS)
        {
            if (status == NU_BAD_SOCKETD)
                FTP_Printf("Invalid socket descriptor.\r\n");

            else if (status == NU_NOT_CONNECTED)
                FTP_Printf("No client connected to socket descriptor.\r\n");

            server->lastError = (INT)status;
        }
        else
        {
            /* Send 'Server Ready' Reply */
            bytesSent = (INT)NU_Send((INT)(server->socketd), MSG220, SIZE220, 0);

            if (bytesSent < 0)
            {
                FTP_Printf("Session_Init - write Send failed.\r\n");
                server->lastError = bytesSent;
            }
            else
            {
                /*  Go get the client's response.  */
                bytesReceived = FSP_Command_Read(server, server->replyBuff,
                                                 FTPS_GENERIC_BUFF_SIZE,
                                                 FTPS_DATA_TIMEOUT);

                if (bytesReceived < 0)
                {
                    /* Error in call to FSP_Command_Read(). */
                    if (bytesReceived == FTP_TIMEOUT)
                    {
                        FTP_Printf("FTP Server timeout in FTPS_Server_Session_Init");
                    }
                    else
                    {
                        FTP_Printf("Session_Init - read Send failed.\r\n");
                        server->lastError = FTP_STACK_ERROR;
                        server->stackError = (INT)bytesReceived;
                    }
                }
                else
                {
#if (IMF_INCLUDED)
                    /* Send IMF error Reply */
                    status = NU_Send((INT)(server->socketd), ERRORMSG,
                                     ERRORMSGSZ, 0);

                    if (status < 0)
                    {
                        FTP_Printf("Session_Init - Send failed.\r\n");
                        server->stackError = bytesSent;
                        server->lastError = FTP_STACK_ERROR;
                    }
#else
                    /* Check if USER command was issued. */
                    if ( (server->replyBuff[0]=='U') &&
                         (server->replyBuff[1]=='S') &&
                         (server->replyBuff[2]=='E') &&
                         (server->replyBuff[3]=='R') )
                    {
                        server->lastError = FTPS_UserInit(server);
                    }
                    else    /* NTWK_10813. */
                    {
                        server->lastError = FTP_BAD_MSG_FORMAT;
                        status = NU_Send((INT)(server->socketd), MSG530,
                                         SIZE530, 0);
                        if (status < 0)
                        {
                            FTP_Printf("Session_Init - Send failed.\r\n");
                        }
                    }
#endif
                }
            }
        }
    }
    else
    {
        FTP_Printf("Session_Init - Unable to create the FTP Event Group.\r\n");
        server->lastError = FTP_STACK_ERROR;
        server->stackError = (INT)status;
    }

    return (server->lastError);

} /* FTPS_Server_Session_Init */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_UserInit
*
*   DESCRIPTION
*
*       This function gets the user name and password in the correct
*       order. Note that if the user and password are not correct, this
*       function still returns success, since the server will need to
*       read in more commands.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the host.
*       FTP_BAD_CMD_SEQUENCE    The host did not issue PASS command.
*
******************************************************************************/
INT FTPS_UserInit(FTP_SERVER *server)
{
    INT status;
    INT bytesReceived;

    /* clear login command sequence flags */
    server->cmdFTP.userFlag = 0;
    server->cmdFTP.passFlag = 0;

    status = FSP_Server_USER(server);

    if (status == NU_SUCCESS)
    {
        /*  Go get the client's password.  */
        bytesReceived = FSP_Command_Read(server, server->replyBuff,
                                         FTPS_GENERIC_BUFF_SIZE, FTPS_DATA_TIMEOUT);

        if (bytesReceived < 0)
        {
            /* Error in call to FSP_Command_Read(). */
            if (bytesReceived == FTP_TIMEOUT)
            {
                FTP_Printf("FTP Server timeout in FTPS_Server_Session_Init");
            }
            else
            {
                FTP_Printf("UserInit - read Send failed.\r\n");
                server->lastError = FTP_STACK_ERROR;
                server->stackError = bytesReceived;
            }
        }
        else
        {
            /* Check if PASS command was issued. */
            if ( (server->replyBuff[0]=='P') &&
                 (server->replyBuff[1]=='A') &&
                 (server->replyBuff[2]=='S') &&
                 (server->replyBuff[3]=='S') )
            {
                if (FSP_Server_PASS(server) != NU_SUCCESS)
                {
                    /* clear login flags */
                    server->cmdFTP.userFlag = 0;
                    server->cmdFTP.passFlag = 0;
                }

                /* Return success so the server can continue      */
                /* receiving commands, regardless of login state. */
                server->lastError = NU_SUCCESS;
            }
            else
            {
                /* Send 'Bad command sequence' message */
                status = (INT)NU_Send(server->socketd, MSG503, SIZE503, 0);

                if (status < 0)
                {
                    FTP_Printf("USERInit - Cmd Send failed.\r\n");
                    server->stackError = status;
                }
                else
                    server->lastError = FTP_BAD_CMD_SEQUENCE;
            }
        }
    }

    else
        server->lastError = status;

  return (server->lastError);

} /* FTPS_UserInit */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Init_Server_Structure
*
*   DESCRIPTION
*
*       This function initializes the server structure.  The only parameter is
*       a pointer to an FTP server structure.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*       socketID                socket descriptor
*       drive                   default hard drive for file system
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       Network layer generated error codes.
*
******************************************************************************/
INT FTPS_Server_Structure_Init(FTP_SERVER *server, UNSIGNED socketID, INT drive,
                               INT16 family)
{
    INT     i;
    STATUS  status;
    INT16   addrLength;
    struct  sockaddr_struct peer;

    addrLength = sizeof(struct sockaddr_struct);

    server->validPattern = 0;
    server->taskID       = NU_Current_Task_Pointer();

    status = NU_Task_Information(server->taskID, server->taskName,
                                 &(server->taskStatus), &(server->scheduleCount),
                                 &(server->priority), &(server->preempt),
                                 &(server->timeSlice), &(server->stackBase),
                                 &(server->stackSize), &(server->minStack));

    if (status != NU_SUCCESS)
        FTP_Printf("Invalid task pointer.\r\n");

    /* Get the local server address info from the control connection. */
    status = NU_Get_Sock_Name((INT)socketID, &peer, &addrLength);

    if (status != NU_SUCCESS)
        FTP_Printf("Cannot get local server address info.\r\n");

    /* Initialize the server address info. This is mainly a placeholder
       for null information, which will be resolved during a bind(). */
    server->serverAddr.family = family;
    server->serverAddr.port = peer.port_num;

#ifdef NET_5_1
    memcpy(&server->serverAddr.id.is_ip_addrs, &peer.ip_num, MAX_ADDRESS_SIZE);

    memcpy(&server->serverDataAddr.id.is_ip_addrs, &peer.ip_num,
           MAX_ADDRESS_SIZE);

    memcpy(&server->clientAddr.id.is_ip_addrs, &peer.ip_num, MAX_ADDRESS_SIZE);

    memcpy(&server->clientDataAddr.id.is_ip_addrs, &peer.ip_num,
           MAX_ADDRESS_SIZE);
#else
    memcpy(&server->serverAddr.id.is_ip_addrs, &peer.ip_num, IP_ADDR_LEN);

    memcpy(&server->serverDataAddr.id.is_ip_addrs, &peer.ip_num, IP_ADDR_LEN);

    memcpy(&server->clientAddr.id.is_ip_addrs, &peer.ip_num, IP_ADDR_LEN);

    memcpy(&server->clientDataAddr.id.is_ip_addrs, &peer.ip_num, IP_ADDR_LEN);
#endif

    server->serverAddr.name = FTPS_Svrcontrol;
    server->socketd = (INT)socketID;

    server->serverDataAddr.family = family;
    server->serverDataAddr.port = peer.port_num;

    server->serverDataAddr.name = FTPS_Srvdata;

    /* Get the client's address info. */
    status = NU_Get_Peer_Name((INT)socketID, &peer, &addrLength);

    /* Initialize the client's control connection port address. The data
     connection port address must always be set by the PORT command
     for this implementation. */
    server->clientAddr.family = family;
    server->clientAddr.port = peer.port_num;
    server->clientAddr.name = FTPS_Clicontrol;

    server->clientDataAddr.family = family;
    server->clientDataAddr.port = peer.port_num;
    server->clientDataAddr.name = FTPS_Clidata;

    server->socketd      = (INT16)socketID;
    server->dataSocketd  = -1;
    server->transferType = -1;
    server->lastError    = status;
    server->stackError   = -855;
    server->replyIndex   = 0;
    server->replyTail    = 0;

    for (i = 0; i < FTPS_REPLY_BUFFER_SIZE; i++)
        server->replyBuff[i] = '\0';

    for (i = 0; i < FTPS_GENERIC_BUFF_SIZE; i++)
    {
        server->fileSpec[i]  = '\0';
        server->path[i] = '\0';
        server->renamePath[i] = '\0';
    }

    strcpy(server->currentWorkingDir, FTPS_NATIVE_PATH_DELIMITER_STR);

    server->defaultDrive = drive;/* Select C as default drive */

    server->cmdFTP.userFlag = 0;
    server->cmdFTP.passFlag = 0;
    server->cmdFTP.rnfrFlag = 0;
    server->cmdFTP.typeFlag = 'I';/* Binary file transfers */
    server->cmdFTP.modeFlag = 'S';/* Transfer mode = Stream */
    server->cmdFTP.struFlag = 'F';/* File Structure */
    server->cmdFTP.taskFlag = 0;

    server->restart = 0;

    return (status);

} /* FTPS_Server_Structure_Init */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Create_Data_Task
*
*   DESCRIPTION
*
*       This function creates the data connection used for transfers between
*       the client and this server. Received commands that will use this
*       data connection task are LIST, NLST, STOR, APPE and RETR.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       OS-specific error codes.
*
******************************************************************************/
INT FTPS_Create_Data_Task(FTP_SERVER *server)
{
    VOID            *pointer;
    INT             status;
    CHAR            taskname[8] = "DATA";
    static UINT8    tasksCreatedCount = 0, i;

    /***** Allocate Memory for the NU_TASK Control Block *****/
    status = NU_Allocate_Memory(FTPS_Memory, (VOID **)&(server->datatask),
                                sizeof(NU_TASK), NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        UTL_Zero(server->datatask, sizeof(NU_TASK));

        /* 'Build' task name based on tasksCreatedCount variable */
        NU_ITOA((INT)(tasksCreatedCount), &taskname[4], 10);

        /***** Create FTP server Data_Recv_Task.  *****/
        status = NU_Allocate_Memory(FTPS_Memory, &pointer,
                                    FTPS_DATA_STACK_SIZE, NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            status = NU_Create_Task(server->datatask, taskname, FST_Data_Task_Entry,
                                    0, (VOID *)server, pointer,
                                    FTPS_DATA_STACK_SIZE, FTPS_DATA_PRIORITY,
                                    DATA_TIME_SLICE, NU_PREEMPT, NU_NO_START);

            if (status == NU_SUCCESS)
            {
                NU_Protect(&FST_Active_List_Protect);

                /* Add Task ID to list of active client tasks */
                for (i = 0; i < FTP_SERVER_MAX_CONNECTIONS; i++)
                {
                    if (FST_Active_List[i].active_task == NU_NULL)
                    {
                        FST_Active_List[i].active_task = server->datatask;
                        FST_Active_List[i].active_sckt = -1;
                        break;
                    }
                }

                NU_Unprotect();

                tasksCreatedCount++;/* Update counter for next name */

                NU_Resume_Task(server->datatask);
            }

            else /* NU_Create_Task() failed */
            {
                FTP_Printf("***** Unable to create FTP server data task *****\r\n");
                NU_Deallocate_Memory(pointer);
                NU_Deallocate_Memory((VOID *)server->datatask);
            }
        }

        else /* NU_Allocate_Memory() for task failed. */
        {
            FTP_Printf("***** Unable to Allocate data task stack memory *****\r\n");
            NU_Deallocate_Memory((VOID *)server->datatask);
        }
    }
    else /* NU_Allocate_Memory() for control block failed. */
    {
        FTP_Printf("***** Unable to allocate data task control block memory.\r\n");
    }

    return (status);

} /* FTPS_Create_Data_Task */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Server_Get
*
*   DESCRIPTION
*
*       This routine creates and/or appends the requested file using the Zero
*       Copy interface if a version of NET that supports Zero Copy is being
*       used in the system.  This routine also generates the appropriate
*       messages to implement the data transfers from the client to this FTP
*       server.  Parameters are a pointer to an FTP server structure.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the host.
*       File system specific error codes.
*
******************************************************************************/
INT FTPS_Server_Get(FTP_SERVER *server, UINT32 gtype)
{
    INT         status, FileErr = 0;/* FileErr -  Partial Data Transfer Error*/
    INT         bytesReceived;
    INT32       filePointer = 0, rest_seek = 0;
    INT         fd;
    SIGNED      totalBytes;
    INT16       mode;
    /* needed by NU_Select() */
    FD_SET      readfs;
    CHAR        nu_drive[3];

    mode = PS_IWRITE;

    /* Determine what drive should the current operation be performed on */
    nu_drive[0] = (CHAR)(((server->defaultDrive) + 1) + 'A' - 1);
    nu_drive[1] = ':';
    nu_drive[2] = '\0';

    /* Grab the File Write semaphore to write to the File. */
    status = NU_Obtain_Semaphore(&(server->fileWriteLock), NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        status = NU_Become_File_User();

        if (status == NU_SUCCESS)
        {
            /* Open disk for accessing */
            status = NU_Open_Disk (nu_drive);

            /* If the disk is already open, return success and leave the current
             * directory where it is.
             */
            if (status == NUF_NO_ACTION)
                status = NU_SUCCESS;

            if (status == NU_SUCCESS)
            {
                status = NU_Set_Default_Drive(server->defaultDrive);

                /* set default drive to C: for files */
                if (status == NU_SUCCESS)
                {
                    /* setup the current working directory for this task */
                    status = NU_Set_Current_Dir((CHAR *)server->currentWorkingDir);

                    if (status == NU_SUCCESS)
                    {
                        /* Place file pointer at end of the file */
                        if (gtype == APPE_Event) /* APPE Command */
                        {
                            fd = NU_Open((CHAR *)server->fileSpec, (PO_WRONLY|PO_CREAT|PO_BINARY), (UINT16)mode);

                            if (fd >= 0)
                                filePointer = NU_Seek(fd,0,PSEEK_END);
                        }
                        else /* STOR Command */
                        {
                            /* Check if REST was set.  If so, use correct File Descriptor. */
                            if (server->restart > 0)
                                fd = NU_Open((CHAR *)server->fileSpec,
                                             (PO_WRONLY|PO_CREAT|PO_BINARY), (UINT16)mode);
                            else
                                fd = NU_Open((CHAR *)server->fileSpec,
                                             (PO_WRONLY|PO_CREAT|PO_BINARY|PO_TRUNC),
                                             (UINT16)mode);
                        }

                        /* Make sure the file is open. */
                        if ( (fd < 0) || (filePointer < 0) )
                        {
                            FTPS_Server_File_Error(server);

                            /* Send 'Requested file action not taken' message */
                            status = (INT)NU_Send((INT)(server->socketd), MSG450, SIZE450, 0);

                            if (status < 0)
                            {
                                FTP_Printf("FTPS_Server_Get - Send failed.\r\n");
                                server->lastError = FTP_STACK_ERROR;
                                server->stackError = status;
                            }
                        }
                        else/* NU_Open successful */
                        {
                            /* Send 'File status okay' message */
                            status = (INT)NU_Send((INT)(server->socketd), MSG150, SIZE150, 0);

                            if (status < 0)
                            {
                                FTP_Printf("FTPS_Server_Get - Send failed.\r\n");
                                server->lastError = FTP_STACK_ERROR;
                                server->stackError = status;
                            }

                            /* REST command implementation for STOR */
                            if ((server->restart > 0) && (gtype == STOR_Event))
                            {
                                /* Set the file descriptor to the correct part of file */
                                rest_seek = NU_Seek(fd, server->restart, PSEEK_SET);

                                /* If seek failed, return failure */
                                if (rest_seek != (server->restart))
                                {
                                    if (rest_seek >= 0)
                                    {
                                        status = FTP_FILE_ERROR;

                                        /* Set the file descriptor to the beginning of
                                         * the file
                                         */
                                        NU_Seek(fd, 0, PSEEK_SET);
                                    }
                                    else
                                    {
                                        status = rest_seek;
                                    }
                                    server->lastError = FTP_STACK_ERROR;
                                    server->stackError = status;
                                }
                            }

                            if (status < 0)
                            {
                                /* Send 'Requested file action not taken' message */
                                NU_Send((INT)(server->socketd), MSG550, SIZE550, 0);
                            }
                            else
                            {
                                /*  Go get the client's data.  */
                                totalBytes = 0;

                                /* set up a file for read for NU_Select() */
                                NU_FD_Init(&readfs);
                                do
                                {
                                    /* reset the Write Data status to zero so that the loop will terminate */
                                    status = 0;

                                    /* reset the number of bytes received from the client to zero
                                     * for detecting an error
                                     */
                                    bytesReceived = 0;

                                    /* required by Nu_Select to bind a socket with a file for read */
                                    NU_FD_Set(server->dataSocketd, &readfs);

                                    /* Wait for data to arrive on one of the sockets. In this case just
                                     * one socket.
                                     */
                                    if (NU_Select(server->dataSocketd + 1, &readfs, NU_NULL, NU_NULL,
                                                  FTPS_DATA_TASK_TIMEOUT) == NU_SUCCESS)
                                    {

#ifdef NET_5_1

                                        /* Get the data from the client */
                                        bytesReceived = (INT)NU_ZC_Recv(server->dataSocketd,
                                                                        &server->ftpsDataBuf,
                                                                        (UINT16)FTPS_REPLY_BUFFER_SIZE, 0);

                                        if (bytesReceived > 0)
                                        {
                                            /* Copy the data from the buffer and write to the file */
                                            status = FTP_ZC_Write_Data(server->ftpsDataBuf, fd);

                                            /* If an error occurred, notify the other side. */
                                            if (status <= 0)
                                            {
                                                /* if there is data on the socket, but it didn't get written to file,*/
                                                /* declare a file error */
                                                FileErr = NU_TRUE;
                                                /* let the FTP server know about the error */
                                                FTPS_Server_File_Error(server);
                                            }

                                            /* Free the buffer */
                                            NU_ZC_Deallocate_Buffer(server->ftpsDataBuf);
                                        }
                                        else
                                        {
                                            status = 0;
											break;
                                        }
#else
                                        /* Get the data from the client */
                                        bytesReceived = NU_Recv(server->dataSocketd, server->replyBuff,
                                                                (UINT16)(sizeof(server->replyBuff)), 0);


                                        /* write/update file if valid data has been read */
                                        if (bytesReceived > 0)
                                        {
                                            status = NU_Write(fd, (CHAR *)server->replyBuff, bytesReceived);

                                            if (status <= 0)
                                                FTPS_Server_File_Error(server);
                                        }
                                        else
                                        {
                                            status = 0;
                                        }
#endif

                                        totalBytes += bytesReceived;
                                    }

                                } while (status > 0);

                                /* Check for the status */
                                if ((status < 0) || ((bytesReceived > 0) && (status <= 0)))
                                {
                                    /* Set the Partial Data Error Flag */
                                    FileErr = 1;

                                    /* Send 'Transfer aborted: Insufficient Storage Space' message */
                                    status = (INT)NU_Send((INT)(server->socketd), MSG452, SIZE452, 0);

                                    if (status < 0)
                                    {
                                        FTP_Printf("FTPS_Server_Get - Send failed.\r\n");
                                        server->lastError = FTP_STACK_ERROR;
                                        server->stackError = status;
                                    }

                                }
                                else if (totalBytes == 0)  /* Check if any bytes for received */
                                {
                                    FTP_Printf("Unable to receive file contents. Recv failed.\r\n");
                                    server->lastError = FTP_STACK_ERROR;
                                    server->stackError = status;

                                    /* Send 'Transfer aborted' message */
                                    status = (INT)NU_Send((INT)(server->socketd), MSG426, SIZE426, 0);

                                    if (status < 0)
                                    {
                                        FTP_Printf("FTPS_Server_Get - Send failed.\r\n");
                                        server->lastError = FTP_STACK_ERROR;
                                        server->stackError = status;
                                    }

                                }
                                else/* File transfer is successful */
                                {
                                    /* Send 'Closing data connection' message */
                                    status = (INT)NU_Send((INT)(server->socketd), MSG226, SIZE226, 0);

                                    if (status < 0)
                                    {
                                        FTP_Printf("FTPS_Server_Get - Send failed.\r\n");
                                        server->lastError = FTP_STACK_ERROR;
                                        server->stackError = status;
                                    }
                                }
                            }

                            /* Close the file */
                            status = NU_Close(fd);

                            if (status != NU_SUCCESS)
                                FTPS_Server_File_Error(server);

                            /* Delete the file containing partial data */
                            if (FileErr)
                            {
                                status = NU_Delete((CHAR *)server->fileSpec);

                                if (status != NU_SUCCESS)
                                    FTPS_Server_File_Error(server);
                            }
                        }

                        /* If restart point was set, unset it. */
                        server->restart = 0;
                    }
                    else
                    {
                        /* NU_Set_Curr_Dir() failed */
                        FTP_Printf("NU_Set_Curr_Dir() failed.\r\n");
                        server->lastError = FTP_CURRENT_DIR_FAILURE;

                        /* Send 'Requested action aborted' message */
                        if (NU_Send(server->socketd, MSG426, SIZE426,0) < 0)
                        {
                            FTP_Printf("FST_Data_Task NU_Send failure.\r\n");
                            server->lastError = FTP_STACK_ERROR;
                        }
                    }
                }
                else /* Set default drive failed. */
                {
                    FTP_Printf("FST_Data_Task cannot select the default drive.\r\n");
                    server->lastError = status;

                    /* Send 'Requested action aborted' message */
                    if (NU_Send(server->socketd, MSG426, SIZE426, 0) < 0)
                    {
                        FTP_Printf("FST_Data_Task NU_Send failure.\r\n");
                        server->lastError = FTP_STACK_ERROR;
                    }
                }

                NU_Close_Disk (nu_drive);
            }
            else
            {
                FTP_Printf("FST_Data_Task cannot open default disk.\r\n");
                server->lastError = status;

                /* Send 'Requested action aborted' message */
                status = (INT)NU_Send(server->socketd, MSG426, SIZE426, 0);

                if (status < 0)
                {
                    FTP_Printf("FST_Data_Task NU_Send failure.\r\n");
                    server->lastError = FTP_STACK_ERROR;
                }
            }

            NU_Release_File_User();
        }
        else
        {
            FTP_Printf("FST_Data_Task cannot register as file system user.\r\n");
            server->lastError = FTP_REGISTRATION_FAILURE;

            /* Send 'Requested action aborted' message */
            if (NU_Send(server->socketd, MSG426, SIZE426, 0) < 0)
            {
                FTP_Printf("FST_Data_Task NU_Send failure.\r\n");
                server->lastError = FTP_STACK_ERROR;
            }
        }

        NU_Release_Semaphore(&(server->fileWriteLock));
    }

    return (status);

} /* FTPS_Server_Get */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Server_Put
*
*   DESCRIPTION
*
*       This routine sends the requested file using the Zero Copy interface
*       if a version of NET that supports Zero Copy is being used in the
*       system.  This routine also generates the appropriate messages to
*       implement the data transfers from this FTP server to the client.
*       Parameters are a pointer to an FTP server structure.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the host.
*       File system specific error codes.
*
******************************************************************************/
INT FTPS_Server_Put(FTP_SERVER *server)
{
    INT         status;
    INT         bytesReceived;
    INT         fd;
    SIGNED      totalBytes;
    INT16       mode;
    INT32       rest_seek = 0;
    UINT32      file_size;
    CHAR        nu_drive[3];

    mode = PS_IREAD;

    /* Determine what drive should the current operation be performed on */
    nu_drive[0] = (CHAR)(((server->defaultDrive) + 1) + 'A' - 1);
    nu_drive[1] = ':';
    nu_drive[2] = '\0';

    status = NU_Become_File_User();

    if (status == NU_SUCCESS)
    {
        /* Open disk for accessing */
        status = NU_Open_Disk (nu_drive);

        /* If the disk is already open, return success and leave the current
         * directory where it is.
         */
        if (status == NUF_NO_ACTION)
            status = NU_SUCCESS;

        if (status == NU_SUCCESS)
        {
            status = NU_Set_Default_Drive(server->defaultDrive);

            /* set default drive to C: for files */
            if (status == NU_SUCCESS)
            {
                /* setup the current working directory for this task */
                status = NU_Set_Current_Dir((CHAR *)server->currentWorkingDir);

                if (status == NU_SUCCESS)
                {
                    fd = NU_Open((CHAR *)server->fileSpec, PO_RDONLY|PO_BINARY, (UINT16)mode);

                    if (fd < 0)
                    {
                        FTPS_Server_File_Error(server);

                        /* Send 'Requested file action not taken' message */
                        status = (INT)NU_Send((INT)(server->socketd), MSG550, SIZE550, 0);

                        if (status < 0)
                        {
                            FTP_Printf("FTPS_Server_Put - Send failed.\r\n");
                            server->lastError = FTP_STACK_ERROR;
                            server->stackError = status;
                        }
                    }

                    else/* NU_Open successful */
                    {
                        /* Send 'File status okay' message */
                        status = (INT)NU_Send((INT)(server->socketd), MSG150, SIZE150, 0);

                        if (status < 0)
                        {
                            FTP_Printf("FTPS_Server_Put - Send failed.\r\n");
                            server->lastError = FTP_STACK_ERROR;
                            server->stackError = status;
                        }

                        /*  Go get the data from the file.  */
                        totalBytes = 0;

                        /* Get the length of the file to be transmitted */
                        file_size = FTP_Handle_File_Length(fd);

                        /* REST command implementation for RETR */
                        if (server->restart > 0)
                        {
                            if (server->restart > file_size)
                            {
                                /* Set an error because the requested Restart size is bigger
                                 * than the file itself.
                                 */
                                status = FTP_FILE_ERROR;
                            }
                            else
                            {
                                /* Set the file descriptor to the correct part of file */
                                rest_seek = NU_Seek(fd, server->restart, 0);

                                /* If seek failed, return failure */
                                if (rest_seek != (server->restart))
                                {
                                    if (rest_seek >= 0)
                                    {
                                        status = FTP_FILE_ERROR;

                                        /* Set the file descriptor to the beginning of the file */
                                        NU_Seek(fd, 0, PSEEK_SET);
                                    }
                                    else
                                    {
                                        status = rest_seek;
                                    }
                                    server->lastError = FTP_STACK_ERROR;
                                    server->stackError = status;
                                }

                                else
                                {
                                    /* Update the Size of the file to be transmitted */
                                    file_size -= rest_seek;
                                }
                            }
                        }

                        if (status < 0)
                        {
                            /* Send 'Requested file action not taken' message */
                            NU_Send((INT)(server->socketd), MSG550, SIZE550, 0);
                        }
                        else
                        {
                            do
                            {
#ifdef NET_5_1

                                /* Copy the data from the file into Zero Copy buffers */
                                bytesReceived = FTP_ZC_Read_Data(&server->ftpsDataBuf,
                                                                 file_size,
                                                                 server->dataSocketd, fd);

                                /* If data was copied into the buffer, send the data. */
                                if (bytesReceived > 0)
                                {
                                    /* Send the datagram. */
                                    status = (INT)NU_ZC_Send(server->dataSocketd,
                                                        (CHAR*)server->ftpsDataBuf,
                                                        (UINT16)bytesReceived, 0);

                                    /* If the data was not sent, the application is
                                     * responsible for freeing the buffer.
                                     */
                                    if (status < 0)
                                        NU_ZC_Deallocate_Buffer(server->ftpsDataBuf);

                                    /* Decrement the size of the file to be transmitted */
                                    file_size -= (UINT32)bytesReceived;
                                }

                                /* A buffer could not be allocated or there is no more
                                 * data in the file.  Either way, stop processing.
                                 */
                                else
                                    break;
#else

                                bytesReceived = NU_Read(fd, (CHAR *)server->replyBuff, sizeof(server->replyBuff));

                                if (bytesReceived > 0)
                                    status = NU_Send(server->dataSocketd,server->replyBuff,
                                                     (UINT16)bytesReceived,  0);
                                else
                                    break;
#endif

                                /* Increment the total number of bytes transmitted */
                                totalBytes += bytesReceived;

#ifdef NET_5_1
                            } while (file_size > 0);
#else
                            } while (bytesReceived > 0);
#endif

                            /* Check if any bytes were sent */
                            if ( (totalBytes == 0) && (bytesReceived < 0) )
                            {
                                FTPS_Server_File_Error(server);
                                FTP_Printf("Unable to send file contents. Send failed).\r\n");
                                server->lastError = FTP_STACK_ERROR;
                                server->stackError = status;

                                /* Send 'Transfer aborted' message */
                                status = (INT)NU_Send((INT)server->socketd, MSG426, SIZE426, 0);

                                if (status < 0)
                                {
                                    FTP_Printf("FTPS_Server_Put - Send failed.\r\n");
                                    server->lastError = FTP_STACK_ERROR;
                                    server->stackError = status;
                                }
                            }

                            else/* File transfer is successful */
                            {
                                /* Send 'Closing data connection' message */
                                status = (INT)NU_Send((INT)server->socketd, MSG226, SIZE226, 0);

                                if (status < 0)
                                {
                                    FTP_Printf("Data_Recv_Task Send failed.\r\n");
                                    server->lastError = FTP_STACK_ERROR;
                                    server->stackError = status;
                                }
                            }
                        }

                        /* Close the file */
                        status = NU_Close(fd);

                        if (status != NU_SUCCESS)
                            FTPS_Server_File_Error(server);
                    }
                }
                else
                {
                    /* NU_Set_Curr_Dir() failed */
                    FTP_Printf("NU_Set_Curr_Dir() failed.\r\n");
                    server->lastError = FTP_CURRENT_DIR_FAILURE;

                    /* Send 'Requested action aborted' message */
                    if (NU_Send(server->socketd, MSG426, SIZE426,0) < 0)
                    {
                        FTP_Printf("FST_Data_Task NU_Send failure.\r\n");
                        server->lastError = FTP_STACK_ERROR;
                    }
                }
            }
            else /* Set default drive failed. */
            {
                FTP_Printf("FST_Data_Task cannot select the default drive.\r\n");
                server->lastError = status;

                /* Send 'Requested action aborted' message */
                if (NU_Send(server->socketd, MSG426, SIZE426, 0) < 0)
                {
                    FTP_Printf("FST_Data_Task NU_Send failure.\r\n");
                    server->lastError = FTP_STACK_ERROR;
                }
            }

           NU_Close_Disk (nu_drive);
        }
        else
        {
            FTP_Printf("FST_Data_Task cannot open default disk.\r\n");
            server->lastError = status;

            /* Send 'Requested action aborted' message */
            status = (INT)NU_Send(server->socketd, MSG426, SIZE426, 0);

            if (status < 0)
            {
                FTP_Printf("FST_Data_Task NU_Send failure.\r\n");
                server->lastError = FTP_STACK_ERROR;
            }
        }

        NU_Release_File_User();
    }
    else
    {
        FTP_Printf("FST_Data_Task cannot register as file system user.\r\n");
        server->lastError = FTP_REGISTRATION_FAILURE;

        /* Send 'Requested action aborted' message */
        if (NU_Send(server->socketd, MSG426, SIZE426, 0) < 0)
        {
            FTP_Printf("FST_Data_Task NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
        }
    }

    /* If restart point was set, unset it. */
    server->restart = 0;

    return (status);

} /* FTPS_Server_Put */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Server_ChDir
*
*   DESCRIPTION
*
*       This function changes the current working directory to the path
*       contained in command argument. The parameter is a pointer to an FTP
*       server structure.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the host.
*       FTP_CURRENT_DIR_FAILURE An error occurred while trying to determine
*                               the current directory on specified drive.
*       File system specific error codes.
*
******************************************************************************/
INT FTPS_Server_ChDir(FTP_SERVER *server)
{
    INT  status;
    INT  current_drive;
    CHAR nu_drive[3];

    /* test to see if the current working directory was changed */
    server->lastError = status = FTP_Is_Dir((CHAR *)server->fileSpec);

    if (status == NU_SUCCESS)
        server->lastError = status =
            NU_Set_Current_Dir ((CHAR *)server->fileSpec);

    if (status != NU_SUCCESS)
    {
        /* Send 'Requested file action not taken' message */
        status = (INT)NU_Send((INT)server->socketd, MSG550, SIZE550, 0);

        if (status < 0)
        {
            FTP_Printf("FTPS_Server_ChDir - Send failed.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
    }

    /* NU_Set_Current_Dir() successful */
    else
    {
        /* Get the new drive number from the received client message. */
        current_drive = FTPS_Parsedrive((UINT8 *)server->fileSpec);

        if (current_drive >= 0)
        {
            /* Save the current drive as the new default drive. */
            server->defaultDrive = current_drive;

            /* update the default drive */
            status = NU_Set_Default_Drive(server->defaultDrive);

            if (status != NU_SUCCESS)
            {
                FTP_Printf("FTPS_Server_ChDir - Set_Curr_Drive failed.\r\n");
                server->lastError = FTP_STACK_ERROR;
                server->stackError = status;
            }

        }

        /* get full path name of the current working directory */
        nu_drive[0] = (CHAR)('A' + server->defaultDrive);
        nu_drive[1] = ':';
        nu_drive[2] = '\0';

        status = NU_Current_Dir ((UINT8*)&nu_drive, (CHAR *)server->currentWorkingDir);

        if (status == NU_SUCCESS)
        {
            /* Send 'Requested file action completed' message */
            status = (INT)NU_Send((INT)server->socketd, MSG250, SIZE250, 0);

            if (status < 0)
            {
                FTP_Printf("FTPS_Server_ChDir - Send failed.\r\n");
                server->lastError = FTP_STACK_ERROR;
                server->stackError = status;
            }

            else
            {
                status = NU_SUCCESS;
            }
        }

        else
        {
            /* Send 'Syntax error' message */
            status = (INT)NU_Send((INT)server->socketd, MSG500, SIZE500, 0);

            if (status < 0)
            {
                FTP_Printf("FTPS_Server_ChDir - Send failed.\r\n");
                server->lastError = FTP_STACK_ERROR;
                server->stackError = status;
            }

            else
            {
                status = FTP_CURRENT_DIR_FAILURE;
            }
        }
    }

    return (status);

} /* FTPS_Server_ChDir */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Server_Dir
*
*   DESCRIPTION
*
*       This function sends a directory listing.  The listing will contain all
*       the files that match the specification defined by server->fileSpec.
*       The parameter is a pointer to an FTP server structure.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the host.
*       File system specific error codes.
*
******************************************************************************/
INT FTPS_Server_Dir(FTP_SERVER *server, UINT32 dirtype)
{
    INT     status = NU_SUCCESS, skipflag;
    DSTAT   statobj;
    CHAR    fileBuffer[FTPS_GENERIC_BUFF_SIZE];
    CHAR    sizeBuffer[64], temp[16], temp2[12];
    INT     attrib;
    UINT32  length;
    CHAR    path[FTPS_GENERIC_BUFF_SIZE];
    CHAR    displayPath[FTPS_GENERIC_BUFF_SIZE];
    CHAR    *fileBuffer_ptr, *temp_ptr;
    CHAR    *start;
    INT     current_drive = -1, new_drive;
    INT     i;
    UINT8   ftps_wildcard = NU_FALSE;
    CHAR    nu_drive[3];

    /* Determine what drive should the current operation be performed on */
    nu_drive[0] = (CHAR)(((server->defaultDrive) + 1) + 'A' - 1);
    nu_drive[1] = ':';
    nu_drive[2] = '\0';

    status = NU_Become_File_User();

    if (status == NU_SUCCESS)
    {
        /* Open disk for accessing */
        status = NU_Open_Disk (nu_drive);

        /* If the disk is already open, return success and leave the current
         * directory where it is.
         */
        if (status == NUF_NO_ACTION)
            status = NU_SUCCESS;

        if (status == NU_SUCCESS)
        {
            status = NU_Set_Default_Drive(server->defaultDrive);

            /* set default drive to C: for files */
            if (status == NU_SUCCESS)
            {
                /* setup the current working directory for this task */
                status = NU_Set_Current_Dir((CHAR *)server->currentWorkingDir);

                if (status == NU_SUCCESS)
                {
                    /* Initialize displayPath to prevent warnings. */
                    displayPath[0] = '\0';

                    temp_ptr = (CHAR *)temp;

                    /* If a drive and/or directory was specified, change to that
                     * drive and/or directory.
                     */
                    if (server->path[0])
                    {
                        /* Save fileSpec on NLST event */
                        if (dirtype == NLST_Event)
                        {
                            /* Save current fileSpec to path display buffer for NLST */
                            strcpy(displayPath, server->fileSpec);

                            /* Get length of displayPath */
                            for (i=1; displayPath[i]; i++) ;

                            /* If last char is '.' or '*', change it to null */
                            if ( (displayPath[i-1] == '.') || (displayPath[i-1] == '*') )
                                displayPath[i-1] = '\0';

                            /* If last char is not path delimiter, add path delimiter */
                            else if ( (displayPath[i-1] > '/') &&
                                      (displayPath[i-1] != FTPS_NATIVE_PATH_DELIMITER) )
                                strcat(displayPath, FTPS_NATIVE_PATH_DELIMITER_STR);

                        }

                        /* If fileSpec is not a directory */
                        if ( (server->fileSpec[0] > '/') &&
                             (server->fileSpec[0] != FTPS_NATIVE_PATH_DELIMITER) &&
                             (FTP_Is_Dir((CHAR *)server->fileSpec) != NU_SUCCESS) )
                        {
                            strcpy(server->path, ".");  /* Add '.' to end of fileSpec */
                        }

                        else
                        {

                            /* If input at root is 'DIR .\.' */
                            if (strcmp(server->currentWorkingDir,
                                FTPS_NATIVE_PATH_DELIMITER_STR) == 0)
                            {
                                /* If fileSpec is ".\.", change to "*" */
                                if (strcmp(server->fileSpec, ".\\.") == 0)
                                {
                                    strcpy(server->fileSpec, "*");

                                    /* Set path to '.' */
                                    strcpy(server->path, ".");
                                }

                            }

                            /* If fileSpec is '.', get current dir */
                            if (strcmp(server->fileSpec, ".") == 0)
                            {
                                strcpy(server->fileSpec, "*");
                                strcpy(server->path, ".");
                            }

                            for (i = 0; server->fileSpec[i]; i++)
                            {
                                /* If the wildcard character is present in the string, set
                                 * the flag indicating this.
                                 */
                                 if (server->fileSpec[i] == '*')
                                 {
                                     ftps_wildcard = NU_TRUE;
                                 }
                            }

                            /* Check end of fileSpec for '*' */
                            /* (i > 2) is checked to prevent illegal value for [i-2] */
                            if ( (server->fileSpec[i-1] != '*') && (i > 2) )
                            {

                                /* If it is "\.", change to "\*" */
                                if ( (server->fileSpec[i-1] == '.') &&
                                     (server->fileSpec[i-2] == FTPS_NATIVE_PATH_DELIMITER) )
                                    server->fileSpec[i-1] = '*';

                                /* If end of fileSpec is a alphanumeric */
                                if ( (ftps_wildcard == NU_FALSE) &&
                                     (server->fileSpec[i-1] > '/') &&
                                     (server->fileSpec[i-1] != FTPS_NATIVE_PATH_DELIMITER) )
                                {
                                    /* Add '\' to end of fileSpec */
                                    strcat(server->fileSpec,
                                        FTPS_NATIVE_PATH_DELIMITER_STR);
                                    i++;
                                }

                                /* If end of fileSpec is '\' */
                                if (server->fileSpec[i-1] == FTPS_NATIVE_PATH_DELIMITER)
                                {
                                    /* Add '*' to end of fileSpec */
                                    strcat(server->fileSpec, "*");
                                }
                            }
                        }

                        /* Save the current drive */
                        current_drive = NU_Get_Default_Drive();

                        /* Save the current directory */
                        nu_drive[0] = (CHAR)('A' + current_drive);
                        nu_drive[1] = ':';
                        nu_drive[2] = '\0';

                        NU_Current_Dir ((UINT8*)&nu_drive, path);


                        /* Check for a drive letter in the path */
                        new_drive = FTPS_Parsedrive((UINT8*)server->path);

                        /* If a drive letter exists, change drives */
                        if (new_drive >= 0)
                            status = server->lastError = NU_Set_Default_Drive(new_drive);
                    }

                    if (status == NU_SUCCESS)
                    {
                        /* Send 'File status okay' message */
                        status = (INT)NU_Send((INT)server->socketd, MSG150, SIZE150, 0);

                        if (status < 0)
                        {
                            FTP_Printf("FTPS_Server_Dir - Send failed.\r\n");
                            server->lastError = FTP_STACK_ERROR;
                            server->stackError = status;
                        }

                        /* If processing an NLST command, add path to fileBuffer. */
                        if (dirtype == NLST_Event)
                        {
                            if (server->path == server->filename)
                                fileBuffer[0] = '\0';
                            else
                            {
                                for (i = 0; displayPath[i]; i++) ;
                                while ( (i >= 1) && (displayPath[i-1] != FTPS_NATIVE_PATH_DELIMITER))
                                    i--;
                                displayPath[i+1] = '\0';
                                strcpy(fileBuffer, displayPath);
                            }
                        }
                        else
                            strcpy(fileBuffer, "-rw-rw-rw-   1 owner    group");

                        start = fileBuffer + strlen(fileBuffer);

                        /* Get first entry in the directory to match file pattern */
                        status = NU_Get_First(&statobj, (CHAR *)server->fileSpec);
                        if (status == NU_SUCCESS)
                        {
                            if (statobj.fattribute & AVOLUME)
                            {
                                status = NU_Get_Next(&statobj);
                            }
                        }

                        fs_user->p_errno = (INT) fsu_get_user_error();


                        /* Check if any entry exists for the valid file specification */
                        if (status == NU_SUCCESS)
                        {
                            do
                            {
                                UTL_Zero(temp2, sizeof(temp2));

                                /* Check for and skip listing . and .. in directory. */
                                strncpy(temp2, statobj.lfname, sizeof(temp2)-1);

                                if (strcmp(temp2, ".") == 0)
                                {
                                    status = NU_Get_Next(&statobj);
                                    continue;
                                }

                                if (strcmp(temp2, "..") == 0)
                                {
                                    status = NU_Get_Next(&statobj);
                                    continue;
                                }

                                /* Start the pointer at the end of the buffer. */
                                fileBuffer_ptr = start;

                                /* Reset skipflag. Used to skip directories in NLST mode. */
                                skipflag = 0;

                                if (dirtype == LIST_Event)
                                {
                                    /* Reset the directory attribute. */
                                    fileBuffer[0] = '-';

                                    /* Clear out any old file sizes. */
                                    strcpy(fileBuffer_ptr, "                ");
                                    fileBuffer_ptr += 16; /* Length of spaces above */
                                }

                                else
                                    *fileBuffer_ptr = '\0';

                                /* Get the file attributes. */
                                attrib = statobj.fattribute;

                                /* If this is a directory, set the directory attribute. */
                                if (attrib & ADIRENT)
                                {
                                    /* Skip if this is an NLST. */
                                    if (dirtype == NLST_Event)
                                        skipflag = 1;

                                    /* Must be a directory. */
                                    else
                                        fileBuffer[0] = 'd';
                                }

                                if (dirtype == LIST_Event)
                                {
                                    /* If this is a LIST entry, check the file attributes
                                       and set the output characters accordingly. Read and
                                       write are the only valid attributes checked. */
                                    if (attrib & ARDONLY)
                                    {
                                        fileBuffer[2] = '-';
                                        fileBuffer[5] = '-';
                                        fileBuffer[8] = '-';
                                    }
                                    else
                                    {
                                        fileBuffer[2] = 'w';
                                        fileBuffer[5] = 'w';
                                        fileBuffer[8] = 'w';
                                    }
                                }

                                /* If doing a LIST command, add size and date info. */
                                if (dirtype == LIST_Event)
                                {
                                    /* Add file size. */
                                    length = statobj.fsize;

                                    NU_ULTOA(length, temp_ptr, 10);
                                    for (i=0; i<(INT)strlen(temp); i++)
                                        fileBuffer_ptr--;
                                    strcpy(fileBuffer_ptr, temp_ptr);

                                    /* Add date and time */
                                    strcat(fileBuffer_ptr, " ");
                                    FTPS_File_Time(&statobj, sizeBuffer, sizeof(sizeBuffer));
                                    strcat(fileBuffer_ptr, sizeBuffer);

                                    /* Space for the filename. */
                                    strcat(fileBuffer_ptr, " ");
                                    fileBuffer_ptr = fileBuffer + strlen(fileBuffer);
                                }

                                /* Send the listing if not skipped. */
                                if (skipflag == 0)
                                {
                                    /* Append the filename into the buffer. */
                                    strcpy(fileBuffer_ptr, statobj.lfname);

                                    /* Terminate string. */
                                    strcat(fileBuffer_ptr, "\r\n");

                                    /* Translate any backslashes to forward slashes for
                                       standard Unix compatibility. */
                                    fileBuffer_ptr = fileBuffer;

                                    while (*fileBuffer_ptr != '\0')
                                    {
                                        if (FTPS_IS_PATH_DELIMITER(*fileBuffer_ptr))
                                            *fileBuffer_ptr = FTPS_TX_PATH_DELIMITER;

                                        ++fileBuffer_ptr;
                                    }

#ifdef NET_5_1
                                    /* Disable Zero Copy mode on the socket */
                                    NU_Fcntl(server->dataSocketd,
                                             NU_SET_ZC_MODE, NU_ZC_DISABLE);
#endif

                                    status = (INT)NU_Send(server->dataSocketd, fileBuffer,
                                                          (UINT16)(strlen(fileBuffer)), 0);

#ifdef NET_5_1
                                    /* Re-enable Zero Copy mode on the socket */
                                    NU_Fcntl(server->dataSocketd,
                                             NU_SET_ZC_MODE, NU_ZC_ENABLE);
#endif

                                    if (status < 0)
                                    {
                                        FTP_Printf("FTPS_Server_Dir - Send failed.\r\n");
                                        server->lastError = FTP_STACK_ERROR;
                                        server->stackError = status;
                                    }
                                }

                                /* Get next file name. */
                                status = NU_Get_Next(&statobj);

                            } while (status == NU_SUCCESS);

                            /* free internal elements when done searching */
                            NU_Done(&statobj);
                        }

                        /* Restore the current drive to the default drive if it was
                         * changed.
                         */
                        if (server->path[0])
                        {
                            server->lastError = NU_Set_Default_Drive(current_drive);
                        }

                        /* Send 'Requested file action successful' message */
                        status = (INT)NU_Send((INT)server->socketd, MSG226, SIZE226, 0);

                        if (status < 0)
                        {
                            FTP_Printf("FTPS_Server_Dir - Send failed.\r\n");
                            server->lastError = FTP_STACK_ERROR;
                            server->stackError = status;
                        }

                        else
                            status = NU_SUCCESS;
                    }

                    else
                    {
                        /* Send 'Requested file action not taken' message */
                        status = (INT)NU_Send((INT)server->socketd, MSG450, SIZE450, 0);

                        if (status < 0)
                        {
                            FTP_Printf("FTPS_Server_Dir - Send failed.\r\n");
                            server->lastError = FTP_STACK_ERROR;
                            server->stackError = status;
                        }
                    }
                }
                else
                {
                    /* NU_Set_Curr_Dir() failed */
                    FTP_Printf("NU_Set_Curr_Dir() failed.\r\n");
                    server->lastError = FTP_CURRENT_DIR_FAILURE;

                    /* Send 'Requested action aborted' message */
                    if (NU_Send(server->socketd, MSG426, SIZE426,0) < 0)
                    {
                        FTP_Printf("FST_Data_Task NU_Send failure.\r\n");
                        server->lastError = FTP_STACK_ERROR;
                    }
                }
            }
            else /* Set default drive failed. */
            {
                FTP_Printf("FST_Data_Task cannot select the default drive.\r\n");
                server->lastError = status;

                /* Send 'Requested action aborted' message */
                if (NU_Send(server->socketd, MSG426, SIZE426, 0) < 0)
                {
                    FTP_Printf("FST_Data_Task NU_Send failure.\r\n");
                    server->lastError = FTP_STACK_ERROR;
                }
            }

           /*  Close Disk */
           nu_drive[0] = (CHAR)((server->defaultDrive + 1) + 'A' - 1);
           nu_drive[1] = ':';
           nu_drive[2] = '\0';

           NU_Close_Disk (nu_drive);
        }
        else
        {
            FTP_Printf("FST_Data_Task cannot open default disk.\r\n");
            server->lastError = status;

            /* Send 'Requested action aborted' message */
            status = (INT)NU_Send(server->socketd, MSG426, SIZE426, 0);

            if (status < 0)
            {
                FTP_Printf("FST_Data_Task NU_Send failure.\r\n");
                server->lastError = FTP_STACK_ERROR;
            }
        }

        NU_Release_File_User();
    }
    else
    {
        FTP_Printf("FST_Data_Task cannot register as file system user.\r\n");
        server->lastError = FTP_REGISTRATION_FAILURE;

        /* Send 'Requested action aborted' message */
        if (NU_Send(server->socketd, MSG426, SIZE426, 0) < 0)
        {
            FTP_Printf("FST_Data_Task NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
        }
    }

    return (status);

} /* FTPS_Server_Dir */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Server_MkDir
*
*   DESCRIPTION
*
*       This function creates a subdirectory in the path specified by
*       server->path. Fails if a file or directory of the same name already
*       exists or if the path is not found.  The parameter is a pointer to
*       an FTP server structure.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_STACK_ERROR         File system returned an i/o error or
*                               a send operation failed.
*
******************************************************************************/
INT FTPS_Server_MkDir(FTP_SERVER *server)
{
    INT     status;
    CHAR    pathBuffer[FTPS_GENERIC_BUFF_SIZE];

    /* Make the directory specified */
    status = NU_Make_Dir((CHAR *)server->fileSpec);

    if (status != NU_SUCCESS)
    {
        FTPS_Server_File_Error(server);

        /* Send 'Requested file action not taken' message */
        status = (INT)NU_Send((INT)server->socketd, MSG550, SIZE550, 0);

        if (status < 0)
        {
            FTP_Printf("FTPS_Server_MkDir - Send failed.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
    }

    else /* NU_Make_Dir() successful */
    {
        /* build FTP 257 reply message */
        strcpy(pathBuffer, "257 \"");
        FTPS_Format_Pathname(&pathBuffer[5], server->fileSpec);
        strcat(pathBuffer, MSG257a);

        status = (INT)NU_Send((INT)server->socketd, pathBuffer,
                         (UINT16)strlen(pathBuffer), 0);

        if (status < 0)
        {
            FTP_Printf("FTPS_Server_MkDir - Send failed.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
        else
        {
            status = NU_SUCCESS;
        }
    }

    return (status);

} /* FTPS_Server_MkDir */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Server_RmDir
*
*   DESCRIPTION
*
*       This function deletes a directory as specified by server->path.
*       Fails if name is not a directory, is read only or contains more than
*       the entries. The parameter is a pointer to an FTP server structure.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_STACK_ERROR         File system returned an i/o error or
*                               a send operation failed.
*
******************************************************************************/
INT FTPS_Server_RmDir(FTP_SERVER *server)
{
    INT status;

    /* Test if path is a valid directory */
    status = FTP_Is_Dir((CHAR *)server->fileSpec);

    /* Check if NU_Is_Dir failure is due to a NULL pathname */
    if (status != NU_SUCCESS)
    {
        /* Send 'Requested file action not taken' message */
        status = (INT)NU_Send((INT)server->socketd, MSG550, SIZE550, 0);

        if (status < 0)
        {
            FTP_Printf("FTPS_Server_RmDir - Send failed.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
    }

    /* NU_Is_Dir() successful */
    else
    {
        /* delete the directory specified */
        status = NU_Remove_Dir((CHAR *)server->fileSpec);

        fs_user->p_errno = (INT) fsu_get_user_error();

        if (status != NU_SUCCESS)
        {
            FTPS_Server_File_Error(server);

            /* Send 'Requested file action not taken' message */
            status = (INT)NU_Send((INT)server->socketd, MSG550, SIZE550, 0);

            if (status < 0)
            {
                FTP_Printf("FTPS_Server_RmDir - Send failed.\r\n");
                server->stackError = status;
            }
        }

        else /* NU_Remove_Dir() successful */
        {
            /* Send 'Requested file action okay' message */
            status = (INT)NU_Send((INT)server->socketd, MSG250, SIZE250, 0);

            if (status < 0)
            {
                FTP_Printf("FTPS_Server_RmDir - Send failed.\r\n");
                server->lastError = FTP_STACK_ERROR;
                server->stackError = status;
            }

            else
            {
                status = NU_SUCCESS;
            }
        }
    }

    return (status);

} /* FTPS_Server_RmDir */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Server_PrintDir
*
*   DESCRIPTION
*
*       This function sends the current working directory to the client.
*       The parameter is a pointer to an FTP server structure.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the host.
*
******************************************************************************/
INT FTPS_Server_PrintDir(FTP_SERVER *server)
{
    INT     status;
    CHAR    pathBuffer[FTPS_GENERIC_BUFF_SIZE];
    CHAR    nu_drive[3];

    /* get full path name of the current working directory */
    nu_drive[0] = (CHAR)('A' + server->defaultDrive);
    nu_drive[1] = ':';
    nu_drive[2] = '\0';

    status = NU_Current_Dir ((UINT8*)&nu_drive, (CHAR *)server->path);

    if (status != NU_SUCCESS)
    {
        /* Send 'Requested file action not taken' message */
        status = (INT)NU_Send((INT)server->socketd, MSG550, SIZE550, 0);

        if (status < 0)
        {
            FTP_Printf("FTPS_Server_PrintDir - Send failed.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
    }

    else/* NU_Current_Dir() successful */
    {
        /* build FTP 257 reply message */
        strcpy(pathBuffer, "257 \"");
        FTPS_Format_Pathname(&pathBuffer[5], server->path);
        strcat(pathBuffer, MSG257);

        status = (INT)NU_Send((INT)server->socketd, pathBuffer,
                         (UINT16)strlen(pathBuffer), 0);

        if (status < 0)
        {
            FTP_Printf("FTPS_Server_PrintDir - Send failed.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
        else
        {
            status = NU_SUCCESS;
        }
    }

    return (status);

} /* FTPS_Server_PrintDir */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Server_Rename_File
*
*   DESCRIPTION
*
*       This function renames the file in server->renamePath[] to the name in
*       server->fileSpec.  Fails if name is invalid, newname already exists or
*       path not found.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the host.
*       FTP_CURRENT_DIR_FAILURE An error occurred while trying to determine
*                               the current directory on specified drive.
*       File system specific error codes.
*
******************************************************************************/
INT FTPS_Server_Rename_File(FTP_SERVER *server)
{
    INT     status;
    CHAR    temp[FTPS_GENERIC_BUFF_SIZE];
    CHAR    current_dir[256];
    INT     drive;
    INT     flag = 0;
    CHAR    nu_drive[3];

    if (server->cmdFTP.rnfrFlag == FTPS_RN_MOVE)
    {
        /* Concatenate server->path and server->filename by replacing \0 with
           a backslash. */
        if ( (server->filename) != 0 &&
             (strcmp(server->path, server->filename) != 0) )
        {
            /* If server->path = path\0filename, server->path = path\filename*/
            if (server->path[strlen(server->path) + 1] != '\0')
                server->path[strlen(server->path)] = FTPS_NATIVE_PATH_DELIMITER;
        }

        /* The Move function will use the fileSpec buffer as
           the destination path, so renamePath is safe to
           delete if there is no path. */
        if (strcmp(server->renamePath, server->renameFile) == 0)
            *server->renamePath = '\0';

        status = FTPS_Move_File(server->path, server->renamePath,
                                server->fileSpec);
    }

    else
    {
        strcpy(temp, FTPS_NATIVE_PATH_DELIMITER_STR);
        strcat(temp, server->path);
        strcat(temp, FTPS_NATIVE_PATH_DELIMITER_STR);

        if ( (strcmp(server->path,server->renamePath) == 0) &&
             (strcmp(temp,server->currentWorkingDir) != 0) )
        {
            /* change current working directory */
            drive = NU_Get_Default_Drive();

            nu_drive[0] = (CHAR)('A' + drive);
            nu_drive[1] = ':';
            nu_drive[2] = '\0';

            NU_Current_Dir ((UINT8*)&nu_drive, current_dir);

            NU_Set_Current_Dir ((CHAR *)server->path);

            flag = 1; /* Current Directory changed flag */
        }

        /* Concatenate server->renamePath and server->renameFile by replacing
           \0 with a backslash. */
        if ( ((server->filename) != 0) &&
             (strcmp(server->path, server->filename) != 0) )
        {
            server->path[strlen(server->path)] = FTPS_NATIVE_PATH_DELIMITER;
        }

        FTPS_Get_Rename((CHAR *)(server->path));

        /* if this is a directory rename */
        if (server->cmdFTP.rnfrFlag == FTPS_RN_DIR)
        {
            FTPS_Get_Rename((CHAR *)(server->renamePath));

            status = NU_Rename((CHAR *)server->path, (CHAR *)server->renamePath);
        }

        else
        {
            FTPS_Get_Rename((CHAR *)(server->renameFile));

            status = NU_Rename((CHAR *)server->path, (CHAR *)server->renameFile);
        }

        /* put back the current directory */
        if (flag)
            NU_Set_Current_Dir((CHAR *)current_dir);
    }

    if (status != NU_SUCCESS)
    {
        /* Send 'File name not allowed' message */
        status = (INT)NU_Send((INT)server->socketd, MSG550, SIZE550, 0);

        if (status < 0)
        {
            FTP_Printf("FTPS_Server_Rename_File - Send failed.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
    }

    else
    {
        /* Send 'Requested file action okay' message */
        status = (INT)NU_Send((INT)server->socketd, MSG250, SIZE250, 0);

        if (status < 0)
        {
            FTP_Printf("FTPS_Server_Rename_File - Send failed.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
        else
            status = NU_SUCCESS;
    }

    return (status);

} /* FTPS_Server_Rename_File */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Server_Delete_File
*
*   DESCRIPTION
*
*       This function deletes the file specified by server->fileSpec.  Fail
*       if not a simple file, if it is open, does not exist or is read only.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the host.
*
******************************************************************************/
INT FTPS_Server_Delete_File(FTP_SERVER *server)
{
    INT status;
    INT loopCount;

    status = NU_SUCCESS;
    loopCount = 0;

    while (status == NU_SUCCESS)
    {
        status = NU_Delete((CHAR *)server->fileSpec);
        loopCount++;
    }

    if (loopCount < 2)
    {
        FTPS_Server_File_Error(server);

        /* Send 'Requested file action not taken' message */
        status = (INT)NU_Send((INT)server->socketd, MSG550, SIZE550, 0);

        if (status < 0)
        {
            FTP_Printf("FTPS_Server_Rm_File - Send failed.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
    }

    else
    {
        /* Send 'Requested file action okay' message */
        status = (INT)NU_Send((INT)server->socketd, MSG250, SIZE250, 0);

        if (status < 0)
        {
            FTP_Printf("FTPS_Server_Delete_File - Send failed.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
        else
            status = NU_SUCCESS;
    }

    return (status);

} /* FTPS_Server_Delete_File */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Server_Find_File_Size
*
*   DESCRIPTION
*
*       This function....
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the host.
*
******************************************************************************/
INT FTPS_Server_Find_File_Size(FTP_SERVER *server)
{
    INT        fd;
    INT32      file_size=0;
    CHAR       sizeBuffer[FTPS_GENERIC_BUFF_SIZE];
    CHAR       temp[10];
    STATUS     status = NU_SUCCESS;

    /* Open the file */
    fd = NU_Open((CHAR *)server->fileSpec, PO_RDONLY, PS_IREAD);

    if (fd >= 0)
    {
        file_size = FTP_Handle_File_Length(fd);

        /* We are done getting the required information from the file,
         * the file may now be closed
         */
        NU_Close(fd);

        if (file_size >= 0)
        {
            /* Build FTP 213 reply message */
            strcpy(sizeBuffer, "213 ");
            NU_ITOA(file_size, temp, 10);
            strcat(sizeBuffer, temp);
            strcat(sizeBuffer, "\r\n");

            status = (INT)NU_Send((INT)server->socketd, sizeBuffer,
                             (UINT16)strlen(sizeBuffer), 0);

            if (status < 0)
            {
                FTP_Printf("FTPS_Server_Find_File_Size - Send failed.\r\n");
                server->lastError = FTP_STACK_ERROR;
                server->stackError = status;
            }
            else
            {
                status = NU_SUCCESS;
            }
        }
        else
        {
            /* Send 'Requested file action not taken' message */
            status = (INT)NU_Send((INT)(server->socketd), MSG550, SIZE550, 0);

            if (status < 0)
            {
                FTP_Printf("FTPS_Server_Find_File_Size - Send failed.\r\n");
                server->lastError = file_size;
                server->stackError = status;
            }

            /* Set the status back to the actual error */
            status = file_size;
        }
    }

    else
    {
        /* Send 'Requested file action not taken' message */
        status = (INT)NU_Send((INT)(server->socketd), MSG550, SIZE550, 0);

        if (status < 0)
        {
            FTP_Printf("FTPS_Server_Find_File_Size - Send failed.\r\n");
            server->lastError = fd;
            server->stackError = status;
        }

        /* Set the status back to the actual error */
        status = fd;
    }

    return (status);
} /* FTPS_Server_Find_File_Size */

/******************************************************************************
*
*   FUNCTION
*
*       Str_Equal
*
*   DESCRIPTION
*
*       This function determines if two strings contain the same characters.
*       The function returns the value 1 if the two strings are equal and a
*       0 if the strings are not.
*
*   INPUTS
*
*       str1                    pointer to first string
*       str2                    pointer to second string
*
*   OUTPUTS
*
*       1 if the two strings are equal, 0 if the two strings are not equal.
*
******************************************************************************/
INT Str_Equal(UNSIGNED_CHAR str1[FTPS_GENERIC_BUFF_SIZE],
              CHAR str2[FTPS_GENERIC_BUFF_SIZE])
{
    while ( (*str1 == *str2) && (*str1) )
    {
        str1++;
        str2++;
    }

    return ((*str1 == '\0') && (*str2 == '\0'));

} /* Str_Equal */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Server_File_Error
*
*   DESCRIPTION
*
*       This function loads the server structure with the appropriate error
*       code corresponding to the received Nucleus File Error.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       None
*
******************************************************************************/
VOID FTPS_Server_File_Error(FTP_SERVER *server)
{
    switch (fs_user->p_errno = (INT) fsu_get_user_error())
    {
        case PEBADF:

            FTP_Printf("File descriptor invalid or open read only.\r\n");
            server->lastError = FTP_BAD_FILE_DESCRIPTOR;
            break;

        case PENOSPC:

            FTP_Printf("Write failed. Presumably because of no space.\r\n");
            server->lastError = FTP_WRITE_FAILED;
            break;

        case PENOENT:

            FTP_Printf("File not found or path to file not found.\r\n");
            server->lastError = FTP_FILE_NOT_FOUND;
            break;

        case PEMFILE:

            FTP_Printf("No file descriptors available (too many files open).\r\n");
            server->lastError = FTP_NO_FILE_DESCRIPTOR_AVAIL;
            break;

        case PEEXIST:

            FTP_Printf("Exclusive access requested but file already exists.\r\n");
            server->lastError = FTP_FILE_ALREADY_EXISTS;
            break;

        case PEACCES:

            FTP_Printf("Attempt to open a read only file or a special (directory).\r\n");
            server->lastError = FTP_SPECIAL_ACCESS_ATTEMPTED;
            break;

        case PEINVAL:

            FTP_Printf("Seek to negative file pointer attempted.\r\n");
            server->lastError = FTP_INVALID_FILE_POINTER;
            break;

        default:

            FTP_Printf("Unknown Nucleus File Error number.\r\n");
            server->lastError = FTP_UNKNOWN_FILE_ERROR;
            break;
    }

} /* FTPS_Server_File_Error */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Authenticate
*
*   DESCRIPTION
*
*       Checks the password list for matching user id and password.
*
*   INPUTS
*
*       *id                     User id.
*       *pw                     Password to check.
*
*   OUTPUTS
*
*       NU_TRUE                 User id and password are valid.
*       NU_FALSE                User id and/or password invalid.
*
******************************************************************************/
STATUS FTPS_Authenticate(CHAR *id, CHAR *pw)
{
    STATUS  found_it;

#if (FTP_USE_ANONYMOUS == NU_TRUE)
    CHAR    *ptr;
#endif

#ifndef NET_5_1
    STATUS  wrong_pw;
    UINT16  index;
#endif

#ifdef NET_5_1

    /* Check if the user is registered with the User Management module */
    if (UM_Validate_User(id, pw, UM_FTP) == NU_SUCCESS)
        found_it = NU_TRUE;
    else
        found_it = NU_FALSE;

#else

    /* Assume an invalid login */
    found_it = NU_FALSE;
    wrong_pw = NU_FALSE;

    /* All we need to do is loop through the password list and compare
       the ID and PW. */
    for (index = 0;
         (FTP_Password_List[index].id[0] != '\0') &&
         (found_it == NU_FALSE) && (wrong_pw == NU_FALSE); index++)
    {
        /* If the IDs match, check the PW */
        if ((strcmp(id, FTP_Password_List[index].id)) == 0)
        {
            /* If they both match then we found it. */
            if ((strcmp(pw, FTP_Password_List[index].pw)) == 0)
                found_it = NU_TRUE;

            /* The password must be wrong. */
            else
                wrong_pw = NU_TRUE;
        }
    }

#endif

#if (FTP_USE_ANONYMOUS == NU_TRUE)

    if (found_it == NU_FALSE)
    {
        /* Check for an anonymous password if it isn't in the database.
           Other special checks should be placed here as well. */
        /* Verify user id using case-insensitive check. */
        if (NU_STRICMP(id, "anonymous") == 0)
        {
            /* Start checking each character in the password. */
            ptr = pw;

            /* First character must not be '@'. */
            if (*ptr != '@')
            {
                while (*ptr)
                {
                    /* Looking for a single instance of '@'. */
                    if (*ptr == '@')
                    {
                        if (found_it == NU_TRUE)
                        {
                            /* More than one '@' was found. */
                            found_it = NU_FALSE;
                            break;
                        }
                        else
                            found_it = NU_TRUE;
                    }
                    else
                    {
                        /* All other characters should be in this range. */
                        if ((*ptr <= 32) || (*ptr >= 127))
                        {
                            /* Invalid character. */
                            found_it = NU_FALSE;
                            break;
                        }
                    }

                    ptr++;
                }

                /* Last character must not be '@'. */
                if ( (found_it == NU_TRUE) && (*(ptr - 1) == '@') )
                    found_it = NU_FALSE;
            }
        }
    }
#endif

    return (found_it);

} /* FTPS_Authenticate */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_File_Time
*
*   DESCRIPTION
*
*       Returns a string with the time and date the file is modified. The
*       form is based on the Unix standard FTP directory listing using LIST.
*
*   INPUTS
*
*       *file                   File information structure.
*       *string                 Buffer for the return string.
*
*   OUTPUTS
*
*       CHAR *string     String of the form 'Jan 01 12:00'.
*
******************************************************************************/
/* Define the bit positions within File's DATESTR */
#define YEARMASK    0xFE00
#define MONTHMASK   0x01E0
#define DAYMASK     0x001F
#define HOURMASK    0xF800
#define MINMASK     0x07E0
#define SECMASK     0x001F
CHAR *FTPS_File_Time(DSTAT *file, CHAR *string, UINT16 size)
{
    CHAR        temp[32];
    INT         tm_hour;
    INT         tm_min;
    INT         tm_sec;
    INT         tm_mon;
    INT         tm_mday;
    INT         tm_year;
    UINT16      remaining;

    /* Initialize string to a zero length string. */
    string[0] = 0;
    
    /* Leave room for the required null terminator. */
    size -= 1;

    /* Calculate the number of bytes available in the buffer. */
    remaining = size-strlen(string);
            
    /* Get date and time from file system. */
    /* Extract time information. */
    tm_year = ((file->fupdate & YEARMASK ) >> 9) + 1980;
    tm_mon  = ((file->fupdate & MONTHMASK) >> 5);
    tm_mday = ( file->fupdate & DAYMASK  );
    tm_hour = ((file->fuptime & HOURMASK ) >> 11);
    tm_min  = ((file->fuptime & MINMASK  ) >> 5);
    tm_sec  = ((file->fuptime & SECMASK  ) << 1);

    /* default value should at least be a March 28,1980 */
    if (tm_mon == 0)
        tm_mon = 3;

    /* Append the month string. eg 3-March */
    strncat(string, FTP_Month[tm_mon-1], remaining);
    remaining = size-strlen(string);

    /* default day should March 28,1980, since 0 day is not correct*/
    if (tm_mday == 0)
        tm_mday = 28;

    /* Append the day. */
    if (tm_mday < 10)
    {
        strncat(string, " ", remaining);
        remaining = size-strlen(string);
    }

    strncat(string, NU_ITOA(tm_mday, temp, 10), remaining);
    remaining = size-strlen(string);
    strncat(string, " ", remaining);
    remaining = size-strlen(string);

    /* if Current year is not year the file was created, then
       send the year else send the time */
    if (FTP_GET_CURRENT_YEAR > tm_year)
    {
        strncat(string, " ", remaining);
        remaining = size-strlen(string);
        strncat(string, NU_ITOA(tm_year, temp, 10), remaining);
        remaining = size-strlen(string);
    }

    else
    {
        /* Append the hour of day. */
        if (tm_hour < 10)
            strncat(string, " ", remaining);

        strncat(string, NU_ITOA(tm_hour, temp, 10), remaining);
        remaining = size-strlen(string);
        strncat(string, ":", remaining);
        remaining = size-strlen(string);

        /* Append the minutes. */
        if (tm_min < 10)
        {
            strncat(string, "0", remaining);
            remaining = size-strlen(string);
        }

        strncat(string, NU_ITOA(tm_min, temp, 10), remaining);
    }
    
    /* In order to remove warning generated by some compilers. */
    (VOID) tm_sec;

    return (string);

} /* FTPS_File_Time */

/******************************************************************************
*
*   FUNCTION
*
*      FTPS_Move_File
*
*   DESCRIPTION
*
*      Used by the RNTO function to rename files to differing
*      subdirectories. This function performs a physical move on a file,
*      using the open, read, write, close, and delete file routines.
*
*   INPUTS
*
*      *dest                Destination pathname to move the file to.
*      *source              Source path where file resides.
*
*   OUTPUTS
*
*      NU_SUCCESS           Operation completed successfully.
*      NU_NO_MEMORY         There is no memory available to allocate.
*      NU_TIMEOUT           The operation timed out waiting on a service.
*      NU_POOL_DELETED      The memory pool was deleted during suspension.
*      FTPS_FILE_ERROR      The move operation failed trying to moving file.
*
******************************************************************************/
INT FTPS_Move_File(CHAR *source, CHAR *destpath, CHAR *dest)
{
    INT             status;
    INT             srcfile;
    INT             dstfile = 0;
    INT32           fsize;
    UINT32          bytesread;
    UINT16          readsize;
    CHAR            *fbuf;
    INT             dstDrive;
    CHAR            dstPath[FTPS_GENERIC_BUFF_SIZE];
    CHAR            nu_drive[3];

    /* Allocate memory for the send buffer */
    status = NU_Allocate_Memory(FTPS_Memory, (VOID *)&fbuf,
                                FTPS_TRANSFER_BLOCK_SIZE, NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Reset status to 'fail' until all operations complete normally. */
        status = FTPS_FILE_ERROR;

        /* Create a backup of destination path */
        dstDrive = FTPS_Parsedrive((UINT8*)dest);
        if ( (dstDrive >= 0) && (dstDrive <= 26) )
        {
            nu_drive[0] = (CHAR)('A' + dstDrive);
            nu_drive[1] = ':';
            nu_drive[2] = '\0';

            NU_Current_Dir ((UINT8*)&nu_drive, dstPath);
        }

        /* Make sure the destination directory exists. */
        if ( (!*destpath) || (FTP_Is_Dir(destpath) == NU_SUCCESS)
            || destpath[1] == ':')
        {
            /* Open for transferring the old file to the new file. */
            srcfile = NU_Open(source, PO_RDONLY|PO_BINARY, PS_IREAD);

            if (srcfile >= 0)
            {
                dstfile = NU_Open(dest, PO_RDWR|PO_CREAT|PO_EXCL|PO_BINARY, PS_IWRITE);

                if (dstfile >= 0)
                    status = NU_SUCCESS;
                else
                    NU_Close(srcfile);
            }

            /* If successful till here, the files are open, start transfer. */
            if (status == NU_SUCCESS)
            {
                /* Get size of the source file. */
                fsize = FTP_Handle_File_Length(srcfile);

                /* Copy contents of source to dest, FTPS_TRANSFER_BLOCK_SIZE
                   bytes at a time to minimize memory usage. */
                readsize = FTPS_TRANSFER_BLOCK_SIZE;
                NU_Seek(srcfile, 0, PSEEK_SET);

                while (fsize > 0)
                {
                    if (fsize < FTPS_TRANSFER_BLOCK_SIZE)
                        readsize = (UINT16)fsize;

                    bytesread = NU_Read(srcfile, fbuf, readsize);

                    if (bytesread == readsize)
                    {
                        bytesread = NU_Write(dstfile, fbuf, readsize);

                        if (bytesread <= 0)
                        {
                            status = FTPS_FILE_ERROR;
                            break;
                        }

                        fsize -= (INT32)bytesread;
                    }

                    else
                    {
                        status = FTPS_FILE_ERROR;
                        break;
                    }
                }

                /* Close the files. */
                NU_Close(srcfile);

                NU_Close(dstfile);

                /* Delete the source file if all was successful,
                   otherwise delete the dest file and return an error. */
                if (status == NU_SUCCESS)
                    NU_Delete(source);
                else
                    NU_Delete(dest);
            }

        }

        /* Restore destination path */
        if ( (dstDrive >= 0) && (dstDrive <= 26) )
        {
            if (NU_Set_Default_Drive(dstDrive) == NU_SUCCESS)
                NU_Set_Current_Dir (dstPath);
        }

        NU_Deallocate_Memory(fbuf);
    }

    return (status);

} /* FTPS_Move_File */

/******************************************************************************
*
*   FUNCTION
*
*       FTPS_Get_Rename
*
*   DESCRIPTION
*
*       This function is used by FTPS_Server_Rename_File to reduce the name
*       of the input string.
*       eg: input /Dir1/Dir2/Dir3/
*           output Dir3
*
*   INPUTS
*
*       *string                 the whole path to the file or folder
*
*   OUTPUTS
*
*       None
*
******************************************************************************/
VOID FTPS_Get_Rename(CHAR* string)
{
    INT     i, j; /* variables */
    CHAR    temp[FTPS_GENERIC_BUFF_SIZE];

    i = j = (INT)strlen(string);

    if (FTPS_IS_PATH_DELIMITER(string[i-1]))
        string[i-1] = '\0';

    while ( (!FTPS_IS_PATH_DELIMITER(string[i])) && i )
    {
        i--;
    }

    if ( (j > i) && i)
    {
        strcpy(temp, (char *)(string + i + 1));
        strcpy(string, temp);
    }

} /* FTPS_Get_Rename */

/************************************************************************
*
*   FUNCTION
*
*       FTPS_Parsedrive
*
*   DESCRIPTION
*
*       This routine parses the drive from a path specifier, if one has
*       has specified.  If the second character in path is ':' then the
*       first char is assumed to be a drive specifier and 'A' is
*       subtracted from it to give the drive number. The drive number is
*       returned. If the second character in path is not ':' then a
*       negative value is returned.
*
*   INPUTS
*
*       path                    A pointer to the path.
*
*   OUTPUTS
*
*       Returns the drive number if one has been specified or NU_IO_ERROR.
*
*************************************************************************/
STATIC INT16 FTPS_Parsedrive(UINT8 *path)
{
    UINT8   *p = path;
    INT16   dno = NU_IO_ERROR;

    /* get drive no */
    if ( (p) && (*p) && (*(p+1) == ':') )
    {
        if ( ((*p) >= 'A') && ((*p) <= 'Z') )
            dno = (INT16) (*p - 'A');
        if ( ((*p) >= 'a') && ((*p) <= 'z') )
            dno = (INT16) (*p - 'a');
    }

    return (dno);

} /* FTPS_Parsedrive */

/************************************************************************
*
*   FUNCTION
*
*       FTPS_Format_Pathname
*
*   DESCRIPTION
*
*       Translate native pathname to a pathname that uses the path
*       delimiter as defined by FTPS_NATIVE_PATH_DELIMITER. This should be
*       used before sending path information to the network.
*
*   INPUTS
*
*       buffer                  Preallocated buffer for resulting string.
*       pathname                String pointer to the native path.
*
*   OUTPUTS
*
*       String pointer to the translated path.
*
*************************************************************************/
STATIC CHAR* FTPS_Format_Pathname(CHAR *buffer, CHAR *pathname)
{
    UINT16 i, length;

    length = (UINT16)strlen(pathname);

    /* Scan pathname for '\\' or '/' delimiter, copy
     * FTPS_TX_PATH_DELIMITER ('/') in its place.
     */
    for (i = 0; i < length; i++)
    {
        if (FTPS_IS_PATH_DELIMITER(pathname[i]))
            buffer[i] = FTPS_TX_PATH_DELIMITER;
        else
            buffer[i] = pathname[i];
    }

    buffer[i] = '\0';

    return (buffer);

} /* FTPS_Format_Pathname */

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     FTP_Is_Dir                                        
*                                                                      
* DESCRIPTION                                                          
*
*     Verify if the given directory exists.
*                                                                      
*  INPUTS
*
*      path                        Directory path to verify.
*
*
* OUTPUTS                                                              
*
*      This function will return the status returned by the Nucleus
*      File function calls.
*
************************************************************************/

INT FTP_Is_Dir(CHAR *path)
{
    CHAR    nu_drive[3];
    CHAR    current_dir[256];
    INT     drive;
    STATUS  status;

    /* Initialize the current directory to be A: */
    current_dir[0] = 'A';

    /* Retrieve the default drive */
    drive = NU_Get_Default_Drive();

    /* Convert the drive number to the corresponding drive letter */
    nu_drive[0] = (CHAR)('A' + drive);
    nu_drive[1] = ':';
    nu_drive[2] = '\0';

    /* Determine the current directory from the input path */
    status = NU_Current_Dir ((UINT8*)nu_drive, current_dir);

    if (status == NU_SUCCESS)
    {
        /* Try to set the input path as the current directory
         * to check and see if the path is valid.
         */
        status = NU_Set_Current_Dir (path);

        if (status == NU_SUCCESS)
        {
            /* If the input path is valid, set the directory in
             * the path to be the current directory.
             */
            status = NU_Set_Current_Dir(current_dir);
        }
    }

    return (status);
}

