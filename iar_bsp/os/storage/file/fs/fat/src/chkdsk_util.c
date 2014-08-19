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
*       chkdsk_util.c
*
* COMPONENT
*
*       FAT Check Disk Utility 
*
* DESCRIPTION
*
*       This file contains the utility functions needed by FILE's
*       FAT Check Disk Utility.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*       chk_bitmap_mark_cl_on_disk          Marks a cluster's bitmap
*                                           entry as VALID_ON_DISK.
*       chk_bitmap_mark_cl_chain            Marks a cluster's bitmap
*                                           entry as the value passed
*                                           in.
*       chk_bitmap_replace_if               Traverses the bitmap
*                                           replacing all values
*                                           that match condition with the
*                                           replacement value passed in.
*       chk_bitmap_set_value                Sets a cluster's bitmap value.
*       chk_bitmap_get_value                Gets a cluster's bitmap value.
*       chk_get_first_fat_trav_blk          Gets the first block of the
*                                           FAT table.
*       chk_get_next_fat_trav_blk           Gets the next block of the 
*                                           FAT table.
*       chk_stack_init                      Initializes a stack.
*       chk_stack_cleanup                   Relinquishes resources
*                                           allocated for stack.
*       chk_stack_push                      Pushes an element on the 
*                                           stack.
*       chk_stack_pop                       Pops an element off the 
*                                           stack.
*       chk_stack_peek                      Peeks at the top element
*                                           on the stack.
*       chk_get_dir_rec_cl                  Parses out a cluster value
*                                           from a directory record.                                           
*       chk_create_error_name               Creates an error name.
*       chk_int_to_str                      Converts an integer to
*                                           a string.
*       chk_traverse_all_dir_paths          Pre-order traversal of the 
*                                           directory hierarchy.
*       chk_is_cl_bad                       Used to determine if a cluster
*                                           is BAD. 
*       chk_is_cl_free                      Used to determine if a cluster
*                                           is FREE. 
*       chk_is_cl_res                       Used to determine if a cluster
*                                           is reserved value. 
*       chk_is_cl_range                     Used to determine if a cluster
*                                           is out of range, based off disk. 
*       chk_get_cl_mask                     Returns FAT specific masking value.
*
************************************************************************/
#include    "storage/chkdsk_util.h"

#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)

/* Globals, Only used with cross-link and lost cluster chain check. */
extern UINT8 *g_chkdsk_fat_bitmap;
extern STATUS chk_is_cl_valid(DDRIVE *pddrive, UINT32 cl);
extern UINT32 FILE_Unused_Param; /* Used to prevent compiler warnings */

/* This lock prevents other task from calling this API until it is complete. */
extern NU_SEMAPHORE NUF_CHKDSK_MUTEX;

#if CHKDSK_DEBUG
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

#endif /* CHKDSK_DEBUG */

extern UINT8 g_chkdsk_mode;           /* Used by the Check Disk Utility so it knows whether to attempt to fix 
                              problems found on the disk or not. */
/************************************************************************
* FUNCTION
*
*   chk_bitmap_mark_cl_on_disk
*
* DESCRIPTION
*
*   This function marks each cluster in the directory record's
*   cluster chain as VALID_ON_DISK in the g_chkdsk_fat_bitmap. 
*          
*
* INPUTS
*  
*   pdir_rec_obj(in)            Pointer to a DIR_REC_OBJ.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  Each cluster in this directory record's
*                               cluster chain was marked as VALID_ON_DISK.
*   NUF_BADPARM                 At least on of the parameters are bad.
*
*************************************************************************/
STATUS chk_bitmap_mark_cl_on_disk(DIR_REC_OBJ *pdir_rec_obj)
{ 
    STATUS ret_val = NU_SUCCESS;
    FAT_CB *pfs_cb = NU_NULL;    
    UINT32 cl;    

    if(pdir_rec_obj != NU_NULL)
    {
        /* Make sure it is a file or directory. */
        if(pdir_rec_obj->pdos_rec->fattribute != CHK_LFN_ATTR)
        {   
            ret_val = fsdh_get_fs_specific(pdir_rec_obj->dh, (VOID**)&pfs_cb);
            if(ret_val == NU_SUCCESS)
            {
                /* Get the current directory record's cluster value. */
                cl = chk_get_dir_rec_cl(pfs_cb->ddrive, pdir_rec_obj->pdos_rec);

                ret_val = chk_bitmap_mark_cl_chain(cl, pfs_cb->ddrive, VALID_ON_DISK);        
            }        
        }
    }
    else
    {
        ret_val = NUF_BADPARM;
    }

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_bitmap_mark_cl_chain
*
* DESCRIPTION
*
*   This function marks each cluster in the start_cl cluster chain 
*   as mark_value in the g_chkdsk_fat_bitmap. 
*          
*
* INPUTS
*  
*   start_cl(in)                Start of cluster chain.
*   pddrive(in)                 Pointer to a DDRIVE.
*   mark_value(in)              Value set to the 
*                               cluster's bitmap value.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  Each cluster in the cluster chain
*                               was marked as the value of mark_value 
*                               in the g_chkdsk_fat_bitmap.
*   NUF_BADPARM                 At least on of the parameters are bad.
*
*************************************************************************/
STATUS chk_bitmap_mark_cl_chain(UINT32 start_cl, DDRIVE *pddrive, UINT8 mark_value)
{
    STATUS ret_val;
    UINT32 nxt_cl, cl_value;

    if(pddrive != NU_NULL)
    {
        /* Traverse this directory record's cluster chain, marking each
           cluster in our bitmap with mark_value. */
        PC_FAT_ENTER(pddrive->dh)
        do 
        {
            ret_val = pc_clnext(&nxt_cl, pddrive, start_cl);  
            if(ret_val == NU_SUCCESS)
            {
                if(nxt_cl != 0)
                {
                    /* Ignore return value as we are just checking to see
                       if next clusters value is going to be zero.
                       Reason for checking for this is because the cluster
                       before the free cluster should be marked as EOC,
                       when handling FREE cl as invalid cluster. */
                    (VOID)pc_faxx(pddrive, nxt_cl, &cl_value);

                    ret_val = chk_is_cl_valid(pddrive, nxt_cl);
                    
                    if((chk_is_cl_free(pddrive, cl_value) == NU_TRUE) || (ret_val == NUF_CHK_CL_INVALID))
                    {                        
                        nxt_cl = 0; /* Break out of loop after marking start_cl. */
                    }
                }

                /* Mark cluster. */
                ret_val = chk_bitmap_set_value(start_cl, mark_value);                

            }

            if(ret_val == NUF_DEFECTIVEC) /* If NUF_DEFECTIVEC returned cluster is BAD. */
            {
                /* Found BAD cluster. */
                ret_val = chk_bitmap_set_value(start_cl, mark_value); 
                break;
            }

             /* Set start_cl to the next clusters. */
            start_cl = nxt_cl;    
    
        } while((start_cl) && (ret_val == NU_SUCCESS));            
        PC_FAT_EXIT(pddrive->dh)
    }
    else
    {
        ret_val = NUF_BADPARM;
    }

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_bitmap_replace_if
*
* DESCRIPTION
*
*   This function traverses the bitmap checking each value against
*   condition. If the bitmap value is equal to condition then that
*   value is replaced with replace_value.
*          
*
* INPUTS
*  
*   dh(in)                      Disk Handle
*   condition(in)               Bitmap replacement condition.
*   replace_value(in)           Replacement value if condition
*                               is meet.                               
*
* OUTPUTS
*
*   NU_SUCCESS                  g_chkdsk_fat_bitmap was successfully
*                               traversed and bitmap entries
*                               that matched the condition where
*                               replaced with replace_value.
*
*************************************************************************/
STATUS chk_bitmap_replace_if(UINT16 dh, UINT8 condition, UINT8 replace_value)
{ 
    STATUS ret_val;
    UINT32 i;
    FAT_CB *pfs_cb = NU_NULL;
    DDRIVE *pddrive;
    UINT32 value;
    UINT8 bitmap_value; 
    
    ret_val = fsdh_get_fs_specific(dh, (VOID**)&pfs_cb);
    if((ret_val == NU_SUCCESS) && (pfs_cb != NU_NULL))
    {
        pddrive = pfs_cb->ddrive;

        for(i = 2; ((i < pddrive->maxfindex) && (ret_val == NU_SUCCESS)); ++i)
        {
            bitmap_value = chk_bitmap_get_value(i);
            /* See if our bitmap value matches the condition. */
            if(bitmap_value == condition)
            {
                PC_FAT_ENTER(dh)
                ret_val = pc_faxx(pddrive, i, &value);
                PC_FAT_EXIT(dh)
                
                /* If the bitmap entry matches the condition, then replace it with
                   replace_value. */
                if((ret_val == NU_SUCCESS) && value)
                {   
                    ret_val = chk_bitmap_set_value(i, replace_value);
                }            
            }
        }
    }

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_bitmap_set_value
*
* DESCRIPTION
*
*   This function sets the cluster(idx) to value in the g_chkdsk_fat_bitmap. 
*   This function makes the assumption that every 2 bits are a
*   bitmap entry.       
*
* INPUTS
*  
*   idx(in)                     Cluster in the bitmap.
*   value(in)                   Value to set the cluster to.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  The cluster's bitmap value was set.
*   NUF_BADPARM                 Cluster passed in is invalid.                  
*
*************************************************************************/
STATUS chk_bitmap_set_value(UINT32 idx, UINT8 value)
{
    UINT8 array_value = g_chkdsk_fat_bitmap[CHK_GET_IDX(idx)];    
    UINT8 temp;
    STATUS ret_val = NU_SUCCESS;
    
    /* Get the bitmap array index's offset. */
    switch(idx%4) 
    {
        case 0:
            temp = value;
            temp <<= 6;

            array_value &= 0x3F;
            array_value |= temp;
    	    break;
        
        case 1:
            temp = value;
            temp <<= 4;

            array_value &= 0xCF;
            array_value |= temp;
    	    break;

        case 2:
            temp = value;
            temp <<= 2;

            array_value &= 0xF3;
            array_value |= temp;
    	    break;

        case 3:            
            array_value &= 0xFC;
            array_value |= value;
            break;

        default:
            ret_val = NUF_BADPARM;
            break;
    }

    if(ret_val == NU_SUCCESS)
    {
        g_chkdsk_fat_bitmap[CHK_GET_IDX(idx)] = array_value;
    }
    
    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_bitmap_get_value
*
* DESCRIPTION
*
*   This function returns the value at cluster(idx) in g_chkdsk_fat_bitmap. 
*   This function makes the assumption that every 2 bits are a
*   bitmap entry.       
*
* INPUTS
*  
*   idx(in)                     Cluster in the bitmap.
*                               
*
* OUTPUTS
*
*  UINT8                        Value at cluster idx.                   
*
*************************************************************************/
UINT8 chk_bitmap_get_value(UINT32 idx)
{  
    UINT8 array_value = g_chkdsk_fat_bitmap[CHK_GET_IDX(idx)];

    /* Get the bitmap array index's offset. */
    switch(idx % 4) 
    {
        case 0:
            array_value &= 0xC0;
            array_value >>= 6;
            break;

        case 1:
            array_value &= 0x30;
            array_value >>= 4;   
            break;

        case 2:
            array_value &= 0x0C;
            array_value >>= 2;
            break;

        case 3:            
            array_value &= 0x03;
            break;    
    }
    
    return array_value;

}

/************************************************************************
* FUNCTION
*
*   chk_get_first_fat_trav_blk
*
* DESCRIPTION
*
*   This function reads in data needed to traverse the FAT table and
*   reads in the first block of the FAT table specified by the value
*   fat_table.
*
* INPUTS
*
*   *pfat_trav_blk(out)         Data block used to traverse a
*                               FAT table.
*   dh(in)                      Disk Handle
*   fat_table(in)               Which FAT table information
*                               that fat_trav_blk is to be setup
*                               with.                               
*
* OUTPUTS
*
*   NU_SUCCESS                  fat_trav_blk was setup correctly.
*   NUF_BADPARM                 At least on of the parameters are bad.
*
*************************************************************************/
STATUS chk_get_first_fat_trav_blk(FAT_TABLE_TRAV_BLK *pfat_trav_blk, UINT16 dh, UINT8 fat_table)
{
    STATUS  ret_val;        
    DDRIVE  *pddrive;
    FAT_CB  *pfs_cb = NU_NULL;

    if(pfat_trav_blk != NU_NULL)
    {
        /* Convert a disk handle to a drive number */
        ret_val = fsdh_get_fs_specific(dh, (VOID**)&pfs_cb);
        if((ret_val == NU_SUCCESS) && (pfs_cb != NU_NULL))
        {        
            pddrive = pfs_cb->ddrive;

            if(fat_table == 1)
	            pfat_trav_blk->current_fat_sector = pfat_trav_blk->start_fat_sector = pddrive->fatblock;
            else
                pfat_trav_blk->current_fat_sector = pfat_trav_blk->start_fat_sector = pddrive->fatblock + (pddrive->secpfat * (fat_table - 1));

            if(CHK_IS_DRIVE_FAT32(pddrive) == NU_TRUE)
            {
               /* We subtract one because start sector, plus secpfat would put us at the start of the next fat table. */
                pfat_trav_blk->last_fat_sector = pfat_trav_blk->start_fat_sector + pddrive->bigsecpfat - 1;
            }
            else
            {
                /* We subtract one because start sector, plus secpfat would put us at the start of the next fat table. */
                pfat_trav_blk->last_fat_sector = pfat_trav_blk->start_fat_sector + pddrive->secpfat - 1;
            }        
	        pfat_trav_blk->fat_table = (UINT16)fat_table;
	        pfat_trav_blk->dh = dh;
        
            PC_DRIVE_IO_ENTER(dh)
            ret_val = fs_dev_io_proc(pfat_trav_blk->dh, (UINT32)pfat_trav_blk->start_fat_sector, &(pfat_trav_blk->data[0]), 1, YES);
            PC_DRIVE_IO_EXIT(dh)
        }
    }
    else
    {
        ret_val = NUF_BADPARM;
    }

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_get_next_fat_trav_blk
*
* DESCRIPTION
*
*   This function reads in the next sector from the FAT specified
*   in fat_trav_blk.
*
* INPUTS
*
*   *pfat_trav_blk(in/out)      Data block used to traverse a
*                               FAT table.
*
* OUTPUTS
*
*   NU_SUCCESS                  Next sector in the FAT table was read.   
*   NUF_BADPARM                If the fat_trav_blk is invalid or its
*                               current sector data has exceeded the 
*                               last sector in this FAT table.
*   
*
*************************************************************************/
STATUS chk_get_next_fat_trav_blk(FAT_TABLE_TRAV_BLK *pfat_trav_blk)
{
    STATUS ret_val;

    if(pfat_trav_blk != NU_NULL)
    {
        if(pfat_trav_blk->current_fat_sector <= pfat_trav_blk->last_fat_sector)
        {
            PC_DRIVE_IO_ENTER(pfat_trav_blk->dh)
            ret_val = fs_dev_io_proc(pfat_trav_blk->dh, (UINT32)(++(pfat_trav_blk->current_fat_sector)), &(pfat_trav_blk->data[0]), 1, YES);
            PC_DRIVE_IO_EXIT(pfat_trav_blk->dh)
        }
        else
        {
            ret_val = NUF_BADPARM;
        }
    }
    else
    {
        ret_val = NUF_BADPARM;
    }

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_stack_init
*
* DESCRIPTION
*
*   This function initializes a stack used to traverse all
*   the directory records on the disk.   
*          
*
* INPUTS
*  
*   *pstack(in/out)             Pointer to DIR_REC_LOC_STACK 
*                               structure.
*   size(in)                    Size to create the stack.
*
* OUTPUTS
*
*   NU_SUCCESS                  If the stack was successfully created.
*   NUF_NO_MEMORY               Not enough memory to allocate stack.
*   NUF_BADPARM                 At least on of the parameters are bad.
*
*************************************************************************/
STATUS chk_stack_init(DIR_REC_LOC_STACK *pstack, INT size)
{
    STATUS ret_val = NU_SUCCESS;

    if(pstack != NU_NULL)
    {
        pstack->stack_top_offset = 0;
        pstack->dir_trav_blk_array = (DIR_REC_LOC *)NUF_Alloc(size);
        if(!pstack->dir_trav_blk_array)
        {
            pstack->dir_trav_blk_array = NU_NULL;
            ret_val = NUF_NO_MEMORY;
        }
    }
    else
    {
        ret_val = NUF_BADPARM;
    }
    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_stack_cleanup
*
* DESCRIPTION
*
*   This function releases all the resources that were allocated
*   by chk_stack_init.   
*          
*
* INPUTS
*  
*   *pstack(in)                 Pointer to DIR_REC_LOC_STACK 
*                               structure.
*
* OUTPUTS
*
*   NU_SUCCESS                  If all resources where relinquished
*                               back to the system successfully.
*   NUF_BADPARM                 At least on of the parameters are bad.
*
*************************************************************************/
STATUS chk_stack_cleanup(DIR_REC_LOC_STACK *pstack)
{
    STATUS ret_val;

    if(pstack != NU_NULL)
    {
        pstack->stack_top_offset = 0;
        ret_val = NU_Deallocate_Memory((VOID*)pstack->dir_trav_blk_array);
    }
    else
    {
        ret_val = NUF_BADPARM;
    }

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_stack_push
*
* DESCRIPTION
*
*   Pushes an element onto the stack.   
*          
*
* INPUTS
*  
*   *pstack(in)                 Pointer to DIR_REC_LOC_STACK 
*                               structure.
*   *pdir_rec_blk(in)           Pointer to a element to push onto 
*                               the stack.
*
* OUTPUTS
*
*   NU_SUCCESS                  If pdire_rec_blk was added to the 
*                               stack successfully.
*   NUF_BADPARM                 At least on of the parameters are bad.
*
*************************************************************************/
STATUS chk_stack_push(DIR_REC_LOC_STACK *pstack, DIR_REC_LOC *pdir_rec_blk)
{   
    STATUS ret_val = NU_SUCCESS;    

    if((pstack != NU_NULL) && (pdir_rec_blk != NU_NULL))
    {
        /* Adjust stack count so that next element will be pushed on the top. */    
        pstack->dir_trav_blk_array[pstack->stack_top_offset].offset = pdir_rec_blk->offset;
        pstack->dir_trav_blk_array[pstack->stack_top_offset].cl_num = pdir_rec_blk->cl_num;
        pstack->dir_trav_blk_array[pstack->stack_top_offset].sec_num = pdir_rec_blk->sec_num;    
        pstack->dir_trav_blk_array[pstack->stack_top_offset].fat_12_16_root = pdir_rec_blk->fat_12_16_root;

        ++pstack->stack_top_offset;
    }
    else
    {
        ret_val = NUF_BADPARM;
    }

    return ret_val; 
}

/************************************************************************
* FUNCTION
*
*   chk_stack_pop
*
* DESCRIPTION
*
*   Pops an element off the stack.   
*          
*
* INPUTS
*  
*   *pstack(in)                 Pointer to DIR_REC_LOC_STACK 
*                               structure.
*   *pdir_rec_blk(out)          Pointer to a DIR_REC_LOC that will 
*                               be filled with the information of the
*                               element popped off the stack.
*
* OUTPUTS
*
*   NU_SUCCESS                  If element was popped off the stack 
*                               successfully.
*   NUF_CHK_STACK_EMPTY         There are no elements in the stack.
*   NUF_BADPARM                 At least on of the parameters are bad.
*
*************************************************************************/
STATUS chk_stack_pop(DIR_REC_LOC_STACK *pstack, DIR_REC_LOC *pdir_rec_blk)
{
    STATUS ret_val = NU_SUCCESS;
    
    /* Make sure the stack isn't empty. */
    if((pstack != NU_NULL) && (pdir_rec_blk != NU_NULL))
    {    
        if(CHK_STACK_ISEMPTY(pstack) == NU_FALSE)
        {
            --pstack->stack_top_offset;
            pdir_rec_blk->offset = pstack->dir_trav_blk_array[pstack->stack_top_offset].offset;
            pdir_rec_blk->cl_num = pstack->dir_trav_blk_array[pstack->stack_top_offset].cl_num;
            pdir_rec_blk->sec_num = pstack->dir_trav_blk_array[pstack->stack_top_offset].sec_num;        
            pdir_rec_blk->fat_12_16_root = pstack->dir_trav_blk_array[pstack->stack_top_offset].fat_12_16_root;
        }
        else
        {
            ret_val = NUF_CHK_STACK_EMPTY;
        }
    }
    else
    {
        ret_val = NUF_BADPARM;
    }

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_stack_peek
*
* DESCRIPTION
*
*   Returns a copy of the top element on the stack, but doesn't 
*   remove it from the stack.
*          
*
* INPUTS
*  
*   *pstack(in)                 Pointer to DIR_REC_LOC_STACK 
*                               structure.
*   *pdir_rec_blk(out)          Pointer to a DIR_REC_LOC that will be
*                               filled with the stack's top element 
*                               info.
*
* OUTPUTS
*
*   NU_SUCCESS                  If a copy of the top element was 
*                               copied to pdire_rec_blk.
*   NUF_CHK_STACK_EMPTY         There are no elements in the stack.
*   NUF_BADPARM                 At least on of the parameters are bad.
*
*************************************************************************/
STATUS chk_stack_peek(DIR_REC_LOC_STACK *pstack, DIR_REC_LOC *pdir_rec_blk)
{
    STATUS ret_val = NU_SUCCESS;

    if((pstack != NU_NULL) && (pdir_rec_blk != NU_NULL))
    {    
        if(CHK_STACK_ISEMPTY(pstack) == NU_FALSE)
        {    
            pdir_rec_blk->offset = pstack->dir_trav_blk_array[pstack->stack_top_offset].offset;
            pdir_rec_blk->cl_num = pstack->dir_trav_blk_array[pstack->stack_top_offset].cl_num;
            pdir_rec_blk->sec_num = pstack->dir_trav_blk_array[pstack->stack_top_offset].sec_num;        
            pdir_rec_blk->fat_12_16_root = pstack->dir_trav_blk_array[pstack->stack_top_offset].fat_12_16_root;
        }
        else
        {           
            ret_val = NUF_CHK_STACK_EMPTY;
        }
    }
    else
    {
        ret_val = NUF_BADPARM;
    }
    
    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_get_dir_rec_cl
*
* DESCRIPTION
*
*   Returns the cluster value of the directory record being pointed to
*   by pdir_rec. Zero is returned if an error occurs.        
*          
*
* INPUTS
*  
*   pddrive(in)                 Pointer to a DDRIVE structure.
*   pdir_rec(in)                Pointer to a directory record entry.
*                               
*
* OUTPUTS
*
*   cl                          Cluster value in the directory record
*                               that pdir_rec is pointing to.                
*
*************************************************************************/
UINT32 chk_get_dir_rec_cl(DDRIVE *pddrive, DOSINODE *pdir_rec)
{
    UINT32 cl = 0;

    if((pddrive != NU_NULL) && (pdir_rec != NU_NULL))
    {
        if(CHK_IS_DRIVE_FAT32(pddrive))
        {
            cl = pdir_rec->fclusterhigh;
            cl <<= 16;
            cl |= pdir_rec->fclusterlow;        
        }
        else
        {            
            cl = pdir_rec->fclusterlow;        
        }
    }

    return cl;
}

/************************************************************************
* FUNCTION
*
*   chk_get_eoc_value_for_drive
*
* DESCRIPTION
*
*   Gets the end of cluster marker depending on the FAT type
*   associated with the pddrive.
*          
*
* INPUTS
*  
*   pddrive(in)                 Pointer to a DDRIVE structure.
*                               
*
* OUTPUTS
*
*   eoc_marker                  End of cluster marker for
*                               this DDRIVE.              
*
*************************************************************************/
UINT32 chk_get_eoc_value_for_drive(DDRIVE *pddrive)
{
    UINT32 eoc_marker;
    UINT32 cl_mask;
    
    /* Determine our FAT cluster mask. */    
    cl_mask = chk_get_cl_mask(pddrive);

    eoc_marker = (cl_mask & CHK_EOC);
    
    return eoc_marker;
}

/************************************************************************
* FUNCTION
*
*   chk_create_error_name
*
* DESCRIPTION
*
*   Takes a error string(perror_str) and appends a number(error_num) 
*   to the end of it to form the string(perror_name). If we are 
*   creating an error file name then we add the extension to our
*   returned string.
*   
*
* INPUTS
*
*   *perror_name(out)           This pointer will point to the completed 
*                               error name with the number append after 
*                               the name.
*   *perror_str(in)             Pointer to the error string.
*   error_num(in)               The number to be appended to perror_str.
*   b_fl_name(in)               Boolean indicating if this file/dir
*                               error name is a file or directory.
*                                                         
*
* OUTPUTS
*
*   NU_SUCCESS                  If the error name was successfully 
*                               created.
*   NUF_BADPARM                 perror_str doesn't point to a string.  
*
*************************************************************************/ 
STATUS chk_create_error_name(CHAR *perror_name, CHAR *perror_str, UINT16 error_num, UINT8 b_fl_name)
{
    UINT8 err_str_len;    
    CHAR error_num_str[CHK_MAX_DIGIT_IDX_IN_ERR_NAME];
    UINT8 i;
    STATUS ret_val = NU_SUCCESS;
    CHAR *perr_str_trav = perror_str;

    if(perror_str != NU_NULL)
    {
        /* Determine the string length. */        
        err_str_len = (UINT8)NUF_Get_Str_Len(perr_str_trav);

        /* Set a traversal string so we don't modify where error_name is pointing. */
        perr_str_trav = perror_name;

        /* Append the name. */
        NUF_Ncpbuf((UINT8*)perr_str_trav, (UINT8*)perror_str, err_str_len);

        perr_str_trav += err_str_len;

        /* Convert the integer to a string. */
        chk_int_to_str(&error_num_str[0],CHK_MAX_DIGIT_IDX_IN_ERR_NAME,error_num);
    
        /* Append the integers that are now in string format,
           to perror_name. */
        for(i = 0; i < CHK_MAX_DIGIT_IDX_IN_ERR_NAME; ++i)
        {
           if(error_num_str[i] == ' ')
               continue;
           else
           {
               *perr_str_trav = error_num_str[i];
               ++perr_str_trav;
           }
               
        }
        /* Is this a filename we are creating, if so we want to include the extension. */
        if(b_fl_name == NU_TRUE)
        {
            *perr_str_trav = '.';

            /* Add the extension. */
            NUF_Copybuff(++perr_str_trav, CHK_FILE_ERROR_EXTENSION, 4);
        }
    
        *perr_str_trav = '\0';
        
    }
    else
    {
        ret_val = NUF_BADPARM;
    }
    
    return ret_val;    
}

/************************************************************************
* FUNCTION
*
*   chk_make_err_dir
*
* DESCRIPTION
*
*   Takes a sfn directory name and adds and extension to it. This extension
*   will be a number. The number will range from 000-999. This is done
*   to allow check disk to be able to run multiple times on the same
*   disk ensuring that any new errors found will be stored in a new
*   error directory and that the old error directory won't be removed.
*
*   Example:*dirname = "DIR"
*   First we will try to create "DIR.000" if that exist then we simply
*   increment the extension by 1("DIR.001) and try that.  
*   
*
* INPUTS
*
*   *dirname(in/out)            Pointer to a directory name that will 
*                               have three additional characters appended
*                               to it. This string should be 13 characters
*                               long: 8 sfn, 3 ext, 1 for dot and 1 for '/0'.                                                        
*
* OUTPUTS
*
*   NU_SUCCESS                  If three characters where appended to
*                               dirname.
*   NUF_BADPARM                 dirname is invalid or not long enough.
*
*************************************************************************/ 
STATUS chk_make_err_dir(CHAR *dirname)
{
    STATUS ret_val;
    UINT16 ext_size = 3;
    CHAR dir_num_ext[(CHK_MAX_DIGIT_IDX_IN_ERR_NAME > MAX_EXT) ? CHK_MAX_DIGIT_IDX_IN_ERR_NAME : MAX_EXT];
    UINT8 i;
    UINT8 j;
    UINT32 str_len;
    
    if(dirname != NU_NULL)
    {
        str_len= NUF_Get_Str_Len(dirname);    
        if(str_len + 3 <= (MAX_SFN + MAX_EXT))
        {
            dirname[str_len++] = '.';
            /* Set root so we can count the number of dirname. */
            ret_val = NU_Set_Current_Dir("\\");
            if(ret_val == NU_SUCCESS)
            {
                for(i = 0; i < 255; ++i)
                {
                    /* Convert int to string. */
                    chk_int_to_str(&dir_num_ext[0],ext_size,i);
                    for(j = 0; j < ext_size;++j)
                    {
                        if(dir_num_ext[j] == ' ')
                            dir_num_ext[j] = '0';
                    }
                

                    /* Create what we think is the next error directory name. */
                    NUF_Copybuff(&dirname[str_len],&dir_num_ext[0],ext_size);

                    dirname[str_len+ext_size] = '\0';

                    /* Try creating the directory. If it fails that mean that error directory
                       already exist so increment the extension for the directory by 1 and try
                       again. */
                    ret_val = NU_Make_Dir(dirname);
                    if(ret_val == NU_SUCCESS)
                        break;

        
                }
            }
        }
        else
        {
            ret_val = NUF_BADPARM;
        }
    }
    else
    {
        ret_val = NUF_BADPARM;
    }



    return ret_val;

}
/************************************************************************
* FUNCTION
*
*   chk_int_to_str
*
* DESCRIPTION
*
*  
*   Takes a number and converts it to a string representation.
*   NOTE: The buffer that string points to must be at least
*   CHK_MAX_DIGIT_IDX_IN_ERR_NAME big. pstring_size should be
*   greater than 2 digits.
*
* INPUTS
*
*   *pstring(out)               String representation of number.
*   pstring_size(in)            Number of characters in number, 
*                               not including null terminator.
*   number(in)                  The number that is being converted
*                               to a string.
*
* OUTPUTS
*
*   None.
*
*************************************************************************/ 
VOID   chk_int_to_str(CHAR *pstring, INT pstring_size, UINT32 number)
{
    /* Now, build the string, starting at the right hand side. */
    pstring[--pstring_size] = (UNSIGNED_CHAR)0; /* NULL terminated string. */
    do
    {
        pstring[pstring_size--] = (UNSIGNED_CHAR)(0x30 + (number % 10));
        number = number / 10;
    }
    while (number);

    while (pstring_size >= 0)
        pstring[pstring_size--] =  ' '; /* Clear the of the string. */

}

/************************************************************************
* FUNCTION
*
*   chk_traverse_all_dir_paths
*
* DESCRIPTION
*
*   This function does a pre-order traversal of the directory hierarchy 
*   on the disk.
*
*   Notes: FAT12/16 both have static root directories. They do not use
*          cluster numbers for the root directory, but instead
*          just use the sector numbers.
*          FAT32 root directory is a dynamic linked list of clusters,
*          it will grow if needed.
*          
*
* INPUTS
*  
*   dh(in)                      Disk Handle
*   *pfunc_ptr(in)              Function pointer used to execute 
*                               a specific operations per directory
*                               record entry.
*   b_check_chk_dir(in)         Boolean used to indicate if function
*                               should traverse error contents created.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  Directory traversal was successful.
*
*************************************************************************/
STATUS chk_traverse_all_dir_paths(UINT16 dh,STATUS (*pfunc_ptr)(DIR_REC_OBJ *pdir_rec_obj),UINT8 skip_lfn_entries, INT8 b_check_chk_dir)
{
    STATUS ret_val;    
    DIR_REC_LOC_STACK stack;
    DIR_REC_LOC curr_dir_rec;
    DIR_REC_LOC child_dir_rec;        
    UINT8 buff[CHKDSK_SECTOR_SIZE];
    UINT8 exit_all_loops = NU_FALSE;
    UINT8 end_of_sector = NU_FALSE;    
    UINT32 curr_sector = 0;
    UINT32 sec_offset;
    UINT32 end_sector;
    UINT16 i;
    DOSINODE *dos_rec;
    /* For FAT32 these will be referencing clusters. 
       For FAT12/16 these will be referencing sector numbers. */
    UINT32 curr_root_dir_pos;
    UINT32 end_root_dir_pos;
    UINT8 new_dir_found = NU_FALSE;
    DIR_REC_OBJ dir_obj;
    UINT8 new_lfn = NU_TRUE;
    DDRIVE *pddrive;
    FAT_CB *pfs_cb = NU_NULL;
    UINT32 cl_value, nxt_cl_value;
    UINT32 current_cl;
    UINT32 eoc_marker;
    
#if (CHKDSK_DEBUG == NU_TRUE)
    CHAR cl_num_str[13];
#endif

    /* Convert disk handle to a drive number */
    ret_val = fsdh_get_fs_specific(dh, (VOID**)&pfs_cb);
    if((ret_val == NU_SUCCESS) &&  (pfs_cb != NU_NULL))
    {        
        pddrive = pfs_cb->ddrive;
        eoc_marker = chk_get_eoc_value_for_drive(pddrive);
#if (FR4_T3)
        /* =============SETUP TEST REMOVE WHEN DONE===========*/
        create_FR4_T3_test(pddrive);
#endif

        /* Fill out our dir_obj. */
        dir_obj.dh = dh;
        dir_obj.disk_updated = NU_FALSE;
        dir_obj.pdos_rec_loc = &curr_dir_rec;        

        /* Determine where the root directory starts. */        
        if(CHK_IS_DRIVE_FAT32(pddrive) == NU_TRUE)
        {
            curr_root_dir_pos = pddrive->rootdirstartcl;
            PC_FAT_ENTER(dh)
            ret_val = pc_faxx(pddrive, curr_root_dir_pos, &end_root_dir_pos);            
            PC_FAT_EXIT(dh)
            
            if(ret_val == NU_SUCCESS)
            {
                /* Make sure the value is valid. */
                ret_val = chk_is_cl_valid(pddrive,end_root_dir_pos);
            }
            
        }
        else
        {
            /* For FAT12/16 the root directory doesn't have a cluster number,
               it's start location is equal to: 
               Start of Partition + # of Reserved Sectors + # of Sectors Per FAT * 2 */
            curr_root_dir_pos = curr_sector = pddrive->rootblock;            
            end_root_dir_pos =  pddrive->firstclblock;
            
        }   

        if(ret_val == NU_SUCCESS)    
        {                        
            /* Initialize our stack, the stack allows us to do
               a pre-order traversal on the directory hierarchy. */
            ret_val = chk_stack_init(&stack, (MAX_SIZE_OF_LFN_NAME*sizeof(DIR_REC_LOC)));        
        }

#if CHKDSK_DEBUG
        PRINTF("STARTING DIRECTORY TRAVERSAL", &port);    
        PRINTF("\n\r", &port);
#endif
        /*
        **************Loop through the Root directory
        */
        while((curr_root_dir_pos != end_root_dir_pos) && (ret_val == NU_SUCCESS) && (exit_all_loops != NU_TRUE))
        {                   
           /* Setup the root directory information. */
            curr_dir_rec.cl_num = curr_root_dir_pos;            
            curr_dir_rec.offset = 0;

            /* If FAT32 drive find out what our next sector number is. */
            if(CHK_IS_DRIVE_FAT32(pddrive) == NU_TRUE)
            {
                curr_dir_rec.sec_num = pc_cl2sector(pddrive, curr_root_dir_pos);
            }
            else
            {
                /* For FAT12/16 next cluster number in root is the same as the 
                   sector number. */
                curr_dir_rec.sec_num = curr_root_dir_pos;
            }

            /* If this isn't a FAT32 partition, then the root directory 
               values aren't clusters but sector numbers. */
            if(CHK_IS_DRIVE_FAT32(pddrive) == NU_FALSE)
            {
                curr_dir_rec.fat_12_16_root = NU_TRUE;
            }

            ret_val = chk_stack_push(&stack, &curr_dir_rec);

            /*
            **************Loop until our stack is empty
            Stack + While Loop = recursion.
            */
            while((CHK_STACK_ISEMPTY(&stack) == NU_FALSE) && (ret_val == NU_SUCCESS) && (exit_all_loops != NU_TRUE))
            {
                ret_val = chk_stack_pop(&stack, &curr_dir_rec);
#if CHKDSK_DEBUG
                PRINTF("Popping on cl #: ", &port);
                chk_int_to_str(&cl_num_str[0],CHK_MAX_DIGIT_IDX_IN_ERR_NAME,(UNSIGNED)curr_dir_rec.cl_num);
                PRINTF(&cl_num_str[0], &port);
                PRINTF("\n\r", &port);
                NU_Sleep(10);
#endif  
                if(ret_val == NU_SUCCESS)
                {
                    curr_sector = curr_dir_rec.sec_num;  
                    current_cl = curr_dir_rec.cl_num;

                    /* Determine sector start and end positions
                       of sector traversal, depending on whether
                       sector's being traversed are the FAT12/16
                       root directory or just a sector list that belongs
                       to a cluster. */
                    if(curr_dir_rec.fat_12_16_root == NU_TRUE)
                    {
                        sec_offset = curr_sector;
                        end_sector = end_root_dir_pos;
                    }
                    else
                    {
                        sec_offset = pc_sec2index(pddrive, curr_sector) + 1;
                        end_sector = pddrive->secpalloc;
                    }
                        
                    do 
                    {
                       /*
                        **************Loop though the current directory's sectors
                        */
                        while((sec_offset <= end_sector) && (exit_all_loops != NU_TRUE))
                        {

                            /* Read in a sector. */
                            PC_DRIVE_IO_ENTER(pddrive->dh)
                            ret_val = fs_dev_io_proc(pddrive->dh, (UINT32)curr_sector, &buff[0], 1, YES);
                            PC_DRIVE_IO_EXIT(pddrive->dh)

                            /* If we encountered an error reading the sector then exit with
                               error code. */
                            if(ret_val != NU_SUCCESS)
                            {
                                exit_all_loops = NU_TRUE;
                            }
                            /*
                            **************Loop though the current directory record's sector's buffer
                            */
                            for(i = curr_dir_rec.offset; ((i < CHKDSK_SECTOR_SIZE) && (exit_all_loops != NU_TRUE));i+=32)
                            {
                                /* If this is a FAT12/16 drive, the first element in the buffer is zero
                                   and this current directory record is in the root directory. 
                                   Then we are done traversing the root directory, so terminate loops. */
                                if((CHK_IS_DRIVE_FAT32(pddrive) == NU_FALSE) && (buff[0] == 0x00)
                                    && (curr_dir_rec.fat_12_16_root == NU_TRUE))
                                {
                                    exit_all_loops = NU_TRUE;
                                    break;
                        
                                }
                        
                                /* If this is a dot and dot dot entry skip them. */
                                if((i == 0) && (buff[i] == '.') && (buff[i+32] == '.') && (buff[i+33] == '.'))
                                    i += 64;                     

                                /* If we want to look at LFN entries, and one has been found, but
                                   its SFN entry is marked as deleted then this LFN entry is invalid,
                                   so clear out its data and reset a new_lfn. */
                                if((skip_lfn_entries == NU_FALSE) && (new_lfn == NU_FALSE) && (buff[i] == 0xE5))
                                {
                                    NUF_Memfill((VOID*)&dir_obj.lfn_start_loc, sizeof(DIR_REC_LOC), 0x00);
                                    new_lfn = NU_TRUE;
                                    continue;
                                }
                        
                                /* If the entry is a deleted directory record go to the next directory record. */
                                if(buff[i] == 0xE5)
                                    continue;

                                /* We have reached the end of all the valid data in this sector. */
                                if(buff[i] == 0x00)
                                {
                                    end_of_sector = NU_TRUE;                            
                                    break;
                                }

                                /* Update information before calling function pointer operation.*/                        
                                curr_dir_rec.offset = i; 
                                curr_dir_rec.sec_num = curr_sector;
                                curr_dir_rec.cl_num = current_cl;
                        
                                dos_rec = (DOSINODE*) &buff[i];

                                /* Skip error directories. */
                                if(b_check_chk_dir == NU_FALSE)
                                {

                                    if((!NUF_Strncmp((CHAR*)&dos_rec->fext[0], (CHAR*)CHK_FILE_ERROR_EXTENSION, MAX_EXT)) || 
                                       (!NUF_Strncmp((CHAR*)&dos_rec->fext[0], LOG_EXT, MAX_EXT)))
                                    {
#if CHKDSK_DEBUG
                                        PRINTF("skipping .CHK", &port);
#endif
                                        continue;
                                    }
                                }

                                /* Should we skip LFN directory record entries? */                        
                                if((skip_lfn_entries == NU_TRUE) && (dos_rec->fattribute == CHK_LFN_ATTR))
                                {
                                    continue;
                                }
                                else
                                {
                                    if((dos_rec->fattribute == CHK_LFN_ATTR) && (new_lfn == NU_TRUE))
                                    {
                                        /* Update our dir_obj's LFN's start location information. 
                                           This is saved because the directory record check
                                           needs this information. */
                                        dir_obj.lfn_start_loc.cl_num = current_cl;
                                        dir_obj.lfn_start_loc.offset = curr_dir_rec.offset;
                                        dir_obj.lfn_start_loc.sec_num = curr_dir_rec.sec_num;
                                        dir_obj.lfn_start_loc.fat_12_16_root = curr_dir_rec.fat_12_16_root;
                                        new_lfn = NU_FALSE;                            
                                    }
                      
                                }

                                if((pfunc_ptr != NU_NULL) && (dos_rec->fattribute != CHK_LFN_ATTR))
                                {
                                    /* Update our dir_obj before calling the function. */
                                    dir_obj.pdos_rec = dos_rec;                            
                                    ret_val = chk_stack_peek(&stack, &dir_obj.dos_rec_moms_loc);
                            
                                    /* If stack is empty, set parent info to root. */
                                    if(ret_val == NUF_CHK_STACK_EMPTY)
                                    {
                                        /* If stack is empty then we are in root directory, so 
                                           make sure mom's information is zeroed out. */
                                        NUF_Memfill((VOID*)&dir_obj.dos_rec_moms_loc, sizeof(DIR_REC_LOC), 0x00);                                
                                    }
                                    else
                                    {
                                        if(ret_val != NU_SUCCESS)
                                        {
                                            exit_all_loops = NU_TRUE;
                                            break;
                                        }                                
                                    }

                            
                                    /* If this directory record doesn't have a LFN record associated with it then,
                                       then zero the information out before calling the check disk function. */
                                    if((skip_lfn_entries == NU_FALSE) && (new_lfn == NU_TRUE))
                                    {
                                        /* Make sure the LFN information in dir_obj's is cleared out. */
                                        NUF_Memfill((VOID*)&dir_obj.lfn_start_loc, sizeof(DIR_REC_LOC), 0x00);
                                    }


                                    ret_val = pfunc_ptr(&dir_obj);
                                    if(ret_val != NU_SUCCESS)
                                    {
                                        exit_all_loops = NU_TRUE;
                                        break;
                                    }
                                    /* Now that we have found the SFN directory record that goes with the LFN one,
                                       reset our flag so we get the next first LFN entry. */
                                    new_lfn = NU_TRUE;
                            
                                }

                                /* If a disk update occurred we need to update our buffer. */
                                if(dir_obj.disk_updated == NU_TRUE)
                                {
                                     /* Make sure all our buffers are free. */
                                    pc_free_all_blk(pddrive);    
                                
                                    dir_obj.disk_updated = NU_FALSE;
                                    PC_DRIVE_IO_ENTER(pddrive->dh)
                                    ret_val = fs_dev_io_proc(pddrive->dh, (UINT32)curr_sector, &buff[0], 1, YES);
                                    PC_DRIVE_IO_EXIT(pddrive->dh)
                                    if(ret_val != NU_SUCCESS)
                                    {
                                        exit_all_loops = NU_TRUE;
                                        break;
                                    }
                                }


                                /* Make sure the current directory record that the function pointer
                                   examined wasn't deleted, if it was then get the next directory
                                   record entry. */                        
                                if(buff[i] == 0xE5)
                                    continue;                        

                                /* Determine if this directory record entry is a directory. */
                                if(dos_rec->fattribute & ADIRENT)
                                {                                                       
                                    /* Add 32 because when we pop it off the stack we want to start at the
                                       next directory record, not this one. */
                                    curr_dir_rec.offset = i + 32; 
                                    curr_dir_rec.sec_num = curr_sector;                                    
                                    curr_dir_rec.cl_num = current_cl;
                               
                                   
                                    /* Remember where we are in the directory hierarchy by pushing the 
                                       current directory records information on the stack, before the child. */
                                    ret_val = chk_stack_push(&stack, &curr_dir_rec);
                                    if(ret_val != NU_SUCCESS)
                                    {
                                        exit_all_loops = NU_TRUE;
                                        break;
                                    }
                            
#if CHKDSK_DEBUG
                                    PRINTF("Pushing on cl #: ", &port);
                                    chk_int_to_str(&cl_num_str[0],CHK_MAX_DIGIT_IDX_IN_ERR_NAME,(UNSIGNED)curr_dir_rec.cl_num);
                                    PRINTF(&cl_num_str[0], &port);
                                    PRINTF("\n\r", &port);
                                    NU_Sleep(10);
#endif
                                    /* Now setup our child's information and then push the child on the stack. */
                            
                                    /* If we are using FAT32 mask in the high word. */
                                    if(CHK_IS_DRIVE_FAT32(pddrive) == NU_TRUE)
                                    {
                                        child_dir_rec.cl_num = dos_rec->fclusterhigh;
                                        child_dir_rec.cl_num <<= 16;
                                        child_dir_rec.cl_num |= dos_rec->fclusterlow;
                                    }
                                    /* FAT12/16 */
                                    else
                                        child_dir_rec.cl_num = dos_rec->fclusterlow;


                                    child_dir_rec.sec_num = pc_cl2sector(pddrive, child_dir_rec.cl_num);                                 
                                    child_dir_rec.offset = 0;
                                    child_dir_rec.fat_12_16_root = NU_FALSE;

                                    /* Don't allow loop back cluster chains.
                                       This could cause infinite loop. These will
                                       be handled by CLC. */
                                    if(child_dir_rec.sec_num != curr_sector)
                                    {
                                    /* Add the new directory record to our stack. */
                                        ret_val = chk_stack_push(&stack, &child_dir_rec);
                                        if(ret_val != NU_SUCCESS)
                                        {
                                            exit_all_loops = NU_TRUE;
                                            break;
                                        }


#if CHKDSK_DEBUG
                                        PRINTF("Pushing on dir: ", &port);                                    
                                        NUF_Copybuff(&cl_num_str[0], &dos_rec->fname[0], NUF_Get_Str_Len(&dos_rec->fname[0]));
                                        cl_num_str[NUF_Get_Str_Len(&dos_rec->fname[0])] = '/0';
                                        PRINTF(&cl_num_str[0], &port);
                                        PRINTF(" CL: ",&port);
                                        chk_int_to_str(&cl_num_str[0],CHK_MAX_DIGIT_IDX_IN_ERR_NAME,(UNSIGNED)child_dir_rec.cl_num);
                                        PRINTF(&cl_num_str[0], &port);
                                        PRINTF("\n\r", &port);
                                        PRINTF("\n\r", &port);
                                        NU_Sleep(10);
#endif
                                        new_dir_found = NU_TRUE;                            
                                        break;
                                    }
                            
                                }                                               
                       
                            }/* End of loop for buffer traversal. */

                            /* If a new directory is found we want to break out of */
                            if((new_dir_found == NU_TRUE) || (end_of_sector == NU_TRUE))
                            {                                
                                break;
                            }
                            else
                            {
                                ++curr_sector;
                                ++sec_offset;

                                /* Reset our buffer offset. */
                                curr_dir_rec.offset = 0;
                            }

                        } /* End of while loop for sector traversal. */

                        if((new_dir_found == NU_TRUE) || (end_of_sector == NU_TRUE))
                        {   
                            new_dir_found = NU_FALSE;
                            end_of_sector = NU_FALSE;
                            break;
                           
                        }
                        else
                        {
                            if(current_cl != curr_root_dir_pos)                            
                            {                                
                                /* Get the value in the FAT table for current_cl. */
                                ret_val = pc_faxx(pddrive,current_cl,&cl_value);                                
                                if(ret_val == NU_SUCCESS && cl_value)
                                {
                                    /* Verify next cluster value isn't zero,
                                       if it is current cluster is invalid cluster
                                       that will be set to EOC, when DDR is
                                       ran in fix mode. */
                                    (VOID)pc_faxx(pddrive, cl_value, &nxt_cl_value);
                                    if(chk_is_cl_free(pddrive, nxt_cl_value) == NU_TRUE)
                                    {
                                        cl_value = nxt_cl_value;
                                    }

                                    /* Make sure the cl_value is valid. */
                                    ret_val = chk_is_cl_valid(pddrive,cl_value); 									
                                    if((ret_val == NU_SUCCESS) && (cl_value < eoc_marker))
                                    {              
                                        current_cl = cl_value;
                                        curr_dir_rec.offset = 0;
                                        curr_sector = pc_cl2sector(pddrive,cl_value);                           
                                        sec_offset = pc_sec2index(pddrive, curr_sector) + 1;
                                        end_sector = pddrive->secpalloc;
                                    }
                                    else
                                    {
										ret_val = NU_SUCCESS;
                                        break;
                                    }
                                }
                            }
                            else
                                break;
                        }
                    }while(exit_all_loops == NU_FALSE);
                }
            }/* End of while loop for stack. */
    
            /* Because the root directory is treated like a linked list in FAT32,
               we must update our cluster number as we traverse the root directory. */
            if(CHK_IS_DRIVE_FAT32(pddrive) == NU_TRUE)
            {
                curr_root_dir_pos = end_root_dir_pos;

                /* If we are not at the end of our cluster chain then get the next one. */
                if(curr_root_dir_pos < (CHK_FAT32_CL_MASK & CHK_EOC))      
                {
                    PC_FAT_ENTER(dh)
                    
                    ret_val = pc_faxx(pddrive, curr_root_dir_pos, &end_root_dir_pos);
                    if(ret_val != NU_SUCCESS)
                    {
                        break;
                    }
                    else
                    {
                        /* Make sure the cl_value is valid. */
                        ret_val = chk_is_cl_valid(pddrive,end_root_dir_pos);
                        if(ret_val != NU_SUCCESS)
                        {
                            break;
                        }
                    }

                    PC_FAT_EXIT(dh)
                }
            }
            /* FAT 12/16 */
            else
            {
                /* Can simply increment to the next sector number because FAT12 and FAT16 root directory 
                   section is static. */
                if(curr_root_dir_pos <= end_of_sector)
                {
			   	     curr_root_dir_pos = curr_sector + 1;
                }
                else
                {					
					break;
                }
				
            }
        } /* End of while loop for root directory traversal. */

        /* If there was already an error encountered then keep that error in our,
           return value, however still try and remove our stack resources. */
        if(ret_val != NU_SUCCESS)
        {
            /* Free our stack. */
            chk_stack_cleanup(&stack);
        }
        else
        {
            /* Free our stack. */
            ret_val = chk_stack_cleanup(&stack);
        }
    }   
    
    return ret_val;
    
}

/************************************************************************
* FUNCTION
*
*   chk_is_cl_bad
*
* DESCRIPTION
*
*   This function returns a boolean(NU_TRUE or NU_FALSE) to indicate
*   if the cluster passed in is equal to the BAD cluster marker. This
*   function does not test to ensure the cluster is actually BAD.
*   Assumes pddrive was validated by calling function. 
*          
*
* INPUTS
*  
*   *pddrive(in)                Drive Pointer.
*   cl(in)                      Cluster value.
*
* OUTPUTS
*
*   NU_TRUE                     If cl is equal to BAD cluster marker.
*   NU_FALSE                    cl is NOT equal to BAD cluster marker. 
*
*************************************************************************/
UINT8 chk_is_cl_bad(DDRIVE *pddrive, UINT32 cl)
{
    UINT8 ret_val;
    UINT32 cl_mask;

    /* Determine our FAT cluster mask. */    
    cl_mask = chk_get_cl_mask(pddrive);

    if(cl == (cl_mask & CHK_BAD_CL))
    {
        ret_val = NU_TRUE;    
    }
    else
    {
        ret_val = NU_FALSE;
    }
    return (ret_val);

}

/************************************************************************
* FUNCTION
*
*   chk_is_cl_free
*
* DESCRIPTION
*
*   This function returns a boolean(NU_TRUE or NU_FALSE) to indicate
*   if the cluster passed in is equal to the FREE cluster marker.
*   Assumes pddrive was validated by calling function. 
*          
*
* INPUTS
*  
*   *pddrive(in)                Drive Pointer.
*   cl(in)                      Cluster value.
*
* OUTPUTS
*
*   NU_TRUE                     If cl is equal to FREE cluster marker.
*   NU_FALSE                    cl is NOT equal to FREE cluster marker. 
*
*************************************************************************/
UINT8 chk_is_cl_free(DDRIVE *pddrive, UINT32 cl)
{    
    UINT8 ret_val;
	FILE_Unused_Param = (UINT32)pddrive;

    if(cl == CHK_FREE_CL)
    {
        ret_val = NU_TRUE;    
    }
    else
    {
        ret_val = NU_FALSE;
    }

    return (ret_val);
}

/************************************************************************
* FUNCTION
*
*   chk_is_cl_res
*
* DESCRIPTION
*
*   This function returns a boolean(NU_TRUE or NU_FALSE) to indicate
*   if the cluster passed in is equal to the Reserved cluster marker 
*   or value.
*   Assumes pddrive was validated by calling function. 
*          
*
* INPUTS
*  
*   *pddrive(in)                Drive Pointer.
*   cl(in)                      Cluster value.
*
* OUTPUTS
*
*   NU_TRUE                     If cl is equal to Reserved cluster marker 
*                               or value.
*   NU_FALSE                    cl is NOT equal to Reserved cluster marker 
*                               or value. 
*
*************************************************************************/
UINT8 chk_is_cl_res(DDRIVE *pddrive, UINT32 cl)
{
    UINT8 ret_val;
    UINT32 cl_mask;
    
    /* Determine our FAT cluster mask. */    
    cl_mask = chk_get_cl_mask(pddrive);

    if(((cl <= (cl_mask & CHK_RESV_CL_END)) && (cl >= (cl_mask & CHK_RESV_CL_START))) || (cl == CHK_RESV_SPECIAL_CASE))
    {
        ret_val = NU_TRUE;    
    }
    else
    {
        ret_val = NU_FALSE;
    }

    return (ret_val);

}

/************************************************************************
* FUNCTION
*
*   chk_is_cl_range
*
* DESCRIPTION
*
*   This function returns a boolean(NU_TRUE or NU_FALSE) to indicate
*   if the cluster passed in is exceeds the max possible cluster
*   value for pddrive.
*   Assumes pddrive was validated by calling function. 
*          
*
* INPUTS
*  
*   *pddrive(in)                Drive Pointer.
*   cl(in)                      Cluster value.
*
* OUTPUTS
*
*   NU_TRUE                     If cl is greater than max possible 
*                               cluster value. 
*   NU_FALSE                    cl is NOT equal greater than max possible
*                               cluster value. 
*
*************************************************************************/
UINT8 chk_is_cl_range(DDRIVE *pddrive, UINT32 cl)
{
    UINT8 ret_val = NU_FALSE;

    if((chk_is_cl_bad(pddrive, cl) != NU_TRUE) && (chk_is_cl_res(pddrive, cl) != NU_TRUE))
    {        
        if(cl > (pddrive->numsecs / pddrive->secpalloc))
        {
            ret_val = NU_TRUE;    
        }
    }

    return (ret_val);
}

/************************************************************************
* FUNCTION
*
*   chk_get_cl_mask
*
* DESCRIPTION
*
*   This function returns the FAT specific masking value. 
*   Assumes pddrive was validated by calling function. 
*          
*
* INPUTS
*  
*   *pddrive(in)                Drive Pointer.
*
* OUTPUTS
*
*    FAT specific masking value. 
*
*************************************************************************/
UINT32 chk_get_cl_mask(DDRIVE *pddrive)
{
    UINT32 cl_mask = CHK_FAT32_CL_MASK;
    
 	/* Determine our FAT cluster mask. */
    if(CHK_IS_DRIVE_FAT16(pddrive))
    {     
        cl_mask = CHK_FAT16_CL_MASK;
    }

    if(CHK_IS_DRIVE_FAT12(pddrive))
    {
        cl_mask = CHK_FAT12_CL_MASK;
    }

    return (cl_mask);
    
}

/************************************************************************
* FUNCTION
*
*   chk_get_chkdsk_lock
*
* DESCRIPTION
*
*   This function gets the check disk utility lock. This lock prevents
*   other task from calling this API until it is complete. 
*          
*
* INPUTS
*  
*   None
*
* OUTPUTS
*
*    NU_SUCCESS                               Lock was acquired.
*    Other                                    See NU_Obtain_Semaphore
*                                             error codes.
*
*************************************************************************/
STATUS chk_get_chkdsk_lock(VOID)
{
    STATUS ret_val;

    ret_val = NU_Obtain_Semaphore(&NUF_CHKDSK_MUTEX, CHKDSK_MUTEX_WAIT_TIME);

    return (ret_val);
}

/************************************************************************
* FUNCTION
*
*   chk_release_chkdsk_lock
*
* DESCRIPTION
*
*   This function releases the check disk utility lock. This lock prevents
*   other task from calling this API until it is complete. 
*          
*
* INPUTS
*  
*   None
*
* OUTPUTS
*
*    NU_SUCCESS                               Lock was released. 
*    Other                                    See NU_Release_Semaphore
*                                             error codes.
*
*************************************************************************/
STATUS chk_release_chkdsk_lock(VOID)
{
    STATUS ret_val;
    
    ret_val = NU_Release_Semaphore(&NUF_CHKDSK_MUTEX);

    return (ret_val);    
}


#endif /* (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1) */

