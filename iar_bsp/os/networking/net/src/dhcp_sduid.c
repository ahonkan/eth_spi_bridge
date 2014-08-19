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
*       dhcp_sduid.c
*
*   DESCRIPTION
*
*       This file contains the API routine to set the DUID for the client
*       at run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Set_DHCP_DUID
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
*       NU_Set_DHCP_DUID
*
*   DESCRIPTION
*
*       This routine sets the DUID for the node to be used in DHCP
*       transactions for both IPv4 and IPv6.
*
*   INPUTS
*
*       *duid_ptr               A pointer to the DHCP_DUID_STRUCT
*                               containing the new DUID parameters.
*                               duid_type is set to the type of DUID
*                               being set; either DHCP_DUID_EN or
*                               DHCP_DUID_LL.  If using DHCP_DUID_EN,
*                               duid_ent_no must be set to the enterprise
*                               number, duid_id must be set to the ID,
*                               and duid_id_no_len must be set to the
*                               length of the ID.  If using DHCP_DUID_LL,
*                               duid_hw_type must be set to the hardware
*                               type and duid_ll_addr must be set to the
*                               6-byte MAC address.
*
*   OUTPUTS
*
*       NU_SUCCESS              The DUID was successfully set.
*       NU_INVALID_PARM         The DUID type is not supported.
*
*************************************************************************/
STATUS NU_Set_DHCP_DUID(const DHCP_DUID_STRUCT *duid_ptr)
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
        if (duid_ptr->duid_type == DHCP_DUID_EN)
        {
            /* Ensure the new DUID ID number length is not longer than
             * the max.
             */
            if (duid_ptr->duid_id_no_len <= DHCP_DUID_MAX_ID_NO_LEN)
            {
                /* Set the type. */
                DHCP_Duid.duid_type = duid_ptr->duid_type;

                /* Set the enterprise number of the DUID. */
                DHCP_Duid.duid_ent_no = duid_ptr->duid_ent_no;

                /* Set the identifier of the DUID. */
                NU_BLOCK_COPY(DHCP_Duid.duid_id, duid_ptr->duid_id,
                            duid_ptr->duid_id_no_len);

                /* Set the length of the ID number. */
                DHCP_Duid.duid_id_no_len = duid_ptr->duid_id_no_len;
            }

            else
            {
                status = NU_INVALID_PARM;

                NLOG_Error_Log("DUID length too long", NERR_INFORMATIONAL,
                               __FILE__, __LINE__);
            }
        }

        /* If the DUID is of type DUID-LL. */
        else if (duid_ptr->duid_type == DHCP_DUID_LL)
        {
            /* Set the type. */
            DHCP_Duid.duid_type = duid_ptr->duid_type;

            /* Set the hardware type of the DUID. */
            DHCP_Duid.duid_hw_type = duid_ptr->duid_hw_type;

            /* Copy the new link-layer address. */
            memcpy(DHCP_Duid.duid_ll_addr, duid_ptr->duid_ll_addr, DADDLEN);
        }

        else
        {
            status = NU_INVALID_PARM;

            NLOG_Error_Log("DUID type not support", NERR_INFORMATIONAL,
                           __FILE__, __LINE__);
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

} /* NU_Set_DHCP_DUID */
