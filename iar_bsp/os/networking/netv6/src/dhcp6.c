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
*       dhcp6.c                                       
*
*   DESCRIPTION
*
*       This file contains all the DHCPv6 routines for performing
*       configuration via DHCPv6.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Dhcp6
*       DHCP6_Create_Struct
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

UINT8   DHCP6_Id = 1;
INT     DHCP6_Enable = 0;

/******************************************************************************
*
*   FUNCTION
*
*       NU_Dhcp6
*
*   DESCRIPTION
*
*       This function performs DHCPv6 processing with a DHCPv6 server based
*       on the options passed into the routine.  The interface index can be
*       acquired from the application by using the API routine 
*       NU_IF_NameToIndex().
*
*   INPUTS
*
*       *dhcp6_cli_ptr          Pointer to the DHCPv6 client structure that 
*                               contains data that can be obtained at the 
*                               application layer.  The user must provide
*                               the dhcp6_dev_index and either fill in the
*                               dhcp6_user_opts and dhcp6_opt_length or set 
*                               the pointer to NULL.
*       suspend                 NU_SUSPEND or NU_NO_SUSPEND, depending on
*                               whether the caller wants to wait for 
*                               completion.
*
*   OUTPUTS
*
*      NU_SUCCESS               Successful operation.
*      NU_INVALID_PARM          dhcp_cli_ptr is invalid.
*      NU_NO_MEMORY             Allocation of Memory failed. 
*      NU_DHCP_REQUEST_FAILED   The DHCPv6 client request failed.
*
******************************************************************************/
STATUS NU_Dhcp6(DHCP6_CLIENT *dhcp_cli_ptr, UNSIGNED suspend)
{
    STATUS          status = NU_SUCCESS;
    NU_TASK         *task_ptr;
    DHCP6_STRUCT    *ds_ptr;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

#if (DHCP6_ENABLE_TAHI_TESTING)
    /* TAHI TESTING */
    if (DHCP6_Enable == 0)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (NU_SUCCESS);
    }
#endif

    /* Validate the incoming pointer. */
    if (!dhcp_cli_ptr)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (NU_INVALID_PARM);
    }

    /* Wait for the DHCPv6 Receive task to initialize. */
    while (!DHCP6_RX_Task_Init)
    {
        NU_Sleep(SCK_Ticks_Per_Second);
    }

    /* Obtain the DHCPv6 client semaphore. */
    NU_Obtain_Semaphore(&DHCP6_Cli_Resource, NU_SUSPEND);

    /* Find the DHCPv6 structure associated with this request, or create
     * a new one if this is the first request on the interface.
     */
    ds_ptr = DHCP6_Find_Struct_By_Dev(dhcp_cli_ptr->dhcp6_dev_index);

    /* If there is no existing structure, create a new one now. */
    if (!ds_ptr)
    {
        /* Create a new structure. */
        ds_ptr = DHCP6_Create_Struct();

        /* Fill in the structure parameters. */
        if (ds_ptr)
        {
            ds_ptr->dhcp6_dev_index = dhcp_cli_ptr->dhcp6_dev_index;
        }
    }
    
    /* If a DHCPv6 structure was successfully created. */
    if (ds_ptr)
    {
        /* Parse the user options to determine how to proceed. */
        DHCP6_Parse_User_Options(dhcp_cli_ptr->dhcp6_user_opts, 
                                 dhcp_cli_ptr->dhcp6_opt_length, 
                                 ds_ptr->dhcp6_client_opts);

        /* Allocate memory for the user options that were passed into 
         * the routine. 
         */
        if (dhcp_cli_ptr->dhcp6_opt_length)
        {
            /* If memory has already been allocated for DHCPv6 client
             * options, check if it is enough memory, and if not, 
             * deallocate the previous memory and allocate new memory.  
             * Otherwise, reuse the existing memory.
             */
            if ( (!ds_ptr->dhcp6_opts) ||
                 (ds_ptr->dhcp6_opts_len < dhcp_cli_ptr->dhcp6_opt_length) )
            {
                /* If there was memory previously allocated. */
                if (ds_ptr->dhcp6_opts)
                {   
                    /* Deallocate the memory. */
                    NU_Deallocate_Memory((VOID*)ds_ptr->dhcp6_opts);
                }

                /* Allocate memory for the user options. */
                status = NU_Allocate_Memory(MEM_Cached, 
                                            (VOID**)&ds_ptr->dhcp6_opts, 
                                            (UNSIGNED)dhcp_cli_ptr->dhcp6_opt_length, 
                                            (UNSIGNED)NU_NO_SUSPEND);
            }

            if (status == NU_SUCCESS)   
            {
                /* Save the length of the options. */
                ds_ptr->dhcp6_opts_len = dhcp_cli_ptr->dhcp6_opt_length;
        
                /* Make a copy of the user options to use for future 
                 * packet transmissions.
                 */
                NU_BLOCK_COPY(ds_ptr->dhcp6_opts, 
                              dhcp_cli_ptr->dhcp6_user_opts, 
                              dhcp_cli_ptr->dhcp6_opt_length);
            }
        }

        /* If memory was successfully allocated. */
        if (status == NU_SUCCESS)
        {
            /* If the user has included an IA_NA option, perform a standard 
             * 4-message exchange.
             */ 
            if (ds_ptr->dhcp6_client_opts[DHCP6_OPT_IA_NA].dhcp6_opt_addr != 0xffffffff)
            {
                /* Invoke server discovery. */
                status = DHCP6_Find_Server(ds_ptr);
            }

            /* RFC 3315 - section 1.2 - When a DHCP client does not need to 
             * have a DHCP server assign it IP addresses, the client sends 
             * an Information Request message to the 
             * All_DHCP_Relay_Agents_and_Servers multicast address.  Servers 
             * respond with a Reply message containing the configuration 
             * information for the client.
             */
            else
            {
                status = DHCP6_Obtain_Config_Info(ds_ptr);
            }

            /* If the packet was sent successfully, suspend the task until a
             * reply is received or until retransmissions are exhausted.
             */
            if ( (status == NU_SUCCESS) && (suspend) )
            {
                /* Get a pointer to the current task. */
                task_ptr = NU_Current_Task_Pointer();

                /* Set the task pointer in the ds_ptr structure. */
                ds_ptr->dhcp6_task = task_ptr;

                /* Release the DHCPv6 client semaphore. */
                NU_Release_Semaphore(&DHCP6_Cli_Resource);

                /* Suspend until a reply is received or we time out. */
                NU_Suspend_Task(task_ptr);

                /* Obtain the DHCPv6 client semaphore. */
                NU_Obtain_Semaphore(&DHCP6_Cli_Resource, NU_SUSPEND);

                /* Clear out the task pointer. */
                ds_ptr->dhcp6_task = NU_NULL;

                /* Return the status value reported by the DHCPv6 module. */
                status = ds_ptr->dhcp6_status;
            }

            /* Set the task pointer to NULL. */
            else
            {
                ds_ptr->dhcp6_task = NU_NULL;
            }
        }
    }

    /* Release the DHCPv6 client semaphore. */
    NU_Release_Semaphore(&DHCP6_Cli_Resource);

    /* Return to user mode */
    NU_USER_MODE();

    return (status);

} /* NU_Dhcp6 */

/*************************************************************************
*
*   FUNCTION
*
*       DHCP6_Create_Struct
*
*   DESCRIPTION
*
*       This routine creates a DHCPv6 client structure.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       A pointer to the DHCP6_STRUCT or NU_NULL if there is not enough
*       memory to create the structure.
*
*************************************************************************/
DHCP6_STRUCT *DHCP6_Create_Struct(VOID)
{
    DHCP6_STRUCT    *ds_ptr;
    STATUS          status;

    /* Allocate memory for the DHCPv6 structure. */
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&ds_ptr, 
                                (UNSIGNED)sizeof(DHCP6_STRUCT),
                                (UNSIGNED)NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Initialize the structure. */
        UTL_Zero(ds_ptr, sizeof(DHCP6_STRUCT));

        /* Assign an ID. */
        ds_ptr->dhcp6_id = DHCP6_Id ++;

        /* Never let DHCP6_Id wrap to zero as this ID is unused. */
        if (DHCP6_Id == 0)
        {
            DHCP6_Id = 1;
        }

        /* Add this structure to the list. */
        DLL_Enqueue(&DHCP6_Structs, ds_ptr);
    }

    return (ds_ptr);

} /* DHCP6_Create_Struct */

#endif
