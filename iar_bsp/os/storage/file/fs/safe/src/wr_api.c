/*************************************************************************/
/*                                                                       */
/*               Copyright 2007 Mentor Graphics Corporation              */
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
*       wr_api.c
*
* COMPONENT
*
*       Nucleus Safe File System 
*
* DESCRIPTION
*
*       This file contains the interface from Safe to the VFS layer. These
*       functions are responsible for translating return values, 
*       parameter lists, and structure types between Safe and VFS. 
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       wr_safe_convert_error               Convert safe error to 
*                                            VFS error.
*       wr_safe_convert_dh_to_drvnum        Convert VFS disk handle
*                                            to Safe drive number.
*       wr_safe_convert_fd                  Gets F_FILE based off fd.
*       wr_safe_convert_mode                Converts VFS file mode
*                                            to its corresponding Safe
*                                            file mode.
*       wr_safe_alloc_f_find                Allocates a F_FIND.
*       wr_safe_dealloc_f_find              Deallocates a F_FIND.
*       wr_safe_init                        Initialize Safe.
*       wr_safe_uninit                      Non-supported operation.
*       wr_safe_mount                       Mounts a drive.
*       wr_safe_unmount                     Unmounts a drive.
*       wr_safe_disk_abort                  Non-supported operation.
*       wr_safe_open                        Open a file.
*       wr_safe_read                        Read bytes from a file.
*       wr_safe_write                       Write Bytes to a file.
*       wr_safe_seek                        Move the file pointer.
*       wr_safe_close                       Close a file.
*       wr_safe_delete                      Delete a file(s).
*       wr_safe_utime                       Set file date and time.
*       wr_safe_set_attr                    Set file attributes.
*       wr_safe_get_attr                    Get a file's attributes.
*       wr_safe_truncate                    Truncate an open file.
*       wr_safe_flush                       Flush an open file.
*       wr_safe_mkdir                       Create a directory.
*       wr_safe_rmdir                       Delete a directory.
*       wr_safe_rename                      Rename a file(s).
*       wr_safe_f_find_to_dstat             Convert Safe F_FIND to 
*                                            VFS DSTAT structure.
*       wr_safe_get_first                   Get stats on the first file
*                                            to match a pattern.
*       wr_safe_get_next                    Get stats on the next file
*                                            to match a pattern.
*       wr_safe_done                        Free resources used by
*                                            wr_safe_get_first/
*                                            wr_safe_get_next.
*       wr_safe_format                      Format disk for use with SAFE 
*       wr_safe_get_format_info             Get format structure 
*       wr_safe_freespace                   Non-supported operation.
*       wr_safe_vnode_allocate              Allocates a vnode.
*       wr_safe_vnode_deallocate            Deallocates a vnode.
*       wr_safe_fsnode_to_string            Converts an fsnode into a
*                                            string.
*       wr_safe_release                     Non-supported operation.
*       wr_safe_reclaim                     Non-supported operation.
*       WR_Safe_Init_FS                     Static init of Safe.
*
************************************************************************/

#include "storage/wr_api.h"
#include "storage/dir_defs.h"
#include "storage/dev_extr.h"
#include "storage/vnode_defs.h"

#if(USE_VFS)
/* Global data */
F_FILE** WrSafeFdMap; /* file descriptor map */
UINT8* g_safe_wr_perm; /* wrapper layer permissions */

NU_SEMAPHORE wr_fd_lock;
UINT32 Safe_Unused_Param; /* Used to resolve compiler warnings. */

/* Externs */
extern FS_MULTI gl_multi[FS_MAXTASK];    /* Array used by Safe for multitask users support. */
extern PFILE_SYSTEM_USER    user_heap;   /* VFS File system user structure pointer. */
extern FS_FILESYSTEM fg_filesystem;      /* Safe file system structure which contains volume descriptors. */

INT g_serr2ferr[] =
{

/*  0 F_NO_ERROR                */ NU_SUCCESS,
/*  1 F_ERR_INVALIDDRIVE        */ NUF_BADDRIVE,
/*  2 F_ERR_NOTFORMATTED        */ NUF_FORMAT,
/*  3 F_ERR_INVALIDDIR          */ NUF_NOFILE,
/*  4 F_ERR_INVALIDNAME         */ NUF_NOFILE,
/*  5 F_ERR_NOTFOUND            */ NUF_NOFILE,
/*  6 F_ERR_DUPLICATED          */ NUF_EXIST,
/*  7 F_ERR_NOMOREENTRY         */ NUF_INTERNAL,
/*  8 F_ERR_NOTOPEN             */ NUF_NOT_OPENED,
/*  9 F_ERR_EOF                 */ NUF_NVALFP, /* Should never occur */
/* 10 F_ERR_RESERVED            */ NUF_INTERNAL, /* Unused return value */
/* 11 F_ERR_NOTUSEABLE          */ NUF_BADPARM,
/* 12 F_ERR_LOCKED              */ NUF_ACCES,
/* 13 F_ERR_ACCESSDENIED        */ NUF_ACCES,
/* 14 F_ERR_NOTEMPTY            */ NUF_NOEMPTY,
/* 15 F_ERR_INITFUNC            */ NUF_INTERNAL,
/* 16 F_ERR_CARDREMOVED         */ NUF_INTERNAL, /* Unused return value */

/* 17 F_ERR_ONDRIVE             */ NUF_INTERNAL,
/* 18 F_ERR_INVALIDSECTOR       */ NUF_INTERNAL,
/* 19 F_ERR_READ                */ NUF_INTERNAL,
/* 20 F_ERR_WRITE               */ NUF_INTERNAL,
/* 21 F_ERR_INVALIDMEDIA        */ NUF_INTERNAL,
/* 22 F_ERR_BUSY                */ NUF_INTERNAL,
/* 23 F_ERR_WRITEPROTECT        */ NUF_INTERNAL,
/* 24 F_ERR_INVFATTYPE          */ NUF_INTERNAL,
/* 25 F_ERR_MEDIATOOSMALL       */ NUF_INTERNAL,
/* 26 F_ERR_MEDIATOOLARGE       */ NUF_INTERNAL,
/* 27 F_ERR_NOTSUPPSECTORSIZE   */ NUF_INTERNAL,
/* 28 F_ERR_UNKNOWN             */ NUF_INTERNAL,
/* 29 F_ERR_DRVALREADYMNT       */ NUF_INTERNAL,
/* 30 F_ERR_TOOLONGNAME         */ NUF_INTERNAL,
/* 31 F_ERR_NOTFORREAD          */ NUF_INTERNAL,
/* 32 F_ERR_DELFUNC             */ NUF_INTERNAL,
/* 33 F_ERR_ALLOCATION          */ NUF_INTERNAL,
/* 34 F_ERR_INVALIDPOS          */ NUF_INTERNAL,
/* 35 F_ERR_NOMORETASK          */ NUF_INTERNAL,
/* 36 F_ERR_NOTAVAILABLE        */ NUF_INTERNAL,
/* 37 F_ERR_TASKNOTFOUND        */ NUF_INTERNAL,
/* 38 F_ERR_UNUSABLE            */ NUF_INTERNAL
};
#define SAFE_MAX_ERROR_CODE 38

/* Internal function declarations */
static STATUS wr_safe_convert_error(INT safe_error);
static INT    wr_safe_convert_dh_to_drvnum(UINT16 dh);
static VOID wr_safe_f_find_to_dstat(DSTAT *statobj, F_FIND *pF);
static F_FILE *wr_safe_convert_fd(INT fd);

/* API Helper/Utility functions. */
static STATUS wr_safe_open_move_fp(F_FILE *safe_fd, UINT16 flag);
static STATUS wr_safe_open_wl_perm(UINT8 *wl_perm, UINT16 flag);
static STATUS wr_safe_open_user_errors(INT user_error);
static STATUS wr_safe_validate_name(CHAR *name);

/* Wildcard search condition constants. */
typedef enum wc_search_condition
{
    WC_FILE, 
    WC_DIR, 
    WC_BOTH
    
} WC_SEARCH_COND;

/************************************************************************
* FUNCTION
*
*       safe_get_first_wc_match
*
* DESCRIPTION
*
*       Used to search for the first occurrence of a file or directory
*       based of search string(str_wdcard) and the search condition
*       (search_cond).
*
* INPUTS
*
*       *f                                  Structure used to return the found
*                                           file or directory.
*       *str_wdcard                         Path containing wildcard characters.
*       search_cond                         Used to indicate whether to look for 
*                                           a file, directory or either.
*
* OUTPUTS
*
*       NU_SUCCESS                          Search found a directory or
*                                           file that matched str_wdcard.
*       Error code                          If service is erroneous.
*
*************************************************************************/
static STATUS safe_get_first_wc_match(F_FIND *f, CHAR *str_wdcard, WC_SEARCH_COND search_cond )
{
    STATUS ret_val;    

    ret_val = f_findfirst(str_wdcard,f);    
    
    /* This loop is based on two search conditions, data validation, and success on status.
    First condition is directory record is a directory, but we are looking for a file.
    Second condition is directory record is a file, but we are looking for a directory. */
    while((ret_val == F_NO_ERROR) && (f) && 
        (((f->attr & FS_ATTR_DIR) && (search_cond == WC_FILE))     /* First condition */
        || (!(f->attr & FS_ATTR_DIR) && (search_cond == WC_DIR)))) /* Second condition */
    {
        ret_val = f_findnext(f);
    }                   

    if(ret_val != F_NO_ERROR)
        ret_val = wr_safe_convert_error(ret_val);
    
    return ret_val;
}

/************************************************************************
* FUNCTION
*
*       safe_get_next_wc_match
*
* DESCRIPTION
*
*       This function should be called to after safe_get_first_wc_match 
*       succeeds to see if there are any other file or directories that match
*       the wildcard string.
*       
*
* INPUTS
*
*       *f                                  Structure used to return the found
*                                           file or directory.*     
*       search_cond                         Used to indicate whether to look for 
*                                           a file, directory or either.
*
* OUTPUTS
*
*       NU_SUCCESS                          Search found next directory
*                                           or file that matched the 
*                                           search condition used in
*                                           safe_get_first_wc_match                                         
*       Error code                          If service is erroneous.
*
*************************************************************************/
static STATUS safe_get_next_wc_match(F_FIND *f, WC_SEARCH_COND search_cond )
{
    STATUS ret_val;    

    ret_val = f_findnext(f);

     /* This loop is based on two search conditions, data validation, and success on status.
    First condition is directory record is a directory, but we are looking for a file.
    Second condition is directory record is a file, but we are looking for a directory. */
    while((ret_val == F_NO_ERROR) && (f) && 
        (((f->attr & FS_ATTR_DIR) && (search_cond == WC_FILE))     /* First condition */
        || (!(f->attr & FS_ATTR_DIR) && (search_cond == WC_DIR)))) /* Second condition */
    {
        ret_val = f_findnext(f);
    }                   
    
    if(ret_val != F_NO_ERROR)
    {
        ret_val = wr_safe_convert_error(ret_val);
    }

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*       wr_safe_convert_error
*
* DESCRIPTION
*
*       Convert the SAFE error value to an appropriate
*        VFS status.
*
* INPUTS
*
*       safe_error                          Safe error code to be
*                                           converted.
*
* OUTPUTS
*
*       NU_SUCCESS                          If safe error is zero.
*       Error code                          If service is erroneous.
*
*************************************************************************/
static STATUS wr_safe_convert_error(INT safe_error)
{
    STATUS ret_val = NUF_BADPARM;
    
    if((safe_error >= 0) && (safe_error < SAFE_MAX_ERROR_CODE))
        ret_val = (STATUS)g_serr2ferr[safe_error];
    else
    {
        if(safe_error < 0)
            ret_val = safe_error;
    }
        
    return ret_val;
}

/************************************************************************
* FUNCTION
*
*       wr_safe_convert_dh_to_drvnum
*
* DESCRIPTION
*
*       Convert VFS dh to a SAFE drive number.
*
* INPUTS
*
*       dh                                  VFS Disk Handle ID.
*
* OUTPUTS
*
*       SAFE drive number                   If look up succeeds
*       Error code                          If service is erroneous.
*
*************************************************************************/
static INT wr_safe_convert_dh_to_drvnum(UINT16 dh)
{    
    MTE_S *mte;

    mte = fsl_mte_from_dh(dh);
    if (mte != NU_NULL)
        return(mte->mte_drive);
    else
        return(-1);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_convert_fd
*
* DESCRIPTION
*
*       Gets Safe F_FILE based off the passed in VFS fd.
*
* INPUTS
*
*       fd                                  File descriptor from VFS.
*
* OUTPUTS
*
*       *F_FILE                             Found F_FILE structure based off
*                                            file descriptor.
*       NU_NULL                             Couldn't find a F_FILE element 
*                                            based off the file descriptor.
*
*************************************************************************/
static F_FILE * wr_safe_convert_fd(INT fd)
{
    if ((fd >=0) && (fd < gl_FD_MAX_FD))
        return (WrSafeFdMap[fd]);
    else
        return NU_NULL;
}

/************************************************************************
* FUNCTION
*
*       wr_safe_alloc_f_find
*
* DESCRIPTION
*
*       Allocates a F_FIND.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       F_FIND                              Pointer to a F_FIND data type.
*
*************************************************************************/
F_FIND * wr_safe_alloc_f_find()
{
    return ((F_FIND*) NUF_Alloc(sizeof(F_FIND)));
}

/************************************************************************
* FUNCTION
*
*       wr_safe_dealloc_f_find
*
* DESCRIPTION
*
*       Deallocates a F_FIND.
*
* INPUTS
*
*       *f                                  F_FIND
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
STATUS wr_safe_dealloc_f_find(F_FIND *f)
{
    STATUS ret_val;

    ret_val = NU_Deallocate_Memory(f);
    f = NU_NULL;

    Safe_Unused_Param = (UINT32)f;

    return (ret_val);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_init
*
* DESCRIPTION
*
*       Initialize Safe file system.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       NU_SUCCESS                          If Safe was initialized 
*                                            successfully.
*       Error code                          If service is erroneous.
*
*
*************************************************************************/
STATUS wr_safe_init(VOID)
{
    INT ret;
    STATUS ret_stat;
    UINT8 i;

    /* Allocate file descriptor map */
    WrSafeFdMap = NUF_Alloc(sizeof(F_FILE) * gl_FD_MAX_FD);
    if (WrSafeFdMap != NU_NULL)
    {
        NUF_Memfill(WrSafeFdMap, (sizeof(F_FILE) * gl_FD_MAX_FD), 0);

        /* Allocate wrapper layer permissions */
        g_safe_wr_perm= NUF_Alloc(sizeof(UINT8) * gl_FD_MAX_FD);
        if (g_safe_wr_perm != NU_NULL)
        {
            NUF_Memfill(g_safe_wr_perm, (sizeof(UINT8) * gl_FD_MAX_FD), 0);
            ret_stat = NU_SUCCESS;
        }
        else
        {
            NU_Deallocate_Memory((VOID*)WrSafeFdMap);
            ret_stat = NUF_NO_MEMORY;
        }
    }
    else
    {
        ret_stat = NUF_NO_MEMORY;
    }

    if (ret_stat == NU_SUCCESS)
    {
        ret = f_init();
        if (ret == F_NO_ERROR)
        {
            /* Create wrapper layer file descriptor map lock */
            ret_stat = NU_Create_Semaphore(&wr_fd_lock, "wr_fd_lock", 1, NU_FIFO);

            /* Set Safe's file system user struct and VFS's so they
              stay in sync. */
            for(i = 0; i < FS_MAXTASK; ++i)
            {
                gl_multi[i].ID = (long *)&(user_heap[i].context_handle);
                gl_multi[i].fs_curdrive = &(user_heap[i].dfltdrv);
                gl_multi[i].lasterror = &(user_heap[i].p_errno);
            }
        }
        else
        {
            ret_stat = wr_safe_convert_error(ret);
        }
    }

    return (ret_stat);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_uninit
*
* DESCRIPTION
*
*       Frees the resources allocated by wr_safe_init.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       NU_SUCCESS                         Resources freed.
*
*
*************************************************************************/
STATUS wr_safe_uninit(VOID)
{
    STATUS ret_stat, tmp_stat;

    /* Dealloc file descriptor map */
    ret_stat = NU_Deallocate_Memory((VOID*)WrSafeFdMap);

    /* Dealloc wrapper layer permissions */
    tmp_stat = NU_Deallocate_Memory((VOID*)g_safe_wr_perm);
    
    /* Preserve the first error code */
    if ((ret_stat == NU_SUCCESS) && (tmp_stat != NU_SUCCESS))
    {
        ret_stat = tmp_stat;
    }

    /* Free semaphore created in init function. */
    tmp_stat = NU_Delete_Semaphore(&wr_fd_lock);

    /* Preserve the first error code */
    if ((ret_stat == NU_SUCCESS) && (tmp_stat != NU_SUCCESS))
    {
        ret_stat = tmp_stat;
    }
    
    return ret_stat;
}

/************************************************************************
* FUNCTION
*
*       wr_safe_mount
*
* DESCRIPTION
*
*       Wrapper function for mounting a drive.
*
* INPUTS
*
*       dh                                  Disk handle.
*       *config                             Safe configuration. 
*
* OUTPUTS
*
*       NU_SUCCESS                          Mount was successful.
*       NUF_BADPARM                         config is invalid.
*       Error code                          If service is erroneous.
*
*************************************************************************/
STATUS wr_safe_mount(UINT16 dh, VOID *config)
{
    STATUS ret_stat;
    STATUS safe_ret;
    INT safe_drvnum;
    FDRV_LOG_CB_S *plog_cb;
    FDRV_PHY_CB_S *pphy_cb; 
    WR_SAFE_DEV_CB *dev_cb; 
    WR_SAFE_MNT_PARAMS *mnt_params;
    
    /* Resolve unused param. */
    Safe_Unused_Param = (UINT32)config;

    /* Get the logical control block */
    ret_stat = fsdh_get_dh_specific(dh, (VOID**) &plog_cb, NU_NULL);
    if((ret_stat == NU_SUCCESS)&&(plog_cb == NU_NULL))
    {
        ret_stat = NUF_INTERNAL;
    }   
    if (ret_stat == NU_SUCCESS)
    {
        /* Get the physical control block */
        ret_stat = fsdh_get_dh_specific(plog_cb->fdrv_vfs_dh, (VOID**) &pphy_cb, NU_NULL);
        if((ret_stat == NU_SUCCESS)&&(pphy_cb == NU_NULL))
        {
            ret_stat = NUF_INTERNAL;
        }   
        if (ret_stat == NU_SUCCESS)
        {
            /* Get the Safe device specific control block */ 
            dev_cb = pphy_cb->fdrv_spec;
            if(dev_cb != NU_NULL)
            {
                mnt_params = &dev_cb->safe_mnt_params;     
        
                safe_drvnum = wr_safe_convert_dh_to_drvnum(dh);
                if(safe_drvnum >= 0)
                {
                    safe_ret = f_mountdrive(safe_drvnum,
                    mnt_params->buffer, mnt_params->buffsize, mnt_params->mountfunc, mnt_params->phyfunc);        
                
                    ret_stat = wr_safe_convert_error(safe_ret);
                    /* If a format error is returned, unmount the drive.
                      This is done because Safe requires the drive to 
                      mounted when formatting. However the VFS requires that
                      the drive not be mounted. wr_safe_format will handle 
                      remounting the drive in Safe if a format is requested. 
                      */
                    if(ret_stat == NUF_FORMAT)
                    {
                        safe_ret = f_unmountdrive(safe_drvnum);
                        ret_stat = wr_safe_convert_error(safe_ret);
    
                        /* If unmount was successful return original error NUF_FORMAT. */
                        if(ret_stat == NU_SUCCESS)
                        {
                            ret_stat = NUF_FORMAT;
                        }
    
                    }
                }
                else
                {
                    /* Return error. */
                    ret_stat = NUF_INTERNAL;
                }
            }        
            else
            {
                /* If dev_cb is NU_NULL and no error was returned from
                 fsdh_get_dh_specific, then FS_Disk_Handles has 
                 become corrupted. */
                ret_stat = NUF_INTERNAL;
            }
        }
    }

    return (ret_stat);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_unmount
*
* DESCRIPTION
*
*        Wrapper function for unmounting a drive.
*
* INPUTS
*
*       dh                                  Disk handle.       
*
* OUTPUTS
*
*       NU_SUCCESS                          Unmount was successful.
*       Error code                          If service is erroneous.
*
*************************************************************************/
STATUS wr_safe_unmount(UINT16 dh)
{
    STATUS ret;
    INT drvnum;
    
    /* Get the drive number. */ 
    drvnum = wr_safe_convert_dh_to_drvnum(dh);  
    /* dh was verified in VFS, so it has to be good if it reached
       here. */
    if(drvnum >= 0)
    {
        /* Check to see if Safe is already a file user,
          if it isn't make it one. This is done because
          Safe requires a task to be a file user when
          calling unmount, but the VFS doesn't. */
        if(NU_Check_File_User() != NU_SUCCESS)            
        {
            ret = NU_Become_File_User();        
        
            if(ret == NU_SUCCESS)
            {
                ret = f_unmountdrive (drvnum);
                if(ret == F_NO_ERROR)
                {
                    
                    /* Release file system user even if this fails because 
                      at the VFS layer while calling this the user isn't
                      a file system user. */
                    ret = NU_Release_File_User();

                }
                else
                {
                    /* If f_unmountdrive returned an error don't check NU_Release_File_User for an error.*/
                    NU_Release_File_User();
                }
            }
            else
            {
                /* If NU_Become_File_User returned an error don't check NU_Release_File_User for an error.*/
                NU_Release_File_User();
            }
             
        }
        else
        {
            ret = f_unmountdrive (drvnum);        
        }
    }
    else
    {
        ret = NUF_INTERNAL;
    }
    

    return (wr_safe_convert_error(ret));
}

/************************************************************************
* FUNCTION
*
*       wr_safe_disk_abort
*
* DESCRIPTION
*
*        Wrapper function for aborting a disk. (This function shouldn't be
*        called.)
*
* INPUTS
*
*       dh                                  Disk handle.       
*
* OUTPUTS
*
*       NUF_INTERNAL              
*       
*************************************************************************/
STATUS wr_safe_disk_abort(UINT16 dh)
{
    /* Resolve unused param. */
    Safe_Unused_Param = dh;
    return (NUF_INTERNAL);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_validate_name
*
* DESCRIPTION
*
*       Used to make sure a file or directory name doesn't exceed 
*       user defined path length.
*
* INPUTS
*
*       *name                               File name
*
* OUTPUTS
*
*       NU_SUCCESS                          If name is valid.
*       NUF_LONGPATH                        Name exceeds user defined length.
*
*************************************************************************/
static STATUS wr_safe_validate_name(CHAR *name)
{
    STATUS ret_val = NU_SUCCESS;
    INT name_len;
    
    /* Validate path length. */
    name_len = NUF_Get_Str_Len(name);
    if(name_len > FS_MAXPATH)   
    {
        ret_val = NUF_LONGPATH;
    }

    return (ret_val);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_convert_mode
*
* DESCRIPTION
*
*       Converts VFS file mode to its corresponding Safe file mode.
*
* INPUTS
*
*       flag                                VFS flag.
*       mode                                VFS mode.
*       *safe_mode                          Pointer to safe mode
*                                           (Value is assumed to be greater
*                                           than 3 character string).
*       b_file_exist                        Boolean value to indicate
*                                           if a file exist. 
*                                           (Only valid values are 
*                                           NU_TRUE and NU_FALSE)
*       b_open_for_wr                       Boolean value to indicate
*                                           if a file is open for writing.
*                                           (Only valid values are 
*                                           NU_TRUE and NU_FALSE)
*
* OUTPUTS
*
*       NU_SUCCESS                          If conversion was successful.
*       NUF_BADPARM                         safe_mode is invalid pointer.
*
*************************************************************************/
STATUS wr_safe_open_convert_mode(UINT16 flag, CHAR *safe_mode, UINT8 b_file_exist, UINT8 b_open_for_wr)
{

    CHAR    *pos;
    STATUS ret_val = NU_SUCCESS;
    
    if (safe_mode != NU_NULL)
    {
        pos = safe_mode;    
        
        /* Clear unused field */
        flag = flag & (UINT16)~(PO_TEXT|PO_BINARY|PO_NOSHAREWRITE|PO_NOSHAREANY);

        /* Handle special cases for if file exist. */
        if(b_file_exist == NU_TRUE)
        {
            /* Handle errors if file exist in system. */
            /* File exist in system. */
            if((flag & (PO_EXCL | PO_CREAT)) == (PO_EXCL | PO_CREAT))
            {
                ret_val = NUF_EXIST;            
            }        
        }
        else
        {
            /* PO_RDONLY, is default file mode. If user only
            passed in PO_CREAT, and the file doesn't exist
            then tell user there is no file. */
            if(!(flag & PO_CREAT))
            {
                ret_val = NUF_NOFILE;    
                
            }else 
            {
                if(b_open_for_wr == NU_FALSE)
                {
                    ret_val = NUF_ACCES;
                }
            }
        }

        if(ret_val == NU_SUCCESS)
        {
             /* IF PO_EXCL was supplied:
               At this point the file requested to be opened doesn't exist 
               in the system and can be created. If use supplied
               PO_EXCL, remove it so that wr_safe_convert_mode will
               set the correct Safe mode. */
            if((flag & PO_EXCL) && (flag & PO_CREAT))
            {
                flag = (UINT16)(flag ^ (PO_EXCL));      
            }
            
            switch(flag)
            {
                case PO_CREAT|PO_RDONLY: /* Can assume file exist, because if it doesn't error was caught earlier. */
                case PO_RDONLY:
                    *pos = 'r';              /* "r" equals readonly access in Safe*/
                    ++pos;
                break;
                
                
                case PO_WRONLY:
                case PO_RDWR:
                    *pos = 'r';              /* "r+" equals read/write access in Safe*/
                    ++pos;                   /* Wrapper layer permissions will be set if PO_WRONLY, see wr_safe_open_wl_perm. */
                    
                    *pos = '+';
                    ++pos; 
                break;

                /* If file already exist, use r+ to preserve the existing state. By using
                 r+ the file will not be automatically truncated so that operation must
                 be performed in wr_safe_open_move_fp. 
                 
                 Reason for open existing file with r+ instead of user supplied w or w+ is
                 because when opening a file with w+ on small systems 
                 a transaction point can occur when swapping the reserved data block
                 with another free block. This causes the in memory file to be 
                 updated to the disk(the in memory file would be empty and of size
                 zero). This way the existing file is always preserved, until a close
                 or flush is performed by the user. */
                case PO_CREAT|PO_TRUNC|PO_WRONLY:
                case PO_CREAT|PO_WRONLY:

                    if(b_file_exist == NU_TRUE)
                    {                    
                        *pos = 'r';
                        ++pos;
                        
                        *pos = '+';
                        ++pos; 
                    }
                    else
                    {
                        /* "w+" in Safe means: If file doesn't exist truncate and open 
                        for write access. If file does exist truncate and
                        open for write access. */
                        *pos = 'w';          
                        ++pos;
                    }

                break;

                /* If file already exist, use r+ to preserve the existing state. By using
                 r+ the file will not be automatically truncated so that operation must
                 be performed in wr_safe_open_move_fp. 
                 
                 Reason for open existing file with r+ instead of user supplied w or w+ is
                 because when opening a file with w+ on small systems 
                 a transaction point can occur when swapping the reserved data block
                 with another free block. This causes the in memory file to be 
                 updated to the disk(the in memory file would be empty and of size
                 zero). This way the existing file is always preserved, until a close
                 or flush is performed by the user. */
                case PO_CREAT|PO_TRUNC|PO_RDWR:
                case PO_CREAT|PO_RDWR:

                    if(b_file_exist == NU_TRUE)
                    {                    
                        *pos = 'r';
                        ++pos;
                        
                        *pos = '+';
                        ++pos; 
                    }
                    else
                    {
                        /* "w+" in Safe means: If file doesn't exist truncate and open 
                        for read/write access. If file does exist truncate and
                        open for read/write access. */
                        *pos = 'w';          
                        ++pos;
                        
                        *pos = '+';
                        ++pos; 
                    }
                   
                break;
                
                case PO_CREAT|PO_APPEND|PO_WRONLY:
                case PO_APPEND|PO_WRONLY:
                    /* "a" in Safe means: If file doesn't exist create it and open 
                     for appending with write only access. If file does exist 
                     open file for appending with write only access.  */
                    *pos = 'a';              
                    ++pos;
                break;
                
                case PO_CREAT|PO_APPEND|PO_RDWR:            
                case PO_APPEND|PO_RDWR:
                    /* "a+" in Safe means: If file doesn't exist create it and open 
                     for appending with read/write access. If file does exist 
                     open file for appending with read/write access.  */
                    *pos = 'a';              
                    ++pos;
                    
                    *pos = '+';
                    ++pos; 
                break;

                default:
                    ret_val = NUF_BADPARM;
                break;
                
            }

            *pos = '\0'; /* null terminate the string */
            
        }            
    }
    else
    {
        return (NUF_BADPARM);
    }
    
    return (ret_val);

}

/************************************************************************
* FUNCTION
*
*       wr_safe_open_move_fp
*
* DESCRIPTION
*
*       Determines and updates the file pointer if the file pointer 
*       needs to be adjusted due to VFS open flags and Safe open 
*       flags/modes not directly mapping.
*       This function assumes the file exists in the system,
*       which means safe_fd has to be valid. 
*
* INPUTS
*
*       *safe_fd                            Safe file descriptor.
*       flag                                VFS flag.
*
* OUTPUTS
*
*       NU_SUCCESS                          If file pointer was moved,
*                                           successfully.
*       Error code                          If service is erroneous.
*
*************************************************************************/
static STATUS wr_safe_open_move_fp(F_FILE *safe_fd, UINT16 flag)
{
    STATUS ret_val;
    INT safe_error;
    
     /* Clear unused field */
    flag = flag & (UINT16)~(PO_TEXT|PO_BINARY|PO_NOSHAREWRITE|PO_NOSHAREANY);

    /* If the file exist in the system and the user wants to truncate the file
      don't rely on Safe's internal truncate when opening the file. In small systems
      under very rare conditions the truncate can trigger a transactional point. This
      would result in the file being empty at its last known good state, instead of
      its last known good state being the previous contents. So if power was loss 
      after attempting open a file with PO_TRUNC, there is a possibility that file 
      contents would be zero. To elevate this behavior call f_seteof whenever a truncate 
      is to be performed on an existing files. */
    if((flag == (PO_CREAT|PO_TRUNC|PO_WRONLY)) || (flag == (PO_CREAT|PO_TRUNC|PO_RDWR)))
    {
        safe_error = f_seteof(safe_fd);
    }
    else
    {
        safe_error = F_NO_ERROR;
    }
        
    if(safe_error != F_NO_ERROR)
    {
        ret_val = wr_safe_convert_error(safe_error);
    }
    else
    {
        ret_val = NU_SUCCESS;
    }

    return (ret_val);

}

/************************************************************************
* FUNCTION
*
*       wr_safe_open_wl_perm
*
* DESCRIPTION
*
*       Determine wrapper layer permissions based off VFS flags.
*
* INPUTS
*
*       *wl_perm                            Where wrapper layer permissions
*                                           are stored based of flag.
*       flag                                VFS flag.
*
* OUTPUTS
*
*       NU_SUCCESS                          If wrapper layer permissions 
*                                           where set to *wl_perm.       
*
*************************************************************************/
static STATUS wr_safe_open_wl_perm(UINT8 *wl_perm, UINT16 flag)
{
    STATUS ret_val = NU_SUCCESS;
    
     /* Clear unused field */
    flag = flag & (UINT16)~(PO_TEXT|PO_BINARY|PO_NOSHAREWRITE|PO_NOSHAREANY);

    /* Check to see if file was opened for write only. */
    if(flag & PO_WRONLY)
    {
        *wl_perm = SAFE_WR_PERM_OF_WRONLY;           
    }

    /* Check to see if file was opened for read only, 
       the PO_RDONLY attribute has to be checked this way because
       it value is defined as zero. This means if we do flag & PO_RDONLY 
       to check for it we will get compiler warnings saying value is
       always false. */
    if(!(flag & (PO_WRONLY | PO_RDWR)))
    {
        *wl_perm = SAFE_WR_PERM_OF_RDONLY;           
    }
    
    return(ret_val);     

}

/************************************************************************
* FUNCTION
*
*       wr_safe_open_user_errors
*
* DESCRIPTION
*
*       Determine if there is a user specific error and if so
*       map to correct NU_Open error. The user specific errors
*       should come from call to f_open, which occurs in 
*       wr_safe_open.
*
* INPUTS
*
*       user_error                          User error from Safe.
*
* OUTPUTS
*
*       NU_SUCCESS                          If conversion was successful.
*       NUF_SHARE                           File opened for exclusive access.
*       NUF_PEMFILE                         No more files could be opened.
*       NUF_INVNAME                         Name passed into f_open was
*                                           invalid.
*       NUF_ACCES                           The file that was suppose to be
*                                           open for writing has its readonly attribute
*                                           set. 
*       NUF_NOFILE                          File wasn't found.
*       NUF_INTERNAL                        Internal Safe file system error.
*
*************************************************************************/
static STATUS wr_safe_open_user_errors(INT user_error)
{
    STATUS ret_val;

    /* Determine user specific error code. */
    switch(user_error)
    {
        case F_ERR_LOCKED:
        case FS_FILE_LOCKED:
            ret_val = NUF_SHARE;             /* File open for exclusive access, generally, means open for writing. */
        break;

        case F_ERR_NOMOREENTRY:            /* No more files could be opened. */
            ret_val = NUF_PEMFILE;
        break;

        case F_ERR_INVALIDNAME:              /* Name is invalid. */
            ret_val = NUF_INVNAME;  
        break;

        case F_ERR_ACCESSDENIED:            /* Files readonly attribute is set. */
            ret_val = NUF_ACCES;
        break;

        case F_ERR_NOTFOUND:                /* File wasn't found. */
            ret_val = NUF_NOFILE;
        break;

        case F_NO_ERROR:                    /* f_open opened to file. */
            ret_val = NU_SUCCESS;
        break;
        
        default:                              /* Error doesn't map, then they are internal issues. */
            ret_val = NUF_INTERNAL;
        break;
    }    
    return (ret_val);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_open
*
* DESCRIPTION
*
*       Wrapper function for opening a file.
*
* INPUTS
*
*       dh                                  Disk handle.
*       *name                               File name.
*       flag                                VFS flag.
*       mode                                VFS mode.
*
* OUTPUTS
*
*       NU_SUCCESS                          File open was successful.
*       NUF_BADPARM                         File wasn't open.
*       Error code                          If service is erroneous.
*
*************************************************************************/
INT wr_safe_open(UINT16 dh, CHAR *name, UINT16 flag, UINT16 mode)
{
    INT i;
    INT fd;
    F_FILE *f;
    CHAR safe_mode[3] = {0};  /* Make sure string is empty.*/
    UINT8 b_open_for_wr;
    UINT8 b_file_exist = NU_FALSE;
    UINT8 safe_wr_perm = SAFE_WR_PERM_OF_NONE;

    /* Grab wr_fd_lock */
    NU_Obtain_Semaphore(&wr_fd_lock, NU_SUSPEND);

    /* Verify dh even though we don't use it. */
    fd = fs_dev_dskchk_proc(dh);    
    if(fd == NU_SUCCESS)         /* Validate parameters.*/
    {
        /* Validate name. */
        fd = wr_safe_validate_name(name); 
        if(fd == NU_SUCCESS)
        {
            /* Validate flag and mode combinations. */
            
            /* We'll need to know this in a few places. */
            if ( flag & (PO_WRONLY | PO_RDWR) )
                b_open_for_wr = NU_TRUE;
            else
                b_open_for_wr = NU_FALSE;


            if( flag & (~(PO_WRONLY|PO_RDWR|PO_APPEND|
                        PO_CREAT|PO_TRUNC|PO_EXCL|PO_TEXT|
                    PO_BINARY|PO_NOSHAREANY|PO_NOSHAREWRITE)))
            {
                /* Invalid parameter is specified. */
                fd = NUF_INVPARM;
            }
            else if ((flag & PO_TRUNC) && (!b_open_for_wr))
            {
                /* Invalid parameter combination is specified. */
                fd = NUF_INVPARCMB;
            }
            else if ((flag & PO_APPEND) && (!b_open_for_wr))
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
            else
            {
                /* Does file exist in the system? */
                f = f_open(name,"r");       
                if(f)
                {
                    /* Close file just opened. */
                    fd = f_close(f);
                    b_file_exist = NU_TRUE;
                
                }
                else
                {   
                    /* If an error occurred when attempting to open a file(f_open), 
                    try to determine a more descriptive error code. */
                    fd = wr_safe_open_user_errors(fs_user->p_errno);

                    /* If the file exist make sure boolean b_file_exist is set to NU_FALSE. */
                    if(fd == NUF_NOFILE)
                    {
                        b_file_exist = NU_FALSE;
                        /* If the file doesn't exist in the system then reset error to NU_SUCCESS,
                        because we may be creating a file. */
                        fd = NU_SUCCESS;
                    }
                    else                                   
                    {
                        /* If NUF_SHARE is returned that means the file is already open,
                        which mean it already exist in the system. */
                        if(fd == NUF_SHARE)
                        {
                            b_file_exist = NU_TRUE;
                        }
                        else
                        {
                            b_file_exist = NU_FALSE;
                        }                    
                    }
                }
            }

            if(fd == NU_SUCCESS)
            {
                /* Convert VFS flags, into internal Safe modes. */
                fd = wr_safe_open_convert_mode(flag, &safe_mode[0], b_file_exist, b_open_for_wr);  

                if(fd == NU_SUCCESS)
                {
                    /* Open the file in Safe. */
                    f = f_open(name, safe_mode);
                    if (!f)
                    {
                        /* If user didn't supply PO_CREAT flag and file doesn't
                        exist in the system, then this error is generated. */
                        fd = NUF_NOFILE;
                    }
                    else 
                    {                        
                        /* If file exist, file pointer may need to be adjusted. 
                        File pointer adjustment is the result of the VFS flags
                        and safe modes not being a 1 to 1 mapping. */
                        if(b_file_exist == NU_TRUE)
                        {
                            fd = wr_safe_open_move_fp(f, flag);
                        }

                        /* If a file has been opened, and the file pointer moved successfully or not at all,
                        then set the wrapper layer permissions. 
                        Wrapper layer permissions are needed, because some VFS flag combinations
                        that specify write or read only get converted to a Safe mode that allows 
                        read/write access. These wrapper layer permissions enforce the VFS user
                        supplied flags. So if PO_WRONLY is passed in but converted to Safe Read/Write,
                        then the wrapper layer will prevent any read operations on this file.                  
                        */
                        if(fd == NU_SUCCESS)
                        {
                            fd = wr_safe_open_wl_perm(&safe_wr_perm, flag);
                            
                            if(fd == NU_SUCCESS)
                            {   
                                for (i = 0; i < gl_FD_MAX_FD; i++)
                                {
                                    if (WrSafeFdMap[i] == NU_NULL)
                                    {
                                        /* If mode parameter is PS_IREAD only, make sure file
                                       attribute gets set to readonly. */
                                        if((b_file_exist == NU_FALSE) && (mode == PS_IREAD))
                                        {
                                            STATUS ret_val;
                                            UINT8 attr;

                                            /* Get the current file attribute. */
                                            ret_val = wr_safe_get_attr(&attr, name);
                                            if(ret_val == NU_SUCCESS)
                                            {
                                                attr |= ARDONLY;
                                                ret_val = wr_safe_set_attr(name, attr);
                                                if(ret_val != NU_SUCCESS)
                                                {                                                    
                                                    /* If error occurs from setting file attributes, then
                                                 make sure the file gets closed. Ignore return value from
                                                 f_close, because previous error from wr_safe_set_attr
                                                 should be returned. */
                                                    (VOID)f_close(f);
                                                    fd = ret_val;
                                                    break;
                                                }
                                            }
                                            else
                                            {                                                
                                                /* If error occurs from getting file attributes, then
                                             make sure the file gets closed. Ignore return value from
                                             f_close, because previous error from wr_safe_get_attr
                                             should be returned. */
                                                (VOID)f_close(f);
                                                fd = ret_val;
                                                break;
                                            }
                                        }
                                        
                                        /* Set the F_FILE pointer in the map */
                                        WrSafeFdMap[i] = f;
                                        fd = i;

                                        /* Assign wrapper layer permissions. */
                                        g_safe_wr_perm[i] = safe_wr_perm;

                                        break;
                                    }
                                }
                                
                                if (i == gl_FD_MAX_FD)
                                {   
                                    /* Case where f_open succeeds, but WrSafeFdMap is full */
                                    /* Close the file. If f_close returns error ignore it, so previous
                                  error is returned. */
                                    (VOID)f_close(f);
                                    
                                    /* Set error to internal since this case indicates a problem in Safe */
                                    fd = NUF_INTERNAL;
                                }
                            }                           
                        }
                    }    
                    
                    /* If an error was encountered in f_open, check to see if there
                    is compatible user defined error that will be more descriptive. */
                     if(fd != NU_SUCCESS)
                     {
                        STATUS user_error;
                        /* If an error occurred when attempting to open a file, 
                        try to determine more descriptive error code. */
                         user_error = wr_safe_open_user_errors(fs_user->p_errno);
                        if(user_error != NU_SUCCESS)
                        {
                            fd = user_error;
                        }                    
                     }
                 
                }                                    
            }            
        }
    }
    
    /* Release lock */
    NU_Release_Semaphore(&wr_fd_lock);

    return (fd);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_read
*
* DESCRIPTION
*
*       Wrapper function for reading data from a file.
*
* INPUTS
*
*       fd                                  File descriptor.
*       *buff                               Buffer to read data.
*       count                               Number of read bytes.
*
* OUTPUTS
*
*       INT32                               A non-negative integer to be
*                                            used as a number of bytes read.       
*       Error code                          If service is erroneous.
*
*************************************************************************/
INT32  wr_safe_read(INT fd, CHAR *buf, INT32 count)
{
    INT32 ret_val;
    INT prev_err;
    F_FILE *pF;

    pF = wr_safe_convert_fd(fd);
    if(pF != NU_NULL)
    {    
        /* fd range was validated in wr_safe_convert_fd no point in validating
          it again.          
          Verify user didn't set file as write only, but wrapper layer
          had to set it as read/write, see wr_safe_open for detailed
          explanation. */
        if(((g_safe_wr_perm[fd]) & SAFE_WR_PERM_OF_WRONLY) == 0 )
        {          
            if((count >= 0) && (buf))
            {
                prev_err = fs_user->p_errno;
                ret_val = f_read( buf, 1, count, pF);

                /* f_read returns number of bytes written. A value of zero usual indicates and error.
                   If return value is zero verify that an error wasn't set. */
                if(ret_val == 0)
                {
                    /* If user error isn't zero, then error was set in f_read. */
                    if(fs_user->p_errno != 0)
                    {
                        ret_val = (INT32)fs_user->p_errno;

                        /* If F_ERR_UNKNOWN was returned from Safe, 
                        just means there was nothing to read. 
                        So change ret_val to success. */           
                        if(ret_val == F_ERR_UNKNOWN)
                        {
                            ret_val = NU_SUCCESS;
                        }
                        else
                        {
                            ret_val = wr_safe_convert_error(ret_val);
                        }
                    }
                    else
                    {
                        /* Set error back to what it was. */
                        fs_user->p_errno = prev_err;
                    }
                }
            }
            else
                ret_val = NUF_BADPARM;
        }
        else
        {
            ret_val = NUF_ACCES;
        }
    }
    else
        ret_val = NUF_BADPARM;
        
    
    return (ret_val);   
    
}

/************************************************************************
* FUNCTION
*
*       wr_safe_write
*
* DESCRIPTION
*
*       Wrapper function for writing data to a file.
*
* INPUTS
*
*       fd                                  File descriptor.
*       *buf                                Pointer of buffer to write.
*       count                               Write byte count.
*
* OUTPUTS
*
*       INT32                               A non-negative integer to be
*                                            used as a number of bytes read.       
*       Error code                          If service is erroneous.
*
*************************************************************************/
INT32  wr_safe_write(INT fd, CHAR *buf, INT32 count)
{
    INT32 ret_val;
    INT prev_err;
    F_FILE *pF;

    pF = wr_safe_convert_fd(fd);
    if(pF != NU_NULL)
    {
        /* fd range was validated in wr_safe_convert_fd no point in validating
          it again.          
          Verify user didn't set file as write only, but wrapper layer
          had to set it as read/write, see wr_safe_open for detailed
          explanation. */
        if(((g_safe_wr_perm[fd]) & SAFE_WR_PERM_OF_RDONLY) == 0 )
        {
            if((count >= 0) && (buf))
            {            
                prev_err = fs_user->p_errno;
                ret_val = f_write( buf, 1, count, pF);
                
                /* f_write returns number of bytes written. A value of zero usual indicates and error.
                  If return value is zero verify that an error wasn't set. */
                if(ret_val == 0)
                {
                    /* If user error isn't zero, then error was set in f_write. */
                    if(fs_user->p_errno != 0)
                    {
                        ret_val = (INT32)fs_user->p_errno;

                        switch(ret_val)
                        {
                            
                            /* If F_ERR_UNKNOWN was returned from Safe, 
                            just means there was nothing to write. 
                            So change ret_val to success. */           
                            case F_ERR_UNKNOWN:

                                ret_val = NU_SUCCESS;
                                
                            break;

                            case F_ERR_NOTOPEN:

                                ret_val = NUF_ACCES;
                                
                            break;

                            default:
                                
                                ret_val = wr_safe_convert_error(ret_val);
                                
                            break;
                        }
                       
                    }
                    else
                    {
                        /* Set error back to what it was. */
                        fs_user->p_errno = prev_err;
                    }
                }
            }
            else
                ret_val = NUF_BADPARM;
        }
        else
        {
            ret_val = NUF_ACCES;
        }
    }
    else
        ret_val = NUF_BADPARM;
        
    
    return (ret_val);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_seek
*
* DESCRIPTION
*
*       Wrapper function for Safe's seek operation.
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
*                                            used as a number of bytes read.       
*       Error code                          If service is erroneous.
*
*************************************************************************/
INT32  wr_safe_seek(INT fd, INT32 offset, INT16 origin)
{
    INT ret;
    INT32 file_length;
    F_FILE *pF;
    INT32 safe_origin = 0;
    INT32 prev_fp = 0;
    

    /* Make sure fd is valid. */
    pF = wr_safe_convert_fd(fd);    
    if(pF)
    {

        /* Adjust the fp so that it is at the origin value passed in. */
        switch(origin)
        {
            /* If we are seeking relative to the current position,
              then remember current file pointer. */
            case PSEEK_CUR:
                
                safe_origin = F_SEEK_CUR;
                /* Get current file pointer position. */
                prev_fp = f_tell(pF);
                ret = F_NO_ERROR;
                break;            

            case PSEEK_END:

                /* Seek to the end of the file so we can determine the size. */
                ret = f_seek(pF,0,F_SEEK_END);
                if(ret == F_NO_ERROR)
                {                    
                    /* Set the prev_fp to the size of the file,
                    because user is seeking based off fp being 
                    equal to eof+1 */
                    prev_fp = f_tell(pF);
                    safe_origin = F_SEEK_END;                 
                }
                else
                {
                    ret = wr_safe_convert_error(ret);
                }

                break;                  

            case PSEEK_SET:

                /* User is seeking based off start of file. */
                ret = f_rewind(pF);
                if(ret == F_NO_ERROR)
                {
                    prev_fp = 0;
                    safe_origin = F_SEEK_SET;
                }
                break;            

            default:
                ret = NUF_BADPARM;
                break;

        }
        
        if(ret == F_NO_ERROR)
        {         
            /* Determine the length of the file. */            
            ret = f_seek(pF,0,F_SEEK_END);
            if(ret == F_NO_ERROR)
            {
                file_length = f_tell(pF);
                /* If user is trying to seek beyond file size, then set file pointer to eof+1. */            
                if((offset + prev_fp) > file_length)
                {
                    /* Seek to the end of the file, so we can determine the size. */
                    ret = f_seek(pF,0,F_SEEK_END);
                    if(ret == F_NO_ERROR)
                    {                    
                        /* Set the prev_fp to the size of the file,
                        because user is seeking based off fp being 
                        equal to eof+1 */
                        offset = file_length;      
                    }                     
                }
                else
                {    
                    /* Check to see if user is seeking past zero, if they are
                     an error needs to be returned. */
                    if( ((prev_fp + offset) < 0) )
                    {
                        ret = NUF_BADPARM;                        
                    }
                    else
                    {                           
                        /* At this point the offset value passed in is within range
                        (i.e offset+fp < file_length & offset+fp > 0). */
                        
                        if(safe_origin != F_SEEK_END) 
                        {
                            /* If user is seeking relative to current file pointer
                            position. Then the file pointer position needs to be
                            reset to where it was before, entering this function.
                            This is accomplished by adding the file pointer value
                            to the offset provided by the user. The value
                            (offset + prev_fp) at this point has to be within the
                            valid range. So (offset + prev_fp) > 0 and 
                            (offset + prev_fp) < the size of this file. */                                                       
                            offset += prev_fp;      
                            ret = f_seek(pF, offset, F_SEEK_SET);                                                                  
                        }
                        else
                        {   
                            ret = f_seek(pF, offset, F_SEEK_END);
                            /* If we have reached this point offset has to be a negative value,
                            therefore subtract it from the file size. */                        
                            offset += file_length;
                        }               
                    }                
                }

               /* Determine if there was an error, if not return bytes seeked. */
               if(ret == F_NO_ERROR)
               {
                    ret = offset;
               }
               else
               {
                    ret = wr_safe_convert_error(ret);
               }
            }        
        }      
    }
    else
    {
        ret = NUF_BADFILE;       
    }

   return ret;
}

/************************************************************************
* FUNCTION
*
*       wr_safe_close
*
* DESCRIPTION
*
*       Wrapper function for Safe's close file operation.
*
* INPUTS
*
*       fd                                  File descriptor.
*
* OUTPUTS
*
*       NU_SUCCESS                          The file was successfully 
*                                            closed.    
*       Error code                          If service is erroneous.
*
*************************************************************************/
STATUS wr_safe_close(INT fd)
{
    INT ret;
    
    /* Grab wr_fd_lock */
    NU_Obtain_Semaphore(&wr_fd_lock, NU_SUSPEND);

    ret = f_close (wr_safe_convert_fd(fd));
    
    if (fd >= 0 && fd < gl_FD_MAX_FD)
    {
        WrSafeFdMap[fd] = NU_NULL;
        /* Don't have to clear g_safe_wr_perm[fd], because
          the file descriptors are checked for validation
          before ever accessing the wrapper layer permission
          table. */
    }
    
    /* Release lock */
    NU_Release_Semaphore(&wr_fd_lock);
    
    return (wr_safe_convert_error(ret));
        
}

/************************************************************************
* FUNCTION
*
*       wr_safe_delete
*
* DESCRIPTION
*
*       Wrapper function for Safe's file deletion operation.
*       Currently only "*", "*." and "*.*" wildcards are allowed in the last
*       path entry. 
*
*       Examples of valid values of name
*          sub1\sub2\*.* 
*          sub1\sub2\*.<insert extension> 
*          sub1\sub2\* 
*          *.* 
*
*       Examples of invalid values
*          sub1\*\*.* is invalid
*          sub1\su?2\*.* is invalid
*
* INPUTS
*
*       *name                               File name to be deleted.
*
* OUTPUTS
*
*       NU_SUCCESS                          Delete request completed
*                                            successfully.  
*       NUF_NOFILE                          The specified file not found. 
*       Error code                          If service is erroneous.
*
*************************************************************************/
STATUS wr_safe_delete(CHAR *name)
{
    STATUS sts = NU_SUCCESS;
    INT found_match = NU_FALSE;
    F_FIND f;
    CHAR parent_path[F_MAXPATH] = {0};    
    INT16 path_idx;
    UINT8 b_wc_found = NU_FALSE;
    UINT8 b_name_is_path = NU_FALSE;
       
    /* There are four possible situations that have to be accounted for:
       Case 1: path with wildcard as last entry
       Case 2: path with no wildcards.
       Case 3: no path just wildcard
       Case 4: no path just filename to be deleted. */

    /* If path/name contains wildcard characters*/

    /* First check to see if user passed in path. */
    for(path_idx = 0; ((path_idx < F_MAXPATH) && (name[path_idx] != FS_SEPARATORCHAR) && (name[path_idx] != '\0')); ++path_idx);

    /* Name contains a path. */
    if(name[path_idx] == FS_SEPARATORCHAR)
    {
        b_name_is_path = NU_TRUE;                      

    }
    
    /* Now see if there is a wildcard at the end of the path.
      Save the path, incase we need it later so we don't have to
      traverse string again. */
    for(path_idx = 0; ((path_idx < F_MAXPATH) && (name[path_idx] != '\0') && (sts == NU_SUCCESS)); ++path_idx)
    {
        if(name[path_idx] == '*')
        {
            b_wc_found = NU_TRUE;
            /* Verify that wildcard character is in last path name. i.e path1/path2/"*.*" */
            while(name[++path_idx] != '\0')
            {
                /* If wildcard character is not in last name return error. */
                if(name[path_idx] == FS_SEPARATORCHAR)
                {
                    b_wc_found = NU_FALSE;
                    sts = NUF_BADPARM;                        
                    break;
                }
            }
            /* Make sure we break out of parent loop as well. */
            break;

        }
        else
            parent_path[path_idx] = name[path_idx];
    }

    if(sts == NU_SUCCESS)
    {
        /* If doesn't contain a wildcard character then
          just delete it.(Case 2 and Case 4) */
        if(((!b_wc_found) && (b_name_is_path)) || ((!b_wc_found) && (!b_name_is_path)))
        {
            sts = f_delete(name);
            if(sts != F_NO_ERROR)
                sts = wr_safe_convert_error(sts);         

        }
        /* If name has a wildcard character in it. (Case 1 and Case 3). */
        else
        {           

            /* At this point we know we have a wildcard character and name is either
              path or the wildcard character. */

             /* If name is a path with the last name as a wilcard,
              then set curr_path to remember the parent
              path. This way filenames can be appended to the parent
              path and passed into f_delete. */
            if(b_name_is_path)
            {
                /* Set curr_path back to last FS_SEPARATORCHAR. */
                while((path_idx > 0) && (parent_path[--path_idx] != FS_SEPARATORCHAR));
                
                /* Null terminate it add one because we want '\0' after FS_SEPARATORCHAR. */
                parent_path[++path_idx] = '\0';   
            }
            /* Because name isn't a path, we are using
             the cwd as our path which is default
             path used. */
            else
            {
                path_idx = 0;
            }
            
            /* So now traverse though the files in curr_path*/
            sts = safe_get_first_wc_match(&f,name,WC_FILE);
            if (sts == F_NO_ERROR)
            {
               do
               {                   
                    /* Add in filename to the parent path */
                   NUF_Ncpbuf((UINT8*)&parent_path[path_idx],(UINT8*)f.filename,FS_MAXPATH);
                                 
                   /* Now remove the file in the parent directory. */
                   sts = f_delete(&parent_path[0]);                  
                                           
                   if (sts != F_NO_ERROR)
                   {
                       /* Not an error if we've already found a match */
                       if ((sts == F_ERR_NOTFOUND) && (found_match == NU_TRUE))
                           break;
                       else
                           sts = wr_safe_convert_error(sts);
                       break;
                   }
                   else
                       found_match = NU_TRUE;
                   
               }while(safe_get_next_wc_match(&f, WC_FILE) == F_NO_ERROR);
            }
            else
            {
                sts = wr_safe_convert_error(sts);
            }
        }
    }    
    
    return (sts);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_utime
*
* DESCRIPTION
*
*       File system operation wrapper for non-supported operation.
*
* INPUTS
*
*       *statobj                            DSTAT object who's times
*                                            and dates should be updated
*                                            to the passed in values.
*       access_date                         New Access Date.
*       access_time                         New Access Time.
*       update_date                         New Update Date.
*       update_time                         New Update Time.
*       create_date                         New Create_Date.
*       create_time                         New Create_Time.
*
* OUTPUTS
*
*       NUF_INTERNAL                            
*
*************************************************************************/
STATUS wr_safe_utime (DSTAT *statobj,UINT16 access_date,UINT16 access_time,
                            UINT16 update_date,UINT16 update_time,UINT16 create_date,
                            UINT16 create_time)
{
    /* Resolve unused param. */
    Safe_Unused_Param = (UINT32)statobj;
    Safe_Unused_Param = access_date;
    Safe_Unused_Param = access_time;
    Safe_Unused_Param = update_date;
    Safe_Unused_Param = update_time;
    Safe_Unused_Param = create_date;
    Safe_Unused_Param = create_time;
    return (NUF_INTERNAL);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_set_attr
*
* DESCRIPTION
*
*       File system operation wrapper for non-supported operation.
*
* INPUTS
*
*       *name                               File name.
*       newattr                             New file attribute.
*
* OUTPUTS
*
*       NU_SUCCESS                            
*
*************************************************************************/
STATUS wr_safe_set_attr(CHAR *name, UINT8 newattr)
{
    INT ret;
    UINT32 psecure;
    STATUS ret_val;

    psecure = (UINT32)newattr;

   /* Safe's attributes are as follows:
         Archive:   0x40
         Directory: 0x20
         Volume:    0x10  -- This attribute isn't used in Safe.
         System:    0x08
         Hidden:    0x04
         Readonly:  0x02
         so they must be converted to match the VFS attributes. */

   /* Verify user isn't attempting to set volume attribute. */
    if(!(psecure&AVOLUME))
    {
        /* Safe needs the attributes in the most significant byte of a UINT32,
           VFS only stores them in a UINT8 so account for this. */
        psecure <<= 25;

        ret = f_setpermission(name,psecure);
        ret_val = wr_safe_convert_error(ret);
    }
    else
        ret_val = NUF_BADPARM;

   return ret_val;

}

/************************************************************************
* FUNCTION
*
*       wr_safe_get_attr
*
* DESCRIPTION
*
*       File system operation wrapper for non-supported operation.
*
* INPUTS
*
*       attr                                Attribute
*       name                                File name
*
* OUTPUTS
*
*       NU_SUCCESS                            
*       
*************************************************************************/
STATUS wr_safe_get_attr(UINT8 *attr, CHAR *name)
{
   INT ret;
   UINT32 psecure;    

   ret = f_getpermission(name, (unsigned long*)&psecure);
   
   /* Safe attributes are returned in the most significant byte of a UINT32,
      so shift them to least significant byte. */    
   *attr = (UINT8)(psecure>>25);
   /* Safe's attributes are as follows:
      Archive:   0x40
      Directory: 0x20
      Volume:    0x10  -- This attribute isn't used in Safe.
      System:    0x08
      Hidden:    0x04
      Readonly:  0x02
      so they must be converted to match the VFS attributes. */

   return (wr_safe_convert_error(ret));
   
}

/************************************************************************
* FUNCTION
*
*       wr_safe_truncate
*
* DESCRIPTION
*
*       Wrapper function for Safe's file truncate operation.
*
* INPUTS
*
*       fd                                  File descriptor.
*       offset                              Truncate offset(bytes).
*
* OUTPUTS
*
*       NU_SUCCESS                          File was truncated.  
*       Error code                          If service is erroneous.
*
*************************************************************************/
STATUS wr_safe_truncate(INT fd, INT32 offset)
{
    INT ret;

    ret = f_ftruncate(wr_safe_convert_fd(fd), offset);
    
    return (wr_safe_convert_error(ret));
}

/************************************************************************
* FUNCTION
*
*       wr_safe_flush
*
* DESCRIPTION
*
*       Wrapper function for Safe's flush operation.
*
* INPUTS
*
*       fd                                  File descriptor.       
*
* OUTPUTS
*
*       NU_SUCCESS                          File system information
*                                            was flushed.  
*       Error code                          If service is erroneous.
*
*************************************************************************/
STATUS wr_safe_flush(INT fd)
{
    INT ret;

    ret = f_flush(wr_safe_convert_fd(fd));
    
    return (wr_safe_convert_error(ret));
}

/************************************************************************
* FUNCTION
*
*       wr_safe_mkdir
*
* DESCRIPTION
*
*       Wrapper function for Safe's make directory operation.
*
* INPUTS
*
*       name                                Path name.
*
* OUTPUTS
*
*       NU_SUCCESS                          Directory was created.
*       Error code                          If service is erroneous.
*
*************************************************************************/
STATUS wr_safe_mkdir(CHAR *name)
{
    INT ret;
    
    ret = f_mkdir(name);

    return (wr_safe_convert_error(ret));
}

/************************************************************************
* FUNCTION
*
*       wr_safe_rmdir
*
* DESCRIPTION
*
*       Wrapper function for Safe's remove directory operation.
*
* INPUTS
*
*       *name                               Path name.
*
* OUTPUTS
*
*       NU_SUCCESS                          Directory was removed.
*       Error code                          If service is erroneous.
*
*************************************************************************/
STATUS wr_safe_rmdir(CHAR *name)
{
    INT ret;
    
    ret = f_rmdir(name);
    
    return (wr_safe_convert_error(ret));
}

/************************************************************************
* FUNCTION
*
*       wr_safe_rename
*
* DESCRIPTION
*
*       Wrapper function for Safe's rename operation.
*
* INPUTS
*
*       *name                               Old file name
*       *newname                            New file name(Rename)
*
*
* OUTPUTS
*
*       NU_SUCCESS                          Directory was renamed.
*       Error code                          If service is erroneous.
*
*************************************************************************/
STATUS wr_safe_rename(CHAR *name, CHAR *newname)
{
    INT ret;
        
    ret = f_move(name,newname);
    
    return (wr_safe_convert_error(ret));
}


/************************************************************************
* FUNCTION
*
*       wr_safe_f_find_to_dstat
*
* DESCRIPTION
*
*       Convert a Safe F_FIND structure into a VFS DSTAT.
*
* INPUTS
*
*       *statobj                            VFS DSTAT to return
*       *pF                                 Safe F_FIND structure to convert
*
*
* OUTPUTS
*
*       NU_SUCCESS                          Directory was renamed.
*       Error code                          If service is erroneous.
*
*************************************************************************/
static VOID wr_safe_f_find_to_dstat(DSTAT *statobj, F_FIND *pF)
{
    /* Copy the lfn. */
    NUF_Ncpbuf((UINT8*)&statobj->lfname[0],(UINT8*)&pF->filename[0],FS_MAXPATH);

    /* We are only concerned with the last byte as it has the file
       attributes. */
    statobj->fattribute = (UINT8)(pF->secure>>25);

    /* Add in directory attribute. */
    if (pF->attr & F_ATTR_DIR)
    {
      statobj->fattribute &= ADIRENT;
    }

    statobj->fcrtime = pF->ctime;
    statobj->fcrdate = pF->cdate;
    statobj->fsize = pF->filesize;

    /* Zero/NULL out members not used. */
    statobj->sfname[0] = '\0';
    statobj->fext[0] = '\0';
    statobj->fcrcmsec = 0;
    statobj->faccdate = 0;
    statobj->fclusterlow = 0;
    statobj->fclusterhigh = 0;
    statobj->fupdate = 0;
    statobj->fuptime = 0;    

}

/************************************************************************
* FUNCTION
*
*       wr_safe_get_first
*
* DESCRIPTION
*
*       Wrapper function for Safe's get first operation.
*
* INPUTS
*
*       *statobj                            Caller's buffer to put file
*                                            info.
*       *pattern                            Path to find.
*
* OUTPUTS
*
*       NU_SUCCESS                          Search for the first match
*                                            pattern was successful.
*       NU_UNAVAILABLE                      If can't allocated a F_FIND. 
*       Error code                          If service is erroneous.                                  
*
*************************************************************************/
STATUS wr_safe_get_first(DSTAT *statobj, CHAR *pattern)
{
    INT ret;
    F_FIND *pF = wr_safe_alloc_f_find();

    if (pF)
    {
        statobj->fs_private = (VOID*)pF;

        ret = f_findfirst(pattern, pF );
        if(ret == NU_SUCCESS)
        {
            wr_safe_f_find_to_dstat(statobj,pF);
        }
        else
        {
            ret = wr_safe_convert_error(ret);
            /* If error is returned, make sure pF is deallocated. */
           (VOID)wr_safe_dealloc_f_find(pF);
        }       
        
    }
    else
    {
        ret = NU_UNAVAILABLE;
    }

    return ret;
}

/************************************************************************
* FUNCTION
*
*       wr_safe_get_next
*
* DESCRIPTION
*
*       Wrapper function for Safe's get next operation.
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
*       Error code                          If service is erroneous.                                   
*
*************************************************************************/
STATUS wr_safe_get_next(DSTAT *statobj)
{
    INT ret;
    F_FIND *pF;
    
    pF = (F_FIND*)statobj->fs_private;
    
    ret = f_findnext(pF);
    
    if(ret == NU_SUCCESS)
    {
        wr_safe_f_find_to_dstat(statobj,pF);
    }
    else
        ret = wr_safe_convert_error(ret);

   return ret;
}

/************************************************************************
* FUNCTION
*
*       wr_safe_done
*
* DESCRIPTION
*
*       Wrapper function for Safe's done operation.
*
* INPUTS
*
*       *statobj                            Caller's buffer to 
*                                            put file info.
*
*
* OUTPUTS
*
*       NU_SUCCESS                          Resources used for find
*                                            operations were all freed.
*
*************************************************************************/
STATUS wr_safe_done(DSTAT *statobj)
{
    return wr_safe_dealloc_f_find((F_FIND*)statobj->fs_private);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_format
*
* DESCRIPTION
*
*       File system operation wrapper for Safe's format function.
*       For proper drive allocation for format, FS_MAXVOLUME should be 
*       set to n + 1 so format has an available temporary drive index.      
*
* INPUTS
*
*       dh                                  Disk handle.
*       params                              Format parameters.
*
* OUTPUTS
*
*       NU_SUCCESS                          If format succeeds.
*       Error code                          If service is erroneous.                                   
*
*************************************************************************/
STATUS wr_safe_format(UINT16 dh, VOID **params)
{
    STATUS status;
    STATUS safe_stat = NU_SUCCESS;
    INT free_drv_num = -1; /* Initialize to -1 to ensure, value gets properly set, because*/
    WR_SAFE_DEV_CB *dev_cb;
    FDRV_LOG_CB_S *plog_cb;
    FDRV_PHY_CB_S *pphy_cb; 
    WR_SAFE_MNT_PARAMS *mnt_params = NU_NULL;
    UINT8 b_already_safe_fs_user = NU_TRUE;
    INT i;

    /* Resolve unused parameters. */
    Safe_Unused_Param = (UINT32)*params;

    /* Get the logical control block */
    status = fsdh_get_dh_specific(dh, (VOID**) &plog_cb, NU_NULL);
    if((status == NU_SUCCESS)&&(plog_cb == NU_NULL))
    {
        status = NUF_INTERNAL;
    }   
    if (status == NU_SUCCESS)
    {
        /* Get the physical control block */
        status = fsdh_get_dh_specific(plog_cb->fdrv_vfs_dh, (VOID**) &pphy_cb, NU_NULL);
        if((status == NU_SUCCESS)&&(pphy_cb == NU_NULL))
        {
            status = NUF_INTERNAL;
        }   
        if (status == NU_SUCCESS)
        {
            /* Get the Safe device specific control block */ 
            dev_cb = pphy_cb->fdrv_spec;
            if(dev_cb != NU_NULL)
            {
                mnt_params = &dev_cb->safe_mnt_params;
                /* See if the task is file system user,
                  VFS considers format to be a system wide operations
                  so to the VFS you don't have to be a user
                  to perform this operation. However, Safe requires
                  file system users for all operations, so if we
                  aren't a file system user become one for this
                  call. */
                if (NU_Check_File_User() == NUF_BAD_USER)
                {
                    /* Register task to access SAFE */
                    status = NU_Become_File_User();   
                    b_already_safe_fs_user = NU_FALSE;
                }
            }
            else
            {
                /* If dev_cb is NU_NULL and no error was returned from
                 fsdh_get_dh_specific, then FS_Disk_Handles has 
                 become corrupted. */
                status = NUF_INTERNAL;
            }
        }
    }

    if((status == NU_SUCCESS) && (mnt_params != NU_NULL)) 
    {   
        /* Get a free drive number in Safe, so we can format drive. This is done
          because a drive must be mounted in Safe in order to perform the format
          operation, where as with the VFS drives must not be mounted in order
          to format. */                
        for (i = 0; i < FS_MAXVOLUME; i++ )
        {
            if(fg_filesystem.vd[i].state == FS_VOL_NOTMOUNT)
            {
                free_drv_num = i;
                break;
            }
        }
        if(free_drv_num >= 0)
        {			
			/* Call mount to initialize system structures */
			safe_stat = f_mountdrive(free_drv_num, mnt_params->buffer, mnt_params->buffsize, mnt_params->mountfunc, mnt_params->phyfunc);
			if ((safe_stat == NU_SUCCESS) || (safe_stat == FS_VOL_NOTFORMATTED))
			{
				/* Call SAFE format */
				safe_stat = f_format(free_drv_num);
			}
	        
			/* If mount was successful and unmount fails we want to remember unmounts error code. */
			if(safe_stat == NU_SUCCESS)
			{                 
				/* Unmount */
				safe_stat = f_unmountdrive(free_drv_num); 
			}
			else
			{
				/* If mount fails remember that error code. */
				f_unmountdrive(free_drv_num);
			}			
        }
        else
        {
            /* There are no more free mount entries in the Safe file system. */
            status = NUF_INTERNAL;

        }
    }

    /* Release file user because user wasn't one when entering this function.*/    
    if (!b_already_safe_fs_user)
    {
        /* Release task access */
        if(status == NU_SUCCESS)
        {            
            status = NU_Release_File_User();
        }
        else
        {
            (VOID)NU_Release_File_User();
        }
    }


    if(status == NU_SUCCESS)
        status = wr_safe_convert_error(safe_stat);    

    return (status);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_get_format_info
*
* DESCRIPTION
*
*       File system operation wrapper to support VFS function.
*       Currently **params is ignored, because there are no
*       format specific information for the devices.
*
* INPUTS
*
*       dh                                  Disk handle.
*       params                              Format parameters.
*
* OUTPUTS
*
*       NU_SUCCESS                            
*       
*************************************************************************/
STATUS wr_safe_get_format_info(UINT16 dh, VOID **params)
{
    STATUS status = NU_SUCCESS;

    /* Resolve unused parameters. */
    Safe_Unused_Param = dh;

    /* Currently Safe doesn't have any file system specific
      format parameters, so assign *params to NU_NULL. */
    *params = NU_NULL;
    
    return (status);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_freespace
*
* DESCRIPTION
*
*       File system operation wrapper for non-supported operation.
*
* INPUTS
*       
*       dh                                  Disk handle.
*       secpcluster                         Sector per cluster.
*       bytepsec                            Bytes per sector.
*       freecluster                         Number of free clusters.
*       totalcluster                        Number of total clusters.
*
* OUTPUTS
*
*       NUF_INTERNAL                            
*
*************************************************************************/
STATUS wr_safe_freespace(UINT16 dh, UINT8 *secpcluster,
                       UINT16 *bytepsec, UINT32 *freecluster,
                       UINT32 *totalcluster)
{
    STATUS ret_val;
    INT safe_stat;
    INT currentdrive;
    F_SPACE space;
    
    ret_val = fs_dev_dskchk_proc(dh);
    if(ret_val == NU_SUCCESS)
    {
        /* Get safe drive number associated with this disk handle. */
        currentdrive = wr_safe_convert_dh_to_drvnum(dh);
        if(currentdrive >= 0)
        {
            /* Get the freespace information from the file system. */
            safe_stat = f_getfreespace(currentdrive, &space);

            /* Convert Safe error code to VFS error code. */
            ret_val = wr_safe_convert_error(safe_stat);            
            if(ret_val == NU_SUCCESS)
            {
                *bytepsec = space.sec_size;
                /* Safe uses the concept of blocks as clusters. */
                *secpcluster = space.sec_per_blk;
                *freecluster = (space.free / (space.sec_per_blk * space.sec_size));
                *totalcluster = space.total / (space.sec_per_blk * space.sec_size); 
            }            
        }
        else
        {   
            /* Disk user is requesting information for isn't
              in the system. */
            ret_val = NUF_NOT_OPENED;
        }        
    }

    return(ret_val);     
    
}

/************************************************************************
* FUNCTION
*
*       wr_safe_vnode_allocate
*
* DESCRIPTION
*
*       Wrapper function for Safe's vnode allocation operation.
*
* INPUTS
*
*       dh                              Disk handle.
*       *path                           String containing path of object.
*       **fsnode                        FS specific node.
*
* OUTPUTS
*
*       NU_SUCCESS                      Vnode was allocated
*                                           
*************************************************************************/
STATUS wr_safe_vnode_allocate(UINT16 dh, CHAR *path, VOID **fsnode)
{
    STATUS ret_stat;
    MTE_S *mte;
    UINT32 idx;
    FS_FIND fs;
    CHAR previous_cwd[EMAXPATH+3];  /* +3 to account for drive letter(A:\\) */
    CHAR new_cwd[EMAXPATH +3];    /* +3 to account for drive letter(A:\\) */
    CHAR *path_trav;
    UINT32 cwd_idx;
     /* 4 because A:\\ and NULL terminator. */
    CHAR root_dir[4]; 
    UINT8 b_found_dots = NU_FALSE;
    UINT8 drive_letter_offset_idx = 2; 

    ret_stat = fs_dev_dskchk_proc(dh);
    /* Verify that the drive exists */
    if (ret_stat == NU_SUCCESS)
    {
        mte = fsl_mte_from_dh(dh);

        if(mte != NU_NULL)
        {     
            /* Check if path contains dot slash(.\) or dot dot slash(..\),
              if it does the wrapper layer will have to parse the string.
              Because there is potential for the user to be trying to set
              the cwd to the roots parent. In that case the cwd
              should be set to root. However, the Safe file system
              would return an error, but that is not what expected
              behavior when interfacing with VFS. */


            /* path has to be a valid string because it was checked in 
              NU_Set_Current_Dir(which is in the file fs_env.c). */
            path_trav = path;
            do{
                /* Make sure there is either a ".\" or "..\" before we parse the string. */
                if(((*path_trav == '.') && (*(path_trav+1) == '.') && (*(path_trav+2) == FS_SEPARATORCHAR)) ||
                    ((*path_trav == '.') && (*(path_trav+1) == FS_SEPARATORCHAR)))
                {
                    b_found_dots = NU_TRUE;
                    break;                    
                }                        
                
                /* Loop until next path entry, where path entry is: 
                 path_entry\\path_entry2\\..etc.*/
                while((*path_trav++ != FS_SEPARATORCHAR) && (*path_trav != '\0'));

            }while(*path_trav != '\0');

            /* If we found a dot or a dot dot as a path entry, then parse and verify path. */
            if(b_found_dots == NU_TRUE)
            {  
                
                /* Make previous_cwd and new_cwd based off absolute path,
                 so both default and non-default drives cwd are handled. */
                 
                /* In the VFS layer a check is done to see if the path 
                 contains a drive letter or not, if it does then the dh
                 is changed to that drive, so mte->mte_drive can always be 
                 assumed to be the correct drive. */
                new_cwd[0] = previous_cwd[0] = root_dir[0] = (CHAR)mte->mte_drive + 'A';
                new_cwd[1] = previous_cwd[1] = root_dir[1] = ':'; 

                /* When getting the cwd it will add the slash and NULL terminated character,
                 to previous_cwd, so don't add a slash and NULL terminated char. */
                new_cwd[2] = root_dir[2] = FS_SEPARATORCHAR;
                new_cwd[3] = previous_cwd[2] = root_dir[3] = '\0';
                 

                /* If path isn't an absolute path, get drives cwd. */
                if((path[1] != ':') && (path[0] != FS_SEPARATORCHAR))
                {
                    /* This will add the slash and NULL terminator to previous_cwd. */                 
                    ret_stat = f_getdcwd(mte->mte_drive, &previous_cwd[drive_letter_offset_idx], EMAXPATH);                     
                }
                else
                {                       
                    /* Don't have to worry about case where path contains a drive letter, 
                    because new_cwd, previous_cwd, and root_dir are initialized to
                    mte->mte_drive. So if user supplied drive letter it is was already 
                    set, however if they supplied a slash as the first character in 
                    path, this means there path is relative to default drives root
                    directory. */
                    if(path[0] == FS_SEPARATORCHAR)
                    {
                        /* Set default drive letter,  */
                        new_cwd[0] = previous_cwd[0] = root_dir[0] = (CHAR)fs_user->dfltdrv + 'A';
                    }
          
                    ret_stat = F_NO_ERROR;
                }

                cwd_idx = NUF_Get_Str_Len(previous_cwd);
                
                if(ret_stat == F_NO_ERROR)
                {                           
                    /* Copy previous_cwd to new_cwd so that the dot or dot dots in the
                    path can be processed. */
                    NUF_Copybuff(new_cwd, previous_cwd, cwd_idx+1);                    

                    /* Assign path to traversal pointer. */      
                    path_trav = path;
                   
                    /* Because path_trav could contain driver letter or slash at the 
                    beginning of the string skip them. */                    
                    if((path_trav[1] == ':') || (path_trav[0] == FS_SEPARATORCHAR))
                    {
                        if(path_trav[1] == ':')
                        {
                            /* If path_trav contains drive letter, skip it. */
                            path_trav += 3;

                        }
                        else
                        {   /* If path_trav contains slash slash, skip it.*/                            
                            ++path_trav;
                        }
                       
                    }  
                    /* If new_cwd is currently root set new_cwd[drive_letter_offset_idx] to NULL. */
                    if(new_cwd[drive_letter_offset_idx] == FS_SEPARATORCHAR && new_cwd[drive_letter_offset_idx+1] == '\0')
                    {
                        new_cwd[drive_letter_offset_idx] = '\0';
                        cwd_idx = drive_letter_offset_idx;
                    }

                    /* Loop until we reach the end of path. */
                    while(*path_trav != '\0')
                    {
                        /* If path isn't a dot, check for dot dots, and validate path entry. Else
                        move to the next path entry. */
                        if(*path_trav != '.' || ((*(path_trav+1) != FS_SEPARATORCHAR) && (*(path_trav+1) != '\0')))
                        {
                            /* If current path entry is dot dot, verify that is valid */
                            if((*path_trav == '.') && (*(path_trav+1) == '.'))
                            {       
                                /* Last path entry is .. and new cwd is root, so return an error, 
                               because the root doesn't contain a dot dot. */
                                if((*(path_trav+2) == '\0') && (new_cwd[drive_letter_offset_idx] == '\0'))
                                {                                        
                                     ret_stat = NUF_NOFILE;
                                     break;
                                }
                                else
                                {    
                                    /* This means path entry is either dot dot and new_cwd isn't root,
                                    which is valid or path entry is ..\\ in which case is valid
                                    no matter what new_cwd is. */
                                    if((*(path_trav+2) == '\0') || (*(path_trav+2) == FS_SEPARATORCHAR))
                                    {
                                        /* Remove last(right most) path entry from our new cwd. */
                                        while((cwd_idx > drive_letter_offset_idx) && (new_cwd[--cwd_idx] != FS_SEPARATORCHAR));
                                        new_cwd[cwd_idx] = '\0';                            
                                    }
                                }

                            }
                            /* This means that path_trav should be pointing to a sub directory
                           in its path. */
                            else
                            {
                                /* Add a slash. */                                   
                                new_cwd[cwd_idx++] = FS_SEPARATORCHAR;
                                
                                /* Append path entry to new cwd. */
                                do{
                                    new_cwd[cwd_idx++] = *path_trav;                                    

                                }while((*(++path_trav) != '\0') && (*path_trav != FS_SEPARATORCHAR));

                                /* NULL terminate our string. */
                                new_cwd[cwd_idx] = '\0';
                                
                                /* Verify directory exist, if it doesn't
                               return error. */
                                ret_stat = f_findfirst(new_cwd,&fs);
                                if(ret_stat != F_NO_ERROR)
                                    break;
                            }
                        }

                        
                        /* Traverse to next path entry. */                        
                        while((*path_trav != '\0') && (*path_trav++ != FS_SEPARATORCHAR));
                        
                    }

                    /* Set new CWD. */                        
                    if(ret_stat == F_NO_ERROR)
                    {
                        /* Verify that user isn't trying to access dot when the new cwd is 
                        root. */                            
                        if((new_cwd[drive_letter_offset_idx] == '\0') &&                                
                             ((*(path_trav-1) == '.') && (*(path_trav-2) == FS_SEPARATORCHAR)))
                        {  
                                ret_stat = f_chdir(previous_cwd);                                    
                                if(ret_stat == F_NO_ERROR)
                                {
                                    ret_stat = NUF_NOFILE;
                                }
                        }
                        else
                        {
                            /* If cwd is root and last entry in path isn't a dot, set 
                           cwd to root. */
                            if(new_cwd[drive_letter_offset_idx] == '\0')
                            {
                                 ret_stat = f_chdir(root_dir);
                            } 
                            /* Set to new cwd passed in by user. */
                            else
                            {                                    
                                ret_stat = f_chdir(new_cwd);                                          
                            }

                       }

                    }                        
                    else
                    {
                        STATUS prev_error = ret_stat;                            
                        /* If code reaches this point then the path passed in 
                        was not correct, so reset CWD to what it was before entering
                        this function. */
                        ret_stat = f_chdir(previous_cwd);
                        if(ret_stat == F_NO_ERROR)
                        {
                            ret_stat = prev_error;
                        }
                    }                                  
                
                }
                
            }
            else                
            {
                /* Set cwd in Safe, this will handle the path processing for
                  dots, and non default drive. */   
                if(ret_stat == NU_SUCCESS)                
                {

                     /* Special error case: If user is in root directory
                     and path is setting to root followed by a 
                     dot with no slash after it. Example "A:\\." or "\\.". 
                     then error should be returned because root doesn't 
                     contain a dot or a dot dot. */
                     
                     /* First test is for drive letter, second is for 
                     accessing root based of slash. */                     
                    if(((path[1] == ':') && (path[3] == '.') && (path[4] == '\0')) ||
                       ((path[0] == FS_SEPARATORCHAR) && (path[1] == '.') && (path[2] == '\0')))
                    {
                        ret_stat = NUF_NOFILE;
                        
                    }
                    else
                    {
                        /* Get the cwd of the drive. */
                        ret_stat = f_getdcwd(mte->mte_drive, previous_cwd, EMAXPATH);                               
                        if(ret_stat == F_NO_ERROR)
                        {
                            /* Handle error case where CWD is root and user is trying to 
                            access dot or dot dot. */
                            if(((previous_cwd[0] == FS_SEPARATORCHAR) && (previous_cwd[1] == '\0')) &&
                              (((path[0] == '.') && (path[1] == '\0')) ||
                              ((path[0] == '.') && (path[1] == '.') && (path[2] == '\0'))))
                            {
                                ret_stat = NUF_NOFILE;
                            }
                            else
                            {
                                ret_stat = f_chdir(path); 
                            }
                        }
                    }
                }            

            }

            /* If cwd was set successfully in Safe, we need to set our
              fsnode to its value. Every file system user has an entry into
              this array, which also contains the cwd of each volume/drive. */
            if(ret_stat == F_NO_ERROR)
            {
                idx = fsu_get_user_index();
                *fsnode = (VOID*)&gl_multi[idx].fs_vols[mte->mte_drive].cwd[0];             
            }
            else
                ret_stat = wr_safe_convert_error(ret_stat);
        }          
                        
    }
    else
    {
        ret_stat = NUF_NO_DISK;
    }

    return(ret_stat);
}

/************************************************************************
* FUNCTION
*
*       wr_safe_vnode_deallocate
*
* DESCRIPTION
*
*       Wrapper function for Safe's vnode deallocation operation.
*
* INPUTS
*
*       dh                              Disk handle
*       *fsnode                         FS node to deallocate
*
* OUTPUTS
*
*       NU_SUCCESS                      Vnode was deallocated
*                                           
*************************************************************************/
STATUS wr_safe_vnode_deallocate(UINT16 dh, VOID *fsnode)
{
    
    /* Resolve unused parameters. */
    Safe_Unused_Param = dh;    
    
    fsnode = NU_NULL;
    
    Safe_Unused_Param = (UINT32)fsnode;  
  
    return NU_SUCCESS;
}

/************************************************************************
* FUNCTION
*
*       wr_safe_fsnode_to_string
*
* DESCRIPTION
*
*       Converts a fsnode to a string representation.
*
* INPUTS
*
*       dh                              Disk handle.
*       *fsnode                         FS node to deallocate.
*       *string                         String representations of the 
*                                        FS node.
*
* OUTPUTS
*
*       NU_SUCCESS                      FS node was converted to a string
*                                        representation.
*                                           
*************************************************************************/
STATUS wr_safe_fsnode_to_string(UINT16 dh, VOID *fsnode, CHAR *string)
{   
    INT size = 0;
    CHAR *pP;

    /* Resolve unused parameters. */
    Safe_Unused_Param = dh; 
    
    pP = (CHAR *)fsnode;
    while((CHAR)*pP++)
        size++;

    string[0] = FS_SEPARATORCHAR;
    NUF_Copybuff(&string[1], fsnode, size);

    /* If size is greater than zero then we 
      aren't dealing with root directory, so add another slash.*/
    if(size > 0)
    {
        string[size+1] = FS_SEPARATORCHAR;    
        string[size+2] = '\0';
    }
    else
    {
        string[size+1] = '\0';
    }
  
    return NU_SUCCESS;
}

/************************************************************************
* FUNCTION
*
*       wr_safe_release
*
* DESCRIPTION
*
*       File system operation wrapper for non-supported operation.
*
* INPUTS
*       
*       dh                                  Disk handle.
*       
* OUTPUTS
*
*       None.                          
*
*************************************************************************/
VOID   wr_safe_release (UINT16 dh)
{
    /* Resolve unused parameters. */
    Safe_Unused_Param = dh; 
    return;
}

/************************************************************************
* FUNCTION
*
*       wr_safe_reclaim
*
* DESCRIPTION
*
*       File system operation wrapper for non-supported operation.
*
* INPUTS
*       
*       dh                                  Disk handle.
*       
* OUTPUTS
*
*       None.                          
*       
*************************************************************************/
VOID   wr_safe_reclaim (UINT16 dh)
{
    /* Resolve unused parameters. */
    Safe_Unused_Param = dh; 
    return;
}

/************************************************************************
* FUNCTION
*
*       WR_Safe_Init_FS
*
* DESCRIPTION
*
*       Performs registration of the Safe file system. This routine is
*        called from the initialization of the application.
*
* INPUTS
*
*       *fsname                             Name of file system
*
* OUTPUTS
*
*       NU_SUCCESS                          Disk was initialized
*                                            successfully.
*       Error code                          If service is erroneous. 
*                                  
*************************************************************************/
STATUS WR_Safe_Init_FS(char *fsname)
{
    STATUS  sts;
    FS_S    fs;
    
    /* Make a file system table entry */
    fs.fs_format         = wr_safe_format;
    fs.fs_get_format_info = wr_safe_get_format_info;
    fs.fs_init           = wr_safe_init;
    fs.fs_uninit         = wr_safe_uninit;    
    fs.fs_mount          = wr_safe_mount;
    fs.fs_unmount        = wr_safe_unmount;
    fs.fs_disk_abort     = wr_safe_disk_abort;

    fs.fs_open           = wr_safe_open;
    fs.fs_read           = wr_safe_read;
    fs.fs_close          = wr_safe_close;
    fs.fs_seek           = wr_safe_seek;
    fs.fs_write          = wr_safe_write;
    fs.fs_flush          = wr_safe_flush;

    fs.fs_set_attr       = wr_safe_set_attr;
    fs.fs_get_attr       = wr_safe_get_attr;
    fs.fs_truncate       = wr_safe_truncate;
    fs.fs_delete         = wr_safe_delete;
    fs.fs_rename         = wr_safe_rename;

    fs.fs_get_first      = wr_safe_get_first;
    fs.fs_get_next       = wr_safe_get_next;
    fs.fs_done           = wr_safe_done;

    fs.fs_mkdir          = wr_safe_mkdir;
    fs.fs_rmdir          = wr_safe_rmdir;

    fs.fs_fsnode_to_string   = wr_safe_fsnode_to_string;
    fs.fs_vnode_allocate     = wr_safe_vnode_allocate;
    fs.fs_vnode_deallocate   = wr_safe_vnode_deallocate;

    fs.fs_release            = NU_NULL;
    fs.fs_reclaim            = NU_NULL;
    fs.fs_freespace          = wr_safe_freespace;

    fs.fs_utime              = wr_safe_utime;

    /* Register the file system */
    sts = NU_Register_File_System(fsname, &fs);
    
    return(sts);

}
#else
STATUS WR_Safe_Init_FS(char *fsname)
{
    return NUF_INTERNAL;
}
#endif


