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

/***********************************************************************
*
*   FILE NAME
*
*       pmtu.c
*
*   COMPONENT
*
*       PMTU - Path MTU Discovery
*
*   DESCRIPTION
*
*       This file contains the implementation of the PMTU protocol.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PMTU_Init
*       PMTU_Increase_PMTU
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_PMTU_DISCOVERY == NU_TRUE)

VOID PMTU_Increase_PMTU(TQ_EVENT, UNSIGNED, UNSIGNED);

TQ_EVENT    PMTU_Timer;

/***********************************************************************
*
*   FUNCTION
*
*       PMTU_Init
*
*   DESCRIPTION
*
*       Initialize the Path MTU module.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID PMTU_Init(VOID)
{
    /* If the user set the PMTU_INC_TIME to 0xffffffff, then we should
     * never try to increase the PMTU for any connection, so don't
     * register the events or set the timers.
     */
    if (PMTU_INC_TIME != 0xffffffffUL)
    {
        /* Register the event to increase the PMTU */
        if (EQ_Register_Event(PMTU_Increase_PMTU, &PMTU_Timer) != NU_SUCCESS)
            NLOG_Error_Log("Failed to register PMTU timer event",
                           NERR_SEVERE, __FILE__, __LINE__);

        /* Set the timer to increase the PMTU of host entries for the IPv4
         * and IPv6 Routing Table entries.
         */
#if (INCLUDE_IPV4 == NU_TRUE)

        TQ_Timerset(PMTU_Timer, NU_FAMILY_IP, PMTU_INC_TIME, 0);

#endif

#if (INCLUDE_IPV6 == NU_TRUE)

        TQ_Timerset(PMTU_Timer, NU_FAMILY_IP6, PMTU_INC_TIME, 0);

#endif
    }

} /* PMTU_Init */

/***********************************************************************
*
*   FUNCTION
*
*       PMTU_Increase_PMTU
*
*   DESCRIPTION
*
*       This routine processes the event to increase the PMTU of the
*       entries in the IPv4 or IPv6 Routing Table.
*
*   INPUTS
*
*       event                   The event that occurred.
*       family                  The family of the Routing Table to
*                               check.
*       extra_data              Unused.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID PMTU_Increase_PMTU(TQ_EVENT event, UNSIGNED family, UNSIGNED extra_data)
{
    ROUTE_ENTRY *current_route;
    UNSIGNED    due_time, increase_time = PMTU_INC_INTERVAL;

#if (INCLUDE_IPV6 == NU_TRUE)
#ifndef IPV6_VERSION_COMP
    UINT8       current_address[MAX_ADDRESS_SIZE];
#endif
#endif

    UNUSED_PARAMETER(event);
    UNUSED_PARAMETER(extra_data);

    /* Get a pointer to the first route in the Route Tree */
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (family == NU_FAMILY_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
#ifdef IPV6_VERSION_COMP
        current_route = (ROUTE_ENTRY*)RTAB6_Find_Next_Route_Entry(NU_NULL);
#else
    {
        /* Zero out the current address to get the first address in the
         * table.
         */
        memset(current_address, 0, MAX_ADDRESS_SIZE);

        current_route = (ROUTE_ENTRY*)RTAB6_Find_Next_Route(current_address);
    }
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
        current_route = (ROUTE_ENTRY*)RTAB4_Find_Next_Route_Entry(NU_NULL);
#endif

    while (current_route)
    {
        /* If the PMTU of this host entry is due to be increased, increase
         * it and reset the timestamp.
         */
        if (current_route->rt_pmtu_timestamp != 0)
        {
            /* Determine when the timer expires */
            due_time = TQ_Check_Duetime(current_route->rt_pmtu_timestamp);

            /* If the timer has expired, increase the PMTU */
            if (due_time == 0)
            {
                /* Set the PMTU to the first hop MTU. */
                current_route->rt_path_mtu =
                    current_route->rt_device->dev_mtu;

                /* Reset the timestamp */
                current_route->rt_pmtu_timestamp = 0;

                /* Stop setting the DF bit */
                current_route->rt_flags &= ~RT_STOP_PMTU;
            }

            /* Otherwise, if the timer is due to expire before the last
             * saved increase_time, set increase_time to the time this
             * timer is set to expire.
             */
            else if (due_time < increase_time)
                increase_time = due_time;
        }

        /* Get a pointer to the next route */
#if (INCLUDE_IPV6 == NU_TRUE)
        if (family == NU_FAMILY_IP6)
#ifdef IPV6_VERSION_COMP
            current_route =
                (ROUTE_ENTRY*)RTAB6_Find_Next_Route_Entry(current_route);
#else
            current_route =
                (ROUTE_ENTRY*)RTAB6_Find_Next_Route(current_route->
                                                    rt_route_node->
                                                    rt_ip_addr);
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
            current_route =
                (ROUTE_ENTRY*)RTAB4_Find_Next_Route_Entry(current_route);
#endif
    }

#if (INCLUDE_TCP == NU_TRUE)

    /* Notify TCP to increase the PMTU of the respective connections */
    TCP_PMTU_Increase_SMSS();

#endif

    /* Set the First Increase Timer to run again. */
    TQ_Timerset(PMTU_Timer, family, increase_time, 0);

} /* PMTU_Increase_PMTU */

#endif
