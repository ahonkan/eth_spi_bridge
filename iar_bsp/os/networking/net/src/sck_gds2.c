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
*
*   FILE NAME
*
*       sck_gds2.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Get_DNS_Servers2.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Get_DNS_Servers2
*       DNS4_Get_DNS_Servers
*
*   DEPENDENCIES
*
*       nu_net.h
*       dns4.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)
#include "networking/dns4.h"
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/dns6.h"
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
extern  DNS_SERVER_LIST     DNS_Servers;
#endif

/****************************************************************************
*
*   FUNCTION
*
*       NU_Get_DNS_Servers2
*
*   DESCRIPTION
*
*       This function retrieves a list of DNS servers of the specified
*       family type.
*
*   INPUTS
*
*       *dest                   A pointer to the memory area in which to place
*                               the IP address of each DNS server.
*       size                    The size of the memory provided.
*       family                  The family of servers to return.
*
*   OUTPUTS
*
*       NU_INVALID_PARM         The memory provided was NULL, the size of
*                               the memory provided was insufficient for one
*                               IP address or the family type was invalid.
*       INT                     The number of addresses stored in the memory.
*       Otherwise, an operating-system specific error is returned.
*
******************************************************************************/
INT NU_Get_DNS_Servers2(UINT8 *dest, INT size, INT16 family)
{
    INT     ret_status;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Validate the parameter list */
    if ( (dest == NU_NULL) ||
         (
#if (INCLUDE_IPV4 == NU_TRUE)
         ((family == NU_FAMILY_IP) && (size < IP_ADDR_LEN))
#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
         ||
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
         ((family == NU_FAMILY_IP6) && (size < IP6_ADDR_LEN))
#endif
         ) )
        return (NU_INVALID_PARM);

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Obtain the semaphore. */
    ret_status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

    if (ret_status == NU_SUCCESS)
    {
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
        /* Retrieve IPv6 servers */
        if (family == NU_FAMILY_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            ret_status = DNS6_Get_DNS_Servers(dest, size);

#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
            /* Retrieve IPv4 servers */
            ret_status = DNS4_Get_DNS_Servers(dest, size);
#endif

        NU_Release_Semaphore(&DNS_Resource);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (ret_status);

} /* NU_Get_DNS_Servers2 */

#if (INCLUDE_IPV4 == NU_TRUE)

/****************************************************************************
*
*   FUNCTION
*
*       DNS4_Get_DNS_Servers
*
*   DESCRIPTION
*
*       This function returns the list of IPv4 DNS servers on the node.
*
*   INPUTS
*
*       *dest                   A pointer to the memory in which to store
*                               the list of servers.
*       size                    The size of the memory.
*
*   OUTPUTS
*
*       INT                     The number of servers stored in the memory.
*
******************************************************************************/
INT DNS4_Get_DNS_Servers(UINT8 *dest, INT size)
{
    /* The memory area provided by the application will be treated as an array
       of 32 bit integers (IP addresses). */
    UINT32                      *dns_array = (UINT32 *)dest;
    INT                         ret_status;
    DNS_SERVER                  *srv_ptr;
    INT                         i;

    for (srv_ptr = DNS_Servers.dnss_head, i = 0;
         srv_ptr && *(UINT32 *)srv_ptr->dnss_ip != 0 && size >= IP_ADDR_LEN;
         srv_ptr = srv_ptr->dnss_next)
    {
        if (srv_ptr->family == NU_FAMILY_IP)
        {
            dns_array[i] = IP_ADDR(srv_ptr->dnss_ip);

            size -= IP_ADDR_LEN;
            i++;
        }
    }

    ret_status = i;

    /* return pointer to beginning of arrays */
    return (ret_status);

} /* DNS4_Get_DNS_Servers */

#endif
