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
*       fsp.c                                        1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Server
*
*   DESCRIPTION
*
*       This file contains all of the FTP command primitives needed to
*       create an RFC compliant FTP server application. The primitives
*       consist of a number of functions, including one for each FTP
*       command in the minimum requirements set, as well as two support
*       functions that perform frequently executed tasks. The command
*       primitives themselves either cause a data connection task to be
*       created or calls a higher level routine.
*       All primitives have at least one parameter in the form of a pointer
*       to an FTP_SERVER structure. None of the command primitives perform
*       any validity checking of this structure.  It is assumed that the
*       higher level calling function will perform this check prior to
*       calling a command primitive.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FSP_Command_Read
*       FSP_Filename_Parser
*       FSP_Server_TYPE
*       FSP_Server_MODE
*       FSP_Server_STRU
*       FSP_Server_PORT
*       FSP_Server_EPRT
*       FSP_Server_LIST
*       FSP_Server_RETR
*       FSP_Server_STOR
*       FSP_Server_APPE
*       FSP_Server_NLST
*       FSP_Server_USER
*       FSP_Server_PASS
*       FSP_Server_ACCT
*       FSP_Server_CWD
*       FSP_Server_MKD
*       FSP_Server_RMD
*       FSP_Server_XCWD
*       FSP_Server_XMKD
*       FSP_Server_XRMD
*       FSP_Server_PASV
*       FSP_Server_EPSV
*       FSP_Server_QUIT
*       FSP_Server_SYST
*       FSP_Server_STAT
*       FSP_Server_HELP
*       FSP_Server_PWD
*       FSP_Server_XPWD
*       FSP_Server_RNFR
*       FSP_Server_RNTO
*       FSP_Server_ABOR
*       FSP_Server_DELE
*       FSP_Server_SIZE
*       FSP_Server_REST
*       FSP_Server_FEAT
*       FSP_Server_NOOP
*       FSP_Server_UNKNOWN
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
*       fsp_extr.h
*       ftps_ext.h
*       fst_extr.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/target.h"
#include "networking/ncl.h"
#include "networking/externs.h"
#include "networking/fc_defs.h"
#include "networking/fc_extr.h"
#include "networking/ftps_def.h"
#include "networking/fsp_extr.h"
#include "networking/ftps_ext.h"
#include "networking/fst_extr.h"

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Command_Read
*
*   DESCRIPTION
*
*       This function is one of the general purpose functions included
*       with the primitives. When called it returns a single FTP command, or
*       an error message if the stack returns an error or if the 'next'
*       command in the buffer is not valid.
*
*   INPUTS
*
*       server                   pointer to valid FTP server structure
*       buffer                   destination buffer
*       buffsize                 size of destination buffer
*       timeout                  timeout value for read
*
*   OUTPUTS
*
*       FTP_TIMEOUT              A timeout occurred while attempting to
*                                communicate with a host.
*       FTP_STACK_ERROR          A send or receive command returned an error
*                                while communicating with the host.
*       FTP_REPLY_BUFFER_OVERRUN The reply buffer does not have enough space.
*
******************************************************************************/
INT FSP_Command_Read(FTP_SERVER *server, CHAR *buffer, INT buffsize,
                     UINT32 timeout)
{
    INT         i;
    INT         status;
    INT         bytesReceived;
    INT         out         = 0;
    INT         state       = FCR_START;
    INT         strIndex    = 0;
    FD_SET      readfs;

    for (i=0; i<buffsize; i++) /* clear buffer - for debug */
        buffer[i] = '\0';

    /* Initialize the descriptors bitmap data structure to zero */
    NU_FD_Init(&readfs);

    while (!out)
    {
        /* Buffer needs to be refreshed */
        if (server->replyIndex == server->replyTail)
        {
            /* Set socketd bit of the bitmap to 1 */
            NU_FD_Set(server->socketd, &readfs);

            status = NU_Select((server->socketd + 1), &readfs, NULL,
                               NULL, timeout);

            if (status != NU_SUCCESS)
            {   /* Disable the NU_Select timeout feature */
                if (status == NU_NO_DATA)
                {
                    FTP_Printf("FTP SERVER TIMEOUT.\r\n");
                    strIndex = server->lastError = FTP_TIMEOUT;
                }
                else
                {
                    FTP_Printf("FTP SERVER SOCKET ERROR.\r\n");
                    strIndex = server->lastError = FTP_STACK_ERROR;
                }

                out++;
                server->replyIndex = 0;
                server->replyTail = 0;
                continue;
            }
            else
            {
                /* Turn on the "block during a read" flag */
                NU_Fcntl(server->socketd, NU_SETFLAG, NU_BLOCK);

                /* Go get the server's response and ensure that there is 
                   space left in the buffer(server->replyBuff) to store 
                   a null terminator */
                bytesReceived = (INT)NU_Recv(server->socketd,
                                             server->replyBuff,
                                             (FTPS_REPLY_BUFFER_SIZE - 1), 0);

                /* Turn blocking back off. */
                NU_Fcntl(server->socketd, NU_SETFLAG, NU_FALSE);

                if (bytesReceived < 0)
                {
                    /* Error in call to NU_Recv(). */
                    server->lastError = FTP_STACK_ERROR;
                    server->stackError = bytesReceived;
                    strIndex = bytesReceived;
                    out++;
                    server->replyIndex = 0;
                    server->replyTail = 0;
                }
                else/* valid data received */
                {
                    server->replyIndex = 0;
                    server->replyTail = bytesReceived;
                    server->replyBuff[bytesReceived] = '\0';
                }/* End of if (bytesReceived < 0) */
            }/* End of if (0) */
        }/* End of if (server->replyIndex == server->replyTail) */

        if (server->replyTail) /* Is there something to read? */
        {
            switch(state)
            {
            case FCR_START: /* Initial state. Look for first command letter */
                if ( (NU_TOUPPER(server->replyBuff[server->replyIndex]) >= 'A')
                     &&
                   (NU_TOUPPER(server->replyBuff[server->replyIndex]) <= 'Z') )
                {
                    buffer[strIndex] =
                       (CHAR)NU_TOUPPER(server->replyBuff[server->replyIndex]);
                    state = FCR_LETTER2;
                    strIndex++;
                }
                else
                {
                    state = FCR_BAD_CMD;
                }
                break;

            case FCR_LETTER2: /* Looking for second command letter. */
                if ( (NU_TOUPPER(server->replyBuff[server->replyIndex]) >= 'A')
                     &&
                   (NU_TOUPPER(server->replyBuff[server->replyIndex]) <= 'Z') )
                {
                    buffer[strIndex] =
                       (CHAR)NU_TOUPPER(server->replyBuff[server->replyIndex]);
                    state = FCR_LETTER3;
                    strIndex++;
                }
                else
                {
                    state = FCR_BAD_CMD;
                }
                break;

            case FCR_LETTER3: /* Looking for third command letter. */
                if ( (NU_TOUPPER(server->replyBuff[server->replyIndex]) >= 'A')
                     &&
                   (NU_TOUPPER(server->replyBuff[server->replyIndex]) <= 'Z') )
                {
                    buffer[strIndex] =
                       (CHAR)NU_TOUPPER(server->replyBuff[server->replyIndex]);
                    state = FCR_LETTER4;
                    strIndex++;
                }
                else
                {
                    state = FCR_BAD_CMD;
                }
                break;

            case FCR_LETTER4: /* Looking for fourth command letter. */
                if ( (NU_TOUPPER(server->replyBuff[server->replyIndex]) >= 'A')
                     &&
                   (NU_TOUPPER(server->replyBuff[server->replyIndex]) <= 'Z') )
                {
                    buffer[strIndex] =
                       (CHAR)NU_TOUPPER(server->replyBuff[server->replyIndex]);
                    state = FCR_SPACE;
                    strIndex++;
                }
                else if (server->replyBuff[server->replyIndex] == ' ')
                {
                    buffer[strIndex] = server->replyBuff[server->replyIndex];
                    state = FCR_AFTER_SPACE;
                    strIndex++;
                }
                else if (server->replyBuff[server->replyIndex] == '\r')
                {
                    buffer[strIndex] = server->replyBuff[server->replyIndex];
                    state = FCR_FIND_LF;
                    strIndex++;
                }
                else
                {
                    state = FCR_BAD_CMD;
                }
                break;

            case FCR_SPACE: /* looking for CR */
                if (server->replyBuff[server->replyIndex] == ' ')
                {
                    buffer[strIndex] = server->replyBuff[server->replyIndex];
                    state = FCR_AFTER_SPACE;
                    strIndex++;
                }
                else if (server->replyBuff[server->replyIndex] == '\r')
                {
                    buffer[strIndex] = server->replyBuff[server->replyIndex];
                    state = FCR_FIND_LF;
                    strIndex++;
                }
                else
                {
                    state = FCR_BAD_CMD;
                }
                break;

            case FCR_AFTER_SPACE: /* looking for CR */
                if (server->replyBuff[server->replyIndex] == '\r')
                {
                    state = FCR_FIND_LF;
                }
                else
                {
                    state = FCR_AFTER_SPACE;
                }
                buffer[strIndex] = server->replyBuff[server->replyIndex];
                strIndex++;
                break;

            case FCR_FIND_LF: /* Looking for LF to follow CR */
                if (server->replyBuff[server->replyIndex] == '\n')
                {
                    out++;
                }
                else
                {
                    state = FCR_AFTER_SPACE;
                }
                buffer[strIndex] = server->replyBuff[server->replyIndex];
                strIndex++;
                break;

            case FCR_BAD_CMD: /* Bad reply. Flush to next CR/LF pair */
                if (server->replyBuff[server->replyIndex] == '\r')
                {
                    state = FCR_BAD_FIND_LF;
                }
                strIndex = server->lastError = FTP_BAD_MSG_FORMAT;
                break;

            case FCR_BAD_FIND_LF:
                if (server->replyBuff[server->replyIndex] == '\n')
                {
                    out++;
                }
                else
                {
                    state = FCR_BAD_CMD;
                }
                break;

            default:
                break;
            } /* End of switch */

            server->replyIndex++;
            if ( (strIndex == buffsize) && (!out) )
            {
                /* Not enough buffer space */
                strIndex = server->lastError = FTP_REPLY_BUFFER_OVERRUN;
                out++;
            }
        }
    }

    return (strIndex);

} /* FSP_Command_Read */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Filename_Parser
*
*   DESCRIPTION
*
*       This function converts the drive letter, path and filename to all
*       uppercase to be compatible with the MS-DOS compatible file system
*       implemented by Nucleus FILE.  This function returns status from
*       parsing the command line.
*
*   INPUTS
*
*       fbuffer                 pointer to the file string buffer
*       pbuffer                 pointer to the path string buffer.
*       filename                pointer to pointer to filename buffer.
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        An invalid drive letter was specified.
*
******************************************************************************/
INT FSP_Filename_Parser(CHAR *fbuffer, CHAR *pbuffer, CHAR **filename)
{
    INT status;
    INT i;
    INT dotFlag;
    INT pathLength;
    INT driveNumber;
    CHAR *f = fbuffer;
    CHAR *p = pbuffer;
    CHAR **fptr = filename;

    status = NU_SUCCESS;

    /* Clear path buffer */
    for (i=0; i < FTPS_GENERIC_BUFF_SIZE; i++)
        *(p+i)= '\0';

    /* check if drive is specified in the path */
    if ( (*f) && ((*(f+1) == ':')) )
    {
        /* check if drive letter is valid */
        if ( (*f >= 'a') && (*f <= 'z') )
        {
            *f = (CHAR)(*f - ('a' - 'A'));
        }
        else if ( (*f < 'A') || (*f > 'Z') )
        {
            /* Invalid drive; Get default drive */
            status = FTP_SYNTAX_ERROR;
            driveNumber = NU_Get_Default_Drive();

            if ( (driveNumber < 0) || (driveNumber > 25) )
            {
                /* invalid drive number returned */
                *f = 'C';/* assume C: drive */
            }
            else
            {
                *f = (CHAR)((CHAR)driveNumber + 'A');/* convert to uppercase */
            }
        }

        /* Move past the drive letter. */
        *p++ = *f++;
        *p++ = *f++;
    }

    /* check if directory path or filename is given */
    if (*f)
    {
        i =0;
        dotFlag = 0;
        pathLength = 0;

        while (*(f+i) != '\0')
        {
            if (FTPS_IS_PATH_DELIMITER(*(f+i)))
            {
                /* Translate to the delimiter of our file system. */
                *(f+i) = FTPS_NATIVE_PATH_DELIMITER;

                /* update length with last '/' position */
                pathLength = i;
            }
            else if ( (*(f+i) >= 'a') && (*(f+i) <= 'z') )
            {
                *(f+i) = (CHAR)(*(f+i) - ('a' - 'A'));
            }
            else if (*(f+i) == '.')
                dotFlag = 1;

            i++;
        }

        if (!dotFlag)/* no file name encountered */
            pathLength = i;

        /* copy directory into server structure */
        i = 0;
        while (*(f+i) != '\0')
        {
            *(p+i) = *(f+i);
            ++i;
        }

        /* Null terminate the whole thing. */
        *(p+i) = '\0';

        /* If there exists a parent path, null terminate it at the last '/'. */
        if (pathLength)
        {
            *(p + pathLength) = '\0';

            /*  Assign the filename pointer to the next char if there is
            a file in this path. */
            if (dotFlag)
                *fptr = p + pathLength + 1;
            else
                *fptr = NU_NULL;
        }

        /* No parent path, which means the filename starts at the beginning. */
        else if (dotFlag)
            *fptr = p;

    }

    return (status);

} /* FSP_Filename_Parser */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_TYPE
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP TYPE
*       command from the client. The only parameter is a pointer to a valid
*       FTP server structure.  Note:  The Nucleus File System only supports
*       the image representation type.  This function returns status.
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
*       FTP_INVALID_TYPE_CODE   The type code in server structure is invalid.
*
******************************************************************************/
INT FSP_Server_TYPE(FTP_SERVER *server)
{
    INT status;
    INT index = 5;

    /* Check if ASCII parameter is sent */
    server->cmdFTP.typeFlag = server->replyBuff[index];

    if (server->replyBuff[index] == 'I')
    {
        /* Update the server structure to reflect image type. */
        server->transferType = FTP_TYPE_IMAGE;

        /* Send 'Command okay' message */
        status = (INT)NU_Send(server->socketd, MSG200, SIZE200, 0);

        if (status < 0)
        {
            FTP_Printf("TYPE Cmd NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
        else
            status = NU_SUCCESS;
    }
    /* Check if other valid parameters were received */
    else if (server->replyBuff[index] == 'A')
    {
        /* Update the server structure to reflect ascii type. */
        server->transferType = FTP_TYPE_ASCII;

        /* Send 'Command okay' message */
        status = (INT)NU_Send(server->socketd, MSG200, SIZE200, 0);

        if (status < 0)
        {
            FTP_Printf("TYPE Cmd NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
        else
            status = NU_SUCCESS;
    }
    /* Check if other valid parameters were received */
    else if ( (server->replyBuff[index] == 'E') ||
        (server->replyBuff[index] == 'L') )
    {
        /* Send 'Command parameter not implemented' message */
        status = (INT)NU_Send(server->socketd, MSG504, SIZE504, 0);

        if (status < 0)
        {
            FTP_Printf("TYPE Cmd NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
        else
            status = NU_SUCCESS;
    }
    else
    {
        /* Send 'Syntax error in parameters' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("TYPE Cmd NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_INVALID_TYPE_CODE;
    }

    return (status);

} /* FSP_Server_TYPE */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_MODE
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP MODE
*       command from the client. The only parameter is a pointer to a valid
*       FTP server structure.  Note: Only MODE = Stream is supported.  This
*       function returns status.
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
*       FTP_INVALID_MODE_CODE   The mode code in server structure is invalid.
*
******************************************************************************/
INT FSP_Server_MODE(FTP_SERVER *server)
{
    INT status;

    /* Check if S parameter (for stream) is sent */
    server->cmdFTP.modeFlag = server->replyBuff[5];

    if (server->cmdFTP.modeFlag == 'S')
    {
        /* Send 'Command okay' message */
        status = (INT)NU_Send(server->socketd, MSG200, SIZE200, 0);

        if (status < 0)
        {
            FTP_Printf("MODE Cmd NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
        else
            status = NU_SUCCESS;
    }
    /* Check if other valid parameters were received */
    else if ( (server->cmdFTP.modeFlag == 'B') ||
        (server->cmdFTP.modeFlag == 'C') )
    {
        /* Send 'Command parameter not implemented' message */
        status = (INT)NU_Send(server->socketd, MSG504, SIZE504, 0);

        if (status < 0)
        {
            FTP_Printf("MODE Cmd NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
        else
            status = NU_SUCCESS;
    }
    else
    {
        /* Send 'Syntax error in parameters' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("MODE Cmd NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_INVALID_MODE_CODE;
    }

    return (status);

} /* FSP_Server_MODE */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_STRU
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP STRU
*       command from the client. The only parameter is a pointer to a valid
*       FTP server structure.  Note: Only STRU = File structure is supported.
*       This function returns status.
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
*       FTP_INVALID_STRU_CODE   The structure code in server struct is invalid.
*
******************************************************************************/
INT FSP_Server_STRU(FTP_SERVER *server)
{
    INT status;

    /* Check if File structure parameter is sent */
    server->cmdFTP.struFlag = server->replyBuff[5];

    if (server->cmdFTP.struFlag == 'F')
    {
        /* Send 'Command okay' message */
        status = (INT)NU_Send(server->socketd, MSG200, SIZE200, 0);

        if (status < 0)
        {
            FTP_Printf("NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
        else
            status = NU_SUCCESS;
    }
    /* Check if other valid parameters were received */
    else if ( (server->cmdFTP.struFlag == 'R') ||
        (server->cmdFTP.struFlag == 'P') )
    {
        /* Send 'Command parameter not implemented' message */
        status = (INT)NU_Send(server->socketd, MSG504, SIZE504, 0);

        if (status < 0)
        {
            FTP_Printf("NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
        else
            status = NU_SUCCESS;
    }
    else
    {
        /* Send 'Syntax error in parameters' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_INVALID_STRU_CODE;
    }

    return (status);

} /* FSP_Server_STRU */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_PORT
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP PORT
*       command from the client.  The PORT command specifies the data port to
*       be used for the data connection.  This function attempts to verify that
*       a valid port number is being sent by comparing IP numbers.  The only
*       parameter is a pointer to a valid FTP server structure.  This function
*       returns status.
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
*       FTP_SYNTAX_ERROR        An invalid port number was specified.
*
*******************************************************************************/
INT FSP_Server_PORT(FTP_SERVER *server)
{
    INT status;

    /* Extract client's IP and port numbers from string. */
    if (FC_StringToAddr(&server->clientDataAddr, server->replyBuff + 5)
        == NU_SUCCESS)
    {
        /* Send 'Command Okay' message */
        status = (INT)NU_Send(server->socketd, MSG200, SIZE200, 0);

        if (status < 0)
        {
            FTP_Printf("PORT Cmd NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
        else
            status = NU_SUCCESS;
    }
    else
    {
        /* The port is invalid if any number is not in range. */
        /* Send 'Syntax error' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("PORT Cmd NU_Send failure.\r\n");
            server->stackError = status;
        }

        status = server->lastError = FTP_SYNTAX_ERROR;
    }

    return (status);

} /* FSP_Server_PORT */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_EPRT
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP EPRT
*       command from the client. The EPRT command specifies the data port to be
*       used for the data connection.  This function attempts to verify that a
*       valid port number is being sent by comparing IP numbers.  The only
*       parameter is a pointer to a valid FTP server structure.  This function
*       returns status.
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
*       FTP_SYNTAX_ERROR        An invalid port or family was specified.
*
*******************************************************************************/
INT FSP_Server_EPRT(FTP_SERVER *server)
{
    INT     status;
    CHAR    *family, *ip_addr, *port;
    INT16   family_int;

    /* Parse out the three components of the reply */
    status = FC_Parse_Extended_Command(&server->replyBuff[5], &family,
                                       &ip_addr, &port,
                                       server->replyBuff[5],
                                       (INT)strlen(&server->replyBuff[5]));

    /* If the length of the buffer is greater than the max buffer size,
     * there is a syntax error in the message.  Send an error to the
     * client.
     */
    if (status != NU_SUCCESS)
    {
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("PORT Cmd NU_Send failure.\r\n");
            server->stackError = status;
        }

        status = server->lastError = FTP_SYNTAX_ERROR;
    }

    else
    {
        /* Get the family type of the address from the packet */
        family_int = (INT16)NU_ATOI(family);

        /* Fill in the family type of the other side */
#if (INCLUDE_IPV4 == NU_TRUE)
        if (family_int == 1)
            server->clientDataAddr.family = NU_FAMILY_IP;
#endif

#if ( (defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE) )
#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
            if (family_int == 2)
                server->clientDataAddr.family = NU_FAMILY_IP6;
#endif

        /* The family type is not supported */
            else
            {
                /* Send an error to the client. */
                status = (INT)NU_Send(server->socketd, MSG522, SIZE522, 0);

                if (status < 0)
                {
                    FTP_Printf("PORT Cmd Unrecognized Family.\r\n");
                    server->stackError = status;
                }

                status = server->lastError = FTP_SYNTAX_ERROR;
            }

            /* Process the valid packet */
            if (status == NU_SUCCESS)
            {
#ifdef NET_5_1
                /* Extract client's IP address from the string. */
                NU_Inet_PTON(server->clientDataAddr.family, ip_addr,
                    server->clientDataAddr.id.is_ip_addrs);

                /* Extract the client's data port number from the string. */
                server->clientDataAddr.port = (UINT16)NU_ATOI(port);
#else
                /* Extract client's IP and port numbers from string. */
                FC_StringToAddr(&server->clientDataAddr, ip_addr);
#endif

                /* Send 'Command Okay' message */
                status = (INT)NU_Send(server->socketd, MSG200, SIZE200, 0);

                if (status < 0)
                {
                    FTP_Printf("EPRT Cmd NU_Send failure.\r\n");
                    server->lastError = FTP_STACK_ERROR;
                    server->stackError = status;
                }
                else
                    status = NU_SUCCESS;
            }
    }

    return (status);

} /* FSP_Server_EPRT */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_LIST
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP LIST
*       command from the client.  The LIST command causes a data connection
*       task to be created which will send a directory listing to the client.
*       The only parameter is a pointer to a valid FTP server structure.
*       The name of the directory to be listed is stored in the FTP server
*       structure.This function returns status.
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
*       FTP_SYNTAX_ERROR        An invalid drive letter was specified.
*       FTP_MEMORY              No memory available to allocate.
*       Nucleus Plus specific error codes.
*
******************************************************************************/
INT FSP_Server_LIST(FTP_SERVER *server)
{
    INT status;
    INT index;
    CHAR *src_ptr = &server->replyBuff[4];
    CHAR *dst_ptr = &server->replyBuff[4];

    index = 0;
    
	/* Move src_ptr to non-white space character. */
    while (*src_ptr == ' ')
        src_ptr++;
		
    /* Update src_ptr to a non-switch word of the command.
       Note: Assumption is made that all switch words will
       exist before the non-switch words of the command.*/
    while (*src_ptr == '-')
    {
        src_ptr++;
		
		/* Move src_ptr ahead of the switch word. */ 
        while ((*src_ptr != '\0') && (*src_ptr != '\n') &&
               (*src_ptr != '\r') && (*src_ptr != ' '))
            src_ptr++;
		
        /* Move src_ptr to non-white space character. */		
        while (*src_ptr == ' ')
            src_ptr++;
    }
	
	/* If there were arguments in the command. */
    if (src_ptr != dst_ptr)
    {
	    /* Replace the switch word(s) with non-swtich word(s). */
        while ((*src_ptr != '\0') && (*src_ptr != '\n') && (*src_ptr != '\r'))
            *dst_ptr++ = *src_ptr++;
		
        /* LIST cmd should end with '\r''\n''\0'. */		
        *dst_ptr++ = '\r';
        *dst_ptr++ = '\n';
        *dst_ptr++ = '\0';
    }

    /* if no argument is found then LIST cmd should be followed by \r & \n. */
    while (((server->fileSpec[index] = server->replyBuff[index+5]) != '\n') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 5)))
        index++;

    if (!index)/* use default directory listing if none specified */
    {
        server->fileSpec[index++]= '*';
        server->fileSpec[index++]= '.';
        server->fileSpec[index++]= '*';
        server->fileSpec[index]= '\0';
        server->path[0] = '\0';
        status = NU_SUCCESS;
    }
    else
    {
        server->fileSpec[index-1]= '\0';

        /* Convert all lower case to upper case in fileSpec */
        status = FSP_Filename_Parser(server->fileSpec, server->path,
                                     &server->filename);
    }

    if (status != NU_SUCCESS) /* Error in parsing command */
    {
        FTP_Printf("LIST Cmd FSP_Filename_Parser failure.\r\n");
        server->lastError = status;
    }
    else
    {

        /* Set an event to denote the function to process. */
        status = NU_Set_Events(&server->FTP_Events,
                               (UNSIGNED)LIST_Event, NU_OR);

        /* If the server is currently in passive mode, then there
         * is already a task waiting for a client data connection.
         * Otherwise we need to create the task now.
         */
        if (server->cmdFTP.pasvFlag == FTP_PASSIVE_MODE_OFF)
        {
            status = FTPS_Create_Data_Task(server);

            if (status != NU_SUCCESS)
            {
                FTP_Printf("LIST Cmd FTPS_Create_Data_Task failure.\r\n");
                server->lastError = FTP_STACK_ERROR;
                server->stackError = status;
            }   /* Error creating data task */

        }   /* Server is not in passive mode */

    }   /* Filename parsed successfully */

    return (status);

} /* FSP_Server_LIST */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_RETR
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP RETR
*       command from the client.  This command causes a FTP data connection
*       task to be created which will transfer a copy of the requested file or
*       data to the client at the other end of the data connection.  The only
*       parameter is a pointer to a valid FTP server structure.  The character
*       string containing the path and/or name of the file to be sent is stored
*       in the FTP server structure.  This function returns status.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        An invalid drive letter was specified.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the host.
*       OS-specific error codes.
*
******************************************************************************/
INT FSP_Server_RETR(FTP_SERVER *server)
{
    INT status;
    INT index;

    index = 0;

    /* Parse the directory pathname or files */
    while ((server->replyBuff[index+5]!= '\r') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 5)))
    {
        server->fileSpec[index] = server->replyBuff[index+5];
        index++;
    }

    server->fileSpec[index] = '\0';

    /* Convert all lower case to upper case in fileSpec */
    server->lastError = FSP_Filename_Parser(server->fileSpec, server->path,
        &server->filename);

    if (!index)
    {
        /* Send 'Syntax error in parameters' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("RETR Cmd NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_SYNTAX_ERROR;
    }
    else
    {
        /* Set an event to denote the function to process. */
        status = NU_Set_Events(&server->FTP_Events,
                               (UNSIGNED)RETR_Event, NU_OR);

        /* If the server is currently in passive mode, then there
         * is already a task waiting for a client data connection.
         * Otherwise we need to create the task now.
         */
        if (server->cmdFTP.pasvFlag == FTP_PASSIVE_MODE_OFF)
        {
            status = FTPS_Create_Data_Task(server);
            if (status != NU_SUCCESS)
            {
                FTP_Printf("RETR Cmd FTPS_Create_Data_Task failure.\r\n");
                server->lastError = FTP_STACK_ERROR;
                server->stackError = status;
            }   /* Error creating data task */

        }   /* Server is not in passive mode */

    }   /* Filename parsed successfully */

    return (status);

} /* FSP_Server_RETR */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_STOR
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP STOR
*       command from the client.  This command causes a FTP data connection
*       to be created which will transfer a copy of the requested file or
*       data from the client. The only parameter is a pointer to a valid FTP
*       server structure.  The character string containing the path and/or name
*       of the file to be sent is stored in the FTP server structure.  This
*       function returns status.
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
*       FTP_SYNTAX_ERROR        An invalid drive letter was specified.
*       OS-specific error codes.
*
******************************************************************************/
INT FSP_Server_STOR(FTP_SERVER *server)
{
    INT status;
    INT index;

    index = 0;

    /* Parse the directory pathname or files */
    while ((server->replyBuff[index+5]!= '\r') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 5)))
    {
        server->fileSpec[index] = server->replyBuff[index+5];
        index++;
    }

    server->fileSpec[index] = '\0';

    /* Convert all lower case to upper case in fileSpec */
    server->lastError = FSP_Filename_Parser(server->fileSpec, server->path,
                                            &server->filename);

    if (!index)
    {
        /* Send 'Syntax error in parameters' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_SYNTAX_ERROR;
    }
    else
    {
        /* Set an event to denote the function to process. */
        status = NU_Set_Events(&server->FTP_Events,
                               (UNSIGNED)STOR_Event, NU_OR);

        /* If the server is currently in passive mode, then there
         * is already a task waiting for a client data connection.
         * Otherwise we need to create the task now.
         */
        if (server->cmdFTP.pasvFlag == FTP_PASSIVE_MODE_OFF)
        {
            status = FTPS_Create_Data_Task(server);
            if (status != NU_SUCCESS)
            {
                FTP_Printf("STOR Cmd FTPS_Create_Data_Task failure.\r\n");
                server->lastError = FTP_STACK_ERROR;
                server->stackError = status;
            }   /* Error creating data task */

        }   /* Server is not in passive mode */

    }   /* Filename parsed successfully */

    return (status);

} /* FSP_Server_STOR */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_APPE
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP APPE
*       command from the client.  This command causes a FTP data connection
*       task to be created which will transfer a copy of the requested file or
*       data from the client and append to a file on the server. The only
*       parameter is a pointer to a valid FTP server structure.  The character
*       string containing the path and/or name of the file to be sent is stored
*       in the FTP server structure.  This function returns status.
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
*       FTP_SYNTAX_ERROR        An invalid drive letter was specified.
*       FTP_MEMORY              No memory available to allocate.
*
******************************************************************************/
INT FSP_Server_APPE(FTP_SERVER *server)
{
    INT status;
    INT index;

    index = 0;

    /* Parse the directory pathname or files */
    while ((server->replyBuff[index+5]!= '\r') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 5)))
    {
        server->fileSpec[index] = server->replyBuff[index+5];
        index++;
    }

    server->fileSpec[index] = '\0';

    /* Convert all lower case to upper case in fileSpec */
    server->lastError = FSP_Filename_Parser(server->fileSpec, server->path,
                                            &server->filename);
    if (index == 0)
    {
        /* Send 'Syntax error in parameters' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("APPE Cmd NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_SYNTAX_ERROR;
    }
    else
    {
        /* Set an event to denote the function to process. */
        status = NU_Set_Events(&server->FTP_Events,
                               (UNSIGNED)APPE_Event, NU_OR);

        /* If the server is currently in passive mode, then there
         * is already a task waiting for a client data connection.
         * Otherwise we need to create the task now.
         */
        if (server->cmdFTP.pasvFlag == FTP_PASSIVE_MODE_OFF)
        {
            status = FTPS_Create_Data_Task(server);

            if (status != NU_SUCCESS)
            {
                FTP_Printf("APPE Cmd FTPS_Create_Data_Task failure.\r\n");
                server->lastError = FTP_STACK_ERROR;
                server->stackError = status;
            }   /* Error creating data task */

        }   /* Server is not in passive mode */

    }   /* Filename parsed successfully */

    return (status);

} /* FSP_Server_APPE */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_NLST
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP NLST
*       command from the client.  The LIST command causes a data connection task
*       to be created which will send a file listing to the client.  The only
*       parameter is a pointer to a valid FTP server structure.  The name of the
*       directory to be listed is stored in the FTP server structure.  This
*       function returns status.
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
*       FTP_SYNTAX_ERROR        An invalid drive letter was specified.
*       OS-specific error codes.
*
******************************************************************************/
INT FSP_Server_NLST(FTP_SERVER *server)
{
    INT status;
    INT index;

    index = 0;

    /* if no argument is found then NLST cmd should be followed by \r & \n. */
    while (((server->fileSpec[index] = server->replyBuff[index+5]) != '\n') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 5)))
        index++;

    if (!index)/* use default directory listing if none specified */
    {
        server->fileSpec[index++]= '*';
        server->fileSpec[index++]= '.';
        server->fileSpec[index++]= '*';
        server->fileSpec[index]= '\0';
        server->path[0] = '\0';
        status = NU_SUCCESS;
    }
    else
    {
        server->fileSpec[index-1]= '\0';

        /* Convert all lower case to upper case in fileSpec */
        status = FSP_Filename_Parser(server->fileSpec, server->path,
                                     &server->filename);
    }

    if (status != NU_SUCCESS)
    {
        FTP_Printf("NLST Cmd NU_Send failure.\r\n");
        server->lastError = status;
    }
    else
    {
        /* Set an event to denote the function to process. */
        status = NU_Set_Events(&server->FTP_Events,
                               (UNSIGNED)NLST_Event, NU_OR);

        /* If the server is currently in passive mode, then there
         * is already a task waiting for a client data connection.
         * Otherwise we need to create the task now.
         */
        if (server->cmdFTP.pasvFlag == FTP_PASSIVE_MODE_OFF)
        {
            status = FTPS_Create_Data_Task(server);

            if (status != NU_SUCCESS)
            {
                FTP_Printf("NLST Cmd FTPS_Create_Data_Task failure.\r\n");
                server->lastError = FTP_STACK_ERROR;
                server->stackError = status;
            }   /* Error creating data task */

        }   /* Server is not in passive mode */

    }   /* Filename parsed successfully */

    return (status);

} /* FSP_Server_NLST */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_USER
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP USER
*       command from the client.  This command is required by this server for
*       access to its file system.  The only parameter is a pointer to a valid
*       FTP server structure.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_INVALID_USER        No username was received from the host.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the host.
*
******************************************************************************/
INT FSP_Server_USER(FTP_SERVER *server)
{
    INT status;
    INT index;

    index = 0;

    if (strlen(server->replyBuff) < FTP_MAX_ID_LENGTH + 7) /* +7 since USER, */
        /* '\r', and '\n' */
        /* are still in   */
        /* the buffer.    */
    {
        /* Update server structure */
        server->cmdFTP.userFlag = FTP_CMD_RECEIVED;

        /*  Loop until we are within bounds of the reply buffer size and
            within bounds of the maximum ID length. Also leave space for
            the terminating null character for the ID. */
        while ((server->replyBuff[index+5] != '\r') &&
               (index < (FTPS_GENERIC_BUFF_SIZE - 5)) &&
               (index < FTP_MAX_ID_LENGTH - 1))
        {
            server->user[index] = server->replyBuff[index+5];
            index++;
        }
        server->user[index]= '\0';

        if (index == 0) /* No user name received */
        {
            /* Send 'Syntax error' message */
            status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

            if (status < 0)
            {
                FTP_Printf("USER Cmd NU_Send failure.\r\n");
                server->stackError = status;
            }
            status = server->lastError = FTP_INVALID_USER;
        }
        else
        {
            /* Send 'need password' Reply */
            status = (INT)NU_Send(server->socketd, MSG331, SIZE331, 0);

            if (status < 0)
            {
                FTP_Printf("USER Cmd NU_Send failure.\r\n");
                server->stackError = status;
            }
            else
            {
                server->lastError = FTP_NEED_PASSWORD;
                status = NU_SUCCESS;
            }
        }
    }
    else
    {
        /* Send 'Syntax error' Reply */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("USER Cmd NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_INVALID_USER;
    }

    return (status);

} /* FSP_Server_USER */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_PASS
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP PASS
*       command from the client.  The only parameter is a pointer to a valid
*       FTP server structure.  This function returns status.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_INVALID_PASSWORD    No password was received from the host or
*                               the received password failed authentication.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the host.
*       NU_MEM_ALLOC            Failed to allocate memory.
*
******************************************************************************/
INT FSP_Server_PASS(FTP_SERVER *server)
{
    INT status;
    INT index;
    CHAR *buffer;
    
    /* allocate memory for buffer out of System Memory */
    status = NU_Allocate_Memory(&System_Memory, (VOID**)&buffer,
                                FTPS_GENERIC_BUFF_SIZE, NU_NO_SUSPEND);
    
    /* if the allocation failed */
    if (status != NU_SUCCESS)
    {
        FTP_Printf("***** FSP_Server_PASS(): Unable to Allocate memory for buffer *****\r\n");
        return (NU_MEM_ALLOC);
    }

    index = 0;

    if ( (server->cmdFTP.userFlag != FTP_CMD_RECEIVED) ||
        (server->cmdFTP.passFlag == FTP_CMD_RECEIVED) ||
        (server->lastError != FTP_NEED_PASSWORD) )
    {
        /* Send 'Bad command sequence' message */
        status = (INT)NU_Send(server->socketd, MSG503, SIZE503, 0);

        if (status < 0)
        {
            FTP_Printf("PASS Cmd NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
        server->lastError = FTP_INVALID_PASSWORD;
    }
    else
    {
        /* FireFTP (version 0.90.1 beta) does not put in     */
        /* RFC 959 required space when there is no password. */
        if (server->replyBuff[index+4] != '\r')
        {
            while ((server->replyBuff[index+5] != '\r') &&
                   (index < (FTPS_GENERIC_BUFF_SIZE - 5)))
            {
                buffer[index] = server->replyBuff[index+5];
                index++;
            }
        }
        buffer[index]= '\0';

        if (index == 0) /* No password received */
        {
            /* Send 'Syntax error' message */
            status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

            if (status < 0)
            {
                FTP_Printf("NU_Send failure.\r\n");
                server->stackError = status;
            }
            server->lastError = FTP_INVALID_PASSWORD;
        }
        else
        {
            /* Check the password against our password list. */
            if (FTPS_Authenticate(server->user, buffer))
            {
                /* set the password flag */
                server->cmdFTP.passFlag = FTP_CMD_RECEIVED;
                server->validPattern = (FTP_VALID_PATTERN | server->socketd);
                server->transferType = FTP_TYPE_ASCII;
                server->lastError    = NU_SUCCESS;

                /* Send 'User logged in' message */
                status = (INT)NU_Send(server->socketd, MSG230, SIZE230, 0);

                if (status < 0)
                {
                    FTP_Printf("NU_Send failure.\r\n");
                    server->lastError = FTP_STACK_ERROR;
                    server->stackError = status;
                }
            }
            else
            {
                /* Password invalid. */
                server->cmdFTP.userFlag = 0;
                server->cmdFTP.passFlag = 0;
                server->lastError = FTP_INVALID_PASSWORD;

                /* Send 'Invalid login' message */
                status = (INT)NU_Send(server->socketd, MSG530, SIZE530, 0);

                if (status < 0)
                {
                    FTP_Printf("NU_Send failure.\r\n");
                    server->lastError = FTP_STACK_ERROR;
                    server->stackError = status;
                }
            }
        }
    }
    
    /* release the buffer memory allocated at the top of the function */
    status = NU_Deallocate_Memory((VOID*)buffer);
    
    /* if the memory release failed */
    if (status != NU_SUCCESS)
        FTP_Printf("NU_Deallocate_Memory error.\r\n");
        
    return (server->lastError);

} /* FSP_Server_PASS */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_ACCT
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP ACCT
*       command from the client.  The only parameter is a pointer to a valid
*       FTP server structure.  This function returns status. Note: the account
*       sent with this command is ignored.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_INVALID_ACCOUNT     No username or password information was
*                               received from the host.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the host.
*       FTP_SYNTAX_ERROR        The server's reply buffer did not contain any
*                               account information.
*
******************************************************************************/
INT FSP_Server_ACCT(FTP_SERVER *server)
{
    INT status;

    /* Account info. is ignored; Loaded into buffer */
    if ( (server->cmdFTP.userFlag != FTP_CMD_RECEIVED) ||
        (server->cmdFTP.passFlag != FTP_CMD_RECEIVED) )
    {
        /* Send 'Bad command sequence' message */
        status = (INT)NU_Send(server->socketd, MSG503, SIZE503, 0);

        if (status < 0)
        {
            FTP_Printf("ACCT Cmd NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_INVALID_ACCOUNT;
    }
    else
    {
        if (server->replyBuff[5] == '\r')
        {
            /* No account information received
            * Send 'Syntax error' message */
            status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

            if (status < 0)
            {
                FTP_Printf("NU_Send failure.\r\n");
                server->stackError = status;
            }
            status = server->lastError = FTP_SYNTAX_ERROR;
        }
        else
        {
            /* clear login command sequence flags for next user */
            server->cmdFTP.userFlag = 0;
            server->cmdFTP.passFlag = 0;

            /* Send 'Command not implemented' message. Remove this
            when the command is handled by the application. */
            status = (INT)NU_Send(server->socketd, MSG202, SIZE202, 0);

            if (status < 0)
            {
                FTP_Printf("NU_Send failure.\r\n");
                server->lastError = FTP_STACK_ERROR;
                server->stackError = status;
            }
            status = NU_SUCCESS;
        }
    }

    return (status);

} /* FSP_Server_ACCT */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_CWD
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP CWD
*       command from the client.  This command allows the user to work with a
*       different directory or dataset for file storage or retrieval.  This
*       routine calls the function which performs the actual CWD task.  The
*       only parameter is a pointer to a valid FTP server structure. A pathname
*       specifying a directory is loaded into the server structure.  This
*       function returns status.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        A send or receive command returned an error
*                               while communicating with the host.
*       FTP_CURRENT_DIR_FAILURE An error occurred while trying to determine
*                               the current directory on specified drive.
*       File system specific error codes.
*
******************************************************************************/
INT FSP_Server_CWD(FTP_SERVER *server)
{
    INT status;
    INT index;

    /* parse the directory path after the command */
    index = 0;
    while ((server->replyBuff[index+4] != '\r') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 4)))
    {
        server->fileSpec[index] = server->replyBuff[index+4];
        index++;
    }

    server->fileSpec[index] = '\0';

    /* Convert all lower case to upper case in fileSpec */
    server->lastError = FSP_Filename_Parser(server->fileSpec, server->path,
                                            &server->filename);

    if (!index)
    {
        /* Send 'Syntax error in parameters' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("FSP_Server_CWD NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_SYNTAX_ERROR;
    }
    else
    {
        status = FTPS_Server_ChDir(server);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("FTPS_Server_ChDir() error during CWD Cmd.\r\n");
            server->lastError = status;
        }
    }

    return (status);

} /* FSP_Server_CWD */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_XCWD
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP XCWD
*       command from the client.  This command allows the user to work with a
*       different directory or dataset for file storage or retrieval.  This
*       routine calls the function which performs the actual XCWD task.  The
*       only parameter is a pointer to a valid FTP server structure. A pathname
*       specifying a directory is loaded into the server structure.  This
*       function returns status.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        A send or receive command returned an error
*                               while communicating with the host.
*       FTP_CURRENT_DIR_FAILURE An error occurred while trying to determine
*                               the current directory on specified drive.
*       File system specific error codes.
*
******************************************************************************/
INT FSP_Server_XCWD(FTP_SERVER *server)
{
    INT status;
    INT index;

    /* parse the directory path after the command */
    index = 0;
    while ((server->replyBuff[index+5]!= '\r') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 5)))
    {
        server->fileSpec[index] = server->replyBuff[index+5];
        index++;
    }

    server->fileSpec[index] = '\0';

    /* Convert all lower case to upper case in fileSpec */
    server->lastError = FSP_Filename_Parser(server->fileSpec, server->path,
                                            &server->filename);

    if (!index)
    {
        /* Send 'Syntax error in parameters' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("FSP_Server_XCWD NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_SYNTAX_ERROR;
    }
    else
    {
        status = FTPS_Server_ChDir(server);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("FTPS_Server_ChDir() error during XCWD Cmd.\r\n");
            server->lastError = status;
        }
        else
            status = NU_SUCCESS;
    }

    return (status);

} /* FSP_Server_XCWD */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_QUIT
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP QUIT
*       command from the client. In response to the QUIT command, this function
*       closes the control connection.  If file transfer is in progress, the
*       connection will remain open for result response before closing. The
*       only parameter is a pointer to a valid FTP server structure.  This
*       function returns status.
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
INT FSP_Server_QUIT(FTP_SERVER *server)
{
    INT status;

    /* Send 'Service closing' message */
    status = (INT)NU_Send(server->socketd, MSG221, SIZE221, 0);

    if (status < 0)
    {
        FTP_Printf("QUIT Cmd NU_Send failure.\r\n");
        server->lastError = FTP_STACK_ERROR;
        server->stackError = status;
    }
    else
        status = NU_SUCCESS;

    return (status);

} /* FSP_Server_QUIT */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_ABOR
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP ABOR
*       command from the client. This command aborts the previous FTP service
*       command and any associated transfer of data.  The only parameter is a
*       pointer to a valid FTP server structure.  This function returns status.
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
INT FSP_Server_ABOR(FTP_SERVER *server)
{
    INT status;

    /* Send 'Closing data connection' message */
    status = (INT)NU_Send(server->socketd, MSG226, SIZE226, 0);

    if (status < 0)
    {
        FTP_Printf("ABOR Cmd NU_Send failure.\r\n");
        server->lastError = FTP_STACK_ERROR;
        server->stackError = status;
    }
    else
        status = NU_SUCCESS;

    return (status);

} /* FSP_Server_ABOR */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_MKD
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP MKD
*       command from the client.  This command causes the directory specified
*       in the pathname to be created.  This routine calls the function that
*       actually performs the MKD task.  The only parameter is a pointer to a
*       valid FTP server structure.  The name of the directory to be created is
*       stored in the FTP server structure.  This function returns status.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        A send or receive command returned an error
*                               while communicating with the host.
*       FTP_STACK_ERROR         File system returned an i/o error or
*                               a send operation failed.
*
******************************************************************************/
INT FSP_Server_MKD(FTP_SERVER *server)
{
    INT status;
    INT index;

    index = 0;
    /* Scan for a '\r' in replyBuff, copy replyBuff to fileSpec */
    while (((server->fileSpec[index] = server->replyBuff[index+4]) != '\r') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 4)))
        index++;
    server->fileSpec[index]= '\0';

    /* Convert all lower case to upper case in fileSpec */
    server->lastError = FSP_Filename_Parser(server->fileSpec, server->path,
                                            &server->filename);

    if (index == 0) /* No directory info. received */
    {
        /* Send 'Syntax error in parameter' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("MKD Cmd NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_SYNTAX_ERROR;
    }
    else /* Directory info. received, make directory */
    {
        status = FTPS_Server_MkDir(server);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("FTPS_Server_MkDir() error during MKD Cmd.\r\n");
            server->lastError = status;
        }
    }

    return (status);

} /* FTP_Server_MKD */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_XMKD
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP XMKD
*       command from the client.  This command causes the directory specified
*       in the pathname to be created.  This routine calls the function that
*       actually performs the XMKD task.  The only parameter is a pointer to a
*       valid FTP server structure.  The name of the directory to be created is
*       stored in the FTP server structure.  This function returns status.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        A send or receive command returned an error
*                               while communicating with the host.
*       FTP_STACK_ERROR         File system returned an i/o error or
*                               a send operation failed.
*
*******************************************************************************/
INT FSP_Server_XMKD(FTP_SERVER *server)
{
    INT status;
    INT index;

    index = 0;
    /* Scan for a '\r' in replyBuff, copy replyBuff to fileSpec */
    while (((server->fileSpec[index] = server->replyBuff[index+5]) != '\r') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 5)))
        index++;
    server->fileSpec[index]= '\0';

    /* Convert all lower case to upper case in fileSpec */
    server->lastError = FSP_Filename_Parser(server->fileSpec, server->path,
                                            &server->filename);

    if (index == 0) /* No directory info. received */
    {
        /* Send 'Syntax error in parameter' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("XMKD Cmd NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_SYNTAX_ERROR;
    }
    else /* Directory info received, create directory */
    {
        status = FTPS_Server_MkDir(server);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("FTPS_Server_MkDir() error during XMKD Cmd.\r\n");
            server->lastError = status;
        }
    }

    return (status);

} /* FSP_Server_XMKD */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_RMD
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP RMD
*       command from the client. This command causes the directory specified in
*       the pathname to be removed.  This routine calls the function that
*       actually performs the RMD task.  The only parameter is a pointer to a
*       valid FTP server structure.  The name of the directory to be removed is
*       stored in the FTP server structure.  This function returns status.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        A send or receive command returned an error
*                               while communicating with the host.
*       FTP_STACK_ERROR         File system returned an i/o error or
*                               a send operation failed.
*
******************************************************************************/
INT FSP_Server_RMD(FTP_SERVER *server)
{
    INT status;
    INT index;

    index = 0;
    /* Scan for a '\r' in replyBuff, copy replyBuff to fileSpec */
    while (((server->fileSpec[index] = server->replyBuff[index+4]) != '\r') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 4)))
        index++;

    server->fileSpec[index]= '\0';

    /* Convert all lower case to upper case in fileSpec */
    server->lastError = FSP_Filename_Parser(server->fileSpec, server->path,
        &server->filename);

    if (index == 0) /* No directory info. received */
    {
        /* Send 'Syntax error in parameter' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("RMD Cmd NU_Send failure.\r\n");
            server->stackError = status;
        }

        status = server->lastError = FTP_SYNTAX_ERROR;
    }
    else /* Directory info received, remove directory */
    {

        status = FTPS_Server_RmDir(server);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("RMD Cmd NU_Send failure.\r\n");
            server->lastError = status;
        }
    }

    return (status);

} /* FSP_Server_RMD */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_XRMD
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP XRMD
*       command from the client. This command causes the directory specified in
*       the pathname to be removed.  This routine calls the function that
*       actually performs the XRMD task.  The only parameter is a pointer to a
*       valid FTP server structure.  The name of the directory to be removed is
*       stored in the FTP server structure.  This function returns status.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        A send or receive command returned an error
*                               while communicating with the host.
*       FTP_STACK_ERROR         File system returned an i/o error or
*                               a send operation failed.
*
******************************************************************************/
INT FSP_Server_XRMD(FTP_SERVER *server)
{
    INT status;
    INT index;

    index = 0;
    /* Scan for a '\r' in replyBuff, copy replyBuff to fileSpec */
    while (((server->fileSpec[index] = server->replyBuff[index+5]) != '\r') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 5)))
        index++;

    server->fileSpec[index]= '\0';

    /* Convert all lower case to upper case in fileSpec */
    server->lastError = FSP_Filename_Parser(server->fileSpec, server->path,
                                            &server->filename);

    if (index == 0) /* No directory info. received */
    {
        /* Send 'Syntax error in parameter' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("XRMD Cmd NU_Send failure.\r\n");
            server->stackError = status;
        }

        status = server->lastError = FTP_SYNTAX_ERROR;
    }
    else /* Directory info received, remove directory */
    {

        status = FTPS_Server_RmDir(server);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("XRMD Cmd NU_Send failure.\r\n");
            server->lastError = status;
        }
    }

    return (status);

} /* FSP_Server_XRMD */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_PASV
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response, which includes
*       an available data port in ASCII form, to an FTP PASV (passive connect)
*       command from the client. The only parameter is a pointer to a valid
*       FTP server structure.  This function returns status.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        A send or receive command returned an error
*                               while communicating with the host.
*
******************************************************************************/
INT FSP_Server_PASV(FTP_SERVER *server)
{
    INT      status;

    /* Get an available port number from the stack and start listening on
    that port. */
    server->cmdFTP.pasvFlag = FTP_PASSIVE_MODE_ON;

    /* Start a task that will listen on a new data connection and send its
    port number back to the client. */
    status = FTPS_Create_Data_Task(server);

    if (status != NU_SUCCESS)
    {
        /* The task was not created. */
        /* Send 'Service not available' message */
        status = (INT)NU_Send(server->socketd, MSG421, SIZE421, 0);

        if (status < 0)
        {
            FTP_Printf("PASV Cmd NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_SYNTAX_ERROR;
    }

    return (status);

} /* FSP_Server_PASV */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_EPSV
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response, which includes
*       an available data port in ASCII form, to an FTP EPSV (passive connect)
*       command from the client. The only parameter is a pointer to a valid
*       FTP server structure.  This function returns status.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        A send or receive command returned an error
*                               while communicating with the host or the IP
*                               family of the host was not recognized.
*
******************************************************************************/
INT FSP_Server_EPSV(FTP_SERVER *server)
{
    INT     status = NU_SUCCESS;
    UINT8   family;

    /* Get an available port number from the stack and start listening on
    that port. */
    server->cmdFTP.pasvFlag = FTP_EPASSIVE_MODE_ON;

    /* If a network protocol type has been specified other than "ALL", verify
    * that it is supported.
    */
    if ( (server->replyBuff[4] == ' ') &&
        (strcmp(&server->replyBuff[5], "ALL") != 0) )
    {
        /* Extract the family type */
        family = (UINT8)NU_ATOI(&server->replyBuff[5]);

        /* If the family type is not supported, send an error to the client
        * along with the family types that are supported.
        */
        if (
#if (INCLUDE_IPV4 == NU_TRUE)
            (family != 1)
#endif
#if ( (defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE) )
#if (INCLUDE_IPV4 == NU_TRUE)
            &&
#endif
            (family != 2)
#endif
            )
        {
            /* Send an error to the client. */
            status = (INT)NU_Send(server->socketd, MSG522, SIZE522, 0);

            if (status < 0)
            {
                FTP_Printf("PORT Cmd Unrecognized Family.\r\n");
                server->stackError = status;
            }

            status = server->lastError = FTP_SYNTAX_ERROR;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Start a task that will listen on a new data connection and send its
        port number back to the client. */
        status = FTPS_Create_Data_Task(server);

        if (status != NU_SUCCESS)
        {
            /* The task was not created. */
            /* Send 'Service not available' message */
            status = (INT)NU_Send(server->socketd, MSG421, SIZE421, 0);

            if (status < 0)
            {
                FTP_Printf("PASV Cmd NU_Send failure.\r\n");
                server->stackError = status;
            }
            status = server->lastError = FTP_SYNTAX_ERROR;
        }
    }

    return (status);

} /* FSP_Server_EPSV */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_SYST
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP SYST
*       command from the client. The only parameter is a pointer to a valid
*       FTP server structure.  This function returns status.
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
INT FSP_Server_SYST(FTP_SERVER *server)
{
    INT status;

    /* Send 'System' message */
    status = (INT)NU_Send(server->socketd, MSG215, SIZE215, 0);

    if (status < 0)
    {
        FTP_Printf("SYST Cmd NU_Send failure.\r\n");
        server->lastError = FTP_STACK_ERROR;
        server->stackError = status;
    }
    else
        status = NU_SUCCESS;

    return (status);

} /* FSP_Server_SYST */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_STAT
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP STAT
*       command from the client. The command causes a status response to be
*       sent over the control connection. The only parameter is a pointer to a
*       valid FTP server structure.  This function returns status.
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
INT FSP_Server_STAT(FTP_SERVER *server)
{
    INT status;

    /* Send 'System' message */
    status = (INT)NU_Send(server->socketd, MSG211, SIZE211, 0);

    if (status < 0)
    {
        FTP_Printf("STAT Cmd NU_Send failure.\r\n");
        server->lastError = FTP_STACK_ERROR;
        server->stackError = status;
    }
    else
        status = NU_SUCCESS;

    return (status);

} /* FSP_Server_STAT */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_HELP
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP HELP
*       command from the client. The only parameter is a pointer to a valid
*       FTP server structure.  This function returns status.
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
INT FSP_Server_HELP(FTP_SERVER *server)
{
    INT status;

    /* Send 'Help' message */
    status = (INT)NU_Send(server->socketd, MSG214, SIZE214, 0);

    if (status >= 0)
    {
        /* Send first line of the help menu */
        status = (INT)NU_Send(server->socketd, MSG214a, SIZE214a, 0);

        if (status >= 0)
        {
            /* Send second line of the help menu */
            status = (INT)NU_Send(server->socketd, MSG214b, SIZE214a, 0);

            if (status >= 0)
            {
                /* Send third line of the help menu */
                status = (INT)NU_Send(server->socketd, MSG214c, SIZE214a, 0);

                if (status >= 0)
                {
                    /* Send fourth line of the help menu */
                    status = (INT)NU_Send(server->socketd, MSG214d,
                                          SIZE214a, 0);

                    if (status >= 0)
                    {
                        /* Send last line of the help menu */
                        status = (INT)NU_Send(server->socketd, MSG214e,
                                              SIZE214e, 0);

                        if (status >= 0)
                           status = NU_SUCCESS;

                    }   /*  Fourth line of help menu */
                }   /*  ... Third  line of help menu */
            }   /* ...  ... Second line of help menu */
        }   /* ... ...  ... First line of help menu */
    }   /* ...  .. ...  ... 'Help' message */

    if (status < 0)
    {
        FTP_Printf("Help Cmd NU_Send failure.\r\n");
        server->lastError = FTP_STACK_ERROR;
        server->stackError = status;
    }

    return (status);

} /* FSP_Server_HELP */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_PWD
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP PWD
*       command from the client.  This command causes the name of the current
*       working directory to be returned.  The only parameter is a pointer to a
*       valid FTP server structure.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        A send or receive command returned an error
*                               while communicating with the host.
*
******************************************************************************/
INT FSP_Server_PWD(FTP_SERVER *server)
{
    INT status;
    INT index;

    /* Is there any data besides the command? - shouldn't be */
    index = 0;
    /* Scan for a '\r' in replyBuff, copy replyBuff to fileSpec */
    while ((server->replyBuff[index+3]!= '\r') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 3)))
    {
        server->fileSpec[index] = server->replyBuff[index+3];
        index++;
    }

    server->fileSpec[index] = '\0';

    if (index) /* Data present after command */
    {
        /* Send 'Syntax error in parameters' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_SYNTAX_ERROR;
    }
    else /* No data, print directory */
    {
        status = FTPS_Server_PrintDir(server);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("PWD Cmd NU_Send failure.\r\n");
            server->lastError = status;
        }
    }

    return (status);

} /* FTP_Server_PWD */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_XPWD
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP XPWD
*       command from the client.  This command causes the name of the current
*       working directory to be returned.  The only parameter is a pointer to a
*       valid FTP server structure.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        A send or receive command returned an error
*                               while communicating with the host.
*
******************************************************************************/
INT FSP_Server_XPWD(FTP_SERVER *server)
{
    INT status;
    INT index;

    /* Is there any data besides the command? - shouldn't be */
    index = 0;
    /* Scan for a '\r' in replyBuff, copy replyBuff to fileSpec */
    while ((server->replyBuff[index+4]!= '\r') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 4)))
    {
        server->fileSpec[index] = server->replyBuff[index+3];
        index++;
    }

    server->fileSpec[index] = '\0';

    if (index) /* Data present after command */
    {
        /* Send 'Syntax error in parameters' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_SYNTAX_ERROR;
    }
    else /* No data after command, print directory */
    {
        status = FTPS_Server_PrintDir(server);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("XPWD Cmd NU_Send failure.\r\n");
            server->lastError = status;
        }
    }

    return (status);

} /* FSP_Server_XPWD */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_RNFR
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP RNFR
*       command from the client.  This command specifies the old pathname of
*       the file which is to be renamed.  The only parameter is a pointer to a
*       valid FTP server structure.  The character string containing original
*       name of the file to be renamed is saved in the FTP server structure.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        A send or receive command returned an error
*                               while communicating with the host.
*
******************************************************************************/
INT FSP_Server_RNFR(FTP_SERVER *server)
{
    INT status;
    INT index, pass;

    /* If rnfrFlag is set, then RNFR command has been executed */
    if (server->cmdFTP.rnfrFlag)
    {
        server->cmdFTP.rnfrFlag = 0;
        /* Send 'Bad sequence of commands' message */
        status = (INT)NU_Send(server->socketd, MSG503, SIZE503, 0);

        if (status < 0)
        {
            FTP_Printf("FSP_Server_RNFR() NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
    }
    else /* Ready to execute RNFR command */
    {
        /* Parse the directory pathname and filename */
        index = 0;
        pass = 5;

        /* If a path delimiter exists in RNFR and in current directory, */
        if ( (FTPS_IS_PATH_DELIMITER(server->replyBuff[pass])) &&
             (strcmp(server->currentWorkingDir, FTPS_NATIVE_PATH_DELIMITER_STR)
                   == 0) )
            pass = 6;   /* Increment pass variable */

        /* Parse replyBuff for \r */
        while ( (index < (FTPS_GENERIC_BUFF_SIZE - pass)) &&
                (server->replyBuff[index + pass] != '\r') )
        {
            /* Copy replyBuff's fileSpec to server->fileSpec */
            server->fileSpec[index] = server->replyBuff[index + pass];
            index++;
        }

        server->fileSpec[index] = '\0';

        /* If index is negative, no pathname received */
        if (!index)
        {
            /* Send 'Syntax error in parameters' message */
            status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

            if (status < 0)
            {
                FTP_Printf("FSP_Server_RNFR() NU_Send failure.\r\n");
                server->stackError = status;
            }

            status = server->lastError = FTP_SYNTAX_ERROR;
        }
        else
        {
            /* Convert all lower case to upper case in fileSpec */
            server->lastError = FSP_Filename_Parser(server->fileSpec,
                                                    server->path,
                                                    &server->filename);

            server->cmdFTP.rnfrFlag = FTPS_RN_SEQ;

            /* Send 'Requested file action pending' message */
            status = (INT)NU_Send(server->socketd, MSG350, SIZE350, 0);

            if (status < 0)
            {
                FTP_Printf("FSP_Server_RNFR() NU_Send failure.\r\n");
                server->lastError = FTP_STACK_ERROR;
                server->stackError = status;
            }
            else
                status = NU_SUCCESS;
        }
    }

    return (status);

} /* FSP_Server_RNFR */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_RNTO
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP RNTO
*       command from the client.  This command specifies the new pathname of
*       the file specified in RNFR command.The only parameter is a pointer to
*       a valid FTP server structure.  The character string containing new name
*       of the file to be renamed is saved in the FTP server structure.  This
*       function returns status.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        A send or receive command returned an error
*                               while communicating with the host.
*
******************************************************************************/
INT FSP_Server_RNTO(FTP_SERVER *server)
{
    INT      status;
    INT      index, pass;
    INT      srcDrive;
    CHAR     srcPath[FTPS_GENERIC_BUFF_SIZE];
    CHAR     nu_drive[3];

    /* rnfrFlag should be set to FTPS_RN_SEQ before RNTO is called */
    if (server->cmdFTP.rnfrFlag != FTPS_RN_SEQ)
    {
        server->cmdFTP.rnfrFlag = 0;

        /* Send 'Bad sequence of commands' message */
        status = (INT)NU_Send(server->socketd, MSG503, SIZE503, 0);

        if (status < 0)
        {
            FTP_Printf("FSP_Server_RNTO() NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
    }
    else /* RNFR command has executed properly, start RNTO sequence */
    {
        /* Parse the directory pathname and/or filename */
        index = 0;
        pass = 5;

        /* If a path delimiter exists in RNTO and in current directory, */
        if ( (FTPS_IS_PATH_DELIMITER(server->replyBuff[pass])) &&
             (strcmp(server->currentWorkingDir, FTPS_NATIVE_PATH_DELIMITER_STR)
                   == 0) )
            pass = 6;   /* Increment pass variable */

        /* Parse replyBuff for \r */
        while ( (index < (FTPS_GENERIC_BUFF_SIZE - pass)) &&
                (server->replyBuff[index + pass] != '\r') )
        {
            /* Copy replyBuff's fileSpec to server->fileSpec */
            server->fileSpec[index] = server->replyBuff[index + pass];
            index++;
        }

        server->fileSpec[index] = '\0';

        /* Check for drive specification in RNTO */
        if ( (!index || ((server->fileSpec[1] == ':') &&
             (server->fileSpec[2] == '\0'))) )
        {
            /* Send 'Syntax error in parameters' message */
            status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

            if (status < 0)
            {
                FTP_Printf("FSP_Server_RNTO() NU_Send failure.\r\n");
                server->stackError = status;
            }
            status = server->lastError = FTP_SYNTAX_ERROR;
        }
        else /* Rename file */
        {
            index = 0;
            /* this code is added to take care of the rename problem
            rename SUB.SUB 123 */
            if (server->filename != NU_NULL)
            {
                /* Check for dot delimiter in the rename filename */
                while ((server->fileSpec[index] != '\0') &&
                       (index < FTPS_GENERIC_BUFF_SIZE))
                {
                    /* If filename has dot delimiter, exit loop */
                    if (server->fileSpec[index] == '.')
                        break;

                    /* If filename did not have a dot delimiter */
                    else if (server->fileSpec[index+1] == '\0' )
                    {
                        /* Add dot delimiter to end of filename */
                        server->fileSpec[index+1] = '.';
                        server->fileSpec[index+2] = '\0';
                        break;
                    }
                    index++;
                }
            }

            /* Convert all lower case to upper case in fileSpec */
            server->lastError = FSP_Filename_Parser(server->fileSpec,
                                                    server->renamePath,
                                                    &server->renameFile);

            /* Create backup of source path */
            if (server->path[1] == ':')
                srcDrive = server->path[0] - 'A';
            else
                srcDrive = NU_Get_Default_Drive();

            nu_drive[0] = (CHAR)('A' + srcDrive);
            nu_drive[1] = ':';
            nu_drive[2] = '\0';

            NU_Current_Dir ((UINT8*)&nu_drive, srcPath);



            if ( ((FTP_Is_Dir((CHAR *)server->path)) == NU_SUCCESS) &&
                (server->filename == 0) )
            {
                server->cmdFTP.rnfrFlag = FTPS_RN_DIR;
            }
            else
            {
                /* If the directories are different, then this is a move.
                Otherwise, it is a simple file rename. */
                if ( ( (server->path != server->filename) ||
                       (server->renamePath != server->renameFile) ) &&
                     (strcmp(server->path, server->renamePath) != 0) )
                    server->cmdFTP.rnfrFlag = FTPS_RN_MOVE;
                else
                    server->cmdFTP.rnfrFlag = FTPS_RN_FILE;
            }
            /* Perform the actual renaming. */
            if (server->cmdFTP.rnfrFlag)
                status = FTPS_Server_Rename_File(server);
            else
            {
                /* Send 'Syntax error in parameters' message */
                status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

                if (status < 0)
                {
                    FTP_Printf("FSP_Server_RNTO() NU_Send failure.\r\n");
                    server->stackError = status;
                }
                status = server->lastError = FTP_SYNTAX_ERROR;
            }

            /* Reset the flag for a new command. */
            server->cmdFTP.rnfrFlag = 0;

            /* Restore source path */
            if (srcDrive != NU_Get_Default_Drive());
                NU_Set_Default_Drive(srcDrive);

            NU_Set_Current_Dir (srcPath);
        }
    }

    return (status);

} /* FSP_Server_RNTO */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_DELE
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP DELE
*       command from the client. The only parameter is a pointer to a valid
*       FTP server structure.  The character string containing the name
*       of the file to be removed is saved in the FTP server structure.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation completed successfully.
*       FTP_SYNTAX_ERROR        A send or receive command returned an error
*                               while communicating with the host.
*
******************************************************************************/
INT FSP_Server_DELE(FTP_SERVER *server)
{
    INT status;
    INT index;

    /* Parse the name of the file */
    index = 0;
    /* Scan for a '\r' in replyBuff, copy replyBuff to fileSpec */
    while ((server->replyBuff[index+5]!= '\r') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 5)))
    {
        server->fileSpec[index] = server->replyBuff[index+5];
        index++;
    }

    server->fileSpec[index] = '\0';

    /* Convert all lower case to upper case in fileSpec */
    server->lastError = FSP_Filename_Parser(server->fileSpec, server->path,
                                            &server->filename);

    if (index == 0) /* No target specified */
    {
        /* Send 'Syntax error in parameters' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_SYNTAX_ERROR;
    }
    else /* Filename present, delete file */
    {

        status = FTPS_Server_Delete_File(server);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("DELE Cmd FTPS_Server_Delete_File failure.\r\n");
            server->lastError = status;
        }
    }

    return (status);

} /* FSP_Server_DELE */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_SIZE
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP SIZE
*       command from the client. The only parameter is a pointer to a valid
*       FTP server structure.  The FTP server structure contains the
*       character string that contains the name of the file for which the
*       client is requesting the size.
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
*       FTP_INVALID_MODE_CODE   The mode code in server structure is invalid.
*
******************************************************************************/
INT FSP_Server_SIZE(FTP_SERVER *server)
{
    INT status;
    INT index;

    /* Parse the name of the file */
    index = 0;
    /* Scan for a '\r' in replyBuff, copy replyBuff to fileSpec */
    while ((server->replyBuff[index+5]!= '\r') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 5)))
    {
        server->fileSpec[index] = server->replyBuff[index+5];
        index++;
    }

    server->fileSpec[index] = '\0';

    /* Convert all lower case to upper case in fileSpec */
    server->lastError = FSP_Filename_Parser(server->fileSpec, server->path,
                                            &server->filename);

    if (index == 0) /* No target specified */
    {
        /* Send 'Syntax error in parameters' message */
        status = (INT)NU_Send(server->socketd, MSG501, SIZE501, 0);

        if (status < 0)
        {
            FTP_Printf("NU_Send failure.\r\n");
            server->stackError = status;
        }
        status = server->lastError = FTP_SYNTAX_ERROR;
    }
    else /* Filename present, find the Size */
    {

        status = FTPS_Server_Find_File_Size(server);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("Failure to find the size of the file.\r\n");
            server->lastError = status;
        }
    }

    return (status);
} /* FSP_Server_SIZE */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_REST
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP REST
*       command from the client. The only parameter is a pointer to a valid
*       FTP server structure.  This function returns status.
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
INT FSP_Server_REST(FTP_SERVER *server)
{
    INT      status;
    UNSIGNED index;
    CHAR     indexpt[10];

    /* Parse for the restart point in file */
    index = 0;

    /* Scan for a '\r' in replyBuff, copy replyBuff to indexpt */
    while ((server->replyBuff[index+5]!= '\r') &&
           (index < (FTPS_GENERIC_BUFF_SIZE - 5)))
    {
        indexpt[index] = server->replyBuff[index+5];
        index++;
    }

    /* Terminate the indexpt string */
    indexpt[index] = '\0';

    /* Convert the ascii index point to integer */
    index = NU_ATOI(indexpt);

    /* Save the restart point in server structure */
    server->restart = index;

    /* Send 'File action pending further info' message */
    status = (INT)NU_Send(server->socketd, MSG350, SIZE350, 0);

    if (status < 0)
    {
        FTP_Printf("NU_Send failure.\r\n");
        server->lastError = FTP_STACK_ERROR;
        server->stackError = status;
    }
    else
        status = NU_SUCCESS;

    return (status);

} /* FSP_Server_REST */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_FEAT
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP FEAT
*       command from the client. The only parameter is a pointer to a valid
*       FTP server structure.  This function sends a list of FTP extensions
*       supported in Nucleus FTP Server.
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
INT FSP_Server_FEAT(FTP_SERVER *server)
{
    INT      status;

    /* Send a list of Extensions supported in Nucleus FTP server */
    status = (INT)NU_Send(server->socketd, MSG211b, sizeof(MSG211b), 0);

    if (status < 0)
    {
        FTP_Printf("Failed to send FEAT command response.\r\n");
        server->lastError = FTP_STACK_ERROR;
        server->stackError = status;
    }
    else
        status = NU_SUCCESS;

    return (status);

} /* FSP_Server_FEAT */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_NOOP
*
*   DESCRIPTION
*
*       This function provides a primitive to send a response to an FTP NOOP
*       command from the client. The only parameter is a pointer to a valid
*       FTP server structure.  This function returns status.
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
INT FSP_Server_NOOP(FTP_SERVER *server)
{
    INT status;

    /* Send 'Command okay' message */
    status = (INT)NU_Send(server->socketd, MSG200, SIZE200, 0);

    if (status < 0)
    {
        FTP_Printf("NU_Send failure.\r\n");
        server->lastError = FTP_STACK_ERROR;
        server->stackError = status;
    }
    else
        status = NU_SUCCESS;

    return (status);

} /* FSP_Server_NOOP */

/******************************************************************************
*
*   FUNCTION
*
*       FSP_Server_UNKNOWN
*
*   DESCRIPTION
*
*       This function sends an "Unrecognized command" message for all unknown
*       commands received. The only parameter is a pointer to a valid FTP
*       server structure.  This function returns status.
*
*   INPUTS
*
*       server                  pointer to valid FTP server structure
*
*   OUTPUTS
*
*       NU_SUCCESS              The command completed successfully.
*       Nucleus NET specific error code.
*
******************************************************************************/
INT FSP_Server_UNKNOWN(FTP_SERVER *server)
{
    INT status;

    /* Send 'Unrecognized command' message */
    status = (INT)NU_Send(server->socketd, MSG500, SIZE500, 0);

    if (status < 0)
    {
        FTP_Printf("NU_Send failure.\r\n");
        server->lastError = FTP_STACK_ERROR;
        server->stackError = status;
    }
    else
        status = NU_SUCCESS;

    return (status);

} /* FSP_Server_UNKNOWN */
