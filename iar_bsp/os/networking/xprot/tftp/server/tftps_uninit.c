/*************************************************************************
*
*             Copyright 1995-2007 Mentor Graphics Corporation
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
*       tftps_uninit.c                                 
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package  -  Nucleus TFTP Server
*
*   DESCRIPTION
*
*       This file contains the TFTP routines necessary to shutdown
*       a TFTP server.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TFTP_Server_Uninit              Uninitializes TFTP Server.
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_net.h
*       nu_tftps.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/nu_networking.h"
#include "networking/nu_tftps.h"

extern TFTPS_CB         TFTPS_Con;
extern VOID            *TFTPS_Task_Ptr;
extern NU_SEMAPHORE     TFTPS_Resource;
extern NU_TASK          tftp_server_task_ptr;

/************************************************************************
*
*   FUNCTION
*
*       TFTP_Server_Uninit
*
*   DESCRIPTION
*
*       This function stops the TFTP Server, deallocates all resources
*       that had been allocated and .uninitializes the server. The
*       TFTP Server can be re-initialized by the application if needed.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       Status          NU_SUCCESS if operation completed successfully
*
************************************************************************/
STATUS TFTP_Server_Uninit()
{
    STATUS status, retval = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    /* Grab the stack semaphore before processing packets. */
    status = NU_Obtain_Semaphore(&TFTPS_Resource, NU_SUSPEND);

    /* Verify that resource was available */
    if (status == NU_SUCCESS)
    {
        /* Close and cleanup existing sockets */
        if (TFTPS_Con.socket_number > -1)
        {
            status = NU_Close_Socket(TFTPS_Con.socket_number);
            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to close TFTP Server socket",
                    NERR_SEVERE, __FILE__, __LINE__);
                retval = NU_INVAL;
            }
        }

        if (TFTPS_Con.session_socket_number > -1)
        {
            status += NU_Close_Socket(TFTPS_Con.session_socket_number);
            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to close TFTP Server session socket",
                    NERR_SEVERE, __FILE__, __LINE__);
                retval = NU_INVAL;
            }
        }

        /* Free up input and output buffer memory from Init Control Block */
        status = NU_Deallocate_Memory((VOID *)(TFTPS_Con.tftp_input_buffer));
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to free TFTP Server input buffer",
                NERR_SEVERE, __FILE__, __LINE__);
            retval = NU_INVAL;
        }

        status = NU_Deallocate_Memory((VOID *)(TFTPS_Con.trans_buf));
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to free TFTP Server trans buffer",
                NERR_SEVERE, __FILE__, __LINE__);
            retval = NU_INVAL;
        }

        /* Terminate TFTP Server Task */
        status = NU_Terminate_Task(&tftp_server_task_ptr);
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to terminate TFTP Server task",
                NERR_SEVERE, __FILE__, __LINE__);
            retval = NU_INVAL;
        }

        /* Delete TFTP Server Task */
        status = NU_Delete_Task(&tftp_server_task_ptr);
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to delete TFTP Server task",
                NERR_SEVERE, __FILE__, __LINE__);
            retval = NU_INVAL;
        }

        /* Free up memory allocated to TFTP Server Task */
        status = NU_Deallocate_Memory(TFTPS_Task_Ptr);
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to free TFTP Server task memory",
                NERR_SEVERE, __FILE__, __LINE__);
            retval = NU_INVAL;
        }

        /* Let other tasks use the TFTP resource */
        if (NU_Release_Semaphore(&TFTPS_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release TFTP Server semaphore",
                NERR_SEVERE, __FILE__, __LINE__);
#ifdef NET_5_2
            NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                NU_Current_Task_Pointer(), NU_NULL);
#endif
        } /* Release resource successful */

        status = NU_Delete_Semaphore(&TFTPS_Resource);
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate TFTP Server semaphore",
                NERR_SEVERE, __FILE__, __LINE__);
            retval = NU_INVAL;
        }
    } /* Obtain resource successful */

    NU_USER_MODE();

    return retval;
} /* end TFTP_Server_Uninit */
