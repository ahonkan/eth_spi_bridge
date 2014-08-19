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
*       multi_gsss.c
*
*   COMPONENT
*
*       Multicast messages:  Both IGMP and MLD
*
*   DESCRIPTION
*
*       This file contains the IP-agnostic function for getting
*       the socket state structure that is used by both IGMP and
*       MLD for maintaining the multicast state of the socket.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       Multi_Get_Socket_State_Struct
*
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"


/*************************************************************************
*
*   FUNCTION
*
*       Multi_Get_Sck_State_Struct
*
*   DESCRIPTION
*
*       Walks the socket's state link list to find a match for the
*       specified multicast address
*
*   INPUTS
*
*       *m_addr                 Target multicast address
*       *moptions               Ptr to the socket options struct
*       *device                 Ptr to the device struct
*       socketd                 Socket descriptor that is used to verify a
*                               match.  If the socket descriptor is
*                               NULL, we will return a ptr to the
*                               head of the socket state list.
*       family_type             Used to determine the IP version
*
*   OUTPUTS
*
*       MULTI_SCK_STATE *       Pointer to the socket state struct
*
*************************************************************************/
MULTI_SCK_STATE *Multi_Get_Sck_State_Struct(const UINT8 *m_addr,
                                            const MULTI_SCK_OPTIONS *moptions,
                                            const DV_DEVICE_ENTRY *device,
                                            INT socketd, INT16 family_type)
{
    UINT8               i, num_of_multi_members = 0;
    MULTI_SCK_STATE     *sck_state = NU_NULL;
    MULTI_DEV_STATE     *dev_state = NU_NULL;

    /* Get the number of groups to which the socket is a member &  a ptr
     *  to the device state structure so that we can remove the
     *  sck state structure from it's list.
     */
#if (INCLUDE_IPV4 == NU_TRUE)

    if (family_type == NU_FAMILY_IP)
    {
        num_of_multi_members = moptions->multio_num_mem_v4;

        dev_state = Multi_Get_Device_State_Struct(device,m_addr, IP_ADDR_LEN);
    }

#if (INCLUDE_IPV6 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    if (family_type == NU_FAMILY_IP6)
    {
        num_of_multi_members = moptions->multio_num_mem_v6;

        dev_state = Multi_Get_Device_State_Struct(device,m_addr, IP6_ADDR_LEN);
    }
#endif

    /* Find the correct socket */
    for (i = 0; i < num_of_multi_members; i++)
    {
        if (
#if (INCLUDE_IPV4 == NU_TRUE)
            ((family_type == NU_FAMILY_IP) &&
              (memcmp(moptions->multio_v4_membership[i]->ipm_addr,
                      m_addr, IP_ADDR_LEN) == 0) &&
              (device == moptions->multio_v4_membership[i]->ipm_data.multi_device))
#if (INCLUDE_IPV6 == NU_TRUE)
                    ||
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
             ((family_type == NU_FAMILY_IP6) &&
              (memcmp(moptions->multio_v6_membership[i]->ipm6_addr,
                      m_addr, IP6_ADDR_LEN) == 0) &&
              (device == moptions->multio_v6_membership[i]->ipm6_data.multi_device))
#endif
                )
        {
            break;
        }
    }

    /* If a match was found */
    if ( (i < num_of_multi_members) && (dev_state != NU_NULL) )
    {
        /* Now, find the exact socket state structure.  If the passed in
         * socket descriptor is NULL, then return the head of the list.
         */
        if (socketd == MULTICAST_GET_SCK_LIST_HEAD)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
            if (family_type == NU_FAMILY_IP)
            {
                /* We will return a pointer to the head of the socket state
                 * link list
                 */
                sck_state = dev_state->dev_sck_state_list.head;
            }

#if (INCLUDE_IPV6 == NU_TRUE)
            else
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

            if (family_type == NU_FAMILY_IP6)
                /* We will return a pointer to the head of the socket state
                 * link list
                 */
                sck_state = dev_state->dev_sck_state_list.head;
#endif
        }

        else
        {
#if (INCLUDE_IPV4 == NU_TRUE)

            if (family_type == NU_FAMILY_IP)
                sck_state = dev_state->dev_sck_state_list.head;


#if (INCLUDE_IPV6 == NU_TRUE)
            else
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

            if (family_type == NU_FAMILY_IP6)
                sck_state = dev_state->dev_sck_state_list.head;

#endif

            /* Find the exact state structure */
            while (sck_state != NU_NULL)
            {
                if (sck_state->sck_socketd == socketd)
                    break;
                else
                    sck_state = sck_state->sck_state_next;
            }
        }
    }

    return (sck_state);

} /* Multi_Get_Sck_State_Struct */
