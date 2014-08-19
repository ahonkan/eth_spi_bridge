/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
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
*       file_shell.c
*
*   COMPONENT
*
*       STORAGE SHELL
*
*   DESCRIPTION
*
*       This file contains functionality for adding file system commands
*       to a command shell
*
*   FUNCTIONS
*
*       add_default_drv
*       command_dir
*       command_del
*       command_mkdir
*       command_rmdir
*       command_copy
*       nu_os_stor_fil_shell_init
*
*   DEPENDENCIES
*
*       <stdio.h>
*       <string.h>
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_storage.h
*
*************************************************************************/
#include <stdio.h>
#include <string.h>
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "storage/nu_storage.h"

/* Local definitions */
#define YEARMASK            0x007F
#define MONTHMASK           0x000F
#define DAYMASK             0x001F
#define HOURMASK            0x001F
#define MINUTEMASK          0x003F

#define READ_BUFFER_SIZE    4096

/* Local functions Definitions */


/*************************************************************************
*
*   FUNCTION
*
*       add_default_drv
*
*   DESCRIPTION
*
*       Adds default drive to name, if no drive present
*
*   INPUTS
*
*       name - name to pre-pend drive to
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID add_default_drv(CHAR * name)
{
    CHAR    buf[CFG_NU_OS_SVCS_SHELL_COLUMNS];


    /* Check if name doesn't have drive */
    if (name[1] != ':')
    {
        /* Copy default drive into temp buffer */
        strcpy(buf, CFG_NU_OS_STOR_FILE_SHELL_DEFAULT_DRV);

        /* Concat the rest of the name to the temp buf */
        strncat(buf, name, (CFG_NU_OS_SVCS_SHELL_COLUMNS - strlen(CFG_NU_OS_STOR_FILE_SHELL_DEFAULT_DRV)));

        /* Copy back into name */
        strcpy(name, buf);
    }
    else
    {
        /* Check if name is only a drive */
        if (strlen(name) < 3)
        {
            /* Concat backslash + wildcard to parameter */
            strcat(name, "\\*");
        }
    }
}


/*************************************************************************
*
*   FUNCTION
*
*       command_dir
*
*   DESCRIPTION
*
*       Function to perform a 'dir' command (directory listing)
*
*   INPUTS
*
*       p_shell - Shell session handle
*       argc - number of arguments
*       argv - pointer to array of arguments
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
static STATUS command_dir(NU_SHELL *   p_shell,
                          INT          argc,
                          CHAR **      argv)
{
    DSTAT       statobj;
    CHAR *      temp_ptr;
    STATUS      status;
    INT         hour, total_files = 0, total_dirs = 0, total_file_size = 0;
    CHAR        buf[64];
    UINT8       attr;


    /* Determine if too many parameters passed-in */
    if (argc > 1)
    {
        /* Output error and format requirements */
        NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
        NU_Shell_Puts(p_shell, "Format: dir <path><search pattern>\r\n");
    }
    else
    {
        /* Check if no parameters */
        if (argc == 0)
        {
            /* Set to default drive configured and search pattern */
            temp_ptr = CFG_NU_OS_STOR_FILE_SHELL_DEFAULT_DRV"*";
        }
        else if (argc == 1)
        {
            /* Add default drive letter if needed */
            add_default_drv(argv[0]);

            /* Get attributes for the specified name */
            status = NU_Get_Attributes(&attr, argv[0]);

            /* Check if parameter is a directory */
            if ((status == NU_SUCCESS) && (attr & ADIRENT))
            {
                /* Concat backslash + wildcard to parameter */
                strcat(argv[0], "\\*");
            }

            /* Use argv */
            temp_ptr = argv[0];
        }

        /* Print dir banner */
        NU_Shell_Puts(p_shell, "\r\nDirectory of ");
        NU_Shell_Puts(p_shell, temp_ptr);
        NU_Shell_Puts(p_shell, "\r\n\n");

        /* Get first entry */
        status = NU_Get_First(&statobj, temp_ptr);

        /* Display messages based on status */
        switch (status)
        {
            case NU_SUCCESS:

                /* Do nothing */

                break;

            case NUF_BADDRIVE:

                /* Show bad drive */
                NU_Shell_Puts(p_shell, "\r\nERROR: The system cannot find the path specified.\r\n");

                break;

            default:

                /* Print file not found */
                NU_Shell_Puts(p_shell, "\r\nERROR: File Not Found\r\n");

                break;
        }

        /* Ensure previous operation successful */
        if (status == NU_SUCCESS)
        {
            /* Loop through entire directory */
            do
            {
                /* Calculate hour */
                hour = ((statobj.fuptime >> 11 ) & HOURMASK);

                /* Determine if AM or PM and adjust hour */
                if (hour > 12)
                {
                    /* Subtract 12 to get 12 hour time and set to PM */
                    hour -= 12;
                    temp_ptr = "PM";
                }
                else
                {
                    /* Set to AM */
                    temp_ptr = "AM";
                }

                /* Create buffer with date/time last updated */
                sprintf(buf, "%02d/%02d/%d  %02d:%02d %s", ((statobj.fupdate >> 5 ) & MONTHMASK),
                                                           (statobj.fupdate & DAYMASK),
                                                           ((statobj.fupdate >> 9 ) & YEARMASK) + 1980,
                                                           hour,
                                                           ((statobj.fuptime >> 5 ) & MINUTEMASK),
                                                           temp_ptr);

                /* Print date/time to shell */
                NU_Shell_Puts(p_shell, buf);
                NU_Shell_Puts(p_shell, "    ");

                /* Determine if a directory */
                if (statobj.fattribute & ADIRENT)
                {
                    /* Print directory */
                    NU_Shell_Puts(p_shell, "<DIR>          ");

                    /* Increment number of directories found */
                    total_dirs++;
                }
                else
                {
                    /* Print spaces for file and file size */
                    sprintf(buf, "     %10d", (INT)statobj.fsize);
                    NU_Shell_Puts(p_shell, buf);

                    /* Increment number of files and total file size */
                    total_files++;
                    total_file_size += (INT)statobj.fsize;
                }

                /* Print name */
                NU_Shell_Puts(p_shell, " ");
                NU_Shell_Puts(p_shell, statobj.lfname);

                /* Go to next line */
                NU_Shell_Puts(p_shell, "\r\n");

            /* Loop while next object available */
            } while (NU_Get_Next(&statobj) == NU_SUCCESS);

            /* Print total number of files and size */
            sprintf(buf, "      %10d File(s)     %10d bytes\r\n", total_files, total_file_size);
            NU_Shell_Puts(p_shell, buf);

            /* Print total number of directories */
            sprintf(buf, "      %10d Dir(s)\r\n", total_dirs);
            NU_Shell_Puts(p_shell, buf);
        }

        /* Free the statobj structure */
        (VOID)NU_Done (&statobj);
    }

    /* Carriage return and line-feed before going back to command shell */
    NU_Shell_Puts(p_shell, "\r\n");

    /* Return success to caller */
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       command_del
*
*   DESCRIPTION
*
*       Function to perform a 'del' command (delete files)
*
*   INPUTS
*
*       p_shell - Shell session handle
*       argc - number of arguments
*       argv - pointer to array of arguments
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
static STATUS command_del(NU_SHELL *   p_shell,
                          INT          argc,
                          CHAR **      argv)
{
    STATUS      status;
    UINT8       attr;
    CHAR        buf[64];


    /* Ensure a single parameter passed-in */
    if (argc != 1)
    {
        /* Output error and format requirements */
        NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
        NU_Shell_Puts(p_shell, "Format: del <file or directory name>\r\n");
    }
    else
    {
        /* Add default drive letter if needed */
        add_default_drv(argv[0]);

        /* Get attributes for the specified name */
        status = NU_Get_Attributes(&attr, argv[0]);

        /* Check if parameter is a directory */
        if ((status == NU_SUCCESS) && (attr & ADIRENT))
        {
            /* Concat backslash + wildcard to parameter */
            strcat(argv[0], "\\*");
        }
        
        /* Try to delete the file(s) */
        status = NU_Delete(argv[0]);

        /* Display messages based on status */
        switch (status)
        {
            case NU_SUCCESS:

                /* Do nothing */

                break;

            case NUF_NOFILE:

                /* Print error saying file not found */
                NU_Shell_Puts(p_shell, "\r\nERROR: Could Not Find ");
                NU_Shell_Puts(p_shell, argv[0]);
                NU_Shell_Puts(p_shell, "\r\n");

                break;

            default:

                /* Show generic failure message and include error number */
                NU_Shell_Puts(p_shell, "\r\nERROR: Failed to delete ");
                NU_Shell_Puts(p_shell, argv[0]);
                sprintf(buf, " (error=%d)\r\n", status);
                NU_Shell_Puts(p_shell, buf);

                break;
        }
    }

    /* Carriage return and line-feed before going back to command shell */
    NU_Shell_Puts(p_shell, "\r\n");

    /* Return success to caller */
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       command_mkdir
*
*   DESCRIPTION
*
*       Function to perform a 'mkdir' command (make directory)
*
*   INPUTS
*
*       p_shell - Shell session handle
*       argc - number of arguments
*       argv - pointer to array of arguments
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
static STATUS command_mkdir(NU_SHELL *   p_shell,
                            INT          argc,
                            CHAR **      argv)
{
    STATUS      status;
    CHAR        buf[64];


    /* Ensure a single parameter passed-in */
    if (argc != 1)
    {
        /* Output error and format requirements */
        NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
        NU_Shell_Puts(p_shell, "Format: mkdir <directory name>\r\n");
    }
    else
    {
        /* Add default drive letter if needed */
        add_default_drv(argv[0]);

        /* Try to make the specified directory */
        status = NU_Make_Dir(argv[0]);

        /* Display messages based on status */
        switch (status)
        {
            case NU_SUCCESS:

                /* Do nothing */

                break;

            case NUF_EXIST:

                /* Print message saying directory already exists */
                NU_Shell_Puts(p_shell, "\r\nERROR: A subdirectory or file ");
                NU_Shell_Puts(p_shell, argv[0]);
                NU_Shell_Puts(p_shell, " already exists.\r\n");

                break;

            default:

                /* Show generic failure message and include error number */
                sprintf(buf, " (error=%d)\r\n", status);
                NU_Shell_Puts(p_shell, "\r\nERROR: Failed to create directory ");
                NU_Shell_Puts(p_shell, argv[0]);
                NU_Shell_Puts(p_shell, buf);

                break;
        }
    }

    /* Carriage return and line-feed before going back to command shell */
    NU_Shell_Puts(p_shell, "\r\n");

    /* Return success to caller */
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       command_rmdir
*
*   DESCRIPTION
*
*       Function to perform a 'rmdir' command (removes a directory)
*
*   INPUTS
*
*       p_shell - Shell session handle
*       argc - number of arguments
*       argv - pointer to array of arguments
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
static STATUS command_rmdir(NU_SHELL *   p_shell,
                            INT          argc,
                            CHAR **      argv)
{
    STATUS      status;
    CHAR        buf[64];


    /* Ensure a single parameter passed-in */
    if (argc != 1)
    {
        /* Output error and format requirements */
        NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
        NU_Shell_Puts(p_shell, "Format: rmdir <directory name>\r\n");
    }
    else
    {
        /* Add default drive letter if needed */
        add_default_drv(argv[0]);

        /* Try to remove the specified directory */
        status = NU_Remove_Dir(argv[0]);

        /* Display messages based on status */
        switch (status)
        {
            case NU_SUCCESS:

                /* Do nothing */

                break;

            case NUF_NOEMPTY:

                /* Show error stating directory not empty */
                NU_Shell_Puts(p_shell, "\r\nERROR: This directory is not empty.\r\n");

                break;

            case NUF_NOFILE:

                /* Show error stating directory not found */
                NU_Shell_Puts(p_shell, "\r\nERROR: The system cannot find the file specified.\r\n");

                break;

            default:

                /* Show generic failure message and include error number */
                sprintf(buf, " (error=%d)\r\n", status);
                NU_Shell_Puts(p_shell, "\r\nERROR: Failed to remove directory ");
                NU_Shell_Puts(p_shell, argv[0]);
                NU_Shell_Puts(p_shell, buf);

                break;
        }
    }

    /* Carriage return and line-feed before going back to command shell */
    NU_Shell_Puts(p_shell, "\r\n");

    /* Return success to caller */
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       command_copy
*
*   DESCRIPTION
*
*       Function to perform a 'copy' command (copy a file)
*
*   INPUTS
*
*       p_shell - Shell session handle
*       argc - number of arguments
*       argv - pointer to array of arguments
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
static STATUS command_copy(NU_SHELL * p_shell,
                           INT        argc,
                           CHAR **    argv)
{
    NU_MEMORY_POOL * sys_pool;
    CHAR *           read_buffer;
    INT              fd_src;
    INT              fd_dst;
    INT              bytes_read;
    INT              bytes_written;
    CHAR             buf[CFG_NU_OS_SVCS_SHELL_COLUMNS];
    STATUS           status = NU_SUCCESS;

    /* Ensure correct number of parameters passed-in */
    if (argc != 2)
    {
        /* Output error and format requirements */
        NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
        NU_Shell_Puts(p_shell, "Format: copy <source name> <destination name>\r\n");
    }
    else
    {
        /* Get the System Memory pool pointer */
        (VOID)NU_System_Memory_Get(&sys_pool, NU_NULL);

        /* Allocate memory for the read buffer */
        status = NU_Allocate_Memory(sys_pool, (VOID *)&read_buffer,    READ_BUFFER_SIZE, NU_NO_SUSPEND);

        /* If we allocated the read buffer */
        if (status == NU_SUCCESS)
        {
            /* Copy source file name into local buffer */
            strncpy(buf, argv[0], CFG_NU_OS_SVCS_SHELL_COLUMNS);

            /* Add default drive letter if needed */
            add_default_drv(buf);

            /* Attempt to open the source file */
            fd_src = NU_Open(buf, (PO_TEXT|PO_RDONLY), PS_IREAD);

            /* If there was an error */
            if (fd_src < 0)
            {
                /* Set error status */
                status = fd_src;
            }
            else
            {
                /* Copy destination file name into local buffer */
                strncpy(buf, argv[1], CFG_NU_OS_SVCS_SHELL_COLUMNS);

                /* Add default drive letter if needed */
                add_default_drv(buf);

                /* Attempt to open the destination file */
                fd_dst = NU_Open(buf, (PO_CREAT|PO_EXCL|PO_TEXT|PO_WRONLY), PS_IWRITE);

                /* If there was an error */
                if (fd_dst < 0)
                {
                    /* Set error status */
                    status = fd_dst;
                }
                else
                {
                    /* Copy the file */
                    do
                    {
                        /* Read from the source file into the buffer */
                        bytes_read = NU_Read(fd_src, read_buffer, READ_BUFFER_SIZE);

                        /* If there was an error */
                        if (bytes_read < 0)
                        {
                            /* Set error status */
                            status = bytes_read;
                        }
                        else
                        {
                            /* If there is something to write */
                            if (bytes_read > 0)
                            {
                                /* Write to the destination file */
                                bytes_written = NU_Write(fd_dst, read_buffer, bytes_read);

                                /* If there was an error */
                                if (bytes_written < 0)
                                {
                                    /* Set error status */
                                    status = bytes_written;
                                }
                            }
                        }

                    } while ((bytes_read > 0) && (bytes_written > 0));

                    /* Close the destination file */
                    (VOID)NU_Close(fd_dst);

                    /* If there was an error */
                    if (status != NU_SUCCESS)
                    {
                        /* Delete the destination file */
                        (VOID)NU_Delete(buf);
                    }
                }

                /* Close the source file */
                (VOID)NU_Close(fd_src);
            }

            /* Deallocate the memory for the read buffer */
            (VOID)NU_Deallocate_Memory((VOID *)read_buffer);
        }

        /* Display messages based on status */
        switch (status)
        {
            case NU_SUCCESS:

                /* Do nothing */

                break;

            case NUF_EXIST:

                /* Show error */
                NU_Shell_Puts(p_shell, "\r\nERROR: Destination file already exists.\r\n");

                break;

            case NUF_NOFILE:

                /* Show error */
                NU_Shell_Puts(p_shell, "\r\nERROR: Source file does not exist.\r\n");

                break;

            case NUF_NOSPC:

                /* Show error */
                NU_Shell_Puts(p_shell, "\r\nERROR: Not enough space on device.\r\n");

                break;

            default:

                /* Show generic failure message and include error number */
                sprintf(buf, " (error=%d)\r\n", status);
                NU_Shell_Puts(p_shell, "\r\nERROR: Failed to copy file ");
                NU_Shell_Puts(p_shell, argv[0]);
                NU_Shell_Puts(p_shell, buf);

                break;
        }
    }

    /* Carriage return and line-feed before going back to command shell */
    NU_Shell_Puts(p_shell, "\r\n");

    /* Return success to caller */
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       nu_os_stor_file_shell_init
*
*   DESCRIPTION
*
*       This function is called by the Nucleus OS run-level system to
*       initialize or terminate the file shell component
*
*   INPUTS
*
*       path - Path of the Nucleus OS registry entry for the Nucleus
*              Agent.
*
*       init_cmd - Run-level commmand.
*
*   OUTPUTS
*
*       NU_SUCCESS - Indicates successful operation.
*
*       <other> - Indicates (other) internal error occured.
*
*************************************************************************/
STATUS nu_os_stor_file_shell_init (CHAR *   path, INT cmd)
{
    STATUS  status;


    /* Determine how to proceed based on the control command. */
    switch (cmd)
    {
        case RUNLEVEL_STOP :
        {
            /* ERROR: Shell service does not support shutdown. */

            /* ERROR RECOVERY: Report success and do nothing. */

            break;
        }

        case RUNLEVEL_START :
        {
            /* Register 'dir' command with all active shell sessions */
            status = NU_Register_Command(NU_NULL, "dir", command_dir);

            /* Ensure previous operation successful */
            if (status == NU_SUCCESS)
            {
                /* Register 'del' command with all active shell sessions */
                status = NU_Register_Command(NU_NULL, "del", command_del);
            }

            /* Ensure previous operation successful */
            if (status == NU_SUCCESS)
            {
                /* Register 'copy' command with all active shell sessions */
                status = NU_Register_Command(NU_NULL, "copy", command_copy);
            }

            /* Ensure previous operation successful */
            if (status == NU_SUCCESS)
            {
                /* Register 'mkdir' command with all active shell sessions */
                status = NU_Register_Command(NU_NULL, "mkdir", command_mkdir);
            }

            /* Ensure previous operation successful */
            if (status == NU_SUCCESS)
            {
                /* Register 'rmdir' command with all active shell sessions */
                status = NU_Register_Command(NU_NULL, "rmdir", command_rmdir);
            }

            break;
        }

        case RUNLEVEL_HIBERNATE :
        case RUNLEVEL_RESUME :
        {
            /* Nothing to do for hibernate operations. */

            break;
        }

        default :
        {
            /* ERROR: Unknown control command value. */

            /* ERROR RECOVERY: Report success and do nothing. */

            break;
        }
    }

    return (status);
}
