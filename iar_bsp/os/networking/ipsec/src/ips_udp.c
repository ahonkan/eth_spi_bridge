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
*       ips_udp.c
*
* COMPONENT
*
*       IPSEC - UDP
*
* DESCRIPTION
*
*       This file contains functions that are specific to UDP (v4 as well
*       as v6) and IPsec.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_UDP_Check_Policy_In
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
*       IPSEC_UDP_Check_Policy_In
*
*   DESCRIPTION
*
*       Called by the UDP Layer to check policy for an incoming packet.
*
*   INPUTS
*
*       *buf_ptr                Pointer to the net buffer list
*       *uptr                   Pointer to the UDP_PORT
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
STATUS IPSEC_UDP_Check_Policy_In (NET_BUFFER *buf_ptr, UDP_PORT *uptr)
{
    /* Stores the result of the request. */
    STATUS              status;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buf_ptr == NU_NULL) || (uptr == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* First protect IPsec structures by obtaining semaphore. */
    status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

    /* If semaphore was successfully obtained. */
    if(status == NU_SUCCESS)
    {
        /* Check if the policy group for the last received packet is equal
         * to policy group for the interface on which the current UDP
         * packet was received. Also compare the packet's selector with
         * the selector in the UDP ports structure. This should match
         * exactly and therefore we just compare the memories.
         */
        if((uptr->up_ips_in_group == NU_NULL) ||
           (uptr->up_ips_in_group !=
            buf_ptr->mem_buf_device->dev_physical->dev_phy_ips_group) ||
           (memcmp(&IPSEC_Pkt_Selector, &uptr->up_ips_in_select,
                                            sizeof(IPSEC_SELECTOR))) != 0)
        {
            /* The last received packet had different credential to the
             * current packet. Update the UDP port structure to reflect
             * the information for this new packet.
             */

            /* We will need to search for the matching security policy.
             * The following call finds such a policy.
             */
            status = IPSEC_Match_Policy_In(
                                        buf_ptr->mem_buf_device->
                                        dev_physical->dev_phy_ips_group,
                                        &IPSEC_Pkt_Selector,
                                        IPSEC_In_Bundle,
                                        IPSEC_SA_Count,
                                        &(uptr->up_ips_in_policy));

            if (status == NU_SUCCESS)
            {
                /* Now update the packet selector in the UDP ports
                 * structure.
                 */
                NU_BLOCK_COPY(&uptr->up_ips_in_select,
                              &IPSEC_Pkt_Selector,
                              sizeof(IPSEC_SELECTOR));

                /* Also update the policy group. */
                uptr->up_ips_in_group = buf_ptr->mem_buf_device->
                                        dev_physical->dev_phy_ips_group;
            }
        }
        else
        {
            /* The packet received has the same selectors as the last
             * processed packet. Therefore, the policy in the UDP port
             * structure is applicable. Verify the security
             * associations that were applied.
             */

            /* There exist three possibilities:
             * 1) the last processed packet was rejected in which case
             *    the policy will be null or the policy will be marked
             *    to discard packets
             * 2) the last processed packet by-passed security in which
             *    case the policy will be marked accordingly.
             * 3) IPsec protocols were applied, here we will check the
             *    number of security associations and the correctness
             *    of their types.
             */
            status = IPSEC_Verify_Policy(uptr->up_ips_in_policy);

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

} /* IPSEC_UDP_Check_Policy_In */

