/************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************/
/************************************************************************
*
*   FILENAME                                               
*
*       snmp_file.c                                              
*
*   DESCRIPTION
*
*       This file contains functions used by SNMP to do file system
*       related operations.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SNMP_Save
*       SNMP_Create_File
*       SNMP_Read_File
*
*   DEPENDENCIES
*
*       nu_net.h
*       snmp_cfg.h
*       snmp.h
*       snmp_file.h
*       pcdisk.h
*
*************************************************************************/
#include "networking/nu_net.h"

#include "networking/snmp_cfg.h"
#include "networking/snmp.h"
#include "networking/snmp_file.h"

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

#include "networking/snmp_file.h"

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Save
*
*   DESCRIPTION
*
*       This function is a general function which enables storage of
*       MIB tables in to a file system. The entry which has been
*       updated is passed to this function. The updates are then
*       also reflected in permanent storage.
*
*   INPUTS
*
*       *list_header        Pointer to the first element in list.
*       *updated_entry      Pointer to the entry that has been updated.
*       *read_entry         Pointer to a temporary location. We read
*                           entries from file in to this location.
*       storage_offset      Offset in bytes from the start of structure
*                           to where the storage type variable is located.
*       status_offset       Offset in bytes from the start of structure
*                           to where the status type variable is located.
*       sizeof_struct       The size of structure in bytes.
*       *file_name          Name of the file which is used by this table.
*       comparison_func     Function pointer which compares indices of
*                           two entries that are passed and returns 0
*                           if they are equal.
*       mib_enable          If MIB for the entries table is enabled,
*                           storage type and status for the entry will
*                           be available and use in the processing of
*                           this function. However, if MIB is disabled,
*                           the entries will still be saved. Status and
*                           storage type will not be considered. The whole
*                           file will always be written.
*
*   OUTPUTS
*
*       NU_SUCCESS          Operation was successful.
*
*************************************************************************/
STATUS SNMP_Save(VOID *list_header, VOID *updated_entry,
                 VOID *read_entry, INT32 storage_offset,
                 INT32 status_offset, UINT16 sizeof_struct,
                 CHAR *file_name, SNMP_INDEX_COMPARISON comparison_func,
                 UINT8 mib_enable)
{
    STATUS          status = NU_SUCCESS;
    INT             file;
    SNMP_NODE       *ptr;
    UINT8           found = NU_FALSE;
    UINT8           update = NU_FALSE;
    INT32           counter = 0;
    CHAR            nu_drive[3];

    nu_drive[0] = (CHAR)((SNMP_DRIVE + 1) + 'A' - 1);
    nu_drive[1] = ':';
    nu_drive[2] = '\0';

    if(updated_entry != NU_NULL)
    {
        /* Register this task to access the file system. */
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
                status = NU_Set_Default_Drive(SNMP_DRIVE);

                if (status == NU_SUCCESS)
                {
                    /*  Set Current Directory to SNMP_DIR */
                    status = NU_Set_Current_Dir(SNMP_DIR);
                    if (status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("SNMP_Save cannot set Current Directory",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("SNMP_Save cannot set Default Drive",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                if (status == NU_SUCCESS)
                {
                    /* If MIB is disabled, we will always re-write the whole file. */
                    if(mib_enable == NU_FALSE);

                    /* If the entry being updated has a storage type of non-volatile
                     * or greater which means that it will be stored to the file
                     * system and the row status is either active not in service or
                     * destroy, then go ahead and update the entry in file.
                     */
                    else if(
                        (GET8(updated_entry, storage_offset) >=
                                                        SNMP_STORAGE_NONVOLATILE) &&
                        ((GET8(updated_entry, status_offset) == SNMP_ROW_ACTIVE) ||
                         (GET8(updated_entry, status_offset) ==
                                                          SNMP_ROW_NOTINSERVICE) ||
                         (GET8(updated_entry, status_offset) == SNMP_ROW_DESTROY)))
                    {
                        /* If the row is not being destroyed. */
                        if(GET8(updated_entry, status_offset) != SNMP_ROW_DESTROY)
                        {
                            /* Traverse the link list to find the index of the
                             * structure stored in the list. We will use this to
                             * determine where the entry to be updated is located
                             * in the file. This way we do not have to traverse
                             * the whole file.
                             */
                            ptr = list_header;

                            while(ptr != NU_NULL)
                            {
                                /* Compare the indices, to determine whether this
                                 * entry is the one which was updated. The
                                 * comparison is specific to a particular table
                                 * and therefore, we use a function pointer to do
                                 * the comparison.
                                 */
                                if(comparison_func(updated_entry, ptr) == 0)
                                {
                                    found = NU_TRUE;
                                    break;
                                }

                                /* This is not the entry we are looking for.
                                 * Before we move ahead we need to determine
                                 * whether we increment the counter which tells us
                                 * how many entries we need to skip in the file.
                                 * We will only increment the counter if the entry
                                 * would be in the file.
                                 */
                                else if((GET8(ptr, storage_offset) >=
                                                        SNMP_STORAGE_NONVOLATILE) &&
                                        ((GET8(ptr, status_offset) ==
                                                        SNMP_ROW_ACTIVE) ||
                                        (GET8(ptr, status_offset) ==
                                                        SNMP_ROW_NOTINSERVICE)))
                                {
                                    counter++;
                                }

                                /* Move to the next entry. */
                                ptr = ptr->snmp_flink;
                            }
                        }

                        /* If we found the updated entry in list, then go to the
                         * location in file where this entry is located. Compare
                         * the indices of entry in memory and in file. If the
                         * are both equal then we just update that entry.
                         * Otherwise, we re-write the whole file.
                         */
                        if(found == NU_TRUE)
                        {
                            /* Open the file to read. */
                            file = NU_Open(file_name, (PO_RDONLY | PO_BINARY),
                                                                    PS_IREAD);

                            /* If the file was successfully opened for read. */
                            if(file >= 0)
                            {
                                /* Directly go to the position in the file. */
                                if(NU_Seek(file, sizeof_struct * counter,
                                            PSEEK_SET) ==
                                                       (INT32)sizeof_struct * counter)
                                {
                                    /* Read the entry from file. */
                                    if(NU_Read(file, read_entry,
                                               sizeof_struct) == sizeof_struct)
                                    {
                                        /* Compare the two entries. */
                                        if(comparison_func(updated_entry,
                                                            read_entry) == 0)
                                        {
                                            /* The entries matched so just update
                                             * the entry no need to re-write the
                                             * whole file.
                                             */
                                            update = NU_TRUE;
                                        }
                                    }
                                }

                                /* Close the file. */
                                NU_Close(file);
                            }
                        }

                        /* If we just need to update one entry in the file. */
                        if(update == NU_TRUE)
                        {
                            /* Open the file to write. */
                            file = NU_Open(file_name,
                                           (PO_WRONLY | PO_BINARY), PS_IWRITE);

                            /* If the file was successfully opened for writing. */
                            if(file >= 0)
                            {
                                /* Move to the location where the entry to be
                                 * updated is living.
                                 */
                                if(NU_Seek(file, sizeof_struct * counter,
                                           PSEEK_SET) >= (INT32)sizeof_struct * counter)
                                {
                                    /* Update the entry in file. */
                                    if(NU_Write(file, (CHAR *)updated_entry,
                                                sizeof_struct) != sizeof_struct)
                                    {
                                        /* If there was an error writing the
                                         * entry, log an error.
                                         */
                                        NLOG_Error_Log("SNMP_Save failed to update the entry in file",
                                                        NERR_SEVERE, __FILE__, __LINE__);
                                    }

                                }

                                /* Close the file. */
                                NU_Close(file);
                            }
                        }

                        /* If a new entry is added, or one is removed. We need to
                         * re-write the whole file.
                         */
                        else if(update == NU_FALSE)
                        {
                            /* Open the file to write, clear the previous contents
                             * of the file.
                             */
                            file = NU_Open(file_name, (PO_TRUNC | PO_WRONLY | PO_BINARY),
                                           PS_IWRITE);

                            /* If the file was successfully opened for writing. */
                            if(file >= 0)
                            {
                                /* Make sure we are at the beginning of the file.
                                 */
                                NU_Seek(file, 0, PSEEK_SET);

                                /* Traverse the whole list. All those entries,
                                 * which qualify to be written to permanent
                                 * storage will be written.
                                 */
                                ptr = list_header;

                                while(ptr != NU_NULL)
                                {
                                    /* Either MIB should be disabled, or the storage
                                     * type should be nonVolatile and status should
                                     * either be not in service or active for us
                                     * to write to file.
                                     */
                                    if((mib_enable == NU_FALSE) ||
                                       ((GET8(ptr, storage_offset) >=
                                                    SNMP_STORAGE_NONVOLATILE) &&
                                        ((GET8(ptr, status_offset) ==
                                                    SNMP_ROW_ACTIVE) ||
                                        (GET8(ptr, status_offset) ==
                                                    SNMP_ROW_NOTINSERVICE))))
                                    {
                                        /* Write this entry to file. */
                                        if(NU_Write(file, (CHAR *)ptr,
                                                    sizeof_struct) != sizeof_struct)
                                        {
                                            /* If there was an error writing the
                                             * entry, log an error.
                                             */
                                            NLOG_Error_Log("SNMP_Save failed to write an entry to file",
                                                           NERR_SEVERE, __FILE__, __LINE__);
                                        }

                                    }

                                    /* Go to the next entry. */
                                    ptr = ptr->snmp_flink;
                                }

                                /* Close the file. */
                                NU_Close(file);
                            }
                        }
                    }
                }

                /* Close the Disk */
                NU_Close_Disk (nu_drive);
            }

            else
            {
                NLOG_Error_Log("SNMP_Save failed to open disk", NERR_SEVERE,
                               __FILE__, __LINE__);
            }

            /* We are done using the file system. */
            NU_Release_File_User();
        }

        else
        {
            NLOG_Error_Log("SNMP_Save cannot register as Nucleus FILE User",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("SNMP_Save: MIB entry to be updated is invalid",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (NU_SUCCESS);

} /* SNMP_Save */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Create_File
*
*   DESCRIPTION
*
*       This function is a general function which creates an MIB
*       file for a particular table.
*
*   INPUTS
*
*       *file_name          Name of the file to be created.
*
*   OUTPUTS
*
*       NU_SUCCESS          Operation was successful.
*
*************************************************************************/
STATUS SNMP_Create_File(CHAR *file_name)
{
    STATUS          status;
    INT             file;
    CHAR            nu_drive[3];

    nu_drive[0] = (CHAR)((SNMP_DRIVE + 1) + 'A' - 1);
    nu_drive[1] = ':';
    nu_drive[2] = '\0';

    /* Register this task to access the file system. */
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
            status = NU_Set_Default_Drive(SNMP_DRIVE);

            if (status == NU_SUCCESS)
            {
                /*  Set Current Directory to SNMP_DIR */
                status = NU_Set_Current_Dir(SNMP_DIR);
                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("SNMP_Create_File cannot set Current Directory",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("SNMP_Create_File cannot set Default Drive",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            if (status == NU_SUCCESS)
            {
                /* Delete the file if it already exists. */
                NU_Delete(file_name);

                /* Create the file. */
                file = NU_Open(file_name, PO_RDWR | PO_CREAT | PO_BINARY,
                               PS_IWRITE);

                if(file >= 0)
                {
                    /* Close the file. */
                    NU_Close(file);

                    /* File was successfully created */
                    status = NU_SUCCESS;
                }
                else
                {
                    /* If an error occurred, make a log entry. */
                    NLOG_Error_Log("SNMP_Create_File failed to create an MIB file",
                                   NERR_SEVERE, __FILE__, __LINE__);

                    /* Failed to create the file */
                    status = SNMP_ERROR;
                }
            }

            /* Close the Disk */
            NU_Close_Disk (nu_drive);
        }

        else
        {
            NLOG_Error_Log("SNMP_Create_File cannot Open Disk",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* We are done using the file system. */
        NU_Release_File_User();
    }

    else
    {
        NLOG_Error_Log("SNMP_Create_File cannot register as Nucleus FILE User",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    return (status);

} /* SNMP_Create_File */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Read_File
*
*   DESCRIPTION
*
*       This function is a general function which reads an MIB
*       file for a particular table.
*
*   INPUTS
*
*       *table_info         Provides information about the file
*                           being read and the table to which the
*                           read data will be updated.
*
*   OUTPUTS
*
*       NU_SUCCESS          Operation was successful.
*       SNMP_ERROR          Unable to open file for read.
*
*************************************************************************/
STATUS SNMP_Read_File(SNMP_READ_FILE *table_info)
{
    STATUS          status;
    INT             file;
    CHAR            nu_drive[3];

    nu_drive[0] = (CHAR)((SNMP_DRIVE + 1) + 'A' - 1);
    nu_drive[1] = ':';
    nu_drive[2] = '\0';

    /* Register this task to access the file system. */
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
            status = NU_Set_Default_Drive(SNMP_DRIVE);

            if (status == NU_SUCCESS)
            {
                /*  Set Current Directory to SNMP_DIR */
                status = NU_Set_Current_Dir(SNMP_DIR);
                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("SNMP_Read_File cannot set Current Directory",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("SNMP_Read_File cannot set Default Drive",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            if (status == NU_SUCCESS)
            {
                /* Open the file to read. */
                file = NU_Open(table_info->snmp_file_name,
                                (PO_RDONLY | PO_BINARY), PS_IREAD);

                /* If the file was successfully opened. */
                if(file >= 0)
                {
                    /* Read each entry from file and add it to the list
                     * for that table through the passed function pointer.
                     */
                    for (;;)
                    {
                        if(NU_Read(file, (CHAR *)(table_info->snmp_read_pointer),
                                   table_info->snmp_sizeof_struct)
                                  == table_info->snmp_sizeof_struct)
                        {
                            /* We successfully got one entry. Add it. */
                            table_info->snmp_insert_func(
                                           table_info->snmp_read_pointer);
                        }
                        else
                        {
                            /* If an error occurred, make a log entry. */
                            NLOG_Error_Log("SNMP_Read_File failed to read an entry from file",
                                           NERR_SEVERE, __FILE__, __LINE__);

                            break;
                        }
                    }

                    /* Close the file. */
                    NU_Close(file);
                }
                else
                {
                    /* We were unable to open the file to read data. Pass
                     * this information back to the calling function.
                     */
                    status = SNMP_ERROR;

                    /* Also log an error. */
                    NLOG_Error_Log("SNMP_Read_File failed to open a file for reading",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            /* Close the Disk */
            NU_Close_Disk (nu_drive);
        }

        else
        {
            NLOG_Error_Log("SNMP_Read_File cannot Open Disk",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* We are done using the file system. */
        NU_Release_File_User();

    }

    else
    {
        NLOG_Error_Log("SNMP_Read_File cannot register as Nucleus FILE User",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    return (status);

} /* SNMP_Read_File */

/******************************************************************************
*
*   FUNCTION
*
*      SNMP_Wait_For_FS
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
STATUS SNMP_Wait_For_FS(CHAR *mount_point, UNSIGNED timeout)
{
    STATUS          status = 0;

    /* Suspend until a File device is mounted */
    status = NU_Storage_Device_Wait(mount_point, timeout);

    return (status);
}

#endif /* (SNMP_ENABLE_FILE_STORAGE == NU_TRUE) */
