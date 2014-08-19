/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ips_grp_ag.c
*
* COMPONENT
*
*       DATABASE - Groups
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Add_Group.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Add_Group
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_externs.h
*       ips_api.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_externs.h"
#include "networking/ips_api.h"

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Add_Group
*
* DESCRIPTION
*
*       This function adds a new IPsec group to the group database.
*
* INPUTS
*
*       *group_name             Specifies the group name.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful operation.
*       NU_TIMEOUT              Timeout before semaphore is obtained.
*       NU_NO_MEMORY            Memory not available.
*       NU_INVALID_PARM         Group already exists.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Add_Group(CHAR *group_name)
{
    STATUS              status;
    UINT32              mem_size;
    IPSEC_POLICY_GROUP  *group_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure the group name pointer is not NULL. */
    if(group_name != NU_NULL)
    {
        /* First grab the semaphore. */
        status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

        /* Check the status value. */
        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain IPsec semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        else
        {
            /* Calculate the memory required. Memory required is sizeof
               IPSEC_POLICY_GROUP plus length of group name. */
            mem_size = sizeof(IPSEC_POLICY_GROUP) +
                                                   strlen(group_name) + 1;

            /* Now allocate the memory for the group entry along with the
               group name entry. */
            status = NU_Allocate_Memory(IPSEC_Memory_Pool,
                            (VOID **)&group_ptr, mem_size, NU_NO_SUSPEND);

            /* Check the status value. */
            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to allocate the memory",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
            else
            {
                /* Normalize the pointer first. */
                group_ptr = TLS_Normalize_Ptr(group_ptr);

                /* Clear the SPDB entries. */
                group_ptr->ipsec_policy_list.ipsec_head = NU_NULL;
                group_ptr->ipsec_policy_list.ipsec_tail = NU_NULL;
                group_ptr->ipsec_policy_list.ipsec_next_policy_index = 0;

                /* Clear the outbound SADB entries. */
                group_ptr->ipsec_outbound_sa_list.ipsec_head = NU_NULL;
                group_ptr->ipsec_outbound_sa_list.ipsec_tail = NU_NULL;
                group_ptr->ipsec_outbound_sa_list.ipsec_next_sa_index= 0;

                /* Clear the inbound SADB entries. */
                group_ptr->ipsec_inbound_sa_list.ipsec_head = NU_NULL;
                group_ptr->ipsec_inbound_sa_list.ipsec_tail = NU_NULL;

                group_ptr->ipsec_inbound_sa_list.
                                ipsec_next_sa_index_esp = IPSEC_SPI_START;

                group_ptr->ipsec_inbound_sa_list.
                                ipsec_next_sa_index_ah  = IPSEC_SPI_START;

                /* Now assign memory to the group name pointer of the
                   structure. */
                group_ptr->ipsec_group_name =
                        ((CHAR *)group_ptr + sizeof(IPSEC_POLICY_GROUP));

                /* Now copy the given group name to the newly created
                   group's name member. */
                strcpy(group_ptr->ipsec_group_name, group_name);

                /* Assign default DF bit processing setting. User can
                 * change the default behavior using set group options
                 * API.
                 */
                group_ptr->ipsec_df_processing = IPSEC_CLEAR_DF_BIT;

                /* Add this newly created group to the list of groups. */
                status = SLL_Insert_Sorted(&IPS_Group_DB, group_ptr,
                                                        IPSEC_Cmp_Groups);

                /* If group has not been successfully added to the DB. */
                if(status != NU_SUCCESS)
                {
                    /* Now deallocate the group memory too, */
                    if(NU_Deallocate_Memory(group_ptr) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to deallocate the memory",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
            }

            /* Now everything is done, release the semaphore. */
            if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the IPsec semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }
    else
    {
        /* Set the error status. */
        status = IPSEC_INVALID_PARAMS;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IPSEC_Add_Group */
