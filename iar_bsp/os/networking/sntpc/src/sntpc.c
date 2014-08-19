/*************************************************************************
*
*            Copyright Mentor Graphics Corporation 2012
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
*       sntpc.c
*
*   DESCRIPTION
*
*       Nucleus SNTP Client core API functions.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SNTPC_Add_Server
*       SNTPC_Delete_Server
*       SNTPC_Purge_Server_List
*       SNTPC_Server_Query
*       SNTPC_Set_Timezone
*       SNTPC_Get_Timezone
*       SNTPC_Get_Time
*       SNTPC_Get_Time_From_Server
*
*   DEPENDENCIES
*
*       sntpc.h
*       sntpc_int.h
*
*************************************************************************/

#include "networking/sntpc.h"
#include "os/networking/sntpc/inc/sntpc_int.h"

/*************************************************************************
*
*   FUNCTION
*
*       SNTPC_Add_Server
*
*   DESCRIPTION
*
*       Add a new NTP server to the list of possible hosts. Servers
*       must be unique by address, port, and family.
*
*   INPUTS
*
*       *server                 A pointer to the new server.
*
*   OUTPUTS
*
*       NU_SUCCESS              Upon successful completion.
*       SNTPC_ALREADY_EXISTS    If server already exists in the database.
*       OS Error                Operating-system specific error.
*
*************************************************************************/
STATUS SNTPC_Add_Server(SNTPC_SERVER *server)
{
    STATUS status;
    SNTPC_SERVER_LIST *new_server;
    SNTPC_SERVER_LIST *ret_server;
    INT hostname_length = 0;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

#if INCLUDE_NET_API_ERR_CHECK
    /* Validate parameters */
    if (server == NU_NULL)
    {
        status = NU_INVALID_PARM;
    }
    else if (
#ifdef CFG_NU_OS_NET_IPV6_ENABLE
            (server->sntpc_server_addr.family != NU_FAMILY_IP6)
#if CFG_NU_OS_NET_STACK_INCLUDE_IPV4
            &&
#endif
#endif
#if CFG_NU_OS_NET_STACK_INCLUDE_IPV4
            (server->sntpc_server_addr.family != NU_FAMILY_IP)
#endif
        )
    {
        status = NU_INVALID_PARM;
    }
    else
#endif
    {
        status = NU_Obtain_Semaphore(&SNTPC_Config->sntpc_semaphore,
                                     NU_SUSPEND);
    }

    if (status == NU_SUCCESS)
    {
        if ((server->sntpc_server_hostname != NU_NULL) &&
            (server->sntpc_server_hostname[0] != '\0'))
        {
            hostname_length = strlen(server->sntpc_server_hostname) + 1;
        }

        /* Is list full */
        if (SNTPC_Config->sntpc_server_list.sntpc_count <
            SNTPC_Config->sntpc_server_list.sntpc_max_servers)
        {
            /* Server addresses must be unique */
            status = SNTPC_Server_List_Query(&server->sntpc_server_addr,
                                             server->sntpc_server_hostname,
                                             &ret_server);
            if (status == NU_SUCCESS)
            {
                /* Entry already exists */
                status = SNTPC_ALREADY_EXISTS;
            }
            else
            {
                /* Allocate new server structure */
                status = NU_Allocate_Memory(SNTPC_Config->sntpc_memory,
                                            (VOID **)(&new_server),
                                            sizeof(SNTPC_SERVER_LIST) +
                                            hostname_length,
                                            NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    memset(new_server, 0, sizeof(SNTPC_SERVER_LIST));

                    /* Set server elements */
                    memcpy(&(new_server->sntpc_server), server,
                            sizeof(SNTPC_SERVER));

                    new_server->sntpc_server.sntpc_server_hostname = NU_NULL;

                    /* Copy the hostname if specified. */
                    if (hostname_length != 0)
                    {
                        new_server->sntpc_server.sntpc_server_hostname =
                            (CHAR *)(new_server + 1);
                        strcpy(new_server->sntpc_server.sntpc_server_hostname,
                                server->sntpc_server_hostname);

                        /* Mark hostname is unresolved. */
                        new_server->sntpc_hostname_resolved = NU_FALSE;
                    }
                    else
                    {
                        /* Mark hostname as resolved since just an IP
                         * has been specified. */
                        new_server->sntpc_hostname_resolved = NU_TRUE;
                    }

                    /* Set initial time to an unsynchronized value. */
                    new_server->sntpc_last_server_time.sntpc_seconds =
                                               SNTPC_01_01_2012;
                    new_server->sntpc_last_plus_ticks = NU_Retrieve_Clock();
                    new_server->sntpc_synced = NU_FALSE;

                    /* Insert into list */
                    DLL_Enqueue(&(SNTPC_Config->sntpc_server_list), new_server);

                    SNTPC_Config->sntpc_server_list.sntpc_count++;

                    /* If this is the first item in the list. */
                    if (SNTPC_Config->sntpc_server_list.sntpc_count == 1)
                    {
                        status = SNTPC_Init_On_First_Server();
                    }

                    /* Set-up timer for polling */
                    status = NU_Create_Timer(&new_server->sntpc_timer,
                                    "SNTPC_TMR", SNTPC_Send_Request_Handler,
                                    (UNSIGNED)new_server, 1,
                                    (NU_TICKS_PER_SECOND *
                                     new_server->sntpc_server.sntpc_poll_interval),
                                    NU_ENABLE_TIMER);
                }
            }
        }
        else
        {
            status = SNTPC_LIST_FULL;
        }

        NU_Release_Semaphore(&SNTPC_Config->sntpc_semaphore);
    }

    NU_USER_MODE();          /* switch to user mode */

    return(status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SNTPC_Delete_Server
*
*   DESCRIPTION
*
*       Remove an NTP server from the list of possible hosts.
*
*   INPUTS
*
*       *server                 A pointer to the server.
*
*   OUTPUTS
*
*       NU_SUCCESS upon successful completion or an operating-system specific
*       error if no data was sent.
*
*************************************************************************/
STATUS SNTPC_Delete_Server(SNTPC_SERVER *server)
{
    STATUS status = NU_SUCCESS, tmp_stat;
    SNTPC_SERVER_LIST *ret_server;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

#if INCLUDE_NET_API_ERR_CHECK
    /* Validate parameters */
    if (server == NU_NULL)
    {
        status = NU_INVALID_PARM;
    }
    else
#endif
    {
        status = NU_Obtain_Semaphore(&SNTPC_Config->sntpc_semaphore, NU_SUSPEND);
    }

    if (status == NU_SUCCESS)
    {
        /* Find server in list */
        status = SNTPC_Server_List_Query(&server->sntpc_server_addr,
                                         server->sntpc_server_hostname,
                                         &ret_server);
        if (status == NU_SUCCESS)
        {
            /* Remove the item from the list. */
            DLL_Remove(&(SNTPC_Config->sntpc_server_list), ret_server);

            /* Delete timer */
            status = NU_Control_Timer (&(ret_server->sntpc_timer),
                                        NU_DISABLE_TIMER);
            if (status == NU_SUCCESS)
            {
                status = NU_Delete_Timer (&(ret_server->sntpc_timer));
            }

            /* Deallocate the server node. */
            tmp_stat = NU_Deallocate_Memory(ret_server);
            if(status == NU_SUCCESS)
            {
                status = tmp_stat;
            }

            SNTPC_Config->sntpc_server_list.sntpc_count--;

            /* If this was the last item in the list. */
            if (SNTPC_Config->sntpc_server_list.sntpc_count == 0)
            {
                /* Free some resources since the database is now empty. */
                tmp_stat = SNTPC_Deinit_On_Last_Server();
                if(status == NU_SUCCESS)
                {
                    status = tmp_stat;
                }
            }
            else
            {
                /* If this was the OWP best, then re-run OWP */
                if (SNTPC_Config->sntpc_server_list.sntpc_owp_best == ret_server)
                {
                    /* Clear the best server */
                    SNTPC_Config->sntpc_server_list.sntpc_owp_best = NU_NULL;

                    /* Attempt to establish a new best */
                    SNTPC_Send_Request_Handler(
                        (UNSIGNED)SNTPC_Config->sntpc_server_list.sntpc_head);
                }
            }
        }
        else
        {
            /* No matching server found in the database. */
            status = NU_NO_DATA;
        }

        NU_Release_Semaphore(&SNTPC_Config->sntpc_semaphore);
    }

    NU_USER_MODE();          /* switch to user mode */

    return(status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SNTPC_Purge_Server_List
*
*   DESCRIPTION
*
*       Remove all NTP servers from the list of possible hosts.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID SNTPC_Purge_Server_List(VOID)
{
    SNTPC_SERVER_LIST *temp, *next;
    STATUS status;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    status = NU_Obtain_Semaphore(&SNTPC_Config->sntpc_semaphore, NU_SUSPEND);
    if (status == NU_SUCCESS)
    {
        /* Get list head */
        temp = SNTPC_Config->sntpc_server_list.sntpc_head;

        /* Delete each entry */
        while(temp)
        {
            /* Remove the item from the list. */
            next = DLL_Remove(&(SNTPC_Config->sntpc_server_list), temp);

            /* Delete timer */
            if (NU_SUCCESS == NU_Control_Timer (&(temp->sntpc_timer),
                                                NU_DISABLE_TIMER))
            {
                NU_Delete_Timer (&(temp->sntpc_timer));
            }

            /* Deallocate the server node. */
            NU_Deallocate_Memory(temp);
            temp = next;
            SNTPC_Config->sntpc_server_list.sntpc_count--;
        }

        /* This case should never occur but checking it just to be safe. */
        if (SNTPC_Config->sntpc_server_list.sntpc_count != 0)
            SNTPC_Config->sntpc_server_list.sntpc_count = 0;

        /* Free some resources since the database is now empty. */
        status = SNTPC_Deinit_On_Last_Server();

        NU_Release_Semaphore(&SNTPC_Config->sntpc_semaphore);
    }

    NU_USER_MODE();          /* switch to user mode */
}

/*************************************************************************
*
*   FUNCTION
*
*       SNTPC_Server_Query
*
*   DESCRIPTION
*
*       Retrieve an NTP server from the list of possible hosts.
*
*   INPUTS
*
*       *addr                   A pointer to the server address.
*       *server_hostname        Hostname of the server. This is optional
*                               and can be set to NU_NULL.
*       *ret_server             A pointer to a server structure that will
*                               contain the server details upon success.
*
*   OUTPUTS
*
*       NU_SUCCESS upon successful completion or an operating-system
*       specific error if no data was sent.
*
*       *ret_server             Upon success, server information will be
*                               returned
*
*************************************************************************/
STATUS SNTPC_Server_Query(struct addr_struct *addr,
                          CHAR *server_hostname,
                          SNTPC_SERVER *ret_server)
{
    STATUS status;
    SNTPC_SERVER_LIST *server;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

#if INCLUDE_NET_API_ERR_CHECK
    if ((addr == NU_NULL) || (server_hostname == NU_NULL) ||
        (ret_server == NU_NULL))
    {
        status = NU_INVALID_PARM;
    }
    else
#endif
    {
        status = SNTPC_Server_List_Query(addr, server_hostname, &server);
        if (status == NU_SUCCESS)
        {
            memcpy(ret_server, &server->sntpc_server, sizeof(SNTPC_SERVER));
            status = NU_SUCCESS;
        }
    }

    NU_USER_MODE();          /* switch to user mode */

    return(status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SNTPC_Set_Timezone
*
*   DESCRIPTION
*
*       Sets the timezone and DST.
*
*   INPUTS
*
*       timezone                Minutes offset from UTC
*       dst_enabled             NU_TRUE - adjust for DST
*
*   OUTPUTS
*
*       NU_SUCCESS upon successful completion or an operating-system specific
*       error if no data was sent.
*
*************************************************************************/
STATUS SNTPC_Set_Timezone(INT16 timezone, UINT8 dst_enabled)
{
    STATUS status = NU_SUCCESS;

#if INCLUDE_NET_API_ERR_CHECK
    /* Validate parameters */
    if ((SNTPC_Config == NU_NULL) || (timezone % 10 != 0)||
        (timezone < SNTPC_TIMEZONE_Y)|| (timezone > SNTPC_TIMEZONE_M))
    {
        status = NU_INVALID_PARM;
    }
    else
#endif
    {
        SNTPC_Config->sntpc_timezone = timezone;
        SNTPC_Config->sntpc_dst_enabled = dst_enabled;
    }

    return(status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SNTPC_Get_Timezone
*
*   DESCRIPTION
*
*       Returns the timezone and DST settings.
*
*   INPUTS
*
*       timezone                Minutes offset from UTC
*       dst_enabled             NU_TRUE - adjust for DST
*
*   OUTPUTS
*
*       NU_SUCCESS upon successful completion or an operating-system specific
*       error if no data was sent.
*
*************************************************************************/
STATUS SNTPC_Get_Timezone(INT16 *timezone, UINT8 *dst_enabled)
{
    STATUS status = NU_SUCCESS;

#if INCLUDE_NET_API_ERR_CHECK
    /* Validate parameters */
    if ((SNTPC_Config == NU_NULL) || (timezone == NU_NULL) ||
        (dst_enabled == NU_NULL))
    {
        status = NU_INVALID_PARM;
    }
    else
#endif
    {
        *timezone = SNTPC_Config->sntpc_timezone;
        *dst_enabled = SNTPC_Config->sntpc_dst_enabled;
    }

    return(status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SNTPC_Get_Time
*
*   DESCRIPTION
*
*       Returns the current time as determined from the most precise
*       server. Adjusted for timezone and DST.
*
*   INPUTS
*
*       *current_time           Contains the current time on return.
*
*   OUTPUTS
*
*       NU_SUCCESS              Upon successful completion.
*       SNTPC_OWP_NOT_RUN       On-wire protocol not yet run.
*       SNTPC_NOT_SYNCED        If time with the remote server has not
*                               been synced yet. Unsynchronized time
*                               is returned nonetheless.
*
*************************************************************************/
STATUS SNTPC_Get_Time(SNTPC_TIME *current_time)
{
    STATUS              status = NU_SUCCESS;
    SNTPC_SERVER_LIST   *server;
    UNSIGNED            ticks;

#if INCLUDE_NET_API_ERR_CHECK
    if ((current_time == NU_NULL) || (SNTPC_Config == NU_NULL))
    {
        status = NU_INVALID_PARM;
    }
    else
#endif
    if (SNTPC_Config->sntpc_server_list.sntpc_owp_best == NU_NULL)
    {
        /* The on-wire protocol hasn't been run yet so return an error. */
        status = SNTPC_OWP_NOT_RUN;
    }
    else
    {
        server = SNTPC_Config->sntpc_server_list.sntpc_owp_best;

        /* Store the server's timestamp into the current time structure.
         * Also convert the timestamp's fractional part into microseconds.
         */
        current_time->sntpc_seconds = server->sntpc_last_server_time.sntpc_seconds
                                      - SNTPC_SECONDS_1900_1970;
        current_time->sntpc_useconds = server->sntpc_last_server_time.sntpc_fraction
                                      / 4295;

        /* Now calculate the time since the last server update and add that
         * to the current time. */
        ticks = NU_Retrieve_Clock() - server->sntpc_last_plus_ticks;
        current_time->sntpc_seconds += ticks / NU_PLUS_Ticks_Per_Second;
        ticks = ticks % NU_PLUS_Ticks_Per_Second;

        /* Convert ticks to microseconds and add to the current time. */
        current_time->sntpc_useconds = ticks * (1000000 / NU_PLUS_Ticks_Per_Second);

        /* Adjust time after calculations. */
        while (current_time->sntpc_useconds > 1000000)
        {
            current_time->sntpc_seconds += 1;
            current_time->sntpc_useconds -= 1000000;
        }

        /* Adjust for timezone. */
        current_time->sntpc_seconds += (SNTPC_SECS_PER_MIN *
                                        SNTPC_Config->sntpc_timezone);

        /* Adjust for DST. */
        if (SNTPC_Config->sntpc_dst_enabled)
        {
            current_time->sntpc_is_dst = NU_TRUE;
            current_time->sntpc_seconds += (SNTPC_DAYLIGHT_SAVINGS_ADJUST);
        }
        else
        {
            current_time->sntpc_is_dst = NU_FALSE;
        }

        /* If this time source is not synchronized then return the
         * time but also return a status indicating this condition. */
        if (server->sntpc_synced == NU_FALSE)
        {
            status = SNTPC_NOT_SYNCED;
        }
    }

    return(status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SNTPC_Get_Time_From_Server
*
*   DESCRIPTION
*
*       Returns the current time as determined by the specified
*       server. Adjusted for timezone and DST.
*
*   INPUTS
*
*       *current_time           Contains the current time on return.
*       *server                 Server with respect to which the time
*                               should be returned.
*
*   OUTPUTS
*
*       NU_SUCCESS              Upon successful completion.
*       SNTPC_NOT_SYNCED        If time with the remote server has not
*                               been synced for the given server.
*
*************************************************************************/
STATUS SNTPC_Get_Time_From_Server(SNTPC_TIME *current_time,
                                  SNTPC_SERVER *server)
{
    STATUS              status;
    SNTPC_SERVER_LIST   *srv_info;
    UNSIGNED            ticks;

#if INCLUDE_NET_API_ERR_CHECK
    if ((current_time == NU_NULL) || (SNTPC_Config == NU_NULL))
    {
        status = NU_INVALID_PARM;
    }
    else
#endif
    {
        /* Find the server */
        status = SNTPC_Server_List_Query(&server->sntpc_server_addr,
                                         server->sntpc_server_hostname,
                                         &srv_info);
        if (status == NU_SUCCESS)
        {
            /* Store the server's timestamp into the current time structure.
             * Also convert the timestamp's fractional part into microseconds.
             */
            current_time->sntpc_seconds = srv_info->sntpc_last_server_time.sntpc_seconds
                                          - SNTPC_SECONDS_1900_1970;
            current_time->sntpc_useconds = srv_info->sntpc_last_server_time.sntpc_fraction
                                          / 4295;

            /* Now calculate the time since the last server update and add that
             * to the current time. */
            ticks = NU_Retrieve_Clock() - srv_info->sntpc_last_plus_ticks;
            current_time->sntpc_seconds += ticks / NU_PLUS_Ticks_Per_Second;
            ticks = ticks % NU_PLUS_Ticks_Per_Second;

            /* Convert ticks to microseconds and add to the current time. */
            current_time->sntpc_useconds = ticks * (1000000 / NU_PLUS_Ticks_Per_Second);

            /* Adjust time after calculations. */
            while (current_time->sntpc_useconds > 1000000)
            {
                current_time->sntpc_seconds += 1;
                current_time->sntpc_useconds -= 1000000;
            }

            /* Adjust for timezone. */
            current_time->sntpc_seconds += (SNTPC_SECS_PER_MIN *
                                            SNTPC_Config->sntpc_timezone);

            /* Adjust for DST. */
            if (SNTPC_Config->sntpc_dst_enabled)
            {
                current_time->sntpc_is_dst = NU_TRUE;
                current_time->sntpc_seconds += (SNTPC_DAYLIGHT_SAVINGS_ADJUST);
            }
            else
            {
                current_time->sntpc_is_dst = NU_FALSE;
            }

            /* If this time source is not synchronized then return the
             * time but also return a status indicating this condition. */
            if (srv_info->sntpc_synced == NU_FALSE)
            {
                status = SNTPC_NOT_SYNCED;
            }
        }
    }

    return(status);
}
