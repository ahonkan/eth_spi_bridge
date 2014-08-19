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
*   FILENAME
*
*       dhcp_gduid.c
*
*   DESCRIPTION
*
*       This file contains the API routine to get the DUID for the client
*       at run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Get_DHCP_DUID
*
*   DEPENDENCIES
*
*       nu_net.h
*
**************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Get_DHCP_DUID
*
*   DESCRIPTION
*
*       This routine gets the DUID for the node that is used in DHCP
*       transactions for both IPv4 and IPv6.
*
*   INPUTS
*
*       *duid_ptr               A pointer to the DHCP_DUID_STRUCT
*                               that will be filled in with the current
*                               DUID values.
*
*   OUTPUTS
*
*       NU_SUCCESS              duid_ptr contains the current DUID
*                               settings for the node.
*
*************************************************************************/
STATUS NU_Get_DHCP_DUID(DHCP_DUID_STRUCT *duid_ptr)
{
    STATUS          status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Obtain the semaphore */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* If the DUID is of type DUID-EN. */
        if (DHCP_Duid.duid_type == DHCP_DUID_EN)
        {
            /* Fill in the type. */
            duid_ptr->duid_type = DHCP_Duid.duid_type;

            /* Fill in the enterprise number of the DUID. */
            duid_ptr->duid_ent_no = DHCP_Duid.duid_ent_no;

            /* Fill in the identifier of the DUID. */
            NU_BLOCK_COPY(duid_ptr->duid_id, DHCP_Duid.duid_id,
                          DHCP_Duid.duid_id_no_len);

            /* Fill in the length of the ID number. */
            duid_ptr->duid_id_no_len = DHCP_Duid.duid_id_no_len;
        }

        /* If the DUID is of type DUID-LL. */
        else
        {
            /* Fill in the type. */
            duid_ptr->duid_type = DHCP_Duid.duid_type;

            /* Fill in the hardware type of the DUID. */
            duid_ptr->duid_hw_type = DHCP_Duid.duid_hw_type;

            /* Fill in the new link-layer address. */
            memcpy(duid_ptr->duid_ll_addr, DHCP_Duid.duid_ll_addr, DADDLEN);
        }

        /* Release the semaphore */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Get_DHCP_DUID */
