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
*   FILENAME
*
*       sck_stxso.c
*
*   DESCRIPTION
*
*       This file contains the routines for setting Sticky Options
*       on a socket.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SCK_Set_TX_Sticky_Options
*       SCK_Clear_TX_Sticky_Options
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/nu_net6.h"

STATUS  SCK_Clear_TX_Sticky_Options(tx_ancillary_data *, const CHAR *, INT);
#endif

/***********************************************************************
*
*   FUNCTION
*
*       SCK_Set_TX_Sticky_Options
*
*   DESCRIPTION
*
*       This function sets Sticky Options for a socket.
*
*   INPUTS
*
*       **tx_sticky_options_ptr A pointer to the sticky options for the
*                               port.
*       *buffer                 A pointer to a buffer containing the new
*                               value of the Sticky Option.
*       length                  The length of the buffer.
*       option                  The Sticky Option to set.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVAL                The option is not recognized.
*       NU_MEM_ALLOC            Insufficient memory to create the
*                               Sticky Option buffer or the Sticky
*                               Option buffer for the socket is full.
*
*************************************************************************/
STATUS SCK_Set_TX_Sticky_Options(tx_ancillary_data **tx_sticky_options_ptr,
                                 const VOID *buffer,
                                 INT length, INT option)
{
    STATUS              status;
#if (INCLUDE_IPV6 == NU_TRUE)
    tx_ancillary_data   *tx_sticky_options;
#else
    UNUSED_PARAMETER(buffer);
    UNUSED_PARAMETER(length);
#endif

#if (INCLUDE_IPV6 == NU_FALSE)
    UNUSED_PARAMETER(tx_sticky_options_ptr);
#endif

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /* If this is the first time Sticky Options have been set on
     * the socket, allocate memory for the buffer.
     */
    if (!(*tx_sticky_options_ptr))
    {
        if (NU_Allocate_Memory(MEM_Cached, (VOID **)tx_sticky_options_ptr,
                               (UNSIGNED)(STICKY_OPTIONS_MAX_SIZE),
                               NU_NO_SUSPEND) != NU_SUCCESS)
            return (NU_MEM_ALLOC);

        UTL_Zero(*tx_sticky_options_ptr, sizeof(tx_ancillary_data));

        /* Set the pointer to the beginning of the memory that will hold
         * the buffer of options.
         */
        (*tx_sticky_options_ptr)->tx_buff =
            (UINT8*)((CHAR HUGE*)*tx_sticky_options_ptr + sizeof(tx_ancillary_data));

#if (INCLUDE_IPV6 == NU_TRUE)
        tx_sticky_options = *tx_sticky_options_ptr;
#endif
    }

#if (INCLUDE_IPV6 == NU_TRUE)
    else
        tx_sticky_options = *tx_sticky_options_ptr;
#endif

#endif

#if (INCLUDE_IPV6 == NU_TRUE)

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    status = NU_SUCCESS;
#endif

    switch (option)
    {
        case IPV6_PKTINFO:

            /* If the caller is clearing the previous IPV6_PKTINFO option set */
            if (((in6_pktinfo*)buffer)->ipi6_ifindex == IP6_UNSPECIFIED)
            {
                /* If the option has been set as a sticky option, unset it */
                if (tx_sticky_options->tx_interface_index)
                {
                    /* Unset the option and reposition the existing options in the
                     * buffer so as to fill in any gap left by clearing the option.
                     */
                    SCK_Clear_TX_Sticky_Options(tx_sticky_options,
                                                (CHAR*)tx_sticky_options->tx_interface_index,
                                                sizeof(UINT32));

                    /* Set the pointer to NU_NULL */
                    tx_sticky_options->tx_interface_index = NU_NULL;
                }
            }

            /* If this Sticky Option has already been previously set, overwrite
             * the existing value with the new value.
             */
            else if (tx_sticky_options->tx_interface_index)
            {
                /* Set the flag to indicate that the sticky option for the route
                 * has changed.
                 */
                tx_sticky_options->tx_flags |= IP6_STCKY_RT_CHNGD;

                PUT32((CHAR*)tx_sticky_options->tx_interface_index,
                      0, ((in6_pktinfo*)buffer)->ipi6_ifindex);
            }

            /* Otherwise, if there is room in the buffer for the new data */
            else if ( (tx_sticky_options->tx_buff_length + sizeof(UINT32)) <
                       STICKY_OPTIONS_MAX_SIZE)
            {
                /* Set the flag to indicate that the sticky option for the route
                 * has changed.
                 */
                tx_sticky_options->tx_flags |= IP6_STCKY_RT_CHNGD;

                /* Copy the structure starting at the end of the existing
                 * data in the buffer.
                 */
                PUT32((((CHAR HUGE*)tx_sticky_options->tx_buff) +
                      tx_sticky_options->tx_buff_length), 0,
                      ((in6_pktinfo*)buffer)->ipi6_ifindex);

                /* Set the interface index pointer */
                tx_sticky_options->tx_interface_index =
                    (UINT8*)(((CHAR HUGE*)tx_sticky_options->tx_buff) +
                                          tx_sticky_options->tx_buff_length);

                /* Increment the total number of bytes in the buffer */
                tx_sticky_options->tx_buff_length += sizeof(UINT32);
            }

            else
                status = NU_MEM_ALLOC;

            /* If the interface index setting failed, return an error */
            if (status == NU_SUCCESS)
            {
                /* Decrement the length by the size of the interface index */
                length -= sizeof(UINT32);

                /* If the caller is clearing the previous IPV6_PKTINFO option set */
                if (memcmp(((in6_pktinfo*)buffer)->ipi6_addr, IP6_ADDR_ANY.is_ip_addrs,
                           IP6_ADDR_LEN) == 0)
                {
                    /* If the option has been set as a sticky option, unset it */
                    if (tx_sticky_options->tx_source_address)
                    {
                        /* Unset the option and reposition the existing options in the
                         * buffer so as to fill in any gap left by clearing the option.
                         */
                        SCK_Clear_TX_Sticky_Options(tx_sticky_options,
                                                    (CHAR*)tx_sticky_options->tx_source_address,
                                                    length);

                        /* Set the pointers to NU_NULL */
                        tx_sticky_options->tx_source_address = NU_NULL;
                    }
                }

                /* If this Sticky Option has already been previously set, overwrite
                 * the existing value with the new value.
                 */
                else if (tx_sticky_options->tx_source_address)
                    memcpy((CHAR*)tx_sticky_options->tx_source_address,
                           ((in6_pktinfo*)buffer)->ipi6_addr, (unsigned int)length);

                /* Otherwise, if there is room in the buffer for the new data */
                else if ( (tx_sticky_options->tx_buff_length + length) <
                           STICKY_OPTIONS_MAX_SIZE)
                {
                    /* Copy the structure starting at the end of the existing
                     * data in the buffer.
                     */
                    memcpy(((CHAR HUGE*)tx_sticky_options->tx_buff) +
                           tx_sticky_options->tx_buff_length,
                           ((in6_pktinfo*)buffer)->ipi6_addr, (unsigned int)length);

                    /* Set the source address pointer */
                    tx_sticky_options->tx_source_address =
                        (UINT8*)(((CHAR HUGE*)tx_sticky_options->tx_buff) +
                                              tx_sticky_options->tx_buff_length);

                    /* Increment the total number of bytes in the buffer */
                    tx_sticky_options->tx_buff_length =
                        (UINT16)(tx_sticky_options->tx_buff_length + length);
                }

                else
                    status = NU_MEM_ALLOC;
            }

            break;

        case IPV6_NEXTHOP:

            /* If the caller is clearing the previous IPV6_NEXTHOP option set */
            if (length == 0)
            {
                /* If the option has been set as a sticky option, unset it */
                if (tx_sticky_options->tx_next_hop)
                {
                    /* Unset the option and reposition the existing options in the
                     * buffer so as to fill in any gap left by clearing the option.
                     */
                    SCK_Clear_TX_Sticky_Options(tx_sticky_options,
                                                (CHAR*)tx_sticky_options->tx_next_hop,
                                                sizeof(struct addr_struct));

                    /* Set the pointer to NU_NULL */
                    tx_sticky_options->tx_next_hop = NU_NULL;
                }
            }

            /* If this Sticky Option has already been previously set, overwrite
             * the existing value with the new value.
             */
            else if (tx_sticky_options->tx_next_hop)
            {
                /* Set the flag to indicate that the sticky option for the route
                 * has changed.
                 */
                tx_sticky_options->tx_flags |= IP6_STCKY_RT_CHNGD;

                memcpy((CHAR*)tx_sticky_options->tx_next_hop, buffer,
                       (unsigned int)length);
            }

            /* If there is room in the buffer for the data */
            else if ( (tx_sticky_options->tx_buff_length + length) <
                       STICKY_OPTIONS_MAX_SIZE)
            {
                /* Set the flag to indicate that the sticky option for the route
                 * has changed.
                 */
                tx_sticky_options->tx_flags |= IP6_STCKY_RT_CHNGD;

                /* Copy the structure starting at the end of the existing
                 * data in the buffer.
                 */
                memcpy(((CHAR HUGE*)tx_sticky_options->tx_buff) +
                       tx_sticky_options->tx_buff_length, buffer,
                       (unsigned int)length);

                /* Set the next-hop pointer to point into the buffer */
                tx_sticky_options->tx_next_hop =
                    (struct addr_struct*)
                    (((CHAR HUGE*)tx_sticky_options->tx_buff) +
                                  tx_sticky_options->tx_buff_length);

                /* Increment the total number of bytes in the buffer */
                tx_sticky_options->tx_buff_length =
                    (UINT16)(tx_sticky_options->tx_buff_length + length);
            }

            else
                status = NU_MEM_ALLOC;

            break;

        case IPV6_TCLASS:

            /* If the caller is clearing the previous IPV6_TCLASS option set */
            if (*(INT *)buffer == -1)
            {
                /* If the option has been set as a sticky option, unset it */
                if (tx_sticky_options->tx_traffic_class)
                {
                    /* Unset the option and reposition the existing options in the
                     * buffer so as to fill in any gap left by clearing the option.
                     */
                    SCK_Clear_TX_Sticky_Options(tx_sticky_options,
                                                (CHAR*)tx_sticky_options->tx_traffic_class,
                                                sizeof(INT));

                    /* Set the pointer to NU_NULL */
                    tx_sticky_options->tx_traffic_class = NU_NULL;
                }
            }

            /* If this Sticky Option has already been previously set, overwrite
             * the existing value with the new value.
             */
            else if (tx_sticky_options->tx_traffic_class)
                *tx_sticky_options->tx_traffic_class = (UINT8)*(INT *)buffer;

            /* If there is room in the buffer for the data */
            else if ( (tx_sticky_options->tx_buff_length + length) <
                       STICKY_OPTIONS_MAX_SIZE)
            {
                /* Copy the structure starting at the end of the existing
                 * data in the buffer.
                 */
                *((CHAR HUGE*)(tx_sticky_options->tx_buff +
                               tx_sticky_options->tx_buff_length)) = (CHAR)*(INT *)buffer;

                /* Set the traffic class pointer to point into the buffer */
                tx_sticky_options->tx_traffic_class =
                    (UINT8*)(((CHAR HUGE*)tx_sticky_options->tx_buff) +
                                          tx_sticky_options->tx_buff_length);

                /* Increment the total number of bytes in the buffer */
                tx_sticky_options->tx_buff_length =
                    (UINT16)(tx_sticky_options->tx_buff_length + length);
            }

            else
                status = NU_MEM_ALLOC;

            break;

        case IPV6_RTHDR:

            /* If the caller is clearing or overwriting the previous
             * IPV6_RTHDR option set, unset the previous option.
             */
            if ( (buffer) && (tx_sticky_options->tx_route_header) )
            {
                /* Unset the option and reposition the existing options in the
                 * buffer so as to fill in any gap left by clearing the option.
                 */
                SCK_Clear_TX_Sticky_Options(tx_sticky_options,
                                            (CHAR*)tx_sticky_options->tx_route_header,
                                            (sizeof(struct ip6_rthdr0) + (((struct ip6_rthdr*)buffer)->ip6r_len << 3)));

                /* Set the pointer to NU_NULL */
                tx_sticky_options->tx_route_header = NU_NULL;
            }

            if ( (buffer) && (length != 0) )
            {
                /* If there is room in the buffer for the data */
                if ((tx_sticky_options->tx_buff_length + length) <
                     STICKY_OPTIONS_MAX_SIZE)
                {
                    /* Copy the structure starting at the end of the existing
                     * data in the buffer.
                     */
                    memcpy(((CHAR HUGE*)tx_sticky_options->tx_buff) +
                           tx_sticky_options->tx_buff_length, buffer,
                           (unsigned int)length);

                    /* Set the router header pointer to point into the buffer */
                    tx_sticky_options->tx_route_header =
                        (UINT8*)(((CHAR HUGE*)tx_sticky_options->tx_buff) +
                                              tx_sticky_options->tx_buff_length);

                    /* Increment the total number of bytes in the buffer */
                    tx_sticky_options->tx_buff_length =
                        (UINT16)(tx_sticky_options->tx_buff_length + length);
                }

                else
                    status = NU_MEM_ALLOC;
            }

            break;

        case IPV6_HOPOPTS:

            /* If the caller is clearing or overwriting the previous
             * IPV6_HOPOPTS option set, unset the previous option.
             */
            if (tx_sticky_options->tx_hop_opt)
            {
                /* Unset the option and reposition the existing options in the
                 * buffer so as to fill in any gap left by clearing the option.
                 */
                SCK_Clear_TX_Sticky_Options(tx_sticky_options,
                                            (CHAR*)tx_sticky_options->tx_hop_opt,
                                            (8 + (((struct ip6_hbh*)tx_sticky_options->tx_hop_opt)->ip6h_len << 3)));

                /* Set the pointer to NU_NULL */
                tx_sticky_options->tx_hop_opt = NU_NULL;
            }

            if (length != 0)
            {
                /* If there is room in the buffer for the data */
                if ((tx_sticky_options->tx_buff_length + length) <
                     STICKY_OPTIONS_MAX_SIZE)
                {
                    /* Copy the structure starting at the end of the existing
                     * data in the buffer.
                     */
                    memcpy(((CHAR HUGE*)tx_sticky_options->tx_buff) +
                           tx_sticky_options->tx_buff_length, buffer,
                           (unsigned int)length);

                    /* Set the hop-by-hop pointer to point into the buffer */
                    tx_sticky_options->tx_hop_opt =
                        (UINT8*)(((CHAR HUGE*)tx_sticky_options->tx_buff) +
                           tx_sticky_options->tx_buff_length);

                    /* Increment the total number of bytes in the buffer */
                    tx_sticky_options->tx_buff_length =
                        (UINT16)(tx_sticky_options->tx_buff_length + length);
                }

                else
                    status = NU_MEM_ALLOC;
            }

            break;

        case IPV6_DSTOPTS:

            /* If the caller is clearing or overwriting the previous
             * IPV6_DSTOPTS option set, unset the previous option.
             */
            if (tx_sticky_options->tx_dest_opt)
            {
                /* Unset the option and reposition the existing options in the
                 * buffer so as to fill in any gap left by clearing the option.
                 */
                SCK_Clear_TX_Sticky_Options(tx_sticky_options,
                                            (CHAR*)tx_sticky_options->tx_dest_opt,
                                            (8 + (((struct ip6_dest*)tx_sticky_options->tx_dest_opt)->ip6d_len << 3)));

                /* Set the pointer to NU_NULL */
                tx_sticky_options->tx_dest_opt = NU_NULL;
            }

            if (length != 0)
            {
                /* If there is room in the buffer for the data */
                if ((tx_sticky_options->tx_buff_length + length) <
                     STICKY_OPTIONS_MAX_SIZE)
                {
                    /* Copy the structure starting at the end of the existing
                     * data in the buffer.
                     */
                    memcpy(((CHAR HUGE*)tx_sticky_options->tx_buff) +
                           tx_sticky_options->tx_buff_length, buffer,
                           (unsigned int)length);

                    /* Set the destination options pointer to point into the buffer */
                    tx_sticky_options->tx_dest_opt =
                        (UINT8*)(((CHAR HUGE*)tx_sticky_options->tx_buff) +
                           tx_sticky_options->tx_buff_length);

                    /* Increment the total number of bytes in the buffer */
                    tx_sticky_options->tx_buff_length =
                        (UINT16)(tx_sticky_options->tx_buff_length + length);
                }

                else
                    status = NU_MEM_ALLOC;
            }

            break;

        case IPV6_RTHDRDSTOPTS:

            /* If the caller is clearing or overwriting the previous
             * IPV6_RTHDRDSTOPTS option set, unset the previous option.
             */
            if (tx_sticky_options->tx_rthrdest_opt)
            {
                /* Unset the option and reposition the existing options in the
                 * buffer so as to fill in any gap left by clearing the option.
                 */
                SCK_Clear_TX_Sticky_Options(tx_sticky_options,
                                            (CHAR*)tx_sticky_options->tx_rthrdest_opt,
                                            (8 + (((struct ip6_dest*)tx_sticky_options->tx_rthrdest_opt)->ip6d_len << 3)));

                /* Set the pointer to NU_NULL */
                tx_sticky_options->tx_rthrdest_opt = NU_NULL;
            }

            if (length != 0)
            {
                /* If there is room in the buffer for the data */
                if ((tx_sticky_options->tx_buff_length + length) <
                     STICKY_OPTIONS_MAX_SIZE)
                {
                    /* Copy the structure starting at the end of the existing
                     * data in the buffer.
                     */
                    memcpy(((CHAR HUGE*)tx_sticky_options->tx_buff) +
                           tx_sticky_options->tx_buff_length, buffer,
                           (unsigned int)length);

                    /* Set the destination options pointer to point into the buffer */
                    tx_sticky_options->tx_rthrdest_opt =
                        (UINT8*)(((CHAR HUGE*)tx_sticky_options->tx_buff) +
                           tx_sticky_options->tx_buff_length);

                    /* Increment the total number of bytes in the buffer */
                    tx_sticky_options->tx_buff_length =
                        (UINT16)(tx_sticky_options->tx_buff_length + length);
                }

                else
                    status = NU_MEM_ALLOC;
            }

            break;

        default:

            status = NU_INVAL;
    }
#else

    UNUSED_PARAMETER(option);
    status = NU_INVAL;

#endif

    return (status);

} /* SCK_Set_TX_Sticky_Options */

#if (INCLUDE_IPV6 == NU_TRUE)
/***********************************************************************
*
*   FUNCTION
*
*       SCK_Clear_TX_Sticky_Options
*
*   DESCRIPTION
*
*       This function clears Sticky Options for a socket and repositions
*       the Sticky Option buffer as necessary to fill in any gap left
*       by the cleared Sticky Option.
*
*   INPUTS
*
*       *sockptr                A pointer to the socket structure.
*       *buffer                 A pointer to the buffer containing the
*                               Sticky Option to clear.
*       length                  The number of bytes to clear.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*
*************************************************************************/
STATUS SCK_Clear_TX_Sticky_Options(tx_ancillary_data *tx_sticky_options,
                                   const CHAR *buffer, INT length)
{
    INT16   memory_above;
    INT16   memory_below;

    /* Determine how many bytes of data there are between the memory being
     * deleted and the beginning of the buffer.
     */
    memory_above =
        (INT16)((CHAR HUGE*)buffer - (CHAR HUGE*)tx_sticky_options->tx_buff);

    /* Determine how many bytes of data there are between the end of the
     * memory being deleted and the end of the total memory in the buffer.
     */
    memory_below =
        (INT16)(tx_sticky_options->tx_buff_length - memory_above - length);

    /* If there is any data below the data that is being deleted, move it
     * up to fill in the empty memory area.
     */
    if (memory_below)
        memmove((VOID *)((CHAR HUGE*)tx_sticky_options->tx_buff + memory_above),
                (VOID *)((CHAR HUGE*)buffer + length), (unsigned int)memory_below);

    /* Adjust the pointers as necessary */
    if ((CHAR*)tx_sticky_options->tx_source_address > buffer)
        tx_sticky_options->tx_source_address -= length;

    if ((CHAR*)tx_sticky_options->tx_hop_limit > buffer)
        tx_sticky_options->tx_hop_limit -= length;

    if ((CHAR*)tx_sticky_options->tx_next_hop > buffer)
        tx_sticky_options->tx_next_hop -= length;

    if ((CHAR*)tx_sticky_options->tx_interface_index > buffer)
        tx_sticky_options->tx_interface_index -= length;

    if ((CHAR*)tx_sticky_options->tx_traffic_class > buffer)
        tx_sticky_options->tx_traffic_class -= length;

    if ((CHAR*)tx_sticky_options->tx_hop_opt > buffer)
        tx_sticky_options->tx_hop_opt -= length;

    if ((CHAR*)tx_sticky_options->tx_dest_opt > buffer)
        tx_sticky_options->tx_dest_opt -= length;

    if ((CHAR*)tx_sticky_options->tx_route_header > buffer)
        tx_sticky_options->tx_route_header -= length;

    /* Decrement the number of bytes in the buffer. */
    tx_sticky_options->tx_buff_length =
        (UINT16)(tx_sticky_options->tx_buff_length - length);

    return (NU_SUCCESS);

} /* SCK_Clear_TX_Sticky_Options */

#endif
