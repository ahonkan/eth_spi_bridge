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
*       sck_ic.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Is_Connected.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Is_Connected
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
*       NU_Is_Connected
*
*   DESCRIPTION
*
*       This function determines if a connection has been established
*       for the input socket descriptor.
*
*   INPUTS
*
*       socketd                 The socket descriptor for the port to be
*                               checked for connection
*
*   OUTPUTS
*
*      NU_TRUE                  The socket is connected.
*      NU_FALSE                 The socket is not connected.
*      NU_INVALID_SOCKET        The value specified in the socketd
*                               parameter is invalid.
*
*************************************************************************/
STATUS NU_Is_Connected(INT socketd)
{
    INT         pnum;
    TCP_PORT    *prt;
    STATUS      return_status;
    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* check validity of socket descriptor */
    if (socketd < 0 || socketd >= NSOCKETS)
        return(NU_INVALID_SOCKET);

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Get the Nucleus NET semaphore. */
    return_status =  NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (return_status != NU_SUCCESS)
    {
        /* Switch back to the user mode */
        NU_USER_MODE();

        return(return_status);
    }

    /* Initialize the status */
    return_status = NU_TRUE;

    if (SCK_Sockets[socketd] == NU_NULL)
    {
        return_status = NU_FALSE;

        /* Release the TCP semaphore */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* Switch back to user mode. */
        NU_USER_MODE();

        return(return_status);
    }

    /*  Convert the input socket descriptor to a TCP_Ports number. */
    pnum = SCK_Sockets[socketd]->s_port_index;

    if (pnum < 0)
    {
        return_status = NU_FALSE;

        /* Release the TCP semaphore */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* Switch back to user mode. */
        NU_USER_MODE();

        return(return_status);
    }

    /* Get the address to the port. */
    prt = TCP_Ports[pnum];

    if (prt == NU_NULL)
    {
        return_status = NU_FALSE;

        /* Release the TCP semaphore */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* Switch back to user mode. */
        NU_USER_MODE();

        return (return_status);
    }

    /*  Check and see if this port is connected. If it is not
        connected and there are still valid bytes to RX then
        the socket will be considered connected until all valid
        bytes are read by the application. */
    if ( ((prt->state == SREADY) || (prt->state == SCLOSED) ||
          (prt->state >= SFW1)) &&
         (SCK_Sockets[socketd]->s_recvbytes == 0) )
       return_status = NU_FALSE;

    /* Check the socket state if the socket is in the process
       of connecting. */
    if (SCK_Sockets[socketd]->s_state & SS_ISCONNECTING)
        return_status = NU_FALSE;

    /* Release the TCP semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (return_status);

} /* NU_Is_Connected */
