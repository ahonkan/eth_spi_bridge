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
*       http_lite_svr_init.c
*
*   COMPONENT
*
*       Nucleus HTTP Lite Server
*
*   DESCRIPTION
*
*       This file holds the HTTP Lite Server initialization and shut
*       down routines.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_HTTP_Lite_Server_Shutdown
*       NU_HTTP_Lite_Server_Init
*
*   DEPENDENCIES
*
*       nu_http_lite.h
*       nu_networking.h
*
************************************************************************/

#include "networking/nu_networking.h"
#include "os/networking/http/inc/http_lite_int.h"

extern NU_MEMORY_POOL   *MEM_Cached;

HTTP_PLUGIN_LIST        HTTP_Lite_Plugins;
NU_TASK                 HTTP_Lite_Svr_Receive_CB;
HTTP_SVR_SESSION_STRUCT *HTTP_Session = NU_NULL;
NU_SEMAPHORE            HTTP_Lite_Resource;
HTTP_FILE_LIST          HTTP_Lite_File_List;

/************************************************************************
*
*   FUNCTION
*
*       NU_HTTP_Lite_Server_Init
*
*   DESCRIPTION
*
*       This function initializes the HTTP Lite tasks and related
*       data structures.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       status              NU_SUCCESS is returned if initialization
*                           has completed successfully.  Otherwise, an
*                           operating-system specific error is returned.
*
************************************************************************/
STATUS NU_HTTP_Lite_Server_Init(VOID)
{
    STATUS         status = NU_SUCCESS;

    /* Ensure the server is not already initialized. */
    if (!HTTP_Session)
    {
        /* If initializing manually, the synchronization semaphore must be
         * created. Ignore failure since the semaphore must be obtained below.
         */
        (VOID)NU_Create_Semaphore(&HTTP_Lite_Resource, "HTTP", (UNSIGNED)1, NU_FIFO);

        /* Obtain the semaphore so the application does not try to shut the
         * server down while it is initializing.
         */
        status = NU_Obtain_Semaphore(&HTTP_Lite_Resource, NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Initialize the file list. */
            HTTP_Lite_File_List.flink = NU_NULL;
            HTTP_Lite_File_List.blink = NU_NULL;

            /* Initialize the plug-in list. */
            HTTP_Lite_Plugins.flink = NU_NULL;
            HTTP_Lite_Plugins.blink = NU_NULL;

            /* Allocate memory for the HTTP Session structure. */
            status = NU_Allocate_Memory(MEM_Cached, (VOID **)&HTTP_Session,
                                        sizeof(HTTP_SVR_SESSION_STRUCT),
                                        (UNSIGNED)NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Initialize the session parameters to zero.  If initialization
                 * fails, this will tell us which parameters to clean up.
                 */
                memset(HTTP_Session, 0, sizeof(HTTP_SVR_SESSION_STRUCT));

                /* Initialize the sockets to -1.  If initialization fails,
                 * this will tell us which sockets to deallocate.
                 */
                HTTP_Session->socketd = -1;
                HTTP_Session->http_listener = -1;
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                HTTP_Session->ssl_listener = -1;
#endif

                /* Allocate memory for the input/output buffer. */
                status = NU_Allocate_Memory(MEM_Cached,
                                            (VOID **)&HTTP_Session->buffer,
                                            HTTP_SVR_RCV_SIZE,
                                            (UNSIGNED)NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
#if (HTTP_PUT_FILE == NU_TRUE)
                    /* Register the default plug-in for the PUT command. */
                    status = HTTP_Lite_Register_Plugin(HTTP_Lite_Svr_Put, NU_NULL,
                                                       HTTP_LITE_PUT);
#endif

#if (HTTP_GET_FILE == NU_TRUE)
                    if (status == NU_SUCCESS)
                    {
                        /* Register the default plug-in for the GET command. */
                        status = HTTP_Lite_Register_Plugin(HTTP_Lite_Svr_Get, NU_NULL,
                                                           HTTP_LITE_GET);
                    }
#endif

#if (HTTP_DELETE_FILE == NU_TRUE)
                    if (status == NU_SUCCESS)
                    {
                        /* Register the default plug-in for the DELETE command. */
                        status = HTTP_Lite_Register_Plugin(HTTP_Lite_Svr_Delete, NU_NULL,
                                                           HTTP_LITE_DELETE);
                    }
#endif

                    if (status == NU_SUCCESS)
                    {
                        /* Allocate memory for the receiving task. */
                        status = NU_Allocate_Memory(MEM_Cached,
                                                    &HTTP_Session->tsk_memory,
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                                                    HTTP_SSL_SVR_STACK_SIZE,
#else
                                                    HTTP_SVR_STACK_SIZE,
#endif
                                                    NU_NO_SUSPEND);

                        if (status == NU_SUCCESS)
                        {
                            /* Create the receive task. */
                            status = NU_Create_Task(&HTTP_Lite_Svr_Receive_CB, "HTTPSrv",
                                                    HTTP_Lite_Receive_Task, 0,
                                                    NU_NULL, HTTP_Session->tsk_memory,
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                                                    HTTP_SSL_SVR_STACK_SIZE,
#else
                                                    HTTP_SVR_STACK_SIZE,
#endif
                                                    HTTP_SVR_PRIORITY,
                                                    HTTP_SVR_TIME_SLICE, HTTP_SVR_PREEMPT,
                                                    NU_START);
                        }
                    }
                }
            }

            /* Release the semaphore. */
            NU_Release_Semaphore(&HTTP_Lite_Resource);

            /* Clean up everything that was successfully initialized. */
            if (status != NU_SUCCESS)
                NU_HTTP_Lite_Server_Shutdown();
        }
    }

    /* If an error occurred, log it. */
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Could not initialize HTTP Lite server.\n",
                       NERR_FATAL, __FILE__, __LINE__);
    }

    return (status);

} /* NU_HTTP_Lite_Server_Init */

/************************************************************************
*
*   FUNCTION
*
*       NU_HTTP_Lite_Server_Shutdown
*
*   DESCRIPTION
*
*       This routine shuts down the HTTP Lite Server.  The routine
*       does not return a status, because even if an error occurs,
*       the shutdown process resumes.
*
*       This routine will not completely shut down the CyaSSL module
*       since the module could be used by other middleware and/or the
*       client.  If the application wants CyaSSL to be shut down, it
*       must explicitly do so.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
************************************************************************/
VOID NU_HTTP_Lite_Server_Shutdown(VOID)
{
    HTTP_PLUGIN_STRUCT  *plug_ptr;

    /* Get the semaphore. */
    if (NU_Obtain_Semaphore(&HTTP_Lite_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* If the HTTP Session structure has been created. */
        if (HTTP_Session)
        {
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)

            /* Close the SSL listener socket. */
            if (HTTP_Session->ssl_listener != -1)
                NU_Close_Socket(HTTP_Session->ssl_listener);

            if (HTTP_Session->ctx)
            {
                /* Free the context structure. */
                CyaSSL_CTX_free(HTTP_Session->ctx);
                HTTP_Session->ctx = NU_NULL;
            }

            if (CyaSSL_Cleanup())
            {
                NLOG_Error_Log("Error cleaning up CyaSSL.\n",
                               NERR_FATAL, __FILE__, __LINE__);
            }

#endif

            /* If the receive task was created. */
            if (HTTP_Session->tsk_memory)
            {
                /* Terminate the receiving task. */
                if (NU_Terminate_Task(&HTTP_Lite_Svr_Receive_CB) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Could not terminate HTTP server task.\n",
                                   NERR_FATAL, __FILE__, __LINE__);
                }

                /* Delete the receiving task. */
                if (NU_Delete_Task(&HTTP_Lite_Svr_Receive_CB) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Could not delete HTTP server task.\n",
                                   NERR_FATAL, __FILE__, __LINE__);
                }

                /* Deallocate memory for the task. */
                if (NU_Deallocate_Memory(HTTP_Session->tsk_memory) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Could not deallocate memory for HTTP receive task.\n",
                                   NERR_FATAL, __FILE__, __LINE__);
                }
            }

            if (HTTP_Session->buffer)
            {
                /* Deallocate memory for the input/output buffer. */
                if (NU_Deallocate_Memory(HTTP_Session->buffer) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Could not deallocate memory for HTTP buffer.\n",
                                   NERR_FATAL, __FILE__, __LINE__);
                }
            }

            /* Close the HTTP listener socket. */
            if (HTTP_Session->http_listener >= 0)
                NU_Close_Socket(HTTP_Session->http_listener);

            /* If a connection was in progress, close the socket. */
            if (HTTP_Session->socketd >= 0)
                NU_Close_Socket(HTTP_Session->socketd);

            /* If a default POST plug-in exists. */
            if (HTTP_Session->plug_ptr_post)
            {
                if (NU_Deallocate_Memory(HTTP_Session->plug_ptr_post) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Could not deallocate memory for HTTP post plugin structure.\n",
                                   NERR_FATAL, __FILE__, __LINE__);
                }
            }

            /* If a default PUT plug-in exists. */
            if (HTTP_Session->plug_ptr_put)
            {
                if (NU_Deallocate_Memory(HTTP_Session->plug_ptr_put) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Could not deallocate memory for HTTP put plugin structure.\n",
                                   NERR_FATAL, __FILE__, __LINE__);
                }
            }

            /* If a default DELETE plug-in exists. */
            if (HTTP_Session->plug_ptr_delete)
            {
                if (NU_Deallocate_Memory(HTTP_Session->plug_ptr_delete) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Could not deallocate memory for HTTP delete plugin structure.\n",
                                   NERR_FATAL, __FILE__, __LINE__);
                }
            }

            /* If a default GET plug-in exists. */
            if (HTTP_Session->plug_ptr_get)
            {
                if (NU_Deallocate_Memory(HTTP_Session->plug_ptr_get) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Could not deallocate memory for HTTP get plugin structure.\n",
                                   NERR_FATAL, __FILE__, __LINE__);
                }
            }

            /* Deallocate memory for the HTTP Session structure. */
            if (NU_Deallocate_Memory(HTTP_Session) != NU_SUCCESS)
            {
                NLOG_Error_Log("Could not deallocate memory for HTTP Session structure.\n",
                               NERR_FATAL, __FILE__, __LINE__);
            }

            HTTP_Session = NU_NULL;
        }

        /* Remove each file from the system. */
        while (HTTP_Lite_File_List.flink)
        {
            /* Delete the file. */
            if (HTTP_Lite_Delete_File(HTTP_Lite_File_List.flink) != NU_SUCCESS)
            {
                NLOG_Error_Log("Could not remove file from HTTP file system.\n",
                               NERR_FATAL, __FILE__, __LINE__);
            }
        }

        HTTP_Lite_File_List.flink = NU_NULL;
        HTTP_Lite_File_List.blink = NU_NULL;

        /* Remove each plug-in from the system. */
        while (HTTP_Lite_Plugins.flink)
        {
            /* Remove the plug-in from the list. */
            plug_ptr = DLL_Dequeue((VOID*)&HTTP_Lite_Plugins);

            /* Deallocate the memory associated with the plug-in. */
            if (NU_Deallocate_Memory(plug_ptr) != NU_SUCCESS)
            {
                NLOG_Error_Log("Could not deallocate memory for HTTP plug-in.\n",
                               NERR_FATAL, __FILE__, __LINE__);
            }
        }

        HTTP_Lite_Plugins.flink = NU_NULL;
        HTTP_Lite_Plugins.blink = NU_NULL;

        /* Delete the synchronization semaphore. */
        if (NU_Delete_Semaphore(&HTTP_Lite_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to delete semaphore.\n",
                           NERR_FATAL, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Could not obtain semaphore to shut down HTTP server.\n",
                       NERR_FATAL, __FILE__, __LINE__);
    }

} /* NU_HTTP_Lite_Server_Shutdown */
