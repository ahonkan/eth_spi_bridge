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
*       dns6.c
*
*   DESCRIPTION
*
*       This file contains the Domain Name System component functions
*       specific to IPv6.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       DNS6_Addr_To_String
*       DNS6_String_To_Addr
*       DNS6_Delete_DNS_Server
*       DNS6_Get_DNS_Servers
*
*   DEPENDENCIES
*
*       externs.h
*       ncl.h
*       dns6.h
*
*************************************************************************/

#include "networking/externs.h"
#include "networking/ncl.h"
#include "networking/dns6.h"

extern  DNS_SERVER_LIST     DNS_Servers;

/*************************************************************************
*
*   FUNCTION
*
*       DNS6_Addr_To_String
*
*   DESCRIPTION
*
*       This function takes an IPv6 address and converts it to a character
*       string.
*
*   INPUTS
*
*       addr                    The address to convert.
*       new_name                The converted address in ascii format.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DNS6_Addr_To_String(CHAR *addr, CHAR *new_name)
{
    INT         i, j;
    CHAR HUGE   *ptr = (CHAR HUGE *)new_name;
    UINT8 HUGE  *a = (UINT8 HUGE *)addr;

    /* We need to add each octet of the address to the name in reverse order. */
    for (i = 15; i >= 0; i--)
    {
        for (j = 0; j <= 1; j++)
        {
            NU_ITOA((0x0f & (a[i] >> (4 * j))), (CHAR *)ptr, 16);

            ptr++;

            /* Add the dot. */
            *ptr = '.';

            ptr++;
        }
    }

    strcpy((CHAR *)ptr, "IP6.ARPA");

} /* DNS6_Addr_To_String */

/****************************************************************************
*
*   FUNCTION
*
*       DNS6_String_To_Addr
*
*   DESCRIPTION
*
*       This function takes a string and converts it to an IPv6 address.
*
*   INPUTS
*
*       *addr                   A pointer to the memory to fill with the address.
*       *name                   A pointer to the null-terminated ASCII string.
*
*   OUTPUTS
*
*       None
*
******************************************************************************/
VOID DNS6_String_To_Addr(CHAR *addr, CHAR *name)
{
    INT         i;
    CHAR        temp_ptr[3];

    for (i = (IP6_ADDR_LEN - 1); i >= 0; i --)
    {
        /* Squeeze out the '.' and flip the two bytes. */
        temp_ptr[1] = name[0];
        temp_ptr[0] = name[2];
        temp_ptr[2] = 0;

        /* Convert the ASCII value back to integer and store it in the address
         * memory.
         */
        addr[i] = NCL_Ahtoi(temp_ptr);

        /* Move to the next two-byte sequence. */
        name += 4;
    }

} /* DNS6_String_To_Addr */

/*************************************************************************
*
*   FUNCTION
*
*       DNS6_Delete_DNS_Server
*
*   DESCRIPTION
*
*       This function deletes an IPv6 DNS server from the list of DNS
*       servers.
*
*   INPUTS
*
*       dns_ip                  The IP address of the DNS server to
*                               delete
*
*   OUTPUTS
*
*       NU_SUCCESS              The server was successfully deleted from
*                               the list of IPv6 servers.
*       NU_INVALID_PARM         The server does not exist.
*
*************************************************************************/
STATUS DNS6_Delete_DNS_Server(const UINT8 *dns_ip)
{
    DNS_SERVER                  *temp;
    STATUS                      status;

    /* Traverse the list of servers until the target server is found */
    for (temp = DNS_Servers.dnss_head;
         temp && (memcmp(dns_ip, temp->dnss_ip, IP6_ADDR_LEN) != 0);
         temp = temp->dnss_next)
         ;

    /* If the target was not found, return an error */
    if (!temp)
        status = NU_INVALID_PARM;

    /* Otherwise, remove the server from the list */
    else
        status = DNS_Remove_DNS_Server(temp);

    return (status);

} /* DNS6_Delete_DNS_Server */

/*************************************************************************
*
*   FUNCTION
*
*       DNS6_Get_DNS_Servers
*
*   DESCRIPTION
*
*       This function retrieves a list of IPv6 DNS servers and copies the
*       IP address into the memory provided.
*
*   INPUTS
*
*       *dest                   A pointer to the area of memory into
*                               which to copy the address of each IPv6
*                               DNS server.
*       size                    The size of the memory provided.
*
*   OUTPUTS
*
*       NU_INVALID_PARM         The memory was NULL or the size of the
*                               memory is not sufficient for one IPv6
*                               address.
*       INT                     The total number of IPv6 addresses in the
*                               area of memory.
*
*************************************************************************/
INT DNS6_Get_DNS_Servers(UINT8 *dest, INT size)
{
    /* The memory area provided by the application will be treated as an array
     * of 128 bit integers (IP addresses).
     */
    INT                         ret_status;
    DNS_SERVER                  *srv_ptr;
    INT                         i;

    if ( (size < IP6_ADDR_LEN) || (dest == NU_NULL) )
        ret_status = NU_INVALID_PARM;

    else
    {
        for (srv_ptr = DNS_Servers.dnss_head, i = 0;
             srv_ptr &&
             (memcmp(srv_ptr->dnss_ip, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                     MAX_ADDRESS_SIZE) != 0) && size >= IP6_ADDR_LEN;
             srv_ptr = srv_ptr->dnss_next)
        {
            if (srv_ptr->family == NU_FAMILY_IP6)
            {
                memcpy(&dest[i], srv_ptr->dnss_ip, IP6_ADDR_LEN);

                /* Decrement the size counter */
                size -= IP6_ADDR_LEN;

                i += IP6_ADDR_LEN;
            }
        }

        ret_status = i / IP6_ADDR_LEN;
    }

    /* Return the number of IP addresses in the memory */
    return (ret_status);

} /* DNS6_Get_DNS_Servers */
