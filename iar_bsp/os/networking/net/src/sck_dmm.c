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
*       sck_dmm.c
*
*   DESCRIPTION
*
*       This file contains the routine to leave a multicast group.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_IP_DROP_MEMBERSHIP
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
*       NU_Setsockopt_IP_DROP_MEMBERSHIP
*
*   DESCRIPTION
*
*       This function leaves a multicast group.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *mreq_in                A pointer to the multicast parameters.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_MEM_ALLOC            Insufficient memory.
*       NU_INVAL                Invalid parameter.
*
*************************************************************************/
STATUS NU_Setsockopt_IP_DROP_MEMBERSHIP(INT socketd, IP_MREQ *mreq_in)
{
    STATUS  status;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if (mreq_in == NU_NULL)
        return (NU_INVAL);

#endif

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        status = IP_Process_Multicast_Listen(socketd,
                                             (UINT8 *)&mreq_in->sck_inaddr,
                                             (UINT8 *)&mreq_in->sck_multiaddr,
                                             MULTICAST_FILTER_INCLUDE,
                                             NU_NULL, 0);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_IP_DROP_MEMBERSHIP */
