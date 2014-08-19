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
*       sck_dds2.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Delete_DNS_Server2.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Delete_DNS_Server2
*       DNS4_Delete_DNS_Server
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

extern DNS_SERVER_LIST          DNS_Servers;

/****************************************************************************
*
*   FUNCTION
*
*       NU_Delete_DNS_Server2
*
*   DESCRIPTION
*
*       This function deletes a DNS server from the list of DNS servers.
*
*   INPUTS
*
*       *dns_ip                 A pointer to the IP address of the DNS server
*                               to delete.
*       family                  The family of the DNS server to delete.
*
*   OUTPUTS
*
*       NU_SUCCESS              The server was successfully deleted.
*       NU_INVALID_PARM         The DNS server was NULL or the family was
*                               invalid.
*       Otherwise, an operating-system specific error is returned.
*
******************************************************************************/
STATUS NU_Delete_DNS_Server2(const UINT8 *dns_ip, INT16 family)
{
    STATUS      ret_status;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if ( (dns_ip == NU_NULL) ||
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
        return(NU_INVALID_PARM);

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

#if (INCLUDE_IPV6 == NU_FALSE)
    UNUSED_PARAMETER(family);
#endif

    /* Obtain the semaphore. */
    ret_status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

    if (ret_status == NU_SUCCESS)
    {
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
        if (family == NU_FAMILY_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            ret_status = DNS6_Delete_DNS_Server(dns_ip);

#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
            ret_status = DNS4_Delete_DNS_Server(dns_ip);
#endif

        NU_Release_Semaphore(&DNS_Resource);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (ret_status);

} /* NU_Delete_DNS_Server2 */

#if (INCLUDE_IPV4 == NU_TRUE)

/****************************************************************************
*
*   FUNCTION
*
*       DNS4_Delete_DNS_Server
*
*   DESCRIPTION
*
*       This function deletes an IPv4 DNS server from the list of DNS
*       servers.
*
*   INPUTS
*
*       *dns_ip                 A pointer to the IPv4 address associated
*                               with the DNS server to delete.
*
*   OUTPUTS
*
*       NU_SUCCESS              The server was successfully removed.
*       NU_INVALID_PARM         The server does not exist on the node.
*
******************************************************************************/
STATUS DNS4_Delete_DNS_Server(const UINT8 *dns_ip)
{
    DNS_SERVER                  *temp;
    STATUS                      ret_status;
    NU_SUPERV_USER_VARIABLES

    if (dns_ip == NU_NULL)
        return (NU_INVALID_PARM);

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    for (temp = DNS_Servers.dnss_head;
         temp && (memcmp(temp->dnss_ip, dns_ip, IP_ADDR_LEN) != 0);
          temp = temp->dnss_next)
          {
              ;
          }

    if (!temp)
        ret_status = NU_INVALID_PARM;
    else
        ret_status = DNS_Remove_DNS_Server(temp);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (ret_status);

} /* DNS4_Delete_DNS_Server */

#endif
