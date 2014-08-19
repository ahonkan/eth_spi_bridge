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
*       lck.c
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Locking
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains functions for initialization and lock management
*       used to protect the generic VFS structures.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None.                                                           
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       lck_init_locks
*       lck_obtain_lock
*       lck_release_lock
*       fs_release
*       fs_reclaim
*                                                                       
*************************************************************************/
#include "storage/pcdisk.h"
#include "storage/lck_extr.h"
#include "storage/fsl_extr.h"

NU_SEMAPHORE    lck_big_lock;
NU_SEMAPHORE    lck_locks[LCK_MAX_LOCKS];

/************************************************************************
* FUNCTION                                                              
*     
*   lck_init_locks
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Initialize generic locks.
*                                                                       
* INPUTS                                                                
*                                                                       
*   None
*                                                                         
* OUTPUTS                                                               
*       
*   NU_SUCCESS
*                                                                       
*************************************************************************/
STATUS lck_init_locks(VOID)
{
STATUS ret_stat;
UINT8  i;
CHAR   lck_name[] = "FS_00\0";


    ret_stat = NU_Create_Semaphore(&lck_big_lock, "BIGLOCK", 1, NU_FIFO);
    
    /* Init the array of locks used by VFS layer structures */
    for (i=0; ( (i<LCK_MAX_LOCKS) && (ret_stat == NU_SUCCESS) ); i++)
    {
        /* Create a unique name for each lock. This only works for 0-9. */ 
        lck_name[4] = i + '0';
        ret_stat = NU_Create_Semaphore(&lck_locks[i],lck_name,1,NU_FIFO);
        
    }
    
    return (ret_stat);


}
/************************************************************************
* FUNCTION                                                              
*                                                                       
*       fs_release                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Allows a FS or device driver to yield locks held by the FS. This
*       facilitates fine grained multitasking. Service dispatches the FS
*       specific routine, if assigned, in order to perform the necessary
*       release of locks.
*                                                                       
* INPUTS                                                                
*                                                                       
*       dh                          Disk handle for the FS being reclaimed
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID fs_release(UINT16 dh)
{
MTE_S *mte;

    /* Look up the mount table entry for this handle */
    mte = fsl_mte_from_dh(dh);

    /* Dispatch the FS specific release routine */
    if ((mte) && (mte->mte_fs))
    {
        /* Verify the FS has a release routine */
        if (mte->mte_fs->fs_release)
            mte->mte_fs->fs_release(dh);
    }
}

/************************************************************************
* FUNCTION                                                              
*                                                                       
*       fs_reclaim                                                      
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Service to regain FS locks previously yielded from fs_release.                                                                
*                                                                       
* INPUTS                                                                
*                                                                       
*       dh                          Disk handle for the FS being reclaimed
*                                                                       
* OUTPUTS                                                               
*                                                                       
*       None.                                                           
*                                                                       
*************************************************************************/
VOID fs_reclaim(UINT16 dh)
{
MTE_S   *mte;

    /* Look up the mount table entry for this handle */
    mte = fsl_mte_from_dh(dh);

    /* Dispatch the FS specific reclaim routine */
    if ((mte) && (mte->mte_fs))
    {
        /* Verify the FS has a reclaim routine */
        if (mte->mte_fs->fs_reclaim)
            mte->mte_fs->fs_reclaim(dh);
    }

}
