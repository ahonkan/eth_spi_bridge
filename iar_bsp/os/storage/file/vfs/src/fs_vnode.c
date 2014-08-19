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
*       fs_vnode.c
*                                                                       
* COMPONENT                                                             
*                                                                       
*       VNODE                                                    
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains services for managing VNODEs.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None.                                                           
*                                                                       
* FUNCTIONS                                                             
*                       
*       fsv_init_vnodes
*       fsv_release_vnodes                                                
*       fsv_is_vnode_set
*       fsv_get_vnode
*       fsv_set_vnode        
*                                                               
*************************************************************************/
#include "storage/pcdisk.h"
#include "storage/vnode_defs.h"
#include "storage/dh_extr.h"
#include "storage/fsl_extr.h"
#include "storage/user_extr.h"

/************************************************************************
* FUNCTION                                                              
*     
*   fsv_init_vnodes          
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Initializes the vnode table for a given disk handle.
*                                                                       
* INPUTS                                                                
*                                                                       
*   dh                              Disk handle
*                                                                         
* OUTPUTS                                                               
*      
*   NU_SUCCESS                      
*   NUF_INTERNAL                     
*   NU_NO_MEMORY                    Error occurred while allocating memory
*                                                                       
*************************************************************************/
STATUS fsv_init_vnodes(UINT16 dh)
{
STATUS  ret_stat;
FSDH_S  *dhs;
VNODE   *vnode;
UNSIGNED pool_size;
VOID    *pool_ptr = NU_NULL;
UINT16  i;

    /* Look up disk handle structure for disk handle */
    ret_stat = fsdh_get_fsdh_struct(dh, &dhs);
    if (ret_stat == NU_SUCCESS)
    {
        /* Verify vnodes have not been allocated */
        if (dhs->fsdh_cwd_vnodes != NU_NULL)
        {
            /* vnode table already initialized, must be an internal error */
            ret_stat = NUF_INTERNAL;
        }
    }

    /* Allocate memory for the vnode list */
    if (ret_stat == NU_SUCCESS)
    {
        /* Determine bytes required */
        pool_size = sizeof(VNODE)*(VFS_NUM_USERS_WDU);
        pool_ptr = NUF_Alloc((INT)pool_size);
        if (!pool_ptr)
            ret_stat = NU_NO_MEMORY;
    }

    /* Initialize the vnodes */
    if (ret_stat == NU_SUCCESS)
    {
        /* Setup the vnode's for this drive */
        vnode = (VNODE*) pool_ptr;

        /* Zero CWD for each possible user of this drive */
        for (i = 0; i<VFS_NUM_USERS_WDU;i++)
        {
            vnode->vnode_fsnode = NU_NULL;
            vnode++;
        }

        /* Assign CWD pool to drive */
        dhs->fsdh_cwd_vnodes = pool_ptr;

    }

    return (ret_stat);
}

/************************************************************************
* FUNCTION                                                              
*     
*   fsv_release_vnodes   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Release the vnodes associated with a disk handle
*                                                                       
* INPUTS                                                                
*                                                                       
*   dh                              Disk handle
*                                                                         
* OUTPUTS                                                               
*       
*   NU_SUCCESS
*   <0
*                                                                       
*************************************************************************/
STATUS fsv_release_vnodes(UINT16 dh)
{
STATUS  ret_stat;
INT     err_cnt = 0;
FSDH_S  *dhs;
VNODE   *vnode;
MTE_S   *mte = NU_NULL;
UINT16  i;

    /* Look up disk handle structure for disk handle */
    ret_stat = fsdh_get_fsdh_struct(dh, &dhs);
    if (ret_stat == NU_SUCCESS)
    {
        if (dhs->fsdh_cwd_vnodes == NU_NULL)
            /* Something is wrong, vnode should be initialized. */
            ret_stat = NUF_INTERNAL;
    }

    /* Get the mount table entry for this disk handle. Needed in order
       to dispatch the FS specific vnode operation */
    if (ret_stat == NU_SUCCESS)
    {
        mte = fsl_mte_from_dh(dh);
        if (mte == NU_NULL)
        {
            /* Internal error, we should be able to match a mte with 
               a disk handle */
            ret_stat = NUF_INTERNAL;
        }

    }
    
    /* Loop over the user's CWD and release them to the FS */
    if (ret_stat == NU_SUCCESS)
    {
        for (i = 0; i < VFS_NUM_USERS_WDU; i++)
        {
            /* Init a pointer to the current vnode */
            vnode = (VNODE*) &dhs->fsdh_cwd_vnodes[i];
            
            /* Ask the FS to release the vnode */
            if ((vnode) && (vnode->vnode_fsnode))
                ret_stat = mte->mte_fs->fs_vnode_deallocate(dh, vnode->vnode_fsnode); 

            /* Track the number of errors so we can report that an error occurred */
            if (ret_stat != NU_SUCCESS)
                err_cnt++;
        }
    }

    /* Free fsdh_cwd_vnode memory */
    NU_Deallocate_Memory(dhs->fsdh_cwd_vnodes);

    /* Set dhs->fsdh_cwd_vnodes to NU_NULL */
    dhs->fsdh_cwd_vnodes = NU_NULL;

    /* Check if there were errors during vnode deallocation */
    if (err_cnt > 0)
        ret_stat = NUF_INTERNAL;

    return (ret_stat);
}

/************************************************************************
* FUNCTION                                                              
*     
*   fsv_is_vnode_set   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Determine if a vnode has been set.
*                                                                       
* INPUTS                                                                
*                                                                       
*   dh                              Disk handle
*                                                                         
* OUTPUTS                                                               
*       
*   TRUE                            Vnode is set.
*   FALSE                           Vnode is not set.
*                                                                       
*************************************************************************/
STATUS fsv_is_vnode_set(UINT16 dh)
{
STATUS  ret_stat;
FSDH_S  *dhs;
VNODE   *vnode;
UINT32  idx;

    /* Lookup the current user */
    idx = fsu_get_user_index();

    /* Get the fsdh struct for this disk */
    ret_stat = fsdh_get_fsdh_struct(dh, &dhs);
    if (ret_stat == NU_SUCCESS)
    {
        if (dhs->fsdh_cwd_vnodes == NU_NULL)
            /* Something is wrong, vnode should be initialized. */
            ret_stat = NUF_INTERNAL;
    }
    
    /* Get the user's CWD vnode */
    if (ret_stat == NU_SUCCESS)
    {
        vnode = (VNODE*) &dhs->fsdh_cwd_vnodes[idx];
        /* Is it null? */
        if (vnode->vnode_fsnode)
            ret_stat = YES;
        else
            ret_stat = NO;

    }
   
    return (ret_stat);
}

/************************************************************************
* FUNCTION                                                              
*     
*   fsv_get_vnode   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Return the FS specific value for a vnode.
*                                                                       
* INPUTS                                                                
*                                                                       
*   dh
*   **fsnode
*                                                                         
* OUTPUTS                                                               
*       
*
*                                                                       
*************************************************************************/
STATUS fsv_get_vnode(UINT16 dh, VOID **fsnode)
{
STATUS ret_stat;
FSDH_S *dhs;
VNODE  *vnode;
UINT32 idx;

    /* Look up the user index */
    idx = fsu_get_user_index();

    /* Get the fsdh structure for this disk handle */
    ret_stat = fsdh_get_fsdh_struct(dh, &dhs);
    if (ret_stat == NU_SUCCESS)
    {
        if (dhs->fsdh_cwd_vnodes == NU_NULL)
            /* Something is wrong, vnode should be initialized. */
            ret_stat = NUF_INTERNAL;
    }

    /* Retrieve the FS specific information assigned to the vnode */
    if (ret_stat == NU_SUCCESS)
    {
        vnode = (VNODE*) &dhs->fsdh_cwd_vnodes[idx];

        /* Assign the FS specific node */
        *fsnode = vnode->vnode_fsnode;
    }

    return (ret_stat);
}

/************************************************************************
* FUNCTION                                                              
*     
*   fsv_set_vnode   
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*   Set the vnode to the FS specific value.
*                                                                       
* INPUTS                                                                
*                                                                       
*   dh
*   *fsnode
*                                                                         
* OUTPUTS                                                               
*       
*                                                                       
*************************************************************************/
STATUS fsv_set_vnode(UINT16 dh, VOID *fsnode)
{
STATUS ret_stat;
FSDH_S  *dhs;
VNODE   *vnode;
UINT32  idx;  

    /* Look up the user index */
    idx = fsu_get_user_index();

    /* Get the fsdh struct for this disk */
    ret_stat = fsdh_get_fsdh_struct(dh, &dhs);
    if (ret_stat == NU_SUCCESS)
    {
        if (dhs->fsdh_cwd_vnodes == NU_NULL)
            /* Something is wrong, vnode should be initialized. */
            ret_stat = NUF_INTERNAL;
    }
    
    /* Set the FS specific information for this vnode */
    if (ret_stat == NU_SUCCESS)
    {
        vnode = (VNODE*) &dhs->fsdh_cwd_vnodes[idx];
     
        /* Set the specific fs node */
        vnode->vnode_fsnode = fsnode;
    }

    return (ret_stat);
}
