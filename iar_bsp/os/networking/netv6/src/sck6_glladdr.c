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
*       sck6_glladdr.c                                       
*
*   DESCRIPTION
*
*       This file contains the API routine to find the link-local address
*       associated with an interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Get_Link_Local_Addr
*
*   DEPENDENCIES
*
*       nu_net.h
*       nu_net6.h
*
**************************************************************************/

#include "networking/nu_net.h"
#include "networking/nu_net6.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Get_Link_Local_Addr
*
*   DESCRIPTION
*
*       This routine gets the link-local address for a given interface.  
*       The interface index can be found at the application layer via a 
*       call to the routine NU_IF_NameToIndex().
*
*   INPUTS
*
*       dev_index               The index of the interface for which to
*                               get the link-local address.
*       *addr_ptr               A pointer to the memory into which to
*                               place the link-local address.
*
*   OUTPUTS
*
*       NU_SUCCESS              The address was successfully retrieved.
*       NU_INVALID_PARM         A matching interface was not found or
*                               there is no link-local address on the
*                               interface.
*
*************************************************************************/
STATUS NU_Get_Link_Local_Addr(const UINT32 dev_index, UINT8 *addr_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status;
    DEV6_IF_ADDRESS *addr_struct;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Obtain the semaphore */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get a pointer to the interface. */
        dev_ptr = DEV_Get_Dev_By_Index(dev_index);

        /* If a pointer to the interface was found and IPv6 is enabled
         * on the interface. 
         */
        if ( (dev_ptr) && (dev_ptr->dev_flags & DV6_IPV6) )
        {
            /* Get the link-local address associated with the 
             * interface. 
             */
            addr_struct = IP6_Find_Link_Local_Addr(dev_ptr);

            /* If an address was found. */
            if (addr_struct)
            {
                /* Copy the address into the user's buffer. */
                NU_BLOCK_COPY(addr_ptr, addr_struct->dev6_ip_addr,
                              IP6_ADDR_LEN);
            }

            else 
            {
                status = NU_INVALID_PARM;

                NLOG_Error_Log("No link-local address on the interface", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
            }
        }

        else
        {
            status = NU_INVALID_PARM;

            NLOG_Error_Log("No matching device entry for name", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }

        /* Release the semaphore */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE, 
                           __FILE__, __LINE__);
        }
    }

    else
    {
        status = NU_INVALID_PARM;

        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE, 
                       __FILE__, __LINE__);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Get_Link_Local_Addr */
