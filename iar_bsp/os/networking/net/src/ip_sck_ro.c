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
*       ip_sck_ro.c
*
* DESCRIPTION
*
*       This file contains the implementation of IP_Set_Raw_Opt.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       IP_Set_Raw_Opt
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/***********************************************************************
*
*   FUNCTION
*
*       IP_Set_Raw_Opt
*
*   DESCRIPTION
*
*       Sets the IP Raw option buffer to the socket.
*
*   INPUTS
*
*       socketd                 Socket descriptor.
*       *optval                 A pointer to the option value
*       optlen                  The option length
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation
*       NU_INVAL                Failure
*
*************************************************************************/
STATUS IP_Set_Raw_Opt(INT socketd, const VOID *optval, INT optlen)
{
    /* Was a value specified and an appropriate size specified. */
    if ( (optval == NU_NULL) || (optlen  < (INT)(sizeof(INT16))) )
        return (NU_INVAL);

    IP_Setsockopt_IP_HDRINCL(socketd, *(INT16*)optval);

    return (NU_SUCCESS);

} /* IP_Set_Raw_Opt */
