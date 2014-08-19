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
*       fc.c                                         1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client/Server
*       Common functions.
*
*   DESCRIPTION
*
*       This file contains functions that can be called by both the
*       client and the server. It helps in consolidating duplicate
*       code.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FC_StringToAddr
*       FC_AddrToString
*       FC_Parse_Extended_Command
*       FC_Store_Addr
*       NU_Get_Sock_Name
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_net.h
*       fc_defs.h
*       fc_extr.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/nu_net.h"
#include "networking/nu_ftp.h"

/******************************************************************************
*
*  FUNCTION
*
*      FC_StringToAddr
*
*  DESCRIPTION
*
*      Converts an ASCII sequence of numbers -- representing the common port
*      number as ip1,ip2,ip3,ip4,p1,p2 -- into their respective types as
*      defined in a addr_struct.
*
*  INPUTS
*
*      addr                     reference pointer to valid destination
*                               addr_struct.
*      string                   pointer to a string buffer. Null-termination
*                               not necessary.
*
*  OUTPUTS
*
*      status                   Either NU_SUCCESS or FTP_SYNTAX_ERROR.
*
*      addr                     modified to contain the new port only if
*                               status == NU_SUCCESS.
*
******************************************************************************/
INT FC_StringToAddr(struct addr_struct *addr, CHAR *string)
{
    INT     i;
    INT     status = NU_SUCCESS;
    INT     portNum;
    UINT8   ipbuff[6];

    /* Initialize ipbuff to prevent warnings. */
    ipbuff[0] = 0;

    /* Verify that we have 6 numbers in the range [0..255]. */
    for (i = 0; i < 6; i++)
    {
        /* Each number is read into an INT so their values can be verified. */
        portNum = NU_ATOI(string);

        if ( (portNum < 0) || (portNum > 255) )
        {
            status = FTP_SYNTAX_ERROR;
            break;
        }

        /* Save the number until all numbers are verified. */
        ipbuff[i] = (UINT8)portNum;

        /* Move pointer to the next ip/port number. */
        while ( (*string >= '0') && (*string <= '9') )
            string++;

        string++;
    }

    /* If we get 6 good address bytes, then copy them into the structure. */
    if (i == 6)
    {
        /* Copy the IP address field. */
        memcpy(addr->id.is_ip_addrs, ipbuff, 4);

        /* Copy the port bytes into the 16-bit port field. */
        addr->port = ipbuff[4];
        addr->port = (UINT16)((addr->port * 256) + ipbuff[5]);

        addr->family = NU_FAMILY_IP;

        status = NU_SUCCESS;
    }

    return (status);

} /* FC_StringToAddr */

/******************************************************************************
*
*  FUNCTION
*
*      FC_AddrToString
*
*  DESCRIPTION
*
*      Converts an IP address and port number, taken from a addr_struct,
*      argument, into a standard ASCII string of the form ip1,ip2,ip3,ip4,p1,p2.
*
*  INPUTS
*
*      addr                     reference pointer to valid source addr_struct.
*      string                   pointer to a pre-allocated string buffer.
*
*  OUTPUTS
*
*      status                   NU_SUCCESS.
*      string                   modified to contain the new ASCII string.
*
******************************************************************************/
INT FC_AddrToString(struct addr_struct *addr, CHAR *string)
{
    INT     i, j;
    INT     portNum;
    INT     status = NU_SUCCESS;

    /* If the address is an IPv4-Mapped address, extract the IPv4 portion
     * of the address.
     */
#if ( (defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE) )
    if ( (addr->family == NU_FAMILY_IP6) &&
         (IPV6_IS_ADDR_V4MAPPED(addr->id.is_ip_addrs)) )
    {
        i = 12;
        j = 16;
    }
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        i = 0;
        j = 4;
    }
#endif

    /* Convert the array of IP numbers into ASCII, directly in the
       string buffer. */
    while (i < j)
    {
        NU_ITOA((INT)addr->id.is_ip_addrs[i], string, 10);

        /* Place the pointer at the end and add the comma. */
        string += strlen(string);
        *string = ',';
        string++;
        i++;
    }

    /* Convert the MSB of the port number into ASCII. */
    portNum = (UINT8)(addr->port >> 8);
    NU_ITOA(portNum, string, 10);
    string += strlen(string);
    *string = ',';
    string++;

    /* Convert the LSB of the port number into ASCII. */
    portNum = (UINT8)(addr->port & 0xFF);
    NU_ITOA(portNum, string, 10);

    /* Null terminate the string. */
    string += strlen(string);
    *(string + 1) = '\0';

    return (status);

} /* FC_AddrToString */

/*************************************************************************
*
*   FUNCTION
*
*       FC_Parse_Extended_Command
*
*   DESCRIPTION
*
*       This function parses the family, IP address and foreign port
*       number from the response to an EPRT or EPSV command.
*
*   INPUTS
*
*       buffer                  A pointer to the buffer containing the
*                               response to parse.
*       **family                A pointer to a pointer to set to the
*                               beginning of the family type in the
*                               buffer.
*       **ip_addr               A pointer to a pointer to set to the
*                               beginning of the IP address in the
*                               buffer.
*       **port                  A pointer to a pointer to set to the
*                               beginning of the port in the buffer.
*       delimiter               The character to use as the delimiter.
*       buff_size               The maximum size of the buffer.
*
*   OUTPUTS
*
*       NU_SUCCESS              The response buffer was successfully
*                               parsed.
*       -1                      There is an error in the response.
*
*************************************************************************/
INT FC_Parse_Extended_Command(CHAR *buffer, CHAR **family, CHAR **ip_addr,
                              CHAR **port, CHAR delimiter, INT buff_size)
{
    INT     delimits = 0;
    INT     i = 0;

    /* Parse out the family, IP address and port numbers - be careful not to
     * get into an infinite loop
     */
    while ( (delimits < 4) && (i < buff_size) )
    {
        /* If a delimiter was found, save off a pointer to one of the
         * fields.
         */
        if (buffer[i] == delimiter)
        {
            /* NULL terminate the string so ATOI can be used */
            buffer[i] = '\0';

            /* Increment the number of delimiters found */
            delimits ++;

            /* If this is the first delimiter, the next character is
             * the start of the family.
             */
            if (delimits == 1)
                *family = (CHAR*)(&(buffer[i+1]));

            /* If this is the second delimiter, the next character is
             * the start of the IP address.
             */
            else if (delimits == 2)
                *ip_addr = (CHAR*)(&(buffer[i+1]));

            /* If this is the third delimiter, the next character is
             * the start of the foreign data port number.
             */
            else if (delimits == 3)
                *port = (CHAR*)(&(buffer[i+1]));
        }

        /* Move ahead in the data buffer */
        i++;
    }

    if (i < buff_size)
        return (NU_SUCCESS);
    else
        return (-1);

} /* FC_Parse_Extended_Command */

/*************************************************************************
*
*   FUNCTION
*
*       FC_Store_Addr
*
*   DESCRIPTION
*
*       This function stores an IP address in ASCII format in the buffer
*       provided, separating each byte with the delimiter specified.
*
*   INPUTS
*
*       *servaddr               A pointer to the IP address to store in
*                               the buffer.
*       *buffer                 A pointer to the buffer into which to
*                               store the ASCII form of the IP address.
*       delimiter               The character to use between each byte
*                               of the IP address.
*
*   OUTPUTS
*
*       The number of bytes stored.
*
*************************************************************************/
INT FC_Store_Addr(struct addr_struct *servaddr, CHAR *buffer,
                  CHAR delimiter)
{
    INT index = 0;

    NU_ITOA((INT)servaddr->id.is_ip_addrs[0], buffer, 10);

    while (buffer[index])
        index++;

    buffer[index] = delimiter;
    index++;
    NU_ITOA((INT)servaddr->id.is_ip_addrs[1], &buffer[index], 10);

    while (buffer[index])
        index++;

    buffer[index] = delimiter;
    index++;
    NU_ITOA((INT)servaddr->id.is_ip_addrs[2], &buffer[index], 10);

    while (buffer[index])
        index++;

    buffer[index] = delimiter;
    index++;
    NU_ITOA((INT)servaddr->id.is_ip_addrs[3], &buffer[index], 10);

    while (buffer[index])
        index++;

    return (index);
} /* FC_Store_Addr */

/************************************************************************
*
*  FUNCTION
*
*      FTP_Handle_File_Length
*
*  DESCRIPTION
*
*      Returns the length of the file associated with the file
*      handle.
*
*  INPUTS
*
*      file_desc                   File descriptor of the file.
*
*  OUTPUTS
*
*      The Length of the File
*
************************************************************************/

UINT32 FTP_Handle_File_Length(INT file_desc)
{
    UINT32      data_length = 0;
    INT32       original_location;

    /* Save off the current location of the file pointer - seek 0 bytes
     * from the current position.
     */
    original_location = NU_Seek(file_desc, 0, PSEEK_CUR);

    if (original_location >= 0)
    {
        /* Get the end location of the file pointer - seek to the end of
         * the file.
         */ 
        data_length = (UINT32)NU_Seek(file_desc, 0, PSEEK_END);

        /* Restore the original position of the file pointer - seek 
         * original_location bytes from the beginning of the file.
         */
        NU_Seek(file_desc, original_location, PSEEK_SET);
    }

    return (data_length);
}

#if (NET_VERSION_COMP <= NET_4_3)
/*************************************************************************
*
*   FUNCTION
*
*       NU_Get_Sock_Name
*
*   DESCRIPTION
*
*       This function returns the local endpoint for the specified
*       socket.
*
*   INPUTS
*
*       socketd
*       *localaddr
*       *addr_length
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_PARM
*       NU_BAD_SOCKETD
*       NU_NOT_CONNECTED
*
*************************************************************************/
STATUS NU_Get_Sock_Name(INT socketd, struct sockaddr_struct *localaddr,
                        INT16 *addr_length)
{
    INT16 return_status = NU_SUCCESS;           /* initialized to success */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    if (*addr_length < (INT16)sizeof(struct sockaddr_struct))
    {
        NU_USER_MODE();    /* Switch back to user mode. */
        return (NU_INVALID_PARM);
    }


    /* check validity of socket descriptor */
    if (socketd < 0 || socketd > NSOCKETS)
         /* return an error status */
         return_status = NU_BAD_SOCKETD;
    else
    {
        /* store the local endpoint for return to the caller */
        memcpy(&localaddr->ip_num, &SCK_Sockets[socketd]->s_local_addr.ip_num, 4);
        memcpy(&localaddr->port_num, &SCK_Sockets[socketd]->s_local_addr.port_num, 2);

        /* store the length of the endpoint structure for return to the
          caller */
        *addr_length = sizeof(struct sockaddr_struct);
    }

    NU_USER_MODE();    /* Switch back to user mode. */

    /* return to caller */
    return (return_status);

} /* NU_Get_Sock_Name */

#endif
