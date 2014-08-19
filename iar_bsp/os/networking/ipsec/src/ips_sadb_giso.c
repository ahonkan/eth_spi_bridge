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
*       ips_sadb_giso.c
*
* COMPONENT
*
*       DATABASE - Security Associations
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Get_Inbound_SA_Opt.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Get_Inbound_SA_Opt
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
*       IPSEC_Get_Inbound_SA_Opt
*
* DESCRIPTION
*
*       This function returns an inbound SA value as specified by optname.
*
* INPUTS
*
*       *group_name             Pointer to IPsec group name.
*       *index                  Pointer to the index of the SA.
*       optname                 Specifies an option.
*       *optval                 Pointer to a location where
*                               the value will be placed on a
*                               successful request
*       *optlen                 Length of the option values.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       NU_TIMEOUT              The operation timed out.
*       IPSEC_NOT_FOUND         The group was not found or there is no
*                               policy corresponding to the passed index.
*       IPSEC_LENGTH_IS_SHORT   Indicates that the value to be returned
*                               required more memory than was passed.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Inbound_SA_Opt(CHAR *group_name,
                                IPSEC_INBOUND_INDEX *index, INT optname,
                                VOID *optval, INT *optlen)
{
    STATUS              status;
    UINT8               address_size;
    UINT16              auth_key_len;
    UINT16              encrypt_key_len;
    IPSEC_INBOUND_SA    *sa_ptr;
    IPSEC_POLICY_GROUP  *group_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check the parameters passed. */
    if((index == NU_NULL) || (group_name == NU_NULL) ||
                             (index->ipsec_spi == 0))
    {
        /* Some invalid parameter has been passed. */
        status = IPSEC_INVALID_PARAMS;
    }
    /* Check the parameters further. */
    else if((optname != IPSEC_IS_SA) &&
          ((optval == NU_NULL) || (optlen == NU_NULL)))
    {
        /* Some invalid parameter has been passed. */
        status = IPSEC_INVALID_PARAMS;
    }
    else
    {
        /* First grab the semaphore. */
        status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

        /* Check the status. */
        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain IPsec semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        else
        {
            /* First get the group entry. */
            status = IPSEC_Get_Group_Entry(group_name, &group_ptr);

            /* Check the status value. */
            if(status == NU_SUCCESS)
            {
                /* First get the required SA first. */
                status = IPSEC_Get_Inbound_SA(group_ptr, index, &sa_ptr);

                /* Check the status. */
                if(status == NU_SUCCESS)
                {
                    /* Decode the request. */
                    switch(optname)
                    {
                    case IPSEC_SELECT:
                    {
                        /* Make sure that there is room for the values to
                           be returned. */
                        if(*optlen < sizeof(sa_ptr->ipsec_select))
                        {
                            /* Set the error code. */
                            status = IPSEC_LENGTH_IS_SHORT;
                        }
                        else
                        {
                            /* Now copy the requested values. */
                            NU_BLOCK_COPY(optval, &(sa_ptr->ipsec_select),
                                            sizeof(sa_ptr->ipsec_select));
                        }

                        /* Return the size of the required structure. */
                        *optlen = sizeof(sa_ptr->ipsec_select);
                    }
                    break;

                    case IPSEC_SECURITY:
                    {
                        /* Make sure that there is room for the values to
                        be returned. */
                        if(*optlen < sizeof(sa_ptr->ipsec_security))
                        {
                            /* Set the error code. */
                            status = IPSEC_LENGTH_IS_SHORT;
                        }
                        else
                        {
                            /* Now copy the requested values. */
                            NU_BLOCK_COPY(optval,
                                            &(sa_ptr->ipsec_security),
                                       sizeof(sa_ptr->ipsec_security));
                        }

                        /* Return the size of the structure required. */
                        *optlen = sizeof(sa_ptr->ipsec_security);
                    }
                    break;

/* Only if the IKE protocol is enabled. */
#if (INCLUDE_IKE == NU_TRUE)

                    case IPSEC_SOFT_LIFETIME:
                    {
                        /* Make sure that there is room for the values to
                        be returned. */
                        if(*optlen < sizeof(sa_ptr->ipsec_soft_lifetime))
                        {
                            /* Set the error code. */
                            status = IPSEC_LENGTH_IS_SHORT;
                        }
                        else
                        {
                            /* Now copy the requested values. */
                            NU_BLOCK_COPY(optval,
                                        &(sa_ptr->ipsec_soft_lifetime),
                                sizeof(sa_ptr->ipsec_soft_lifetime));
                        }

                        /* Return the size of the required structure. */
                        *optlen = sizeof(sa_ptr->ipsec_soft_lifetime);
                    }
                    break;

                    case IPSEC_HARD_LIFETIME:
                    {
                        /* Make sure that there is room for the values to
                        be returned. */
                        if(*optlen < sizeof(sa_ptr->ipsec_hard_lifetime))
                        {
                            /* Set the error code. */
                            status = IPSEC_LENGTH_IS_SHORT;
                        }
                        else
                        {
                            /* Now copy the requested values. */
                            NU_BLOCK_COPY(optval,
                                        &(sa_ptr->ipsec_hard_lifetime),
                                    sizeof(sa_ptr->ipsec_hard_lifetime));
                        }

                        /* Return the size of the structure required. */
                        *optlen = sizeof(sa_ptr->ipsec_hard_lifetime);

                    }
                    break;
#endif
/* End of #if (INCLUDE_IKE == NU_TRUE). */

                    case IPSEC_AUTH_KEY:
                    {
                        /* Get authentication algorithm key length. */
                        auth_key_len = IPSEC_GET_AUTH_KEY_LEN(sa_ptr->
                                        ipsec_security.ipsec_auth_algo);

                        /* Make sure that there is room for the values to
                           be returned. */
                        if(*optlen < (INT)auth_key_len)
                        {
                            /* Set the error code. */
                            status = IPSEC_LENGTH_IS_SHORT;
                        }
                        else
                        {
                            /* Now copy the requested values. */
                            NU_BLOCK_COPY(optval, sa_ptr->ipsec_auth_key,
                                          auth_key_len);
                        }

                        /* Return the size of the structure required. */
                        *optlen = auth_key_len;

                    }
                    break;

                    case IPSEC_ENCRYPTION_KEY:
                    {
                        /* Get authentication algorithm key length. */
                        encrypt_key_len = IPSEC_GET_ENCRYPT_KEY_LEN(
                            sa_ptr->ipsec_security.ipsec_encryption_algo);

                        /* Make sure that there is room for the values to
                           be returned. */
                        if(*optlen < (INT)encrypt_key_len)
                        {
                            /* Set the error code. */
                            status = IPSEC_LENGTH_IS_SHORT;
                        }
                        else
                        {
                            /* Now copy the requested values. */
                            NU_BLOCK_COPY(optval,
                                        sa_ptr->ipsec_encryption_key,
                                        encrypt_key_len);
                        }

                        /* Return the size of the structure required. */
                        *optlen = encrypt_key_len;
                    }
                    break;

                    case IPSEC_IS_SA:
                        /* If control reaches here, it means that the SA
                         * has been found. So just break the case.
                         * The status value is already NU_SUCCESS.
                         */
                    break;

                    case IPSEC_NEXT_SA:
                    {
                        /* Make sure that there is room for the values to
                           be returned. */
                        if(*optlen < sizeof(IPSEC_INBOUND_INDEX))
                        {
                            /* Set the error code. */
                            status = IPSEC_LENGTH_IS_SHORT;
                        }
                        else
                        {
                            if(sa_ptr->ipsec_flink != NU_NULL)
                            {
                                /* Get the next SA pointer. */
                                sa_ptr = sa_ptr->ipsec_flink;
                            }
                            else
                            {
                                /* Get the first SA pointer in the DB. */
                                sa_ptr = group_ptr->
                                        ipsec_inbound_sa_list.ipsec_head;
                            }

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
                            /* Get the no of address bytes to compare. */
                            if(sa_ptr->ipsec_select.ipsec_dest_type &
                                                            IPSEC_IPV4)
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
                                address_size = IP_ADDR_LEN;
#endif
#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
                            else
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                                address_size = IP6_ADDR_LEN;
#endif
                                /* Assign the SPI of the first SA. */
                                ((IPSEC_INBOUND_INDEX*)optval)->ipsec_spi
                                                      = sa_ptr->ipsec_spi;

                                /* Assign the security protocol of SA. */
                                ((IPSEC_INBOUND_INDEX*)optval)->
                                                        ipsec_protocol =
                                    sa_ptr->ipsec_security.ipsec_protocol;

                                /* Assign the destination address. */
                                NU_BLOCK_COPY(((IPSEC_INBOUND_INDEX*)
                                                optval)->ipsec_dest,
                                                sa_ptr->ipsec_select.
                                ipsec_dest_ip.ipsec_addr, address_size);

                                /* Assign the destination type. */
                                ((IPSEC_INBOUND_INDEX*)optval)->
                                                    ipsec_dest_type =
                                    sa_ptr->ipsec_select.ipsec_dest_type;
                        }

                        /* Return the length of the requested value. */
                        *optlen = sizeof(IPSEC_INBOUND_INDEX);
                    }
                    break;

                    default:
                    {
                        /* Unable to decode the request. */
                        status = IPSEC_NOT_FOUND;
                    }
                    } /* End of switch case. */
                }
            }

            /* Now everything is done, release the semaphore too. */
            if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release IPsec semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IPSEC_Get_Inbound_SA_Opt */
