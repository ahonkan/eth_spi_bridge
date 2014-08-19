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
*       tcp_pu.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Push.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Push
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
*       NU_Push
*
*   DESCRIPTION
*
*       OBSOLETE FUNCTION
*
*       This functions sets the push bit for a given output port. This
*       function is obsolete, and remains here only for compatibility
*       reasons. NU_Setsockopt should be used in favor of this function.
*
*   INPUTS
*
*       socketd                 A socket descriptor
*
*   OUTPUTS
*
*       Nucleus Status Code
*
*************************************************************************/
STATUS NU_Push(INT socketd)
{
    UINT8   set = 1;

    return (NU_Setsockopt_TCP_NODELAY(socketd, set));

} /* NU_Push */
