/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
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
*       fileio_trace_comms.c
*
*   COMPONENT
*
*       Trace Communication
*
*   DESCRIPTION
*
*       Implement the FileIO components for trace data transmit
*
*************************************************************************/

#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "os/services/trace/comms/trace_comms.h"

#if (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == FILE_INTERFACE)
#include    "drivers/nu_drivers.h"
#include    "storage/nu_storage.h"
#include    "storage/dir_defs.h"
#include    "services/nu_trace.h"


/* Function prototypes. */
VOID File_Comms_Up_Task_Entry(UNSIGNED argc, VOID *argv);
VOID Buffer_Flush_Deamon_Entry(UNSIGNED argc, VOID *argv);
VOID Buffer_Flush_HISR_Entry(VOID);

/* Globals */
NU_TASK         Flush_Deamon;
NU_TASK         *File_Comms_Up_Task;
NU_SEMAPHORE    Flush_Semaphore;
NU_SEMAPHORE    Buffer_Mutex;
NU_HISR         Flush_Hisr;
INT             File_Handle = -1;
UINT32          File_Err_Count = 0;
CHAR            Metadata_String[256];
CHAR            Root_Dir[3];
CHAR            File_Name[32];
BOOLEAN         File_Ready = NU_FALSE;
BOOLEAN         Flush_In_Progress = NU_FALSE;

/***********************************************************************
*
*   FUNCTION
*
*       FileIO_Trace_Comms_Open
*
*   DESCRIPTION
*
*       Creates task which initializes File communication interface.
*
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status        NU_SUCCESS or an error code
*
***********************************************************************/
STATUS FileIO_Trace_Comms_Open(VOID)
{
    NU_MEMORY_POOL*     sys_pool_ptr;
    STATUS              status;


    /* Get system memory pool */
    status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

    if (status == NU_SUCCESS)
    {
        /* Create a auto-clean task to block until the networking interface
         * is up */
        status = NU_Create_Auto_Clean_Task(&File_Comms_Up_Task, "COMMS_UP",
                                           File_Comms_Up_Task_Entry, 0, NU_NULL, sys_pool_ptr,
                                           TRACE_COMMS_UP_TSK_STK_SZ, TRACE_COMMS_UP_TSK_PRIORITY, 0,
                                           NU_PREEMPT, NU_START);
    }

    if(status != NU_SUCCESS)
    {
        status = NU_TRACE_COMMS_OPEN_ERROR;
    }

    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       File_Comms_Up_Task_Entry
*
*   DESCRIPTION
*
*       Entry function for file communication initialization task. Waits
*       for storage device to become available. Once storage device is
*       available creates other objects required for I/O.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  File_Comms_Up_Task_Entry(UNSIGNED argc, VOID *argv)
{
    STATUS             status = NU_SUCCESS;
    VOID               *pointer = NU_NULL;
    NU_MEMORY_POOL     *system_pool = NU_NULL;
    MNT_LIST_S         *root_list = NU_NULL;
    MNT_LIST_S         *mount_index = NU_NULL;
    BOOLEAN            rd_found = NU_FALSE;
    CHAR               endianess[16];
    CHAR               io_name[4] ="rda";


    /* Check for storage device */
    status = NU_Storage_Device_Wait( NU_NULL, NU_SUSPEND );

    if ( status == NU_SUCCESS )
    {
        /******************************************
         * Get a list of currently mounted drives *
         ******************************************/
        status = NU_List_Mount( &root_list );

        if ( status == NU_SUCCESS )
        {

            /***********************************************************
             * For multiple drives search the ramdisk in mount list.
             * If ram disk is found use it as a storage device otherwise
             * use first drive in the list.
             **********************************************************/

            for( mount_index = root_list; mount_index != NU_NULL; mount_index = mount_index->next)
            {
                if (strncmp(mount_index->dev_name, io_name, 3) == 0)
                {
                    rd_found = NU_TRUE;
                    break;
                }
            }

            /*****************************************************
             * Create the root directory string based on the     *
             * information for the  drive in the mount list      *
             *****************************************************/

            if (rd_found)
            {
                Root_Dir[0] = mount_index->mnt_name[0];
            }
            else
            {
                Root_Dir[0] = root_list->mnt_name[0];
            }

            /* Append ":" to drive letter to create the root directory string. */
            Root_Dir[1] = ':';
            Root_Dir[2] = '\0';

            /* Free the mount list memory */
            status = NU_Free_List( (VOID **) &root_list );
        }
    }

    if (status == NU_SUCCESS)
    {
        status = NU_Create_Semaphore( &Flush_Semaphore, "FlushSema", 0, NU_FIFO );
        if ( status == NU_SUCCESS )
        {
            status = NU_Create_Semaphore( &Buffer_Mutex, "BuffMutex", 1, NU_FIFO );
            if ( status == NU_SUCCESS )
            {
                /* Get system memory pool */
                status = NU_System_Memory_Get( &system_pool, NU_NULL );
                if ( status == NU_SUCCESS )
                {
                    /* Flush buffer task. */
                    status = NU_Allocate_Memory(system_pool, &pointer, FLUSH_DEAMON_STACK_SIZE, NU_NO_SUSPEND);
                    if ( status == NU_SUCCESS )
                    {
                        status = NU_Create_Task(&Flush_Deamon, "FlushDem", Buffer_Flush_Deamon_Entry, 0, NU_NULL, pointer,
                                                FLUSH_DEAMON_STACK_SIZE, FLUSH_DEAMON_PRIORITY, FLUSH_DEAMON_TIME_SLICE,
                                                NU_PREEMPT, NU_START );
                        if ( status == NU_SUCCESS )
                        {
                            /* Flush buffer HISR. */
                            status = NU_Allocate_Memory(system_pool, &pointer, FLUSH_DEAMON_STACK_SIZE, NU_NO_SUSPEND);
                            if ( status == NU_SUCCESS)
                            {
                                status = NU_Create_HISR(&Flush_Hisr,"FlushHisr", Buffer_Flush_HISR_Entry,
                                                        FLUSH_HISR_PRIORITY, pointer, FLUSH_HISR_STACK_SIZE);
                                if ( status == NU_SUCCESS )
                                {
                                    if ( ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN )
                                    {
                                        strcpy( endianess, "big" );
                                    }
                                    else
                                    {
                                        strcpy( endianess, "little" );
                                    }

                                    sprintf( Metadata_String, "<\n\r%s=%s\n\r>\n\r", ENDIANESS, endianess );
                                    File_Ready = NU_TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       FileIO_Trace_Comms_Close
*
*   DESCRIPTION
*
*       Performs cleanup.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       Return status of close call.
*
***********************************************************************/
STATUS FileIO_Trace_Comms_Close( VOID )
{
    STATUS status;

    File_Ready = NU_FALSE;

    status = NU_Delete_HISR( &Flush_Hisr );
    status |= NU_Terminate_Task( &Flush_Deamon );
    status |= NU_Delete_Task( &Flush_Deamon );
    status |= NU_Delete_Semaphore( &Flush_Semaphore );
    status |= NU_Delete_Semaphore( &Buffer_Mutex );

    if (File_Handle >= 0)
        status |= NU_Close(File_Handle);

    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       Buffer_Flush_Deamon_Entry
*
*   DESCRIPTION
*
*       This task flushes the trace buffer to file.
*
*   INPUTS
*
*        argc        NU_NULL
*        argv        NU_NULL
*
*   OUTPUTS
*
*        None
*
***********************************************************************/
VOID Buffer_Flush_Deamon_Entry( UNSIGNED argc, VOID *argv )
{

    STATUS status = NU_SUCCESS;

#if(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE_LOGGING_WITH_FLUSH == NU_FALSE)

    UINT32 orig_trace_mask = 0;

#endif /*(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE_LOGGING_WITH_FLUSH == NU_FALSE).*/

    while ( 1 )
    {
        status = NU_Obtain_Semaphore( &Flush_Semaphore, NU_SUSPEND );
        if ( status == NU_SUCCESS )
        {

#if(FILE_ENABLE_LOGGING_WITH_FLUSH == NU_FALSE)

            orig_trace_mask = Gbl_Trace_Mask;
            NU_Trace_Disarm(NU_TRACE_ALL);

#endif /* (CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE_LOGGING_WITH_FLUSH == NU_FALSE)*/

            /* Flush contents of complete trace buffer to file.*/
            status = FileIO_Flush_Buffer_To_File();
            if ( status != NU_SUCCESS )
            {
                File_Err_Count++;
            }

            Flush_In_Progress = NU_FALSE;

#if(FILE_ENABLE_LOGGING_WITH_FLUSH == NU_FALSE)

            NU_Trace_Arm(orig_trace_mask);

#endif /* (CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE_LOGGING_WITH_FLUSH == NU_FALSE)*/
        }
    }
}


/***********************************************************************
*
*   FUNCTION
*
*       FileIO_Trace_Comms_Is_Ready
*
*   DESCRIPTION
*
*       Is File interface ready for trace communications ?
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status        NU_TRUE or NU_FALSE
*
***********************************************************************/
BOOLEAN   FileIO_Trace_Comms_Is_Ready(VOID)
{
    return (File_Ready);
}

/***********************************************************************
*
*   FUNCTION
*
*       FileIO_Start_Trace_Buffer_Flush
*
*   DESCRIPTION
*
*       Thus function is called when buffer threshold condition is met.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID FileIO_Start_Trace_Buffer_Flush(VOID)
{

    if ((!Flush_In_Progress) && (File_Ready == NU_TRUE))
    {
        Flush_In_Progress = NU_TRUE;
        NU_Activate_HISR(&Flush_Hisr);
    }

}

/***********************************************************************
*
*   FUNCTION
*
*       Buffer_Flush_HISR_Entry
*
*   DESCRIPTION
*
*      Entry function for flush buffer HISR.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID Buffer_Flush_HISR_Entry(VOID)
{
    NU_Release_Semaphore(&Flush_Semaphore);
}

/***********************************************************************
*
*   FUNCTION
*
*       FileIO_Flush_Buffer_To_File
*
*   DESCRIPTION
*
*        Flushes the contents of trace buffer to new file.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status
*
***********************************************************************/
STATUS FileIO_Flush_Buffer_To_File(VOID)
{
    STATUS        status = NU_SUCCESS;
    UINT32        size = 0;
    INT           bytes_written = 0;
    UINT32        rd_head = 0;
    UINT32        wr_head = 0;
    CHAR          time_stamp[16];
    CHAR          *p_buff = NU_NULL;
    UINT8         sec_pcluster;
    UINT16        byte_psec;
    UINT32        free_cluster;
    UINT32        total_cluster;
    UINT32        free_space;

    /*
     * Serialize access to buffer control blocks to enforce protection because
     * this function can be called from flush task and trace shell commands.
     */
    status = NU_Obtain_Semaphore( &Buffer_Mutex, NU_SUSPEND );
    if ( status == NU_SUCCESS )
    {
        /*
         * Check if free space is available in disk to create new files.
         */

        status = NU_FreeSpace(Root_Dir, &sec_pcluster, &byte_psec, &free_cluster, &total_cluster);

        if ( status == NU_SUCCESS)
        {
            /* Free space available in the file system. */
            free_space = sec_pcluster * byte_psec * free_cluster;

            /* Buffer size to flush . */
            size = Get_Used_Buffer_Space();

            if ( size > free_space)
                status = NUF_NOSPC;

            if ( status == NU_SUCCESS )
            {
                Get_Buffer_Read_Write_Head( &rd_head, &wr_head );

                if ( rd_head != wr_head )
                {
                    p_buff = Get_Trace_Buffer();

                    memset( File_Name, 0x00, sizeof(File_Name) );
                    memset( time_stamp, 0x00, sizeof(time_stamp) );

                    /*
                     * Create a new file with the time stamp as name.
                     */

                    strcpy( File_Name, Root_Dir );
                    strcat( File_Name, "\\" );
                    sprintf( time_stamp, "%llu", NU_Get_Time_Stamp() );
                    strcat( File_Name, time_stamp );
                    strcat( File_Name, ".trace" );

                    File_Handle = NU_Open( File_Name, (PO_TEXT | PO_RDWR | PO_CREAT | PO_TRUNC),
                            (PS_IWRITE | PS_IREAD) );

                    if ( File_Handle < 0 )
                    {
                        status = File_Handle;
                    }
                    else
                    {
                        /*
                         * Write Meta-data string containing target info as a string to file.
                         */
                        bytes_written = NU_Write( File_Handle, Metadata_String,
                                strlen( Metadata_String ) + 1 );

                        if ( bytes_written != (strlen( Metadata_String ) + 1) )
                        {
                            status = bytes_written;
                        }

                        if ( status == NU_SUCCESS )
                        {
                            /*
                             * Calculate amount of data that is to be read from trace buffer and
                             * subsequently write to file.There are two cases for it; If there is
                             * no buffer roll over i.e read head is less than write head then we
                             * need to read Contiguous memory region between read and write pointers.
                             * If there is buffer roll over then we have to perform split memory
                             * read as the memory read region will not be contiguous.
                             */

                            if ( rd_head < wr_head )
                            {
                                /*
                                 * Buffer is not roll over so size would be difference of
                                 * write and read heads.
                                 */
                                size = wr_head - rd_head;

                                bytes_written = NU_Write( File_Handle, (p_buff + rd_head), size );

                                if ( bytes_written != size )
                                {
                                    status = bytes_written;
                                }
                            }
                            else if ( rd_head > wr_head )
                            {
                                /*
                                 * Buffer has been rolled over so read data from the buffer in two steps.
                                 */
                                size = (Get_Buffer_Size() - rd_head);

                                bytes_written = NU_Write( File_Handle, (p_buff + rd_head), size );

                                if ( bytes_written != size )
                                {
                                    status = bytes_written;
                                }
                                else
                                {
                                    bytes_written = NU_Write( File_Handle, p_buff, wr_head );
                                    if ( bytes_written != wr_head )
                                    {
                                        status = bytes_written;
                                    }
                                }
                            }

                            if ( status == NU_SUCCESS )
                            {
                                status = NU_Flush( File_Handle );

                                if ( status == NU_SUCCESS )
                                {
                                    /* Critical section for updating read head. */
                                    static ESAL_AR_INT_CONTROL_VARS
                                    ESAL_GE_INT_ALL_DISABLE();
                                    Set_Read_Head( wr_head );
                                    ESAL_GE_INT_ALL_RESTORE();
                                }
                            }

                            NU_Close( File_Handle );
                        }
                    }
                }
            }
        }

        if ( status != NU_SUCCESS )
        {
            /*
             * In case of errors set read head equals write head
             * to mimic buffer empty condition.
             */
            static ESAL_AR_INT_CONTROL_VARS
            ESAL_GE_INT_ALL_DISABLE();
            Get_Buffer_Read_Write_Head( &rd_head, &wr_head );
            Set_Read_Head( wr_head );
            ESAL_GE_INT_ALL_RESTORE();
        }

        status |= NU_Release_Semaphore(&Buffer_Mutex);

        NU_Relinquish();
    }

    return status;
}

#ifdef CFG_NU_OS_SVCS_SHELL_ENABLE

/***********************************************************************
*
*   FUNCTION
*
*       FileIO_Get_File_System_Info
*
*   DESCRIPTION
*
*        Retrieves trace related file system info. Only uses with the
*        Nucleus Shell.
*
*   INPUTS
*
*       buff                            - Buffer for holding info.
*
*   OUTPUTS
*
*       status
*
***********************************************************************/
STATUS FileIO_Get_File_System_Info(CHAR ** buff)
{
    STATUS        status;
    UINT8         sec_pcluster   = 0;
    UINT16        byte_psec      = 0;
    UINT32        free_cluster   = 0;
    UINT32        total_cluster  = 0;
    UINT32        free_space     = 0;
    UINT32        total_space    = 0;
    UINT32        used_space     = 0;
    UINT32        file_count     = 0;
    DSTAT         statobj;
    CHAR          info_buff[256];
    CHAR          pattern[32] ;

    *buff = info_buff;

    memset(info_buff,0x00,sizeof(info_buff));

    status = NU_FreeSpace(Root_Dir, &sec_pcluster, &byte_psec, &free_cluster, &total_cluster);

    if ( status == NU_SUCCESS)
    {
        /* Free space available in the file system. */
        free_space = sec_pcluster * byte_psec * free_cluster;
        total_space = sec_pcluster * byte_psec * total_cluster;
        used_space = total_space - free_space;

        strcpy( pattern, Root_Dir );
        strcat( pattern, "\\" );
        strcat( pattern, "*.trace" );

        if ( NU_Get_First(&statobj , pattern) ==  NU_SUCCESS)
        {
            file_count++;
            while (1)
            {
                if (NU_Get_Next(&statobj) != NU_SUCCESS)
                {
                    NU_Done(&statobj);
                    break;
                }
                else
                {
                    file_count++;
                }
            }
        }

        sprintf(info_buff,"FS Size       : %lu bytes\n\rFS Free Space : %lu bytes\n\r" \
                "FS Used Space : %lu bytes\n\rFS Trace Logs : %lu\n\r",
                total_space, free_space, used_space, file_count);
    }

    return ( status );
}

#endif /* CFG_NU_OS_SVCS_SHELL_ENABLE */
#endif /* (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == 4) */
