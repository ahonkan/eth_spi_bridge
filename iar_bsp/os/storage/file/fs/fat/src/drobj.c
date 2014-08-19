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
*       drobj.c
*
* COMPONENT
*
*       FAT
*
* DESCRIPTION
*
*       Directory object manipulation routines.
*
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       pc_fndnode                          Find a file or directory on
*                                            disk and return a DROBJ.
*       pc_get_inode                        Find a filename within a
*                                            subdirectory.
*       pc_next_inode                       Find a next file or
*                                            directory on disk and
*                                            return a DROBJ.
*       chk_sum                             Calculating short file name
*                                            check byte.
*       pc_cre_longname                     Create long-filename ascii
*                                            strings.
*       pc_cre_shortname                    Create short-filename ascii
*                                            strings.
*       lnam_clean                          Clear long filename
*                                            information structure.
*       pc_findin                           Find a filename in the same
*                                            directory as argument.
*       pc_get_mom                          Find the parent inode of a
*                                            subdirectory.
*       pc_mkchild                          Allocate a DROBJ and fill
*                                            it based on parent object.
*       pc_mknode                           Create an empty subdirectory
*                                            or file.
*       pc_insert_inode                     Called only by pc_mknode.
*       pc_del_lname_block                  Delete a long filename
*                                            entry.
*       pc_renameinode                      Rename an inode.
*       pc_rmnode                           Delete an inode
*                                            unconditionally.
*       pc_update_inode                     Flush an inode to disk.
*       pc_get_root                         Create the special ROOT
*                                            object for a drive.
*       pc_firstblock                       Return the absolute block
*                                            number of a directory.
*       pc_next_block                       Calculate the next block
*                                            owned by an object.
*       pc_l_next_block                     Calculate the next block in
*                                            a chain.
*       pc_marki                            Set dr:sec:index, and stitches
*                                            FINODE into the inode list.
*       pc_scani                            Search for an inode in the
*                                            internal inode list.
*       pc_allocobj                         Allocates and zeros the
*                                            space needed to store a
*                                            DROBJ structure.
*       pc_alloci                           Allocates and zeros a
*                                            FINODE structure.
*       pc_free_all_i                       Release all inode buffers.
*       pc_freei                            Release FINODE structure.
*       pc_freeobj                          Return a drobj structure to
*                                            the heap.
*       pc_dos2inode                        Take the data from pbuff
*                                            which is a raw disk
*                                            directory entry.
*       pc_ino2dos                          Take in memory native format
*                                            inode information.
*       pc_init_inode                       Take an uninitialized inode.
*       pc_isadir                           Check the root or
*                                            subdirectory.
*       pc_isroot                           Check the root directory.
*
*************************************************************************/

#include        "storage/fat_defs.h"
extern STATUS  ascii_to_cp_format(UINT8 *cp_format,UINT8 *ascii_cp);
extern STATUS  ascii_cp_format_to_ascii(UINT8 *ascii_cp_format);

extern FINODE       *inoroot;               /* Beginning of inode pool.  */

/* Internal function prototypes. */
static UINT16 pc_sfn_chksum (UINT8 *buf_ptr, UINT32 length);
/************************************************************************
* FUNCTION
*
*       pc_fndnode
*
* DESCRIPTION
*
*       Take a full path name and traverse the path until we get to the
*       file or subdir at the end of the path specifier. When found
*       allocate and initialize (OPEN) a DROBJ.
*
*
* INPUTS
*
*       dh                                  Disk handle
*       **pobj                              Drive object structure
*       path                                Path name
*
* OUTPUTS
*
*       NU_SUCCESS                          File was found.
*       NUF_NOT_OPENED                      Drive not opened.
*       NUF_LONGPATH                        Path or directory name too
*                                            long.
*       NUF_NOFILE                          The specified file not
*                                            found.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS pc_fndnode(UINT16 dh, DROBJ **pobj, UINT8 *path)
{
STATUS      ret_stat;
DROBJ       *pmom;
DROBJ       *pchild;
DDRIVE      *pdrive;
UINT8       *childpath;
FAT_CB      *fs_cb;
CHAR        *str_ptr;
CHAR        *path_ptr;

    /* Use the control block to get the drive structure. */
    ret_stat = fsdh_get_fs_specific(dh, (VOID**)&fs_cb);

    if (ret_stat == NU_SUCCESS)
        pdrive = fs_cb->ddrive;
    else
        pdrive = NU_NULL;

    if (!pdrive)
        return(NUF_NOT_OPENED);

    /* Get the top of the current path */
    if (path)
    {
        if ( *path == BACKSLASH )
        {
            /* Get root directory. */
            *pobj = pc_get_root(pdrive);
            if (!*pobj)
                return(NUF_NO_DROBJ);

            /* Increment path. */
            NUF_NEXT_CHAR(path);
        }
        else
        {
            /* Get current directory. */
            *pobj = pc_get_cwd(pdrive);
            if (!*pobj)
                return(NUF_NO_DROBJ);
        }
    }
    else
    {
        /* Get current directory. */
        *pobj = pc_get_cwd(pdrive);
        if (!*pobj)
            return(NUF_NO_DROBJ);
        return(NU_SUCCESS);
    }

    ret_stat = NU_SUCCESS;

    /* Search through the path until exhausted */
    while (*path)
    {
        /* Move to the next path. */
        childpath = pc_nibbleparse(path);
        if (!childpath)
        {
            break;
        }
        ret_stat = NUF_GET_CHAR(&str_ptr,(CHAR*)path,1);
        if(ret_stat != NU_SUCCESS)
        {
            /* path isn't encoded correctly. */
            break;
        }

        ret_stat = NUF_GET_CHAR(&path_ptr,(CHAR*)path,2);
        if(ret_stat != NU_SUCCESS)
        {
            /* path isn't encoded correctly. */
            break;
        }

        /* is dot OR is root and dot dot */
        if (( (*path == '.') && ((*str_ptr == BACKSLASH) || (*str_ptr == '\0')) ) ||
            (((*pobj)->isroot == NU_TRUE) &&
            (*path == '.') && (*str_ptr == '.') &&
            ((*path_ptr == BACKSLASH) || (*path_ptr == '\0')) ))
            ;
        else
        {
            /* Lock the finode. */
            PC_INODE_ENTER((*pobj)->finode, NO)

            /* Find Filename in pobj and initialize pchild with result. */
            pchild = NU_NULL;
            ret_stat = pc_get_inode(&pchild, (*pobj), path);
            if (ret_stat != NU_SUCCESS)
            {
                /* Release exclusive use of finode. */
                PC_INODE_EXIT((*pobj)->finode)
                pc_freeobj(*pobj);
                /* clear pointer so it won't be deallocated in NU_Done() */
                *pobj = NU_NULL;
                break;
            }

            /* We found it. We have one special case. if "..", we need
            to shift up a level so we are not the child of mom
            but of grand mom. */
            ret_stat = NUF_GET_CHAR(&str_ptr,(CHAR*)path,1);
            if(ret_stat != NU_SUCCESS)
            {
                /* path isn't encoded correctly. */
                break;
            }

            ret_stat = NUF_GET_CHAR(&path_ptr,(CHAR*)path,2);
            if(ret_stat != NU_SUCCESS)
            {
                /* path isn't encoded correctly. */
                break;
            }

            if ((*path == '.') && (*str_ptr == '.') &&
                ((*path_ptr== BACKSLASH) || (*path_ptr == '\0')) )
            {
                /* Find pobj's parent. By looking back from ".." */
                ret_stat = pc_get_mom(&pmom, pchild);
                /* Release exclusive use of finode. */
                PC_INODE_EXIT((*pobj)->finode)
                /* We're done with pobj for now */
                pc_freeobj(*pobj);

                if (ret_stat != NU_SUCCESS)
                {
                    /* We're done with pchild for now. */
                    pc_freeobj(pchild);
                    break;
                }
                else
                {
                    /* We found the parent now free the child */
                    *pobj = pmom;
                    pc_freeobj(pchild);
                }
            }
            else
            {
                /* Release exclusive use of finode. */
                PC_INODE_EXIT((*pobj)->finode)
                /* We're done with pobj for now */
                pc_freeobj(*pobj);
                /* Make sure pobj points at the next inode */
                *pobj = pchild;
            }
        }
        /* Move to the next path. */
        path = childpath;
    }
    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       pc_get_inode
*
* DESCRIPTION
*
*       Search the directory for the pattern or name in filename.
*       If pobj is NULL start the search at the top of pmom (getfirst)
*       and allocate pobj before returning it.
*       Otherwise start the search at pobj .
*
*
* INPUTS
*
*       **pobj                              Output search drive object
*       *pmom                               Search the drive object
*       *filename                           Search file name
*
* OUTPUTS
*
*       NU_SUCCESS                          Search successful.
*       NUF_LONGPATH                        Path or directory name too
*                                            long.
*       NUF_NOFILE                          The specified file not
*                                            found.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS pc_get_inode(DROBJ **pobj, DROBJ *pmom, UINT8 *filename)
{
INT         starting = NO;
INT         len;
STATUS      ret_stat = NU_SUCCESS;
UINT8       *curr_char;

    /* measure long file name length */
    if (filename != NU_NULL)
    {
        for (len = 0; ret_stat == NU_SUCCESS; len++)
        {
            ret_stat = NUF_GET_CHAR((CHAR**)&curr_char,(CHAR *)filename,len);

            /* Don't include null terminating character in our count. */
            if((ret_stat != NU_SUCCESS) || (*((UINT8 *)curr_char) == 0))
            {
                /* Reset status to success because we are done counting. */
                ret_stat = NU_SUCCESS;
                break;
            }
        }

    }
    else
        len = 0;

    /* Check file name length. */
    if ((len + pmom->finode->abs_length) > EMAXPATH)
    {
        pc_report_error(PCERR_PATHL);
        ret_stat = NUF_LONGPATH;
    }

    /* Handle case of ".." in root */
    if ((pmom->isroot == NU_TRUE) && (filename))
    {
        if(NUF_Strncmp((CHAR*)filename, "..\\", 3) == NU_SUCCESS)
        {
            filename += 3;
        }
    }

    /* Create the child if just starting */
    if (!*pobj && (ret_stat == NU_SUCCESS) )
    {
        starting = YES;
        /* Allocate DROBJ. */
        *pobj = pc_mkchild(pmom);
        if (!*pobj)
            ret_stat = NUF_NO_DROBJ;
    }
    /* If doing a gnext don't get stuck in and endless loop */
    else if (ret_stat == NU_SUCCESS)
    {
        /* Increment entry index. */
        if ( ++((*pobj)->blkinfo.my_index) >= INOPBLOCK )
        {
            /* Move to the next block directory entry. */
            ret_stat = pc_next_block(*pobj);
            if (ret_stat != NU_SUCCESS)
            {
                if (ret_stat == NUF_NOSPC)
                    ret_stat = NUF_NOFILE;
            }
            else
                (*pobj)->blkinfo.my_index = 0;
        }
        if (ret_stat == NU_SUCCESS)
            (*pobj)->finode->abs_length = pmom->finode->abs_length;
    }

    if (ret_stat == NU_SUCCESS)
    {
        /* Find the filename */
        ret_stat = pc_findin(*pobj, filename);

        if (ret_stat != NU_SUCCESS)
        {
            /* Mark the fname[0] == "\0" entry blocks and index */
            if (ret_stat == NUF_NOFILE)
            {
                pmom->blkinfo.end_block = (*pobj)->blkinfo.my_block;
                pmom->blkinfo.end_index = (*pobj)->blkinfo.my_index;
            }
            if (starting)
            {
                /* We're done with pobj for now. */
                pc_freeobj(*pobj);
                *pobj = NU_NULL;
            }
        }
    }
    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       pc_next_inode
*
* DESCRIPTION
*
*       Next search the directory for the pattern or name in filename.
*
*
* INPUTS
*
*       pobj                                Pobj must not be NULL.
*       *pmom                               Search the drive object
*       *filename                           Search file name
*       attrib                              File attributes
*
* OUTPUTS
*
*       NU_SUCCESS                          Search successful.
*       NUF_ACCES                           Attempt to open a read only
*                                            file or a special.
*       NUF_NOFILE                          The specified file not
*                                            found.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS pc_next_inode(DROBJ *pobj, DROBJ *pmom, UINT8 *filename, INT attrib)
{
STATUS      ret_stat;
STATUS      ret_stat_w;


    /* Defaults. */
    ret_stat = NUF_NOFILE;

    while (pobj)
    {
        /* Now find the next file */
        ret_stat_w = pc_get_inode(&pobj, pmom, (UINT8 *)filename);
        if (ret_stat_w != NU_SUCCESS)
        {
            if (ret_stat != NUF_ACCES)
                ret_stat = ret_stat_w;
            break;
        }

        /* Check file attributes. */
        if (pobj->finode->fattribute & attrib)
        {
            /* Special entry? */
            if (! (pc_isdot(pobj->finode->fname, pobj->finode->fext)) &&
                ! (pc_isdotdot(pobj->finode->fname, pobj->finode->fext)) )
            {
                ret_stat = NUF_ACCES;
            }
            if (pobj->linfo.lnament)
            {
                /* We need to clean long filename information */
                lnam_clean(&pobj->linfo, pobj->pblkbuff);
                pc_free_buf(pobj->pblkbuff, NO);
            }
        }
        else
        {
            ret_stat = NU_SUCCESS;
            break;
        }
    }

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       chk_sum
*
* DESCRIPTION
*
*       Calculating Short Filename Check byte
*
*
* INPUTS
*
*       *sname                              Short Filename String
*
* OUTPUTS
*
*       Check byte
*
*************************************************************************/
UINT8 chk_sum(UINT8 *sname)
{
INT         i;
UINT8       aa,bb,cc,dd;


    dd = 0;
    for (i = 0; i < 10; i++)
    {
        aa = sname[i] + dd;
        bb = (UINT8) (aa >> 1);
        cc = bb + 0x80;
        if (aa % 2)
        {
            aa = cc;
        }
        else
        {
            aa = bb;
        }
        dd = aa;
    }

    return(dd + sname[i]);
}


/************************************************************************
* FUNCTION
*
*       pc_cre_longname
*
* DESCRIPTION
*
*       Create long filename ascii strings from directory entry.
*
*
* INPUTS
*
*       *filename                           Pointer to filename to write
*                                            filename.
*       *info                               Long filename information.
*
* OUTPUTS
*
*       Always YES                          Success complete.
*
*************************************************************************/
INT pc_cre_longname(UINT8 *filename, LNAMINFO *linfo)
{
INT         i, ci;
INT         my_index;
BLKBUFF     *bbuf;
LNAMENT     *ent;
INT16       fileend_flag;


    if (linfo->lnamblk3)
    {
        /* Top entry is in rbuf. */
        my_index = linfo->lnament -
        (INOPBLOCK - linfo->lnam_index) - 1 - INOPBLOCK;
        bbuf = linfo->lnamblk3;
    }
    else if (linfo->lnamblk2)
    {
        /* Top entry is in rbuf. */
        my_index = linfo->lnament - (INOPBLOCK - linfo->lnam_index) - 1;
        bbuf =  linfo->lnamblk2;
    }
    else
    {
        /* Top entry is in buf. */
        my_index = linfo->lnam_index + linfo->lnament - 1;
        bbuf = linfo->lnamblk1;
    }

    fileend_flag = 0;

    for (i = 0; i < linfo->lnament; )
    {
        /* Long filename start block. */
        ent = (LNAMENT *)bbuf->data;

        /* Long filename start entry. */
        ent +=  my_index;

        for (; (my_index >= 0) && (i < linfo->lnament); my_index--, i++)
        {
            for (ci = 0; ci < 10; ci += 2)
            {
                if (!fileend_flag)
                {
                    /* Encode Unicode into UTF-8. */
                    unicode_to_utf8(filename,&ent->str1[ci]);
                    if (!*filename)
                        fileend_flag = 1;
                    else
                        NUF_NEXT_CHAR(filename);
                }
            }
            for (ci = 0; ci < 12; ci += 2)
            {
                if (!fileend_flag)
                {
                    /* Encode Unicode into UTF-8. */
                    unicode_to_utf8(filename,&ent->str2[ci]);
                    if (!*filename)
                        fileend_flag = 1;
                    else
                        NUF_NEXT_CHAR(filename);
                }
            }
            for (ci = 0; ci < 4; ci += 2)
            {
                if (!fileend_flag)
                {
                    /* Encode Unicode into UTF-8. */
                    unicode_to_utf8(filename,&ent->str3[ci]);
                    if (!*filename)
                        fileend_flag = 1;
                    else
                        NUF_NEXT_CHAR(filename);
                }
            }
            *filename = '\0';
            ent--;
        }

        /* End directory entry index on the sector. */
        my_index = INOPBLOCK - 1;

        /* Long file name block. */
        if (bbuf == linfo->lnamblk3)
            bbuf = linfo->lnamblk2;
        else if (bbuf == linfo->lnamblk2)
            bbuf = linfo->lnamblk1;
    }

    return(YES);
}


/************************************************************************
* FUNCTION
*
*       pc_cre_shortname
*
* DESCRIPTION
*
*       Create short filename ascii strings from directory entry.
*
*
* INPUTS
*
*       *filename                           Pointer to filename to write
*                                            filename.
*       *fname                              Pointer to filename.
*       *fext                               Pointer to file extension.
*
* OUTPUTS
*
*       Short filename length
*
*************************************************************************/
INT pc_cre_shortname(UINT8 *filename, UINT8 *fname, UINT8 *fext)
{
INT16       i;
UINT8       *top = filename;
INT         len;

    i = 0;
    /* Setup the filename. */
    while(*fname)
    {
        if (*fname == ' ')
            break;
        else
        {
            i += NUF_GET_CP_LEN((CHAR*)fname);
            NUF_COPYBUFF(filename,fname,1);
            NUF_NEXT_CHAR(filename);
            NUF_NEXT_CHAR(fname);
        }
        if (i == 8)
            break;
    }

    /* save filename length */
    len = i;

    /* Setup the file extension. */
    i = 0;
    if ( (fext) && (*fext!= ' ') )
    {
        /* Extension mark. */
        *filename++ = '.';
        while (*fext)
        {
            if (*fext == ' ')
                break;
            else
            {
                /* Copy file extension. */
                *filename++ = *fext++;

                i++;
            }
            if (i == 3)
                break;
        }
    }

    len += i;

    /* Get rid of trailing '.' s */
    if ( (i == 0) && (*(filename-1) == '.') && (*top!= '.') )
    {
        filename--;
    }

    *filename = '\0';


    return(len);
}


/************************************************************************
* FUNCTION
*
*       lnam_clean
*
* DESCRIPTION
*
*       Clear long filename information structure. This function does
*       not free the block that contains the short filename entry of this
*       long filename.
*
*
* INPUTS
*
*       *linfo                              Clear long filename
*                                            information
*       *rbuf                               Clear long filename block
*                                            buffer
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID lnam_clean(LNAMINFO *linfo, BLKBUFF *rbuf)
{

    /* Previous block. */
    if (linfo->lnamblk1 != rbuf)
    {
        /* Free long filename entry block. */
        pc_free_buf(linfo->lnamblk1, NO);
    }
    linfo->lnamblk1 = (BLKBUFF *)0;

    /* Previous block. */
    if ( (linfo->lnamblk2 ) && (linfo->lnamblk2 != rbuf) )/* Previous block */
    {
        /* Free long filename entry block. */
        pc_free_buf(linfo->lnamblk2, NO);
    }
    linfo->lnamblk2 = (BLKBUFF *)0;

    /* Clear long filename start block */
    linfo->lnament = 0;

}

/************************************************************************
* FUNCTION
*
*       pc_findin
*
* DESCRIPTION
*
*       Find a filename in the same directory as the argument.
*
*
* INPUTS
*
*       *pobj                               Drive object structure
*       *filename                           Search file name
*
* OUTPUTS
*
*       NU_SUCCESS                          Found the file.
*       NUF_NOFILE                          The specified file not
*                                            found.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS pc_findin(DROBJ *pobj, UINT8 *filename)
{
STATUS      ret_stat;
BLKBUFF     *rbuf;
DIRBLK      *pd;
DOSINODE    *pi;
FINODE      *pfi;
INT         found_flag = 0;
LNAMINFO    *linfo;
UINT8       ckval;
UINT8       namebuf[MAX_LFN+1];
UINT16      lent_cou;
INT         len = 0;
INT         short_name;
CHAR        *str_ptr = NU_NULL;
MTE_S       *mte;
INT         name_len;
UINT8       *nb_ptr;
#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1 && ENABLE_W_JAPANESE_SHIFT_JIS == NU_TRUE)
UINT8        cp932_special_char[2];
#endif
UINT8        cp_format[2];

    /* For convenience, we want to get at block info here */
    pd = &pobj->blkinfo;

    /* Move long filename information pointer into internal pointer.  */
    linfo = &pobj->linfo;

    /* Clear the number of long filename entry */
    linfo->lnament = 0;

    /* Clear the sequence number of long file name entry. */
    lent_cou = 0;

    /* Initialize short file name flag. */
    short_name = 0;

    /* Read the directory entry data. */
    ret_stat = pc_read_blk(&rbuf, pobj->pdrive, pobj->blkinfo.my_block);

    /* Move BLKBUFF pointer to DROBJ. */
    pobj->pblkbuff = rbuf;

    while (ret_stat == NU_SUCCESS)
    {
        /* Move directory entry pointer into internal pointer */
        pi = (DOSINODE *) &rbuf->data[0];

        /* Look at the current inode */
        pi += pd->my_index;

        /* And look for a match */
        while ( pd->my_index < INOPBLOCK )
        {
            /* End of dir if name is 0 */
            if (!pi->fname[0])
            {
                /* Is there long filename information in buffer */
                if (linfo->lnament)
                {
                    /* Clean long filename information */
                    lnam_clean(linfo, rbuf);
                }
                /* Free work block */
                pc_free_buf(rbuf, NO);

                return(NUF_NOFILE);
            }

            /* This entry is long filename entry */
            if ( *((UINT8 *)pi + 0x0b) == 0x0f )
            {
                /* Initialize the short file name flag. */
                short_name = 0;

                /* It is not the first entry of the long filename */
                if (linfo->lnament)
                {
                    /* Long filename entry numbers are between 1 and 14h */
                    if ( (*((UINT8 *)pi) == 0x00) || (*((UINT8 *)pi) > 0x14) )
                    {
#ifdef DEBUG2
                        if (*((UINT8 *)pi) != PCDELETE)
                            DEBUG_PRINT("pc_findin  long filename error \r\n");
#endif
                        /* Clean long filename information */
                        lnam_clean(linfo, rbuf);
                    }
                    /* Checking the check value of short filename */
                    else if ( linfo->lnamchk != *((UINT8 *)pi + 0x0d) )
                    {
#ifdef DEBUG2
                        DEBUG_PRINT("pc_findin  short filename check value error \r\n");
#endif

                        /* Clean long filename information */
                        lnam_clean(linfo, rbuf);
                    }
                    /* Checking the long filename entry number */
                    else if ( (lent_cou ) != *((UINT8 *)pi) )
                    {
#ifdef DEBUG2
                        DEBUG_PRINT("pc_findin  long filename entry number error \r\n");
#endif

                        /* Clean long filename information */
                        lnam_clean(linfo, rbuf);
                    }
                    else
                    {
                        /* Increment the number of long filename entry */
                        linfo->lnament++;

                        /* Decrement the sequence number of long filename entry */
                        lent_cou--;

                        /* Save BLKBUFF pointer into long filename info structure */
                        /* BLKBUFF pointer 2 is not used yet */
                        if (linfo->lnamblk2 == 0)
                        {
                            /* Read block is changed */
                            if (linfo->lnamblk1 != rbuf)
                            {
                                /* Save new BLKBUFF pointer into long filename info structure */
                                linfo->lnamblk2 = rbuf;
                            }
                        }
                        /* BLKBUFF pointer 2 is used and BLKBUFF pointer 3 is not used yet */
                        else if (linfo->lnamblk3 == 0)
                        {
                            /* Read block is changed */
                            if (linfo->lnamblk2 != rbuf)
                            {
                                /* Save new BLKBUFF pointer into long filename info structure */
                                linfo->lnamblk3 = rbuf;
                            }
                        }
                    }
                }
                else /* Long filename first entry */
                {
                    /* Long filename first entry must be added 0x40 */
                    if ( ((*((UINT8 *)pi) & 0xF0) != 0x40) &&
                       ((*((UINT8 *)pi) & 0xF0) != 0x50) )
                    {
#ifdef DEBUG2
                        if (*((UINT8 *)pi) != PCDELETE)
                            DEBUG_PRINT("pc_findin  long filename error \r\n");
#endif
                    }
                    else
                    {
                        /* Set long filename start block */
                        linfo->lnamblk1 = rbuf;
                        linfo->lnamblk2 = NU_NULL;
                        linfo->lnamblk3 = NU_NULL;

                        /* Set long filename start index */
                        linfo->lnam_index = (INT16)pd->my_index;

                        /* Set check value of short filename */
                        linfo->lnamchk = *((UINT8 *)pi + 0x0d);

                        /* Increment long filename entry */
                        linfo->lnament++;

                        /* Save long filename entry number */
                        lent_cou = *((UINT8 *)pi) - 0x40;

                        /* Next entry number */
                        lent_cou--;
                    }
                }
            }
            /* Short filename entry of the long filename, */
            else if (linfo->lnament)
            {
                if (*((UINT8 *)pi) == PCDELETE)
                {
#ifdef DEBUG2
                    DEBUG_PRINT("pc_findin short filename of long filename deleted \r\n");
#endif

                    /* Clean long filename information */
                    lnam_clean(linfo, rbuf);
                }
                else
                {
                    /* Calculate check value of short filename */
                    ckval = chk_sum((UINT8 *)pi);

                    /* Checking the check value of short filename */
                    if (linfo->lnamchk != ckval)
                    {
#ifdef DEBUG2
                        DEBUG_PRINT("pc_findin  short filename check value error %s %d\r\n",
                        __FILE__, __LINE__);
#endif

                        /* Clean long filename information */
                        lnam_clean(linfo, rbuf);
                    }
                    /* The sequence number of long filename entry must be 0 */
                    else if (lent_cou)
                    {
#ifdef DEBUG2
                        DEBUG_PRINT("pc_findin  long filename entry number error %s %d\r\n",
                        __FILE__, __LINE__);
#endif

                        /* Clean long filename information */
                        lnam_clean(linfo, rbuf);
                    }
                    else
                    {
                        /* Create long filename string from directory entry */
                        pc_cre_longname(namebuf, linfo);

                        /* Compare long filename */
                        if (YES == pc_patcmp(namebuf, filename ))
                        {
                            for (len = 0; (NUF_GET_CHAR(&str_ptr,(CHAR*)namebuf,len) == NU_SUCCESS)
                                    && *str_ptr != '\0'; len++){;}

                                /* Set the file found flag */
                                found_flag = 1;
                        }
                    }
                }
                /* Set the short filename flag. */
                short_name = 1;
            }

            /* Short filename entry */
            if ( (!linfo->lnament) || (short_name) )
            {
                /* The file is not deleted */
                if (*((UINT8 *)pi) != PCDELETE)
                {
                    if (!found_flag)
                    {
                        /* Create short filename string(UTF8) from directory entry(codepage). */
                        mte = fsl_mte_from_dh(pobj->pdrive->dh);
                        nb_ptr = &namebuf[0];
                        if((pi->fname[0] != '.') && (pi->fname[1] != '.'))
                        {
                            len = 0;
#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1 && ENABLE_W_JAPANESE_SHIFT_JIS == NU_TRUE)
                            /* Check for special case in CP932 if leading char 
                            begins with 0xE5, then it was written to disk as 0x05. */
                            if(pi->fname[0] == 0x05)
                            {
                                cp932_special_char[0] = 0xe5;
                                cp932_special_char[1] = pi->fname[1];
                                mte->mte_cp->cp_op.cp_to_utf8(nb_ptr,&cp932_special_char[0]);
                                len += NUF_GET_CP_LEN((CHAR*)nb_ptr);
                                NUF_NEXT_CHAR(nb_ptr);

                            }
#endif
                            for(; ((len < 8) && (pi->fname[len] != ' ')); )
                            {
                                ret_stat = ascii_to_cp_format(&cp_format[0],&pi->fname[len]);
                                /* If pi->fname[len] was an ASCII character convert it to 
                                   codepage format. */
                                if(ret_stat == NU_SUCCESS)
                                {
                                    mte->mte_cp->cp_op.cp_to_utf8(nb_ptr,&cp_format[0]);
                                }
                                else
                                {
                                    mte->mte_cp->cp_op.cp_to_utf8(nb_ptr,&pi->fname[len]);
                                }
                                len += NUF_GET_CP_LEN((CHAR*)nb_ptr);
                                NUF_NEXT_CHAR(nb_ptr);
                            }
                        }
                        else
                        {
                            /* Account for dot and dot dot. */
                            /* Just dot */
                            if(pi->fname[1] != '.')
                            {
                                *nb_ptr++ = '.';
                                len = 1;
                            }
                            else
                            {
                                *nb_ptr++ = '.';
                                *nb_ptr++ = '.';
                                len = 2;
                            }

                        }
                        if ( (pi->fext[0]) && (pi->fext[0] != ' ') )
                        {
                            *nb_ptr = '.';
                            NUF_NEXT_CHAR(nb_ptr);
                            name_len = len;
                            for(len = 0; ((len < 3) && (pi->fext[len] != ' '));)
                            {
                                ret_stat = ascii_to_cp_format(&cp_format[0],&pi->fext[len]);
                                /* If pi->fext[len] was an ASCII character convert it to 
                                codepage format. */
                                if(ret_stat == NU_SUCCESS)
                                {
                                    mte->mte_cp->cp_op.cp_to_utf8(nb_ptr,&cp_format[0]);
                                }
                                else
                                {
                                    mte->mte_cp->cp_op.cp_to_utf8(nb_ptr,&pi->fext[len]);
                                }
                                len += NUF_GET_CP_LEN((CHAR*)nb_ptr);
                                NUF_NEXT_CHAR(nb_ptr);

                            }
                            len += name_len;
                        }

                        *nb_ptr = '\0';
                        if (len)
                        {
                            /* Compare the filename */
                            if (YES == pc_patcmp(namebuf, filename ))
                            {
                                /* Set the file found flag */
                                found_flag = 1;
                            }
                            /* Long filename? */
                            else if (linfo->lnament)
                            {
                                /* Clean long filename information */
                                lnam_clean(linfo, rbuf);
                            }
                        }
                    }
                }
            }
            /* The file is found */
            if (found_flag)
            {
#ifdef DEBUG1
                DEBUG_PRINT("pc_findin  file found my_block=%d  my_index=%d my_first=%d\r\n",
                    pd->my_block, pd->my_index, pd->my_frstblock );
#endif
                /* We found it */
                /* See if it already exists in the inode list.
                If so.. we use the copy from the inode list */
                pfi = pc_scani(pobj->pdrive, rbuf->blockno, pd->my_index);

                if (pfi)
                {
                    /* Free the inode. */
                    pc_freei(pobj->finode);
                    /* Since we changed the list go back to the top. */
                    pobj->finode = pfi;
                }
                else
                {
                    /* No inode in the inode list. Copy the data over
                    and mark where it came from */
                    pfi = pc_alloci();
                    if (pfi)
                    {
                        /* Assign a disk handle to the inode */
                        pfi->lock_object.dh = pobj->pdrive->dh;

                        /* Calculate Absolute path length */
                        pfi->abs_length = (UINT16) (pobj->finode->abs_length + len);

                        /* Release the current inode. */
                        pc_freei(pobj->finode);
                        /* Since we changed the list go back to the top. */
                        pobj->finode = pfi;

                        /* Convert a dos inode to in mem form. */
                        pc_dos2inode(pobj->finode, pi,pobj->pdrive->dh);

                        /* Mark the inode in the inode buffer. */
                        pc_marki(pobj->finode, pobj->pdrive, pd->my_block,
                            pd->my_index);
                    }
                    else
                    {
                        /* Is there long filename information in buffer */
                        if (linfo->lnament)
                        {
                            /* Clean long filename information */
                            lnam_clean(linfo, rbuf);
                        }
                        /* Error. Free current buffer. */
                        pc_free_buf(rbuf, NO);

                        return(NUF_NO_FINODE);
                    }
                }
                /* This is not a long filename */
                if (!linfo->lnament)
                {
                    /* Free current buffer. */
                    pc_free_buf(rbuf, NO);
                }

                return(NU_SUCCESS);
            }                   /* if (found_flag) */

            pd->my_index++;
            pi++;
        }
        /* Is there long filename information in buffer */
        if (!linfo->lnament)
        {
            /* Free current buffer. */
            pc_free_buf(rbuf, NO);
        }

        /* Update the objects block pointer */
        ret_stat = pc_next_block(pobj);
        if (ret_stat != NU_SUCCESS)
        {
            if (ret_stat == NUF_NOSPC)
                ret_stat = NUF_NOFILE;
            break;
        }

        pd->my_index = 0;

        /* Read the next block directory data. */
        ret_stat = pc_read_blk(&rbuf, pobj->pdrive,  pobj->blkinfo.my_block);
        pobj->pblkbuff = rbuf;

    }

    /* Is there long filename information in buffer
    Note: Short filename buffer is already free */
    if (linfo->lnament)
    {
        /* Clean long filename information */
        lnam_clean(linfo, rbuf);
    }

    /* Always error return */
    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       pc_get_mom
*
* DESCRIPTION
*
*       Given a DROBJ initialized with the contents of a subdirectory's
*       ".." entry, initialize a DROBJ which is the parent of the
*       current directory.
*
*
* INPUTS
*
*       **pmom                              Output parent drive object
*       **pdotdot                           ".." entry drive object
*
* OUTPUTS
*
*       NU_SUCCESS                          If service is successful.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS pc_get_mom(DROBJ **pmom, DROBJ *pdotdot)
{
STATUS      ret_stat = NU_SUCCESS;
DDRIVE      *pdrive = pdotdot->pdrive;
UINT32      sectorno;
BLKBUFF     *rbuf;
DIRBLK      *pd;
DOSINODE    *pi;
FINODE      *pfi;


    /* We have to be a subdir */
    if (!pc_isadir(pdotdot))
        ret_stat = NUF_INTERNAL;

    /* If ..->cluster is zero then parent is root */
    if (!pdotdot->finode->fcluster && (ret_stat == NU_SUCCESS) )
    {
        /* Get root directory. */
        *pmom = pc_get_root(pdrive);
        if (!*pmom)
            ret_stat = NUF_NO_DROBJ;
    }
    /* Otherwise : cluster points to the beginning of our parent.
    We also need the position of our parent in it's parent   */
    else if (ret_stat == NU_SUCCESS)
    {
        *pmom = pc_allocobj();
        if (!*pmom)
            ret_stat = NUF_NO_DROBJ;
        else
        {
            (*pmom)->pdrive = pdrive;
            /* Setup the lock to allow yielding */
            (*pmom)->finode->lock_object.dh = pdrive->dh;

            /* Find .. in our parent's directory */
            sectorno = pc_cl2sector(pdrive, (UINT32)pdotdot->finode->fcluster);
            /* We found .. in our parents dir. */
            (*pmom)->pdrive = pdrive;
            (*pmom)->blkinfo.my_frstblock =  sectorno;
            (*pmom)->blkinfo.my_block     =  sectorno;
            (*pmom)->blkinfo.my_index     =  0;
            (*pmom)->isroot = NO;

            /* Read the data */
            ret_stat = pc_read_blk(&rbuf, (*pmom)->pdrive, (*pmom)->blkinfo.my_block);
            (*pmom)->pblkbuff = rbuf;
            if (ret_stat == NU_SUCCESS)
            {
                /* Convert a dos inode to in mem form. */
                pi = (DOSINODE *) &rbuf->data[0];
                pc_dos2inode((*pmom)->finode, pi,pdotdot->pdrive->dh);

                /* Free current buffer. */
                pc_free_buf(rbuf, NO);

                /* See if the inode is in the buffers */
                pfi = pc_scani(pdrive, sectorno, 0);
                if (pfi)
                {
                    /* Since we changed the list go back to the top. */
                    pc_freei((*pmom)->finode);
                    (*pmom)->finode = pfi;
                }
                else
                {
                    /* Mark the inode in the inode buffer. */
                    pd = &((*pmom)->blkinfo);
                    pc_marki((*pmom)->finode, (*pmom)->pdrive, pd->my_block,
                        pd->my_index);
                }
            }
            else    /* Error, something didn't work */
            {
                /* We're done with pmom for now. */
                pc_freeobj(*pmom);
            }
        }
    }
    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       pc_mkchild
*
* DESCRIPTION
*
*       Allocate an object and fill in as much of the  block pointer
*       section as possible based on the parent.
*
*
* INPUTS
*
*       *pmom                               Drive object
*
* OUTPUTS
*
*       Returns a partially initialized DROBJ if enough core available
*       and pmom was a valid subdirectory.
*
*************************************************************************/
DROBJ *pc_mkchild(DROBJ *pmom)
{
    DROBJ       *pobj;
    DIRBLK      *pd;

    pobj = pc_allocobj();   /* Allocate an empty DROBJ and FINODE. */
    if (pobj)
    {
        pd = &pobj->blkinfo;
        pobj->isroot = NO;              /* Child can not be root */
        pobj->pdrive =  pmom->pdrive;   /* Child inherits moms drive */
        /* Setup a handle for yielding the inode lock */
        pobj->finode->lock_object.dh = pobj->pdrive->dh;
        /* Now initialize the fields storing where the child inode lives */
        pd->my_index = 0;
        pd->my_block = pd->my_frstblock = pc_firstblock(pmom);
        if (!pd->my_block)
        {
            /* We're done with pobj for now. */
            pc_freeobj(pobj);
            pobj = NU_NULL;
        }
        else
        {
            /* Set absolute path length. */
            pobj->finode->abs_length = pmom->finode->abs_length + 1;
        }
    }
    return(pobj);
}

/************************************************************************
* FUNCTION
*
*       pc_sfn_chksum
*
* DESCRIPTION
*
*       Simple checksum algorithm used to create pseudo random value.
*
*
* INPUTS
*
*       *buf_ptr(in)                           Pointer to buffer
*       length(in)                             Length of the buffer
*
* OUTPUTS
*
*       Returns the checksum value from buf_ptr.
*
*************************************************************************/
static UINT16 pc_sfn_chksum (UINT8 *buf_ptr, UINT32 length)
{
    UINT16  sum = 0;
    static INT8 rand_shift = 0;

    /* Loop through entire buffer */
    while (length)
    {
        /* Add current character */
        sum += (UINT16)*buf_ptr;
        
        /* Move to next character */
        buf_ptr++;

        /* Decrement remaining characters */
        length--;            
    }

    /* Get 1's compliment of the sum */
    sum = (UINT16)~sum;
    sum += (sum >> rand_shift);

    /* Don't allow rand_shift to exceed 16 because sum is a UINT16. */
    if(++rand_shift > 16)
    {
    	rand_shift = 0;    
    }
    
    /* Return inverted result */
    return ((UINT16) ~sum);
    
}

/************************************************************************
* FUNCTION
*
*       pc_gen_sfn
*
* DESCRIPTION
*
*       This function will attempt to generate a unique SFN. It will 
*       do this by performing a checksum on the LFN then and adding 
*       in other potentially random values to increase the changes 
*       of the checksum being unique. If the first two characters in 
*       the LFN are ASCII then they will be used as the first two 
*       characters in the SFN. If the first two characters aren't ASCII 
*       then SF will be used. The next 4 characters will be randomly 
*       selected using the calculated checksum value as a key into a 
*       table. The range of the next 4 characters is limited to the 
*       hex values (0-F). Character seven will be a tilde ('~') and 
*       character eight will be 1. 
*
*
* INPUTS
*
*       *sfn_fname(out)                        Where the SFN that is 
*                                              generated is to be stored.
*       *lfn(in)                               LFN whose SFN is to be 
*                                              generated
*       *time_date                             Used to make the checksum
*                                              value more random
*       *free_cl_cnt                           Used to make the checksum
*                                              value more random

*
* OUTPUTS
*
*       Returns the checksum value from buf_ptr.
*
*************************************************************************/
STATUS pc_gen_sfn(UINT8 *sfn_fname, UINT8 *lfn, DATESTR *time_date, UINT32 free_cl_cnt)
{
    UINT32 lfn_len;
    UINT16 chksum;
    UINT8 sfn_tbl_idx;
    UNSIGNED plus_clk;   
    STATUS ret_val = NU_SUCCESS;
    UINT8 lfn_char;
    static UNSIGNED num_times_func_called = 0;
    static UINT8 sfn_gen_table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                                   'A', 'B', 'C', 'D', 'E', 'F'};
    
    if((sfn_fname != NU_NULL) && (lfn != NU_NULL) && (time_date != NU_NULL))
    {
        lfn_len = NUF_Get_Str_Len((CHAR*)lfn);    

        /* Now add time and free cluster counts to lfn_checksum, because the time and free cluster 
          count are always changing. */
        chksum = pc_sfn_chksum(lfn, lfn_len);
        chksum += (free_cl_cnt>>16);
        chksum += time_date->time;
        chksum += (free_cl_cnt & 0x0000FFFF);

        /* These are mainly used for systems that don't have the time/date functions in 
          file\base\src\ pc_update.c implemented. If pc_getsysdate is implemented
          then these three lines can be commented out or removed, because pc_getsysdate
          should plus free_cl_cnt should provide enough randomness to the checksum value. */
        plus_clk = NU_Retrieve_Clock();
        chksum += ((num_times_func_called*plus_clk)>>16);
        chksum += ((num_times_func_called*plus_clk) & 0x0000FFFF);
        
          
        lfn_char = *lfn;
        /* If lfn is an ASCII character, then use it. Otherwise, make sfn
           all ascii. Check first character of LFN. */
        if(((lfn_char > 'A') && (lfn_char < 'Z')) || ((lfn_char > '0') && (lfn_char < '9')))
        {
            *sfn_fname++ = lfn_char;
        }
        else
        {
            /* If characters are ASCII, but lowercase. Then make them uppercase,
              because SFN characters have to be uppercase according to FAT specification. */
            if(((lfn_char > 'a') && (lfn_char < 'z')))
            {            
                *sfn_fname++ = ('A' + (lfn_char - 'a'));
            }
            else
            {
                /* lfn is a non-ASCII character so use our default. */
                *sfn_fname++ = 'S';
            }
        }

        ++lfn;
        lfn_char = *lfn;
        
        /* If lfn is in ascii, then use it. Otherwise, make sfn
           all ascii. Check second character of LFN. */
        if(((lfn_char > 'A') && (lfn_char < 'Z')) || ((lfn_char > '0') && (lfn_char < '9')))
        {
            *sfn_fname++ = lfn_char;       
        }
        else
        {
            /* If characters are ASCII, but lowercase. Then make them uppercase,
              because SFN characters have to be uppercase according to FAT specification. */
            if(((lfn_char > 'a') && (lfn_char < 'z')))
            {            
                *sfn_fname++ = ('A' + (lfn_char - 'a'));
            }
            else
            {
                /* lfn is a non-ASCII character so use our default. */
                *sfn_fname++ = 'F';
            }
        }
        
        /* Now copy in hashed characters 3-6(inclusive). */
        sfn_tbl_idx = (UINT8)(chksum>>12);
        *sfn_fname++ = sfn_gen_table[sfn_tbl_idx];

        sfn_tbl_idx = (UINT8)((chksum>>8) &0x000F);
        *sfn_fname++ = sfn_gen_table[sfn_tbl_idx];

        sfn_tbl_idx = (UINT8)((chksum>>4) &0x000F);
        *sfn_fname++ = sfn_gen_table[sfn_tbl_idx];
        
        sfn_tbl_idx = (UINT8)(chksum & 0x000F);
        *sfn_fname++ = sfn_gen_table[sfn_tbl_idx];

        /* Characters 7 and 8. */
        *sfn_fname++ = '~';
        *sfn_fname = '1';

        ++num_times_func_called;
    }
    else
    {
        /* If any of the pointers passed in are NU_NULL, the the filesystem
          has messed something up. */
        ret_val = NUF_INTERNAL;
    }

    return ret_val;
   
}
/************************************************************************
* FUNCTION
*
*       pc_mknode
*
* DESCRIPTION
*
*       Creates a file or subdirectory ("inode") depending on the flag
*       values in attributes. A pointer to an inode is returned for
*       further processing.
*       Note: After processing, the DROBJ must be released by calling
*       pc_freeobj.
*
*
* INPUTS
*
*       **pobj                              Create file's DROBJ pointer
*       *pmom                               Drive object
*       *filename                           Create filename
*       *fileext                            Create file extension
*       attributes                          Create file attributes
*
*
* OUTPUTS
*
*       NU_SUCCESS                          If service is successful.
*       NUF_ROOT_FULL                       Root directory full.
*       NUF_INVNAME                         Path or filename includes
*                                            invalid character.
*       NUF_NOSPC                           No space to create directory
*                                             in this disk.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_NO_FINODE                       No FINODE buffer available.
*       NUF_NO_DROBJ                        No DROBJ buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS pc_mknode(DROBJ **pobj, DROBJ *pmom, UINT8 *filename,
                 UINT8 *fileext, UINT8 attributes)
{
STATUS      ret_stat;
DOSINODE    *pdinodes;
FINODE      lfinode;
UINT32      cluster;
DATESTR     crdate;
BLKBUFF     *pbuff;
DDRIVE      *pdrive;
DROBJ       *psobj;
UINT8       attr;
UINT8       fname[MAX_SFN + 1];
UINT8       fext[MAX_EXT + 1];
UINT8       shortname[MAX_SFN + MAX_EXT + 2];
INT         longfile;
STATUS      status;
#if ENABLE_SFN_GEN
INT         sfn_attempts = 0;
INT         sfn_collisions = 0;
#endif

    /* Defaults. */
    *pobj = NU_NULL;
    ret_stat = NU_SUCCESS;

    /* Move DDRIVE pointer into local pointer. */
    pdrive = pmom->pdrive;

    /* Make a directory? */
    if (attributes & ADIRENT)
    {
        /*Grab a cluster for a new dir and clear it */
        PC_FAT_ENTER(pdrive->dh)

        /* Allocate a cluster. */
        ret_stat = pc_clalloc(&cluster, pdrive);

#ifdef DEBUG1
        DEBUG_PRINT("pc_mknode  allocate directory cluster number %d\r\n", cluster);
#endif

        /* Release non-exclusive use of FAT. */
        PC_FAT_EXIT(pdrive->dh)

        if (ret_stat == NU_SUCCESS)
        {
            /* Zero out the cluster. */
            ret_stat = pc_clzero(pdrive, cluster);
            if (ret_stat != NU_SUCCESS)
            {
                /* Grab exclusive access to the FAT. */
                PC_FAT_ENTER(pdrive->dh)
                    /* Release the cluster. */
                    pc_clrelease(pdrive, cluster);
                /* Release non-exclusive use of FAT. */
                PC_FAT_EXIT(pdrive->dh)
            }
        }
    }
    else    /* File case. */
        cluster = 0L;

    if (ret_stat != NU_SUCCESS)
    {
        return(ret_stat);
    }

    /* For a subdirectory, first make it a simple file. We will change the
    attribute after all is clean */

    attr = attributes;
    if (attr & ADIRENT)
        attr = ANORMAL;


    /* Allocate an empty DROBJ and FINODE to hold the new file */
    *pobj = pc_allocobj();
    if (!(*pobj))
    {
        if (cluster)
        {
            /* Grab exclusive access to the FAT. */
            PC_FAT_ENTER(pdrive->dh)
            /* Release the cluster. */
            pc_clrelease(pdrive, cluster);
            /* Release non-exclusive use of FAT. */
            PC_FAT_EXIT(pdrive->dh)
        }

        return(NUF_NO_DROBJ);
    }
    else
        /* Setup the lock to allow yielding */
        (*pobj)->finode->lock_object.dh = pdrive->dh;


    
    /* Take a (long or short) filename and extension and attempt see if either
      SFN is sufficient or LFN is needed. */
    longfile = pc_fileparse(fname, fext, filename, fileext);

    if (longfile < 0)
    {
        ret_stat = (STATUS)longfile;
    }
    else
    {
        /* Upperbar short filename ?  */
        while( (longfile == FUSE_UPBAR) && (ret_stat == NU_SUCCESS) )
        {

#if ENABLE_SFN_GEN
            /* If our hash function has a collision, then just keep incrementing numeric tail,
              till we find a unique name. */
            if(!sfn_collisions && (sfn_attempts >= SFN_STANDARD_ATTEMPTS))
            {
                pc_getsysdate(&crdate);
                ret_stat = pc_gen_sfn(fname, filename, &crdate, pdrive->free_clusters_count);                
                if(ret_stat == NU_SUCCESS)
                {
                    sfn_attempts = 0;
                    ++sfn_collisions;
                }
                else
                {   
                    /* If an error was encountered when generating our SFN, exit loop
                    because the filesystem can't create a SFN. */
                    break;
                }
            }
#endif

            /* Search the short filename */
            pc_cre_shortname((UINT8 *)shortname, fname, fext);
            psobj = NU_NULL;
            ret_stat = pc_get_inode(&psobj, pmom, (UINT8 *)shortname);

            /* File not found. We can use this short filename. */
            if (ret_stat == NUF_NOFILE)
            {
                ret_stat = NU_SUCCESS;
                break;
            }
            else if (ret_stat == NU_SUCCESS)
            {
                /* Free the short filename object */
                pc_freeobj(psobj);
                
                /* Get the next short filename */
                pc_next_fparse(fname);                
            }

#if ENABLE_SFN_GEN
            ++sfn_attempts;
#endif
        }
    }


    if (ret_stat != NU_SUCCESS)
    {
        /* We're done with pobj for now. */
        pc_freeobj(*pobj);
        if (cluster)
        {
            /* Grab exclusive access to the FAT. */
            PC_FAT_ENTER(pdrive->dh)
            /* Release the cluster. */
            pc_clrelease(pdrive, cluster);
            /* Release non-exclusive use of FAT. */
            PC_FAT_EXIT(pdrive->dh)
        }

        return(ret_stat);
    }

#if ENABLE_SFN_GEN
    if(sfn_collisions >= 1)
    {        
        /* Load the inode copy name, ext, attr, cluster, size, date, and time*/
        pc_init_inode( (*pobj)->finode, fname, fext,
            attr, cluster, /*size*/ 0L, &crdate);
    }
    else
    {
        /* Load the inode copy name, ext, attr, cluster, size, date, and time*/
        pc_init_inode( (*pobj)->finode, fname, fext,
        attr, cluster, /*size*/ 0L, pc_getsysdate(&crdate) );
    }
#else
    /* Load the inode copy name, ext, attr, cluster, size, date, and time*/
    pc_init_inode( (*pobj)->finode, fname, fext,
        attr, cluster, /*size*/ 0L, pc_getsysdate(&crdate) );
#endif
    /* Convert pobj to native and stitch it in to mom */
    ret_stat = pc_insert_inode(*pobj, pmom, filename, longfile);
    if (ret_stat != NU_SUCCESS)
    {
        /* Root directory entry full or Disk full ? */
        if ( (ret_stat == NUF_ROOT_FULL) || (ret_stat == NUF_NOSPC) )
        {
            /* Try again */
            ret_stat = pc_insert_inode(*pobj, pmom, filename, longfile);
            if (ret_stat != NU_SUCCESS)
            {
                /* We're done with pobj for now. */
                pc_freeobj(*pobj);
                if (cluster)
                {
                    /* Grab exclusive access to the FAT. */
                    PC_FAT_ENTER(pdrive->dh)
                    /* Release the cluster. */
                    pc_clrelease(pdrive, cluster);
                    /* Release non-exclusive use of FAT. */
                    PC_FAT_EXIT(pdrive->dh)
                }

                return(ret_stat);
            }
        }
        else
        {
            /* We're done with pobj for now. */
            pc_freeobj(*pobj);
            if (cluster)
            {
                /* Grab exclusive access to the FAT. */
                PC_FAT_ENTER(pdrive->dh)
                /* Release the cluster. */
                pc_clrelease(pdrive, cluster);
                /* Release non-exclusive use of FAT. */
                PC_FAT_EXIT(pdrive->dh)
            }

            return(ret_stat);
        }
    }

    /* Now if we are creating a subdirectory we have to make the DOT
    and DOT DOT inodes and then change pobj's attribute to ADIRENT.
    The DOT and DOTDOT are not buffered inodes. */
    if (attributes & ADIRENT)
    {
        /* Set up a buffer to do surgery */
        status = pc_alloc_blk(&pbuff, pdrive, pc_cl2sector(pdrive, cluster));
        if (status < 0)
        {
            /* We're done with pobj for now. */
            pc_freeobj(*pobj);
            /* Grab exclusive access to the FAT. */
            PC_FAT_ENTER(pdrive->dh)
            /* Release the cluster. */
            pc_clrelease(pdrive, cluster);
            /* Release non-exclusive use of FAT. */
            PC_FAT_EXIT(pdrive->dh)

                return((STATUS)status);
        }
        NUF_Memfill(pbuff->data, 512, '\0');

        pdinodes = (DOSINODE *) &pbuff->data[0];
        /* Load DOT and DOTDOT in native form */
        /* DOT first. It points to the beginning of this sector */
        pc_init_inode(&lfinode, (UINT8 *)".", (UINT8 *)"", ADIRENT, cluster, /*size*/ 0L, &crdate);

        /* And to the buffer in intel form */
        pc_ino2dos (pdinodes, &lfinode,pdrive->dh);


        /* Now DOTDOT points to mom's cluster */
        if (pmom->isroot)
        {
            pc_init_inode(&lfinode, (UINT8 *)"..", (UINT8 *)"", ADIRENT, 0L, /*size*/ 0L, &crdate);
        }
        else
        {
            pc_init_inode(&lfinode, (UINT8 *)"..", (UINT8 *)"", ADIRENT,
            pc_sec2cluster(pdrive, (*pobj)->blkinfo.my_frstblock),
            /*size*/ 0L, &crdate);
        }

        /* And to the buffer in intel form */
        pc_ino2dos (++pdinodes, &lfinode,pdrive->dh);
        /* Write the cluster out */
        ret_stat = pc_write_blk(pbuff);
        if (ret_stat != NU_SUCCESS)
        {
            /* Error. Free current buffer. */
            pc_free_buf(pbuff, YES);
            /* We're done with pobj for now. */
            pc_freeobj(*pobj);
            /* Grab exclusive access to the FAT. */
            PC_FAT_ENTER(pdrive->dh)
            /* Release the cluster. */
            pc_clrelease(pdrive, cluster);
            /* Release non-exclusive use of FAT. */
            PC_FAT_EXIT(pdrive->dh)

            return(ret_stat);
        }
        else
            /* Free current buffer. */
            pc_free_buf(pbuff, NO);

        /* And write the node out with the original attributes */
        (*pobj)->finode->fattribute = attributes;

        /* Convert to native and overwrite the existing inode*/
        ret_stat = pc_update_inode(*pobj, DSET_CREATE);
        if (ret_stat != NU_SUCCESS)
        {
            /* We're done with pobj for now. */
            pc_freeobj(*pobj);
            /* Grab exclusive access to the FAT. */
            PC_FAT_ENTER(pdrive->dh)
            /* Release the cluster. */
            pc_clrelease(pdrive, cluster);
            /* Release non-exclusive use of FAT. */
            PC_FAT_EXIT(pdrive->dh)

            return(ret_stat);
        }
    }

    /* Grab exclusive access to the FAT. */
    PC_FAT_ENTER(pdrive->dh)
    /* Flush the file allocation table. */
    ret_stat = pc_flushfat(pdrive);
    /* Release non-exclusive use of FAT. */
    PC_FAT_EXIT(pdrive->dh)

        if (ret_stat != NU_SUCCESS)
        {
            /* We're done with pobj for now. */
            pc_freeobj(*pobj);
            /* Grab exclusive access to the FAT. */
            PC_FAT_ENTER(pdrive->dh)
            /* Release the cluster. */
            pc_clrelease(pdrive, cluster);
            /* Release non-exclusive use of FAT. */
            PC_FAT_EXIT(pdrive->dh)
        }

       return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       pc_insert_inode
*
* DESCRIPTION
*
*       Take mom, a fully defined DROBJ, and pobj, a DROBJ with a
*       finode containing name, ext, etc., but not yet stitched into the
*       inode buffer pool, and fill in pobj and its inode. Write it to
*       disk and make the inode visible in the inode buffer pool.
*
*
* INPUTS
*
*       *pobj                               Create file's DROBJ.
*       *pmom                               Drive object.
*       *filename                           Create file name.
*       longfile                            If 1, long file name is
*                                            given.
*
* OUTPUTS
*
*       NU_SUCCESS                          If service is successful.
*       NUF_ROOT_FULL                       Root directory full.
*       NUF_NOSPC                           No space to create directory
*                                             in this disk.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS pc_insert_inode(DROBJ *pobj, DROBJ *pmom, UINT8 *lfilename,
                       INT longfile)
{
STATUS      ret_stat;
INT         ret_val;
BLKBUFF     *pbuff;
DIRBLK      *pd;
DOSINODE    *pi;
INT16       i;
UINT32      cluster;
INT16       len;
INT16       entry = 0,entry_cou;
INT16       found_flag = 0;
LNAMINFO    *linfo;
BLKBUFF     *cblk;
UINT8       *lmake_ptr;
LNAMENT     *ent;
INT16       ci;
INT16       fileend_flag;
UINT16      add_entrysec;
UINT8       chk_byte;
CHAR*       str_ptr;


    /* Set up pobj. */
    pobj->pdrive = pmom->pdrive;
    pobj->isroot = NO;
    pd = &pobj->blkinfo;

    /* Long filename is given. */
    if (longfile)
    {
        /* measure long filename length. */
        for (len = 0; (NUF_GET_CHAR(&str_ptr,(CHAR*)lfilename,len) == NU_SUCCESS)
                      && *str_ptr != '\0'; len++){;}

        /* Calculate how many entry is needed for long filename. */
        entry = (len + 12) / 13;

        /* Increase for short filename. */
        entry++;
    }

    /* Initialize long filename entry counter. */
    entry_cou = 0;

    /* Now get the start of the dir. */
    pd->my_block = pd->my_frstblock = pc_firstblock(pmom);
    if (!pd->my_block)
    {
        return(NUF_INTERNAL);
    }

    pd->my_index = 0;

    /* Set the blank entry blocks and index. */
    if (pmom->blkinfo.end_block)
    {
        pd->my_block = pmom->blkinfo.end_block;
        pd->my_index = pmom->blkinfo.end_index;
    }

    /* Move long filename information pointer into internal pointer. */
    linfo = &pobj->linfo;

    /* Long filename is given. */
    if (longfile)
    {
        /* Set the long file name entry. */
        linfo->lnament = entry - 1;
    }

    /* Read the directory entry data. */
    ret_stat = pc_read_blk(&pbuff, pobj->pdrive, pobj->blkinfo.my_block);
    pobj->pblkbuff = pbuff;
    while (ret_stat == NU_SUCCESS)
    {
        /* Move directory entry pointer into local pointer. */
        i = pd->my_index;
        pi = (DOSINODE *) &pbuff->data[0];
        pi += i;

        /* Look for a slot. */
        while (i < INOPBLOCK)
        {
            /* End of dir if name is 0. */
            if ( (pi->fname[0] == '\0') || (pi->fname[0] == PCDELETE) )
            {
                /* Long filename is given. */
                if (longfile)
                {
                    /* This is long filename first entry. */
                    if (!entry_cou)
                    {
                        /* Set long filename start block. */
                        linfo->lnamblk1 = pbuff;
                        linfo->lnamblk2 = NU_NULL;
                        linfo->lnamblk3 = NU_NULL;

                        /* Set long filename start index. */
                        linfo->lnam_index = i;

                    }
                    /* Save BLKBUFF pointer into long filename info
                    structure. */
                    /* BLKBUFF pointer 2 is not used yet. */
                    if (linfo->lnamblk2 == 0)
                    {
                        /* Read block is changed. */
                        if (linfo->lnamblk1 != pbuff)
                        {
                            /* Save new BLKBUFF pointer into long filename
                            info structure. */
                            linfo->lnamblk2 = pbuff;
                        }
                    }
                    /* BLKBUFF pointer 2 is used and BLKBUFF pointer 3
                    is not used yet. */
                    else if (linfo->lnamblk3 == 0)
                    {
                        /* Read block is changed. */
                        if (linfo->lnamblk2 != pbuff)
                        {
                            /* Save new BLKBUFF pointer into long filename
                            info structure. */
                            linfo->lnamblk3 = pbuff;
                        }
                    }
                    /* Long filename entry counter increment. */
                    entry_cou++;
                    /* Found room to put the long filename entry. */
                    if (entry_cou >= entry)
                        /* We found enough space. */
                        found_flag = 1;
                }
                else    /* Short filename. */
                    found_flag = 1;

                if (found_flag)     /* Found enough space. */
                {
                    /* Set current index */
                    pd->my_index = i;

#ifdef DEBUG1
                    DEBUG_PRINT("pc_insert_inode my_block index %d\r\n",
                        pd->my_block, pd->my_index);
#endif
                    /* Update the DOS disk. */
                    pc_ino2dos(pi, pobj->finode,pobj->pdrive->dh);

                    /* Long filename case. */
                    if (longfile)
                    {
                        /* Calculate check value of short filename. */
                        chk_byte = chk_sum((unsigned char *)pi);

                        /* Calculate long filename entry. */
                        entry--;

                        /* Get LNAMENT pointer. */
                        ent = (LNAMENT *)pi;
                        /* Set long filename pointer. */
                        lmake_ptr = lfilename;
                        /* Clear file name end flag. */
                        fileend_flag = 0;

                        /* Set the current block. */
                        cblk = pbuff;

                        /* Create long filename entry. */
                        for (entry_cou = 0; entry_cou < entry; entry_cou++)
                        {
                            if ( ent == (LNAMENT *)&cblk->data[0] )
                            {
                                /* Is short filename entry in current
                                block? */
                                if (cblk != pbuff)
                                {
                                    /* Write out current block. */
                                    ret_stat = pc_write_blk(cblk);
                                    if (ret_stat != NU_SUCCESS)
                                    {
                                        /* Clean long filename
                                        information. */
                                        lnam_clean(linfo, pbuff);
                                        /* Free current buffer. */
                                        pc_free_buf(pbuff, YES);

                                        return(ret_stat);
                                    }
                                    /* Free current buffer. */
                                    pc_free_buf(cblk, NO);
                                }

                                /* Current block is 3rd. */
                                if (cblk == linfo->lnamblk3)
                                {
                                    /* Set current block pointer block
                                    2nd. */
                                    cblk = linfo->lnamblk2;
                                }
                                /* Current block is 2nd. */
                                else if (cblk == linfo->lnamblk2)
                                {
                                    /* Set current block pointer block
                                    3rd. */
                                    cblk = linfo->lnamblk1;
                                }

                                /* Move long filename entry pointer into
                                local pointer. */
                                ent = (LNAMENT *)
                                    &cblk->data[(INOPBLOCK-1) << 5];
                            }
                            else
                                ent--;

                            /* Set sequence number. */
                            ent->ent_num = (UINT8) (entry_cou + 1);

                            /* Top entry. */
                            if ( (entry_cou + 1) == entry )
                                ent->ent_num += 0x40;

                            /* Set file attributes and checksum. */
                            ent->attrib = 0x0f;
                            ent->reserve = 0;
                            ent->snamchk = chk_byte;
                            ent->fatent[0] = 0;
                            ent->fatent[1] = 0;

                            /* Write 1st filename block. */
                            for (ci = 0; ci < 10; ci += 2)
                            {
                                if (fileend_flag)
                                {
                                    ent->str1[ci] = 0xFF;
                                    ent->str1[ci+1] = 0xFF;
                                }
                                else
                                {
                                    /* Decode UTF8 into Unicode. */
                                    utf8_to_unicode(&ent->str1[ci], lmake_ptr);
                                    if (!*lmake_ptr)
                                        fileend_flag = 1;
                                    else
                                        NUF_NEXT_CHAR(lmake_ptr);
                                }
                            }
                            /* Write 2nd filename block. */
                            for (ci = 0; ci < 12; ci += 2)
                            {
                                if (fileend_flag)
                                {
                                    ent->str2[ci] = 0xFF;
                                    ent->str2[ci+1] = 0xFF;
                                }
                                else
                                {
                                    /* Decode UTF8 into Unicode. */
                                    utf8_to_unicode(&ent->str2[ci], lmake_ptr);
                                    if (!*lmake_ptr)
                                        fileend_flag = 1;
                                    else
                                        NUF_NEXT_CHAR(lmake_ptr);

                                }
                            }
                            /* Write 3rd filename block. */
                            for (ci = 0; ci < 4; ci += 2)
                            {
                                if (fileend_flag)
                                {
                                    ent->str3[ci] = 0xFF;
                                    ent->str3[ci+1] = 0xFF;
                                }
                                else
                                {
                                    /* Decode UTF8 into Unicode. */
                                    utf8_to_unicode(&ent->str3[ci], lmake_ptr);
                                    if (!*lmake_ptr)
                                        fileend_flag = 1;
                                    else
                                        NUF_NEXT_CHAR(lmake_ptr);
                                }
                            }
                        }
                        if (cblk != pbuff)
                        {
                            /* Write out current block. */
                            ret_stat = pc_write_blk(cblk);
                            if (ret_stat != NU_SUCCESS)
                            {
                                /* Clean long filename information. */
                                lnam_clean(linfo, pbuff);
                                /* Free current buffer. */
                                pc_free_buf(pbuff, YES);

                                return(ret_stat);
                            }
                            /* Free current buffer. */
                            pc_free_buf(cblk, NO);
                        }
                    }

                    /* Write the data. */
                    ret_stat = pc_write_blk(pobj->pblkbuff);
                    /* Mark the inode in the inode buffer. */
                    if (ret_stat == NU_SUCCESS)
                    {
                        /* Mark the inode in the inode buffer. */
                        pc_marki(pobj->finode, pobj->pdrive,
                            pd->my_block, pd->my_index );
                        /* Free current buffer. */
                        pc_free_buf(pbuff, NO);
                    }
                    else
                        /* Error. Free current buffer. */
                        pc_free_buf(pbuff, YES);

                    return(ret_stat);
                }
            }
            else
            {
                /* Save the long filename BLKBUFF pointer. */
                if (entry_cou)
                {
                    /* Clean long filename information. */
                    lnam_clean(linfo, pbuff);
                    /* Resetup linfo */
                    linfo = &pobj->linfo;
                }
                entry_cou = 0;
            }
            i++;
            pi++;
        }

        if (! entry_cou)
        {
            /* Not in that block. Try again. */
            pc_free_buf(pbuff, NO);
        }

        /* Update the objects block pointer. */
        ret_stat = pc_next_block(pobj);

        /* No more next allocated block or error. */
        if (ret_stat != NU_SUCCESS)
            break;

        /* Read the next directory sector. */
        ret_stat = pc_read_blk(&pbuff, pobj->pdrive,
            pobj->blkinfo.my_block);
        pobj->pblkbuff = pbuff;
        pd->my_index = 0;
    }

    if ( (ret_stat != NU_SUCCESS) && (ret_stat != NUF_NOSPC) )
    {
        return(ret_stat);
    }

    /* Check if the search start pointer is set. */
    if (pmom->blkinfo.end_block)
    {

        /* Clean long filename BLKBUFF pointer. */
        if (entry_cou)
        {
            /* Clean long filename information. */
            lnam_clean(linfo, pbuff);

            /* Free the buffer. */
            pc_free_buf(pbuff, NO);

        }

        /* Clear the block entry blocks and index. */
        pmom->blkinfo.end_block = (UINT32)0;
        pmom->blkinfo.end_index = (UINT16)0;

        /* To search again from first block, return once. */
        return(NUF_NOSPC);
    }

    if (pobj->pdrive->fasize <= 4)
    {
        /* Hmmm - root full?. This is a problem. */
        if (pc_isroot(pmom))
        {
#ifdef DEBUG2
            DEBUG_PRINT("Root directory full\r\n");
#endif

            return(NUF_ROOT_FULL);
        }
    }

    /* There are no slots in mom. We have to make one.
    And copy our stuff in. */

    /* Grab exclusive access to the FAT. */
    PC_FAT_ENTER(pobj->pdrive->dh)

    /* Grow a directory chain. */
    ret_stat = pc_clgrow(&cluster, pobj->pdrive,
    pc_sec2cluster(pmom->pdrive, pobj->blkinfo.my_block));

    /* Release non-exclusive use of FAT. */
    PC_FAT_EXIT(pobj->pdrive->dh)

        if (ret_stat != NU_SUCCESS)
        {
            return(ret_stat);
        }

        /* Zero out the cluster. */
        ret_stat = pc_clzero(pobj->pdrive, cluster);
        if (ret_stat != NU_SUCCESS)
        {
            /* Grab exclusive access to the FAT. */
            PC_FAT_ENTER(pobj->pdrive->dh)
            /* Release the cluster. */
            pc_clrelease(pobj->pdrive, cluster);
            /* Release non-exclusive use of FAT. */
            PC_FAT_EXIT(pobj->pdrive->dh)

                return(ret_stat);
        }

        /* Don't forget where the new item is. */
        pd->my_block = pc_cl2sector(pobj->pdrive, cluster);
        pd->my_index = 0;

        /* Copy the item into the first block. */
        /* Setup the short filename entry block buffer. */
        if (longfile)
        {
            /* Use entry blocks is short filename block only. */
            add_entrysec = 0;

            /* Setup the short filename entry. */
            if (linfo->lnamblk1)
            {
                /* Three sector step over to directory entry? */
                if ( ( (linfo->lnam_index + entry - INOPBLOCK) > INOPBLOCK ) )
                {
                    if (!linfo->lnamblk2)
                    {
                        /* Number of add allocate blocks. */
                        add_entrysec++;

                        /* Set the short filename entry block number. */
                        pd->my_block += 1;
                    }
                    /* Set the short filename entry index. */
                    if (linfo->lnam_index + entry > INOPBLOCK)
                    {
                        pd->my_index = linfo->lnam_index +
                            entry - INOPBLOCK - 1 - INOPBLOCK;
                    }
                    else
                    {
                        pd->my_index = linfo->lnam_index + entry - 1;
                    }
                }
                else
                {
                    if (linfo->lnam_index + entry > INOPBLOCK)
                        pd->my_index = linfo->lnam_index + entry - INOPBLOCK - 1;
                    else
                        pd->my_index = linfo->lnam_index + entry - 1;
                }
            }
            else
            {
                /* Two sector step over to directory entry? */
                if (entry > INOPBLOCK)
                {
                    /* Number of add allocate blocks. */
                    add_entrysec++;

                    /* Set the short filename entry block number. */
                    pd->my_block += 1;
                    pd->my_index = entry - INOPBLOCK - 1;
                }
                else
                    pd->my_index = entry - 1;
            }

            if (add_entrysec)
            {
                /* Set up the first block of the long filename entry. */
                ret_val = pc_alloc_blk(&cblk, pobj->pdrive, (pd->my_block - 1));
                if (ret_val < 0)
                {
                    /* Grab exclusive access to the FAT. */
                    PC_FAT_ENTER(pobj->pdrive->dh)
                    /* Release the cluster. */
                    pc_clrelease(pobj->pdrive, cluster);
                    /* Release non-exclusive use of FAT. */
                    PC_FAT_EXIT(pobj->pdrive->dh)

                    return((STATUS)ret_val);
                }

                /* Clear the data block. */
                NUF_Memfill(cblk->data, 512, '\0');

                /* Set the long filename block buffer. */
                if (linfo->lnamblk1)
                {
                    /* Three sector step over to directory entrys
                    Note: Long filename information setup
                    (linfo->lnamblk2 == NULL)
                    linfo->lnamblk1 : already setup(First long filename
                    entry blocks).
                    linfo->lnamblk2 : cblk(Next long filename entry blocks).
                    linfo->lnamblk3 : Short filename entry blocks.
                    */
                    linfo->lnamblk2 = cblk;
                    linfo->lnamblk3 = NU_NULL;
                }
                else
                {
                    /* Two sector step over to directory entries
                    Note: Long filename information setup
                    linfo->lnamblk1 : cblk(First long filename entry
                    blocks).
                    linfo->lnamblk2 : Short filename entry blocks.
                    linfo->lnamblk3 : NULL.
                    */
                    linfo->lnamblk1 = cblk;
                    linfo->lnamblk2 = NU_NULL;
                    linfo->lnamblk3 = NU_NULL;
                }
            }
        }

        /* Allocate short filename entry block buffer. */
        ret_val = pc_alloc_blk(&pbuff, pobj->pdrive, pd->my_block);
        if (ret_val < 0)
        {
            /* Grab exclusive access to the FAT. */
            PC_FAT_ENTER(pobj->pdrive->dh)
            /* Release the cluster. */
            pc_clrelease(pobj->pdrive, cluster);
            /* Release non-exclusive use of FAT. */
            PC_FAT_EXIT(pobj->pdrive->dh)

            return((STATUS)ret_val);
        }

        /* Clear the data block. */
        NUF_Memfill(pbuff->data, 512, '\0');

        if (longfile) /* Long filename */
        {
            /* Move to end directory entry index on the sector. */
            pi = (DOSINODE *)&pbuff->data[pd->my_index << 5];

            /* Create short filename entry. */
            pc_ino2dos((DOSINODE *)pi, pobj->finode,pobj->pdrive->dh);

            /* Calculate check value of short filename. */
            chk_byte = chk_sum((unsigned char *)pi);

            /* Calculate long filename entry. */
            entry--;

            /* Get LNAMENT pointer. */
            ent = (LNAMENT *)pi;
            /* Set long filename pointer. */
            lmake_ptr = lfilename;
            /* Clear filename end flag. */
            fileend_flag = 0;

            /* Set the current block. */
            cblk = pbuff;

            /* Set the BLKBUFF pointer into long filename info structure. */
            if (linfo->lnamblk1 == 0)
            {
                linfo->lnamblk1 = cblk;
            }
            else if (linfo->lnamblk2 == 0)
            {
                linfo->lnamblk2 = cblk;
            }
            else if (linfo->lnamblk3 == 0)
            {
                linfo->lnamblk3 = cblk;
            }

            for (entry_cou = 0; entry_cou < entry; entry_cou++)
            {
                if ( ent == (LNAMENT *)&cblk->data[0] )
                {
                    /* Is short filename entry in current block? */
                    if (cblk != pbuff)
                    {
                        /* Write out current block. */
                        ret_stat = pc_write_blk(cblk);
                        if (ret_stat != NU_SUCCESS)
                        {
                            /* Clean long filename information. */
                            lnam_clean(linfo, pbuff);
                            /* Error. Free current buffer. */
                            pc_free_buf(pbuff, YES);

                            /* Grab exclusive access to the FAT. */
                            PC_FAT_ENTER(pobj->pdrive->dh)
                            /* Release the cluster. */
                            pc_clrelease(pobj->pdrive, cluster);
                            /* Release non-exclusive use of FAT. */
                            PC_FAT_EXIT(pobj->pdrive->dh)

                                return(ret_stat);
                        }
                        /* Free current buffer. */
                        pc_free_buf(cblk, NO);
                    }

                    /* Current block is 3st. */
                    if (cblk == linfo->lnamblk3)
                    {
                        /* Set current block pointer block 2nd. */
                        cblk = linfo->lnamblk2;
                    }
                    /* Current block is 2nd. */
                    else if (cblk == linfo->lnamblk2)
                    {
                        /* Set current block pointer block 1rd. */
                        cblk = linfo->lnamblk1;
                    }
                    /* Move long filename entry pointer into local pointer. */
                    ent = (LNAMENT *)&cblk->data[(INOPBLOCK-1) << 5];
                }
                else
                    ent--;

                /* Set sequence number. */
                ent->ent_num = (UINT8) (entry_cou + 1);

                if ( (entry_cou + 1) == entry ) /* Top entry. */
                    ent->ent_num += 0x40;

                /* Set file attributes and checksum. */
                ent->attrib = 0x0f;
                ent->reserve = 0;
                ent->snamchk = chk_byte;
                ent->fatent[0] = 0;
                ent->fatent[1] = 0;

                for (ci = 0; ci < 10; ci += 2)
                {
                    if (fileend_flag)
                    {
                        ent->str1[ci] = 0xFF;
                        ent->str1[ci+1] = 0xFF;
                    }
                    else
                    {
                        /* Decode UTF8 into Unicode. */
                        utf8_to_unicode(&ent->str1[ci], lmake_ptr);
                        if (!*lmake_ptr)
                            fileend_flag = 1;
                        else
                            NUF_NEXT_CHAR(lmake_ptr);
                    }
                }
                for (ci = 0; ci < 12; ci += 2)
                {
                    if (fileend_flag)
                    {
                        ent->str2[ci] = 0xFF;
                        ent->str2[ci+1] = 0xFF;
                    }
                    else
                    {
                        /* Decode UTF8 into Unicode. */
                        utf8_to_unicode(&ent->str2[ci], lmake_ptr);
                        if (!*lmake_ptr)
                            fileend_flag = 1;
                        else
                            NUF_NEXT_CHAR(lmake_ptr);
                    }
                }
                for (ci = 0; ci < 4; ci += 2)
                {
                    if (fileend_flag)
                    {
                        ent->str3[ci] = 0xFF;
                        ent->str3[ci+1] = 0xFF;
                    }
                    else
                    {
                        /* Decode UTF8 into Unicode. */
                        utf8_to_unicode(&ent->str3[ci], lmake_ptr);
                        if (!*lmake_ptr)
                            fileend_flag = 1;
                        else
                            NUF_NEXT_CHAR(lmake_ptr);
                    }
                }
            }
            if (cblk != pbuff)
            {
                /* Write out current block. */
                ret_stat = pc_write_blk(cblk);
                if (ret_stat != NU_SUCCESS)
                {
                    /* Clean long filename information. */
                    lnam_clean(linfo, pbuff);
                    /* Error. Free current buffer. */
                    pc_free_buf(pbuff, YES);

                    /* Grab exclusive access to the FAT. */
                    PC_FAT_ENTER(pobj->pdrive->dh)
                    /* Release the cluster. */
                    pc_clrelease(pobj->pdrive, cluster);
                    /* Release non-exclusive use of FAT. */
                    PC_FAT_EXIT(pobj->pdrive->dh)

                        return(ret_stat);
                }
                /* Free current buffer. */
                pc_free_buf(cblk, NO);
            }
        }
        else /* short filename */
        {
            /* Create short filename entry. */
            pc_ino2dos((DOSINODE *)&pbuff->data[0], pobj->finode,pobj->pdrive->dh);
        }

        /* Write it out. */
        ret_stat = pc_write_blk(pbuff);
        if (ret_stat != NU_SUCCESS)
        {
            /* Error. Free current buffer. */
            pc_free_buf(pbuff, YES);
            /* Grab exclusive access to the FAT. */
            PC_FAT_ENTER(pobj->pdrive->dh)
            /* Release the cluster. */
            pc_clrelease(pobj->pdrive, cluster);
            /* Release non-exclusive use of FAT. */
            PC_FAT_EXIT(pobj->pdrive->dh)

                return(ret_stat);
        }

        /* We made a new slot. Mark the inode as belonging there. */
        pc_marki(pobj->finode, pobj->pdrive, pd->my_block, pd->my_index);

        /* Free current buffer. */
        pc_free_buf(pbuff, NO);

        return(NU_SUCCESS);
}


/************************************************************************
* FUNCTION
*
*       pc_del_lname_block
*
* DESCRIPTION
*
*       Delete all long filename directory entry sectors.
*
*
* INPUTS
*
*       *pobj                               Drive object(Delete long
*                                            filename information )
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID pc_del_lname_block(DROBJ *pobj)
{
LNAMINFO    *linfo;
BLKBUFF     *cblk;
LNAMENT     *ent;
INT16       my_index;
INT         i;


    /* get long filename info */
    linfo = &pobj->linfo;

    /* Get long filename start block */
    cblk = linfo->lnamblk1;
    /* Get long filename start index */
    my_index = linfo->lnam_index;

    /* Move long filename entry pointer into local pointer */
    ent = (LNAMENT *)&cblk->data[0];

    /* adjusting entry pointer */
    ent += my_index;

    /* Delete all long filename entry */
    for (i = 0; i < linfo->lnament; i++)
    {
        /* Put delete character */
        ent->ent_num = PCDELETE;
        /* Increment entry pointer */
        ent++;
        /* Increment index */
        my_index++;

        /* This block end */
        if (my_index >= INOPBLOCK)
        {
            /* Reset index */
            my_index = 0;

            /* Write out current block */
            pc_write_blk(cblk);

            /* Is short filename entry in current block ? */

            if (cblk != pobj->pblkbuff)
            {
                /* Free current buffer */
                pc_free_buf(cblk, NO);
            }

            /* Current block is 1st */
            if (cblk == linfo->lnamblk1)
            {
                /* Set current block pointer block 2nd */
                cblk = linfo->lnamblk2;
            }
            /* Current block is 2nd */
            else if (cblk == linfo->lnamblk2)
            {
                /* Set current block pointer block 3rd */
                cblk = linfo->lnamblk3;
            }
            /* Move long filename entry pointer into local pointer */
            ent = (LNAMENT *)&cblk->data[0];
        }
    }
    /* Last block also need to be flush */
    pc_write_blk(pobj->pblkbuff);
}


/************************************************************************
* FUNCTION
*
*       pc_renameinode
*
* DESCRIPTION
*
*       Rename an inode.
*
*
* INPUTS
*
*       *pobj                               Rename file drive object
*       *poldmom                            Parent object for src
*       *pnewmom                            Parent object for destination
*       *fnambf                             Short filename buffer
*       *fextbf                             Short file extension buffer
*       *new_nme                            Long filename pointer
*       longdest                            Old file is long filename
*
* OUTPUTS
*
*       NU_SUCCESS                          If service is successful.
*       NUF_ROOT_FULL                       Root directory full.
*       NUF_NOSPC                           No space to create directory
*                                             in this disk.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS pc_renameinode(DROBJ *pobj, DROBJ *poldmom, DROBJ *pnewmom, UINT8 *fnambuf,
                      UINT8 *fextbuf, UINT8 *new_name, INT longdest)
{
DROBJ       *new_pobj;
STATUS      ret_stat;

    /* Convert longdest in to a form for insert inode */
    if (longdest)
        longdest = 1;

    if ( (poldmom == pnewmom) && (!longdest)) /* New filename is short filename and in same*/
    {                                         /* parent directory */
        /* Copy the filename. */
        NUF_COPYBUFF(pobj->finode->fname, fnambuf, 8);
        NUF_COPYBUFF(pobj->finode->fext, fextbuf, 3);

        /* Original filename is long filename */
        if (pobj->linfo.lnament) /* long filename */
        {
            /* We need to delete long filename entry */
            pc_del_lname_block(pobj);

        }

        /* Update an inode to disk. */
        ret_stat = pc_update_inode(pobj, DSET_ACCESS);
    }
    else /* same parents - long filename, different parents - long or short file name */
    {
        /* Allocate an empty DROBJ and FINODE to hold the new file */
        new_pobj = pc_allocobj();
        if (!new_pobj)
        {
            ret_stat = NUF_NO_DROBJ;
        }
        else
        {
            /* Load the inode copy name, ext, attr, cluster, size, date, and time*/
            pc_init_inode(new_pobj->finode, fnambuf, fextbuf,
                0, 0, /*size*/ 0L, 0);
            /* Copy the lock information. This will allow yielding the FS lock
            if fs_release/fs_reclaim is used */
            new_pobj->finode->lock_object.dh = pobj->pdrive->dh;

            /* copy attribute from original file */
            new_pobj->finode->fattribute = pobj->finode->fattribute;

            new_pobj->finode->fcrcmsec    = pobj->finode->fcrcmsec;
            new_pobj->finode->fcrtime     = pobj->finode->fcrtime;
            new_pobj->finode->fcrdate     = pobj->finode->fcrdate;

            new_pobj->finode->faccdate    = pobj->finode->faccdate;

            new_pobj->finode->fuptime     = pobj->finode->fuptime;
            new_pobj->finode->fupdate     = pobj->finode->fupdate;

            new_pobj->finode->fcluster  = pobj->finode->fcluster;
            new_pobj->finode->fsize     = pobj->finode->fsize;

            /* Convert pobj to native and stitch it into mom */
            ret_stat = pc_insert_inode(new_pobj, pnewmom, new_name, longdest );
            if (ret_stat != NU_SUCCESS)
            {
                /* Root directory entry full or Disk full ? */
                if ( (ret_stat == NUF_ROOT_FULL) || (ret_stat == NUF_NOSPC) )
                {
                    /* Try again */
                    ret_stat = pc_insert_inode(new_pobj, pnewmom, new_name, longdest );
                    if (ret_stat != NU_SUCCESS)
                    {
                        /* We're done with new_pobj for now. */
                        pc_freeobj(new_pobj);
                    }
                }
                else
                {
                    /* We're done with new_pobj for now. */
                    pc_freeobj(new_pobj);
                }
            }

            if (ret_stat == NU_SUCCESS)
            {
                /* Mark it deleted and unlink the cluster chain */
                pobj->finode->file_delete = PCDELETE;

                if (pobj->linfo.lnament) /* long filename */
                {
                    /* We need to delete long filename entry */
                    pc_del_lname_block(pobj);
                }
                /* We free up store right away. Don't leave cluster pointer
                hanging around to cause problems. */
                pobj->finode->fcluster = 0L;

                /* Update an inode to disk. */
                ret_stat = pc_update_inode(pobj, DSET_ACCESS);
                if (ret_stat == NU_SUCCESS)
                {
                    /* Grab exclusive access to the FAT. */
                    PC_FAT_ENTER(new_pobj->pdrive->dh)

                    /* Flush the file allocation table. */
                    ret_stat = pc_flushfat(new_pobj->pdrive);

                    /* Release non-exclusive use of FAT. */
                    PC_FAT_EXIT(new_pobj->pdrive->dh)
                }

                /* We're done with new_pobj for now. */
                pc_freeobj(new_pobj);
            }
        }
    }


    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       pc_rmnode
*
* DESCRIPTION
*
*       Delete the inode at pobj and flush the file allocation table.
*       Does not check file permissions or if the file is already open.
*       The inode is marked deleted on the disk and the cluster chain
*       associated with the inode is freed. (Un-delete won't work)
*
*
* INPUTS
*
*       *pobj                               Delete file drive object
*
* OUTPUTS
*
*       NU_SUCCESS                          If service is successful.
*                                            file or a special.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS pc_rmnode(DROBJ *pobj)
{
UINT32      cluster;
STATUS      ret_stat;


    /* Mark it deleted and unlink the cluster chain */
    pobj->finode->file_delete = PCDELETE;
    cluster = pobj->finode->fcluster;

    if (pobj->linfo.lnament) /* long filename */
    {
        /* We need to delete long filename entry */
        pc_del_lname_block(pobj);
    }

    /* We free up store right away. Don't leave cluster pointer
    hanging around to cause problems. */
    pobj->finode->fcluster = 0L;


    /* Update an inode to disk. */
    ret_stat = pc_update_inode(pobj, DSET_ACCESS);
    if (ret_stat == NU_SUCCESS)
    {
        /* And clear up the space */

        /* Grab exclusive access to the FAT. */
        PC_FAT_ENTER(pobj->pdrive->dh)

        /* Release the chain. */
        pc_freechain(pobj->pdrive, cluster);
        /* Flush the file allocation table. */
        ret_stat = pc_flushfat(pobj->pdrive);

        /* Release non-exclusive use of FAT. */
        PC_FAT_EXIT(pobj->pdrive->dh)
    }


    /* If it gets here we had a problem */
    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       pc_update_inode
*
* DESCRIPTION
*
*       Read the disk inode information stored in pobj and write it to
*       the block and offset on the disk where it belongs. The disk is
*       first read to get the block and then the inode info is merged in
*       and the block is written. (see also pc_mknode() )
*
*       setdate values are
*
*       DSET_ACCESS                         Set the only access-time,
*                                            date.
*       DSET_UPDATE                         Set the access-time,date and
*                                            update-time,date.
*       DSET_CREATE                         Set the all time and date.
*
*       DSET_MANUAL_UPDATE                  Set when the time and date have
*                                            already been updated manually.
*
*
* INPUTS
*
*       *pobj                               Update drive object
*       setdate                             Set date flag
*
* OUTPUTS
*
*       NU_SUCCESS                          If all went well.
*       NUF_NO_BLOCK                        No block buffer available.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS pc_update_inode(DROBJ *pobj, INT setdate)
{
STATUS      ret_stat = NU_SUCCESS;
BLKBUFF     *pbuff;
DOSINODE    *pi;
INT16       i;
DIRBLK      *pd;
DATESTR     fds;


    /* Move DIRBLK pointer to local. */
    pd = &pobj->blkinfo;
    /* Get a directory entry index. */
    i  = pd->my_index;

    /* Index into block. */
    if ( i >= INOPBLOCK || i < 0 )
        ret_stat = NUF_INTERNAL;

    if (ret_stat == NU_SUCCESS && setdate != DSET_MANUAL_UPDATE)
    {
        /* Get the date */
        pc_getsysdate(&fds);

        /* Set the file create time and date */
        if (setdate == DSET_CREATE)
        {
            pobj->finode->fcrcmsec = fds.cmsec;
            pobj->finode->fcrtime = fds.time;
            pobj->finode->fcrdate = fds.date;
        }

        /* Directory does not need file access date. */
        if ((pobj->finode->fattribute & ADIRENT) != ADIRENT)
        {
            /* Set the file update time and date. */
            if (setdate)
            {
                pobj->finode->fuptime = fds.time;
                pobj->finode->fupdate = fds.date;
            }

            /* Always set the access date. */
            pobj->finode->faccdate = fds.date;
        }
    }

    /* Read the directory data. */
    ret_stat = pc_read_blk(&pbuff, pobj->pdrive, pobj->blkinfo.my_block);
    if (ret_stat == NU_SUCCESS)
    {
        pi = (DOSINODE *) &pbuff->data[0];
        /* Copy it off and write it */
        pc_ino2dos((pi+i), pobj->finode,pobj->pdrive->dh);

        /* Write the directory data. */
        pobj->pblkbuff = pbuff;
        ret_stat = pc_write_blk(pbuff);

        /* Free the buff. If ret_stat != NU_SUCCESS, it will discard
        the pbuff. */
        if (ret_stat == NU_SUCCESS)
            pc_free_buf(pbuff, NO);
        else
            pc_free_buf(pbuff, YES);
    }

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       pc_get_root
*
* DESCRIPTION
*
*       Use the information in pdrive to create a special object for
*       accessing files in the root directory.
*
*
* INPUTS
*
*       *pdrive                             Drive information
*
* OUTPUTS
*
*       Returns a pointer to a DROBJ, or NULL if no core available.
*
*************************************************************************/
DROBJ *pc_get_root(DDRIVE *pdrive)
{
    DIRBLK      *pd;
    DROBJ       *pobj;
    FINODE      *pfi;


    /* Allocate an empty DROBJ and FINODE. */
    pobj = pc_allocobj();
    if (pobj)
    {
        /* Search for an inode list. */
        pfi = pc_scani(pdrive, 0, 0);
        if (pfi)
        {
            /* Free the inode that comes with allocobj. */
            pc_freei(pobj->finode);
            /* Since we changed the list go back to the top. */
            pobj->finode = pfi;
        }
        else    /* No inode in the inode list. Copy the data over
                and mark where it came from */
        {
            /* Mark the inode in the inode buffer. */
            pc_marki(pobj->finode, pdrive, 0, 0);
            pfi = pobj->finode;
        }
        pobj->pdrive = pdrive;
        /* Setup the lock to allow yielding of the FS semaphore */
        pobj->finode->lock_object.dh = pobj->pdrive->dh;
        /* Set up the tree stuff so we know it is the root */
        pd = &pobj->blkinfo;
        pd->my_frstblock = pdrive->rootblock;
        pd->my_block = pdrive->rootblock;
        pd->my_index = 0;
        pobj->isroot = YES;
        pfi->abs_length = 3; /* "A:\" */
    }
    return(pobj);
}


/************************************************************************
* FUNCTION
*
*       pc_firstblock
*
* DESCRIPTION
*
*       Returns the block number of the first inode in the subdirectory.
*       If pobj is the root directory the first block of the root will
*       be returned.
*
*
* INPUTS
*
*       *pobj                               Drive object
*
* OUTPUTS
*
*       Returns 0 if the obj does not point to a directory, otherwise
*       the first block in the directory is returned.
*
*************************************************************************/
UINT32 pc_firstblock(DROBJ *pobj)
{
    UINT32      ret_val;

    /* Check the directory. */
    if (!pc_isadir(pobj))
        ret_val = BLOCKEQ0;
    else if (pobj->isroot)                  /* Root directory? */
        ret_val = pobj->pdrive->rootblock;  /* yes - first block. */
    else
    {
        /* Convert the cluster number. */
        ret_val = pc_cl2sector(pobj->pdrive, pobj->finode->fcluster);
    }
    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       pc_next_block
*
* DESCRIPTION
*
*       Find the next block owned by an object in either the root or a
*       cluster chain and update the blockinfo section of the object.
*
*
* INPUTS
*
*       *pobj                               Drive object
*
* OUTPUTS
*
*       NU_SUCCESS                          If service is successful.
*       NUF_NOSPC                           No space to create directory
*                                            on this disk.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS pc_next_block(DROBJ *pobj)
{
STATUS      ret_stat;
UINT32      next;


    /* Get the next block. */
    ret_stat = pc_l_next_block(&next, pobj->pdrive, pobj->blkinfo.my_block);

    if (ret_stat == NU_SUCCESS)
    {
        /* Set the directory entry block. */
        pobj->blkinfo.my_block = next;
    }

    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       pc_l_next_block
*
* DESCRIPTION
*
*       Find the next block in either the root or a cluster chain.
*
*
* INPUTS
*
*       *block                              Next block number.
*
* OUTPUTS
*
*       NU_SUCCESS                          If service is successful.
*       NUF_NOSPC                           No space to create directory
*                                            on this disk.
*       NUF_IO_ERROR                        Driver I/O error.
*       NUF_INTERNAL                        Nucleus FILE internal error.
*
*************************************************************************/
STATUS pc_l_next_block(UINT32 *block, DDRIVE *pdrive, UINT32 curblock)
{
STATUS      ret_stat = NU_SUCCESS;
UINT32      cluster;


    /* If the block is in the root area */
    if ( (pdrive->fasize < 8) && (curblock < pdrive->firstclblock) )
    {
        /* Check root directory area. */
        if (curblock < pdrive->rootblock)
        {
            ret_stat = NUF_INTERNAL;
        }
        else if (++curblock < pdrive->firstclblock)
        {
            /* Set the next block. */
            *block = curblock;
            ret_stat = NU_SUCCESS;
        }
        else
            ret_stat = NUF_NOSPC;
    }
    else if (curblock >= pdrive->numsecs) /* Check data area. */
        ret_stat = NUF_INTERNAL;

    else    /* In cluster space */
    {
        /* Get the next block */
        curblock += 1;

        /* If the next block is not on a cluster edge then it must be
        in the same cluster as the current. - otherwise we have to
        get the first block from the next cluster in the chain */
        if (pc_sec2index(pdrive, curblock))
        {
            /* Set the next block. */
            *block = curblock;
        }
        else
        {
            /* Original current block. */
            curblock -= 1;

            /* Get the old cluster number - No error test needed */
            cluster = pc_sec2cluster(pdrive, curblock);

            /* Grab exclusive access to the FAT. */
            PC_FAT_ENTER(pdrive->dh)

            /* Consult the FAT for the next cluster. */
            ret_stat = pc_clnext(&cluster, pdrive, cluster);

            /* Release non-exclusive use of FAT. */
            PC_FAT_EXIT(pdrive->dh)

                if (ret_stat == NU_SUCCESS)
                {
                    if (cluster != 0)      /* Not end of chain */
                        /* Convert the new cluster number. */
                        *block = pc_cl2sector(pdrive, cluster);
                    else
                        ret_stat = NUF_NOSPC;
                }
        }
    }
    return(ret_stat);
}


/************************************************************************
* FUNCTION
*
*       pc_marki
*
* DESCRIPTION
*
*       Each inode is uniquely determined by DRIVE, BLOCK and Index into
*       that block. This routine takes an inode structure assumed to
*       contain the equivalent of a DOS directory entry, and stitches it
*       into the current active inode list. Drive block and index are
*       stored for later calls to pc_scani and the inode's opencount is
*       set to one.
*
*
* INPUTS
*
*       *pfi                                File directory entry
*       *pdrive                             Drive information
*       sectorno                            Start sector number
*       index                               Entry number
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID pc_marki(FINODE *pfi, DDRIVE *pdrive, UINT32 sectorno, INT16 index)
{
    /* Mark the drive, sectorno, index. */
    pfi->my_drive = pdrive;
    pfi->my_block = sectorno;
    pfi->my_index = index;
    pfi->opencount = 1;
    pfi->file_delete = 0;

    /* Stitch the inode at the front of the list */
    if (inoroot)
        inoroot->pprev = pfi;

    pfi->pprev = NU_NULL;
    pfi->pnext = inoroot;
    inoroot = pfi;
}


/************************************************************************
* FUNCTION
*
*       pc_scani
*
* DESCRIPTION
*
*       Each inode is uniquely determined by DRIVE, BLOCK and Index into
*       that block. This routine searches the current active inode list
*       to see if the inode is in use. If so the opencount is changed
*       and a pointer is returned. This guarantees that two processes
*       will work on the same information when manipulating the same
*       file or directory.
*
*
* INPUTS
*
*       *pdrive                             Drive object
*       sectorno                            Sector number
*       index                               Entry number
*
* OUTPUTS
*
*       A pointer to the FINODE for pdrive:sector:index or NULL if not
*       found.
*
*************************************************************************/
FINODE *pc_scani(DDRIVE *pdrive, UINT32 sectorno, INT16 index)
{
    FINODE      *pfi;
    STATUS      found = 0;

    /* Get inode list. */
    pfi = inoroot;
    while (pfi && !found)
    {
        /* Search a drive's directory entry. */
        if ( (pfi->my_drive == pdrive) &&
            (pfi->my_block == sectorno) &&
            (pfi->my_index == index) )
        {
            /* Increment inode opencount. */
            pfi->opencount += 1;
            found = 1;
        }
        else
        {
            pfi = pfi->pnext;   /* Move to the next inode. */
        }
    }
    return(pfi);
}


/************************************************************************
* FUNCTION
*
*       pc_allocobj
*
* DESCRIPTION
*
*       Allocates and zeros the space needed to store a DROBJ structure.
*       Also allocates and zeros a FINODE structure and links the two
*       via the finode field in the DROBJ structure.
*
*
* INPUTS
*
*      None.
*
* OUTPUTS
*
*      Returns a valid pointer or NULL if no more core.
*
*************************************************************************/
DROBJ *pc_allocobj(VOID)
{
    DROBJ   *pobj;
    DROBJ   *ret_val;

    /* Alloc a DROBJ */
    ret_val = NU_NULL;
    pobj = pc_memory_drobj(NU_NULL);
    if (pobj)
    {
        pobj->finode = pc_alloci();     /* Allocate a FINODE. */
        if (!pobj->finode)              /* if not available,... */
            pc_memory_drobj(pobj);      /* Free the DROBJ */
        else
            ret_val = pobj;             /* otherwise return DROBJ. */
    }

    return(ret_val);
}


/************************************************************************
* FUNCTION
*
*       pc_alloci
*
* DESCRIPTION
*
*       Allocates and zeros a FINODE structure.
*
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       Returns a valid pointer or NULL if no more core.
*
*************************************************************************/
FINODE *pc_alloci(VOID)
{
    FINODE  *rtn_val;

    /* Allocate a FINODE structure. */
    rtn_val = pc_memory_finode(NU_NULL);

    return(rtn_val);
}


/************************************************************************
* FUNCTION
*
*       pc_free_all_i
*
* DESCRIPTION
*
*       Release all inode buffers associated with a drive.
*
*
* INPUTS
*
*       *pdr                                Drive information
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID pc_free_all_i(DDRIVE *pdrive)
{
    FINODE      *pfi;

    /* Get inode list. */
    pfi = inoroot;
    while (pfi)
    {
        /* Search a drive. */
        if (pfi->my_drive == pdrive)
        {
            /* Set the opencount to 1 so freei releases the inode */
            pfi->opencount = 1;
            pc_report_error(PCERR_FREEINODE);
            /* Free the inode that comes with allocobj. */
            pc_freei(pfi);
            /* Since we changed the list go back to the top */
            pfi = inoroot;
        }
        else
            /* Move to the next inode. */
            pfi = pfi->pnext;
    }
}


/************************************************************************
* FUNCTION
*
*       pc_freei
*
* DESCRIPTION
*
*       If the FINODE structure is only being used by one file or DROBJ,
*       unlink it from the internal active inode list and return it to
*       the heap; otherwise reduce its open count.
*
*
* INPUTS
*
*       *pfi                                File directory entry
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID pc_freei(FINODE *pfi)
{
    if (!pfi)
        pc_report_error(PCERR_FREEINODE);
    else if (pfi->opencount)
    {
        /* Decrement opencount and release INODE if zero. */
        if (!--pfi->opencount)
        {
            /* Point the guy behind us at the guy in front. */
            if (pfi->pprev)
                pfi->pprev->pnext = pfi->pnext;
            else /* No prev, we were the first, make the next guy first. */
                inoroot = pfi->pnext;
            if (pfi->pnext) /* Make the next guy point behind. */
                pfi->pnext->pprev = pfi->pprev;
            pc_memory_finode(pfi);    /* release the core */
        }
    }
    else
        pc_memory_finode(pfi);    /* release the core */
}


/************************************************************************
* FUNCTION
*
*       pc_freeobj
*
* DESCRIPTION
*
*       Return a drobj structure to the heap. Calls pc_freei to reduce
*       the open count of the finode structure it points to and return
*       it to the heap if appropriate.
*
*
* INPUTS
*
*       *pobj                               Drive object
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID pc_freeobj(DROBJ *pobj)
{
    if (pobj)
    {
        /* Free the inode that comes with allocobj. */
        pc_freei(pobj->finode);

        /* Free the Long filename entry and Short filename entry BLKBUFF
        Note: See pc_freebuf not call case in pc_findin */
        if (pobj->linfo.lnament)
        {
            /* Clear the short filename BLKBUFF */
            pc_free_buf(pobj->pblkbuff, NO);

            lnam_clean(&pobj->linfo, pobj->pblkbuff);
        }
        /* Release the core */
        pc_memory_drobj(pobj);
    }
    else
        pc_report_error(PCERR_FREEDROBJ);
}


/************************************************************************
* FUNCTION
*
*       pc_dos2inode
*
* DESCRIPTION
*
*       Take the data from pbuff which is a raw disk directory entry and
*       copy it to the inode at pdir. The data goes from INTEL byte
*       ordering to native during the transfer.
*
*
* INPUTS
*
*       *pdir                               File directory entry
*       *pbuff                              Dos directory entry
*       dh                                  Disk handle
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID pc_dos2inode(FINODE *pdir, DOSINODE *pbuff,UINT16 dh)
{
UINT16      clusterhigh;
UINT16      clusterlow;
UINT8       *utf8_char_ptr;
MTE_S       *mte;
UINT8       total_bytes_of_cp;
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1) & (ENABLE_W_JAPANESE_SHIFT_JIS == NU_TRUE)
UINT8       ftemp[2];
#endif
UINT8        cp_format[2];
    
    utf8_char_ptr = &pdir->fname[0];

    /* Get mte in order to access the codepage specific conversion functions. */
    mte = fsl_mte_from_dh(dh);
    if(mte != NU_NULL)
    {
        /* Copy filename and file extension. */
        for(total_bytes_of_cp = 0; total_bytes_of_cp < 8; )
        {
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1) & (ENABLE_W_JAPANESE_SHIFT_JIS == NU_TRUE)
            /* Special case for CP932 if leading byte is 0xE5. */
            if((total_bytes_of_cp == 0) && (mte->mte_cp->cp == CP_JAPANESE_SHIFT_JIS) 
                && (pbuff->fname[0] == 0x05))
            {
                ftemp[0] = 0xE5;
                ftemp[1] = pbuff->fname[1];
                mte->mte_cp->cp_op.cp_to_utf8(utf8_char_ptr,&ftemp[0]);
                total_bytes_of_cp += NUF_GET_CP_LEN((CHAR*)utf8_char_ptr);
                /* Get the next character that is encoded in UTF8. */
                NUF_NEXT_CHAR(utf8_char_ptr);           
            }
#endif
            
            /* If pbuff->fname[total_bytes_of_cp] was an ASCII character convert it to 
            codepage format. */
            if(ascii_to_cp_format(&cp_format[0],&pbuff->fname[total_bytes_of_cp]) == NU_SUCCESS)
            {
                mte->mte_cp->cp_op.cp_to_utf8(utf8_char_ptr,&cp_format[0]);
            }
            else
            {
                mte->mte_cp->cp_op.cp_to_utf8(utf8_char_ptr,&pbuff->fname[total_bytes_of_cp]);
            }
            total_bytes_of_cp += NUF_GET_CP_LEN((CHAR*)utf8_char_ptr);
            /* Get the next character that is encoded in UTF8. */
            NUF_NEXT_CHAR(utf8_char_ptr);
        }

        utf8_char_ptr = &pdir->fext[0];

        for(total_bytes_of_cp = 0; total_bytes_of_cp < 3; )
        {        
            /* If pbuff->fext[total_bytes_of_cp] was an ASCII character convert it to 
            codepage format. */
            if(ascii_to_cp_format(&cp_format[0],&pbuff->fext[total_bytes_of_cp]) == NU_SUCCESS)
            {
                mte->mte_cp->cp_op.cp_to_utf8(utf8_char_ptr,&cp_format[0]);
            }
            else
            {
                mte->mte_cp->cp_op.cp_to_utf8(utf8_char_ptr,&pbuff->fext[total_bytes_of_cp]);
            }       
            total_bytes_of_cp += NUF_GET_CP_LEN((CHAR*)utf8_char_ptr);
            /* Get the next character that is encoded in UTF8. */
            NUF_NEXT_CHAR(utf8_char_ptr);

        }

        /* Set file attributes. */
        pdir->fattribute = pbuff->fattribute;

        /* Reserved. */
        pdir->reserve = pbuff->reserve;

        /* Set file date and time. */
        pdir->fcrcmsec = pbuff->fcrcmsec;
        SWAP16(&pdir->fcrtime, &pbuff->fcrtime);
        SWAP16(&pdir->fcrdate, &pbuff->fcrdate);

        SWAP16(&pdir->faccdate, &pbuff->faccdate);

        SWAP16(&pdir->fuptime, &pbuff->fuptime);
        SWAP16(&pdir->fupdate, &pbuff->fupdate);

        /* Set cluster for data file. */
        SWAP16(&clusterhigh, &pbuff->fclusterhigh);
        SWAP16(&clusterlow, &pbuff->fclusterlow);
        pdir->fcluster =  (UINT32) ((clusterhigh << 16) | clusterlow);

        /* Set file size. */
        SWAP32(&pdir->fsize, &pbuff->fsize);
    }
}


/************************************************************************
* FUNCTION
*
*       pc_init_inode
*
* DESCRIPTION
*
*       Take an uninitialized inode (pdir) and fill in some fields. No
*       other processing is done. This routine simply copies the
*       arguments into the FINODE structure.
*       Note: filename & fileext do not need null termination.
*
*
* INPUTS
*
*       *pdir                               File directory entry
*       *filename                           File name
*       *fileext                            File extension
*       attr                                File attributes
*       cluster                             File start cluster number
*       size                                File size
*       *fds                                Date stamping buffer
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID pc_init_inode(FINODE *pdir, UINT8 *filename, UINT8 *fileext,
                   UINT8 attr, UINT32 cluster, UINT32 size, DATESTR *fds)
{

    /* Copy the filenames and pad with ' ''s */
    pc_cppad(pdir->fname, (UINT8 *)filename, 8);
    pc_cppad(pdir->fext, (UINT8 *)fileext, 3);

    /* Set file attributes. */
    pdir->fattribute = attr;

    /* Reserved. */
    pdir->reserve = '\0';

    if (fds)
    {
        /* Set file date and time. */
        pdir->fcrcmsec = fds->cmsec;
        pdir->fcrtime = fds->time;
        pdir->fcrdate = fds->date;

        pdir->faccdate = fds->date;

        pdir->fuptime = fds->time;
        pdir->fupdate = fds->date;
    }
    /* Set cluster for data file. */
    pdir->fcluster = cluster;
    /* Set file size. */
    pdir->fsize = size;
}


/************************************************************************
* FUNCTION
*
*       pc_ino2dos
*
* DESCRIPTION
*
*       Take in memory native format inode information and copy it to a
*       buffer. Translate the inode to INTEL byte ordering during the
*       transfer.
*
*
* INPUTS
*
*       *pbuf                               Dos Directory Entry
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID pc_ino2dos(DOSINODE *pbuff, FINODE *pdir, UINT16 dh)
{
UINT16      cltemp;
MTE_S       *mte;
UINT8       num_of_bytes_of_cp;
UINT8       *utf8_char_ptr;
UINT8       total_num_of_bytes;
UINT8       cp_value[4];
UINT8       i;

    utf8_char_ptr = &pdir->fname[0];
    /* Get mte in order to access the codepage specific conversion functions. */
    mte = fsl_mte_from_dh(dh);
    if (mte != NU_NULL)
    {
        /* Copy filename and file extension. */
        for(total_num_of_bytes = 0; total_num_of_bytes < 8;)
        {
            num_of_bytes_of_cp = mte->mte_cp->cp_op.utf8_to_cp(&cp_value[0],
                utf8_char_ptr);
            /* If it is an ASCII character in codepage format change it to 
               to Microsoft's codepage format, which is just a plan ASCII character. */    
            if(ascii_cp_format_to_ascii(&cp_value[0]) == NU_SUCCESS)
            {
                num_of_bytes_of_cp -= 1;
            }
            /* Get the next character that is encoded in UTF8. */
            NUF_NEXT_CHAR(utf8_char_ptr);

            if(total_num_of_bytes + num_of_bytes_of_cp <8)
            {
                /*copy*/
                NUF_Copybuff(&pbuff->fname[total_num_of_bytes],&cp_value[0],num_of_bytes_of_cp);
                total_num_of_bytes += num_of_bytes_of_cp;
            }
            else
            {
                for(i = 0; ((total_num_of_bytes >= 4) && (total_num_of_bytes < 8)); ++i)
                {
                    NUF_Copybuff(&pbuff->fname[total_num_of_bytes],&cp_value[i],1);
                    ++total_num_of_bytes;
                }
                break;
            }
        }

        /* Delete mark? */
        if (pdir->file_delete == PCDELETE)
            pbuff->fname[0] = PCDELETE;
#if (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CP_SUPPORT == 1)
#if (ENABLE_W_JAPANESE_SHIFT_JIS == NU_TRUE)
        /* Check to see if we have 0xE5, which means we need to change to 0x05. */
        if(pbuff->fname[0] == 0xE5 && pdir->file_delete != PCDELETE && mte->mte_cp->cp == CP_JAPANESE_SHIFT_JIS)
        {
            pbuff->fname[0] = 0x05;
        }
#endif
#endif

        utf8_char_ptr = &pdir->fext[0];
        for(total_num_of_bytes = 0; total_num_of_bytes < 3;)
        {

            num_of_bytes_of_cp = mte->mte_cp->cp_op.utf8_to_cp(&cp_value[0],
                utf8_char_ptr);
            /* If it is an ASCII character in codepage format change it to 
            to Microsoft's codepage format,which is just a plan ASCII character. */    
            if(ascii_cp_format_to_ascii(&cp_value[0]) == NU_SUCCESS)
            {
                num_of_bytes_of_cp -= 1;
            }
            /* Get the next character that is encoded in UTF8. */
            NUF_NEXT_CHAR(utf8_char_ptr);

            if(total_num_of_bytes + num_of_bytes_of_cp <3)
            {
                /*copy*/
                NUF_Copybuff(&pbuff->fext[total_num_of_bytes],&cp_value[0],num_of_bytes_of_cp);
                total_num_of_bytes += num_of_bytes_of_cp;
            }
            else
            {
                for(i = 0; total_num_of_bytes <3; ++i)
                {
                    NUF_Copybuff(&pbuff->fext[total_num_of_bytes],&cp_value[i],1);
                    ++total_num_of_bytes;
                }
                break;
            }
        }

        /* Set file attributes. */
        pbuff->fattribute = pdir->fattribute;

        /* Reserved. */
        pbuff->reserve = pdir->reserve;

        /* Set file date and time. */
        pbuff->fcrcmsec = pdir->fcrcmsec;
        SWAP16(&pbuff->fcrtime, &pdir->fcrtime);
        SWAP16(&pbuff->fcrdate, &pdir->fcrdate);

        SWAP16(&pbuff->faccdate,&pdir->faccdate);

        cltemp = (UINT16) (pdir->fcluster >> 16);
        SWAP16(&pbuff->fclusterhigh,&cltemp);

        SWAP16(&pbuff->fuptime,&pdir->fuptime);
        SWAP16(&pbuff->fupdate,&pdir->fupdate);

        /* Set cluster for data file. */
        cltemp = (UINT16) (pdir->fcluster & 0x0000ffff);
        SWAP16(&pbuff->fclusterlow,&cltemp);

        /* Set file size. */
        SWAP32(&pbuff->fsize,&pdir->fsize);
    }
}


/************************************************************************
* FUNCTION
*
*       pc_isavol
*
* DESCRIPTION
*
*       Looks at the appropriate elements in pobj and determines if it
*       is a volume.
*
*
* INPUTS
*
*       *pobj                               Drive object
*
* OUTPUTS
*
*       Returns NO if the obj does not point to a volume.
*
*************************************************************************/
INT pc_isavol(DROBJ *pobj)                                      /*__fn__*/
{

    return(pobj->finode->fattribute & AVOLUME);
}


/************************************************************************
* FUNCTION
*
*       pc_isadir
*
* DESCRIPTION
*
*       Looks at the appropriate elements in pobj and determines if it
*       is a root or subdirectory.
*

* INPUTS
*
*       *pobj                               Drive object
*
* OUTPUTS
*
*       Returns NO if the obj does not point to a directory.
*
*************************************************************************/
INT pc_isadir(DROBJ *pobj)                                      /*__fn__*/
{

    return((pobj->isroot) || (pobj->finode->fattribute & ADIRENT));
}


/************************************************************************
* FUNCTION
*
*       pc_isroot
*
* DESCRIPTION
*
*       Looks at the appropriate elements in pobj and determines if it
*       is a root directory.
*
*
* INPUTS
*
*       *pobj                               Drive object
*
* OUTPUTS
*
*       Returns NO if the obj does not point to the root directory.
*
*************************************************************************/
INT pc_isroot(DROBJ *pobj)                                      /*__fn__*/
{

    return(pobj->isroot);
}


