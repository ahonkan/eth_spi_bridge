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
*       usm_api.c                                                
*
*   DESCRIPTION
*
*       This file contains API functions specific to the User-Based
*       Security Model. These function are also required by USM MIB.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       USM_Remove_User
*       Get_USM_User_Entry_Util
*       Get_USM_User_Entry
*
*   DEPENDENCIES
*
*       target.h
*       snmp.h
*       snmp_dis.h
*       usm.h
*
*************************************************************************/

#include "networking/target.h"
#include "networking/snmp.h"
#include "networking/snmp_dis.h"
#include "networking/usm.h"

extern USM_MIB_STRUCT                  Usm_Mib;

#if (INCLUDE_MIB_USM == NU_TRUE)
extern USM_USERS_TABLE_ROOT            Temp_USM_Mib_Root;
#endif
/************************************************************************
*
*   FUNCTION
*
*       USM_Remove_User
*
*   DESCRIPTION
*
*       This function removes a new user from the usm_user_table.
*
*   INPUTS
*
*       *user       Pointer to the user to be removed.
*
*   OUTPUTS
*
*       NU_SUCCESS          Successfully removed user. 
*       SNMP_ERROR          No such user found.
*       SNMP_BAD_PARAMETER  Null pointer passed in.
*
*************************************************************************/
STATUS USM_Remove_User(USM_USERS_STRUCT *user)
{
    /* Status to return success or error code. */
    STATUS              status;
    USM_USERS_STRUCT    *dummy;
    
    NU_SUPERV_USER_VARIABLES

    /* switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

        /* If we had valid pointer. */
    if (user != NU_NULL)
    {
        /* Start traversing from start of the list. */
        dummy = Usm_Mib.usm_user_table.next;

        /* Traverse the list to find the entry. */
        while (dummy != NU_NULL)
        {
            /* If we found the entry then break through the loop. */
            if (dummy == user)
            {
                /* Break through the loop. */
                break;
            }

            /* Moving forward in the list. */
            dummy = dummy->next;
        }

        /* node found in the list. */
        if (dummy != NU_NULL)
        {
            /* Remove the node from the table. */
            DLL_Remove(&Usm_Mib.usm_user_table, user);

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

            /* Set the communities row Status to destroy so that the
             * community is not added to the table if this call is made
             * by the user.
             */
            user->usm_status = SNMP_ROW_DESTROY;

            /* Save the user table. */
            USM_Save_User(user);
#endif

            /* Deallocate the memory occupied by the node. */
            if (NU_Deallocate_Memory(user) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to deallocate USM user "
                            "structure", NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Return success code. */
            status = NU_SUCCESS;
        }
        
        else
        {
            status = SNMP_ERROR;
        }
    }

    else
    {
        status = SNMP_BAD_PARAMETER;
    }
    
    /* return to user mode */
    NU_USER_MODE();

    /* Return success or error code. */
    return (status);

} /* USM_Remove_User */

#if (INCLUDE_MIB_USM == NU_TRUE)
/*************************************************************************
*
*   FUNCTION
*
*       Get_USM_User_Entry_Util
*
*   DESCRIPTION
*
*       This function is used to get the handle to the USM user structure
*       by specifying the Engine ID and user name.
*
*   INPUTS
*
*       *engine_id              A pointer to the location where engine ID
*                               is stored.
*       *engine_id_len          The length of engine ID.
*       *user_name              Pointer to the location where user name is
*                               stored.
*       user_name_len           Length of user name.
*       *root                   The pointer to the root of list containing
*                               USM user structures.
*
*   OUTPUTS
*
*       NU_NULL                 When fail to get handle to USM user
*                               structure indexes passed in.
*       USM_USERS_STRUCT *      When handle to USM user structure found
*                               with engine id and user name passed in.
*
*************************************************************************/
USM_USERS_STRUCT *Get_USM_User_Entry_Util(const UINT8 *engine_id,
                                          UINT32 engine_id_len,
                                          const UINT8 *user_name,
                                          UINT32 user_name_len,
                                         const USM_USERS_TABLE_ROOT *root)
{
    /* Handle to the USM user structure. */
    USM_USERS_STRUCT *usm_user_ptr = root->next;

    /* Loop to find the USM user structure with engine id and user name
     * passed in.
     */
    while (usm_user_ptr)
    {
        /* If we reached at required USM user structure the break through
         * the loop.
         */
        if ( (usm_user_ptr->usm_user_engine_id_len ==
                                                (UINT8)engine_id_len) &&
             (memcmp(usm_user_ptr->usm_user_engine_id, engine_id,
                     engine_id_len) == 0) &&
             (user_name_len ==
               (UINT32)(strlen((CHAR *)(usm_user_ptr->usm_user_name)))) &&
             (strcmp((CHAR *)usm_user_ptr->usm_user_name,
                     (CHAR *)user_name) == 0) )
        {
            break;
        }

        /* Moving forward in the list. */
        usm_user_ptr = usm_user_ptr->next;
    }

    /* Return pointer to USM user structure if found. Otherwise return
     * NU_NULL.
     */
    return (usm_user_ptr);

} /* Get_USM_User_Entry_Util */

/*************************************************************************
*
*   FUNCTION
*
*       Get_USM_User_Entry
*
*   DESCRIPTION
*
*       This function is used to get the handle to the USM user structure
*       by specifying the engine ID and user name indexes and list of
*       USM user structures.
*
*   INPUTS
*
*       *engine_id              A pointer to the location where engine ID
*                               is stored.
*       engine_id_len           The length of engine ID.
*       *user_name              Pointer to the location where user name is
*                               stored.
*       getflag                 The flag to distinguish between GET and
*                               GET-NEXT request.
*
*   OUTPUTS
*
*       NU_NULL                 When fail to get handle to USM user
*                               structure indexes passed in.
*       USM_USERS_STRUCT *      When handle to USM user structure found
*                               with engine id and user name passed in.
*
*************************************************************************/
USM_USERS_STRUCT *Get_USM_User_Entry(UINT8 *engine_id,
                                   UINT32 engine_id_len, UINT8 *user_name,
                                   UINT32 user_name_len, UINT8 getlfag)
{
    /* Handle to the USM user structure. */
    USM_USERS_STRUCT    *usm_user_ptr;

    /* Variable to hold comparison result. */
    INT                 cmp_result;

    /* If we are handling GET request. */
    if (getlfag)
    {
        /* Try to get the USM user structure handle from permanent list.
         */
        usm_user_ptr = Get_USM_User_Entry_Util(engine_id, engine_id_len,
                                      user_name, user_name_len,
                                      &Usm_Mib.usm_user_table);

        /* If USM user structure handle was not found in permanent list.
         */
        if (!usm_user_ptr)
        {
            /* Try to get the USM user structure handle from temporary
             * list.
             */
            usm_user_ptr = Get_USM_User_Entry_Util(engine_id,
                            engine_id_len, user_name, user_name_len,
                            &Temp_USM_Mib_Root);
        }
    }

    /* If we are handling GET-NEXT request. */
    else
    {
        /* Start traversing the permanent list. */
        usm_user_ptr = Usm_Mib.usm_user_table.next;

        /* Loop to find the next USM user structure based on the indexes
         * passed in.
         */
        while (usm_user_ptr)
        {
            /* Compare engine ID length. */
            cmp_result = (INT)(usm_user_ptr->usm_user_engine_id_len) -
                         (INT)(engine_id_len);

            /* If engine ID length comparison does not help in making
             * decision.
             */
            if (cmp_result == 0)
            {
                /* Compare the value of engine ID. */
                cmp_result = memcmp(usm_user_ptr->usm_user_engine_id,
                                   engine_id, engine_id_len);

                /* If engine ID comparison does not result in decision.
                 */
                if (cmp_result == 0)
                {
                    /* Compare user name length. */
                    cmp_result = (INT)(strlen((CHAR *)usm_user_ptr->
                                usm_user_name)) - (INT)(user_name_len);

                    /* If user name length comparison does not result in
                     * decision.
                     */
                    if (cmp_result == 0)
                    {
                        /* Compare user name. */
                        cmp_result = (INT)(strcmp((CHAR *)usm_user_ptr->
                                       usm_user_name, (CHAR *)user_name));
                    }
                }
            }

            /* If we have reached at a USM user structure with greater
             * indexes as passed in then break through the loop.
             */
            if (cmp_result > 0)
            {
                /* Break through the loop. */
                break;
            }

            /* Moving forward in the list. */
            usm_user_ptr = usm_user_ptr->next;
        }
    }

    /* Return the handle to the USM user structure, if found. Otherwise
     * return NU_NULL.
     */
    return (usm_user_ptr);

} /* Get_USM_User_Entry */

#endif /* (INCLUDE_MIB_USM == NU_TRUE) */
