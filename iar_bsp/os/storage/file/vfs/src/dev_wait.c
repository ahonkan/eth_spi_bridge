/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
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
*       dev_wait.c
*
*   COMPONENT
*
*       Storage
*
*   DESCRIPTION
*
*       This file contains a common initialization routine for apps
*       which require a Storage device.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Storage_Device_Wait              Waits for a device to be 
*                                           mounted.
*
*   DEPENDENCIES
*
*       pcdisk.h                            Storage functions
*
*************************************************************************/

#include "storage/pcdisk.h"
#include "storage/lck_extr.h"


/*************************************************************************
*
*   FUNCTION
*
*       NU_Storage_Device_Wait
*
*   DESCRIPTION
*
*       Allows an application to wait for a storage device to become 
*       available. The mount_name parameter may specify a mount point 
*       character (i.e. 'C'); or if NULL the function will return on
*       any device mount. The following is a summary of the possible 
*       values for the suspend parameter.
*       * NU_NO_SUSPEND (0)
*           The service returns immediately regardless of whether or 
*           not the request can be satisfied.
*           This is the only valid option if the service is called 
*           from a non-task thread.
*       * NU_SUSPEND (0xFFFFFFFF)
*           The calling task is suspended until the event flag
*           combination is available.
*       * timeout value
*           (1 – 4,294,967,293). The calling task is suspended until 
*           the event flag combination is available or until the 
*           specified number of ticks has expired.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Event_Group_Information
*       NU_List_Mount
*       strcmp
*       NU_Retrieve_Events
*
*   INPUTS
*
*       mount_name                          Char pointer (can be NULL)
*       suspend                             Specifies whether to suspend 
*                                            the calling task if the 
*                                            requested device is not 
*                                            available.
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      If service is successful
*           < 0                             Indicates one of the called
*                                            functions failed.
*
*************************************************************************/
STATUS NU_Storage_Device_Wait(CHAR *mount_name, UNSIGNED suspend)
{
    STATUS status;
    CHAR group_name[NU_MAX_NAME];
    UNSIGNED event_flags;
    UNSIGNED events;
    UNSIGNED tasks_suspended;
    NU_TASK *first_task;
    MNT_LIST_S  *mount_list = NU_NULL;
    BOOLEAN match = NU_FALSE;
    UNSIGNED start_time;
    UNSIGNED end_time;
    UNSIGNED max_ticks = (UNSIGNED)-1;
    UNSIGNED elapsed_time;
    
    LCK_FS_ENTER()

    /* Check if the Storage component has been initialized by checking 
       if the event group exists. */
    status = NU_Event_Group_Information(&FILE_Application_Event_Group, group_name,
                                        &event_flags, &tasks_suspended, &first_task);
    
    /* Loop checks if mount exists, then waits for a "mount event" and rechecks if the 
       specified device exists. Will exit if specified device is mounted (or any mount 
       if mount_name is NULL) or if a timeout occurs from event wait. */
    while (status == NU_SUCCESS)
    {
        /* If mount_name was passed in, then check if the specified device was mounted. Else return success. */
        status = NU_List_Mount(&mount_list);
        if (status == NU_SUCCESS)
        {
            if(mount_name != NU_NULL)
            {
                /* iterate through the mount list checking for the specified mount point */
                for (match = NU_FALSE; mount_list != NU_NULL; mount_list = mount_list->next)
                {
                    /* Compare the first character of mount_name */
                    if (strncmp(mount_name, mount_list->mnt_name, 1) == 0)
                    {
                        /* Specified mount point is mounted. */
                        match = NU_TRUE;
                        
                        /* Free the allocated list */
                        status = NU_Free_List((VOID**)&mount_list); 
                        
                        break;
                    }
                }
            }
            else if (mount_list != NU_NULL)
            {
                /* There is a mounted device and mount_name was not specified */ 
                match = NU_TRUE;
            }
            /* Free the allocated list */
            status = NU_Free_List((VOID**)&mount_list); 
        }
        
        /* Check that the requested device was not found already */
        if (match == NU_TRUE)
        {
            /* mount exists, so return */
            break;
        }
        
        if (status == NU_SUCCESS)
        {
            /* Get the current time */
            start_time = NU_Retrieve_Clock();
            
            /* Wait for a device to be mounted. */
            status = NU_Retrieve_Events(&FILE_Application_Event_Group, NUF_EVT_MOUNT_CREATE, 
                                        NU_AND_CONSUME, &events, suspend);
            if (status == NU_SUCCESS)
            {
                /* A device was mounted. */
                if (mount_name == NU_NULL)
                {
                    /* A device was mounted, so return now. */
                    break;
                }
                else if(suspend != NU_SUSPEND)
                {
                    /* Calculate the elapsed time */
                    end_time = NU_Retrieve_Clock();
                    if(end_time < start_time)
                    {
                        /* Rollover occurred */
                        elapsed_time = (max_ticks - start_time) + end_time;
                    }
                    else
                    {
                        elapsed_time = end_time - start_time;
                    }
                    
                    /* subtract elapsed time from the suspend value */
                    if((INT)(suspend - elapsed_time) >= 0)
                    {
                        suspend -= elapsed_time;
                    }
                    else
                    {
                        suspend = 0;
                    }
                }
            }
        }
    }

    LCK_FS_EXIT()

    return (status);
}

