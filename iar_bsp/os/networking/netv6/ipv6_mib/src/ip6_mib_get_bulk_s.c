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
*       ip6_mib_get_bulk_s.c                         
*
*   DESCRIPTION
*
*       This file contains the implementation of Get Bulk operation.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IPv6_Get_Bulk
*
*   DEPENDENCIES
*
*       nu_net.h
*       snmp_api.h
*       ip6_mib_s.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_api.h"
#include "networking/ip6_mib_s.h"

#ifndef SNMP_2_3
/************************************************************************
* FUNCTION
*
*        IPv6_Get_Bulk
*
* DESCRIPTION
*
*        This function is used to handle SNMP BET-BULK request on IPv6
*        MIBs.
*
* INPUTS
*
*       *obj                    Pointer to SNMP Object
*       snmp_get_function       Pointer to getter function.
*
* OUTPUTS                                                              
*                                                                      
*        [outputs of this function]                                    
*                                                                      
************************************************************************/
UINT16 IPv6_Get_Bulk(snmp_object_t *obj, IPV6_GET_FUNCTION snmp_get_function)
{
    UINT16          status = IP6_MIB_SUCCESS;
    snmp_object_t   dummy_object;
    UINT32          max_repetitions;
    UINT32          index;

    /* Get the no of repetitions for this particular object. */
    max_repetitions = obj[0].Syntax.LngUns;

    /* Get the object. */
    dummy_object= obj[0];

    /* Loop for max number of repetitions. */
    for(index = 0;
                 (index < max_repetitions) && (status == IP6_MIB_SUCCESS);
                 index++)
    {
        /* Clear the temporary object where, the instance will be 
         * retrieved.
         */
        UTL_Zero(dummy_object.Syntax.BufChr, SNMP_SIZE_BUFCHR);

        /* Get the next object. */
        if(snmp_get_function((&dummy_object), 0) != IP6_MIB_SUCCESS)
        {
            /* If no next object was available and this is the first loop
             * (no object is found) return no such  name. This will tell
             * the MIB engine not to look at the SNMP object list. If this
             * is not the first loop (at least one instance has been
             * retrieved), return success.The MIB engine will retrieve
             * the values from the SNMP object list.
             */
            if(index == 0)
            {
                status = IP6_MIB_NOSUCHNAME;
            }

            break;
        }

        /* The temporary object should indicate that this is a GET-BULK
         * request.
         */
        dummy_object.Request = SNMP_PDU_BULK;

        /* Put the retrieved instance in to the SNMP object list. */
        obj[index] = dummy_object;
    }

    /* Return status. */
    return (status);
}
#endif
#endif /* (INCLUDE_SNMP == NU_TRUE) */
