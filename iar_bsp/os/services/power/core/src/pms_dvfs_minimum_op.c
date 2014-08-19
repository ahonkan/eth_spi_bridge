/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
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
*       pms_dvfs_minimum_op.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for controlling a minimum operating
*       point
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_Request_Min_OP
*       NU_PM_Release_Min_OP
*
*   DEPENDENCIES
*
*       power_core.h
*       dvfs.h
*       string.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/power_core.h"
#include "os/services/power/core/inc/dvfs.h"
#include <string.h>

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

extern UINT8          PM_DVFS_OP_Count;
extern UINT8          PM_DVFS_Minimum_OP;
extern UINT8          PM_DVFS_Current_OP;
extern UINT32         PM_DVFS_Min_Request_Count;
extern NU_PROTECT     PM_DVFS_List_Protect;
extern CS_NODE       *PM_DVFS_Min_Request_List;
extern NU_MEMORY_POOL System_Memory;

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Request_Min_OP
*
*   DESCRIPTION
*
*       This function requests a minimum operating point below which the
*       DVFS Service will not switch to until the request is released via
*       NU_PM_Release_Min_OP call
*
*   INPUT
*
*       op_id               Minimum operating point requested
*       handle_ptr          Where the request handle will be written to
*
*   OUTPUT
*
*       NU_SUCCESS          Successful transition
*       PM_TRANSITION_FAILED Attempted transition to the requested OP
*                            failed
*       PM_INVALID_OP_ID    Supplied OP is invalid
*       PM_INVALID_POINTER  Provided pointer is invalid
*
*************************************************************************/
STATUS NU_PM_Request_Min_OP(UINT8 op_id, PM_MIN_REQ_HANDLE *handle_ptr)
{
    STATUS         pm_status;
    PM_OP_REQUEST *pm_request;
    STATUS         status;
    
    NU_SUPERV_USER_VARIABLES

    /* Verify initialization is complete */
    pm_status = PMS_DVFS_Status_Check();

    if (pm_status == NU_SUCCESS)
    {
        /* Verify the pointer is valid */
        if (handle_ptr == NU_NULL)
        {
            pm_status = PM_INVALID_POINTER;
        }
        /* Verify the OP is valid */
        else if (op_id >= PM_DVFS_OP_Count)
        {
            pm_status = PM_INVALID_OP_ID;
        }
        else
        {
            /* Switch to supervisor mode */
            NU_SUPERVISOR_MODE();
            
            /* Allocate a new request structure */
            status = NU_Allocate_Memory(&System_Memory, (VOID *)&pm_request, sizeof(PM_OP_REQUEST), NU_NO_SUSPEND);
            if (status == NU_SUCCESS)
            {
                /* Clear the memory */
                memset(pm_request, 0, sizeof(PM_OP_REQUEST));

                /* Set as valid request block */
                pm_request -> pm_id = PM_OP_REQUEST_ID;

                /* Store the lowest requested OP */
                pm_request -> pm_op_id = op_id;

                /* Protect the request list */
                NU_Protect(&PM_DVFS_List_Protect);

                /* Check to see if this is the new minimum op */
                if (PM_DVFS_Minimum_OP < op_id)
                {
                    /* It is update the minimum */
                    PM_DVFS_Minimum_OP = op_id;
                }

                /* Place the request on the list */
                NU_Place_On_List(&PM_DVFS_Min_Request_List, &(pm_request -> pm_node));

                /* Increment the number of requests */
                PM_DVFS_Min_Request_Count++;

                /* Unprotect the list */
                NU_Unprotect();

                /* Return the handle */
                *handle_ptr = pm_request;

                /* If the current OP is lower than the newly requested OP attempt to
                   set a new OP to the newly min */
                if ((PM_DVFS_Current_OP < op_id) || (PM_DVFS_Current_OP == PM_STARTUP_OP))
                {
                    pm_status = NU_PM_Set_Current_OP(op_id);
                    
                    /* If this fails reset the minimum request */
                    if (pm_status != NU_SUCCESS)
                    {
                        /* Call release min OP which will remove the min OP from
                           the list and deallocate the request structure */
                        (VOID)NU_PM_Release_Min_OP(pm_request);
                    }
                }
            }
            
            /* Return to user mode */
            NU_USER_MODE();
        }
    }

    return (pm_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Release_Min_OP
*
*   DESCRIPTION
*
*       This function releases a minimum operating point request create
*       via NU_PM_Request_Min_OP call
*
*   INPUT
*
*       handle              Request handle returned from
*                           NU_PM_Request_Min_OP
*
*   OUTPUT
*
*       NU_SUCCESS          Successful transition
*       PM_INVALID_REQ_HANDLE Provided handle is invalid
*       PM_UNEXPECTED_ERROR Unexpected error has occurred
*
*************************************************************************/
STATUS NU_PM_Release_Min_OP(PM_MIN_REQ_HANDLE handle)
{
    STATUS         pm_status;
    STATUS         status;
    PM_OP_REQUEST *pm_request;
    CS_NODE       *node_ptr;
    INT            i;
    
    NU_SUPERV_USER_VARIABLES

    /* Verify initialization is complete */
    pm_status = PMS_DVFS_Status_Check();

    if (pm_status == NU_SUCCESS)
    {
        /* Setup the request pointer */
        pm_request = (PM_OP_REQUEST *)handle;

        /* Verify the handle is valid */
        if (pm_request == NU_NULL)
        {
            pm_status = PM_INVALID_REQ_HANDLE;
        }
        else if ((pm_request -> pm_id) != PM_OP_REQUEST_ID)
        {
            pm_status = PM_INVALID_REQ_HANDLE;
        }
        else
        {
            /* Switch to supervisor mode */
            NU_SUPERVISOR_MODE();
            
            /* Protect the list */
            NU_Protect(&PM_DVFS_List_Protect);

            /* Remove the request block */
            NU_Remove_From_List(&PM_DVFS_Min_Request_List, &(pm_request -> pm_node));

            /* Decrement the number of requests */
            PM_DVFS_Min_Request_Count--;

            /* Was this the minimum request? */
            if (pm_request -> pm_op_id == PM_DVFS_Minimum_OP)
            {
                /* Reset the minimum OP to the lowest available */
                PM_DVFS_Minimum_OP = 0;

                /* Search the list for the new minimum OP */
                node_ptr = PM_DVFS_Min_Request_List;
                for (i = 0; ((i < PM_DVFS_Min_Request_Count) && (node_ptr != NU_NULL)); i++)
                {
                    /* Test to see if it is the new min */
                    if (((PM_OP_REQUEST *)node_ptr) -> pm_op_id > PM_DVFS_Minimum_OP)
                    {
                        /* update the new min OP */
                        PM_DVFS_Minimum_OP = ((PM_OP_REQUEST *)node_ptr) -> pm_op_id;
                    }

                    /* Position the node pointer to the next node.  */
                    node_ptr = node_ptr -> cs_next;
                }
            }

            /* Remove the list protection */
            NU_Unprotect();

            /* Clear the request id */
            pm_request -> pm_id = 0;

            /* Deallocate the memory used in the request structure */
            status = NU_Deallocate_Memory(pm_request);
            if (status != NU_SUCCESS)
            {
                pm_status = PM_UNEXPECTED_ERROR;
            }
            
            /* Return to user mode */
            NU_USER_MODE();
        }
    }

    return (pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


