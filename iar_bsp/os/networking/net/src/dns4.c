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
*       dns4.c
*
*   DESCRIPTION
*
*       This file contains the Domain Name System component functions
*       specific to IPv4.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       DNS4_Addr_To_String
*       DNS4_String_To_Addr
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/****************************************************************************
*
*   FUNCTION
*
*       DNS4_Addr_To_String
*
*   DESCRIPTION
*
*       This function takes an IPv4 address and converts it to a character
*       string.
*
*   INPUTS
*
*       *addr                   A pointer to the address to convert.
*       *new_name               A pointer to the address in ascii format.
*
*   OUTPUTS
*
*       None
*
******************************************************************************/
VOID DNS4_Addr_To_String(CHAR *addr, CHAR *new_name)
{
    INT         i;
    CHAR        *ptr = new_name;
    UINT8       *a = (UINT8 *)addr;

    /* We need to add each octet of the address to the name in reverse order. */
    for (i = 3; i >= 0; i--)
    {
        /* Convert the octet to a character string. */
        NU_ITOA(((INT)((UINT8)a[i])), ptr, 10);

        /* Move past the string just added. */
        ptr += strlen(ptr);

        /* Add the dot. */
        *ptr++ = '.';
    }

    strcpy(ptr, "IN-ADDR.ARPA");

} /* DNS4_Addr_To_String */

/****************************************************************************
*
*   FUNCTION
*
*       DNS4_String_To_Addr
*
*   DESCRIPTION
*
*       This function takes a string and converts it to an IPv4 address.
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
VOID DNS4_String_To_Addr(CHAR *addr, CHAR *name)
{
    INT         i;
    UINT8       temp[IP_ADDR_LEN];

    /* Convert the IP address to integers. */
    NU_Inet_PTON(NU_FAMILY_IP, name, temp);

    /* Reverse the IP address. */
    for (i = 3; i >= 0; i--)
    {
        *addr = temp[i];
        addr ++;
    }

} /* DNS4_String_To_Addr */

#endif
