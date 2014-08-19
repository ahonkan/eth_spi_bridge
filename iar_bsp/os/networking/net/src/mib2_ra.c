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
* FILE NAME
*
*        mib2_ra.c
*
* COMPONENT
*
*        MIB II - PHY Receive Address.
*
* DESCRIPTION
*
*       This file manages a table that contains an entry for each address
*       (broadcast, multicast, or uni-cast) for which the system will
*       receive packets/frames on a particular interface, except as
*       follows:
*
*           - For an interface operating in promiscuous mode, entries
*             are only required for those addresses for which the system
*             would receive frames where it is not operating in
*             promiscuous mode.
*
*           - For 802.5 functional addresses, only one entry is
*             required, for the address which has the functional address
*             bit ANDed with the bit mask of all functional addresses for
*             which the interface will accept frames.
*
*       A system is normally able to use any unicast address which
*       corresponds to an entry in this table as a source address.
*
* DATA STRUCTURES
*
*       NET_MIB2_RCV_ADDR_Memory
*       NET_MIB2_RCV_ADDR_Used
*
* FUNCTIONS
*
*       MIB2_Get_If_Address
*       MIB2_Get_Next_If_Address
*
* DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/
#include "networking/nu_net.h"

#if (INCLUDE_MIB2_RFC1213 == NU_TRUE)

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_If_Address
*
* DESCRIPTION
*
*       This function validates the MAC address and interface combination
*       passed in.
*
* INPUTS
*
*      if_index                 The interface index of the device.
*      *recv_addr               MAC address of interface.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_If_Address(UINT32 if_index, const UINT8 *recv_addr)
{
    /* Handle to the device. */
    DV_DEVICE_ENTRY         *dev;

    /* Status for returning success or error code. */
    INT16                   status = MIB2_UNSUCCESSFUL;

    /* Variable for holding comparison result. */
    INT                     cmp_result;

    /* Dummy variable for searching the receive address through the
       list of addresses */
    NET_MULTI               *recv_addr_entry;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = MIB2_UNSUCCESSFUL;
    }
    else
    {
        /* Getting handle to the device. */
        dev = DEV_Get_Dev_By_Index(if_index);

        /* If we got the handle to the device and that device is real
         * then proceed, otherwise return error code (default value of
         * status is an error code).
         */
        if ((dev != NU_NULL) && ((dev->dev_flags & DV_VIRTUAL_DEV) == 0))
        {
            /* Comparing interface MAC address with address passed in.. */
            cmp_result = memcmp(recv_addr, dev->dev_mac_addr,
                                MIB2_MAX_PADDRSIZE);

            /* If interface MAC address and address passed in are equal
               then return success code. */
            if (cmp_result == 0)
                status = NU_SUCCESS;

            /* If interface MAC address and address passed in are
               non-equal then search for the address in address list. */
            else
            {
                /* Searching from start of the list. */
                recv_addr_entry = dev->dev_ethermulti;

                /* Keeping on searching till end of the list. */
                while(recv_addr_entry)
                {
                    /* Comparing MAC address passed in to address in entry
                       of list. */
                    cmp_result = memcmp(recv_addr,
                                        recv_addr_entry->nm_addr,
                                        MIB2_MAX_PADDRSIZE);

                    /* If we found the equivalent entry in the list then
                       break through the loop and return success code. */
                    if (cmp_result == 0)
                    {
                        /* Returning success code. */
                        status = NU_SUCCESS;

                        /* Breaking through the loop. */
                        break;
                    }

                    /* Moving forward in the list. */
                    recv_addr_entry = recv_addr_entry->nm_next;
                }
            }
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Get_If_Address */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_Next_If_Address
*
* DESCRIPTION
*
*       This function gets next the MAC address and interface combination.
*
* INPUTS
*
*      *if_index                The interface index of the device.
*      *recv_addr               MAC address of interface.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_Next_If_Address(UINT32 *if_index, UINT8 *recv_addr)
{
    /* Handle to the interface. */
    DV_DEVICE_ENTRY         *dev;

    /* Status for returning success or error code. */
    INT16                   status = MIB2_UNSUCCESSFUL;

    /* Variable to hold comparison result. */
    INT                     cmp_result;

    /* Handle to the address entry. */
    NET_MULTI               *recv_addr_entry;

    /* Local variable to hold the current least maximum address. */
    UINT8                   temp_recv_addr[MIB2_MAX_PADDRSIZE];

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }
    else
    {
        /* Loop through the device table. */
        for (dev = DEV_Table.dv_head; (dev) ; dev = dev -> dev_next)
        {
            /* If current device is real. */
            if  (!(dev->dev_flags & DV_VIRTUAL_DEV))
            {
                /* If interface device index passed in is zero then we are
                   interested in minimum MAC address of first real device. */
                if ( (*if_index) == 0)
                {
                    /* Selecting device unicast MAC address as candidate
                       for minimum. */
                    NU_BLOCK_COPY(temp_recv_addr, dev -> dev_mac_addr,
                                  MIB2_MAX_PADDRSIZE);

                    /* Setting status to success code because we have
                       selected a candidate. */
                    status = NU_SUCCESS;
                }

                /* If we have reached at device having same interface
                   index as passed in. */
                else if (dev->dev_index == ((*if_index) - 1))
                {
                    /* Comparing unicast MAC address of interface with MAC
                       address passed in. */
                    cmp_result = memcmp (recv_addr ,  dev -> dev_mac_addr,
                                         MIB2_MAX_PADDRSIZE);

                    /* If interface unicast MAC address is greater than
                       the MAC address passed in. */
                    if (cmp_result < 0)
                    {
                        /* Selecting device unicast MAC address as
                           candidate for minimum. */
                        NU_BLOCK_COPY(temp_recv_addr, dev -> dev_mac_addr,
                                      MIB2_MAX_PADDRSIZE);

                        /* Setting status to success code because we have
                           selected a candidate. */
                        status = NU_SUCCESS;
                    }

                    else
                    {
                        continue;
                    }
                }

                /* If we have reached at interface with greater interface
                   index then passed in. */
                else if ( dev->dev_index >= (*if_index))
                {
                    /* Clearing MAC address as from now onward we are only
                       interested in any minimum MAC address of this
                       device. */
                    UTL_Zero(recv_addr, MIB2_MAX_PADDRSIZE);

                    /* Selecting device unicast MAC address as candidate
                       for minimum. */
                    NU_BLOCK_COPY(temp_recv_addr, dev -> dev_mac_addr,
                                  MIB2_MAX_PADDRSIZE);

                    /* Setting status to success code because we have
                       selected a candidate. */
                    status = NU_SUCCESS;
                }

                /* If we are currently at a device with lesser interface
                   index as passed in then move to next device. */
                else
                {
                    continue;
                }

                /* Loop through device multicast address for searching
                   the minimum greater MAC address than passed in. */
                for (recv_addr_entry = dev -> dev_ethermulti;
                     (recv_addr_entry);
                     recv_addr_entry = recv_addr_entry->nm_next)
                {
                    /* Comparing current multicast MAC address with MAC
                       address passed in. */
                    cmp_result = memcmp(recv_addr,
                                        recv_addr_entry -> nm_addr,
                                        MIB2_MAX_PADDRSIZE);

                    /* If current multicast MAC address is greater than
                       the MAC address passed, then current multicast MAC
                       address has the potential of being minimum greater
                       MAC address. */
                    if (cmp_result < 0)
                    {
                        /* Comparing current candidate for being
                           minimum greater MAC address with current
                           multicast MAC address. */
                        cmp_result = memcmp(temp_recv_addr,
                                            recv_addr_entry->nm_addr,
                                            MIB2_MAX_PADDRSIZE);

                        /* If current multicast MAC address is better
                           candidate for being minimum greater MAC
                           address then update the current candidate. */
                        if (cmp_result > 0)
                        {
                            /* Updating current candidate. */
                            NU_BLOCK_COPY(temp_recv_addr,
                                          recv_addr_entry -> nm_addr,
                                          MIB2_MAX_PADDRSIZE);
                        }
                    }
                }

                /* Updating interface index. */
                (*if_index) = dev ->dev_index + 1;

                /* Selecting current candidate as minimum greater MAC
                   address. */
                NU_BLOCK_COPY(recv_addr, temp_recv_addr,
                              MIB2_MAX_PADDRSIZE);

                /* Breaking out of the loop. */
                break;
            }
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning success or error code. */
    return (status);

} /* MIB2_Get_Next_If_Address */


#endif  /* (INCLUDE_MIB2_RFC1213 == NU_TRUE) */

