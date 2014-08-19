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
*       ike_psk_gpi.c
*
* COMPONENT
*
*       IKE - Pre-shared Keys
*
* DESCRIPTION
*
*       This file contains the implementation of
*       IKE_Get_Preshared_Key_Index.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Get_Preshared_Key_Index
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"

#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)

/************************************************************************
*
* FUNCTION
*
*       IKE_Get_Preshared_Key_Index
*
* DESCRIPTION
*
*       This function searches the pre-shared key database for
*       the given identifier. The identifier is searched
*       logically, so an identifier of type IKE_WILDCARD would
*       match all pre-shared keys. Only the first match is
*       returned.
*
*       This is an IKE API function.
*
* INPUTS
*
*       *id                     Identifier to be searched.
*       *return_index           On return, this contains the
*                               index of the matching item.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Specified identifier does not
*                               match any of the pre-shared keys
*                               in the database.
*
************************************************************************/
STATUS IKE_Get_Preshared_Key_Index(IKE_IDENTIFIER *id,
                                   UINT16 *return_index)
{
    STATUS              status;
    IKE_PRESHARED_KEY   *ret_psk;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure the parameters are valid. */
    if((id == NU_NULL) || (return_index == NU_NULL))
    {
        status = IKE_INVALID_PARAMS;
    }

    /* Verify identifier type. */
    else if(
#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
            (id->ike_type != IKE_DOMAIN_NAME)      &&
            (id->ike_type != IKE_USER_DOMAIN_NAME) &&
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
            (id->ike_type != IKE_IPV4)             &&
            (id->ike_type != IKE_IPV4_SUBNET)      &&
            (id->ike_type != IKE_IPV4_RANGE)       &&
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            (id->ike_type != IKE_IPV6)             &&
            (id->ike_type != IKE_IPV6_SUBNET)      &&
            (id->ike_type != IKE_IPV6_RANGE)       &&
#endif
            (id->ike_type != IKE_WILDCARD))
    {
        status = IKE_INVALID_PARAMS;
    }

#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
    /* If the identifier type is a domain name, make sure the
     * domain name is specified within the identifier.
     */
    else if(((id->ike_type == IKE_DOMAIN_NAME)       ||
             (id->ike_type == IKE_USER_DOMAIN_NAME)) &&
            ((id->ike_addr.ike_domain  == NU_NULL)   ||
             (*id->ike_addr.ike_domain == 0)))
    {
        status = IKE_INVALID_PARAMS;
    }
#endif

    else
    {
        /* First grab the semaphore. */
        status = NU_Obtain_Semaphore(&IKE_Data.ike_semaphore, IKE_TIMEOUT);

        if(status == NU_SUCCESS)
        {
            /* Search the pre-shared key by identifier. */
            status = IKE_Get_Preshared_Key_By_ID(id, &ret_psk,
                         IKE_MATCH_IDENTIFIERS);

            if(status == NU_SUCCESS)
            {
                /* Now return the required index. */
                *return_index = ret_psk->ike_index;
            }

            /* Now everything is done, release the semaphore. */
            if(NU_Release_Semaphore(&IKE_Data.ike_semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to obtain IKE semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IKE_Get_Preshared_Key_Index */

#endif /* (IKE_INCLUDE_PSK_AUTH == NU_TRUE) */
