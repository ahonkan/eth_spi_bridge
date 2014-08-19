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
*       fs_dir.c
*
* COMPONENT
*
*       Directory
*
* DESCRIPTION
*
*       Contains API for directory and directory object operations.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Make_Dir
*       NU_Remove_Dir
*       NU_Get_First
*       NU_Get_Next
*       NU_Done
*       NU_Utime                Sets file time and date.
*
*************************************************************************/
#include "storage/pcdisk.h"
#include "storage/lck_extr.h"
#include "storage/dir_defs.h"
#include "storage/fsl_extr.h"
#include "storage/user_extr.h"
#include "storage/dh_extr.h"
#include "services/nu_trace_os_mark.h"

STATIC STATUS check_date(const UINT16 *date);
STATIC STATUS check_time(const UINT16 *time);

/************************************************************************
* FUNCTION
*
*       NU_Make_Dir
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
STATUS NU_Make_Dir(CHAR *name)
{
STATUS      ret_stat;
MTE_S       *mte;
UINT8       user_state;

    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_stat = CHECK_USER()) == NU_SUCCESS)
    {
        if (!name)
            ret_stat = NUF_BADPARM;
        else
        {
            /* Resolve mount table entry from name */
            mte = fsl_mte_from_name(name);
            if (mte)
            {
                /* Get the current user state for the disk */        
                if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
                {
                    /* Verify disk is opened */
                    if (user_state == FSDH_USER_OPENED)
                    {
                        /* Dispatch FS specific routine */
                        ret_stat = mte->mte_fs->fs_mkdir(name);
                    }
                    else
                        ret_stat = NUF_NOT_OPENED;
                }
                else
                    ret_stat = NUF_BAD_USER;
                    
                /* Trace log */
                T_DIR_CREATE_INFO(mte->mte_mount_name, mte->mte_device_name, name);
            }                        
            else
                ret_stat = NUF_BADDRIVE;
        }
        
        /* Trace log */
        T_DIR_CREATE_STAT(name, ret_stat);
    }
    LCK_FS_EXIT()
    return(ret_stat);
}

/************************************************************************
* FUNCTION
*
*       NU_Remove_Dir
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
STATUS NU_Remove_Dir(CHAR *name)
{
    STATUS      ret_stat;
    MTE_S       *mte;
    UINT8       user_state;
    
    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_stat = CHECK_USER()) == NU_SUCCESS)
    {
        /* Verify parameter */
        if (!name)
            ret_stat = NUF_BADPARM;
        else
        {
            /* Resolve mount table entry from name */
            mte = fsl_mte_from_name(name);

            if(mte)
            {
                /* Get the current user state for the disk */        
                if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
                {
                    /* Verify disk is opened */
                    if (user_state == FSDH_USER_OPENED)
                    {
                        /* Dispatch FS specific routine */
                        ret_stat = mte->mte_fs->fs_rmdir(name);
                    }
                    else
                        ret_stat = NUF_NOT_OPENED;
                }
                else
                    ret_stat = NUF_BAD_USER;
            }                        
            else
                ret_stat = NUF_BADDRIVE;
        }
        
        /* Trace log */
        T_DIR_DEL_STAT(name, ret_stat);
    }
    LCK_FS_EXIT()
    return(ret_stat);
}

/************************************************************************
* FUNCTION
*
*       NU_Get_First
*
* DESCRIPTION
*
*       Given a pattern which contains both a path specifier and a
*       search pattern fill in the structure at statobj with information
*       about the file and set up internal parts of statobj to supply
*       appropriate information for calls to NU_Get_Next.
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
STATUS NU_Get_First(DSTAT *statobj, CHAR *pattern)
{
    STATUS      ret_stat;
    MTE_S*      mte;
    UINT8       user_state;

    LCK_FS_ENTER()

    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_stat = CHECK_USER()) == NU_SUCCESS)
    {
        /* Verify input parameters */
        if ((!statobj) || (!pattern))
            ret_stat = NUF_BADPARM;
        else
        {
            /* Resolve mount table entry from name */
            mte = fsl_mte_from_name(pattern);
            if(mte)
            {
                /* Get the current user state for the disk */        
                if ( fsdh_get_user_state(mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
                {
                    /* Verify disk is opened */
                    if (user_state == FSDH_USER_OPENED)
                    {
            
                        /* Save mte and disk handle for subsequent calls */
                        statobj->fs_mte = mte;
                        statobj->dh = mte->mte_dh;
        
                        /* Dispatch FS specific routine */
                        ret_stat = mte->mte_fs->fs_get_first(statobj, pattern);
        
                        /* Setup the object's device id for verification */
                        if (ret_stat == NU_SUCCESS)
                            statobj->drive_id = mte->mte_drive_id;
                        else
                            statobj->drive_id = NU_NULL;
                    }
                    else
                        ret_stat = NUF_NOT_OPENED;
                }
                else
                    ret_stat = NUF_BAD_USER;
            }                
            else
                ret_stat = NUF_BADDRIVE;
       
        }
    }
    LCK_FS_EXIT()
    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       NU_Get_Next
*
* DESCRIPTION
*
*       Given a pointer to a DSTAT structure that has been set up by a
*       call to NU_Get_First(), search for the next match of the
*       original pattern in the original path. Return yes if found and
*       update statobj for subsequent calls to NU_Get_Next.
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
STATUS NU_Get_Next(DSTAT *statobj)
{
    STATUS      ret_stat;
    UINT8       user_state;

    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
    if ((ret_stat = CHECK_USER()) == NU_SUCCESS)
    {
        if (statobj)
        {
            /* Verify statobj */
            if (!(statobj->fs_mte))
                ret_stat = NUF_BADPARM;
            else
            {
                /* Get the current user state for the disk */        
                if ( fsdh_get_user_state(statobj->fs_mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
                {
                    /* Verify disk is opened */
                    if (user_state == FSDH_USER_OPENED)
                    {
                        /* Verify the drive id. If the disk was aborted or 
                           unmounted, the objects are prevented from being
                           reused */
                        if (statobj->drive_id != statobj->fs_mte->mte_drive_id)
                            ret_stat = NUF_NO_DISK;
        
                        /* Dispatch FS specific routine */
                        if (ret_stat == NU_SUCCESS)
                            ret_stat = statobj->fs_mte->mte_fs->fs_get_next(statobj);
                    }
                    else
                        ret_stat = NUF_NOT_OPENED;
                }
                else
                    ret_stat = NUF_BAD_USER;
            }
        }
        else
            ret_stat = NUF_BADPARM;
    }
    LCK_FS_EXIT()
    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       NU_Done
*
* DESCRIPTION
*
*       Given a pointer to a DSTAT structure that has been set up by a
*       call to NU_Get_First(), free internal elements used by the
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
*       NU_SUCCESS							Resources successfully returned
*       NUF_NO_DISK							Disk is removed
*       NUF_NOT_OPENED 						Disk isn't opened 
*       NUF_BAD_USER						Calling task isn't registered 
*											file user
*		NUF_BADPARM							Invalid parameter passed in                           
*
*************************************************************************/
#if (FILE_VERSION_COMP == FILE_2_5)
VOID NU_Done(DSTAT *statobj)
{
#elif (FILE_VERSION_COMP > FILE_2_5)
STATUS NU_Done(DSTAT *statobj)
{
    STATUS ret_val;
#endif

    UINT8   user_state;
    
    LCK_FS_ENTER()
    /* Verify the task is a registered FILE user if multitasking */
#if (FILE_VERSION_COMP == FILE_2_5)
    if (CHECK_USER() == NU_SUCCESS)
#endif
#if (FILE_VERSION_COMP > FILE_2_5)
    if ((ret_val = CHECK_USER()) == NU_SUCCESS)
#endif
    {
        if ((statobj != NU_NULL) && (statobj->fs_mte!= NU_NULL))
        {

            /* Get the current user state for the disk */        
            if ( fsdh_get_user_state(statobj->fs_mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
            {
                /* Verify disk is opened */
                if (user_state == FSDH_USER_OPENED)
                {
                    /* Verify the object is valid */
                    if (statobj->drive_id == statobj->fs_mte->mte_drive_id)
                    {
                        /* Dispatch FS specific routine */
#if (FILE_VERSION_COMP > FILE_2_5)
                        ret_val = statobj->fs_mte->mte_fs->fs_done(statobj);
#else
                        statobj->fs_mte->mte_fs->fs_done(statobj);
#endif
                        /* Cleanup the VFS specific portions of the DSTAT object */
                        statobj->fs_private = NU_NULL;
                        statobj->fs_mte = NU_NULL;
                    }
#if (FILE_VERSION_COMP > FILE_2_5)
                    else
                        ret_val = NUF_NO_DISK;
#endif
                }
#if (FILE_VERSION_COMP > FILE_2_5)
                else
                    ret_val = NUF_NOT_OPENED;
#endif
            }
#if (FILE_VERSION_COMP > FILE_2_5)
            else
                ret_val = NUF_BAD_USER;
#endif
        }
#if (FILE_VERSION_COMP > FILE_2_5)
        else
            ret_val = NUF_BADPARM;
#endif
    }
    LCK_FS_EXIT()

#if (FILE_VERSION_COMP > FILE_2_5)
    return (ret_val);
#endif

}
/************************************************************************
* FUNCTION
*
*       get_driveno_from_dstat
*
* DESCRIPTION
*
*       Given a pointer to a DSTAT structure this function will set
        *driveno to that DSTAT's associated drive number.
*
*
* INPUTS
*
*       *statobj                            The DSTAT structure who's
*                                           drive number is being
*                                           requested.
*       *drive                              Drive number of *stateobj.
*
* OUTPUTS
*
*      NU_SUCCESS                           The drive number was found.
*      NUF_BADPARM                          Invalid parameter specified.
*
*
*************************************************************************/
STATUS get_driveno_from_dstat(DSTAT *statobj,INT *drive)
{
    STATUS ret_val;

    if ((ret_val = CHECK_USER()) == NU_SUCCESS)
    {
        if(statobj)
            *drive = statobj->fs_mte->mte_drive;
        else
            ret_val = NUF_BADPARM;
    }
    return ret_val;
}

/************************************************************************
* FUNCTION
*
*       check_date
*
* DESCRIPTION
*
*       Given a pointer to an UINT16 this function will verify that
*       it is valid date based on FAT32 File System Specification. The
*       specification states that bits 0-4 represent the day of the
*       month, bits 5-8 represents the month, and bits 9-15 represent
*       the year relative to 1980.
*
*
* INPUTS
*
*       *date                               Pointer to a date that is
*                                           is to be validated.
*
* OUTPUTS
*
*      NU_SUCCESS                           The date is valid.
*      NUF_BADPARM                          Invalid parameter specified.
*
*
*************************************************************************/
STATIC STATUS check_date(const UINT16 *date)
{
    /* Assume success. */
    STATUS  ret_val = NU_SUCCESS;
    UINT16  date_copy = *date;
    UINT8   date_copy_day;

    /* Validate the day of the month, which are bits 0-4. */
    date_copy &=  31;
    if(date_copy == 0)
    {
        ret_val = NUF_BADPARM;
    }
    /* Validate the month which are bits 5-8. */
    if(ret_val == NU_SUCCESS)
    {
        /* Remember the day so we can bound check it based off month. */
        date_copy_day = (UINT8)date_copy & 0x001F;
        
        date_copy = *date;
        /* Get just the month. */
        date_copy >>= 5;
        date_copy &= 15;

        /* Validate day of month. */ 
        switch(date_copy)
        {
            /* The following months have 31 days so need to validate date. */
            case 1: 	/* January */
            case 3: 	/* March */
            case 5: 	/* May */
            case 7: 	/* July */
            case 8: 	/* August */
            case 10:	/* October */
            case 12: 	/* December */            
            {
                ret_val = NU_SUCCESS;
                break;
            }

            /* February max day value is 28 except leap year when it is 29. */
            case 2:
            {                

                if(date_copy_day > 29)
                {
                    ret_val = NUF_BADPARM;
                }
                else if(date_copy_day == 29) /* Make sure we have a leap year. */
                {
					/* Year is bits 9-15 of *date */ 
                    UINT16 year = (*date & 0xFE00);
					year >>= 9; /* Shift right 8 to get actual number. */
					year += 1980;  /* add in 1980 as that is start of year offset for FAT. */
                    
                    /* If it isn't a leap year then the 29th day of Feb isn't valid.
                    Leap year rule: if the year is divisible by 4, its a leap year.
                    If the year is divisible by 100 and 400 then it is a leap year. */
                    if(((year%4) != 0) || ((year%100 == 0) && (year%400 != 0)))
                    {
                        ret_val = NUF_BADPARM;
                    }
                }
                else
                {
                    ret_val = NU_SUCCESS;
                }

                break;
            }

            /* Months with max day equal to 30 */
            case 4: 	/* April */
            case 6: 	/* June */
            case 9: 	/* September */
            case 11: 	/* November */
            {
                if(date_copy_day == 31)
                {
                    ret_val = NUF_BADPARM;
                }
                else
                {
                    ret_val = NU_SUCCESS;
                }
                
                break;              
            }
            /* If month is less than 1 or greater than 12, then it is invalid. */
            default:
            {
                ret_val = NUF_BADPARM;
                break;
            }
        }
    }
    /* Don't have to validate year because
       none of its possible values are invalid.*/

    return ret_val;
}
/************************************************************************
* FUNCTION
*
*       check_time
*
* DESCRIPTION
*
*       Given a pointer to an UINT16 this function will verify that
*       it is valid time based on FAT32 File System Specification. The
*       specification states that bits 0-4 represent a 2-sec count,
*       bits 5-10 represent the minutes, and bits 11-15 represent
*       the hours.
*
*
* INPUTS
*
*       *time                               Pointer to a time that is
*                                           is to be validated.
*
* OUTPUTS
*
*      NU_SUCCESS                           The time is valid.
*      NUF_BADPARM                          Invalid parameter specified.
*
*
*************************************************************************/
STATIC STATUS check_time(const UINT16 *time)
{
    /* Assume success. */
    STATUS ret_val = NU_SUCCESS;
    UINT16 time_copy = *time;

    /* Validate the seconds, which are bits 0-4. */
    time_copy &= 31;
    if(time_copy > 29)
    {
        ret_val = NUF_BADPARM;
    }

    /* Validate the minutes, which are bits 5-10. */
    if(ret_val == NU_SUCCESS)
    {
        /* Reset time_copy to the original time passed in. */
        time_copy = *time;
        /* Get just the minutes. */
        time_copy >>= 5;
        time_copy &= 63;

        if(time_copy > 59)
        {
            ret_val = NUF_BADPARM;
        }

    }
    /* Validate the hours, which are bits 11-15. */
    if(ret_val == NU_SUCCESS)
    {
        /* Reset time_copy to the original time passed in. */
        time_copy = *time;
        /* Get just the hours. */
        time_copy >>= 11;

        if(time_copy > 23)
        {
            ret_val = NUF_BADPARM;
        }

    }

    return ret_val;
}
/************************************************************************
* FUNCTION
*
*       NU_Utime
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
*
* INPUTS
*
*       *statobj                            Caller's buffer's values to
*                                           set.
*       access_date                         Access date to be set to faccdate.
*       access_time                         Access time to be set to facctime.
*       update_date                         Update date to be set to fupdate.
*       update_time                         Update time to be set to fupdate.
*       create_date                         Creation date to be set to
*                                           crtdate.
*       create_time                         Creation time to be set to
*                                           crtime.
*
*
* OUTPUTS
*
*      NU_SUCCESS                           The statobj members where set.
*      NUF_BADPARM                          Invalid parameter specified.
*      NUF_BADDRIVE                         Invalid drive specified.
*      NUF_ACCES                            This file has at least one
*                                           of the following attributes:
*                                           RDONLY,HIDDEN,SYSTEM,VOLUME
*
*
*************************************************************************/
STATUS NU_Utime(DSTAT *statobj,UINT16 access_date,UINT16 access_time,
                UINT16 update_date, UINT16 update_time,UINT16 create_date,
                UINT16 create_time)
{
    STATUS ret_stat;
    UINT8  user_state;
    LCK_FS_ENTER()

        /* Verify the task is a registered FILE user if multitasking */
    if ((ret_stat = CHECK_USER()) == NU_SUCCESS)
    {
        /* Verify input parameters */
        if (!statobj)
            ret_stat = NUF_BADPARM;
        else if((statobj->fattribute & ARDONLY) || (statobj->fattribute & AHIDDEN) ||
            (statobj->fattribute & ASYSTEM) || (statobj->fattribute & AVOLUME))
        {
            ret_stat = NUF_ACCES;
        }
        else
        {
            /* Verify that access_date is being set. */
            if(access_date != 0xFFFF)
            {
                /* Make sure date is valid. */
                ret_stat = check_date(&access_date);
            }
            /* Verify that update_time is being set. */
            if(ret_stat == NU_SUCCESS && update_date != 0xFFFF)
            {
                /* Make sure date is valid. */
                ret_stat = check_date(&update_date);
            }
            /* Verify that create_date is being set. */
            if(ret_stat == NU_SUCCESS && create_date != 0xFFFF)
            {
                /* Make sure date is valid. */
                ret_stat = check_date(&create_date);
            }
            /* Verify that access_time is being set. */
            if(ret_stat == NU_SUCCESS && access_time != 0xFFFF)
            {
                /* Make sure time is valid. */
                ret_stat = check_time(&access_time);
            }
            /* Verify that update_time is being set. */
            if(ret_stat == NU_SUCCESS && update_time != 0xFFFF)
            {
                /* Make sure time is valid. */
                ret_stat = check_time(&update_time);
            }
            /* Verify that create_time is being set. */
            if(ret_stat == NU_SUCCESS && create_time != 0xFFFF)
                /* Make sure time is valid. */
                ret_stat = check_time(&create_time);
        }
        if(ret_stat == NU_SUCCESS)
        {
            if (statobj->fs_mte != NU_NULL)
            {
                /* Get the current user state for the disk */        
                if ( fsdh_get_user_state(statobj->fs_mte->mte_dh, fsu_get_user_index(), &user_state) == NU_SUCCESS)
                {
                    /* Verify disk is opened */
                    if (user_state == FSDH_USER_OPENED)
                    {
                        /* Call file system specific routine. */
                        ret_stat = statobj->fs_mte->mte_fs->fs_utime(statobj,access_date,access_time,
                            update_date,update_time,
                            create_date,create_time);
                    }
                    else
                        ret_stat = NUF_NOT_OPENED;
                }
                else
                    ret_stat = NUF_BAD_USER;
            }
            else
                ret_stat = NUF_BADDRIVE;
        }
    }
    LCK_FS_EXIT()
    return ret_stat;
}

