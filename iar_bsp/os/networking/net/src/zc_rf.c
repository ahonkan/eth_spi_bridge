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
*
*   FILE NAME
*
*       zc_rf.c
*
*   DESCRIPTION
*
*       This file contains the implementation of the function
*       NU_ZC_Recv_From.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_ZC_Recv_From
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_ZC_Recv_From
*
*   DESCRIPTION
*
*       This function is responsible for receiving data across a network
*       during a connectionless transfer using the ZEROCOPY mode.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       **buff                  Pointer to the data buffer
*       nbytes                  Specifies the number of bytes of data
*       flags                   Used for socket compatibility we are
*                               currently not making any use of it
*       *from                   Pointer to the source protocol-specific
*                               address structure
*       *addrlen                This parameter is reserved for future use.
*                               A value of zero should be used
*
*   OUTPUTS
*
*       > 0                     Number of bytes received.
*       NU_INVALID_PARM         The addr_struct from is NU_NULL.
*       NU_NO_PORT_NUMBER       No local port number was stored in the
*                               socket descriptor.
*       NU_INVALID_SOCKET       The socket parameter was not a valid socket
*                               value or it had not been previously allocated
*                               via the NU_Socket call.
*       NU_NOT_CONNECTED        The socket is not connected.
*       NU_WOULD_BLOCK          No data is available, and the socket is
*                               non-blocking.
*       NU_NO_DATA_TRANSFER     The data transfer was not completed.
*       NU_DEVICE_DOWN          The device that this socket was communicating
*                               over has gone down. If the device is a PPP
*                               device it is likely the physical connection
*                               has been broken. If the device is ethernet
*                               and DHCP is being used, the lease of the IP
*                               address may have expired.
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
INT32 NU_ZC_Recv_From(INT socketd, NET_BUFFER **buff, UINT16 nbytes,
                      INT16 flags, struct addr_struct *from, INT16 *lenptr)
{
    INT32       bytes_recv;
    UINT32      addr;
    OPTION   old_preempt;    /* indicates preemption state */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /*  Validate the socket number.  */
    if ( (socketd < 0) || (socketd >= NSOCKETS) )
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (NU_INVALID_SOCKET);
    }

    /* Disable preemption before accessing socket list. */
    old_preempt = NU_Change_Preemption(NU_NO_PREEMPT);

    if (SCK_Sockets[socketd] == NU_NULL)
    {
        /* Restore preemption */
        NU_Change_Preemption(old_preempt);

        /* Return to user mode */
        NU_USER_MODE();

        return (NU_NOT_CONNECTED);
    }

    /* Validate ZEROCOPY mode on this socket */
    if (!(SCK_Sockets[socketd]->s_flags & SF_ZC_MODE))
    {
        /* Restore preemption */
        NU_Change_Preemption(old_preempt);

        /* Return to user mode */
        NU_USER_MODE();

        return (NU_INVALID_PARM);
    }

    /* Restore preemption */
    NU_Change_Preemption(old_preempt);

    /* Return to user mode */
    NU_USER_MODE();

    bytes_recv = NU_Recv_From(socketd, (CHAR *)&addr, nbytes, flags, from,
        lenptr);

    /* Return the RX buffer memory address. */
    *buff = (NET_BUFFER *)addr;

    return (bytes_recv);

} /* NU_ZC_Recv_From */
