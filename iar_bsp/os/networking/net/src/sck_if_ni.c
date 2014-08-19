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
*       sck_if_ni.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_IF_NameIndex.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_IF_NameIndex
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Allocate enough memory for NET_MAX_DEVICES if_nameindex entries with
 * if_name of DEV_NAME_LENGTH and one additional if_nameindex with
 * a zero if_index and NULL if_name.
 */
UINT8   NET_IF_NameIndex_Memory[((sizeof(struct if_nameindex) +
                                 DEV_NAME_LENGTH) * NET_MAX_DEVICES) +
                                 sizeof(struct if_nameindex)];

UINT8   NET_IF_NameIndex_Memory_Flag = NU_FALSE;

#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_IF_NameIndex
*
*   DESCRIPTION
*
*       This function returns an array of all interface names and indexes.
*       The memory used for the array is obtained dynamically and must be
*       freed by NU_IF_FreeNameIndex().
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       An array of if_nameindex entries.
*
*************************************************************************/
struct if_nameindex *NU_IF_NameIndex(VOID)
{
    DV_DEVICE_ENTRY     *dv_entry;
    struct if_nameindex *ni_ptr = NU_NULL, *tmp_ptr;
	NU_MEMORY_POOL 		*pool;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    INT                 dev_entries = 0;
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

     /* Get the Nucleus NET semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (NU_NULL);
    }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /*  Look at the first in the list. */
    dv_entry = DEV_Table.dv_head;

    /* Count the number of entries in the device table. */
    while (dv_entry != NU_NULL)
    {
        dev_entries ++;
        dv_entry = dv_entry->dev_next;
    }

    /* If at least one interface exists */
    if (dev_entries)
    {
		pool = MEM_Cached;

        /* Allocate enough memory for dev_entries if_nameindex entries with
         * if_name of DEV_NAME_LENGTH and one additional if_nameindex with
         * a zero if_index and NULL if_name.
         */
        if (NU_Allocate_Memory(pool, (VOID **)&ni_ptr,
                               (UNSIGNED)((sizeof(struct if_nameindex) +
                               DEV_NAME_LENGTH) * dev_entries) +
                               sizeof(struct if_nameindex), NU_NO_SUSPEND) != NU_SUCCESS)
            NLOG_Error_Log("Failed to allocate memory",
                           NERR_SEVERE, __FILE__, __LINE__);
#else
        /* If the memory is not currently being used, let the caller use it */
        if (NET_IF_NameIndex_Memory_Flag == NU_FALSE)
        {
            ni_ptr = (struct if_nameindex*)NET_IF_NameIndex_Memory;
            NET_IF_NameIndex_Memory_Flag = NU_TRUE;
        }

#endif

        if (ni_ptr)
        {
            tmp_ptr = ni_ptr;

            /*  Look at the first in the list. */
            dv_entry = DEV_Table.dv_head;

            /* Copy the index and name of each entry in the device table into
             * the memory that will be returned to the application.
             */
            while (dv_entry != NU_NULL)
            {
                tmp_ptr->if_index = (INT32)(dv_entry->dev_index);
                tmp_ptr->if_name = (CHAR*)((CHAR HUGE*)tmp_ptr + sizeof(struct if_nameindex));
                strcpy(tmp_ptr->if_name, dv_entry->dev_net_if_name);

                /* Move the pointer ahead in memory */
                tmp_ptr = (struct if_nameindex*)((CHAR HUGE*)tmp_ptr +
                          (sizeof(struct if_nameindex) + DEV_NAME_LENGTH));

                dv_entry = dv_entry->dev_next;
            }

            /* The last entry contains a 0 index and NU_NULL name */
            tmp_ptr->if_index = 0;
            tmp_ptr->if_name = NU_NULL;
        }
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    }
#endif

    /* Release the TCP semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (ni_ptr);

} /* NU_IF_NameIndex */
