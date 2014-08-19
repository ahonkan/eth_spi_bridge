/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
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
*       vacm.c                                                   
*
*   DESCRIPTION
*
*       This file contains functions specific to verifying access rights
*       view-based access model (VACM)
*
*   DATA STRUCTURES
*
*       Vacm_Mib_Table
*       Vacm_Status
*
*   FUNCTIONS
*
*       VACM_Init
*       VACM_Config
*       VACM_Search_Context
*       VACM_Search_Group
*       VACM_Search_AccessEntry
*       VACM_Search_MibView
*       VACM_Filling_FamilyMask
*       VACM_CheckInstance_Access
*       VACM_Get_Context_Name_Util
*       VACM_Get_Sec2Group_Entry_Util
*       VACM_Get_Sec2Group_Entry
*       VACM_MIB_Get_AccessEntry_Util
*       VACM_MIB_Get_AccessEntry
*       VACM_GetViewTree_Entry_Util
*       VACM_GetViewTreeFamily_Entry
*       VACM_Find_AccessEntry
*       VACM_FindIn_MibView
*       VACM_InsertContext
*       VACM_Add_Context
*       VACM_InsertGroup_Util
*       VACM_InsertGroup
*       VACM_InsertAccessEntry_Util
*       VACM_InsertAccessEntry
*       VACM_InsertMibView
*       VACM_Compare_Group
*       VACM_Save_Group
*       VACM_Compare_Access
*       VACM_Save_Access
*       VACM_Compare_View
*       VACM_Save_View
*
*   DEPENDENCIES
*
*       nu_net.h
*       snmp_cfg.h
*       vacm.h
*       snmp_utl.h
*       mib.h
*       snmp_no.h
*       nu_sd.h
*       ncl.h
*       snmp_file.h
*
*************************************************************************/

#include "networking/nu_net.h"

#include "networking/snmp_cfg.h"
#include "networking/vacm.h"
#include "networking/snmp_utl.h"
#include "networking/mib.h"
#include "networking/snmp_no.h"

#if (SNMP_CONFIG_SERIAL == NU_TRUE)
#include "drivers/serial.h"
#include "networking/ncl.h"
#endif

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
#include "networking/snmp_file.h"
#endif

#if (SNMP_CONFIG_SERIAL == NU_TRUE)
extern NU_SERIAL_PORT           *Snmp_Serial_Port;
#endif

extern NU_MEMORY_POOL           System_Memory;

#if (INCLUDE_MIB_VACM == NU_TRUE)
extern VACM_SEC2GROUP_ROOT      Vacm_Mib_Temp_Sec2Group_Table;
extern VACM_ACCESS_ROOT         VACM_Mib_Temp_Access_Root;
extern VACM_VIEWTREE_ROOT       Vacm_Mib_Temp_View_Family_Root;
#endif

#if (VACM_CONTEXT_TBL_SIZE > 0)
extern UINT8 Snmp_Cfg_Context_Names[VACM_CONTEXT_TBL_SIZE]
                                    [SNMP_SIZE_SMALLOBJECTID];
#endif

#if (VACM_SEC2GRP_TBL_SIZE > 0)
extern VACM_SEC2GROUP Snmp_Cfg_Sec2Groups [VACM_SEC2GRP_TBL_SIZE];
#endif

#if (VACM_ACCESS_TBL_SIZE > 0)
extern VACM_ACCESS Snmp_Cfg_Access [VACM_ACCESS_TBL_SIZE];
#endif

#if (VACM_MIB_VIEW_TBL_SIZE > 0)
extern VACM_VIEWTREE Snmp_Cfg_Mib_View [VACM_MIB_VIEW_TBL_SIZE];
#endif

VACM_MIB_STRUCT                 Vacm_Mib_Table;
STATIC UINT8                    Vacm_Status = SNMP_MODULE_NOTSTARTED;

/************************************************************************
*
*   FUNCTION
*
*       VACM_Init
*
*   DESCRIPTION
*
*       This function initializes VACM.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS      Initialization was successful.
*       SNMP_ERROR      Initialization was unsuccessful.
*
*************************************************************************/
STATUS VACM_Init(VOID)
{

#if ((VACM_CONTEXT_TBL_SIZE > 0) || (SNMP_ENABLE_FILE_STORAGE == NU_TRUE))
    UINT8                   i;
#endif

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

    /* Temporary instances used to store information read from file. */
    VACM_SEC2GROUP_STRUCT   sec2group;
    VACM_ACCESS_STRUCT      access;
    VACM_VIEWTREE_STRUCT    view_tree;

    /* Information that will be passed to the file component. */
    SNMP_READ_FILE          table_information[3] = {

        /* File name, Structure pointer, Sizeof structure, Insert
         * function.
         */
        {VACM_SEC2GROUP_STRUCT_FILE, NU_NULL,
         (SNMP_ADD_MIB_ENTRY)VACM_InsertGroup,
         sizeof(VACM_SEC2GROUP_STRUCT)},
        {VACM_ACCESS_STRUCT_FILE, NU_NULL,
         (SNMP_ADD_MIB_ENTRY)VACM_InsertAccessEntry,
         sizeof(VACM_ACCESS_STRUCT)},
        {VACM_VIEWTREE_STRUCT_FILE, NU_NULL,
         (SNMP_ADD_MIB_ENTRY)VACM_InsertMibView,
         sizeof(VACM_VIEWTREE_STRUCT)}
    };

#endif

    STATUS                  status = SNMP_ERROR;

#if (VACM_CONTEXT_TBL_SIZE > 0)

    /* Add the context names. */
    for(i = 0; i < VACM_CONTEXT_TBL_SIZE; i++)
    {
        if (VACM_Add_Context(Snmp_Cfg_Context_Names[i]) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: VACM_Add_Context failed.",
                            NERR_SEVERE, __FILE__, __LINE__);
        }
    }

#endif

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

    /* Update the read pointers to point to the temporary variables that
     * will store the read information.
     */
    table_information[0].snmp_read_pointer = &sec2group;
    table_information[1].snmp_read_pointer = &access;
    table_information[2].snmp_read_pointer = &view_tree;

    /* In this loop we initialize all the three tables: Security 2 Group,
     * Access and MIB view.
     */
    for(i = 0; i < 3; i++)
    {
        status = SNMP_Read_File(&table_information[i]);

        if(status != NU_SUCCESS)
        {
            Vacm_Status = SNMP_MODULE_NOTINITIALIZED;
            break;
        }
    }

    /* If all files were correctly read. */
    if(status == NU_SUCCESS)
    {
        /* VACM has been successfully initialized. */
        Vacm_Status = SNMP_MODULE_INITIALIZED;
    }
#else

    Vacm_Status = SNMP_MODULE_NOTINITIALIZED;

#endif

    return (status);

} /* VACM_Init */


/************************************************************************
*
*   FUNCTION
*
*       VACM_Config
*
*   DESCRIPTION
*
*       This function configures VACM.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS          Operation successful.
*
*************************************************************************/
STATUS VACM_Config(VOID)
{

#if (SNMP_CONFIG_SERIAL == NU_TRUE)
    CHAR            input[2];
#endif

#if (VACM_SEC2GRP_TBL_SIZE > 0)
    VACM_SEC2GROUP_STRUCT   sec2group;
#endif

#if (VACM_ACCESS_TBL_SIZE > 0)
    VACM_ACCESS_STRUCT      access;
#endif

#if (VACM_MIB_VIEW_TBL_SIZE > 0)
    VACM_VIEWTREE_STRUCT    viewtree;
#endif

    /* config_option determines the initial configuration. The following
     * values are meaningful:
     *
     * 1. Default configuration as specified in snmp_cfg.c
     * 2. No access configuration
     */
#if ((VACM_SEC2GRP_TBL_SIZE > 0) || (VACM_ACCESS_TBL_SIZE > 0) || \
     (VACM_MIB_VIEW_TBL_SIZE > 0) || (SNMP_CONFIG_SERIAL == NU_TRUE))
    UINT8           config_option = 1;
#endif

#if ((VACM_SEC2GRP_TBL_SIZE > 0) || (VACM_ACCESS_TBL_SIZE > 0) || \
     (VACM_MIB_VIEW_TBL_SIZE > 0))
    UINT8           i;
#endif

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */


#if (SNMP_CONFIG_SERIAL == NU_TRUE)

    /* Null terminate. */
    input[1] = '\0';

#endif

    /* Wait for the VACM to be initialized. */
    while(Vacm_Status == SNMP_MODULE_NOTSTARTED)
    {
        /* Sleep for a second. */
        NU_Sleep(TICKS_PER_SECOND);
    }

    /* Check whether the initialization has succeeded. */
    if(Vacm_Status == SNMP_MODULE_NOTINITIALIZED)
    {
        /* The initialization did not succeed. Manual configuration is now
         * required.
         */

#if (SNMP_CONFIG_SERIAL == NU_TRUE)

        /* Determine the Configuration Option required by the user. */

        /* This is the VACM Configuration Wizard. */
        NU_SD_Put_String("*************************************",
                         Snmp_Serial_Port);
        NU_SD_Put_String("*************************************\n\r",
                        Snmp_Serial_Port);
        NU_SD_Put_String("Copyright (c) 1998 - 2007 MGC - Nucleus ",
                          Snmp_Serial_Port);
        NU_SD_Put_String("SNMP - VACM Configuration Wizard",
                         Snmp_Serial_Port);
        NU_SD_Put_String("\n\r", Snmp_Serial_Port);
        NU_SD_Put_String("*************************************",
                         Snmp_Serial_Port);
        NU_SD_Put_String("*************************************\n\r",
                         Snmp_Serial_Port);
        NU_SD_Put_String("1. Default configuration as specified in "
                         "snmp_cfg.c", Snmp_Serial_Port);
        NU_SD_Put_String("\n\r", Snmp_Serial_Port);
        NU_SD_Put_String("2. No access configuration", Snmp_Serial_Port);
        NU_SD_Put_String("\n\r", Snmp_Serial_Port);

        NU_SD_Put_String("\n\n\r", Snmp_Serial_Port);
        NU_SD_Put_String("Enter Configuration Option: ",
                         Snmp_Serial_Port);

        /* Wait for an input. */
        for(;;)
        {
            while (!NU_SD_Data_Ready(Snmp_Serial_Port));

            input[0] = SDC_Get_Char(Snmp_Serial_Port);

            /* Convert from hexadecimal to digit. */
            config_option = (UINT8)NU_AHTOI(input);

            if(config_option <= 2 && config_option >= 1)
            {
                /* Echo back the input. */
                NU_SD_Put_Char(input[0], Snmp_Serial_Port, NU_FALSE);

                /* Break out of the loop. */
                break;
            }
        }

#endif

#if ((VACM_SEC2GRP_TBL_SIZE > 0) || (VACM_ACCESS_TBL_SIZE > 0) || \
     (VACM_MIB_VIEW_TBL_SIZE > 0))
        /* If the configuration posture is as defined in snmp_cfg.c. */
        if(config_option == 1)
        {

#if (VACM_SEC2GRP_TBL_SIZE > 0)

            /* Clear the structure. */
            UTL_Zero(&sec2group, sizeof(VACM_SEC2GROUP_STRUCT));

#if (INCLUDE_MIB_VACM == NU_TRUE)
            /* Set the storage type and row status. */
            sec2group.vacm_storage_type = SNMP_STORAGE_DEFAULT;

            sec2group.vacm_status = SNMP_ROW_ACTIVE;
#endif

            /* Add the Security 2 Group entries. */
            for (i = 0; i < VACM_SEC2GRP_TBL_SIZE; i++)
            {
                /* Copy the packed structure in to the full structure. */
                sec2group.vacm_security_model =
                    Snmp_Cfg_Sec2Groups[i].vacm_security_model;

                strcpy((CHAR *)sec2group.vacm_security_name,
                       (CHAR *)Snmp_Cfg_Sec2Groups[i].vacm_security_name);

                strcpy((CHAR *)sec2group.vacm_group_name,
                       (CHAR *)Snmp_Cfg_Sec2Groups[i].vacm_group_name);

                /* Insert the entry. */
                if(VACM_InsertGroup(&sec2group) != NU_SUCCESS)
                {
                    /* If en error occurred, make a log entry. */
                    NLOG_Error_Log(
                            "Unable to add Security 2 Group entry",
                            NERR_SEVERE, __FILE__, __LINE__);
                }
            }

#endif

#if (VACM_ACCESS_TBL_SIZE > 0)

            /* Clear the structure. */
            UTL_Zero(&access, sizeof(VACM_ACCESS_STRUCT));

#if (INCLUDE_MIB_VACM == NU_TRUE)
            /* Set the storage type and row status. */
            access.vacm_storage_type = SNMP_STORAGE_DEFAULT;

            access.vacm_status = SNMP_ROW_ACTIVE;
#endif

            /* Add the Access entries. */
            for (i = 0; i < VACM_ACCESS_TBL_SIZE; i++)
            {
                /* Copy the packed structure in to the full structure. */
                access.vacm_security_model =
                    Snmp_Cfg_Access[i].vacm_security_model;

                strcpy((CHAR *)access.vacm_group_name,
                       (CHAR *)Snmp_Cfg_Access[i].vacm_group_name);

                strcpy((CHAR *)access.vacm_context_prefix,
                       (CHAR *)Snmp_Cfg_Access[i].vacm_context_prefix);

                access.vacm_security_level =
                    Snmp_Cfg_Access[i].vacm_security_level;

                access.vacm_context_match =
                    Snmp_Cfg_Access[i].vacm_context_match;

                strcpy((CHAR *)access.vacm_read_view,
                       (CHAR *)Snmp_Cfg_Access[i].vacm_read_view);

                strcpy((CHAR *)access.vacm_write_view,
                       (CHAR *)Snmp_Cfg_Access[i].vacm_write_view);

                strcpy((CHAR *)access.vacm_notify_view,
                       (CHAR *)Snmp_Cfg_Access[i].vacm_notify_view);

                /* Insert the entry. */
                if(VACM_InsertAccessEntry(&access) != NU_SUCCESS)
                {
                    /* If en error occurred, make a log entry. */
                    NLOG_Error_Log(
                            "Unable to add Access entry",
                            NERR_SEVERE, __FILE__, __LINE__);
                }
            }

#endif

#if (VACM_MIB_VIEW_TBL_SIZE > 0)

            /* Clear the structure. */
            UTL_Zero(&viewtree, sizeof(VACM_VIEWTREE_STRUCT));

#if (INCLUDE_MIB_VACM == NU_TRUE)
            /* Set the storage type and row status. */
            viewtree.vacm_storage_type = SNMP_STORAGE_DEFAULT;

            viewtree.vacm_status = SNMP_ROW_ACTIVE;
#endif

            /* Add the MIB view entries. */
            for (i = 0; i < VACM_MIB_VIEW_TBL_SIZE; i++)
            {
                /* Copy the packed structure in to the full structure. */
                viewtree.vacm_subtree_len =
                   Snmp_Cfg_Mib_View[i].vacm_subtree_len;

                NU_BLOCK_COPY(viewtree.vacm_subtree,
                              Snmp_Cfg_Mib_View[i].vacm_subtree,
                              viewtree.vacm_subtree_len * sizeof(UINT32));

                viewtree.vacm_mask_length =
                   Snmp_Cfg_Mib_View[i].vacm_mask_length;

                NU_BLOCK_COPY(viewtree.vacm_family_mask,
                              Snmp_Cfg_Mib_View[i].vacm_family_mask,
                              viewtree.vacm_mask_length);

                strcpy((CHAR *)viewtree.vacm_view_name,
                       (CHAR *)Snmp_Cfg_Mib_View[i].vacm_view_name);

                viewtree.vacm_family_type =
                   Snmp_Cfg_Mib_View[i].vacm_family_type;

                /* Insert the entry. */
                if(VACM_InsertMibView(&viewtree) != NU_SUCCESS)
                {
                    /* If en error occurred, make a log entry. */
                    NLOG_Error_Log(
                            "Unable to add Access entry",
                            NERR_SEVERE, __FILE__, __LINE__);
                }
            }

#endif
        }
#endif

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

        /* Create the files. */
        SNMP_Create_File(VACM_SEC2GROUP_STRUCT_FILE);
        SNMP_Create_File(VACM_ACCESS_STRUCT_FILE);
        SNMP_Create_File(VACM_VIEWTREE_STRUCT_FILE);

        /* Save the configurations to file. */
        VACM_Save_Group(Vacm_Mib_Table.vacm_security_to_group_tab.next);
        VACM_Save_Access(Vacm_Mib_Table.vacm_access_tab.next);
        VACM_Save_View(Vacm_Mib_Table.vacm_view_tree_family_tab.next);

#endif /* (SNMP_ENABLE_FILE_STORAGE == NU_TRUE) */

        /* VACM is now initialized. */
        Vacm_Status = SNMP_MODULE_INITIALIZED;

#if (SNMP_CONFIG_SERIAL == NU_TRUE)

        NU_SD_Put_String("\n\r", Snmp_Serial_Port);
        NU_SD_Put_String("VACM successfully configured!",
                         Snmp_Serial_Port);
        NU_SD_Put_String("\n\r", Snmp_Serial_Port);
        NU_SD_Put_String("Quitting Configuration Wizard",
                         Snmp_Serial_Port);
        NU_SD_Put_String("\n\n\n\r", Snmp_Serial_Port);
#endif

    }

    /* return to user mode */
    NU_USER_MODE();
    return (NU_SUCCESS);

} /* VACM_Config */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Search_Context
*
*   DESCRIPTION
*
*       This function searches whether incoming context name is present in
*       the context table.
*
*   INPUTS
*
*       *context_name       Context name.
*       *loc_ptr            Pointer which returns the found node.
*
*   OUTPUTS
*
*       NU_SUCCESS          Context name is present in the table.
*       SNMP_ERROR          Context name is not present in the table.
*       SNMP_BAD_PARAMETER  Null pointer passed in. 
*
*************************************************************************/
STATUS VACM_Search_Context(const UINT8 *context_name,
                           VACM_CONTEXT_STRUCT **loc_ptr)
{
    STATUS                  status = SNMP_ERROR;
    VACM_CONTEXT_STRUCT     *location_ptr =
                                Vacm_Mib_Table.vacm_context_table.next;
    INT                     cmp;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if ((context_name != NU_NULL) && (loc_ptr != NU_NULL))
    {
        *loc_ptr = NU_NULL;

        /* Go through the list to find the required context name. */
        while(location_ptr != NU_NULL)
        {
            /* Compare the context names. */
            cmp = strcmp(location_ptr->context_name,
                         (CHAR *)context_name);

            /* If the strings matched. */
            if(cmp == 0)
            {
                /* We found the context we were looking for. Set the
                 * status to success and return pointer to this context.
                 */
                *loc_ptr = location_ptr;
                status = NU_SUCCESS;
                break;
            }

            /* Go to the next node. */
            location_ptr = location_ptr->next;
        }
    }
    else
    {
        if (loc_ptr != NU_NULL)
            *loc_ptr = NU_NULL;
        status = SNMP_BAD_PARAMETER;
    }
    
    NU_USER_MODE();
    return (status);

} /* VACM_Search_Context */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Search_Group
*
*   DESCRIPTION
*
*       This function searches for a group given a security model and
*       security name. The found entry is returned through the return
*       pointer.
*
*   INPUTS
*
*       snmp_sm             Security model.
*       *security_name      Security name.
*       *location_ptr       Pointer which returns the found node.
*
*   OUTPUTS
*
*       NU_SUCCESS          Entry found.
*       SNMP_ERROR          Entry not found.
*       SNMP_BAD_PARAMETER  Invalid parameters
*
*************************************************************************/
STATUS VACM_Search_Group(UINT32 snmp_sm, const UINT8 *security_name,
                         VACM_SEC2GROUP_STRUCT **location_ptr)
{
    STATUS                  status = SNMP_ERROR;
    VACM_SEC2GROUP_STRUCT   *dummy_ptr =
                           Vacm_Mib_Table.vacm_security_to_group_tab.next;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if ((security_name != NU_NULL) && (location_ptr != NU_NULL))
    {
        *location_ptr = NU_NULL;

        /* Go through the whole list. */
        while(dummy_ptr != NU_NULL)
        {
            /* Compare the security model and the security name. */
            if((dummy_ptr->vacm_security_model == snmp_sm) &&
               (strcmp((CHAR *)dummy_ptr->vacm_security_name,
                       (CHAR *)security_name) == 0))
            {
                /* We found the entry we were looking for. Set the status
                 * to success and return pointer to this entry.
                 */
                status = NU_SUCCESS;
                *location_ptr = dummy_ptr;
                break;
            }

            /* Go to the next entry. */
            dummy_ptr = dummy_ptr->next;
        }
    }

    else
    {
        if (location_ptr != NU_NULL)
            *location_ptr = NU_NULL;
        status = SNMP_BAD_PARAMETER;
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* VACM_Search_Group */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Search_AccessEntry
*
*   DESCRIPTION
*
*       This function searches for an access entry which matches the
*       passed group name, context prefix, security model and security
*       level.
*
*   INPUTS
*
*       *group_name             Group name.
*       *context_prefix         Context prefix.
*       snmp_sm                 Security model.
*       *security_level         Security level.
*       *location_ptr           Pointer which returns the found node.
*
*   OUTPUTS
*
*       NU_SUCCESS              Entry found.
*       SNMP_ERROR              Entry not found.
*
*************************************************************************/
STATUS VACM_Search_AccessEntry(const UINT8 *group_name,
                               const UINT8 *context_prefix,
                               UINT32 snmp_sm, UINT8 security_level,
                               VACM_ACCESS_STRUCT **location_ptr)
{
    STATUS                  status = SNMP_ERROR;
    VACM_ACCESS_STRUCT      *dummy_ptr =
                                Vacm_Mib_Table.vacm_access_tab.next;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if ( (group_name != NU_NULL) &&
         (context_prefix != NU_NULL) &&
         (location_ptr != NU_NULL) )
    {
        *location_ptr = NU_NULL;

        /* Go through the whole list till we find the entry we were
         * looking for or we exhaust the list.
         */
        while(dummy_ptr != NU_NULL)
        {
            /* Compare the group name, context prefix, security model and
             * security name.
             */
            if((strcmp((CHAR *)dummy_ptr->vacm_group_name,
                       (CHAR *)group_name) == 0) &&
               (strcmp((CHAR *)dummy_ptr->vacm_context_prefix,
                       (CHAR *)context_prefix) == 0)&&
               (dummy_ptr->vacm_security_model == snmp_sm) &&
               (dummy_ptr->vacm_security_level == security_level))

            {
                /* We found the entry we were looking for. Set the status
                 * to success and return pointer to this entry.
                 */
                status = NU_SUCCESS;
                *location_ptr = dummy_ptr;
                break;
            }

            /* Go to the next entry. */
            dummy_ptr = dummy_ptr->next;
        }
    }

    else if (location_ptr != NU_NULL)
    {
        *location_ptr = NU_NULL;
    }

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* VACM_Search_AccessEntry */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Search_MibView
*
*   DESCRIPTION
*
*       This function searches for an MIB view entry which matches the
*       passed view name, subtree and subtree length.
*
*   INPUTS
*
*       *view_name              View name.
*       *subtree                Subtree for this view.
*       subtree_len             Length of the subtree.
*       *location_ptr           Pointer which returns the found node.
*
*   OUTPUTS
*
*       NU_SUCCESS              Entry found.
*       SNMP_ERROR              Entry not found.
*       SNMP_BAD_PARAMETER      Invalid parameters.
*
*************************************************************************/
STATUS VACM_Search_MibView(const UINT8 *view_name, const UINT32 *subtree,
                           UINT32 subtree_len,
                           VACM_VIEWTREE_STRUCT **location_ptr)
{
    STATUS                  status = SNMP_ERROR;
    VACM_VIEWTREE_STRUCT    *dummy_ptr =
                            Vacm_Mib_Table.vacm_view_tree_family_tab.next;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if ( (view_name != NU_NULL) &&
         (subtree != NU_NULL) &&
         (location_ptr != NU_NULL) )
    {
        *location_ptr = NU_NULL;
        
        /* Traverse the list till we find the node we were looking for or
         * the list gets exhausted.
         */
        while(dummy_ptr != NU_NULL)
        {
            /* Compare the view name sub-tree length and subtree. */
            if((strcmp((CHAR *)dummy_ptr->vacm_view_name,
                       (CHAR *)view_name) == 0) &&
               (MibCmpObjId(dummy_ptr->vacm_subtree,
                            (UINT32)dummy_ptr->vacm_subtree_len,
                            subtree,
                            (UINT32)subtree_len) == 0))
            {
                /* We found the entry we were looking for. Set the status
                 * to success and return pointer to this entry.
                 */
                status = NU_SUCCESS;
                *location_ptr = dummy_ptr;
                break;
            }

            /* Go to the next entry. */
            dummy_ptr = dummy_ptr->next;
        }
    }

    else 
    {
        if (location_ptr != NU_NULL)
        {
            *location_ptr = NU_NULL;
        }
        status = SNMP_BAD_PARAMETER;
    }
	

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* VACM_Search_MibView */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Filling_FamilyMask
*
*   DESCRIPTION
*
*       This function extends the family masks with 1's. For all
*       identifiers which are located outside the length of the mask
*       it is assumed that the mask was actually 1. This extension helps
*       us with this requirement.
*
*   INPUTS
*
*       *vacm_family_mask               Bit pattern representing the
*                                       family mask.
*       mask_len                        Length of the mask in octets.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID VACM_Filling_FamilyMask(UINT8 *vacm_family_mask, UINT32 mask_len)
{
    UINT32          i;

    /* Fill the remaining mask with 1's. */
    for(i = mask_len; i < VACM_MASK_SIZE; i++)
    {
        vacm_family_mask[i] = 0xFF;
    }

} /* VACM_Filling_FamilyMask */

/************************************************************************
*
*   FUNCTION
*
*       VACM_CheckInstance_Access
*
*   DESCRIPTION
*
*       1)  Looks up  a matching context entry for the passed context
*           name.
*
*       2)  Looks up the group corresponding to the passed security model
*           and security name from Security 2 Group table.
*
*       3)  Looks up an access entry corresponding to the group name,
*           context name, security model, security level and view type.
*
*       4)  Check access of the instance by looking up the MIB view
*           corresponding to the view type and determining whether the
*           passed OID is part of this MIB view.
*
*   INPUTS
*
*       snmp_sm                 Security model.
*       security_level          Security level.
*       *security_name          Security name.
*       *context_name           Context name.
*       view_type               Type of view.
*       *oid                    OID for which access is to be checked.
*       length                  Length of the OID.
*
*   OUTPUTS
*
*       NU_SUCCESS              Access is granted.
*       VACM_NOTINVIEW          An MIB view was found, but access is
*                               denied. The passed OID is not in view.
*       VACM_NOSUCHVIEW         An MIB view was not found.
*       VACM_NOSUCHCONTEXT      Context name specified was not found.
*       VACM_NOGROUPNAME        There was no group specified for the
*                               combination of security model and
*                               security name.
*       VACM_NOACCESSENTRY      Access entry was not found.
*
************************************************************************/
STATUS VACM_CheckInstance_Access (UINT32 snmp_sm, UINT8 security_level,
                                  UINT8 *security_name,
                                  UINT8 *context_name, UINT8 view_type,
                                  UINT32 *oid, UINT32 length)
{
    /* Status of the request. We start off with the assumption that access
     * is granted.
     */
    STATUS                      status;

    VACM_SEC2GROUP_STRUCT       *sec2group_obj;
    VACM_CONTEXT_STRUCT         *context_obj;
    VACM_ACCESS_STRUCT          *access_obj;
    CHAR                        *view_name;

    /* Look up a matching context entry for the passed name. */
    if(VACM_Search_Context(context_name, &context_obj) != NU_SUCCESS)
    {
        status = VACM_NOSUCHCONTEXT;
    }

    /* Look up a group corresponding to the passed security model
     * and security name.
     */
    else if(VACM_Search_Group(snmp_sm, security_name, &sec2group_obj) !=
                                                               NU_SUCCESS)
    {
        /* If we did not find such a group. */
        status = VACM_NOGROUPNAME;
    }

#if (INCLUDE_MIB_VACM == NU_TRUE)

    /* Make sure that the group is active. */
    else if(sec2group_obj->vacm_status != SNMP_ROW_ACTIVE)
    {
        /* If the group is in-active, this is a similar case to the
         * group not existing.
         */
        status = VACM_NOGROUPNAME;
    }

#endif

    /* Look up an access entry corresponding to the group name, context
     * name, security model, security level and view type.
     */
    else if(VACM_Find_AccessEntry((CHAR *)sec2group_obj->vacm_group_name,
                                  (CHAR *)context_name, snmp_sm,
                                  security_level, &access_obj) !=
                                                               NU_SUCCESS)
    {
        /* If we did not find a matching group. */
        status = VACM_NOACCESSENTRY;
    }

    /* Otherwise, we now look for the OID in the view specified by the
     * access entry.
     */
    else
    {
        /* Depending on the view type requested, determine the view
         * name.
         */
        switch(view_type)
        {

        default:

            /* We should always get one of the following three types
             * in our view. This case is just to get rid of compiler
             * warnings.
             */

        case VACM_READ_VIEW:

            /* Get the read view name. */
            view_name = (CHAR *)access_obj->vacm_read_view;
            break;

        case VACM_WRITE_VIEW:

            /* Get the write view name. */
            view_name = (CHAR *)access_obj->vacm_write_view;
            break;

        case VACM_NOTIFY_VIEW:

            /* Get the notify view name. */
            view_name = (CHAR *)access_obj->vacm_notify_view;
            break;
        }

        /* Make sure that a view name has been specified for the type we
         * require.
         */
        if(strlen(view_name) != 0)
        {
            /* Now find the passed OID in this view. */
            status = VACM_FindIn_MibView(view_name, oid, length);
        }

        /* Otherwise, there is no view specified. */
        else
        {
            status = VACM_NOSUCHVIEW;
        }
    }

    /* We are done with our access checks. Pass the result back to the
     * calling module.
     */
    return (status);

} /* VACM_CheckInstance_Access */

/*************************************************************************
*
*   FUNCTION
*
*       VACM_Get_Context_Name_Util
*
*   DESCRIPTION
*
*       This function is used to get the handle to the context name entry
*       by specifying context name.
*
*   INPUTS
*
*       *context_name       Pointer to the memory location where context
*                           name is stored.
*       context_name_len    Length of context name.
*       getflag             Flag to distinguish GET and GET-NEXT request.
*                            0) GET-NEXT Request.
*                            1) GET Request.
*                            2) GET-FIRST Request.
*
*   OUTPUTS
*
*       VACM_CONTEXT_STRUCT*    Successful.
*       NU_NULL                 Failure.
*
*************************************************************************/
VACM_CONTEXT_STRUCT *VACM_Get_Context_Name_Util(const UINT8 *context_name,
                                                UINT32 context_name_len,
                                                UINT8 getflag)
{
    /* Handle to the context name entry. */
    VACM_CONTEXT_STRUCT     *location_ptr =
                                Vacm_Mib_Table.vacm_context_table.next;

    /* Variable to hold comparison result. */
    INT                     cmp_result;

    /* If we are handling GET request. */
    if (getflag == 1)
    {
        /* Loop to find the context name entry with exact matching index
         * passed in.
         */
        while (location_ptr)
        {
            /* If we have reached at context name entry with exact
             * matching indexes as passed in then break through the loop.
             */
            if ((strlen(location_ptr->context_name) ==
                                                context_name_len) &&
                (strcmp(location_ptr->context_name, (CHAR *)context_name)
                                                            == 0))
            {
                /* Break through the loop. */
                break;
            }

            /* Moving forward in the list. */
            location_ptr = location_ptr->next;
        }
    }

    /* If we are handling GET-NEXT request. */
    else if (getflag == 0)
    {
        /* Loop to find the context name entry with greater indexes as
         * passed in.
         */
        while (location_ptr)
        {
            /* Comparing context name lengths. */
            cmp_result = (INT)(strlen(location_ptr->context_name)) -
                                                (INT)(context_name_len);

            /* If context name length of the current entry is equal to
             * context name length passed in then compare context name of
             * the current entry with context name passed in.
             */
            if (cmp_result == 0)
            {
                /* Comparing context name passed in with the context name
                 * of the current entry.
                 */
                cmp_result = strcmp(location_ptr->context_name,
                                    (CHAR *)context_name);
            }

            /* If we have reached at context name entry with greater
             * indexes as passed in then break through the loop.
             */
            if (cmp_result > 0)
            {
                /* Breaking through the loop. */
                break;
            }

            /* Moving forward in the list. */
            location_ptr = location_ptr->next;
        }
    }

    /* Returning handle to the context name entry if found. Otherwise
     * return NU_NULL.
     */
    return (location_ptr);

} /* VACM_Get_Context_Name_Util */

/*************************************************************************
*
*   FUNCTION
*
*       VACM_Get_Sec2Group_Entry_Util
*
*   DESCRIPTION
*
*       This function is used to get handle to the Security to Group
*       entry.
*
*   INPUTS
*
*       vacm_sec_model      Security model.
*       vacm_sec_name_len   Security name length.
*       *vacm_sec_name      Pointer to the memory location where security
*                           name is stored.
*       *root               Pointer to the root of the list containing
*                           security to group entries.
*
*   OUTPUTS
*
*       VACM_SEC2GROUP_STRUCT*  Successful.
*       NU_NULL                 Failure.
*
*************************************************************************/
VACM_SEC2GROUP_STRUCT *VACM_Get_Sec2Group_Entry_Util(
                        UINT32 vacm_sec_model, UINT32 vacm_sec_name_len,
                        const UINT8 *vacm_sec_name,
                        const VACM_SEC2GROUP_ROOT * root)
{
    /* Handle to security to group entries. */
    VACM_SEC2GROUP_STRUCT   *location_ptr = root->next;

    /* Loop to reach at entry that have exact matching indexes as
     * passed in.
     */
    while (location_ptr)
    {
        /* If we have reached at entry with exact matching indexes as
         * passed in then break through the loop.
         */
        if ((location_ptr->vacm_security_model == vacm_sec_model) &&
            (strlen((CHAR *)location_ptr->vacm_security_name) ==
                                                vacm_sec_name_len) &&
            (strcmp((CHAR *)location_ptr->vacm_security_name,
                                            (CHAR *)vacm_sec_name) == 0))
        {
            /* Break through the loop. */
            break;
        }

        /* Moving forward in the list. */
        location_ptr = location_ptr->next;
    }

    /* Returning handle to vacmSec2GroupEntry if found.
     * Otherwise return NU_NULL. */
    return (location_ptr);

} /* VACM_Get_Sec2Group_Entry_Util */

/*************************************************************************
*
*   FUNCTION
*
*       VACM_Get_Sec2Group_Entry
*
*   DESCRIPTION
*
*       This function is used to get handle to the Security to Group
*       entry.
*
*   INPUTS
*
*       vacm_sec_model      Security model.
*       vacm_sec_name_len   Security name length.
*       *vacm_sec_name      Pointer to the memory location where security
*                           name is stored.
*       getflag             Flag to distinguish GET and GET-NEXT request.
*
*   OUTPUTS
*
*       VACM_SEC2GROUP_STRUCT*  Successful.
*       NU_NULL                 Failure.
*
*************************************************************************/
VACM_SEC2GROUP_STRUCT *VACM_Get_Sec2Group_Entry(UINT32 vacm_sec_model,
                                                UINT32 vacm_sec_name_len,
                                                UINT8 *vacm_sec_name,
                                                UINT8 getflag)
{
    /* Variable to hold the return value. */
    VACM_SEC2GROUP_STRUCT   *location_ptr;

    /* Variable to hold comparison result. */
    INT                     cmp_result;

    /* If are handling GET request. */
    if (getflag)
    {
        /* Attempt to get required handle from permanent list. */
        location_ptr =
          VACM_Get_Sec2Group_Entry_Util(vacm_sec_model, vacm_sec_name_len,
                        vacm_sec_name,
                        &(Vacm_Mib_Table.vacm_security_to_group_tab));

#if (INCLUDE_MIB_VACM == NU_TRUE)
        /* If we did not get the required handle from permanent list then
         * attempt to get it from temporary list.
         */
        if (!location_ptr)
        {
            location_ptr = VACM_Get_Sec2Group_Entry_Util(vacm_sec_model,
                                        vacm_sec_name_len, vacm_sec_name,
                                        &(Vacm_Mib_Temp_Sec2Group_Table));
        }
#endif
    }

    /* If we are handling GET-NEXT request. */
    else
    {
        /* Start searching. */
        location_ptr = Vacm_Mib_Table.vacm_security_to_group_tab.next;

        /* Loop to find the entry that has greater indexes as passed in.
         */
        while (location_ptr)
        {
            /* Comparing security model of current entry with what is
             * passed in.
             */
            cmp_result = ((INT)(location_ptr->vacm_security_model) -
                          (INT)(vacm_sec_model));

            /* If security model of current entry and security model
             * passed are equal the compare security name length and
             * security name of current entry with what is passed in.
             */
            if (cmp_result == 0)
            {
                /* Comparing security name length of current entry with
                 * security name length passed in.
                 */
                cmp_result = ( (INT)(strlen((CHAR *)(location_ptr->
                                            vacm_security_name))) -
                               (INT)(vacm_sec_name_len) );

                /* If security name length of current entry is equal to
                 * security name length passed in then compare the value
                 * of security name.
                 */
                if (cmp_result == 0)
                {
                    /* Comparing security name of current entry with
                     * security name passed in.
                     */
                    cmp_result = strcmp((CHAR *)(location_ptr->
                            vacm_security_name), (CHAR *)vacm_sec_name);
                }
            }

            /* If we have reached at entry that have greater indexes as
             * passed in then break through the loop.
             */
            if (cmp_result > 0)
            {
                /* Break through the loop. */
                break;
            }

            /* Moving forward in the list. */
            location_ptr = location_ptr->next;
        }
    }

    /* Return handle to the vacmSec2GroupEntry if found.
     * Otherwise return NU_NULL.
     */
    return (location_ptr);

} /* VACM_Get_Sec2Group_Entry */

/*************************************************************************
*
*   FUNCTION
*
*       VACM_MIB_Get_AccessEntry_Util
*
*   DESCRIPTION
*
*       This function is used to get handle to the access entry.
*
*   INPUTS
*
*       vacm_group_name_len     Group name length.
*       *vacm_group_name        Pointer to the memory location where group
*                               name is stored.
*       vacm_context_prefix_len Context prefix length.
*       *vacm_context_prefix    Pointer to the memory location where
*                               context prefix is stored.
*       *root                   Pointer to the root of the list containing
*                               access entries.
*
*   OUTPUTS
*
*       VACM_ACCESS_STRUCT*     Successful.
*       NU_NULL                 Failure.
*
*************************************************************************/
VACM_ACCESS_STRUCT *VACM_MIB_Get_AccessEntry_Util(
               UINT32 vacm_group_name_len, const UINT8 *vacm_group_name,
               UINT32 vacm_context_prefix_len,
               const UINT8 *vacm_context_prefix, UINT32 vacm_sec_model,
               UINT32 vacm_sec_level, const VACM_ACCESS_ROOT *root)
{
    /* Handle to the VACM Access entry. */
    VACM_ACCESS_STRUCT *location_ptr = root->next;

    /* Loop to find the VACM entry that have exact matching indexes as
     * passed in.
     */
    while (location_ptr)
    {
        /* If we have reached at an entry that same indexes as passed in
         * then break through the loop.
         */
        if ( (strlen((CHAR *)(location_ptr->vacm_group_name))
                                                == vacm_group_name_len) &&
             (strcmp((CHAR *)(location_ptr->vacm_group_name),
                     (CHAR *)vacm_group_name) == 0) &&
             (strlen((CHAR *)(location_ptr->vacm_context_prefix)) ==
                                            vacm_context_prefix_len) &&
             (strcmp((CHAR *)(location_ptr->vacm_context_prefix),
                     (CHAR *)(vacm_context_prefix))
                                                                  == 0) &&
             (location_ptr->vacm_security_model == vacm_sec_model) &&
             (location_ptr->vacm_security_level == (UINT8)vacm_sec_level))
        {
            /* Break through the loop. */
            break;
        }

        /* Moving forward in the list. */
        location_ptr = location_ptr->next;
    }

    /* Return handle to the VACM access entry if found. Otherwise return
     * error code.
     */
    return (location_ptr);

} /* VACM_MIB_Get_AccessEntry_Util */

/*************************************************************************
*
*   FUNCTION
*
*       VACM_MIB_Get_AccessEntry
*
*   DESCRIPTION
*
*       This function is used to get handle to the access entry.
*
*   INPUTS
*
*       vacm_group_name_len     Group name length.
*       *vacm_group_name        Pointer to the memory location where group
*                               name is stored.
*       vacm_context_prefix_len Context prefix length.
*       *vacm_context_prefix    Pointer to the memory location where
*                               context prefix is stored.
*       getflag                 Flag to distinguish GET and GET-NEXT
*                               requests.
*
*   OUTPUTS
*
*       VACM_ACCESS_STRUCT*     Successful.
*       NU_NULL                 Failure.
*
*************************************************************************/
VACM_ACCESS_STRUCT *VACM_MIB_Get_AccessEntry(UINT32 vacm_group_name_len,
                   UINT8 *vacm_group_name, UINT32 vacm_context_prefix_len,
                   UINT8 *vacm_context_prefix, UINT32 vacm_sec_model,
                   UINT32 vacm_sec_level, UINT8 getflag)
{
    /* Handle to the VACM access entry. */
    VACM_ACCESS_STRUCT      *location_ptr;

    /* Variable to hold the comparison result. */
    INT                     cmp_result;

    /* If we are handling GET request. */
    if (getflag)
    {
#if (INCLUDE_MIB_VACM == NU_TRUE)
        /* Attempt to get handle to VACM access entry from temporary list.
         */
        location_ptr = VACM_MIB_Get_AccessEntry_Util(vacm_group_name_len,
                            vacm_group_name, vacm_context_prefix_len,
                            vacm_context_prefix, vacm_sec_model,
                            vacm_sec_level, &(VACM_Mib_Temp_Access_Root));

        /* If we did not get handle to VACM access entry from temporary
         * list then try to get it from permanent list.
         */
        if (!location_ptr)
#endif
        {
            /* Get handle to VACM access entry from permanent list. */
            location_ptr = VACM_MIB_Get_AccessEntry_Util(
                            vacm_group_name_len, vacm_group_name,
                            vacm_context_prefix_len, vacm_context_prefix,
                            vacm_sec_model, vacm_sec_level,
                            &Vacm_Mib_Table.vacm_access_tab);
        }
    }

    /* If we are handling GET_NEXT request. */
    else
    {
        /* Initialize local variable to start traversing the list. */
        location_ptr = Vacm_Mib_Table.vacm_access_tab.next;

        /* Loop to find VACM Access entry with greater indexes. */
        while (location_ptr)
        {
            /* Comparing group name length of current entry with group
             * name length passed in.
             */
            cmp_result = (INT)(strlen((CHAR *)(location_ptr->
                          vacm_group_name))) - (INT)(vacm_group_name_len);

            /* If group name length comparison does not result in
             * decision.
             */
            if (cmp_result == 0)
            {
                /* Comparing group name of current entry with group name
                 * length passed in.
                 */
                cmp_result = (INT)(strcmp((CHAR *)(location_ptr->
                                                vacm_group_name),
                                          (CHAR *)vacm_group_name));

                /* If group name comparison does result in decision. */
                if (cmp_result == 0)
                {
                    /* Compare context prefix length of current entry with
                     * context prefix length passed in.
                     */
                    cmp_result = ((INT)(strlen((CHAR *)(location_ptr->
                                                vacm_context_prefix))) -
                                  (INT)(vacm_context_prefix_len));

                    /* If context prefix length comparison does not result
                     * in decision.
                     */
                    if (cmp_result == 0)
                    {
                        /* Compare context prefix of current entry with
                         * context prefix passed in.
                         */
                        cmp_result = strcmp((CHAR *)(location_ptr->
                                                    vacm_context_prefix),
                                            (CHAR *)vacm_context_prefix);

                        /* If context prefix comparison does result in
                         * decision.
                         */
                        if (cmp_result == 0)
                        {
                            /* Compare security model of current entry
                             * with security model passed in.
                             */
                            cmp_result = ((INT)(location_ptr->
                                                vacm_security_model) -
                                          (INT)(vacm_sec_model));

                            /* If security model comparison does result in
                             * decision.
                             */
                            if (cmp_result == 0)
                            {
                                /* Compare security level of current entry
                                 * with security level passed in.
                                 */
                                cmp_result = ((INT)(location_ptr->
                                                    vacm_security_level) -
                                              (INT)(vacm_sec_level));
                            }
                        }
                    }
                }
            }

            /* If we have reached at VACM Access entry with greater index
             * as passed in then break through the loop.
             */
            if (cmp_result > 0)
            {
                break;
            }

            /* Moving forward in the list. */
            location_ptr = location_ptr->next;
        }
    }

    /* Return handle to the VACM access entry if found. Otherwise return
     * NU_NULL.
     */
    return (location_ptr);

} /* VACM_MIB_Get_AccessEntry */

/*************************************************************************
*
*   FUNCTION
*
*       VACM_GetViewTree_Entry_Util
*
*   DESCRIPTION
*
*       This function is used to get handle to the view tree family entry.
*
*   INPUTS
*
*       vacm_view_name_len      View name length.
*       *vacm_view_name         Pointer to the memory location where view
*                               name is stored.
*       vacm_subtree_len        Sub-tree length.
*       *vacm_subtree           Pointer to the memory location where
*                               sub-tree is stored.
*       *root                   Pointer to the root of the list containing
*                               view tree family entries.
*
*   OUTPUTS
*
*       VACM_VIEWTREE_STRUCT*   Successful.
*       NU_NULL                 Failure.
*
*************************************************************************/
VACM_VIEWTREE_STRUCT *VACM_GetViewTree_Entry_Util(
                UINT32 vacm_view_name_len, const UINT8 *vacm_view_name,
                UINT32 vacm_subtree_len, const UINT32 *vacm_subtree,
                const VACM_VIEWTREE_ROOT *root)
{
    /* Handle to VACM view tree entry. */
    VACM_VIEWTREE_STRUCT    *location_ptr = root->next;

    /* Loop to find an entry that exact matching indexes as passed in. */
    while (location_ptr)
    {
        /* If we have reached at entry that have exact matching indexes as
         * passed in then break through the loop.
         */
        if ((strlen((CHAR *)(location_ptr->vacm_view_name)) ==
                                                    vacm_view_name_len) &&
            (strcmp((CHAR *)(location_ptr->vacm_view_name),
                    (CHAR *)(vacm_view_name)) == 0) &&
            (location_ptr->vacm_subtree_len == vacm_subtree_len) &&
            (memcmp(location_ptr->vacm_subtree, vacm_subtree,
                    vacm_subtree_len * sizeof(UINT32)) == 0))
        {
            /* Breaking through the loop. */
            break;
        }

        /* Moving forward in the list. */
        location_ptr = location_ptr->next;
    }

    /* Return handle to the view tree family entry if found. Otherwise
     * return NU_NULL.
     */
    return (location_ptr);

} /* VACM_GetViewTree_Entry_Util */

/*************************************************************************
*
*   FUNCTION
*
*       VACM_GetViewTreeFamily_Entry
*
*   DESCRIPTION
*
*       This function is used to get handle to the view tree family entry.
*
*   INPUTS
*
*       vacm_view_name_len      View name length.
*       *vacm_view_name         Pointer to the memory location where view
*                               name is stored.
*       vacm_subtree_len        Sub-tree length.
*       *vacm_subtree           Pointer to the memory location where
*                               sub-tree is stored.
*       getflag                 Flag to distinguish GET and GET-NEXT
*                               request.
*
*   OUTPUTS
*
*       VACM_VIEWTREE_STRUCT*   Successful.
*       NU_NULL                 Failure.
*
*************************************************************************/
VACM_VIEWTREE_STRUCT *VACM_GetViewTreeFamily_Entry(
                        UINT32 vacm_view_name_len, UINT8 *vacm_view_name,
                        UINT32 vacm_subtree_len, UINT32 *vacm_subtree,
                        UINT8 getflag)
{
    /* Handle to view tree family entry. */
    VACM_VIEWTREE_STRUCT    *location_ptr;

    /* Variable to hole Comparison result. */
    INT                     cmp_result;

    /* If we are handling GET request. */
    if (getflag)
    {
        /* Get handle to the view tree family entry from permanent entry
         * list.
         */
        location_ptr = VACM_GetViewTree_Entry_Util(
                            vacm_view_name_len, vacm_view_name,
                            vacm_subtree_len, vacm_subtree,
                            &(Vacm_Mib_Table.vacm_view_tree_family_tab) );

#if (INCLUDE_MIB_VACM == NU_TRUE)
        /* If we did not get the handle to the view family tree entry from
         * permanent list then get the handle to the view family tree
         * entry from temporary list.
         */
        if (!location_ptr)
        {
            /* Get the handle of view tree family entry from temporary
             * list.
             */
            location_ptr = VACM_GetViewTree_Entry_Util(
                                    vacm_view_name_len, vacm_view_name,
                                    vacm_subtree_len, vacm_subtree,
                                    &(Vacm_Mib_Temp_View_Family_Root) );
        }
#endif
    }

    /* If we are handling GET-NEXT request. */
    else
    {
        /* Get starting handle to start traversing. */
        location_ptr = Vacm_Mib_Table.vacm_view_tree_family_tab.next;

        /* Loop to get handle of entry that have greater indexes as passed
         * in.
         */
        while (location_ptr)
        {
            /* Comparing view name length of current entry with what is
             * passed in.
             */
            cmp_result = (INT)(strlen((CHAR *)(location_ptr->
                            vacm_view_name))) - (INT)(vacm_view_name_len);

            /* If view name length comparison does not result in a
             * decision.
             */
            if (cmp_result == 0)
            {
                /* Comparing view name of current entry with view name
                 * passed in.
                 */
                cmp_result = strcmp((CHAR *)(location_ptr->
                                                vacm_view_name),
                                    (CHAR *)(vacm_view_name));

                /* If view name comparison does not result in decision. */
                if (cmp_result == 0)
                {
                    /* Comparing subtree length of current entry with
                     * subtree length passed in.
                     */
                    cmp_result = ((INT)(location_ptr->vacm_subtree_len)) -
                                 ((INT)(vacm_subtree_len));

                    /* If subtree length comparison does not result in
                     * decision.
                     */
                    if (cmp_result == 0)
                    {
                        /* Comparing subtree value of current entry with
                         * subtree value passed in.
                         */
                        cmp_result = memcmp(location_ptr->vacm_subtree,
                            vacm_subtree, location_ptr->vacm_subtree_len  * sizeof(UINT32));
                    }
                }
            }

            /* If we have reached at entry with greater indexes as passed
             * in.
             */
            if (cmp_result > 0)
            {
                break;
            }

            /* Moving forward in the list. */
            location_ptr = location_ptr->next;
        }
    }

    /* Return handle to the view tree family entry if found. Otherwise
     * return NU_NULL.
     */
    return (location_ptr);

} /* VACM_GetViewTreeFamily_Entry */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Find_AccessEntry
*
*   DESCRIPTION
*
*       Finds an access entry that
*
*   INPUTS
*
*       *group_name          Group Name.
*       *context_name        Context Name.
*       snmp_sm              Security Model.
*       security_level       Security Level.
*       **location_ptr       Pointer to found access entry.
*
*   OUTPUTS
*
*       NU_SUCCESS          Matching entry found.
*       SNMP_ERROR          Matching entry not found.
*
************************************************************************/
STATUS VACM_Find_AccessEntry(const CHAR *group_name,
                             const CHAR *context_name,
                             UINT32 snmp_sm, UINT8 security_level,
                             VACM_ACCESS_STRUCT **location_ptr)
{
    /* Status of the request. We start off with the assumption that we
     * will not find a relevant entry.
     */
    STATUS                      status = SNMP_ERROR;

    /* Pointer used to traverse the access entries. */
    VACM_ACCESS_STRUCT          *dummy_ptr;

    /* Array of pointers that contains the selected candidates. */
    VACM_ACCESS_STRUCT          *selected_entries[VACM_SELECTENTRY_SIZE];

    /* Number of selected candidates. */
    UINT8                       selected_no = 0;

    /* Variables for string lengths. */
    INT                         str_len1;
    INT                         str_len2;

    /* Variable for loops. */
    UINT8                       i;

    /* Get the string length for the context name. This is used with in
     * the loop we get it here to avoid doing this for each iteration.
     */
    str_len1 = strlen(context_name);

    /* Go through the whole list and find all matching entries. */
    dummy_ptr = Vacm_Mib_Table.vacm_access_tab.next;

    while(dummy_ptr != NU_NULL)
    {

#if (INCLUDE_MIB_VACM == NU_TRUE)

        /* Make sure that the entry is active. */
        if(dummy_ptr->vacm_status == SNMP_ROW_ACTIVE)

#endif
        {
            /* Compare the group names. */
            if(strcmp((CHAR *)dummy_ptr->vacm_group_name, group_name)
                                                                == 0)
            {
                /* Get the string length for context prefix. */
                str_len2 = strlen((CHAR *)dummy_ptr->vacm_context_prefix);

                /* Compare the prefixes. If it is an exact match, just do
                 * a complete memory comparison, otherwise, do a memory
                 * comparison of up to the length of the prefix.
                 */
                if(((dummy_ptr->vacm_context_match ==
                                                VACM_CTXTMATCH_EXACT) &&
                    (str_len1 == str_len2) &&
                    (memcmp(dummy_ptr->vacm_context_prefix,
                            context_name, str_len2) == 0)) ||
                    ((dummy_ptr->vacm_context_match ==
                                                VACM_CTXTMATCH_PREFIX) &&
                    (str_len2 <= str_len1) &&
                    (memcmp(dummy_ptr->vacm_context_prefix,
                            context_name, str_len2) == 0)))
                {
                    /* Now we need to make sure that either the security
                     * model for the entry matches, or ANY security model
                     * is specified. The security level for the entry
                     * should also be less than or equal to the security
                     * level specified.
                     */
                    if(((dummy_ptr->vacm_security_model == snmp_sm) ||
                        (dummy_ptr->vacm_security_model == VACM_ANY)) &&
                       (dummy_ptr->vacm_security_level <= security_level))
                    {
                        if (selected_no < VACM_SELECTENTRY_SIZE)
                        {
                            /* This is the matching entry that we were
                             * looking for. Set this entry as a candidate.
                             */
                            selected_entries[selected_no] = dummy_ptr;

                            /* Increment the number of selected
                             * candidates.
                             */
                            selected_no++;
                        }
                    }
                }
            }
        }

        /* Move to the next entry. */
        dummy_ptr = dummy_ptr->next;
    }

    /* If we found more at least one entry. */
    if(selected_no > 0)
    {
        /* Select the first entry is our candidate. */
        *location_ptr = selected_entries[0];

        /* If we found more than one entry than we need to short-list the
         * best matching entry.
         */
        for(i = 1; (i < selected_no) && (i < VACM_SELECTENTRY_SIZE); i++)
        {
            /* If the security model for the current entry is
             * 'any' and the security model in the current candidate is
             * a not 'any' then our candidate is better and we will
             * ignore this entry.
             */
            if((selected_entries[i]->vacm_security_model == VACM_ANY) &&
               ((*location_ptr)->vacm_security_model != VACM_ANY))
            {
                /* Move to the next entry. */
                continue;
            }

            /* Otherwise, if the security model for the current entry is
             * not 'any' and the security model in the current candidate
             * is 'any' then the current entry is a better candidate.
             */
            else if((selected_entries[i]->vacm_security_model !=
                                                            VACM_ANY) &&
                    ((*location_ptr)->vacm_security_model == VACM_ANY))
            {
                /* Update the candidate to this entry. */
                *location_ptr = selected_entries[i];

                /* Move to the next entry. */
                continue;
            }

            /* Otherwise, if the context match for the current entry is
             * 'prefix' and the context match in the current candidate is
             * 'exact' then our candidate is better and we will ignore
             * this entry.
             */
            else if((selected_entries[i]->vacm_context_match ==
                     VACM_CTXTMATCH_PREFIX) &&
                    ((*location_ptr)->vacm_context_match ==
                     VACM_CTXTMATCH_EXACT))
            {
                /* Move to the next entry. */
                continue;
            }

            /* Otherwise, if the context match for the current entry is
             * 'exact' and the context match in the current candidate is
             * 'prefix' then the current entry is a better candidate.
             */
            else if((selected_entries[i]->vacm_context_match ==
                     VACM_CTXTMATCH_EXACT) &&
                    ((*location_ptr)->vacm_context_match ==
                     VACM_CTXTMATCH_PREFIX))
            {
                /* Update the candidate to this entry. */
                *location_ptr = selected_entries[i];

                /* Move to the next entry. */
                continue;
            }

            /* Otherwise, if this entry has a smaller context prefix,
             * than our current entry is better.
             */
            else if(strlen((CHAR *)selected_entries[i]->
                                                    vacm_context_prefix) <
                   strlen((CHAR *)((*location_ptr)->vacm_context_prefix)))
            {
                /* Move to the next entry. */
                continue;
            }

            /* Otherwise, if this entry has a larger context prefix,
             * than make this entry our choice.
             */
            else if(strlen((CHAR *)selected_entries[i]->
                                    vacm_context_prefix) >
                    strlen((CHAR *)((*location_ptr)->
                                        vacm_context_prefix)))
            {
                /* Update the candidate to this entry. */
                *location_ptr = selected_entries[i];

                /* Move to the next entry. */
                continue;
            }

            /* Otherwise, if this entry has a higher security level
             * than make this entry our choice.
             */
            else if (selected_entries[i]->vacm_security_level >
                     (*location_ptr)->vacm_security_level)
            {
                /* Update the candidate to this entry. */
                *location_ptr = selected_entries[i];

                /* Move to the next entry. */
                continue;
            }
        }

        /* We found a matching entry. Update the status. */
        status = NU_SUCCESS;
    }


    return (status);
} /* VACM_Find_AccessEntry */

/************************************************************************
*
*   FUNCTION
*
*       VACM_FindIn_MibView
*
*   DESCRIPTION
*
*       This function determines whether the passed OID is part of the
*       passed MIB view.
*
*   INPUTS
*
*       *view_name                  View Name.
*       *in_oid                     OID.
*       *in_oid_len                 Length of OID.
*
*   OUTPUTS
*
*       NU_SUCCESS                  Passed OID is part of view.
*       VACM_NOTINVIEW              OID is not in view.
*       VACM_ENDOFVIEW              We reached the end of view, and the
*                                   OID is outside this view.
*
*************************************************************************/
STATUS VACM_FindIn_MibView(const CHAR *view_name, UINT32 *in_oid,
                           UINT32 in_oid_len)
{
    /* Status of the request. We assume that the OID is not in view
     * till we are proved wrong. */
    STATUS                  status = VACM_NOTINVIEW;

    /* Pointer used to traverse the list of view entries. */
    VACM_VIEWTREE_STRUCT    *dummy_ptr;

    /* Pointer to the current candidate that will determine the inclusion
     * or exclusion of the OID from view.
     */
    VACM_VIEWTREE_STRUCT    *selected_entry = NU_NULL;

    /* Look through all the entries in the list. */
    dummy_ptr = Vacm_Mib_Table.vacm_view_tree_family_tab.next;

    while(dummy_ptr != NU_NULL)
    {
        /* If we have found an entry for the view we were looking for.
         * Make sure that the subtree length for this entry is not
         * greater than our OID length. We cannot possibly match with
         * a subtree which has greater length.
         */
        if((strcmp((CHAR *)dummy_ptr->vacm_view_name, view_name) == 0)

#if (INCLUDE_MIB_VACM == NU_TRUE)
            && (dummy_ptr->vacm_status == SNMP_ROW_ACTIVE)
#endif

          )
        {
            /* Compare the view from the passed OID. */
            if(SNMP_Subtree_Compare(in_oid, in_oid_len,
                                 dummy_ptr->vacm_subtree,
                                 dummy_ptr->vacm_subtree_len,
                                 dummy_ptr->vacm_family_mask)
                                 == NU_SUCCESS)
            {
                /* The passed OID matches this subtree. This is our best
                 * candidate. The reason is that our list is sorted
                 * in ascending order of subtree length and then subtree.
                 * Specifications require that we have the subtree with
                 * the largest length and the lexicographically greatest
                 * identifiers. As we advance the list, the latest match
                 * is therefore the best match.
                 */
                selected_entry = dummy_ptr;
            }
        }

        /* Go to the next entry. */
        dummy_ptr = dummy_ptr->next;
    }

    /* If there is a matching entry and that entry includes, matching
     * OIDs, then we return success.
     */
    if((selected_entry != NU_NULL) &&
       (selected_entry->vacm_family_type == VACM_FAMILY_INCLUDED))
    {
        status = NU_SUCCESS;
    }

    return (status);

} /* VACM_FindIn_MibView */

/************************************************************************
*
*   FUNCTION
*
*       VACM_InsertContext
*
*   DESCRIPTION
*
*       This function inserts the node in to the context table. List is
*       sorted in ascending order of SNMP Admin strings.
*
*   INPUTS
*
*       *node           Pointer to a context structure node.
*
*   OUTPUTS
*
*       NU_SUCCESS      Operation successful.
*       SNMP_WARNING    Operation failed: Entry with the same index
*                       already exists.
*       SNMP_NO_MEMORY  Memory allocation failed. 
*
*************************************************************************/
STATUS VACM_InsertContext(VACM_CONTEXT_STRUCT *node)
{
    VACM_CONTEXT_STRUCT         *temp;
    VACM_CONTEXT_STRUCT         *new_node;
    INT32                       cmp;
    STATUS                      status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Get a pointer to the root of the list. */
    temp = Vacm_Mib_Table.vacm_context_table.next;

    /* Find the location where the root is to be inserted. */
    while(temp != NU_NULL)
    {
        /* If the element has a context name which is
           lexicographically greater then we need to insert the new
           node before this. */
        if((cmp = (UTL_Admin_String_Cmp(
                          (CHAR *)(temp->context_name),
                          (CHAR *)(node->context_name)))) >= 0)
        {
            /* If both the strings were equal, then the node is already
               present and we do not need to insert it. */
            if(cmp == 0)
            {
                status = SNMP_WARNING;
            }

            break;
        }

        temp = temp->next;
    }

    /* Insert the node at the appropriate location. */
    if(status == NU_SUCCESS)
    {
        /* Allocate memory for the new node. */
        if ((status = NU_Allocate_Memory(&System_Memory,
                                         (VOID**)&(new_node),
                                         sizeof(VACM_CONTEXT_STRUCT),
                                         NU_NO_SUSPEND)) == NU_SUCCESS)
        {
            new_node = TLS_Normalize_Ptr(new_node);

            /* Copy value to the new node. */
            NU_BLOCK_COPY(new_node, node, sizeof(VACM_CONTEXT_STRUCT));

            /* If temp is NU_NULL that means we need to enqueue at
               the end of the list. */
            if(temp == NU_NULL)
            {
                DLL_Enqueue(&Vacm_Mib_Table.vacm_context_table, new_node);
            }
            else
            {
                /* Otherwise, insert before temp. */
                DLL_Insert(&Vacm_Mib_Table.vacm_context_table, new_node,
                           temp);
            }
        }
        else
        {
            status = SNMP_NO_MEMORY;
        }
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* VACM_InsertContext */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Add_Context
*
*   DESCRIPTION
*
*       Adds a new context name to the list of context's.
*
*   INPUTS
*
*       *context_name           Context name.
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation successful.
*       SNMP_WARNING            Entry already exists.
*       SNMP_NO_MEMORY          Memory allocation failed
*       SNMP_BAD_PARAMETER      Null pointer was passed in.
*       SNMP_ERROR              Context name too big.
*
*************************************************************************/
STATUS VACM_Add_Context(const UINT8 *context_name)
{
    STATUS                  status;
    VACM_CONTEXT_STRUCT     node;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (context_name != NU_NULL)
    {
        /* Make sure that the context name is not larger than the
         * maximum limit.
         */
        if (strlen((CHAR *)context_name) >= SNMP_SIZE_SMALLOBJECTID)
        {
            status = SNMP_ERROR;
        }
        else
        {
            /* Clear the node structure. */
            UTL_Zero((VOID*)&node, sizeof(VACM_CONTEXT_STRUCT));

            /* Copy the context name. */
            strcpy((CHAR *)node.context_name,(CHAR *)context_name);

            /* Calling the function to insert node. */
            status = VACM_InsertContext(&node);

        }
    }

    else
    {
        status = SNMP_BAD_PARAMETER;
    }

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* VACM_Add_Context */

/************************************************************************
*
*   FUNCTION
*
*       VACM_InsertGroup_Util
*
*   DESCRIPTION
*
*       This function inserts a Security 2 Group entry to the list passed
*       in.
*
*   INPUTS
*
*       *node           Pointer to Security 2 Group entry.
*       *root           Pointer to the root of Security 2 Group entry
*                       list.
*
*   OUTPUTS
*
*       NU_SUCCESS         Operation successful.
*       SNMP_WARNING       Operation failed: An entry with the same indices
*                          already exists.
*       SNMP_BAD_PARAMETER Invalid arguments.
*       SNMP_NO_MEMORY     Memory allocation failure.
*
*************************************************************************/
STATUS VACM_InsertGroup_Util(VACM_SEC2GROUP_STRUCT *node,
                             VACM_SEC2GROUP_ROOT *root)
{
    VACM_SEC2GROUP_STRUCT    *temp;
    VACM_SEC2GROUP_STRUCT    *new_node;
    INT32                    cmp;
    STATUS                   status = NU_SUCCESS;

    if ((node) && (root))
    {
        /* Get a pointer to the root of the list. */
        temp = root->next;

        /* Find the location where the root is to be inserted. */
        while(temp)
        {
            /* If the element' security model is greater or is equal,
             * then, this may be where we will be inserting the new node.
             */
            if((cmp = temp->vacm_security_model -
                      node->vacm_security_model) >= 0)
            {
                /* If the security models are equal, then we will insert
                 * the node before this if the security name is greater.
                 */
                if(cmp == 0)
                {
                    if((cmp = UTL_Admin_String_Cmp(
                                 (CHAR *)(temp->vacm_security_name),
                                 (CHAR *)(node->vacm_security_name)))>= 0)
                    {
                        /* If the comparison showed both strings are
                         * equal. Then we will not insert the node because
                         * it already exists.
                         */
                        if(cmp == 0)
                        {
                            status = SNMP_WARNING;
                        }

                        /* Break through the loop. */
                        break;
                    }
                }

                else
                {
                    /* We will insert the node here, because the next
                     * entry has a greater security model.
                     */
                    break;
                }
            }

            /* Moving forward in the list. */
            temp = temp->next;
        }

        /* Insert the node at the appropriate location. */
        if(status == NU_SUCCESS)
        {
            /* Allocate memory for the new node. */
            if ((NU_Allocate_Memory(&System_Memory,
                                            (VOID**)&(new_node),
                                            sizeof(VACM_SEC2GROUP_STRUCT),
                                            NU_NO_SUSPEND)) == NU_SUCCESS)
            {
                new_node = TLS_Normalize_Ptr(new_node);

                /* Copy value to the new node. */
                NU_BLOCK_COPY(new_node, node,
                              sizeof(VACM_SEC2GROUP_STRUCT));

                /* If temp is NU_NULL that means we need to enqueue at the
                 * end of the list.
                 */
                if(temp == NU_NULL)
                {
                    DLL_Enqueue(root, new_node);
                }
                else
                {
                    /* Otherwise, insert before temp. */
                    DLL_Insert(root, new_node, temp);
                }
            }

            else
            {
                NLOG_Error_Log("SNMP: Memory allocation failed",
                                NERR_SEVERE, __FILE__, __LINE__);
                status = SNMP_NO_MEMORY;
            }
        }
    }

    /* If we have invalid arguments then return error code. */
    else
    {
        /* Return error code. */
        status = SNMP_BAD_PARAMETER;
    }

    /* Return success or error code. */
    return (status);

} /* VACM_InsertGroup_Util */

/************************************************************************
*
*   FUNCTION
*
*       VACM_InsertGroup
*
*   DESCRIPTION
*
*       This function inserts a Security 2 Group entry to the list.
*
*   INPUTS
*
*       *node           Pointer to Security 2 Group entry.
*
*   OUTPUTS
*
*       NU_SUCCESS          Operation successful.
*       SNMP_WARNING        Operation failed: An entry with the same indices
*                           already exists.
*       SNMP_BAD_PARAMETER  Invalid argument.
*       SNMP_NO_MEMORY      Memory allocation failed
*
*************************************************************************/
STATUS VACM_InsertGroup(VACM_SEC2GROUP_STRUCT *node)
{
    STATUS                   status;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if ( (node != NU_NULL) &&
#if (INCLUDE_MIB_VACM == NU_TRUE)
         (node->vacm_status >= SNMP_ROW_ACTIVE) &&
         (node->vacm_status <= SNMP_ROW_DESTROY) &&
         ( ( (node->vacm_status != SNMP_ROW_ACTIVE) &&
             (node->vacm_status != SNMP_ROW_NOTINSERVICE) ) ||
           ( (node->vacm_storage_type >= SNMP_STORAGE_OTHER) &&
             (node->vacm_storage_type <= SNMP_STORAGE_READONLY) &&
#else       
         ( (
#endif
           (((node->vacm_security_model >= 1) &&
             (node->vacm_security_model <= 2147483647) &&
             (strlen((CHAR *)node->vacm_security_name) != 0) &&
             (strlen((CHAR *)node->vacm_group_name) != 0))) ) ) )
    {
        status = VACM_InsertGroup_Util(node,
                            &(Vacm_Mib_Table.vacm_security_to_group_tab));
    }
    else
    {
        status = SNMP_BAD_PARAMETER;
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* VACM_InsertGroup */

/************************************************************************
*
*   FUNCTION
*
*       VACM_InsertAccessEntry_Util
*
*   DESCRIPTION
*
*       This function inserts an Access entry to the corresponding list.
*
*   INPUTS
*
*       *node           Pointer to an access entry.
*       *root           Pointer to the root of access entry list.
*
*   OUTPUTS
*
*       NU_SUCCESS      Operation successful.
*       SNMP_WARNING    Operation failed: an entry with the same indices
*                       already exists.
*
*************************************************************************/
STATUS VACM_InsertAccessEntry_Util(VACM_ACCESS_STRUCT *node,
                                   VACM_ACCESS_ROOT *root)
{
    VACM_ACCESS_STRUCT       *temp;
    VACM_ACCESS_STRUCT       *new_node;
    INT32                    cmp;
    STATUS                   status = NU_SUCCESS;

    /* Get a pointer to the root of the list. */
    temp = root->next;

    /* Find the location where the root is to be inserted. */
    while(temp)
    {
        /* If the group name is lexicographically greater or is equal,
           then, this may be where we will be inserting the new node. */
        if((cmp = UTL_Admin_String_Cmp(
                         (CHAR *)(temp->vacm_group_name),
                         (CHAR *)(node->vacm_group_name))) >= 0)
        {
            /* If the strings are equal, then compare the
               context prefix. */
            if(cmp == 0)
            {
                if((cmp = UTL_Admin_String_Cmp(
                                 (CHAR *)(temp->vacm_context_prefix),
                                 (CHAR *)(node->vacm_context_prefix)))
                                                                    >= 0)
                {
                    /* If the comparison showed both strings are equal.
                     * Then we will need to compare the security model.
                     */
                    if(cmp == 0)
                    {
                        if((cmp = (temp->vacm_security_model -
                                   node->vacm_security_model)) >= 0)
                        {
                            /* If the comparison showed both models are
                             * equal. Then we will need to compare the
                             * security level.
                             */
                            if(cmp == 0)
                            {
                                if((cmp = (temp->vacm_security_level -
                                         node->vacm_security_level)) >= 0)
                                {
                                    /* If the comparison showed both
                                     * levels are equal. Then we will not
                                     * be inserting the node, because it
                                     * already exists.
                                     */
                                    if(cmp == 0)
                                    {
                                        status = SNMP_WARNING;
                                    }

                                    /* We will be inserting the node
                                     * here.
                                     */
                                    break;
                                }
                            }
                            else
                            {
                                /* We will be inserting the node here. */
                                break;
                            }

                        }

                    }
                    else
                    {
                        /* We will be inserting the node here. */
                        break;
                    }
                }

            }
            else
            {
                /* We will be inserting the node here. */
                break;
            }
        }

        temp = temp->next;
    }

    /* Insert the node at the appropriate location. */
    if(status == NU_SUCCESS)
    {
        /* Allocate memory for the new node. */
        if ((NU_Allocate_Memory(&System_Memory,
                                         (VOID**)&(new_node),
                                         sizeof(VACM_ACCESS_STRUCT),
                                         NU_NO_SUSPEND)) == NU_SUCCESS)
        {
            new_node = TLS_Normalize_Ptr(new_node);

            /* Copy value to the new node. */
            NU_BLOCK_COPY(new_node, node, sizeof(VACM_ACCESS_STRUCT));

            /* If temp is NU_NULL that means we need to enqueue at
               the end of the list. */
            if(temp == NU_NULL)
            {
                DLL_Enqueue(root, new_node);
            }
            else
            {
                /* Otherwise, insert before temp. */
                DLL_Insert(root, new_node, temp);
            }
        }

        else
        {
            NLOG_Error_Log("SNMP: Failed to allocate memory", NERR_SEVERE,
                            __FILE__, __LINE__);
            status = SNMP_NO_MEMORY;
        }
    }

    /* Return success or error code. */
    return (status);

} /* VACM_InsertAccessEntry_Util */

/************************************************************************
*
*   FUNCTION
*
*       VACM_InsertAccessEntry
*
*   DESCRIPTION
*
*       This function inserts an Access entry to the corresponding list.
*
*   INPUTS
*
*       *node           Pointer to an access entry.
*
*   OUTPUTS
*
*       NU_SUCCESS      Operation successful.
*       SNMP_ERROR      Operation failed: an entry with the same indices
*                       already exists.
*
*************************************************************************/
STATUS VACM_InsertAccessEntry(VACM_ACCESS_STRUCT *node)
{
    VACM_ACCESS_STRUCT       *temp = NU_NULL;
    VACM_ACCESS_STRUCT       *new_node;
    INT32                    cmp;
    STATUS                   status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if ( (node != NU_NULL) &&
#if (INCLUDE_MIB_VACM == NU_TRUE)
         (node->vacm_status >= SNMP_ROW_ACTIVE) &&
         (node->vacm_status <= SNMP_ROW_DESTROY) &&
         ( ( (node->vacm_status != SNMP_ROW_ACTIVE) &&
             (node->vacm_status != SNMP_ROW_NOTINSERVICE) ) ||
           ( (node->vacm_storage_type >= SNMP_STORAGE_OTHER) &&
             (node->vacm_storage_type <= SNMP_STORAGE_READONLY) &&
#else       
         ( (
#endif
           (((node->vacm_context_match >= VACM_CTXTMATCH_EXACT) &&
             (node->vacm_context_match <= VACM_CTXTMATCH_PREFIX))) ) ) )
    {
        /* Get a pointer to the root of the list. */
        temp = Vacm_Mib_Table.vacm_access_tab.next;

        /* Find the location where the root is to be inserted. */
        while(temp != NU_NULL)
        {
            /* If the group name is lexicographically greater or is equal,
             * then, this may be where we will be inserting the new node.
             */
            if((cmp = UTL_Admin_String_Cmp(
                             (CHAR *)(temp->vacm_group_name),
                             (CHAR *)(node->vacm_group_name))) >= 0)
            {
                /* If the strings are equal, then compare the
                   context prefix. */
                if(cmp == 0)
                {
                    if((cmp = UTL_Admin_String_Cmp(
                                 (CHAR *)(temp->vacm_context_prefix),
                                 (CHAR *)(node->vacm_context_prefix)))
                                                                    >= 0)
                    {
                        /* If the comparison showed both strings are
                         * equal. Then we will need to compare the
                         * security model.
                         */
                        if(cmp == 0)
                        {
                            if((cmp = (temp->vacm_security_model -
                                       node->vacm_security_model)) >= 0)
                            {
                                /* If the comparison showed both models
                                 * are equal. Then we will need to compare
                                 * the security level.
                                 */
                                if(cmp == 0)
                                {
                                    if((cmp = (temp->vacm_security_level -
                                         node->vacm_security_level)) >= 0)
                                    {
                                        /* If the comparison showed both
                                         * levels are equal. Then we will
                                         * not be inserting the node,
                                         * because it already exists.
                                         */
                                        if(cmp == 0)
                                        {
                                            status = SNMP_WARNING;
                                        }

                                        /* We will be inserting the node
                                         * here.
                                         */
                                        break;
                                    }
                                }
                                else
                                {
                                    /* We will be inserting the node here.
                                     */
                                    break;
                                }

                            }

                        }
                        else
                        {
                            /* We will be inserting the node here. */
                            break;
                        }
                    }

                }
                else
                {
                    /* We will be inserting the node here. */
                    break;
                }
            }

            temp = temp->next;
        }
    }
    else
    {
        status = SNMP_BAD_PARAMETER;
    }
    
    /* Insert the node at the appropriate location. */
    if(status == NU_SUCCESS)
    {
        /* Allocate memory for the new node. */
        if (NU_Allocate_Memory(&System_Memory,
                                         (VOID**)&(new_node),
                                         sizeof(VACM_ACCESS_STRUCT),
                                         NU_NO_SUSPEND) == NU_SUCCESS)
        {
            new_node = TLS_Normalize_Ptr(new_node);

            /* Copy value to the new node. */
            NU_BLOCK_COPY(new_node, node, sizeof(VACM_ACCESS_STRUCT));

            /* If temp is NU_NULL that means we need to enqueue at
               the end of the list. */
            if(temp == NU_NULL)
            {
                DLL_Enqueue(&Vacm_Mib_Table.vacm_access_tab, new_node);
            }
            else
            {
                /* Otherwise, insert before temp. */
                DLL_Insert(&Vacm_Mib_Table.vacm_access_tab, new_node,
                           temp);
            }
        }
        else
        {
            status = SNMP_NO_MEMORY;
        }
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* VACM_InsertAccessEntry */

/************************************************************************
*
*   FUNCTION
*
*       VACM_InsertMibView_Util
*
*   DESCRIPTION
*
*       This function inserts an MIB view entry to the corresponding list.
*
*   INPUTS
*
*       *node           Pointer to an MIB view entry.
*
*   OUTPUTS
*
*       NU_SUCCESS      Operation successful.
*       SNMP_WARNING    Operation failed: an entry with the same indices
*                       already exists.
*       SNMP_NO_MEMORY  Memory allocation failure.
*
*************************************************************************/
STATUS VACM_InsertMibView_Util(VACM_VIEWTREE_STRUCT *node,
                               VACM_VIEWTREE_ROOT *root)
{
    VACM_VIEWTREE_STRUCT        *temp;
    VACM_VIEWTREE_STRUCT        *new_node;
    INT32                       cmp;
    STATUS                      status = NU_SUCCESS;

    /* Get a pointer to the root of the list. */
    temp = root->next;

    /* Find the location where the root is to be inserted. */
    while(temp != NU_NULL)
    {
        /* If the element has a view name which is
           lexicographically greater or is equal, then, this may
           be where we will be inserting the new node. */
        if((cmp = (UTL_Admin_String_Cmp(
                          (CHAR *)(temp->vacm_view_name),
                          (CHAR *)(node->vacm_view_name)))) >= 0)
        {
            /* If both the strings were equal, then we will
               insert the node before this if the subtree is greater. */
            if(cmp == 0)
            {
                /* First check the length of the OID's as they are the
                 * primary determinant in ordering of the index.
                 */
                cmp = temp->vacm_subtree_len - node->vacm_subtree_len;

                /* If the lengths match, now compare the actual subtrees
                 * to determine, which comes first.
                 */
                if(cmp == 0)
                {
                    cmp = MibCmpObjId(temp->vacm_subtree,
                                    temp->vacm_subtree_len,
                                    node->vacm_subtree,
                                    node->vacm_subtree_len);
                }

                /* If the comparison showed both subtrees are equal. Then
                 * we will not insert the node because it already exists.
                 */
                if(cmp == 0 )
                {
                    status = SNMP_WARNING;
                    break;
                }

                /* Otherwise, if the first subtree is greater,
                   we will insert the node here. */
                else if(cmp > 0)
                {
                    break;
                }
            }
            else
            {
                /* We will insert the node here. */
                break;
            }

        }

        temp = temp->next;
    }

    /* Insert the node at the appropriate location. */
    if(status == NU_SUCCESS)
    {
        /* Allocate memory for the new node. */
        if (NU_Allocate_Memory(&System_Memory,
                                         (VOID**)&(new_node),
                                         sizeof(VACM_VIEWTREE_STRUCT),
                                         NU_NO_SUSPEND) == NU_SUCCESS)
        {
            new_node = TLS_Normalize_Ptr(new_node);

            /* Copy value to the new node. */
            NU_BLOCK_COPY(new_node, node, sizeof(VACM_VIEWTREE_STRUCT));

            /* If temp is NU_NULL that means we need to enqueue at
               the end of the list. */
            if(temp == NU_NULL)
            {
                DLL_Enqueue(root, new_node);
            }
            else
            {
                /* Otherwise, insert before temp. */
                DLL_Insert(root, new_node, temp);
            }
        }

        /* If memory allocation failed. */
        else
        {
            NLOG_Error_Log("SNMP: Failed to allocate memory",
                            NERR_SEVERE, __FILE__, __LINE__);
            status = SNMP_NO_MEMORY;
        }
    }

    /* Return success or error code. */
    return (status);

} /* VACM_InsertMibView_Util */

/************************************************************************
*
*   FUNCTION
*
*       VACM_InsertMibView
*
*   DESCRIPTION
*
*       This function inserts an MIB view entry to the corresponding list.
*
*   INPUTS
*
*       *node           Pointer to an MIB view entry.
*
*   OUTPUTS
*
*       NU_SUCCESS         Operation successful.
*       SNMP_WARNING       Operation failed: an entry with the same indices
*                          already exists.
*       SNMP_NO_MEMORY     Memory allocation failure.
*       SNMP_BAD_PARAMETER Invalid arguments
*
*************************************************************************/
STATUS VACM_InsertMibView(VACM_VIEWTREE_STRUCT *node)
{
    STATUS                      status;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* If we have valid arguments. */
    if ( (node != NU_NULL) &&
#if (INCLUDE_MIB_VACM == NU_TRUE)
         (node->vacm_status >= SNMP_ROW_ACTIVE) &&
         (node->vacm_status <= SNMP_ROW_DESTROY) &&
         ( ( (node->vacm_status != SNMP_ROW_ACTIVE) &&
             (node->vacm_status != SNMP_ROW_NOTINSERVICE) ) ||
           ( (node->vacm_storage_type >= SNMP_STORAGE_OTHER) &&
             (node->vacm_storage_type <= SNMP_STORAGE_READONLY) &&
#else       
         ( (
#endif
           (((strlen((CHAR *)node->vacm_view_name) != 0) &&
             (node->vacm_subtree_len <= VACM_SUBTREE_LEN) &&
             (node->vacm_mask_length <= VACM_MASK_SIZE) &&
             (node->vacm_family_type >= VACM_FAMILY_INCLUDED) &&
             (node->vacm_family_type <= VACM_FAMILY_EXCLUDED))) ) ) )
    {
        VACM_Filling_FamilyMask(node->vacm_family_mask,
                                node->vacm_mask_length);

        status = VACM_InsertMibView_Util(node,
                            &(Vacm_Mib_Table.vacm_view_tree_family_tab));
    }

    /* If we don't have valid argument then return error code. */
    else
    {
        /* Returning error code. */
        status = SNMP_BAD_PARAMETER;
    }

    /* return to user mode */
    NU_USER_MODE();

    /* Return success or error code. */
    return (status);

} /* VACM_InsertMibView */

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
/************************************************************************
*
*   FUNCTION
*
*       VACM_Compare_Group
*
*   DESCRIPTION
*
*       This function is used to compare the indices of the Security
*       2 Group entries. This function is being used by the file
*       component.
*
*   INPUTS
*
*       *left_side                      Pointer to a Security 2 Group
*                                       entry.
*       *right_side                     Pointer to a Security 2 Group
*                                       entry.
*
*   OUTPUTS
*
*       0                               Both the entries are equal.
*       -1                              The entries are not equal.
*
*************************************************************************/
INT32 VACM_Compare_Group(VOID *left_side, VOID *right_side)
{
    VACM_SEC2GROUP_STRUCT   *left_ptr = left_side;
    VACM_SEC2GROUP_STRUCT   *right_ptr = right_side;
    INT32                   cmp = -1;

    /* Compare the security model and security name. This forms the index
     * for Security 2 Group table.
     */
    if((left_ptr->vacm_security_model ==
                        right_ptr->vacm_security_model) &&
       (strcmp((CHAR *)left_ptr->vacm_security_name,
               (CHAR *)right_ptr->vacm_security_name)== 0))
    {
        cmp = 0;
    }

    return (cmp);

} /* VACM_Compare_Group */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Save_Group
*
*   DESCRIPTION
*
*       This function saves/updates the Security 2 Group data to file.
*
*   INPUTS
*
*       *vacm_security_to_group         Pointer to the data which needs
*                                       to be updated.
*
*   OUTPUTS
*
*       NU_SUCCESS                      Operation successful.
*
*************************************************************************/
STATUS VACM_Save_Group(VACM_SEC2GROUP_STRUCT *vacm_security_to_group)
{
    /* This variable will be used by the file component to temporarily
     * store information read from file.
     */
    VACM_SEC2GROUP_STRUCT   read_sec2group;
    STATUS                  status;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Call the function that performs the actual logic. */
    status = SNMP_Save(
        Vacm_Mib_Table.vacm_security_to_group_tab.next,
        vacm_security_to_group,
        &read_sec2group,
        SNMP_MEMBER_OFFSET(VACM_SEC2GROUP_STRUCT, vacm_storage_type),
        SNMP_MEMBER_OFFSET(VACM_SEC2GROUP_STRUCT, vacm_status),
        sizeof(VACM_SEC2GROUP_STRUCT),
        VACM_SEC2GROUP_STRUCT_FILE,
        VACM_Compare_Group,
        INCLUDE_MIB_VACM);

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* VACM_Save_Group */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Compare_Access
*
*   DESCRIPTION
*
*       This function is used to compare the indices of the Access table
*       entries. This function is being used by the file component.
*
*   INPUTS
*
*       *left_side                      Pointer to an Access table entry.
*       *right_side                     Pointer to an Access table entry.
*
*   OUTPUTS
*
*       0                               Both the entries are equal.
*       -1                              The entries are not equal.
*
*************************************************************************/
INT32 VACM_Compare_Access(VOID *left_side, VOID *right_side)
{
    VACM_ACCESS_STRUCT      *left_ptr = left_side;
    VACM_ACCESS_STRUCT      *right_ptr = right_side;
    INT32                   cmp = -1;

    /* Compare the group name, context prefix, security model and security
     * level. This forms the index for the Access table.
     */
    if((!strcmp((CHAR*)left_ptr->vacm_group_name,
                (CHAR*)right_ptr->vacm_group_name))&&
       (!strcmp((CHAR*)left_ptr->vacm_context_prefix,
                (CHAR*)right_ptr->vacm_context_prefix)) &&
       (left_ptr->vacm_security_model ==
                            right_ptr->vacm_security_model) &&
       (left_ptr->vacm_security_level == right_ptr->vacm_security_level))
    {
        cmp = 0;
    }

    return (cmp);

} /* VACM_Compare_Access */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Save_Access
*
*   DESCRIPTION
*
*       This function saves/updates the Access table's data to file.
*
*   INPUTS
*
*       *vacm_access                    Pointer to the entry which needs
*                                       to be updated.
*
*   OUTPUTS
*
*       NU_SUCCESS                      Operation successful.
*
*************************************************************************/
STATUS VACM_Save_Access(VACM_ACCESS_STRUCT *vacm_access)
{
    /* This variable will be used by the file component to temporarily
     * store information read from file.
     */
    VACM_ACCESS_STRUCT      access;
    STATUS                  status;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Call the function that performs the actual logic. */
    status = SNMP_Save(
        Vacm_Mib_Table.vacm_access_tab.next,
        vacm_access,
        &access,
        SNMP_MEMBER_OFFSET(VACM_ACCESS_STRUCT, vacm_storage_type),
        SNMP_MEMBER_OFFSET(VACM_ACCESS_STRUCT, vacm_status),
        sizeof(VACM_ACCESS_STRUCT),
        VACM_ACCESS_STRUCT_FILE,
        VACM_Compare_Access,
        INCLUDE_MIB_VACM);

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* VACM_Save_Access */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Compare_View
*
*   DESCRIPTION
*
*       This function is used to compare the indices of the MIB View table
*       entries. This function is being used by the file component.
*
*   INPUTS
*
*       *left_side                      Pointer to a view table entry.
*       *right_side                     Pointer to a view table entry.
*
*   OUTPUTS
*
*       0                               Both the entries are equal.
*       -1                              The entries are not equal.
*
*************************************************************************/
INT32 VACM_Compare_View(VOID *left_side, VOID *right_side)
{
    VACM_VIEWTREE_STRUCT    *left_ptr = left_side;
    VACM_VIEWTREE_STRUCT    *right_ptr = right_side;
    INT32                   cmp = -1;

    /* Compare the subtree length, view name and subtree. This forms the
     * index for the MIB view table.
     */
    if((left_ptr->vacm_subtree_len ==
        right_ptr->vacm_subtree_len) &&
       (!strcmp((CHAR*)left_ptr->vacm_view_name,
                (CHAR*)right_ptr->vacm_view_name)) &&
       (!memcmp(left_ptr->vacm_subtree, right_ptr->vacm_subtree,
                left_ptr->vacm_subtree_len * sizeof(UINT32))))
    {
        cmp = 0;
    }

    return (cmp);

} /* VACM_Compare_View */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Save_View
*
*   DESCRIPTION
*
*       This function saves/update the MIB view table's data to file.
*
*   INPUTS
*
*       *vacm_view_tree_family          Pointer to the entry which needs
*                                       to be updated.
*
*   OUTPUTS
*
*       NU_SUCCESS                      Operation successful.
*
*************************************************************************/
STATUS VACM_Save_View(VACM_VIEWTREE_STRUCT *vacm_view_tree_family)
{
    /* This variable will be used by the file component to temporarily
     * store information read from file.
     */
    VACM_VIEWTREE_STRUCT    viewtree;
    STATUS                  status;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Call the function that performs the actual logic. */
    status = SNMP_Save(
        Vacm_Mib_Table.vacm_view_tree_family_tab.next,
        vacm_view_tree_family,
        &viewtree,
        SNMP_MEMBER_OFFSET(VACM_VIEWTREE_STRUCT, vacm_storage_type),
        SNMP_MEMBER_OFFSET(VACM_VIEWTREE_STRUCT, vacm_status),
        sizeof(VACM_VIEWTREE_STRUCT),
        VACM_VIEWTREE_STRUCT_FILE,
        VACM_Compare_View,
        INCLUDE_MIB_VACM);

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* VACM_Save_View */

#endif /* (SNMP_ENABLE_FILE_STORAGE == NU_TRUE) */
