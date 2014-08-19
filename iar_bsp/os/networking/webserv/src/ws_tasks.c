/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2002              
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/

/************************************************************************
*
* FILE NAME                                                           
*                                                                      
*      ws_tasks.c                                                 
*                                                                      
* COMPONENT                                                            
*                                                                      
*      Nucleus Web Server Tasking                                    
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*      This file holds the Nucleus Web Server tasks.  It handles task   
*      initialization.  The tasks initialized handle binding a socket,  
*      listening to a socket, accepting a connection, and processing a  
*      connection to handle HTTP 1.1/1.0 connections with a web browser.
*      Also contains support routines for the webserv product.         
*                                                                      
* DATA STRUCTURES                                                      
*                                                                      
*      socketQueue                 Pointer to socket queue             
*                                  control block.                      
*                                                                      
* FUNCTIONS                                                            
*                                                                      
*      nu_os_net_web_init          Run level initialization routine.    
*      WS_Receive_Task             Initializes Nucleus Web Server,      
*                                  creates Web server task, opens       
*                                  socket connection, Binds socket      
*                                  connection, Listens to the socket    
*                                  connection, and Accepts sockets      
*                                  indefinitely and transmits the       
*                                  sockets through a queue to the server
*                                  task.                                
*      WS_Make_Worker_Task         Creates a worker task
*      WS_Worker_Task              Receives a connection and processes  
*                                  the HTTP request.                    
*      WS_Webserv_Initialize       Initialize WebServ.                  
*      WS_Set_Client_Addr          Gets the Peer names address.         
*      WS_Remove_Socket_Entry      Removes a socket entry from the      
*                                  doubly linked list.                  
*      WS_Add_Socket_Entry         Adds an entry to the HTTP            
*                                  socket doubly linked list.           
*      WS_Create_Socket_List       Initializes list that keeps track
*                                  of currently active sockets
*      WS_In_String                Compares string to actual buffer.    
*      WS_Register_Plugin          Register plugins.                
*      WS_Initialize_Server        Initializes Nucleus Web Server.  
*      WS_Find_Token_by_Case       Finds a token in a string.       
*      WS_Mem_Cpy                  A memcpy for both 16 and 32 bit
*                                  processors
*      WS_Strn_Cpy                 A strncpy for both 16 and 32 bit
*                                  processors
*      WS_Clear_Rdata              Clears the outgoing data within 
*                                  the request structure
*      WS_Cleanup_Server           Removes all tasks, queues, and files
*                                  that are present in operating system
*      WS_Receive_Data             Receives a request from the client
*      WS_Receive_Header           Receives the request header
*                                                                      
* DEPENDENCIES                                                         
*
*      nu_websr.h
*      netevent.h
*      wpw_auth.h
*                                                                      
************************************************************************/

#include "networking/nu_websr.h"


#include "networking/netevent.h"
#include "networking/wpw_auth.h"
#include "os/kernel/plus/supplement/inc/error_management.h"

#ifdef NET_5_1
extern NU_MEMORY_POOL   *MEM_Cached;

#if INCLUDE_IPV6 == NU_TRUE
#include "networking/ip6.h"

#endif
#else
extern NU_MEMORY_POOL   System_Memory;
#endif

#if INCLUDE_SSL
extern SSL_CTX *NU_SSL_CTX;
extern NU_TASK NU_SSL_Receive_CB;
#endif

/* declare space for the master server structure */
WS_SERVER WS_Master_Server;      
WS_FS_FILE * HTTP_Fs_File;

#if !INCLUDE_FILE_SYSTEM
/* Semaphore to access the in-memory file system. */
NU_SEMAPHORE            WS_FS_Access;
#endif

/* Define default mount point */
static CHAR WS_DEFAULT_MP = WS_DEFAULT_DRIVE + 'A';

/* Declare space for runtime configuration variables. */
UINT32 WSC_Use_Hostname = NU_FALSE;

/* Protect structure for protecting Webserver globals */
NU_PROTECT              WS_Protect;

/* Define Application data structures.  */

NU_TASK                 WS_HTTP_Receive_CB;
NU_QUEUE                WS_Socket_Queue;
NU_TASK                 WS_Servers[WS_MAX_WORKER_TASKS];

extern WS_FS_FILE Embed_Fs[];


/*  WS_TASKS function prototypes */

STATIC STATUS  WS_Make_Worker_Task(INT index);
STATIC VOID    WS_Worker_Task(UNSIGNED argc, VOID  *argv);
STATIC VOID    WS_Set_Client_Addr(WS_REQUEST  *Req);
STATIC STATUS  WS_Create_Socket_List(WS_SOCK_LIST *free_sock);
STATIC VOID    WS_Add_Socket_Entry(WS_SOCK_LIST *http_sock, WS_SOCK_LIST *free_sock, WS_SOCKET *socket_desc);
STATIC VOID    WS_Remove_Socket_Entry(WS_SOCK_LIST *http_sock, WS_SOCK_LIST *free_sock, WS_SOCKET_STRUCT *hsock_ent);
STATIC STATUS  WS_Initialize_Server(WS_SERVER *server_info);
STATIC STATUS  WS_Receive_Data(WS_REQUEST *req);
STATIC CHAR *  WS_Receive_Header(WS_REQUEST *req);
STATUS WS_Wait_For_FS(VOID);

WS_PLUGIN_STRUCT *HTTP_Plugins;

/* String to store registry path for web server */
CHAR                Web_Server_Registry_Path[REG_MAX_KEY_LENGTH] = {0};

#if ((defined NET_5_1) && (INCLUDE_BASIC_AUTH || WS_AUTHENTICATION))
extern struct WPW_AUTH_NODE WPW_Table[];
#endif

/************************************************************************
* FUNCTION
*
*       nu_os_net_web_init
*
* DESCRIPTION
*
*       This function initializes the Nucleus Web Server.
*       It is the entry point of the product initialization
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
*       WS_INVALID_PARAMS       Invalid parameter(s).
*       <other> -               Indicates (other) internal error occured.
*
************************************************************************/
STATUS nu_os_net_web_init(CHAR *path, INT startstop)
{
    STATUS              status = -1;
#if ((defined NET_5_1) && (INCLUDE_BASIC_AUTH || WS_AUTHENTICATION))
    UINT8               i;
#endif

    if(path != NU_NULL)
    {
        /* Save a copy locally. */
        strcpy(Web_Server_Registry_Path, path);
    }

    if(startstop)
    {
        /* Initialize the Nucleus Web server Task, Queue and Semaphore. */
        status = WS_Webserv_Initialize();

        if(status == NU_SUCCESS)
        {
#if ((defined NET_5_1) && (INCLUDE_BASIC_AUTH || WS_AUTHENTICATION))
            /* Initialize the list of Web Server users. 
             * Initailize Linked List for password structure 
             */
            for (i = 0; WPW_Table[i].wpw_user_id[0]; i++)
            {
                status = UM_Add_User(WPW_Table[i].wpw_user_id, WPW_Table[i].wpw_password, UM_WEB, NU_NULL);
            }
#endif
        }
        else
        {
            NLOG_Error_Log("Error at call to WS_Webserv_Initialize().\n",
                                            NERR_FATAL, __FILE__, __LINE__);
        }

    }
    else
    {
        /* Stop requested */
        WS_Cleanup_Server();

        /* Return success */
        status = NU_SUCCESS;
    }

    return (status);
} /* nu_os_net_web_init */
/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WS_Webserv_Initialize                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Function that allows easy interface to hook in the Nucleus       
*     Webserv product. This function initializes the web server tasks  
*     and the queue.  It also calls the service initialization function.                                                 
*                                                                      
* INPUTS                                                               
*                                                                      
*     None.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     status            NU_SUCCESS is returned if initialization
*                       has completed successfully.                                    
*                                                                      
************************************************************************/

STATUS WS_Webserv_Initialize(VOID)
{
    VOID           *pointer;
    STATUS         status;
    INT            num = 0;

    /* Setup MMU if it is present */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    /* Set the memory pool that WebServ will use.
     */
#ifdef NET_5_1
    WS_Master_Server.ws_memory_pool = MEM_Cached;
#else
    WS_Master_Server.ws_memory_pool = &System_Memory;
#endif

    /* Create application tasks and queue */

    status = NU_Allocate_Memory(WS_Master_Server.ws_memory_pool, &pointer, 2000,
                            NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
#if NU_WEBSERV_DEBUG
        printf ("Can not obtain memory for WS_Receive_Task.\n");
#endif
    }

    if(status == NU_SUCCESS)
    {
        /*  Create Nucleus Web Server Receiving Task  */
        status = NU_Create_Task(&WS_HTTP_Receive_CB, "NUWebSrv", WS_Receive_Task,
                                WS_HTTP_PORT, NU_NULL, pointer, 2000, TM_PRIORITY + 2, 1000,
                                NU_PREEMPT, NU_NO_START);
    
        if (status != NU_SUCCESS)
        {
            NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
#if NU_WEBSERV_DEBUG
            printf ("Cannot create WS_Receive_Task\n");
#endif
        }
        else
        {
            NU_Resume_Task(&WS_HTTP_Receive_CB);
        }
    }

    if(status == NU_SUCCESS)
    {
        status = NU_Allocate_Memory(WS_Master_Server.ws_memory_pool, &pointer, 
                                WS_QSIZE * sizeof(UNSIGNED), NU_NO_SUSPEND);
    
        if (status != NU_SUCCESS)
        {
            NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
#if NU_WEBSERV_DEBUG
            printf ("Can not obtain memory for SocQueue.\n");
#endif 
        }
    }
    
    /*  Create Nucleus Web Server Socket Queue  */
    if(status == NU_SUCCESS)
    {
        status = NU_Create_Queue(&WS_Socket_Queue, "SocketQ", pointer, 
                                WS_QSIZE, NU_FIXED_SIZE, 2, NU_FIFO);
    
        if (status != NU_SUCCESS)
        {
            NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
#if NU_WEBSERV_DEBUG
        printf ("Can not create Socket Queue.\n");
#endif
        }
    }
    
#if !INCLUDE_FILE_SYSTEM
    /*  Create Semaphore for access of in-memory file system */
    if(status == NU_SUCCESS)
    {
        /* Create synchronization semaphore.  This semaphore is used to arbitrate
         * access to the In-Memory File System.
         */
        status = NU_Create_Semaphore(&WS_FS_Access, "WS_FS", (UNSIGNED)1, NU_FIFO);

        if (status != NU_SUCCESS)
        {
            NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
#if NU_WEBSERV_DEBUG
        printf ("Can not create Semaphore.\n");
#endif
        }
    }
#endif

    /*  Create tasks to handle all requests  */
    if(status == NU_SUCCESS)
    {
        for(;num < WS_MAX_WORKER_TASKS && status == NU_SUCCESS;num++)
        {
            status = WS_Make_Worker_Task(num);
        
            if(status != NU_SUCCESS)
            {
                NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
#if NU_WEBSERV_DEBUG
        printf ("Can not create Worker Task.\n");
#endif
            }
        }
    }
    
    /* If a failure has occured, free up allocated memory and 
     * kill all created tasks
     */
    if(status != NU_SUCCESS)
        WS_Cleanup_Server();

    NU_USER_MODE();
    return(status);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       WS_Initialize_Server                                                        
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function initializes all features of the webserver.  This        
*       includes registering plugins for all the features found in         
*       ws_cfg.h.  The file system is also set up here. 
*                                                                       
* INPUTS                                                                
*                                                                       
*       None
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       status              NU_SUCCESS if initializtion completed
*                           correctly.
*                                                                       
*************************************************************************/
STATIC STATUS WS_Initialize_Server(WS_SERVER *server_info)
{
    STATUS  status = NU_SUCCESS;
    
#if INCLUDE_INITIAL_FILES
    WS_FS_FILE * file_ptr;
#endif
    
    UNUSED_PARAMETER(server_info);

    /* Zero out the Webserv protection data structure */
    UTL_Zero(&WS_Protect, sizeof(NU_PROTECT));

    HTTP_Plugins = NU_NULL;

#if !INCLUDE_FILE_SYSTEM
#if INCLUDE_INITIAL_FILES
    /* Initialize WebServ's in core file system */
    {
        file_ptr = Embed_Fs;
        HTTP_Fs_File = file_ptr;
        
        /* convert an array of WS_FS_FILE structures
         * into a linked list
         * to make it easier to add/remove files
         */
        while(file_ptr->ws_addr)
        {
            file_ptr->ws_next = (file_ptr + 1);
            file_ptr++;
        }
        
        file_ptr--;
        file_ptr->ws_next = NU_NULL;                             
    }

#endif /* INCLUDE_INITIAL_FILES */
#else /* INCLUDE_FILE_SYSTEM */
 
    HTTP_Fs_File = NU_NULL;
    
#if INCLUDE_INITIAL_FILES
    
    /* Load the initial files into the external file system */
    file_ptr = Embed_Fs;
    for(; file_ptr->ws_addr && (status == NU_SUCCESS); file_ptr++)
    {
        if(WSF_Write_File_System(file_ptr->ws_name, file_ptr->ws_addr, 
                                 (UINT32)file_ptr->ws_length) != NU_SUCCESS)
        {
            NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
            status = WS_FAILURE;
        }
    }

#endif /* INCLUDE_INITIAL_FILES */
#endif /* if not INCLUDE_FILE_SYSTEM */
    
    if(status == NU_SUCCESS)
    {
        /* Register "internal" plugins */
#if INCLUDE_UPLOAD_PLGN
        WS_Register_Plugin(UPL_File_Upload, "upload", NU_NULL);
#endif
    
#if INCLUDE_DIR_PLGN
        WS_Register_Plugin(DIR_List_Directory, "dir", NU_NULL);
#endif
    
        /* Initialize authentication scheme */
#if INCLUDE_BASIC_AUTH
        UNUSED_PARAMETER(server_info);
        status = BSC_Auth_Init();
#endif
    
#if INCLUDE_DES_AUTH
        DES_Auth_Initialize(server_info);
#endif

#if INCLUDE_JS_AUTH
        status = JS_Init_Auth(server_info);
#endif
    }

    return(status);
}

/*************************************************************************                                                                       
* FUNCTION                                                              
*                                                                       
*      WS_Make_Worker_Task                                              
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*      This function creates the WS_Worker_Tasks.                      
*                                                                       
* INPUTS                                                                
*                                                                       
*      index                       Number of server entry.              
*                                                                       
* OUTPUTS                                                               
*                                                                       
*      None.                                                            
*                                                                       
*************************************************************************/

STATIC STATUS WS_Make_Worker_Task(INT index)
{
    VOID        *pointer;
    STATUS      status;
    CHAR        server[8];

    server[0] = 's';
    server[1] = 'e';
    server[2] = 'r';
    server[3] = 'v';
    server[4] = 'e';
    server[5] = 'r';
    server[6] = (CHAR)(65 + index);
    server[7] = '\0';

    status = NU_Allocate_Memory(WS_Master_Server.ws_memory_pool, &pointer, WS_STACK_SIZE, 
                                NU_NO_SUSPEND);

    if (status != NU_SUCCESS) 
    {
#if NU_WEBSERV_DEBUG
        printf ("No memory for worker task\n");
#endif
        return(status);
    }

    /* Create the actual worker task based on the index entry */
    status = NU_Create_Task(&WS_Servers[index], server, WS_Worker_Task, 
                            (UNSIGNED)index, NU_NULL, pointer, WS_STACK_SIZE,
                            (TM_PRIORITY + 5), 0, NU_PREEMPT, NU_NO_START);
    
    if (status != NU_SUCCESS) 
    {
#if NU_WEBSERV_DEBUG
    printf ("Cannot create worker task = %d\n",status);
#endif
        return(status);
    }
        else
        {
            NU_Resume_Task(&WS_Servers[index]);
        }

    return(NU_SUCCESS);
}


/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WS_Receive_Task                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     The main server function.  It receives the client connection  
*     and uses a queue to transmit the the connected socket once the    
*     it has been accepted.  This socket is then handled by the worker
*     tasks.                                          
*                                                                      
* INPUTS                                                               
*                                                                      
*     None.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
************************************************************************/

VOID WS_Receive_Task(UNSIGNED port, VOID *argv)
{
    INT                 socketd;                    
    WS_SOCKET           ws_socketd;
    INT                 new_socket;                 
    struct addr_struct  servaddr;                   
    struct addr_struct  client_addr;
    STATUS              status;
    INT16               family;
#if INCLUDE_FILE_SYSTEM || INCLUDE_SSL
    CHAR                nu_drive[3];
#endif

    /* Setup MMU if it is present */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

#if ((defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE))
    family = NU_FAMILY_IP6;
#else
    family = NU_FAMILY_IP;
#endif

    /* Remove any warnings */
    UNUSED_PARAMETER(argv);

    /*  Print banner.  */
#if NU_WEBSERV_DEBUG
    printf("Nucleus WebServ\nHTTP 1.1 Server\n\n");
#endif 

#if INCLUDE_FILE_SYSTEM || INCLUDE_SSL

        /* Wait for File system to initialize */
        status = WS_Wait_For_FS();

        if(status == NU_SUCCESS)
        {
            status = NU_Become_File_User();
            if (status == NU_SUCCESS)
            {
                /* Open disk for accessing */
                nu_drive[0] = WS_DEFAULT_MP;
                nu_drive[1] = ':';
                nu_drive[2] = '\0';    

                 status = NU_Open_Disk (nu_drive);

                /* If the disk is already open, return success and 
                 * leave the current directory where it is.
                 */
                if (status == NUF_NO_ACTION)
                    status = NU_SUCCESS;
                    
                if (status == NU_SUCCESS)
                {
                    /* Set the default drive */
                    status = NU_Set_Default_Drive(WS_DEFAULT_DRIVE);
                }
            }
        }

        if(status != NU_SUCCESS)
        {
#if NU_WEBSERV_DEBUG
            NLOG_Error_Log("Failed to access file system .\n",
                                NERR_FATAL, __FILE__, __LINE__);
#endif
            ERC_System_Error(status);
        }
#endif

    /* Perform initialization of WebServ services */
    status = WS_Initialize_Server(&WS_Master_Server);

    if (status != NU_SUCCESS)
    {
        NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
#if NU_WEBSERV_DEBUG
        printf ("WebServ services failed initialization.\n");
#endif
    }

    /* This process hangs in an endless loop
     * doing socket accept's. Each accept yields
     * a socket descriptor which is put on the Queue
     */

    /* open a connection via the socket interface */
    if ((socketd = NU_Socket(family, NU_TYPE_STREAM, 0)) >=0 )
    {

        /* fill in a structure with the server address */
        servaddr.family    = family;
        servaddr.port      = (UINT16)port;
        
        /*  Set the ip address to IP addr ANY  */
#if ((defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE))
        servaddr.id = IP6_ADDR_ANY;
#else
        *(UINT32 *)servaddr.id.is_ip_addrs = IP_ADDR_ANY;
#endif

        /* make an NU_Bind() call to bind the server's address */
        if ((NU_Bind(socketd, &servaddr, 0)) >= 0)
        {
            /* be ready to accept connection requests */
            status = NU_Listen(socketd, 10);

            if (status == NU_SUCCESS)
            {
                while(1)
                {
                    /* block in NU_Accept until a client connects */
                    new_socket = NU_Accept(socketd, &client_addr, 0);
                    if (new_socket >= 0)
                    {
#if NU_WEBSERV_DEBUG
                        printf("accept\n");
#endif
                        ws_socketd.ws_family = client_addr.family;
                        ws_socketd.ws_new_socket = new_socket;

                        /* turn on the "block during a read" flag */    
                        NU_Fcntl(new_socket, NU_SETFLAG, NU_BLOCK);

                        /* process the new connection */
                        NU_Send_To_Queue(&WS_Socket_Queue, &ws_socketd, 2, 
                                         NU_SUSPEND);
                            
                    }   /* end successful NU_Accept */
                }       /* end for loop */
            }           /* end successful NU_Listen */
        }               /* end successful NU_Bind */
    }                   /* end successful NU_Socket */
    NU_USER_MODE();
}

/******************************************************************************
*
*   FUNCTION
*
*      WS_Wait_For_FS
*
*   DESCRIPTION
*
*      Wait for a drive to be initialized.
*
*   INPUTS
*
*      None
*
*   OUTPUTS
*
*      NU_SUCCESS             File System has been successfully initialized
*         < 0                 File System initialization error
*
*******************************************************************************/
STATUS WS_Wait_For_FS(VOID)
{
    STATUS          status;
    
    /* Suspend until a File device is mounted */
    status = NU_Storage_Device_Wait(&WS_DEFAULT_MP, NU_SUSPEND);
                 
    return (status);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WS_Worker_Task                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This function does the main processing of the client connections.
*     It first checks for a new socket in the global queue.  If one is
*     found, it adds it to it's local list of sockets.  The task then
*     locates a socket trying to communicate with the server and 
*     processes the connection.
*                                                                      
* INPUTS                                                               
*                                                                      
*     None.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
************************************************************************/

STATIC VOID WS_Worker_Task(UNSIGNED argc, VOID *argv)
{
    WS_SOCKET               ws_sockfd;
    WS_REQUEST              Req;
    FD_SET                  readfs;
    UINT32                  curtime;

    WS_SOCK_LIST            http_sock;
    WS_SOCK_LIST            free_sock;
    WS_SOCKET_STRUCT        *hsock_ent;
    WS_SOCKET_STRUCT        *hsock_temp;
    
    STATUS                  status;
    UNSIGNED                suspend;
    UNSIGNED                actsize;
    CHAR                    nu_drive[3];


    /* Setup MMU if it is present */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();
    
    /*  Remove warnings for unused parameters.  */
    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);
    
#if INCLUDE_FILE_SYSTEM
    /* Check to see if the File System has been initialized. */
    status = WS_Wait_For_FS();

    if(status == NU_SUCCESS)
        status = NU_Become_File_User();

    if(status != NU_SUCCESS)
        return;
#endif

    /* Clear the rdata structure */
    Req.ws_rdata.ws_in_data = NU_NULL;
    Req.ws_rdata.ws_in_data_sz = 0;
    Req.ws_rdata.ws_in_header_sz = 0;
    memset( Req.ws_rdata.ws_in_header, 0x00, sizeof(Req.ws_rdata.ws_in_header) );
    Req.ws_rdata.ws_next_in_data = NU_NULL;
    Req.ws_rdata.ws_next_in_data_sz = 0;
    Req.ws_rdata.ws_in_data_flag = 0;

    Req.ws_rdata.ws_out_data = NU_NULL;
    WS_Clear_Rdata(&Req);

    /* Setup pointer to server state structure */
    Req.ws_server = &WS_Master_Server;  

    /* Initialize the pointers to the working list and the free list */
    http_sock.ws_sock_list_head = NU_NULL;
    http_sock.ws_sock_list_tail = NU_NULL;
    
    free_sock.ws_sock_list_head = NU_NULL;
    free_sock.ws_sock_list_tail = NU_NULL;
    
    /*  Initialize list of nodes to be used to store socket information */
    status = WS_Create_Socket_List(&free_sock);
    if(status != NU_SUCCESS)
        return;

    /* Open disk for accessing */
    nu_drive[0] = WS_DEFAULT_MP;
    nu_drive[1] = ':';
    nu_drive[2] = '\0';

    for( ; ; ) /* Endless loop processing HTTP requests */
    {   
        status = NU_SUCCESS;

#if INCLUDE_FILE_SYSTEM
        status = NU_Open_Disk (nu_drive);

        /* If the disk is already open, return success and leave the current
         * directory where it is.
         */
        if (status == NUF_NO_ACTION)
            status = NU_SUCCESS;

        if (status == NU_SUCCESS)
        {
            /* Set the default drive */
            status = NU_Set_Default_Drive(WS_DEFAULT_DRIVE);
        }
        else
        {
            NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
        }

        if (status == NU_SUCCESS)
        {
            /*  Set Current Directory to "\" */
            status = NU_Set_Current_Dir("\\");

            if (status != NU_SUCCESS)
            {
                NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
                NU_Close_Disk (nu_drive);
            }
        }
        else
        {
            NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
            NU_Close_Disk (nu_drive);
        }
#endif /* INCLUDE_FILE_SYSTEM */

        if (status == NU_SUCCESS)
        {
            /* Initialize the bitmap */
            NU_FD_Init(&readfs);

            /* Check for a new socket in the queue, suspend if there are no
             * connections to process in the list or the queue
             */
            if(free_sock.ws_sock_list_head != NU_NULL)
            {
                if(http_sock.ws_sock_list_head == NU_NULL)
                    suspend = (UNSIGNED)NU_SUSPEND;
                else
                    suspend = (UNSIGNED)NU_NO_SUSPEND;

                status = NU_Receive_From_Queue(&WS_Socket_Queue, (VOID *)&ws_sockfd,
                                               2, &actsize, suspend);

                if(status == NU_SUCCESS)
                    WS_Add_Socket_Entry(&http_sock, &free_sock, &ws_sockfd);
            }

            /* Check for any dead connections */
            hsock_ent = http_sock.ws_sock_list_head;

            while(hsock_ent)
            {
                /* Get the current time in seconds */
                curtime = WS_Get_Seconds;

                /* Get the number of seconds elapsed */
                if(curtime < hsock_ent->ws_time_click)
                {
                    curtime = curtime + (0xfffffffful/TICKS_PER_SECOND -
                        hsock_ent->ws_time_click);
                }
                else
                    curtime = curtime - hsock_ent->ws_time_click;

                if((NU_Is_Connected(hsock_ent->ws_socketd) == NU_TRUE) &&
                   (curtime < WS_SOCKET_GRACEFUL_TIMEOUT))
                {
                    /* This connection is still good, mark it in the bitmap */
                    NU_FD_Set(hsock_ent->ws_socketd, &readfs);
                    hsock_ent = hsock_ent->ws_next_link;
                }
                else
                {
                    /* This connection is bad, close it and remove it from the list */
                    hsock_temp = hsock_ent;
                    hsock_ent = hsock_ent->ws_next_link;
                    WS_Remove_Socket_Entry(&http_sock, &free_sock, hsock_temp);
                }
            }


            /*  Select on all of the sockets within the system based on an HTTP timeout */
            if((NU_Select(NSOCKETS, &readfs, NU_NULL, NU_NULL, WS_TIMEOUT)) == NU_SUCCESS)
            {
                /*  Find which socket has the incoming data available */
                for(hsock_ent = http_sock.ws_sock_list_head; hsock_ent; hsock_ent = hsock_ent->ws_next_link)
                {
                    if(NU_FD_Check(hsock_ent->ws_socketd, &readfs) == NU_TRUE)
                    {
                        /* This socket has data ready to proccess */
                        Req.ws_sd = hsock_ent->ws_socketd;
                        Req.ws_family = hsock_ent->ws_family;

                        /* Set if this is an SSL connection */
                        Req.ws_ssl = hsock_ent->ws_ssl_ptr;

                        /* Do the following loop as long as the client has sent data to process */
                        do
                        {
                            status = WS_Receive_Data(&Req);
                            if(status == NU_SUCCESS)
                            {
                                if(WS_Find_Token(WS_HTTP_11_STRING, (CHAR HUGE *)Req.ws_rdata.ws_in_header,
                                                (CHAR*)(Req.ws_rdata.ws_in_header + Req.ws_rdata.ws_in_header_sz)))
                                    hsock_ent->ws_http_ver = WS_HTTP_11;
                                else
                                    hsock_ent->ws_http_ver = WS_HTTP_10;

                                /* Get the clients IP address */
                                WS_Set_Client_Addr(&Req);

                                Req.ws_http_ver = &(hsock_ent->ws_http_ver);
                                Req.ws_server_ip = hsock_ent->ws_ip;

                                /* Call the server for every HTTP request */
                                HTTP_Parse_Request(&Req);

                                /* flush the output buffer */
                                if(Req.ws_rdata.ws_out_data)
                                    if(WSN_Write_To_Net(&Req, 0, 0, WS_PLUGIN_SEND) != NU_SUCCESS)
                                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                            }

                            /* Clear the outgoing rdata structure */
                            WS_Clear_Rdata(&Req);

                         /* continue loop only if request is HTTP 1.1 and
                          * data still exists in the buffer
                          */
                        }while(hsock_ent->ws_http_ver == WS_HTTP_11 &&
                               (Req.ws_rdata.ws_in_data_flag & WS_DATA_AVAIL) &&
                               (status == NU_SUCCESS));

                        /* Clear the input buffer and flags */
                        if(Req.ws_rdata.ws_in_data_flag & WS_MEM_ALLOC)
                        {
                            if(NU_Deallocate_Memory(Req.ws_rdata.ws_in_data) != NU_SUCCESS)
                                NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
                        }

                        Req.ws_rdata.ws_in_data = NU_NULL;
                        Req.ws_rdata.ws_in_data_sz = 0;
                        Req.ws_rdata.ws_in_header_sz = 0;
                        memset( Req.ws_rdata.ws_in_header, 0x00, sizeof(Req.ws_rdata.ws_in_header) );
                        Req.ws_rdata.ws_next_in_data = NU_NULL;
                        Req.ws_rdata.ws_next_in_data_sz = 0;
                        Req.ws_rdata.ws_in_data_flag = 0;

                        /*  Check if connection should be closed */
                        if(hsock_ent->ws_http_ver == WS_HTTP_10 || status != NU_SUCCESS)
                        {
                            /*  Not HTTP 1.1 but HTTP 1.0 so close the socket and
                             *  remove it from the web socket cache.
                             */
                            WS_Remove_Socket_Entry(&http_sock, &free_sock, hsock_ent);
                        }
                        else
                        {
                            /*  This is HTTP 1.1, Get the current time to reset
                             *  the timeout on this socket.
                             */
                            hsock_ent->ws_time_click = WS_Get_Seconds;
                        }
    #if NU_WEBSERV_DEBUG
                        printf("REQUEST DONE\n");
    #endif
                    }
                }
            }
        }
        /* Allow for another task to run */
        NU_Relinquish();  

#if INCLUDE_FILE_SYSTEM
        NU_Close_Disk (nu_drive);
#endif

    }    /* (END for( ; ; ) loop */
}


/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WS_Receive_Data                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*      Receives data from the network and places it into the 
*      request structure.            
*                                                                      
* INPUTS                                                               
*                                                                      
*     WS_REQUEST        *req        Pointer to Request structure that   
*                                   holds all information pertaining to 
*                                   the HTTP request.                   
*     UINT8             mode        This is the action the function
*                                   will take.
* OUTPUTS                                                              
*                                                                      
*     STATUS                        NU_SUCCESS on a successful receive            
*                                                                      
************************************************************************/
STATIC STATUS WS_Receive_Data(WS_REQUEST *req)
{
    STATUS      status = NU_SUCCESS;
    INT32       bytes_recv;
    CHAR HUGE   *data_start = NU_NULL;
    CHAR        *size;
    CHAR        *temp_ptr;
    UINT32      con_len;
    UINT32      data_bytes;
    UINT32      temp_size;

        /* This check is for when a second request may have come in with
         * an earlier receive.
         */
        if(req->ws_rdata.ws_in_data_flag & WS_DATA_AVAIL)
        {
            NU_BLOCK_COPY(req->ws_rdata.ws_in_header, 
                          req->ws_rdata.ws_next_in_data,
                          (unsigned int)req->ws_rdata.ws_in_data_sz);

            req->ws_rdata.ws_in_header_sz = 0;
            data_start = WS_Find_Token(WS_CRLFCRLF, (CHAR HUGE*)req->ws_rdata.ws_in_header,
                                       &req->ws_rdata.ws_in_header[req->ws_rdata.ws_in_data_sz]);
        }
        
        /* Receive the header */
        if(!data_start)
            data_start = WS_Receive_Header(req);

        /* clear the flags for future use */
        req->ws_rdata.ws_in_data_flag &= ~(WS_DATA_AVAIL | WS_MEM_ALLOC);
        
        /* Was a complete header received? */
        if(!data_start)
            return(WS_FAILURE);

        /* Determine if this is a post operation.  If it is, then there
         * will be data attached
         */
        req->ws_method = (INT16)(req->ws_rdata.ws_in_header[0] + 
                                 req->ws_rdata.ws_in_header[1] + 
                                 req->ws_rdata.ws_in_header[2]);
        
        if(req->ws_method == WS_POST)
        {
            /* Search for the Content-Length header */
            size = WS_Find_Token_by_Case(WS_CONTENT_LENGTH, (CHAR HUGE*)req->ws_rdata.ws_in_header, 
                                        (CHAR*)data_start, WS_TOKEN_CASE_INSENS);

            if(!size)
            {
                /* If there is not a length header, the HTTP header must have
                 * been split.  This occurs in older Netscape browsers.
                 * Receive the rest of the header
                 */
                temp_ptr = (CHAR*)&req->ws_rdata.ws_in_header[req->ws_rdata.ws_in_header_sz];

                data_start = WS_Receive_Header(req);

                if(data_start)
                {
                    /* Search for the Content-Length header */
                    size = WS_Find_Token_by_Case(WS_CONTENT_LENGTH, temp_ptr, (CHAR*)data_start, WS_TOKEN_CASE_INSENS);
                }
            }

            if(size)
            {
                /* Increment Pointer to Content Length within the buffer */
                size += sizeof(WS_CONTENT_LENGTH) - 1;
        
                /* Get the length of the data */
                con_len = NU_ATOL(size);
                data_bytes = req->ws_rdata.ws_in_data_sz;
                
                /* Check if data will fit into the header array with null terminator */
                temp_size = req->ws_rdata.ws_in_header_sz + con_len;
                if(temp_size + 1 >= WS_RECEIVE_SIZE)
                {
                    /* Figure how much memory is needed.  If more than our macro
                     * is required, only allocate the macro, WS_MAX_RECV_BYTES
                     */
                    if(con_len > WS_MAX_RECV_BYTES)
                        req->ws_rdata.ws_in_data_sz = WS_MAX_RECV_BYTES;
                    else
                        req->ws_rdata.ws_in_data_sz = con_len;

                    /* Allocate enough memory to add null terminator to end */
                    status = NU_Allocate_Memory(WS_Master_Server.ws_memory_pool,
                                                (VOID**)&req->ws_rdata.ws_in_data,
                                                req->ws_rdata.ws_in_data_sz + 1, 
                                                NU_NO_SUSPEND);
                    if(status != NU_SUCCESS)
                    {
                        NERRS_Log_Error (NERR_INFORMATIONAL, __FILE__, __LINE__);
                        HTTP_Send_Status_Message(req, WS_PROTO_SERVER_ERROR,
                                                  "Server low on resources");
                        return WS_FAILURE;
                    }

                    req->ws_rdata.ws_in_data_flag |= WS_MEM_ALLOC;

                    /* Copy data from the header array into the new memory buffer */
                    WS_Mem_Cpy(req->ws_rdata.ws_in_data, data_start, data_bytes);
                }
                else
                {
                    /* Data will fit into the header array, set up the struct */
                    req->ws_rdata.ws_in_data = (CHAR*)data_start;
                    req->ws_rdata.ws_in_data_sz = con_len;
                }

                /* Receive any data that is out there */
                while(data_bytes < req->ws_rdata.ws_in_data_sz)
                {
                    bytes_recv = WSN_Read_Net(req, (CHAR*)(req->ws_rdata.ws_in_data + data_bytes),
                                              (UINT16)(req->ws_rdata.ws_in_data_sz - data_bytes),
                                              NU_NO_SUSPEND);
            
                    if (bytes_recv <= 0)
                        return WS_FAILURE;
            
                    /* Increment connection byte counter */
                    data_bytes += bytes_recv;
                }

                if(!(req->ws_rdata.ws_in_data_flag & WS_MEM_ALLOC) && 
                    (data_bytes > req->ws_rdata.ws_in_data_sz + 2))
                {
                    /* If memory was not allocated, it is possible that two
                     * requests came in on one receive.  Set up buffer pointers
                     * to test for this
                     */
                    req->ws_rdata.ws_next_in_data_sz = data_bytes - con_len;
                    req->ws_rdata.ws_next_in_data = data_start + con_len;

                    /* Netscape may append the data with extre "\r\n" tags.
                     * These tags are not added into the Content-Length field.
                     */
                    if(*req->ws_rdata.ws_next_in_data == '\r')
                        req->ws_rdata.ws_next_in_data += 2;

                    req->ws_rdata.ws_in_data_flag |= WS_DATA_AVAIL;
                }

                /* NULL terminate the input buffer for easy parsing later */
                req->ws_rdata.ws_in_data[req->ws_rdata.ws_in_data_sz] = NU_NULL;
            }
            else
                /* In case data was sent without the content size header */
                if(req->ws_rdata.ws_in_data_sz)
                    req->ws_rdata.ws_in_data = &req->ws_rdata.ws_in_header[req->ws_rdata.ws_in_header_sz];
            
        }
        else
        {
            /* Check if more than one request was received */
            if(req->ws_rdata.ws_in_data_sz)
                req->ws_rdata.ws_in_data_flag |= WS_DATA_AVAIL;
        }

    return status;
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WS_Receive_Header                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*      Receives the HTTP header.            
*                                                                      
* INPUTS                                                               
*                                                                      
*     WS_REQUEST        *req        Pointer to Request structure that   
*                                   holds all information pertaining to 
*                                   the HTTP request.                   
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     CHAR              *data_start Pointer to end of header                                    
*                                                                      
************************************************************************/
STATIC CHAR * WS_Receive_Header(WS_REQUEST *req)
{
    CHAR HUGE   *data_start;
    INT32       bytes_recv;
    UINT32      arr_index = req->ws_rdata.ws_in_header_sz + req->ws_rdata.ws_in_data_sz;

    do
    {
        bytes_recv = WSN_Read_Net(req, (CHAR*)&req->ws_rdata.ws_in_header[arr_index],
                                  (UINT16)(WS_RECEIVE_SIZE - arr_index),
                                  WS_TIMEOUT);
    
        /* Check for errors */
        if(bytes_recv <= 0)
            return(NU_NULL);
    
        /* Check if the full header was received */
        data_start = WS_Find_Token(WS_CRLFCRLF, &req->ws_rdata.ws_in_header[arr_index] - 4,
                                   &req->ws_rdata.ws_in_header[arr_index + bytes_recv]);
        
        /* Increment connection byte counter */
        arr_index += bytes_recv;
    
    }while(data_start == NU_NULL);

    /* Set pointer to begining of data, if any */
    data_start += 4;

    /* Set up the request data structure */
    req->ws_rdata.ws_in_data_sz = (UINT32)(&req->ws_rdata.ws_in_header[arr_index] - data_start);
    req->ws_rdata.ws_in_header_sz = (UINT32)(data_start - req->ws_rdata.ws_in_header);

    return((CHAR*)data_start);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WS_Clear_Rdata                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*      Clears the outgoing info in the request data structure.            
*                                                                      
* INPUTS                                                               
*                                                                      
*     WS_REQUEST        *req        Pointer to Request structure that   
*                                   holds all information pertaining to 
*                                   the HTTP request.                   
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None                                    
*                                                                      
************************************************************************/
VOID WS_Clear_Rdata(WS_REQUEST *req)
{
    /* Deallocate any allocated memory */
    if(req->ws_rdata.ws_out_data)
    {
        if(NU_Deallocate_Memory(req->ws_rdata.ws_out_data) != NU_SUCCESS)
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
        req->ws_rdata.ws_out_data = NU_NULL;
    }
    
    /* Clear the rdata structure */
    memset( req->ws_rdata.ws_out_header, 0x00, sizeof(req->ws_rdata.ws_out_header) );
    req->ws_rdata.ws_out_data_sz = 0;
    req->ws_rdata.ws_out_data_free = 0;
    req->ws_rdata.ws_no_head_flag = 0;
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WS_Set_Client_Addr                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*      Gets the IP address of the current client connection and
*      sets it in the request structure.            
*                                                                      
* INPUTS                                                               
*                                                                      
*     WS_REQUEST        *Req        Pointer to Request structure that   
*                                   holds all information pertaining to 
*                                   the HTTP request.                   
*
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
************************************************************************/

STATIC VOID WS_Set_Client_Addr(WS_REQUEST *Req)
{
    INT                     i;
    struct sockaddr_struct  client_addr;
    INT16                   size;
    
    /* Get the IP of the client */
    size = sizeof (struct sockaddr_struct);
    if(NU_Get_Peer_Name(Req->ws_sd, &client_addr,&size) != NU_SUCCESS)
    {
#if NU_WEBSERV_DEBUG
        printf("BAD Peer NAME\n");
#endif 
        NERRS_Log_Error (NERR_INFORMATIONAL, __FILE__, __LINE__);
    }
    else 
    {    
        /* Copy IP into req structure */
        if (Req->ws_family == NU_FAMILY_IP)
        {
            for( i = 0; i < IP_ADDR_LEN; i++)
                Req->ws_ip[i] = client_addr.ip_num.is_ip_addrs[i];
        }

#if ((defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE))
        else
        {
            for( i = 0; i < IP6_ADDR_LEN; i++)
                Req->ws_ip[i] = client_addr.ip_num.is_ip_addrs[i];
        }
#endif
    }
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WS_Create_Socket_List                                       
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Creates an initialized linked list used for holding socket
*     information
*                                                                      
* INPUTS                                                               
*                                                                      
*     free_sock               Pointer to the socket list
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     status                  NU_SUCCESS on successful creation of
*                             the list                                    
*                                                                      
************************************************************************/

STATIC STATUS WS_Create_Socket_List(WS_SOCK_LIST *free_sock)
{
    WS_SOCKET_STRUCT      *hsock_ent;
    INT                   list_node;
    STATUS                status = NU_SUCCESS;

    if(NU_Allocate_Memory(WS_Master_Server.ws_memory_pool, (VOID **)&hsock_ent,
                          (sizeof(WS_SOCKET_STRUCT)) * WS_LOCAL_QUEUE_SIZE,
                          NU_SUSPEND) == NU_SUCCESS)
    {
        for(list_node = 0; list_node < WS_LOCAL_QUEUE_SIZE; list_node++)
        {
            /* Initialize the free node */
            hsock_ent->ws_http_ver      = -1;
            hsock_ent->ws_socketd       = -1;
            hsock_ent->ws_time_click    = 0;

            /* Add the free node to the list of free nodes */
            DLL_Enqueue((tqe_t *) free_sock, (tqe_t *) hsock_ent);
        
            /* Increment pointer to next free space in memory */
            hsock_ent++;
        }
    }
    else
    {
        NERRS_Log_Error (NERR_FATAL, __FILE__, __LINE__);
        status = WS_FAILURE;
    }

    return (status);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WS_Add_Socket_Entry                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Adds a socket entry into the doubly linked list.  The node that
*     is added comes from a list of already created and initialized nodes.
*                                                                      
* INPUTS                                                               
*                       
*     http_sock                     Pointer to the working socket list.
*     free_sock                     Pointer to the list containing the
*                                    free nodes.
*     socket                        The actual socket to be placed in
*                                    in the list.
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
************************************************************************/

STATIC VOID WS_Add_Socket_Entry(WS_SOCK_LIST *http_sock, WS_SOCK_LIST *free_sock, 
                                WS_SOCKET *socket_desc)
{
    struct sockaddr_struct  addr;
    INT16                   addr_length = sizeof(struct sockaddr_struct);
    STATUS                  status;

    /* Set the Values and add the entry into the socket linked list */
    free_sock->ws_sock_list_head->ws_socketd = socket_desc->ws_new_socket;
    free_sock->ws_sock_list_head->ws_time_click = WS_Get_Seconds;

    free_sock->ws_sock_list_head->ws_family = socket_desc->ws_family;
    
    /*  We now must get the IP address of the device the connection was made on */
    status = NU_Get_Sock_Name(socket_desc->ws_new_socket, &addr, &addr_length);

    if(status == NU_SUCCESS)
    {
        if (socket_desc->ws_family == NU_FAMILY_IP)
            memcpy(free_sock->ws_sock_list_head->ws_ip, addr.ip_num.is_ip_addrs, 4);

#if ((defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE))
        else
            memcpy(free_sock->ws_sock_list_head->ws_ip, addr.ip_num.is_ip_addrs, 16);
#endif
        
#if INCLUDE_SSL
        /* Check if this is coming through the SSL port */
        if(addr.port_num == WS_SSL_PORT)
        {
            /* If it is begin SSL handshake */
            status = WS_SSL_Accept((SSL**)&(free_sock->ws_sock_list_head->ws_ssl_ptr),
                                   socket_desc->ws_new_socket, NU_SSL_CTX);
            
            /* If the handshake was not successful, close the socket */
            if(status != NU_SUCCESS)
            {
                if(NU_Close_Socket(socket_desc->ws_new_socket) != NU_SUCCESS)
                    NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
        else
            free_sock->ws_sock_list_head->ws_ssl_ptr = NU_NULL;
#else
        free_sock->ws_sock_list_head->ws_ssl_ptr = NU_NULL;
#endif /* INCLUDE_SSL */
    }

    if(status == NU_SUCCESS)
        /* Add this entry to the list. */
        DLL_Enqueue(http_sock, DLL_Dequeue(free_sock));
    else
        NERRS_Log_Error(NERR_INFORMATIONAL, __FILE__, __LINE__);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WS_Remove_Socket_Entry                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Removes a socket entry from the doubly linked list.  The
*     removed node is placed back onto the free node list.
*                                                                      
* INPUTS                                                               
*                                                                      
*     http_sock                     Pointer to the working socket list.
*     free_sock                     Pointer to the list containing the
*                                    free nodes.
*     hsock_ent                     Pointer to node which will be removed
*                                    from the list.
*                                                                      
* OUTPUTS                                                              
*                                                                      
*                                         
*                                                                      
************************************************************************/

STATIC VOID WS_Remove_Socket_Entry(WS_SOCK_LIST *http_sock, WS_SOCK_LIST *free_sock, WS_SOCKET_STRUCT *hsock_ent)
{

#if INCLUDE_SSL
    if(hsock_ent->ws_ssl_ptr)
    {
        SSL_shutdown((SSL*)(hsock_ent->ws_ssl_ptr));
        SSL_free((SSL*)(hsock_ent->ws_ssl_ptr)); 
        hsock_ent->ws_ssl_ptr   = NU_NULL;
    }
#endif /* INCLUDE_SSL */

    if(NU_Close_Socket(hsock_ent->ws_socketd) != NU_SUCCESS)
        NERRS_Log_Error (NERR_RECOVERABLE, __FILE__, __LINE__);

    /* Reinitialize the node */
    hsock_ent->ws_http_ver      = -1;
    hsock_ent->ws_socketd       = -1;
    hsock_ent->ws_time_click    = 0;

    /* Remove the node from the list of active sockets */
    DLL_Remove(http_sock, hsock_ent);

    /* Add the node the list of free nodes */
    DLL_Enqueue(free_sock, hsock_ent);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WS_Strn_Cpy                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This function is an intermediary between WebServ and
*     strncpy().  It was created to prevent execution differences
*     between 16 bit and 32 bit proccessors.
*                                                                      
* INPUTS                                                               
*                                                                      
*     target            Where the string will be copied to.
*     buffer            The string that is to be copied.
*     size              32 bit size of the buffer
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None            
*                                                                      
************************************************************************/
VOID WS_Strn_Cpy(CHAR HUGE * target, CHAR HUGE * buffer, UINT32 size)
{
    /* If this is a 16 bit machine, send buffer in chunks */
    target = TLS_Normalize_Ptr(target);
    buffer = TLS_Normalize_Ptr(buffer);
    
    while(UINT_MAX < size)
    {
        /* Copy only the maximum allowed by an int */
        strncpy((CHAR*)target, (CHAR*)buffer, (unsigned int)UINT_MAX);
        size -= UINT_MAX;
        target += UINT_MAX;
        buffer += UINT_MAX;
    }
    
    /* Be sure there is something to copy */
    if(size)
        strncpy((CHAR*)target, (CHAR*)buffer, (unsigned int)size);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     WS_Mem_Cpy                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This function is an intermediary between WebServ and
*     memcpy().  It was created to prevent execution differences
*     between 16 bit and 32 bit proccessors.
*                                                                      
* INPUTS                                                               
*                                                                      
*     target            Where the string will be copied to.
*     buffer            The string that is to be copied.
*     size              32 bit size of the buffer
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None            
*                                                                      
************************************************************************/

VOID WS_Mem_Cpy(CHAR HUGE * target, CHAR HUGE * buffer, UINT32 size)
{
    /* If this is a 16 bit machine, send buffer in chunks */
    target = TLS_Normalize_Ptr(target);
    buffer = TLS_Normalize_Ptr(buffer);
    
    while(UINT_MAX < size)
    {
        /* Copy only the maximum allowed by an int */
        NU_BLOCK_COPY((CHAR*)target, (CHAR*)buffer, (unsigned int)UINT_MAX);
        size -= UINT_MAX;
        target += UINT_MAX;
        buffer += UINT_MAX;
    }
    
    /* Be sure there is something to copy */
    if(size)
        NU_BLOCK_COPY((CHAR*)target, (CHAR*)buffer, (unsigned int)size);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       WS_In_String                                                        
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to find a string in a string.                      
*                                                                       
* INPUTS                                                                
*                                                                       
*       *target                 The string sequence to be looked 
*                               for.                             
*       *string_buf             The string in which the sequence is  
*                               to be searched for.              
*                                                                       
* OUTPUTS                                                               
*              
*       string_buf              Found the string
*       NU_NULL                 Could not find the string                                                         
*                                                                       
*************************************************************************/
CHAR *WS_In_String(CHAR *target, CHAR HUGE *string_buf)
{
    unsigned int  len = strlen(target);
    
    string_buf = (CHAR HUGE*)TLS_Normalize_Ptr(string_buf);
    
    for(;*string_buf; string_buf++)
    {
        if (*target == *string_buf && strncmp(target, (CHAR*)string_buf, len) == 0) 
            return (CHAR *)string_buf;
    }
    
    return(NU_NULL);
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       WS_Find_Token                                                       
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Return the position in the second             
*       string where an occurence of the first string starts.            
*                                                                       
* INPUTS                                                                
*                                                                       
*       *token                  Token string to be found.        
*       *file                   File to look for the token in.   
*       *last                   Size of the file.                
*       mode                    Case sensitivity flag.
*                                                                       
* OUTPUTS                                                               
*
*       file                    Found the token                                                                       
*       NU_NULL                 Could not find the token
*                                                                       
*************************************************************************/
CHAR* WS_Find_Token_by_Case(CHAR *token, CHAR HUGE *file, CHAR *last, UINT8 mode)
{
    unsigned int    len = strlen(token);
    unsigned int    count;
    CHAR HUGE       *s1;
    CHAR HUGE       *s2;

    last = TLS_Normalize_Ptr(last);

    if(mode == WS_TOKEN_CASE_INSENS)
    {
        for(;file < (CHAR HUGE*)last; file++)
        {
            s1 = TLS_Normalize_Ptr(token);
            s2 = TLS_Normalize_Ptr(file);

            for(count = 1; 
                (NU_TOUPPER(*s1) == NU_TOUPPER(*s2)) && count < len;
                ++s1, ++s2, ++count);
        
            if(NU_TOUPPER(*s1) == NU_TOUPPER(*s2) && count == len)
                return((CHAR*)file);
        }  
    }
    else
    {
        for(;file < (CHAR HUGE*)last; file++)
        {
            if (*token == *file && strncmp(token, (CHAR*)file, len) == 0) 
                return (CHAR *)file;
        }
    }

    return(NU_NULL);  
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       HTTP_Register_Plugin                                                    
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Function to register a plugin within the plugin structure.  It   
*       takes the plugin function name and uri name and places it within 
*       this plugin structure.  Every plugin must be registered or the   
*       web server will not be able to recognize the plugin when it is   
*       called within an HTML file.                                      
*                                                                       
* INPUTS                                                                
*                                                                       
*       plug_in                 The name of the plugin function.  
*       uri                     The string used to identify       
*                               plugin.     
*       flags                   Flags that pertain to plugin
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/

VOID  WS_Register_Plugin(INT (*plug_in)(WS_REQUEST *), CHAR *uri, UINT8 flags)
{
    INT                 total = 0;
    WS_PLUGIN_STRUCT    *pointer;
    STATUS              status = WS_FAILURE;

    /* Setup MMU if it is present */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    if(strlen(uri) < WS_PLUGIN_NAME_LEN)
    {
        /* Check if the plugin list has been created */
        if(!HTTP_Plugins)
        {
            status = NU_Allocate_Memory(WS_Master_Server.ws_memory_pool, 
                                        (VOID**)&HTTP_Plugins, 
                                        sizeof(WS_PLUGIN_STRUCT), NU_NO_SUSPEND);
            pointer = HTTP_Plugins;
        }
        else
        {    
            pointer = HTTP_Plugins;

            while(pointer->ws_next != NU_NULL)
            {
                pointer = pointer->ws_next;
                total++;
            }

            if(total < WS_MAX_PLUGIN)
            {
                status = NU_Allocate_Memory(WS_Master_Server.ws_memory_pool, 
                                            (VOID**)&pointer->ws_next, 
                                             sizeof(WS_PLUGIN_STRUCT), NU_NO_SUSPEND);
                if(status == NU_SUCCESS)
                    pointer = pointer->ws_next;
            }
        }
    }

    if(status == NU_SUCCESS)
    {
        pointer->ws_next = NU_NULL;
        strncpy(pointer->ws_name, uri, WS_PLUGIN_NAME_LEN);
        pointer->plugin = plug_in;
        pointer->ws_plg_flag = flags;
    }
    else    
    {
#if NU_WEBSERV_DEBUG
        printf("Failed to register plugin.\n");
#endif 
        NERRS_Log_Error (NERR_SEVERE, __FILE__, __LINE__);
    }

    NU_USER_MODE();
}

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       WS_Set_Auth_Callback                                                    
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       This function sets up the function that will be called for
*       each authentication that occurs successfully.                                      
*                                                                       
* INPUTS                                                                
*                                                                       
*       function            Pointer to the function to be called                           
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
#if WS_AUTHENTICATION
VOID WS_Set_Auth_Callback(VOID (*function)(WS_AUTH*))
{
    /* Setup MMU if it is present */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();
    
    WS_Master_Server.ws_master.ws_callback = function;

    NU_USER_MODE();
}
#endif

/************************************************************************
*                                                                       
* FUNCTION                                                              
*                                                                       
*       WS_Cleanup_Server                                                        
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Removes the presence of the server from the operating system. 
*                                                                       
* INPUTS                                                                
*                                                                       
*       None
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/

VOID WS_Cleanup_Server(VOID)
{
    UINT16      task_num;
#if INCLUDE_INITIAL_FILES && INCLUDE_FILE_SYSTEM
    WS_FS_FILE * file_ptr;
#endif

    /* Setup MMU if it is present */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();
    
    
#if NU_WEBSERV_DEBUG
    printf("Initialization failed, killing the server.\n");
#endif 
    
    /* Work backwards to clean up the server*/
#if INCLUDE_SSL
    /* Shutdown SSL */
    NU_Terminate_Task(&NU_SSL_Receive_CB);
    NU_Delete_Task(&NU_SSL_Receive_CB);
#endif

    /* Clean up the worker tasks */
    for(task_num = 0; task_num < WS_MAX_WORKER_TASKS; task_num++)
    {
        NU_Terminate_Task(&WS_Servers[task_num]);
        NU_Delete_Task(&WS_Servers[task_num]);
    }

    /* Clean up queue */
    NU_Delete_Queue(&WS_Socket_Queue);

    /* Clean up receive task */
    NU_Terminate_Task(&WS_HTTP_Receive_CB);
    NU_Delete_Task(&WS_HTTP_Receive_CB);
    
#if INCLUDE_INITIAL_FILES && INCLUDE_FILE_SYSTEM    
    /* Clean up the file system if one is included */
    file_ptr = Embed_Fs;
    for(; file_ptr->ws_addr; file_ptr++)
    {
        NU_Delete(&file_ptr->ws_name[1]);
    }
#endif

    NU_USER_MODE();
}

