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
*   FILENAME                                         
*
*       dhcp6_shtdwn.c                                       
*
*   DESCRIPTION
*
*       This file contains the DHCPv6 routines for shutting down the
*       DHCPv6 client module.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Dhcp6_Shutdown
*       DHCP6_Find_TX_Struct_By_DSID
*
*   DEPENDENCIES
*
*       nu_net.h
*       nu_net6.h
*
**************************************************************************/

#include "networking/nu_net.h"
#include "networking/nu_net6.h"

#if (INCLUDE_DHCP6 == NU_TRUE)

STATIC  DHCP6_TX_STRUCT *DHCP6_Find_TX_Struct_By_DSID(UINT8);

/******************************************************************************
*
*   FUNCTION
*
*       NU_Dhcp6_Shutdown
*
*   DESCRIPTION
*
*       This function shuts down DHCPv6 processing for a specific interface.
*       Any previous allocated DHCPv6 addresses will not undergo the DHCPv6
*       release process.  To release the addresses, the user must issue a call 
*       to NU_Remove_IP_From_Device for each IPv6 address on the interface.
*
*   INPUTS
*
*       dev_index               The index of the interface to shut down.
*       flags                   Currently unused.
*
*   OUTPUTS
*
*      NU_SUCCESS               Successful operation.
*      NU_INVALID_PARM          An input parameter is invalid.
*
******************************************************************************/
STATUS NU_Dhcp6_Shutdown(UINT32 dev_index, UINT8 flags)
{
    DHCP6_STRUCT    *ds_ptr;
    DEV6_IF_ADDRESS *addr_ptr = NU_NULL;
    DV_DEVICE_ENTRY *dv_ptr;
    DHCP6_TX_STRUCT *tx_ptr;
    STATUS          status;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    UNUSED_PARAMETER(flags);

    /* Obtain the DHCPv6 client semaphore. */
    status = NU_Obtain_Semaphore(&DHCP6_Cli_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Find the DHCPv6 structure associated with the interface. */
        ds_ptr = DHCP6_Find_Struct_By_Dev(dev_index);

        /* If a structure was found. */
        if (ds_ptr)
        {
            /* Clear all timers associated with this structure. */
            tx_ptr = DHCP6_Find_TX_Struct_By_DSID(ds_ptr->dhcp6_id);

            while (tx_ptr)
            {
                /* Free the transmission structure entry. */
                DHCP6_Free_TX_Struct(tx_ptr);

                /* Get a pointer to the next tx_ptr structure. */
                tx_ptr = DHCP6_Find_TX_Struct_By_DSID(ds_ptr->dhcp6_id);
            }

            /* Free any addresses that are currently being released or
             * declined.
             */
            DHCP6_Free_Address(ds_ptr);

			/* Deallocate any memory that has been allocated for the options.
             */
            if (ds_ptr->dhcp6_opts)
            {
                /* Deallocate the memory. */
                if (NU_Deallocate_Memory(ds_ptr->dhcp6_opts) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate memory for the options",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            /* Deallocate any memory allocated for the list of user classes.
             */
            if (ds_ptr->dhcp6_user_classes)
            {
                /* Deallocate the memory. */
                if (NU_Deallocate_Memory(ds_ptr->dhcp6_user_classes) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate memory for user classes", 
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                /* Set the count back to zero. */
                ds_ptr->dhcp6_user_class_count = 0;
            }

            /* Remove the entry from the list. */
            DLL_Remove(&DHCP6_Structs, ds_ptr);

            /* Free the DHCPv6 structure. */
            NU_Deallocate_Memory(ds_ptr);

            /* Get the NET semaphore. */
            status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Unset the timer to renew the IA_NA. */
                TQ_Timerunset(DHCP6_Renew_IA_NA_Event, TQ_CLEAR_EXACT, 
                              dev_index, DHCP6_RENEW);

                /* Get a pointer to the device structure. */
                dv_ptr = DEV_Get_Dev_By_Index(dev_index);

                if (dv_ptr)
                {
                    /* Get a pointer to the first address in the list. */
                    if (dv_ptr->dev_flags & DV6_IPV6)
                        addr_ptr = dv_ptr->dev6_addr_list.dv_head;

                    /* Process each IP address, deleting all DHCPv6 obtained
                     * addresses.
                     */
                    while (addr_ptr)
                    {   
                        /* If this address was obtained using DHCPv6. */
                        if (addr_ptr->dev6_addr_flags & ADDR6_DHCP_ADDR)
                        {
                            /* Clear the DHCP flag so the DHCPv6 release
                             * process is not invoked for this address.
                             * By invoking the release process, we will
                             * be forced to restart the DHCPv6 module, 
                             * which is currently being shutdown.
                             */
                            addr_ptr->dev6_addr_flags &= ~ADDR6_DHCP_ADDR;

                            /* Delete the IP address. */
                            DEV6_Delete_IP_From_Device(addr_ptr);
                        }

                        /* Get a pointer to the next address. */
                        addr_ptr = addr_ptr->dev6_next;
                    }
                }

                else
                {
                    status = NU_INVALID_PARM;
                }

                /* Release the NET semaphore. */
                NU_Release_Semaphore(&TCP_Resource);
            }
        }

        else
        {
            status = NU_INVALID_PARM;
        }

        /* Release the DHCPv6 client semaphore. */
        NU_Release_Semaphore(&DHCP6_Cli_Resource);
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (status);

} /* NU_Dhcp6_Shutdown */

/*************************************************************************
*
*   FUNCTION
*
*       DHCP6_Find_TX_Struct_By_DSID
*
*   DESCRIPTION
*
*       This routine finds a transmission structure based on the ID of the
*       DHCP6_STRUCT associated with this DHCP6_TX_STRUCT.
*
*   INPUTS
*
*       ds_id                   The ID of the DHCP6_STRUCT associated with
*                               the target DHCP6_TX_STRUCT.
*
*   OUTPUTS
*
*       A pointer to the DHCP6_TX_STRUCT or NU_NULL if no matching structure
*       was found.
*
*************************************************************************/
STATIC DHCP6_TX_STRUCT *DHCP6_Find_TX_Struct_By_DSID(UINT8 ds_id)
{
    DHCP6_TX_STRUCT    *tx_ptr = NU_NULL;
    INT                 i;

    /* Loop through the list of structures until a match is found. */
    for (i = 0; i < DHCP6_SIM_TX_COUNT; i++)
    {
        /* If this is the target entry, return it. */
        if (DHCP6_Tx_List[i].dhcp6_ds_id == ds_id)
        {
            tx_ptr = &DHCP6_Tx_List[i];

            break;
        }
    }

    return (tx_ptr);

} /* DHCP6_Find_TX_Struct_By_DSID */

#endif
