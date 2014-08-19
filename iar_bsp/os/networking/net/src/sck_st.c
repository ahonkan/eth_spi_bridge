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
*       sck_st.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Send_To.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Send_To
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Send_To
*
*   DESCRIPTION
*
*       This function is responsible for transmitting data across
*       a network during a connectionless transfer.
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
*       > 0                     Number of bytes sent.
*       NU_NO_PORT_NUMBER       No local port number was stored in the
*                               socket descriptor.
*       NU_NOT_CONNECTED        If the socket is not connected.
*       NU_INVALID_PARM         The addr_struct to or buffer is NU_NULL
*       NU_INVALID_SOCKET       The socket parameter was not a valid socket
*                               value or it had not been previously allocated
*                               via the NU_Socket call.
*       NU_INVALID_ADDRESS      The address passed in was most likely
*                               incomplete (i.e., missing the IP or port number).
*       NU_NO_DATA_TRANSFER     The data transfer was not completed.
*       NU_DEVICE_DOWN          The device that this socket was communicating
*                               over has gone down. If the device is a PPP
*                               device it is likely the physical connection
*                               has been broken. If the device is ethernet
*                               and DHCP is being used, the lease of the IP
*                               address may have expired.
*       NU_WOULD_BLOCK          The system needs to suspend on a resource, but
*                               the socket is non-blocking.
*
*       An ICMP Error code will be returned if an ICMP packet was
*       received for the socket:
*
*       NU_DEST_UNREACH_ADMIN
*       NU_DEST_UNREACH_ADDRESS
*       NU_DEST_UNREACH_PORT
*       NU_TIME_EXCEED_HOPLIMIT
*       NU_TIME_EXCEED_REASM
*       NU_PARM_PROB_HEADER
*       NU_PARM_PROB_NEXT_HDR
*       NU_PARM_PROB_OPTION
*       NU_DEST_UNREACH_NET
*       NU_DEST_UNREACH_HOST
*       NU_DEST_UNREACH_PROT
*       NU_DEST_UNREACH_FRAG
*       NU_DEST_UNREACH_SRCFAIL
*       NU_PARM_PROB
*       NU_SOURCE_QUENCH
*
*************************************************************************/
INT32 NU_Send_To(INT socketd, CHAR *buff, UINT16 nbytes, INT16 flags,
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

    /* If the buffer is NULL and the number of bytes to send is not zero */
    if ( (buff == NU_NULL) && (nbytes != 0) )
        return (NU_INVALID_PARM);

#endif

    /* Obtain the semaphore and validate the socket */
    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status == NU_SUCCESS)
    {
        /*  Clean up warnings.  This parameter is used for socket compatibility
         *  but we are currently not making any use of it.  */
        UNUSED_PARAMETER(flags);
        UNUSED_PARAMETER(addrlen);

        /* Send the data */
        return_status = UDP_Send_Data(socketd, buff, nbytes, to);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    /* return to caller */
    return (return_status);

} /* NU_Send_To */
