/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS 
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS 
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME                                        
*
*       2465gen.c                                    
*
*   DESCRIPTION
*
*       This file contains the implementation of IPv6 MIBs that don't
*       fall under any group.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       ipv6Forwarding
*       ipv6DefaultHopLimit
*
*   DEPENDENCIES
*
*       nu_net.h
*       externs6.h
*       ip6_mib.h
*       snmp_api.h
*       mib.h
*       ip6_mib_s.h
*
************************************************************************/
#include "networking/nu_net.h"

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IPV6_MIB == NU_TRUE))

#include "networking/externs6.h"
#include "networking/ip6_mib.h"
#include "networking/snmp_api.h"
#include "networking/mib.h"
#include "networking/ip6_mib_s.h"

/* Maximum value for default hop limit. */
#define IP6_MIB_MAX_DEF_HOPLIMIT_VAL       255

/************************************************************************
* FUNCTION
*
*        ipv6Forwarding
*
* DESCRIPTION
*
*        This function is used to handle SNMP request on 'ipv6Forwarding'.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  Length of SNMP request.
*        *param                 Additional parameters (Not used).
*
* OUTPUTS
*
*        SNMP_NOERROR           When successful.
*        SNMP_GENERROR          General error.
*        SNMP_NOSUCHNAME        Invalid request.
*        SNMP_WRONGVALUE        Wrong value in SET request.
*
************************************************************************/
UINT16 ipv6Forwarding(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16                  status;

    /* Avoiding compilation warning. */
    UNUSED_PARAMETER(param);

#if (INCLUDE_IP_FORWARDING == NU_TRUE)

    /* Validating SNMP request. */
    if (MibSimple(obj, idlen) == NU_TRUE)
    {
        status = SNMP_NOERROR;

        if ( (obj->Request == SNMP_PDU_SET) ||
             (obj->Request == SNMP_PDU_UNDO) )
        {
            /* If we have valid value to set is 'true'. */
            if (obj->Syntax.LngInt == IP6_MIB_TRUE)
            {
                /* Perform the actual set request. */
                IP6_MIB_FORWARDING_SET(IP6_MIB_TRUE, status);
            }

            /* If value to set is 'false'. */
            else if (obj->Syntax.LngInt == IP6_MIB_FALSE)
            {
                /* Perform the actual set request. */
                IP6_MIB_FORWARDING_SET(IP6_MIB_FALSE, status);
            }

            /* If we don't have valid value to set then return error code.
             */
            else
            {
                status = SNMP_WRONGVALUE;
            }
        }

        /* In order to process get request. */
        else if (obj->Request != SNMP_PDU_COMMIT)
        {
            /* Get the value of ipv6Forwarding. */
            IP6_MIB_FORWARDING_GET(obj->Syntax.LngInt, status);
        }
    }

    else
    {
#else
    UNUSED_PARAMETER(obj);
    UNUSED_PARAMETER(idlen);
#endif
        status = SNMP_NOSUCHNAME;

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
    }
#endif

    /* Returning success or error code. */
    return (status);

} /* ipv6Forwarding */

/************************************************************************
* FUNCTION
*
*        ipv6DefaultHopLimit
*
* DESCRIPTION
*
*        This function is used to handle SNMP requests on
*        'ipv6DefaultHopLimit'.
*
* INPUTS
*
*        *obj                   SNMP object containing request.
*        idLen                  Length of SNMP request.
*        *param                 Additional parameters (Not used).
*
* OUTPUTS
*
*        SNMP_NOERROR           When successful.
*        SNMP_GENERROR          General error.
*        SNMP_NOSUCHNAME        Invalid request.
*        SNMP_WRONGVALUE        Wrong value in SET request.
*
************************************************************************/
UINT16 ipv6DefaultHopLimit(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16              status;

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(param);

    /* Validating the SNMP request. */
    if (MibSimple(obj, idlen) == NU_TRUE)
    {
        /* Set success code to return. */
        status = SNMP_NOERROR;

        /* Process the SET and UNDO request. */
        if ( (obj->Request == SNMP_PDU_SET) ||
             (obj->Request == SNMP_PDU_UNDO) )
        {
            /* If we have valid value to set. */
            if (obj->Syntax.LngUns <= IP6_MIB_MAX_DEF_HOPLIMIT_VAL)
            {
                IP6_MIB_DEFAULTHOPLIMIT_SET(obj->Syntax.LngUns, status);
            }

            /* If we did not have valid value to set then return error
             * code.
             */
            else
            {
                status = SNMP_WRONGVALUE;
            }
        }

        /* Process the GET, GET-NEXT and GET-BULK requests. */
        else if(obj->Request != SNMP_PDU_COMMIT)
        {
            /* Getting the value of ipv6DefaultHopLimit. */
            IP6_MIB_DEFAULTHOPLIMIT_GET(obj->Syntax.LngInt, status);
        }
    }

    /* If we did not have valid SNMP request. */
    else
    {
        status = SNMP_NOSUCHNAME;
    }

    /* Return success or error code. */
    return (status);

} /* ipv6DefaultHopLimit */

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IPV6_MIB == NU_TRUE)) */
