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
*       zc_re.c
*
*   DESCRIPTION
*
*       This file contains the implementation of the function NU_ZC_Recv.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_ZC_Recv
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
*       NU_ZC_Recv
*
*   DESCRIPTION
*
*       This auction will handle receiving data across a network during a
*       connection oriented transfer. Used only in ZEROCOPY operations.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       **buff                  Pointer to a pointer to the Net Buffer.
*       nbytes                  Specifies the max number of bytes of data
*       flags                   This parameter is used for socket compatibility
*                               but we are currently not making any use of it
*
*   OUTPUTS
*
*       Number of bytes received.
*       NU_NO_PORT_NUMBER       No local port number was stored in the
*                               socket descriptor.
*       NU_INVALID_SOCKET       The socket parameter was not a valid socket
*                               value or it had not been previously
*                               allocated via the NU_Socket call.
*       NU_NOT_CONNECTED        The connection is broken for some reason.
*                               Stop using the socket; it is best to close it.
*       NU_WOULD_BLOCK          No data is available, and the socket is
*                               non-blocking.
*       NU_NO_ROUTE_TO_HOST     This is an icmp_error if no route to host exist
*       NU_CONNECTION_REFUSED   This is an icmp_error if the connection is refused.
*       NU_MSG_TOO_LONG         This is an icmp_error if the message is too large.
*       NU_CONNECTION_TIMED_OUT TCP Keep-Alive packets found that the
*                               connection has timed out.
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
INT32 NU_ZC_Recv(INT socketd, NET_BUFFER **buff, UINT16 nbytes, INT16 flags)
{
    INT32       bytes_recv;   /* number of bytes read */
    UINT32      addr;         /* hold a 32 bit address */
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

    bytes_recv = NU_Recv(socketd, (CHAR *)&addr, nbytes, flags);

    /* set the address to return to caller */
    *buff = (NET_BUFFER *)addr;

    return (bytes_recv);

} /* NU_ZC_Recv */
