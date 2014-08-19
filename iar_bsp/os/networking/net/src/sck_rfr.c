/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       sck_rfr.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Recv_From_Raw.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Recv_From_Raw
*
* DEPENDENCIES
*
*       nu_net.h
*       ipraw.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/ipraw.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Recv_From_Raw
*
*   DESCRIPTION
*
*       This function is responsible for receiving data across a network
*       during a connectionless IP Raw transfer.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *buff                   Pointer to the data buffer
*       nbytes                  Specifies the maximum number of bytes
*                               of data the application can receive
*       flags                   This parameter is used for socket
*                               compatibility but we are currently not
*                               making any use of it
*       *from                   Pointer to the source protocol-specific
*                               address structure
*       *addrlen                This parameter is reserved for future use.
*                               A value of zero should be used
*
*   OUTPUTS
*
*       > 0                     The number of bytes received.
*       NU_INVALID_PARM         The addr_struct from is NU_NULL.
*       NU_INVALID_SOCKET       The socket parameter was not a valid socket
*                               value or it had not been previously
*                               allocated via the NU_Socket call.
*       NU_NOT_CONNECTED        The socket is not connected.
*       NU_WOULD_BLOCK          No data is available, and the socket is
*                               non-blocking.
*       NU_NO_PORT_NUMBER       No port number.
*       NU_DEVICE_DOWN          The device that this socket was
*                               communicating over has gone down. If the
*                               device is a PPP device it is likely the
*                               physical connection has been broken. If
*                               the device is ethernet and DHCP is being
*                               used, the lease of the IP address may
*                               have expired.
*       NU_NO_DATA_TRANSFER     The data transfer was not completed.
*
*************************************************************************/
INT32 NU_Recv_From_Raw(INT socketd, CHAR *buff, UINT16 nbytes, INT16 flags,
                       struct addr_struct *from, INT16 *addrlen)
{
    INT32               return_status;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if (from == NU_NULL)
        return (NU_INVALID_PARM);

#endif

    /* Obtain the semaphore and validate the socket */
    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status == NU_SUCCESS)
    {
        /* Clean up warnings.  This parameter is used for socket compatibility
         * but we are currently not making any use of it.
         */
        UNUSED_PARAMETER(flags);
        UNUSED_PARAMETER(addrlen);

        /* Call the routine to receive the data pending on the socket */
        return_status = IPRAW_Recv_Data(socketd, buff, nbytes, from);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    /* return to caller */
    return (return_status);

} /* NU_Recv_From_Raw */
