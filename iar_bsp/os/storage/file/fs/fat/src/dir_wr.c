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
*       dir_wr.c
*
* COMPONENT
*
*       FAT
*
* DESCRIPTION
*
*       Wrapper routines for directory operations
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       fat_alloc_fat_dstat
*       fat_dealloc_fat_dstat
*       fat_dealloc_fat_dstat_for_dh
*       fat_copy_dstat
*       fat_wr_get_first
*       fat_wr_get_next
*       fat_wr_done
*
*************************************************************************/
#include        "storage/fat_defs.h"

FAT_DSTAT*  FAT_DSTAT_List;
/************************************************************************
* FUNCTION
*
*   fat_alloc_fat_dstat
*
* DESCRIPTION
*
*   Allocates fat_dstat objects.
*
*************************************************************************/
FAT_DSTAT* fat_alloc_fat_dstat(VOID)
{
FAT_DSTAT* fdstat;

    fdstat = NUF_Alloc(sizeof(FAT_DSTAT));
    if (fdstat)
    {
        NUF_Memfill(fdstat, sizeof(FAT_DSTAT), (UINT8) 0);

        /* Insert at the head of the list */
        fdstat->next = FAT_DSTAT_List;
        FAT_DSTAT_List = fdstat;
    }

    return (fdstat);

}
/************************************************************************
* FUNCTION
*
*   fat_dealloc_fat_dstat
*
* DESCRIPTION
*
*   Deallocates fat_dstat objects.
*
*************************************************************************/
STATUS fat_dealloc_fat_dstat(FAT_DSTAT* fdstat)
{
FAT_DSTAT* search_fdstat;
STATUS      sts;

    if (FAT_DSTAT_List)
    {
        if (FAT_DSTAT_List == fdstat)
            FAT_DSTAT_List = fdstat->next;
        else
        {
            search_fdstat = FAT_DSTAT_List;
            while((search_fdstat->next != NU_NULL) && (search_fdstat->next != fdstat))
                search_fdstat = search_fdstat->next;
            if(fdstat == search_fdstat->next)
                search_fdstat->next = fdstat->next;
        }

        NU_Deallocate_Memory(fdstat);
        sts = NU_SUCCESS;
    }
    else
        sts = -1;

    return sts;
}
/************************************************************************
* FUNCTION
*
*   fat_dealloc_fat_dstat_for_dh
*
* DESCRIPTION
*
*   Deallocates all fat_dstat objects and the DROBJ's allocated
*   when NU_Get_First is called. PC_DRIVE_ENTER(dh,YES) should be 
*   called before calling function. This function is only called from 
*   NU_Abort_Disk. fat_dealloc_fat_dstat isn't called when removing the 
*   fat_dstat object because that would result in us traversing the 
*   FAT_DSTAT_List twice.
*************************************************************************/
STATUS fat_dealloc_fat_dstat_for_dh(UINT16 dh)
{
FAT_DSTAT* trav_fdstat;
FAT_DSTAT* pre_trav_fdstat = NU_NULL; /* Used to remember the next to last node, so that 
                               if we remove the last element we can update the
                               new last node's next value to NU_NULL.*/
FAT_DSTAT* remove_fdstat;
STATUS      sts = NU_SUCCESS;
   
    /* Make sure there is something in the list. */
    if(FAT_DSTAT_List)
    {
        trav_fdstat = FAT_DSTAT_List;
        /* Traverse the list deallocating all FAT_DSTAT objects,
           and the objects it allocated. These other objects that 
           FAT_DSTAT allocated would be two DROBJ's and the DROBJ's FINODES.*/
        while(trav_fdstat != NU_NULL && sts == NU_SUCCESS)
        {
            
            /* Make sure we are only freeing the FAT_DSTAT associated
               with the passed in dh. */
            if(trav_fdstat->dh == dh)
            {
                /* Check to see if it is the first node in the list,
                   if so FAT_DSTAT_List needs to be updated because it 
                   should always point to the first node in the list. */
                if(FAT_DSTAT_List == trav_fdstat)
                {
                    FAT_DSTAT_List = FAT_DSTAT_List->next;
                }
                
                
                remove_fdstat = trav_fdstat;
                trav_fdstat = trav_fdstat->next;

                /* Special case if the last node removed was end of the list,
                   then we need to set the previous node's next element to NU_NULL. */
                if(remove_fdstat->next == NU_NULL && pre_trav_fdstat != NU_NULL)
                {
                    pre_trav_fdstat->next = NU_NULL;                    
                }


                /* Free the search object used by fat_get_first 
                   and fat_get_next. */
                if (remove_fdstat->pobj)
                    pc_freeobj(remove_fdstat->pobj);
                pc_freeobj(remove_fdstat->pmom);            
            
                sts = NU_Deallocate_Memory(remove_fdstat);

                
            }
            else
            {
                pre_trav_fdstat = trav_fdstat;
                trav_fdstat = trav_fdstat->next;
            }

                
        }
    }
    else
        sts = -1;
    
    return sts;
}

/************************************************************************
* FUNCTION
*
*   fat_copy_dstat
*
* DESCRIPTION
*
*   Copy the FAT specific directory object to the generic DSTAT.
*
*************************************************************************/
VOID fat_copy_dstat(DSTAT *statobj, FAT_DSTAT *fdstat)
{
    if (statobj && fdstat)
    {
        NUF_COPYBUFF(statobj->sfname, fdstat->sfname,9);
        NUF_COPYBUFF(statobj->fext, fdstat->fext,4);
        NUF_COPYBUFF(statobj->lfname, fdstat->lfname,EMAXPATH+1);
        statobj->fattribute = fdstat->fattribute;
        statobj->fcrcmsec = fdstat->fcrcmsec;
        statobj->fcrtime = fdstat->fcrtime;
        statobj->fcrdate = fdstat->fcrdate;
        statobj->faccdate = fdstat->faccdate;
        statobj->fclusterhigh = fdstat->fclusterhigh;
        statobj->fuptime = fdstat->fuptime;
        statobj->fupdate = fdstat->fupdate;
        statobj->fclusterlow = fdstat->fclusterlow;
        statobj->fsize = fdstat->fsize;

    }

}

/************************************************************************
* FUNCTION
*
*   fat_wr_get_first
*
* DESCRIPTION
*
*   Wrapper function for fat_get_first. Handles allocation of fat specific
*   directory objects.
*
*************************************************************************/
STATUS fat_wr_get_first(DSTAT *statobj, CHAR *pattern)
{
STATUS      sts;
FAT_DSTAT* fdstat;
MTE_S      *mte;

    /* Must be last line in declarations */
    PC_FS_ENTER()

    if (statobj && pattern)
    {
        fdstat = fat_alloc_fat_dstat();

        if (fdstat)
        {
            statobj->fs_private = (VOID *)fdstat;

            /* Get the drive number. */
            /* Convert name to a mount table entry */
            mte = fsl_mte_from_name(pattern);
            if (mte)
            {
                /* Call with the fs info */
                sts = fat_get_first(mte->mte_dh, fdstat, pattern);
            }
            else
                sts = NUF_BADDRIVE;

            if(sts == NU_SUCCESS)
            {
                /* Copy the information from our file system
                   specific object back to the generic object */
                fat_copy_dstat(statobj, fdstat);
            }
            else
            {
                fat_dealloc_fat_dstat(fdstat);
                statobj->fs_private = NU_NULL;
            }
                

        }
        else
            sts = NUF_NO_MEMORY;

    }
    else
        sts = NUF_BADPARM;

    /* Restore the kernel state */
    PC_FS_EXIT()

    return (sts);

}
/************************************************************************
* FUNCTION
*
*   fat_wr_get_next
*
* DESCRIPTION
*
*   Wrapper routine for FAT specific get next operation.
*
*************************************************************************/
STATUS fat_wr_get_next(DSTAT *statobj)
{
STATUS      sts;
FAT_DSTAT   *fdstat;

    /* Must be last line in declarations */
    PC_FS_ENTER()

    if (statobj)
    {
        if (statobj->fs_private)
        {
            fdstat = (FAT_DSTAT*) statobj->fs_private;
            sts = fat_get_next(fdstat);

            fat_copy_dstat(statobj,fdstat);

        }
        else
            sts = NUF_BADPARM;
    }
    else
        sts = NUF_BADPARM;

    /* Restore the kernel state */
    PC_FS_EXIT()

    return (sts);
}
/************************************************************************
* FUNCTION
*
*   fat_wr_done
*
* DESCRIPTION
*
*   Wrapper for the FAT done directory operation.
*
*************************************************************************/
STATUS fat_wr_done(DSTAT *statobj)
{
FAT_DSTAT   *fdstat;

    /* Must be last line in declarations */
    PC_FS_ENTER()
    
    if(statobj)
    {
        if(statobj->fs_private)
        {
            fdstat = (FAT_DSTAT *) statobj->fs_private;  
            
            fat_done(fdstat);

            fat_dealloc_fat_dstat(fdstat);
        }
    }

    /* Restore the kernel state */
    PC_FS_EXIT();

    return (NU_SUCCESS);

}
