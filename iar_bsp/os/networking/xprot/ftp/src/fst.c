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
*       fst.c                                          
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Server
*
*   DESCRIPTION
*
*       The Ftp Server Task collection of functions provide APIs for
*       starting the ftp server. The NU_FTP_Server_Init routine is
*       responsible for defining the initial application environment.
*       Along with creating the needed queues and semaphore, a master task,
*       to handle establishing the control connection for each FTP client,
*       is created.  A cleaner task is also created to eliminate each
*       client's data and control tasks when it is completed.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_FTP_Server_Init
*       Master_Task
*       Cleaner_Task
*       Control_Task
*       FTP_Printf
*       FST_Client_Connect
*       FST_Data_Task_Entry
*       nu_os_net_prot_ftp_init
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_net.h
*       nu_ftp.h
*       ftps_ext.h
*       fsp_extr.h
*       fc_extr.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/nu_net.h"
#include "networking/nu_ftp.h"
#include "networking/ftps_ext.h"
#include "networking/fsp_extr.h"
#include "networking/fc_extr.h"
#ifdef NET_5_1
#include "networking/um_defs.h"
#endif

/* Define Application data structures.  */
NU_SEMAPHORE    FST_Master_Lock;
NU_TASK         FST_Master_Task;
NU_TASK         FST_Cleaner_Task;
NU_QUEUE        FST_Cleaner_Queue;
NU_MEMORY_POOL  *FTPS_Memory;
VOID            *FST_Master_Task_Mem;
VOID            *FST_Cleaner_Task_Mem;
VOID            *FST_Cleaner_Queue_Mem;
UINT16          FST_Active_Clients = 0;
UNSIGNED_CHAR   FST_Total_Conn;
TQ_EVENT        FST_Cleanup_Event;
UINT8           FST_Master_Task_Terminated = NU_TRUE;
NU_PROTECT      FST_Active_List_Protect, FST_Master_Task_Terminated_Protect;
FST_ACTIVE_LIST FST_Active_List[FTP_SERVER_MAX_CONNECTIONS];
INT             FST_Master_Socketd;
#ifdef NET_5_1
extern FTPSACCT     FTP_Password_List[];
#endif

/* String to store registry path for ftps */
CHAR                FTPS_Registry_Path[REG_MAX_KEY_LENGTH] = {0};

/* Prototype for FTPS_Wait_For_FS() */
STATIC STATUS FTPS_Wait_For_FS(CHAR *mount_point, UNSIGNED timeout);

/* this variable is used in FTP_UNUSED_PARAMETER macro
   defined in Fc_defs.h.  The macro is used to remove
   compilation warnings */
UINT32 FTP_Unused_Parameter;

/************************************************************************
* FUNCTION
*
*       nu_os_net_prot_ftp_init
*
* DESCRIPTION
*
*       This function initializes the Nucleus XPROT FTP Server
*       modules. It is the entry point of the product initialization
*       sequence.
*
* INPUTS
*
*       *path                   Path to the configuration settings.
*       startstop               Whether product is being started or
*                               being stopped.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of successful initialization.
*       NU_NO_MEMORY            Memory not available.
*       FTPS_ALREADY_RUNNING    FTPS already initialized.
*       FTPS_INVALID_PARAMS     Invalid parameter(s).
*       <other> -               Indicates (other) internal error occured.
*
************************************************************************/
STATUS nu_os_net_prot_ftp_init(CHAR *path, INT startstop)
{
    STATUS              status = -1;
#ifdef NET_5_1
    UINT8               i = 0;
#endif

    if(path != NU_NULL)
    {
        /* Save a copy locally. */
        strcpy(FTPS_Registry_Path, path);
    }

    if(startstop)
    {
        /* Initialize the Nucleus ftp server component. */
        status = NU_FTP_Server_Init();

        if(status == NU_SUCCESS)
        {
#ifdef NET_5_1
            /* Initialize the list of FTP users */
            while (FTP_Password_List[i].id[0] != '\0')
            {
                status = UM_Add_User(FTP_Password_List[i].id, FTP_Password_List[i].pw,
                            UM_FTP, 0);
                i++;
            }
#endif /* NET_5_1 */
        }
        else
        {
            NLOG_Error_Log("Error at call to NU_FTP_Server_Init().\n",
                        NERR_FATAL, __FILE__, __LINE__);
        }
    }
    else
    {
        /* Stop requested, for now nothing to do */
        status = NU_SUCCESS;
    }

    return (status);
} /* nu_os_net_prot_ftp_init */


/************************************************************************
*
*   FUNCTION
*
*       NU_FTP_Server_Init
*
*   DESCRIPTION
*
*       Initializes the Nucleus FTP Server by creating all necessary
*       tasks
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS              The FTP Server was successfully
*                               initialized.
*       NU_MEM_ALLOC            Insufficient memory to initialize
*                               all resources.
*       NU_INVAL                A general-purpose error condition. This
*                               generally indicates that a required
*                               resource (task, semaphore, etc.) could not
*                               be created.
*
************************************************************************/
INT32 NU_FTP_Server_Init(VOID)
{
    STATUS          status;
    static UINT8    firstRun = 0;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

#ifdef NET_5_1
    FTPS_Memory = MEM_Cached;
#else
    FTPS_Memory = &System_Memory;
#endif

    /* Check if Master Task Protect structure needs to be zeroed out */
    if (firstRun == 0)
    {
        UTL_Zero(&FST_Master_Task_Terminated_Protect, sizeof(NU_PROTECT));
        firstRun = 1;
    }

    NU_Protect(&FST_Master_Task_Terminated_Protect);

    status = FST_Master_Task_Terminated;

    NU_Unprotect();

    /* Check to see if Master Task is active */
    if (status == NU_FALSE)
    {
        NU_USER_MODE();
        return(NU_INVAL);
    }

    /* If clients are active, then this is a re-initialization. */
    if (FST_Active_Clients == 0)
    {    
        /***** Create ftp connection synchronization semaphore *****/
        status = NU_Create_Semaphore(&FST_Master_Lock, "MSTRLOCK",
                                     FTP_SERVER_MAX_CONNECTIONS, NU_FIFO);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("***** Unable to create Master Lock Semaphore *****\r\n");
            NU_USER_MODE();

            return (status);
        }

        /* Create Cleaner_Task communication queue */
        status = NU_Allocate_Memory(FTPS_Memory, &FST_Cleaner_Queue_Mem,
                                    (CLEANER_QUEUE_SIZE * sizeof(UNSIGNED)),
                                    NU_NO_SUSPEND);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("***** Unable to Allocate Cleaner Queue Memory *****\r\n");
            NU_USER_MODE();

            return (NU_MEM_ALLOC);
        }

        status = NU_Create_Queue(&FST_Cleaner_Queue, "CLNRQUE", FST_Cleaner_Queue_Mem,
                                 CLEANER_QUEUE_SIZE, NU_FIXED_SIZE, 1, NU_FIFO);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("***** Unable to create FST_Cleaner_Queue *****\r\n");
            NU_USER_MODE();

            return (status);
        }
    }
    
    /***** Create each FTP Server task in the system. *****/

    /***** Create FTP server Master_Task. *****/
    status = NU_Allocate_Memory(FTPS_Memory, &FST_Master_Task_Mem,
                                FTPS_MASTER_STACK_SIZE, NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        FTP_Printf("***** Unable to Allocate Master Task Memory *****\r\n");
        NU_USER_MODE();

        return (NU_MEM_ALLOC);
    }

    status = NU_Create_Task(&FST_Master_Task, "FTPSERVR", Master_Task, 0,
                            NU_NULL, FST_Master_Task_Mem, FTPS_MASTER_STACK_SIZE,
                            FTPS_MASTER_PRIORITY, 0, NU_PREEMPT, NU_NO_START);

    if (status != NU_SUCCESS)
    {
        FTP_Printf("***** Unable to create FST_Master_Task *****\r\n");
        NU_USER_MODE();

        return (status);
    }

    NU_Resume_Task(&FST_Master_Task);

    /* If clients are active, then this is a re-initialization. */
    if (FST_Active_Clients == 0)
    {
        FST_Total_Conn = 0;
        UTL_Zero(&FST_Active_List_Protect, sizeof(NU_PROTECT));

        /***** Create FTP server Cleaner_Task. *****/
        status = NU_Allocate_Memory(FTPS_Memory, &FST_Cleaner_Task_Mem,
                                    FTPS_CLEANER_STACK_SIZE, NU_NO_SUSPEND);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("***** Unable to Allocate Cleaner Task Memory *****\r\n");
            NU_USER_MODE();

            return (NU_MEM_ALLOC);
        }

        status = NU_Create_Task(&FST_Cleaner_Task, "CLEANER", Cleaner_Task, 0,
                                NU_NULL, FST_Cleaner_Task_Mem, FTPS_CLEANER_STACK_SIZE,
                                FTPS_MASTER_PRIORITY, 0, NU_PREEMPT, NU_NO_START);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("***** Unable to create FTP server Cleaner_Task *****\r\n");
            NU_USER_MODE();

            return (status);
        }

        NU_Resume_Task(&FST_Cleaner_Task);
    }

    NU_Protect(&FST_Master_Task_Terminated_Protect);

    /* Master task is active */
    FST_Master_Task_Terminated = NU_FALSE;

    NU_Unprotect();

    NU_USER_MODE();

    return (NU_SUCCESS);

} /* NU_FTP_Server_Init */

/******************************************************************************
*
*   FUNCTION
*
*      FTPS_Wait_For_FS
*
*   DESCRIPTION
*
*      Wait for a given drive to be initialized.
*
*   INPUTS
*
*      mount_point            Mount point to wait for
*      timeout                Timeout value, NU_SUSPEND or NU_NO_SUSPEND
*
*   OUTPUTS
*
*      NU_SUCCESS             File System has been successfully initialized
*         < 0                 File System initialization error
*
*******************************************************************************/
STATIC STATUS FTPS_Wait_For_FS(CHAR *mount_point, UNSIGNED timeout)
{
    STATUS          status;

    /* Suspend until a File device is mounted */
    status = NU_Storage_Device_Wait(mount_point, timeout);

    return (status);
}

/******************************************************************************
*
*   FUNCTION
*
*       Master_Task
*
*   DESCRIPTION
*
*      This function provides the entry point for the master control task
*      of the FTP client.  This function is responsible for building the
*      necessary structures and making the appropriate calls to set
*      up the FTP socket.  The function then waits for a connection.  Once a
*      connection is established, and verified as a proper FTP connection,
*      the server creates an instance of Control_Task, Passing the socket
*      descriptor for the connection via function parameter.  The function
*      then resumes waiting for a connection, repeating the above sequence
*      indefinitely.
*
*   INPUTS
*
*       argc                    unused
*       argv                    unused
*
*   OUTPUTS
*
*       None
*
*******************************************************************************/
VOID Master_Task(UNSIGNED argc, VOID *argv)
{
    /* Define general variables.  */
    INT             shutdown = 0;
    STATUS          status;
    INT             temp;
    INT16           family;
    FSP_CB          fsp_cb;
    struct          addr_struct clientAddr;
    struct          addr_struct serverAddr;
    UNSIGNED        *myAddr;
    VOID            *pointer;
    NU_TASK         *newTask;
    CHAR            taskName[8] = "FTP";
    CHAR            nu_drive[3];


#ifdef FILE_3_1
    /* extern  INT Default_Drive; */
#endif

#if (NU_ENABLE_NOTIFICATION == NU_TRUE)
    NET_NTFY_Debug_Struct ntfy_struct;
#endif

    NU_SUPERV_USER_VARIABLES

#if ( (defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE) )
    family = NU_FAMILY_IP6;
#else
    family = NU_FAMILY_IP;
#endif

    NU_SUPERVISOR_MODE();

    FTP_UNUSED_PARAMETER(argc);
    FTP_UNUSED_PARAMETER(argv);

#if (NU_ENABLE_NOTIFICATION == NU_TRUE)
    ntfy_struct.net_ntfy_type = NET_NTFY_GENERAL;
#endif

    nu_drive[0] = (CHAR)((FTPS_DEFAULT_DRIVE + 1) + 'A' - 1);
    nu_drive[1] = ':';
    nu_drive[2] = '\0';    

    /* Check to see if the File System has been initialized and the
     * Default drive is available.
     */
    status = FTPS_Wait_For_FS(nu_drive, FTPS_INIT_TIMEOUT);

    if(status != NU_SUCCESS)
    {
        FTP_Printf("The File System could not initialized successfully.\r\n");

        /* Change to User mode before returning */
        NU_USER_MODE();
        return;
    }
    else
    {
        status = NU_Become_File_User();

        if (status == NU_SUCCESS)
        {
            /* Open disk for accessing */
            status = NU_Open_Disk (nu_drive);

            /* If the disk is already open, return success and leave the current
             * directory where it is.
             */
            if (status == NUF_NO_ACTION)
                status = NU_SUCCESS;

            if (status == NU_SUCCESS)
            {
                /* Set the default drive */
                status = NU_Set_Default_Drive(FTPS_DEFAULT_DRIVE);
            }
            else
            {
                FTP_Printf("Task cannot open disk as Nucleus FILE User\r\n");
            }
        }
        else
        {
            FTP_Printf("Task cannot register as Nucleus FILE User\r\n");

            /* Change to User mode before returning */
            NU_USER_MODE();
            return;
        }

        if (status != NU_SUCCESS)
        {
            /* Change to User mode before returning */
            NU_USER_MODE();
            return;
        }
    }

    /*  Set Current Directory to "\" */
    status = NU_Set_Current_Dir("\\");
    if (status != NU_SUCCESS)
    {
        FTP_Printf("Task cannot set current directory as Nucleus FILE User\r\n");

        NU_USER_MODE();
        return;
    }

    fsp_cb.fsp_drive = NU_Get_Default_Drive();

    /* Print Banner */
    FTP_Printf("Nucleus NET\r\n");
    FTP_Printf("TCP/IP FTP Server\r\n");

    NU_Protect(&FST_Active_List_Protect);

    /* Clear active tasks list if no clients are active */
    if (FST_Active_Clients == 0)
    {
        for (temp = 0; temp < FTP_SERVER_MAX_CONNECTIONS; temp++)
        {
            FST_Active_List[temp].active_task = NU_NULL;
            FST_Active_List[temp].active_sckt = -1;
        }
    }

    NU_Unprotect();

    /*
    ********************
    Establish TCP communications:
    1) Create a socket (NU_Socket)
    2) Bind the Server port and IP address (NU_Bind)
    3) Perform a listen call (NU_Listen)
    4) Wait for client connections (NU_Accept)
    5) Return control after connection has been established
    ********************
    */

    /* Open a connection via socket interface.  NU_Socket is
       responsible for establishing a new socket descriptor and
       defining the type of communication protocol to be used. */
    FST_Master_Socketd = NU_Socket(family, NU_TYPE_STREAM, NU_NONE);

    if (FST_Master_Socketd >= 0)
    {
        /* Fill in a structure with the server address */
        serverAddr.family = family;
        serverAddr.port = FTPS_SERVER_DEFAULT_PORT; /* should be 21 */

        /* Return the local machine's 32-bit IP number upon request */
        myAddr = (UNSIGNED*)serverAddr.id.is_ip_addrs;

#ifdef NET_5_1
        memset(myAddr, 0, MAX_ADDRESS_SIZE);
#else
        memset(myAddr, 0, IP_ADDR_LEN);
#endif

        /* Set the block flag in socket structure */
        NU_Fcntl(FST_Master_Socketd, NU_SETFLAG, NU_BLOCK);

        /* Allow multiple addresses to bind to the same port */
        NU_Setsockopt_SO_REUSEADDR(FST_Master_Socketd, 1);

        /* Make an NU_Bind() call to bind the server's address */
        if (NU_Bind(FST_Master_Socketd, &serverAddr, 0) >= 0)
        {
            /*be ready to accept connection requests */
            status = NU_Listen(FST_Master_Socketd, FTPS_SERVER_MAX_PENDING);

            if (status == NU_SUCCESS)
            {
                while (!shutdown)
                {
                    status = NU_Obtain_Semaphore(&FST_Master_Lock, NU_SUSPEND);

                    if (status == NU_SUCCESS)
                    {
                        fsp_cb.fsp_socketd = NU_Accept(FST_Master_Socketd, &clientAddr, 0);

                        FTP_Printf("Client has connected.\r\n");

                        if (fsp_cb.fsp_socketd >= 0)
                        {
                            status = NU_Allocate_Memory(FTPS_Memory,
                                                        (VOID **)&newTask,
                                                        sizeof(NU_TASK),
                                                        NU_NO_SUSPEND);

                            if (status == NU_SUCCESS)
                            {
                                UTL_Zero(newTask, sizeof(NU_TASK));

                                /* 'Build' task name based on total_connections
                                   variable. */
                                NU_ITOA((INT)FST_Total_Conn, &taskName[3],10);

                                /***** Create FTP server Control_Task.  *****/
                                status = NU_Allocate_Memory(FTPS_Memory,
                                                            &pointer,
                                                            FTPS_CONTROL_STACK_SIZE,
                                                            NU_NO_SUSPEND);

                                if (status == NU_SUCCESS)
                                {
                                    fsp_cb.fsp_family = clientAddr.family;

                                    status = NU_Create_Task(newTask,
                                                            taskName,
                                                            Control_Task,
                                                            (UNSIGNED)0,
                                                            (VOID *)&fsp_cb,
                                                            pointer,
                                                            FTPS_CONTROL_STACK_SIZE,
                                                            FTPS_CONTROL_PRIORITY,
                                                            0, NU_PREEMPT,
                                                            NU_NO_START);

                                    if (status == NU_SUCCESS)
                                    {   /* New task created successfully */
                                        NU_Protect(&FST_Active_List_Protect);

                                        fsp_cb.fsp_currentID = newTask;
                                        FST_Total_Conn++;       /* Increment total connections */
                                        FST_Active_Clients++;   /* Increment active clients */

                                        /* Add created task to active tasks list */
                                        for (temp = 0; temp < FTP_SERVER_MAX_CONNECTIONS; temp++)
                                        {
                                            /* Place new task id and socket in active list */
                                            if (FST_Active_List[temp].active_task == NU_NULL)
                                            {
                                                FST_Active_List[temp].active_task = fsp_cb.fsp_currentID;
                                                FST_Active_List[temp].active_sckt = fsp_cb.fsp_socketd;
                                                break;
                                            }
                                        }

                                        NU_Unprotect();
                                        NU_Resume_Task(newTask);
                                    }

                                    else /* Control_Task Create failure */
                                    {
                                        FTP_Printf("***** Unable to create FTP server Control_Task *****\r\n");
                                        shutdown++;
#if (NU_ENABLE_NOTIFICATION == NU_TRUE)
                                        NET_Notify(FTPS_CTASK_CREATE_FAILURE, __FILE__, __LINE__,
                                                   NU_NULL, &ntfy_struct);
#endif
                                        NU_Close_Socket((INT16)fsp_cb.fsp_socketd);
                                        NU_Deallocate_Memory((VOID *)newTask);
                                        NU_Deallocate_Memory(pointer);
                                    }
                                }

                                else /* Memory allocation failure for Control_Task */
                                {
                                    shutdown++;
#if (NU_ENABLE_NOTIFICATION == NU_TRUE)
                                    NET_Notify(FTPS_CTASK_MALLOC_FAILURE, __FILE__, __LINE__,
                                               NU_NULL, &ntfy_struct);
#endif
                                    NU_Close_Socket((INT16)fsp_cb.fsp_socketd);
                                    NU_Deallocate_Memory((VOID *)newTask);
                                }
                            }

                            else /* Failed to allocate Control_Task structure memory */
                            {
                                shutdown++;
#if (NU_ENABLE_NOTIFICATION == NU_TRUE)
                                NET_Notify(FTPS_CTASK_STRU_MALLOC_FAILURE, __FILE__, __LINE__,
                                           NU_NULL, &ntfy_struct);
#endif
                                NU_Close_Socket((INT16)fsp_cb.fsp_socketd);
                            }
                        } /* NU_Accept() */
                        else
                        {
                            shutdown++;
#if (NU_ENABLE_NOTIFICATION == NU_TRUE)
                            NET_Notify(FTPS_SERVER_TERMINATED, __FILE__, __LINE__,
                                       NU_NULL, &ntfy_struct);
#endif
                        }
                    } /* NU_Obtain_Semaphore() */
                } /* while-loop */
            } /* NU_Listen() */
        } /* NU_Bind() */

        NU_Close_Socket((INT16)FST_Master_Socketd);

    } /* End of NU_Socket() */

    /*  Close Disk */
    nu_drive[0] = (CHAR)(((CHAR) FTPS_DEFAULT_DRIVE + 1) + 'A' - 1);
    nu_drive[1] = ':';
    nu_drive[2] = '\0';    

    NU_Close_Disk (nu_drive);


    NU_Release_File_User();


    NU_USER_MODE();

} /* Master_Task */

/******************************************************************************
*
*   FUNCTION
*
*       Cleaner_Task
*
*   DESCRIPTION
*
*      This function provides the entry point for the task cleanup task of
*      the FTP Server.  This function is responsible for dismissing completed
*      control and data tasks, and deallocating the resources associated with
*      those completed tasks.  The function waits for task ID's over a Nucleus
*      PLUS queue, performing verification and task termination before
*      returning to wait for the queue.
*
*   INPUTS
*
*       argc                    unused
*       argv                    unused
*
*   OUTPUTS
*
*       None
*
******************************************************************************/
VOID Cleaner_Task(UNSIGNED argc, VOID *argv)
{
    STATUS          status;
    UNSIGNED        scratch;
    INT             temp;

    /* variables for NU_Task_Information () */
    NU_TASK         *taskToClean;
    CHAR            taskName[8];
    DATA_ELEMENT    taskStatus;
    UNSIGNED        taskSched;
    OPTION          taskPriority;
    OPTION          taskPreempt;
    UNSIGNED        taskTimeSlice;
    VOID            *taskStack;
    UNSIGNED        taskStackSize;
    UNSIGNED        taskMinStack;

    FTP_UNUSED_PARAMETER(argc);
    FTP_UNUSED_PARAMETER(argv);

    FTP_Printf("Cleaner_Task activated.\r\n");

    /* Register the event to handle cleanup of the Cleaner Task */
    if (EQ_Register_Event(FST_Cleanup, &FST_Cleanup_Event) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register Cleaner Task Cleanup event",
                        NERR_SEVERE, __FILE__, __LINE__);

        return; /* Terminate task */
    }

    for (;;)
    {
        /*  Wait on input queue for task pointers */
        status = NU_Receive_From_Queue(&FST_Cleaner_Queue,
                                       (VOID *)&taskToClean, 1,
                                       &scratch, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            FTP_Printf("Cleaner_Task activated.\r\n");

            status = NU_Task_Information(taskToClean, taskName, &taskStatus,
                                         &taskSched, &taskPriority,
                                         &taskPreempt, &taskTimeSlice,
                                         &taskStack, &taskStackSize,
                                         &taskMinStack);

            if (status == NU_SUCCESS)
            {
                status = NU_Terminate_Task(taskToClean);

                if (status == NU_SUCCESS)
                {
                    status = NU_Delete_Task(taskToClean);

                    if (status == NU_SUCCESS)
                    {
                        status = NU_Deallocate_Memory((VOID*)taskToClean);

                        if (status == NU_SUCCESS)
                        {
                            status = NU_Deallocate_Memory(taskStack);

                            if (status == NU_SUCCESS)
                            {
                                if (taskName[0] == 'F')
                                {   /* This was a control conn. */
                                    NU_Release_Semaphore(&FST_Master_Lock);
                                }
                                else
                                {   /* No control connection found */
                                    FTP_Printf("Data_XXX_Task deleted from system.\r\n");
                                }

                                NU_Protect(&FST_Active_List_Protect);

                                FST_Active_Clients--;

                                /* Remove deleted task from active tasks list */
                                for (temp = 0; temp < FTP_SERVER_MAX_CONNECTIONS; temp++)
                                {
                                    if (FST_Active_List[temp].active_task == taskToClean)
                                    {
                                        FST_Active_List[temp].active_task = NU_NULL;
                                        FST_Active_List[temp].active_sckt = -1;
                                        break;
                                    }
                                }

                                NU_Unprotect();

                                /* If Terminate is in progress, break out of loop */
                                if ( (FST_Active_Clients == 0) &&
                                     (FST_Master_Task_Mem == NU_NULL) )
                                {
                                    /* Master Task terminated, Urgency = NU_FALSE */
                                    EQ_Put_Event(FST_Cleanup_Event, 0, 0);
                                    break;
                                }
                            }
                            else/* NU_Deallocate_Memory(taskStack) */
                            {
                                FTP_Printf("Unable to deallocate task stack memory.\r\n");
                            }
                        }
                        else/* NU_Deallocate_Memory((VOID*)taskToClean) failed */
                        {
                            FTP_Printf("Unable to deallocate pointer to task.\r\n");
                        }
                    }
                    else/* NU_Delete_Task() failed */
                    {
                        FTP_Printf("Unable to delete task.\r\n");
                    }
                }
                else/* NU_Terminate_Task() failed */
                {
                    FTP_Printf("Unable to terminate the task.\r\n");
                }
            }
            else/* NU_Task_Information() failed */
            {
                FTP_Printf("Failed to get task information.\r\n");
            }
        }
        else/* NU_Receive_From_Queue() failed */
        {
            FTP_Printf("Error from NU_Receive_From_Queue().\r\n");
        }
    }

} /* Cleaner_Task */

/******************************************************************************
*
*   FUNCTION
*
*       Control_Task
*
*   DESCRIPTION
*
*       This function provides the entry point for an instance of an
*       FTP control connection.  The function receives the appropriate
*       socket descriptor via parameter.  The function immediately begins
*       waiting on data over the socket, parsing FTP commands, performing
*       the appropriate actions for each command, and sending out FTP reply
*       messages in response.  If necessary, the function establishes the data
*       connection, passing the file name via parameters to a data task which
*       it creates.  The function continues to wait for and process data from
*       the socket until an FTP QUIT command is received or the timeout value
*       (which is reset to zero for each command received) is reached. The
*       function then sends a message to the cleaner task via a Nucleus PLUS
*       queue and self suspends.
*
*   INPUTS
*
*       argc                    unused
*       control_block           pointer to task control block
*
*   OUTPUTS
*
*       None
*
******************************************************************************/
VOID Control_Task(UNSIGNED argc, VOID *control_block)
{
    STATUS      status;
    INT32       bytesReceived;
    INT         parseIndex;
    INT         out = 0;
    CHAR        *buffer;
    CHAR        commandBuf[FTPS_COMMAND_BUFFER_SIZE];
    FTP_SERVER  server;
    NU_TASK     *pointerToThisTask;
    INT         value;
    CHAR        *pointer;
    FSP_CB      *fsp_cb;
    MNT_LIST_S  *mount_list;
    MNT_LIST_S  *mount_entry;
    INT         drive;
    CHAR        nu_drive[3];


    NU_SUPERV_USER_VARIABLES

    UNUSED_PARAMETER(argc);

    NU_SUPERVISOR_MODE();
    
    /* set the server structure to zero */
    UTL_Zero(&server, sizeof(FTP_SERVER));
    
    /* the following allocations of memory for buffers keep these structures off of the stack */
    /* allocate memory for buffer */
    status = NU_Allocate_Memory(&System_Memory, (VOID **)&buffer,
                                FTPS_GENERIC_BUFF_SIZE, NU_NO_SUSPEND);

    /* buffer allocation failed */                            
    if (status == NU_SUCCESS)
    {
        /* allocate memory for replyBuff in the server structure */ 
        status = NU_Allocate_Memory(&System_Memory,(VOID **)&server.replyBuff,
                                    FTPS_REPLY_BUFFER_SIZE, NU_NO_SUSPEND);
        
        /* replyBuff allocation failed */                            
        if (status == NU_SUCCESS)
        {
            /* allocate memory for fileSpec in the server structure */
            status = NU_Allocate_Memory(&System_Memory, (VOID **)&server.fileSpec,
                                        FTPS_GENERIC_BUFF_SIZE, NU_NO_SUSPEND);
            
            /* fileSpec allocation failed */
            if (status == NU_SUCCESS)
            {
                /* allocate memory for path in the server structure */
                status = NU_Allocate_Memory(&System_Memory, (VOID **)&server.path,
                                            FTPS_GENERIC_BUFF_SIZE, NU_NO_SUSPEND);
                
                /* path allocation failed */
                if (status == NU_SUCCESS)
                {
                    /* allocate memory for renamePath in the server structure */
                    status = NU_Allocate_Memory(&System_Memory, (VOID **)&server.renamePath,
                                                FTPS_GENERIC_BUFF_SIZE, NU_NO_SUSPEND);
                    
                    /* renamePath allocation failed */
                    if (status == NU_SUCCESS)
                    {
                        /* allocate memory for currentWorkingDir in the server structure */
                        status = NU_Allocate_Memory(&System_Memory, 
                                                    (VOID **)&server.currentWorkingDir,
                                                    FTPS_GENERIC_BUFF_SIZE, NU_NO_SUSPEND);
                    }
                }
            }
        }    
    }      
    /* If the memory allocations were all successful. */
    if (status == NU_SUCCESS)
    {       
        /***** Create a FTP File write synchronization semaphore *****/
        status = NU_Create_Semaphore(&(server.fileWriteLock), "FSTWRITE",
                                     1, NU_FIFO);

        if (status != NU_SUCCESS)
        {
            FTP_Printf("***** Unable to create FTP File Write Semaphore *****\r\n");
            server.lastError = status;
        }
        else
        {
            /* Get a pointer to the FTP Server Control Block */
            fsp_cb = (FSP_CB*)control_block;


            status = FTPS_Server_Session_Init(&server, (UNSIGNED)(fsp_cb->fsp_socketd),
                                              fsp_cb->fsp_drive,
                                              fsp_cb->fsp_family);
        }
    
        if (status != NU_SUCCESS)
        {
            FTP_Printf("Session_Init() has failed.\r\n");
        }
        else
        {
            nu_drive[0] = (CHAR)((FTPS_DEFAULT_DRIVE + 1) + 'A' - 1);
            nu_drive[1] = ':';
            nu_drive[2] = '\0';    
            
            /* If we get here, file system must have been initialized and so
             * wait for sometime for the required drive to be available.
             */
            status = FTPS_Wait_For_FS(nu_drive, FTPS_INACT_TIMEOUT);
            
            if(status == NU_SUCCESS)
            {
                status = NU_Become_File_User();
                if (status == NU_SUCCESS)
                {
                    /* Proceed to access the Disk */

                    /* Obtain a pointer to all the disks that have been
                       mounted on the system */
                    status = NU_List_Mount(&mount_list);

                    if (status != NU_SUCCESS)
                    {
                        FTP_Printf("Control_Task failed to get a list of disks.\r\n");

                        /* skip control loop */
                        out++;
                    }
                    else
                    {
                        /* Store a copy of the entry point */
                        mount_entry = mount_list;

                        /* Traverse through the entire mount list to open all
                           the disks */
                        while (mount_entry)
                        {
                            /* Convert Drive letter to upper case and then drive number. */
                            drive = (mount_entry->mnt_name[0] & 0xDF) - 'A';

                            /* Open disk for accessing */
                            nu_drive[0] = (CHAR)(((CHAR) drive + 1) + 'A' - 1);
                            nu_drive[1] = ':';
                            nu_drive[2] = '\0';

                            status = NU_Open_Disk (nu_drive);

                            /* If the disk is already open, return success and leave the current
                             * directory where it is.
                             */
                            if (status == NUF_NO_ACTION)
                               status = NU_SUCCESS;

                            if (status != NU_SUCCESS)
                            {
                                FTP_Printf("Control_Task cannot open disk.\r\n");
                                out++;/* skip control loop */
                            }

                            /* Get the next mount item */
                            mount_entry = mount_entry -> next;
                        }

                        /* Free the mount list */
                        status = NU_Free_List((VOID**)&mount_list);

                        if (status != NU_SUCCESS)
                        {
                            FTP_Printf("Control_Task: Failed to free the File devices mount list.\r\n");
                            out++;/* skip control loop */
                        }
                    }

                    status = NU_Set_Default_Drive(server.defaultDrive);

                    if (status != NU_SUCCESS)
                    {
                        FTP_Printf("Control_Task cannot select current drive.\r\n");
                        out++;/* skip control loop */
                    }

                    status = NU_Set_Current_Dir(server.currentWorkingDir);

                    if (status != NU_SUCCESS)
                    {
                        FTP_Printf("Control_Task cannot select current working directory.\r\n");
                        out++;/* skip control loop */
                    }

                    /* Start Control Loop until QUIT or timeout */
                    while (!out)
                    {
                        /*  Go get the client's next command.  */
                        bytesReceived = FSP_Command_Read(&server, buffer,
                                                         FTPS_GENERIC_BUFF_SIZE,
                                                         FTPS_INACT_TIMEOUT);

                        if (bytesReceived < 0)
                        {
                            /* Error in call to FSP_Command_Read(). */
                            if (bytesReceived == FTP_TIMEOUT)
                            {
                                /* The connection timed out. Check to see if transfer
                                   is in progress before closing the connection.
                                   server.lastError is already set. */
                                if (!server.transferStatus)
                                    out++;

                                continue;
                            }

                            else
                            {
                                /* Error in call to FSP_Command_Read(). */
                                server.lastError = FTP_STACK_ERROR;
                                server.stackError = (INT)bytesReceived;
                                out++;
                            }
                        }

                        else /* FSP_Command_Read successful */
                        {
                            /* parse buffer for next FTP command */
                            parseIndex = 0;

                            while ( (buffer[parseIndex]!= ' ') &&
                                    (buffer[parseIndex]!= '\r') )
                            {
                                commandBuf[parseIndex]= buffer[parseIndex];
                                parseIndex++;
                            }

                            commandBuf[parseIndex]= '\0'; /* complete command string */

                            /****  PRE-LOGIN COMMANDS  ****/

                            if (Str_Equal((UINT8 *)commandBuf,"USER"))
                                status = FTPS_UserInit(&server);

                            else if (Str_Equal((UINT8 *)commandBuf,"PASS"))
                                status = FSP_Server_PASS(&server);

                            else if (Str_Equal((UINT8 *)commandBuf,"QUIT"))
                            {
                                status = FSP_Server_QUIT(&server);
                                out++;
                            }
                            /* User must be logged in to get beyond this point */
                            else if ( !(server.cmdFTP.userFlag
                                        && server.cmdFTP.passFlag))
                            {
                                /* Send 'Not logged in' message */
                                status = (INT)NU_Send((INT)server.socketd, MSG530,
                                                 SIZE530, 0);

                                if (status < 0)
                                {
                                    FTP_Printf("Control_Task() NU_Send failure.\r\n");
                                    server.lastError = FTP_STACK_ERROR;
                                    server.stackError = status;
                                }

                                status = FTP_INVALID_USER;
                            }
                            /**** Check for out of sequence commands ****/
                            /* Check RNFR & RNTO states */
                            else if ( (!(Str_Equal((UINT8 *)commandBuf,"RNTO"))) &&
                                      (server.cmdFTP.rnfrFlag) )
                            {
                                /* Send 'Bad sequence of commands' message */
                                status = (INT)NU_Send((INT)server.socketd, MSG503,
                                                 SIZE503, 0);

                                if (status < 0)
                                {
                                    FTP_Printf("Control_Task() NU_Send failure.\r\n");
                                    server.lastError = FTP_STACK_ERROR;
                                    server.stackError = status;
                                }

                                server.cmdFTP.rnfrFlag = 0;
                            }
                            else
                            {
                                pointer = commandBuf;
                                value = 0;

                                while (*pointer)
                                {
                                    value += (INT) *pointer;
                                    pointer++;
                                }

                                /****  ACCESS CONTROL COMMANDS ****/
                                switch (value)
                                {
                                case ACCT:
                                    if (Str_Equal((UINT8 *)commandBuf,"ACCT"))
                                        status = FSP_Server_ACCT(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case CWD:
                                    if (Str_Equal((UINT8 *)commandBuf,"CWD"))
                                        status = FSP_Server_CWD(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case CDUP:
                                    if (Str_Equal((UINT8 *)commandBuf,"CDUP"))
                                    {
                                        strcpy(server.replyBuff, "CWD ..\r\n");
                                        status = FSP_Server_CWD(&server);
                                    }
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case XCWD:
                                    if (Str_Equal((UINT8 *)commandBuf,"XCWD"))
                                        status = FSP_Server_XCWD(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                    /**** TRANSFER PARAMETER COMMANDS ****/
                                case PORT:
                                    if (Str_Equal((UINT8 *)commandBuf,"PORT"))
                                        status = FSP_Server_PORT(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case PASV:
                                    if (Str_Equal((UINT8 *)commandBuf,"PASV"))
                                    {
                                        server.parentID = fsp_cb->fsp_currentID;
                                        status = FSP_Server_PASV(&server);
                                    }
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case EPSV_REST:
                                    if (Str_Equal((UINT8 *)commandBuf,"EPSV"))
                                    {
                                        server.parentID = fsp_cb->fsp_currentID;
                                        status = FSP_Server_EPSV(&server);
                                    }
                                    else if (Str_Equal((UINT8 *)commandBuf,"REST"))
                                        status = FSP_Server_REST(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case TYPE:
                                    if (Str_Equal((UINT8 *)commandBuf,"TYPE"))
                                        status = FSP_Server_TYPE(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case MODE:
                                    if (Str_Equal((UINT8 *)commandBuf,"MODE"))
                                        status = FSP_Server_MODE(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case STRU:
                                    if (Str_Equal((UINT8 *)commandBuf,"STRU"))
                                        status = FSP_Server_STRU(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                    /**** FTP FILE SERVICE COMMANDS ****/
                                case RETR:
                                    if (Str_Equal((UINT8 *)commandBuf,"RETR"))
                                    {

                                        /* Data task is created by RETR */
                                        server.parentID = fsp_cb->fsp_currentID;
                                        status = FSP_Server_RETR(&server);

                                        /* One more data task to keep track */
                                        if (status == NU_SUCCESS)
                                            FST_Active_Clients++;

                                    }
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case STOR:
                                    if (Str_Equal((UINT8 *)commandBuf,"STOR"))
                                    {

                                        /* Data task is created by STOR */
                                        server.parentID = fsp_cb->fsp_currentID;
                                        status = FSP_Server_STOR(&server);

                                        /* One more data task to keep track */
                                        if (status == NU_SUCCESS)
                                            FST_Active_Clients++;

                                    }
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case APPE:
                                    if (Str_Equal((UINT8 *)commandBuf,"APPE"))
                                    {

                                        /* Data task is created by APPE */
                                        server.parentID = fsp_cb->fsp_currentID;
                                        status = FSP_Server_APPE(&server);

                                        /* One more data task to keep track */
                                        if (status == NU_SUCCESS)
                                            FST_Active_Clients++;

                                    }
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                    /**** FTP DIR SERVICE COMMANDS ****/
                                case RNFR:
                                    if (Str_Equal((UINT8 *)commandBuf,"RNFR"))
                                        status = FSP_Server_RNFR(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case RNTO_XPWD:
                                    if (Str_Equal((UINT8 *)commandBuf,"RNTO"))
                                        status = FSP_Server_RNTO(&server);
                                    else if (Str_Equal((UINT8 *)commandBuf,"XPWD"))
                                        status = FSP_Server_XPWD(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case ABOR:
                                    if (Str_Equal((UINT8 *)commandBuf,"ABOR"))
                                        status = FSP_Server_ABOR(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case DELE:
                                    if (Str_Equal((UINT8 *)commandBuf,"DELE"))
                                        status = FSP_Server_DELE(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case EPRT_XRMD_SIZE:
                                    if (Str_Equal((UINT8*)commandBuf, "EPRT"))
                                        status = FSP_Server_EPRT(&server);
                                    else if (Str_Equal((UINT8*)commandBuf, "XRMD"))
                                        status = FSP_Server_XRMD(&server);
                                    else if (Str_Equal((UINT8*)commandBuf, "SIZE"))
                                        status = FSP_Server_SIZE(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case RMD:
                                    if (Str_Equal((UINT8*)commandBuf, "RMD"))
                                        status = FSP_Server_RMD(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case MKD:
                                    if (Str_Equal((UINT8*)commandBuf, "MKD"))
                                        status = FSP_Server_MKD(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case XMKD:
                                    if (Str_Equal((UINT8*)commandBuf, "XMKD"))
                                        status = FSP_Server_XMKD(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case PWD:
                                    if (Str_Equal((UINT8*)commandBuf, "PWD"))
                                        status = FSP_Server_PWD(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case NLST:
                                    if (Str_Equal((UINT8*)commandBuf, "NLST"))
                                    {

                                        /* Data task is created by NLST */
                                        server.parentID = fsp_cb->fsp_currentID;
                                        status = FSP_Server_NLST(&server);

                                        /* One more data task to keep track */
                                        if (status == NU_SUCCESS)
                                            FST_Active_Clients++;

                                    }
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case LIST_STAT_NOOP:
                                    if (Str_Equal((UINT8 *)commandBuf,"LIST"))
                                    {

                                        /* Data task is created by LIST */
                                        server.parentID = fsp_cb->fsp_currentID;
                                        status = FSP_Server_LIST(&server);

                                        /* One more data task to keep track */
                                        if (status == NU_SUCCESS)
                                            FST_Active_Clients++;
                                    }
                                    else if (Str_Equal((UINT8 *)commandBuf,"STAT"))
                                        status = FSP_Server_STAT(&server);
                                    else if (Str_Equal((UINT8 *)commandBuf,"NOOP"))
                                        status = FSP_Server_NOOP(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case SYST:
                                    if (Str_Equal((UINT8 *)commandBuf,"SYST"))
                                        status = FSP_Server_SYST(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                case FST_HELP:
                                    if (Str_Equal((UINT8 *)commandBuf,"HELP"))
                                        status = FSP_Server_HELP(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                    /**** FTP EXTENSION COMMANDS ****/
                                case FEAT:
                                    if (Str_Equal((UINT8 *)commandBuf,"FEAT"))
                                        status = FSP_Server_FEAT(&server);
                                    else
                                        status = FTP_CMD_UNRECOGNIZED;

                                    break;

                                default:
                                    status = FTP_CMD_UNRECOGNIZED;
                                }
                            }
                            if (status != NU_SUCCESS)
                            {
                                if (status == FTP_CMD_UNRECOGNIZED)
                                    FSP_Server_UNKNOWN(&server);

                                FTP_Printf("Unable to process last command successfully.\r\n");
                            }
                        }
                    }
                    /* Release this task as a file system user. */
                    /* Note - this must be at this level, if release is called
                     * without calling Become_File_User, a slot in the file
                     * user table is lost ! - NTWK_10785
                     */

                    /* Obtain a pointer to all the disks in use */
                    status = NU_List_Mount(&mount_list);

                    if (status != NU_SUCCESS)
                    {
                        FTP_Printf("Control_Task failed to get a list of disks.\r\n");
                    }

                    else
                    {
                        /* Store a copy of the entry point */
                        mount_entry = mount_list;

                        /* Traverse through the entire mount list to close all
                           the disks */
                        while (mount_entry)
                        {
                            /* Convert Drive letter to upper case and then drive number. */
                            drive = (mount_entry->mnt_name[0] & 0xDF) - 'A';

                            /* All done, close the disk */
                            nu_drive[0] = (CHAR)(((CHAR) drive + 1) + 'A' - 1);
                            nu_drive[1] = ':';
                            nu_drive[2] = '\0';

                            status = NU_Close_Disk (nu_drive);

                            if (status != NU_SUCCESS)
                            {
                                FTP_Printf("Control_Task failed to close disk.\r\n");
                            }

                            /* Get the next mount item */
                            mount_entry = mount_entry -> next;
                        }

                        /* Free the mount list */
                        status = NU_Free_List((VOID**)&mount_list);

                        if (status != NU_SUCCESS)
                        {
                            FTP_Printf("Control_Task: Failed to free the File devices mount list.\r\n");
                            out++;/* skip control loop */
                        }
                    }

                    NU_Release_File_User();
                }

                else
                {
                    FTP_Printf("Cannot register as file system user.\r\n");
                    server.lastError = FTP_REGISTRATION_FAILURE;
                }
            }
        }

        /* Ensure that, if the control task has spawned a data task, it does
         * not leave it behind before it itself is terminated
         */
        if (server.datatask)
        {
            if (NU_Resume_Task(server.datatask) != NU_SUCCESS)
                NLOG_Error_Log("Failed to resume Data Task", NERR_SEVERE,
                               __FILE__, __LINE__);

            /* Grab the FST_File_Write_Lock semaphore to ensure that the Data task is
               not writing to a File */
            status = NU_Obtain_Semaphore(&(server.fileWriteLock), NU_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Suspend the Data Task. */
                status = NU_Suspend_Task (server.datatask);

                if (status != NU_SUCCESS)
                    FTP_Printf("Failed to suspend the Data Task.\r\n");

                /* Close the Data Task socket. */
                status = NU_Close_Socket(server.dataSocketd);

                if (status != NU_SUCCESS)
                    FTP_Printf("Failed to close the Data Task socket.\r\n");

                /* Send the Data Task to the Cleaner_Task for resource cleanup. */
                status = NU_Send_To_Queue(&FST_Cleaner_Queue, (VOID*)&(server.datatask),
                                          1, NU_SUSPEND);

                if (status != NU_SUCCESS)
                    FTP_Printf("Failed to enqueue the Data Task to the cleaner queue.\r\n");

                NU_Release_Semaphore(&(server.fileWriteLock));
            }
            else
            {
                NLOG_Error_Log("Failed to Obtain File Write Semaphore\n",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        /* Close control connection */
        FTP_Printf("Closing Control_Task control connection.\r\n");
    
        status = NU_Close_Socket(server.socketd);
    
        if (status != NU_SUCCESS)
            FTP_Printf("Control_Task connection has not been terminated.\r\n");
    
        /* Delete the event group. */
        status = NU_Delete_Event_Group(&server.FTP_Events);
    
        if (status != NU_SUCCESS)
            FTP_Printf("Cannot Delete Event Group in Control Task.\r\n");
    } 

    /* Now cleanup the Control task. */
    
    if (NU_Delete_Semaphore(&(server.fileWriteLock)) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to delete Master Lock semaphore\n",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Send the task ID to the Cleaner_Task for resource cleanup. */
    pointerToThisTask = NU_Current_Task_Pointer();

    status = NU_Send_To_Queue(&FST_Cleaner_Queue, (VOID*)&pointerToThisTask,
                              1, NU_SUSPEND);

    if (status != NU_SUCCESS)
        FTP_Printf("NU_Send_To_Queue error.\r\n");

    if (buffer != NU_NULL)
    {
        /* release the buffer memory allocated at the top of the function */
        status = NU_Deallocate_Memory((VOID*)buffer);

        /* release failed */
        if (status != NU_SUCCESS)
            FTP_Printf("NU_Deallocate_Memory error: Control_Task() - buffer.\r\n");
    }

    if (server.replyBuff != NU_NULL)
    {
        /* release the replyBuff memory allocated at the top of the function */
        status = NU_Deallocate_Memory((VOID*)server.replyBuff);

        /* release failed */
        if (status != NU_SUCCESS)
            FTP_Printf("NU_Deallocate_Memory error: Control_Task() - replyBuff.\r\n");
    }

    if (server.fileSpec != NU_NULL)
    {
        /* release the fileSpec memory allocated at the top of the function */
        status = NU_Deallocate_Memory((VOID*)server.fileSpec);

        /* release failed */
        if (status != NU_SUCCESS)
            FTP_Printf("NU_Deallocate_Memory error: Control_Task() - fileSpec.\r\n");

    }

    if (server.path != NU_NULL)
    {
        /* release the path memory allocated at the top of the function */
        status = NU_Deallocate_Memory((VOID*)server.path);
            /* release failed */
        if (status != NU_SUCCESS)
            FTP_Printf("NU_Deallocate_Memory error: Control_Task() - path.\r\n");
    }
    if (server.renamePath != NU_NULL)
    {
        /* release the renamePath memory allocated at the top of the function */
        status = NU_Deallocate_Memory((VOID*)server.renamePath);
        /* release failed */
        if (status != NU_SUCCESS)
            FTP_Printf("NU_Deallocate_Memory error: Control_Task() - renamePath.\r\n");
    }

    if (server.currentWorkingDir != NU_NULL)
    {
        /* release the currentWorkingDir memory allocated at the top of the function */
        status = NU_Deallocate_Memory((VOID*)server.currentWorkingDir);

        /* release failed */
        if (status != NU_SUCCESS)
            FTP_Printf("NU_Deallocate_Memory error: Control_Task() - currentWorkingDir.\r\n");
    }
    
    NU_USER_MODE();    

} /* Control_Task */

/******************************************************************************
*
*   FUNCTION
*
*       FST_Data_Task_Entry
*
*   DESCRIPTION
*
*       This function provides an entry point for an instance of an FTP data
*       connection for client/server file transfer.  The appropriate command
*       parameters are read from the server structure once the control
*       connection receives it and sets the 'ready' event. After the command
*       processing is complete, the data connection is closed. The function
*       then sends a message to the cleaner task via a Nucleus PLUS queue and
*       self suspends.
*
*   INPUTS
*
*       argc                    unused
*       svr                     pointer to valid FTP server structure
*
*   OUTPUTS
*
*       None
*
******************************************************************************/
VOID FST_Data_Task_Entry(UNSIGNED argc, VOID * svr)
{
    STATUS      status;
    FTP_SERVER *server;
    NU_TASK    *pointerToThisTask;
    UINT32     events;
    INT        id;
    CHAR       nu_drive[3];

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    FTP_UNUSED_PARAMETER(argc);

    FTP_Printf("FST_Data_Task activated.\r\n");

    server = (FTP_SERVER *)svr;

#ifdef CFG_NU_BSP_REALVIEW_EB_CT926EJS_ENABLE
    /* Force client address to correct IP for QEMU */
    server->clientDataAddr.id.is_ip_addrs[0]=10;
    server->clientDataAddr.id.is_ip_addrs[1]=0;
    server->clientDataAddr.id.is_ip_addrs[2]=2;
    server->clientDataAddr.id.is_ip_addrs[3]=2;
#endif

    /* Open a connection via socket interface */
    server->dataSocketd = FST_Client_Connect(server);

    /* If a connection could not be opened, clear any events that
     * have not been handled.
     */
    if (server->dataSocketd < 0)
    {
        /* Reset passive mode flag for this connection in case it was set. */
        server->cmdFTP.pasvFlag = FTP_PASSIVE_MODE_OFF;

        NU_Set_Events(&server->FTP_Events, 0, NU_AND);

        FTP_Printf("FST_Data_Task FST_Client_Connect failure.\r\n");
        server->lastError  = FTP_STACK_ERROR;
        server->stackError = server->dataSocketd;

        /* Send 'Can't open data connection' message */
        status = (INT)NU_Send(server->socketd, MSG425, SIZE425, 0);

        if (status < 0)
        {
            FTP_Printf("FST_Data_Task NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
    }

    else
    {
#ifdef NET_5_1

        /* Enable Zero Copy mode on the socket */
        NU_Fcntl(server->dataSocketd, NU_SET_ZC_MODE, NU_ZC_ENABLE);

#endif
        NU_Protect(&FST_Active_List_Protect);

        /* Match data socket number with the current task CB */
        for (id = 0; id < FTP_SERVER_MAX_CONNECTIONS; id++)
        {
            if (FST_Active_List[id].active_task == server->taskID)
            {
                FST_Active_List[id].active_sckt = server->dataSocketd;
                break;
            }
        }

        NU_Unprotect();

        /* Set the transfer status bit else server will timeout
         * due to lack of command activity
         */
        server->transferStatus = NU_TRUE;

        /* Wait for an event that signifies that the command has arrived
         * from the client.
         */
        if (NU_Retrieve_Events(&server->FTP_Events, FTP_COMMAND_READY,
                               NU_OR_CONSUME, &events,
                               (UNSIGNED)FTPS_DATA_TIMEOUT) == NU_SUCCESS)
        {
            /* Reset passive mode flag for this connection in case it was set. */
            server->cmdFTP.pasvFlag = FTP_PASSIVE_MODE_OFF;

            /* Determine what drive should the current operation be performed on */
            nu_drive[0] = (CHAR)(((server->defaultDrive) + 1) + 'A' - 1);
            nu_drive[1] = ':';
            nu_drive[2] = '\0';    
            
            /* If we get here, file system must have been initialized and so
             * check to see if the required drive is available.
             */
            status = FTPS_Wait_For_FS(nu_drive, NU_NO_SUSPEND);
            
            if(status == NU_SUCCESS)
            {
                /* Process the operation based on which event came in.
                   There can be only one event. */
                switch (events)
                {
                case RETR_Event:
                    /* get data from file and send to client. */
                    FTPS_Server_Put(server);
                    break;

                case STOR_Event:
                    /* get data from client and save to a file */
                    FTPS_Server_Get(server, STOR_Event);
                    break;

                case APPE_Event:
                    /* get data from client and append to a file */
                    FTPS_Server_Get(server, APPE_Event);
                    break;

                case LIST_Event:
                    /* send a formatted list of file information */
                    FTPS_Server_Dir(server, LIST_Event);
                    break;

                case NLST_Event:
                    /* send a basic list of filepaths */
                    FTPS_Server_Dir(server, NLST_Event);
                    break;

                default:
                    /* Bad event received. */
                    FTP_Printf("Bad event received in FST_Data_Task.\r\n");
                }
            }
            else
            {
                FTP_Printf("FST_Data_Task: Requested Drive not available.\r\n");
                server->lastError = FTP_OPEN_DRIVE_FAILURE;

                /* Send 'Requested action aborted' message */
                if (NU_Send(server->socketd, MSG426, SIZE426, 0) < 0)
                {
                    FTP_Printf("FST_Data_Task NU_Send failure.\r\n");
                    server->lastError = FTP_STACK_ERROR;
                }
            }

        }
        else
        {
            /* Reset passive mode flag for this connection in case it was set. */
            server->cmdFTP.pasvFlag = FTP_PASSIVE_MODE_OFF;

            FTP_Printf("Cannot Retrieve Events in FST_Data_Task_Entry.\r\n");
        }

        /* Clear the transfer status bit */
        server->transferStatus = NU_FALSE;

        /* Clean the Data task pointer from the Server structure */
        server->datatask = NU_NULL;

        /* Close data connection */
        status = NU_Close_Socket(server->dataSocketd);

        if (status < 0)
        {
            FTP_Printf("FST_Data_Task NU_Close_Socket failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
    }

    /* Send the task ID to the Cleaner_Task for resource cleanup. */
    pointerToThisTask = NU_Current_Task_Pointer();

    status = NU_Send_To_Queue(&FST_Cleaner_Queue, (VOID*)&pointerToThisTask,
                              1, NU_SUSPEND);
    if (status != NU_SUCCESS)
        FTP_Printf("FST_Data_Task NU_Send_To_Queue error.\r\n");

    NU_USER_MODE();

} /* FST_Data_Task_Entry */

/************************************************************************
*
*   FUNCTION
*
*       FST_Client_Connect
*
*   DESCRIPTION
*
*       Connects to a client using NU_Accept or NU_Connect, depending
*       on whether or not the server is in passive mode.
*
*   INPUTS
*
*       server                  Pointer to the server structure.
*
*   OUTPUTS
*
*       A new socket ( >= 0 ) or an error message ( < 0 ).
*
************************************************************************/
INT FST_Client_Connect(FTP_SERVER *server)
{
    INT         socket, newsock = -1;
    CHAR        buffer[64];
    CHAR        *strptr;
    INT         status;
    FD_SET      readfs;

    socket = NU_Socket(server->clientDataAddr.family, NU_TYPE_STREAM,
                       NU_NONE);

    if (socket >= 0)
    {
        /* Set the block flag in socket structure */
        NU_Fcntl(socket, NU_SETFLAG, NU_BLOCK);

        /* If in passive mode, set up a socket to accept a client connection. */
        if ( (server->cmdFTP.pasvFlag == FTP_PASSIVE_MODE_ON) ||
             (server->cmdFTP.pasvFlag == FTP_EPASSIVE_MODE_ON) )
        {
#if (NET_VERSION_COMP > NET_4_3)
            do
            {
                /* Create a unique port number >= 0x800. */
                server->serverDataAddr.port = (UINT16)(UTL_Rand() | 2048);

                /* If the bind fails, then the port is in use. Find another. */
                status = NU_Bind(socket, &server->serverDataAddr, 0);

            } while (status == NU_INVALID_ADDRESS);
#else

            /* Bind to a unique port number. */
            server->serverDataAddr.port = UTL_Get_Unique_Port_Number();

            status = NU_Bind(socket, &server->serverDataAddr, 0);
#endif
            /* Continue only if bind is successful. */
            if (status >= 0)
            {
                /* We have a valid port. Be ready to accept a connection. */
                status = NU_Listen(socket, 1);

                /* At this point we know that a data connection is ready
                   to be made with another server, so send the client a
                   successful response message. */
                if (status == NU_SUCCESS)
                {
                    /* Send an EPSV response */
                    if (server->cmdFTP.pasvFlag == FTP_EPASSIVE_MODE_ON)
                    {
                        /* Build the response string. */
                        strcpy(buffer, MSG229);
                        strptr = buffer + SIZE229;

                        /* Put the port in the message */
                        NU_ITOA(server->serverDataAddr.port, strptr, 10);

                        /* Close the parenthesis, terminate, and send. */
                        strcat(strptr, "|)\r\n");
                    }

                    /* Send a PASV response */
                    else
                    {
                        /* Build the response string. */
                        strcpy(buffer, MSG227);
                        strptr = buffer + SIZE227;

                        /* Append the IP address and port number as an ASCII
                           sequence of numbers. This function puts it right into
                           the buffer. */
                        FC_AddrToString(&server->serverDataAddr, strptr);

                        /* Close the parenthesis, terminate, and send. */
                        strcat(strptr, ")\r\n");
                    }

                    /* Send the message */
                    status = (INT)NU_Send(server->socketd, buffer,
                                     (UINT16)strlen(buffer), 0);

                    if (status < 0)
                    {
                        FTP_Printf("NU_Send failure.\r\n");
                        server->lastError = FTP_STACK_ERROR;
                        server->stackError = status;
                    }
                    else
                    {
                        NU_FD_Init(&readfs);
                        NU_FD_Set(socket, &readfs);

                        /* Wait for the other server to establish the data
                           connection */
                        status = NU_Select(socket + 1, &readfs, NU_NULL, NU_NULL,
                                           FTPS_DATA_TIMEOUT);

                        if ( (status == NU_SUCCESS) &&
                             (NU_FD_Check(socket, &readfs) == NU_TRUE) )
                            newsock = NU_Accept(socket, &server->clientDataAddr, 0);
                        else
                            FTP_Printf("NU_Select failure.\r\n");

                        /* Close the listening socket. Only one connection
                           will be accepted per data transfer. */
                        NU_Close_Socket(socket);
                    }
                }
            } /* Successful bind. */

            else
            {
                /* Send 'Transfer aborted' message */
                status = (INT)NU_Send(server->socketd, MSG426, SIZE426, 0);

                if (status < 0)
                {
                    FTP_Printf("NU_Send failure.\r\n");
                    server->lastError = FTP_STACK_ERROR;
                    server->stackError = status;
                }
            }
        }

        else
        {
            /* In active mode, simply connect to a client. */
            status = NU_Connect(socket, &server->clientDataAddr, 0);

            if (status < 0)
            {
                FTP_Printf("FST_Client_Connect NU_Connect failure.\r\n");
                newsock = status;
            }
            else
                newsock = socket;
        }
    }

    else/* Call to NU_Socket() failed */
    {
        FTP_Printf("FST_Client_Connect NU_Socket failure.\r\n");
        server->lastError = FTP_STACK_ERROR;
        server->stackError = socket;

        /* Send 'Can't open data connection' message */
        status = (INT)NU_Send(server->socketd, MSG425, SIZE425, 0);

        if (status < 0)
        {
            FTP_Printf("FST_Client_Connect NU_Send failure.\r\n");
            server->lastError = FTP_STACK_ERROR;
            server->stackError = status;
        }
    }

    return (newsock);

} /* FST_Client_Connect */

/************************************************************************
*
*   FUNCTION
*
*       FTP_Printf
*
*   DESCRIPTION
*
*       Handles all printing macroed.  The user should put his serial
*       print or console print commands here
*
*   INPUTS
*
*       fmt                     Pointer to data to print
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID FTP_Printf(CHAR *fmt)
{
    /* Remove compilation warnings */
    FTP_UNUSED_PARAMETER(fmt);

    /* This must be filled in for specific hardware */

} /* FTP_Printf */
