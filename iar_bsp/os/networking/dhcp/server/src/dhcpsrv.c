/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2003              
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
**************************************************************************

**************************************************************************
* FILE NAME                                                           
*                                                                      
*   dhcpsrv.c                                                
*                                                                      
* COMPONENT                                                            
*                                                                      
*   Nucleus DHCP Server                                   
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*   This file contains the functions that are used by the DHCP Server.
*   These functions handle the initialization and main processing of 
*   the server.
*
* DATA STRUCTURES
*
*   None
*                                                                      
* FUNCTIONS                                                            
*
*   DHCPS_Init                                                                      
*   DHCPS_Server
*   DHCPS_Get_Option
*   DHCPS_Process_DISCOVER
*   DHCPS_Process_REQUEST
*   DHCPS_Process_DECLINE
*   DHCPS_Process_RELEASE
*   DHCPS_Get_File_Entry
*   DHCPS_Process_Configuration_Entry
*   DHCPS_Evaluate_Symbol
*   DHCPS_Process_Binding_Entry
*   DHCPS_Get_Client_id
*   DHCPS_Get_Subnet
*   DHCPS_Find_Configuration_By_Entryname
*   DHCPS_Parse_String
*   DHCPS_Get_Binding_For_Lease
*   DHCPS_Find_Binding_With_Client_ID
*   DHCPS_Find_Binding_With_Req_IP
*   DHCPS_Find_New_Binding
*   DHCPS_Choose_Lease_Length
*   DHCPS_Add_Binding_To_Offer
*   DHCPS_Server_Options
*   DHCPS_Build_Message
*   DHCPS_Init_DHCP_Layer
*   DHCPS_Move_Offer_To_Binding
*   DHCPS_Send
*   DHCPS_Save_Binding_To_File
*   DHCPS_Update_Lease_Times
*   DHCPS_Check_Offered_Times
*   DHCPS_Lease_Timer
*   DHCPS_Create_Config_Entry
*   DHCPS_Delete_Config_Entry
*   DHCPS_Get_Config_Entry
*   DHCPS_Add_Option_To_Memory_Block
*   DHCPS_Remove_Option_From_Memory
*   DHCPS_Get_Option_From_Memory
*   DHCPS_Set_Subnet_Address
*   DHCPS_Get_Subnet_Address
*   DHCPS_Set_Subnet_Mask
*   DHCPS_Get_Subnet_Mask
*   DHCPS_Add_Broadcast_Address
*   DHCPS_Remove_Broadcast_Address
*   DHCPS_Get_Broadcast_Address
*   DHCPS_Add_Entry_To_Array
*   DHCPS_Remove_Entry_From_Array
*   DHCPS_Enable_Configuration
*   DHCPS_Disable_Configuration
*   DHCPS_Find_Configuration_Control_Block
*   DHCPS_Get_Option_From_Configuration
*   DHCPS_Shutdown_Server
*   DHCPS_Server_Reset
*   DHCPS_Delete_Config_Name
*   DHCPS_Check_Duetime
*                                                                      
* DEPENDENCIES                                                         
*
*   nucleus_gen_cfg.h
*   networking/nu_networking.h
*   os/networking/dhcp/server/inc/dhcpsrv.h
*   networking/nu_net.h
*   networking/net_extr.h
*   networking/dhcp.h
*   networking/udp.h
*   networking/nerrs.h
*   storage/pcdisk.h
*                                                                      
**********************************************************************************/

/* Includes */
#include  "nucleus_gen_cfg.h"
#include  "networking/nu_networking.h"
#include  "os/networking/dhcp/server/inc/dhcpsrv.h"
#include  "networking/nu_net.h"
#include  "networking/net_extr.h"
#include  "networking/dhcp.h"
#include  "networking/udp.h"
#include  "networking/nerrs.h"
#include  "storage/pcdisk.h"

/* Task parameters */
#define NORMAL_TASK_PRIORITY            (TM_PRIORITY)
#define DHCPS_LEASE_TIME_TASK_PRIORITY  4
#define DHCPS_TASK_STACK_SIZE           6000
#define TASK_STACK_SIZE                 3000

#ifndef NET_4_5
#define NET_4_5                         5
#endif

NU_TASK                         dhcp_server_task_cb;
STATIC  VOID                    *dhcp_server_mem;
STATIC  NU_TASK                 dhcpserv_lease_timer_task_cb;
STATIC  VOID                    *dhcpserv_lease_mem;

CHAR    DHCPSrv_Directory_Path[DHCPS_MAX_FILE_NAME_SIZE];
CHAR    DHCPSrv_Binding_File_Name[DHCPS_MAX_FILE_NAME_SIZE];
CHAR    DHCPSrv_Binding_File_Backup[DHCPS_MAX_FILE_NAME_SIZE];
CHAR    DHCPSrv_Configuration_File_Name[DHCPS_MAX_FILE_NAME_SIZE];
CHAR    DHCPSrv_Options_Block_File_Name[DHCPS_MAX_FILE_NAME_SIZE];

STATIC DHCPLAYER        *DHCPS_Message;
STATIC INT              Socket_Descriptor;


NU_MEMORY_POOL              *DHCPServer_Memory;
DHCPS_CONFIGURATION_LIST    DHCPS_Config_List;  
NU_SEMAPHORE                DHCPS_Semaphore;
BOOLEAN                     DHCPS_Initialized = NU_FALSE;

UINT32  DHCPS_Unused_Parameter;

/* this macro is used to remove warnings. */
#define DHCPS_UNUSED_PARAMETER(x)  DHCPS_Unused_Parameter = ((UINT32)(x))

/* Define prototypes for tasks.  */
STATIC VOID DHCPS_Server_Entry(UNSIGNED argc, VOID *argv);
STATIC VOID DHCPS_Lease_Timer(UNSIGNED argc, VOID *argv);

#if (NET_VERSION_COMP >= NET_4_5)
extern  UINT32 UTL_Check_Duetime(UINT32);
#endif    

INT DHCPS_Default_Drive = CFG_NU_OS_NET_DHCP_SERVER_DEFAULT_DRIVE;
STATUS DHCPS_Delete_Config_Name(CHAR* config_name);
/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Init                                        
*                                                                      
* DESCRIPTION 
*                                                         
*     This function will handle the initialization of the DHCP server.  The 
*     function will read the saved configuration control blocks and bindings
*     from a disk storage (if used) and rebuild the control block link list
*     and restore the history of the leases that the server maintained
*     before it was shutdown.
*                                                                      
* INPUTS                                                               
*                                                                      
*     NU_MEMORY_POOL *                                      
*     CHAR *
*     CHAR *
*     CHAR *
*     CHAR *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Init(NU_MEMORY_POOL *dhcpserv_memory_pool, const CHAR *directory_path, 
                  const CHAR *config_file_name, const CHAR *binding_file_name, 
                  const CHAR *binding_backup, const CHAR *option_file_name)
{
    INT                         binding_fp,
                                config_fp,
                                options_fp;
    CHAR                        binding_config[DHCPS_MAX_ENTRY_NAME_LENGTH],
                                fbuf[DHCPS_FILE_READ_SIZE],
                                buffer[DHCPS_FILE_READ_SIZE],
                                *fileptr,
                                *buffptr,
                                *filebuf_begin,
                                *buffer_pointer;
                                
    INT                         bytes_read,
                                total_bytes_read = 0;
    UINT16                      buff_read,
                                buffer_size;
    INT                         partial_entry = 0,
                                backup_binding_file = NU_FALSE;
                                                                
    STATUS                      status = NU_INVALID_PARM;
    DHCPS_BINDING               previous_binding;
                                
    DHCPS_CONFIG_CB             *config_cb;
    VOID                        *dll_ptr = NU_NULL;

    char                        drive_name[2];

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();                                

    /* Save off the past in memory pool pointer to our global memory pool pointer */
    DHCPServer_Memory = dhcpserv_memory_pool;

    /* Save the passed-in file parameters to the global variables. */
    strcpy(DHCPSrv_Directory_Path, directory_path);
    strcpy(DHCPSrv_Configuration_File_Name, directory_path);
    strcat(DHCPSrv_Configuration_File_Name, config_file_name);
    strcpy(DHCPSrv_Binding_File_Name, directory_path);
    strcat(DHCPSrv_Binding_File_Name, binding_file_name);
    strcpy(DHCPSrv_Binding_File_Backup, directory_path);
    strcat(DHCPSrv_Binding_File_Backup, binding_backup);
    strcpy(DHCPSrv_Options_Block_File_Name, directory_path);
    strcat(DHCPSrv_Options_Block_File_Name, option_file_name);

    /* Create semaphore for protection of critical data */
    status = NU_Create_Semaphore (&DHCPS_Semaphore, "DHCPS_Semaphore", 1, NU_FIFO);
    if(status != NU_SUCCESS)
    {
        return (status);
    }

#ifdef FILE_3_1

    /* Wait 200 ticks for drive to become mounted */
    drive_name[0] = DHCPS_Default_Drive + 'A';
    drive_name[1] = '\0';

    status = NU_Storage_Device_Wait(drive_name, NU_PLUS_TICKS_PER_SEC * DHCPS_DISK_TIMEOUT_DELAY);
    if (status != NU_SUCCESS)
    {
        NU_USER_MODE();
        return (status);
    }
	
    if (NU_Set_Default_Drive(DHCPS_Default_Drive) != NU_SUCCESS)
    {
       NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
       NU_USER_MODE();
       return (status);
    }

#endif 

    /* Set pointers to the first locations of the buffers */
    fileptr = fbuf;
    buffptr = buffer;
    filebuf_begin = fbuf;

    /* Initialize the configuration link list. */
    DHCPS_Config_List.dhcp_config_head = NU_NULL;
    DHCPS_Config_List.dhcp_config_tail = NU_NULL;

    /* Initialize the buffers */
    UTL_Zero(fileptr, DHCPS_FILE_READ_SIZE);
    UTL_Zero(buffptr, DHCPS_FILE_READ_SIZE);
    UTL_Zero(&previous_binding, sizeof(DHCPS_BINDING));

 
    /* Set the current directory to the DHCP Server directory. */
    status = NU_Set_Current_Dir(DHCPSrv_Directory_Path);

    if (status != NU_SUCCESS)
    {
        /* Since the directory was not found, create the directory. */
        status = NU_Make_Dir(DHCPSrv_Directory_Path);

        if (status != NU_SUCCESS)
        {
            status = NU_INVALID_PARM;
        }    
    }    

    if (status == NU_SUCCESS)
    {
        /* Set the buffer_size variable to equal FILE_READ_SIZE */
        buffer_size = DHCPS_FILE_READ_SIZE;

        /* Open the configuration file */
        config_fp = NU_Open((CHAR *)DHCPSrv_Configuration_File_Name, PO_RDONLY, PS_IREAD);


        /* If the target does not have a disk file system, then the NU_Open should fail.  If
            the open fails, just move on */
        if (config_fp >= 0)
        {

            for (;;)
            {
                /* Read a block of memory from the configuration file into a buffer to be parsed 
                    into individual entries */
                bytes_read = (UINT16)NU_Read(config_fp, fileptr, buffer_size);

                if (bytes_read == 0)
                    break;    

                /* If there has been a partial entry, adjust the fileptr accordingly. */
                if (partial_entry == 1)
                {   
                    fileptr = filebuf_begin;
                    buffer_size = DHCPS_FILE_READ_SIZE;
                    partial_entry = 0;
                }    

                /* If the length is zero, then we are done with the entries */
                for (;;)
                {
                    /* Parse the entries in the file into the configuration structures */
                    status = DHCPS_Get_File_Entry(buffptr, &buff_read, fileptr, bytes_read);

                    if (status != NU_SUCCESS)
                    {
                        /* Is this the end of the file? */
                        if (status == END_OF_FILE)
                            break;

                        else if (status == -1)
                        {
                            /* Only a partial entry was read from the file.  The partial entry
                                must be copied and the pointers adjusted. */
                            memcpy(filebuf_begin, fileptr, (UINT16)bytes_read);

                            /* Adjust the fileptr to the end of the partial entry */
                            fileptr = filebuf_begin + bytes_read;
                          
                            /* Set the buffer size to account for the partial entry */
                            buffer_size = (UINT16)(DHCPS_FILE_READ_SIZE - bytes_read); 

                            /* Set a flag to let us know that we have had a partial entry */
                            partial_entry = 1;

                            break;
                        }                
                    }           
                    fileptr += buff_read; 

                    /* Allocate a block of memory for the control block that we are about to add. */
                    status = NU_Allocate_Memory(DHCPServer_Memory, (VOID **)&config_cb, 
                                sizeof(DHCPS_CONFIG_CB), NU_NO_SUSPEND);

                    if (status != NU_SUCCESS)
                    {
                        /* An error occurred while allocating memory. */
                        status = DHCPSERR_UNABLE_TO_ALLOCATE_MEMORY;
                    }    

                    else
                    {
                        /* Initialize the memory for the new control block. */
                        UTL_Zero(config_cb, sizeof(DHCPS_CONFIG_CB));

                        /* Place the entry into the a control block structure. */
                        status = DHCPS_Process_Configuration_Entry(config_cb, buffer);       

                        /* Ensure that this is a valid control block.  It must have a device 
                            name or be the global control block. */
                        if (config_cb->device_name[0] != NU_NULL)
                        {
                            /* Place the configuration on the list. */
                            dll_ptr = DLL_Enqueue(&DHCPS_Config_List, config_cb);
                        }    

                        else if (strcmp(config_cb->config_entry_name, "GLOBAL") == NU_SUCCESS)
                        {
                            /* Place the configuration on the list. */
                            dll_ptr = DLL_Insert(&DHCPS_Config_List, config_cb, 
                                                DHCPS_Config_List.dhcp_config_head);
                        }    

                        else 
                        {
                            NERRS_Log_Error(NERR_RECOVERABLE, __FILE__, __LINE__);
                        }    

                        /* Ensure that the control block was inserted to the list. */
                        if (dll_ptr == NU_NULL)
                        {
                            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                            break;
                        }    

                        if (status != END_OF_ENTRY)
                        {
                            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                        }           

                        /* If there are no more entries available, then break. */
                        if ((bytes_read - buff_read) <= 0)
                            break;

                        else
                            bytes_read -= buff_read;
                    }    
                }
            }    
            /* Close the configuration file. */
            status = NU_Close(config_fp);
               
            if (status != NU_SUCCESS)
            {
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);        
            } 

            else
            {
                /* Once the config file has been processed, the binding file can be processed */
                fileptr = fbuf;
                buffptr = buffer;
                buffer_size = DHCPS_FILE_READ_SIZE;

                /* Initialize the buffers */
                UTL_Zero(fileptr, DHCPS_FILE_READ_SIZE);
                UTL_Zero(buffptr, DHCPS_FILE_READ_SIZE);

                /* Open the binding file */
                binding_fp = NU_Open((CHAR *)DHCPSrv_Binding_File_Name, (PO_RDONLY), (PS_IREAD));

                /* If the target does not have a disk file system, then the NU_Open should fail.  If
                    the open fails, just move on */
                if (binding_fp >= 0)
                {
                    /* First we must determine if the binding file is a valid one.  If the entire binding 
                        structure was not saved to file, the backup binding file will be used in its place. */
                    for (;;)
                    {
                        /* Read in the binding file to see if the end of file marker has been placed at 
                            the end of the file. */
                        bytes_read = (UINT16)NU_Read(binding_fp, fileptr, buffer_size);

                        if ((bytes_read == 0) && (backup_binding_file == NU_FALSE))
                        {
                            /* The end of binding file marker was not found.  Close the binding file. */  
                            status = NU_Close(binding_fp);

                            if (status != NU_SUCCESS)
                            {
                                /* An error occurred while closing the binding file.  Log an error. */
                                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                            }    

                            /* Use the backup binding file. */
                            binding_fp = NU_Open((CHAR *)DHCPSrv_Binding_File_Backup, (PO_RDONLY), (PS_IREAD));
                            backup_binding_file = NU_TRUE;
                            continue;
                        }    
                        else if ((bytes_read == 0) && (backup_binding_file == NU_TRUE))
                            break;

                        /* Add the bytes read to the total bytes that have been read */
                        total_bytes_read += bytes_read;

                        /* Adjust the fileptr to the end of the written data. */
                        fileptr += bytes_read;

                        /* Backup the fileptr to account for the end of binding marker. */
                        fileptr -= sizeof(UINT32);

                        /* Test to see if this is the end of binding file. */
                        if ((GET8(fileptr,0) == 0x7D) && (GET8(fileptr,1) == 0xC0) &&
                            (GET8(fileptr,2) == 0xC1) && (GET8(fileptr,3) == 0xC2))
                        {
                            /* We have found the end of binding file entry.  This is a valid binding file.
                                The binding file must be closed and re-opened to reset the file pointers. */
                            status = NU_Close(binding_fp);

                            if (status == NU_SUCCESS)
                                /* Reopen the binding file. */
                                binding_fp = NU_Open((CHAR *)DHCPSrv_Binding_File_Name, (PO_RDONLY),
                                                    (PS_IREAD));

                            else
                                /* The binding file did not close properly.  Log an error. */
                                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);

                            break;
                        }    

                        else
                        {
                            /* Reset the file ptr to the begining of the buffer */
                            fileptr = fbuf;
                            continue;
                        }    
                    }   

                    /* Set a pointer to the beginning of the binding file buffer */
                    fileptr = filebuf_begin;

                    if (total_bytes_read > 0)
                    {
                        for (;;)
                        {
                            if (partial_entry != 1)
                                fileptr = filebuf_begin;

                            /* Read the entire binding file into a buffer to be parsed into 
                            individual entries */
                            bytes_read = (UINT16)NU_Read(binding_fp, fileptr, buffer_size);

                            if (bytes_read == 0)
                                break;    

                            /* If there has been a partial entry, adjust the fileptr accordingly. */
                            if (partial_entry == 1)
                            {   
                                fileptr = filebuf_begin;
                                bytes_read = (DHCPS_FILE_READ_SIZE - buffer_size) + bytes_read;
                                buffer_size = DHCPS_FILE_READ_SIZE;
                                partial_entry = 0;
                            }            

                            /* If the length is zero, then we are done with the entries */
                            for (;;)
                            {
                                /* Parse the entries in the file into the binding structure. */
                                status = DHCPS_Get_File_Entry(buffptr, &buff_read, fileptr, bytes_read);

                                /* Set a pointer to the buffer to be used */
                                buffer_pointer = buffptr;

                                if (status != NU_SUCCESS)
                                {
                                    /* Is this the end of the file? */
                                    if (status == END_OF_FILE)
                                        break;

                                    else if (status == -1)
                                    {
                                        /* Only a partial entry was read from the file.  The partial entry
                                            must be copied and the pointers adjusted. */
                                        memcpy(filebuf_begin, fileptr, (UINT16)bytes_read);

                                        /* Adjust the fileptr to the end of the partial entry */
                                        fileptr = filebuf_begin + bytes_read;
                                  
                                        /* Set the buffer size to account for the partial entry */
                                        buffer_size = (UINT16)(DHCPS_FILE_READ_SIZE - bytes_read); 

                                        /* Set a flag to let us know that we have had a partial entry */
                                        partial_entry = 1;

                                        break;
                                    }  

                                    else if (status == NEW_CONFIG_FOR_BINDINGS)
                                    {
                                        /* Get the configuration control block name that is 
                                            associated with this binding. */
                                        status = DHCPSDB_Get_String(&buffer_pointer, binding_config);

                                        DHCPSDB_Adjust_Buffer(&buffer_pointer);
                                    }    
                                }           
                                /* Increment the fileptr past the data that has been copied into the buffer. */
                                fileptr += buff_read;

                                /* Process the binding entry into the binding list. */
                                DHCPS_Process_Binding_Entry(buffer_pointer, binding_config);

                                /* If there are no more entries available, then break. */
                                if ((bytes_read - buff_read) <= 0)
                                    break;

                                else
                                    bytes_read -= buff_read;
                            }
                        }
                    }
                    /* Close the binding file. */        
                    status = NU_Close(binding_fp);

                    if (status != NU_SUCCESS)
                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);        
                }
                
                /* Now, the options file can be processed */
                fileptr = fbuf;
                buffptr = buffer;

                /* Initialize the buffers */
                UTL_Zero(fileptr, DHCPS_FILE_READ_SIZE);
                UTL_Zero(buffptr, DHCPS_FILE_READ_SIZE);

                
                /* Open the options block file */
                options_fp = NU_Open((CHAR *)DHCPSrv_Options_Block_File_Name, (PO_RDONLY), (PS_IREAD));

                /* If the target does not have a disk file system, then the NU_Open should fail.  If
                    the open fails, just move on */
                if (options_fp >= 0)
                {
                    for (;;)
                    {
                       /* Read the entire options block file into a buffer to be parsed into 
                        individual entries */
                        bytes_read = (UINT16)NU_Read(options_fp, fileptr, buffer_size);

                        if (bytes_read == 0)
                            break;    

                        /* If there has been a partial entry, adjust the fileptr accordingly. */
                        if (partial_entry == 1)
                        {   
                            fileptr = filebuf_begin;
                            partial_entry = 0;
                        }            

                        /* If the length is zero, then we are done with the entries */
                        for (;;)
                        {
                           /* Parse the entries in the file into the option structure. */
                            status = DHCPS_Get_File_Entry(buffptr, &buff_read, fileptr, bytes_read);

                            if (status != NU_SUCCESS)
                            {
                                /* Is this the end of the file? */
                                if (status == END_OF_FILE)
                                    break;

                                else if (status == -1)
                                {
                                   /* Only a partial entry was read from the file.  The partial entry
                                        must be copied and the pointers adjusted. */
                                    memcpy(filebuf_begin, fileptr, (UINT16)bytes_read);

                                    /* Adjust the fileptr to the end of the partial entry */
                                    fileptr = filebuf_begin + bytes_read;
                              
                                    /* Set the buffer size to account for the partial entry */
                                    buffer_size = (UINT16)(DHCPS_FILE_READ_SIZE - bytes_read); 

                                    /* Set a flag to let us know that we have had a partial entry */
                                    partial_entry = 1;

                                    break;
                                }                
                            }           

                            /* Increment the fileptr past the data that has been copied into the 
                                buffer. */
                            fileptr += buff_read;

                            /* Set a pointer to the begining of the buffer that contains the entry. */
                            buffer_pointer = buffer;

                            /* Increment the pointer past the opening bracket that denotes the 
                                begining of the option entries. */
                            buffer_pointer++;                

                            /* Get the configuration control block name that is associated with this 
                                option entry. */
                            status = DHCPSDB_Get_String(&buffer_pointer, binding_config);

                            if (status != NU_SUCCESS)
                            {
                                /* Log an error */
                                NERRS_Log_Error(NERR_RECOVERABLE, __FILE__, __LINE__);
                                break;
                            }    

                            /* Process the option block into the options block list. */
                            DHCPS_Process_Options_Block_Entry(buffer_pointer, binding_config);


                            /* If there are no more entries available, then break. */
                            if ((bytes_read - buff_read) <= 0)
                                break;

                            else
                                bytes_read -= buff_read;
                        }
                    }
                    /* Close the binding file. */        
                    status = NU_Close(options_fp);

                    if (status != NU_SUCCESS)
                    {
                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);        
                    }
                }
            }
        }    
        /* Allocate stack space for and create the DHCP Server_Entry task. */
        status = NU_Allocate_Memory(DHCPServer_Memory, &dhcp_server_mem, DHCPS_TASK_STACK_SIZE, 
                                        NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Create the DHCP_Server_Entry task. */
            status = NU_Create_Task(&dhcp_server_task_cb, "DHCPSERVER", DHCPS_Server_Entry,
                                    0, NU_NULL, dhcp_server_mem, DHCPS_TASK_STACK_SIZE, 
                                    NORMAL_TASK_PRIORITY, 0, NU_PREEMPT, NU_NO_START);

            if (status == NU_SUCCESS)
            {
                /* Allocate stack space for and create the DHCPS_Lease_Timer Task. */
                status = NU_Allocate_Memory(DHCPServer_Memory, &dhcpserv_lease_mem, DHCPS_TASK_STACK_SIZE, 
                                            NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Create the DHCPS_Lease_Timer task. */
                    status = NU_Create_Task(&dhcpserv_lease_timer_task_cb, "DHCPSERVLEASETIMER", 
                                            DHCPS_Lease_Timer, 0, NU_NULL, dhcpserv_lease_mem, 
                                            DHCPS_TASK_STACK_SIZE, NORMAL_TASK_PRIORITY, 0, 
                                            NU_PREEMPT, NU_NO_START);

                    if (status == NU_SUCCESS)
                    {
                        /* Start the DHCPS_Lease_Timer task */
                        NU_Resume_Task(&dhcpserv_lease_timer_task_cb);
                    }

                    else
                        NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
                }

                else
                {
                    NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
                    status = DHCPSERR_UNABLE_TO_ALLOCATE_MEMORY;
                }    

                /* Start the DHCP_Server_Entry task */
                NU_Resume_Task(&dhcp_server_task_cb);
            }
            else
                NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
        }    
        else
        {
            NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
            status = DHCPSERR_UNABLE_TO_ALLOCATE_MEMORY;
        }    
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return(status);
}    

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Server_Entry                                        
*                                                                      
* DESCRIPTION  
*                                                        
*     This is the main DHCP server function.  It will create and monitor a socket
*     to the DHCP server port and call the proper function to process any
*     DHCP messages that are received. 
*                                                                      
* INPUTS                                                               
*                                                                      
*     UNSIGNED
*     VOID
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
STATIC VOID DHCPS_Server_Entry(UNSIGNED argc, VOID *argv)
{

    DV_DEVICE_ENTRY     *device;
    DHCPS_CONFIG_CB     *config_cb;
    INT                 socketd;        /* the socket descriptor */
    struct addr_struct  servaddr,       /* holds the server address structure */
                        cliaddr;        /* holds the client address structure */

    struct id_struct    interface_addr, /* IP addr of the interface which received message */
                        gateway_ip,
                        device_addr,
                        ip_addr_any;
    STATUS              status;
    INT32               bytes_received;

    UINT16              sockoptstatus,
                        sockoptlen;
    DHCPLAYER           *dhcp_ptr;
    INT16               clilen;

    UINT8               *ret_int,
                        message_type = 0;
    CHAR                ip_addr[]          = {(CHAR) 0,(CHAR) 0,(CHAR) 0,(CHAR) 0};

    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();    
    
    /* No compilation warnings allowed.  */
    DHCPS_UNUSED_PARAMETER(argc);
    DHCPS_UNUSED_PARAMETER(argv);
    
#ifdef FILE_3_1

    if(NU_Set_Default_Drive(DHCPS_Default_Drive) != NU_SUCCESS)
    {
       NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__); 
       NU_USER_MODE();
       return;
    }

#endif 
    /*  Allocate space for the receiving buffer.  */
    status = NU_Allocate_Memory (DHCPServer_Memory, (VOID **)&dhcp_ptr,
                                 IOSIZE, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Save off a pointer to the dhcp_ptr to a global variable. */
        DHCPS_Message = dhcp_ptr;

        /* open a connection via the socket interface */
        if ((socketd = NU_Socket(NU_FAMILY_IP, NU_TYPE_DGRAM, 0))>=0)
        {
            /* fill in a structure with the server address */
            servaddr.family    = NU_FAMILY_IP;
            servaddr.port      = IPPORT_DHCPS;                  /* DHCP Server Port */
            *(UINT32 *)servaddr.id.is_ip_addrs = IP_ADDR_ANY;
            servaddr.name       = "DHCP_Server";

            /* initialize client address */
            cliaddr.family    = NU_FAMILY_IP;
            cliaddr.port      = IPPORT_DHCPC;                   /* DHCP Client Port */
            cliaddr.id.is_ip_addrs[0]  = (UINT8) 0;
            cliaddr.id.is_ip_addrs[1]  = (UINT8) 0;
            cliaddr.id.is_ip_addrs[2]  = (UINT8) 0;
            cliaddr.id.is_ip_addrs[3]  = (UINT8) 0;
            cliaddr.name = "";


            /* The socket must be setup to return the interface on which the packet
                was received. */
            sockoptlen = sizeof(UINT16);

            sockoptstatus = IP_RECVIFADDR;

            status = NU_Setsockopt(socketd, IPPROTO_IP, IP_RECVIFADDR,
                                &sockoptstatus, (INT)sockoptlen);

            if (status == NU_SUCCESS)
            {
                /*  Bind our address to the socket.  */
                status = NU_Bind(socketd, &servaddr, 0);

                if (status >= 0)
                {

                    /* Save off the socket descriptor to a global variable. */
                    Socket_Descriptor = status;

                    for(;;)
                    {
                        /*  Wait until we receive something from a client. */
                        bytes_received = NU_Recv_From(socketd, (CHAR *)dhcp_ptr,
                                                        IOSIZE, 0, &cliaddr, &clilen);

                        /*  If we got back less than zero there is an error. */
                        if (bytes_received < 0)
                        {
                            NERRS_Log_Error(NERR_INFORMATIONAL, __FILE__, __LINE__);
                            break;
                        }

                        else
                        {
                            /* Get the address of the interface that received the
                                message. This will be the same interface that our
                                reply will need to be sent on. */
                            status = NU_Recv_IF_Addr(socketd,
                                                        interface_addr.is_ip_addrs);

                            if (status != NU_SUCCESS)
                            {
                                NERRS_Log_Error(NERR_RECOVERABLE, __FILE__, __LINE__);
                            }

                            /* We need to determine what kind of message was received. */
                            ret_int = DHCPS_Get_Option(dhcp_ptr, DHCPS_MSG_TYPE);

                            if (ret_int != NU_NULL)
                            {
                                message_type = GET8(ret_int, 0);
                            }

                            /* We must grab the NET semaphore */
                            status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);
                            if (status == NU_SUCCESS)
                            {
                                /* Get the device structure for the network interface that the
                                   discover message was received on. */
                                device = DEV_Get_Dev_By_Addr(interface_addr.is_ip_addrs);

                                if(device != NU_NULL)
                                {
                                    /* Get the gateway address from the DHCPLAYER structure.  If the
                                        address is present, it means that this request was received
                                        from a relay agent. */
#if (defined NET_5_1)                                
                                    memcpy(gateway_ip.is_ip_addrs, dhcp_ptr->dp_giaddr, IP_ADDR_LEN);
#else                                
                                    memcpy(gateway_ip.is_ip_addrs, dhcp_ptr->dp_giaddr.is_ip_addrs, IP_ADDR_LEN);
#endif

                                    memcpy(ip_addr_any.is_ip_addrs, ip_addr, IP_ADDR_LEN);

                                    /* If the gateway IP address is null, then the request came
                                        from a client on the same network segment as the server
                                        interface that received the discover.  Else, we will need
                                        to calculate the subnet of the requesting client. */
                                    status = DHCPSDB_IP_Compare(gateway_ip.is_ip_addrs, ip_addr_any.is_ip_addrs);

                                    if (status == NU_SUCCESS)
                                    {
                                        PUT32(device_addr.is_ip_addrs, 0, device->dev_addr.dev_ip_addr);

                                        /* Find a control block that will match the subnet address of the
                                            device that received the DHCP message. */
                                        config_cb = DHCPS_Find_Configuration_Control_Block(device->dev_net_if_name,
                                                                                        &device_addr);

                                    }

                                    else
                                    {
                                        /* Find a control block that will match the subnet address of the relay
                                            agent. */
                                        config_cb = DHCPS_Find_Configuration_Control_Block(device->dev_net_if_name,
                                                                                      &gateway_ip);
                                    }

                                    /* Release the NET semaphore */
                                    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                                    {
                                        /* Log the error */
                                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                                        break;
                                    }
                                    /* Ensure that a configuration control block has been found. */
                                    if (config_cb != NU_NULL)
                                    {

                                        /* Now to call the proper function to handle the
                                            specific message type */
                                        switch (message_type)
                                        {
                                            case DHCPDISCOVER:
                                                /* We will send a DHCPOFFER in response to a
                                                    DHCPDISCOVER message */
                                                status = DHCPS_Process_Discover(config_cb, dhcp_ptr,
                                                                            socketd, &interface_addr);

                                                if (status != NU_SUCCESS)
                                                {
                                                    NERRS_Log_Error(NERR_INFORMATIONAL, __FILE__,
                                                                                    __LINE__);
                                                }
                                                break;

                                            case DHCPREQUEST:
                                                /* We need to send a DHCPACK in response to the
                                                    DHCPREQUEST message that was received */
                                                status = DHCPS_Process_Request(config_cb, dhcp_ptr,
                                                                    socketd, &interface_addr);

                                                if (status != NU_SUCCESS)
                                                {
                                                    NERRS_Log_Error(NERR_INFORMATIONAL, __FILE__,
                                                                                    __LINE__);
                                                }
                                                break;

                                            case DHCPRELEASE:
                                                /* We must set the client's binding entry as no
                                                    longer in use */
                                                status = DHCPS_Process_Release(config_cb, dhcp_ptr);

                                                if (status != NU_SUCCESS)
                                                {
                                                    NERRS_Log_Error(NERR_INFORMATIONAL, __FILE__,
                                                                            __LINE__);
                                                }
                                                break;

                                            case DHCPDECLINE:
                                                /* We must determine if the address offered to the
                                                    client is in use by a client with a binding
                                                    entry */
                                                DHCPS_Process_Decline(config_cb, dhcp_ptr);
                                                break;

                                            case DHCPINFORM:

                                                /* We will send a DHCPACK in response to a
                                                    DHCPINFORM message */
                                                status = DHCPS_Process_Inform(config_cb, dhcp_ptr,
                                                                        socketd, &interface_addr);
    
                                                if (status != NU_SUCCESS)
                                                {
                                                    NERRS_Log_Error(NERR_INFORMATIONAL, __FILE__,
                                                                            __LINE__);
                                                }
                                                break;

                                            default:

                                                /* The message must not have been a DHCP message
                                                bound for a server.  Just ignore the message */
                                                break;
                                        }
                                    }
                                }
                                else
                                {
                                    /* No device found.  */
                                    NERRS_Log_Error(NERR_RECOVERABLE, __FILE__, __LINE__);

                                    /* Release the NET semaphore */
                                    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                                    {
                                        /* Log the error */
                                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);

                                        break;
                                    }
                                }
                            }
                            else
                            {
                                /* Failed to obtain the NET semaphore */
                                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);

                                break;
                            }
                        }
                    }
                }
                else
                    /* Error during the binding of the socket */
                    NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
            }
            else
                /* The socket has not been setup for proper operation. */
                NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);

        } /* end successful NU_Socket */
    }
    else
        /* Error occurred during the allocation of memory for receiving buffer */
        NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);

    /* Switch back to user mode */
    NU_USER_MODE();
    
}

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Get_Option                                        
*                                                                      
* DESCRIPTION  
*                                                        
*     This function will search the options field of a received message to find
*     desired option and return a pointer to the option value. 
*                                                                      
* INPUTS                                                               
*                                                                      
*     DHCPLAYER *
*     CHAR
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     UINT8 *                                    
*                                                                      
**********************************************************************************/
UINT8 *DHCPS_Get_Option (DHCPLAYER *message, CHAR option_tag)
{
    INT     sname_is_opt = 0,
            file_is_opt = 0;
            
    UINT8   *opt,
            *found = NU_NULL;
            
    UINT8    i,
            option_length;
    

    /* Search option field.*/
    for (i = OPTION_COOKIE_INCREMENT; message->dp_vend[i] != DHCP_END; i++) 
    {

        if (message->dp_vend[i] == option_tag) 
        {
            /* Step past the option tag and the length to set found equal to the
                data value. */
            found = &message->dp_vend[i+2];
            break;
        }

        else if (message->dp_vend[i] == DHCP_END) 
        {
            break;
        }
        else if (message->dp_vend[i] == DHCP_OVERLOAD) 
        {
            /* Step past the option tag and the length */
            i += 2 ;

            if (message->dp_vend[i] == OVERFLOW_IN_FILE)
                file_is_opt = NU_TRUE;

            else if (message->dp_vend[i] == OVERFLOW_IN_SNAME)
                sname_is_opt = NU_TRUE;

            else if (message->dp_vend[i] == OVERFLOW_IN_BOTH)
                file_is_opt = sname_is_opt = NU_TRUE;
            continue;
        }

        else
        {
            /* Increment to the next character, which will be the length of the option. */
            i++;

            /* Get the option data length. */
            option_length = (CHAR)message->dp_vend[i];

            /* Advance the incrementing variable past the option data to the next option tag. */
            i += option_length;
        }
    }
    if (((found == NU_NULL) && ((file_is_opt == NU_TRUE) || (sname_is_opt == NU_TRUE))))
    {
        /* if necessary, search file field   */
        if (file_is_opt) 
        {
           opt = &message->dp_file[0];

            for (i = 0; message->dp_file[i] != DHCP_END; i++) 
            {
                if (*(opt + i) == option_tag) 
                {
                    /* Adjust the pointer past the option tag and the option length
                        and set the data value equal to found */
                    found = (opt + (i+2));
                    break;
                }

                else if (*(opt+i) == DHCP_PAD) 
                {
                    continue;
                }

                else if (*(opt+i) == DHCP_END) 
                {
                    break;
                }

                else
                {
                    /* Increment to the next character, which will be the length of the option. */
                    i++;

                    /* Get the option data length. */
                    option_length = (CHAR)message->dp_file[i];

                    /* Advance the incrementing variable past the option data to the next option tag. */
                    i += option_length;
                }
            }

            if (found != NULL)
                return(found);
        }

        /* if necessary, search sname field  */
        if (sname_is_opt) 
        {
            opt = &message->dp_sname[0];

            for (i = 0; message->dp_sname[i] != DHCP_END; i++) 
            {
                if (*(opt + i) == option_tag) 
                {
                   /* Adjust the pointer past teh option tag and the option length
                        and set the data value equal to found */                    
                   found = (opt + (i+2));
                   break;
                }

                else if (*(opt+i) == DHCP_PAD) 
                {
                    continue;
                }

                else if (*(opt+i) == DHCP_END) 
                {
                   break;
                }

                else
                {
                    /* Increment to the next character, which will be the length of the option. */
                    i++;

                    /* Get the option data length. */
                    option_length = (CHAR)message->dp_file[i];

                    /* Advance the incrementing variable past the option data to the next option tag. */
                    i += option_length;
                }

            }
        }
    }    
    return(found);
}

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Process_Discover                                        
*                                                                      
* DESCRIPTION 
*                                                         
*     This function will process any received DHCPDISCOVER message and respond
*     to the requesting client if necessary
*                                                                      
*                                                                      
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     DHCPLAYER *
*     INT
*     struct id_struct *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS.                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Process_Discover(DHCPS_CONFIG_CB *config_cb, DHCPLAYER *dhcp_ptr, INT socketd, 
                                     struct id_struct *interface_addr)
{
    DHCPS_BINDING               *binding;
    DHCPS_CLIENT_ID             client_id;
    struct id_struct            requested_ip;
    UINT32                      offer_lease_length;
    CHAR                        *temp;
    STATUS                      status = -1;


    /* Search the option field for the requested IP address */
    temp = (CHAR *)DHCPS_Get_Option(dhcp_ptr, DHCP_REQUEST_IP);

    if (temp != NU_NULL)
        /* Store the requested IP address in a structure */
        memcpy(requested_ip.is_ip_addrs, temp, IP_ADDR_LEN);

    else
        /* Ensure that the requested IP address structure is initialized to zero. */
        UTL_Zero(&requested_ip, sizeof(struct id_struct));

    /* Clear the client_id structure */
    memset(&client_id, 0 , sizeof(DHCPS_CLIENT_ID));

    /* Search the option field for the client id and store the id in the 
        client_id struct */
    DHCPS_Get_Client_id(dhcp_ptr, &client_id);

    /* Find a binding to provide to the requesting client */
    if ((binding = DHCPS_Get_Binding_For_Lease(config_cb, &client_id, &requested_ip)) 
                        != NU_NULL)        
    {
        /* Protect each of the global data structures while we are writing to them. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Add the client hardware address to the binding */
        binding->chaddr.hardware_type = (CHAR)dhcp_ptr->dp_htype;
        binding->chaddr.hardware_addr_len = dhcp_ptr->dp_hlen;
        memcpy(binding->chaddr.hardware_addr, dhcp_ptr->dp_chaddr, DADDLEN);

        /* Add the client's id to the binding */
        memcpy(&binding->client_id, &client_id, sizeof(DHCPS_CLIENT_ID));

        /* Unprotect the structures now that we are done. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
       
        /* Chose the duration of the lease to be offered to the requesting client */
        offer_lease_length = DHCPS_Choose_Lease_Length (config_cb, binding);

        /* Protect each of the global data structures while we are writing to them. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);
        
        /* Store the offered_lease_length in the binding structure */
        binding->lease_length = offer_lease_length;

        /* Unprotect the structures now that we are done. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        /* Move the binding to the offered bindings structure so that it can be retrieved 
            when and if a REQUEST message is received */
        binding = DHCPS_Add_Binding_To_Offer(config_cb, binding);

        if (binding != NU_NULL)
        {
            /* Send a DHCPOFFER to the client */
            status = DHCPS_Send(dhcp_ptr, DHCPOFFER, binding, socketd, interface_addr);
        }
        else
        {
            /* Problem with adding binding to offer. */
            NERRS_Log_Error(NERR_INFORMATIONAL, __FILE__, __LINE__);
        }

        if (status != NU_SUCCESS)
        {
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
        }                   
    }   
    return(status);
}

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Process_Request                                        
*                                                                      
* DESCRIPTION
*                                                          
*     This function will process any received DHCPREQUEST messages and responde
*     to the requesting client if necessary.
*                                                                      
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     DHCPLAYER *
*     INT
*     struct id_struct
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Process_Request(const DHCPS_CONFIG_CB *config_cb, DHCPLAYER *dhcp_ptr, 
                             INT socketd, struct id_struct *interface_addr)
{
    DHCPS_BINDING       *binding;
    DHCPS_OFFERED       *accepted_offer,
                        *offered;
    DHCPS_CLIENT_ID     client_id;
    INT                 request_for_me = 0,
                        j,
                        flags;
    CHAR                *option,
                        client_id_type;
    UINT8               test_ip_addr[IP_ADDR_LEN],
                        server_ip[IP_ADDR_LEN],
                        offered_ip_addr[IP_ADDR_LEN],
                        binding_ip_addr[IP_ADDR_LEN],
                        test_hardware_addr[DADDLEN];
    STATUS              status = -1;
    UINT32              clock_time;

    /* Start by checking if this message was just for us or if it may have been broadcast. */
    option = (CHAR *)DHCPS_Get_Option(dhcp_ptr, DHCPS_SERVER_ID);

    /* Copy the server ID from the received packet. */
    if (option != NU_NULL)
    {
        memcpy(server_ip, option, IP_ADDR_LEN);

        /* Protect each of the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        if (DHCPSDB_IP_Compare(config_cb->server_ip_addr, server_ip) == NU_SUCCESS)        
        {
            request_for_me = NU_TRUE;
        }

        /* Unprotect the structures now that we are done. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }    

    /* Get the requested IP address from the client's message. */
    option = (CHAR *)DHCPS_Get_Option(dhcp_ptr, DHCP_REQUEST_IP);

    if (option != NU_NULL)
    {
        memcpy(test_ip_addr, option, IP_ADDR_LEN);

        /* If the server ID option is present, then this message is perhaps in 
            response to a OFFER that was made.  Check to see if there is an offered binding in  
            the offered structure that matches this client */
        if (request_for_me == NU_TRUE)
        {
            /* Walk the offered link list to see if the requested IP address matches an entry. */
            for (offered = config_cb->ip_offer_list.dhcp_offer_head;
                 offered;
                 offered = offered->dhcp_offer_next)
            {   
                /* Ensure that there is an offered binding present. */
                if (offered->offered_binding != NU_NULL )
                {
                    /* Protect the global data structures. */
                    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                    /* Save off the IP address that was offered to a local variable. */
                    memcpy(offered_ip_addr, offered->dp_yiaddr->is_ip_addrs, IP_ADDR_LEN);

                    /* Unprotect the global data structures. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);

                    if (DHCPSDB_IP_Compare(test_ip_addr, offered_ip_addr) == 
                                                NU_SUCCESS)
                    {
                        /* Protect the global data structures. */
                        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                        /* Check to see if the client's hardware address matches the hardware address 
                            in the offered binding */
                        memcpy(test_hardware_addr, (UINT8 *)offered->dp_chaddr->hardware_addr,
                                                                                            DADDLEN);

                        /* Unprotect the global data structures. */
                        NU_Release_Semaphore(&DHCPS_Semaphore);

                        if (memcmp(test_hardware_addr, dhcp_ptr->dp_chaddr, DADDLEN) == NU_SUCCESS)
                        {
                            /* This offered binding has been accepted by the client.  Now, the binding 
                                needs to be marked as in use and the offer removed from the offered 
                                array. */
                            accepted_offer = offered;

                            /* Save off the binding of the offer array */
                            binding = accepted_offer->offered_binding;

                            /* Protect the global data structures. */
                            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                            /* Set the binding as in use */
                            flags = binding->flags;
                            flags &= ~BINDING_OFFERED;
                            flags |= IN_USE;
                            binding->flags = flags;

                            /* Set the end lease time in the binding to the current time plus
                                the lease length. */
                            clock_time = NU_Retrieve_Clock();

                            binding->end_lease_time = ((clock_time/SCK_Ticks_Per_Second) + 
                                                binding->lease_length);

                            /* Set the lease_time_remaining to the lease length */
                            binding->lease_time_remaining = (INT32)binding->lease_length;

                            /* Unprotect the global data structures. */
                            NU_Release_Semaphore(&DHCPS_Semaphore);

                            /* Send a DHCPACK to the client */
                            status = DHCPS_Send(dhcp_ptr, DHCPACK, binding, socketd, interface_addr);

                            if (status == NU_SUCCESS)
                            {
                                /* Clean out the offered structure */
                                DHCPS_Remove_Offer(accepted_offer); 
                                break;
                            }
                        } /* if (memcmp... */   
                    } /* if (DHCPSDB_IP_Compare... */
                }  /* if (offer->offered_binding... */    
            } /* for (offered... */

            return(status);
        } /* if (request_for_me... */
    }   /* if (option != NU_NULL...*/    
    else
    {
        /* Get the IP address that the client is currently using. */
#if (defined NET_5_1)
        memcpy(test_ip_addr, dhcp_ptr->dp_ciaddr, IP_ADDR_LEN);
#else
        memcpy(test_ip_addr, dhcp_ptr->dp_ciaddr.is_ip_addrs, IP_ADDR_LEN);
#endif
    }

    /* This could still be a client that the server has a binding for that is attempting to 
       renew/rebind or is rebooting.  We should search through the binding link list to find a 
       match to the ciaddr */

    for (binding = config_cb->ip_binding_list.dhcp_bind_head;
         binding;
         binding = binding->dhcp_bind_next)
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Save the binding IP address to a local variable. */
        memcpy(binding_ip_addr, binding->yiaddr.is_ip_addrs, IP_ADDR_LEN);

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        if (DHCPSDB_IP_Compare(test_ip_addr, binding_ip_addr) ==  NU_SUCCESS)
        {
            /* Get the client ID of the requesting client. */
            DHCPS_Get_Client_id(dhcp_ptr, &client_id);

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Save off the client ID type to a local variable. */
            client_id_type = binding->client_id.idtype;

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            /* Ensure that it is the same client that currently holds a lease on the 
               binding that sent the request.  If not, we must send a DHCPNAK to the 
               requesting client. */
            if (client_id_type == client_id.idtype)
            {
                for (j = 0; j < (client_id.idlen); j++)
                {
                    /* Protect the global data structures. */
                    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                    if (binding->client_id.id[j] != client_id.id[j])
                        break;

                    /* Unprotect the global data structures. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);
                }

                if (j != (client_id.idlen))
                {      
                    /* Protect the global data structures. */
                    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                    /* Save the current config control block into as a binding pointer so
                        that it may be used to process the DHCPNAK message. */
                    binding = (DHCPS_BINDING *)config_cb;  

                    /* Unprotect the global data structures. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);

                    /* We must send the requesting client a DHCPNAK message to
                        inform them they are improperly requesting an address. */
                    status = DHCPS_Send(dhcp_ptr, DHCPNAK, binding, socketd, interface_addr);
                }

                else
                {
                    /* Get the current time from the system clock. */
                    clock_time = NU_Retrieve_Clock();

                    /* Protect the global data structures. */
                    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                    /* Calculate the number of seconds till the lease will expire. */
                    binding->end_lease_time = ((clock_time/SCK_Ticks_Per_Second) + 
                                                        binding->lease_length);

                    /* Set the lease time remaining to the full length of the lease. */
                    binding->lease_time_remaining = (INT32)binding->lease_length;

                    /* Set the binding as IN_USE */
                    binding->flags |= IN_USE;

                    /* Unprotect global data structures. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);

                    /* Send a DHCPACK to the client */
                    status = DHCPS_Send(dhcp_ptr, DHCPACK, binding, socketd, 
                                            interface_addr);

                    /* We are done.  Get out. */    
                    break;

                } /* else */    
            } /* if (binding->... */
            else
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Save the current config control block into as a binding pointer so
                    that it may be used to process the DHCPNAK message. */
                binding = (DHCPS_BINDING *)config_cb;

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);

                /* Since the client ID types do not match, we must send the requesting client 
                    a DHCPNAK message to inform them they are improperly requesting an address. */
                status = DHCPS_Send(dhcp_ptr, DHCPNAK, binding, socketd, interface_addr);
            }    
        } /* if (DHCPSDB_IP_Compare... */
    } /* for (binding... */
    return(status);
}

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Process_Decline                                        
*                                                                      
* DESCRIPTION 
*                                                         
*     This function will process any DHCPDECLINE messages received from clients
*     and take the necessary action on the binding that was offered to the
*     client.
*                                                                      
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     DHCPLAYER *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
VOID DHCPS_Process_Decline (const DHCPS_CONFIG_CB *config_cb, DHCPLAYER *dhcp_ptr)
{
    DHCPS_CLIENT_HADDR  chaddr;
    DHCPS_BINDING       *binding;
    struct  id_struct   declined_ip_addr;
    CHAR                *option;
    UINT8               server_ip[IP_ADDR_LEN],
                        binding_haddr[DADDLEN],
                        binding_ip_addr[IP_ADDR_LEN],
                        request_for_me = NU_FALSE;

    UINT32              clock_time;

    /* DECLINE for this server? */
    option = (CHAR *)DHCPS_Get_Option(dhcp_ptr, DHCPS_SERVER_ID);

    if (option != NU_NULL)
    {
        /* Copy the server ID from the received packet. */
        memcpy(server_ip, option, IP_ADDR_LEN);

        if (DHCPSDB_IP_Compare (config_cb->server_ip_addr, server_ip) == NU_SUCCESS)
        {
            request_for_me = NU_TRUE;
        }
    
        if (request_for_me)
        {
            /* Get the client's hardware address to assist in determining which binding was
                offered to the client */
            memcpy(chaddr.hardware_addr, dhcp_ptr->dp_chaddr, DADDLEN);

            /* Get the declined IP address to use in identifying the binding that was declined
                by the client */
            option = (CHAR *)DHCPS_Get_Option(dhcp_ptr, DHCP_REQUEST_IP);

            if (option != NU_NULL)
            {
                /* Copy the requested IP address into an id_struct. */
                memcpy(declined_ip_addr.is_ip_addrs, option, IP_ADDR_LEN);

                /* Search for the client's hardware address in the binding link list */
                for (binding = config_cb->ip_binding_list.dhcp_bind_head;
                     binding;
                     binding = binding->dhcp_bind_next)
                {
                    /* Protect the global data structures. */
                    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                    /* Save the binding hardware address to a local array. */
                    memcpy(binding_haddr, binding->chaddr.hardware_addr, DADDLEN);

                    /* Unprotect the global data structures. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);

                    /* Compare the client's hardware address with the hardware addresses in the offered
                        link list in an attempt to find a match. */
                    if (memcmp(chaddr.hardware_addr, binding_haddr, DADDLEN) == NU_SUCCESS)
                    {
                        /* Protect the global data structures. */
                        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                        /* Save the binding IP address to a local array. */
                        memcpy(binding_ip_addr, binding->yiaddr.is_ip_addrs, IP_ADDR_LEN);

                        /* Unprotect the global data structures. */
                        NU_Release_Semaphore(&DHCPS_Semaphore);

                        /* Check to see if this offered IP address is the binding in this offer */
                        if (DHCPSDB_IP_Compare(declined_ip_addr.is_ip_addrs, binding_ip_addr) == 0)
                        {
                            /* Protect the global data structures. */
                            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                            /* This is the binding that was declined.  Set the binding as IN_USE
                                and set the lease time to the default declined time to allow the illegal
                                client time to relinquish the IP address */
                            binding->flags |= IN_USE;

                            /* Set the lease length as the default wait time for a lease that has
                                been declined by a client. */
                            binding->lease_length = DHCPS_DEFAULT_DECLINED_LEASE_LENGTH;

                            /* Get the current clock. */
                            clock_time = NU_Retrieve_Clock();

                            /* Save off the time the lease will expire to the binding structure. */
                            binding->end_lease_time = ((clock_time/SCK_Ticks_Per_Second) +
                                                        binding->lease_length);

                            /* Set the remaining lease time as the total lease time. */
                            binding->lease_time_remaining = (INT32)binding->lease_length;

                            /* Unprotect the global data structures. */
                            NU_Release_Semaphore(&DHCPS_Semaphore);

                            break;
                        }
                    }
                }
            }
        }
    }
    return;
}

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Process_Release                                        
*                                                                      
* DESCRIPTION 
*                                                         
*     This function will process any DHCPRELEASE message that is received and 
*     mark the affected binding as available.                                               
*                                                                      
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     DHCPLAYER *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Process_Release(const DHCPS_CONFIG_CB *config_cb, DHCPLAYER *dhcp_ptr)
{
    DHCPS_BINDING       *binding;
    CHAR                *option;
    INT                 request_for_me = NU_FALSE;
    UINT8               server_ip[IP_ADDR_LEN],
                        config_serv_ip[IP_ADDR_LEN];
    STATUS              ret_status = -1;
                                

    /* Release for this server? */
    option = (CHAR *)DHCPS_Get_Option(dhcp_ptr, DHCPS_SERVER_ID);

    if (option != NU_NULL)
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Save the server IP address from the control block to a local variable. */
        memcpy(config_serv_ip, config_cb->server_ip_addr, IP_ADDR_LEN);

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        /* Copy the server IP address from the message to the server_ip variable. */
        memcpy(server_ip, option, IP_ADDR_LEN);

        if (DHCPSDB_IP_Compare (config_serv_ip, server_ip) == NU_SUCCESS)
        {
            request_for_me = NU_TRUE;
        }

        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        if (request_for_me)
        {
            /* We need to find which binding contains the client's IP address and set it
                available for use */
            for (binding = config_cb->ip_binding_list.dhcp_bind_head;
                 binding;
                 binding = binding->dhcp_bind_next)
            {
#if (defined NET_5_1)
                if (DHCPSDB_IP_Compare(binding->yiaddr.is_ip_addrs,
                    dhcp_ptr->dp_ciaddr) == NU_SUCCESS)
#else
                if (DHCPSDB_IP_Compare(binding->yiaddr.is_ip_addrs,
                    dhcp_ptr->dp_ciaddr.is_ip_addrs) == NU_SUCCESS)
#endif
                {
                    /* Set the flags for this binding to available */
                    binding->flags = binding->flags & ~IN_USE;

                    /* Clear the start lease time and lease length from the binding */
                    binding->end_lease_time = 0;
                    binding->lease_time_remaining = 0;

                    ret_status = NU_SUCCESS;
                    break;
                }
            }   
        }

        else
        {
            /* To provide support for some non RFC-compliant DHCP clients that do not
                include the DHCP server ID inside the message, we will search the binding
                list to check if the binding is indeed owned by this server. */
            for (binding = config_cb->ip_binding_list.dhcp_bind_head;
                 binding;
                 binding = binding->dhcp_bind_next)
            {
#if (defined NET_5_1)
                if (DHCPSDB_IP_Compare(binding->yiaddr.is_ip_addrs,
                    dhcp_ptr->dp_ciaddr) == NU_SUCCESS)
#else
                if (DHCPSDB_IP_Compare(binding->yiaddr.is_ip_addrs,
                    dhcp_ptr->dp_ciaddr.is_ip_addrs) == NU_SUCCESS)
#endif
                {
                    /* Set the flags for this binding to available */
                    binding->flags = binding->flags & ~IN_USE;

                    /* Clear the start lease time and lease length from the binding */
                    binding->end_lease_time = 0;
                    binding->lease_time_remaining = 0;

                    ret_status = NU_SUCCESS;
                    break;
                }
            }   
        }

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }

    return(ret_status);
}


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Process_Inform                                        
*                                                                      
* DESCRIPTION 
*                                                         
*     This function will process any DHCPINFORM messages that are received and
*     provide the requesting clients with the requested information.
*                                                                      
*                                                                      
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     DHCPLAYER *
*     INT
*     struct id_struct
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Process_Inform(DHCPS_CONFIG_CB *config_cb, const DHCPLAYER *dhcp_ptr, INT socketd, 
                                     struct id_struct *interface_addr)
{
    DHCPS_BINDING               *binding = NU_NULL;
    STATUS                      status;
    
    /* Set the binding pointer that will be passed to the send function equal
        to the configuration control block, since we are not offering a binding. */
    binding = (DHCPS_BINDING *) config_cb;

    /* Send a DHCPACK to the client */
    status = DHCPS_Send(dhcp_ptr, DHCPINFORM, binding, socketd, interface_addr);

    if (status != NU_SUCCESS)
    {
        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
    }                   

    return(status);
}

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Get_File_Entry                                        
*                                                                      
* DESCRIPTION 
*                                                         
*     This function is responsible for reading one complete entry, either 
*     configuration, binding, or option, from the buffer that the entire
*     file was copied to.
*                                                                      
* INPUTS                                                               
*                                                                      
*     CHAR *
*     UINT16
*     CHAR
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Get_File_Entry(CHAR *buffer, UINT16 *bufsiz, const CHAR *fp, INT bytes_in_buff)
{
    UINT16  length = 0;
    CHAR    c;
    STATUS  ret_status = -1;

    c = *fp;

    /* Test to see if the first character is an open bracket */
    if (c == OPEN_BRACKET)
    {
        ret_status = NEW_CONFIG_FOR_BINDINGS;
        fp++;
        length++;
        bytes_in_buff--;
    }    

    while (bytes_in_buff)
    {
        c = *fp;

        /* If the close bracket has not been encountered, continue to get characters */
        if (c != CLOSE_BRACKET)
        {
            *buffer++ = c;              /* Store other characters */
            length++;
            fp++;
        }

        /* If the END_OF_CONFIG has been encountered, we need to continue to get more 
            bindings. */
        else if ((GET8((fp - 3),0) == 0x3A) && (GET8((fp - 2),0) == 0x3A)
                                            && (GET8((fp - 1),0) == 0x24))
        {
            *buffer++ = '}';                        /* Terminate string */
            fp++;                                   /* Increment the file pointer. */
            length++;                               /* Increment past the end of entry char */
            *bufsiz = length;                       /* Tell the caller its length */            
            ret_status = NU_SUCCESS;
            break;
        }

        /* If the END_OF_BINDING has been encountered, we need to continue to get more 
            bindings. */
        else if ((GET8((fp - 3),0) == 0x3A) && (GET8((fp - 2),0) == 0x3A)
                                            && (GET8((fp - 1),0) == 0x40))
        {
            /* Decrement the buffer pointer so that we can write over one of the ':' and 
                the '@'. */
            buffer -= 2;

            /* Copy a semi-colon into the buffer. */
            *buffer++ = '}';

            /* Increment the file pointer past the close bracket. */
            fp++;
            length++;
            
            /* Tell the caller its length */
            *bufsiz = length;

            if (ret_status == NEW_CONFIG_FOR_BINDINGS)
                break;
            else
            {
                ret_status = NU_SUCCESS;
                break;
            }
        }    

        /* If the END_OF_OPTIONS has been encountered, we need to break out so that
            it can be processed. */
        else if ((GET8((fp - 3),0) == 0x3A) && (GET8((fp - 2),0) == 0x3A)
                                            && (GET8((fp - 1),0) == 0x23))
        {
            /* Decrement the buffer pointer so that we can write over one of the ':' and 
                the '#'. */
            buffer -= 2;
            
            /* Copy a close bracket into the buffer. */
            *buffer++ = '}';

            /* Increment the file pointer past the close bracket. */
            fp++;

            /* Increment the length. */
            length++;

            /* Tell the caller its length */            
            *bufsiz = length;

            /* Increment the file pointer past the close bracket. */
            fp++;            

            ret_status = NU_SUCCESS;
            break;
        }    

        /* If this is the end of file, return the end of file status */
        else if ((GET8((fp + 1),0) == 0xC0) && (GET8((fp + 2),0) == 0xC1)
                                            && (GET8((fp + 3),0) == 0xC2))
        {
            /* This is the end of the file */
            *bufsiz = length + 4;
            ret_status = END_OF_FILE;
            break;
        } 

        /* The file data might have been corrupted. */
        else 
        {
            *buffer++ = c;              /* Store other characters */
            length++;
            fp++;
            bytes_in_buff--;
        } 

        /* Decrement the number of bytes left in the buffer */
        bytes_in_buff--;
    }   
    
    return(ret_status);
}


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Process_Configuration_Entry                                        
*                                                                      
* DESCRIPTION 
*                                                         
*     This function will take the configuration control block entry and copy it
*     into a control block and placing it on the link list.
*                                                                      
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     CHAR *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Process_Configuration_Entry(DHCPS_CONFIG_CB *config, CHAR *buffer)
{
    CHAR    entry_name[DHCPS_MAX_ENTRY_NAME_LENGTH];
            
    STATUS  ret_status;
    
    /* Ensure that both pointers past in are not NULL */
    if ( (config != NU_NULL) && (buffer != NU_NULL) ) 
    {
        /* Increment the buffer pointer past the initial open bracket */
        if (*buffer == OPEN_BRACKET)
            buffer += 1;

        /* Set the entry name of the configuration structure as the name of the file
            entry */
        ret_status = DHCPSDB_Get_String(&buffer, entry_name);

        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Copy the entry into the control block. */
        strcpy(config->config_entry_name, entry_name);

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        /* Move the buffer pointer past the colon following the entry name */
        buffer++;

        for (;;) 
        {
            /* Adjust the buffer pointer to just past the entry delimiter (:) */
            DHCPSDB_Adjust_Buffer(&buffer);

            /* Determine the configuration parameter that the pointer 'buffer'
                is pointing at and call proper function to store the parameter's 
                value in the configuration structure */
            ret_status = DHCPS_Evaluate_Symbol(&buffer, config);

            if (ret_status != NU_SUCCESS)
                break;
        }
    }
    else
        ret_status = -1;

    return (ret_status);
}
/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Evaluate_Symbol                                        
*                                                                      
* DESCRIPTION 
*                                                         
*     This function is responsible for determining the function to handle each of
*     the file entries that are read in.
*                                                                      
* INPUTS                                                               
*                                                                      
*     CHAR **
*     DHCPS_CONFIG_CB *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Evaluate_Symbol(CHAR **symbol, DHCPS_CONFIG_CB *config)
{
    struct OPTIONMAP    optionptr;    
    CHAR                optioncode[5];    
    STATUS              status,
                        ret_status = -1;
 
    /* Determine if a proper option type is where the pointer is pointing */
    if (((*symbol)[0] == '$') && ((*symbol)[1] == '}'))
       ret_status = END_OF_ENTRY;

    else if ((*symbol)[0] == ':') 
       ret_status = NU_SUCCESS;

    else
    {
        DHCPSDB_Eat_Whitespace(symbol);

        /* Move the option tag into a local variable */
        optioncode[0] = (*symbol)[0]; 
        optioncode[1] = (*symbol)[1]; 
        optioncode[2] = (*symbol)[2]; 
        optioncode[3] = (*symbol)[3]; 
        optioncode[4] = 0x00;               /* Null terminate the variable */

        /* Skip past the option tag and the = character to point to the data */
        (*symbol) += 5;

        /* Call the function to determine the correct handler for this option code */
        status = DHCPSDB_Supported_Options_List(optioncode, &optionptr);            

        if (status != NU_SUCCESS)
        {
            NERRS_Log_Error(NERR_RECOVERABLE, __FILE__, __LINE__);
        }   

        if (optionptr.func != NULL)                 
        {
           status = optionptr.func(optionptr.option_name, symbol, config);               
        }

        ret_status = status;
    }    

    return(ret_status);
}


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Process_Binding_Entry
*                                                                      
* DESCRIPTION 
*                                                         
*     This function will handle the copying of the binding entries read from the
*     file into a binding link list.
*                                                                      
* INPUTS                                                               
*                                                                      
*     CHAR *
*     CHAR *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
VOID DHCPS_Process_Binding_Entry(CHAR *buffptr, const CHAR *config_name)
{
    DHCPS_BINDING       *binding;
    DHCPS_OFFERED       *offered;
    DHCPS_CONFIG_CB     *config_cb;
    CHAR                entry_name[DHCPS_MAX_ENTRY_NAME_LENGTH];
    UINT32              current_clock;
    UINT8               flags;
    STATUS              status;
    VOID                *dll_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Search through the control block link list to find a match for the entry name. */
    for (config_cb = DHCPS_Config_List.dhcp_config_head;
         config_cb;
         config_cb = config_cb->dhcps_config_next)
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Copy the configuration entry name to a local variable. */
        strcpy(entry_name, config_cb->config_entry_name);

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        /* Is this a match for the entry name we are looking for. */
        if (strcmp(config_name, entry_name) == NU_SUCCESS)
        {
            /* We now have the control block where the binding entries will be listed. */
            break;
        }
    }

    if (config_cb != NU_NULL)
    {
        /* Loop through the binding entries until they have all been placed in the binding list. */
        while (*buffptr != CLOSE_BRACKET)
        {
            /* Allocate a new binding structure for the entry to be stored in. */
            status = NU_Allocate_Memory(DHCPServer_Memory, (VOID **)&binding, sizeof(DHCPS_BINDING), 
                                            NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Allocate a new offered structure that will be used when this binding is presented
                    to a requesting client. */
                status = NU_Allocate_Memory(DHCPServer_Memory, (VOID **)&offered, sizeof(DHCPS_OFFERED),
                                                NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Initialize the memory for the new binding structure. */
                    UTL_Zero(binding, sizeof(DHCPS_BINDING));

                    /* Initialize the memory for the new offered structure */
                    UTL_Zero(offered, sizeof(DHCPS_OFFERED));

                    /* Protect the global data structures. */
                    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                    /* Add the new offered structure to the link list. */
                    dll_ptr = DLL_Enqueue(&config_cb->ip_offer_list, offered);

                    /* Unprotect the global data structures. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);

                    if (dll_ptr != NU_NULL)
                    {
                        /* Copy the binding entry into a structure. */
                        memcpy(binding, buffptr, sizeof(DHCPS_BINDING));

                        /* Increment the buffptr by the number of bytes that have been copied. */
                        buffptr += sizeof(DHCPS_BINDING);

                        /* Increment past the binding entry delimiter (:). */
                        buffptr++;

                        /* Protect the global data structures. */
                        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);
                        
                        /* Place the binding into the link list. */
                        dll_ptr = DLL_Enqueue(&config_cb->ip_binding_list, binding);

                        /* Unprotect the global data structures. */
                        NU_Release_Semaphore(&DHCPS_Semaphore);

                        if (dll_ptr != NU_NULL)
                        {
                            /* Protect the global data structures. */
                            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                            /* Set the configuration control block pointer to point at the control block
                                that matches the entry name. */
                            binding->config = config_cb;

                            /* If the binding was in use at the time the binding entry was saved, then 
                                the end lease time must be calculated. */
                            flags = IN_USE;

                            if ((binding->flags & flags) != 0)
                            {
                                /* Setup the end of lease and time remaining times */
                                binding->lease_time_remaining = binding->lease_time_remaining;

                                /* Retrieve the clock ticks */
                                current_clock = NU_Retrieve_Clock();

                                /* Calculate the end_lease_time and save it to the binding. */
                                binding->end_lease_time = ((current_clock/SCK_Ticks_Per_Second) + 
                                                            (UINT32)binding->lease_time_remaining);
                            } 

                            /* Unprotect the global data structures. */
                            NU_Release_Semaphore(&DHCPS_Semaphore);
                        }

                        else
                        {
                            /* Since we were unable to add the binding to the link list, deallocate
                                the memory for the binding and the offered struct and log an error. */
                            status = NU_Deallocate_Memory(binding);

                            if (status != NU_SUCCESS)
                            {
                                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                            }    

                            /* Protect the global data structures. */
                            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                            /* Remove the offered struct from the link list. */
                            dll_ptr = DLL_Remove(&config_cb, offered);

                            /* Unprotect the global data structures. */
                            NU_Release_Semaphore(&DHCPS_Semaphore);

                            if (dll_ptr != NU_NULL)
                            {
                                status = NU_Deallocate_Memory(offered);

                                if (status != NU_SUCCESS)
                                {
                                    NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                                }    
                            }

                            else
                            {
                                /* Unable to remove the offered struct from the link list. 
                                    Log an error. */
                                NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
                            }    
                            NERRS_Log_Error(NERR_RECOVERABLE, __FILE__, __LINE__);
                        }    
                    }
                    else
                    {
                        /* Since we were unable to add the offered structure to the link list,
                            we must deallocate both the offered structure and the binding 
                            structure. */
                        status = NU_Deallocate_Memory(offered);

                        if (status == NU_SUCCESS)
                        {
                            status = NU_Deallocate_Memory(binding);

                            if (status != NU_SUCCESS)
                            {
                                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);    
                            } 
                        }

                        else
                        {
                            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);    
                        }    

                        NERRS_Log_Error(NERR_RECOVERABLE, __FILE__, __LINE__);
                    }    
                }
                else
                {
                   /* Log an error since we were unable to allocate memory. */
                    NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
                }    
            }
            else
            {
                /* Log an error since we were unable to allocate memory. */
                NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
            }    
        }
    }

    /* Switch back to user mode */
    NU_USER_MODE();
}

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Process_Options_Block_Entry
*                                                                      
* DESCRIPTION                                                          
*     This function will handle the copying of the option memory blocks into
*     the proper configuration control block.
*                                                                      
* INPUTS                                                               
*                                                                      
*     CHAR *
*     CHAR *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
VOID DHCPS_Process_Options_Block_Entry(CHAR *buffptr, const CHAR *config_name)
{
    DHCPS_OPTIONS       *options;
    DHCPS_CONFIG_CB     *config_cb;
    STATUS              status;
    VOID                *dll_ptr;
    UINT8               *option_block_ptr;
    CHAR                entry_name[DHCPS_MAX_ENTRY_NAME_LENGTH];

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Search through the control block link list to find a match for the entry name. */
    for (config_cb = DHCPS_Config_List.dhcp_config_head;
         config_cb;
         config_cb = config_cb->dhcps_config_next)
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Copy the configuration entry name to a local variable. */
        strcpy(entry_name, config_cb->config_entry_name);

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
       
        /* Is this a match for the entry name we are looking for. */
        if (strcmp(config_name, entry_name) == NU_SUCCESS)
        {
            /* We now have the control block where the binding entries will be listed. */
            break;
        }
    }

    if (config_cb != NU_NULL)
    {
        /* Adjust the buffer so that it will point at the begining of the first options block entry. */
        DHCPSDB_Adjust_Buffer(&buffptr);

        /* Loop through the binding entries until they have all been placed in the binding list. */
        while (*buffptr != CLOSE_BRACKET)
        {
            /* Allocate a new options structure for the entry to be stored in. */
            status = NU_Allocate_Memory(DHCPServer_Memory, (VOID **)&options, sizeof(DHCPS_OPTIONS), 
                                            NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Allocate a new options structure for the entry to be stored in. */
                status = NU_Allocate_Memory(DHCPServer_Memory, (VOID **)&option_block_ptr, 
                                DHCPS_OPTIONS_MEMORY_BLOCK_SIZE, NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Protect the global data structures. */
                    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                    /* Initialize the memory for the new binding structure. */
                    UTL_Zero(options, sizeof(DHCPS_OPTIONS));

                    /* Copy the binding entry into a structure. */
                    memcpy(options, buffptr, sizeof(DHCPS_OPTIONS));

                    /* Increment the buffptr by the number of bytes that have been copied. */
                    buffptr += sizeof(DHCPS_OPTIONS);

                    /* Increment past the option block delimiter ([). */
                    buffptr++;

                    /* Copy the option block into the memory that we allocated. */
                    memcpy(option_block_ptr, buffptr, options->bytes_written);

                    /* Set the option block pointer in the structure to the new memory. */
                    options->options_ptr = option_block_ptr;

                    /* Increment the pointer past the option block and it's delimiter. */
                    buffptr += options->bytes_written + 1;

                    /* Place the option block into the link list. */
                    dll_ptr = DLL_Enqueue(&config_cb->options_buff, options);

                    /* Unprotect the global data structures. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);

                    if (dll_ptr == NU_NULL)
                    {
                        /* Since we were unable to add the option to the link list, deallocate
                            the memory for the option and log an error. */
                        status = NU_Deallocate_Memory(options);

                        if (status != NU_SUCCESS)
                        {
                            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                        }    

                        NERRS_Log_Error(NERR_RECOVERABLE, __FILE__, __LINE__);                    
                    }    
                }
                else
                {
                    /* Log an error since we were unable to allocate memory. */
                    NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
                }    
            }
            else
            {
                /* Log an error since we were unable to allocate memory. */
                NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
            }    

        }
    }
    /* Switch back to user mode */
    NU_USER_MODE();
}

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Get_Client_id                                        
*                                                                      
* DESCRIPTION
*                                                          
*     Search the received DHCP message for the client ID and copy it into
*     the ID structure that was passed in.
*                                                                      
* INPUTS                                                               
*                                                                      
*     DHCPSLAYER *
*     DHCPS_CLIENT_ID *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
VOID DHCPS_Get_Client_id(DHCPLAYER *message, DHCPS_CLIENT_ID *cid)
{
    CHAR    *option;
    INT     i;


    if ((option = (CHAR *)DHCPS_Get_Option(message, DHCP_CLIENT_CLASS_ID)) != NULL) 
    {
        /* If the id type is not 0, then the client id is a hardware address of the device
            of the client */
        if (*option == 0x01)
        {
            /* Due to the fact that the function DHCPServer_Get_Option will return a pointer
            to the data, it is neccessary to decrement the pointer by 1 to obtain the 
            length of the client id. Also, the idlen includes the idtype. Therefore, 
            the actual id is only idlen - 1 in length. */
            cid->idlen = (*(UINT8 *)((option) - 1) - 1);        /* Length of client ID */

            cid->idtype = *(option);                            /* The type of client ID */

            /* The idlen includes the idtype. Therefore, the actual id is only 
                                      idlen - 1 in length. */
            for(i = 0; (i < cid->idlen) && (i < DHCPS_MAX_CLIENT_ID_LENGTH); i++)
            {
                cid->id[i] = (UINT8)option[i+1];                /* Actual ID */
            }
        }    
        else
        {
            /* The client id is a string of unique characters that can be used to identify
                                     the client. */
            cid->idlen = *(UINT8 *)((option) - 1);              /* Length of client ID */

            cid->idtype = 0;                                    /* The type of client ID */

            for(i = 0; (i < cid->idlen) && (i < DHCPS_MAX_CLIENT_ID_LENGTH) ; i++)
            {
                cid->id[i] = (UINT8)option[i];                  /* Actual ID */
            }
        }    
    }

    else 
    {      
        /* Since no client id was present, the haddr is used as a substitute. */
        cid->idlen = message->dp_hlen;
        cid->idtype = (CHAR)message->dp_htype;
        for(i = 0; i < cid->idlen; i++)
            cid->id[i] = message->dp_chaddr[i];
    }

  return;
}

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Get_Binding_For_Lease                                        
*                                                                      
* DESCRIPTION   
*                                                       
*     Search for an available binding to offer to the requesting client.  This
*     function will first search through the static bindings to ensure that
*     the requesting client does not have a reserved binding.  Then, the 
*     search will continue through the rest of the link list until an 
*     available binding is found.
*                                                                      
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     DHCPS_CLIENT_ID *
*     struct id_struct *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     DHCPS_BINDING *                                    
*                                                                      
**********************************************************************************/
DHCPS_BINDING *DHCPS_Get_Binding_For_Lease(const DHCPS_CONFIG_CB *config_cb, 
                                           const DHCPS_CLIENT_ID *client_id, 
                                           const struct id_struct *requested_ip)
{

    UINT8           j,
                    match_found = NU_FALSE,
                    binding_ip_addr[IP_ADDR_LEN];
    INT             flags;
    DHCPS_BINDING   *binding = NU_NULL,
                    *temp_binding = NU_NULL,
                    *ret_binding = NU_NULL;
    DHCPS_OFFERED   *offered_binding;
    CHAR            client_type;

    /* Walk the binding link list to find if the requesting client has previously held
        a lease with this server. */
    for (binding = config_cb->ip_binding_list.dhcp_bind_head;
         binding;
         binding = binding->dhcp_bind_next)
    {
        /* Update the lease times and set expired leases as available.  This should be
            done so that any available addresses can be used by the server. */
        DHCPS_Update_Lease_Times(binding);

        /* Get the status flags for the binding. */
        flags = binding->flags;

        /* We will first search through the static bindings to see if there is a match. */
        flags = flags & STATIC_ENTRY;

        if (flags != 0)
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Now we will compare the client ID of the binding with the requesting client ID. */
            if (binding->client_id.idtype == client_id->idtype)
            {
                for (j = 0; j < (client_id->idlen); j++)
                {
                    if (binding->client_id.id[j] != client_id->id[j])
                        break;
                }

                if (j == (client_id->idlen))
                {
                    /* Set the match_found flag. */
                    match_found = NU_TRUE;
                    ret_binding = binding;
                    /* Unprotect the global data structures. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);
                    break;
                }
            }
            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }    

        else
        {
            /* There are no more static bindings. */
            break;
        } 
    }

    /* If a match was not found in the static bindings, continue walking the list and search for the
        requested IP address.  If the requested IP address is in use, offer the client any IP address
        that is not in use. */
    if (match_found != NU_TRUE)
    {
        /* Continue walking the binding link list from the end of the static binding and search 
            for the requested IP. */
        for (binding = binding;
             binding;
             binding = binding->dhcp_bind_next)
        {
            /* Update the lease times and set expired leases as available.  This should be
                done so that any available addresses can be used by the server. */
            DHCPS_Update_Lease_Times(binding);

            /* Get the status flags for the binding. */
            flags = binding->flags;

            /* Check the status flags to see if the binding is in use. */
            flags = flags & IN_USE;

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the binding IP address to a local variable. */
            memcpy(binding_ip_addr, binding->yiaddr.is_ip_addrs, IP_ADDR_LEN);

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            /* Compare the IP address of this binding with the requested IP address. */
            if (DHCPSDB_IP_Compare(binding_ip_addr, requested_ip->is_ip_addrs) 
                                    == NU_SUCCESS)
            {
                /* If the requested IP address is not in use, then we can offer the requested IP. */
                if (flags == 0)
                {
                    match_found = NU_TRUE;

                    ret_binding = binding;

                    break;
                }    

                else
                {
                    /* Protect the global data structures. */
                    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                    /* Copy the client ID type to a local variable. */
                    client_type = binding->client_id.idtype;

                    /* Unprotect the global data structures. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);

                    /* We have found a match for the requested IP.  Now, verify that it is not
                        in use by another client. */
                    if (client_type == client_id->idtype)
                    {
                        /* Protect the global data structures. */
                        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                        for (j = 0; j < (client_id->idlen); j++)
                        {
                            if (binding->client_id.id[j] != client_id->id[j])
                                break;
                        }

                        /* Unprotect the global data structures. */
                        NU_Release_Semaphore(&DHCPS_Semaphore);

                        if (j == (client_id->idlen))
                        {
                            /* This binding is for the requesting client. */
                            match_found = NU_TRUE;
                            ret_binding = binding;
                            break;
                        }
                    }    
                    
                    else
                    {
                        /* Set a flag that signifies that the requested IP address has been found. */
                        match_found = NU_TRUE;

                        /* If a free IP has not been found yet, continue the search, else break out. */
                        if (temp_binding == NU_NULL)
                        {
                            /* Continue the search of the link list. */
                            continue;
                        }

                        else
                        {
                            /* We have a free binding to offer the client. */
                            ret_binding = temp_binding;
                            break;
                        }    
                    }    
                }
            }

            /* If the IP that did not match the requested IP is not in use, save a pointer to it
                just in case the requested IP is in use. */
            if (flags == 0)
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Copy the binding pointer to a local variable. */
                temp_binding = binding;

                /* If we have already found the requested IP address is in use, we can now break. */
                if (match_found)
                {
                    ret_binding = temp_binding;
                    /* Unprotect the global data structures. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);
                    break;
                }    
                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);
            }    
            /* Check may be the the client is already assigned the IP that is in use. */
            else
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Copy the client ID type to a local variable. */
                client_type = binding->client_id.idtype;

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);

                /* Now, verify the IP is in use by the same client. */
                if (client_type == client_id->idtype)
                {
                    /* Protect the global data structures. */
                    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                    for (j = 0; j < (client_id->idlen); j++)
                    {
                        if (binding->client_id.id[j] != client_id->id[j])
                            break;
                    }

                    /* Unprotect the global data structures. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);

                    if (j == (client_id->idlen))
                    {
                        /* This binding is for the same client. */
                        match_found = NU_TRUE;
                        ret_binding = binding;
                        break;
                    }
                }
            }
        }
    }

    /* If the requested IP address was never located, offer the client a free IP. */
    if (ret_binding == NU_NULL)
        ret_binding = temp_binding;

    /* Ensure that the binding is not currently being offered to another client */
    for (offered_binding = config_cb->ip_offer_list.dhcp_offer_head;
         offered_binding;
         offered_binding = offered_binding->dhcp_offer_next)
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Ensure that there is an offered binding present. */
        if (offered_binding->offered_binding != NU_NULL)
        {
            if (ret_binding == offered_binding->offered_binding)
            {
                /* This binding is in the offered array.  Check to see if it is the 
                    same client making the request. */
                for (j = 0; j < client_id->idlen; j++)
                {
                    if (offered_binding->offered_binding->client_id.id[j] != 
                                                client_id->id[j])
                        /* This is not the correct client ID. */
                        break;
                }

                if (j == (client_id->idlen))
                    ret_binding = offered_binding->offered_binding;

                else
                    ret_binding = NU_NULL;
            }   
        }
        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }
    return (ret_binding);
}    


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Choose_Lease_Length                                       
*                                                                      
* DESCRIPTION 
*                                                         
*     Choose the proper lease length to offer the requesting client.                                                                
*                                                                             
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     DHCPS_BINDING *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     UINT32                                    
*                                                                      
**********************************************************************************/
UINT32 DHCPS_Choose_Lease_Length (const DHCPS_CONFIG_CB *config_cb, const DHCPS_BINDING *binding)
{
    UINT32              offer_lease;
    UINT8               *lease_ptr = NU_NULL;

    /* First, we must search for the lease length that is associated with this binding. The
        binding memory block is searched first, then the  structure.  IF no value is
        found, the global value will be used. */
    lease_ptr = DHCPS_Get_Option_From_Memory_Block(config_cb, &binding->yiaddr, DEFAULT_LEASE);

    if (lease_ptr == NU_NULL)
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* The lease length is not in the memory block.  Check the control block. */
        if (config_cb->default_lease_length != 0)
        {
            /* This is the lease length that will be offered. */
            offer_lease = config_cb->default_lease_length;
        }    

        else
        {
            /* The global lease length will be offered. */
            offer_lease = DHCPS_Config_List.dhcp_config_head->default_lease_length;
        }    

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }

    else
    {
        /* Increment the pointer past the option tag and length so that it will point at the lease 
            length. */
        lease_ptr += 2;

        /* Copy the lease length so that it can be returned. */
        offer_lease = GET32(lease_ptr, 0);
    }    

    return(offer_lease);
}

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Add_Binding_To_Offer                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Place the binding that is being offered onto the offered link list to 
*     enable the server to more quickly find the offered binding when the
*     client sends a DHCPREQUEST.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     DHCPS_BINDING *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     DHCPS_BINDING *                                    
*                                                                      
**********************************************************************************/
DHCPS_BINDING *DHCPS_Add_Binding_To_Offer(DHCPS_CONFIG_CB *config_cb, DHCPS_BINDING *binding)
{
    INT             already_offered = NU_FALSE;
    DHCPS_OFFERED   *offered_struct;
    UINT32          clock_time;


    /* Protect the global data structures. */
    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

    /* First, determine if the requesting client already has a offer pending.  Some
        clients can request a used IP address up to three times until accepting
        another offered address which differs from its requested address */
    for (offered_struct = config_cb->ip_offer_list.dhcp_offer_head;
         offered_struct;
         offered_struct = offered_struct->dhcp_offer_next)
    {
        /* Ensure that the offered_sturcture contains a valid entry. */
        if (offered_struct->dp_chaddr == NU_NULL)
            break;

        /* Look to see if there is a match of the client's hardware addresses */
        else if (memcmp(binding->chaddr.hardware_addr, offered_struct->dp_chaddr->hardware_addr, 
                    DADDLEN) == NU_SUCCESS)    
        {
            /* Ensure that we are offering the same IP address again to the requesting client */
            if (DHCPSDB_IP_Compare(binding->yiaddr.is_ip_addrs, offered_struct->dp_yiaddr->is_ip_addrs)
                            != NU_SUCCESS)
            {
                /* This client currently has a different address being offered.  We must 
                    resend the same address. */
                binding = offered_struct->offered_binding;
                already_offered = NU_TRUE;
                break;
            } 

            else
            {
                /* We are currently offering this binding to the requesting client.  Update the times
                    and resend it. */
                clock_time = NU_Retrieve_Clock();
                offered_struct->start_offered_time = (clock_time/SCK_Ticks_Per_Second); 
                already_offered = NU_TRUE;
                break;
            }    
        } 
    }   

    /* Unprotect the global data structures. */
    NU_Release_Semaphore(&DHCPS_Semaphore);


    /* Find an empty offered entry in the link list and place the passed in binding info in the 
        structure.  If there are no empty entries, we will have to allocate one. */
    if (!already_offered)
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Search the offered link list for an empty entry. */
        for (offered_struct = config_cb->ip_offer_list.dhcp_offer_head;
             offered_struct;
             offered_struct = offered_struct->dhcp_offer_next)
        {
            /* If the client hardware address field is empty, then the entry is empty and can be used. */
            if (offered_struct->dp_chaddr == NU_NULL)
            {
                /* This one is empty.  We can use it. */
                break;
            }    
        }         

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        if (offered_struct != NU_NULL)
        {
            /* Protect the offered data structure while we add the new offer. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Set the client hardware address of the binding to the offer */
            offered_struct->dp_chaddr = &binding->chaddr;
            
            /* Fill in all the entries of the offered structure with the binding info */
            offered_struct->dp_chaddr->hardware_type = binding->chaddr.hardware_type;
            offered_struct->dp_chaddr->hardware_addr_len = binding->chaddr.hardware_addr_len;
            memcpy(offered_struct->dp_chaddr->hardware_addr, binding->chaddr.hardware_addr,
                            DADDLEN);
            offered_struct->dp_yiaddr = &binding->yiaddr;
            offered_struct->offered_binding = binding;

            /* Get the offered wait time associated with this configuration. */
            if (binding->config->offered_wait_time == NU_NULL)
                /* Use the global offered wait time. */
                offered_struct->offered_wait_time = DHCPS_Config_List.dhcp_config_head->offered_wait_time;

            else
                /* Use the specific configuration offered wait time. */
                offered_struct->offered_wait_time = binding->config->offered_wait_time;

            /* Set the start_offered_time in the offer to the current time */
            clock_time = NU_Retrieve_Clock();

            offered_struct->start_offered_time = (clock_time/SCK_Ticks_Per_Second);

            /* Set the binding as being offered */
            offered_struct->offered_binding->flags |= BINDING_OFFERED;
            
            /* Unprotect the data structures now that we are done. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }    
    }    
    return (binding);
}


/**************************************************************************************
* FUNCTION                                                                   
*                                                                            
*    DHCPS_Server_Options                                                       
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*    Function handles all vendor options returned by the DHCP server.          
*                                                                            
* INPUTS                                                                     
*                                                                            
*    UINT8 *
*    UINT8 *
*    DHCPS_BINDING *
*    UINT8
*                                                                            
* OUTPUTS                                                                    
*                                                                            
*    INT16
*                                                                            
****************************************************************************************/
STATIC INT16 DHCPS_Server_Options(const UINT8 *opt_data, UINT8 *ret_opt, DHCPS_BINDING *binding, 
                              UINT8 msg_type)
{
    UINT8               opcode,
                        opt_length,
                        option_data_length = 0,
                        found = NU_FALSE,
                        *mem_option_block;
    UINT16              length = 0;
    UINT8               i;
    DHCPS_CONFIG_CB     *config_cb;

    /* If this is a DHCPINFORM or DHCPNAK message, the passed in binding pointer is actually a pointer
        to the configuration control block.  This is done due to the fact that no binding
        structure is choosen when replying to a DHCPINFORM or DHCPNAK message.  Set the config_cb to 
        this control block. */
    if ((msg_type == DHCPINFORM) || (msg_type == DHCPNAK))
    {
        config_cb = (DHCPS_CONFIG_CB *)binding;
    }    

    else
    {
        /* Get a pointer to the configuration control block that is associated with the binding
            that is being presented to the requesting client. */
        config_cb = binding->config;
    }    
    
    /* Loop through all of the options received from the client and respond with the 
        configuration parameters that are supported */

    /* Search for the requested options list tag. */
    for (i = OPTION_COOKIE_INCREMENT; opt_data[i] != DHCP_END; i++) 
    {
        if (opt_data[i] == DHCP_REQUEST_LIST) 
        {
            /* Step past the option tag and list length, so that the pointer will point to the list of 
                requested clients. */
            i += 2;
            opt_data += i;
            found = NU_TRUE;
            break;
        }

        else
        {
            /* Increment to the next character, which will be the length of the option. */
            i++;

            /* Get the option data length. */
            opt_length = opt_data[i];

            /* Advance the incrementing variable past the option data to the next option tag. */
            i += opt_length;
        }
        
    }

    /* If the requested options list is found, process each of the requests. */
    if (found == NU_TRUE)
    {
        for( ;*opt_data != DHCP_END; )
        {
            if( *opt_data == DHCP_PAD )          /* move past PAD bytes */
            {
                opt_data++;
                continue;
            }
                
            opcode = *opt_data++;                /* save opcode */

            /* First, check the control block option block for the option for our
               particular IP address. */
            if (msg_type != DHCPINFORM)
            {
                mem_option_block = DHCPS_Get_Option_From_Memory_Block(config_cb, &binding->yiaddr, opcode);
            }

            else
            {
                /* Since this is a INFORM message, we would not have any binding specific info. */
                mem_option_block = NU_NULL;
            }    

            if (mem_option_block != NU_NULL)
            {
                /* We have found the option in the memory block.  Now we copy it to the option
                    buffer and move on. */

                /* Place the option code into the return buffer. */
                *ret_opt++ = *mem_option_block++;

                /* Save off the data length and increment the pointer. */
                option_data_length = *mem_option_block;

                /* Place the option length into the return buffer. */
                *ret_opt++ = *mem_option_block++;


                /* Save the option data to the option buffer. */
                for (i = 0; i < option_data_length; i++)
                    *ret_opt++ = *mem_option_block++;            

                /* Increment the length variable to reflect the option data written into the
                    option buffer. */
                length += option_data_length + 2;
            }    

            else
            {
                /* Search through the configuration control block to find the option value to
                    return to the requesting client. */
                option_data_length = DHCPS_Get_Option_From_Configuration(binding, opcode, 
                                                                        msg_type, ret_opt);

                /* Increment the ret_opt pointer to reflect the data that was written into it. */
                ret_opt += option_data_length;

                /* Increment the total option length. */
                length += option_data_length;
            }    
        }
    }
    return((INT16)length);
}   


/************************************************************************************
*
*FUNCTION
*
*     DHCPS_Init_DHCP_Layer
*
*DESCRIPTION
*
*     Initialize the DHCP layer of the message.
*
*INPUTS
*
*     CHAR *
*     UINT8
*     DHCPLAYER *
*     DHCPS_BINDING *
*
*OUTPUTS
*
*     INT
*
************************************************************************************/
INT DHCPS_Init_DHCP_Layer(CHAR *buf_ptr, UINT8 msg_type, const DHCPLAYER *client_msg,
                             DHCPS_BINDING *binding)
{
    INT         len = 0,
                opt_length,
                default_opt_length;
    UINT8       *pt,
                *options,
                default_options[16];
    DHCPLAYER   pkt;    

    /* The size of the Nucleus NET buffer chains are user configurable. This 
       makes it difficult build the DHCP packet in the Nucleus NET buffer chain.
       To avoid these complexities build the buffer in contiguous memory and 
       then copy it to the buffer chain. */

    /* Initialize the pkt buffer */
    UTL_Zero(&pkt, sizeof(DHCPLAYER));            

    pkt.dp_op       = BOOTREPLY;        /* opcode, 1 octet */
    pkt.dp_htype    = HARDWARE_TYPE;    /* hardware type, 1 octet */
    pkt.dp_hlen     = DADDLEN;          /* hardware address length, 1 octet */
    pkt.dp_hops     = 0;                
    pkt.dp_xid      = client_msg->dp_xid;
    pkt.dp_secs     = 0;

#if (defined NET_5_1)    
    memcpy(pkt.dp_ciaddr, client_msg->dp_ciaddr, IP_ADDR_LEN);
#else    
    memcpy(pkt.dp_ciaddr.is_ip_addrs, client_msg->dp_ciaddr.is_ip_addrs, IP_ADDR_LEN);    
#endif    

    /* If this is going to a relay server, the message must be broadcast. */
#if (defined NET_5_1)    
    if (client_msg->dp_giaddr[0] != NU_NULL)        
#else
    if (client_msg->dp_giaddr.is_ip_addrs[0] != NU_NULL)    
#endif        
    {
        /* The message must be broadcast. */
        pkt.dp_flags = DHCP_BROADCAST_FLAG;                        
    }    

    else
    {
        /* Comply with the client's request. */
        pkt.dp_flags = client_msg->dp_flags;                        
    }    

#if (defined NET_5_1)        
    memcpy(pkt.dp_giaddr, client_msg->dp_giaddr, IP_ADDR_LEN);        
#else
    memcpy(pkt.dp_giaddr.is_ip_addrs, client_msg->dp_giaddr.is_ip_addrs, IP_ADDR_LEN);            
#endif

    /* Fill in the client's hardware address. */
    memcpy(pkt.dp_chaddr, client_msg->dp_chaddr, DADDLEN);        

    /* Fill in the server's name */
    strcpy((CHAR *)pkt.dp_sname, (CHAR *)client_msg->dp_sname);        

    if ((msg_type == DHCPNAK) || (msg_type == DHCPINFORM))
    {
        /* We do not provide IP addresses to a requesting client when 
            responding with these two messages.  Therefore, leave the
            yiaddr field blank. */
    }    
    else
    {
        /* Protect the offered data structure while we add the new offer. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Fill in the IP address that we are offering the client. */
#if (defined NET_5_1)                
        memcpy(pkt.dp_yiaddr, binding->yiaddr.is_ip_addrs, IP_ADDR_LEN);                                        
#else
        memcpy(pkt.dp_yiaddr.is_ip_addrs, binding->yiaddr.is_ip_addrs, IP_ADDR_LEN);        
#endif        

        /* Unprotect the data structures now that we are done. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }
    
    len = sizeof(pkt.dp_op) +
      sizeof(pkt.dp_htype)  +
      sizeof(pkt.dp_hlen)   +
      sizeof(pkt.dp_hops)   +
      sizeof(pkt.dp_xid)    +
      sizeof(pkt.dp_secs)   +
      sizeof(pkt.dp_flags)  +
      sizeof(pkt.dp_ciaddr) +
      sizeof(pkt.dp_yiaddr) +
      sizeof(pkt.dp_siaddr) +
      sizeof(pkt.dp_giaddr) +
      sizeof(pkt.dp_chaddr) +
      sizeof(pkt.dp_sname)  +
      sizeof(pkt.dp_file);        
    
    pt = pkt.dp_vend;        
    PUT32(pt, 0, DHCP_COOKIE);
    pt += 4;
    *pt++ = DHCPS_MSG_TYPE;
    *pt++ = 1;

    if (msg_type == DHCPINFORM)
    {
        *pt++ = DHCPACK;
    }    

    else
    {
        *pt++ = msg_type;
    }    

    len += 7;

    if (msg_type == DHCPOFFER)
    {
        /* Set a pointer to the start of the option buffer. */
        options = &default_options[0];

        /* Copy the magic cookie into the buffer that will signify this is an 
            options buffer. */
        PUT32(options, 0, DHCP_COOKIE);
        options += 4;

        /* Copy the default options into the options buffer. */
        PUT8(options, 0, DHCP_REQUEST_LIST);
        options++;

        /* Length of the default options. */
        PUT8(options, 0, 0x05);
        options++;

        PUT8(options, 0, SUBNET_MASK);
        options++;

        PUT8(options, 0, DHCP_IP_LEASE_TIME);
        options++;

        PUT8(options, 0, DHCPS_RENEWAL_T1);
        options++;

        PUT8(options, 0, DHCPS_REBIND_T2);
        options++;

        PUT8(options, 0, DHCPS_SERVER_ID);
        options++;

        PUT8(options, 0, DHCP_END);
    }

    /* If this is an DHCPACK message in response to a DHCPINFORM message, do not send any 
        parameters that deal with binding lease times. */
    else if (msg_type == DHCPINFORM)
    {
        /* Set a pointer to the start of the option buffer. */
        options = &default_options[0];

        /* Copy the magic cookie into the buffer that will signify this is an 
            options buffer. */
        PUT32(options, 0, DHCP_COOKIE);
        options += 4;

        /* Copy the default options into the options buffer. */
        PUT8(options, 0, DHCP_REQUEST_LIST);
        options++;

        /* Length of the default options. */
        PUT8(options, 0, 0x02);
        options++;

        PUT8(options, 0, SUBNET_MASK);
        options++;

        PUT8(options, 0, DHCPS_SERVER_ID);
        options++;

        PUT8(options, 0, DHCP_END);
    }    

    /* If this is an DHCPNAK message, we must only send the server ID to the 
        requesting client. */
    else if (msg_type == DHCPNAK)
    {
        /* Set a pointer to the start of the option buffer. */
        options = &default_options[0];

        /* Copy the magic cookie into the buffer that will signify this is an 
            options buffer. */
        PUT32(options, 0, DHCP_COOKIE);
        options += 4;

        /* Copy the default options into the options buffer. */
        PUT8(options, 0, DHCP_REQUEST_LIST);
        options++;

        /* Length of the default options. */
        PUT8(options, 0, 0x01);
        options++;

        PUT8(options, 0, DHCPS_SERVER_ID);
        options++;

        PUT8(options, 0, DHCP_END);
    }            

    else
    {
        /* Set a pointer to the start of the option buffer. */
        options = &default_options[0];

        /* Copy the magic cookie into the buffer that will signify this is an 
            options buffer. */
        PUT32(options, 0, DHCP_COOKIE);
        options += 4;

        /* Copy the default options into the options buffer. */
        PUT8(options, 0, DHCP_REQUEST_LIST);
        options++;

        /* Length of the default options. */
        PUT8(options, 0, 0x04);
        options++;

        PUT8(options, 0, DHCP_IP_LEASE_TIME);
        options++;

        PUT8(options, 0, DHCPS_RENEWAL_T1);
        options++;

        PUT8(options, 0, DHCPS_REBIND_T2);
        options++;

        PUT8(options, 0, DHCPS_SERVER_ID);
        options++;

        PUT8(options, 0, DHCP_END);
    }    


    
    /* Save the default options into the option buffer. */
    default_opt_length = DHCPS_Server_Options(default_options, pt, binding, msg_type);

    /* Adjust the data length and buffer pointers to reflect the adding of the default options. */
    len += default_opt_length;

    pt += default_opt_length;

    /* Fill in the option field with the requested options */
    opt_length = DHCPS_Server_Options(client_msg->dp_vend, pt, binding, msg_type);

    len += opt_length;

    pt  += opt_length;

    /* Mark the end of the options list. */
    *pt = DHCP_END;
    len++;

    /* Some DHCP servers (SUN Solaris) reject DHCP messages that are less than 
       300 bytes in length. Pad the message if necessary. */
    if (len < 300)
        len = 300;

    if (len <= DHCP_PACKET_LEN)
    {
        /* Copy the DHCP structure into the transmit structure */
        NU_BLOCK_COPY(buf_ptr, &pkt, (UINT16)len);
    }
    else
    {
        len = -1;
    }

    return (len);

} /* DHCPS_Init_DHCP_Layer */


/**************************************************************************************
* FUNCTION                                                                   
*                                                                            
*      DHCPS_Remove_Offer                                                         
*                                                                            
* DESCRIPTION                                                                
*                                                                                   
*      Remove the offered binding from the offer link list and initialize each of the
*      fields.
*                                                                            
* INPUTS                                                                     
*                                                                            
*      DHCPS_CONFIG_CB *
*      DHCPS_OFFERED *
*
* OUTPUTS                                                                    
*                                                                            
*      NONE
*                                                                            
****************************************************************************************/
VOID DHCPS_Remove_Offer(DHCPS_OFFERED *accepted_offer)
{
    /* Protect the global data structures. */
    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

    /* Set all of the members of the offered structure to 0 so that it will be freed up 
       to be used again */   
    accepted_offer->dp_chaddr = NU_NULL;
    accepted_offer->dp_yiaddr = NU_NULL;
    accepted_offer->offered_binding = NU_NULL;
    accepted_offer->offered_wait_time = 0;
    accepted_offer->start_offered_time = 0;

    /* Unprotect the global data structures. */
    NU_Release_Semaphore(&DHCPS_Semaphore);
}

/****************************************************************************************
* FUNCTION                                                                   
*                                                                            
*     DHCPS_Send                                                              
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*     Send a DHCP Message.                                                    
*                                                                            
* INPUTS                                                                     
*                                                                            
*     DHCPLAYER *
*     UINT8
*     DHCPS_BINDING *
*     INT
*     struct id_struct *
*                                                                            
* OUTPUTS                                                                    
*                                                                            
*     NU_SUCCESS              SUCCESS                                              
*     -1                      Fail.                                         
*                                                                            
****************************************************************************************/
STATUS DHCPS_Send(const DHCPLAYER *dhcp_ptr, UINT8 msg_type, DHCPS_BINDING *binding, 
                     INT socketd, struct id_struct *interface_addr)
{
    STATUS              status,
                        ret_status = -1;
    INT32               bytes_sent;
    struct addr_struct  cliaddr;            /* holds the client address structure */
    struct id_struct    ip_addr_any;
    CHAR                trans_buff[IOSIZE];
    CHAR                *buf_ptr,
                        ip_addr[]          = {(CHAR) 0,(CHAR) 0,(CHAR) 0,(CHAR) 0};
    INT                 buff_length;

    /* Set the buffer ptr to the first location of the transmit buffer */
    buf_ptr = &trans_buff[0];

    
    /* build the first DHCP discovery message */
    buff_length = DHCPS_Init_DHCP_Layer(buf_ptr, msg_type, dhcp_ptr, binding);
    if (buff_length > 0)
    {
        /* The socket's broadcast interface needs to be set to the interface on which
            the message was received. */
        status = NU_Setsockopt(socketd, IPPROTO_IP, IP_BROADCAST_IF,
                                   interface_addr->is_ip_addrs, IP_ADDR_LEN);

        if (status != NU_SUCCESS)
        {
            /* The socket has not been setup for proper operation. */
            NERRS_Log_Error(NERR_RECOVERABLE, __FILE__, __LINE__);

            ret_status = status;
        }

        else
        {
            /* initialize client address */
            cliaddr.family    = NU_FAMILY_IP;
            cliaddr.name = "";

            /* If this message is going to a relay agent, then this message must be sent to 
                the DHCP server port. */
#if (defined NET_5_1)            
            if (dhcp_ptr->dp_giaddr[0] != NU_NULL)                
#else
            if (dhcp_ptr->dp_giaddr.is_ip_addrs[0] != NU_NULL)                
#endif                
            {
                /* Send to server port. */
                cliaddr.port = IPPORT_DHCPS;
            }    

            else
            {
                /* Send to client port. */
                cliaddr.port   = IPPORT_DHCPC;
            }    

            /* If this message was received from a relay agent, we must send our
                response back to the relay agent. */
            memcpy(ip_addr_any.is_ip_addrs, ip_addr, IP_ADDR_LEN);            

#if (defined NET_5_1)            
            if (DHCPSDB_IP_Compare(dhcp_ptr->dp_giaddr, ip_addr_any.is_ip_addrs) 
                                    != NU_SUCCESS)                                    
#else
            if (DHCPSDB_IP_Compare(dhcp_ptr->dp_giaddr.is_ip_addrs, ip_addr_any.is_ip_addrs) 
                                    != NU_SUCCESS)                                        
#endif                                    
            {
                /* This message was received via a relay agent.  Our response must 
                    be sent to the relay agent. */
#if (defined NET_5_1)                
                IP_ADDR_COPY(cliaddr.id.is_ip_addrs, dhcp_ptr->dp_giaddr);
#else
                IP_ADDR_COPY(cliaddr.id.is_ip_addrs, dhcp_ptr->dp_giaddr.is_ip_addrs);                        
#endif                
            }    

            else
            {
                /* This message was received from a DHCP client on the same network
                    as the server. */

                /* Determine if the message should be unicast or broadcast */
#if (defined NET_5_1)            
                if ((dhcp_ptr->dp_ciaddr[0] == 0) &&
                    (dhcp_ptr->dp_ciaddr[1] == 0) &&
                    (dhcp_ptr->dp_ciaddr[2] == 0) &&
                    (dhcp_ptr->dp_ciaddr[3] == 0))                    
#else
                if ((dhcp_ptr->dp_ciaddr.is_ip_addrs[0] == 0) &&
                    (dhcp_ptr->dp_ciaddr.is_ip_addrs[1] == 0) &&
                    (dhcp_ptr->dp_ciaddr.is_ip_addrs[2] == 0) &&
                    (dhcp_ptr->dp_ciaddr.is_ip_addrs[3] == 0))                    
#endif                    
                {
                    /* The message needs to be broadcast */
                    cliaddr.id.is_ip_addrs[0]  = (UINT8) 255;
                    cliaddr.id.is_ip_addrs[1]  = (UINT8) 255;
                    cliaddr.id.is_ip_addrs[2]  = (UINT8) 255;
                    cliaddr.id.is_ip_addrs[3]  = (UINT8) 255;
                }
                else
#if (defined NET_5_1)                                    
                    memcpy(cliaddr.id.is_ip_addrs, dhcp_ptr->dp_ciaddr, IP_ADDR_LEN); 
#else               
                    memcpy(cliaddr.id.is_ip_addrs, dhcp_ptr->dp_ciaddr.is_ip_addrs, IP_ADDR_LEN);
#endif                    
            }

            /*  Send the string back to the client.  */
            bytes_sent = NU_Send_To(socketd, buf_ptr, (UINT16)buff_length, 0,
                                    &cliaddr, 0);

            if (bytes_sent > 0)
                ret_status = NU_SUCCESS;
        }    
    }
    return(ret_status);
} /* DHCPS_Send */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Save_Config_Data_To_File                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Save each of the binding entries to the binding file.  This file can be
*     used in the case of a server restart to determine which bindings are
*     still in use by clients.
*                                                                      
* INPUTS                                                               
*                                                                      
*     None.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Save_Config_Data_To_File(VOID)
{
    STATUS  ret_status = NU_SUCCESS;

    /* Call the function to save the configuration link list to file. */
    ret_status = DHCPS_Save_Config_Struct_To_File();

    if (ret_status != NU_SUCCESS)
    {
        /* An error occurred while saving the config structures to file. */
        ret_status = DHCPSERR_CONFIG_FILE_ERROR;
    }    

    else
    {
        ret_status = DHCPS_Save_Options_Block_To_File();

        if (ret_status != NU_SUCCESS)
        {
            ret_status = DHCPSERR_OPTIONS_FILE_ERROR;
        }

        if (ret_status == NU_SUCCESS)
        {
            ret_status = DHCPS_Save_Binding_To_File();
        }
    }    
    return (ret_status);
}
/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Save_Config_Struct_To_File                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Save each of the binding entries to the binding file.  This file can be
*     used in the case of a server restart to determine which bindings are
*     still in use by clients.
*                                                                      
* INPUTS                                                               
*                                                                      
*     None.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Save_Config_Struct_To_File(VOID)
{
    INT             config_fp;
    CHAR            *buffer_pointer,
                    *buffer_start;
    
    UINT16          i;
                  
    UINT32          bytes_written = 0,
                    end_of_file;
    DHCPS_CONFIG_CB *config_cb;
    INT             file_status;
    STATUS          ret_status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Set the current directory to the DHCP Server directory. */
    ret_status = NU_Set_Current_Dir(DHCPSrv_Directory_Path);

    if (ret_status == NU_SUCCESS)
    {
        /* Open the file and prepare it for writing */
        config_fp = NU_Open((CHAR *)DHCPSrv_Configuration_File_Name, (PO_RDWR|PO_CREAT|PO_TRUNC),
                                                         (PS_IWRITE|PS_IREAD));

        if (config_fp < 0)
        {
            /* An error occurred while opening the configuration file. */
            ret_status = -1;
        }    

        else
        {
            /* Determine the total number of control blocks that will need to be saved. */
            for (config_cb = DHCPS_Config_List.dhcp_config_head, i = 0;
                 config_cb;
                 config_cb = config_cb->dhcps_config_next, i++)
            {
                continue;         
            }
                        

            /* Allocate a block of memory to copy the control blocks into. */
            ret_status = NU_Allocate_Memory(DHCPServer_Memory, (VOID **)&buffer_pointer, 
                              (UNSIGNED)((sizeof(DHCPS_CONFIG_CB)+ CONFIG_FILE_BUFF_OVERHEAD) * i),
                              NU_SUSPEND);

            /* Save the start of the buffer to a pointer. */
            buffer_start = buffer_pointer;

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* We will need to copy each entry in the configuration control block that is 
                not a pointer into the file with a identifier tag. */
            for (config_cb = DHCPS_Config_List.dhcp_config_head;
                 config_cb;
                 config_cb = config_cb->dhcps_config_next)
            {
                /* The first thing that we will save will be the configuration control block 
                     entry name. */
                *buffer_pointer++ = '{';
                bytes_written++;

                /* Copy the control block entry name into the buffer. */
                strcpy(buffer_pointer, config_cb->config_entry_name);

                /* Determine the length of the entry name so that the buffer pointer may be 
                    incremented. */
                buffer_pointer += strlen(config_cb->config_entry_name);
                bytes_written  += strlen(config_cb->config_entry_name);

                /* Place a colon at the end of the entry name to signify the end of the 
                    entry name. */
                strcpy(buffer_pointer, ": ");

                /* Increment the buffer pointer. */
                buffer_pointer += 2;
                bytes_written  += 2;

                /* Copy the server IP address tag into the buffer. */
                strcpy(buffer_pointer, ":svip=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;


                /* Copy the server IP address into the buffer. */
                memcpy(buffer_pointer, config_cb->server_ip_addr, IP_ADDR_LEN);

                /* Increment the buffer pointer. */
                buffer_pointer += IP_ADDR_LEN;
                bytes_written  += IP_ADDR_LEN;


                /* Copy the subnet address tag into the buffer. */
                strcpy(buffer_pointer, ":suba=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Copy the subnet address into the buffer. */
                memcpy(buffer_pointer, config_cb->subnet_addr, IP_ADDR_LEN);

                /* Increment the buffer pointer. */
                buffer_pointer += IP_ADDR_LEN;
                bytes_written  += IP_ADDR_LEN;

                /* Copy the subnet mask tag into the buffer. */
                strcpy(buffer_pointer, ":snmk=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Copy the subnet mask into the buffer. */
                memcpy(buffer_pointer, config_cb->subnet_mask_addr, IP_ADDR_LEN);

                /* Increment the buffer pointer. */
                buffer_pointer += IP_ADDR_LEN;
                bytes_written  += IP_ADDR_LEN;


                /* Copy the broadcast address tag into the buffer. */
                strcpy(buffer_pointer, ":brda=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Copy the broadcast address into the buffer. */
                memcpy(buffer_pointer, config_cb->broadcast_addr, IP_ADDR_LEN);

                /* Increment the buffer pointer. */
                buffer_pointer += IP_ADDR_LEN;        
                bytes_written  += IP_ADDR_LEN;


#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DOMAIN_NAME == NU_TRUE)
                /* Copy the DNS domain name tag into the buffer. */
                strcpy(buffer_pointer, ":dnsd=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Copy the DNS domain name into the buffer. */
                memcpy(buffer_pointer, config_cb->dns_domain_name, config_cb->dns_domain_name_length);

                /* Increment the buffer pointer. */
                buffer_pointer += config_cb->dns_domain_name_length;
                bytes_written  += config_cb->dns_domain_name_length;
#endif              

#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DNS_SERVER == NU_TRUE)

                /* Copy the DNS server IP address tag into the buffer. */
                strcpy(buffer_pointer, ":dnsv=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Loop through the DNS server array and copy each of the IP addresses into 
                    the buffer. */
                for (i = 0; i < DHCPS_MAX_DNS_SERVERS ; i++)
                {
                    if (config_cb->dns_server[i].is_ip_addrs[0] == NU_NULL)
                    {
                        /* This entry is empty. Skip to the next entry. */
                        continue;
                    }

                    else
                    {
                        /* There is a DNS server entry in this control block.  Copy this DNS server 
                            into the buffer. */
                        memcpy(buffer_pointer, config_cb->dns_server[i].is_ip_addrs, IP_ADDR_LEN);

                        /* Increment the buffer pointer. */
                        buffer_pointer += IP_ADDR_LEN;
                        bytes_written  += IP_ADDR_LEN;
                    }    
                }
#endif

#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_ROUTER == NU_TRUE)

                /* Copy the router IP address tag into the buffer. */
                strcpy(buffer_pointer, ":rout=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Loop through the router IP address array and copy each of the IP addresses into the 
                    buffer. */
                for (i = 0; i < DHCPS_MAX_ROUTERS; i++)
                {
                    if (config_cb->router[i].is_ip_addrs[0] == NU_NULL)
                    {
                        /* This entry is empty. Skip to the next entry. */
                        continue;
                    }

                    else
                    {
                        /* There is a router entry in this control block.  Copy this DNS server into the 
                            buffer. */
                        memcpy(buffer_pointer, config_cb->router[i].is_ip_addrs, IP_ADDR_LEN);

                        /* Increment the buffer pointer. */
                        buffer_pointer += IP_ADDR_LEN;
                        bytes_written  += IP_ADDR_LEN;
                    }    
                }
#endif              

                /* Copy the device interface name tag into the buffer. */
                strcpy(buffer_pointer, ":ifnm=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Copy the device interface name into the buffer. */
                strcpy(buffer_pointer, config_cb->device_name);

                /* Increment the buffer pointer. */
                buffer_pointer += strlen(config_cb->device_name);  
                bytes_written  += strlen(config_cb->device_name);


                /* Copy the IP time to live name tag into the buffer. */
                strcpy(buffer_pointer, ":ittl=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Copy the IP time to live value into the buffer. */
                memcpy(buffer_pointer, &config_cb->default_ip_ttl, sizeof(UINT8));

                /* Increment the buffer pointer. */
                buffer_pointer += 1;
                bytes_written  += 1;


                /* Copy the TCP time to live tag into the buffer. */
                strcpy(buffer_pointer, ":tttl=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Copy the TCP time to live into the buffer. */
                memcpy(buffer_pointer, &config_cb->default_tcp_ttl, sizeof(UINT8));

                /* Increment the buffer pointer. */
                buffer_pointer += 1;
                bytes_written  += 1;


                /* Copy the status flags tag into the buffer. */
                strcpy(buffer_pointer, ":flgs=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Copy the status flags into the buffer. */
                memcpy(buffer_pointer, &config_cb->flags, sizeof(UINT16));

                /* Increment the buffer pointer. */
                buffer_pointer += 2;
                bytes_written  += 2;

                /* Copy the renewal T1 tag into the buffer. */
                strcpy(buffer_pointer, ":dht1=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Copy the renewal T1 into the buffer. */
                memcpy(buffer_pointer, &config_cb->dhcp_renew_t1, sizeof(UINT32));

                /* Increment the buffer pointer. */
                buffer_pointer += 4;
                bytes_written  += 4;


                /* Copy the rebinding T2 tag into the buffer. */
                strcpy(buffer_pointer, ":dht2=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Copy the rebinding T2 into the buffer. */
                memcpy(buffer_pointer, &config_cb->dhcp_rebind_t2, sizeof(UINT32));

                /* Increment the buffer pointer. */
                buffer_pointer += 4;
                bytes_written  += 4;


                /* Copy the lease length tag into the buffer. */
                strcpy(buffer_pointer, ":dfll=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Copy the lease length into the buffer. */
                memcpy(buffer_pointer, &config_cb->default_lease_length, sizeof(UINT32));

                /* Increment the buffer pointer. */
                buffer_pointer += 4;
                bytes_written  += 4;


                /* Copy the offered wait time tag into the buffer. */
                strcpy(buffer_pointer, ":dofw=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Copy the offered wait time into the buffer. */
                memcpy(buffer_pointer, &config_cb->offered_wait_time, sizeof(UINT32));

                /* Increment the buffer pointer. */
                buffer_pointer += 4;
                bytes_written  += 4;


                /* Copy the ARP cache timeout tag into the buffer. */
                strcpy(buffer_pointer, ":arpt=");

                /* Increment the buffer pointer. */
                buffer_pointer += 6;
                bytes_written  += 6;

                /* Copy the ARP cache timeout into the buffer. */
                memcpy(buffer_pointer, &config_cb->arp_cache_to, sizeof(UINT32));

                /* Increment the buffer pointer. */
                buffer_pointer += 4;
                bytes_written  += 4;

                /* Place a string at the end of the config entry to signify the
                    end of the entry */
                PUT32(buffer_pointer, 0, END_OF_CONFIG);

                /* Increment the buffer pointer. */
                buffer_pointer += 4;
                bytes_written  += 4;
            }

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            /* Place a tag at the end of the buffer to signify the end of the file. */
            end_of_file = END_OF_CONFIG_FILE;

            PUT32(buffer_pointer, 0, end_of_file);
            
            bytes_written += 4;

            /* Write the buffer out to the configuration file. */
            bytes_written = NU_Write(config_fp, buffer_start, (UINT16)bytes_written);

            if (((INT32)bytes_written) <= 0)
            {
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
            }    

            /* Close the file */
            file_status = NU_Close(config_fp);

            if (file_status != NU_SUCCESS)
            {   
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
            }    

            /* Deallocate the memory that was allocated. */
            ret_status = NU_Deallocate_Memory(buffer_start);
        }
    }    

    /* Switch back to user mode */
    NU_USER_MODE();
    
    return(ret_status);
}


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Save_Binding_To_File                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Save each of the binding entries to the binding file.  This file can be
*     used in the case of a server restart to determine which bindings are
*     still in use by clients.
*                                                                      
* INPUTS                                                               
*                                                                      
*     None.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Save_Binding_To_File(VOID)
{
    
    INT             binding_fp,
                    back_bind_fp;
    CHAR            buff[DHCPS_MAX_ENTRY_NAME_LENGTH + 4],
                    back_buff[DHCPS_FILE_READ_SIZE],
                    *backptr,
                    *bufptr;
    UINT8           more_data = NU_TRUE;

    UINT16          buffer_size;

    INT32           bytes_written = 0,
                    bytes_read = 0;
    
    DHCPS_CONFIG_CB *config_cb;
    DHCPS_BINDING   *binding;
    INT             file_status;
    STATUS          ret_status = NU_SUCCESS;


     /* Set the bufptr to the begining of buffer. */
    bufptr = &buff[0];

    /* Set the backptr to the begining of the back_buff. */
    backptr = &back_buff[0];

    /* Set the buffer_size variable to equal FILE_READ_SIZE */
    buffer_size = DHCPS_FILE_READ_SIZE;  

    /* Open the file and prepare it for writing */
    binding_fp = NU_Open((CHAR *)DHCPSrv_Binding_File_Name, (PO_RDONLY), (PS_IREAD));

    if (binding_fp < 0)
    {
        /* An error occurred while trying to open the binding file. This may be the first time
            we have tried to save the binding file, so this might not be a problem.  Continue
            from here. */
        ret_status = NU_SUCCESS;
    }    

    else
    {
        /* Open the backup file and prepare it for writing. */
        back_bind_fp = NU_Open((CHAR *)DHCPSrv_Binding_File_Backup, (PO_RDWR|PO_CREAT|PO_TRUNC),
                                                     (PS_IWRITE|PS_IREAD));

        if (back_bind_fp < 0)
        {
            /* An error occurred while trying to open the backup binding file. */
            ret_status = -1;            
        }    

        else
        {
            /* Copy the data from the binding file into the backup binding file so that 
                an updated binding file can be written. */
            while(more_data)
            {
                /* Read a block of memory from the binding file into a buffer so that it 
                    can be written out to the backup file. */
                bytes_read = (INT32)NU_Read(binding_fp, backptr, buffer_size);
                
                if (bytes_read <= 0)
                {
                    /* We have read all of the bytes from the binding file. */
                    more_data = NU_FALSE;
                    break;
                }

                else
                {
                    /* Copy the data from the buffer into the backup file. */
                    bytes_written = (INT32)NU_Write(back_bind_fp, backptr, (UINT16)bytes_read);
                }    

            }    

            /* Close the backup file. */
            file_status = NU_Close(back_bind_fp);

            if (file_status != NU_SUCCESS)
            {   
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Close the binding file so that it may be reopened with write permission. */
            file_status = NU_Close(binding_fp);

            if (file_status != NU_SUCCESS)
            {   
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }    

    /* Update the binding file with the current binding structure status. */
    binding_fp = NU_Open((CHAR *)DHCPSrv_Binding_File_Name, (PO_RDWR|PO_CREAT|PO_TRUNC),
                                             (PS_IWRITE|PS_IREAD));

    if (binding_fp >= 0)
    {
        /* Now the new binding data can be written into the binding file. */    

        /* Reset the variable bytes_written. */
        bytes_written = 0;

        /* We will need to place each binding structures into a buffer to be able to 
            save the bindings as a file. */
        for (config_cb = DHCPS_Config_List.dhcp_config_head;
             config_cb;
             config_cb = config_cb->dhcps_config_next)
        {            
            /* Ensure that this control block contains bindings that need to be saved. */
            if (config_cb->ip_binding_list.dhcp_bind_head != NU_NULL)
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Copy the begining bracket into the buffer. */
                memcpy(bufptr, "{", 1);
                bufptr++;
                bytes_written++;

                /* Copy the control block entry name into the buffer. */
                strcpy(bufptr, config_cb->config_entry_name);

                /* Increment the bufptr. */
                bufptr += strlen(config_cb->config_entry_name);
                bytes_written += (INT32)strlen(config_cb->config_entry_name);

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);

                /* Copy a colon into the buffer. */
                memcpy(bufptr, ":", 1);
                bytes_written++;

                /* Write the buffer to the binding file. */
                bytes_written = (INT32)NU_Write(binding_fp, buff,(UINT16)bytes_written);

                for (binding = config_cb->ip_binding_list.dhcp_bind_head;
                     binding;
                     binding = binding->dhcp_bind_next)
                {

                    bytes_written = (INT32)NU_Write(binding_fp, (CHAR *)binding, sizeof(DHCPS_BINDING));

                    if (bytes_written <= 0)
                    {
                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                    }    

                    /* Write the end of binding string out to the file */
                    PUT32(buff, 0, END_OF_BINDING);

                    bytes_written = (INT32)NU_Write(binding_fp, buff, sizeof(UINT32));

                    if (bytes_written <= 0)
                    {
                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                        ret_status = -1;
                        break;    
                    }            
                }
            }
        }

        if (ret_status == NU_SUCCESS)
        {
            /* Place an end of binding entry onto the end of the file */
            PUT32(buff, 0, END_OF_BINDING_FILE);

            bytes_written = (INT32)NU_Write(binding_fp, buff, sizeof(UINT32));

            if (bytes_written <= 0)
            {
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
            }    
        }    

        /* Close the file */
        file_status = NU_Close(binding_fp);

        if (file_status != NU_SUCCESS)
        {   
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
        }
        ret_status = file_status;
    }    
    return(ret_status);
}


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Save_Options_Block_To_File                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Save each of the binding entries to the binding file.  This file can be
*     used in the case of a server restart to determine which bindings are
*     still in use by clients.
*                                                                      
* INPUTS                                                               
*                                                                      
*     None.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Save_Options_Block_To_File(VOID)
{
    
    INT             options_fp;
    CHAR            buff[DHCPS_MAX_ENTRY_NAME_LENGTH + 4],
                    *bufptr;
    UINT8           bytes_in_buffer = 0,
                    option_buffer_delimiter;

    UINT32          bytes_written = 0;
    DHCPS_CONFIG_CB *config_cb;
    DHCPS_OPTIONS   *options;
    INT             file_status;
    STATUS          ret_status = NU_SUCCESS;

     /* Set the bufptr to the begining of buffer. */
    bufptr = &buff[0];

    /* Set the current directory to the DHCP Server directory. */
    ret_status = NU_Set_Current_Dir(DHCPSrv_Directory_Path);

    if (ret_status == NU_SUCCESS)
    {
        /* Open the file and prepare it for writing */
        options_fp = NU_Open((CHAR *)DHCPSrv_Options_Block_File_Name, (PO_RDWR|PO_CREAT|PO_TRUNC),
                                                         (PS_IWRITE|PS_IREAD));
      
        if (options_fp < 0)
        {
            /* An error occurred while opening the options file. */
            ret_status = -1;
        }    

        else
        {
            /* We will need to place each option block structure into a buffer to be able to save the 
                options block as a file. */
            for (config_cb = DHCPS_Config_List.dhcp_config_head;
                 config_cb;
                 config_cb = config_cb->dhcps_config_next)
            {
                /* Ensure that this control block contains option block that need to be saved. */
                if (config_cb->options_buff.dhcps_options_head != NU_NULL)
                {
                    /* Copy the begining bracket into the buffer. */
                    memcpy(bufptr, "{", 1);
                    bufptr++;
                    bytes_written++;

                    /* Protect the global data structures. */
                    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                    /* Copy the control block entry name into the buffer. */
                    strcpy(bufptr, config_cb->config_entry_name);

                    /* Increment the bufptr. */
                    bufptr += strlen(config_cb->config_entry_name);
                    bytes_written += strlen(config_cb->config_entry_name);

                    /* Unprotect the structures now that we are done. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);

                    /* Copy a colon into the buffer. */
                    memcpy(bufptr, ":", 1);
                    bytes_written++;

                    /* Write the buffer to the binding file. */
                    bytes_written = NU_Write(options_fp, buff,(UINT16)bytes_written);

                    for (options = config_cb->options_buff.dhcps_options_head;
                         options;
                         options = options->dhcps_options_next)
                    {

                        bytes_written = NU_Write(options_fp, (CHAR *)options, sizeof(DHCPS_OPTIONS));

                        if (((INT32)bytes_written) <= 0)
                        {
                            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                        }    

                        /* Copy the option buffer delimiter to file. */
                        memcpy(&option_buffer_delimiter, "[", 1);

                        bytes_written = NU_Write(options_fp, (CHAR *)&option_buffer_delimiter, sizeof(UINT8));

                        /* Get the number of bytes that are in the option buffer. */
                        bytes_in_buffer = options->bytes_written;

                        /* Copy the option buffer to the file. */
                        bytes_written = NU_Write(options_fp, (CHAR *)options->options_ptr, (UINT16)bytes_in_buffer);

                        /* Copy the option buffer delimiter to file. */
                        memcpy(&option_buffer_delimiter, "]", 1);

                        bytes_written = NU_Write(options_fp, (CHAR *)(&option_buffer_delimiter), sizeof(UINT8));

                        /* Copy the end of options delimited to the file. */
                        PUT32(buff, 0, END_OF_OPTIONS);

                        bytes_written = NU_Write(options_fp, buff, sizeof(INT16));

                        if (((INT32)bytes_written) <= 0)
                        {
                            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                            ret_status = -1;
                            break;    
                        }            
                    }
                }
            }
            if (ret_status == NU_SUCCESS)
            {
                /* Place an end of options entry onto the end of the file */
                PUT32(buff, 0, END_OF_OPTIONS_FILE);

                bytes_written = NU_Write(options_fp, buff, sizeof(INT32));

                if (((INT32)bytes_written) <= 0)
                {
                    NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                }    
            }
            /* Close the file */
            file_status = NU_Close(options_fp);

            if (file_status != NU_SUCCESS)
            {   
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
            }
            ret_status = file_status;
        }    
    }    
    return(ret_status);
}


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Update_Lease_Times                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Update the lease times for each of the bindngs that are currently in use.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_BINDING *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
VOID DHCPS_Update_Lease_Times(DHCPS_BINDING *binding)
{

    UINT32  lease_remaining;
    INT     flags;
            
    /* Ensure that the binding is in use.  If not, then the lease does not need to be
        updated. */
    flags = IN_USE    

    if ((binding->flags & flags) != 0)
    {

#if (NET_VERSION_COMP >= NET_4_5)
        /* Get the new lease time remaining. */
        lease_remaining = UTL_Check_Duetime((binding->end_lease_time * SCK_Ticks_Per_Second));
#else
        /* Get the new lease time remaining. */
        lease_remaining = DHCPS_Check_Duetime(binding->end_lease_time * SCK_Ticks_Per_Second);
#endif

        /* Protect the data structures while we adjust the lease times. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        if (lease_remaining > 0)
        {
            /* Update the lease remaining variable. */
            binding->lease_time_remaining = (INT32)(lease_remaining/SCK_Ticks_Per_Second) ;
        }    

        else
        {
            /* This lease has expired and should be set as available */
            binding->flags = binding->flags & ~IN_USE;

            /* Reset the lease values */
            binding->end_lease_time = 0;
            binding->lease_time_remaining = 0;
        }    

        /* Unprotect the structures now that we are done. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }
    return;    
}

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Check_Offered_Times                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Check to see if the offered wait time for the binding that has been offered
*     to a client has expired.  If it has, this binding can be offered to other
*     clients.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_OFFERED *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
VOID DHCPS_Check_Offered_Times(DHCPS_OFFERED *offer)
{

    UINT32  current_time;
    UINT32  offer_duration;

    /* Get the offer duration of the current offer */
    offer_duration = offer->offered_wait_time;

    /* Retrieve the current time to determine if the lease has expired */
    current_time = NU_Retrieve_Clock();

    /* Convert to seconds. */
    current_time = current_time/SCK_Ticks_Per_Second;

    /* Protect the data structures while we adjust the lease times. */
    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

    /* Determine if the lease of the current binding has expired */
    if (INT32_CMP((offer->start_offered_time + offer_duration), current_time) < 0)
    {
        /* This lease has expired and should be set as available */
        offer->offered_binding->flags = offer->offered_binding->flags & ~BINDING_OFFERED; 

        /* Unprotect the structures now that we are done. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        /* Clear the offered binding structure. */
        DHCPS_Remove_Offer(offer);

        /* Protect the data structures while we adjust the lease times. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);
    }   

    /* Unprotect the structures now that we are done. */
    NU_Release_Semaphore(&DHCPS_Semaphore);
}


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Lease_Timer                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Timer task to call DHCPS_Update_Lease_Times and  
*     DHCPS_Check_Offered_Times functions.
*
* INPUTS                                                               
*                                                                      
*     UNSIGNED
*     VOID
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
STATIC VOID DHCPS_Lease_Timer(UNSIGNED argc, VOID *argv)
{
    STATUS          status;
    DHCPS_CONFIG_CB *config_cb;
    DHCPS_BINDING   *binding;
    DHCPS_OFFERED   *offered;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();    

    /* Remove compiler warnings */
    DHCPS_UNUSED_PARAMETER(argc);
    DHCPS_UNUSED_PARAMETER(argv);


#ifdef FILE_3_1

    if(NU_Set_Default_Drive(DHCPS_Default_Drive) != NU_SUCCESS)
    {
       NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__); 
       NU_USER_MODE();
       return;
    }

    /*  Set Current Directory to "\" */
    if (NU_Set_Current_Dir("\\") != NU_SUCCESS)
    {
       NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__); 
       NU_USER_MODE();
       return;
    }

#endif 

    else
    {
        while(1)
        {
            /* Sleep for the defined time.  The value of the sleep time is defined in 
                DHCPSERV.H.  */
            NU_Sleep(DHCPS_LEASE_TIMER_SLEEP);

            /* Loop through each of the configuration control blocks so that each of the
                lease times of the bindings can be updated. */
            for (config_cb = DHCPS_Config_List.dhcp_config_head;
                 config_cb;
                 config_cb = config_cb->dhcps_config_next)
            {
                /* Loop through the binding link list and update the lease times of each
                    binding. */
                for (binding = config_cb->ip_binding_list.dhcp_bind_head;
                     binding;
                     binding = binding->dhcp_bind_next)
                {
                    /* Update lease time for binding. */
                    DHCPS_Update_Lease_Times(binding);
                }
            }     

            /* Loop through each of the configuration control blocks so that the offered
                times can be updated. */
            for (config_cb = DHCPS_Config_List.dhcp_config_head;
                 config_cb;
                 config_cb = config_cb->dhcps_config_next)
            {
                /* Loop through the offered link list and update the offered wait times
                    for each entry. */
                for (offered = config_cb->ip_offer_list.dhcp_offer_head;
                     offered;
                     offered = offered->dhcp_offer_next)
                {
                    /* Ensure that there is an offered binding present. */
                    if (offered->offered_binding != NU_NULL)
                    {
                        /* Update the offered times. */      
                        DHCPS_Check_Offered_Times(offered);                  
                    }    
                }         

            }

            /* Save the newly updated binding array to a file */
            status = DHCPS_Save_Binding_To_File();

            if (status != NU_SUCCESS)
            {
                NERRS_Log_Error(NERR_RECOVERABLE, __FILE__, __LINE__);
            }    
        }

#ifdef FILE_3_1
       
       /* Close Disk */
       /* Code should never be reached. Put in for completeness */
       /* FAL_Close_Disk(DHCPS_Default_Drive); */
       /* FAL_Release_File_User(); */
#endif 
        
    }
    /* This mode switch code will never be executed, but has been put into place for completeness of the
        process */
    /*NU_USER_MODE();*/

}


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Create_Config_Entry                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                             
*     Create configuration control block, place it in the control block 
*     link list, and return a pointer to the control block.  If the
*     specified config_name is already present, a message will be 
*     returned to alert the application and a pointer to the control
*     block will be returned. 
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB **
*     CHAR *
*     CHAR *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Create_Config_Entry (DHCPS_CONFIG_CB **config_cb, const CHAR *config_name, CHAR *interface_name)
{
    STATUS                  status = NU_SUCCESS;
    DHCPS_CONFIG_CB         *temp_config;
    DV_DEVICE_ENTRY         *device = NU_NULL;
    UINT8                   interface_name_len,
                            config_found = NU_FALSE;
    VOID                    *dll_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Protect the data structures while we adjust the lease times. */
    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

    /* First, we need to ensure that the configuration control block that is to be added is not already
        present.  This is done by comparing the config entry name. */
    for (temp_config = DHCPS_Config_List.dhcp_config_head;
         temp_config;
         temp_config = temp_config->dhcps_config_next)
    {
        /* Compare the entry name. */         
        if (strcmp(temp_config->config_entry_name, config_name) == NU_SUCCESS)
        {
            /* We have found a match for the entry name.  We do not need to add this entry. */
            config_found = NU_TRUE;

            /* Set the passed in config_cb ptr to the location of the control block that is already
                present. */
            *config_cb = temp_config;

            /* Return a message to the calling application to let it know that the configuration
                control block was already present. */
            status = DHCPSERR_CONFIGURATION_PRESENT;

            break;
        } 
    }         
    /* Unprotect the structures now that we are done. */
    NU_Release_Semaphore(&DHCPS_Semaphore);

    if (!config_found) 
    {
        /* We must allocate a block of memory for the new configuration control block. */
        status = NU_Allocate_Memory (DHCPServer_Memory, (VOID **) &temp_config, 
                                     sizeof(DHCPS_CONFIG_CB), NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Initialize the newly created memory block. */
            UTL_Zero(temp_config, sizeof(DHCPS_CONFIG_CB));
            
            /* If this is to be the global control block, the interface_name pointer will be null.
                    If it is null, the configuration should be placed at the head of the link list. */
            if (!interface_name)
            {
                /* Ensure that this is to be the global entry by verifying the config name. */
                if ((strcmp(config_name, "GLOBAL") == NU_SUCCESS) || 
                    (strcmp(config_name, "global") == NU_SUCCESS))
                {
                    /* Protect global data structures. */
                    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                    /* Add the control block to the link list. */
                    dll_ptr = DLL_Insert(&DHCPS_Config_List, temp_config, 
                                        DHCPS_Config_List.dhcp_config_head);

                    /* Unprotect global data structures. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);

                    if (dll_ptr == NU_NULL)
                    {
                        /* Since we were unable to add the control block to the link list,
                            deallocate the memory and add an error entry. */
                        status = NU_Deallocate_Memory(temp_config);

                        if (status != NU_SUCCESS)
                        {
                            /* Error deallocating memory. */
                            NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
                        }    

                        /* Add an error message for the inability to add the control block. */
                        NERRS_Log_Error(NERR_RECOVERABLE, __FILE__, __LINE__);

                        /* Set the return pointer as NU_NULL. */
                        *config_cb = NU_NULL;

                        /* Unable to get a valid pointer to the network device structure. */
                        status = NU_INVALID_PARM;						
                    }    

                    else
                    {
                        /* Protect global data structures. */
                        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                        /* Set the global configuartion bit in the flag variable. */
                        temp_config->flags = temp_config->flags | GLOBAL_CONFIGURATION;

                        /* Set the entry name as global. */
                        strcpy(temp_config->config_entry_name, config_name);

                        /* Unprotect global data structures. */
                        NU_Release_Semaphore(&DHCPS_Semaphore);
                    }    
                }
                else
                {
                    /* The interface name was left out of the passed in parameters and this control block
                        is designated to be the global. */

                    /* Deallocate the memory. */
                    status = NU_Deallocate_Memory(temp_config);

                    /*  Return an error message to the application. */
                    status = NU_INVALID_PARM;
                }    
            }    

            else
            {
                /* Determine the length of the interface name. */
                interface_name_len = (UINT8)strlen(interface_name);

                /* Protect global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Add the network interface name to the configuration control block. */
                memcpy(temp_config->device_name, interface_name, interface_name_len); 

                /* Unprotect global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);

                /* We must grab the NET semaphore */
                status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);
                if (status == NU_SUCCESS)
                {
                    /* Get a pointer to the device using the device name. */
                    device = DEV_Get_Dev_By_Name(interface_name);

                    /* Ensure that the device pointer is valid before we continue */
                    if (device != NU_NULL)
                    {
                        /* Store the device's IP address in the control block. */
                        PUT32(temp_config->server_ip_addr, 0, device->dev_addr.dev_ip_addr);                    

                        if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                        {
                            /* Log the error */
                            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                        }
                        /* Protect global data structures. */
                        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);
    
                        /* Add the control block to the end of the link list. */
                        dll_ptr = DLL_Enqueue(&DHCPS_Config_List, temp_config);

                        /* Unprotect global data structures. */
                        NU_Release_Semaphore(&DHCPS_Semaphore);

                        if (dll_ptr == NU_NULL)
                        {
                            /* Since we were unable to add the control block to the link list,
                                deallocate the memory and add an error entry. */
                            status = NU_Deallocate_Memory(temp_config);

                            if (status != NU_SUCCESS)
                            {
                                /* Error deallocating memory. */
                                NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
                            }

                            /* Add an error message for the inability to add the control block. */
                            NERRS_Log_Error(NERR_RECOVERABLE, __FILE__, __LINE__);
                        }

                        else
                        {
                            /* Protect global data structures. */
                            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                            /* Add an entry name to the configuration control block. */
                            strcpy(temp_config->config_entry_name, config_name);

                            /* Unprotect global data structures. */
                            NU_Release_Semaphore(&DHCPS_Semaphore);
                        }
                    }

                    else
                    {
                        /* Device does not exist. Release the NET semaphore */
                        if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                        {
                            /* Log the error */
                            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                        }
                        /* Since we were unable to add the control block to the link list,
                            deallocate the memory and add an error entry. */
                        status = NU_Deallocate_Memory(temp_config);

                        if (status != NU_SUCCESS)
                        {
                            /* Error deallocating memory. */
                            NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
                        }    

                        /* Add an error message for the inability to add the control block. */
                        NERRS_Log_Error(NERR_RECOVERABLE, __FILE__, __LINE__);

                        /* Set the return pointer as NU_NULL. */
                        *config_cb = NU_NULL;

                        /* Unable to get a valid pointer to the network device structure. */
                        status = NU_INVALID_PARM;

					}    

 
                }
                else
                {
                    /* Failed to obtain the NET semaphore */
                    NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);

                }
            }
        }    
        else
        {
            /* Add an error message for the inability of allocating memory. */
            NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
        }    

        /* Set the passed in config_cb ptr to the location of the newly created control block. */
        *config_cb = temp_config;
        
    }    
    /* Switch back to user mode */
    NU_USER_MODE();

    return (status);
}   /* DHCPS_Create_Config_Entry */

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Delete_Config_Entry                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Remove the specified configuration control block from the link list. 
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Delete_Config_Entry (const DHCPS_CONFIG_CB *config_cb)
{
    STATUS                  status = NU_SUCCESS;
    DHCPS_CONFIG_CB         *temp_config;
    DHCPS_BINDING           *binding,*temp_bind;
    DHCPS_OPTIONS           *options,*temp_opt;
    DHCPS_OFFERED           *offered,*temp_off;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Protect global data structures. */
    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

    /* Search through the link list to find the correct control block. */
    for (temp_config = DHCPS_Config_List.dhcp_config_head; 
         temp_config; 
         temp_config = temp_config->dhcps_config_next)
    {
        if (temp_config == config_cb)
            break;
    }

    /* Unprotect global data structures. */
    NU_Release_Semaphore(&DHCPS_Semaphore);

    if (temp_config == NU_NULL)
        status = NU_INVALID_PARM;

    else
    {
        /* Protect global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Deallocate the memory used in the control blocks. */
        for (options = temp_config->options_buff.dhcps_options_head; options ;)
        {
            /* Deallocate the memory for the option block. */
            if (options->options_ptr != NU_NULL)
                status = NU_Deallocate_Memory(options->options_ptr);

            if (status != NU_SUCCESS)
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);

            /* Store current options pointer in a temporary pointer */
            temp_opt = options;

            /* Get next option pointer */
            options = options->dhcps_options_next;

            /* Deallocate the memory for the options structure. */
            status = NU_Deallocate_Memory(temp_opt);

            if (status != NU_SUCCESS)
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
        }

        for (binding = temp_config->ip_binding_list.dhcp_bind_head; binding ;)
        {
            /* Store current binding pointer in a temporary pointer */
            temp_bind = binding;

            /* Get the next binding */
            binding = binding->dhcp_bind_next;

            /* Deallocate the memory for the binding structure. */
            status = NU_Deallocate_Memory(temp_bind);

            if (status != NU_SUCCESS)
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
        }

        for (offered = temp_config->ip_offer_list.dhcp_offer_head; offered ;)
        {
            /* Store current offered pointer in a temporary pointer */
            temp_off = offered;

            /* Get the next offered structure */
            offered = offered->dhcp_offer_next;

            /* Deallocate the memory for the offered IP address list. */
            status = NU_Deallocate_Memory(temp_off);

            if (status != NU_SUCCESS)
                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Remove the control block from the list */
        DLL_Remove(&DHCPS_Config_List, temp_config);

        /* Unprotect global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        /* Initialize all values of the control block */
        UTL_Zero(temp_config, sizeof(DHCPS_CONFIG_CB));

        /* Deallocate the memory that we were unable to enqueue back to the 
            list. */
        status = NU_Deallocate_Memory(temp_config);

        if (status != NU_SUCCESS)
        {
            /* An error occurred during deallocating the memory.  Log an error. */
            NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
        }    
        
        else
            status = NU_SUCCESS;
    }    
    /* Switch back to user mode */
    NU_USER_MODE();

    return(status);
}   /* DHCPS_Delete_Config_Entry */

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Get_Config_Entry                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                        
*     This function will copy each of the configuration control block pointers
*     into the buffer provided by the application.  The total number
*     of control block pointers that were copied will be returned.
*
* INPUTS                                                               
*                                                                      
*     UINT8 *
*     INT                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     INT                                    
*                                                                      
**********************************************************************************/
INT DHCPS_Get_Config_Entry (UINT8 *config_buffer, INT buffer_size)
{
    INT                     ret_status = NU_SUCCESS,
                            i;
    DHCPS_CONFIG_CB         *current_config;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that each of the passed-in parameters are valid */
    if (config_buffer == NU_NULL)
        ret_status = NU_INVALID_PARM;

    else
    {
        /* Loop through each of the entries of the link list and copy the control block
            pointer into the config_array. */
        for (current_config = DHCPS_Config_List.dhcp_config_head, i = 0;
             current_config;
             current_config = current_config->dhcps_config_next)
        {
            if (buffer_size < sizeof(DHCPS_CONFIG_CB *))
            {
                ret_status = DHCPSERR_BUFFER_TOO_SMALL;
                break;
            }    

            else
            {
                /* Protect global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Copy all of the configuration values into the provided buffer. */
                memcpy(config_buffer, &current_config, sizeof(DHCPS_CONFIG_CB *));

                /* Increment the buffer pointer. */
                config_buffer += sizeof(DHCPS_CONFIG_CB *);

                /* Increment the counter that tracks the number of configurations
                    that are copied into the buffer. */
                i += sizeof(DHCPS_CONFIG_CB *);

                /* Decrement the buffer size by the amount that was just copied. */
                buffer_size -= sizeof(DHCPS_CONFIG_CB *);

                /* Unprotect global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);
            }    
        }            

        if (ret_status == NU_SUCCESS)      
            ret_status = i;
    }    
    /* Switch back to user mode */
    NU_USER_MODE();

    return(ret_status);
}   /* DHCPS_Get_Config_Entry */



/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Add_Option_To_Memory_Block                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     The binding-specific option will have room in a block of memory allocated
*     so that the application may place the option into the memory block.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     struct id_struct *
*     UINT8
*     DHCPS_OPTIONS **
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     UINT8 *                                    
*                                                                      
**********************************************************************************/
UINT8 *DHCPS_Add_Option_To_Memory_Block (DHCPS_CONFIG_CB *config_cb, 
                                         const struct id_struct *client_ip_addr, 
                                         UINT8 option_being_added, DHCPS_OPTIONS **option_buffer_ptr)
{
    UINT8               option_buffer[25],
                        *option_block,
                        block_length_to_move,
                        option_length_to_move,
                        remaining_bytes,
                        temp_ip_addr[IP_ADDR_LEN],
                        i,
                        *test_ptr,
                        *option_ptr = NU_NULL,
                        *return_ptr = NU_NULL;
    DHCPS_OPTIONS       *temp_options;
    STATUS              ret_status;
    INT                 found = NU_FALSE;
    VOID                *dll_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Search to see if this IP address has had a previous option block allocated for it. */
    for (temp_options = config_cb->options_buff.dhcps_options_head;
         temp_options;
         temp_options = temp_options->dhcps_options_next)
    {
        /* Protect global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Save the temp_options IP address to a local variable. */
        memcpy(temp_ip_addr, temp_options->client_ip_addr.is_ip_addrs, IP_ADDR_LEN);

        /* Unprotect global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        if (DHCPSDB_IP_Compare(temp_ip_addr, client_ip_addr->is_ip_addrs) == NU_SUCCESS)
        {
            /* We have found a match. */
            break;
        }                        
    }

    if (temp_options == NU_NULL)
    {
        /* This IP address has not had a static entry previously added.  Therefore, we
            must allocate memory for the options structure. */
        ret_status = NU_Allocate_Memory (DHCPServer_Memory, (VOID **) &temp_options, 
                        sizeof(DHCPS_OPTIONS), NU_SUSPEND);

        if (ret_status == NU_SUCCESS)
        {
            /* We must allocate the memory block, add the IP address to the structure and 
                add the DNS server to the memory block. */
            ret_status = NU_Allocate_Memory (DHCPServer_Memory, (VOID **) &option_block, 
                         DHCPS_OPTIONS_MEMORY_BLOCK_SIZE, NU_SUSPEND);

            if (ret_status == NU_SUCCESS)
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Add the client's IP address to the options structure. */
                memcpy(temp_options->client_ip_addr.is_ip_addrs, client_ip_addr->is_ip_addrs, IP_ADDR_LEN);

                /* Set the pointer of the block of memory that was allocated to the option_ptr of the 
                    structure. */
                temp_options->options_ptr = option_block;

                /* Save the configuration control block name to the options buffer structure. */
                strcpy(temp_options->config_entry_name, config_cb->config_entry_name);

                /* Add this new options buffer to the link list. */
                dll_ptr = DLL_Enqueue(&config_cb->options_buff, temp_options);

                if (dll_ptr != NU_NULL)
                {
                    /* Initialize the number of bytes written into the buffer. */
                    temp_options->bytes_written = 0;

                    /* Copy the end option tag into the buffer. */
                    *option_block = DHCP_END;

                    /* Increment the number of bytes written into the buffer. */
                    temp_options->bytes_written++;

                    /* Set the return pointer to the memory block so that the option may be added. */
                    return_ptr = option_block;

                    /* Set the past in pointer to the new option_buffer. */
                    *option_buffer_ptr = (VOID *)temp_options;
                }

                else
                {
                    /* An error occurred adding the new entry to the option link list.  Deallocate
                        the memory for the structure and buffer and logg an error. */
                    NU_Deallocate_Memory(option_block);
                    NU_Deallocate_Memory(temp_options);

                    NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                }

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);
            }

            else
            {
                NU_Deallocate_Memory(temp_options);

                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
            }    
        }
        else
        {
            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
        }    
    }    

    else
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* This IP address has a memory block allocated.  Search the block to see if the 
            option that we are adding is already in place. */

        /* Save off the pointer to the option block. */
        test_ptr = temp_options->options_ptr;

        /* Set the past in pointer to the option_buffer. */
        *option_buffer_ptr = (VOID *)temp_options;

        for (i = 0; i < DHCPS_OPTIONS_MEMORY_BLOCK_SIZE; i++)
        {
            /* If we found the desired option tag, break. */
            if (*test_ptr == option_being_added)
            {
                found = NU_TRUE;
                
                /* Save the location of the option tag so that it may be returned. */
                option_ptr = test_ptr;
            
                /* Move the pointer past the option data. */
                test_ptr += (*(test_ptr + 1) + 2);

                break;
            }  

            /* If we have reached the end of all of the options that in the block, break. */
            else if (*test_ptr == DHCP_END)
            {
                /* We have reached the end.  Break. */
                break;
            }    

            else
            {
                /* Increase the incrementing variable to account for this movement.  Only +1
                    is being added since the for loop will increment i. */
                i += *(test_ptr + 1) + 1;

                /* Increment past the option and data to the next option. */
                test_ptr += (*(test_ptr + 1) + 2);
            }    
        }    

        if ((found == NU_TRUE) && (option_ptr != NU_NULL))
        {    

            /* Determine if anything needs to be moved to accomodate the adding of this
                    new option value. */
            if (*test_ptr != DHCP_END)
            {
                /* Get the length of the option that we will be moving to the end
                    of the buffer. */
                option_length_to_move = (*(option_ptr + 1) + 2);

                /* Copy the option into a temporary buffer. */
                memcpy(option_buffer, option_ptr, option_length_to_move);
                
                /* Set a pointer to the location where we will begin to copy the
                    remainder of the buffer. */
                test_ptr = option_ptr + (*(option_ptr + 1) + 2);

                /* Get the total number of bytes left in the buffer that need to be copied. */
                remaining_bytes = temp_options->bytes_written - option_length_to_move;

                /* Calculate the number of bytes that will need to be moved. */
                block_length_to_move = (UINT8)(remaining_bytes - (option_ptr - temp_options->options_ptr));
                                        
                /* Move the remainder of the buffer up to were the previous option was placed. */
                memcpy(option_ptr, test_ptr, block_length_to_move);

                /* Move the pointer to the end of the moved data. This should be the DHCPEND tag.*/
                option_ptr += block_length_to_move - 1;

                /* Copy the displaced option back into the buffer. */
                memcpy(option_ptr, option_buffer, option_length_to_move);

                /* Set the return pointer to the location of the option tag. */
                return_ptr = option_ptr;

                /* Increment the buffer pointer to the end of the option that was just copied in. */
                option_ptr += option_length_to_move;

                /* Place a new end tag. */
                *option_ptr = DHCP_END;
            } 

            else
            {
                /* Set the return pointer to new location. */
                return_ptr = option_ptr;
            }    
        }

        else
        {
            /* The specified option was not found in the memory block.  Therefore, the option can be added to 
                the end of the memory block. The option ptr needs to be decremented one space so that it is 
                pointing at the DHCP_END tag.  This is where the new option should be added. */
            return_ptr = test_ptr;
        }    

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }    

    /* Switch back to user mode */
    NU_USER_MODE();
    
    /* Return a pointer to the option block where the new option can be added. */
    return(return_ptr);
    
}   /* DHCPS_Add_Option_To_Memory_Block */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Remove_Option_From_Memory_Block                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Remove the specified option from the memory block.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     struct id_struct *
*     UINT8
*     VOID *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
***********************************************************************************/
STATUS DHCPS_Remove_Option_From_Memory_Block (const DHCPS_CONFIG_CB *config_cb, 
                                              const struct id_struct *client_ip_addr, UINT8 option_to_delete, 
                                              const VOID *option_value)                                              
{
    UINT8           i, j,
                    delete_entire_option = NU_FALSE,
                    *test_ptr,
                    temp_ip_addr[IP_ADDR_LEN],
                    *option_ptr,
                    remaining_bytes,
                    block_length_to_move,
                    *option_length_ptr,
                    option_length;
    DHCPS_OPTIONS   *temp_options;
    STATUS          ret_status;
    INT             found = NU_FALSE,
                    option_found = NU_FALSE;

    /* Search for the IP address in the static options. */
    for (temp_options = config_cb->options_buff.dhcps_options_head;
         temp_options;
         temp_options = temp_options->dhcps_options_next)             
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Save the temp_options IP address to a local variable. */
        memcpy(temp_ip_addr, temp_options->client_ip_addr.is_ip_addrs, IP_ADDR_LEN);

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        if (DHCPSDB_IP_Compare(temp_ip_addr, client_ip_addr->is_ip_addrs) == NU_SUCCESS)
        {
            /* We have found our match.  We can break. */
            break;
        }    
        else
            continue;
    }         

    if (!temp_options)
    {
        /* The specified IP address was not found in the options block. */
        ret_status = NU_INVALID_PARM;
    }    

    else
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* This IP address has a memory block allocated.  Search the block to see if the 
            option that we are deleting is actually in place. */

        /* Save off the pointer to the option block. */
        test_ptr = temp_options->options_ptr;

        for (i = 0; i < DHCPS_OPTIONS_MEMORY_BLOCK_SIZE; i++)
        {
            /* If we found the DNS server option tag, break. */
            if (*test_ptr == option_to_delete)
            {
                found = NU_TRUE;
                break;
            }  

            /* If we have reached the end of all of the options that in the block, break. */
            else if (*test_ptr == DHCP_END)
                break;

            else
            {
                /* Increase the incrementing variable to account for this movement. */  
                i += *(test_ptr + 1) + 1;

                /* Increment past the option and data to the next option. */
                test_ptr += (*(test_ptr + 1) + 2);
            }
        }            

        if (found == NU_TRUE)
        {
            /* We have found the option inside the option memory block.  Now, we must locate the specific
                option data that we are deleting. */

            /* Increment up to the the option length and save off these value. */
            option_length = *(test_ptr + 1);
            
            /* Save a pointer to the option length so that it can be adjusted once the option has been 
                deleted. */
            option_length_ptr = (test_ptr + 1);

            /* Increment the pointer to the option data. */
            test_ptr += 2;

            /* Compare each of the entries in the option data to the option that is being added */
            for (j = 0; j < option_length; )
            {
                if (option_to_delete == DNS_DOMAIN_NAME)
                {
                    ret_status = memcmp((test_ptr + j), option_value, option_length);

                    if (ret_status == NU_SUCCESS)
                    {
                        /* A match for the option that is being deleted has been found.  We can break out
                            of the for loop. */
                        option_found = NU_TRUE;
                        j += option_length;
                        break;
                    }    

                    else
                    {
                        j += option_length;
                    }    
                }

                else
                {
                    ret_status = memcmp((test_ptr + j), option_value, IP_ADDR_LEN);

                    if (ret_status == NU_SUCCESS)
                    {
                        /* A match for the option that is being deleted has been found.  We can break out
                            of the for loop. */
                        option_found = NU_TRUE;
                        j += IP_ADDR_LEN;
                        break;
                    }
                
                    else
                    {
                        j += IP_ADDR_LEN;
                    }
                }    
            }

            if (option_found == NU_TRUE)
            {
                /*  Now, we need to remove the specific option by shifting everything that is past 
                    the option backward in the memory block. */

                /* Move the pointer past the option that is being deleted. */
                if (option_to_delete == DNS_DOMAIN_NAME)
                {
                    test_ptr += option_length;
                }    

                else
                    /*test_ptr += IP_ADDR_LEN;*/
                    test_ptr += j;

                /* Save a pointer to the location of the data block that will be moved. */
                option_ptr = test_ptr;

                /* If this entry is the only one for this option, we can delete the option tag and 
                    the data length as well. */

                /* Move the option pointer back to the position of the option that was moved. */
                if (option_to_delete == DNS_DOMAIN_NAME)
                {
                    if (*option_length_ptr == option_length)
                    {
                        test_ptr -= (option_length + 2);
                        delete_entire_option = NU_TRUE;
                    }    
                    else    
                        test_ptr -= option_length;
                }    
                else
                {
                    if (*option_length_ptr == IP_ADDR_LEN)
                    {
                        test_ptr -= IP_ADDR_LEN + 2;    
                        delete_entire_option = NU_TRUE;
                    } 

                    else
                        test_ptr -= IP_ADDR_LEN;
                }    

                /* Get the total number of bytes left in the buffer. */
                if (option_to_delete == DNS_DOMAIN_NAME)
                {
                    if (delete_entire_option == NU_FALSE)
                    {
                        remaining_bytes = temp_options->bytes_written - option_length;
                    }

                    else
                        remaining_bytes = temp_options->bytes_written - (option_length + 2);
                }    
                else
                {
                    if (delete_entire_option == NU_FALSE)
                    {
                        remaining_bytes = temp_options->bytes_written - IP_ADDR_LEN;
                    }

                    else
                        remaining_bytes = temp_options->bytes_written - (IP_ADDR_LEN + 2);
                }    

                /* Calculate the number of bytes that will need to be moved. */
                block_length_to_move = (UINT8)(remaining_bytes - (test_ptr - temp_options->options_ptr));

                /* Now, reduce the option data length by the number of bytes removed. */
                if (option_to_delete == DNS_DOMAIN_NAME)
                {
                    if (delete_entire_option == NU_FALSE)
                    {
                        *option_length_ptr -= option_length;
                        temp_options->bytes_written -= option_length;
                    }

                    else
                        temp_options->bytes_written -= (option_length + 2);
                }    
                else
                {
                    if (delete_entire_option == NU_FALSE)
                    {
                        *option_length_ptr -= IP_ADDR_LEN;
                        temp_options->bytes_written -= IP_ADDR_LEN;
                    }

                    else
                        temp_options->bytes_written -= (IP_ADDR_LEN + 2);
                }

                /* Paste the contents of the memory block buffer to the options block. */
                memcpy(test_ptr, option_ptr, block_length_to_move);

                /* Return that the option has been successfully removed. */
                ret_status = NU_SUCCESS;
            }

            else
                /* The specified option IP address was not found in the options block. */
                ret_status = DHCPSERR_PARAMETER_NOT_FOUND;
        }

        else
            /* The specified option was not found in the options block. */
            ret_status = DHCPSERR_PARAMETER_NOT_FOUND;

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }    
    return(ret_status);
}   /* DHCPS_Remove_Option_From_Memory_Block */                                                


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Get_Option_From_Memory_Block                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Get all values from the memory block that are associated with the specified
*     option.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_BINDING *
*     struct id_struct *
*     UINT8
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     UINT8 *.                                    
*                                                                      
**********************************************************************************/
UINT8 *DHCPS_Get_Option_From_Memory_Block (const DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                            UINT8 option_to_get)
{
    UINT8               i,
                        *test_ptr,
                        temp_ip_addr[IP_ADDR_LEN],
                        *return_ptr;
    DHCPS_OPTIONS       *temp_options;
    INT                 found = NU_FALSE;


    /* Search to see if this IP address has had a previous option block allocated for it. */
    for (temp_options = config_cb->options_buff.dhcps_options_head;
         temp_options;
         temp_options = temp_options->dhcps_options_next)         
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Save the temp_options IP address to a local variable. */
        memcpy(temp_ip_addr, temp_options->client_ip_addr.is_ip_addrs, IP_ADDR_LEN);

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        if (DHCPSDB_IP_Compare(temp_ip_addr, client_ip_addr->is_ip_addrs) == NU_SUCCESS)
        {
            /* We have found a match.  Break. */
            break;
        }    

        else
            continue;         
    }         

    if (temp_options == NU_NULL)
    {
        /* The specified static IP address was not found in the options link list. */
        return_ptr = NU_NULL;
    }    

    else
    {
        /* This IP address has a memory block allocated.  Search the block to see if the 
            option that we are adding is already in place. */

        /* Save off the pointer to the option block. */
        test_ptr = temp_options->options_ptr;

        for (i = 0; i < DHCPS_OPTIONS_MEMORY_BLOCK_SIZE; i++)
        {
            /* If we found the option tag, break. */
            if (*test_ptr == option_to_get)
            {
                found = NU_TRUE;
                break;
            }  

            /* If we have reached the end of all of the options that in the block, break. */
            else if (*test_ptr == DHCP_END)
                break;

            else
            {
                /* Increase the incrementing variable to account for this movement.  The +1
                    is being left off since the for loop will increment i. */
                i += *(test_ptr + 1);

                /* Increment past the option and data to the next option. */
                test_ptr += (*(test_ptr + 1) + 2);
            }    
        }    

        if (found == NU_TRUE)
        {
            /* Now that we have found the desired options, return a pointer to the option. */
            return_ptr = test_ptr;            
        }

        else
            /* The desired option was not located inside the option memory block.  Return a null
                pointer to signify that the option was not found. */
            return_ptr = NU_NULL;   
    }
    return (return_ptr);
}   /* DHCPS_Get_Option_From_Memory_Block */    



/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Set_Subnet_Address                                     
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Add the subnet address that is associated with the IP addresses of the bindings
*     for the control block.  The subnet address must be present for a control block
*     to grant leases to the IP bindings.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     struct id_struct *
*     struct id_struct *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Set_Subnet_Address (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                const struct id_struct *subnet_addr)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT8                   *options_block;
    DHCPS_OPTIONS           *option_buffer;
                            
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, we must determine if this subnet mask entry is for a particular IP address (static IP).  
        This can be determined if a client IP address was passed in.  If so, then the subnet 
        address needs to be added to an IP option block. */
    if (client_ip_addr != NU_NULL)
    {
        /* Perform any memory block manipulation that may need to be done to make room for
            the subnet address that is being added. */
        options_block = DHCPS_Add_Option_To_Memory_Block(config_cb, client_ip_addr, SUBNET_ADDRESS,
                                                         &option_buffer);    

        /* Test to ensure that the option block has been prepared to receive the new option. */
        if (options_block != NU_NULL)
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* The option can now be added to the options block. First, we must ensure that 
                the option has been previously added.  If not, we can just add the option tag and
                data length now. */
            if (*options_block == DHCP_END)
            {
                /* Store the option into the memory block in the format that it will be
                   provided to the client. */

                /* Option Tag */
                *options_block++ = SUBNET_ADDRESS;

                /* Data Length */
                *options_block++ = IP_ADDR_LEN;

                /* Subnet address */
                memcpy(options_block, subnet_addr, IP_ADDR_LEN);

                /* Increment the options block ptr */
                options_block += IP_ADDR_LEN;

                /* Copy the end option tag into the buffer. */
                *options_block = DHCP_END;

                /* Increment the bytes written. */
                option_buffer->bytes_written += (IP_ADDR_LEN + 2);

            }    

            else
            {
                /* We must overwrite the option value that is currently in place.  */

                /* Increment to the location of the option data. */
                options_block += 2;

                /* Subnet address */
                memcpy(options_block, subnet_addr, IP_ADDR_LEN);

                /* Increment the options block ptr */
                options_block += IP_ADDR_LEN;

                /* Add the end option tag. */
                *options_block = DHCP_END;

                /* Increment the bytes written. */
                option_buffer->bytes_written += IP_ADDR_LEN;
            }

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }

        else
            /* An invalid parameter was passed into the function.  The subnet address can not be added. */
            ret_status = NU_INVALID_PARM;
    }       

    else
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Copy the subnet address into the structure of the control block. */
        memcpy(config_cb->subnet_addr, subnet_addr->is_ip_addrs, IP_ADDR_LEN); 

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
                
        /* Set the return status as successful */
        ret_status = NU_SUCCESS;
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (ret_status);
} /* DHCPS_Set_Subnet_Address */


/****************************************************************************
* FUNCTION                                                                   
*                                                                            
*   DHCPS_Get_Subnet_Address                                                       
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*   Returns the value of the subnet address for the specified configuration
*   control block.  The returning value will be the total number
*   of bytes that were written into the provided buffer.
*
* INPUTS                                                                     
*                                                                            
*   DHCPS_CONFIG_CB *
*   struct id_struct *
*   UINT8 *
*   INT
*                                                                            
* OUTPUTS                                                                    
*                                                                            
*   INT
*                                                                            
******************************************************************************/
INT DHCPS_Get_Subnet_Address (const DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                UINT8 *subnet_buffer, INT buffer_size)
{
    UINT8                       *option_ptr,
                                option_data_len;
    INT                         bytes_written;
 
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, ensure that the buffer that has been passed in is large enough to accept at least one
        entry. */
    if ((buffer_size < IP_ADDR_LEN) || (subnet_buffer == NU_NULL))
    {
        bytes_written = NU_INVALID_PARM;
    }

    /* Check to see if the subnet address entry is destined for a static IP entry. */
    else if (client_ip_addr != NU_NULL)
    {
        /* Search through the static IP entries to find the subnet address entry. */
        option_ptr = DHCPS_Get_Option_From_Memory_Block(config_cb, client_ip_addr, SUBNET_ADDRESS);

        if (option_ptr == NU_NULL)
        {
            /* An error occurred during the search for the subnet address for the particular IP address. */
            bytes_written = NU_INVALID_PARM;
        }    

        else
        {
            /* The subnet address option has been found for the desired IP address. */

            /* Increment the option pointer to point at the length of the option. */
            option_data_len = *(option_ptr + 1);

            /* Ensure that the buffer is large enough to hold the option data. */
            if (buffer_size < option_data_len)
            {
                bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
            }    

            else
            {
                /* Increment the option pointer to point at the data that will be copied. */
                option_ptr += 2;

                /* Copy the option into the buffer that has been provided by the application. */
                memcpy(subnet_buffer, option_ptr, option_data_len);

                /* Save the number of bytes that have been written into the buffer for returning to
                    the application. */
                bytes_written = option_data_len;
            }    
        }    
    }    

    else 
    {
        /* Ensure that the buffer is large enough to hold the option data. */
        if (buffer_size < IP_ADDR_LEN)
        {
            bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
        }    

        else        
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the subnet address option entry into the buffer provided by the application. */
            memcpy(subnet_buffer, config_cb->subnet_addr, IP_ADDR_LEN);

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            /* Set the return variable to the number of bytes that were written into the buffer. */
            bytes_written = IP_ADDR_LEN;  
        }    
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (bytes_written); 

} /* DHCPS_Get_Subnet_Address */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Set_Subnet_Mask                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Adds a subnet mask to the specified configuration control block or specific
*     binding
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     struct id_struct *
*     struct id_struct *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Set_Subnet_Mask (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                const struct id_struct *subnet_mask)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT8                   *options_block;
    DHCPS_OPTIONS           *option_buffer;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, we must determine if this subnet mask entry is for a particular IP address (static IP).  
        This can be determined if a client IP address was passed in.  If so, then the subnet 
        mask needs to be added to an IP option block. */
    if (client_ip_addr != NU_NULL)
    {
        /* Perform any memory block manipulation that may need to be done to make room for
            the subnet mask that is being added. */
        options_block = DHCPS_Add_Option_To_Memory_Block(config_cb, client_ip_addr, SUBNET_MASK, 
                                                            &option_buffer);    

        /* Test to ensure that the option block has been prepared to receive the new option. */
        if (options_block != NU_NULL)
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* The option can now be added to the options block. First, we must ensure that 
                the option has been previously added.  If not, we can just add the option tag and
                data length now. */
            if (*options_block == DHCP_END)
            {
                /* Store the option into the memory block in the format that it will be
                   provided to the client. */

                /* Option Tag */
                *options_block++ = SUBNET_MASK;

                /* Data Length */
                *options_block++ = IP_ADDR_LEN;

                /* Subnet mask */
                memcpy(options_block, subnet_mask, IP_ADDR_LEN);

                /* Increment the option block ptr */
                options_block += IP_ADDR_LEN;

                /* Copy the end option tag into the buffer. */
                *options_block = DHCP_END;

                /* Increment the bytes written. */
                option_buffer->bytes_written += (IP_ADDR_LEN + 2);

            }    

            else
            {
                /* We must overwrite the option value that is currently in place.  */

                /* Increment to the location of the option data. */
                options_block += 2;

                /* Subnet mask */
                memcpy(options_block, subnet_mask, IP_ADDR_LEN);

                /* Increment the option block ptr */
                options_block += IP_ADDR_LEN;

                /* Add the end option tag. */
                *options_block = DHCP_END;

                /* Increment the bytes written. */
                option_buffer->bytes_written += IP_ADDR_LEN;
            }

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }

        else
            /* An invalid parameter was passed into the function.  The subnet mask can not be added. */
            ret_status = NU_INVALID_PARM;
    }       

    else
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Copy the subnet mask into the structure of the control block. */
        memcpy(config_cb->subnet_mask_addr, subnet_mask, IP_ADDR_LEN); 

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
                
        /* Set the return status as successful */
        ret_status = NU_SUCCESS;
    }
    /* Switch to user mode */
    NU_USER_MODE();

    return (ret_status);
} /* DHCPS_Set_Subnet_Mask */


/****************************************************************************
* FUNCTION                                                                   
*                                                                            
*    DHCPS_Get_Subnet_Mask                                                       
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*    Returns the subnet mask from the specified configuration control block
*    or specific binding.  The returning value will be the total number
*    of bytes that were written into the provided buffer.
*
* INPUTS                                                                     
*                                                                            
*    DHCPS_CONFIG_CB *
*    struct id_struct *
*    UINT8 *
*    INT
*                                                                            
* OUTPUTS                                                                    
*                                                                            
*    INT
*                                                                            
******************************************************************************/
INT DHCPS_Get_Subnet_Mask (const DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                UINT8 *subnet_buffer, INT buffer_size)
{
    UINT8           *option_ptr,
                    option_data_len;
    INT             bytes_written;
                                
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, ensure that the passed in parameters are valid. */
    if ((config_cb == NU_NULL) || (subnet_buffer == NU_NULL))
    {
        bytes_written = NU_INVALID_PARM;
    }

    /* Check to see if the subnet mask entry is destined for a static IP entry. */
    else if (client_ip_addr != NU_NULL)
    {
        /* Search through the static IP entries to find the subnet mask entry. */
        option_ptr = DHCPS_Get_Option_From_Memory_Block(config_cb, client_ip_addr, SUBNET_MASK);

        if (option_ptr == NU_NULL)
        {
            /* An error occurred during the search for the subnet mask for the particular IP address. */
            bytes_written = NU_INVALID_PARM;
        }    

        else
        {
            /* The subnet mask option has been found for the desired IP address. */
            
            /* Increment the option pointer to point at the length of the option. */
            option_data_len = *(option_ptr + 1);

            /* Ensure that the buffer is large enough for the option data. */
            if (buffer_size < option_data_len)
            {
                bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
            }    

            else
            {
                /* Increment the option pointer to point at the data that will be copied. */
                option_ptr += 2;

                /* Copy the option into the buffer that has been provided by the application. */
                memcpy(subnet_buffer, option_ptr, option_data_len);

                /* Save the number of bytes that have been written into the buffer for returning to
                    the application. */
                bytes_written = option_data_len;
            }    
        }    
    }    

    else 
    {
        /* Ensure that the buffer is large enough to copy the option data. */
        if (buffer_size < IP_ADDR_LEN)
        {
            bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
        }    

        else
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the subnet mask option entry into the buffer provided by the application. */
            memcpy(subnet_buffer, config_cb->subnet_mask_addr, IP_ADDR_LEN);

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            /* Set the return variable to the number of bytes that were written into the buffer. */
            bytes_written = IP_ADDR_LEN;      
        }    
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (bytes_written); 

} /* DHCPS_Get_Subnet_Mask */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Add_Broadcast_Address                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Add a broadcast address to the specific configuration control block or the
*     specified binding.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     struct id_struct *
*     struct id_struct *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Add_Broadcast_Address (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                    const struct id_struct *broadcast_addr)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT8                   *options_block;
    DHCPS_OPTIONS           *option_buffer;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
                               
    /* Ensure that the passed in parameters are valid. */
    if ((config_cb == NU_NULL) || (broadcast_addr == NU_NULL))
    {
        ret_status = NU_INVALID_PARM;
    }    

    else
    {
        /* First, we must determine if this broadcast address entry is for a particular IP address 
            (static IP).  This can be determined if a client IP address was passed in.  If so, then 
            the subnet mask needs to be added to an IP option block. */
        if (client_ip_addr != NU_NULL)
        {
            /* Perform any memory block manipulation that may need to be done to make room for
                the broadcast address that is being added. */
            options_block = DHCPS_Add_Option_To_Memory_Block(config_cb, client_ip_addr, DHCP_BROADCAST_ADDR, 
                                                                &option_buffer);    

            /* Test to ensure that the option block has been prepared to receive the new option. */
            if (options_block != NU_NULL)
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* The option can now be added to the options block. First, we must ensure that 
                    the option has been previously added.  If not, we can just add the option tag and
                    data length now. */
                if (*options_block == DHCP_END)
                {
                    /* Store the option into the memory block in the format that it will be
                       provided to the client. */

                    /* Option Tag */
                    *options_block++ = DHCP_BROADCAST_ADDR;

                    /* Data Length */
                    *options_block++ = IP_ADDR_LEN;

                    /* Broadcast address */
                    memcpy(options_block, broadcast_addr, IP_ADDR_LEN);

                    /* Increment the options block ptr */
                    options_block += IP_ADDR_LEN;

                    /* Copy the end option tag into the buffer. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += (IP_ADDR_LEN + 2);

                }    

                else
                {
                    /* We must overwrite the option value that is currently in place.  */

                    /* Increment to the location of the option data. */
                    options_block += 2;

                    /* Broadcast address */
                    memcpy(options_block, broadcast_addr, IP_ADDR_LEN);

                    /* Increment the options block ptr */
                    options_block += IP_ADDR_LEN;

                    /* Add the end option tag. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += IP_ADDR_LEN;
                }

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);
            }

            else
                /* An invalid parameter was passed into the function.  The broadcast address can not 
                    be added. */
                ret_status = NU_INVALID_PARM;
        }       

        else
        {
            /* This broadcast address entry is not meant for a certain IP address.  It can be added 
                to configuration control block. */

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the broadcast address into the structure of the control block. */
            memcpy(config_cb->broadcast_addr, broadcast_addr, IP_ADDR_LEN); 
                    
            /* Set the return status as successful */
            ret_status = NU_SUCCESS;

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (ret_status);
} /* DHCPS_Add_Broadcast_Address */

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Delete_Broadcast_Address                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Remove the broadcast address from the specified configuration control block or
*     the specific binding.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     struct id_struct *
*     struct id_struct *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Delete_Broadcast_Address (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                            const struct id_struct *broadcast_addr)
{
    STATUS  ret_status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that the passed in parameters are valid. */
    if ((config_cb == NU_NULL) || (broadcast_addr == NU_NULL))
    {
        ret_status = NU_INVALID_PARM;
    }    

    else
    {
        /* First, we must determine if the broadcast address is in a particular IP address option block.  
            This can be determined if a client IP address was passed in. */
        if (client_ip_addr != NU_NULL)
        {
            /* The broadcast address must be removed from the option memory block and the remaining 
                contents of the block adjusted to compensate for the removal of an entry. */
            ret_status = DHCPS_Remove_Option_From_Memory_Block(config_cb, client_ip_addr, DHCP_BROADCAST_ADDR, 
                                                                broadcast_addr);
        }

        else
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Ensure that we have a match of the broadcast address. */
            if (DHCPSDB_IP_Compare(config_cb->broadcast_addr, broadcast_addr->is_ip_addrs) == NU_SUCCESS)
            {
                /* Since the broadcast address entry to be removed is not a IP specific option, the option might
                    be a configuration-wide entry.  Therefore, we will clear the broadcast address structure from 
                    the control block. */
                memset(config_cb->broadcast_addr, 0, IP_ADDR_LEN);

                ret_status = NU_SUCCESS;
            }

            else
            {
                ret_status = DHCPSERR_PARAMETER_NOT_FOUND;
            }    

            /* Unprotect the globa data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }
    }    
    /* Switch back to user mode */
    NU_USER_MODE();

    return ret_status;

} /* DHCPS_Delete_Broadcast_Address */

/****************************************************************************
* FUNCTION                                                                   
*                                                                            
*    DHCPS_Get_Broadcast_Address                                                       
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*    Returns the broadcast address from the specified configuration control 
*    block or specific binding.  The returning value will be the total number
*    of bytes that were written into the provided buffer.
*                                                                        
* INPUTS                                                                     
*                                                                            
*    DHCPS_CONFIG_CB *
*    struct id_struct *
*    UINT8 *
*    INT
*                                                                            
* OUTPUTS                                                                    
*                                                                            
*    INT
*                                                                            
******************************************************************************/
INT DHCPS_Get_Broadcast_Address (const DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                    UINT8 *broadcast_buffer, INT buffer_size)
{
    UINT8                       *option_ptr,
                                option_data_len;
    INT                         bytes_written;

    NU_SUPERV_USER_VARIABLES 

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    
    /* First, ensure that the buffer that has been passed in is large enough to accept at least one
        entry. */
    if ((buffer_size < IP_ADDR_LEN) || (broadcast_buffer == NU_NULL))
    {
        bytes_written = NU_INVALID_PARM;
    }

    /* Check to see if the broadcast address entry is destined for a static IP entry. */
    else if (client_ip_addr != NU_NULL)
    {
        /* Search through the static IP entries to find the broadcast address entry. */
        option_ptr = DHCPS_Get_Option_From_Memory_Block(config_cb, client_ip_addr, DHCP_BROADCAST_ADDR);

        if (option_ptr == NU_NULL)
        {
            /* An error occurred during the search for the broadcast address for the particular 
                IP address. */
            bytes_written = NU_INVALID_PARM;
        }    
        else
        {
            /* The broadcast address option has been found for the desired IP address. */
            
            /* Increment the option pointer to point at the length of the option. */
            option_data_len = *(option_ptr + 1);

            /* Ensure that the buffer is large enough to hold the option data. */
            if (buffer_size < option_data_len)
            {
                bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
            }    

            else
            {
                /* Increment the option pointer to point at the data that will be copied. */
                option_ptr += 2;

                /* Copy the option into the buffer that has been provided by the application. */
                memcpy(broadcast_buffer, option_ptr, option_data_len);

                /* Save the number of bytes that have been written into the buffer for returning to
                    the application. */
                bytes_written = option_data_len;
            }    
        }    
    }    

    else 
    {
        /* Ensure that the buffer is large enough to hold the option data. */
        if (buffer_size < IP_ADDR_LEN)
        {
            bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
        }    

        else
        { 
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the broadcast address option entry into the buffer provided by the application. */
            memcpy(broadcast_buffer, config_cb->broadcast_addr, IP_ADDR_LEN);

            /* Unprotect the globa data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            /* Set the return variable to the number of bytes that were written into the buffer. */
            bytes_written = IP_ADDR_LEN;      
        }    
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (bytes_written); 

} /* DHCPS_Get_Broadcast_Address */

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Add_Entry_To_Array                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This function handles the adding of new entries to options that are in an
*     array format, such as the DNS server IP's or router IP's.
*
* INPUTS                                                               
*                                                                      
*     struct id_struct *
*     struct id_struct *
*     UINT8
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Add_Entry_To_Array (struct id_struct *first_array_entry, const struct id_struct *entry_to_add, 
                                 UINT8 max_array_entries)
{
    STATUS              ret_status = NU_INVALID_PARM;
    UINT8               i, j, k,
                        entry_found = NU_FALSE;
    struct id_struct    *first_array;

    /* Save off a pointer to the position of the first array entry. */
    first_array = first_array_entry;
    
    /* Loop through the array and ensure that the entry that we are adding is not already
        present. */
    for (i = 0; i < max_array_entries; i++)
    {
        k = 0;

        for (j = 0; j < IP_ADDR_LEN; j++)
        {
            if (first_array->is_ip_addrs[j] == entry_to_add->is_ip_addrs[j])
                k++;
            else
            {
                /* This is not a match, increment the array pointer to the next entry. */
                first_array++;
                break;
            }    
        }

        if (k == IP_ADDR_LEN)
        {
            entry_found = NU_TRUE;
            ret_status = DHCPSERR_OPTION_ALREADY_PRESENT;
        }    
    }    

    if (!entry_found)
    {
        /* Loop through all the entries of the array to find an empty entry. */
        for (i = 0; i < max_array_entries; i++)
        {
            if (first_array_entry->is_ip_addrs[0] == NU_NULL)
            {
                /* This array entry is empty.  We can add the new entry here. */
                memcpy(first_array_entry->is_ip_addrs, entry_to_add->is_ip_addrs, IP_ADDR_LEN);

                /* Entry has been successfully added. */
                ret_status = NU_SUCCESS;

                break;
            }
            else
                /* Increment to the next entry in the array. */
                first_array_entry++;
        }

        if (ret_status != NU_SUCCESS)
            ret_status = DHCPSERR_ARRAY_ENTRIES_FULL;
    }
    return(ret_status);
}

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Remove_Entry_From_Array                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Remove the spefied entry from the option array.  This function is used to 
*     remove entries from such options as the DNS servers and router.
*
* INPUTS                                                               
*                                                                      
*     struct id_struct *
*     struct id_struct *
*     UINT8
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Remove_Entry_From_Array (struct id_struct *first_array_entry, const  struct id_struct *entry_to_remove, 
                                    UINT8 max_array_entries)
{
    STATUS      ret_status = DHCPSERR_PARAMETER_NOT_FOUND;
    UINT8       i;

    /* Loop through all the entries of the array to find the desired entry. */
    for (i = 0; i < max_array_entries; i++)
    {
        if (DHCPSDB_IP_Compare(first_array_entry->is_ip_addrs, entry_to_remove->is_ip_addrs) == NU_SUCCESS)
        {
            /* This is the array entry that is to be removed. Clear the entry completely. */
            memset(first_array_entry->is_ip_addrs, IP_ADDR_ANY, IP_ADDR_LEN);

            /* Entry has been successfully added. */
            ret_status = NU_SUCCESS;

            break;
        }
        else
            /* Increment to the next entry in the array. */
            first_array_entry++;
    }

    return(ret_status);
}


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Enable_Configuration                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Mark the configuration control block to enable.  This signifies that the 
*     control block has been configured and ready to server requesting clients.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Enable_Configuration (DHCPS_CONFIG_CB *config_cb)
{
    DHCPS_CONFIG_CB         *temp_config;
    STATUS                  ret_status = NU_SUCCESS;
    UINT16                  flag_stat;
                            
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that we have been passed a valid control block pointer. */
    if (config_cb != NU_NULL)
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Search the control block link list for a match of the configuration that is to
            be enabled. */ 
        for(temp_config = DHCPS_Config_List.dhcp_config_head;
            temp_config;
            temp_config = temp_config->dhcps_config_next)
        {
            if (temp_config == config_cb)
            {
                /* We have found our match.  Break. */
                break;
            }    
        }        

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        if (temp_config != NU_NULL)    
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Once a configuration has been initialized with the desired values, it can be enabled to 
                to start serving clients. */
            flag_stat = config_cb->flags;

            /* Set the configuration enabled bit of the flags variable. */
            flag_stat = flag_stat | CONFIGURATION_ENABLED;

            /* Save the new flag settings to the configuration control block. */
            config_cb->flags = flag_stat;

            /* The configuration has been successfully enabled. */  
            ret_status = NU_SUCCESS;

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }

        else
        {
            /* The specified configuration control block could not be found in the link list. */
            ret_status = DHCPSERR_CONFIGURATION_NOT_FOUND;
        }    
    }

    else
    {
        /* An invalid control block pointer was passed in. */
        ret_status = NU_INVALID_PARM;
    }    
    /* Switch to user mode */
    NU_USER_MODE();

    return(ret_status);
}   /* DHCPS_Enable_Configuration */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Disable_Configuration                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This will disable a configuration control block and make it unavailable to
*     provide bindings and parameters to requesting clients.  This function should
*     be called first whenever option parameters are being changed for a control
*     block.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Disable_Configuration (DHCPS_CONFIG_CB *config_cb)
{
    DHCPS_CONFIG_CB         *temp_config;
    STATUS                  ret_status = NU_SUCCESS;
    UINT16                  flag_stat;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that we have been passed a valid control block pointer. */
    if (config_cb != NU_NULL)
    {
        /* A configuration should be disabled if any changes are being made to the 
            parameters of the control block.  */

        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Search the control block link list for a match of the configuration that is to
            be enabled. */ 
        for(temp_config = DHCPS_Config_List.dhcp_config_head;
            temp_config;
            temp_config = temp_config->dhcps_config_next)
        {
            if (temp_config == config_cb)
            {
                /* We have found our match.  Break. */
                break;
            }    
        }        

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        if (temp_config != NU_NULL)    
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Get the current settings of the control block flags. */
            flag_stat = config_cb->flags;

            /* Clear the configuration enabled bit */
            flag_stat = flag_stat & ~CONFIGURATION_ENABLED;

            /* Save the new flag settings to the configuration control block. */
            config_cb->flags = flag_stat;

            /* The configuration has been successfully disabled. */  
            ret_status = NU_SUCCESS;

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }

        else
        {
            /* The specified configuration control block could not be found in the link list. */
            ret_status = DHCPSERR_CONFIGURATION_NOT_FOUND;
        }    
    }

    else
    {
        /* An invalid control block pointer was passed in. */
        ret_status = NU_INVALID_PARM;
    }    
    /* Switch back to user mode */
    NU_USER_MODE();

    return(ret_status);
}   /* DHCPS_Disable_Configuration */



/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Find_Configuration_Control_Block                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This function is used by the main server function to choose the correct 
*     control block that will be used to server the requesting client.
*
* INPUTS                                                               
*                                                                      
*     CHAR *
*     struct id_struct *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     DHCPS_CONFIG_CB *
*                                                                      
**********************************************************************************/
DHCPS_CONFIG_CB *DHCPS_Find_Configuration_Control_Block (const CHAR *device_name, 
                                                         const  struct id_struct *recv_if_addr)
{
    DHCPS_CONFIG_CB     *ret_cb = NU_NULL,
                        *temp_config;
    struct id_struct    subnet_addr,
                        subnet_mask;
    STATUS              status;
    UINT16              flags;
    CHAR                temp_dev_name[DEV_NAME_LENGTH];
    UINT8               temp_ip_addr[IP_ADDR_LEN];

    /* We must walk the configuration control block list to find a match of the specified 
        device name and the subnet address. */
    for (temp_config = DHCPS_Config_List.dhcp_config_head;
         temp_config;
         temp_config = temp_config->dhcps_config_next)
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Save the temp_config device name to a local variable. */
        strcpy(temp_dev_name, temp_config->device_name);

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        /* Check to see if the network interface name matches the specified device. */
        if (strcmp(device_name, temp_dev_name) == NU_SUCCESS)
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Save off the subnet mask from the temp config control block to a local 
                variable.  If the subnet mask of the config control block is null, then
                use the global subnet mask. */
            if (temp_config->subnet_mask_addr[0] == NU_NULL)
            {
                /* Copy the global mask into the local variable. */
                memcpy(subnet_mask.is_ip_addrs, DHCPS_Config_List.dhcp_config_head->subnet_mask_addr,
                        IP_ADDR_LEN);
            }    
            else
            {
                /* Copy the mask from the temp configuration. */
                memcpy(subnet_mask.is_ip_addrs, temp_config->subnet_mask_addr, IP_ADDR_LEN);
            }    

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            /* We must now calculate the subnet address of the interface of the address
                that the message was received.  The subnet mask of the control block will
                be used for the calculation. */
            status = DHCPSDB_Calculate_Subnet_Number(recv_if_addr, &subnet_mask,
                                                        &subnet_addr);

            if (status == NU_SUCCESS)
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Save the temp_config subnet address to a local variable. */
                memcpy(temp_ip_addr, temp_config->subnet_addr, IP_ADDR_LEN);

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);

                /* The network interfaces match, now see if the subnet addresses match. */
                if (DHCPSDB_IP_Compare(subnet_addr.is_ip_addrs, temp_ip_addr) == NU_SUCCESS)
                {
                    /* Protect the global data structures. */
                    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                    /* One last test needs to be made.  We must ensure that the configuration
                        control block has been enabled.  If not, then this control block can
                        not be used. */
                    flags = temp_config->flags;

                    /* Unprotect the global data structures. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);

                    if ((flags & CONFIGURATION_ENABLED) != NU_NULL)
                    {
                        /* We have found a match.  Set the return control block pointer
                            to the temp pointer. */
                        ret_cb = temp_config;
                        break;
                    }    
                }               
            }    
        }    
    }
    return(ret_cb);     
}


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Get_Option_From_Configuration                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Copy the requested option values associated with the binding into the provided 
*     buffer.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_BINDING *
*     UINT8
*     UINT8
*     UINT8*
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     UINT8
*                                                                      
**********************************************************************************/
UINT8 DHCPS_Get_Option_From_Configuration(DHCPS_BINDING *binding, UINT8 opcode, 
                                            UINT8 msg_type, UINT8 *option_buff)                                            
{

#if ((CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_ROUTER == NU_TRUE) || (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DNS_SERVER == NU_TRUE))

    UINT8               j,
                        *length_location,
                        parameter_found = NU_FALSE,
                        number_of_addr = 0;

#endif

    UINT8               i, option_data_length = 0;
    DHCPS_CONFIG_CB     *config_cb;
    UINT16              flags;


    /* If this is a DHCPINFORM message, the passed in binding pointer is actually a pointer
        to the configuration control block.  This is done due to the fact that no binding
        structure is choosen when replying to a DHCPINFORM message.  Set the config_cb to 
        this control block. */
    if (msg_type == DHCPINFORM || msg_type == DHCPNAK)
    {
        config_cb = (DHCPS_CONFIG_CB *)binding;
    }    

    else
    {
        /* Get a pointer to the configuration control block that is associated with the binding
            that is being presented to the requesting client. */
        config_cb = binding->config;
    } 

    /* Protect the global data structures. */
    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

    /* Search for the requested option code for a return value.  If the option is
        supported by the specified configuration control block, copy it into the 
        option buffer. */
    switch( opcode )
    {
        case DHCP_NETMASK:
            *option_buff++ = DHCP_NETMASK;
            *option_buff++ = IP_ADDR_LEN;

            /* If the subnet mask for the control block has not been set, use the 
                global one. */
            if (config_cb->subnet_mask_addr[0] == NU_NULL)
            {
                /* Use the global control block subnet mask. */
                config_cb = DHCPS_Config_List.dhcp_config_head;
            }
            /* Copy the subnet mask. */
            for (i = 0; i < IP_ADDR_LEN; i++)
               *option_buff++ = config_cb->subnet_mask_addr[i];

            option_data_length += 6;
            break;

#ifdef DHCPS_UNSUPPPORTED_OPTION                
        case DHCP_TIME_OFFSET:
            break;
#endif
#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_ROUTER == NU_TRUE)
        case DHCP_ROUTE:
            *option_buff++ = DHCP_ROUTE;

            /* Save off the location to place the length of the router addresses */
            length_location = option_buff;
            option_buff++;

            /* Determine if the control block has had any routers added.  If not,
                we will use the router IP addresses from the global control block. */
            for (i = 0; i < DHCPS_MAX_ROUTERS; i++)
            {
                if (config_cb->router[i].is_ip_addrs[0] == NU_NULL)
                {
                    /* This entry is empty. Skip to the next entry. */
                    continue;
                }

                else
                {
                    /* There is a router entry in this control block.  We can use this one. */
                    parameter_found = NU_TRUE;
                    break;
                }    
            }

            if (parameter_found == NU_FALSE)
            {
                /* Since no router entries were found in the control block, the routers from the
                    global control block will be used. */
                config_cb = DHCPS_Config_List.dhcp_config_head;
            }    

            /* Loop through the router array and copy entries into the buffer. */
            for (i = 0; i < DHCPS_MAX_ROUTERS; i++)
            {
                /* Check to see if there is a router address in this array entry. */
                if (config_cb->router[i].is_ip_addrs[0] == NU_NULL)
                {
                    /* This array entry is empty.  Skip to the next entry. */
                    continue;
                }    
                else
                {
                    /* Copy the router IP address into the buffer. */    
                    for (j = 0; j < IP_ADDR_LEN; j++)
                    {
                        *option_buff++ = config_cb->router[i].is_ip_addrs[j];
                    }    

                    /* Increment the option_data_length. */
                    number_of_addr++;
                }    
            }         

            *length_location = (UINT8)(IP_ADDR_LEN * number_of_addr);

            option_data_length += (UINT8)(0x02 + (IP_ADDR_LEN * number_of_addr));
            break;
#endif
            
#ifdef DHCPS_UNSUPPPORTED_OPTION                                
        case DHCP_TIME:
        case DHCP_NAME_SERVER:
            break;
#endif                

#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DNS_SERVER == NU_TRUE)
        case DHCP_DNS:
            *option_buff++ = DHCP_DNS;

            /* Save off the location to place the length of the DNS server addresses */
            length_location = option_buff;
            option_buff++;

            /* Determine if the control block has had any DNS servers added.  If not,
                we will use the DNS IP addresses from the global control block. */
            for (i = 0; i < DHCPS_MAX_DNS_SERVERS; i++)
            {
                if (config_cb->dns_server[i].is_ip_addrs[0] == NU_NULL)
                {
                    /* This entry is empty. Skip to the next entry. */
                    continue;
                }

                else
                {
                    /* There is a DNS server entry in this control block.  We can use this one. */
                    parameter_found = NU_TRUE;
                    break;
                }    
            }

            if (parameter_found == NU_FALSE)
            {
                /* Since no DNS server entries were found in the control block, the DNS from the
                    global control block will be used. */
                config_cb = DHCPS_Config_List.dhcp_config_head;
            }    

            /* Loop through the DNS server array and copy entries into the buffer. */
            for (i = 0; i < DHCPS_MAX_DNS_SERVERS; i++)
            {
                /* Check to see if there is a router address in this array entry. */
                if (config_cb->dns_server[i].is_ip_addrs[0] == NU_NULL)
                {
                    /* This array entry is empty.  Skip to the next entry. */
                    continue;
                }    
                else
                {
                    /* Copy the DNS server IP address into the buffer. */    
                    for (j = 0; j < IP_ADDR_LEN; j++)
                    {
                        *option_buff++ = config_cb->dns_server[i].is_ip_addrs[j];
                    }    

                    /* Increment the option_data_length. */
                    number_of_addr++;
                }    
            }         
            *length_location = (UINT8)(IP_ADDR_LEN * number_of_addr);

            option_data_length += (UINT8)(0x02 + (IP_ADDR_LEN * number_of_addr));
            break;
#endif

#ifdef DHCPS_UNSUPPPORTED_OPTION                
        case DHCP_LOG_SERVER:
        case DHCP_COOKIE_SERVER:
        case DHCP_LPR_SERVER:
        case DHCP_IMPRESS_SERVER:
        case DHCP_RESOURCE_SERVER:
        case DHCP_HOSTNAME:
            break;
#endif

#ifdef DHCPS_UNSUPPPORTED_OPTION                
        case DHCP_BOOT_FILE_SIZE:
        case DHCP_MERIT_DUMP_FILE:
            break;
#endif                

#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DOMAIN_NAME == NU_TRUE)
        case DHCP_DOMAIN_NAME:
            *option_buff++ = DHCP_DOMAIN_NAME;

            /* Determine if the control block domain name has been set. If not, use the 
                global one. */
            if (config_cb->dns_domain_name_length == NU_NULL)
            {
                /* We will use the global control block. */
                config_cb = DHCPS_Config_List.dhcp_config_head;
            }

            *option_buff++ = (UINT8)strlen(config_cb->dns_domain_name);
            strcpy((CHAR *)option_buff, config_cb->dns_domain_name);
            option_buff += (UINT8)strlen(config_cb->dns_domain_name);  
            option_data_length += (UINT8)(2 + strlen(config_cb->dns_domain_name));
            break;
#endif

#ifdef DHCPS_UNSUPPPORTED_OPTION                
        case DHCP_SWAP_SERVER:
        case DHCP_ROOT_PATH:
        case DHCP_EXTENSIONS_PATH:
            break;

        /* IP Layer Parameters per Host. */
        case DHCP_IP_FORWARDING:
        case DHCP_NL_SOURCE_ROUTING:
        case DHCP_POLICY_FILTER:
        case DHCP_MAX_DATAGRAM_SIZE:
        case DHCP_IP_TIME_TO_LIVE:
        case DHCP_MTU_AGING_TIMEOUT:
        case DHCP_MTU_PLATEAU_TABLE:
            break;

        /* IP Layer Parameters per Interface. */
        case DHCP_INTERFACE_MTU:
        case DHCP_ALL_SUBNETS:
            break;
#endif                

        case DHCP_BROADCAST_ADDR:
            *option_buff++ = DHCP_BROADCAST_ADDR;
            *option_buff++ = IP_ADDR_LEN;

            /* Determine if the broadcast address for the control block has been set. */
            if (config_cb->broadcast_addr[0] == NU_NULL)
            {
                /* The global control block broadcast address will be used. */
                config_cb = DHCPS_Config_List.dhcp_config_head;
            }    
            for (i = 0; i < IP_ADDR_LEN; i++)
                *option_buff++ = config_cb->broadcast_addr[i];

            option_data_length += 6;
            break;

#ifdef DHCPS_UNSUPPPORTED_OPTION                                
        case DHCP_MASK_DISCOVERY:
        case DHCP_MASK_SUPPLIER:
        case DHCP_ROUTER_DISCOVERY:
        case DHCP_ROUTER_SOLICI_ADDR:
        case DHCP_STATIC_ROUTE:
            break;
#endif

        /* Link Layer Parameters per Interface. */
        case DHCP_TRAILER_ENCAP:
            *option_buff++ = DHCP_TRAILER_ENCAP;
            *option_buff++ = 1;

            /* Check the status flag variable to see if trailer encapsulation should be used. */
            flags = config_cb->flags;

            if ((flags & TRAILER_ENCAPSULATION) != 0)
            {
                /* Set trailer encapsulation to true. */
                *option_buff++ = 1;
            }    

            else
            {
                /* Set trailer encapsulation to false. */
                *option_buff++ = 0;
            }
            option_data_length += 3;
            break;


        case DHCP_ARP_CACHE_TIMEOUT:
            *option_buff++ = DHCP_ARP_CACHE_TIMEOUT;
            *option_buff++ = 4;

            /* If the ARP cache timeout has not been set in the subnet's control block, use the
                global value. */
            if (config_cb->arp_cache_to == NU_NULL)
            {
                /* We will use the global value. */
                PUT32(option_buff, 0, DHCPS_Config_List.dhcp_config_head->arp_cache_to);
            }    
            else
            {
                /* Use the subnet's ARP cache timeout value. */
                PUT32(option_buff, 0, config_cb->arp_cache_to);
            }    
            option_buff += 4;
            option_data_length += 6;

            break;

        case DHCP_ETHERNET_ENCAP:
            *option_buff++ = DHCP_ETHERNET_ENCAP;
            *option_buff++ = 1;

            /* Check the status flag variable to see if trailer encapsulation should be used. */
            flags = config_cb->flags;

            if ((flags & ETHERNET_IEEE_802_ENCAPSULATION) != 0)
            {
                /* Set ethernet encapsulation to true. */
                *option_buff++ = 1;
            }    

            else
            {
                /* Set ethernet encapsulation to false. */
                *option_buff++ = 0;
            }    
            option_data_length += 3;
            break;


        /* TCP Parameters. */
        case DHCP_TCP_DEFAULT_TTL:
            *option_buff++ = DHCP_TCP_DEFAULT_TTL;
            *option_buff++ =  1;

            /* Determine if the TCP TTL for the control block has been set. */
            if (config_cb->default_tcp_ttl == NU_NULL)
            {
                /* The global control block TTL will be used. */
                config_cb = DHCPS_Config_List.dhcp_config_head;
            }    
            PUT8(option_buff, 0, config_cb->default_tcp_ttl);

            option_data_length += 3;    
            break;

        case DHCP_TCP_KEEPALIVE_TIME:
            *option_buff++ = DHCP_TCP_KEEPALIVE_TIME;
            *option_buff++ =  1;

            /* Determine if the keepalive time for the control block has been set. */
            if (config_cb->tcp_keepalive_interval == NU_NULL)
            {
                /* The global control block broadcast address will be used. */
                config_cb = DHCPS_Config_List.dhcp_config_head;
            }    
    
            PUT32(option_buff, 0, config_cb->tcp_keepalive_interval);

            option_data_length += 3;    
            break;            
            
        case DHCP_TCP_KEEPALIVE_GARB:
            *option_buff++ = DHCP_TCP_KEEPALIVE_GARB;
            *option_buff++ = 1;

            /* Check the status flag variable to see if TCP keepalive garbage should be used. */
            flags = config_cb->flags;

            if ((flags & TCP_KEEPALIVE_GARBAGE) != 0)
            {
                /* Set keepalive garbage to true. */
                *option_buff++ = 1;
            }    

            else
            {
                /* Set keepalive garbage to false. */
                *option_buff++ = 0;
            }
            option_data_length += 3;
            break;            

        /* Application and Service Parameters. */
#ifdef DHCPS_UNSUPPPORTED_OPTION                                
        case DHCP_NIS_DOMAIN:
        case DHCP_NIS:
        case DHCP_NTP_SERVERS:
        case DHCP_VENDOR_SPECIFIC:
        case DHCP_NetBIOS_NAME_SER:
        case DHCP_NetBIOS_DATA_SER:
        case DHCP_NetBIOS_NODE_TYPE:
        case DHCP_NetBIOS_SCOPE:
        case DHCP_X11_FONT_SERVER:
        case DHCP_X11_DISPLAY_MGR:
        case DHCP_NIS_PLUS_DOMAIN:
        case DHCP_NIS_PLUS_SERVERS:
        case DHCP_MOBILE_IP_HOME:
        case DHCP_SMTP_SERVER:
        case DHCP_POP3_SERVER:
        case DHCP_NNTP_SERVER:
        case DHCP_WWW_SERVER:
        case DHCP_FINGER_SERVER:
        case DHCP_IRC_SERVER:
        case DHCP_STREETTALK_SERVER:
        case DHCP_STDA_SERVER:
            break;
#endif
        /* DHCP Extensions */
        case DHCP_REQUEST_IP:
            break;


        case DHCP_IP_LEASE_TIME:
            *option_buff++ = DHCP_IP_LEASE_TIME;
            *option_buff++ = 4;

            /* Determine if the lease length has been stored in the binding. */
            if (binding->lease_length != 0)
            {
                PUT32(option_buff, 0, binding->lease_length);
                option_buff += 4;
                option_data_length += 6;
            }

            else
            {
                /* Determine if the broadcast address for the control block has been set. */
                if (config_cb->default_lease_length == NU_NULL)
                {
                    /* The global control block broadcast address will be used. */
                    config_cb = DHCPS_Config_List.dhcp_config_head;
                }                
                PUT32(option_buff, 0, config_cb->default_lease_length);
                option_buff += 4;
                option_data_length += 6;
            }
            break;

        case DHCP_OVERLOAD:
            break;

        case DHCPS_MSG_TYPE:
            *option_buff++ = DHCPS_MSG_TYPE;
            *option_buff++ = 1;                             
            *option_buff++ = msg_type;
            option_data_length += 3;
            break;

        case DHCPS_SERVER_ID:
            *option_buff++ = DHCPS_SERVER_ID;
            *option_buff++ = IP_ADDR_LEN;

            /* Determine if the broadcast address for the control block has been set. */
            if (config_cb->server_ip_addr[0] == NU_NULL)
            {
                /* The global control block broadcast address will be used. */
                config_cb = DHCPS_Config_List.dhcp_config_head;
            }    

            for (i = 0; i < IP_ADDR_LEN; i++)
                *option_buff++ = config_cb->server_ip_addr[i];

            option_data_length += 6;
            break;
            
#ifdef DHCPS_UNSUPPPORTED_OPTION                
        case DHCP_REQUEST_LIST:
        case DHCPS_MESSAGE:
        case DHCPS_MAX_MSG_SIZE:
            break;
#endif                
        case DHCPS_RENEWAL_T1:
            *option_buff++ = DHCPS_RENEWAL_T1;
            *option_buff++ = 4;

            /* Determine if the broadcast address for the control block has been set. */
            if (config_cb->dhcp_renew_t1 == NU_NULL)
            {
                /* The global control block broadcast address will be used. */
                config_cb = DHCPS_Config_List.dhcp_config_head;
            }                
            PUT32(option_buff, 0, config_cb->dhcp_renew_t1);
            option_buff += 4;
            option_data_length += 6;
            break;

        case DHCPS_REBIND_T2:
            *option_buff++ = DHCPS_REBIND_T2;
            *option_buff++ = 4;

            /* Determine if the broadcast address for the control block has been set. */
            if (config_cb->dhcp_rebind_t2 == NU_NULL)
            {
                /* The global control block broadcast address will be used. */
                config_cb = DHCPS_Config_List.dhcp_config_head;
            }                
            PUT32(option_buff, 0, config_cb->dhcp_rebind_t2);
            option_buff += 4;
            option_data_length += 6;
            break;

#ifdef DHCPS_UNSUPPPORTED_OPTION                                
        case DHCP_VENDOR_CLASS_ID:
        case DHCP_CLIENT_CLASS_ID:
            break;
#endif
        default:
            break;
    }

    /* Unprotect the global data structures. */
    NU_Release_Semaphore(&DHCPS_Semaphore);

    /* Return the number of bytes written into the option buffer. */
    return(option_data_length);
}



/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Shutdown_Server                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This function will save off each of the configuration control blocks, bindings,
*     and option blocks and deallocate memory so that the server may be restarted.
*
* INPUTS                                                               
*                                                                      
*     None
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Shutdown_Server(VOID)
{
    STATUS              ret_status;
    DHCPS_CONFIG_CB     *config_cb,*temp_cb;
    DHCPS_BINDING       *binding,*temp_bind;
    DHCPS_OPTIONS       *options,*temp_opt;
    DHCPS_OFFERED       *offered,*temp_off;
    UINT16              flags;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();


#ifdef FILE_3_1

    ret_status = NU_Set_Default_Drive(DHCPS_Default_Drive);
    if(ret_status != NU_SUCCESS)
    {
       NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__); 
       NU_USER_MODE();
       return (ret_status);
    }

    /*  Set Current Directory to "/" */
    ret_status = NU_Set_Current_Dir("\\");
    if (ret_status != NU_SUCCESS)
    {
       NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__); 
       NU_USER_MODE();
       return (ret_status);
    }

#endif 

    if (ret_status == NU_SUCCESS)
    {
        /* Close the socket that the DHCP messages are received. */
        ret_status = NU_Close_Socket(Socket_Descriptor);

        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Loop through the control block list and disable each of the control blocks. */
        for (config_cb = DHCPS_Config_List.dhcp_config_head;
             config_cb;
             config_cb = config_cb->dhcps_config_next)
        {
            /* Get the status flags for the configuration control block. */
            flags = config_cb->flags;

            /* Disable the control block. */
            flags = flags & ~CONFIGURATION_ENABLED;

            /* Store the status flags back to the control block. */
            config_cb->flags = flags;
        }         
            
        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        /* Save off all of the configuration control blocks to a file. */
        ret_status = DHCPS_Save_Config_Struct_To_File();

        if (ret_status == NU_SUCCESS)
        {
            /* Save off all of the binding structures to a file. */
            ret_status = DHCPS_Save_Binding_To_File();

            if (ret_status == NU_SUCCESS)
            {
                /* Save off all of the options blocks to a file. */
                ret_status = DHCPS_Save_Options_Block_To_File();

                if (ret_status == NU_SUCCESS)
                {
                    /* Deallocate the memory used in the control blocks. */
                    for (config_cb = DHCPS_Config_List.dhcp_config_head; config_cb ;)
                    {
                        for (options = config_cb->options_buff.dhcps_options_head; options ;)
                        {
                            /* Deallocate the memory for the option block. */
                            if (options->options_ptr != NU_NULL)
                                ret_status = NU_Deallocate_Memory(options->options_ptr);

                            if (ret_status != NU_SUCCESS)
                                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);

                            /* Store current options pointer in a temporary pointer */
                            temp_opt = options;

                            /* Get next option pointer */
                            options = options->dhcps_options_next;

                            /* Deallocate the memory for the options structure. */
                            ret_status = NU_Deallocate_Memory(temp_opt);

                            if (ret_status != NU_SUCCESS)
                                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);                            
                        }

                        for (binding = config_cb->ip_binding_list.dhcp_bind_head; binding ;)
                        {
                            /* Store current binding pointer in a temporary pointer */
                            temp_bind = binding;

                            /* Get the next binding */
                            binding = binding->dhcp_bind_next;

                            /* Deallocate the memory for the binding structure. */
                            ret_status = NU_Deallocate_Memory(temp_bind);

                            if (ret_status != NU_SUCCESS)
                                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);                            
                        } 

                        for (offered = config_cb->ip_offer_list.dhcp_offer_head; offered ;)
                        {
                            /* Store current offered pointer in a temporary pointer */
                            temp_off = offered;

                            /* Get the next offered structure */
                            offered = offered->dhcp_offer_next;

                            /* Deallocate the memory for the offered IP address list. */
                            ret_status = NU_Deallocate_Memory(temp_off);

                            if (ret_status != NU_SUCCESS)
                                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);                            
                        } 

                        /* Store current config_cb in a temporary pointer */
                        temp_cb = config_cb;

                        /* Get next control block */
                        config_cb = config_cb->dhcps_config_next;

                        /* Finally, deallocate the control block itself. */
                        ret_status = NU_Deallocate_Memory(temp_cb);

                        if (ret_status != NU_SUCCESS)
                            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);                        
                    }         

                    /* Terminate the server lease timer task. */
                    ret_status = NU_Terminate_Task(&dhcpserv_lease_timer_task_cb);

                    if (ret_status != NU_SUCCESS)
                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);                    
                    /* Terminate the DHCP server task. */
                    ret_status = NU_Terminate_Task(&dhcp_server_task_cb);

                    if (ret_status != NU_SUCCESS)
                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);                                        

                    /* Deallocate the memory that the server uses for received messages. */
                    if (DHCPS_Message != NU_NULL)
                    {
                        ret_status = NU_Deallocate_Memory(DHCPS_Message);

                        if (ret_status != NU_SUCCESS)
                            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);                                        
                    }

                    /* Delete the server lease timer task. */
                    ret_status = NU_Delete_Task(&dhcpserv_lease_timer_task_cb);

                    if (ret_status != NU_SUCCESS)
                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);                                        

                    /* Deallocate memory for the lease timer task. */
                    ret_status = NU_Deallocate_Memory(dhcpserv_lease_mem);

                    if (ret_status != NU_SUCCESS)
                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);                                        

                    /* Delete the DHCP server task. */
                    ret_status = NU_Delete_Task(&dhcp_server_task_cb);

                    if (ret_status != NU_SUCCESS)
                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);                                        

                    /* Deallocate memory for the DHCP server task. */
                    ret_status = NU_Deallocate_Memory(dhcp_server_mem);

                    if (ret_status != NU_SUCCESS)
                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);

                    /* Delete the DHCPS_Semaphore Semaphore */
                    ret_status = NU_Delete_Semaphore(&DHCPS_Semaphore);

                    if (ret_status != NU_SUCCESS)
                        NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);

                    /* The server has been successfully shutdown. */
                    ret_status = NU_SUCCESS;

                }    
            }
        } 
    }

    if (ret_status == NU_SUCCESS)
    {
        /* Switch back to user mode */
        NU_USER_MODE();

        return (ret_status);
    }

    else
    {
        /* Switch back to user mode */
        NU_USER_MODE();

        return(DHCPSERR_SERVER_NOT_SHUTDOWN);
    }    
}
/******************************************************************************************
*
* FUNCTION
*
*      DHCPS_Server_Reset
*
* DESCRIPTION
*
*      Disables and deletes all DHCP server configurations that were done during runlevel
*      initialization.
*
* INPUTS
*
*      None
*
* OUTPUTS
*
*      STATUS
*
******************************************************************************************/
STATUS DHCPS_Server_Reset(VOID)
{
    STATUS                  status = NU_SUCCESS;

    if (DHCPS_Initialized == NU_TRUE)
    {
        status = DHCPS_Delete_Config_Name("SUBNET1");

        if (status == NU_SUCCESS)
        {
            status = DHCPS_Delete_Config_Name("GLOBAL");
        }
    }
    else
    {
        status = DHCPSERR_NOT_INITIALIZED;
    }

    return status;
}
/******************************************************************************************
*
* FUNCTION
*
*      DHCPS_Delete_Config_Name
*
* DESCRIPTION
*
*      Disables and deletes the config entry whose name is passed as a parameter.
*
* INPUTS
*
*      CHAR*:        Character string containing name of the config entry to delete.
*
* OUTPUTS
*
*      STATUS
*
******************************************************************************************/
STATUS DHCPS_Delete_Config_Name(CHAR* config_name)
{
    STATUS                  status = NU_SUCCESS;
    DHCPS_CONFIG_CB         *temp_config;

    /* Find and remove config entry.*/
    for (temp_config = DHCPS_Config_List.dhcp_config_head;
         temp_config;
         temp_config = temp_config->dhcps_config_next)
    {
        /* Compare the config entry name. */
        if ((strcmp(temp_config->config_entry_name, config_name) == NU_SUCCESS))
        {
            /* Disable the configuration */
            status = DHCPS_Disable_Configuration(temp_config);

            if(status == NU_SUCCESS)
            {
                status = DHCPS_Delete_Config_Entry(temp_config);
                break;
            }
        }
    }

    return status;
}
#if (NET_VERSION_COMP < NET_4_5)
/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       DHCPS_Check_Duetime                
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Check to see if the due time has expired.  The function will take
*       into accout if the clock has wrapped.
*                                                                       
*   INPUTS                                                                
*
*       UINT32
*                                                                      
*   OUTPUTS                                                               
*
*       UINT32                                                               
*                                                                       
*************************************************************************/
UINT32 DHCPS_Check_Duetime (UINT32 lease_expire_time)
{
    UINT32  current_time;
    UINT32  wait_time;

    current_time = NU_Retrieve_Clock();

    /* Account for the wrapping of the clock. */
    if (lease_expire_time > current_time)
        wait_time = (lease_expire_time - current_time);

    else
    {
        /* If the lease_expire_time and current_time differ by an enormous amount, it is
            safe to assume that the clock has wrapped. */
        if((current_time - lease_expire_time) > 0x80000000UL)
            wait_time = ((0xFFFFFFFFUL - lease_expire_time) + (current_time + 1));
        
        else
            /* The current_time has passed the due_time.  */
            wait_time = NU_NO_SUSPEND;
    }

    return (wait_time);
}    
#endif
