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
* FILE NAME
*
*       sck_ads2.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Add_DNS_Server2.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Add_DNS_Server2
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

extern DNS_SERVER_LIST          DNS_Servers;

/****************************************************************************
*
*   FUNCTION
*
*       NU_Add_DNS_Server2
*
*   DESCRIPTION
*
*       Add a new DNS server to the list of DNS servers to use.
*
*   INPUTS
*
*       *new_dns_server         A pointer to the IP address of the DNS
*                               server to add.
*       where                   The position in the list in which to add
*                               the server.
*       family                  The family of the new server.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful operation.
*       NU_QUEUE_FULL           Indicates the DNS server list is full.
*       NU_INVALID_PARM         Indicates that one of the parameters was a
*                               NULL pointer.
*       Otherwise, an operating-specific error code is returned.
*
******************************************************************************/
STATUS NU_Add_DNS_Server2(const UINT8 *new_dns_server, INT where, INT16 family)
{
    DNS_SERVER                  *current_server, *temp;
    STATUS                      ret_status;

#if (INCLUDE_IPV4 == NU_TRUE)
    UINT32                      server_ip = 0;
#endif

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if ( (new_dns_server == NU_NULL) ||
         (
#if (INCLUDE_IPV4 == NU_TRUE)
         (family != NU_FAMILY_IP)
#endif

#if (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE)
         &&
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
         (family != NU_FAMILY_IP6)
#endif
         ) )
        return (NU_INVALID_PARM);

#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
    if (family == NU_FAMILY_IP)
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
    {
        server_ip = IP_ADDR(new_dns_server);

        /* If this is not a class A, B, or C address, or if it is 0 then it is not
         * a valid IP address.
         */
        if ( (!IP_CLASSA_ADDR(server_ip) && !IP_CLASSB_ADDR(server_ip) &&
              !IP_CLASSC_ADDR(server_ip)) || server_ip == 0)
        {
             return (NU_INVALID_PARM);
        }
    }

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

#if (INCLUDE_IPV4 == NU_TRUE)

    if (server_ip == 0)
    {
        /* server_ip is "used" here to remove a warning generated
         * by Real View tools.
         */
        UNUSED_PARAMETER(server_ip);
    }
#endif

    /* Obtain the semaphore. */
    ret_status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

    if (ret_status == NU_SUCCESS)
    {
        /* First, check if the DNS Server is already on the list.  If so,
         * return successfully.
         */
        current_server = DNS_Servers.dnss_head;

        /* Traverse through the list of servers */
        while ( (current_server) &&
                (memcmp(current_server->dnss_ip, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                        MAX_ADDRESS_SIZE) != 0) )
        {
            /* If the family and IP address are a match, break out of the loop */
            if (
#if (INCLUDE_IPV6 == NU_TRUE)
                 ((current_server->family == family) && (family == NU_FAMILY_IP6) &&
                  (memcmp(current_server->dnss_ip, new_dns_server,
                          IP6_ADDR_LEN) == 0))
#if (INCLUDE_IPV4 == NU_TRUE)
                      ||
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
                 ((current_server->family == family) && (family == NU_FAMILY_IP) &&
                  (*(UINT32 *)current_server->dnss_ip == server_ip))
#endif
                 )

            {
                ret_status = NU_SUCCESS;
                break;
            }

            /* Otherwise, get a pointer to the next entry in the list */
            else
                current_server = current_server->dnss_next;
        }

        /* If there is no next entry, then the queue is full */
        if (!current_server)
            ret_status = NU_QUEUE_FULL;

        /* If this server does not already exist in the list, add it */
        else if (memcmp(current_server->dnss_ip, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                        MAX_ADDRESS_SIZE) == 0)
        {
            /* Validate the where parameter */
            if ( (where != DNS_ADD_TO_FRONT) && (where != DNS_ADD_TO_END) )
                ret_status = NU_INVALID_PARM;

            else
            {
                /* Remove the last one from the list. After initializing it we will
                   put it back in its new location. */
                current_server = DNS_Servers.dnss_tail;

                DLL_Remove(&DNS_Servers, current_server);

                /* Populate the server structure. */
                memcpy(current_server->dnss_ip, new_dns_server, MAX_ADDRESS_SIZE);
                current_server->family = family;

                if (where == DNS_ADD_TO_FRONT)
                {
                    /* Insert the current server at the front of the list. */
                    DLL_Insert(&DNS_Servers, current_server, DNS_Servers.dnss_head);
                }
                else
                {
                    /* Walk the list until either the last node in the list is reached
                       or the last node that is in use is reached, whichever occurs
                       first. */
                    for ( temp = DNS_Servers.dnss_head;
                          temp && (memcmp(temp->dnss_ip,
                                          "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                                          MAX_ADDRESS_SIZE) != 0);
                          temp = temp->dnss_next)
                          {
                              ;
                          }

                    /* If all of the nodes in the list are in use. Enqueue this at the
                       end. Else insert it into the proper position. */
                    if (temp == NU_NULL)
                        DLL_Enqueue(&DNS_Servers, current_server);
                    else
                        DLL_Insert(&DNS_Servers, current_server, temp);
                }

                ret_status = NU_SUCCESS;
            }
        }

        NU_Release_Semaphore(&DNS_Resource);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return */
    return (ret_status);

} /* NU_Add_DNS_Server2 */
