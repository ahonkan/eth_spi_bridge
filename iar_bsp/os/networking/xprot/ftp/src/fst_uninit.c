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
*       fst_uninit.c                                   
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Server
*
*   DESCRIPTION
*
*       The Ftp Server uninit function provides an API for stopping the
*       ftp server and freeing up all resources associated with it.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_FTP_Server_Uninit
*       FST_Cleanup
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_net.h
*       nu_ftp.h
*       fc_extr.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/nu_net.h"
#include "networking/nu_ftp.h"
#include "networking/fc_extr.h"

/* Define external data structures.  */
extern NU_SEMAPHORE    FST_Master_Lock;
extern NU_TASK         FST_Master_Task;
extern NU_TASK         FST_Cleaner_Task;
extern NU_QUEUE        FST_Cleaner_Queue;
extern VOID            *FST_Master_Task_Mem;
extern VOID            *FST_Cleaner_Task_Mem;
extern VOID            *FST_Cleaner_Queue_Mem;
extern UINT16          FST_Active_Clients;
extern UNSIGNED_CHAR   FST_Total_Conn;
extern UINT8           FST_Master_Task_Terminated;
extern NU_PROTECT      FST_Master_Task_Terminated_Protect;
extern FST_ACTIVE_LIST FST_Active_List[FTP_SERVER_MAX_CONNECTIONS];
extern INT             FST_Master_Socketd;

/************************************************************************
*
*   FUNCTION
*
*       NU_FTP_Server_Uninit
*
*   DESCRIPTION
*
*       Uninitialize the Nucleus FTP Server
*
*   INPUTS
*
*       flags                   A variable to determine shutdown mode.
*                               Possible values are
*                               (flags & FTP_SERVER_STOP_ALL) is NU_TRUE
*                                   for total shutdown and
*                               (flags & FTP_SERVER_STOP_ALL) is NU_FALSE
*                                   for partial shutdown.
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation successful
*       NU_INVAL                A general-purpose error condition. This
*                               generally indicates that a required
*                               resource (task, semaphore, etc.) could not
*                               be deallocated or deleted.
*
************************************************************************/
STATUS NU_FTP_Server_Uninit(UINT8 flags)
{
    STATUS          status, retval = NU_SUCCESS;
    INT             temp;
    NU_TASK         *taskToClean;
    UINT8           taskTerminated = NU_FALSE;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    NU_Protect(&FST_Master_Task_Terminated_Protect);

    /* Get FST_Master_Task_Terminated status */
    status = FST_Master_Task_Terminated;

    NU_Unprotect();

    /* If the Terminated flag is False */
    if (status == NU_FALSE)
    {
        /* Suspend the Master Task */
        status = NU_Suspend_Task(&FST_Master_Task);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to Suspend FTP Server master task\n",
                           NERR_SEVERE, __FILE__, __LINE__);
            retval = NU_INVAL;
        }

        /* Close the Master Task Socket */
        status = NU_Close_Socket((INT16)FST_Master_Socketd);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to close FTP Server master socket\n",
                           NERR_SEVERE, __FILE__, __LINE__);
            retval = NU_INVAL;
        }

        /* Terminate FTP Server Task */
        status = NU_Terminate_Task(&FST_Master_Task);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to terminate FTP Server master task\n",
                           NERR_SEVERE, __FILE__, __LINE__);
            retval = NU_INVAL;
        }

        /* Delete FTP Server Task */
        status = NU_Delete_Task(&FST_Master_Task);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to delete FTP Server master task\n",
                           NERR_SEVERE, __FILE__, __LINE__);
            retval = NU_INVAL;
        }

        /* Deallocate FTP Server Task Memory */
        status = NU_Deallocate_Memory(FST_Master_Task_Mem);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to free FTP Server master task memory\n",
                           NERR_SEVERE, __FILE__, __LINE__);
            retval = NU_INVAL;
        }

        /* Set Master Task Memory pointer to Null */
        FST_Master_Task_Mem = NU_NULL;

        NU_Protect(&FST_Master_Task_Terminated_Protect);

        /* Set Master Task Terminated flag to True */
        FST_Master_Task_Terminated = NU_TRUE;

        NU_Unprotect();
    }


    /* Check flag to determine if client tasks have
     * to be terminated immediately or to allow completion.
     */
    if (flags & FTP_SERVER_STOP_ALL)
    {
        /* Send all active client tasks to the Cleaner Queue */
        for (temp = 0; temp < FTP_SERVER_MAX_CONNECTIONS; temp++)
        {
            if ( (FST_Active_List[temp].active_task != NU_NULL) &&
                 (FST_Active_List[temp].active_sckt >= 0) )
            {
                taskToClean = FST_Active_List[temp].active_task;

                /* Send 'Service closing' message */
                status = (INT)NU_Send(FST_Active_List[temp].active_sckt,
                                 MSG221, SIZE221, 0);

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed sending 'Service closing' message\n",
                                   NERR_SEVERE, __FILE__, __LINE__);
                    retval = NU_INVAL;
                }

                /* Close active socket */
                status = NU_Close_Socket(FST_Active_List[temp].active_sckt);

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to close FTP Server control socket\n",
                                   NERR_SEVERE, __FILE__, __LINE__);
                    retval = NU_INVAL;
                }

                /* Send Client Task to Cleaner Queue */
                status = NU_Send_To_Queue(&FST_Cleaner_Queue,
                                          (VOID *)&taskToClean, 1, NU_SUSPEND);

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed sending client task to cleaner queue\n",
                                   NERR_SEVERE, __FILE__, __LINE__);
                    retval = NU_INVAL;
                }

                /* At least one task was sent to cleaner queue */
                taskTerminated = NU_TRUE;
            }
        } /* End For loop */
    }

    /* If no client tasks are active
     *   or
     * if an total shutdown found no client tasks to terminate,
     * call Cleanup to free all resources.
     */
    if ( (FST_Active_Clients == 0) ||
         ( (flags & FTP_SERVER_STOP_ALL) &&
           (taskTerminated == NU_FALSE) ) )
        FST_Cleanup((TQ_EVENT)NU_NULL, 0, 0);

    NU_USER_MODE();

    return (retval);
} /* NU_FTP_Server_Uninit */

/************************************************************************
*
*   FUNCTION
*
*       FST_Cleanup
*
*   DESCRIPTION
*
*       Cleans up resources after the Cleaner Task has executed.
*
*   INPUTS
*
*       event                   The event (if any) that called Cleanup
*       dat0                    unused
*       dat1                    unused
*
*   OUTPUTS
*
*       none
*
************************************************************************/
VOID FST_Cleanup(TQ_EVENT event, UNSIGNED dat0, UNSIGNED dat1)
{
    STATUS status;
    FST_Total_Conn = 0;

    FTP_UNUSED_PARAMETER(event);
    FTP_UNUSED_PARAMETER(dat0);
    FTP_UNUSED_PARAMETER(dat1);

    /* All Client Tasks have been completed. Terminate Cleaner Task */
    status = NU_Terminate_Task(&FST_Cleaner_Task);

    if (status != NU_SUCCESS)
        NLOG_Error_Log("Failed to terminate Cleaner Task\n",
                       NERR_SEVERE, __FILE__, __LINE__);

    /* Delete Cleaner Task */
    status = NU_Delete_Task(&FST_Cleaner_Task);

    if (status != NU_SUCCESS)
        NLOG_Error_Log("Failed sending delete Cleaner Task\n",
                       NERR_SEVERE, __FILE__, __LINE__);

    /* Deallocate Cleaner Task Memory */
    status = NU_Deallocate_Memory(FST_Cleaner_Task_Mem);

    if (status != NU_SUCCESS)
        NLOG_Error_Log("Failed to free Cleaner Task memory\n",
                       NERR_SEVERE, __FILE__, __LINE__);

    /* Delete Cleaner Queue */
    status = NU_Delete_Queue(&FST_Cleaner_Queue);

    if (status != NU_SUCCESS)
        NLOG_Error_Log("Failed to delete Cleaner Queue\n",
                       NERR_SEVERE, __FILE__, __LINE__);

    /* Deallocate Cleaner Queue Memory */
    status = NU_Deallocate_Memory(FST_Cleaner_Queue_Mem);

    if (status != NU_SUCCESS)
        NLOG_Error_Log("Failed to free Cleaner Queue\n",
                       NERR_SEVERE, __FILE__, __LINE__);

    /* Delete Master Lock Semaphore */
    status = NU_Delete_Semaphore(&FST_Master_Lock);

    if (status != NU_SUCCESS)
        NLOG_Error_Log("Failed to delete Master Lock semaphore\n",
                       NERR_SEVERE, __FILE__, __LINE__);

} /* FST_Cleanup */
