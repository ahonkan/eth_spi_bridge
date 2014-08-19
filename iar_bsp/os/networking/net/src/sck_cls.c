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
*       sck_cls.c
*
*   DESCRIPTION
*
*       This file contains the implementation of SCK_Cleanup_Socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       SCK_Cleanup_Socket
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/externs6.h"
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
extern tx_ancillary_data    NET_Sticky_Options_Memory[];
extern UINT8                NET_Sticky_Options_Memory_Flags[];
#endif

/*************************************************************************
*
*   FUNCTION
*
*       SCK_Cleanup_Socket
*
*   DESCRIPTION
*
*       This function is responsible for cleaning up the resources of
*       a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID SCK_Cleanup_Socket(INT socketd)
{
    struct sock_struct  *sockptr = SCK_Sockets[socketd];

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
#if (INCLUDE_IPV4 == NU_TRUE)
    IP_MREQ     mreq_struct;
#endif
    INT         multi_members;
#if (INCLUDE_IPV6 == NU_TRUE)
    IP6_MREQ    mreq_struct6;
#endif
#endif

#if (INCLUDE_TCP == NU_TRUE)
    /* delete its task table entry */
    SCK_TaskTable_Entry_Delete(socketd);

    /* If this is a TCP socket, and there is still a port associated
     * with the socket, ensure that the port structure no longer
     * references this socket.  This does not need to be done for UDP
     * or RAW sockets since UDP and RAW port structures are deallocated
     * when the sockets are deallocated.
     */
    if ( (sockptr->s_protocol == NU_PROTO_TCP) &&
         (sockptr->s_port_index != -1) &&
         (TCP_Ports[sockptr->s_port_index]) )
        TCP_Ports[sockptr->s_port_index]->p_socketd = -1;
#endif

    /* Free the buffer saved off for ancillary data purposes */
    if (sockptr->s_rx_ancillary_data)
        MEM_One_Buffer_Chain_Free(sockptr->s_rx_ancillary_data,
                                  &MEM_Buffer_Freelist);

    /* If there are buffers on the RX list then free them. */
    if (sockptr->s_recvlist.head != NU_NULL)
        MEM_Buffer_Cleanup(&(sockptr->s_recvlist));

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

    /* If there are multicast options associated with the socket, unset
     * each of the socket options and deallocate the memory for the
     * multicast data structure if necessary.
     */
#if (INCLUDE_IPV6 == NU_TRUE)
    if ( (sockptr->s_family == NU_FAMILY_IP6) &&
         (sockptr->s_moptions_v6) )
    {
        /* Set the multicast options to default values so the memory
         * will be deallocated if the socket is a member of any groups.
         */
        sockptr->s_moptions_v6->multio_device = NU_NULL;
        sockptr->s_moptions_v6->multio_hop_lmt = IP6_DEFAULT_MULTICAST_TTL;

        /* Leave each multicast group */
        for (multi_members = sockptr->s_moptions_v6->multio_num_mem_v6;
             multi_members > 0; multi_members--)
        {
            /* Set a pointer to the device index */
            mreq_struct6.ip6_mreq_dev_index =
                sockptr->s_moptions_v6->
                multio_v6_membership[multi_members - 1]->ipm6_data.
                multi_device->dev_index;

            /* Copy the multicast address */
            NU_BLOCK_COPY(mreq_struct6.ip6_mreq_multiaddr,
                          sockptr->s_moptions_v6->
                          multio_v6_membership[multi_members - 1]->ipm6_addr,
                          IP6_ADDR_LEN);

            /* Leave the multicast group */
            IP6_Process_Multicast_Listen(socketd, mreq_struct6.ip6_mreq_dev_index,
                                         mreq_struct6.ip6_mreq_multiaddr,
                                         MULTICAST_FILTER_INCLUDE, NU_NULL, 0);
        }

        /* If unsetting all multicast options did not deallocate the
         * multicast data structure, deallocate it now.
         */
        if (sockptr->s_moptions_v6)
        {
            if (NU_Deallocate_Memory(sockptr->s_moptions_v6) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory", NERR_SEVERE,
                               __FILE__, __LINE__);
            else
                sockptr->s_moptions_v6 = NU_NULL;
        }
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    if (sockptr->s_moptions_v4)
    {
        /* Set the multicast options to default values so the memory
         * will be deallocated if the socket is a member of any groups.
         */
        sockptr->s_moptions_v4->multio_device = NU_NULL;
        sockptr->s_moptions_v4->multio_ttl = IP_DEFAULT_MULTICAST_TTL;

        /* Leave each multicast group */
        for (multi_members = sockptr->s_moptions_v4->multio_num_mem_v4;
             multi_members > 0; multi_members--)
        {
            mreq_struct.sck_inaddr = IP_ADDR_ANY;

            mreq_struct.sck_multiaddr =
                GET32(sockptr->s_moptions_v4->
                      multio_v4_membership[multi_members - 1]->ipm_addr, 0);

            /* Leave the multicast group */
            IP_Process_Multicast_Listen(socketd, (UINT8 *)&mreq_struct.sck_inaddr,
                                        sockptr->s_moptions_v4->
                                        multio_v4_membership[multi_members - 1]->
                                        ipm_addr, MULTICAST_FILTER_INCLUDE,
                                        NU_NULL, 0);
        }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

        /* If unsetting all multicast socket options did not result
         * in the multicast data structure being deallocated, deallocate
         * it now.
         */
        if (sockptr->s_moptions_v4)
        {
            if (NU_Deallocate_Memory(sockptr->s_moptions_v4) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory", NERR_SEVERE,
                               __FILE__, __LINE__);
            else
                sockptr->s_moptions_v4 = NU_NULL;
        }
#endif
    }
#endif
#endif

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /* release the memory used by this socket */
    if (NU_Deallocate_Memory(sockptr) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory", NERR_SEVERE,
                       __FILE__, __LINE__);
#endif

    SCK_Sockets[socketd] = NU_NULL;

} /* SCK_Cleanup_Socket */
