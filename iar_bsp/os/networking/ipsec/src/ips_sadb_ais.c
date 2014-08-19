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
*       ips_sadb_ais.c
*
* COMPONENT
*
*       DATABASE - Security Associations
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Add_Inbound_SA.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Add_Inbound_SA
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
*       IPSEC_Add_Inbound_SA
*
* DESCRIPTION
*
*       This function verifies a new inbound SA to be added to the
*       list of inbound SAs.
*
* INPUTS
*       *group_name             Name of the group.
*       *sa_entry               The inbound SA to be added.
*       *return_spi             Pointer to the callers SPI.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       NU_TIMEOUT              The operation timed out.
*       NU_NO_MEMORY            There was not enough memory to satisfy
*                               this request.
*       NU_INVALID_PARM         This SA already exists.
*       IPSEC_NOT_FOUND         The group was not found or there is no
*                               policy corresponding to the passed index.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Add_Inbound_SA(CHAR *group_name, IPSEC_INBOUND_SA *sa_entry,
                            UINT32 *return_spi)
{
    STATUS              status;
    IPSEC_INBOUND_SA    *return_sa_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Validate input parameters. */
    if((group_name == NU_NULL) || (sa_entry == NU_NULL) ||
        (return_spi == NU_NULL))
    {
        status = IPSEC_INVALID_PARAMS;
    }

    else if((sa_entry->ipsec_security.ipsec_protocol != IPSEC_AH) &&
        (sa_entry->ipsec_security.ipsec_protocol != IPSEC_ESP))
    {
        /* Invalid security protocol. At least one should be present. */
        status = IPSEC_INVALID_PARAMS;
    }

    else if((sa_entry->ipsec_security.ipsec_security_mode
        != IPSEC_TRANSPORT_MODE) &&
        (sa_entry->ipsec_security.ipsec_security_mode
        != IPSEC_TUNNEL_MODE))
    {
        /* Invalid security mode. At least one should be present. */
        status = IPSEC_INVALID_PARAMS;
    }

    /* Make sure the SPI is within a valid range, if specified. */
    else if((sa_entry->ipsec_spi != 0) &&
        ((sa_entry->ipsec_spi < IPSEC_SPI_START) ||
        (sa_entry->ipsec_spi > IPSEC_SPI_END)))
    {
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
        /* Also verify the algorithm ID and replace it with the index of the
           algorithm from the global array of authentication algorithms. */
        status =
            IPSEC_Get_Auth_Algo_Index(&(sa_entry->ipsec_security.
                                      ipsec_auth_algo));

        /* Check the status. */
        if(status == NU_SUCCESS)
        {
            /* Verify the algorithm ID and replace it with the index of
                the algorithm from the global array of encryption algos. */
            status =
                IPSEC_Get_Encrypt_Algo_Index(&sa_entry->ipsec_security.
                                             ipsec_encryption_algo);

            /* Check the status. */
            if(status == NU_SUCCESS)
            {
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

                /* First grab the semaphore. */
                status = NU_Obtain_Semaphore(&IPSEC_Resource,
                                             IPSEC_SEM_TIMEOUT);
                /* Check the status. */
                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to obtain IPsec semaphore",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
                else
                {
                    /* Now call the actual function. */
                    status = IPSEC_Add_Inbound_SA_Real(group_name,
                                                       sa_entry,
                                                       &return_sa_ptr);
                    if(status == NU_SUCCESS)
                    {
                        /* Set SPI to be returned to caller. */
                        *return_spi = return_sa_ptr->ipsec_spi;
                    }

                    /* Release the semaphore. */
                    if(NU_Release_Semaphore(&IPSEC_Resource)!= NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to release the semaphore",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IPSEC_Add_Inbound_SA */
