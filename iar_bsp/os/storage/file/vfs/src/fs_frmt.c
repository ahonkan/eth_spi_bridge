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
*       fs_frmt.c
*
* COMPONENT
*
*       Format
*
* DESCRIPTION
*
*       Dispatches the file system specific format routine.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Format
*       NU_Get_Format_Info
*
*************************************************************************/
#include "nucleus.h"

#include "storage/pcdisk.h"
#include "storage/lck_extr.h"
#include "storage/dev_extr.h"
#include "storage/dh_extr.h"
#include "storage/fst_defs.h"
#include "storage/fsl_extr.h"
#include "storage/fs_extr.h"
#include "storage/util_extr.h"

extern FSTE_S FS_Table[FSC_MAX_TABLE_SIZE];
/************************************************************************
* FUNCTION
*
*       NU_Format
*
* DESCRIPTION
*
*       Given a drive number and a file system specific format parameter block,
*       the drive will be formatted to operation.
*
*
* INPUTS
*
*       fs_name                             File system name to format drive with
*       dev_name                            Device name to format
*       params                              File system specific format parameters
*
* OUTPUTS
*
*       NU_SUCCESS                          If the filesystem disk was
*                                            successfully initialized.
*       NUF_BAD_USER                        Not a file user.
*       NUF_BADDRIVE                        Invalid drive specified.
*       NUF_NOT_OPENED                      The disk is not opened yet.
*       NUF_NO_DISK                         Disk is removed.
*       NUF_BADPARM                         Invalid parameter specified.
*       NUF_BADDISK                         Bad Disk.
*       NUF_NO_PARTITION                    No partition in disk.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*       **FS SPECIFIC ERRORS**              File system specific errors
*                                           are also returned that are not
*                                           covered in this list.
*
*************************************************************************/
STATUS NU_Format(CHAR *fs_name, CHAR *dev_name, VOID **params)
{
UINT32  idx;
FSTE_S  *fste = NU_NULL;
FDEV_S  *dev = NU_NULL;
MTE_S   *pmte;
UINT16  dh;
STATUS  ret_stat;

    LCK_FS_ENTER()

    /* Verify parameters */
    if (!fs_name)
        ret_stat = NUF_BADPARM;
    else if (!dev_name)
        ret_stat = NUF_BADPARM;
    else
        ret_stat = NU_SUCCESS;

    /* Verify device is not mounted */
    /* Lock the mount table to prevent changes while searching */
    LCK_ENTER(LCK_MT_TABLE)

    if (ret_stat == NU_SUCCESS)
    {
        ret_stat = fsdh_devname_to_dh(dev_name, &dh);
        if (ret_stat == NU_SUCCESS)
        {

            /* Check if the device is already mounted */
            pmte = fsl_mte_from_dh(dh);
            if(pmte)
                /* Matching device in the mount table means already mounted */
                ret_stat = NUF_IN_USE;
        }
    }


    /* Unlock the mount table */
    LCK_EXIT(LCK_MT_TABLE)


    /* Get matching device by name */
    if (ret_stat == NU_SUCCESS)
    {
        /* Lock the dev table */
        LCK_ENTER(LCK_DEV_TABLE)

        /* Grab the fdev structure */
        ret_stat = fs_dev_devname_to_fdev(dev_name, &dev);

        /* Update usage count while lock is held */
        if (dev)
            dev->fdev_cnt++;
        LCK_EXIT(LCK_DEV_TABLE)
    }

    /* Lock the FS table before searching */
    LCK_ENTER(LCK_FS_TABLE)

    /* Get matching FS by name */
    if (ret_stat == NU_SUCCESS)
    {
        /* Find the matching file system entry */
        for (idx = 0; idx < FSC_MAX_TABLE_SIZE; idx++)
        {
            if (FS_Table[idx].fste_flags & FSTE_FL_VALID)
            {
                if (NUF_Strncmp(fs_name, &FS_Table[idx].fste_name[0], FSC_MAX_FS_NAME) == 0)
                {
                    fste = &FS_Table[idx];
                    /* Update the usage count so we can prevent this table entry from being
                       removed while in use. FS table lock is held when this count is
                       modified */
                    fste->fste_cnt++;
                    break;
                }
            }
        }
    }

    /* Unlock the FS table */
    LCK_EXIT(LCK_FS_TABLE)

    if ((ret_stat == NU_SUCCESS) && (!fste))
    {
        ret_stat = NUF_FS_NOT_FOUND;
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Call file system specific format routine */
        if(dev)
        {
            ret_stat = fste->fste_fs.fs_format(dev->fdev_dh, params);
        }
        else
        {
            ret_stat = NUF_INTERNAL;
        }
    
    }

    /* Lower the usage count on the FS table entry. Obtain the
       FS table lock before modifying count */
    if (fste)
    {
        LCK_ENTER(LCK_FS_TABLE)
        fste->fste_cnt--;
        LCK_EXIT(LCK_FS_TABLE)
    }

    /* Lower the usage count on the device */
    if (dev)
    {
        LCK_ENTER(LCK_DEV_TABLE)
        dev->fdev_cnt--;
        LCK_EXIT(LCK_DEV_TABLE)
    }

    LCK_FS_EXIT()

    return(ret_stat);
}

/************************************************************************
* FUNCTION
*
*       NU_Get_Format_Info
*
* DESCRIPTION
*
*       Given a drive letter a file system, the disk will be read to
*       fill out a file system specific format parameter block.
*
* INPUTS
*
*       fs_name                 File system name of expected disk format.
*       dev_name                Device name.
*       params                  File system specific format parameters
*
* OUTPUTS
*
*       NU_SUCCESS              If the format parameter was
*                                successfully initialized.
*       NUF_BAD_USER            Not a file user.
*       NUF_BADDRIVE            Invalid drive specified.
*       NUF_NOT_OPENED          The disk is not opened yet.
*       NUF_NO_DISK             Disk is removed.
*       NUF_BADPARM             Invalid parameter specified.
*       NUF_BADDISK             Bad Disk.
*       NUF_NO_PARTITION        No partition in disk.
*       NUF_IO_ERROR            Driver I/O error.
*       NUF_INTERNAL            Nucleus FILE internal error.
*       **FS SPECIFIC ERRORS**  File system specific errors
*                                are also returned that are not
*                                covered in this list.
*
*************************************************************************/
STATUS NU_Get_Format_Info(CHAR *fs_name, CHAR *dev_name, VOID **params)
{
UINT32  idx;
FSTE_S  *fste = NU_NULL;
FDEV_S  *dev = NU_NULL;
STATUS  ret_stat = NU_SUCCESS;

    LCK_FS_ENTER()

    /* Verify parameters */
    if (!fs_name)
        ret_stat = NUF_BADPARM;
    else if (!dev_name)
        ret_stat = NUF_BADPARM;

    /* Get matching device by name */
    if (ret_stat == NU_SUCCESS)
    {
        /* Lock the dev table */
        LCK_ENTER(LCK_DEV_TABLE)

        /* Grab the fdev structure */
        ret_stat = fs_dev_devname_to_fdev(dev_name, &dev);

        /* Update usage count while lock is held */
        if (dev)
            dev->fdev_cnt++;
        LCK_EXIT(LCK_DEV_TABLE)
    }

    /* Lock the FS table before searching */
    LCK_ENTER(LCK_FS_TABLE)

    /* Get matching FS by name */
    if (ret_stat == NU_SUCCESS)
    {
        /* Find the matching file system entry */
        for (idx = 0; idx < FSC_MAX_TABLE_SIZE; idx++)
        {
            if (FS_Table[idx].fste_flags & FSTE_FL_VALID)
            {
                if (NUF_Strncmp(fs_name, &FS_Table[idx].fste_name[0], FSC_MAX_FS_NAME) == 0)
                {
                    fste = &FS_Table[idx];
                    /* Update the usage count so we can prevent this table entry from being
                       removed while in use. FS table lock is held when this count is
                       modified */
                    fste->fste_cnt++;
                    break;
                }
            }
        }
    }

    /* Unlock the FS table */
    LCK_EXIT(LCK_FS_TABLE)

    if ((ret_stat == NU_SUCCESS) && (!fste))
    {
        ret_stat = NUF_FS_NOT_FOUND;
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Call file system specific format routine */
        if(dev)
        {
            ret_stat = fste->fste_fs.fs_get_format_info(dev->fdev_dh, params);
        }
        else
        {
            ret_stat = NUF_INTERNAL;
        }
    }

    /* Lower the usage count on the FS table entry. Obtain the
       FS table lock before modifying count */
    if (fste)
    {
        LCK_ENTER(LCK_FS_TABLE)
        fste->fste_cnt--;
        LCK_EXIT(LCK_FS_TABLE)
    }

    /* Lower the usage count on the device */
    if (dev)
    {
        LCK_ENTER(LCK_DEV_TABLE)
        dev->fdev_cnt--;
        LCK_EXIT(LCK_DEV_TABLE)
    }

    LCK_FS_EXIT()

    return(ret_stat);
}
