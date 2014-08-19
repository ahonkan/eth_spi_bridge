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
*       dvs_common.c
*
*   COMPONENT
*
*       DV - Device Manager
*
*   DESCRIPTION
*
*       This file contains the supplemental routines for the Device
*       Manager component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       DVS_Label_Append                    This function appends a label
*                                           to a label list.
*       DVS_Label_Append_Instance           This function appends the registry
*                                           instance label to a list
*       DVS_Label_Copy                      This function copies a label list
*       DVS_Label_List_Contains             This function looks for a specific
*                                           label in a list of labels
*       DVS_Label_Remove                    This function removes a label
*                                           from a list.
*       DVS_Label_Replace                   This function replaces a label
*                                           in a list.
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus OS API
*       nu_kernel.h                         Nucleus Kernel API
*       thread_control.h                    Nucleus threads internal API
*       reg_api.h                           System Registry API
*       device_manager.h                    Device Manager internal API
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "services/reg_api.h"
#include "os/kernel/devmgr/inc/device_manager.h"

/* Define external inner-component global data references.  */

extern DV_DEV_REGISTRY *DVD_Dev_Registry;
extern INT              DVD_Max_Dev_Id_Cnt;
extern UNSIGNED         DVD_Initialized;


/*************************************************************************
*
*   FUNCTION
*
*       DVS_Label_Append
*
*   DESCRIPTION
*
*       Takes two label lists and creates a third label list containing
*       all the labels.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       new_label_list[]                    The new label list
*       new_label_max                       The max number of labels the
*                                           new list can contain
*       old_label_list[]                    The original label list
*       old_label_cnt                       The number of labels in the
*                                           original label list
*       app_label_list[]                    The list of labels to append
*                                           to the original
*       app_label_cnt                       The number of labels in the
*                                           append label list
*
*   OUTPUTS
*
*       NU_SUCCESS
*       DV_INVALID_INPUT_PARAMS
*       DV_LABEL_LIST_TOO_SMALL
*
*************************************************************************/
STATUS  DVS_Label_Append (DV_DEV_LABEL new_label_list[], INT new_label_max,
                          DV_DEV_LABEL old_label_list[], INT old_label_cnt,
                          DV_DEV_LABEL app_label_list[], INT app_label_cnt)
{
    STATUS  status = NU_SUCCESS;
    INT     new_index, old_index, app_index;

#if (DV_ERR_CHECK_ENABLE == NU_TRUE)
    /* Check the input parameters */
    if ((new_label_list == NU_NULL) ||
        (new_label_max < 0) ||
        ((old_label_list == NU_NULL) && (old_label_cnt > 0)) ||
        (old_label_cnt < 0) ||
        ((app_label_list == NU_NULL) && (app_label_cnt > 0)) ||
        (app_label_cnt < 0))
    {
        /* Invalid input parameters */
        status = DV_INVALID_INPUT_PARAMS;
    }
    else
    {
        /* Make sure new label array is large enough */
        if (new_label_max < (old_label_cnt + app_label_cnt))
        {
            /* Return label list too small error */
            status = DV_LABEL_LIST_TOO_SMALL;
        }
    }

    /* If everything is OK */
    if (status == NU_SUCCESS)
#endif
    {
        /* Initialize the new label index */
        new_index = 0;

        /* Copy all entries from the old label list to the new one */
        for (old_index = 0; old_index < old_label_cnt; old_index++)
        {
            new_label_list[new_index] = old_label_list[old_index];

            /* Increment the new label index */
            new_index++;
        }

        /* Append all entries from the appending label list to the new one */
        for (app_index = 0; app_index < app_label_cnt; app_index++)
        {
            new_label_list[new_index] = app_label_list[app_index];

            /* Increment the new label index */
            new_index++;
        }
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVS_Label_Append_Instance
*
*   DESCRIPTION
*
*       Finds the device's instance label in the registry and appends it
*       to the label list.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       new_label_list[]                    The new label list
*       new_label_max                       The max number of labels the
*                                           new list can contain
*       old_label_list[]                    The original label list
*       old_label_cnt                       The number of labels in the
*                                           original label list
*       *key                                Pointer to the registry path
*                                           for this instance
*
*   OUTPUTS
*
*       NU_SUCCESS
*       DV_INVALID_INPUT_PARAMS
*       DV_LABEL_LIST_TOO_SMALL
*       DV_UNEXPECTED_ERROR
*
*************************************************************************/
STATUS  DVS_Label_Append_Instance (DV_DEV_LABEL new_label_list[], INT new_label_max,
                                   DV_DEV_LABEL old_label_list[], INT old_label_cnt,
                                   const CHAR* key)
{
    STATUS          status = NU_SUCCESS;
    INT             i;
    DV_DEV_LABEL    instance_label[1];
    STATUS          reg_status;

#if (DV_ERR_CHECK_ENABLE == NU_TRUE)
    /* Check the input parameters */
    if ((new_label_list == NU_NULL) ||
        (new_label_max < 0) ||
        ((old_label_list == NU_NULL) && (old_label_cnt > 0)) ||
        (old_label_cnt < 0) ||
        (key == NU_NULL))
    {
        /* Invalid input parameters */
        status = DV_INVALID_INPUT_PARAMS;
    }
    else
    {
        /* Make sure new label array is large enough */
        if (new_label_max < (old_label_cnt + 1))
        {
            /* Return label list too small error */
            status = DV_LABEL_LIST_TOO_SMALL;
        }
        else
#endif
        {

            /* Try and get the instance label */                                       
            reg_status = REG_Get_Bytes_Value (key, "/labels/instance_label", (UINT8*)(instance_label), sizeof(DV_DEV_LABEL));

            /* If we could not get the instance label */
            if(reg_status != NU_SUCCESS)
            {
                status = DV_UNEXPECTED_ERROR;
            }
        }
#if (DV_ERR_CHECK_ENABLE == NU_TRUE)
    }
#endif

    /* If everything is OK */
    if (status == NU_SUCCESS)
    {
        /* Copy all entries from the old label list to the new one */
        for (i = 0; i < old_label_cnt; i++)
        {
            new_label_list[i] = old_label_list[i];
        }

        /* Append the instance label */
        new_label_list[i] = instance_label[0];
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVS_Label_Copy
*
*   DESCRIPTION
*
*       Copies a list of labels
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       to_label_list[]                     The destination label list
*       to_label_max                        The max number of labels the
*                                           destination list can contain
*       from_label_list[]                   The source label list
*       from_label_cnt                      The number of labels in the
*                                           source label list
*
*   OUTPUTS
*
*       NU_SUCCESS
*       DV_INVALID_INPUT_PARAMS
*       DV_LABEL_LIST_TOO_SMALL
*
*************************************************************************/
STATUS  DVS_Label_Copy (DV_DEV_LABEL to_label_list[], INT to_label_max,
                        DV_DEV_LABEL from_label_list[], INT from_label_cnt)
{
    STATUS  status = NU_SUCCESS;
    INT     i;

#if (DV_ERR_CHECK_ENABLE == NU_TRUE)
    /* Check the input parameters */
    if ((to_label_list == NU_NULL) ||
        (to_label_max < 0) ||
        ((from_label_list == NU_NULL) && (from_label_cnt > 0)) ||
        (from_label_cnt < 0))
    {
        /* Invalid input parameters */
        status = DV_INVALID_INPUT_PARAMS;
    }
    else
    {
        /* Make sure to label array is large enough */
        if (to_label_max < from_label_cnt)
        {
            /* Return label list too small error */
            status = DV_LABEL_LIST_TOO_SMALL;
        }
    }

    /* If everything is OK */
    if (status == NU_SUCCESS)
#endif
    {
        /* Copy all entries from the from label list to the to label list */
        for (i = 0; i < from_label_cnt; i++)
        {
            to_label_list[i] = from_label_list[i];
        }
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVS_Label_List_Contains
*
*   DESCRIPTION
*
*       Checks a list of labels for a specific label.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       label_list[]                        A list of labels to search
*       label_cnt                           The number of labels in the list
*       search_label                        Label to search for
*
*   OUTPUTS
*
*       NU_SUCCESS
*       DV_LABEL_NOT_FOUND
*       DV_INVALID_INPUT_PARAMS
*
*************************************************************************/
STATUS  DVS_Label_List_Contains (DV_DEV_LABEL label_list[], INT label_cnt,
                                 DV_DEV_LABEL search_label)
{
    STATUS  status = NU_SUCCESS;
    INT     found = NU_FALSE;
    INT     i;

#if (DV_ERR_CHECK_ENABLE == NU_TRUE)
    /* Check the input parameters */
    if (((label_list == NU_NULL) && (label_cnt > 0)) ||
        (label_cnt < 0))
    {
        /* Invalid input parameters */
        status = DV_INVALID_INPUT_PARAMS;
    }

    /* If everything is OK */
    if (status == NU_SUCCESS)
#endif
    {
        /* Loop through all entries */
        for (i = 0; ((i < label_cnt) && (found != NU_TRUE)) ; i++)
        {
            /* If the device has a matching label */
            if (DV_COMPARE_LABELS(&search_label, &(label_list[i])))
            {
                /* Exit the loop */
                found = NU_TRUE;
            }
        }

        /* If we did not find a match */
        if (found == NU_FALSE)
        {
            status = DV_LABEL_NOT_FOUND;
        }
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVS_Label_Remove
*
*   DESCRIPTION
*
*       Searches a label list for a specific label then removes that
*       label from the list.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       label_list[]                        A list of labels to search
*       label_cnt                           The number of labels in the list
*       remove_label                        Label to remove
*
*   OUTPUTS
*
*       NU_SUCCESS
*       DV_LABEL_NOT_FOUND
*
*************************************************************************/
STATUS  DVS_Label_Remove (DV_DEV_LABEL label_list[], INT label_cnt,
                          DV_DEV_LABEL remove_label)
{
    STATUS  status = NU_SUCCESS;
    INT     found = NU_FALSE;
    INT     i;

#if (DV_ERR_CHECK_ENABLE == NU_TRUE)
    /* Check the input parameters */
    if (((label_list == NU_NULL) && (label_cnt > 0)) ||
        (label_cnt < 0))
    {
        /* Invalid input parameters */
        status = DV_INVALID_INPUT_PARAMS;
    }

    /* If everything is OK */
    if (status == NU_SUCCESS)
#endif
    {
        /* Loop through all entries */
        for (i = 0; i < label_cnt; i++)
        {
            /* If we have not found the label */
            if (found != NU_TRUE)
            {
                /* See if this is the label we want to remove */
                if (DV_COMPARE_LABELS(&label_list[i], &remove_label))
                {
                    found = NU_TRUE;
                }
            }
            else
            {
                /* Move remaining labels down one */
                label_list[i-1] = label_list[i];
            }
        }

        /* If we removed a label */
        if (found == NU_TRUE)
        {
            /* Clear the last label position */
            DV_CLEAR_LABEL(&label_list[(label_cnt - 1)]);
        }
        else
        {
            status = DV_LABEL_NOT_FOUND;
        }
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVS_Label_Replace
*
*   DESCRIPTION
*
*       Searches a label list for a specific label then replaces that
*       label with the new label.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       label_list[]                        A list of labels to search
*       label_cnt                           The number of labels in the list
*       search_label                        Label to search for
*       new_label                           Label to replace the found label
*
*   OUTPUTS
*
*       NU_SUCCESS
*       DV_LABEL_NOT_FOUND
*       DV_INVALID_INPUT_PARAMS
*
*************************************************************************/
STATUS  DVS_Label_Replace (DV_DEV_LABEL label_list[], INT label_cnt,
                           DV_DEV_LABEL search_label, DV_DEV_LABEL new_label)
{
    STATUS  status = NU_SUCCESS;
    INT     found = NU_FALSE;
    INT     i;

#if (DV_ERR_CHECK_ENABLE == NU_TRUE)
    /* Check the input parameters */
    if (((label_list == NU_NULL) && (label_cnt > 0)) ||
        (label_cnt < 0))
    {
        /* Invalid input parameters */
        status = DV_INVALID_INPUT_PARAMS;
    }

    /* If everything is OK */
    if (status == NU_SUCCESS)
#endif
    {
        /* Loop through all entries */
        for (i = 0; ((i < label_cnt) && (found != NU_TRUE)) ; i++)
        {
            /* Search and replace if found */
            if (DV_COMPARE_LABELS(&label_list[i], &search_label))
            {
                label_list[i] = new_label;

                /* Exit the loop */
                found = NU_TRUE;
            }
        }

        /* If we did not find a match */
        if (found == NU_FALSE)
        {
            status = DV_LABEL_NOT_FOUND;
        }
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DVS_Dev_Handles_Get
*
*   DESCRIPTION
*
*       This function retrieves the list of device handles for a device.
*
*   CALLED BY
*
*       Middleware (Power Management System)
*
*   CALLS
*
*       NU_Sleep
*       TCCT_Schedule_Lock
*       TCCT_Schedule_Unlock
*
*   INPUTS
*
*       dev_id                              Device ID
*       dev_handle_list[]                   Device handle list we return
*       *dev_handle_cnt_ptr                 Pointer to variable that
*                                           contains the maximum handles
*                                           to return and it is also the
*                                           place where we return the
*                                           number of handles found
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS
*           DV_INVALID_INPUT_PARAMS
*           DV_DEV_LIST_TOO_SMALL
*
*************************************************************************/
STATUS DVS_Dev_Handles_Get (DV_DEV_ID dev_id, DV_DEV_HANDLE dev_handle_list[],
                                INT * dev_handle_cnt_ptr)
{
    STATUS  status = NU_SUCCESS;
    INT32   reg_index;
    INT32   ses_index;
    INT     dev_handle_cnt;

    NU_SUPERV_USER_VARIABLES

    /* Get device registry index */
    reg_index = DV_GET_REG_INDEX(dev_id);

#if (DV_ERR_CHECK_ENABLE == NU_TRUE)
    /* Check the input parameters */
    if (((reg_index < (DV_DEV_ID)0) || (reg_index >= DVD_Max_Dev_Id_Cnt)) ||
        (dev_handle_list == NU_NULL) ||
        (dev_handle_cnt_ptr == NU_NULL) ||
        (*dev_handle_cnt_ptr <= 0))
    {
        /* Invalid input parameters */
        status = DV_INVALID_INPUT_PARAMS;
    }
#endif /* DV_ERR_CHECK_ENABLE */

    /* If everything is OK */
    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* If the DM has not been initialized yet */
        while (DVD_Initialized != DV_INITIALIZED)
        {
            /* Sleep and then try again */
            NU_Sleep(2);
        }

        /* Protect against access to the registry.  */
        TCCT_Schedule_Lock();

        /* Add all open sessions to the list keeping count and ensuring
           that the provided list memory is not overrun */
        dev_handle_cnt = 0;
        ses_index = 0;
        while ((ses_index < DV_MAX_DEV_SESSION_CNT) &&
               (dev_handle_cnt < DVD_Dev_Registry[reg_index].active_open_cnt) &&
               (dev_handle_cnt < *dev_handle_cnt_ptr))
        {
            /* Ensure session is open */
            if (DVD_Dev_Registry[reg_index].session[ses_index].state_flag == DV_SES_OPEN)
            {
                /* Add device handle to the list */
                dev_handle_list[dev_handle_cnt] = DV_CREATE_DEV_HANDLE(dev_id, ses_index);

                /* Update list count */
                dev_handle_cnt++;

            }

            /* Move to next session */
            ses_index++;

        }

        /* Release protection against access to the registry.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();

        /* Update number of device handles returned */
        *dev_handle_cnt_ptr = dev_handle_cnt;

    }

    return (status);
}
