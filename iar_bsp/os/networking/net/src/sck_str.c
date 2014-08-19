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
*       sck_str.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Send_To_Raw.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Send_To_Raw
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
*       NU_Send_To_Raw
*
*   DESCRIPTION
*
*       This function is responsible for transmitting data across
*       a network during a connectionless IP Raw transfer.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *buff                   Pointer to the data buffer
*       nbytes                  Specifies the number of bytes of data
*       flags                   This parameter is used for socket
*                               compatibility we are currently not making
*                               any use of it
*       *to                     Pointer to the destination's
*                               protocol-specific address structure
*       addrlen                 This parameter is reserved for future use.
*                               A value of zero should be used
*
*   OUTPUTS
*
*       > 0                     The number of bytes transmitted
*       NU_INVALID_PARM         The addr_struct to is NU_NULL
*       NU_INVALID_SOCKET       The socket parameter was not a valid
*                               socket value or it had not been
*                               previously allocated via the NU_Socket
*                               call.
*       NU_NOT_CONNECTED        The socket is not connected.
*       NU_WOULD_BLOCK          The system needs to suspend on a resource,
*                               but the socket is non-blocking.
*       NU_INVALID_ADDRESS      The address passed in was most likely
*                               incomplete (i.e., missing the IP number).
*       NU_NO_PORT_NUMBER       The port number does not exist.
*       NU_DEVICE_DOWN          The device that this socket was
*                               communicating over has gone down. If the
*                               device is a PPP device it is likely the
*                               physical connection has been broken. If
*                               the device is Ethernet and DHCP is being
*                               used, the lease of the IP address may
*                               have expired.
*       NU_NO_DATA_TRANSFER     The data transfer was not completed.
*       Nucleus Status Code
*
*************************************************************************/
INT32 NU_Send_To_Raw(INT socketd, CHAR *buff, UINT16 nbytes, INT16 flags,
                     const struct addr_struct *to, INT16 addrlen)
{
    INT32               return_status;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if ( (to == NU_NULL) || ((to->family != SK_FAM_IP)
#if (INCLUDE_IPV6 == NU_TRUE)
        && (to->family != SK_FAM_IP6)
#endif
        ) )
        return (NU_INVALID_PARM);

#endif

    /* Obtain the semaphore and validate the socket */
    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status == NU_SUCCESS)
    {
        /*  Clean up warnings.  This parameter is used for socket compatibility
         *  but we are currently not making any use of it.
         */
        UNUSED_PARAMETER(flags);
        UNUSED_PARAMETER(addrlen);

        /* Transmit the RAW data */
        return_status = IPRAW_Send_Data(socketd, buff, nbytes, to);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    /* return to caller */
    return (return_status);

} /* NU_Send_To_Raw */
