/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ips_sadb_aos.c
*
* COMPONENT
*
*       DATABASE - Security Associations
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Add_Outbound_SA.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Add_Outbound_SA
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_externs.h
*       ips_api.h
*
*************************************************************************/
/* Including the required header files. */
#include "networking/nu_net.h"
#include "networking/ips_externs.h"
#include "networking/ips_api.h"

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Add_Outbound_SA
*
* DESCRIPTION
*
*       This function adds a new SA to the list of outbound SAs.
*
* INPUTS
*
*       *group_name             Name of the group.
*       *sa_entry               The outbound SA to be added.
*       *return_index           Index to be returned.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*
*       NU_TIMEOUT              The operation timed out.
*       NU_NO_MEMORY            There was not enough memory to satisfy
*                               this request.
*       IPSEC_NOT_FOUND         The group was not found or there is no
*                               policy corresponding to the passed index.
*       IPSEC_INVALID_ALGO_ID   Invalid Authentication and Encryption
*                               algorithm specified.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Add_Outbound_SA(CHAR *group_name,
                             IPSEC_OUTBOUND_SA *sa_entry,
                             UINT32 *return_index)
{
    STATUS              status;
    IPSEC_OUTBOUND_SA   *return_sa_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Validating input parameters. */
    if((group_name == NU_NULL) || (sa_entry == NU_NULL) ||
        (return_index == NU_NULL))
    {
        /* Group name is null. */
        status = IPSEC_INVALID_PARAMS;
    }

    else if ((sa_entry->ipsec_select.ipsec_transport_protocol
                                                != IPSEC_WILDCARD) &&
             (sa_entry->ipsec_select.ipsec_transport_protocol
                                                != IP_TCP_PROT) &&
             (sa_entry->ipsec_select.ipsec_transport_protocol
                                                != IP_UDP_PROT) &&
             (sa_entry->ipsec_select.ipsec_transport_protocol
                                                != IP_ICMP_PROT)
#if (INCLUDE_IPV6 == NU_TRUE)
              && (sa_entry->ipsec_select.ipsec_transport_protocol
                                                != IP_ICMPV6_PROT)
#endif
                                                )
    {
            /* Transport protocol is not correct. */
            status = IPSEC_INVALID_PARAMS;
    }

    /* Validate IPsec security protocol. */
    else if (IPSEC_Validate_Sec_Prot(&sa_entry->ipsec_security)
        != NU_SUCCESS)
    {
        status = IPSEC_INVALID_PARAMS;
    }

    /* Validate IPsec selector. */
    else if (IPSEC_Validate_Selector(&sa_entry->ipsec_select)
        != NU_SUCCESS)
    {
        status = IPSEC_INVALID_PARAMS;
    }

    /* Validate authentication key. */
    else if (sa_entry->ipsec_auth_key == NU_NULL)
    {
        status = IPSEC_INVALID_PARAMS;
    }

    /* Validate encryption key. */
    else if ((sa_entry->ipsec_security.ipsec_protocol == IPSEC_ESP) &&
        (sa_entry->ipsec_encryption_key == NU_NULL))
    {
        status = IPSEC_INVALID_PARAMS;
    }

    else
    {
#if (INCLUDE_IKE == NU_TRUE)
        /* Set the lifetime pointers to NULL. These are only
         * used in SAs added by IKE.
         */
        sa_entry->ipsec_hard_lifetime = NU_NULL;
        sa_entry->ipsec_soft_lifetime = NU_NULL;
#endif

        /* Update SA selector's fields if incoming user request
         * is for ICMP protocol requiring Wildcard support. Update
         * the fields to include all possible ICMP message types
         * and codes.
         */
        if( ((sa_entry->ipsec_select.ipsec_transport_protocol == IP_ICMP_PROT)
#if (INCLUDE_IPV6 == NU_TRUE)
          || (sa_entry->ipsec_select.ipsec_transport_protocol == IP_ICMPV6_PROT)
#endif
              ) &&
            ((sa_entry->ipsec_select.ipsec_icmp_msg == IPSEC_WILDCARD) &&
             (sa_entry->ipsec_select.ipsec_icmp_msg_high == IPSEC_WILDCARD) &&
             (sa_entry->ipsec_select.ipsec_icmp_code == IPSEC_WILDCARD) &&
             (sa_entry->ipsec_select.ipsec_icmp_code_high == IPSEC_WILDCARD))
             )
        {
            /* Update the ICMP message type and code values to include all possible
             * values i.e. From 0 to 255. This will enable all types of
             * ICMP messages to pass through selector matching process.
             */
            sa_entry->ipsec_select.ipsec_icmp_msg = 0;
            sa_entry->ipsec_select.ipsec_icmp_msg_high = 255;

            sa_entry->ipsec_select.ipsec_icmp_code = 0;
            sa_entry->ipsec_select.ipsec_icmp_code_high = 255;
        }

        /* First obtain the semaphore before going ahead. */
        status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

        /* Check the status. */
        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain IPsec semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        else
        {
            /* Now call the actual function. */
            status = IPSEC_Add_Outbound_SA_Real(group_name, sa_entry,
                                                &return_sa_ptr);
            if(status == NU_SUCCESS)
            {
                /* ESN not being used. */
                return_sa_ptr->ipsec_seq_num.ipsec_is_esn = NU_FALSE;

                /* Set the index of this SA. */
                *return_index = return_sa_ptr->ipsec_index;
            }

            /* Now everything is done, release the semaphore too. */
            if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IPSEC_Add_Outbound_SA */
