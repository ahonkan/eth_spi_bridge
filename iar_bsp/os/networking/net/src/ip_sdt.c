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
*       ip_sdt.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Set_Default_TTL.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Set_Default_TTL
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/************************************************************************
*
*   FUNCTION
*
*       NU_Set_Default_TTL
*
*   DESCRIPTION
*
*       This function sets the IP Default Time To Live variable.
*
*   INPUTS
*
*       default_ttl             The default time to live
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS NU_Set_Default_TTL(UINT8 default_ttl)
{
    STATUS      status;
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NU_USER_MODE();
        return (status);
    }

    IP_Time_To_Live = default_ttl;

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    NU_USER_MODE();

    return (NU_SUCCESS);

} /* NU_Set_Default_TTL */
