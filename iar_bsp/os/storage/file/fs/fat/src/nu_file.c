/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/*************************************************************************
* FILE NAME
*
*       nu_file.c
*
* COMPONENT
*
*       FAT
*
* DESCRIPTION
*
*       This file contains FAT file system interface.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       FAT_Init_FS                          Static init of FAT
*       fat_disk_abort                       Abort all operations on a
*                                            disk.
*       fat_check_disk                       Checks the disk for errors.
*       fat_open                             Open a file.
*       fat_read                             Read bytes from a file.
*       fat_write                            Write Bytes to a file.
*       fat_seek                             Move the file pointer.
*       fat_close                            Close a file and flush the
*                                            file allocation table.
*       fat_utime                            Set file date and time.
*       fat_set_attributes                   Set file attributes.
*       fat_get_attributes                   Get a file's attributes.
*       fat_flush                            Flush an open file.
*       fat_truncate                         Truncate an open file.
*       fat_rename                           Rename a file(s).
*       fat_delete                           Delete a file(s).
*       fat_make_dir                         Create a directory.
*       fat_remove_dir                       Delete a directory.
*       pc_fat_size                         Calculate blocks required
*                                            for a volume's Allocation
*                                            Table.
*       fat_format                           Create a file system.
*       fat_get_format_info                  Fill out a format structure.
*       fat_freespace                        Calculate and return the
*                                            free space on a disk.
*       fat_get_first                        Get stats on the first file
*                                            to match a pattern.
*       fat_get_next                         Get stats on the next file
*                                            to match a pattern.
*       fat_done                             Free resources used by
*                                            fat_get_first/fat_get_next.
*
*************************************************************************/

#include        "storage/fat_defs.h"
#include        "storage/part_extr.h"
#include        "storage/dev_extr.h"

UINT32 FILE_Unused_Param; /* Used to prevent compiler warnings */

STATIC UINT32 MAX_FAT12_CLUSTERS = 0x0ff6L;
STATIC UINT32 MAX_FAT16_CLUSTERS = 0xfff5L;
STATIC UINT32 MAX_FAT32_CLUSTERS = 0x0ffffff5L;

#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)
#include        "storage/chkdsk_extr.h"
#include        "storage/chkdsk_log.h"
#include        "storage/chkdsk_util.h"

extern UINT8 g_chkdsk_mode;
extern INT g_chkdsk_fd;

    #if(CHKDSK_DEBUG)
        #if (PLUS_VERSION_COMP <= PLUS_1_14)
            #include "plus/nu_sd.h"
        #else
            #include "hardware_drivers/serial/nu_sd.h"
        #endif

        extern  NU_SERIAL_PORT  port;

        #if (PLUS_2_0 && (PLUS_VERSION_COMP >= PLUS_2_0))
            INT             NU_SIO_Putchar(INT);           
            #define PRINTF(X, Y) SDC_Put_String(X,Y)
        #else
            #define         PRINTF(X, Y)   NU_SD_Put_String(X, Y)
        #endif
    #endif
#endif
/************************************************************************
* FUNCTION
*
*       FAT_Init_FS
*
* DESCRIPTION
*
*       Performs registration of the FAT file system. This routine is
*       called from the initialization of the application.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS                          Disk was initialized
*                                            successfully.
*       NU_UNAVAILABLE                      Initialization failed.
*       NUF_FATCORE                         FAT cache table too small.
*       NUF_NO_MEMORY                       Can't allocate internal
*                                            buffer.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS FAT_Init_FS (CHAR *fsname)
{
STATUS sts;
FS_S   fs;

    /* Make a file system table entry */
    fs.fs_format         = fat_format;
    fs.fs_get_format_info= fat_get_format_info;
    fs.fs_init           = fat_init;
    fs.fs_uninit         = fat_uninit;    
    fs.fs_mount          = fat_mount;
    fs.fs_unmount        = fat_unmount;
    fs.fs_disk_abort     = fat_disk_abort;
#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)
    fs.fs_check_disk     = fat_check_disk;
#endif
    fs.fs_freespace      = fat_freespace;
    fs.fs_flush          = fat_flush;

    fs.fs_open           = fat_open;
    fs.fs_close          = fat_close;
    fs.fs_seek           = fat_seek;
    fs.fs_read           = fat_read;
    fs.fs_write          = fat_write;
    fs.fs_utime          = fat_utime;

    fs.fs_set_attr       = fat_set_attributes;
    fs.fs_get_attr       = fat_get_attributes;
    fs.fs_truncate       = fat_truncate;
    fs.fs_delete         = fat_delete;
    fs.fs_rename         = fat_rename;

    fs.fs_get_first      = fat_wr_get_first;
    fs.fs_get_next       = fat_wr_get_next;
    fs.fs_done           = fat_wr_done;

    fs.fs_mkdir          = fat_make_dir;
    fs.fs_rmdir          = fat_remove_dir;

    fs.fs_fsnode_to_string   = fat_fsnode_to_string;
    fs.fs_vnode_allocate     = fat_allocate_fsnode;
    fs.fs_vnode_deallocate   = fat_deallocate_fsnode;

    fs.fs_release            = fat_release;
    fs.fs_reclaim            = fat_reclaim;

    /* Register the file system */
    sts = NU_Register_File_System(fsname,&fs);

    return (sts);
}

/************************************************************************
* FUNCTION
*
*       fat_disk_abort
*
* DESCRIPTION
*
*       If an application senses that there are problems with a disk, it
*       should call fat_disk_abort("D:"). This will cause all resources
*       associated with that drive to be freed, but no disk writes will
*       be attempted. All file descriptors associated with the drive
*       become invalid. After correcting the problem call
*       fat_open_disk("D:") to re-mount the disk and re-open your files.
*
*
* INPUTS
*
*       dh                                  Disk handle
*
* OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS fat_disk_abort(UINT16 dh)
{
STATUS sts;

    /* Must be last line in declarations. */
    PC_FS_ENTER()

    /* Grab exclusive access to the drive. */
    PC_DRIVE_ENTER(dh, YES)

    /* Release the drive unconditionally. */
    sts = pc_dskfree(dh, YES);

    /* Release non-exclusive use of drive. */
    PC_DRIVE_EXIT(dh)

    /* Restore the kernel state */
    PC_FS_EXIT()

    return (sts);
}

/************************************************************************
* FUNCTION
*
*       fat_check_disk
*
* DESCRIPTION
*
*       FAT Check disk utility. Used to check the disk for errors. 
*       For memory requirements please see the documentation of your 
*       file systems specific check disk. This utility will can be used 
*       to check for the following issues:
*
*               Lost Cluster Chains
*               Cross-linked Chains
*               Invalid file lengths
*               Damaged Directory Record entries
*               Mismatch FAT Tables
*
*
* INPUTS
*
*       dh(in)                              Drive Letter.
*       flag(in)                            Option flags to indicate
*                                           which test(s) to run.
*       mode(in)                            Used to indicate
*                                           which mode the test
*                                           should run in.
*
* OUTPUTS
*
*       NU_SUCCESS              If no errors where found.
*       NUF_LOG_FILE_CREATED    If errors where found and a log file
*                               was created to report them.
*       NUF_FAT_TABLES_DIFFER   FAT tables differ so log file
*                               couldn't be created.
*
*
*************************************************************************/
#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)
STATUS fat_check_disk(UINT16 dh,UINT8 flag,UINT8 mode)
{
    STATUS ret_val;        
    DSTAT statobj;            
    STATUS prev_err;
    
    /* Make sure we get exclusive access the check disk utility.  */
    ret_val = chk_get_chkdsk_lock();
    
    if(ret_val == NU_SUCCESS)
    {
        /* Set the mode that the Check Disk Utility should run in. */
        g_chkdsk_mode = mode;

        FILE_Unused_Param = ret_val;
        /* Determine which test to run. */
        if((flag & CHK_CHECK_FAT_TABLES))
        {
#if(CHKDSK_DEBUG == NU_TRUE)
            PRINTF("Checking FAT Tables\n\r",&port);
#endif
            /* If we are suppose to fix errors. Then,
               we can start logger when comparing FAT tables,
               because if they differ the contents of FAT Table 1 will
               be copied into the contents of FAT Table 2. */
            if(g_chkdsk_mode == CHK_FIX_ERRORS)
            {
                /* Start error logger. */
                ret_val = chk_start_error_logger(dh,DEFAULT_LOG_FILE);
                if(ret_val == NU_SUCCESS)
                {
                    ret_val = chk_fat_table_compare_test(dh);
                }
            }
            /* If the user just wants to log the errors and compare the FAT
               Tables then first make sure the FAT Tables are equal.
               If the FAT Tables are equal then return an error
               code. */
            else
            {
                ret_val = chk_fat_table_compare_test(dh);
                /* Can start logger if the tables are equal. */
                if(ret_val == NU_SUCCESS)
                {
                    /* Start error logger. */
                    ret_val = chk_start_error_logger(dh,DEFAULT_LOG_FILE);
                }
            }

            /* Make sure logger data gets stored on disk before performing other
              test. */
            if(ret_val == NU_SUCCESS)
            {
                ret_val = chk_flush_error_logger();
            }
        }
        /* If user doesn't care about the state of the FAT tables. */
        else
        {        
            /* Start error logger. */
            ret_val = chk_start_error_logger(dh,DEFAULT_LOG_FILE);
        }   

                
        /* Check directory record entries. */
        if((ret_val == NU_SUCCESS) && (flag & CHK_CHECK_DIR_RECORDS))
        {
#if(CHKDSK_DEBUG == NU_TRUE)
            PRINTF("Checking for Damaged Directory Records\n\r",&port);
#endif
            ret_val = chk_dir_rec_entry_test(dh);
        }

      
        /* Check for cross-linked chains. */
        if((ret_val == NU_SUCCESS) && (flag & CHK_CHECK_CROSS_LINKED_CHAIN))
        {
#if(CHKDSK_DEBUG == NU_TRUE)
            PRINTF("Checking for Cross-link chains\n\r",&port);
#endif
            ret_val = chk_crosslink_test(dh);   
            /* If cross link tests encounters invalid cluster in cluster chain, and
              user is running Damaged Directory Records(DDR) tests, in report 
              only mode, then ignore error. We can ignore error because
              they where logged when DDR ran.  If invalid clusters exist in 
              chains, they must be resolved before cross link test can
              run properly. Otherwise return error NUF_CHK_CL_INVALID, so 
              user knows to include DDR when running check disk. */
            if(((ret_val == NUF_DEFECTIVEC) || (ret_val == NUF_CHK_CL_INVALID)) && 
                ((flag & CHK_CHECK_DIR_RECORDS) && (mode == CHK_REPORT_ERRORS_ONLY)))
            {
                ret_val = NU_SUCCESS;
            }
            else if((ret_val == NUF_DEFECTIVEC) && ((flag & CHK_CHECK_DIR_RECORDS) == 0))
            {
                /* If user is running without DDR enabled, and NUF_DEFECTIVEC is returned,
                  then change return value to NUF_CHK_CL_INVALID. This is done because NUF_CHK_CL_INVALID is used
                  it indicate to the user that if DDR isn't enabled it needs to be. */
                  ret_val = NUF_CHK_CL_INVALID;

            }        
            else
            {
                /* Use return value returned. */
            }
            
        }

        /* Check for lost cluster chains */
        if((ret_val == NU_SUCCESS) && (flag & CHK_CHECK_LOST_CL_CHAIN))
        {
#if(CHKDSK_DEBUG == NU_TRUE)
            PRINTF("Checking for Lost Cluster Chains\n\r",&port);
#endif
            ret_val = chk_lost_cl_chain_test(dh);
        }

        /* Check for file sizes. */
        if((ret_val == NU_SUCCESS) && (flag & CHK_CHECK_FILES_SIZES))
        {
#if(CHKDSK_DEBUG == NU_TRUE)
           PRINTF("Checking for Invalid File Lengths\n\r",&port);
#endif
           ret_val = chk_file_size_test(dh);
           /* If file size tests encounters invalid cluster in cluster chain, and
              user is running Damaged Directory Records(DDR) tests, in report 
              only mode, then ignore error. We can ignore error because
              they where logged when DDR ran.  If invalid clusters exist in 
              chains, they must be resolved before file size test can
              run properly. Otherwise return error NUF_CHK_CL_INVALID, so 
              user knows to include DDR when running check disk. */
            if(((ret_val == NUF_DEFECTIVEC) || (ret_val == NUF_CHK_CL_INVALID)) && 
                ((flag & CHK_CHECK_DIR_RECORDS) && (mode == CHK_REPORT_ERRORS_ONLY)))
            {
                ret_val = NU_SUCCESS;
                
            }
            else if((ret_val == NUF_DEFECTIVEC) && ((flag & CHK_CHECK_DIR_RECORDS) == 0))
            {
                /* If user is running without DDR enabled, and NUF_DEFECTIVEC is returned,
                  then change return value to NUF_CHK_CL_INVALID. This is done because NUF_CHK_CL_INVALID is used
                  it indicate to the user that if DDR isn't enabled it needs to be. */
                  ret_val = NUF_CHK_CL_INVALID;
            }
            else
            {
                /* Use return value returned. */
            }

        }

          
        
        /* If the FAT tables are valid attempt to close the logger,
           even if we encountered another error. */
        if(ret_val != NUF_FAT_TABLES_DIFFER)
        {
            prev_err = ret_val;
             /* Stop the error logger. */
             ret_val = chk_stop_error_logger();
             if(ret_val == NU_SUCCESS)
             {
                 /* Check the size of the log file to see if any errors where found. */
                 ret_val = NU_Get_First(&statobj,DEFAULT_LOG_FILE);
             
                 if(ret_val == NU_SUCCESS)
                 {
                 
                     /* Check the file size if it is greater than zero,
                        then errors where found. So set the ret_val
                        to NUF_LOG_FILE_CREATED so user knows there is a
                        log file. */
                     if(statobj.fsize > 0)
                     {
                        ret_val = NU_Done(&statobj);
                        /* Set error code so user knows error file was created. */
                        if(ret_val == NU_SUCCESS)
                        {
                            ret_val = NUF_LOG_FILE_CREATED;
                        }
                    
                     }
                     /* Delete log file because no errors where found. */
                     else
                     {
                         ret_val = NU_Done(&statobj);
                         if(ret_val == NU_SUCCESS)
                         {
                             ret_val = chk_del_error_logger(DEFAULT_LOG_FILE);
                         }
                    
                     }
                 }     
            }

            if(prev_err != NU_SUCCESS)            
            {
                ret_val = prev_err;
            }
            
        }
    }
   
    if(ret_val == NU_SUCCESS)
    {
        ret_val = chk_release_chkdsk_lock();
    }
    else
    {
        /* Return previous error code. */
        (VOID)chk_release_chkdsk_lock();
    }
    
    return ret_val;    
}
#endif
/************************************************************************
* FUNCTION
*
*       fat_make_dir
*
* DESCRIPTION
*
*       Create a subdirectory in the path specified by name. Fails if a
*       file or directory of the same name already exists or if the path
*       is not found.
*
*
* INPUTS
*
*       name                                Path name
*
* OUTPUTS
*
*       NU_SUCCESS                          Directory was
*                                            made successfully.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_NOSPC                           No space to create directory
*       NUF_LONGPATH                        Path or directory name too
*                                            long.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*       NUF_ROOT_FULL                       Root directory full
*                                            in this disk.
*       NUF_NOFILE                          The specified file not
*                                            found.
*       NUF_EXIST                           The directory already exists.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        I/O error occurred.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PENOENT                             File not found or path to
*                                            file not found.
*       PEEXIST                             Exclusive access requested
*                                            but file already exists.
*       PENOSPC                             Create failed.
*
*************************************************************************/
STATUS fat_make_dir(CHAR *name)
{
STATUS      ret_stat;
DROBJ       *pobj = NU_NULL;
DROBJ       *parent_obj = NU_NULL;
MTE_S       *mte;
VOID        *path = NU_NULL;
VOID        *filename = NU_NULL;
VOID        *fileext = NU_NULL;
UINT16      len;
CHAR        *str_ptr = NU_NULL;


    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Set user error */
    fsu_set_user_error(0);

    /* Initialize locals */
    parent_obj = NU_NULL;
    pobj = NU_NULL;

    /* Convert name to a mount table entry */
    mte = fsl_mte_from_name(name);
    if (!mte)
    {
        ret_stat = NUF_BADDRIVE;
    }
    else
    {
        /* Verify that the drive exists */
        ret_stat = fs_dev_dskchk_proc(mte->mte_dh);
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Get out the filename and d:parent */
        ret_stat = pc_parsepath(&path, &filename, &fileext, (UINT8 *)name);
    }

    if (ret_stat != NU_SUCCESS)
    {
        fsu_set_user_error(PENOENT);
    }
    /* Check the directory name */
    else if (filename == NU_NULL)
    {
        fsu_set_user_error(PENOENT);
        ret_stat = NUF_INVNAME;
    }
    /* Check the "." or ".." */
    else if ( (pc_isdot((UINT8 *)filename, (UINT8 *)fileext)) ||
              (pc_isdotdot((UINT8 *)filename, (UINT8 *)fileext)) )
    {
        fsu_set_user_error(PEACCES);
        ret_stat = NUF_ACCES;
    }
    else
    {
        /* Register drive in use */
        PC_DRIVE_ENTER(mte->mte_dh, NO)

        /* Find the parent and make sure it is a directory */
        ret_stat = pc_fndnode(mte->mte_dh, &parent_obj, (UINT8 *)path);

        if ( (ret_stat == NU_SUCCESS) && (pc_isadir(parent_obj)) && (!pc_isavol(parent_obj)) )
        {
            /* Lock the parent */
            PC_INODE_ENTER(parent_obj->finode, YES)

            /* Check if the new directory will make the path too long */
            for (len = 0;(NUF_GET_CHAR(&str_ptr,(CHAR*)filename,len) == NU_SUCCESS)
                         && *str_ptr != '\0'; len++){;}

            if ( (parent_obj->finode->abs_length + len) >= (EMAXPATH - 12) )
            {
                ret_stat = NUF_LONGPATH;
            }
            else
            {
                /* Fail if the directory exists */
                pobj = NU_NULL;
                ret_stat = pc_get_inode(&pobj, parent_obj, (UINT8 *)filename);
            }
            if (ret_stat == NU_SUCCESS)
            {
                fsu_set_user_error(PEEXIST);        /* Exclusive fail */
                /* Free the current object. */
                pc_freeobj(pobj);
                ret_stat = NUF_EXIST;
            }
            else if (ret_stat == NUF_NOFILE)  /* File not found */
            {
                /* Create Directory */
                ret_stat = pc_mknode(&pobj, parent_obj, (UINT8 *)filename, (UINT8 *)fileext, ADIRENT);
                if (ret_stat != NU_SUCCESS)
                {
                    fsu_set_user_error(PENOSPC);
                }
                else
                {
                    pc_freeobj(pobj);
                }
            }

            /* Release exclusive use of finode. */
            PC_INODE_EXIT(parent_obj->finode)
            /* Free the parent object. */
            pc_freeobj(parent_obj);
        }
        else
        {
            fsu_set_user_error(PENOENT);
        }

        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(mte->mte_dh)
    }

    /* Restore the kernel state */
    PC_FS_EXIT()

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       _synch_file_ptrs
*
* DESCRIPTION
*
*       Synchronize file pointers. Read write Seek and close all call
*       here.
*       This fixes the following BUGS:
*       1. If a file is created and left open and then opened again with
*          a new file handle before any writing takes place, neither
*          file will get its fptr_cluster set correctly initially. The
*          first one to write would get set up correctly but the other
*          wouldn't. Thus if fptr_cluster is zero we see if we can set it.
*       2. If one file seeked to the end of the file or has written to
*          the end of the file its file pointer will point beyond the
*          last cluster in the chain, the next call to write will notice
*          the fptr is beyond the file size and extend the file by
*          allocating a new cluster to the chain. During this time the
*          cluster/block and byte offsets are out of synch. If another
*          instance extends the file during this time the next call to
*          write will miss this condition since fptr is not >= fsize
*          any more. To fix this we note in the file when this condition
*          is true AND, afterwards each time we work with the file we
*          see if the file has grown and adjust the cluster pointer and
*          block pointer if needed.
*
*
* INPUTS
*
*       *pfile                              Internal file representation.
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID _synch_file_ptrs(PC_FILE *pfile)
{
UINT32      clno = 0;


    if (!pfile->fptr_cluster)
    {
        /* Current cluster - note on a new file this will be zero */
        pfile->fptr_cluster = pfile->pobj->finode->fcluster;

        if (pfile->fptr_cluster)
            pfile->fptr_block = pc_cl2sector(pfile->pobj->pdrive, pfile->fptr_cluster);
        else
            pfile->fptr_block = 0L;
    }
    if (pfile->at_eof)
    {
        if (pfile->fptr_cluster)
        {
            /* Get the next cluster in a cluster chain. */
            pc_clnext(&clno, pfile->pobj->pdrive, pfile->fptr_cluster);
            /* Not file end cluster. */
            if (clno)
            {
                /* Set file cluster and sector number. */
                pfile->fptr_cluster = clno;
                pfile->fptr_block = pc_cl2sector(pfile->pobj->pdrive, pfile->fptr_cluster);
                pfile->at_eof = NO;
            }
        }
    }
}


/************************************************************************
* FUNCTION
*
*       fat_open
*
* DESCRIPTION
*
*       Open the file for access as specified in flag. If creating, use
*       mode to set the access permissions on the file.
*
*       Flag values are
*
*       PO_RDONLY                           Open for read only.
*       PO_WRONLY                           Open for write only.
*       PO_RDWR                             Read/write access allowed.
*
*       PO_APPEND                           Seek to eof on each write.
*       PO_CREAT                            Create the file if it does
*                                            not exist.
*       PO_TRUNC                            Truncate the file if it
*                                            already exists.
*       PO_EXCL                             If flag contains
*                                            (PO_CREAT | PO_EXCL) and
*                                            the file already exists
*                                            fail and set
*                                            fs_user->p_errno to EEXIST.
*
*       PO_BINARY                           Ignored. All file access is
*                                            binary.
*       PO_TEXT                             Ignored.
*
*       PO_NOSHAREANY                       Fail if the file is already
*                                            open.
*       PO_NOSHAREWRITE                     Fail if the file is already
*                                            open for write.
*
*       Mode values are
*
*       PS_IWRITE                           Write permitted.
*       PS_IREAD                            Read permitted.
*                                           (Always true anyway)
*
*
* INPUTS
*       dh                                  Disk handle
*       *name                               Open file name
*       flag                                Open flag
*       mode                                Open mode
*
* OUTPUTS
*
*       Returns a non-negative integer to be used as a file descriptor.
*       Returning a negative integer indicates an error as follows:
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_NOSPC                           No space to create directory
*       NUF_LONGPATH                        Path or directory name too
*                                            long.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*       NUF_INVPARM                         Invalid Flag/Mode is specified.
*       NUF_INVPARCMB                       Invalid Flag/Mode combination
*       NUF_PEMFILE                         No file descriptors.
*                                            available.
*                                           (too many files open)
*       NUF_ACCES                           You can't open the file
*                                            which has Directory or
*                                            VOLLABEL attributes.
*       NUF_NOSPC                           No space to create directory
*                                            in this disk.
*       NUF_SHARE                           The access conflict from
*                                            multiple task to a specific
*                                            file.
*       NUF_NOFILE                          The specified file not
*                                            found.
*       NUF_EXIST                           The directory already exists.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        I/O error occurred.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PENOENT                             File not found or path to
*                                            file not found.
*       PEMFILE                             No file descriptors
*                                            available (too many files
*                                            open).
*       PEEXIST                             Exclusive access requested
*                                            but file already exists.
*       PEACCES                             Attempt to open a read only
*                                            file or a special
*                                            (directory) file.
*       PENOSPC                             Create failed.
*       PESHARE                             Already open in exclusive
*                                            mode or we want exclusive
*                                            and it's already open.
*
*************************************************************************/
INT fat_open(UINT16 dh, CHAR *name, UINT16 flag, UINT16 mode)
{
INT         fd;
STATUS      ret_stat;
PC_FILE     *pfile;
UINT32      cluster;
DROBJ       *parent_obj;
DROBJ       *pobj;
VOID        *path;
VOID        *filename;
VOID        *fileext;
INT         open_for_write;
INT         sharing_error;

    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Initialize locals. */
    fd = 0;
    fsu_set_user_error(0);

    parent_obj = NU_NULL;
    pobj = NU_NULL;

    /* We'll need to know this in a few places. */
    if ( flag & (PO_WRONLY | PO_RDWR) )
        open_for_write = YES;
    else
        open_for_write = NO;

    /* Verify that the drive exists */
    if ( fs_dev_dskchk_proc(dh) != NU_SUCCESS )
        fd = NUF_NO_DISK;

    if (fd  < 0)
    {
        /* Bad drive or No disk. */
        fsu_set_user_error(PENOENT);
    }
    else if( flag & (~(PO_WRONLY|PO_RDWR|PO_APPEND|
                    PO_CREAT|PO_TRUNC|PO_EXCL|PO_TEXT|
                PO_BINARY|PO_NOSHAREANY|PO_NOSHAREWRITE)))
    {
        /* Invalid parameter is specified. */
        fd = NUF_INVPARM;
    }
    else if ((flag & PO_TRUNC) && (! open_for_write))
    {
        /* Invalid parameter combination is specified. */
        fd = NUF_INVPARCMB;
    }
    else if ((flag & PO_APPEND) && (!open_for_write))
    {
        /* Invalid parameter combination is specified. */
        fd = NUF_INVPARCMB;
    }
    else if ((flag & PO_EXCL) && (!(flag & PO_CREAT)))
    {
        /* Invalid parameter combination is specified. */
        fd = NUF_INVPARCMB;
    }
    else if ((flag & PO_WRONLY) && (flag & PO_RDWR))
    {
        /* Invalid parameter combination is specified. */
        fd = NUF_INVPARCMB;
    }
    else if ((flag & PO_TRUNC) && (flag & PO_APPEND))
    {
        /* Invalid parameter combination is specified. */
        fd = NUF_INVPARCMB;
    }
    else if((mode & (~(PS_IWRITE|PS_IREAD)))||(!(mode)))
    {
        /* Invalid parameter is specified. */
        fd = NUF_INVPARM;
    }
    else if ((fd = pc_allocfile()) < 0)     /* Grab a file */
    {
        /* File descriptor is not available */
        fsu_set_user_error(PEMFILE);
        fd = NUF_PEMFILE;
    }
    else
    {
        /* Get the FILE. This will never fail */
        pfile = pc_fd2file(fd);
        /* Paranoia. Set the obj to null until we have one */
        pfile->pobj = NU_NULL;
        /* Clear the File update flag */
        pfile->fupdate = NO;

        /* Register drive in use */
        PC_DRIVE_ENTER(dh, NO)

        /* Get out the filename and d:parent */
        ret_stat = pc_parsepath(&path, &filename, &fileext, (UINT8 *)name);
        if (ret_stat != NU_SUCCESS)
        {
            fsu_set_user_error(PENOENT);
            /* Release a file structure. */
            pc_freefile(fd);
            fd = (INT)ret_stat;
        }
        /* Check the "." or ".." */
        else if ( (pc_isdot((UINT8 *)filename, (UINT8 *)fileext)) ||
                  (pc_isdotdot((UINT8 *)filename, (UINT8 *)fileext)) )
        {
            fs_user->p_errno = PEACCES;
            /* Release a file structure. */
            pc_freefile(fd);
            fd = (INT)NUF_ACCES;
        }
        /* Check the directory name */
        else if (filename == NU_NULL)
        {
            fs_user->p_errno = PENOENT;
            /* Release a file structure. */
            pc_freefile(fd);
            fd = (INT)NUF_INVNAME;
        }
        else
        {
            /* Find the parent */
            ret_stat = pc_fndnode(dh, &parent_obj, (UINT8 *)path);
            if (ret_stat != NU_SUCCESS)
            {
                fsu_set_user_error(PENOENT);
                /* Release a file structure. */
                pc_freefile(fd);
                fd = (INT)ret_stat;
            }
            else if ( (!pc_isadir(parent_obj)) || (pc_isavol(parent_obj)) )
            {
                /* Release a file structure. */
                pc_freefile(fd);
                /* Free the parent object. */
                pc_freeobj(parent_obj);

                fsu_set_user_error(PENOENT);
                fd = NUF_ACCES;
            }
            else
            {
                /* Lock the parent finode. */
                PC_INODE_ENTER(parent_obj->finode, YES)

                /* Find the file and init the structure. */
                pobj = NU_NULL;
                ret_stat =  pc_get_inode(&pobj, parent_obj, (UINT8 *)filename);
                if (ret_stat == NU_SUCCESS)
                {
                    /* If we goto exit: we want them linked so we can clean up */
                    pfile->pobj = pobj;            /* Link the file to the object */

                    /* The file is already open by someone. Lets see if we are
                           compatible */
                    sharing_error = NO;

                    /* Check the sharing conditions */
                    if (pobj->finode->opencount != 1)
                    {
                        /* 1. We don't want to share with anyone */
                        if (flag & PO_NOSHAREANY)
                            sharing_error = YES;
                        /* 2. Someone else doesn't want to share */
                        if (pobj->finode->openflags & OF_EXCLUSIVE)
                            sharing_error = YES;
                        /* 3. We want exclusive write but already open for write */
                        if ( open_for_write && (flag & PO_NOSHAREWRITE) &&
                             (pobj->finode->openflags & OF_WRITE) )
                            sharing_error = YES;
                        /* 4. We want open for write but it's already open for
                              exclusive */
                        if ( (open_for_write) &&
                             (pobj->finode->openflags & OF_WRITEEXCLUSIVE) )
                            sharing_error = YES;
                        /* 5. Open for trunc when already open */
                        if (flag & PO_TRUNC)
                            sharing_error = YES;
                    }
                    if (sharing_error)
                    {
                        /* Release a file structure. */
                        pc_freefile(fd);
                        fsu_set_user_error(PESHARE);
                        fd = NUF_SHARE;
                    }
                    else if ( pc_isadir(pobj) || pc_isavol(pobj) )
                    {
                        /* Release a file structure. */
                        pc_freefile(fd);
                        fsu_set_user_error(PEACCES);        /* is a directory */
                        fd = NUF_ACCES;
                    }
                    else if ( (flag & (PO_EXCL | PO_CREAT)) == (PO_EXCL | PO_CREAT) )
                    {
                        /* Release a file structure. */
                        pc_freefile(fd);
                        fsu_set_user_error(PEEXIST);        /* Exclusive fail */
                        fd = NUF_EXIST;
                    }
                    else if (flag & PO_TRUNC)
                    {
                        cluster = pobj->finode->fcluster;
                        pobj->finode->fcluster = 0L;
                        pobj->finode->fsize = 0L;

                        /* Update an inode to disk. */
                        ret_stat = pc_update_inode(pobj, DSET_UPDATE);
                        if (ret_stat == NU_SUCCESS)
                        {
                            /* And clear up the space */
                            /* Grab exclusive access to the FAT. */
                            PC_FAT_ENTER(pobj->pdrive->dh)

                            /* Release the chain. */
                            ret_stat = pc_freechain(pobj->pdrive,cluster);
                            if (ret_stat == NU_SUCCESS)
                            {
                                /* Flush the file allocation table. */
                                ret_stat = pc_flushfat(pobj->pdrive);
                            }

                            /* Release non-exclusive use of FAT. */
                            PC_FAT_EXIT(pobj->pdrive->dh)
                        }
                    }
                    if (ret_stat != NU_SUCCESS)
                    {
                        fsu_set_user_error(PEACCES);
                        /* Release a file structure. */
                        pc_freefile(fd);
                        fd = (INT)ret_stat;
                    }
                }
                else if (ret_stat == NUF_NOFILE)   /* File not found */
                {
                    if (!(flag & PO_CREAT))
                    {
                        /* Release a file structure. */
                        pc_freefile(fd);
                        fsu_set_user_error(PENOENT);        /* File does not exist */
                        fd = NUF_NOFILE;
                    }
                    /* Do not allow create if write bits not set */
                    else if (!open_for_write)
                    {
                        /* Release a file structure. */
                        pc_freefile(fd);
                        fsu_set_user_error(PEACCES);        /* read only file */
                        fd = NUF_ACCES;
                    }
                    else
                    {
                        /* Create for read only if write perm not allowed */
                        ret_stat = pc_mknode(&pobj, parent_obj, (UINT8 *)filename, (UINT8 *)fileext,
                                            (UINT8) ((mode == PS_IREAD) ? (ARDONLY | ARCHIVE) : ARCHIVE));
                        if (ret_stat == NU_SUCCESS)
                        {
                            pfile->pobj = pobj;            /* Link the file to the object */
                        }
                        else
                        {
                            /* Release a file structure. */
                            pc_freefile(fd);
                            fsu_set_user_error(PENOSPC);
                            fd = (INT)ret_stat;
                        }
                    }
                }
                else  /* I/O Error */
                {
                    fs_user->p_errno = PENOENT;
                    /* Release a file structure. */
                    pc_freefile(fd);
                    fd = (INT)ret_stat;
                }
                if (fd >= 0) /* No error */
                {
                    /* Set the file sharing flags in the shared finode structure */
                    /* clear flags if we just opened it . */
                    if (pobj->finode->opencount == 1)
                        pobj->finode->openflags = 0;

                    if (open_for_write)
                        pobj->finode->openflags |= OF_WRITE;

                    if (flag & PO_NOSHAREWRITE)
                        pobj->finode->openflags |= OF_WRITEEXCLUSIVE;

                    if (flag & PO_NOSHAREANY)
                        pobj->finode->openflags |= OF_EXCLUSIVE;

                    pfile->flag = flag;            /* Access flags */
                    pfile->fptr = 0L;             /* File pointer */

                    /* Set the cluster and block file pointers */
                    _synch_file_ptrs(pfile);

                    fsu_set_user_error(0);

                    if (flag & PO_APPEND)
                    {
                        /* Call the internal seek routine to set file pointer to file end  */
                         ret_stat = _po_lseek(pfile, 0, PSEEK_END);

                        if(ret_stat != NU_SUCCESS)
                        {
                            fd = (INT)ret_stat;
                        }
                    }
                }

                /* Release exclusive use of finode. */
                PC_INODE_EXIT(parent_obj->finode)
                /* Free the parent object. */
                pc_freeobj(parent_obj);
            }
        }

        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(dh)
    }

    /* Restore the kernel state */
    PC_FS_EXIT()

    return(fd);
}


/************************************************************************
* FUNCTION
*
*       fat_read
*
* DESCRIPTION
*
*       Attempt to read count bytes from the current file pointer of
*       file at fd and put them in buf. The file pointer is updated.
*
*
* INPUTS
*
*       fd                                  File descriptor
*       buf                                 Buffer to read data.
*       count                               Number of read bytes.
*
* OUTPUTS
*
*       Returns the number of bytes read or negative value on error.
*
*       INT32                               A non-negative integer to be
*                                            used as a number of bytes
*                                            read.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_BADFILE                         Invalid file descriptor.
*       NUF_ACCES                           Open flag is  PO_WRONLY.
*       NUF_IO_ERROR                        I/O error occurred.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PEBADF                              Invalid file descriptor.
*       PENOSPC                             Create failed.
*
*************************************************************************/
INT32 fat_read(INT fd, CHAR *buf, INT32 count)
{
INT32       ret_val;
PC_FILE     *pfile;
UINT32      block_in_cluster;
UINT16      byte_offset_in_block;
UINT32      n_bytes;
UINT32      next_cluster;
UINT32      n_clusters;
UINT32      block_to_read;
DDRIVE      *pdrive;
UINT32      saved_ptr;
UINT32      saved_ptr_block;
UINT32      saved_ptr_cluster;
INT32       n_left;
UINT32      n_to_read;
UINT32      utemp;
UINT16      nblocks;
UINT8       local_buf[512];


    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Start by assuming success. NU_SUCCESS(0). */
    fsu_set_user_error(0);
    pdrive = NU_NULL;

    /* Get the FILE. Second argument is ignored */
    if ((pfile = pc_fd2file(fd)) == NU_NULL)
    {
        fsu_set_user_error(PEBADF);
        ret_val = NUF_BADFILE;
    }
    /* Opened for read ? */
    else if (pfile->flag & PO_WRONLY)
    {
        fsu_set_user_error(PEACCES);
        ret_val = NUF_ACCES;
    }
    /* return -1 if bad arguments */
    else if ( (!buf) || (count < 0))
    {
        ret_val = NUF_BADPARM;
    }
    else
    {
        /* Move ddrive pointer to local. */
        pdrive = pfile->pobj->pdrive;

        if (pdrive)
        {
            /* Verify that the drive exists */
            ret_val = fs_dev_dskchk_proc(pdrive->dh);
        }
        else
            ret_val = NUF_BADDRIVE;
    }

    if ( (!ret_val) &&
            /* Don't read if done. */
            ((pfile->fptr < pfile->pobj->finode->fsize) && (count)) )
    {
        /* Register Drive in use */
        PC_DRIVE_ENTER(pdrive->dh, NO)
        /* Grab exclusive access to the drobj */
        PC_INODE_ENTER(pfile->pobj->finode, YES)

        /* Set the cluster and block file pointers if not already set */
        _synch_file_ptrs(pfile);

        saved_ptr           = pfile->fptr;
        saved_ptr_block     = pfile->fptr_block;
        saved_ptr_cluster   = pfile->fptr_cluster;


        /* Truncate the read count if we need to */
        if ( (pfile->fptr + count) >= pfile->pobj->finode->fsize )
            count = (INT32) (pfile->pobj->finode->fsize - pfile->fptr);

        ret_val = n_left = count;

        while (n_left) /* Loop until read specified count bytes */
        {
            block_in_cluster = (UINT32) (pfile->fptr & pdrive->byte_into_cl_mask);
            block_in_cluster >>= 9;
            block_to_read = pfile->fptr_block + block_in_cluster;

            /* how many clusters are left */
            n_to_read = (UINT32) ( (n_left + 511) >> 9 );
            n_clusters =(UINT32) ((n_to_read+pdrive->secpalloc-1) >> pdrive->log2_secpalloc);

            /* Grab exclusive access to the FAT. */
            PC_FAT_ENTER(pdrive->dh)
            /* how many contiguous clusters can we get ? <= n_clusters */
            ret_val = pc_get_chain(pdrive, pfile->fptr_cluster, &next_cluster, n_clusters);
            /* Release non-exclusive use of FAT. */
            PC_FAT_EXIT(pdrive->dh)

             if (ret_val < 0)
            {
                fsu_set_user_error(PENOSPC);
                break;
            }

            /* Set contiguous clusters. */
            n_clusters = ret_val;

            /* Are we inside a block */
            if ( (pfile->fptr & 0x1ffL) || (n_left < 512) )
            {
                /* Get byte offset in present sector. */
                byte_offset_in_block = (UINT16) (pfile->fptr & 0x1ffL);

                /* Grab the device driver. */
                PC_DRIVE_IO_ENTER(pdrive->dh)
                /* READ */
                if (fs_dev_io_proc(pdrive->dh, block_to_read, local_buf, (UINT16) 1, YES) != NU_SUCCESS )
                {
                    /* Release the drive io locks. */
                    PC_DRIVE_IO_EXIT(pdrive->dh)
                    fsu_set_user_error(PENOSPC);
                    ret_val = NUF_IO_ERROR;
                    break;
                }
                /* Release the drive io locks. */
                PC_DRIVE_IO_EXIT(pdrive->dh)

                /* Copy source data to the local buffer */
                n_bytes = (UINT32) (512 - byte_offset_in_block);
                if (n_bytes > (UINT32)n_left)
                    n_bytes = (UINT32)n_left;

                NUF_Copybuff((UINT8 *)buf, &local_buf[byte_offset_in_block], (INT)n_bytes);

                buf += n_bytes;
                n_left -= n_bytes;
                pfile->fptr += n_bytes;

                /* Are we on a cluster boundary  ? */
                if ( !(pfile->fptr & pdrive->byte_into_cl_mask) )
                {
                    if (--n_clusters)             /* If contiguous */
                    {
                        pfile->fptr_block += pdrive->secpalloc;
                        pfile->fptr_cluster += 1;
                    }
                    else
                    {
                        pfile->fptr_cluster = next_cluster;
                        pfile->fptr_block = pc_cl2sector(pdrive, next_cluster);
                    }   /* if (--nclusters) {} else {}; */
                }       /* if (!(pfile->fptr & pdrive->byte_into_cl_mask)) */
            }           /* if ( (pfile->fptr & 0x1ff) || (n_left < 512) ) */
            else
            {
                /* Read as many blocks as possible */
                /* How many blocks in the current chain */
                n_to_read = (UINT32) (n_clusters << pdrive->log2_secpalloc);
                /* subtract out any leading blocks */
                n_to_read -= block_in_cluster;
                /* How many blocks yet to read */
                utemp = (UINT32) (n_left >> 9);
                /* take the smallest of the two */
                if (n_to_read > utemp)
                    n_to_read = utemp;

                if (n_to_read)
                {
                /* If we get here we need to read contiguous blocks */
                    block_to_read = pfile->fptr_block + block_in_cluster;

                    utemp = n_to_read;
                    while(utemp)
                    {
                        if (utemp > MAXSECTORS)
                            nblocks = MAXSECTORS;
                        else
                            nblocks = (UINT16) utemp;

                        /* Grab the device driver. */
                        PC_DRIVE_IO_ENTER(pdrive->dh)
                        /* READ */
                        if (fs_dev_io_proc(pdrive->dh, block_to_read, (UINT8 *)buf, nblocks, YES) != NU_SUCCESS )
                        {
                            /* Release the drive io locks. */
                            PC_DRIVE_IO_EXIT(pdrive->dh)
                            fsu_set_user_error(PENOSPC);
                            ret_val = NUF_IO_ERROR;
                            break;
                        }
                        /* Release the drive io locks. */
                        PC_DRIVE_IO_EXIT(pdrive->dh)

                        utemp -= nblocks;
                        block_to_read += nblocks;
                        buf += (nblocks << 9);
                    }
                    /* Check to see if there was an error in the inner while loop. */
                    /* If there was no need to continue outer loop.                   */
                    if(ret_val < 0)
                    {
                        break;
                    }

                    n_bytes = (UINT32) (n_to_read << 9);
                    n_left -= n_bytes;
                    pfile->fptr += n_bytes;

                    /* if we advanced to a cluster boundary advance the
                       cluster pointer */
                    /* utemp ==s how many clusters we read */
                    utemp =(UINT32) ((n_to_read+block_in_cluster) >> pdrive->log2_secpalloc);

                    if (utemp == n_clusters)
                    {
                        /* file-end, if previous and next cluster are identical, but only if file-size is reached*/
                        if((pfile->fptr_cluster == next_cluster) && (pfile->fptr < pfile->pobj->finode->fsize) )
                        {
                          /* no more Fat-Entries -> Error */
                          fs_user->p_errno = PEBADF;
                          ret_val = NUF_BADFILE;
                          break;
                        }
                        else
                        {
                          pfile->fptr_cluster = next_cluster;
                        }
                    }
                    else
                    {
                        /* advance the pointer as many as we read */
                        pfile->fptr_cluster += utemp;
                    }
                    /* Set file pointer. */
                    pfile->fptr_block = pc_cl2sector(pdrive, pfile->fptr_cluster);
                }
            }
        }       /* while n_left */

        if (ret_val < 0)
        {
            /* Restore pointers and return */
            pfile->fptr = saved_ptr;
            pfile->fptr_block = saved_ptr_block;
            pfile->fptr_cluster = saved_ptr_cluster;
        }
        else
        {
            /* Round the file size up to its cluster size by adding in clustersize-1
               and masking off the low bits */
            utemp =  (pfile->pobj->finode->fsize + pdrive->byte_into_cl_mask) &
                             ~(pdrive->byte_into_cl_mask);

            /* If the file pointer is beyond the space allocated to the file note it.
               Since we may need to adjust this file's cluster and block pointers
               later if someone else extends the file behind our back */
            if (pfile->fptr >= utemp)
                pfile->at_eof = YES;
            else
                pfile->at_eof = NO;

            ret_val = count;
        }

        /* Release exclusive use of finode. */
        PC_INODE_EXIT(pfile->pobj->finode)
        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(pdrive->dh)
    }

    /* Restore the kernel state */
    PC_FS_EXIT()

    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       fat_write
*
* DESCRIPTION
*
*       Attempt to write count bytes from buf to the current file
*       pointer of file at fd. The file pointer is updated.
*
*
* INPUTS
*
*       fd                                  File descriptor.
*       *buf                                Pointer of buffer to write.
*       count                               Write byte count.
*
* OUTPUTS
*
*       Returns the number of bytes written or negative value on error.
*
*       INT32                               A non-negative integer to be
*                                            used as a number of bytes
*                                            written.
*
*       If the return value is negative, the meaning is follows:
*
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_BADFILE                         File descriptor invalid.
*       NUF_ACCES                           Not a PO_WRONLY or PO_RDWR
*                                            open flag or file
*                                            attributes is ARDONLY.
*       NUF_NOSPC                           Write failed. Presumably
*                                            because of no space.
*       NUF_IO_ERROR                        I/O error occurred.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PEBADF                              Invalid file descriptor.
*       PEACCES                             Attempt to open a read only
*                                            file or a special
*                                            (directory) file.
*       PENOSPC                             Create failed.
*
*************************************************************************/
INT32 fat_write(INT fd, CHAR *buf, INT32 count)
{
INT32       ret_val;
STATUS      status;
PC_FILE     *pfile;
DDRIVE      *pdrive;
UINT32      block_in_cluster;
UINT16      byte_offset_in_block;
UINT32      next_cluster;
UINT32      saved_ptr;
UINT32      saved_ptr_block;
UINT32      saved_ptr_cluster;
UINT32      ltemp;
UINT32      n_bytes;
UINT32      n_to_write;
UINT32      n_left;
UINT32      n_blocks_left;
UINT16      nblocks;
UINT32      n_clusters;
UINT32      alloced_size;
UINT32      block_to_write;
UINT8       local_buf[512];


    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Start by assuming success. NU_SUCCESS(0) */
    fsu_set_user_error(0);
    pdrive = NU_NULL;

    /* Get the FILE. Second argument is ignored */
    if ( (pfile = pc_fd2file(fd)) == NU_NULL )
    {
        fsu_set_user_error(PEBADF);
        ret_val = NUF_BADFILE;
    }
    /* Check the file attributes */
    else if (pfile->pobj->finode->fattribute & ARDONLY)
    {
        fsu_set_user_error(PEACCES);
        ret_val = NUF_ACCES;
    }
    /* Opened for write ? */
    else if ( !((pfile->flag & PO_WRONLY) || (pfile->flag & PO_RDWR)) )
    {
        fsu_set_user_error(PEACCES);
        ret_val = NUF_ACCES;
    }
    /* Return -1 bad parameter */
    else if ( count < 0 || !buf )
    {
        ret_val = NUF_BADPARM;
    }
    else
    {
        /* Move ddrive pointer to local. */
        pdrive = pfile->pobj->pdrive;

        if (pdrive)
        {
            /* Verify that the drive exists */
            ret_val = fs_dev_dskchk_proc(pdrive->dh);
        }
        else
            ret_val = NUF_BADDRIVE;
    }

    /* (!count) : Return 0 (none written) on bad args. */
    if ( (!ret_val) && (count) )
    {
        /* Register drive in use (non-exclusive) */
        PC_DRIVE_ENTER(pdrive->dh, NO)

        /* Only one process may write at a time */
        PC_INODE_ENTER(pfile->pobj->finode, YES)

        /* if the file is size zero make sure the current cluster pointer
           is invalid */
        if (!pfile->pobj->finode->fsize)
            pfile->fptr_cluster = 0L;

        /* Set the cluster and block file pointers if not already set */
        _synch_file_ptrs(pfile);
        saved_ptr = pfile->fptr;
        saved_ptr_block = pfile->fptr_block;
        saved_ptr_cluster = pfile->fptr_cluster;

        /* calculate initial values */
        n_left = count;

        /* Round the file size up to its cluster size by adding in clustersize-1
           and masking off the low bits */
        alloced_size =  (pfile->pobj->finode->fsize + pdrive->byte_into_cl_mask) &
                         ~(pdrive->byte_into_cl_mask);

        while (n_left)
        {
            /* Get sector offset in present cluster. */
            block_in_cluster = (UINT32) (pfile->fptr & pdrive->byte_into_cl_mask);
            block_in_cluster >>= 9;

            if (pfile->fptr >= alloced_size)
            {
                /* Extending the file */

                /* Calculate write sectors. */
                n_blocks_left = (UINT32) ((n_left + 511) >> 9);
                /* how many clusters are left-
                 *  assume 1 for the current cluster.
                 *   subtract out the blocks in the current
                 *   round up by adding secpalloc-1 and then
                 *   divide by sectors per cluster

                  |  n_clusters = 1 +
                  |      (n_blocks_left-
                  |          (pdrive->secpalloc-block_in_cluster)
                  |          + pdrive->secpalloc-1) >> pdrive->log2_secpalloc;
                    ==>
                 */
                n_clusters = ( UINT32) (1 +
                ((n_blocks_left + block_in_cluster - 1) >> pdrive->log2_secpalloc));

                /* Check file size. */
                if (pdrive->fasize == 8)    /* FAT32 */
                {
                    /* Calculate file total size */
                    ltemp = alloced_size >> pdrive->log2_secpalloc;
                    ltemp >>= 9;  /* Divide by 512 */
                    ltemp += n_clusters;

                    /* Check the maximum file size(4GB) */
                    if (pdrive->maxfsize_cluster < ltemp)
                    {
                        ret_val = NUF_MAXFILE_SIZE;
                        break;  /* Exit from write loop */
                    }
                }


                /* Grab exclusive access to the FAT. */
                PC_FAT_ENTER(pdrive->dh)
                /* Call pc_alloc_chain to build a chain up to n_cluster clusters
                   long. Return the first cluster in pfile->fptr_cluster and
                   return the # of clusters in the chain. If pfile->fptr_cluster
                   is non zero, link the current cluster to the new one */
                status = pc_alloc_chain(&n_clusters, pdrive, &(pfile->fptr_cluster), n_clusters);
                /* Release non-exclusive use of FAT. */
                PC_FAT_EXIT(pdrive->dh)

                if (status != NU_SUCCESS)
                {
                    ret_val = (INT32)status;
                    break;  /* Exit from write loop */
                }
                /* Calculate the last cluster in this chain. */
                next_cluster = (UINT32) (pfile->fptr_cluster + n_clusters - 1);

                /* link the chain to the directory object if just starting */
                if (!pfile->pobj->finode->fcluster)
                    pfile->pobj->finode->fcluster = pfile->fptr_cluster;

                /* calculate the new block pointer */
                pfile->fptr_block = pc_cl2sector(pdrive, pfile->fptr_cluster);

                /* calculate amount of space used by the file */
                ltemp = n_clusters << pdrive->log2_secpalloc; ltemp <<= 9;
                alloced_size += ltemp;
            }
            else        /* Not extending the file. (writing inside the file) */
            {
                /* Calculate write sectors. */
                n_blocks_left = (UINT32) ((n_left + 511) >> 9);

                /* how many clusters are left-
                 *  assume 1 for the current cluster.
                 *   subtract out the blocks in the current
                 *   round up by adding secpalloc-1 and then
                 *   divide by sectors per cluster

                  |  n_clusters = 1 +
                  |      (n_blocks_left-
                  |          (pdrive->secpalloc-block_in_cluster)
                  |          + pdrive->secpalloc-1) >> pdrive->log2_secpalloc;
                    ==>
                */
                n_clusters = (UINT32) (1 +
                   ((n_blocks_left + block_in_cluster - 1) >> pdrive->log2_secpalloc));

                /* Grab exclusive access to the FAT. */
                PC_FAT_ENTER(pdrive->dh)
                /* how many contiguous clusters can we get ? <= n_clusters */
                ret_val = pc_get_chain(pdrive, pfile->fptr_cluster,
                                              &next_cluster, n_clusters);

                /* Release non-exclusive use of FAT. */
                PC_FAT_EXIT(pdrive->dh)

                if (pdrive->fasize == 8)    /* FAT32 */
                {
                    /* Calculate file total size */
                    ltemp = alloced_size >> pdrive->log2_secpalloc;
                    ltemp >>= 9;  /* Divide by 512 */
                    ltemp += n_clusters;

                    /* Check the maximum file size(2GB) */
                    if (pdrive->maxfsize_cluster < ltemp)
                    {
                        ret_val = NUF_MAXFILE_SIZE;
                        break;  /* Exit from write loop */
                    }
                }

                /* I/O error occurred */
                if (ret_val <= 0)
                {
                    break;  /* Exit from write loop */
                }

                /* Set contiguous clusters. */
                n_clusters = ret_val;
            }

            /* Are we inside a block */
            if ( (pfile->fptr & 0x1ffL) || (n_left < 512) )
            {
                block_in_cluster = (UINT32) (pfile->fptr & pdrive->byte_into_cl_mask);
                block_in_cluster >>= 9;
                block_to_write = pfile->fptr_block + block_in_cluster;

                byte_offset_in_block = (UINT16) (pfile->fptr & 0x1ffL);

                /* Copy source data to the local buffer */
                n_bytes = (UINT32) (512 - byte_offset_in_block);
                if (n_bytes > n_left)
                    n_bytes = n_left;

                /* Grab the device driver. */
                PC_DRIVE_IO_ENTER(pdrive->dh)
                /* File pointer is not at block boundary, then we need to read the block. */
                /* Read */
                if ( fs_dev_io_proc(pdrive->dh,
                              block_to_write, local_buf, (UINT16) 1, YES) != NU_SUCCESS )
                {
                    /* Release the drive io locks. */
                    PC_DRIVE_IO_EXIT(pdrive->dh)
                    ret_val = NUF_IO_ERROR;
                    break;  /* Exit from write loop */
                }
                NUF_Copybuff(&local_buf[byte_offset_in_block], (UINT8 *)buf, (INT)n_bytes);

                /* Write */
                if ( fs_dev_io_proc(pdrive->dh,
                                                block_to_write, local_buf, (UINT16) 1, NO) != NU_SUCCESS )
                {
                    /* Release the drive io locks. */
                    PC_DRIVE_IO_EXIT(pdrive->dh)
                    ret_val = NUF_IO_ERROR;
                    break;  /* Exit from write loop */
                }
                /* Release the drive io locks. */
                PC_DRIVE_IO_EXIT(pdrive->dh)

                buf += n_bytes;
                n_left -= n_bytes;
                pfile->fptr += n_bytes;

                /* Are we on a cluster boundary  ? */
                if ( !(pfile->fptr & pdrive->byte_into_cl_mask) )
                {
                    if (--n_clusters)             /* If contiguous */
                    {
                        pfile->fptr_block += pdrive->secpalloc;
                        pfile->fptr_cluster += 1;
                    }
                    else
                    {
    /* NOTE:            Put the next cluster into the pointer. If we had
                        allocated a chain this value is the last cluster in
                        the chain and does not concur with the byte file pointer.
                        This is not a problem since the cluster pointer is known
                        to be off at this point anyway (fptr>=alloced_size) */
                        pfile->fptr_cluster = next_cluster;
                        pfile->fptr_block = pc_cl2sector(pdrive, next_cluster);
                    }   /* if (--nclusters) {} else {}; */
                }       /* if (!(pfile->fptr & byte_into_cl_mask)) */
            }           /* if ( (pfile->fptr & 0x1ff) || (n_left < 512) ) */

            if (n_clusters && (n_left>511))
            {
                /* If we get here we need to write contiguous blocks */
                block_in_cluster = (UINT32) (pfile->fptr & pdrive->byte_into_cl_mask);
                block_in_cluster >>= 9;
                block_to_write = pfile->fptr_block + block_in_cluster;
                /* how many do we write ? */
                n_blocks_left = (UINT32) (n_left >> 9);
                n_to_write = (UINT32) ((n_clusters << pdrive->log2_secpalloc) - block_in_cluster);

                if (n_to_write > n_blocks_left)
                {
                    n_to_write = n_blocks_left;

                 /* If we are not writing to the end of the chain we may not
                    advance the cluster pointer to the beginning of the next
                    chain. We add in block_in_cluster so we account for the
                    partial cluster we've already seen */
                    next_cluster = (UINT32) (pfile->fptr_cluster +
                                     ((n_to_write+block_in_cluster) >>  pdrive->log2_secpalloc));
                }

                ltemp = n_to_write;
                ret_val = 0;
                while (ltemp)
                {
                    if (ltemp > MAXSECTORS)
                        nblocks = MAXSECTORS;
                    else
                        nblocks = (UINT16) ltemp;

                    /* Grab the device driver. */
                    PC_DRIVE_IO_ENTER(pdrive->dh)
                    /* WRITE */
                    if ( fs_dev_io_proc(pdrive->dh,
                        block_to_write, (UINT8 *)buf, nblocks, NO) != NU_SUCCESS )
                    {
                        /* Release the drive io locks. */
                        PC_DRIVE_IO_EXIT(pdrive->dh)
                        ret_val = NUF_IO_ERROR;
                        break;  /* Exit from write loop */
                    }
                    /* Release the drive io locks. */
                    PC_DRIVE_IO_EXIT(pdrive->dh)

                    ltemp -= nblocks;
                    block_to_write += nblocks;
                    buf += (nblocks << 9);
                }
                if (ret_val != 0) /* I/O Error */
                {
                    break;  /* Exit from write loop */
                }
                n_bytes = (UINT32) (n_to_write << 9);

                n_left -= n_bytes;
                pfile->fptr += n_bytes;

                /* See note above */
                pfile->fptr_cluster = next_cluster;
                pfile->fptr_block   = pc_cl2sector(pdrive, next_cluster);
            }
        }       /* while n_left */

        if (n_left == 0)
        {
            /* Update the directory entry file size. */
            if (pfile->fptr > pfile->pobj->finode->fsize)
                pfile->pobj->finode->fsize = pfile->fptr;

            /* If the file pointer is beyond the space allocated to the file note it.
               Since we may need to adjust this file's cluster and block pointers
               later if someone else extends the file behind our back */
            if (pfile->fptr >= alloced_size)
                pfile->at_eof = YES;
            else
                pfile->at_eof = NO;

            /* Set the File update flag */
            pfile->fupdate = YES;
            ret_val = count;
        }
        else
        {
            /* Restore pointers and return */
            pfile->fptr = saved_ptr;
            pfile->fptr_block = saved_ptr_block;
            pfile->fptr_cluster = saved_ptr_cluster;
            fsu_set_user_error(PENOSPC);
        }

        /* Release exclusive use of finode */
        PC_INODE_EXIT(pfile->pobj->finode)
        /* Release non-exclusive use of drive */
        PC_DRIVE_EXIT(pdrive->dh)
    }

    /* Restore the kernel state */
    PC_FS_EXIT()

    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       fat_seek
*
* DESCRIPTION
*
*       Move the file pointer offset bytes from the origin described by
*       "origin". The file pointer is set according to the following rules.
*
*       Origin                                  Rule
*       PSEEK_SET                           offset from beginning of file.
*       PSEEK_CUR                           offset from current file.
*                                            pointer.
*       PSEEK_END                           offset from end of file.
*
*       Attempting to seek beyond end of file puts the file pointer one
*       byte past eof.
*
*
* INPUTS
*
*       fd                                  File descriptor.
*       offset                              Seek bytes.
*       origin                              Origin.
*
* OUTPUTS
*
*       INT32                               A non-negative integer to be
*                                            used as a number of bytes
*                                            seek.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_BADFILE                         File descriptor invalid.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno will be set with one of the following:
*
*       PEBADF                              File descriptor invalid.
*       PEINVAL                             Seek to negative file
*                                            pointer attempted.
*
*************************************************************************/
INT32 fat_seek(INT fd, INT32 offset, INT16 origin)
{
INT32       ret_val;
STATUS      ret_stat;
PC_FILE     *pfile;
DDRIVE      *pdrive;


    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Start by assuming success. */
    fsu_set_user_error(0);
    
    /* Get the FILE. We don't want it if an error has occurred */
    if ( (pfile = pc_fd2file(fd)) == NU_NULL )
    {
        fsu_set_user_error(PEBADF);
        ret_val = NUF_BADFILE;
    }
    else
    {
        /* Move ddrive pointer to local. */
        pdrive = pfile->pobj->pdrive;
        if (!pdrive)
        {
            ret_val = NUF_BADDRIVE;
        }
        else
        {
            /* Verify that the drive exists */
            ret_val = fs_dev_dskchk_proc(pdrive->dh);
            if (ret_val == NU_SUCCESS)
            {
                /* Register drive in use */
                PC_DRIVE_ENTER(pdrive->dh, NO)
                /* Grab exclusive access to the drobj */
                PC_INODE_ENTER(pfile->pobj->finode, YES)

                /* Set the cluster and block file pointers if not already set */
                _synch_file_ptrs(pfile);

                /* Call the internal seek routine that we share with fat_truncate */
                ret_stat =  _po_lseek(pfile, offset, origin);

                if (ret_stat == NU_SUCCESS)
                    ret_val = pfile->fptr;
                else
                    ret_val = (INT32)ret_stat;

                /* Release exclusive use of finode. */
                PC_INODE_EXIT(pfile->pobj->finode)
                /* Release non-exclusive use of drive. */
                PC_DRIVE_EXIT(pdrive->dh)
            }
        }
    }

    /* Restore the kernel state */
    PC_FS_EXIT()

    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       fat_truncate
*
* DESCRIPTION
*
*       Move the file pointer offset bytes from the beginning of the
*       file and truncate the file beyond that point by adjusting the
*       file size and freeing the cluster chain past the file pointer.
*
*
* INPUTS
*
*       fd                                  File descriptor.
*       offset                              Truncate offset(bytes).
*
* OUTPUTS
*
*       NU_SUCCESS                          The file was
*                                            successfully truncated.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_BADFILE                         File descriptor invalid.
*       NUF_ACCES                           You can't change the file
*                                            which has PO_RDONLY or file
*                                            attributes is ARDONLY.
*       NUF_SHARE                           The access conflict from
*                                            multiple task to a specific
*                                            file.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno will be set with one of the following:
*
*       PEBADF                              File descriptor invalid or
*                                            open read only.
*       PENOSPC                             I/O error.
*       PEINVAL                             Invalid offset.
*       PESHARE                             Can not truncate a file open
*                                            by more than one handle.
*
*************************************************************************/
STATUS fat_truncate(INT fd, INT32 offset)
{
STATUS      ret_stat;
PC_FILE     *pfile;
DDRIVE      *pdrive = NU_NULL;
UINT32      first_cluster_to_release;
UINT32      last_cluster_in_chain;
UINT32      clno;
UINT32      saved_ptr;
UINT32      saved_ptr_block;
UINT32      saved_ptr_cluster;


    /* Must be last line in declarations. */
    PC_FS_ENTER()

    /* Start by assuming success. */
    fsu_set_user_error(0);

    /* Get the FILE. We don't want it if an error has occurred. */
    if ( (pfile = pc_fd2file(fd)) == NU_NULL )
    {
        fsu_set_user_error(PEBADF);
        ret_stat = NUF_BADFILE;
    }
    /* Check the file attributes. */
    else if (pfile->pobj->finode->fattribute & ARDONLY)
    {
        fsu_set_user_error(PEACCES);
        ret_stat = NUF_ACCES;
    }
    /* Make sure we have write privileges. */
    else if ( !((pfile->flag & PO_WRONLY) || (pfile->flag & PO_RDWR)) )
    {
        fsu_set_user_error(PEACCES);
        ret_stat = NUF_ACCES;
    }
    /* Can only truncate a file that you hold exclusively. */
    else if (pfile->pobj->finode->opencount > 1)
    {
        fsu_set_user_error(PESHARE);
        ret_stat = NUF_SHARE;
    }
    else
    {
        /* Move ddrive pointer to local. */
        pdrive = pfile->pobj->pdrive;

        if (pdrive)
        {
            /* Verify that the drive exists */
            ret_stat = fs_dev_dskchk_proc(pdrive->dh);
        }
        else
            ret_stat = NUF_BADDRIVE;
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Register drive in use. */
        PC_DRIVE_ENTER(pdrive->dh, NO)
        /* Grab exclusive access to the file. */
        PC_INODE_ENTER(pfile->pobj->finode, YES)

        /* Set the cluster and block file pointers if not already set. */
        _synch_file_ptrs(pfile);

        /* Save pointers. */
        saved_ptr = pfile->fptr;
        saved_ptr_block = pfile->fptr_block;
        saved_ptr_cluster = pfile->fptr_cluster;

        /* Call the internal seek routine that we share with fat_seek.
           Seek to offset from the origin of zero. */
        ret_stat =  _po_lseek(pfile, offset, PSEEK_SET);

        if (ret_stat == NU_SUCCESS)
        {
            offset = pfile->fptr;
            if ((UINT32)offset >= pfile->pobj->finode->fsize)
            {
                ret_stat = NUF_BADPARM;
                /* Restore pointers and return. */
                pfile->fptr = saved_ptr;
                pfile->fptr_block = saved_ptr_block;
                pfile->fptr_cluster = saved_ptr_cluster;
            }
            else
            {
                /* Are we on a cluster boundary? */
                if ( !(offset & pdrive->byte_into_cl_mask) )
                {
                    /* Free the current cluster and beyond since we're
                       on a cluster boundary. */
                    first_cluster_to_release = pfile->fptr_cluster;

                    /* Grab exclusive access to the FAT. */
                    PC_FAT_ENTER(pdrive->dh)

                    /* Find the previous cluster so we can terminate
                       the chain. */
                    clno = pfile->pobj->finode->fcluster;
                    last_cluster_in_chain = clno;

                    while (clno != first_cluster_to_release)
                    {
                        last_cluster_in_chain = clno;
                        ret_stat = pc_clnext(&clno, pdrive, clno);
                        if (ret_stat != NU_SUCCESS)
                        {
                            fsu_set_user_error(PENOSPC);
                            break;
                        }
                    }

                    /* Release non-exclusive use of FAT. */
                    PC_FAT_EXIT(pdrive->dh)
                    if (ret_stat == NU_SUCCESS)
                    {
                        /* Set ptr_cluster to last in chain so
                           read & write will work right. */
                        pfile->fptr_cluster = last_cluster_in_chain;
                        if (last_cluster_in_chain)
                            pfile->fptr_block =
                                pc_cl2sector(pdrive, last_cluster_in_chain);
                        else
                            pfile->fptr_block = 0L;
                        pfile->at_eof = YES;
                    }
                }
                /* Simple case. We aren't on a cluster boundary, just free. */
                /* The chain beyond us and terminate the list. */
                else
                {
                    /* Grab exclusive access to the FAT. */
                    PC_FAT_ENTER(pdrive->dh)

                    last_cluster_in_chain = pfile->fptr_cluster;
                    /* Make sure we are at the end of chain. */
                    ret_stat = pc_clnext(&first_cluster_to_release,
                                            pdrive, pfile->fptr_cluster);

                    /* Release non-exclusive use of FAT. */
                    PC_FAT_EXIT(pdrive->dh)
                    pfile->at_eof = YES;
                }

                if (ret_stat == NU_SUCCESS)
                {
                    /*  Now update the directory entry. */
                    pfile->pobj->finode->fsize = offset;
                    /* If the file goes to zero size unlink the chain. */
                    if (!offset)
                    {
                        pfile->pobj->finode->fcluster = 0L;
                        pfile->fptr_cluster = 0L;
                        pfile->fptr_block = 0L;
                        pfile->fptr = 0L;
                        pfile->at_eof = NO;
                        /* We're freeing the whole chain so we don't
                           mark last_cluster in chain. */
                        last_cluster_in_chain = 0;
                    }

                    /* Update an inode to disk. */
                    ret_stat = pc_update_inode(pfile->pobj, DSET_UPDATE);
                    if (ret_stat != NU_SUCCESS)
                    {
                        fsu_set_user_error(PENOSPC);
                    }
                    else
                    {
                        /* Terminate the chain and free the lost chain
                           part. */

                        /* Grab exclusive access to the FAT. */
                        PC_FAT_ENTER(pfile->pobj->pdrive->dh)

                        /* Free the rest of the chain. */
                        if (first_cluster_to_release)
                        {
                            /* Release the chain. */
                            ret_stat = pc_freechain(pfile->pobj->pdrive,
                                                first_cluster_to_release);
                        }
                        if (ret_stat == NU_SUCCESS)
                        {
                            /* Null terminate the chain. */
                            if (last_cluster_in_chain)
                            {
                                /* Terminate the list we just made. */
                                ret_stat = pc_pfaxx(pdrive,
                                                    last_cluster_in_chain,
                                                    ((UINT32) -1));
                                if (ret_stat != NU_SUCCESS)
                                {
                                    fsu_set_user_error(PENOSPC);
                                }
                            }
                        }
                        if (ret_stat == NU_SUCCESS)
                        {
                            /* Flush the file allocation table. */
                            ret_stat = pc_flushfat(pfile->pobj->pdrive);
                            if (ret_stat != NU_SUCCESS)
                            {
                                fsu_set_user_error(PENOSPC);
                            }
                        }

                        /* Release non-exclusive use of FAT. */
                        PC_FAT_EXIT(pfile->pobj->pdrive->dh)

                        /* Set the File update flag. */
                        pfile->fupdate = YES;
                    }
                }
            }
        }

        /* Release exclusive use of finode. */
        PC_INODE_EXIT(pfile->pobj->finode)
        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(pdrive->dh)
    }

    /* Restore the kernel state. */
    PC_FS_EXIT()

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       _po_lseek
*
* DESCRIPTION
*
*       Behaves as fat_seek but takes a file instead of a file descriptor.
*       Attempting to seek beyond end of file puts the file pointer one
*       byte past eof.
*       All setting up such as drive_enter and drobj_enter should have
*       been done before calling here.
*
*
*
* INPUTS
*
*       *pfile                              Internal file representation.
*       offset                              Seek bytes.
*       origin                              Origin.
*
* OUTPUTS
*
*
*       NU_SUCCESS                          If service is successful.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno will be set with one of the following:
*
*       PEBADF                              File descriptor invalid.
*       PEINVAL                             Seek to negative file
*                                            pointer attempted.
*
*************************************************************************/
STATUS _po_lseek(PC_FILE *pfile, INT32 offset, INT16 origin)
{
INT32       ret_val;
INT32       file_pointer;
DDRIVE      *pdrive;
UINT32      ltemp;
UINT32      ltemp2;
UINT32      n_clusters_to_seek;
UINT32      n_clusters;
UINT32      first_cluster;
UINT32      alloced_size;
INT         past_file;
INT16       log2_bytespcluster;


    fsu_set_user_error(0);

    /* offset from beginning of file */
    if (origin == PSEEK_SET)
    {
        file_pointer = offset;
    }
    /* offset from current file pointer */
    else if (origin == PSEEK_CUR)
    {
        file_pointer = (INT32) pfile->fptr;
        file_pointer += offset;
    }
    /*     offset from end of file */
    else if (origin == PSEEK_END)
    {
        file_pointer = (INT32) pfile->pobj->finode->fsize;
        file_pointer += offset;
    }
    else    /* Illegal origin */
    {
        fsu_set_user_error(PEINVAL);
        return(NUF_BADPARM);
    }

    if (file_pointer < 0L)
    {
        fsu_set_user_error(PEINVAL);
        return(NUF_BADPARM);
    }

    pdrive = pfile->pobj->pdrive;
    if (!pdrive)
    {
        fsu_set_user_error(PEINVAL);
        return(NUF_BADDRIVE);
    }
    
    /* If file is size zero, we are done */
    if (!pfile->pobj->finode->fsize)
    {
        return(NU_SUCCESS);
    }

    /* Check file size. */
    if (file_pointer > (INT32) pfile->pobj->finode->fsize)
    {
        file_pointer = (INT32) pfile->pobj->finode->fsize;
        past_file = YES;
    }
    else
        past_file = NO;

    /* Get byte offset into the cluster. */
    log2_bytespcluster = (INT16) (pdrive->log2_secpalloc + 9);

    /* How many clusters do we need to seek */
    /* use the current cluster as the starting point if we can */
    if (file_pointer >= (INT32)pfile->fptr)
    {
        first_cluster = pfile->fptr_cluster;
        ltemp  = file_pointer >> log2_bytespcluster;
        ltemp2 = pfile->fptr  >> log2_bytespcluster;
        n_clusters_to_seek = (UINT32) (ltemp - ltemp2);
    }
    else
    {
        /* seek from the beginning */
        first_cluster = pfile->pobj->finode->fcluster;
        ltemp = file_pointer >> log2_bytespcluster;
        n_clusters_to_seek = (UINT32) ltemp;
    }

    /* Cluster chain check. */
    while (n_clusters_to_seek)
    {
        /* Grab exclusive access to the FAT. */
        PC_FAT_ENTER(pdrive->dh)

        /* Get cluster chain. */
        ret_val = pc_get_chain(pdrive, first_cluster,
                                  &first_cluster, n_clusters_to_seek);
        /* Release non-exclusive use of FAT. */
        PC_FAT_EXIT(pdrive->dh)

        if (ret_val < 0)
        {
            fsu_set_user_error(PEINVAL);
            return(ret_val);
        }

        n_clusters = ret_val;
        n_clusters_to_seek -= n_clusters;
    }

    /* Update pointers. */
    pfile->fptr_cluster = first_cluster;
    pfile->fptr_block = pc_cl2sector(pdrive, first_cluster);
    pfile->fptr= file_pointer;

    /* If seeking to the end of file, see if we are beyond the allocated size of
       the file. If we are, we set the at_eof flag so we know to try to move the
       cluster pointer in case another file instance extends the file */
    if (past_file)
    {
        /* Round the file size up to its cluster size by adding in clustersize-1
           and masking off the low bits */
        alloced_size =  (pfile->pobj->finode->fsize + pdrive->byte_into_cl_mask) &
                         ~(pdrive->byte_into_cl_mask);
        /* If the file pointer is beyond the space allocated to the file note it
           since we may need to adjust this file's cluster and block pointers
           later if someone else extends the file behind our back */
        if (pfile->fptr >= alloced_size)
            pfile->at_eof = YES;
        else
            pfile->at_eof = NO;
    }
    else
        pfile->at_eof = NO;

    return(NU_SUCCESS);
}


/************************************************************************
* FUNCTION
*
*       _po_flush
*
* DESCRIPTION
*
*       Internal version of fat_flush() called by fat_flush and fat_close.
*
*
* INPUTS
*
*       *pfile                              Internal file representation.
*
* OUTPUTS
*
*       NU_SUCCESS                          If service is successful.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_IO_ERROR                        I/O error occurred.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PENOSPC                             I/O error.
*
*************************************************************************/
STATUS _po_flush(PC_FILE *pfile)
{
STATUS      ret_val;
INT         setdate = DSET_ACCESS;


    /* Start by assuming success */
    fsu_set_user_error(0);

    /* File update ? */
    if (pfile->fupdate)
        setdate = DSET_UPDATE;

    /* Convert to native and overwrite the existing inode*/
    ret_val =  pc_update_inode(pfile->pobj, setdate);
    if (ret_val != NU_SUCCESS )
    {
        fsu_set_user_error(PENOSPC);
    }
    else
    {
        /* Grab exclusive access to the FAT. */
        PC_FAT_ENTER(pfile->pobj->pdrive->dh)

        /* Flush the file allocation table. */
        ret_val = pc_flushfat(pfile->pobj->pdrive);

        /* Release non-exclusive use of FAT. */
        PC_FAT_EXIT(pfile->pobj->pdrive->dh)
    }

    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       fat_flush
*
* DESCRIPTION
*
*       Flush the file, updating the disk.
*
*
* INPUTS
*
*       fd                                  File descriptor.
*
* OUTPUTS
*
*       NU_SUCCESS                          The file was
*                                            successfully flushed.
*       NUF_BAD_USER                        Not a file user.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_BADFILE                         Invalid file descriptor.
*       NUF_IO_ERROR                        I/O error occurred.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PEBADF                              Invalid file descriptor.
*       PENOSPC                             I/O error.
*
*************************************************************************/
STATUS fat_flush(INT fd)
{
STATUS      ret_val;
PC_FILE     *pfile;

    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Start by assuming success */
    fsu_set_user_error(0);

    /* Get the FILE. Take it even if an error has occurred */
    if ((pfile = pc_fd2file(fd)) == NU_NULL)
    {
        fsu_set_user_error(PEBADF);
        ret_val = NUF_BADFILE;
    }
    else
    {
        /* Verify that the drive exists */
        ret_val = fs_dev_dskchk_proc(pfile->pobj->pdrive->dh);
    }

    if (ret_val == NU_SUCCESS)
    {
        /* Register drive in use */
        PC_DRIVE_ENTER(pfile->pobj->pdrive->dh, NO)

        if (pfile->flag & ( PO_RDWR | PO_WRONLY ))
        {
            /* Claim exclusive access on flush */
            PC_INODE_ENTER(pfile->pobj->finode, YES)

            /* Flush directory entry and FAT. */
            ret_val = _po_flush(pfile);

            /* Release exclusive use of finode. */
            PC_INODE_EXIT(pfile->pobj->finode)
        }

        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(pfile->pobj->pdrive->dh)
    }

    /* Restore the kernel state */
    PC_FS_EXIT()

    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       fat_close
*
* DESCRIPTION
*
*       Close the file updating the disk and freeing all core associated
*       with FD.
*
*
* INPUTS
*
*       fd                                  File descriptor.
*
* OUTPUTS
*
*       NU_SUCCESS                          The file was
*                                            successfully closed.
*       NUF_BAD_USER                        Not a file user.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_BADFILE                         Invalid file descriptor.
*       NUF_IO_ERROR                        I/O error occurred.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PEBADF                              Invalid file descriptor.
*       PENOSPC                             I/O error.
*
*************************************************************************/
STATUS fat_close(INT fd)
{
STATUS      ret_val;
PC_FILE     *pfile;
UINT16      dh;

    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Start by assuming success. */
    fsu_set_user_error(0);

    /* Get the FILE. We don't want it if an error has occurred. */
    if ((pfile = pc_fd2file(fd)) == NU_NULL)
    {
        fsu_set_user_error(PEBADF);
        ret_val = NUF_BADFILE;
    }
    else
    {
        dh = pfile->pobj->pdrive->dh;

        /* Verify that the drive exists */
        ret_val = fs_dev_dskchk_proc(dh);

        /* Register drive in use */
        PC_DRIVE_ENTER(dh, NO)

        if (ret_val == NU_SUCCESS)
        {
            if (pfile->flag & ( PO_RDWR | PO_WRONLY ))
            {
                /* Grab exclusive access to the file. */
                PC_INODE_ENTER(pfile->pobj->finode, YES)

                /* Flush directory entry and FAT. */
                ret_val = _po_flush(pfile);

                /* Release exclusive use of finode. */
                PC_INODE_EXIT(pfile->pobj->finode)
            }

            /* If this file was opened exclusively, then clear the exclusive
               bits of the openflags */
            if (pfile->flag & (PO_WRONLY | PO_RDWR))
                pfile->pobj->finode->openflags &= ~OF_WRITE;

            if (pfile->flag & PO_NOSHAREWRITE)
                pfile->pobj->finode->openflags &= ~OF_WRITEEXCLUSIVE;

            if (pfile->flag & PO_NOSHAREANY)
                pfile->pobj->finode->openflags &= ~OF_EXCLUSIVE;
        }

        /* Release the FD and its core */
        pc_freefile(fd);
        
        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(dh)
    }

    /* Restore the kernel state. */
    PC_FS_EXIT()

    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       fat_set_attributes
*
* DESCRIPTION
*
*       Set a file attributes.
*
*
* INPUTS
*
*       *name                               File name
*       newattr                             New file attribute
*
* OUTPUTS
*
*       NU_SUCCESS                          The attributes were
*                                            set successfully.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*       NUF_ACCES                           You can't change VOLLABELs.
*       NUF_NOFILE                          The specified file not
*                                            found.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PENOENT                             File not found or path to
*                                            file not found.
*       PEACCES                             Attempt to open a read only
*                                            file or a special
*                                            (directory) file.
*       PENOSPC                             I/O error.
*
*************************************************************************/
STATUS fat_set_attributes(CHAR *name, UINT8 newattr)
{
STATUS      ret_stat;
STATUS      ret_stat_w;
DROBJ       *pobj;
DROBJ       *parent_obj;
MTE_S       *mte;
VOID        *mompath =  NU_NULL;
VOID        *filename = NU_NULL;
VOID        *fileext =  NU_NULL;
INT         wdcard;
UINT8       fattribute;

    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Start by assuming success. */
    fsu_set_user_error(0);

    pobj = NU_NULL;
    parent_obj = NU_NULL;

    mte = fsl_mte_from_name(name);
    if (!mte)
    {
        ret_stat = NUF_BADDRIVE;
    }
    else
    {
        /* Verify that the drive exists */
        ret_stat = fs_dev_dskchk_proc(mte->mte_dh);
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Get the path */
        ret_stat = pc_parsepath(&mompath, &filename, &fileext, (UINT8 *)name);
    }
    
    if (ret_stat != NU_SUCCESS)
    {
        fsu_set_user_error(PENOENT);
    }
    /* Check the "." or ".." */
    else if ( (pc_isdot((UINT8 *)filename, (UINT8 *)fileext)) ||
              (pc_isdotdot((UINT8 *)filename, (UINT8 *)fileext)) )
    {
        fsu_set_user_error(PENOENT);
        ret_stat = NUF_ACCES;
    }
    else
    {
        /* Set the use wild card flag and check the parameters */
        wdcard = pc_use_wdcard((UINT8 *)filename);
        if (( wdcard ) && (newattr & ADIRENT) )
        {
            fsu_set_user_error(PEINVAL);
            ret_stat = NUF_BADPARM;
        }
        else if (newattr & AVOLUME)
        {
            fsu_set_user_error(PEINVAL);
            ret_stat = NUF_BADPARM;
        }
        else
        {
            /* Register access to the drive */
            PC_DRIVE_ENTER(mte->mte_dh, NO)

            /* Find the parent */
            ret_stat = pc_fndnode(mte->mte_dh, &parent_obj, (UINT8 *)mompath);
            if (ret_stat != NU_SUCCESS)
            {
                fsu_set_user_error(PENOENT);
            }
            else
            {
                /* Lock the parent finode. */
                PC_INODE_ENTER(parent_obj->finode, YES)

                /* Find the file and init the structure */
                pobj = NU_NULL;
                ret_stat =  pc_get_inode(&pobj, parent_obj, (UINT8 *)filename);
                if (ret_stat != NU_SUCCESS)
                {
                    fsu_set_user_error(PENOENT);
                }
                else
                {
                    /* Check the "." or ".." */
                    if ( (pc_isdot(pobj->finode->fname, pobj->finode->fext)) ||
                         (pc_isdotdot(pobj->finode->fname, pobj->finode->fext)) )
                    {
                        fsu_set_user_error(PENOENT);
                        ret_stat = NUF_NOFILE;
                    }
                    /* Check the access permissions */
                    else if ( (wdcard && (pobj->finode->fattribute & ADIRENT)) ||
                              (pobj->finode->fattribute & AVOLUME) )
                    {
                        fsu_set_user_error(PEACCES);
                        ret_stat = NUF_ACCES;
                    }
                    else
                    {
                        /* We can't remove attribute directory from directory file */
                        if ( ((pc_isadir(pobj)) && (newattr & ADIRENT)) ||
                             ((!pc_isadir(pobj)) && !(newattr & ADIRENT)) )
                        {
                            /* Set new attribute */
                            fattribute = pobj->finode->fattribute;
                            pobj->finode->fattribute = newattr;
                            /* Update an inode to disk. */
                            ret_stat = pc_update_inode(pobj, DSET_ACCESS);
                            if (ret_stat != NU_SUCCESS)
                            {
                                pobj->finode->fattribute = fattribute;
                            }
                        }
                        else
                        {
                            /* We can't remove attribute directory from directory file */
                            fsu_set_user_error(PEACCES);
                            ret_stat = NUF_ACCES;
                        }
                    }
                }

                /* If we couldn't change first file attribute, but as long as wild card is used
                   for search, we need to find next file */
                if ( (wdcard) && ((ret_stat == NU_SUCCESS) || (ret_stat == NUF_ACCES) || (ret_stat == NUF_NOFILE)) )
                {
                    /* Search loop */
                    while(wdcard)
                    {
                        /* We need to clean long filename information */
                        if ((pobj != NU_NULL) && (pobj->linfo.lnament))
                        {
                            lnam_clean(&pobj->linfo, pobj->pblkbuff);
                            pc_free_buf(pobj->pblkbuff, NO);
                        }

                        /* Now find the next file */
                        ret_stat_w = pc_next_inode(pobj, parent_obj, (UINT8 *)filename, (AVOLUME | ADIRENT));
                        if (ret_stat_w != NU_SUCCESS)
                        {
                            if ( (ret_stat == NU_SUCCESS) && (ret_stat_w == NUF_NOFILE) )
                            {
                                fsu_set_user_error(0);
                            }
                            else if ( (ret_stat != NU_SUCCESS) && (ret_stat_w == NUF_ACCES) )
                            {
                                fsu_set_user_error(PEACCES);
                                ret_stat = NUF_ACCES;
                            }
                            break;                      /* Next file is not found. */
                        }

                        /* Set new attribute */
                        fattribute = pobj->finode->fattribute;
                        pobj->finode->fattribute = newattr;

                        /* Update an inode to disk. */
                        ret_stat = pc_update_inode(pobj, DSET_ACCESS);
                        if (ret_stat != NU_SUCCESS)
                        {
                            /* I/O Error */
                            pobj->finode->fattribute = fattribute;
                            break;
                        }
                    }
                }
                /* Free the current object. */
                if (pobj)
                    pc_freeobj(pobj);

                /* Release exclusive use of finode. */
                PC_INODE_EXIT(parent_obj->finode)
                /* Free the parent object. */
                pc_freeobj(parent_obj);
            }

            /* Release non-exclusive use of drive. */
            PC_DRIVE_EXIT(mte->mte_dh)
        }
    }

    /* Restore the kernel state */
    PC_FS_EXIT()

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       fat_get_attributes
*
* DESCRIPTION
*
*       Get a file's attributes.
*
*
* INPUTS
*
*       attr                                Attribute
*       name                                File name
*
* OUTPUTS
*
*       File attributes (returned by reference in "attr").
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*       NUF_ACCES                           You can't get attributes the
*                                            file which has "." or "..".
*       NUF_NOFILE                          The specified file not
*                                            found.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PENOENT                             File not found or path to
*                                            file not found.
*
*************************************************************************/
STATUS fat_get_attributes(UINT8 *attr, CHAR *name)
{
STATUS      ret_stat;
DROBJ       *pobj;
DROBJ       *parent_obj;
MTE_S       *mte;
VOID        *mompath = NU_NULL;
VOID        *filename = NU_NULL;
VOID        *fileext = NU_NULL ;

    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Start by assuming success. */
    fsu_set_user_error(0);

    pobj = NU_NULL;
    parent_obj = NU_NULL;

    mte = fsl_mte_from_name(name);
    if (mte)
    {
        /* Verify that the drive exists */
        ret_stat = fs_dev_dskchk_proc(mte->mte_dh);
    }
    else
        ret_stat = NUF_NO_DISK;

    if (ret_stat == NU_SUCCESS)
    {
        /* Get the path */
        ret_stat = pc_parsepath(&mompath, &filename, &fileext, (UINT8 *)name);
    }

    if (ret_stat != NU_SUCCESS)
    {
        fsu_set_user_error(PENOENT);
    }
    /* Check the use of wild cards.
       Wild card is not available on get attribute service */
    else if (pc_use_wdcard((UINT8 *)filename))
    {
        fsu_set_user_error(PEINVAL);
        ret_stat = NUF_BADPARM;
    }
    /* Check the "." or ".." */
    else if ( (pc_isdot((UINT8 *)filename, (UINT8 *)fileext)) ||
              (pc_isdotdot((UINT8 *)filename, (UINT8 *)fileext)) )
    {
        fsu_set_user_error(PENOENT);
        ret_stat = NUF_ACCES;
    }
    else
    {
        /* Register access to the drive */
        PC_DRIVE_ENTER(mte->mte_dh, NO)

        /* Find the parent */
        ret_stat = pc_fndnode(mte->mte_dh, &parent_obj, (UINT8 *)mompath);

        if (ret_stat != NU_SUCCESS)
        {
            fsu_set_user_error(PENOENT);
        }
        else
        {
            /* Lock the parent finode. */
            PC_INODE_ENTER(parent_obj->finode, YES)

            /* Find the file and init the structure */
            pobj = NU_NULL;
            ret_stat =  pc_get_inode(&pobj, parent_obj, (UINT8 *)filename);
            if (ret_stat != NU_SUCCESS)
            {
                fsu_set_user_error(PENOENT);
            }
            else
            {
                *attr = pobj->finode->fattribute;
                pc_freeobj(pobj);
            }

            /* Release exclusive use of finode. */
            PC_INODE_EXIT(parent_obj->finode)
            /* Free the parent object. */
            pc_freeobj(parent_obj);
        }

        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(mte->mte_dh)
    }

    /* Restore the kernel state */
    PC_FS_EXIT()

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       fat_rename
*
* DESCRIPTION
*
*       Renames the file in path ("name") to "newname". Fails if "name" is
*       invalid, "newname" already exists or path not found. Does not test
*       if "name" is a simple file. It is possible to rename directories.
*       (This may change in the multiuser version)
*
*
* INPUTS
*
*       *name                               Old file name
*       *newname                            New file name(Rename)
*
* OUTPUTS
*
*       NU_SUCCESS                          File was successfully renamed.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_ROOT_FULL                       Root directory full.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*       NUF_ACCES                           You can't change the file
*                                            which has VOLLABEL, HIDDEN,
*                                            or SYSTEM attributes.
*       NUF_NOSPC                           No space to create directory
*       NUF_NOFILE                          The specified file not
*                                            found.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PENOENT                             File not found or path to
*                                            file not found.
*       PEACCES                             Attempt to open a read only
*                                            file or a special
*                                            (directory) file.
*       PEEXIST                             Exclusive access requested
*                                            but file already exists.
*
*************************************************************************/
INT fat_rename(CHAR *name, CHAR *newname)
{
STATUS      ret_stat;
STATUS      ret_stat_w;
DROBJ       *pobj;
DROBJ       *new_pobj;
DROBJ       *new_parent_obj;
DROBJ       *old_parent_obj;
MTE_S       *mte;
VOID        *momnewpath;
VOID        *momoldpath;
VOID        *oldfilename;
VOID        *oldfileext;
VOID        *newfilename;
VOID        *newext;
VOID        *new_name;
VOID        *new_ext;
INT         longdest;
INT         wdcard;
INT         len;
UINT8       fnambuf[MAX_SFN + 1];
UINT8       fextbuf[MAX_EXT + 1];
            /* The +2 is used because shortname should be the size of fnambuf + fextbuf. */
UINT8       shortname[MAX_EXT + MAX_SFN + 2];
UINT8       fname[MAX_LFN+1];
CHAR        *str_ptr;

    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Start by assuming success. */
    fsu_set_user_error(0);

    newfilename = oldfilename = NU_NULL;
    momoldpath = NU_NULL;
    pobj = NU_NULL;
    new_parent_obj = NU_NULL;
    old_parent_obj = NU_NULL;

    mte = fsl_mte_from_name(name);
    if (!mte)
    {
        ret_stat = NUF_BADDRIVE;
    }
    else
    {
        /* Verify that the drive exists */
        ret_stat = fs_dev_dskchk_proc(mte->mte_dh);
    }
    
    if (ret_stat == NU_SUCCESS)
    {
        /* Verify the source and destination are the same drive */
        if (mte != fsl_mte_from_name(newname))
        {
            ret_stat = NUF_BADPARM;
        }
    }

    if (ret_stat != NU_SUCCESS)
    {
        fsu_set_user_error(PENOENT);
    }
    else
    {

        /* Get the new filename. */
        ret_stat = pc_parsepath(&momnewpath, &newfilename,
                                    &newext, (UINT8 *)newname);
        if (ret_stat != NU_SUCCESS)
        {
            fsu_set_user_error(PENOENT);
        }
        else
        {
            /* Get the old path */
            ret_stat = pc_parsepath(&momoldpath, &oldfilename, &oldfileext, (UINT8 *)name);
            if (ret_stat != NU_SUCCESS)
            {
                fsu_set_user_error(PENOENT);
            }
            else
            {

                /* If a new path is not given in the destination, reuse the src path */
                if (!momnewpath)
                {
                    momnewpath = momoldpath;
                }

                /* Check the "." or ".." */
                if ( (pc_isdot((UINT8 *)newfilename, (UINT8 *)newext)) ||
                     (pc_isdotdot((UINT8 *)newfilename, (UINT8 *)newext)) ||
                     (pc_isdot((UINT8 *)oldfilename, (UINT8 *)oldfileext)) ||
                     (pc_isdotdot((UINT8 *)oldfilename, (UINT8 *)oldfileext)) )
                {
                    fsu_set_user_error(PEACCES);
                    ret_stat = NUF_ACCES;
                }
            }
        }

        /* Check for the same oldname as newname. */
        if ( (YES == pc_patcmp((UINT8 *)oldfilename, (UINT8 *)newfilename)) &&
             (YES == pc_patcmp((UINT8 *)momoldpath, (UINT8*) momnewpath)) )

        {
            fsu_set_user_error(PEINVAL);
            ret_stat = NUF_BADPARM;
        }


        /* File syntax check OK */
        if (ret_stat == NU_SUCCESS)
        {
            /* Register exclusive access to the drive */
            PC_DRIVE_ENTER(mte->mte_dh, YES)

            /* Find the old parent */
            ret_stat = pc_fndnode(mte->mte_dh, &old_parent_obj, (UINT8 *)momoldpath);
            if (ret_stat != NU_SUCCESS)
            {
                fsu_set_user_error(PENOENT);
            }
            else
            {
                /* Find the new parent */
                ret_stat = pc_fndnode(mte->mte_dh, &new_parent_obj, (UINT8 *)momnewpath);
                if (ret_stat != NU_SUCCESS)
                {
                    /* Free the parent objects. */
                    pc_freeobj(old_parent_obj); 
                    fsu_set_user_error(PENOENT);
                }
            }

            if (ret_stat == NU_SUCCESS)
            {

                /* Set the use wild card flag */
                wdcard = pc_use_wdcard((UINT8 *)oldfilename);
                wdcard |= pc_use_wdcard((UINT8 *)newfilename);

                /* Lock the parent finode. */
                PC_INODE_ENTER(old_parent_obj->finode, YES)

                /* Find the source file and init the structure */
                pobj = NU_NULL;
                ret_stat =  pc_get_inode(&pobj, old_parent_obj, (UINT8 *)oldfilename);
                if (ret_stat != NU_SUCCESS)
                {
                    fsu_set_user_error(PENOENT);
                }
                else if ( pobj->finode->fattribute & ADIRENT )
                {
                    /* Check if the new directory name will make the path too long */
                    for (len = 0; (NUF_GET_CHAR(&str_ptr,(CHAR*)newname,len) == NU_SUCCESS)
                                  && *str_ptr != '\0'; len++){;}

                    if ( (new_parent_obj->finode->abs_length + len) >= (EMAXPATH - 12) )
                    {
                        ret_stat = NUF_LONGPATH;
                    }
                }
                /* Verify exclusive access to the file, cannot be 'open' */
                else if (pobj->finode->opencount > 1)
                {
                    fsu_set_user_error(PESHARE);
                    ret_stat = NUF_SHARE;
                }

                if (ret_stat == NU_SUCCESS)
                {
                    /* Check the "." or ".." */
                    if ( (pc_isdot(pobj->finode->fname, pobj->finode->fext)) ||
                         (pc_isdotdot(pobj->finode->fname, pobj->finode->fext)) )
                    {
                        fsu_set_user_error(PENOENT);
                        ret_stat = NUF_NOFILE;
                    }
                    /* Check the access permissions */
                    else if ( (pobj->finode->fattribute & (AHIDDEN | AVOLUME| ASYSTEM)) ||
                              (wdcard && (pobj->finode->fattribute & ADIRENT)) )
                    {
                        fsu_set_user_error(PEACCES);
                        ret_stat = NUF_ACCES;
                    }
                    else
                    {
                        if (wdcard)
                        {
                            /* Setup the new file name */
                            pc_parsenewname(pobj, (UINT8 *)newfilename, (UINT8 *)newext,
                                                    &new_name, &new_ext, fname);
                        }
                        else
                        {
                            new_name = newfilename;
                            new_ext  = newext;
                        }

                        /* Find the source file and init the structure */
                        new_pobj = NU_NULL;
                        ret_stat =  pc_get_inode(&new_pobj, new_parent_obj, (UINT8 *)new_name);
                        if (ret_stat == NU_SUCCESS)
                        {
                            /* Free the current object. */
                            pc_freeobj(new_pobj);
                            fsu_set_user_error(PEEXIST);
                            ret_stat = NUF_EXIST;
                        }
                        /* New filename must not exist. */
                        else if (ret_stat == NUF_NOFILE)
                        {
                            /* Status clear. */
                            ret_stat = NU_SUCCESS;

                            /* Take a short filename and extension. */
                            longdest = pc_fileparse( fnambuf, fextbuf, new_name, new_ext );
                            if (longdest < 0)
                            {
                                fsu_set_user_error(PENOENT);
                                ret_stat = longdest;
                            }

                            /* New filename is a long filename */
                            /* Upperbar short filename ?  */
                            while( (longdest == FUSE_UPBAR) && (ret_stat == NU_SUCCESS) )
                            {
                                /* Search the short filename */
                                pc_cre_shortname(shortname, fnambuf, fextbuf);
                                new_pobj = NU_NULL;
                                ret_stat =  pc_get_inode(&new_pobj, new_parent_obj, shortname);
                                if (ret_stat == NUF_NOFILE)
                                {
                                    ret_stat = NU_SUCCESS;
                                    break;
                                }
                                else if (ret_stat == NU_SUCCESS)
                                {
                                    /* Free the current object. */
                                    pc_freeobj(new_pobj);
                                    /* Get the next short filename */
                                    pc_next_fparse(fnambuf);
                                }
                            }

                            /* measure long filename length */
                            for (len = 0; (new_name != NU_NULL) &&
                                          (NUF_GET_CHAR(&str_ptr,(CHAR *)new_name,len) == NU_SUCCESS) &&
                                          (*str_ptr != '\0'); len++){;}

                            if ((len + new_parent_obj->finode->abs_length) > EMAXPATH)
                            {
                                pc_report_error(PCERR_PATHL);
                                ret_stat = NUF_LONGPATH;
                            }
                            if (ret_stat == NU_SUCCESS)
                            {
                                /* Rename the file */
                                ret_stat = pc_renameinode(pobj, old_parent_obj, new_parent_obj, fnambuf, fextbuf, (UINT8 *)new_name, longdest);
                            }
                        }
                    }

                    /* If we couldn't change first filename, but as long as wild card is used
                       for search, we need to find next file */
                    if ( (wdcard) && ((ret_stat == NU_SUCCESS) || (ret_stat == NUF_ACCES) || (ret_stat == NUF_NOFILE)) )
                    {
                        while (wdcard)
                        {
                            /* We need to clean long filename information */
                            if (pobj->linfo.lnament)
                            {
                                lnam_clean(&pobj->linfo, pobj->pblkbuff);
                                pc_free_buf(pobj->pblkbuff, NO);
                            }
                            /* Now find the next file */
                            ret_stat_w = pc_next_inode(pobj, old_parent_obj, (UINT8 *)oldfilename,
                                                       (AHIDDEN | AVOLUME | ADIRENT | ASYSTEM));
                            if (ret_stat_w == NU_SUCCESS)
                            {
                                ret_stat = ret_stat_w;
                            }
                            else
                            {
                                if ( (ret_stat == NU_SUCCESS) && (ret_stat_w == NUF_NOFILE) )
                                {
                                    fsu_set_user_error(0);
                                }
                                else if ( (ret_stat != NU_SUCCESS) && (ret_stat_w == NUF_ACCES) )
                                {
                                    fsu_set_user_error(PEACCES);
                                    ret_stat = NUF_ACCES;
                                }
                                break;                      /* Next file is not found. */
                            }

                            /* Setup the new filename */
                            pc_parsenewname(pobj, (UINT8 *)newfilename, (UINT8 *)newext,
                                                    &new_name, &new_ext, fname);

                            /* Take a short filename and extension. */
                            longdest = pc_fileparse(fnambuf, fextbuf, new_name, new_ext);
                            if (longdest < 0)
                            {
                                fsu_set_user_error(PENOENT);
                                ret_stat = (STATUS)longdest;
                                break;
                            }

                            /* New filename is a long filename */
                            /* Upperbar short filename ?  */
                            while( (longdest == FUSE_UPBAR) && (ret_stat == NU_SUCCESS) )
                            {
                                pc_cre_shortname(shortname, fnambuf, fextbuf);
                                /* Search the short filename */
                                new_pobj = NU_NULL;
                                ret_stat =  pc_get_inode(&new_pobj, new_parent_obj, shortname);
                                if (ret_stat == NUF_NOFILE)
                                {
                                    ret_stat = NU_SUCCESS;
                                    break;
                                }
                                else if (ret_stat == NU_SUCCESS)
                                {
                                    /* Free the current object. */
                                    pc_freeobj(new_pobj);
                                    /* Get the next short filename */
                                    pc_next_fparse(fnambuf);
                                }
                            }
                            /* measure long filename length */
                            for (len = 0; (NUF_GET_CHAR(&str_ptr,(CHAR*)new_name,len) == NU_SUCCESS)
                                           && *str_ptr != '\0'; len++){;}

                            if ((len + new_parent_obj->finode->abs_length) > EMAXPATH)
                            {
                                pc_report_error(PCERR_PATHL);
                                ret_stat = NUF_LONGPATH;
                            }

                            if (ret_stat == NU_SUCCESS)
                            {
                                /* Find the source file and init the structure */
                                new_pobj = NU_NULL;
                                ret_stat =  pc_get_inode(&new_pobj, new_parent_obj, (UINT8 *)new_name);
                                if (ret_stat == NU_SUCCESS)
                                {
                                    /* Free the current object. */
                                    pc_freeobj(new_pobj);
                                    fsu_set_user_error(PEEXIST);
                                    ret_stat = NUF_EXIST;
                                    break;
                                }
                                /* Rename the file */
                                ret_stat = pc_renameinode(pobj, old_parent_obj, new_parent_obj, fnambuf, fextbuf, (UINT8 *)new_name, longdest);
                                if (ret_stat != NU_SUCCESS)
                                {
                                    /* I/O error */
                                    break;
                                }
                            }
                        }
                    }
                }
                /* Free the current object. */
                if (pobj)
                    pc_freeobj(pobj);

                /* Release exclusive use of finode. */
                PC_INODE_EXIT(old_parent_obj->finode)
                /* Free the parent objects. */
                pc_freeobj(old_parent_obj);
                pc_freeobj(new_parent_obj);
            }

            /* Release non-exclusive use of drive. */
            PC_DRIVE_EXIT(mte->mte_dh)
        }
    }

    /* Restore the kernel state */
    PC_FS_EXIT()

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       fat_delete
*
* DESCRIPTION
*
*       Delete the file in "name". Fail if not a simple file, if it
*       is open, does not exist, or is read only.
*
*
* INPUTS
*
*       *name                               File name to be deleted.
*
* OUTPUTS
*
*       NU_SUCCESS                          Delete request completed
*                                            successfully.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*       NUF_ACCES                           This file has at least one
*                                            of the following attributes:
*                                            RDONLY,HIDDEN,SYSTEM,VOLUME,
*                                            DIRENT.
*       NUF_SHARE                           The access conflict from
*                                            multiple task to a specific
*                                            file.
*       NUF_NOFILE                          The specified file not found.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        Driver I/O function routine
*       NUF_INTERNAL                        Nucleus FILE internal error.
*                                            returned error.
*
*       fs_user->p_errno is set to one of these values
*
*       PENOENT                             File not found or path to
*                                            file not found.
*       PEACCES                             Attempt delete a directory
*                                            or an open file.
*       PENOSPC                             Write failed.
*
*************************************************************************/
STATUS fat_delete(CHAR *name)
{
STATUS      ret_stat;
STATUS      ret_stat_w;
DROBJ       *pobj;
DROBJ       *parent_obj;
MTE_S       *mte;
VOID        *path;
VOID        *filename;
VOID        *fileext;
INT         wdcard;

    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Start by assuming success. */
    fsu_set_user_error(0);

    parent_obj  = NU_NULL;
    pobj        = NU_NULL;

    mte = fsl_mte_from_name(name);
    if (!mte)
    {
        ret_stat = NUF_BADDRIVE;
    }
    else
    {
        /* Verify that the drive exists */
        ret_stat = fs_dev_dskchk_proc(mte->mte_dh);
    }
    
    if (ret_stat == NU_SUCCESS)
    {
        /* Get out the filename and d:parent */
        ret_stat = pc_parsepath(&path, &filename, &fileext, (UINT8 *)name);
    }

    if (ret_stat != NU_SUCCESS)
    {
        fsu_set_user_error(PENOENT);
    }
    /* Check the "." or ".." */
    else if ( (pc_isdot((UINT8 *)filename, (UINT8 *)fileext)) ||
              (pc_isdotdot((UINT8 *)filename, (UINT8 *)fileext)) )
    {
        fsu_set_user_error(PEACCES);
        ret_stat = NUF_ACCES;
    }
    else
    {
        /* Register access to the drive */
        PC_DRIVE_ENTER(mte->mte_dh, NO)

        /* Find the parent and make sure it is a directory */
        ret_stat = pc_fndnode(mte->mte_dh, &parent_obj, (UINT8 *)path);
        if (ret_stat == NU_SUCCESS)
        {
            if ( !parent_obj || !pc_isadir(parent_obj) ||  pc_isavol(parent_obj))
            {
                fsu_set_user_error(PEACCES);
                ret_stat = NUF_ACCES;
            }
            else
            {
                /* Set the use wild card flag */
                wdcard = pc_use_wdcard((UINT8 *)filename);

                /* Lock the parent finode. */
                PC_INODE_ENTER(parent_obj->finode, YES)

                /* Find the file and init the structure. */
                pobj = NU_NULL;
                ret_stat = pc_get_inode(&pobj, parent_obj, (UINT8 *)filename);
                if (ret_stat != NU_SUCCESS)
                {
                    fsu_set_user_error(PENOENT);
                }
                else
                {
                    /* Be sure it is not the root. Since the root is an abstraction
                        we can not delete it */
                    if (pc_isroot(pobj))
                    {
                        fsu_set_user_error(PEACCES);
                        ret_stat = NUF_ACCES;
                    }
                    else if (pobj->finode->opencount > 1)
                    {
                        fsu_set_user_error(PESHARE);
                        ret_stat = NUF_SHARE;
                    }
                    else if ( (pc_isdot(pobj->finode->fname, pobj->finode->fext)) ||
                              (pc_isdotdot(pobj->finode->fname, pobj->finode->fext)) )
                    {
                        fsu_set_user_error(PENOENT);
                        ret_stat = NUF_NOFILE;
                    }
                    else if ( pobj->finode->fattribute & (ARDONLY | AHIDDEN | ASYSTEM | AVOLUME | ADIRENT) )
                    {
                        fsu_set_user_error(PEACCES);
                        ret_stat = NUF_ACCES;
                    }
                    else
                    {
                        /* Delete an inode.  */
                        ret_stat = pc_rmnode(pobj);
                        if (ret_stat != NU_SUCCESS)
                        {
                            fsu_set_user_error(PENOSPC);
                            /* Even if wildcard is used, abort delete process on error. */
                            wdcard = 0;
                        }
                    }

                    while(wdcard)
                    {
                        /* We need to clean long filename information */
                        if (pobj->linfo.lnament)
                        {
                            lnam_clean(&pobj->linfo, pobj->pblkbuff);
                            pc_free_buf(pobj->pblkbuff, NO);
                        }

                        /* Now find the next file */
                        ret_stat_w = pc_next_inode(pobj, parent_obj, (UINT8 *)filename,
                                                    (ARDONLY | AHIDDEN | ASYSTEM | AVOLUME | ADIRENT));
                        if (ret_stat_w != NU_SUCCESS)
                        {
                            if ( (ret_stat == NU_SUCCESS) && (ret_stat_w == NUF_NOFILE) )
                            {
                                fsu_set_user_error(0);
                            }
                            else if ( (ret_stat != NU_SUCCESS) && (ret_stat_w == NUF_ACCES) )
                            {
                                fsu_set_user_error(PEACCES);
                                ret_stat = NUF_ACCES;
                            }
                            break;                      /* Next file is not found. */
                        }
                        /* Be sure it is not the root. Since the root is an abstraction
                            we can not delete it */
                        else if (pc_isroot(pobj))
                        {
                            fsu_set_user_error(PEACCES);
                            ret_stat = NUF_ACCES;
                        }
                        else if (pobj->finode->opencount > 1)
                        {
                            fsu_set_user_error(PESHARE);
                            ret_stat = NUF_SHARE;
                        }
                        else
                        {
                            /* Delete an inode.  */
                            ret_stat = pc_rmnode(pobj);
                            if (ret_stat != NU_SUCCESS)
                            {
                                fsu_set_user_error(PENOSPC);
                                break;
                            }
                        }
                    } /* End while wild card */
                }

                /* Free the current object. */
                if (pobj)
                    pc_freeobj(pobj);

                /* Release exclusive use of finode. */
                PC_INODE_EXIT(parent_obj->finode)
                /* Free the parent object. */
                pc_freeobj(parent_obj);
            }
        }

        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(mte->mte_dh)
    }

    /* Restore the kernel state */
    PC_FS_EXIT()

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       fat_remove_dir
*
* DESCRIPTION
*
*       Delete the directory specified in name. Fail if name is not a
*       directory, directory is read only, or contains more than the
*       entries "." and ".."
*
*
* INPUTS
*
*       *name                               Directory name to
*                                            be deleted.
*
* OUTPUTS
*
*       NU_SUCCESS                          If the directory was
*                                            successfully removed.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*       NUF_NOFILE                          The specified file not found.
*       NUF_ACCES                           This file has at least one
*                                            of the following attributes:
*                                            RDONLY,HIDDEN,SYSTEM,VOLUME
*       NUF_SHARE                           The access conflict from
*                                            multiple task to a specific
*                                            file.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*       fs_user->p_errno is set to one of these values
*
*       PENOENT                             Directory not found or path
*                                            to file not found.
*       PEACCES                             Not a directory, not empty
*                                            or in use.
*       PENOSPC                             Write failed.
*
*************************************************************************/
STATUS fat_remove_dir(CHAR *name)
{
STATUS      ret_stat;
DROBJ       *parent_obj;
DROBJ       *pobj;
DROBJ       *pchild;
MTE_S       *mte;
VOID        *path;
VOID        *filename;
VOID        *fileext;

    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Start by assuming success. */
    fsu_set_user_error(0);

    parent_obj = NU_NULL;
    pchild = NU_NULL;
    pobj = NU_NULL;

    mte = fsl_mte_from_name(name);
    if (mte)
    {
        /* Verify that the drive exists */
        ret_stat = fs_dev_dskchk_proc(mte->mte_dh);
    }
    else
    {
        ret_stat = NUF_BADDRIVE;
    }
    
    if (ret_stat == NU_SUCCESS)
    {
        /* Get out the filename and d:parent */
        ret_stat = pc_parsepath(&path, &filename, &fileext, (UINT8 *)name);
    }

    if (ret_stat != NU_SUCCESS)
    {
        fsu_set_user_error(PENOENT);
    }
    else
    {
        /* Grab exclusive access to the drive. */
        PC_DRIVE_ENTER(mte->mte_dh, YES)

        /* Find the parent and make sure it is a directory */
        ret_stat = pc_fndnode(mte->mte_dh, &parent_obj, (UINT8 *)path);
        if (ret_stat == NU_SUCCESS)
        {
            if ( !pc_isadir(parent_obj) || pc_isavol(parent_obj) )
            {
                fsu_set_user_error(PENOENT);
                ret_stat = NUF_NOFILE;
            }
            /* Check the use of wild cards.
                Wild card is not available on remove directory service */
            else if (pc_use_wdcard((UINT8 *)filename))
            {
                fsu_set_user_error(PEINVAL);
                ret_stat = NUF_BADPARM;
            }
            /* Check the "." or ".." */
            else if ( (pc_isdot((UINT8 *)filename, (UINT8 *)fileext)) ||
                      (pc_isdotdot((UINT8 *)filename, (UINT8 *)fileext)) )
            {
                fsu_set_user_error(PENOENT);
                ret_stat = NUF_ACCES;
            }

            if (ret_stat == NU_SUCCESS)
            {
                /* Lock the parent finode. */
                PC_INODE_ENTER(parent_obj->finode, YES)

                /* Find the file */
                pobj = NU_NULL;
                ret_stat = pc_get_inode(&pobj, parent_obj, (UINT8 *)filename);
                if (ret_stat != NU_SUCCESS)
                {
                    fsu_set_user_error(PENOENT);
                }
                else
                {
                    if (!pc_isadir(pobj))
                    {
                        fsu_set_user_error(PEACCES);
                        ret_stat = NUF_ACCES;
                    }
                    else if ( pobj->finode->fattribute & (ARDONLY | AHIDDEN | ASYSTEM | AVOLUME) )
                    {
                        fsu_set_user_error(PEACCES);
                        ret_stat = NUF_ACCES;
                    }
                    else if (pobj->finode->opencount > 1)
                    {
                        fsu_set_user_error(PESHARE);
                        ret_stat = NUF_SHARE;
                    }
                    else
                    {
                        /* Search through the directory. look at all files */
                        /* Any file that is not '.' or '..' is a problem */
                        /* Call pc_get_inode with NULL to give us an obj */
                        pchild = NU_NULL;
                        while(1)
                        {
                            /* Call pc_get_inode with NU_NULL to give us
                               an obj. */
                            ret_stat = pc_get_inode(&pchild, pobj, (UINT8 *)"*");
                            if (ret_stat == NU_SUCCESS)
                            {
                                if ( (!(pc_isdot(pchild->finode->fname, pchild->finode->fext) ) ) &&
                                     (!(pc_isdotdot(pchild->finode->fname, pchild->finode->fext))) )
                                {
                                    fsu_set_user_error(PEACCES);
                                    ret_stat = NUF_NOEMPTY;
                                    break;
                                }
                            }
                            else
                                break;
                        }
                        /* Make sure this directory has no file. */
                        if (ret_stat == NUF_NOFILE)
                        {
                            /* Delete an inode.  */
                            ret_stat = pc_rmnode(pobj);
                            if (ret_stat != NU_SUCCESS)
                            {
                                fsu_set_user_error(PENOSPC);
                            }
                        }
                        /* Free the child object. */
                        pc_freeobj(pchild);
                    }
                    /* Free the current object. */
                    pc_freeobj(pobj);
                }
                /* Release exclusive use of finode. */
                PC_INODE_EXIT(parent_obj->finode)
            }

            /* Free the parent object. */
            pc_freeobj(parent_obj);
        }

        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(mte->mte_dh)
    }

    /* Restore the kernel state */
    PC_FS_EXIT()

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       pc_fat_size
*
* DESCRIPTION
*
*       Calculate a disk's FAT size based on input parameters.
*       Given a drive description, including number of reserved sectors
*       (this includes block 0), number of root directory entries (must
*       be an even multiple of 16), cluster size, and number of fat
*       copies, this routine calculates the size in blocks of
*       one copy of the FAT. This routine may be used to format volumes
*       without experimenting to find the best possible size for the FAT.
*
*       Note: cluster_size must be 1 or a multiple of 2 (1 2 4 8 16 ..).
*             Root_entries must be a multiple of 16. nfat_copies should
*             be 1 or 2. Use one on a ramdisk since redundancy is
*             useless.
*
*       Note: This algorithm is not perfect. When the number of clusters
*             is slightly greater than 4087 the decision about whether 3
*             or four nibble FAT entries could be made wrong. I added
*             some code to try to resolve it but you are best off
*             keeping total clusters less than 4087 or greater than
*             say.. 4200.
*
*
* INPUTS
*
*       bytepsec                            Bytes per sectors
*       nreserved                           Reserved sectors before the
*                                            FAT
*       cluster_size                        Sectors per cluster
*       n_fat_copies                        Number of FATs
*       root_entries                        Root dir entries
*       volume_size                         Volume size
*       fasize                              FAT size
*
* OUTPUTS
*
*       Returns the size in blocks of the FAT.
*
*************************************************************************/
UINT32 pc_fat_size(UINT16 bytepsec, UINT16 nreserved, UINT16 cluster_size,
                    UINT16 n_fat_copies, UINT16 root_entries, UINT32 volume_size, UINT16 fasize)
{
UINT32      fat_size;
UINT32      fat_size_w;
UINT32      total_clusters;
UINT16      root_sectors;
UINT16      btemp;


    /* Calculate root directory using sectors
        Note: root_entries must be an even multiple of INOPBLOCK (16).
              FAT32 is always zero. Root directory into data area. */
    if (fasize <= 4)
        root_sectors = (UINT16) ((root_entries + INOPBLOCK - 1) / INOPBLOCK);
    else
        root_sectors = 0;

    /* Calculate total cluster size. Assuming zero size FAT:
       We round up to the nearest cluster boundary */
    total_clusters = (UINT32) (volume_size - nreserved - root_sectors);
    total_clusters /= cluster_size;

    /* Calculate the number of FAT entries per block in the FAT. If
       < 4087 clusters total, the FAT entries are 12 bits. Hence 341
       will fit; else 256 will fit.
       We add in n_fat_copies * 12 here, since it takes 12 blocks to represent
       4087 clusters in 3 nibble form. So we add in the worst case FAT size
       here to enhance the accuracy of our guess of the total clusters.
    */
    btemp = (bytepsec * 8);

    fat_size = (((2 + total_clusters) * (fasize * 4)) + btemp - 1) / btemp;

    /* Recast accounts FAT size */
    total_clusters = (UINT32) (volume_size - nreserved - root_sectors);
    total_clusters -= (n_fat_copies * fat_size);
    total_clusters /= cluster_size;

    fat_size_w = (((2 + total_clusters) * (fasize * 4)) + btemp - 1) / btemp;
    fat_size = (fat_size + fat_size_w) / 2;

    return(fat_size);
}


/************************************************************************
* FUNCTION
*
*       fat_format
*
* DESCRIPTION
*
*       Given a drive number and a format parameter block, put a FAT
*       file system on the drive.
*       The disk MUST already have a low level format. All blocks on the
*       drive should be initialized with E5's or zeros.
*
*       Some common parameters. Note: For other drive types, use debug to
*       get the parameters from block zero after FORMAT has been run.
*
*       Note: If fat_format is called with secpfat == zero, secpfat will be
*             calculated internally.
*
*
* INPUTS
*
*       dh                                  Disk handle
*       pfmt                                Format parameters
*
* OUTPUTS
*
*       NU_SUCCESS                          If the filesystem disk was
*                                            successfully initialized.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_FATCORE                         FAT cache table too small.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_BADDISK                         Bad Disk.
*       NUF_NO_PARTITION                    No partition in disk.
*       NUF_NOFAT                           No FAT type in this
*                                            partition.
*       NUF_FMTCSIZE                        Too many clusters for this
*                                            partition.
*       NUF_FMTFSIZE                        File allocation table too
*                                            small.
*       NUF_FMTRSIZE                        Numroot must be an even
*                                            multiple of 16.
*       NUF_INVNAME                         Volume label includes
*                                            invalid character.
*       NUF_NO_MEMORY                       Can't allocate internal
*                                            buffer.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS fat_format(UINT16 dh, VOID **params)
{
    FMTPARMS *pfmt;
    STATUS   ret_stat;
    UINT8    b[512], volabel[12], part_id;
    UINT32   ltotsecs, fatsecs, nclusters, ldata_area, blockno, ltemp;
    UINT16   fausize, wvalue, i, j, j_max;
#ifdef DEBUG
    UINT32   bad_cluster;
#endif

    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Start by assuming success. */
    pfmt = NU_NULL;
    fausize = 0;
    
    /* Check drive exist ( if there is check driver service ). */
    ret_stat = fs_dev_dskchk_proc(dh);

    if (ret_stat == NU_SUCCESS)
    {
        /* Set the parameter */
        pfmt = (FMTPARMS *) *params;

        if (pfmt == NU_NULL)
        {
            ret_stat = NUF_BADPARM;
        }
    }

    if (ret_stat == NU_SUCCESS)
    {
        if (!pfmt->secpalloc)
        {
            ret_stat = NUF_BADPARM;
        }
        /* The number of bytes per sector. */
        else if (pfmt->bytepsec != 512)
        {
            ret_stat = NUF_BADPARM;
        }
        else
        {
             /* Set OEM name and Volume label
                default name:
                    OEM name is NFILE3.1        Nucleus File Version 3.X
                    Volume label is NO NAME     */
            if (pfmt->oemname[0] == '\0')
                pc_cppad((UINT8 *)pfmt->oemname, (UINT8 *)"NFILE3.1", 8);

            NUF_Memfill(&volabel[0], 12, '\0');
            if (pfmt->text_volume_label[0] == '\0')
            {
                pc_cppad((UINT8 *)pfmt->text_volume_label, (UINT8 *)"NO NAME    ", 11);
            }
            else
            {
                /* Check the Volume label */
                pfmt->text_volume_label[11] = '\0';
                if (!pc_checkpath((UINT8 *)pfmt->text_volume_label, YES))
                {
                    ret_stat = NUF_INVNAME;
                }

                for (i = 0; i < 11; i++)
                {
                    if ( (pfmt->text_volume_label[i] >= 'a') &&
                         (pfmt->text_volume_label[i] <= 'z') )
                    {
                        volabel[i] = (UINT8)('A' + pfmt->text_volume_label[i] - 'a');
                    }
                    else
                    {
                        volabel[i] = pfmt->text_volume_label[i];
                    }
                }
                pc_cppad((UINT8 *)pfmt->text_volume_label, (UINT8 *)volabel, 11);
            }
        }
    }
    if (ret_stat != NU_SUCCESS)
    {
        /* Restore the kernel state. */
        PC_FS_EXIT()
        return(ret_stat);
    }
    /* =================================================================
        Build up a Partition Boot Record(PBR)
            Note: PBR include a BIOS Parameter Block(BPB)
       =================================================================== */

    /* ============== Common parameter ============== */
    NUF_Memfill(&b[0], 512, '\0');
    b[0] = (UINT8) 0xE9;    /* Jump vector. Used to id MS-DOS disk */
    b[1] = (UINT8) 0x00;
    b[2] = (UINT8) 0x00;

    /* Copy the OEM name */
    pc_cppad(&b[3], (UINT8 *)pfmt->oemname, 8);

    /* The number of bytes per sector. */
    SWAP16((UINT16 *)&(b[0xb]), (UINT16 *) &pfmt->bytepsec);

    /* The number of sectors per cluster. */
    b[0xd] = pfmt->secpalloc;

    /* The number of File Allocation Tables. */
    b[0x10] = pfmt->numfats;

    /* The media descriptor. Values in this field are identical to standard BPB. */
    b[0x15] = pfmt->mediadesc;

    /* The number of sectors per track. */
    SWAP16((UINT16 *)&(b[0x18]), (UINT16 *)&pfmt->secptrk);

    /* The number of read/write heads on the drive. */
    SWAP16((UINT16 *)&(b[0x1a]), (UINT16 *)&pfmt->numhead);

    /* The number of hidden sectors on the drive. Disks without partition tables
       do not need hidden sectors. */
    wvalue = 0;

    if (pfmt->partdisk)
    {
        SWAP16((UINT16 *)&(b[0x1c]), (UINT16 *)&pfmt->secptrk);
    }
    else
    {
        SWAP16((UINT16 *)&(b[0x1c]),&wvalue);
    }

    /* Calculate total sectors in the volume. */
    ltotsecs = pfmt->totalsec;

    /* Now fill in 4.0 specific section of the boot block */
    if (ltotsecs > 0xffffL)
    {
        /* HUGE partition  the 3.xx totsecs field is zeroed */
        b[0x13] = 0;
        b[0x14] = 0;

        /* HUGE partition */
        SWAP32((UINT32 *)&(b[0x20]), (UINT32 *)&ltotsecs);
    }
    else
    {
        wvalue = (UINT16) ltotsecs;
        SWAP16((UINT16 *)&(b[0x13]), &wvalue);
        b[0x20] = 0;
        b[0x21] = 0;
        b[0x22] = 0;
        b[0x23] = 0;
    }

    /* File System Type. */
    {
        UINT16 part_type;
        UINT32 size, offset;
        UINT8 offset_units;
        CHAR devname[FS_DEV_MAX_DEVNAME];

        fsdh_dh_to_devname(dh, devname);
        ret_stat = NU_Get_Partition_Info(devname, &part_type, &size, &offset, &offset_units, &part_id);
    }

    if(ret_stat == NU_SUCCESS)
    {
        if(pfmt->fat_type == FSFAT_AUTO)
        {
            /* Determine type based on size. */
            if (ltotsecs <= 0xF424) /* 32MB or smaller is FAT12 */
            {
                pfmt->fat_type = FSFAT_12;
            }
            else if (ltotsecs < 0x3FFD40) /* 32MB <= FAT16 < 2GB */
            {
                pfmt->fat_type = FSFAT_16;
            }
            else /* FAT32 >= 2GB */
            {
                pfmt->fat_type = FSFAT_32;
            }
        }
    }

    if( ret_stat == NU_SUCCESS )
    {
        /* Check the fat type against the partition type id. If the disk does
           not have a partition table, then ignore part_id */
        switch (pfmt->fat_type)
        {
            case FSFAT_12:
            {
                if ((part_id == 0x01) ||
                    (pfmt->partdisk == 0))
                {
                    pc_cppad(&(b[0x36]), (UINT8 *)"FAT12   ", 8);
                    fausize = 3;
                }
                else
                    ret_stat = NUF_NOFAT;
                break;
            }
            case FSFAT_16:
            {
                if((part_id == 0x04) ||
                   (part_id == 0x06) ||
                   (part_id == 0x0E) ||
                   (pfmt->partdisk == 0))
                {
                    pc_cppad(&(b[0x36]), (UINT8 *)"FAT16   ", 8);
                    fausize = 4;
                }
                else
                    ret_stat = NUF_NOFAT;
                break;
            }
            case FSFAT_32:
            {
                if((part_id == 0x0B) ||
                   (part_id == 0x0C) ||
                   (pfmt->partdisk == 0))
                {
                    pc_cppad(&(b[0x52]), (UINT8 *)"FAT32   ", 8);
                    fausize = 8;
                }
                else
                    ret_stat = NUF_NOFAT;
                break;
            }
            default:
                break;
        }
    }

    if(ret_stat != NU_SUCCESS)
    {
        pc_report_error(PCERR_NOFAT);
        /* Restore the kernel state */
        PC_FS_EXIT()
        return(ret_stat);
    }

    /* ============== FAT16/FAT12 parameter ============== */
    if (fausize <= 4)
    {
        /* The number of reserved sectors, beginning with sector 0.
            Note: Main FAT start sector number. There is always 1 sector. */
        SWAP16((UINT16 *)&(b[0xe]), (UINT16 *)&pfmt->secreserved);

        /* The number of dirents in root */
        SWAP16((UINT16 *)&(b[0x11]), (UINT16 *)&pfmt->numroot);

        /* Boot Drive Information
            80h     Boot Drive.
            00h     Not boot drive */
        b[0x24] = pfmt->physical_drive_no;

        /* Extended boot signature */
        b[0x26] = 0x29;

        /* Volume ID or Serial Number */
        SWAP32((UINT32 *)&(b[0x27]), (UINT32 *)&pfmt->binary_volume_label);

        /* Volume Label */
        pc_cppad(&(b[0x2b]), (UINT8 *)pfmt->text_volume_label, 11);
    }

    /* ============== Only FAT32 parameter ============== */
    /* FAT32
        FAT32 BPB is an extended version of the FAT16/FAT12 BPB.
        Note:
         offset Description
          11h   Number of dirents in root. This field is ignored on FAT32 drives.
          28h   Activate FAT.   Always this value set zero.
                Bit 15-08   Reserved
                       07   Mask indicating FAT mirroring state. 0:enable 1:disabled
                    06-04   Reserved
                    03-00   The number of Activate FAT.
          2Ah   File System Version.    Always set this value to zero. */
    else
    {
        /* The number of reserved sectors, beginning with sector 0.
            Note: Main FAT start sector number. */
        SWAP16((UINT16 *)&(b[0xe]), (UINT16 *)&pfmt->secreserved);

        /* The cluster number of the root directory first cluster. */
        b[0x2c] = 0x02;           /* Always 02 cluster. */

        /* The sector number of the FSINFO(File System INFOrmation) sector. */
        b[0x30] = 0x01;           /* Always 01 sector. */

        /* The sector number of the backup boot sector. */
        b[0x32] = 0x06;           /* Always 06 sector */

        /* Boot Drive Information
            80h     Boot Drive.
            00h     Not boot drive */
        b[0x40] = pfmt->physical_drive_no;

        /* Extended Boot Signature */
        b[0x42] = 0x29;

        /* Volume ID or Serial Number */
        SWAP32((UINT32 *)&(b[0x43]), (UINT32 *)&pfmt->binary_volume_label);

        /* Volume Label */
        pc_cppad(&(b[0x47]), (UINT8 *)pfmt->text_volume_label, 11);

    }

    /* if secpfat was not provided calculate it here */
    fatsecs = pc_fat_size(pfmt->bytepsec, pfmt->secreserved, pfmt->secpalloc,
                          pfmt->numfats, pfmt->numroot, ltotsecs, fausize);

    /* Sectors per fat */
    if (fausize <= 4)
    {
        wvalue=  (UINT16)fatsecs;
        SWAP16((UINT16 *)&(b[0x16]), &wvalue);
    }
    else
        SWAP32((UINT32 *)&(b[0x24]), &fatsecs);

    /* Signature word */
    wvalue = 0xAA55;
    SWAP16((UINT16 *)&(b[0x1fe]), &wvalue);
    /* ============== End of PBR set parameter ============== */


    /* Count the size of the area managed by the FAT. */
    /* Calculate max cluster number */
    ldata_area = ltotsecs;
    ldata_area -= pfmt->numfats * fatsecs;
    ldata_area -= pfmt->secreserved;

    /* Note: numroot must be an even multiple of INOPBLOCK.
             FAT32 format Root directory entries(pfmt->numroot) are ignored. */
    if (fausize <= 4)
        ldata_area -= pfmt->numroot/INOPBLOCK;

    /* Nibbles/fat entry if < 4087 clusters then 12 bit, else 16 */
    ltemp =  ldata_area/pfmt->secpalloc;
    if (fausize == 3)       /* FAT12 */
    {
        if (ltemp > MAX_FAT12_CLUSTERS) /* 0x0ff6L */
        {
            pc_report_error(PCERR_FMTCSIZE);
            /* Restore the kernel state */
            PC_FS_EXIT()
            return(NUF_FMTCSIZE);
        }
        else
        {
            nclusters = (UINT16) ltemp;
        }
    }
    else if (fausize == 4)      /* FAT16 */
    {
        if (ltemp > MAX_FAT16_CLUSTERS) /* 0xfff5L */
        {
            pc_report_error(PCERR_FMTCSIZE);
            /* Restore the kernel state */
            PC_FS_EXIT()
            return(NUF_FMTCSIZE);
        }
        else
        {
            nclusters = (UINT16) ltemp;
        }
    }
    else    /* FAT32 */
    {
        if (ltemp > MAX_FAT32_CLUSTERS) /* 0x0ffffff5L */
        {
            pc_report_error(PCERR_FMTCSIZE);
            /* Restore the kernel state */
            PC_FS_EXIT()
            return(NUF_FMTCSIZE);
        }
        else
        {
            nclusters = ltemp;
        }
    }

    /* Check the FAT.
    if ( (nibbles needed) > (nibbles if fatblocks)
            trouble;
    */
    {
    INT32 ltotnibbles;
    INT32 lnibsinfatbls;

        /* Total nibbles = (# clusters * nibbles/cluster) */
        ltotnibbles = nclusters;
        ltotnibbles *= fausize;

        /* How many nibbles are available. */
        lnibsinfatbls = fatsecs;
        lnibsinfatbls <<= 10;            /* 1024 nibbles/block */
        if (ltotnibbles > lnibsinfatbls)
        {
            pc_report_error(PCERR_FMTFSIZE);
            /* Restore the kernel state */
            PC_FS_EXIT()
            return(NUF_FMTFSIZE);
        }
    }

    /* Check the root directory entries. */
    if (pfmt->numroot % INOPBLOCK)
    {
        pc_report_error(PCERR_FMTRSIZE);
        /* Restore the kernel state */
        PC_FS_EXIT()
        return(NUF_FMTRSIZE);
    }
    /* ============== Update the boot sector(PBR) ============== */
    if (fs_dev_io_proc(dh, 0L, &(b[0]), (UINT16) 1, NO) != NU_SUCCESS )
    {
        pc_report_error(PCERR_FMTWBZERO);
        /* Restore the kernel state */
        PC_FS_EXIT()

        return(NUF_IO_ERROR);
    }

    /* Update the backup boot sector and FSINFO(File System INFOrmation).
       Only the FAT32 file system. */
    if (fausize == 8)       /* FAT32 */
    {
        /* ============== Update the backup boot sector ============== */
        if (fs_dev_io_proc(dh, (UINT32) 6L, &(b[0]), (UINT16) 1, NO) != NU_SUCCESS )
        {
            pc_report_error(PCERR_FMTWBZERO);
            /* Restore the kernel state */
            PC_FS_EXIT()

            return(NUF_IO_ERROR);
        }

        /* ===================================================================
            Build up a Partition FSINFO(File System INFOrmation)
                Note: PBR include a BIOS Parameter Block(BPB)
           =================================================================== */
        NUF_Memfill(&b[0], 512, '\0');

        /* FSI signature offset 0 */
        pc_cppad(&b[0], (UINT8 *)"RRaA", 4);

        /* FSI signature offset 0x1E0 */
        pc_cppad(&b[0x1e4], (UINT8 *)"rrAa", 4);

        /* The count of free clusters on the drive.
            Note: Total data clusters =
            data area total sectors / sector per cluster - 1(root directory clusters) */
        ltemp = nclusters - 1L;        /* root directory cluster */
        SWAP32((UINT32 *)&b[0x1e8], (UINT32 *)&ltemp);

        /* The cluster number of the cluster that was most recently allocated.
            Always 02 cluster. This cluster is root directory's first cluster.
            0xFFFFFFFF  when the count is unknown.*/
        ltemp = 2L;
        SWAP32((UINT32 *)&b[0x1ec], (UINT32 *)&ltemp);

        /* Signature word */
        wvalue = 0xAA55;
        SWAP16((UINT16 *)&(b[0x1fe]), &wvalue);

        /* ============== Update FSINFO ============== */
        /* Update
            first   FSINFO(File System INFOrmation)     offset 1 sector.
            next    Backup FSINFO.                      offset 7 sector. */
        ltemp = 1L;
        for (i = 0; i < 2; i++)
        {
            if (fs_dev_io_proc(dh, ltemp, &(b[0]), (UINT16) 1, NO) != NU_SUCCESS )
            {
                pc_report_error(PCERR_FMTWBZERO);
                /* Restore the kernel state */
                PC_FS_EXIT()

                return(NUF_IO_ERROR);
            }
            ltemp += 6L;
        }
    }
    /* ============== Now write the FATs out ============== */
    /* The first byte of a FAT media descriptor */
    /* Get the FAT media descriptor value */
    if ( fs_dev_io_proc(dh, (UINT32) pfmt->secreserved, &(b[0]), (UINT16) 1, YES) != NU_SUCCESS )
    {
        pc_report_error(PCERR_FMTWFAT);
        /* Restore the kernel state */
        PC_FS_EXIT()
        return(NUF_IO_ERROR);
    }
    /* un-formatted disk format */
    for (i = 0; i < pfmt->numfats; i++)
    {
        NUF_Memfill(&b[0], 512, '\0');
        b[0] = pfmt->mediadesc;
        b[1] = (UINT8) 0xff;
        b[2] = (UINT8) 0xff;
        if (fausize == 4)
        {
            b[3] = (UINT8) 0xff;
        }
        else if (fausize == 8)
        {
            b[3] = (UINT8) 0x0f;
            ltemp = (UINT32)0x0fffffff;
            SWAP32((UINT32 *)&b[4], &ltemp);
            SWAP32((UINT32 *)&b[8], &ltemp); /* root directory */
        }
        blockno = pfmt->secreserved + (i * fatsecs);
        for (j = 0; j < fatsecs; j++)
        {
            /* WRITE */
            if ( fs_dev_io_proc(dh, blockno, &(b[0]), (UINT16) 1, NO) != NU_SUCCESS )
            {
                pc_report_error(PCERR_FMTWFAT);
                /* Restore the kernel state */
                PC_FS_EXIT()
                return(NUF_IO_ERROR);
            }
            blockno += 1;
            if (j == 0)
                NUF_Memfill(&b[0], 512, '\0');
        }
    }
    /* ============== Now write the root directory ============== */
    blockno = (UINT32) pfmt->secreserved + pfmt->numfats * fatsecs;
    NUF_Memfill(&b[0], 512, '\0');
    if (volabel[0] != '\0')
    {   /* Create Volume Label entry as first entry. */
        DATESTR     crdate;
    
        pc_cppad((UINT8 *)&b[0x0], (UINT8 *)volabel, 11); /* name */
        b[0x0B] = 0x08; /* attribute */
        pc_getsysdate(&crdate);
        b[0x0D] = crdate.cmsec; /* fine resolution */
        SWAP16((UINT16 *)&(b[0x0E]), &crdate.time); /* create time */
        SWAP16((UINT16 *)&(b[0x10]), &crdate.date); /* create date */
        SWAP16((UINT16 *)&(b[0x12]), &crdate.date); /* last access date */
        SWAP16((UINT16 *)&(b[0x16]), &crdate.time); /* modfied time */
        SWAP16((UINT16 *)&(b[0x18]), &crdate.date); /* modified date */
    }
    if (fausize <= 4)
        j_max = pfmt->numroot / INOPBLOCK;  /* FAT16/12 */
    else
        j_max = pfmt->secpalloc;            /* FAT32 */
    for (j = 0; j < j_max; j++)
    {
        if (fs_dev_io_proc(dh, blockno, &(b[0]), (UINT16) 1, NO) != NU_SUCCESS)
        {
            pc_report_error(PCERR_FMTWROOT);
            ret_stat = NUF_IO_ERROR;
            break;
        }
        if (j == 0) /* Only write volume label once. */
            NUF_Memfill(&b[0], 32, '\0');
        blockno += 1L;
    }
    PC_FS_EXIT()
    return(ret_stat);
}

/************************************************************************
* FUNCTION
*
*       fat_get_format_info
*
* DESCRIPTION
*
*       Given a drive number, read the disk information into a format
*       parameter block.
*
*       Some common parameters. Note: For other drive types, use debug to
*       get the parameters from block zero after FORMAT has been run.
*
* INPUTS
*
*       dh                                  Disk handle
*       pfmt                                Format parameters
*
* OUTPUTS
*
*       NU_SUCCESS                          If the format parameter was
*                                            successfully initialized.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_FATCORE                         FAT cache table too small.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_BADDISK                         Bad Disk.
*       NUF_NO_PARTITION                    No partition in disk.
*       NUF_NOFAT                           No FAT type in this
*                                            partition.
*       NUF_FMTCSIZE                        Too many clusters for this
*                                            partition.
*       NUF_FMTFSIZE                        File allocation table too
*                                            small.
*       NUF_FMTRSIZE                        Numroot must be an even
*                                            multiple of 16.
*       NUF_INVNAME                         Volume label includes
*                                            invalid character.
*       NUF_NO_MEMORY                       Can't allocate internal
*                                            buffer.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS fat_get_format_info(UINT16 dh, VOID **params)
{
STATUS ret_stat;
FDEV_S *phys_fdev;
FSDH_S *dhs;
CHAR phys_dev_name[FPART_MAX_PHYS_NAME];
PFPART_LIST_S plist, list_head = NU_NULL;
FPART_DISK_INFO_S disk_info;
UINT16 /*sec_p_cyl,*/ start_cyl, end_cyl;
FMTPARMS *pfmt;

    pfmt = NU_NULL;
    plist = NU_NULL;

    if(*params != NU_NULL)
    {
        /* Cast the parameter structure */
        pfmt = (FMTPARMS *)*params;

        /* Clear the structure */
        NUF_Memfill(pfmt, sizeof(FMTPARMS), 0);

        /* Get a disk handle struct */
        ret_stat = fsdh_get_fsdh_struct(dh, &dhs);
    }
    else
    {
        ret_stat = NUF_BADPARM;
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Parse the physical name from the logical name */
        NUF_Copybuff(phys_dev_name, dhs->fsdh_disk_name, FPART_MAX_PHYS_NAME);

        phys_dev_name[FPART_MAX_PHYS_NAME - 1] = NU_NULL;

        /* Get the device table entry */
        ret_stat = fs_dev_devname_to_fdev(phys_dev_name, &phys_fdev);
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Clear the structure */
        NUF_Memfill(&disk_info, sizeof(FPART_DISK_INFO_S), 0);

        /* Get the device specific information */
        ret_stat = fs_dev_ioctl_proc(phys_fdev->fdev_dh, FDEV_GET_DISK_INFO, (VOID *)&disk_info, sizeof(FPART_DISK_INFO_S));
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Calculate the number of sectors per cylinder */
        if (disk_info.fpart_cyls != 0)
        {
            /*sec_p_cyl = disk_info.fpart_tot_sec / disk_info.fpart_cyls;*/
        }
        else
        {
            ret_stat = NUF_INTERNAL; /* invalid disk info */
        }
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Get a list of all partitions on the physical device */
        ret_stat = NU_List_Partitions(phys_dev_name, &list_head);
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Set the ret_status to an error. */
        ret_stat = NUF_INVALID_DEVNAME;     /* Could not find partition requested */

        /* Find the specific partition by matching the device name */
        for(plist = list_head; plist != NU_NULL; plist = plist->fpart_next)
        {
            if(NUF_Strncmp(plist->fpart_name, dhs->fsdh_disk_name, FPART_MAX_LOG_NAME) == NU_SUCCESS)
            {
                /* Found the matching partition */
                ret_stat = NU_SUCCESS;
                break;
            }
        }
    }

    if(ret_stat == NU_SUCCESS)
    {
        /* ============== User setting format parameters ============== */
        pfmt->oemname[0] = '\0';
        pfmt->binary_volume_label = (UINT32) 0;
        pfmt->text_volume_label[0] = '\0';


        /* ============== Common fixed format parameters ============== */
        /* The number of bytes per sector. */
        pfmt->bytepsec  = (UINT16) disk_info.fpart_bytes_p_sec;
        
        if(FPART_DI_RAMDISK == (disk_info.fpart_flags & FPART_DI_RAMDISK))
        {
            /* Ramdisks have a "special" configuration */
            pfmt->numfats = 1;          /* The number of File Allocation Tables. 1 
                                           for Ram disk to waste less space*/
            pfmt->mediadesc = 0xFA;     /* The media descriptor. Ram disk, any size. */
        }
        else
        {           
            pfmt->numfats = 2;          /* The number of File Allocation Tables. */
            pfmt->mediadesc = 0xF8;     /* The media descriptor. Hard disk, any size. */
        }

        /* Physical Drive Number: This is related to the BIOS physical drive number.
           Floppy drives are numbered starting with 0x00 for the A: drive, while
           physical hard disks are numbered starting with 0x80. Typically, you
           would set this value prior to issuing an INT 13 BIOS call in order to
           specify the device to access. The on-disk value stored in this field is
           typically 0x00 for floppies and 0x80 for hard disks, regardless of how
           many physical disk drives exist, because the value is only relevant if
           the device is a boot device.*/
        pfmt->physical_drive_no = 0x80;

        /* Number of heads, sectors. */
        pfmt->numhead = plist->fpart_ent.fpart_e_head + 1;
        pfmt->secptrk = plist->fpart_ent.fpart_e_sec & 0x3F;

        /* Number of cylinders */
        fpart_cyl_read(plist->fpart_ent, &start_cyl, &end_cyl);
        pfmt->numcyl = end_cyl - start_cyl + 1;

        /* Set the partition total sectors */
        pfmt->totalsec = plist->fpart_ent.fpart_size;

        /* Set the partition disk flag */
        if (plist->fpart_ent.fpart_boot == 0xFF)
        {
            /* Artificial partition entries are created for removable
               media that do not have a partition table. These entries
               can be identified by the 0xFF value for bootable flag
               (normally only valid as 0 or 1). */
            pfmt->partdisk = 0;
        }
        else
        {
            pfmt->partdisk = 1;
        }
    }

    /* ============== Build up the FAT32 format parameters ============== */
    if ((ret_stat == NU_SUCCESS) &&
        ((plist->fpart_ent.fpart_id == 0x0B) ||
         (plist->fpart_ent.fpart_id == 0x0C) ||
         ((pfmt->partdisk == 0) && (pfmt->totalsec >= 0x400000))))
    {
        /* Set the fat type */
        pfmt->fat_type = FSFAT_32;

        /* The number of reserved sectors, beginning with sector 0.
           Note: Main FAT start sector number.
                 Always 32 sectors.(FAT32)  */
        pfmt->secreserved = 32;

        /*  Number of dirents in root. This field is ignored on FAT32 drives */
        pfmt->numroot = 0;

        /* Set the sector per cluster (smallest allocation unit).
           MiB = 2^20B; GiB = 2^30B; TiB = 2^40B
           totalsec = xMiB * 2^20 / 512
           Logical disk size   Sectors per cluster
              33MiB -  64MiB              1
              65MiB - 128MiB              2
             129MiB - 256MiB              4
             257MiB -   8GiB              8
               8GiB -  16GiB              16
              16GiB -  32GiB              32
              32GiB -   2TiB              64                  */

        if(pfmt->totalsec < 0x10800)        /* < 33MiB */
        {
            ret_stat = NUF_IDE_DISK_SIZE;
        }
        else if(pfmt->totalsec < 0x20000)   /* < 64MiB */
        {
            pfmt->secpalloc = (UINT8) 1;
        }
        else if(pfmt->totalsec < 0x40000)   /* < 128MiB */
        {
            pfmt->secpalloc = (UINT8) 2;
        }
        else if(pfmt->totalsec < 0x80000)   /* < 256MiB */
        {
            pfmt->secpalloc = (UINT8) 4;
        }
        else if(pfmt->totalsec < 0x1000000) /* < 8GiB */
        {
            pfmt->secpalloc = (UINT8) 8;
        }
        else if(pfmt->totalsec < 0x2000000) /* < 16GiB */
        {
            pfmt->secpalloc = (UINT8) 16;
        }
        else if(pfmt->totalsec < 0x4000000) /* < 32GiB */
        {
            pfmt->secpalloc = (UINT8) 32;
        }
        else /* if(pfmt->totalsec < 0x20000000000)*/ /* < 2TiB */
        {
            pfmt->secpalloc = (UINT8) 64;
        }
#if 0   /* 64bit totalsec not supported by Windows. 64 sectors per cluster
           is the default for >=32GiB */
        else
        {
            /* Although the theoretical limit for FAT32 is 8TiB,
               it is only supported up to 2TiB */
            ret_stat = NUF_IDE_DISK_SIZE;
        }
#endif

    }

    /* ============== Build up the FAT16/12 format parameters ============== */
    else if (ret_stat == NU_SUCCESS)
    {
        /* The number of reserved sectors, beginning with sector 0.
            Note: Main FAT start sector number == Number of Boot sectors
                  Always 1 sectors(FAT16/12)  */
        pfmt->secreserved = 1;

        /* Disk FAT16/12 format is always 512 entries. */
        if(FPART_DI_RAMDISK == (disk_info.fpart_flags & FPART_DI_RAMDISK))
        {
            pfmt->numroot = 64;
        }
        else
        {
            pfmt->numroot = 512;
        }

        /* Set the sector per cluster.
           MiB = 2^20B; GiB = 2^30B; TiB = 2^40B
           totalsec = #MiB * 2^20 / 512
           Logical disk size      Sectors per cluster
                0 -   32MiB                1
               33 -   64MiB                2
               65 -  128MiB                4
              129 -  256MiB                8
              257 -  512MiB               16
              513 - 1024MiB               32
             1025 - 2048MiB               64
             2049 - 4096MiB              128 */

        /* == FAT12 == */
        if ((plist->fpart_ent.fpart_id == 0x01) ||
            ((pfmt->partdisk == 0) && (pfmt->totalsec <= 0xF424)))
        {
            /* Set the fat type */
            pfmt->fat_type = FSFAT_12;

            if (pfmt->totalsec < (MAX_FAT12_CLUSTERS * 2))    /* < 4 MB */
            {
                pfmt->secpalloc = (UINT8)2;
            }
            else if (pfmt->totalsec < (MAX_FAT12_CLUSTERS * 4))    /* < 8 MB */
            {
                pfmt->secpalloc = (UINT8)4;
            }
            else if (pfmt->totalsec < (MAX_FAT12_CLUSTERS * 8))    /* < 16 MB */
            {
                pfmt->secpalloc = (UINT8)8;
            }
            else if (pfmt->totalsec <= (MAX_FAT12_CLUSTERS * 16))    /* < 32 MB */
            {
                pfmt->secpalloc = (UINT8)16;
            }
            else
            {
                /* Invalid size */
                ret_stat = NUF_IDE_DISK_SIZE;
            }
        }

        /* == FAT16 == */
        else if ( (plist->fpart_ent.fpart_id == 0x04) ||
                  (plist->fpart_ent.fpart_id == 0x06) ||
                  (plist->fpart_ent.fpart_id == 0x0E) ||
                  (pfmt->partdisk == 0))
        {
            /* Set the fat type */
            pfmt->fat_type = FSFAT_16;

			if (pfmt->totalsec < MAX_FAT16_CLUSTERS)  /* < 32MB */
            {
                pfmt->secpalloc = (UINT8)1;
            }
            else if (pfmt->totalsec < (MAX_FAT16_CLUSTERS * 2))  /* < 64MB */
            {
                pfmt->secpalloc = (UINT8)2;
            }
            else if (pfmt->totalsec < (MAX_FAT16_CLUSTERS * 4))  /* < 128MB */
            {
                pfmt->secpalloc = (UINT8)4;
            }
            else if (pfmt->totalsec < (MAX_FAT16_CLUSTERS * 8))  /* < 256MB */
            {
                pfmt->secpalloc = (UINT8)8;
            }
            else if (pfmt->totalsec < (MAX_FAT16_CLUSTERS * 16)) /* < 512MB */
            {
                pfmt->secpalloc = (UINT8)16;
            }
            else if (pfmt->totalsec < (MAX_FAT16_CLUSTERS * 32)) /* < 1GB */
            {
                pfmt->secpalloc = (UINT8)32;
            }
            else if (pfmt->totalsec < (MAX_FAT16_CLUSTERS * 64)) /* < 2GB */
            {
                pfmt->secpalloc = (UINT8)64;
            }
            else if (pfmt->totalsec < (MAX_FAT16_CLUSTERS * 128)) /* < 4GB */
            {
                pfmt->secpalloc = (UINT8)128;
            }
            else
            {
                /* Invalid size */
                ret_stat = NUF_IDE_DISK_SIZE;
            }
        }
        else
        {
            /* The fat type was not in the partition record.
               Well calculate it in format. */
            pfmt->fat_type = FSFAT_AUTO;

            ret_stat = NUF_IDE_FAT_TYPE; /* Invalid partition type id */
        }
    }

    NU_Free_Partition_List(list_head);

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       fat_freespace
*
* DESCRIPTION
*
*       Given a path containing a valid drive specifier count the number
*       of free clusters, sector per cluster, bytes per sector and number
*       of total clusters on the drive.
*
*
* INPUTS
*
*       dh                                  Disk handle
*       secpcluster                         Sector per cluster
*       bytepsec                            Bytes per sector
*       freecluster                         Number of free clusters
*       totalcluster                        Number of total clusters
*
* OUTPUTS
*
*       NU_SUCCESS                          If service is successful.
*                                           Returns the sector per
*                                            cluster, bytes per sector,
*                                            number of free clusters and
*                                            number of total clusters.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NOT_OPENED                      Drive not opened.
*       NUF_NO_DISK                         Disk is removed.
*
*************************************************************************/
INT fat_freespace(UINT16 dh, UINT8 *secpcluster, UINT16 *bytepsec,
                   UINT32 *freecluster, UINT32 *totalcluster)
{
STATUS      ret_stat;
FAT_CB      *fs_cb = NU_NULL;
DDRIVE      *pdr;

    /* Must be last line in declarations */
    PC_FS_ENTER()

    /* Initialize local */
    pdr = NU_NULL;

    ret_stat =  fsdh_get_fs_specific(dh, (VOID**)&fs_cb);
    if (ret_stat == NU_SUCCESS)
    {
        /* Setup DDRIVE pointer */
        pdr = fs_cb->ddrive;

        /* Verify DDRIVE has been configured */
        if (!pdr)
            ret_stat = NUF_NOT_OPENED;
    }

    /* Verify the disk exists */
    if (ret_stat == NU_SUCCESS)
    {
        /* Grab exclusive access to the drive. */
        PC_DRIVE_ENTER(dh, YES)

        if (fs_dev_dskchk_proc(dh) != NU_SUCCESS)
            ret_stat = NUF_NO_DISK;

        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(dh)
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Sector per cluster */
        *secpcluster = pdr->secpalloc;

        /* Bytes per sector */
        *bytepsec = pdr->bytspsector;

        /* Number of total clusters */
        *totalcluster = pdr->maxfindex - 1;

        /* Grab exclusive access to the drive */
        PC_DRIVE_ENTER(dh, YES)

        /* Number of free clusters */
        *freecluster = pc_ifree(dh);

        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(dh)
    }

    /* Restore the kernel state */
    PC_FS_EXIT()

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       fat_get_first
*
* DESCRIPTION
*
*       Given a pattern which contains both a path specifier and a
*       search pattern fill in the structure at statobj with information
*       about the file and set up internal parts of statobj to supply
*       appropriate information for calls to fat_get_next.
*
*
* INPUTS
*
*       *statobj                            Caller's buffer to put file
*                                            info.
*       *name                               Path to find
*
* OUTPUTS
*
*       NU_SUCCESS                          Search for the first match
*                                            pattern was successful.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*       NUF_NOFILE                          The specified file not found.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        Driver I/O function routine
*                                            returned error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS fat_get_first(UINT16 dh, FAT_DSTAT *statobj, CHAR *pattern)
{
STATUS      ret_stat;
VOID        *mompath;
UINT8       *filename;
VOID        *fileext;
CHAR        dot[] = ".\\*", dot_dot[] = "..\\*";
CHAR        *str_ptr;
CHAR        *pattern_ptr;

    statobj->pobj = NU_NULL;
    statobj->pmom = NU_NULL;

    /* Verify that the drive exists */
    ret_stat = fs_dev_dskchk_proc(dh);
    if (ret_stat == NU_SUCCESS)
    {
        /* Store the dh. */
        statobj->dh = dh;

        /* Special cases of "." or  ".." */
        ret_stat = NUF_GET_CHAR(&str_ptr,pattern,1);
        if ( (*pattern == '.') && (*str_ptr == '\0') && (ret_stat == NU_SUCCESS) )
        {
            pattern = dot;
        }
        else
        {
            ret_stat = NUF_GET_CHAR(&pattern_ptr,pattern,2);
            if ((*pattern == '.') && (*str_ptr == '.') && (*pattern_ptr == '\0')
                && (ret_stat == NU_SUCCESS) )
            {
                pattern = dot_dot;
            }
        }
        /* Get the path, filename and file extension. */
        ret_stat = pc_parsepath(&mompath, (VOID**)(&filename),
                                    &fileext, (UINT8 *)pattern);
        if (ret_stat != NU_SUCCESS)
        {
            ret_stat = NUF_INVNAME;
        }
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Register drive in use. */
        PC_DRIVE_ENTER(dh, NO)

        /* Save the pattern. we'll need it in fat_get_next. */
        NUF_NCPBUFF(statobj->pname,filename, EMAXPATH+1);
        if (fileext)
           NUF_NCPBUFF(statobj->pext,fileext,4);
        else
            statobj->pext[0] = (UINT8)0;
        if (mompath)
            /* Copy over the path. we will need it later. */
           NUF_NCPBUFF((UINT8 *)statobj->path, mompath, EMAXPATH+1);

        else
            statobj->path[0] = (INT8)0;

        /* Find the file and init the structure. */
        ret_stat = pc_fndnode(dh, &statobj->pmom, (UINT8 *)mompath);
        if (ret_stat == NU_SUCCESS)
            /* Found it. Check access permissions. */
        {
            if (pc_isadir(statobj->pmom))
            {
                /* Lock the finode. */
                PC_INODE_ENTER(statobj->pmom->finode, NO)
                /* Now find pattern in the directory. */
                statobj->pobj = NU_NULL;
                ret_stat = pc_get_inode(&statobj->pobj,
                                        statobj->pmom, (UINT8 *)filename);
                if (ret_stat == NU_SUCCESS)
                {
                    /* And update the stat structure. */
                    pc_upstat(statobj);
                    /* Long file name? */
                    if (statobj->pobj->linfo.lnament)
                    {
                        /* We need to clean long filename information. */
                        lnam_clean(&statobj->pobj->linfo,
                                            statobj->pobj->pblkbuff);
                        pc_free_buf( statobj->pobj->pblkbuff, NO);
                    }
                }
                /* Release exclusive use of finode. */
                PC_INODE_EXIT(statobj->pmom->finode)
            }
            else
            {
                /* if entry is not a directory, say it ! */
                ret_stat = NUF_INVNAME;
            }

            /* Find first fail, free the pmom. */
            if (ret_stat != NU_SUCCESS)
            {
                /* Free the search object. */
                pc_freeobj(statobj->pmom);
                statobj->pmom = NU_NULL;
            }
        }

        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(dh)
    }

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       fat_get_next
*
* DESCRIPTION
*
*       Given a pointer to a FAT_DSTAT structure that has been set up by a
*       call to fat_get_first(), search for the next match of the
*       original pattern in the original path. Return yes if found and
*       update statobj for subsequent calls to fat_get_next.
*
*
* INPUTS
*
*       *statobj                            Caller's buffer to put file
*                                            info.
*
* OUTPUTS
*
*       NU_SUCCESS                          Search for the next match
*                                            pattern was successful.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_NOFILE                          The specified file not found.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_IO_ERROR                        Driver I/O function routine
*                                            returned error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS fat_get_next(FAT_DSTAT *statobj)
{
STATUS      ret_stat;

    /* Do we get correct statobj? */
    if ( (!statobj) || (!statobj->pmom) )
    {
        ret_stat = NUF_BADPARM;
    }
    else
    {
        /* Verify that the drive exists */
        ret_stat = fs_dev_dskchk_proc(statobj->dh);
    }
    
    if (ret_stat == NU_SUCCESS)
    {
        /* Register drive in use. */
        PC_DRIVE_ENTER(statobj->dh, NO)
        /* Lock the finode. */
        PC_INODE_ENTER(statobj->pmom->finode, NO)

        /* Now find the next instance of pattern in the directory. */
        ret_stat = pc_get_inode(&statobj->pobj,
                                    statobj->pmom, statobj->pname);
        if (ret_stat == NU_SUCCESS)
        {
            /* And update the stat structure. */
            pc_upstat(statobj);

            /* Long filename? */
            if (statobj->pobj->linfo.lnament)
            {
                /* We need to clean long filename information. */
                lnam_clean(&statobj->pobj->linfo, statobj->pobj->pblkbuff);
                pc_free_buf(statobj->pobj->pblkbuff, NO);
            }
        }

        /* Release exclusive use of finode. */
        PC_INODE_EXIT(statobj->pmom->finode)
        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(statobj->dh)
    }
 
    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       fat_done
*
* DESCRIPTION
*
*       Given a pointer to a FAT_DSTAT structure that has been set up by a
*       call to fat_get_first(), free internal elements used by the
*       statobj.
*
*       Note: You MUST call this function when done searching through a
*             directory.
*
*
* INPUTS
*
*       *statobj                            Caller's buffer to put file
*                                            info.
*
* OUTPUTS
*
*      None.
*
*************************************************************************/
VOID fat_done(FAT_DSTAT *statobj)
{
UINT16 dh;
      
    if ( (statobj) && (statobj->pmom) )
    {   
        /* okay, release the elements */
        dh = statobj->pmom->pdrive->dh;
        
        /* Remove compiler warning. */
        FILE_Unused_Param = (UINT32) dh;
        
        /* Register drive in use. */
        PC_DRIVE_ENTER(dh, NO)
        /* Free the search object. */
        if (statobj->pobj)
            pc_freeobj(statobj->pobj);
        pc_freeobj(statobj->pmom);
        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(dh)
    }
}

/************************************************************************
* FUNCTION
*
*       fat_utime
*
* DESCRIPTION
*
*       Given a pointer to a DSTAT structure this function will set
*       fcrtime, fcrdate, faccdate, facctime, fuptime, and fupdate to
*       their corresponding parameters passed in. For example, *stateobj’s
*       faccdate member will be set to access_date, as long as access_date
*       is valid. If a parameter is 0xFFFF it will be set to the current
*       system time or date depending on the parameter. Each date is stored
*       based on the FAT32 File System Specification, which means bits 0-4
*       represent the day of the month, bits 5-8 represents the month, and
*       bits 9-15 represent the year relative to 1980. The times are also
*       stored according to the FAT32 File System Specification,which means
*       bits 0-4 represent a 2-sec count, bits 5-10 represent the minutes,
*       and bits 11-15 represent the hours.
*
*
* INPUTS
*
*       *statobj                            DSTAT object who's times
*                                           and dates should be updated
*                                           to the passed in values.
*       access_date                         New Access Date.
*       access_time                         New Access Time.
*       update_date                         New Update Date.
*       update_time                         New Update Time.
*       create_date                         New Create_Date.
*       create_time                         New Create_Time.
*
*
* OUTPUTS
*
*      NUF_BADPARM                          Bad parameter given.
*      NUF_NO_DISK                          Disk is removed.
*      NU_SUCCESS                           Dates and times successfully
*                                           updated.
*
*************************************************************************/
STATUS fat_utime(DSTAT *statobj,UINT16 access_date,UINT16 access_time,
                 UINT16 update_date,UINT16 update_time,UINT16 create_date,
                 UINT16 create_time)
{
    /* Assume success. */
    STATUS ret_val = NU_SUCCESS;
    DATESTR     fds;
    FAT_DSTAT *fdstat = NU_NULL;

    PC_FS_ENTER()

    /* Access access_time to remove compiler warning. */
    FILE_Unused_Param = (UINT32)access_time;

    /* Make sure we have a valid statobj and private file system pointer.*/
    if((statobj) && (statobj->fs_private))
    {
        fdstat = (FAT_DSTAT *) statobj->fs_private;
    }
    else
    {
        ret_val = NUF_BADPARM;
    }
    
     /* Verify that the drive exists */
    if (ret_val == NU_SUCCESS)
    {
        ret_val = fs_dev_dskchk_proc(fdstat->pmom->pdrive->dh);
    }

    if(ret_val == NU_SUCCESS)
    {
        /* Register drive in use. */
        PC_DRIVE_ENTER(fdstat->pmom->pdrive->dh, NO)
        /* Lock the finode. */
        PC_INODE_ENTER(fdstat->pmom->finode, YES)

        /* Get the system clock and date in case the user didn't manual
        update one of the parameters.*/
        pc_getsysdate(&fds);

        /* Value isn't 0xFFFF so update to value passed in. */
        if(access_date != 0xFFFF)
        {
            statobj->faccdate = access_date;
            fdstat->pobj->finode->faccdate = access_date;
            fdstat->faccdate = access_date;
        }
        /* Value is 0xFFFF so update to system date. */
        else
        {
            statobj->faccdate = fds.date;
            fdstat->faccdate = fds.date;
            fdstat->pobj->finode->faccdate = fds.date;
        }

        /* Value isn't 0xFFFF so update to value passed in. */
        if(update_date != 0xFFFF)
        {
            statobj->fupdate = update_date;
            fdstat->pobj->finode->fupdate = update_date;
            fdstat->fupdate = update_date;
        }
        /* Value is 0xFFFF so update to system date. */
        else
        {
            statobj->fupdate = fds.date;
            fdstat->pobj->finode->fupdate = fds.date;
            fdstat->fupdate = fds.date;
        }

        /* Value isn't 0xFFFF so update to value passed in. */
        if(update_time != 0xFFFF)
        {
            statobj->fuptime = update_time;
            fdstat->pobj->finode->fuptime = update_time;
            fdstat->fuptime = update_time;
        }
        /* Value is 0xFFFF so update to system clock. */
        else
        {
            statobj->fuptime = fds.time;
            fdstat->pobj->finode->fuptime = fds.time;
            fdstat->fuptime = fds.time;
        }

        /* Value isn't 0xFFFF so update to value passed in. */
        /* Otherwise just leave it to what it original was.*/
        if(create_date != 0xFFFF)
        {
            statobj->fcrdate = create_date;
            fdstat->pobj->finode->fcrdate = create_date;
            fdstat->fcrdate = create_date;
        }

        /* Value isn't 0xFFFF so update to value passed in. */
        /* Otherwise just leave it to what it original was.*/
        if(create_time != 0xFFFF)
        {
            statobj->fcrtime = create_time;
            fdstat->pobj->finode->fcrtime = create_time;
            fdstat->fcrtime = create_time;
        }

        /* Update the inode and write out to disk. */
        ret_val = pc_update_inode(fdstat->pobj,DSET_MANUAL_UPDATE);

        /* Release exclusive use of finode. */
        PC_INODE_EXIT(fdstat->pmom->finode)
        /* Release non-exclusive use of drive. */
        PC_DRIVE_EXIT(fdstat->pmom->pdrive->dh)

    }

    /* Restore the kernel state. */
    PC_FS_EXIT()

    return ret_val;
}
/************************************************************************
* FUNCTION
*
*       pc_l_pwd
*
* DESCRIPTION
*
*       Get the full path name.
*
*
* INPUTS
*
*       *path                               Path name buffer
*       *pobj                               Drive object
*
* OUTPUTS
*
*       YES                                 If getting the path name
*                                            was successful.
*       NO                                  Following  error code:
*        NUF_NOFILE                          The specified file not
*                                             found.
*        NUF_NO_BLOCK                       No block buffer available.
*        NUF_NO_FINODE                      No FINODE buffer available.
*        NUF_NO_DROBJ                       No DROBJ buffer available.
*        NUF_IO_ERROR                       Driver I/O function routine
*                                            returned error.
*        NUF_INTERNAL                       Nucleus FILE internal error.
*
*************************************************************************/
INT pc_l_pwd(UINT8 *path, DROBJ *pobj)
{
STATUS      ret_stat;
DROBJ       *pchild;
INT16       lnend;
INT16       n;
INT16       ci;
UINT8       lname[MAX_LFN+1];


    lnend = 0;

    while (YES)
    {
        /* Root directory */
        lname[lnend] = BACKSLASH;

        /* Root directory? */
        if (pc_isroot(pobj))
        {
            /* Free the current object. */
            pc_freeobj(pobj);
            break;      /* path search end. */
        }
        else
        {
            lnend++;    /* for BACKSLASH */

            /* Find '..' so we can find the parent. */
            pchild = NU_NULL;
            ret_stat = pc_get_inode(&pchild, pobj, (UINT8 *)"..");
            if (ret_stat != NU_SUCCESS)
            {
                /* Free the current object. */
                pc_freeobj(pobj);
                return(NO);
            }

            /* Free the current object. */
            pc_freeobj(pobj);

            /* Get the parent directory. */
            ret_stat = pc_get_mom(&pobj, pchild);
            if (ret_stat != NU_SUCCESS)
            {
                /* Free the child object. */
                 pc_freeobj(pchild);
                 return(NO);
            }

            /* Get the name of the current directory by searching the
               parent for an inode with cluster matching the cluster
               value in ".." */
            if ( !pc_gm_name(&lname[lnend], pobj, pchild) )
            {
                /* Free the child and current object. */
                pc_freeobj(pchild);
                pc_freeobj(pobj);
                return(NO);
            }

            /* Free the child  object. */
            pc_freeobj(pchild);

            /* Measure path name length. */
            for (; *(lname + lnend); lnend++){;}
        }
    }

    /* Path string set up the right order. */
    *path++ = BACKSLASH;    /* first BACKSLASH is root. */
    for (n = lnend-1; n >= 0; n--)
    {
        /* Is current name is slash? */
        if (lname[n] == '\\')
        {
            for (ci = n+1; ci < lnend; ci++)
                *path++ = lname[ci];

            *path++ = BACKSLASH;
            lnend = n;  /* Set BACKSLASH index. */
        }
    }
    *path = '\0';

    return(YES);
}

/************************************************************************
* FUNCTION
*
*       pc_gm_name
*
* DESCRIPTION
*
*       Check the directory entry links.
*
*
* INPUTS
*
*       *path                               Output path pointer
*       *parent_obj                         Parent object
*       *pdotdot                            ".." entry drive object
*
* OUTPUTS
*
*       YES                                 If the check path was
*                                            successful.
*       NO                                  Following  error code:
*        NUF_NOFILE                          The specified file not
*                                             found.
*        NUF_NO_FINODE                      No FINODE buffer available.
*        NUF_NO_DROBJ                       No DROBJ buffer available.
*        NUF_IO_ERROR                       Driver I/O function routine
*                                            returned error.
*        NUF_INTERNAL                       Nucleus FILE internal error.
*
*************************************************************************/
INT pc_gm_name(UINT8 *path, DROBJ *parent_obj, DROBJ *pdotdot)
{
STATUS      ret_stat;
DROBJ       *pchild;
LNAMINFO    *linfo;
DOSINODE    pi;
UINT32      clusterno;
UINT32      fcluster;
INT         ret_val;


    ret_val = NO;

    /* Convert sector to cluster. */
    clusterno = pc_sec2cluster(pdotdot->pdrive, pdotdot->blkinfo.my_frstblock);

    /* Now find pattern in the directory. */
    pchild = NU_NULL;
    ret_stat = pc_get_inode(&pchild, parent_obj, (UINT8 *)"*");
    if (ret_stat == NU_SUCCESS)
    {
        for (;;)
        {
            fcluster = pchild->finode->fcluster;
            if ( (pchild->finode->file_delete != PCDELETE) && (fcluster == clusterno) )
             {
                /* Setup the filename */

                /* get long filename info */
                linfo = &pchild->linfo;
                if (linfo->lnament)         /* Long filename */
                {
                    /* Convert directory entry long filename to character long filename. */
                    pc_cre_longname((UINT8 *)path, linfo);
                }
                else        /* Short file name */
                {
                    pc_ino2dos(&pi, pchild->finode,pchild->pdrive->dh);

                    /* Convert directory entry short filename to character short filename. */
                    pc_cre_shortname((UINT8 *)path, pi.fname, pi.fext);
                }
                ret_val = YES;
                break;
            }

            if (pchild->linfo.lnament)
            {
                /* We need to clean long filename information */
                lnam_clean(&pchild->linfo, pchild->pblkbuff);
                pc_free_buf(pchild->pblkbuff, NO);
            }

            ret_val = pc_get_inode(&pchild, parent_obj, (UINT8 *)"*");
            if (ret_val != NU_SUCCESS)
            {
                ret_val = NO;
                break;
            }
        }
        /* Free the child  object. */
        pc_freeobj(pchild);
    }

    return(ret_val);
}
