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
*       tftps.c                                        
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package  -  Nucleus TFTP Server
*
*   DESCRIPTION
*
*       This file contains the TFTP routines necessary to implement
*       a TFTP server.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TFTP_Server_Init                Initializes TFTP Server.
*       TFTP_Server_Task                Process TFTP Server.
*       TFTPS_Init_Control_Block        Initializes the TFTP control block.
*       TFTPS_Process_Write_Request     Processes a write request.
*       TFTPS_Process_Read_Request      Processes a read request.
*       TFTPS_Cleanup_Buffers           Cleans up the buffers used.
*       TFTPS_Recv                      Recv a packet.
*       TFTPS_RRecv                     Recv a request packet.
*       TFTPS_Process_Data              Process a data packet.
*       TFTPS_Process_Request_PACKET    Process a request packet.
*       TFTPS_Ack                       Send a TFTP ack.
*       TFTPS_Process_Ack               Process an ack packet.
*       TFTPS_Send_Data                 Send a TFTP data packet.
*       TFTPS_Retransmit                Retransmit the last TFTP packet.
*       TFTPS_Error                     Send a TFTP error packet.
*       TFTPS_Check_Options             Verifies/Sets Client options.
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_net.h
*       nu_tftps.h
*       tftps_cfg.h
*       ip6.h
*       tftpdefs.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/nu_networking.h"
#include "networking/nu_tftps.h"
#include "networking/tftps_cfg.h"

#if ((defined(NET_5_1)) && (!defined(NET_5_2)) && (INCLUDE_IPV6 == NU_TRUE))
#include "networking/ip6.h"
#endif



#define TFTP_TASK_SIZE      CFG_NU_OS_NET_PROT_TFTP_SERVER_STACK_SIZE
#define TFTP_PRIORITY       3

NU_MEMORY_POOL  *TFTPS_Memory;
TFTPS_CB         TFTPS_Con;
VOID            *TFTPS_Task_Ptr;
NU_SEMAPHORE     TFTPS_Resource;

/*  TFTP defines 9 error codes, with values 0 through 8.  In the event that one
*  of these errors is received it will be passed back to the application.  Since
*  a value of 0 is defined to be NU_SUCCESS in the Nucleus PLUS OS it is
*  necessary to redefine the first error code, and since the convention at ATESD
*  is to return negative numbers for error codes all were redefined below.
*
*  VALUE       ERROR
*  -100        Not defined, see error message (if any)
*  -101        File not found
*  -102        Access violation
*  -103        Disk full or allocation exceeded
*  -104        Illegal TFTP operation
*  -105        Unknown transfer ID
*  -106        File already exists
*  -107        No such user
*  -108        Bad TFTP Option
*/

/* TFTP defines 9 error codes, with values 0 through 8.  In the event that one
 * of these errors is received it will be passed back to the application.  Since
 * a value of 0 is defined to be NU_SUCCESS in the Nucleus PLUS OS it is
 * necessary to redefine the first error code, and since the convention at
 * Mentor Graphics is to return negative numbers for error codes all were
 * redefined. */
static const INT16  tftps_errors[] = {TFTP_ERROR, TFTP_FILE_NFOUND, TFTP_ACCESS_VIOLATION,
                                      TFTP_DISK_FULL, TFTP_BAD_OPERATION, TFTP_UNKNOWN_TID,
                                      TFTP_FILE_EXISTS, TFTP_NO_SUCH_USER, TFTP_BAD_OPTION};


/*      Start TFTP Negotiations and Setup buffers  */

VOID TFTPS_Process_Read_Request(INT file_desc);
VOID TFTPS_Process_Write_Request(INT file_desc);
VOID TFTPS_Init_Control_Block(UINT32 dev_addr);
#if (TFTPS_RFC2347_COMPLIANT == NU_TRUE)
VOID TFTPS_Cleanup_Buffers(VOID);
#endif

NU_TASK tftp_server_task_ptr;

/* String to store registry path for tftps */
CHAR                TFTPS_Registry_Path[REG_MAX_KEY_LENGTH] = {0};

/* this variable is used in TFTP_UNUSED_PARAMETER macro
   defined in tftpdefs.h.  The macro is used to remove
   compilation warnings */
UINT32 TFTP_Unused_Parameter;

/* Variable use to keep track of init state of server */
static BOOLEAN TFTP_Server_Running = NU_FALSE;


/************************************************************************
*
*   FUNCTION
*
*       TFTP_Is_Server_Running
*
*   DESCRIPTION
*
*       Returns NU_TRUE if the server is running and NU_FALSE
*       otherwise
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_TRUE if server is running
*       NU_FALSE if server is not running
*
************************************************************************/
BOOLEAN TFTP_Is_Server_Running(VOID)
{
    /* Return internal flag */
    return (TFTP_Server_Running);
}


/************************************************************************
* FUNCTION
*
*       nu_os_net_prot_tftp_server_init
*
* DESCRIPTION
*
*       This function initializes the Nucleus XPROT TFTP Server
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
*       TFTPS_INVALID_PARAMS    Invalid parameter(s).
*       <other> -               Indicates (other) internal error occured.
*
************************************************************************/
STATUS nu_os_net_prot_tftp_server_init(CHAR *path, INT startstop)
{
    STATUS              status = -1;

    if(path != NU_NULL)
    {
        /* Save a copy locally. */
        strcpy(TFTPS_Registry_Path, path);
    }

    if(startstop)
    {
        /* Initialize the Nucleus TFTP Server component. By passing zero we
         * initialize it for all interfaces 
         */
        status = TFTP_Server_Init(0);

        /* Set flag */
        TFTP_Server_Running = NU_TRUE;

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Error at call to TFTP_Server_Init().\n",
                        NERR_FATAL, __FILE__, __LINE__);
        }
    }
    else
    {
        /* Stop requested */
        TFTP_Server_Uninit();

        /* Clear flag */
        TFTP_Server_Running = NU_FALSE;

        status = NU_SUCCESS;
    }

    return (status);
} /* nu_os_net_prot_tftps_init */

/************************************************************************
*
*   FUNCTION
*
*       TFTP_Server_Init
*
*   DESCRIPTION
*
*       This function starts the TFTP server and processes both read
*       and write requests.  It reads and writes data from/to an
*       in-memory file system.  Note: This supports only octet (binary)
*       mode of file transfer.
*
*   INPUTS
*
*       dv_name     Device where Server IP address is attached.
*
*   OUTPUTS
*
*       NU_SUCCESS when successful, else various error
*       codes are returned.
*
************************************************************************/
STATUS TFTP_Server_Init(CHAR *dv_name)
{
    STATUS              status = NU_SUCCESS;
    UINT32              dev_addr = 0;
    SCK_IOCTL_OPTION    option;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */
	
    memset (option.s_ret.s_ipaddr,0,sizeof(UINT8)*MAX_ADDRESS_SIZE);

#ifdef NET_5_1
    TFTPS_Memory = MEM_Cached;
#else
    TFTPS_Memory = &System_Memory;
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    if (dv_name)
    {
        option.s_optval = (UINT8*)dv_name;

        /* Get the IP address of the interface */
        status = NU_Ioctl(IOCTL_GETIFADDR, &option, sizeof(SCK_IOCTL_OPTION));

        if (status == NU_SUCCESS)
            dev_addr = IP_ADDR(option.s_ret.s_ipaddr);

        else
            status = NU_INVALID_PARM;
    }

    else
#endif
        dev_addr = NU_NULL;

    if (status == NU_SUCCESS)
    {
        /* Create TFTP_Server_Task.  */
        status = NU_Allocate_Memory(TFTPS_Memory, &TFTPS_Task_Ptr,
                                    (UNSIGNED)TFTP_TASK_SIZE,
                                    (UNSIGNED)NU_NO_SUSPEND);

        if (status != NU_SUCCESS)
        {
#ifdef NU_DEBUG_TFTP_SERVER
            printf ("Cannot create memory for TFTP_Server_Task.\n");
#endif
        }
        else
        {
            status = NU_Create_Task(&tftp_server_task_ptr, "TFTPSERV",
                TFTP_Server_Task,
                dev_addr, NU_NULL,
                TFTPS_Task_Ptr, (UNSIGNED)TFTP_TASK_SIZE,
                TFTP_PRIORITY, (UNSIGNED)0,
                NU_PREEMPT, NU_NO_START);

            if (status != NU_SUCCESS)
            {
#ifdef NU_DEBUG_TFTP_SERVER
                printf ("Cannot create TFTP_Server_Task\n\r");
#endif
            }
            else
            {
                /* Create synchronization semaphore.  */
                status = NU_Create_Semaphore(&TFTPS_Resource, "TFTP", (UNSIGNED)1, NU_FIFO);
                if (status == NU_SUCCESS)
                {
                    NU_Resume_Task(&tftp_server_task_ptr);
                }
            } /* TFTP Server Task created successfully */
        } /* Memory allocated for TFTP_Server_Task */
    }

    NU_USER_MODE();    /* return to user mode */

    return (status);
}

/************************************************************************
*
*  FUNCTION
*
*      TFPTS_Disk_Space
*
*  DESCRIPTION
*
*      Returns the amount of space available on the current disk.
*
*  INPUTS
*
*      None
*
*  OUTPUTS
*
*      The amount of free space on the current disk.
*
************************************************************************/

STATIC UINT32 TFPTS_Disk_Space(VOID)
{
    CHAR    drive[3];
    UINT32  free_clusters[1];
    UINT32  total_clusters[1];
    UINT8   sectors_per_cluster[1];
    UINT16  bytes_per_sector[1];


    drive[0] = (CHAR)('A' + NU_Get_Default_Drive());
    drive[1] = ':';
    drive[2] = '\0';


    if (NU_FreeSpace (drive, (UINT8*)sectors_per_cluster,
                 (UINT16*)bytes_per_sector, free_clusters, total_clusters) < 0)
        return (0);

    return ((*sectors_per_cluster) * (*bytes_per_sector) * (*free_clusters));
}

/******************************************************************************
*
*   FUNCTION
*
*      TFTPS_Wait_For_FS
*
*   DESCRIPTION
*
*      Wait for a given drive to be initialized.
*
*   INPUTS
*
*      mount_point            drive name to be waited for mounting.
*
*   OUTPUTS
*
*      NU_SUCCESS             File System has been successfully initialized
*         < 0                 File System initialization error
*
*******************************************************************************/
STATUS TFTPS_Wait_For_FS(CHAR* mount_point)
{
    STATUS          status;
    
    /* Suspend until a File device is mounted */
    status = NU_Storage_Device_Wait(mount_point, NU_SUSPEND);
                 
    return (status);
}

/************************************************************************
*
*   FUNCTION
*
*       TFTP_Server_Task
*
*   DESCRIPTION
*
*       This function starts the TFTP server and processes both read
*       and write requests.  It reads and writes data from/to an
*       in-memory file system.  Note: This supports only octet (binary)
*       mode of file transfer.
*
*   INPUTS
*
*       dv_name     Device where Servers IP address is attached
*
*   OUTPUTS
*
*       NU_SUCCESS when successful, else various error
*       codes are returned.
*
************************************************************************/
VOID TFTP_Server_Task(UNSIGNED dev_addr, VOID *argv)
{
    INT      file_desc;
    INT32    bytes_received;
    CHAR     nu_drive[3];
    STATUS   status;


#if (NU_ENABLE_NOTIFICATION == NU_TRUE)
    NET_NTFY_Debug_Struct ntfy_struct;
    ntfy_struct.net_ntfy_type = NET_NTFY_GENERAL;
#endif

    /* Remove warnings for unused parameters. */
    TFTP_UNUSED_PARAMETER(argv);

    nu_drive[0] = (CHAR)(TFTPS_DEFAULT_DRIVE + 'A');
    nu_drive[1] = ':';
    nu_drive[2] = '\0';

    /* Check to see if the File System has been initialized. */
    status = TFTPS_Wait_For_FS(nu_drive);

    if(status == NU_SUCCESS)
    {
        status = NU_Become_File_User();

        if (status == NU_SUCCESS)
        {
            /* Open disk for accessing. */

            status = NU_Open_Disk (nu_drive);

            /* If the disk is already open, return success and leave the current
             * directory where it is.
             */
            if (status == NUF_NO_ACTION)
                status = NU_SUCCESS;

            if (status == NU_SUCCESS)
            {
                /* Set the default drive */
                status = NU_Set_Default_Drive(TFTPS_DEFAULT_DRIVE);
            }
#ifdef NU_DEBUG_TFTP_SERVER
            else
                printf ("Task cannot open disk as Nucleus FILE User\n");
#endif
        }
#ifdef NU_DEBUG_TFTP_SERVER
        else
            printf("Task cannot register as Nucleus FILE User\n");
#endif

        if (status != NU_SUCCESS)
        {
            return;
        }
    }

    /*  Set Current Directory to "\" */
    status = NU_Set_Current_Dir("\\");
    if (status != NU_SUCCESS)
    {
#ifdef NU_DEBUG_TFTP_SERVER
        printf("TFTP Task cannot set current directory as Nucleus FILE User\n");
#endif
        return;
    }

    /* Initialize the TFTP control block */
    TFTPS_Init_Control_Block((UINT32)dev_addr);

    /* Receive a message from a client and don't return until data
     * is received from the socket or until server uninitialize
     * request is activated.
     */
    for (;;)
    {
        /* Initialize the buffer size fields of the TFTP CB. */
        TFTPS_Con.options.tsize = TFPTS_Disk_Space();
        TFTPS_Con.options.blksize = TFTP_BLOCK_SIZE_DEFAULT;
        TFTPS_Con.options.timeout = TFTP_TIMEOUT_DEFAULT;

        bytes_received = TFTPS_RRecv();

        if (bytes_received > 0)
        {
            /* Grab the stack semaphore before processing packets. */
            status = NU_Obtain_Semaphore(&TFTPS_Resource, NU_SUSPEND);

            /* Verify that resource was available */
            if (status == NU_SUCCESS)
            {

                /* Process the Incoming Packet from the TFTP Client */
                file_desc = TFTPS_Process_Request_PACKET(bytes_received);

                if (file_desc >= 0)
                {
                    /* If the client is requesting to write to the server */
                    if (TFTPS_Con.type == WRITE_TYPE)
                        TFTPS_Process_Write_Request(file_desc);

                    /* Else the client is requesting to read a file from the server */
                    else if (TFTPS_Con.type == READ_TYPE)
                        TFTPS_Process_Read_Request(file_desc);
                }

#if (TFTPS_RFC2347_COMPLIANT == NU_TRUE)

                /* Clean up the buffers that were used for the transmission */
                TFTPS_Cleanup_Buffers();

#endif

                /* Let other tasks use the TFTP Resource */
                if (NU_Release_Semaphore(&TFTPS_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release TFTP semaphore", NERR_SEVERE,
                        __FILE__, __LINE__);
#ifdef NET_5_2
                    NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                        NU_Current_Task_Pointer(), NU_NULL);
#endif
                } /* Resource released */
            } /* Obtained Semaphore */
        } /* Bytes received > 0 */

        /* TFTP Server Termination in effect */
        else
        {

#if (NU_ENABLE_NOTIFICATION == NU_TRUE)
            NET_Notify(TFTPS_SERVER_TERMINATED, __FILE__, __LINE__,
                NU_NULL, &ntfy_struct);
#endif
            break;

        } /* Bytes received <= 0 */
    }  /*  End While (1) loop */

    /*  Close disk and de-register from File. */

    NU_Close_Disk (nu_drive);

    NU_Release_File_User();

} /* end TFTP_Server_Task */

/************************************************************************
*
*   FUNCTION
*
*       TFTPS_Init_Control_Block
*
*   DESCRIPTION
*
*       This function initializes the members of the TFTP Server
*       control block and binds the server to the TFTP port.
*
*   INPUTS
*
*       dev_addr        The IP address to use for the server.
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID TFTPS_Init_Control_Block(UINT32 dev_addr)
{
    STATUS  status;
    INT16   family;

    /* If an IP address was specified, the TFTP Server should be bound
     * to that address and only process IPv4 requests; otherwise, bind
     * to the WILDCARD address and process IPv4 and IPv6 requests.
     */
#if ( (defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE) )
    if (dev_addr == 0)
        family = NU_FAMILY_IP6;
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
        family = NU_FAMILY_IP;
#endif

    /* Clear TFTPS_Con structure before use */
    UTL_Zero(&TFTPS_Con, (UINT32)(sizeof(TFTPS_Con)));

    /* Set Socket number and Session Socket number to invalid */
    TFTPS_Con.socket_number = -1;
    TFTPS_Con.session_socket_number = -1;

    /* Attempt to allocate memory for the output buffer until successful */
    do
    {
        /* Allocate memory for the initial request from the client */
        if ((status = NU_Allocate_Memory(TFTPS_Memory,
                                         (VOID*)&(TFTPS_Con.trans_buf),
                                         TFTP_BLOCK_SIZE_DEFAULT +
                                         (UNSIGNED)TFTP_HEADER_SIZE,
                                         NU_SUSPEND)) != NU_SUCCESS)
        {
#ifdef NU_DEBUG_TFTP_SERVER
            printf ("Cannot create memory for the output buffer.\n");
#endif
#ifdef NET_5_2
            NET_Sleep(SCK_Ticks_Per_Second);
#else
            NU_Sleep(SCK_Ticks_Per_Second);
#endif
        }
    } while (status != NU_SUCCESS);

    /* Attempt to allocate memory for the input buffer until successful */
    do
    {
        /* Allocate memory for the initial request from the client */
        if ((status = NU_Allocate_Memory(TFTPS_Memory,
                                         (VOID*)&(TFTPS_Con.tftp_input_buffer),
                                         TFTP_BLOCK_SIZE_DEFAULT +
                                         (UNSIGNED)TFTP_HEADER_SIZE,
                                         NU_SUSPEND)) != NU_SUCCESS)
        {
#ifdef NU_DEBUG_TFTP_SERVER
            printf ("Cannot create memory for the input buffer.\n");
#endif
#ifdef NET_5_2
            NET_Sleep(SCK_Ticks_Per_Second);
#else
            NU_Sleep(SCK_Ticks_Per_Second);
#endif
        }
    } while (status != NU_SUCCESS);

    /* Attempt to create a socket until successful */
    do
    {
        /* Create a socket */
        TFTPS_Con.socket_number = (INT16)NU_Socket(family,
                                                   NU_TYPE_DGRAM, 0);

        if (TFTPS_Con.socket_number < 0)
#ifdef NET_5_2
            NET_Sleep(SCK_Ticks_Per_Second);
#else
            NU_Sleep(SCK_Ticks_Per_Second);
#endif
    } while (TFTPS_Con.socket_number < 0);

    /* Fill in a structure with the server address */
    TFTPS_Con.server_addr.family = family;
    TFTPS_Con.server_addr.port = CFG_NU_OS_NET_PROT_TFTP_SERVER_UDP_PORT;
    TFTPS_Con.server_addr.name = "tftp";

#if ( (defined(NET_5_1)) && (INCLUDE_IPV6 == NU_TRUE) )
    if (family == NU_FAMILY_IP6)
        TFTPS_Con.server_addr.id = IP6_ADDR_ANY;
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        TFTPS_Con.server_addr.id.is_ip_addrs[0]  =
            (UINT8) ((dev_addr >> 24) & 0x000000FF);
        TFTPS_Con.server_addr.id.is_ip_addrs[1]  =
            (UINT8) ((dev_addr >> 16) & 0x000000FF);
        TFTPS_Con.server_addr.id.is_ip_addrs[2]  =
            (UINT8) ((dev_addr >> 8) & 0x000000FF);
        TFTPS_Con.server_addr.id.is_ip_addrs[3]  =
            (UINT8) (dev_addr & 0x000000FF);
    }
#endif

    /* Bind our address to the socket. */
    NU_Bind(TFTPS_Con.socket_number, &(TFTPS_Con.server_addr), 0);
}

/************************************************************************
*
*   FUNCTION
*
*       TFTPS_Process_Write_Request
*
*   DESCRIPTION
*
*       This function processes an incoming write request.
*
*   INPUTS
*
*       file_dest       The file descriptor associated with the
*                       file being written to the server.
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID TFTPS_Process_Write_Request(INT file_desc)
{
    INT32   bytes_received;
    INT16   retransmits;
    STATUS  status;

    /*  Set the Length of the buffer. */
    TFTPS_Con.cur_buf_size = TFPTS_Disk_Space();

    retransmits = 0;

    /* Current State:
     *
     * client: WRQ --->
     *                 <--- server: ACK or OACK (if confirmed options)
     *
     * The server is now expecting the client to send a DATA packet,
     * or an ERROR packet.  If a DATA or ERROR are not received within
     * the timeout period, the server will retransmit the ACK (or OACK)
     * packet.
     */

    /* Retransmit the last packet until we receive a DATA or ERROR
     * packet from the client or until we have retransmitted
     * TFTP_NUM_RETRANS times
     */
    while ( (((bytes_received = TFTPS_Recv()) == NU_NO_DATA) ||
            ((GET16(TFTPS_Con.tftp_input_buffer, 0) != TFTP_DATA_OPCODE) &&
             (GET16(TFTPS_Con.tftp_input_buffer, 0) != TFTP_ERROR_OPCODE))) &&
             (retransmits < TFTP_NUM_RETRANS) )
    {
        TFTPS_Retransmit(TFTPS_Con.bytes_sent);
        retransmits++;
    }

    /* We received something, so set the tid */
    if ( (bytes_received > 0) && (retransmits < TFTP_NUM_RETRANS) )
    {
        TFTPS_Con.tid = TFTPS_Con.server_addr.port;

        /* Increment Block Number */
        TFTPS_Con.block_number ++;

        /* Process the first data packet that was sent to the server.
         * We cannot get a duplicate data packet yet, because this is
         * the first data packet, so precautions do not need to be taken
         * to retransmit the ACK/OACK.
         */
        if ((status = TFTPS_Process_Data(bytes_received,
                                         file_desc)) != NU_SUCCESS)
            TFTPS_Con.status = (INT16)status;

        /* While we are still transferring the file */
        while (TFTPS_Con.status == TRANSFERRING_FILE)
        {
            if (status != TFTPS_DUPLICATE_DATA)
                retransmits = 0;
            else
            {
                TFTPS_Retransmit(TFTPS_Con.bytes_sent);
                retransmits++;
            }

            /* Current State:
             *
             * client: DATA --->
             *                  <--- server: ACK
             *
             * The "session" is established and the client is transmitting
             * the file.  The server has just sent an ACK in response to the
             * client's DATA packet and is expecting either a DATA packet
             * or an ERROR packet.  If a DATA or ERROR are not received,
             * the server will retransmit the ACK packet.
             */

            /* Try to retrieve the next DATA packet until we receive it
             * or have tried TFTP_NUM_RETRANS times
             */
            while ( ((bytes_received = TFTPS_Recv()) == NU_NO_DATA)
                    && (retransmits < TFTP_NUM_RETRANS) )
            {
                TFTPS_Retransmit(TFTPS_Con.bytes_sent);
                retransmits++;
            }

            /* If a data packet was received then process it.
             * Else a problem occurred, and we need to exit.
             */
            if ( (bytes_received) && (retransmits <= TFTP_NUM_RETRANS) )
            {                                              /* NTWK_10842 */
                if ((status = TFTPS_Process_Data(bytes_received,
                                                 file_desc)) != NU_SUCCESS)
                    TFTPS_Con.status = (INT16)status;
            }
            else
                TFTPS_Con.status = TFTP_CON_FAILURE;
        }
    }

    /* We did not receive anything */
    else
        TFTPS_Con.status = TFTP_CON_FAILURE;

    NU_Close_Socket(TFTPS_Con.session_socket_number);

    /* We are finished transferring the file - close it */
    status = NU_Close(file_desc);
    if (status!= NU_SUCCESS)
        TFTPS_Con.status = (INT16)status;
}

/************************************************************************
*
*   FUNCTION
*
*       TFTPS_Process_Read_Request
*
*   DESCRIPTION
*
*       This function processes an incoming read request.
*
*   INPUTS
*
*       file_desc       The file descriptor associated with the file
*                       being read from the server.
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID TFTPS_Process_Read_Request(INT file_desc)
{
    INT32   bytes_received;
    INT16   retransmits;
    STATUS  status = NU_SUCCESS;

    /*  Set the Length of the buffer. */
    TFTPS_Con.cur_buf_size = TFPTS_Disk_Space();

    /* Initialize the retransmit counter. */
    retransmits = 0;

    TFTPS_Con.tid = TFTPS_Con.server_addr.port;

#if (TFTPS_RFC2347_COMPLIANT == NU_TRUE)

    if (GET16(TFTPS_Con.trans_buf, 0) == TFTP_OACK_OPCODE)
    {

        /* Current State:
         *
         * client: RRQ --->
         *                  <--- server: OACK
         *
         * The server is now expecting an ACK or ERROR packet from
         * the client.  If an ACK or ERROR are not received, the
         * server will retransmit the OACK or DATA packet that it
         * just sent.
         */

        /* While an ACK of the data packet has not been received
         * and the maximum number of retransmits has not yet been
         * reached, retransmit the last data packet.
         */
        while ( (((bytes_received = TFTPS_Recv()) == NU_NO_DATA) ||
                 ((GET16(TFTPS_Con.tftp_input_buffer, 0) != TFTP_ACK_OPCODE) &&
                 (GET16(TFTPS_Con.tftp_input_buffer, 0)  != TFTP_ERROR_OPCODE)))
                 && (retransmits < TFTP_NUM_RETRANS) )
        {
            TFTPS_Retransmit(TFTPS_Con.bytes_sent);
            retransmits++;
        }

        /* If we received something, setup the tid and process the ACK. */
        if ( (bytes_received > 0) && (retransmits < TFTP_NUM_RETRANS) )
        {
            if ((status = TFTPS_Process_Ack()) != NU_SUCCESS)
                TFTPS_Con.status = (INT16)status;
        }
        else
            TFTPS_Con.status = TFTP_CON_FAILURE;
    }

#endif

    /* While we are transferring the file */
    while (TFTPS_Con.status == TRANSFERRING_FILE)
    {
        /* If the packet received does not contain a duplicate ACK,
         * send the next data packet.
         */
        if (status != TFTPS_DUPLICATE_ACK)
        {
            /* Send a data packet. */
            if (TFTPS_Send_Data(file_desc) < 0)
                break;

            /* Initialize the retransmit counter. */
            retransmits = 0;
        }

        /* Current State:
         *
         * client: ACK --->
         *                  <--- server: DATA
         *
         * The "session" is established and the server is transmitting
         * the file.  The server has just sent a DATA packet and is
         * expecting either an ACK packet or an ERROR packet.  If an ACK
         * or ERROR are not received, the server will retransmit the
         * DATA packet.
         */

        /* While an ACK of the data packet has not been received and
         * the maximum number of retransmits has not yet been reached.
         * Retransmit the last data packet.
         */
        while ( ((bytes_received = TFTPS_Recv()) == NU_NO_DATA)
                && (retransmits < TFTP_NUM_RETRANS) )
        {
            TFTPS_Retransmit(TFTPS_Con.bytes_sent);
            retransmits++;
        }

        /* Process the ACK. */
        if ( (bytes_received > 0) && (retransmits <= TFTP_NUM_RETRANS) )
        {
            if ((status = TFTPS_Process_Ack()) == TFTP_CON_FAILURE)
                TFTPS_Con.status = (INT16)status;
        }
        else
            TFTPS_Con.status = TFTP_CON_FAILURE;

    }  /* while transferring file. */

    NU_Close_Socket(TFTPS_Con.session_socket_number);

    /* We are finished transferring the file - close it. */
    if (NU_Close(file_desc) != NU_SUCCESS)
        TFTPS_Con.status = TFTP_CON_FAILURE;
    /* end If Read type */
}

#if (TFTPS_RFC2347_COMPLIANT == NU_TRUE)

/************************************************************************
*
*   FUNCTION
*
*       TFTPS_Cleanup_Buffers
*
*   DESCRIPTION
*
*       This function deallocates the buffers being used by the
*       previous session and allocates new buffers if necessary.
*       Only the buffer used to transmit or receive DATA is
*       deallocated and reallocated, and then only if the block size
*       was negotiated to be larger than the default block size.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID TFTPS_Cleanup_Buffers(VOID)
{
    STATUS  status;

    /* If the block size was changed to a value larger than the default,
     * deallocate the memory and reallocate the default size.  This does
     * not need to be done if the block size was changed to a value smaller
     * than the default, because we do not deallocate and reallocate a
     * smaller value for the buffer when the block size option is
     * negotiated.
     */
    if (TFTPS_Con.options.blksize > TFTP_BLOCK_SIZE_DEFAULT)
    {
        if (TFTPS_Con.type == READ_TYPE)
        {
            /* Deallocate the memory we were using */
            NU_Deallocate_Memory((VOID*)TFTPS_Con.trans_buf);

            /* Attempt to allocate memory for the output buffer until successful */
            do
            {
                /* Allocate memory for the next initial request from the client */
                status = NU_Allocate_Memory(TFTPS_Memory,
                                            (VOID**)&TFTPS_Con.trans_buf,
                                            TFTP_BLOCK_SIZE_DEFAULT +
                                            TFTP_HEADER_SIZE, NU_SUSPEND);

                if (status != NU_SUCCESS)
#ifdef NET_5_2
                    NET_Sleep(SCK_Ticks_Per_Second);
#else
                    NU_Sleep(SCK_Ticks_Per_Second);
#endif

            } while (status != NU_SUCCESS);
        }

        else if (TFTPS_Con.type == WRITE_TYPE)
        {
            NU_Deallocate_Memory((VOID*)TFTPS_Con.tftp_input_buffer);

            /* Attempt to allocate memory for the input buffer until successful */
            do
            {
                /* Allocate memory for the next initial request from the client */
                status = NU_Allocate_Memory(TFTPS_Memory,
                                            (VOID**)&TFTPS_Con.tftp_input_buffer,
                                            TFTP_BLOCK_SIZE_DEFAULT +
                                            TFTP_HEADER_SIZE, NU_SUSPEND);

                if (status != NU_SUCCESS)
#ifdef NET_5_2
                    NET_Sleep(SCK_Ticks_Per_Second);
#else
                    NU_Sleep(SCK_Ticks_Per_Second);
#endif

            } while (status != NU_SUCCESS);
        }
    }
}

#endif

/************************************************************************
*
*   FUNCTION
*
*       TFTPS_Recv
*
*   DESCRIPTION
*
*       This function is responsible for receiving data from a TFTP
*       client.  NU_Select is used to timeout.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       The number of bytes received when successful.
*       NU_NO_DATA  when NU_Select fails to find a data ready socket.
*
************************************************************************/
INT32 TFTPS_Recv()
{
    FD_SET      readfs;
    INT32       status;
    INT16       clilen;

    /* Do a select on this socket.  In the case that the foreign port fails
     * to respond we don't want to suspend on receive forever.
     */
    NU_FD_Init(&readfs);

    NU_FD_Set(TFTPS_Con.session_socket_number, &readfs);

    if ((status = (INT32)NU_Select(TFTPS_Con.session_socket_number + 1, &readfs, NU_NULL, NU_NULL,
                                   (TFTPS_Con.options.timeout *
                                   SCK_Ticks_Per_Second))) != NU_SUCCESS)
        return(status);

    /* We must have received something.  Go get the client's request or
     * response
     */
    return ((INT32)NU_Recv_From(TFTPS_Con.session_socket_number,
                                TFTPS_Con.tftp_input_buffer,
                                (UINT16)(TFTPS_Con.options.blksize +
                                TFTP_HEADER_SIZE), 0,
                                &TFTPS_Con.server_addr, &clilen));
} /* end TFTPS_Recv */

/************************************************************************
*
*   FUNCTION
*
*       TFTPS_RRecv
*
*   DESCRIPTION
*
*       This function is responsible for receiving a TFTP request from a
*       TFTP Client.  NU_Select is used to Suspend Until Reception of
*       Data.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       The number of bytes received when successful.
*       NU_NO_DATA  when NU_Select fails to find a data ready socket.
*
************************************************************************/
INT16 TFTPS_RRecv()
{
    FD_SET    readfs;
    INT16     status, clilen;

    /* Do a select on this socket.  Do a  suspend on receive forever until
     * a Client's Request is sent.
     */
    NU_FD_Init(&readfs);

    NU_FD_Set(TFTPS_Con.socket_number, &readfs);

    if ((status = (INT16)NU_Select(TFTPS_Con.socket_number + 1, &readfs, NU_NULL, NU_NULL,
                                   NU_SUSPEND)) != NU_SUCCESS)
        return(status);

    /*  We must have received something.  Go get the client's request or
     *  response
     */
    return ((INT16)NU_Recv_From(TFTPS_Con.socket_number,
                                TFTPS_Con.tftp_input_buffer,
                                TFTP_BUFFER_SIZE_MIN, 0,
                                &TFTPS_Con.server_addr, &clilen));
} /* end TFTPS_RRecv */

/************************************************************************
*
*   FUNCTION
*
*       TFTPS_Process_Data
*
*   DESCRIPTION
*
*       This function is responsible for processing a data packet
*       whenever a read request is in progress.
*
*   INPUTS
*
*       bytes_received  Number of bytes in the packet
*       file_desc       The Index into the file system
*
*   OUTPUTS
*
*       NU_SUCCESS whenever the expected data was received, -1 otherwise.
*
************************************************************************/
STATUS TFTPS_Process_Data(INT32 bytes_received, INT file_desc)
{
    UINT16 data_size;
    UINT16 temp;
    
    /* What kind of packet is this. */
    switch(GET16(TFTPS_Con.tftp_input_buffer, 0))
    {
    case TFTP_DATA_OPCODE:

    /* If the current block number is greater than the block number
     * of the ACK packet we are receiving, then this is an old
     * packet and we have already processed it - we do not want to
     * exit, error or acknowledge the packet (because we have
     * already processed it) so we will get the next packet
     */
        if ( (TFTPS_Con.block_number > GET16(TFTPS_Con.tftp_input_buffer, 2))
             && (TFTPS_Con.tid == TFTPS_Con.server_addr.port) )
            return (TFTPS_DUPLICATE_DATA);

        /* If data was received make sure that block number and TID are
         * correct.
         */
        if ( (TFTPS_Con.block_number == GET16(TFTPS_Con.tftp_input_buffer, 2))
             && (TFTPS_Con.tid == TFTPS_Con.server_addr.port) )
        {
            /* Calculate the amount of data in this packet. */
            data_size = (UINT16)(bytes_received - TFTP_HEADER_SIZE);

            /* Make sure there is enough room left in the user's buffer for
             * the received data.
             */
            if (TFTPS_Con.cur_buf_size < data_size)
                data_size = (UINT16)TFTPS_Con.cur_buf_size;

            NU_Write(file_desc, (CHAR*)&(TFTPS_Con.tftp_input_buffer[4]), data_size);

            /* If 512 bytes of data was copied then send an ACK.  We know
             * that the other side will send at least one more data packet,
             * and that all data in the current packet was accepted.
             */
            if (data_size == TFTPS_Con.options.blksize)
                TFTPS_Ack();

            /* Else if less data was copied than was received then we have
             * filled the user's buffer, and can accept no more data.  Send
             * an error condition indicating that no more data can be
             * accepted.
             */
            else if (data_size < (UINT16)(bytes_received - TFTP_HEADER_SIZE))
            {
                TFTPS_Con.status = TRANSFER_COMPLETE;
                TFTPS_Error(3, "Error: Storage Media full.");

                return (TFTP_CON_FAILURE);
            }

            /* Else the last data packet has been received.  We are done.
             * Send the last ack.
             */
            else
            {
                TFTPS_Con.status = TRANSFER_COMPLETE;
                TFTPS_Ack();
            }

            /* Update the amount of space left in the user's buffer and the
             * pointer into the user's buffer.
             */
            TFTPS_Con.cur_buf_size -= data_size;

            /* Increment the block number. */
            TFTPS_Con.block_number++;
        }

        else
        {
            TFTPS_Error(5, "Error: Unknown Transfer ID");
            return(TFTP_CON_FAILURE);
        }
        break;

    case TFTP_ERROR_OPCODE:
        temp = GET16(TFTPS_Con.tftp_input_buffer, 2);
        if (temp < (sizeof(tftps_errors) / sizeof(tftps_errors[0])))
            return (tftps_errors[temp]);

        else
            return(TFTP_CON_FAILURE);

    case TFTP_RRQ_OPCODE:
    case TFTP_WRQ_OPCODE:
    case TFTP_ACK_OPCODE:
    default:
        {
            TFTPS_Error(4, "Error: Illegal TFTP Operation");
            return (TFTP_CON_FAILURE);
        }
    }
    return (NU_SUCCESS);
}  /* TFTPS_Process_Data */

/************************************************************************
*
*   FUNCTION
*
*       TFTPS_Process_Request_PACKET
*
*   DESCRIPTION
*
*       This function is responsible processing a request packet whenever
*       a TFTP Client sends one.
*
*   INPUTS
*
*       bytes_received  Number of bytes in the packet
*       trans_buf       The pointer in memory to our transmission buffer.
*                       It may need to be reset if the user specifies a
*                       blksize other other than 512 in the request
*
*   OUTPUTS
*
*       file_desc           The Index where the file is located in
*                           the file system
*       TFTP_CON_FAILURE    Error has occurred
*
************************************************************************/
INT TFTPS_Process_Request_PACKET(INT32 bytes_received)
{
    INT16   count = 2, count1 = 0, prev_socket;
    char    file_name[TFTP_PARSING_LENGTH];
    INT     file_desc;    /* File Descriptor for in-memory file system */

#if (TFTPS_RFC2347_COMPLIANT == NU_TRUE)

    STATUS  status;

#endif

#if (TFTPS_RFC2347_COMPLIANT == NU_FALSE)

    TFTP_UNUSED_PARAMETER(bytes_received);

#endif

    /* What kind of packet is this. */
    switch(GET16(TFTPS_Con.tftp_input_buffer, 0))
    {
    case TFTP_RRQ_OPCODE:

        /*  get the Filename */
        while ( (TFTPS_Con.tftp_input_buffer[count] != '\0') &&
                (count1 < TFTP_PARSING_LENGTH-1) )
        {
            file_name[count1] = TFTPS_Con.tftp_input_buffer[count];
            count ++;
            count1 ++;
        }

        file_name[count1] = '\0';

        count++;

        /* Only Octet (binary mode) is supported at this time, so
         * ignore the mode specifier in the buffer and increment
         * "count" upto the end of the buffer.
         */
        while (TFTPS_Con.tftp_input_buffer[count] != '\0')
            count ++;

#if (TFTPS_RFC2347_COMPLIANT == NU_TRUE)

        count++;

#endif

        file_desc = NU_Open(file_name, PO_RDONLY, PS_IREAD);

        if (file_desc < 0)
        {
            /* Session Socket Number is null,
             * so, send error out on listener session
             */
            prev_socket = (INT16)TFTPS_Con.session_socket_number;
            TFTPS_Con.session_socket_number = TFTPS_Con.socket_number;
            TFTPS_Error(3, "Error: File not found.");

            /* Set the file descriptor and type to an error so processing
             * will stop.
             */
            file_desc= -99;
            TFTPS_Con.type = -1;

            TFTPS_Con.session_socket_number = prev_socket;
            break;
        }

        TFTPS_Con.type = READ_TYPE;

        /* Attempt to create a socket for the new connection until successful */
        do
        {
            /* Create a socket */
            TFTPS_Con.session_socket_number =
                (INT16)NU_Socket(TFTPS_Con.server_addr.family, NU_TYPE_DGRAM, 0);

            if (TFTPS_Con.session_socket_number < 0)
#ifdef NET_5_2
                NET_Sleep(SCK_Ticks_Per_Second);
#else
                NU_Sleep(SCK_Ticks_Per_Second);
#endif

        } while (TFTPS_Con.session_socket_number < 0);

#if (TFTPS_RFC2347_COMPLIANT == NU_TRUE)

        /* If the client is RFC 2347 compliant */
        if (count != bytes_received)
        {
            status = TFTPS_Check_Options(bytes_received, count, file_desc);

            if (status != NU_SUCCESS)
            {
                TFTPS_Error(3, "Error: Options error");

                file_desc= -99;
                TFTPS_Con.type = -1;
            }

            /* Acknowledge Read request Packet */
            else
            {
                /*  Set Block Number to 0  */
                TFTPS_Con.block_number = 0;

                TFTPS_Ack();
            }
        }

        /* Else, the client is not compliant, and block number should
         * begin at 1
         */
        else
#endif
            TFTPS_Con.block_number = 1;

        /* Set whether a Read or Write from the Server.  */
        TFTPS_Con.status = TRANSFERRING_FILE;

        break;

    case TFTP_WRQ_OPCODE:

        /* Get the Filename */
        while ( (TFTPS_Con.tftp_input_buffer[count] != '\0') &&
                (count1 < TFTP_PARSING_LENGTH-1) )
        {
            file_name[count1] = TFTPS_Con.tftp_input_buffer[count];
            count ++;
            count1++;
        }

        file_name[count1] = '\0';

        count++;

        file_desc = NU_Open(file_name, PO_RDONLY, PS_IREAD);

        if (file_desc >= 0)
        {
             /*The file already exists, close the file and return error*/
        	TFTPS_Con.status = NU_Close(file_desc);
		
			/* Session Socket Number is null,
            * so, send error out on listener session
            */
            prev_socket = (INT16)TFTPS_Con.session_socket_number;
            TFTPS_Con.session_socket_number = TFTPS_Con.socket_number;
            TFTPS_Error(6, "Error: File already exists.");

            /* Set the file descriptor and type to an error so processing
            * will stop.
            */
            file_desc= -99;
            TFTPS_Con.type = -1;

            TFTPS_Con.session_socket_number = prev_socket;
            break;
        }

        /* Only Octet (binary mode) is supported at this time, so
         * ignore the mode specifier in the buffer and increment
         * "count" upto the end of the buffer.
         */
        while (TFTPS_Con.tftp_input_buffer[count] != '\0')
            count ++;

#if (TFTPS_RFC2347_COMPLIANT == NU_TRUE)

        count++;

#endif

        /*  Retrieve File_Descriptor */
        file_desc =NU_Open(file_name, PO_RDWR|PO_CREAT|PO_TRUNC, PS_IWRITE);

        if (file_desc < 0)
        {
            /* Session Socket Number is null,
            * so, send error out on listener session
            */
            prev_socket = (INT16)TFTPS_Con.session_socket_number;
            TFTPS_Con.session_socket_number = TFTPS_Con.socket_number;
            TFTPS_Error(3, "Error: Disk full.");

            /* Set the file descriptor and type to an error so processing
             * will stop.
             */
            file_desc= -99;
            TFTPS_Con.type = -1;

            TFTPS_Con.session_socket_number = prev_socket;
            break;
        }

        TFTPS_Con.type = WRITE_TYPE;

#if (TFTPS_RFC2347_COMPLIANT == NU_TRUE)

        /* If there is more left in the packet, then the client is
         * RFC 2347 compliant and requesting options for us to evaluate
         */
        if (count != bytes_received)
        {
            status = TFTPS_Check_Options(bytes_received, count,
                                         file_desc);

            if (status != NU_SUCCESS)
            {
                /* Close and delete the file - nothing was ever written to it */
                NU_Close(file_desc);
                NU_Delete(file_name);

                /* Session Socket Number is null,
                   so, send error out on listener session */
                prev_socket = (INT16)TFTPS_Con.session_socket_number;
                TFTPS_Con.session_socket_number = TFTPS_Con.socket_number;

                /* Set appropriate error codes */
                TFTPS_Error(3, "Error: Options error");
                file_desc= -99;
                TFTPS_Con.type = -1;

                /* Restore socket number */
                TFTPS_Con.session_socket_number = prev_socket;
            }
        }
#endif

        TFTPS_Con.block_number = 0;

        /* Attempt to create a socket for the new connection until successful */
        do
        {
            /* Create a socket */
            TFTPS_Con.session_socket_number =
                (INT16)NU_Socket(TFTPS_Con.server_addr.family, NU_TYPE_DGRAM, 0);

            if (TFTPS_Con.session_socket_number < 0)
#ifdef NET_5_2
                NET_Sleep(SCK_Ticks_Per_Second);
#else
                NU_Sleep(SCK_Ticks_Per_Second);
#endif

        } while (TFTPS_Con.session_socket_number < 0);

        /* Acknowledge Write Request Packet */
        TFTPS_Ack();

        /*  Set the Status to transferring file */
        TFTPS_Con.status = TRANSFERRING_FILE;

        break;

    case TFTP_ACK_OPCODE:
    case TFTP_ERROR_OPCODE:
    case TFTP_DATA_OPCODE:
    default:
        TFTPS_Error(4, "Error: Illegal TFTP Operation");
        return (TFTP_CON_FAILURE);
    }

    return (file_desc);
}  /* TFTPS_Process_Request_PACKET */

/************************************************************************
*
*   FUNCTION
*
*       TFTPS_Ack
*
*   DESCRIPTION
*
*       This function is responsible for sending an acknowledgement of
*       a TFTP data packet.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       The Number of bytes sent on success.
*
************************************************************************/
STATUS TFTPS_Ack()
{
#if (TFTPS_RFC2347_COMPLIANT == NU_TRUE)

    INT   send_size = 2;
    UINT8 temp[10];

#endif

#if (TFTPS_RFC2347_COMPLIANT == NU_TRUE)

    /* If the options acknowledged flag is set, then the client is
     * RFC 2347 compliant, and an OACK must be sent back to the client
     */
    if ((TFTPS_Con.options.blksize_acknowledged == NU_TRUE) ||
        (TFTPS_Con.options.tsize_acknowledged == NU_TRUE) ||
        (TFTPS_Con.options.timeout_acknowledged == NU_TRUE))
    {
        /* Setup the OACK packet. */
        PUT16(TFTPS_Con.trans_buf, 0, TFTP_OACK_OPCODE);

        /* Check if there is a blksize, timeout or tsize option indicated,
         * if so, append the option name and option value to the end of
         * the packet - all null terminated
         */
        if (TFTPS_Con.options.blksize_acknowledged == NU_TRUE)
        {
            strcpy(&(TFTPS_Con.trans_buf[send_size]), "blksize");
            send_size = strlen("blksize") + send_size;
            TFTPS_Con.trans_buf[send_size++] = 0;
            strcpy(&(TFTPS_Con.trans_buf[send_size]),
                   NU_ITOA((INT)(TFTPS_Con.options.blksize),
                   (CHAR*)temp, 10));
            send_size += strlen((CHAR*)temp);
            TFTPS_Con.trans_buf[send_size++] = 0;
        }

        if (TFTPS_Con.options.timeout_acknowledged == NU_TRUE)
        {
            strcpy(&(TFTPS_Con.trans_buf[send_size]), "timeout");
            send_size = strlen("timeout") + send_size;
            TFTPS_Con.trans_buf[send_size++] = 0;
            strcpy(&(TFTPS_Con.trans_buf[send_size]),
                   NU_ITOA((INT)(TFTPS_Con.options.timeout),
                   (CHAR*)temp, 10));
            send_size += strlen((CHAR*)temp);
            TFTPS_Con.trans_buf[send_size++] = 0;
        }

        if (TFTPS_Con.options.tsize_acknowledged == NU_TRUE)
        {
            strcpy(&(TFTPS_Con.trans_buf[send_size]), "tsize");
            send_size = strlen("tsize") + send_size;
            TFTPS_Con.trans_buf[send_size++] = 0;
            strcpy(&(TFTPS_Con.trans_buf[send_size]),
                   NU_ULTOA(TFTPS_Con.options.tsize,
                   (CHAR*)temp, 10));
            send_size += strlen((CHAR*)temp);
            TFTPS_Con.trans_buf[send_size++] = 0;
        }

        /* Set them back to false, because we only send an OACK
         * after the initial request
         */
        TFTPS_Con.options.tsize_acknowledged = NU_FALSE;
        TFTPS_Con.options.timeout_acknowledged = NU_FALSE;
        TFTPS_Con.options.blksize_acknowledged = NU_FALSE;

        TFTPS_Con.bytes_sent = send_size;

        /* Send the write request. */
        return (NU_Send_To(TFTPS_Con.session_socket_number, TFTPS_Con.trans_buf,
                           (UINT16)send_size, 0, &TFTPS_Con.server_addr, 0));
    }

    else
#endif
    {
        /* Setup the ACK packet. */
        PUT16(TFTPS_Con.trans_buf, 0, TFTP_ACK_OPCODE);
        PUT16(TFTPS_Con.trans_buf, 2, TFTPS_Con.block_number);

        TFTPS_Con.bytes_sent = TFTP_HEADER_SIZE;

        /* Send the ACK packet. */
        return ((STATUS)NU_Send_To(TFTPS_Con.session_socket_number, TFTPS_Con.trans_buf,
                           TFTP_HEADER_SIZE, 0, &TFTPS_Con.server_addr, 0));
    }
} /* end TFTPS_Ack */

/************************************************************************
*
*   FUNCTION
*
*       TFTPS_Process_Ack
*
*   DESCRIPTION
*
*       This function is responsible processing an ack packet whenever
*       a read request is in progress.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS whenever the expected data was received, -1 otherwise.
*
************************************************************************/
STATUS TFTPS_Process_Ack()
{
    UINT16 temp;
    
    /* What kind of packet is this. */
    switch(GET16(TFTPS_Con.tftp_input_buffer, 0))
    {
    case TFTP_ACK_OPCODE:

    /* If the current block number is greater than the block number
     * of the ACK packet we are receiving, then this is an old
     * packet and we have already processed it - we do not want to
     * exit, error or acknowledge the packet (because we have
     * already processed it) so we will get the next packet
     */
        if ( (TFTPS_Con.block_number > GET16(TFTPS_Con.tftp_input_buffer, 2))
             && (TFTPS_Con.tid == TFTPS_Con.server_addr.port) )
            return (TFTPS_DUPLICATE_ACK);

        /* Make sure the block number and TID are correct. */
        if ( (TFTPS_Con.block_number == GET16(TFTPS_Con.tftp_input_buffer, 2))
             && (TFTPS_Con.tid == TFTPS_Con.server_addr.port) )
            TFTPS_Con.block_number++;

        else
            return(TFTP_CON_FAILURE);

        break;

    case TFTP_ERROR_OPCODE:
        temp = GET16(TFTPS_Con.tftp_input_buffer, 2);
        if (temp < (sizeof(tftps_errors) / sizeof(tftps_errors[0])))
            return (tftps_errors[temp]);

        else
            return(TFTP_CON_FAILURE);

    case TFTP_RRQ_OPCODE:
    case TFTP_WRQ_OPCODE:
    case TFTP_DATA_OPCODE:
    default:
        TFTPS_Error(4, "Error: Illegal TFTP Operation");
        return (TFTP_CON_FAILURE);
    }
    return (NU_SUCCESS);
}  /* TFTPS_Process_Ack */

/************************************************************************
*
*   FUNCTION
*
*       TFTPS_Send_Data
*
*   DESCRIPTION
*
*       This function is responsible for sending data for the Client's
*       Read Request.  It will read data from the file and send data
*       until all bytes in the file have been sent.
*
*   INPUTS
*
*       file_desc       Index into file system.
*
*   OUTPUTS
*
*       The Number of bytes sent on success.
*
************************************************************************/
INT32 TFTPS_Send_Data(INT file_desc)
{
    UINT32 num_bytes;

    /* Fill in the opcode and block number. */
    PUT16(TFTPS_Con.trans_buf, 0, TFTP_DATA_OPCODE);
    PUT16(TFTPS_Con.trans_buf, 2, TFTPS_Con.block_number);

    /*  Read data from the file into the the TFTP CB send buffer. */
    num_bytes = NU_Read(file_desc, (CHAR*)&(TFTPS_Con.trans_buf[4]), TFTPS_Con.options.blksize);

    /* If this is the last packet update the status. */
    if (num_bytes < TFTPS_Con.options.blksize)
        TFTPS_Con.status = TRANSFER_COMPLETE;

    /* Send the data. */
    return ((INT32)NU_Send_To(TFTPS_Con.session_socket_number, TFTPS_Con.trans_buf,
                              (UINT16)(num_bytes + TFTP_HEADER_SIZE), 0,
                              &TFTPS_Con.server_addr, 0));
} /* end TFTPS_Send_Data */

/************************************************************************
*
*   FUNCTION
*
*       TFTPS_Retransmit
*
*   DESCRIPTION
*
*       This function will retransmit the last packet sent.
*
*   INPUTS
*
*       nbytes          The number of bytes to retransmit.
*
*   OUTPUTS
*
*       The Number of bytes sent on success.
*
************************************************************************/
INT32 TFTPS_Retransmit(INT32 nbytes)
{
    /* Retransmit the last packet. */
    return((INT32)NU_Send_To(TFTPS_Con.session_socket_number, TFTPS_Con.trans_buf,
                             (UINT16)nbytes, 0, &TFTPS_Con.server_addr, 0));
}/* TFTPS_Retransmit */

/************************************************************************
*
*   FUNCTION
*
*       TFTPS_Error
*
*   DESCRIPTION
*
*       This function will send an error packet.
*
*   INPUTS
*
*       error_code      The TFTP error code.
*       err_string      The error message to send.
*
*   OUTPUTS
*
*       The Number of bytes sent on success.
*
************************************************************************/
STATUS TFTPS_Error(INT16 error_code, char *err_string)
{
    INT   send_size;

    /* Fill in the opcode and block number. */
    PUT16(TFTPS_Con.trans_buf, 0, TFTP_ERROR_OPCODE);
    PUT16(TFTPS_Con.trans_buf, 2, (unsigned short)error_code);

    /* send_size = (INT16)sprintf(&(TFTPS_Con.trans_buf[4]), "%s%c", err_string, 0); */
    strcpy(&(TFTPS_Con.trans_buf[4]), err_string);
    send_size = (INT)strlen(err_string)+4;
    TFTPS_Con.trans_buf[send_size++] = 0;

    /* Send the datagram. */
    return ((STATUS)NU_Send_To(TFTPS_Con.session_socket_number, TFTPS_Con.trans_buf,
                       (UINT16)(send_size + TFTP_HEADER_SIZE),
                       0, &TFTPS_Con.server_addr, 0));
} /* end TFTPS_Error */

#if (TFTPS_RFC2347_COMPLIANT == NU_TRUE)

/************************************************************************
*
*  FUNCTION
*
*      FTP_Handle_File_Length
*
*  DESCRIPTION
*
*      Returns the length of the file associated with the file
*      handle.
*
*  INPUTS
*
*      file_desc                   File descriptor of the file.
*
*  OUTPUTS
*
*      The Length of the File
*
************************************************************************/

STATIC UINT32 TFTP_Handle_File_Length(INT file_desc)
{
    UINT32      data_length = 0;
    INT32       original_location;

    /* Save off the current location of the file pointer - seek 0 bytes
     * from the current position.
     */
    original_location = NU_Seek(file_desc, 0, PSEEK_CUR);

    if (original_location >= 0)
    {
        /* Get the end location of the file pointer - seek to the end of
         * the file.
         */ 
        data_length = (UINT32)NU_Seek(file_desc, 0, PSEEK_END);

        /* Restore the original position of the file pointer - seek 
         * original_location bytes from the beginning of the file.
         */
        NU_Seek(file_desc, original_location, PSEEK_SET);
    }

    return (data_length);
}


/************************************************************************
*
*   FUNCTION
*
*       TFTPS_Check_Options
*
*   DESCRIPTION
*
*       Accepts/Rejects the options requested by a client.
*       If a block size other than TFTP_BLOCK_SIZE_DEFAULT is requested,
*       the server must deallocate the memory currently being used for
*       the transmission buffer and allocate another
*       TFTPS_Con.options.blksize + TFTP_HEADER_SIZE bytes of memory.
*
*   INPUTS
*
*       bytes_received  Number of bytes in the request packet, minus the
*                       opcode and mode.
*       count           Index into the request packet, starting at the
*                       first option.
*       file_desc       Index into file system.
*       trans_buf       Pointer to the transmission buffer's memory space.
*
*   OUTPUTS
*
*       TFTP_DISK_FULL  There is not tsize memory to hold the file.
*       NU_NO_MEMORY    There is not blksize memory available to allocate
*       NU_SUCCESS      Successful
*
************************************************************************/
STATUS TFTPS_Check_Options(INT32 bytes_received, INT16 count, INT file_desc)
{
    INT16 count1;
    char temp2[TFTP_PARSING_LENGTH];
    char temp3[TFTP_PARSING_LENGTH];
    char *op_holder;
    char *value_holder;
    STATUS status;

    TFTPS_Con.options.tsize_acknowledged = NU_FALSE;
    TFTPS_Con.options.timeout_acknowledged = NU_FALSE;
    TFTPS_Con.options.blksize_acknowledged = NU_FALSE;

    /* Parse the options */
    while (count < (bytes_received-1))
    {
        count1 = 0;

        /* Parse the first option from the transmission buffer */
        while ( (TFTPS_Con.tftp_input_buffer[count] != '\0') &&
                (count1 < TFTP_PARSING_LENGTH) )
        {
            temp2[count1] = TFTPS_Con.tftp_input_buffer[count];
            count ++;
            count1 ++;
        }

        temp2[count1] = '\0';
        op_holder = (CHAR *)&temp2;

        count++;
        count1 = 0;

        /* Parse the first value from the transmission buffer */
        while ( (TFTPS_Con.tftp_input_buffer[count] != '\0') &&
                (count1 < TFTP_PARSING_LENGTH) )
        {
            temp3[count1] = TFTPS_Con.tftp_input_buffer[count];
            count ++;
            count1 ++;
        }

        temp3[count1] = '\0';
        value_holder = (CHAR *)&temp3;

        count++;

        /* The server has the authority to change the value of some
         * options we specified.
         */
        if (strcmp(op_holder, "timeout") == 0)
        {
            /* The server may not change the timeout value */
            TFTPS_Con.options.timeout = (UINT16)NU_ATOI(value_holder);

            TFTPS_Con.options.timeout_acknowledged = NU_TRUE;
        }

        else if (strcmp(op_holder, "tsize") == 0)
        {
            if ((UINT32)NU_ATOI(value_holder) == 0)
            {
                if ((TFTPS_Con.options.tsize = TFTP_Handle_File_Length(file_desc)) > 0)
                {
                    /* We only return a tsize acknowledgement on a RRQ */
                    TFTPS_Con.options.tsize_acknowledged = NU_TRUE;
                }
            }

            else
            {
                if ((UINT32)NU_ATOI(value_holder) <= (UINT32)TFPTS_Disk_Space())
                    TFTPS_Con.options.tsize = (UINT32)NU_ATOI(value_holder);

                else
                {
                    TFTPS_Error(3, "Error: Storage Media full.");
                    return(TFTP_DISK_FULL);
                }
            }
        }

        else if (strcmp(op_holder, "blksize") == 0)
        {
            /* Check that the requested blksize is <= 65464 and >= 8*/
            if ( ((UINT16)NU_ATOI(value_holder) <= TFTP_BLOCK_SIZE_MAX) &&
                 ((UINT16)NU_ATOI(value_holder) >= TFTP_BLOCK_SIZE_MIN) )
                TFTPS_Con.options.blksize = (UINT16)NU_ATOI(value_holder);

            TFTPS_Con.options.blksize_acknowledged = NU_TRUE;
        }
    }

    /* If the block size option has been sent, and the requested block size
     * is greater than the default block size, deallocate the previous memory
     * allocated for the data transmission buffer and reallocate the requested
     * size.
     */
    if ( (TFTPS_Con.options.blksize_acknowledged == NU_TRUE) &&
         (TFTPS_Con.options.blksize > TFTP_BLOCK_SIZE_DEFAULT) )
    {
        if (TFTPS_Con.type == WRITE_TYPE)
        {
            /* Deallocate the original TFTP_BLOCK_SIZE_DEFAULT of memory */
            if ((status = NU_Deallocate_Memory((VOID*)TFTPS_Con.
                                               tftp_input_buffer)) ==
                                               NU_SUCCESS)
            {
                /* Allocate memory for the transmission buffer as specified
                 * by the blksize option
                 */
                status = NU_Allocate_Memory(TFTPS_Memory,
                                           (VOID**)&TFTPS_Con.tftp_input_buffer,
                                           (UNSIGNED)(TFTPS_Con.options.blksize +
                                           TFTP_HEADER_SIZE), NU_SUSPEND);
            }
        }
        else
        {
            /* Deallocate the original TFTP_BLOCK_SIZE_DEFAULT of memory */
            if ((status = NU_Deallocate_Memory((VOID*)TFTPS_Con.
                                               trans_buf)) == NU_SUCCESS)
            {
                /* Allocate memory for the transmission buffer as specified
                 * by the blksize option
                 */
                status = NU_Allocate_Memory(TFTPS_Memory,
                                            (VOID**)&TFTPS_Con.trans_buf,
                                            (UNSIGNED)(TFTPS_Con.options.blksize +
                                            TFTP_HEADER_SIZE), NU_SUSPEND);
            }
        }

        if (status != NU_SUCCESS)
            return(status);
    }

    return (NU_SUCCESS);
}

#endif

