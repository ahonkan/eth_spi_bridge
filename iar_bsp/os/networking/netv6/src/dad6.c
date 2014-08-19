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
/* Portions of this program were written by: */
/*************************************************************************
*
* Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000 and 2001 WIDE Project.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of the project nor the names of its contributors
*    may be used to endorse or promote products derived from this
*    software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       dad6.c
*
*   DESCRIPTION
*
*       This file contains the functions to perform Duplicate Address
*       Detection on an interface.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       nd6_dad_initialize
*       nd6_dad_find
*       nd6_dad_starttimer
*       nd6_dad_stoptimer
*       nd6_dad_start
*       nd6_dad_stop
*       nd6_dad_event
*       nd6_dad_timer
*       nd6_dad_duplicated
*       nd6_dad_ns_output
*       nd6_dad_ns_input
*       nd6_dad_na_input
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       dad6.h
*       nd6nsol.h
*
*************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/dad6.h"
#include "networking/nd6nsol.h"
#include "networking/nu_net6.h"

#if (INCLUDE_DAD6 == NU_TRUE)

static  DAD_LIST    *DAD_Root;
static  int         dad_init = 0;
static  int         dad_maxtry = 15;    /* max # of *tries* to transmit DAD packet */

static  TQ_EVENT    ND6_DAD_Event;

STATIC struct dadq *nd6_dad_find(const UINT32 addr_id);
STATIC void nd6_dad_starttimer(const UINT32 dev_index, UINT32 timeout);
STATIC void nd6_dad_stoptimer(const UINT32 dev_index);
STATIC void nd6_dad_ns_output(DADQ *dp, const DEV6_IF_ADDRESS *ifa);

extern UINT8    IP6_Solicited_Node_Multi[];

/*************************************************************************
*
*   FUNCTION
*
*       nd6_dad_initialize
*
*   DESCRIPTION
*
*       This function initializes the data structures used for Duplicate
*       Address Detection.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID nd6_dad_initialize(VOID)
{
    /* Register the event to trigger a DAD session */
    if (EQ_Register_Event(nd6_dad_event, &ND6_DAD_Event) != NU_SUCCESS)
        NLOG_Error_Log("Failed to register DAD event", NERR_SEVERE,
                       __FILE__, __LINE__);

} /* nd6_dad_initialize */

/*************************************************************************
*
*   FUNCTION
*
*       nd6_dad_find
*
*   DESCRIPTION
*
*       This function finds a DAD record matching the passed in
*       address structure.
*
*   INPUTS
*
*       addr_id                 The ID of the target address.
*
*   OUTPUTS
*
*       A pointer to the matching DAD structure or NU_NULL if none
*       exists.
*
*************************************************************************/
STATIC struct dadq *nd6_dad_find(const UINT32 addr_id)
{
    DADQ    *dp;

    for (dp = DAD_Root->dad_head; dp; dp = dp->dadq_next)
    {
        if (dp->dad_addr_id == addr_id)
            break;
    }

    return (dp);

} /* nd6_dad_find */

/*************************************************************************
*
*   FUNCTION
*
*       nd6_dad_starttimer
*
*   DESCRIPTION
*
*       This function starts the DAD timer for the specified DAD
*       structure.
*
*   INPUTS
*
*       dev_index               The index of the interface for which to
*                               start the timer.
*       timeout                 The number of seconds before the timer
*                               expires.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC void nd6_dad_starttimer(const UINT32 dev_index, UINT32 timeout)
{
    if (TQ_Timerset(ND6_DAD_Event, dev_index, timeout, 0) != NU_SUCCESS)
        NLOG_Error_Log("Failed to start DAD timer", NERR_SEVERE,
                       __FILE__, __LINE__);

} /* nd6_dad_starttimer */

/*************************************************************************
*
*   FUNCTION
*
*       nd6_dad_stoptimer
*
*   DESCRIPTION
*
*       This function stops the DAD timer associated with the DAD
*       structure.
*
*   INPUTS
*
*       dev_index               The index of the interface for which to
*                               stop the timer.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC void nd6_dad_stoptimer(const UINT32 dev_index)
{
    if (TQ_Timerunset(ND6_DAD_Event, TQ_CLEAR_EXACT, dev_index, 0) != NU_SUCCESS)
        NLOG_Error_Log("Failed to stop DAD timer", NERR_SEVERE,
                       __FILE__, __LINE__);

} /* nd6_dad_stoptimer */

/*************************************************************************
*
*   FUNCTION
*
*       nd6_dad_start
*
*   DESCRIPTION
*
*       This function invokes DAD for a given address structure.
*
*   INPUTS
*
*       *ifa                    A pointer to the address structure for
*                               which to start DAD.
*       retry                   The number of DAD messages to transmit
*                               before deducing that the address is
*                               unique.
*       *tick                   The initial number of ticks for the
*                               initial DAD timer associated with the
*                               new DAD structure for the address
*                               structure provided.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
void nd6_dad_start(DEV6_IF_ADDRESS *ifa, UINT8 retry, UINT32 *tick)
{
    DADQ    *dp;
    UINT32  ntick;

    /* If the state of the address is not TENTATIVE, then DAD has
     * already been finished.
     */
    if (!(ifa->dev6_addr_state & DV6_TENTATIVE))
        return;

    /* If the address is an anycast address, do not perform DAD on
     * it.
     */
    if (ifa->dev6_addr_state & DV6_ANYCAST)
    {
        /* Indicate that the address can be used */
        ifa->dev6_addr_state &= ~DV6_TENTATIVE;
        return;
    }

    /* If DAD is disabled for the device, return */
    if (!retry)
    {
        ifa->dev6_addr_state &= ~DV6_TENTATIVE;
        return;
    }

    /* If the device is not UP, return */
    if ( (!(ifa->dev6_device->dev_flags & DV_UP)) ||
         (!(ifa->dev6_device->dev_flags2 & DV6_UP)) )
    {
        NLOG_Error_Log("Cannot begin DAD, because the device is not UP",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
        return;
    }

    if (!dad_init)
    {
        if (NU_Allocate_Memory(MEM_Cached, (VOID**)&DAD_Root, sizeof(DAD_LIST),
                               NU_NO_SUSPEND) != NU_SUCCESS)
        {
            NLOG_Error_Log("Cannot begin DAD due to lack of memory",
                           NERR_SEVERE, __FILE__, __LINE__);
            return;
        }

        UTL_Zero(DAD_Root, sizeof(DAD_LIST));

        dad_init++;
    }

    /* If a matching DAD data structure already exists for this address,
     * DAD is already in progress for this address, so return.
     */
    if (nd6_dad_find(ifa->dev6_id) != NULL)
    {
        NLOG_Error_Log("DAD already in progress for this address",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
        return;
    }

    /* Allocate memory for the new DAD data structure */
    if (NU_Allocate_Memory(MEM_Cached, (VOID**)&dp, sizeof(DADQ),
                           NU_NO_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Cannot begin DAD due to lack of memory",
                       NERR_SEVERE, __FILE__, __LINE__);
        return;
    }

    UTL_Zero(dp, sizeof(DADQ));

    /* Add the new entry to the DAD list */
    DLL_Enqueue(DAD_Root, dp);

    /* Set the parameters of the data structure */
    dp->dad_addr_id = ifa->dev6_id;
    dp->dad_count = retry;

    /* If this is the first packet to be sent, send the packet and start the
     * timer.
     */
    if (!tick)
    {
        nd6_dad_ns_output(dp, ifa);
        nd6_dad_starttimer(ifa->dev6_device->dev_index, ifa->dev6_device->dev6_retrans_timer);
    }
    /* Otherwise, compute a new timeout value, and start the timer. */
    else
    {
        if (*tick == 0)
            ntick = ICMP6_Random_Delay(ifa->dev6_ip_addr, ifa->dev6_ip_addr,
                                       IP6_RETRANS_TIMER);
        else
            ntick = *tick + ICMP6_Random_Delay(ifa->dev6_ip_addr, ifa->dev6_ip_addr,
                                               (UINT32)IP6_RETRANS_TIMER) %
                                               (UINT32)IP6_RETRANS_TIMER;

        *tick = ntick;
        nd6_dad_starttimer(ifa->dev6_device->dev_index, ntick);
    }

} /* nd6_dad_start */

/*************************************************************************
*
*   FUNCTION
*
*       nd6_dad_stop
*
*   DESCRIPTION
*
*       This function stops DAD for a given address structure.
*
*   INPUTS
*
*       *ifa                    A pointer to the address structure for
*                               which to stop DAD.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
void nd6_dad_stop(const DEV6_IF_ADDRESS *ifa)
{
    DADQ    *dp;

    /* If DAD has not been initialized, return */
    if (!dad_init)
        return;

    /* Get a pointer to the DAD data structure */
    dp = nd6_dad_find(ifa->dev6_id);

    /* If DAD has not yet been invoked for this address, return */
    if (!dp)
    {
        NLOG_Error_Log("DAD has not yet been invoked for this address",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
        return;
    }

    /* Stop the DAD timer */
    nd6_dad_stoptimer(ifa->dev6_device->dev_index);

    /* Remove the entry from the list */
    DLL_Remove(DAD_Root, dp);

    /* Deallocate the memory used for the entry */
    if (NU_Deallocate_Memory((VOID*)dp) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory for DAD structure",
                       NERR_SEVERE, __FILE__, __LINE__);

} /* nd6_dad_stop */

/*************************************************************************
*
*   FUNCTION
*
*       nd6_dad_event
*
*   DESCRIPTION
*
*       This function is called when the DAD timer for an address expires.
*
*   INPUTS
*
*       event                   The event being handled.
*       dev_index               The index of the interface on which the
*                               DAD timer expired is located.
*       unused_data             Unused.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID nd6_dad_event(TQ_EVENT event, UNSIGNED dev_index, UNSIGNED unused_data)
{
    DV_DEVICE_ENTRY *device;
    DEV6_IF_ADDRESS *current_address = NU_NULL;
    DADQ            *dadq_struct = NU_NULL;

    UNUSED_PARAMETER(event);
    UNUSED_PARAMETER(unused_data);

    /* Get a pointer to the device. */
    device = DEV_Get_Dev_By_Index(dev_index);

    if (device)
    {
        /* Get a pointer to the first address in the device's address list */
        if (device->dev6_ipv6_data)
        {
            current_address = device->dev6_addr_list.dv_head;

            /* Walk the list of addresses searching for an address on which DAD
             * is currently being performed.
             */
            while (current_address)
            {
                /* If the address state is TENTATIVE, search for a DAD structure */
                if (current_address->dev6_addr_state & DV6_TENTATIVE)
                {
                    dadq_struct = nd6_dad_find(current_address->dev6_id);

                    if (dadq_struct != NU_NULL)
                        break;
                }

                current_address = current_address->dev6_next;
            }

            /* If an entry was found, start the timer */
            if (dadq_struct)
                nd6_dad_timer(current_address, dadq_struct);

            else
            {
                NLOG_Error_Log("Address deleted while DAD in progress",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
            }
        }
    }

    else
    {
        NLOG_Error_Log("Interface deleted while DAD in progress",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

} /* nd6_dad_event */

/*************************************************************************
*
*   FUNCTION
*
*       nd6_dad_timer
*
*   DESCRIPTION
*
*       This function determines if the results of DAD have been
*       determined.
*
*   INPUTS
*
*       *ifa                    A pointer to the address structure.
*       *dp                     A pointer to the DAD structure.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
void nd6_dad_timer(DEV6_IF_ADDRESS *ifa, DADQ *dp)
{
    int     irq_level;
    int     duplicate = 0;

    if (ifa == NU_NULL)
        return;

    /* Disable interrupts */
    irq_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* If an entry was not found, the address has already been determined to
     * be a duplicate, or the address is not in the tentative state, re-enable
     * interrupts and return */
    if ( (ifa->dev6_addr_state & DV6_DUPLICATED) ||
         ((ifa->dev6_addr_state & DV6_TENTATIVE) == 0) )
    {
        NU_Local_Control_Interrupts(irq_level);
        return;
    }

    /* If the number of Neighbor Solicitation messages received is greater
     * than the maximum number of Neighbor Advertisements to send, there
     * is something wrong.
     */
    if (dp->dad_ns_tcount > dad_maxtry)
    {
        DLL_Remove(DAD_Root, dp);

        if (NU_Deallocate_Memory((VOID*)dp) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for the DAD data structure",
                           NERR_SEVERE, __FILE__, __LINE__);

        NU_Local_Control_Interrupts(irq_level);
        return;
    }

    /* If the number of Neighbor Solicitation messages sent is less than
     * the number configured to send, send another message and start
     * the DAD timer.
     */
    if (dp->dad_ns_ocount < dp->dad_count)
    {
        nd6_dad_ns_output(dp, ifa);
        nd6_dad_starttimer(ifa->dev6_device->dev_index, ifa->dev6_device->dev6_retrans_timer);
    }

    /* Otherwise, we have transmitted the maximum number of DAD packets.
     * Check to see if the address is unique.
     */
    else
    {
        /* If we have received a Neighbor Advertisement, increment the
         * duplicate counter.
         */
        if (dp->dad_na_icount)
            duplicate++;

        /* If we have received a Neighbor Solicitation, increment the
         * duplicate counter.
         */
        else if (dp->dad_ns_icount)
            duplicate++;

        /* If the address is not unique, take the appropriate actions
         * to invalidate the address.
         */
        if (duplicate)
            nd6_dad_duplicated(ifa);

        /* Otherwise, the address is unique */
        else
        {
            /* Set the address as not being tentative */
            ifa->dev6_addr_state &= ~DV6_TENTATIVE;

            /* Remove the entry from the list and deallocate the memory
             * it was using.
             */
            DLL_Remove(DAD_Root, dp);

            if (NU_Deallocate_Memory((VOID*)dp) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate the memory for the DAD data structure",
                               NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    NU_Local_Control_Interrupts(irq_level);

} /* nd6_dad_timer */

/*************************************************************************
*
*   FUNCTION
*
*       nd6_dad_duplicated
*
*   DESCRIPTION
*
*       This function sets the indicated address as a duplicate and
*       cleans up the DAD structures used for the address.
*
*   INPUTS
*
*       *ifa                    A pointer to the address structure.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
void nd6_dad_duplicated(DEV6_IF_ADDRESS *ifa)
{
    DADQ    *dp;

    /* Get a pointer to the entry in the DAD list */
    dp = nd6_dad_find(ifa->dev6_id);

    /* If an entry was found, flag it as a duplicate */
    if ( (dp != NULL) && (ifa) )
    {
        ifa->dev6_addr_state &= ~DV6_TENTATIVE;
        ifa->dev6_addr_state |= DV6_DUPLICATED;

        /* Stop the timer since we are finished with DAD */
        nd6_dad_stoptimer(ifa->dev6_device->dev_index);

        /* Remove the entry from the list and deallocate the memory
         * it was using.
         */
        DLL_Remove(DAD_Root, dp);

        if (NU_Deallocate_Memory((VOID*)dp) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate the memory for the DAD data structure",
                           NERR_SEVERE, __FILE__, __LINE__);
    }

} /* nd6_dad_duplicated */

/*************************************************************************
*
*   FUNCTION
*
*       nd6_dad_ns_output
*
*   DESCRIPTION
*
*       This function sends the Neighbor Solicitation for DAD.
*
*   INPUTS
*
*       *dp                     A pointer to the DAD structure.
*       *ifa                    A pointer to the address structure.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC void nd6_dad_ns_output(DADQ *dp, const DEV6_IF_ADDRESS *ifa)
{
    DV_DEVICE_ENTRY *ifp = ifa->dev6_device;
    UINT8           dest_addr[IP6_ADDR_LEN];

    /* Increment the number of Neighbor Solicitations received */
    dp->dad_ns_tcount++;

    /* If the interface is down, return */
    if ( (ifp->dev_flags & (DV_UP | DV_RUNNING)) != (DV_UP | DV_RUNNING) ||
         (!(ifa->dev6_device->dev_flags2 & DV6_UP)) )
    {
        NLOG_Error_Log("Cannot send a Neighbor Solicitation for DAD, because the device is down",
                       NERR_SEVERE, __FILE__, __LINE__);
        return;
    }

    /* Increment the number of Neighbor Solicitation messages send */
    dp->dad_ns_ocount++;

    NU_BLOCK_COPY(dest_addr, IP6_Solicited_Node_Multi, IP6_ADDR_LEN);

    dest_addr[13] = ifa->dev6_ip_addr[13];
    dest_addr[14] = ifa->dev6_ip_addr[14];
    dest_addr[15] = ifa->dev6_ip_addr[15];

    /* Send a Neighbor Solicitation */
    if (ND6NSOL_Output(ifp, dest_addr, ifa->dev6_ip_addr, NU_NULL, 1) != NU_SUCCESS)
        NLOG_Error_Log("Failed to Transmit Neighbor Solicitation for DAD",
                       NERR_SEVERE, __FILE__, __LINE__);

} /* nd6_dad_ns_output */

/*************************************************************************
*
*   FUNCTION
*
*       nd6_dad_ns_input
*
*   DESCRIPTION
*
*       This function determines whether a received Neighbor Solicitation
*       indicates a duplicated address.
*
*   INPUTS
*
*       *ifa                    A pointer to the address structure.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
void nd6_dad_ns_input(DEV6_IF_ADDRESS *ifa)
{
    DADQ    *dp;

    /* If the device is NULL, return */
    if (!ifa->dev6_device)
        return;

    /* Find a matching entry in the DAD list */
    dp = nd6_dad_find(ifa->dev6_id);

    /* If DAD has not began and we received a Neighbor Solicitation message,
     * someone else is already using this address.
     */
    if ( (!dp) || (dp->dad_ns_ocount == 0) )
        nd6_dad_duplicated(ifa);

    /* Otherwise, may or may not have received a duplicate */
    else
        dp->dad_ns_icount++;

} /* nd6_dad_ns_input */

/*************************************************************************
*
*   FUNCTION
*
*       nd6_dad_na_input
*
*   DESCRIPTION
*
*       This function determines whether a received Neighbor Advertisement
*       indicates a duplicated address.
*
*   INPUTS
*
*       *ifa                    A pointer to the address structure.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
void nd6_dad_na_input(DEV6_IF_ADDRESS *ifa)
{
    DADQ    *dp;

    /* If the device is NULL, return */
    if (!ifa->dev6_device)
    {
        NLOG_Error_Log("There is no device associated with the DEV6_IF_ADDRESS",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
        return;
    }

    /* Get a pointer to the device entry */
    dp = nd6_dad_find(ifa->dev6_id);

    /* If an entry exists, increment the number of Neighbor Advertisements
     * received.
     */
    if (dp)
        dp->dad_na_icount++;

    /* The address is a duplicate */
    nd6_dad_duplicated(ifa);

} /* nd6_dad_na_input */

#endif
