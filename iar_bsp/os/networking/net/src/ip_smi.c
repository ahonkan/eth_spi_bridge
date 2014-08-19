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
*       ip_smi.c
*
*   DESCRIPTION
*
*       This file contains the routine to change the interface to use
*       for sending multicast packets.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Setsockopt_IP_MULTICAST_IF
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
extern MULTI_SCK_OPTIONS NET_Amm_Memory[];
#endif
/*************************************************************************
*
*   FUNCTION
*
*       IP_Setsockopt_IP_MULTICAST_IF
*
*   DESCRIPTION
*
*       This function sets the interface to use for sending multicast
*       packets.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *dev_address            A pointer to the IP address of the
*                               interface to set.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_MEM_ALLOC            Insufficient memory.
*       NU_INVAL                No interface exists on the node for
*                               the given IP address.
*
*************************************************************************/
STATUS IP_Setsockopt_IP_MULTICAST_IF(INT socketd, UINT8 *dev_address)
{
    struct  sock_struct *sck_ptr = SCK_Sockets[socketd];
    MULTI_SCK_OPTIONS   *moptions;
    UINT8               *ip_addr_ptr;
    DV_DEVICE_ENTRY     *dev;
    STATUS              status = NU_SUCCESS;

    /* Is there a multicast option buffer attached to the socket. */
    if (sck_ptr->s_moptions_v4 == NU_NULL)
    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        /* Allocate a multicast option buffer. */
        if (NU_Allocate_Memory(MEM_Cached, (VOID**)&sck_ptr->s_moptions_v4,
                               sizeof(*sck_ptr->s_moptions_v4),
                               (UNSIGNED)NU_NO_SUSPEND) != NU_SUCCESS)
            return (NU_MEM_ALLOC);
#else
        /* Assign memory to multicast options*/
        sck_ptr->s_moptions_v4 = &NET_Amm_Memory[socketd];
#endif
        /* Initialize the option buffer to the default values. */
        moptions = sck_ptr->s_moptions_v4;
        UTL_Zero(moptions, sizeof(MULTI_SCK_OPTIONS));
        moptions->multio_device = NU_NULL;
        moptions->multio_ttl = IP_DEFAULT_MULTICAST_TTL;
        moptions->multio_loop = IP_DEFAULT_MULTICAST_LOOP;
        moptions->multio_num_mem_v4 = 0;
    }
    else
        moptions = sck_ptr->s_moptions_v4;

    /* If the IP address is NU_NULL then just remove the interface */
    if (dev_address == NU_NULL)
        moptions->multio_device = NU_NULL;
    else
    {
        /* Point to the address */
        ip_addr_ptr = dev_address;

        /* If the IP address is all zeros, remove the interface */
        if (IP_ADDR(ip_addr_ptr) == IP_ADDR_ANY)
            moptions->multio_device = NU_NULL;
        else
        {
            /* Get a pointer to the interface */
            dev = DEV_Get_Dev_By_Addr(ip_addr_ptr);

            /* The interface is valid so save it off. */
            if (dev)
                moptions->multio_device = dev;

            /* The interface does not exist on the node */
            else
                status = NU_INVAL;
        }
    }

    /* If all the options have default values, then there is no need
     * to keep the structure.
     */
    if ( (moptions->multio_device == NU_NULL) &&
         (moptions->multio_ttl == IP_DEFAULT_MULTICAST_TTL) &&
         (moptions->multio_loop == IP_DEFAULT_MULTICAST_LOOP) &&
         (moptions->multio_num_mem_v4 == 0) )
    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        if (NU_Deallocate_Memory(moptions) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for multicast options",
                           NERR_SEVERE, __FILE__, __LINE__);
#endif
        sck_ptr->s_moptions_v4 = NU_NULL;
    }

    return (status);

} /* IP_Setsockopt_IP_MULTICAST_IF */

#endif
