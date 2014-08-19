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
*       ips_tcp.c
*
* COMPONENT
*
*       IPSEC - TCP
*
* DESCRIPTION
*
*       This file contains functions that are specific to TCP (v4 as well
*       as v6) and IPsec.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_TCP_Check_Policy_In
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_externs.h
*       ips_api.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_externs.h"
#include "networking/ips_api.h"

/*************************************************************************
*
*   FUNCTION
*
*       IPSEC_TCP_Check_Policy_In
*
*   DESCRIPTION
*
*       Called by the TCP Layer to check policy for an incoming packet.
*
*   INPUTS
*
*       *buf_ptr                Pointer to the net buffer list.
*       *prt                    Pointer to the TCP Port structure.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion.
*       NU_TIMEOUT              The operation timed out
*       IPSEC_PKT_DISCARD       Packet does not pass security checks and
*                               must be discarded.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
*************************************************************************/
STATUS IPSEC_TCP_Check_Policy_In (NET_BUFFER *buf_ptr, TCP_PORT *prt)
{
    /* Stores the result of the request. */
    STATUS              status;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buf_ptr == NU_NULL) || (prt == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* First protect IPsec structures by obtaining semaphore. */
    status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

    /* If semaphore was successfully obtained. */
    if(status == NU_SUCCESS)
    {
        /* If the port structure does not have an associated policy, find
         * a matching policy. If we have an associated policy, but the
         * packet was received on an interface that has a different policy
         * group, find the policy for this group.
         */
        if((prt->tp_ips_in_policy == NU_NULL) ||
           (prt->tp_ips_group !=
            buf_ptr->mem_buf_device->dev_physical->dev_phy_ips_group))
        {
            status =
                IPSEC_Match_Policy_In(buf_ptr->mem_buf_device->
                                      dev_physical->dev_phy_ips_group,
                                      &IPSEC_Pkt_Selector,
                                      IPSEC_In_Bundle,
                                      IPSEC_SA_Count,
                                      &(prt->tp_ips_in_policy));

            /* Check the status value. */
            if(status == NU_SUCCESS)
            {
                /* Update the policy group. */
                prt->tp_ips_group = buf_ptr->mem_buf_device->
                                    dev_physical->dev_phy_ips_group;
            }
        }
        else
        {

            /* There exist three possibilities:
             * 1) the last processed packet was rejected in which case
             *    the policy will be null or the policy will be marked
             *    to discard packets
             * 2) IPsec security requires that security be by-passed.
             * 3) IPsec requires that security be applied, here we will
             *    check the number of security associations and the
             *    correctness of their types.
             */
            status = IPSEC_Verify_Policy(prt->tp_ips_in_policy);

        }

        /* Release the semaphore. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            /* Failed to release the semaphore. */
            NLOG_Error_Log("Failed to release IPsec semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }
    else
    {
        /* Failed to grab the semaphore. */
        NLOG_Error_Log("Failed to obtain IPsec semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IPSEC_TCP_Check_Policy_In */


