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
*       ips_sadb_goso.c
*
* COMPONENT
*
*       DATABASE - Security Associations
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Get_Outbound_SA_Opt.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Get_Outbound_SA_Opt
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
*       IPSEC_Get_Outbound_SA_Opt
*
* DESCRIPTION
*
*       This function returns an outbound SA value as specified by
*       optname.
*
* INPUTS
*
*       *index                  Index of the SA.
*       optname                 Name of the options.
*       *optval                 Values to be set for optname.
*       *optlen                 Length of the option value passed.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*
*       NU_TIMEOUT              The operation timed out.
*       IPSEC_NOT_FOUND         The group was not found or there is no
*                               policy corresponding to the passed index.
*       IPSEC_LENGTH_IS_SHORT   Indicates that the value to be returned
*                               required more memory than was passed.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Outbound_SA_Opt(IPSEC_OUTBOUND_INDEX *index, INT optname,
                                 VOID *optval, INT *optlen)
{
    STATUS              status;
    UINT16              auth_key_len;
    UINT16              encrypt_key_len;
    IPSEC_POLICY_GROUP  *group_ptr;
    IPSEC_OUTBOUND_SA   *sa_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check the parameters passed. */
    if((index == NU_NULL) || (index->ipsec_group == NU_NULL) ||
        (index->ipsec_index == NU_NULL))
    {
        /* Some invalid parameter has been passed. */
        status = IPSEC_INVALID_PARAMS;
    }

    /* Check the parameters further. */
    else if((optname != IPSEC_IS_SA) && ((optval == NU_NULL) ||
        (optlen == NU_NULL)))
    {
            /* Some invalid parameter has been passed. */
            status = IPSEC_INVALID_PARAMS;
    }

    else
    {
        /* First grab the semaphore. */
        status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

        /* Check the status value. */
        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain IPsec semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        else
        {
            /* First get the group pointer. */
            status = IPSEC_Get_Group_Entry(index->ipsec_group,&group_ptr);

            /* Check the status value. */
            if(status == NU_SUCCESS)
            {
                /* Get the required SA. */
                status = IPSEC_Get_Outbound_SA_By_Index(
                    index->ipsec_index,
                    group_ptr->ipsec_outbound_sa_list.ipsec_head,
                    &sa_ptr);

                /* Check the status value. */
                if(status == NU_SUCCESS)
                {
                    /* Decode the request. */
                    switch(optname)
                    {
                        case IPSEC_SELECT:
                        {
                            /* Make sure that there is room for the values
                               to be returned. */
                            if(*optlen < sizeof(sa_ptr->ipsec_select))
                            {
                                /* Set the error code. */
                                status = IPSEC_LENGTH_IS_SHORT;
                            }
                            else
                            {
                                /* Now copy the requested values. */
                                NU_BLOCK_COPY(optval,
                                    &(sa_ptr->ipsec_select),
                                    sizeof(sa_ptr->ipsec_select));
                            }

                            /* Return the size of the value required. */
                            *optlen = sizeof(sa_ptr->ipsec_select);
                        }
                        break;

                        case IPSEC_SECURITY:
                        {
                            /* Make sure that there is room for the values
                                to be returned. */
                            if(*optlen < sizeof(sa_ptr->ipsec_security))
                            {
                                /* Set the error code. */
                                status = IPSEC_LENGTH_IS_SHORT;
                            }
                            else
                            {
                                /* Copy the requested structure. */
                                NU_BLOCK_COPY(optval,
                                    &(sa_ptr->ipsec_security),
                                    sizeof(sa_ptr->ipsec_security));
                            }

                            /* Return the size of the value required.*/
                            *optlen = sizeof(sa_ptr->ipsec_security);
                        }
                        break;

#if (INCLUDE_IKE == NU_TRUE)

                        case IPSEC_SOFT_LIFETIME:
                        {
                            /* Make sure that there is room for the values
                               to be returned. */
                            if(*optlen < sizeof(IPSEC_SA_LIFETIME))
                            {
                                /* Set the error code. */
                                status = IPSEC_LENGTH_IS_SHORT;
                            }
                            else
                            {
                                /* If lifetime pointer is valid. */
                                if(sa_ptr->ipsec_soft_lifetime != NU_NULL)
                                {
                                    /* Copy the requested structure. */
                                    NU_BLOCK_COPY(optval,
                                        sa_ptr->ipsec_soft_lifetime,
                                        sizeof(IPSEC_SA_LIFETIME));
                                }
                                /* Otherwise, lifetime has not been set.
                                 */
                                else
                                {
                                    /* Set the structure to zero, instead
                                     * of returning an error. This
                                     *  maintains consistency with
                                     * outbound SA processing.
                                     */
                                    UTL_Zero(optval,
                                        sizeof(IPSEC_SA_LIFETIME));
                                }
                            }

                            /* Return the size of the value required. */
                            *optlen = sizeof(IPSEC_SA_LIFETIME);
                        }
                        break;

                        case IPSEC_HARD_LIFETIME:
                        {
                            /* Make sure that there is room for the values
                               to be returned. */
                            if(*optlen < sizeof(IPSEC_SA_LIFETIME))
                            {
                                /* Set the error code. */
                                status = IPSEC_LENGTH_IS_SHORT;
                            }

                            else
                            {
                                /* If lifetime pointer is valid. */
                                if(sa_ptr->ipsec_hard_lifetime != NU_NULL)
                                {
                                    /* Copy the requested structure. */
                                    NU_BLOCK_COPY(optval,
                                        sa_ptr->ipsec_hard_lifetime,
                                        sizeof(IPSEC_SA_LIFETIME));
                                }
                                /* Otherwise, lifetime has not been set.
                                 */
                                else
                                {
                                    /* Set the structure to zero, instead
                                     * of returning an error. This
                                     *  maintains consistency with
                                     * outbound SA processing.
                                     */
                                    UTL_Zero(optval,
                                        sizeof(IPSEC_SA_LIFETIME));
                                }
                            }

                            /* Return the size of the value required. */
                            *optlen = sizeof(IPSEC_SA_LIFETIME);
                        }
                        break;

#endif /* if (INCLUDE_IKE == NU_TRUE) */

                        case IPSEC_AUTH_KEY:
                        {
                            /* Get authentication algorithm key length. */
                            auth_key_len = IPSEC_GET_AUTH_KEY_LEN(
                                sa_ptr->ipsec_security.ipsec_auth_algo);

                            /* Make sure that there is room for the values
                               to be returned. */
                            if(*optlen < (INT)auth_key_len)
                            {
                                /* Set the error code. */
                                status = IPSEC_LENGTH_IS_SHORT;
                            }
                            else
                            {
                                /* Copy the requested structure. */
                                NU_BLOCK_COPY(optval,
                                    sa_ptr->ipsec_auth_key, auth_key_len);
                            }

                            /* Return the size of the value required. */
                            *optlen = auth_key_len;
                        }
                        break;

                        case IPSEC_ENCRYPTION_KEY:
                        {
                            /* Get encryption key length. */
                            encrypt_key_len =
                                IPSEC_GET_ENCRYPT_KEY_LEN(sa_ptr->
                                ipsec_security.ipsec_encryption_algo);

                            /* Make sure that there is room for the values
                               to be returned. */
                            if(*optlen < (INT)encrypt_key_len)
                            {
                                /* Set the error code. */
                                status = IPSEC_LENGTH_IS_SHORT;
                            }
                            else
                            {
                                /* Copy the requested structure. */
                                NU_BLOCK_COPY(optval,
                                    sa_ptr->ipsec_encryption_key,
                                    encrypt_key_len);
                            }

                            /* Return the size of the value required. */
                            *optlen = encrypt_key_len;
                        }
                        break;

                        case IPSEC_IS_SA :
                            /* If the control reaches here, it means that
                             * passed index is an SA. Just break the case.
                             */
                        break;

                        case IPSEC_NEXT_SA:
                        {
                            if (*optlen < sizeof(UINT32))
                            {
                                status = IPSEC_LENGTH_IS_SHORT;
                            }
                            else
                            {
                                /* If the next SA does exist! */
                                if (sa_ptr->ipsec_flink != NU_NULL)
                                {
                                    /* Return the next SA's index. */
                                    *((UINT32*)optval) =
                                        sa_ptr->ipsec_flink->ipsec_index;
                                }

                                else
                                {
                                    /* Return the index of the first SA.
                                     */
                                    *((UINT32*)optval) =
                                       (group_ptr->ipsec_outbound_sa_list.
                                        ipsec_head->ipsec_index);
                                }

                                /* Size of the value required. */
                                *optlen = sizeof(sa_ptr->ipsec_index);
                            }
                        }
                        break;

                        default:
                        {
                            /* Unable to decode the request. */
                            status = IPSEC_NOT_FOUND;
                        }
                    }
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

} /* IPSEC_Get_Outbound_SA_Opt */
