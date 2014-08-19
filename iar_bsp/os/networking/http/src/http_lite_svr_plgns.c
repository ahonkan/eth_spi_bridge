/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2012
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/************************************************************************
*
*   FILE NAME
*
*       http_lite_svr_plgns.c
*
*   COMPONENT
*
*       Nucleus HTTP Lite Server
*
*   DESCRIPTION
*
*       This file holds the HTTP Lite Server routines.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_HTTP_Lite_Register_Plugin
*       HTTP_Lite_Register_Plugin
*       HTTP_Lite_Create_Plugin
*       NU_HTTP_Lite_Remove_Plugin
*       NU_HTTP_Lite_Register_Upgrade_Plugin
*       HTTP_Lite_Register_Upgrade_Plugin
*       NU_HTTP_Lite_Remove_Upgrade_Plugin
*       HTTP_Lite_Remove_Upgrade_Plugin
*
*   DEPENDENCIES
*
*       http_lite.h
*       nu_networking.h
*
************************************************************************/

#include "networking/nu_networking.h"
#include "os/networking/http/inc/http_lite_int.h"
#include "services/reg_api.h"

extern NU_MEMORY_POOL           *MEM_Cached;
extern HTTP_PLUGIN_LIST         HTTP_Lite_Plugins;
extern NU_SEMAPHORE             HTTP_Lite_Resource;
extern HTTP_SVR_SESSION_STRUCT  *HTTP_Session;

STATIC HTTP_PLUGIN_STRUCT *HTTP_Lite_Create_Plugin(CHAR *,
                                                   STATUS (*plug_in)(INT, CHAR*, HTTP_SVR_SESSION_STRUCT*),
                                                   STATUS (*uplug_in)(INT, CHAR*, CHAR*, INT, INT, VOID*),
                                                   INT, STATUS*);
STATIC STATUS HTTP_Lite_Register_Upgrade_Plugin(STATUS (*plug_in)(INT, CHAR *, CHAR *, INT, INT, VOID*),
                                                CHAR *upgrade, INT methods);
STATIC VOID HTTP_Lite_Remove_Upgrade_Plugin(CHAR *upgrade, INT methods);

/************************************************************************
*
*   FUNCTION
*
*       NU_HTTP_Lite_Register_Plugin
*
*   DESCRIPTION
*
*       API function to register a plug-in for the specified method(s).
*       A URI of NULL will set the default plug-in for the specified
*       method(s).
*
*   INPUTS
*
*       *plug_in                The name of the plug-in function.
*       *uri                    The string used to identify the
*                               plug-in.  If NULL, the plug-in should
*                               be used as the default plug-in for
*                               the method(s) specified.
*       methods                 The method(s) for which this plug-in
*                               is allowed; HTTP_LITE_PUT,
*                               HTTP_LITE_POST, HTTP_LITE_DELETE,
*                               HTTP_LITE_GET
*
*   OUTPUTS
*
*       NU_SUCCESS                  The plug-in was registered.
*       HTTP_INVALID_PARAMETER      One of the input parameters is
*                                   invalid.
*
*************************************************************************/
STATUS NU_HTTP_Lite_Register_Plugin(STATUS (*plug_in)(INT, CHAR *, HTTP_SVR_SESSION_STRUCT*),
                                    CHAR *uri, INT methods)
{
    STATUS  status;

    /* Validate the input parameters. */
    if (plug_in)
    {
        /* Get the semaphore. */
        status = NU_Obtain_Semaphore(&HTTP_Lite_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Register the plug-in. */
            status = HTTP_Lite_Register_Plugin(plug_in, uri, methods);

            /* Release the semaphore. */
            NU_Release_Semaphore(&HTTP_Lite_Resource);
        }
    }

    else
    {
        status = HTTP_INVALID_PARAMETER;
    }

    return (status);

} /* NU_HTTP_Lite_Register_Plugin */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Register_Plugin
*
*   DESCRIPTION
*
*       Function to register a plug-in for the specified method(s).
*       A URI of NULL will set the default plug-in for the specified
*       method(s).
*
*   INPUTS
*
*       *plug_in                The name of the plug-in function.
*       *uri                    The string used to identify the
*                               plug-in.  If NULL, the plug-in should
*                               be used as the default plug-in for
*                               the method(s) specified.
*       methods                 The method(s) for which this plug-in
*                               is allowed; HTTP_LITE_PUT,
*                               HTTP_LITE_POST, HTTP_LITE_DELETE,
*                               HTTP_LITE_GET
*
*   OUTPUTS
*
*       NU_SUCCESS                  The plug-in was registered.
*       HTTP_INVALID_PARAMETER      One of the input parameters is
*                                   invalid.
*
*************************************************************************/
STATUS HTTP_Lite_Register_Plugin(STATUS (*plug_in)(INT, CHAR *, HTTP_SVR_SESSION_STRUCT*),
                                 CHAR *uri, INT methods)
{
    HTTP_PLUGIN_STRUCT  *plug_ptr;
    STATUS              status;

    /* If the URI is NULL, this is a default plug-in for the specified
     * method.
     */
    if (!uri)
    {
        /* At least one valid method must have been specified. */
        status = HTTP_INVALID_PARAMETER;

        /* The same plug-in can be used for multiple methods, so check
         * each method separately.
         */
        if (methods & HTTP_LITE_PUT)
        {
            /* If a plug-in has not already been registered for the
             * PUT command.
             */
            if (!HTTP_Session->plug_ptr_put)
            {
                HTTP_Session->plug_ptr_put =
                        HTTP_Lite_Create_Plugin(uri, plug_in, NU_NULL, HTTP_LITE_PUT, &status);
            }

            /* Overwrite the existing default. */
            else
            {
                HTTP_Session->plug_ptr_put->plugin = plug_in;
                status = NU_SUCCESS;
            }
        }

        if (methods & HTTP_LITE_POST)
        {
            /* If a plug-in has not already been registered for the
             * POST command.
             */
            if (!HTTP_Session->plug_ptr_post)
            {
                HTTP_Session->plug_ptr_post =
                        HTTP_Lite_Create_Plugin(uri, plug_in, NU_NULL, HTTP_LITE_POST, &status);
            }

            /* Overwrite the existing default. */
            else
            {
                HTTP_Session->plug_ptr_post->plugin = plug_in;
                status = NU_SUCCESS;
            }
        }

        if (methods & HTTP_LITE_DELETE)
        {
            /* If a plug-in has not already been registered for the
             * DELETE command.
             */
            if (!HTTP_Session->plug_ptr_delete)
            {
                HTTP_Session->plug_ptr_delete =
                        HTTP_Lite_Create_Plugin(uri, plug_in, NU_NULL, HTTP_LITE_DELETE, &status);
            }

            /* Overwrite the existing default. */
            else
            {
                HTTP_Session->plug_ptr_delete->plugin = plug_in;
                status = NU_SUCCESS;
            }
        }

        if (methods & HTTP_LITE_GET)
        {
            /* If a plug-in has not already been registered for the
             * PUT command.
             */
            if (!HTTP_Session->plug_ptr_get)
            {
                HTTP_Session->plug_ptr_get =
                        HTTP_Lite_Create_Plugin(uri, plug_in, NU_NULL, HTTP_LITE_GET, &status);
            }

            /* Overwrite the existing default. */
            else
            {
                HTTP_Session->plug_ptr_get->plugin = plug_in;
                status = NU_SUCCESS;
            }
        }
    }

    else
    {
        /* Check if this plug-in has already been created.  Do not
         * look for a plug-in by method.
         */
        plug_ptr = HTTP_Lite_Get_Plugin(uri, NU_NULL);

         /* If there is not already a plug-in with this name. */
        if (plug_ptr == NU_NULL)
        {
            plug_ptr = HTTP_Lite_Create_Plugin(uri, plug_in, NU_NULL, methods, &status);

            /* Set up the elements of the new plug-in. */
            if (plug_ptr)
            {
                /* Add the plug-in to the list. */
                DLL_Enqueue(&HTTP_Lite_Plugins, plug_ptr);
            }
        }

        else
        {
            /* Add the specified methods for this plug-in. */
            plug_ptr->methods |= methods;

            status = NU_SUCCESS;
        }
    }

    return (status);

} /* HTTP_Lite_Register_Plugin */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Create_Plugin
*
*   DESCRIPTION
*
*       Function to allocate memory for a plug-in for the specified
*       method(s) and set up the plug-in data structure accordingly.
*
*   INPUTS
*
*       *name                   The string used to identify the
*                               plug-in.  If NULL, the plug-in should
*                               be used as the default plug-in for
*                               the method(s) specified.
*       *plug_in                The name of the plug-in function if not
*                               being used for an upgrade command.
*       *uplug_in               If this registration is for a plug-in for
*                               the upgrade command, the name of the plug-in
*                               function to use when that upgrade command
*                               is received.
*       methods                 The method(s) for which this plug-in
*                               is allowed; HTTP_LITE_PUT,
*                               HTTP_LITE_POST, HTTP_LITE_DELETE,
*                               HTTP_LITE_GET
*       *status                 Pointer that will be filled in with
*                               status of the call.
*
*   OUTPUTS
*
*       Pointer to the new plug-in structure.
*
*************************************************************************/
STATIC HTTP_PLUGIN_STRUCT *HTTP_Lite_Create_Plugin(CHAR *name,
                                                   STATUS (*plug_in)(INT, CHAR*, HTTP_SVR_SESSION_STRUCT*),
                                                   STATUS (*uplug_in)(INT, CHAR*, CHAR*, INT, INT, VOID*),
                                                   INT methods, STATUS *status)
{
    HTTP_PLUGIN_STRUCT  *plug_ptr = NU_NULL;

    /* Allocate memory for the new plug-in. */
    *status = NU_Allocate_Memory(MEM_Cached, (VOID**)&plug_ptr,
                                 sizeof(HTTP_PLUGIN_STRUCT) + (name ? (strlen(name) + 1) : 0),
                                 NU_SUSPEND);

    /* Set up the elements of the new plug-in. */
    if (*status == NU_SUCCESS)
    {
        if (name)
        {
            /* Set up the pointer to the memory for the name. */
            plug_ptr->http_name = (CHAR*)(&plug_ptr[1]);

            /* Copy parameters. */
            strcpy(plug_ptr->http_name, name);
        }

        else
        {
            plug_ptr->http_name = NU_NULL;
        }

        /* Null out the flink and blink. */
        plug_ptr->blink = NU_NULL;
        plug_ptr->flink = NU_NULL;

        /* Set up the remaining parameters. */
        plug_ptr->plugin = plug_in;
        plug_ptr->uplugin = uplug_in;
        plug_ptr->methods = methods;
    }

    return (plug_ptr);

} /* HTTP_Lite_Create_Plugin */

/************************************************************************
*
*   FUNCTION
*
*       NU_HTTP_Lite_Remove_Plugin
*
*   DESCRIPTION
*
*       Function to remove a plug-in.
*
*   INPUTS
*
*       *uri                    The string used to identify the
*                               plug-in. If no string is specified,
*                               the default URI for the specified
*                               method(s) will be cleared.
*       methods                 The method(s) for which this plug-in
*                               is deleted; HTTP_LITE_PUT,
*                               HTTP_LITE_POST, HTTP_LITE_DELETE,
*                               HTTP_LITE_GET
*
*   OUTPUTS
*
*       NU_SUCCESS              The plug-in was removed.  This routine
*                               returns NU_SUCCESS whether the URI was
*                               found or not.
*
*************************************************************************/
STATUS NU_HTTP_Lite_Remove_Plugin(CHAR *uri, INT methods)
{
    STATUS              status;

    /* Get the semaphore. */
    status = NU_Obtain_Semaphore(&HTTP_Lite_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Remove the plug-in. */
        HTTP_Lite_Remove_Plugin(uri, methods);

        /* Release the semaphore. */
        NU_Release_Semaphore(&HTTP_Lite_Resource);
    }

    return (status);

} /* NU_HTTP_Lite_Remove_Plugin */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Remove_Plugin
*
*   DESCRIPTION
*
*       Function to remove a plug-in.
*
*   INPUTS
*
*       *uri                    The string used to identify the
*                               plug-in. If no string is specified,
*                               the default URI for the specified
*                               method(s) will be cleared.
*       methods                 The method(s) for which this plug-in
*                               is deleted; HTTP_LITE_PUT,
*                               HTTP_LITE_POST, HTTP_LITE_DELETE,
*                               HTTP_LITE_GET
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID HTTP_Lite_Remove_Plugin(CHAR *uri, INT methods)
{
    HTTP_PLUGIN_STRUCT  *plug_ptr;

    /* If a specific URI is being referenced. */
    if (uri)
    {
        /* Get a pointer to the plug-in structure. */
        plug_ptr = HTTP_Lite_Get_Plugin(uri, NU_NULL);

        /* If the plug-in was registered. */
        if (plug_ptr)
        {
            /* Delete the requested methods. */
            plug_ptr->methods &= ~methods;

            /* If all methods were deleted, remove the plug-in from
             * the system.
             */
            if (!plug_ptr->methods)
            {
                /* Remove the plug-in from the list. */
                DLL_Remove(&HTTP_Lite_Plugins, plug_ptr);

                /* Deallocate the memory. */
                NU_Deallocate_Memory((VOID*)plug_ptr);
            }
        }
    }

    /* The application wants to remove a default plug-in. */
    else
    {
        /* The same plug-in can be used for multiple methods, so check
         * each method separately.
         */
        if (methods & HTTP_LITE_PUT)
        {
            /* If a plug-in has been registered for the PUT command. */
            if (HTTP_Session->plug_ptr_put)
            {
                /* Do not deallocate memory - just NULL out the plug-in. */
                HTTP_Session->plug_ptr_put->plugin = NU_NULL;
            }
        }

        if (methods & HTTP_LITE_POST)
        {
            /* If a plug-in has been registered for the POST command. */
            if (HTTP_Session->plug_ptr_post)
            {
                /* Do not deallocate memory - just NULL out the plug-in. */
                HTTP_Session->plug_ptr_post->plugin = NU_NULL;
            }
        }

        if (methods & HTTP_LITE_DELETE)
        {
            /* If a plug-in has been registered for the DELETE command. */
            if (HTTP_Session->plug_ptr_delete)
            {
                /* Do not deallocate memory - just NULL out the plug-in. */
                HTTP_Session->plug_ptr_delete->plugin = NU_NULL;
            }
        }

        if (methods & HTTP_LITE_GET)
        {
            /* If a plug-in has been registered for the GET command. */
            if (HTTP_Session->plug_ptr_get)
            {
                /* Do not deallocate memory - just NULL out the plug-in. */
                HTTP_Session->plug_ptr_get->plugin = NU_NULL;
            }
        }
    }

} /* HTTP_Lite_Remove_Plugin */

/************************************************************************
*
*   FUNCTION
*
*       NU_HTTP_Lite_Register_Upgrade_Plugin
*
*   DESCRIPTION
*
*       API function to register a plug-in for the specified upgrade request
*       and method(s).  An upgrade of NULL will set the default plug-in for
*       the specified method(s).
*
*   INPUTS
*
*       *plug_in                The name of the plug-in function to invoke
*                               when an upgrade request is received for the
*                               specified method(s).
*       *upgrade                The string used to identify the
*                               plug-in.  If NULL, the plug-in should
*                               be used as the default plug-in for
*                               the method(s) specified when any upgrade
*                               request is received that does not match a
*                               specific plug-in.
*       methods                 The method(s) for which this plug-in
*                               is allowed; HTTP_LITE_PUT,
*                               HTTP_LITE_POST, HTTP_LITE_DELETE,
*                               HTTP_LITE_GET
*
*   OUTPUTS
*
*       NU_SUCCESS                  The plug-in was registered.
*       HTTP_INVALID_PARAMETER      One of the input parameters is
*                                   invalid.
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATUS NU_HTTP_Lite_Register_Upgrade_Plugin(STATUS (*plug_in)(INT, CHAR *,
                                            CHAR *, INT, INT, VOID *),
                                            CHAR *upgrade, INT methods)
{
    STATUS  status;

    /* Validate the input parameters. */
    if (plug_in)
    {
        /* Get the semaphore. */
        status = NU_Obtain_Semaphore(&HTTP_Lite_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Register the plug-in. */
            status = HTTP_Lite_Register_Upgrade_Plugin(plug_in, upgrade, methods);

            /* Release the semaphore. */
            NU_Release_Semaphore(&HTTP_Lite_Resource);
        }
    }

    else
    {
        status = HTTP_INVALID_PARAMETER;
    }

    return (status);

} /* NU_HTTP_Lite_Register_Upgrade_Plugin */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Register_Upgrade_Plugin
*
*   DESCRIPTION
*
*       Function to register a plug-in for the specified upgrade and
*       respective method(s). An upgrade of NULL will set the default
*       plug-in for the specified method(s).
*
*   INPUTS
*
*       *plug_in                The name of the plug-in function.
*       *upgrade                The string used to identify the
*                               plug-in.  If NULL, the plug-in should
*                               be used as the default plug-in for
*                               the method(s) specified.
*       methods                 The method(s) for which this plug-in
*                               is allowed; HTTP_LITE_PUT,
*                               HTTP_LITE_POST, HTTP_LITE_DELETE,
*                               HTTP_LITE_GET
*
*   OUTPUTS
*
*       NU_SUCCESS                  The plug-in was registered.
*       HTTP_INVALID_PARAMETER      One of the input parameters is
*                                   invalid.
*
*************************************************************************/
STATUS HTTP_Lite_Register_Upgrade_Plugin(STATUS (*plug_in)(INT, CHAR *,
                                         CHAR *, INT, INT, VOID *),
                                         CHAR *upgrade, INT methods)
{
    HTTP_PLUGIN_STRUCT  *plug_ptr;
    STATUS              status;

    /* If the upgrade is NULL, this is a default plug-in for the specified
     * method.
     */
    if (!upgrade)
    {
        /* At least one valid method must have been specified. */
        status = HTTP_INVALID_PARAMETER;

        /* The same plug-in can be used for multiple methods, so check
         * each method separately.
         */
        if (methods & HTTP_LITE_PUT)
        {
            /* If a plug-in has not already been registered for the
             * PUT command.
             */
            if (!HTTP_Session->uplug_ptr_put)
            {
                HTTP_Session->uplug_ptr_put =
                        HTTP_Lite_Create_Plugin(upgrade, NU_NULL, plug_in, HTTP_LITE_PUT, &status);
            }

            /* Overwrite the existing default. */
            else
            {
                HTTP_Session->uplug_ptr_put->uplugin = plug_in;
                status = NU_SUCCESS;
            }
        }

        if (methods & HTTP_LITE_POST)
        {
            /* If a plug-in has not already been registered for the
             * POST command.
             */
            if (!HTTP_Session->uplug_ptr_post)
            {
                HTTP_Session->uplug_ptr_post =
                        HTTP_Lite_Create_Plugin(upgrade, NU_NULL, plug_in, HTTP_LITE_POST, &status);
            }

            /* Overwrite the existing default. */
            else
            {
                HTTP_Session->uplug_ptr_post->uplugin = plug_in;
                status = NU_SUCCESS;
            }
        }

        if (methods & HTTP_LITE_DELETE)
        {
            /* If a plug-in has not already been registered for the
             * DELETE command.
             */
            if (!HTTP_Session->uplug_ptr_delete)
            {
                HTTP_Session->uplug_ptr_delete =
                        HTTP_Lite_Create_Plugin(upgrade, NU_NULL, plug_in, HTTP_LITE_DELETE, &status);
            }

            /* Overwrite the existing default. */
            else
            {
                HTTP_Session->uplug_ptr_delete->uplugin = plug_in;
                status = NU_SUCCESS;
            }
        }

        if (methods & HTTP_LITE_GET)
        {
            /* If a plug-in has not already been registered for the
             * GET command.
             */
            if (!HTTP_Session->uplug_ptr_get)
            {
                HTTP_Session->uplug_ptr_get =
                        HTTP_Lite_Create_Plugin(upgrade, NU_NULL, plug_in, HTTP_LITE_GET, &status);
            }

            /* Overwrite the existing default. */
            else
            {
                HTTP_Session->uplug_ptr_get->uplugin = plug_in;
                status = NU_SUCCESS;
            }
        }
    }

    else
    {
        /* Check if this plug-in has already been created.  Do not
         * look for a plug-in by method.
         */
        plug_ptr = HTTP_Lite_Get_Upgrade_Plugin(upgrade, NU_NULL);

         /* If there is not already a plug-in with this name. */
        if (plug_ptr == NU_NULL)
        {
            plug_ptr = HTTP_Lite_Create_Plugin(upgrade, NU_NULL, plug_in, methods, &status);

            /* Set up the elements of the new plug-in. */
            if (plug_ptr)
            {
                /* Add the plug-in to the list. */
                DLL_Enqueue(&HTTP_Lite_Plugins, plug_ptr);
            }
        }

        else
        {
            /* Add the specified methods for this plug-in. */
            plug_ptr->methods |= methods;

            status = NU_SUCCESS;
        }
    }

    return (status);

} /* HTTP_Lite_Register_Upgrade_Plugin */

/************************************************************************
*
*   FUNCTION
*
*       NU_HTTP_Lite_Remove_Upgrade_Plugin
*
*   DESCRIPTION
*
*       Function to remove an upgrade plug-in.
*
*   INPUTS
*
*       *upgrade                The string used to identify the
*                               plug-in. If no string is specified,
*                               the default plug-in for the specified
*                               method(s) will be cleared.
*       methods                 The method(s) for which this plug-in
*                               is deleted; HTTP_LITE_PUT,
*                               HTTP_LITE_POST, HTTP_LITE_DELETE,
*                               HTTP_LITE_GET
*
*   OUTPUTS
*
*       NU_SUCCESS              The plug-in was removed.  This routine
*                               returns NU_SUCCESS whether the upgrade was
*                               found or not.
*
*************************************************************************/
STATUS NU_HTTP_Lite_Remove_Upgrade_Plugin(CHAR *upgrade, INT methods)
{
    STATUS              status;

    /* Get the semaphore. */
    status = NU_Obtain_Semaphore(&HTTP_Lite_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Remove the plug-in. */
        HTTP_Lite_Remove_Upgrade_Plugin(upgrade, methods);

        /* Release the semaphore. */
        NU_Release_Semaphore(&HTTP_Lite_Resource);
    }

    return (status);

} /* NU_HTTP_Lite_Remove_Upgrade_Plugin */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Remove_Upgrade_Plugin
*
*   DESCRIPTION
*
*       Function to remove an upgrade plug-in.
*
*   INPUTS
*
*       *upgrade                The string used to identify the
*                               plug-in. If no string is specified,
*                               the default plug-in for the specified
*                               method(s) will be cleared.
*       methods                 The method(s) for which this plug-in
*                               is deleted; HTTP_LITE_PUT,
*                               HTTP_LITE_POST, HTTP_LITE_DELETE,
*                               HTTP_LITE_GET
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID HTTP_Lite_Remove_Upgrade_Plugin(CHAR *upgrade, INT methods)
{
    HTTP_PLUGIN_STRUCT  *plug_ptr;

    /* If a specific upgrade is being referenced. */
    if (upgrade)
    {
        /* Get a pointer to the plug-in structure. */
        plug_ptr = HTTP_Lite_Get_Upgrade_Plugin(upgrade, NU_NULL);

        /* If the plug-in was registered. */
        if (plug_ptr)
        {
            /* Delete the requested methods. */
            plug_ptr->methods &= ~methods;

            /* If all methods were deleted, remove the plug-in from
             * the system.
             */
            if (!plug_ptr->methods)
            {
                /* Remove the plug-in from the list. */
                DLL_Remove(&HTTP_Lite_Plugins, plug_ptr);

                /* Deallocate the memory. */
                NU_Deallocate_Memory((VOID*)plug_ptr);
            }
        }
    }

    /* The application wants to remove a default plug-in. */
    else
    {
        /* The same plug-in can be used for multiple methods, so check
         * each method separately.
         */
        if (methods & HTTP_LITE_PUT)
        {
            /* If a plug-in has been registered for the PUT command. */
            if (HTTP_Session->uplug_ptr_put)
            {
                /* Do not deallocate memory - just NULL out the plug-in. */
                HTTP_Session->uplug_ptr_put->uplugin = NU_NULL;
            }
        }

        if (methods & HTTP_LITE_POST)
        {
            /* If a plug-in has been registered for the POST command. */
            if (HTTP_Session->uplug_ptr_post)
            {
                /* Do not deallocate memory - just NULL out the plug-in. */
                HTTP_Session->uplug_ptr_post->uplugin = NU_NULL;
            }
        }

        if (methods & HTTP_LITE_DELETE)
        {
            /* If a plug-in has been registered for the DELETE command. */
            if (HTTP_Session->uplug_ptr_delete)
            {
                /* Do not deallocate memory - just NULL out the plug-in. */
                HTTP_Session->uplug_ptr_delete->uplugin = NU_NULL;
            }
        }

        if (methods & HTTP_LITE_GET)
        {
            /* If a plug-in has been registered for the GET command. */
            if (HTTP_Session->uplug_ptr_get)
            {
                /* Do not deallocate memory - just NULL out the plug-in. */
                HTTP_Session->uplug_ptr_get->uplugin = NU_NULL;
            }
        }
    }

} /* HTTP_Lite_Remove_Upgrade_Plugin */
